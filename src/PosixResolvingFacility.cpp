/*
 * PosixResolvingFacility.cpp - resolve network names using getaddrinfo
 * $Id: PosixResolvingFacility.cpp 26 2008-02-22 14:35:29Z lostrace $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */
 

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
using namespace std;


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libnetworkd/NameResolution.hpp>

namespace libnetworkd
{


void PosixResolvingFacility::resolveName(string name, NameResolver * resolver)
{
	struct addrinfo hint = { 0 };
	struct addrinfo * resultList;
	int result;
	
	hint.ai_flags = AI_ADDRCONFIG;
	
	if((result = getaddrinfo(name.c_str(), 0, &hint, &resultList)) != 0)
	{
		if(result == EAI_ADDRFAMILY || result == EAI_NODATA || result == EAI_NONAME)
			resolver->nameResolved(name, list<string>(), NRS_HOST_UNKNOWN);
		else
			resolver->nameResolved(name, list<string>(), NRS_FAILED);
	}
	else
	{
		list<string> addresses;
		
		for(struct addrinfo * i = resultList; i; i = i->ai_next)
			if(i->ai_addrlen == sizeof(struct sockaddr_in) && i->ai_family == AF_INET)
				addresses.push_back(string(inet_ntoa(((struct sockaddr_in *) i->ai_addr)->sin_addr)));

		resolver->nameResolved(name, addresses, NRS_OK);
		
		freeaddrinfo(resultList);
	}
}

#ifndef HAVE_LIBUDNS

NameResolvingFacility * NameResolvingFacility::
	createFacility(IOManager * manager, TimeoutManager * mgr)
{
	return new PosixResolvingFacility();
}

#endif

}
