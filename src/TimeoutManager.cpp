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


#include <stdio.h>


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
			if(m_iterator == m_timeouts.end())
			{ // Not iterating, just remove it.
				m_timeouts.erase(it);
				delete (TimeoutInfo *) timeout;
			}
			else if((* m_iterator)->firets < ((TimeoutInfo *) timeout)->firets)
			{ // Iterating and would not get removed automatically because it is passed just now.
				TimeoutInfo * current = * m_iterator;

				m_timeouts.erase(it);
				m_iterator = m_timeouts.find(current);

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
	TimeoutInfo * current = 0;
	
	for(std::multiset<TimeoutInfo *>::iterator it = m_timeouts.begin();
		it != m_timeouts.end(); it = next)
	{
		next = it;
		++next;
		
		if((* it)->receiver == receiver)
		{
			if(m_iterator == m_timeouts.end())
			{ // Not iterating, just remove it.
				delete (TimeoutInfo *) (* it);
				m_timeouts.erase(it);
			}
			else if((* m_iterator)->firets < (* it)->firets)
			{ // In the future, we need to proactively remove it.
				current = * m_iterator;

				delete (TimeoutInfo *) (* it);
				m_timeouts.erase(it);
			}
		}
	}

	if(current)
		m_iterator = m_timeouts.find(current);
}


void TimeoutManager::fireTimeouts()
{
	unsigned int now = time(0);	
	std::multiset<TimeoutInfo *>::iterator next;
	
	for(m_iterator = m_timeouts.begin(); m_iterator != m_timeouts.end() && (* m_iterator)->firets <= now; ++m_iterator)
	{
		TimeoutInfo * p = * m_iterator;
	
		p->receiver->timeoutFired((Timeout) * m_iterator);
		delete p;
	}

	m_timeouts.erase(m_timeouts.begin(), m_iterator);
	m_iterator = m_timeouts.end();
}


}
