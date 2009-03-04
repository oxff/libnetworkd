/*
 * Configuration.hpp - flexible configuration class, including yacc based parser
 * $Id: Configuration.hpp 3 2007-11-02 16:15:39Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */
 

#ifndef __INCLUDE_libnetworkd_Configuration_hpp
#define __INCLUDE_libnetworkd_Configuration_hpp

#include <stdint.h>

#include <string>
#include <list>

using namespace std;

namespace libnetworkd
{


//! Internal use only.
enum ConfigurationNodeType
{
	CFGNT_NONE,
	CFGNT_SECTION,
	CFGNT_VALUE,
	CFGNT_LIST,
};

//! Internal use only.
struct ConfigurationNode
{
	ConfigurationNodeType nodeType;
	
	struct
	{
		list<ConfigurationNode *> children;
		string value;
		list<string> valueList;
	} data;
	
	string nodeName;
	
	// temporarily used for parsing
	ConfigurationNode * backlink;
};

/**
* Representation of daemon's / module's configuration parsed from a file. In a
* normal case, the ModuleManager allocates and de-allocates a Configuration
* instance for all modules, hence only the get-functions should be of interest
* to module writers.
*
* Value paths are of the format `section:subsection:value'. An additional colon
* may be prepended to the whole path. The the config file format for a detailed
* describtion of sections and values.
*/
class Configuration
{
public:
	Configuration();
	virtual ~Configuration();
	
	/**
	* Parse the configuration from a file. This function is usually only
	* called by the core.
	* @param[in]	filePath	The path to the file to be parsed.
	* @throws	char*	Human readable error description.
	*/
	void parseFile(const char * filePath);
	
	/**
	* Obtain a configuration value as a string.
	* @param[in]	valuePath	Value's path in the file.
	* @param[in]	defaultValue	Value returned if the path was not
	*						found.
	* @return A string pointer to the value, valid until the Configuration
	*	object is destroyed or a new file is parsed.						
	*/
	virtual const char * getString(const char * valuePath, const char * defaultValue);
	
	/**
	* Obtain a configuration value as an integer.
	* @param[in]	valuePath	Value's path in the file.
	* @param[in]	defaultValue	Value returned if the path was not
	*						found.
	* @return The integer representation of the value.						
	*/
	virtual uint32_t getInteger(const char * valuePath, uint32_t defaultValue);
	
	/**
	* Obtain a configuration value list as string list.
	* @param[in]	valuePath	Value's path in the file.
	* @return A list of string pointers representing the values of the
	*	list. If the value cannot be found, an empty list is returned.				
	*/
	virtual list<string> getStringList(const char * listPath);
	
	
	/**
	* List all subkeys (subsections or values) for a given section path. A
	* single colon or an empty string indicate the root section.
	* @param[in]	keyPath	Path to enumerate the children for.
	* @return A list to string pointers containing the childrens' names,
	*	valid until the Configuration object is destroyed or a new file
	*	is parsed.
	*/
	virtual list<string> enumerateSubkeys(const char * keyPath);
	
	/**
	* Check presence of any subkeys (sections or values).
	* @param[in]	keyPath	Path of section or value to check.
	* @return Boolean indicating presence of subkeys.
	*/
	virtual bool hasSubkeys(const char * keyPath);
	
	/**
	* Get type of node specified by the given path.
	* @param[in]	nodePath	Path to the node to check.
	* @return Returns the node type or CFGNT_NONE if the node does not
	*	exist.
	*/
	virtual ConfigurationNodeType nodeType(const char * nodePath);
	
protected:
	//! Free node structure and it's children structures.
	void freeNodeAndChildren(ConfigurationNode * node);
	
	ConfigurationNode * traversePath(const char * path);

private:
	//! The root node of configuration.
	ConfigurationNode * m_rootNode;
};


}

#endif // __INCLUDE_libnetworkd_Configuration_hpp
