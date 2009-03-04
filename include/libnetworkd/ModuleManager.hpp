/*
 * ModuleManager.hpp - dynamic extension module loader and manager
 * $Id: ModuleManager.hpp 39 2008-03-12 03:18:56Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */


#ifndef __INCLUDE_libnetworkd_ModuleManager_hpp
#define __INCLUDE_libnetworkd_ModuleManager_hpp

#include <stdint.h>
#include <unistd.h>

#include <list>
using namespace std;

#include "libnetworkd.hpp"



#define LIBNETWORKD_MODULE_IFACE_VERSION 1

#define LIBNETWORKD_MODULE_INITIALIZER_FNNAME	__libnetworkd_init_module
#define LIBNETWORKD_MODULE_INITIALIZER_FNSTRING	"__libnetworkd_init_module"
#define LIBNETWORKD_MODULE_INITIALIZER			libnetworkd::Module * LIBNETWORKD_MODULE_INITIALIZER_FNNAME(uint16_t libnetworkdVersion, void * userData)
#define EXPORT_LIBNETWORKD_MODULE(a, b)			extern "C" { \
	LIBNETWORKD_MODULE_INITIALIZER { \
	if(libnetworkdVersion != LIBNETWORKD_MODULE_IFACE_VERSION) { return 0; } \
	return (libnetworkd::Module *) new a((b) userData); \
	} }


namespace libnetworkd
{

/**
* Abstract extension module interface, usually instanciated by the modules' C
* wrapper functions and called by the core upon module initialization and
* shutdown.
*/
class Module
{
public:
	virtual ~Module() { }

	/**
	* Ask the module to start its service by registering with necessary
	* core managers and such functionality. Provides the module with
	* a reference to the core daemon and an already parsed configuration.
	* @param[in]	moduleConfiguration	Pre-parsed configuration.
	* @param[in]	userdefined data from the daemon, mostly a set of pointers
	*	to other libnetworkd specific classes.
	* @return Returns true if the module was started successfully or false
	*	if startup failed.
	*/
	virtual bool start(Configuration * configuration) = 0;
	
	/**
	* Ask the module to gracefully stop its service by unregistering from
	* all core managers and stopping in-progress operations. The module is
	* afterwards unloaded regardless of the return value, but a warning may
	* be printed to indicate ungraceful shutdown.
	* @return True if the module could stop gracefully or false if the stop
	*	had to be enforced.
	*/
	virtual bool stop() = 0;
	
	/**
	* Provides a short, one-word name for the module.
	* @return A static, valid string pointer to the name.
	*/
	virtual const char * getName() = 0;
	
	/**
	* Provides a single-sentence description of the tasks this module
	* performs or fulfills.
	* @ return A static, valid string pointer to the description.
	*/
	virtual const char * getDescription() = 0;
};


/**
* Structure representing a single module instance, only used internally by the
* ModuleManager and when enumerating loaded modules using
* ModuleManager::enumerateModules.
*/
struct ModuleEncapsulation
{
	//! reference id used for unloading modules
	uint32_t moduleId;
	
	//! pointer to interfacing Module instance
	Module * moduleInterface;
	//! configuration loaded for module, needs to be ::free'd upon unload.
	Configuration * configuration;
	//! dynamic library pointer for module, needs to be ::dlclose'd upon unload.
	void * library;
};


/**
* Manager for loadable extension modules, performing dynamic link library
* loading, parser invocation for modules' configurations and memory management
* of modules' configuration.
*/
class ModuleManager
{
public:
	ModuleManager();
	virtual ~ModuleManager();
	
	/**
	* Load the specified module from the given dynamic link library image
	* and parse the given configuration file for it. Automatically calls
	* Module::start after successfully loading the module.
	* Throws a descriptive error message through a const char * upon failure.
	* @param[in]	libraryPath	Path to module binary image.
	* @param[in]	configuration	Path to configuration file to pass.
	* @param[in]	userData		User data provided from the daemon for the
	*	module.
	* @return An unique integer identifying the loaded module in this
	*	manager for the currently running process.
	*/
	virtual uint32_t loadModule(const char * libraryPath, const char * configuration, void * userData = 0);
	
	/**
	* Stop and cleanly unload the specified module.
	* @param[in]	moduleId		ID of the module to unload,
	*	either obtained by loadModule or enumerateModules.
	* @param[in]	force			Force unloading even if Module::stop failed.
	* @return True if the module was found and succesfully unloaded, false if
	*	the module wasn't known by the ModuleManager or Module::stop failed and
	*	force was set to false.
	*/
	virtual bool unloadModule(uint32_t moduleId, bool force = false);
	
	
	/**
	* Stop and cleanly unload all loaded modules.
	* @return True if all modules could be cleanly unloaded, false otherwise.
	*	They are not forced to be unloaded if Module::stop fails. If a module
	*	fails to unload, unloading is aborted and false returned.
	*/
	virtual bool unloadAll();
	
	/**
	* Enumerate all currently loaded modules.
	* @param[out]	moduleList	A list of all loaded modules.
	*/
	virtual void enumerateModules(list<ModuleEncapsulation> * moduleList);

protected:
	bool unloadModule(ModuleEncapsulation * module, bool force);
	
private:
	list<ModuleEncapsulation> m_modules;
	uint32_t m_idCounter;
};


}

#endif // __INCLUDE_libnetworkd_ModuleManager_hpp
