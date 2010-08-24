/*
 * TcpSocket.cpp - IOSocket implementation for TCP Berkley sockets
 * $Id: ModuleManager.cpp 39 2008-03-12 03:18:56Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#else
#error Unknown dynamic link library interface, no dlfcn.h!
#endif

#include <libnetworkd/ModuleManager.hpp>
#include <stdio.h>

namespace libnetworkd
{


ModuleManager::ModuleManager()
{
	m_idCounter = 1;
}

ModuleManager::~ModuleManager()
{
	unloadAll();
}


typedef Module * (* module_init_fp_t)(uint16_t, void *);

uint32_t ModuleManager::loadModule(const char * libraryPath, const char * configuration, void * userData)
{
	ModuleEncapsulation encaps;
	module_init_fp_t initFunction;
	
	if(!(encaps.library = dlopen(libraryPath, RTLD_NOW)))
	{
		static char errorBuffer[256];
		snprintf(errorBuffer, sizeof(errorBuffer) - 1, "Could not load library (`%s').", dlerror());
		throw ((const char *) errorBuffer);
	}

	if(!(initFunction = (module_init_fp_t) dlsym(encaps.library, LIBNETWORKD_MODULE_INITIALIZER_FNSTRING)))
		throw "Could not load symbol " LIBNETWORKD_MODULE_INITIALIZER_FNSTRING " from dynamic link library!";
	
	encaps.configuration = new Configuration();
	
	if(configuration && * configuration)
	{		
		try
		{
			encaps.configuration->parseFile(configuration);
		}
		catch(const char * err)
		{
			dlclose(encaps.library);
			delete encaps.configuration;
			throw err;
		}
	}
	else
		encaps.configuration = 0;
		
	if(!(encaps.moduleInterface = initFunction(LIBNETWORKD_MODULE_IFACE_VERSION, userData)))
	{
		dlclose(encaps.library);
		delete encaps.configuration;
		throw "Creation of module instance failed!";
	}
	
	if(!encaps.moduleInterface->start(encaps.configuration))
	{
		dlclose(encaps.library);
		delete encaps.configuration;
		throw "Module failed to start up!";
	}
	
	encaps.moduleId = m_idCounter;
	++m_idCounter;
	
	m_modules.push_back(encaps);
	return encaps.moduleId;
}

bool ModuleManager::unloadModule(ModuleEncapsulation * module, bool force)
{	
	if(!module->moduleInterface->stop() && !force)
		return false;
		
	delete module->configuration;
	delete module->moduleInterface;
//	dlclose(module->library);
	
	return true;
}

bool ModuleManager::unloadModule(uint32_t id, bool force)
{
	for(list<ModuleEncapsulation>::iterator i = m_modules.begin(); i != m_modules.end(); ++i)
		if(i->moduleId == id)
		{
			bool clean = unloadModule(&(* i), force);
			m_modules.erase(i);
			
			return clean;
		}
			
	return false;
}

bool ModuleManager::unloadAll()
{
	bool clean = true;
	
	for(list<ModuleEncapsulation>::iterator i = m_modules.begin(); i != m_modules.end();)
	{
		if(!unloadModule(&(* i), false))
		{
			++i;
			clean = false;
		}
		else
			i = m_modules.erase(i);
	}
	
	return clean;
}

void ModuleManager::enumerateModules(list<ModuleEncapsulation> * moduleList)
{
	* moduleList = m_modules;
}

}
