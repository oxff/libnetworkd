/*
 * EventManager.hpp - inter module communication subsystem
 * $Id: EventManager.hpp 3 2007-11-02 16:15:39Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */


#ifndef __INCLUDE_libnetworkd_EventManager_hpp
#define __INCLUDE_libnetworkd_EventManager_hpp

#include "Event.hpp"
#include "LogManager.hpp"


#include <string>
#include <list>
#include <sys/time.h>
#include <time.h>


namespace libnetworkd
{


#define CONTINOUS_TIMEVAL(a) ((uint64_t) ((uint64_t) a.tv_sec * 1000 + a.tv_usec))

/**
 * Abstract representation of the subscriber to an event.
 */
class EventSubscriber
{
public:
	virtual ~EventSubscriber() { }
	
	/**
	 * Invoked by the EventManager, each time one of the subscribed events
	 * is fired (directly or after some delay if specified).
	 * @param[in]	firedEvent	The event that was fired.
	 */
	virtual void handleEvent(Event * firedEvent) = 0;
};

//! Used internally by the EventManager to track subscribers.
struct EventSubscription
{
	string eventMask;
	EventSubscriber * subscriber;
	bool exclusive;
};

/**
 * Manager for abstract Event objects. Allows direct fireing of events or
 * after a specified delay. If delayed, events can also be fired periodically,
 * providing a functionality comparable to a timer.
 *
 * Events can be subscribed based upon an event's name. If desired, events can
 * be subscribed exclusively (so nothing else can subscribe this event).
 */
class EventManager
{
public:
	EventManager() { m_logManager = 0; }
	virtual ~EventManager() { }
	
	/**
	 * Fire the given event.
	 * @param[in]	event		Abstract Event representation.
	 */
	virtual void fireEvent(Event * event);

	/**
	 * Log all events with LogLevel LL_EVENT to the given manager, deactivate by setting NULL.
	 * @param[in]	manager		The LogManager to use.
	 */
	inline void setLogManager(LogManager * manager)
	{ m_logManager = manager; }
	
	virtual bool subscribeEventMask(string eventMask, EventSubscriber * eventSubscriber, bool subscribeExclusively = false);
	virtual bool unsubscribeEventMask(string eventMask, EventSubscriber * eventSubscriber);
	virtual bool unsubscribeAll(EventSubscriber * eventSubscriber);
	
	
protected:
	inline bool nameLikeMask(string name, string mask);
	
private:
	list<EventSubscription> m_eventSubscriptions;

	LogManager * m_logManager;
};


}

#endif // __INCLUDE_libnetworkd_EventManager_hpp
