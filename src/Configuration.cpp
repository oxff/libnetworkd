/*
 * Configuration.cpp -- yacc & lex based configuration parser
 * $Id: Configuration.cpp 3 2007-11-02 16:15:39Z oxff $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */
 
#include <libnetworkd/Configuration.hpp>
#include <stdlib.h>


namespace libnetworkd
{


Configuration::Configuration()
{
	m_rootNode = 0;
}

Configuration::~Configuration()
{
	if(m_rootNode)
		freeNodeAndChildren(m_rootNode);
}


void Configuration::freeNodeAndChildren(ConfigurationNode * node)
{
	if(node->nodeType == CFGNT_SECTION)
	{
		for(vector<ConfigurationNode *>::iterator i = node->data.children.begin(); i != node->data.children.end(); ++i)
			freeNodeAndChildren(* i);
			
		node->data.children.clear();
	}
	else if(node->nodeType == CFGNT_VALUE)
		node->data.value.erase();
	else if(node->nodeType == CFGNT_LIST)
		node->data.valueList.clear();
	
	delete node;
}

Configuration::ConfigurationNode * Configuration::traversePath(const char * nodePath)
{
	ConfigurationNode * current = m_rootNode;
	string name;
	
	if(* nodePath == ':')
		++nodePath;
		
	while(* nodePath)
	{
		if(* nodePath == ':')
		{
			bool found = false;
			++nodePath;
			
			for(vector<ConfigurationNode *>::iterator i = current->data.children.begin(); i != current->data.children.end(); ++i)
				if((* i)->nodeName == name)
				{
					current = * i;
					found = true;
					
					break;
				}
				
			if(!found)
				return 0;
				
			name.erase();
		}
		else
		{
			name.append(nodePath, 1);
			++nodePath;
		}
	}
	
	if(!name.empty())	
	{
		bool found = false;
		++nodePath;
		
		for(vector<ConfigurationNode *>::iterator i = current->data.children.begin(); i != current->data.children.end(); ++i)
			if((* i)->nodeName == name)
			{
				current = * i;
				found = true;
				
				break;
			}
			
		if(!found)
			return 0;
	}
	
	return current;
}


const char * Configuration::getString(const char * valuePath, const char * defaultValue)
{
	ConfigurationNode * valueNode = traversePath(valuePath);
	
	if(!valueNode || valueNode->nodeType != CFGNT_VALUE)
		return defaultValue;
		
	return valueNode->data.value.c_str();
}

uint32_t Configuration::getInteger(const char * valuePath, uint32_t defaultValue)
{
	ConfigurationNode * valueNode = traversePath(valuePath);
	
	if(!valueNode || valueNode->nodeType != CFGNT_VALUE)
		return defaultValue;
		
	return strtoul(valueNode->data.value.c_str(), 0, 0);
}


ConfigurationNodeType Configuration::nodeType(const char * nodePath)
{
	ConfigurationNode * node = traversePath(nodePath);
	
	if(!node)
		return CFGNT_NONE;
		
	return node->nodeType;
}

vector<string> Configuration::getStringList(const char * listPath)
{
	ConfigurationNode * node = traversePath(listPath);
	
	if(!node)
		throw ((const char *) "List node not found!");
		
	if(node->nodeType != CFGNT_LIST)
		throw ((const char *) "Node is not of list type!");
		
	return node->data.valueList;
}

vector<string> Configuration::enumerateSubkeys(const char * keyPath)
{
	ConfigurationNode * node = traversePath(keyPath);
	
	if(!node)
		throw ((const char *) "List node not found!");
		
	if(node->nodeType != CFGNT_SECTION)
		throw ((const char *) "Node is not of section type!");
		
	vector<string> resultList;
	
	for(vector<ConfigurationNode *>::iterator i = node->data.children.begin(); i != node->data.children.end(); ++i)
		resultList.push_back((* i)->nodeName);
		
	return resultList;
}

bool Configuration::hasSubkeys(const char * keyPath)
{
	ConfigurationNode * node = traversePath(keyPath);
	
	if(!node || node->nodeType != CFGNT_SECTION)
		return false;
		
	return !(node->data.children.empty());
}


}
