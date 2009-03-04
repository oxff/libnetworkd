/*
 * UdpSocket.cpp - Implementation of IOSocket for UDP based POSIX network sockets.
 * $Id: UdpSocket.cpp 3 2007-11-02 16:15:39Z oxff $
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


UdpSocketWrapper::~UdpSocketWrapper()
{
}

void UdpSocketWrapper::send(const char * buffer, uint32_t length)
{
	m_parent->sendTo(buffer, length, m_node);
}

bool UdpSocketWrapper::close(bool force)
{
	return m_parent->dropEndpoint(this, m_node);
}

NetworkSocketState UdpSocketWrapper::getState()
{
	return m_parent->getState();
}


UdpSocket::UdpSocket(IOManager * ioManager, NetworkEndpointFactory * endpointFactory)
{
	m_ioManager = ioManager;
	m_serverEndpointFactory = endpointFactory;
	
	m_socket = -1;
	m_state = NETSOCKSTATE_UNINITIALIZED;
}

UdpSocket::~UdpSocket()
{
}

bool UdpSocket::socket()
{	
	if(m_socket >= 0 || m_state != NETSOCKSTATE_UNINITIALIZED)
		return false;

	m_socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	
	if(m_socket < 0)
		return false;
		
	// TODO: check how this behaves with udp sockets
	// if(setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &trueval, sizeof(trueval)) < 0)
	//	return false;
		
	return (fcntl(m_socket, F_SETFL, fcntl(m_socket, F_GETFL) | O_NONBLOCK) == 0);
}

UdpSocketWrapper * UdpSocket::addEndpoint(const NetworkNode& networkNode, NetworkEndpoint * endpoint)
{
	UdpSocketWrapper * wrapper = new UdpSocketWrapper(this, networkNode);
	
	m_clientEndpoints[networkNode] = std::pair<UdpSocketWrapper *, NetworkEndpoint *>(wrapper, endpoint);
	return wrapper;
}

bool UdpSocket::dropEndpoint(UdpSocketWrapper * wrapper, NetworkNode& remoteNode)
{
	map<NetworkNode, pair<UdpSocketWrapper *, NetworkEndpoint *>, NetworkNode>::iterator it;
	
	if((it = m_clientEndpoints.find(remoteNode)) == m_clientEndpoints.end())
		return false;
	
	if(wrapper && wrapper != it->second.first)
		return false;
	
	it->second.second->connectionClosed();
	delete it->second.first;
	m_clientEndpoints.erase(it);
	
	return true;
}


void UdpSocket::pollRead()
{
	// TODO FIXME: implement
}

void UdpSocket::pollWrite()
{
	// TODO FIXME: implement
}

void UdpSocket::pollError()
{
	// TODO FIXME: implement
}

bool UdpSocket::bind(sockaddr_in address)
{
	// TODO FIXME: implement
	return false;
}

void UdpSocket::sendTo(const char * buffer, uint32_t length, const NetworkNode& target)
{
	// TODO FIXME: implement
}


}

