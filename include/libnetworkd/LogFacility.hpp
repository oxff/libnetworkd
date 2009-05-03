/*
 * LogFacility.hpp - log facility abstraction to be added to LogManager
 * $Id: LogFacility.hpp 3 2007-11-02 16:15:39Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */

#include "LogManager.hpp"

#ifndef __INCLUDE_libnetworkd_LogFacility_hpp
#define __INCLUDE_libnetworkd_LogFacility_hpp

namespace libnetworkd
{


/**
* Generic logging facility abstraction class. Instances of this class can be
* added to a the global LogManager. Each registered instance then receives
* log messages as rendered string and should output them to a facilitiy
* specific medium.
*/
class LogFacility
{
public:
	virtual ~LogFacility() { }
	
	/**
	* This function is called from the LogManager, once a new message
	* is to be logged.
	* @param[in]	level		The level, this message is classified
	* 				as.
	* @param[in]	renderedMessage	The message, the LogManager rendered 
	*				and now wants to be logged.
	*/
	virtual void logMessage(LogManager::LogLevel level, const char * renderedMessage) = 0;
	
	/**
	* The LogManager calls this function to obtain a facility's name when
	* it wants to generate a listing for debugging or configuration
	* purposes.
	* @return A valid, static pointer to the facility's name string.
	*/
	virtual const char * getName() = 0;
	
	/**
	* The LogManager calls this function to obtain a facility's description
	* when it wants to generate a listing for debugging or configuration
	* purposes.
	* @return A valid, static pointer to the facility's description string.
	*/
	virtual const char * getDescription() = 0;
	
	/**
	* This function should render a log target's description understandable
	* by a human interpreter, such as a file name for a file logger or an
	* target URI for a network logging module.
	* @return A valid, static pointer to the facility's log target string.
	*/
	virtual const char * getTarget() = 0;
};


}

#endif // __INCLUDE_libnetworkd_LogFacility_hpp
