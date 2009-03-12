/*
 * ProxiedTcpSocket.cpp - Proxy-enabled version of TcpSocket
 * $Id: ProxiedTcpSocket.cpp 43 2008-06-16 22:06:22Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007,2008 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *              and David R. Piegdon <mwcollect@p23q.org>
 */

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include <libnetworkd/Network.hpp>
#include <libnetworkd/LogManager.hpp>
#include <libnetworkd/ProxiedNetwork.hpp>

// {{{ socks5 protocol structures
struct socks5_vid {
	uint8_t version; // == 5!
	uint8_t nMethods; // here == 2!
	uint8_t method1;
	uint8_t method2;
} __attribute__((__packed__));

struct socks5_vidResponse {
	uint8_t version; // == 5!
	uint8_t method;
} __attribute__((__packed__));

struct socks5_rq {
	uint8_t version; // == 5!
	uint8_t command;
	uint8_t rsv; // == 0!
	uint8_t addressType; // XXX: only IPv4 (1) supported for now
	uint32_t ipAddress;
	uint16_t port;
} __attribute__((__packed__));

struct socks5_rqResponse {
	uint8_t version; // == 5!
	uint8_t reply;
	uint8_t rsv; // == 0!
	uint8_t addressType;
	uint32_t bndAddress;
	uint16_t bndPort;
} __attribute__((__packed__));

struct socks5_userauthResponse {
	uint8_t version; // == 1!
	uint8_t status;
} __attribute__((__packed__));

// use socks5_userauthResponse->status as index
static const char *socks5_connect_errorlist[] = {
	"succeeded",
	"general SOCKS server failure",
	"connection not allowed by ruleset",
	"Network unreachable",
	"Host unreachable",
	"Connection refused",
	"TTL expired",
	"Command not supported",
	"Address type not supported"
};
// }}}


