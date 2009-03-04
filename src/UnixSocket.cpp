/*
 * TcpSocket.cpp - IOSocket implementation for UNIX domain sockets
 * $Id: UnixSocket.cpp 2 2007-11-02 15:31:40Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>

#include <libnetworkd/Network.hpp>


#define UNIX_PATH_MAX 108

namespace libnetworkd
{

UnixSocket::UnixSocket(IOManager * ioManager, NetworkEndpoint * clientEndpoint)
{
	m_ioManager = ioManager;
	m_clientEndpoint = clientEndpoint;
}

UnixSocket::UnixSocket(IOManager * ioManager, NetworkEndpointFactory * serverEndpointFactory)
{
	m_ioManager = ioManager;
	m_serverEndpointFactory = serverEndpointFactory;
}

UnixSocket::UnixSocket(IOManager * ioManager, int existingSocket, NetworkEndpointFactory * factory)
{
	m_ioManager = ioManager;
	m_socket = existingSocket;
	m_serverEndpointFactory = factory;
	m_clientEndpoint = factory->createEndpoint(this);
	
	{
		m_ioManager->addSocket(this, m_socket);
		m_ioSocketState = IOSOCKSTAT_IDLE;
		
		m_state = NETSOCKSTATE_IDLE;
	}
	
	m_clientEndpoint->connectionEstablished(0, 0);
}


bool UnixSocket::connect(const char * serverPath)
{
	if(m_state != NETSOCKSTATE_UNINITIALIZED)
		return false;
		
	if(m_socket == -1 && !socket())
		return false;
	
	struct sockaddr_un serverAddress;
	
	serverAddress.sun_family = AF_UNIX;
	strncpy(serverAddress.sun_path, serverPath, UNIX_PATH_MAX);
		
	if(::connect(m_socket, (struct sockaddr *) &serverAddress, sizeof(struct sockaddr_un)) == 0)
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
	
	return false;
}

bool UnixSocket::bind(const char * serverPath)
{
	struct sockaddr_un serverAddress;
	
	serverAddress.sun_family = AF_UNIX;
	strncpy(serverAddress.sun_path, serverPath, UNIX_PATH_MAX);
	
	if(!socket())
		return false;
	
	return (::bind(m_socket, (struct sockaddr *) &serverAddress, sizeof(struct sockaddr_un)) == 0);
}

bool UnixSocket::socket()
{
	int trueval = 1;
	
	if(m_socket >= 0 || m_state != NETSOCKSTATE_UNINITIALIZED)
		return false;

	m_socket = ::socket(AF_UNIX, SOCK_STREAM, 0);
	
	if(m_socket < 0)
		return false;
		
	if(setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &trueval, sizeof(trueval)) < 0)
		return false;
		
	return (fcntl(m_socket, F_SETFL, fcntl(m_socket, F_GETFL) | O_NONBLOCK) == 0);
}

void UnixSocket::pollRead()
{
	if(m_serverSocket)
	{ // server
		struct sockaddr_in clientAddress;
		socklen_t clientLen = sizeof(clientAddress);
		int clientSocket = accept(m_socket, (struct sockaddr *) &clientAddress, &clientLen);
		
		if(clientSocket > 0)
		{
			new UnixSocket(m_ioManager, clientSocket, m_serverEndpointFactory);
		}
	}
	else
		return TcpSocket::pollRead();
}


}

