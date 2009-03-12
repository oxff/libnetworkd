/*
 * LogManager.cpp - global manager for distributed logging
 * $Id: LogManager.cpp 3 2007-11-02 16:15:39Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include <libnetworkd/LogManager.hpp>


namespace libnetworkd
{


LogManager::~LogManager()
{
	// TODO: unregister all loggers if needed
}

void LogManager::addLogFacility(LogFacility * logFacility)
{
	m_logFacilities.push_back(logFacility);
}

void LogManager::removeLogFacility(LogFacility * logFacility)
{
	m_logFacilities.remove(logFacility);
}


void LogManager::logMessage(const char * message)
{
	for(list<LogFacility *>::iterator i = m_logFacilities.begin(); i != m_logFacilities.end(); ++i)
		(* i)->logMessage(message);
}

void LogManager::logFormatMessage(const char * format, ...)
{
	 char * logMessage;
	        
	{
		va_list vaParameters;

		va_start(vaParameters, format);

		if(vasprintf(&logMessage, format, vaParameters) < 0)
			return;

		va_end(vaParameters);
	}
	
	this->logMessage(logMessage);
	free(logMessage);
}


}
