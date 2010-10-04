/*
 * Network.hpp - socket manager with connection establishment
 * $Id: Network.hpp 3 2007-11-02 16:15:39Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */


#ifndef __INCLUDE_libnetworkd_Network_hpp
#define __INCLUDE_libnetworkd_Network_hpp

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <map>
#include <list>
#include <string>
using namespace std;

#include "IO.hpp"
#include "NameResolution.hpp"


namespace libnetworkd
{

//! A host-on-a-network-representation.
struct NetworkNode
{
	//! IPv4, IPv6 or DNS string
	string name;
	
	//! TCP or UDP port
	uint16_t port;
	
	bool operator==(NetworkNode& b)
	{
		return ((name == "any" || b.name == "any" || name == b.name) && port == b.port);
	}
	
	bool operator()(const NetworkNode& a, const NetworkNode& b)
	{
		return (a.name != "any" && b.name != "any" && a.name < b.name) && a.port < b.port;
	}
};


/**
 * Endpoints of a connection are derived from this virtual class. It provides
 * callbacks to the different events that can happen on a network connection.
 * The derived class is either instanciated manually for a connecting connection
 * or crafted in a NetworkEndpointFactory in the case of a server socket.
 */
class NetworkEndpoint
{
public:
	//! Empty virtual deconstructor.
	virtual ~NetworkEndpoint() { }
	
	/**
	 * This function is called, as soon as incoming data has been received on
	 * the connection. This function has to be implemented by all endpoints.
	 * @param[in]	buffer		The data received.
	 * @param[in]	dataLength	The lenght of the received data in bytes.
	 */
	virtual void dataRead(const char * buffer, uint32_t dataLength) = 0;
	
	/**
	 * This function is called, once data has been successfully sent over the
	 * connection. The default implementation ignores this event.
	 * @param[in]	length	The length of the data sent in bytes.
	 */
	virtual void dataSent(uint32_t length) { }
	
	virtual void connectionEstablished(NetworkNode * remoteNode, NetworkNode * localNode) { }
	virtual void connectionClosed() { }
	virtual void connectionLost() { connectionClosed(); }
};

//! Abstract state of a NetworkSocket implementation.
enum NetworkSocketState
{
	//! The socket is unitialized.
	NETSOCKSTATE_UNINITIALIZED,
	
	//! A non-blocking connection attempt was started and is waiting for
	//! completition.
	NETSOCKSTATE_GOING_UP,
	
	//! The network socket is idle waiting for new input.
	NETSOCKSTATE_IDLE,
	
	//! The last non-blocking send could not transfer all data at once to
	//! kernel, the output is buffered in userland and will be send once
	//! possible.
	NETSOCKSTATE_BUFFERING,
	
	//! The socket was asked to be closed and now flushes all remaining
	//! output, then closes.
	NETSOCKSTATE_GOING_DOWN,
	
	//! The socket was closed.
	NETSOCKSTATE_DOWN,
};

class NetworkSocket
{
public:
	virtual ~NetworkSocket() { }
	
	virtual void send(const char * buffer, uint32_t length) = 0;
	virtual bool close(bool force = false) = 0;
	virtual NetworkSocketState getState() = 0;
};


class NetworkEndpointFactory
{
public:
	virtual ~NetworkEndpointFactory() { }
	
	virtual NetworkEndpoint * createEndpoint(NetworkSocket * clientSocket) = 0;
	virtual void destroyEndpoint(NetworkEndpoint * endpoint) { delete endpoint; }
};


class UdpSocket;

class NetworkManager : public IOManager
{
public:	
	virtual ~NetworkManager();
	
	virtual NetworkSocket * connectStream(const NetworkNode * remoteNode, NetworkEndpoint * localEndpoint,
		const NetworkNode * localNode = 0);
	virtual NetworkSocket * serverStream(const NetworkNode * localNode, NetworkEndpointFactory * endpointFactory, uint8_t serverBacklogSize);
	virtual bool closeStream(NetworkSocket * socket, bool force = false);
	
	virtual NetworkSocket * connectUnix(const char * path, NetworkEndpoint * localEndpoint);
	virtual NetworkSocket * serverUnix(const char * path, NetworkEndpointFactory * endpointFactory, uint8_t serverBacklogSize);
//	virtual bool closeUnix(NetworkSocket * unix, bool force = false);
	
