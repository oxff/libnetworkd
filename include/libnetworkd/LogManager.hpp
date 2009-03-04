/*
 * LogManager.hpp - subscribable log manager for daemon output redirection
 * $Id: LogManager.hpp 8 2008-01-11 13:47:40Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */
 

#ifndef __INCLUDE_libnetworkd_LogManager_hpp
#define __INCLUDE_libnetworkd_LogManager_hpp

#include <unistd.h>
#include <list>

using namespace std;


#include "LogFacility.hpp"



#ifdef _DEBUG
#define ASSERT(a); if(!(a)) { LOG("Assertion %s failed in %s / %s:%u!", __STRING(a), __PRETTY_FUNCTION__, __FILE__, __LINE__); exit(-1); }
#else
#define ASSERT(a)
#endif 


namespace libnetworkd
{


/**
* The LogManager class is a container for registering logging facilities --
* represented by a LogFacility instance -- and logging information to these
* facilities.
*/
class LogManager
{
public:
	virtual ~LogManager();
	
	/**
	* Add a new log facility to the chain of facilities being notified upon
	* new messages to be logged.
	* @param[in]	logFacility	The facility being added.
	*/
	virtual void addLogFacility(LogFacility * logFacility);
	
	/**
	* Remove an already registered facility from the log chain.
	* @param[in]	logFacility	The facility to be removed.
	*/
	virtual void removeLogFacility(LogFacility * logFacility);
	
	/**
	* Globally log a message using printf style syntax. The rendered
	* message will be send to all registered LogFacility instances.
	* @param[in]	format	A printf style format string.
	*/
	virtual void logFormatMessage(const char * format, ...);
	
	/**
	* This function performs the same job as logMessage(format, ...) but is
	* format string vulnerability safe and a little faster.
	* @param[in]	message	The message to be logged.
	*/
	virtual void logMessage(const char * message);
	
	inline list<LogFacility *> getLogFacilities()
	{ return m_logFacilities; }
	
private:
	list<LogFacility *> m_logFacilities;
};

//! global LogManager singleton
extern LogManager * g_logManager;


}

#endif // __INCLUDE_libnetworkd_LogManager_hpp
