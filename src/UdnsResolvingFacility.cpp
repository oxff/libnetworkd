/*
 * UdnsResolvingFacility.cpp - resolve network names using libudns (async)
 * $Id: UdnsResolvingFacility.cpp 41 2008-04-01 14:03:50Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */

#ifdef HAVE_LIBUDNS

#include <libnetworkd/NameResolution.hpp>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


namespace libnetworkd
{


NameResolvingFacility * NameResolvingFacility::
	createFacility(IOManager * manager, TimeoutManager * mgr)
{
	return new UdnsResolvingFacility(manager, mgr);
}

UdnsResolvingFacility::UdnsResolvingFacility(IOManager * mgr,
	TimeoutManager * tomgr)
{
	m_context = NULL;

	dns_reset(m_context);
		
	if(dns_init(m_context, 1) < 0)
		throw;
		
	m_ioSocketState = IOSOCKSTAT_IDLE;
		
	m_ioManager = mgr;
	mgr->addSocket(this, dns_sock(m_context));
	
	m_timeoutManager = tomgr;
	m_timeout = TIMEOUT_EMPTY;
}

UdnsResolvingFacility::~UdnsResolvingFacility()
{
	m_ioManager->removeSocket(this);
	
	if(m_timeout != TIMEOUT_EMPTY)
		m_timeoutManager->dropReceiver(this);
	
	m_timeout = TIMEOUT_EMPTY;
	

	for(ResolutionSet::iterator it = m_pendingResolutions.begin();
		it != m_pendingResolutions.end(); ++it)
	{
		delete * it;
	}

	
	m_pendingResolutions.clear();

	dns_close(m_context);
	
	if(m_context)
		dns_free(m_context);
}

void UdnsResolvingFacility::pollRead()
{
	dns_ioevent(m_context, 0);
}

void UdnsResolvingFacility::pollError()
{
	// TODO FIXME: handle the socket error (our upstream DNS is broken)
}

void UdnsResolvingFacility::resolveName(string name, NameResolver * resolver)
{
	char tmp[6];
	
	if(dns_pton(AF_INET, name.c_str(), tmp) > 0)
	{
		list<string> addresses;
		addresses.push_back(name);
		resolver->nameResolved(name, addresses, NRS_OK);

		return;
	}
	
	ResolutionEntry * entry = new ResolutionEntry;
	
	entry->facility = this;
	entry->resolver = resolver;
	entry->domain = name;

	if((entry->query = dns_submit_a4(m_context, name.c_str(), 0,
		&UdnsResolvingFacility::callback, entry)))
	{
		m_pendingResolutions.insert(entry);
	}
	else
	{
		delete entry;
		
		resolver->nameResolved(name, list<string>(), NRS_FAILED);
	}

	int nextTimeout = dns_timeouts(m_context, -1, 0);
	
	if(nextTimeout >= 0)
	{
		if(m_timeout != TIMEOUT_EMPTY)
			m_timeoutManager->dropTimeout(m_timeout);
			
		m_timeout = m_timeoutManager->scheduleTimeout(nextTimeout, this);
	}
}

void UdnsResolvingFacility::callback(dns_ctx * ctx, struct dns_rr_a4 * result,
	void * data)
{
	((ResolutionEntry *) data)->facility->processResult(result,
		(ResolutionEntry *) data);
}

void UdnsResolvingFacility::timeoutFired(Timeout timeout)
{
	int nextTimeout = dns_timeouts(m_context, -1, 0);
	
	if(nextTimeout >= 0)
		m_timeout = m_timeoutManager->scheduleTimeout(nextTimeout, this);
	else
		m_timeout = TIMEOUT_EMPTY;
}

void UdnsResolvingFacility::processResult(struct dns_rr_a4 * result,
	ResolutionEntry * entry)
{
	m_pendingResolutions.erase(entry);

	if(!result || !result->dnsa4_qname || result->dnsa4_nrr <= 0)
	{
		entry->resolver->nameResolved(entry->domain,
			list<string>(), NRS_HOST_UNKNOWN);
	}
	else	
	{
		list<string> addresses;
		
		for(int i = 0; i < result->dnsa4_nrr; ++i)
			addresses.push_back(string(inet_ntoa(result->dnsa4_addr[i])));
		
		entry->resolver->nameResolved(entry->domain, addresses, NRS_OK);
	}

	delete entry;
}

bool UdnsResolvingFacility::cancelResolutions(NameResolver * resolver)
{
	ResolutionSet::iterator next;
	bool found = false;

	for(ResolutionSet::iterator it = m_pendingResolutions.begin();
		it != m_pendingResolutions.end(); it = next)
	{
		next = it;
		++next;

		if((* it)->resolver == resolver)
		{
			dns_cancel(m_context, (* it)->query);
			delete * it;
			m_pendingResolutions.erase(it);
			
			found = true;
		}
	}
	
	return found;
}


}

#endif // HAVE_LIBUDNS
