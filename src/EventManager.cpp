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

void EventManager::fireEvent(Event * event)
{
	string eventName = event->getName();
	
	if(m_logManager)
		m_logManager->logMessage(LogManager::LL_EVENT, event->toString().c_str());
	
	for(list<EventSubscription>::iterator i = m_eventSubscriptions.begin(); i != m_eventSubscriptions.end(); ++i)
	{
		if(nameLikeMask(eventName, i->eventMask))
			i->subscriber->handleEvent(event);
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


uint8_t Event::m_incrementing;


}
