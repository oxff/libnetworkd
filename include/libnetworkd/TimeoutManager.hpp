/*
 * TimeoutManager.hpp - asynchronous scheduling of future processing
 * $Id: TimeoutManager.hpp 43 2008-06-16 22:06:22Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */


#ifndef __INCLUDE_libnetworkd_TimeoutManager_hpp
#define __INCLUDE_libnetworkd_TimeoutManager_hpp


#include <set>
#include <time.h>


namespace libnetworkd
{


#define TIMEOUT_EMPTY ((void *) 0)

typedef void * Timeout;


/**
 * Abstract receiver of timeouts.
 */
class TimeoutReceiver
{
public:
	virtual ~TimeoutReceiver() { };

	/**
	 * A timeout passed and the associated action should be run.
	 * @param[in]	timeout	The timeout object fired.
	 */
	virtual void timeoutFired(Timeout timeout) = 0;
};


/**
 * Manager of timeout events, allows for registering timeouts, firing passed
 * timeouts and requesting delta to the next timeout.
 */
class TimeoutManager
{
public:
	//! Destruct the manager and free all allocated timeouts.
	~TimeoutManager();
	
	/**
	 * Schedule a new timeout, passed to the registered receiver.
	 * @param	delta[in]		Offset in seconds for this timeout to be fired.
	 * @param	receiver[in]	TimeoutReceiver of this timeout.
	 */
	Timeout scheduleTimeout(unsigned int delta, TimeoutReceiver * receiver);	
	
	/**
	 * Unregister the given timeout and don't call it after specified delta.
	 * @param	timeout[in] Timeout to be dropped.
	 */
	void dropTimeout(Timeout timeout);
	
	/**
	 * Unregister all timeouts of the specified TimeoutReceiver.
	 * @param	receiver[in] TimeoutReceiver which's timeouts are dropped.
	 */
	void dropReceiver(TimeoutReceiver * receiver);
	
	/**
	 * Get the delta from now to the next event in seconds.
	 */
	inline unsigned int deltaNext()
	{
		if(m_timeouts.empty())
			return (unsigned int) -1;
		
		return (* m_timeouts.begin())->firets - time(0);
	}
	
	/**
	 * Fire all events which's delta has passed until now.
	 */
	void fireTimeouts();

private:
	struct TimeoutInfo
	{
		unsigned int firets;
		TimeoutReceiver * receiver;
	};
	
	struct TimeoutInfoEq
	{
		inline bool operator()(TimeoutInfo * a, TimeoutInfo * b)
		{
			return a->firets < b->firets;
		}
	};
	
	std::multiset<TimeoutInfo *, TimeoutInfoEq> m_timeouts;
	std::multiset<TimeoutInfo *>::iterator m_lockedTimeout;
};


}

#endif // __INCLUDE_libnetworkd_TimeoutManager_hpp