	virtual NetworkSocket * connectDatagram(const NetworkNode * remoteNode, NetworkEndpoint * localEndpoint, const NetworkNode * localNode = 0);
	virtual NetworkSocket * serverDatagram(const NetworkNode * localNode, NetworkEndpointFactory * endpointFactory);
	virtual bool closeDatagram(NetworkSocket * socket, bool force = false);
	
	void dropDatagramSocket(NetworkNode& node, UdpSocket * socket);

protected:
	map<NetworkNode, UdpSocket *, NetworkNode> m_boundDatagramSockets;
};


//! Implementation of IOSocket for TCP based POSIX network sockets.
class TcpSocket : public NetworkSocket, public IOSocket
{
public:
	TcpSocket();
	TcpSocket(IOManager * ioManager, NetworkEndpoint * clientEndpoint);
	TcpSocket(IOManager * ioManager, NetworkEndpointFactory * serverEndpointFactory);
	virtual ~TcpSocket();
	
	virtual void pollRead();
	virtual void pollWrite();
	virtual void pollError();
		
	virtual bool connect(struct sockaddr_in * remoteHost);
	virtual bool bind(struct sockaddr_in * localAddress);
	virtual bool listen(uint8_t backlogSize);	
	virtual bool close(bool force = false);
	
	virtual void send(const char * buffer, uint32_t length);
	
	virtual NetworkSocketState getState();
	
protected:
	bool socket();
	
	TcpSocket(IOManager * ioManager, int connectedSocket, NetworkEndpointFactory * factory, struct sockaddr_in * remoteAddress);
	

protected:	
	IOManager * m_ioManager;
	
	string m_outputBuffer;
	int m_socket;
	
	NetworkEndpoint * m_clientEndpoint;
	NetworkEndpointFactory * m_serverEndpointFactory;

	NetworkSocketState m_state;
	bool m_serverSocket;
};


class UnixSocket : public TcpSocket
{
public:
	UnixSocket(IOManager * ioManager, NetworkEndpoint * clientEndpoint);
	UnixSocket(IOManager * ioManager,
		NetworkEndpointFactory * serverEndpointFactory);
	virtual ~UnixSocket() { }
	
	virtual void pollRead();

	virtual bool connect(const char * serverPath);
	virtual bool bind(const char * serverPath);

protected:
	bool socket();
	
	UnixSocket(IOManager * ioManager, int connectedSocket, NetworkEndpointFactory * factory);
};


class UdpSocket;

//! Wrapper around a UdpSocket featuring the classical send() as it locally
//! stores the destination network node for all packets.
class UdpSocketWrapper : public NetworkSocket
{
public:
	UdpSocketWrapper(UdpSocket * parent, const NetworkNode& node)
	{ m_parent = parent; m_node = node; }
	virtual ~UdpSocketWrapper();
	
	virtual void send(const char * buffer, uint32_t length);
	virtual bool close(bool force = false);
	virtual NetworkSocketState getState();
	
private:
	UdpSocket * m_parent;
	NetworkNode m_node;
};

//! Implementation of IOSocket for UDP based POSIX network sockets.
class UdpSocket : public IOSocket
{
public:
	UdpSocket(IOManager * ioManager, NetworkEndpointFactory * endpointFactory = 0);
	virtual ~UdpSocket();
	
	virtual void pollRead();
	virtual void pollWrite();
	virtual void pollError();
	
	virtual bool bind(struct sockaddr_in localAddress);	
	virtual void sendTo(const char * buffer, uint32_t length, const NetworkNode& target);
	
	virtual UdpSocketWrapper * addEndpoint(const NetworkNode& networkNode, NetworkEndpoint * endpoint);
	virtual bool dropEndpoint(UdpSocketWrapper * wrapper, NetworkNode& remoteNode);
	
	inline NetworkSocketState getState()
	{ return m_state; }
	
protected:
	bool socket();
	
private:
	struct PacketCache
	{
		NetworkNode target;
		string buffer;
	};
	
	list<PacketCache> m_packetCache;

	IOManager * m_ioManager;
	int m_socket;
	
	map<NetworkNode, pair<UdpSocketWrapper *, NetworkEndpoint *>, NetworkNode> m_clientEndpoints;
	
	/**
	 * If desired, this endpoint factory creates new endpoints each time a 
	 * new unique ip:port pair sends a packet to this UDP socket. Otherwise,
	 * only packets from a previously known source are accepted and others
	 * are ignored.
	 */
	NetworkEndpointFactory * m_serverEndpointFactory;
	
	NetworkSocketState m_state;
};


}

#endif // __INCLUDE_libnetworkd_Network_hpp
