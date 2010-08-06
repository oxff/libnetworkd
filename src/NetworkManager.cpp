/*
 * NetworkManager.cpp - creation and management of network connections
 * $Id: NetworkManager.cpp 44 2008-09-29 22:48:10Z oxff $
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

NetworkManager::~NetworkManager()
{
}

NetworkSocket * NetworkManager::connectStream(const NetworkNode * remoteNode, NetworkEndpoint * localEndpoint)
{
	struct sockaddr_in address;
	TcpSocket * socket;
	
	address.sin_family = AF_INET;
	address.sin_port = htons(remoteNode->port);
	
	if(inet_aton(remoteNode->name.c_str(), &address.sin_addr) == 0)
		return 0;
	
	socket = new TcpSocket(this, localEndpoint);	
	
	if(!socket->connect(&address))
	{
		socket->close(true);
		
		delete socket;
		return 0;
	}
	
	return socket;
}

NetworkSocket * NetworkManager::serverStream(const NetworkNode * localNode, NetworkEndpointFactory * factory, uint8_t backlog)
{
	struct sockaddr_in localAddress;
	TcpSocket * socket;
	
	localAddress.sin_family = AF_INET;
	localAddress.sin_port = htons(localNode->port);
	
	if(localNode->name == "any")
		localAddress.sin_addr.s_addr = INADDR_ANY;
	else
	{
		if(inet_aton(localNode->name.c_str(), &localAddress.sin_addr) == 0)
			return 0;
	}
	
	socket = new TcpSocket(this, factory);	
	
	if(!socket->bind(&localAddress) || !socket->listen(backlog))
	{
		socket->close(true);
		
		return 0;
	}
	
	return socket;
}

bool NetworkManager::closeStream(NetworkSocket * tcpSocket, bool force)
{
	return tcpSocket->close(force);
}



NetworkSocket * NetworkManager::connectUnix(const char * path, NetworkEndpoint * localEndpoint)
{
	UnixSocket * socket;
	
	socket = new UnixSocket(this, localEndpoint);
	
	if(!socket->connect(path))
	{
		socket->close(true);
		
		delete socket;
		return 0;
	}
	
	return socket;
}

NetworkSocket * NetworkManager::serverUnix(const char * path, NetworkEndpointFactory * factory, uint8_t backlog)
{
	UnixSocket * socket = new UnixSocket(this, factory);
	
	if(!socket->bind(path) || !socket->listen(backlog))
	{
		socket->close(true);
		
		delete socket;
		return 0;
	}
	
	return socket;
}



NetworkSocket * NetworkManager::connectDatagram(const NetworkNode * remoteNode, NetworkEndpoint * localEndpoint, const NetworkNode * localNode )
{
	// TODO FIXME: implement!
	return 0;
}

NetworkSocket * NetworkManager::serverDatagram(const NetworkNode * localNode, NetworkEndpointFactory * endpointFactory)
{
	// TODO FIXME: implement!
	return 0;
}

bool NetworkManager::closeDatagram(NetworkSocket * socket, bool force)
{
	return socket->close(force);
}


}
