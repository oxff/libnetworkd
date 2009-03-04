/*
 * TcpSocket.cpp - IOSocket implementation for TCP Berkley sockets
 * $Id: TcpSocket.cpp 44 2008-09-29 22:48:10Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <libnetworkd/Network.hpp>
#include <libnetworkd/LogManager.hpp>

namespace libnetworkd
{


TcpSocket::TcpSocket()
{
	m_socket = -1;
	m_serverEndpointFactory = 0;
	m_clientEndpoint = 0;
	m_state = NETSOCKSTATE_UNINITIALIZED;
	m_serverSocket = false;
	m_ioManager = 0;
}

TcpSocket::TcpSocket(IOManager * ioManager, NetworkEndpoint * clientEndpoint)
{
	m_ioManager = ioManager;
	m_socket = -1;
	m_clientEndpoint = clientEndpoint;
	m_serverEndpointFactory = 0;
	m_state = NETSOCKSTATE_UNINITIALIZED;
	m_serverSocket = false;
}

TcpSocket::TcpSocket(IOManager * ioManager, NetworkEndpointFactory * serverEndpointFactory)
{
	m_state = NETSOCKSTATE_UNINITIALIZED;

	m_ioManager = ioManager;
	m_socket = -1;
	m_serverEndpointFactory = serverEndpointFactory;
	m_serverSocket = false;
	m_clientEndpoint = 0;
}

TcpSocket::TcpSocket(IOManager * ioManager, int existingSocket, NetworkEndpointFactory * factory, struct sockaddr_in * remoteAddress)
{
	NetworkNode remoteNode, localNode;
	m_ioManager = ioManager;
	m_socket = existingSocket;
	m_serverEndpointFactory = factory;
	m_clientEndpoint = factory->createEndpoint(this);
	m_serverSocket = false;

	{
		m_ioManager->addSocket(this, m_socket);
		m_ioSocketState = IOSOCKSTAT_IDLE;

		m_state = NETSOCKSTATE_IDLE;
	}


	{
		remoteNode.name = inet_ntoa(remoteAddress->sin_addr);
		remoteNode.port = ntohs(remoteAddress->sin_port);
	}

	{
		struct sockaddr_in localAddress;
		socklen_t len = sizeof(localAddress);

		if(getsockname(existingSocket, (struct sockaddr *) &localAddress, &len) < 0)
			localNode.port = 0;
		else
		{
			localNode.name = inet_ntoa(localAddress.sin_addr);
			localNode.port = ntohs(localAddress.sin_port);
		}
	}

	m_clientEndpoint->connectionEstablished(&remoteNode, &localNode);
}

TcpSocket::~TcpSocket()
{
	if(m_socket > 0)
		close();

	if(!m_serverSocket && m_clientEndpoint && m_serverEndpointFactory)
		m_serverEndpointFactory->destroyEndpoint(m_clientEndpoint);
}


bool TcpSocket::socket()
{
	int trueval = 1;

	if(m_socket >= 0 || m_state != NETSOCKSTATE_UNINITIALIZED)
		return false;

	m_socket = ::socket(AF_INET, SOCK_STREAM, 0);

	if(m_socket < 0)
		return false;

	if(setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &trueval, sizeof(trueval)) < 0)
	{
		::close(m_socket);
		return false;
	}

	if(fcntl(m_socket, F_SETFL, fcntl(m_socket, F_GETFL) | O_NONBLOCK) != 0)
	{
		::close(m_socket);
		return false;
	}

	return true;
}

bool TcpSocket::connect(struct sockaddr_in * remoteHost)
{
	if(m_state != NETSOCKSTATE_UNINITIALIZED)
		return false;

	if(m_socket == -1)
		if(!socket())
		{
			m_socket = -1;
			return false;
		}

	if(::connect(m_socket, (struct sockaddr *) remoteHost, sizeof(struct sockaddr)) == 0)
	{
		m_ioManager->addSocket(this, m_socket);
		m_ioSocketState = IOSOCKSTAT_IDLE;

		m_state = NETSOCKSTATE_IDLE;
		m_clientEndpoint->connectionEstablished(0, 0); // TODO give remote & local info

		return true;
	}
	else if(errno == EINPROGRESS)
	{
		m_ioManager->addSocket(this, m_socket);
		m_ioSocketState = IOSOCKSTAT_BUFFERING;

		m_state = NETSOCKSTATE_GOING_UP;

		return true;
	}

	perror(__PRETTY_FUNCTION__);
	return false;
}

bool TcpSocket::bind(struct sockaddr_in * localAddress)
{
	if(!socket())
		return false;

	return (::bind(m_socket, (struct sockaddr *) localAddress, sizeof(struct sockaddr)) == 0);
}

bool TcpSocket::listen(uint8_t backlog)
{
	if(::listen(m_socket, backlog) != -1)
	{
		m_ioManager->addSocket(this, m_socket);
		m_ioSocketState = IOSOCKSTAT_IDLE;

		m_state = NETSOCKSTATE_IDLE;
		m_serverSocket = true;

		return true;
	}

	return false;
}

bool TcpSocket::close(bool force)
{
	if(m_socket == -1)
		return true;

	if(m_state == NETSOCKSTATE_BUFFERING && !force)
	{
		m_state = NETSOCKSTATE_GOING_DOWN;
		return false;
	}

	if(m_ioManager)
		m_ioManager->removeSocket(this);

	::close(m_socket);

	m_socket = -1;
	m_state = NETSOCKSTATE_DOWN;

	if(m_clientEndpoint)
	{
		if(m_state == NETSOCKSTATE_BUFFERING)
			m_clientEndpoint->connectionLost();
		else
			m_clientEndpoint->connectionClosed();
	}

	return true;
}


NetworkSocketState TcpSocket::getState()
{
	return m_state;
}


void TcpSocket::send(const char * buffer, uint32_t length)
{
	if(m_state == NETSOCKSTATE_IDLE)
	{
		int sent;

		if((uint32_t) (sent = ::send(m_socket, buffer, length, MSG_NOSIGNAL)) == length)
			return;

		if(sent <= 0)
		{
			if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
			{
				m_state = NETSOCKSTATE_BUFFERING;
				m_ioSocketState = IOSOCKSTAT_BUFFERING;

				m_outputBuffer.append(buffer, length);
			}
			
			// TODO: check if not freeing data here creates a stale socket, actually, we should get some POLLERR but you never know
		}
		else
		{
			m_state = NETSOCKSTATE_BUFFERING;
			m_ioSocketState = IOSOCKSTAT_BUFFERING;

			m_outputBuffer.append(buffer + sent, length - sent);
		}
	}
	else if(m_state == NETSOCKSTATE_BUFFERING || m_state == NETSOCKSTATE_GOING_UP)
	{
		m_outputBuffer.append(buffer, length);
	}
}


void TcpSocket::pollRead()
{
	static char buffer[4096];

	ASSERT(m_state == NETSOCKSTATE_IDLE || m_state == NETSOCKSTATE_BUFFERING || m_state == NETSOCKSTATE_GOING_UP);

	if(m_serverSocket)
	{ // server
		struct sockaddr_in clientAddress;
		socklen_t clientLen = sizeof(clientAddress);
		int clientSocket = accept(m_socket, (struct sockaddr *) &clientAddress, &clientLen);

		if(clientSocket > 0)
		{
			new TcpSocket(m_ioManager, clientSocket, m_serverEndpointFactory, &clientAddress);
		}
	}
	else
	{ // client
		ASSERT(m_state == NETSOCKSTATE_IDLE || m_state == NETSOCKSTATE_BUFFERING);

		{
			int read = ::recv(m_socket, buffer, sizeof(buffer), 0);

			if(read <= 0)
			{
				::close(m_socket);
				m_socket = -1;

				if(!read && m_state == NETSOCKSTATE_IDLE)
				{
					ASSERT(m_outputBuffer.empty());
					m_clientEndpoint->connectionClosed();
				}
				else
					m_clientEndpoint->connectionLost();

				m_ioManager->removeSocket(this);
				delete this;

				return;
			}

			m_clientEndpoint->dataRead(buffer, read);
		}
	}
}

void TcpSocket::pollWrite()
{
	int sent;

	if(m_state == NETSOCKSTATE_GOING_UP)
	{
		if(!m_outputBuffer.empty())
		{
			m_state = NETSOCKSTATE_BUFFERING;
			m_ioSocketState = IOSOCKSTAT_BUFFERING;
		}
		else
		{
			m_state = NETSOCKSTATE_IDLE;
			m_ioSocketState = IOSOCKSTAT_IDLE;
		}

		// TODO provide local and remote node information
		m_clientEndpoint->connectionEstablished(0, 0);
		return;
	}

	ASSERT(m_state == NETSOCKSTATE_BUFFERING || m_state == NETSOCKSTATE_GOING_DOWN);

	sent = ::send(m_socket, m_outputBuffer.data(), m_outputBuffer.size(), MSG_NOSIGNAL);

	if(sent <= 0)
	{
		if(errno == EWOULDBLOCK || errno == EAGAIN)
			return;

		::close(m_socket);
		m_socket = -1;

		m_clientEndpoint->connectionLost();

		m_ioManager->removeSocket(this);
		delete this;

		return;
	}

	m_outputBuffer.erase(0, sent);

	if(m_outputBuffer.empty())
	{
		if(m_state == NETSOCKSTATE_GOING_DOWN)
		{
			m_state = NETSOCKSTATE_DOWN;
			m_ioManager->removeSocket(this);
			::close(m_socket);

			m_clientEndpoint->connectionClosed();
			delete this;
		}
		else
		{
			m_state = NETSOCKSTATE_IDLE;
			m_ioSocketState = IOSOCKSTAT_IDLE;
		}
	}
}

void TcpSocket::pollError()
{
	::close(m_socket);
	m_socket = -1;

	m_clientEndpoint->connectionLost();

	m_ioManager->removeSocket(this);
	delete this;
}


}
