/*
 * ProxiedNetworkManager.cpp - Proxy-enabled version of NetworkManager
 * $Id: ProxiedNetworkManager.cpp 38 2008-03-05 23:24:34Z oxff $
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

#include <libnetworkd/Network.hpp>
#include <libnetworkd/LogManager.hpp>
#include <libnetworkd/ProxiedNetwork.hpp>

namespace libnetworkd
{


ProxiedNetworkManager::~ProxiedNetworkManager()
{
	// nil
}

	// NOTE
	// proxy-addresses are assumed in number/dots format "[user:passwort@]IP:port", NOT hostnames.
	// otherwise we will have to use a resolver here.

bool ProxiedNetworkManager::addProxy(int set, string proxy)
{
	struct ProxyAddress address;
	
	int m;
	string prefix, postfix;
	string host, port;

	m = proxy.find_first_of('@',0);
	if(m >= 0) {
		prefix = proxy.substr(0,m);
		postfix = proxy.substr(m+1, proxy.length());

		m = prefix.find_first_of(':',0);
		address.user = prefix.substr(0, m);
		address.password = prefix.substr(m+1, prefix.length());
	} else {
		postfix = proxy;
		address.user = string();
		address.password = string();
	}

	m = postfix.find_first_of(':',0);
	host = postfix.substr(0, m);
	port = postfix.substr(m+1,proxy.length());

	if(inet_aton(host.c_str(), (struct in_addr *) &address.host) == 0)
	{
		return false;
	}
	
	address.port = htons(atoi(port.c_str()));
	
	ProxyPool::iterator it = m_proxyPool.find(set);
	
	if(it == m_proxyPool.end())
	{
		ProxySet s;
		
		s.proxies.push_back(address);
		
		m_proxyPool[set] = s;
	}
	else
		it->second.proxies.push_back(address);
	
	if(set != -1)
		return addProxy(-1, proxy);
		
	return true;
}

void ProxiedNetworkManager::clearProxies(void)
{
	m_proxyPool.clear();
}

bool ProxiedNetworkManager::usesProxies(void)
{
	return !m_proxyPool[m_currentSet].proxies.empty();
}

void ProxiedNetworkManager::activateSet(int set)
{
	if(m_proxyPool.find(set) != m_proxyPool.end())
		m_currentSet = set;
}


struct ProxyAddress ProxiedNetworkManager::getNextProxy()
{
	// just cycle through existing proxies
	struct ProxyAddress next;
	
	if(m_proxyPool[m_currentSet].nextProxy ==
		m_proxyPool[m_currentSet].proxies.end())
	{
		m_proxyPool[m_currentSet].nextProxy = m_proxyPool[m_currentSet]
			.proxies.begin();
	}

	next = * m_proxyPool[m_currentSet].nextProxy;
	
	++m_proxyPool[m_currentSet].nextProxy;

	return next;
}


NetworkSocket * ProxiedNetworkManager::connectStream(const NetworkNode * remoteNode, NetworkEndpoint * localEndpoint, bool proxy)
{
	if( proxy && usesProxies() )
	{
		struct sockaddr_in address;
		ProxiedTcpSocket * socket;

		address.sin_family = AF_INET;
		address.sin_port = htons(remoteNode->port);

		if(inet_aton(remoteNode->name.c_str(), &address.sin_addr) == 0)
			return 0;

		socket = new ProxiedTcpSocket( this, localEndpoint, getNextProxy() );
		
		if(!socket->connect(&address))
		{
			socket->close(true);

			delete socket;
			return 0;
		}

		return socket;
	}
	else 
	{
		return NetworkManager::connectStream(remoteNode, localEndpoint);
	}
}


} // end namespace libnetworkd

// vim: fdm=marker
