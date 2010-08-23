/*
 * IO.hpp - single-threaded, abstracted I/O poll'ing
 * $Id: IO.hpp 3 2007-11-02 16:15:39Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */
 
#ifndef __INCLUDE_libnetworkd_IO_hpp
#define __INCLUDE_libnetworkd_IO_hpp

#include <stdint.h>

#include <list>
using namespace std;


namespace libnetworkd
{

//! The abstract status of a socket as returned by Socket::getStatus.
enum IOSocketState
{
	/**
	* Freshly added sockets are in this state until overwritten, they are
	* completly ignored by an IOManager.
	*/
	IOSOCKSTAT_IGNORE,
	
	/**
	* The socket is idle and waiting for input data, it has no output
	* waiting:
	* - POLLIN is set and checked, IOSocket::pollRead will be called if data
	* is actually available.
	* - POLLOUT is neither set nor checked.
	* - POLLERR is set and checked, IOSocket::pollError is called upon
	* error.
	* This is the default state in which most sockets should normally be.
	*/
	IOSOCKSTAT_IDLE,
	
	/**
	* The socket is ready for both side data transfers:
	* - POLLIN is set and checked, IOSocket::pollRead will be called if data
	* is actually available.
	* - POLLOUT is set and checked, IOSocket::pollWrite will be called if
	* data can be written
	* - POLLERR is set and checked, IOSocket::pollError will be called if an
	* error occured.
	*/
	IOSOCKSTAT_BUFFERING,
	
	/**
	* The socket is busy with some internal stuff, POLLERR is set and
	* checked, IOSocket:pollError will be called upon error.
	*/
	IOSOCKSTAT_BUSY,
};

/**
* Basic virtual interface for everything that should be polled and notified in a
* SocketManager, usually during each main loop iteration.
*/
class IOSocket
{
public:
	virtual ~IOSocket() { }
	
	/**
	* This function is called if poll returned POLLIN and depending on the
	* SocketState, this socket is currently in (set with
	* IOManager::setStatus).
	*/
	virtual void pollRead() = 0;
	
	/**
	* This function is called if poll returned POLLOUT and depending on the
	* SocketState, this socket is currently in (set with
	* IOManager::setStatus).
	*/
	virtual void pollWrite() = 0;
	
	/**
	* This function is called if poll returned POLLERR and depending on the
	* SocketState, this socket is currently in (set with
	* IOManager::setStatus).
	*/
	virtual void pollError() = 0;
	
	/**
	* The state of this socket. This is really ugly to keep here from an OOP
	* view but really pays out in performance.
	*/
	IOSocketState m_ioSocketState;
};


//! Used internally by the IOManager to associate data with an IOSocket
struct IOSocketRelated
{
	IOSocket * socket;
	int fileDescriptor;
};


/**
* Manager for maintaining IO sockets and polling them, can sleep when there is
* no input to be acted upon. There is usually only one instance of this class
* or a derived class (e.g. NetworkManager) per process.
*
* A typical use case is an IOSocket which adds itself to the global IOManager
* and updates its state with setState, depending on its current buffer
* situation.
*/
class IOManager
{
public:
	IOManager()
		: m_iterator(m_socketList.end())
	{ }
	
	virtual ~IOManager();
	
	/**
	* waitForEventsAndProcess waits for IO events on registered IOSocket's
	* and notifies these, depending on the IOSocketState they provided.
	* This is usually implemented by a call to poll or a similiar function.
	* @param[in]	maxWaitMillis	Maximum amount of milliseconds to wait
	*	until the function returns, even if no event occured. Set to 0
	*	to wait infinitely. This parameter is optional and defaults to
	*	0.
	*/
	void waitForEventsAndProcess(uint32_t maxWaitMillis = 0);

	
	/**
	* This functions registers an IOSocket at the IOManager, optionally
	* initializing the associated file descriptor and the initial socket
	* state.
	* @param[in]	socket		Pointer to the IOSocket to be added.
	* @param[in]	fileDescriptor	File descriptor that is associated with
	*	this IOSocket. This parameter is optional and can be set later
	*	on with IOManager::setFileDescriptor.
	* @return	Returns true if the socket was added or false if the
	*	socket was alread registered with the IOManager.
	*/	
	virtual bool addSocket(IOSocket * socket, int fileDescriptor = 0);
	
	/**
	* This function unregisters a previously registered IOSocket.
	* @param[in]	socket		The socket to be unregistered.
	* @return	True if the socket was unregistered or false if the
	*	socket was not previously known to the IOManager.
	*/
	virtual bool removeSocket(IOSocket * socket);
	
	/**
	* Update the file descriptor associated with an IOSocket and which is
	* going to be poll'ed. This is usually only called once, when the new
	* socket has been added without the optional parameters of addSocket.
	* However, there is no restrictions here, you can change the file
	* descriptor whenever you want.
	* @param[in]	socket		The socket to be updated.
	* @param[in]	fileDescriptor	The new file descriptor to be
	*	associated.
	*/
	virtual void setFileDescriptor(IOSocket * socket, int fileDescriptor);
	
protected:
	list<IOSocketRelated> m_socketList;
	list<IOSocketRelated>::iterator m_iterator;
};


}

#endif // __INCLUDE_libnetworkd_IO_hpp
