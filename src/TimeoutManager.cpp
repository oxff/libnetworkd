/*
 * TimeoutManager.cpp - asynchronous scheduling of future processing
 * $Id: TimeoutManager.cpp 43 2008-06-16 22:06:22Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */

#include <time.h>

#include <libnetworkd/TimeoutManager.hpp>


namespace libnetworkd
{


TimeoutManager::~TimeoutManager()
{
	for(std::multiset<TimeoutInfo *>::iterator it = m_timeouts.begin();
		it != m_timeouts.end(); ++it)
	{
		delete (* it);
	}
	
	m_timeouts.clear();
}


Timeout TimeoutManager::scheduleTimeout(unsigned int delta,
	TimeoutReceiver * receiver)
{
	TimeoutInfo * timeout = new TimeoutInfo;
	
	timeout->firets = time(0) + delta;
	timeout->receiver = receiver;
	
	m_timeouts.insert(timeout);
	return (Timeout) timeout;
}

void TimeoutManager::dropTimeout(Timeout timeout)
{
	if(timeout == TIMEOUT_EMPTY)
		return;

	std::multiset<TimeoutInfo *, TimeoutInfo>::iterator it;	
	it = m_timeouts.lower_bound((TimeoutInfo *) timeout);
	
	while(it != m_timeouts.end()
		&& (* it)->firets == ((TimeoutInfo *) timeout)->firets)
	{
		if(* it == (TimeoutInfo *) timeout)
		{
			if(it != m_lockedTimeout)
			{
				m_timeouts.erase(it);
				delete (TimeoutInfo *) timeout;
			}
			
			return;
		}
		
		++it;
	}
}

void TimeoutManager::dropReceiver(TimeoutReceiver * receiver)
{
	std::multiset<TimeoutInfo *>::iterator next;
	
	for(std::multiset<TimeoutInfo *>::iterator it = m_timeouts.begin();
		it != m_timeouts.end(); it = next)
	{
		next = it;
		++next;

		if(it == m_lockedTimeout)
			continue;
		
		if((* it)->receiver == receiver)
		{
			delete (TimeoutInfo *) (* it);
			m_timeouts.erase(it);
		}
	}
}


void TimeoutManager::fireTimeouts()
{
	unsigned int now = time(0);	
	std::multiset<TimeoutInfo *>::iterator next;
	
	for(std::multiset<TimeoutInfo *>::iterator it = m_timeouts.begin();
		it != m_timeouts.end(); it = next)
	{
		m_lockedTimeout = it;
		next = it;		
		
		if((* it)->firets <= now)
		{
			TimeoutInfo * p = * it;
		
			p->receiver->timeoutFired((Timeout) * it);		

			++next;			
			m_timeouts.erase(it);

			delete p;
		}
		else
		{
			m_lockedTimeout = m_timeouts.end();
			return;
		}
	}

	m_lockedTimeout = m_timeouts.end();
}


}
