/*
 * ProxiedNetwork.hpp - Proxy-enabled version of socketmanager
 * $Id: ProxiedNetwork.hpp 38 2008-03-05 23:24:34Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007,2008 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *              and David R. Piegdon <mwcollect@p23q.org>
 */

#ifndef __INCLUDE_libnetworkd_ProxiedNetwork_hpp
#define __INCLUDE_libnetworkd_ProxiedNetwork_hpp

#include <libnetworkd/Network.hpp>


#include <list>
#include <tr1/unordered_map>


using namespace std;
using namespace std::tr1;


namespace libnetworkd
{



struct ProxyAddress	
{
	uint32_t host;
	uint16_t port;
	string		user;
	string		password;
};



// proxy-enabled version of NetworkManager:
class ProxiedNetworkManager : public NetworkManager
{
public:
	ProxiedNetworkManager() { m_currentSet = 0; }
	virtual ~ProxiedNetworkManager();

	virtual bool addProxy(int set, string proxy);
	virtual void activateSet(int set);
	
	virtual void clearProxies(void);
	virtual bool usesProxies(void);

	virtual NetworkSocket * connectStream(const NetworkNode * remoteNode, NetworkEndpoint * localEndpoint, bool proxy = true);


protected:
	virtual struct ProxyAddress getNextProxy(void);


private:

	class ProxySet
	{
	public:
		ProxySet& operator=(ProxySet const& other)
		{
			proxies = other.proxies;
			nextProxy = proxies.end();
			
			return * this;
		}
		
		list<ProxyAddress> proxies;
		list<ProxyAddress>::iterator nextProxy;
	};
	
	typedef unordered_map<int, ProxySet> ProxyPool;
	
	ProxyPool m_proxyPool;
	
	int m_currentSet;
};


enum proxyConnectionStatus_e {
	PROXY_NONE,		// nothing (not using proxies or pre-init)
	PROXY_WAIT_VID,		// waiting for VID from proxy

	PROXY_WAIT_USERAUTH,	// waiting for plain USER-AUTH ACK from proxy

	PROXY_SEND_CONN,	// sending CONNECT command
	PROXY_WAIT_CONN,	// waiting for CONNECT OK

	PROXY_DONE		// all done.
};


// proxy-enabled version of TcpSocket
class ProxiedTcpSocket : public TcpSocket, public NetworkEndpoint
{
public:
	// TcpSocket functionality:
	ProxiedTcpSocket(void);
	ProxiedTcpSocket(IOManager * ioManager, NetworkEndpoint * clientEndpoint, struct ProxyAddress proxy);
	virtual ~ProxiedTcpSocket(void);

	virtual bool connect(struct sockaddr_in * remoteHost);

	// NetworkEndpoint functionality: (for proxy negotiation)
	virtual void dataRead(const char * buffer, uint32_t dataLength);
	virtual void dataSent(uint32_t length) { };
	virtual void connectionEstablished(NetworkNode * remoteNode, NetworkNode * localNode);
	virtual void connectionClosed(void);
	virtual void connectionLost(void);

protected:
	virtual void pivotEndpoints(NetworkNode * proxyNode, char * buffer, uint32_t dataLength);

private:
	// using a proxy for this connection?
	bool m_useProxy;
	// proxy address
	struct ProxyAddress m_proxy;
	// status of proxy negotiation
	enum proxyConnectionStatus_e m_proxyStatus;
	// I/O buffer for endpoint
	char *m_buffer;
	uint32_t m_bufferLength;

	// final address of connection
	struct sockaddr_in m_remoteHost;
	// final endpoint
	NetworkEndpoint	* m_finalClientEndpoint;
};

} // end namespace libnetworkd

#endif // __INCLUDE_libnetworkd_ProxiedNetwork_hpp

