 /*
 * NameResolution.hpp - DNS resolution facilities
 * $Id: NameResolution.hpp 14 2008-02-14 19:53:34Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */


#ifndef __INCLUDE_libnetworkd_NameResolution_hpp
#define __INCLUDE_libnetworkd_NameResolution_hpp

#include <list>
#include <string>
#include <ext/hash_set>
using namespace std;
using namespace __gnu_cxx;

#include <sys/socket.h>

#ifdef HAVE_LIBUDNS
#include <udns.h>
#endif

#include "IO.hpp"
#include "TimeoutManager.hpp"


namespace libnetworkd
{


//! Status of a name resolution reported through NameResolver::nameResolved.
enum NameResolutionStatus
{
	//! The name resolution was successful and the address has been resolved.
	NRS_OK,
	//! The name to be resolved was not known by the used name servers.
	NRS_HOST_UNKNOWN,
	//! The resolution query timed out.
	NRS_TIMEOUT,
	//! The resolution query failed for some other reason.
	NRS_FAILED,
};

/**
* The abstract definition of an entity, which wants to resolve names (more
* specifically wants to receive notifactions about the results of a name
* resolution).
*/
class NameResolver
{
public:
	virtual ~NameResolver() { }

	/**
	* Event notification callback, normally called by a NameResolvingFacility,
	* to inform the implementing class about the result of a resultion.
	* @param[in]	name	The name which was resolved (or failed to resolve).
	* @param[in]	address	The address the name was resolved to, if successful.
	*	NULL otherwise.
	* @param[in]	Length of the resolved address or 0 on failure.
	* @param[in]	status	Status indicating the success of the query.
	*/
	virtual void nameResolved(string name, list<string> addresses,
		NameResolutionStatus status) = 0;
};

class NameResolvingFacility
{
public:
	virtual ~NameResolvingFacility() { }

	/**
	* Instruct the resolution facility to issue a new query for a network name.
	* Whether this call is blocking or not depends on the actual underlying
	* implementation, both is possible.
	* @param[in]	name		The name to be querried for.
	* @param[in]	resolver	The resolving entity, which is going to be
	*	informed about the resolution's result.
	*/
	virtual void resolveName(string name, NameResolver * resolver) = 0;
	
	/**
	* Removes all active queries from a non-blocking resolving facility, so call
	* this before you actually delete the NameResolver (the deconstructor is a
	* good place).
	* @param[in]	resolver	The target, which is going to be invalidated and
	*	which's queries should hence be dropped.
	* @return	Returns true on success or false on some error, generally it is
	*	safe to delete the target anyway (this status just indicates whether
	*	removal was clean or forced).
	*/
	virtual bool cancelResolutions(NameResolver * resolver) = 0;
	
	/**
	* Creates a UdnsResolvingFacility if available (libudns was present during
	* libnetworkd build time; linked statically) or a PosixResolvingFacility
	* otherwise. Has a virtual deconstructor, so is safe to `delete'.
	* @param[in]	manager		IO manager used to send async queries.
	* @param[in]	mgr			Timeout manager used to schedule timeouts (DNS)
	*/
	static NameResolvingFacility * createFacility(IOManager * manager,
		TimeoutManager * mgr);
};


/**
* A blocking / synchronous implementation of the NameResolvingFacility class
* using the getaddrinfo libc function. Generally, using this class is not a
* good idea. However, it can be used when libnetworkd was compiled without
* libudns as a dependency.
*/
class PosixResolvingFacility : public NameResolvingFacility
{
public:
	virtual ~PosixResolvingFacility() { }
	
	virtual void resolveName(string name, NameResolver * resolver);
	virtual bool cancelResolutions(NameResolver * resolver) { return true; }
};



/**
* Asynchronous DNS resolver using libudns, only for responses < 4kb (uses UDP
* DNS resolution only). This is the prefered resolver, but libudns isn't
* always available.
*/
class UdnsResolvingFacility : public NameResolvingFacility, public IOSocket,
	public TimeoutReceiver
{
public:
	UdnsResolvingFacility(IOManager * manager, TimeoutManager * mgr);
	virtual ~UdnsResolvingFacility();
	
	virtual void resolveName(string name, NameResolver * resolver);
	virtual bool cancelResolutions(NameResolver * resolver);
	
	virtual void pollRead();
	virtual void pollWrite() { }
	virtual void pollError();
	
	virtual void timeoutFired(Timeout timeout);
	
protected:
	struct ResolutionEntry
	{
		UdnsResolvingFacility * facility;
		NameResolver * resolver;
		string domain;
		
		#ifdef HAVE_LIBUDNS
		dns_query * query;
		#else
		void * query;
		#endif
	};
	
	#ifdef HAVE_LIBUDNS
	static void callback(dns_ctx * ctx, struct dns_rr_a4 * result, void * data);
	#else
	static void callback(void * ctx, void * result, void * data);
	#endif

	void processResult(struct dns_rr_a4 * result, ResolutionEntry * entry);

private:
	#ifdef HAVE_LIBUDNS
	struct dns_ctx * m_context;
	#endif
	
	// A bunch of totally ridicilous hacks because hash<void *> is broken
	// on gcc 4.1.x 64bit, the compiler optimizes all this way
	
	typedef ResolutionEntry * ResolutionReference;
	struct ResolutionHash
    { 
    	size_t operator()(ResolutionReference __x) const
    	{ return (size_t) __x; }
    };
	typedef hash_set<ResolutionReference, ResolutionHash >
		ResolutionSet;
	ResolutionSet m_pendingResolutions;
	
	IOManager * m_ioManager;
	TimeoutManager * m_timeoutManager;
	
	Timeout m_timeout;
};


}

#endif // __INCLUDE_libnetworkd_NameResolution_hpp
