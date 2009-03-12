/*
 * IOManager.cpp - implementation of poll'ing IOSocket Manager
 * $Id: IOManager.cpp 37 2008-03-05 17:50:21Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */

#include <libnetworkd/IO.hpp>
#include <libnetworkd/LogManager.hpp>

#include <stdlib.h>
#include <sys/poll.h>


namespace libnetworkd
{


IOManager::IOManager()
{
			m_removalCount = 0;
}

IOManager::~IOManager()
{
	// TODO: implement
}


bool IOManager::addSocket(IOSocket * socket, int fileDescriptor)
{
	list<IOSocketRelated>::iterator i;

	{
		IOSocketRelated info;
		
		info.socket = socket;
		info.fileDescriptor = fileDescriptor;
		info.markedForRemoval = false;
		
		m_socketList.push_back(info);
	}
	
	return true;
}

bool IOManager::removeSocket(IOSocket * socket)
{
	list<IOSocketRelated>::iterator i = m_socketList.begin();
	
	for(; i != m_socketList.end(); ++i)
		if(i->socket == socket)
			break;
	
	if(i == m_socketList.end() || i->markedForRemoval)
		return false;

	i->markedForRemoval = true;
	++m_removalCount;
		
	return true;
}


void IOManager::setFileDescriptor(IOSocket * socket, int fd)
{
	list<IOSocketRelated>::iterator i = m_socketList.begin();
	
	for(; i != m_socketList.end(); ++i)
		if(i->socket == socket)
			break;
	
	if(i == m_socketList.end())
		return;
		
	i->fileDescriptor = fd;
}


void IOManager::removeMarked()
{
	list<IOSocketRelated>::iterator next;
	
	for(list<IOSocketRelated>::iterator i = m_socketList.begin(); m_removalCount && i != m_socketList.end(); i = next)
	{
		next = i;
		++next;
		
		if(i->markedForRemoval)
		{
			m_socketList.erase(i);
			--m_removalCount;
		}
	}
}


void IOManager::waitForEventsAndProcess(uint32_t maxwait)
{
	removeMarked();
	
	struct pollfd * pollfds = (struct pollfd *) malloc(m_socketList.size() * sizeof(struct pollfd));
	list<IOSocketRelated>::iterator i;
	int pollResult;
	uint32_t j, c;
	
	c = m_socketList.size();
	
	{
		j = 0;
			
		for(i = m_socketList.begin(); j < c; ++i, ++j)
		{
			ASSERT(i != m_socketList.end());
			
			pollfds[j].fd = i->fileDescriptor;
			pollfds[j].revents = 0;
			
			if(i->socket->m_ioSocketState == IOSOCKSTAT_IGNORE)
				pollfds[j].events = 0;
			else if(i->socket->m_ioSocketState == IOSOCKSTAT_IDLE)
				pollfds[j].events = POLLIN | POLLERR;
			else if(i->socket->m_ioSocketState == IOSOCKSTAT_BUFFERING)
				pollfds[j].events = POLLIN | POLLOUT | POLLERR;
			else
				pollfds[j].events = POLLERR;	
		}
		
		pollResult = poll(pollfds, j, maxwait);
		
		if(pollResult <= 0)
		{
			free(pollfds);
			return;
		}
			
		// TODO: error message if pollResult < 0
	}
	
	{
		j = 0;
			
		for(i = m_socketList.begin(); j < c; ++i, ++j)
		{
			ASSERT(i != m_socketList.end());
			
			if(i->markedForRemoval || i->socket->m_ioSocketState == IOSOCKSTAT_IGNORE)
				continue;
			
			if(pollfds[j].revents & POLLERR)
				i->socket->pollError();
				
			if(i->markedForRemoval)
				continue;
				
			if(pollfds[j].revents & POLLOUT)
				i->socket->pollWrite();
			
			if(i->markedForRemoval)
				continue;
				
			if(pollfds[j].revents & POLLIN)
				i->socket->pollRead();
		}
	}
	
	free(pollfds);
	
	removeMarked();
}


}