namespace libnetworkd
{


ProxiedTcpSocket::ProxiedTcpSocket()
{
	m_useProxy = false;
	m_proxyStatus = PROXY_NONE;
	m_buffer = NULL;
	m_bufferLength = 0;

	m_clientEndpoint = NULL;
	m_finalClientEndpoint = NULL;
	m_serverEndpointFactory = NULL;

	m_socket = -1;
	m_state = NETSOCKSTATE_UNINITIALIZED;
	m_serverSocket = false;
	m_ioManager = 0;

}

ProxiedTcpSocket::ProxiedTcpSocket(IOManager * ioManager, NetworkEndpoint * clientEndpoint, struct ProxyAddress proxy)
{
	m_useProxy = true;
	m_proxy = proxy;
	m_proxyStatus = PROXY_NONE;
	m_buffer = NULL;
	m_bufferLength = 0;

	m_finalClientEndpoint = clientEndpoint;
	m_clientEndpoint = this;
	m_serverEndpointFactory = NULL;

	m_socket = -1;
	m_state = NETSOCKSTATE_UNINITIALIZED;
	m_serverSocket = false;
	m_ioManager = ioManager;
}

ProxiedTcpSocket::~ProxiedTcpSocket(void)
{
	if(m_buffer)
		free(m_buffer);
}

bool ProxiedTcpSocket::connect(struct sockaddr_in * remoteHost)
{
	if(m_state != NETSOCKSTATE_UNINITIALIZED)
		return false;
		
	struct sockaddr_in sin;
	
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = m_proxy.host;
	sin.sin_port = m_proxy.port;

	if(m_useProxy) {
		m_remoteHost = *remoteHost;
		TcpSocket::connect( &sin );
	} else {
		return TcpSocket::connect( remoteHost );
	}
}

void ProxiedTcpSocket::pivotEndpoints(NetworkNode * proxyNode, char * buffer, uint32_t dataLength)
{
	NetworkNode remoteNode;

	if(m_finalClientEndpoint) {
		remoteNode.name = inet_ntoa(m_remoteHost.sin_addr);
		remoteNode.port = ntohs(m_remoteHost.sin_port);
		m_clientEndpoint = m_finalClientEndpoint;
		m_finalClientEndpoint = NULL;

		if(m_clientEndpoint) {
			m_clientEndpoint->connectionEstablished( &remoteNode, proxyNode);
			if(dataLength > 0)
				m_clientEndpoint->dataRead(buffer, dataLength);
		}
	}
}

void ProxiedTcpSocket::dataRead(const char * buffer, uint32_t dataLength)
{
	// append read data to buffer
	if(m_buffer == NULL)
		m_buffer = (char*)malloc(dataLength);
	else
		m_buffer = (char*)realloc(m_buffer, m_bufferLength + dataLength);

	memcpy(m_buffer + m_bufferLength, buffer, dataLength);
	m_bufferLength += dataLength;


	switch (m_proxyStatus) {
		case PROXY_WAIT_VID:
			if(m_bufferLength == sizeof(struct socks5_vidResponse)) {
				struct socks5_vidResponse *vidR;

				vidR = (struct socks5_vidResponse*)m_buffer;
				if(vidR->version != 5) {
					printf("socks5 version error in socks5_vidResponse (%d != canonical 5)\n", vidR->version);
					close(true);
					return;
				}
				switch(vidR->method) {
					case 0:
						// no authentication
						m_proxyStatus = PROXY_SEND_CONN;
						break;
					case 2:
						// user/password
						//printf( "authenticating with \"%s:%s\"\n",
						//		m_proxy.user.c_str(),
						//		m_proxy.password.c_str() );
						int ulen, plen;

						ulen = m_proxy.user.length();
						plen = m_proxy.password.length();
						if(ulen == 0 || plen == 1) {
							close(true);
							return;
						} else {
							char * buf;
							buf = (char*)malloc(ulen + plen + 3);

							// version
							buf[0] = 1;
							// user-length
							buf[1] = ulen;
							// user
							memcpy(buf+2, m_proxy.user.c_str(), ulen);
							// password-length
							buf[ulen+2] = plen;
							// password
							memcpy(buf+ulen+3, m_proxy.password.c_str(), plen);

							send(buf, ulen + plen + 3);
							free( buf );
							m_proxyStatus = PROXY_WAIT_USERAUTH;
						};
						break;
					default:
						// unknown/unsupported
						close(true);
						return;
				}
				free(m_buffer);
				m_buffer = NULL;
				m_bufferLength = 0;
			} else {
				// simply wait for rest of data
				if(m_bufferLength > sizeof(struct socks5_vid)) {
					// bad socks server?!
					close(true);
					return;
				}
			}
			break;

		case PROXY_WAIT_USERAUTH:
			if(m_bufferLength == sizeof(struct socks5_userauthResponse)) {
				struct socks5_userauthResponse *uaR;
				uaR = (struct socks5_userauthResponse*)buffer;
				if(uaR->version != 1) {
					close(true);
					return;
				}
				if(uaR->status != 0) {
					close(true);
					return;
				} else {
					// auth'd successfully. now send connect command
					m_proxyStatus = PROXY_SEND_CONN;
				}
			} else {
				if(m_bufferLength > sizeof(struct socks5_userauthResponse)) {
					close(true);
					return;
				}
			}
			free(m_buffer);
			m_buffer = NULL;
			m_bufferLength = 0;
			break;

		case PROXY_WAIT_CONN:
			if(m_bufferLength >= sizeof(struct socks5_rqResponse)) {
				struct socks5_rqResponse *rqR;
				struct in_addr adr;
				NetworkNode localNode;

				rqR = (struct socks5_rqResponse*)m_buffer;

				if(rqR->version != 5) {
					close(true);
					return;
				}
				if(rqR->reply != 0) {
					close(true);
					return;
				}

				adr.s_addr = rqR->bndAddress;
				localNode.name = inet_ntoa(adr);
				localNode.port = ntohs(rqR->bndPort);

				pivotEndpoints(&localNode, m_buffer + sizeof(struct socks5_rqResponse),
						m_bufferLength - sizeof(struct socks5_rqResponse));

				free(m_buffer);
				m_buffer = NULL;
				m_bufferLength = 0;
				return;
			} else {
				// simply wait for rest of data
				// special case: if more than sizeof(struct socks5_rqResponse):
				//   this will be data from the new connection and needs to be
				//   relayed to the final endpoint
			}
			break;
		default:
			printf("socks5: internal error: bad m_proxyStatus\n");
			close(true);
			return;
	}

	if( m_proxyStatus == PROXY_SEND_CONN ) {
		struct socks5_rq rq;
		rq.version = 5;
		rq.command = 1;
		rq.rsv = 0;
		rq.addressType = 1;
		rq.ipAddress = m_remoteHost.sin_addr.s_addr;
		// m_remoteHost.sin_port is already in network byte order
		rq.port = m_remoteHost.sin_port;
		send((char*)&rq, sizeof(rq));
		m_proxyStatus = PROXY_WAIT_CONN;
	}
}

void ProxiedTcpSocket::connectionEstablished(NetworkNode * remoteNode, NetworkNode * localNode)
{	
	if( m_proxyStatus == PROXY_NONE ) {
		struct socks5_vid vid;
		vid.version = 5;
		vid.nMethods = 2;
		vid.method1 = 0; // no auth
		vid.method2 = 2; // user+pass
		send((char*) &vid, sizeof(struct socks5_vid));
		m_proxyStatus = PROXY_WAIT_VID;
	} else {
		close(true);
		return;
	};
}

void ProxiedTcpSocket::connectionClosed(void)
{
	if(m_finalClientEndpoint)
		m_finalClientEndpoint->connectionClosed();

	delete this;
}

void ProxiedTcpSocket::connectionLost(void)
{
	if(m_finalClientEndpoint)
		m_finalClientEndpoint->connectionLost();

	delete this;
}


} // end namespace libnetworkd

// vim: fdm=marker
