/*
 * EventManager.cpp - generic event manager, supports firing and subscribing
 * $Id: EventManager.cpp 3 2007-11-02 16:15:39Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */

#include <libnetworkd/EventManager.hpp>
#include <libnetworkd/LogManager.hpp>


namespace libnetworkd
{


bool EventManager::nameLikeMask(string name, string mask)
{
	string::iterator a, b;
	
	a = name.begin();
	b = mask.begin();
	
	for(; a != name.end() && b != mask.end() && * a == * b; ++a, ++b);

	if((a == name.end() && b == mask.end()) || * b == '*')
		return true;
		
	return false;
}

void EventManager::fireEvent(Event * event, uint32_t delay, bool periodic)
{
	if(!delay)
	{
		ASSERT(!periodic);
		string eventName = event->getName();
		
		if(m_logManager)
			m_logManager->logMessage(LogManager::LL_EVENT, event->toString().c_str());
		
		for(list<EventSubscription>::iterator i = m_eventSubscriptions.begin(); i != m_eventSubscriptions.end(); ++i)
		{
			if(nameLikeMask(eventName, i->eventMask))
				i->subscriber->handleEvent(event);
		}
	}
	else
	{
		EventCache cache;
				
		cache.event = * event;		
		gettimeofday(&cache.invoked, 0);
		cache.delay = delay;
		cache.periodic = periodic;
		
		m_timedEvents.push_back(cache);
		
		if(m_nextEvent == m_timedEvents.end())
			--m_nextEvent;
		else if(CONTINOUS_TIMEVAL(m_nextEvent->invoked) + m_nextEvent->delay < CONTINOUS_TIMEVAL(cache.invoked) + cache.delay)
		{
			m_nextEvent = m_timedEvents.end();
			--m_nextEvent;
		}
	}
}


bool EventManager::subscribeEventMask(string eventMask, EventSubscriber * eventSubscriber, bool subscribeExclusively)
{
	ASSERT(!(subscribeExclusively && (* (eventMask.end() --)) == '*'));
	
	for(list<EventSubscription>::iterator i = m_eventSubscriptions.begin(); i != m_eventSubscriptions.end(); ++i)
	{
		if((subscribeExclusively && nameLikeMask(eventMask, i->eventMask)) || (i->exclusive && nameLikeMask(i->eventMask, eventMask)))
			return false;
			
		if(i->subscriber == eventSubscriber && (nameLikeMask(eventMask, i->eventMask) || nameLikeMask(i->eventMask, eventMask)))
			return false;
	}
	
	EventSubscription subscription;
	
	subscription.eventMask = eventMask;
	subscription.subscriber = eventSubscriber;
	subscription.exclusive = subscribeExclusively;
	m_eventSubscriptions.push_back(subscription);
	
	return true;
}

bool EventManager::unsubscribeEventMask(string eventMask, EventSubscriber * eventSubscriber)
{	
	for(list<EventSubscription>::iterator i = m_eventSubscriptions.begin(); i != m_eventSubscriptions.end(); ++i)
	{
		if(i->eventMask == eventMask && i->subscriber == eventSubscriber)
		{
			m_eventSubscriptions.erase(i);			
			return true;
		}
	}
	
	return false;
}

bool EventManager::unsubscribeAll(EventSubscriber * eventSubscriber)
{
	bool foundOne = false;	
	list<EventSubscription>::iterator next;
	
	for(list<EventSubscription>::iterator i = m_eventSubscriptions.begin(); i != m_eventSubscriptions.end(); i = next)
	{
		next = i;
		++next;
		
		if(i->subscriber == eventSubscriber)
		{
			m_eventSubscriptions.erase(i);
			foundOne = true;
		}
	}
	
	return foundOne;
}

void EventManager::loopTimedEvents()
{
	struct timeval now;
	
	gettimeofday(&now, 0);
	
	while(m_nextEvent != m_timedEvents.end() && CONTINOUS_TIMEVAL(now) >= CONTINOUS_TIMEVAL(m_nextEvent->invoked) + m_nextEvent->delay)
	{
		fireEvent(&m_nextEvent->event);
		
		if(!m_nextEvent->periodic)
			m_timedEvents.erase(m_nextEvent);
			
		m_nextEvent = m_timedEvents.begin();
		
		for(std::list<EventCache>::iterator i = m_nextEvent; i != m_timedEvents.end(); ++i)
			if(CONTINOUS_TIMEVAL(i->invoked) + i->delay > CONTINOUS_TIMEVAL(m_nextEvent->invoked) + m_nextEvent->delay)
				m_nextEvent = i;
	}
}

uint64_t EventManager::nextEventDelta()
{
	struct timeval now;
	
	if(m_nextEvent == m_timedEvents.end())
		return 0;
	
	gettimeofday(&now, 0);
	
	return (CONTINOUS_TIMEVAL(m_nextEvent->invoked) + m_nextEvent->delay) - CONTINOUS_TIMEVAL(now);
}



uint8_t Event::m_incrementing;


}
