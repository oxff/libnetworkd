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

#include <algorithm>

#include <stdlib.h>
#include <sys/poll.h>


namespace libnetworkd
{


IOManager::~IOManager()
{
}


bool IOManager::addSocket(IOSocket * socket, int fileDescriptor)
{
	list<IOSocketRelated>::iterator i;

	{
		IOSocketRelated info;
		
		info.socket = socket;
		info.fileDescriptor = fileDescriptor;
		
		m_socketList.push_back(info);
	}
	
	return true;
}

bool IOManager::removeSocket(IOSocket * socket)
{
	list<IOSocketRelated>::iterator it = m_socketList.begin();
	for(; it != m_socketList.end() && it->socket != socket; ++it);

	if(it == m_socketList.end())
		return false;

	if(it == m_iterator)
		++m_iterator;

	m_socketList.erase(it);
	return true;
}


void IOManager::setFileDescriptor(IOSocket * socket, int fd)
{
	list<IOSocketRelated>::iterator it = m_socketList.begin();
	for(; it != m_socketList.end() && it->socket != socket; ++it);

	if(it == m_socketList.end())
		return;
		
	it->fileDescriptor = fd;
}

void IOManager::waitForEventsAndProcess(uint32_t maxwait)
{
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
			
		for(m_iterator = m_socketList.begin(); m_iterator != m_socketList.end() && j < c; ++j)
		{
			if(m_iterator->fileDescriptor != pollfds[j].fd || i->socket->m_ioSocketState == IOSOCKSTAT_IGNORE)
				continue;
			
			if(pollfds[j].revents & POLLERR)
				m_iterator->socket->pollError();
				
			if(m_iterator->fileDescriptor != pollfds[j].fd)
				continue;
				
			if(pollfds[j].revents & POLLOUT)
				m_iterator->socket->pollWrite();
			
			if(m_iterator->fileDescriptor != pollfds[j].fd)
				continue;
				
			if(pollfds[j].revents & POLLIN)
				m_iterator->socket->pollRead();

			++m_iterator;
		}
	}
	
	free(pollfds);
}


}

