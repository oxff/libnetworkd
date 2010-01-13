/*
 * Event.hpp - inter module communication packet
 * $Id: Event.hpp 6 2007-11-23 05:07:24Z lostrace $
 *
 * This code is distributed governed by the terms listed in the LICENSE file in
 * the top directory of this source package.
 *
 * (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>
 *
 */


#ifndef __INCLUDE_libnetworkd_Event_hpp
#define __INCLUDE_libnetworkd_Event_hpp

#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

#include <algorithm>
#include <string>
using namespace std;

#include <tr1/unordered_map>
using namespace std::tr1;

#include "LogManager.hpp"


namespace libnetworkd
{


class Event;


class EventSerializationException
{
public:
	EventSerializationException()
	{
		m_failingAttribute = string("<unspecified>");
	}
	
	EventSerializationException(string failingAttribute)
	{
		m_failingAttribute = failingAttribute;
	}
	
	string getFailingAttribute()
	{
		return m_failingAttribute;
	}
	
	string toString()
	{
		return ("Failed to serialize event attribute \"" + m_failingAttribute + "\"; it is a pointer!");
	}
	
private:
	string m_failingAttribute;	
};


enum EventAttributeType
{
	EVENT_AT_EMPTY,
	EVENT_AT_INTEGER,
	EVENT_AT_STRING,
	EVENT_AT_POINTER,
};

class EventAttribute
{
public:
	EventAttribute()
	{
		m_attributeType = EVENT_AT_EMPTY;
	}
	
	void operator=(uint32_t value)
	{
		m_attributeType = EVENT_AT_INTEGER;
		m_integerValue = value;
	}
		
	void operator=(string value)
	{
		m_attributeType = EVENT_AT_STRING;
		m_stringValue = value;
	}
	
	void operator=(void * value)
	{
		if(value != NULL)
		{
			m_attributeType = EVENT_AT_POINTER;
			m_pointerValue = value;
		}
		else
			m_attributeType = EVENT_AT_EMPTY;
	}
	
	void clear()
	{
		m_attributeType = EVENT_AT_EMPTY;
	}
	
	
	EventAttributeType getType() const
	{
		return m_attributeType;
	}
	
	uint32_t getIntegerValue() const
	{
		if(m_attributeType == EVENT_AT_INTEGER)
			return m_integerValue;
		else
			return 0;
	}
	
	const string getStringValue() const
	{
		if(m_attributeType == EVENT_AT_STRING)
			return m_stringValue;
		else if(m_attributeType == EVENT_AT_EMPTY)
			return string("<empty>");
		else
		{
			char buffer[15];

			if(m_attributeType == EVENT_AT_INTEGER)
				snprintf(buffer, sizeof(buffer) - 1, "%i", m_integerValue);
			else
				snprintf(buffer, sizeof(buffer) - 1, "%p", m_pointerValue);
				
			buffer[sizeof(buffer) - 1] = 0;
				
			return string(buffer);
		}
	}

	string toString() const
	{		
		if(m_attributeType == EVENT_AT_STRING)
		{
			string rendered = m_stringValue;

			if(rendered.size() > 64)
				rendered.erase(64);

			transform(rendered.begin(), rendered.end(), rendered.begin(), sanitizeNonAscii);
			return rendered;
		}

		return getStringValue();
	}
	
	inline const string operator*() const
	{
		return getStringValue();
	}
	
	void * getPointerValue() const
	{
		if(m_attributeType == EVENT_AT_POINTER)
			return m_pointerValue;
		else
			return 0;
	}
	
protected:
	friend class Event;

	static char sanitizeNonAscii(char in)
	{
		if(!isprint(in) || in == '\n' || in == '\r')
			return '.';

		return in;
	}
	
	inline string serialize()
	{
		string serialization;
		uint32_t integer;
		
		switch(m_attributeType)
		{
		default:
		case EVENT_AT_EMPTY:
			serialization = string("e");
			break;
			
		case EVENT_AT_INTEGER:
			integer = htonl(m_integerValue);
			serialization = string("i");
			serialization.append((const char *) &integer, sizeof(integer));
			break;
			
		case EVENT_AT_STRING:
			integer = htonl(m_stringValue.size());
			serialization = string("s");
			serialization.append((const char *) &integer, sizeof(integer));
			serialization += m_stringValue;
			break;
			
		case EVENT_AT_POINTER:
			throw (void *) 0;
		}
	}
	
	inline bool unserialize(string& buffer)
	{		
		switch(buffer[0])
		{
		case 'e':
			m_attributeType = EVENT_AT_EMPTY;
			buffer.erase(0, 1);
			return true;
			
		case 'i':
			buffer.erase(0, 1);
			
			if(buffer.size() < sizeof(m_integerValue))
				return false;
			
			m_integerValue = * (uint32_t *) buffer.data();
			buffer.erase(0, sizeof(m_integerValue));
			
			m_attributeType = EVENT_AT_INTEGER;	
			return true;
		
		case 's':
			buffer.erase(0, 1);
			
			if(buffer.size() < sizeof(m_integerValue))
				return false;
				
			// abused unused value member to hold string length
			m_integerValue = * (uint32_t *) buffer.data();
			buffer.erase(0, sizeof(m_integerValue));
			
			if(buffer.size() < m_integerValue)
				return false;
				
			m_stringValue = buffer.substr(0, m_integerValue);
			buffer.erase(0, m_integerValue);
			
			m_attributeType = EVENT_AT_STRING;			
			return true;
			
		default:
			return false;
		}
	}
	
private:	
	uint32_t m_integerValue;
	string m_stringValue;
	void * m_pointerValue;

	EventAttributeType m_attributeType;
};


class Event
{
public:
	inline Event()
	{
		bzero(m_uid, sizeof(m_uid));
	}
	
	inline Event(string eventName)
	{
		bzero(m_uid, sizeof(m_uid));
		m_eventName = eventName;
		
		gettimeofday((struct timeval *) &m_uid[1], 0);
		m_uid[0] = ++ m_incrementing; 
	}
	
	
	inline EventAttribute& operator[](const char * attributeName)
	{
		return m_attributes[string(attributeName)];
	}
	
	inline EventAttribute& operator[](string attributeName)
	{
		return m_attributes[attributeName];
	}
	
	inline void remove(const char * attributeName)
	{
		m_attributes.erase(string(attributeName));
	}
	
	inline void remove(string attributeName)
	{
		m_attributes.erase(attributeName);
	}
	
	
	inline string getName()
	{
		return m_eventName;
	}
	
	inline string operator*()
	{
		return m_eventName;
	}
	
	
	string serialize()
	{
		uint32_t integer;
		string buffer = string((char *) m_uid, sizeof(m_uid));
		
		integer = htonl(m_eventName.size());
		buffer.append((const char *) &integer, sizeof(integer));
		buffer += m_eventName;
		
		integer = htonl(m_attributes.size());
		buffer.append((const char *) &integer, sizeof(integer));
		
		for(AttributeMap::iterator i = m_attributes.begin(); i != m_attributes.end(); ++i)
		{
			try
			{
				integer = htonl(i->first.size());
				buffer.append((const char *) &integer, sizeof(integer));
				
				buffer += i->second.serialize();
			}
			catch(void * e)
			{
				throw EventSerializationException(i->first);
			}
		}
		
		return buffer;
	}
	
	bool unserialize(string buffer)
	{
		uint32_t integer, count;
		
		if(buffer.size() < sizeof(m_uid))
			return false;
			
		memcpy(m_uid, buffer.data(), sizeof(m_uid));
		buffer.erase(0, sizeof(m_uid));
		
		if(buffer.size() < sizeof(integer))
			return false;
		
		integer = ntohl(* (uint32_t *) buffer.data());
		buffer.erase(0, sizeof(integer));
		
		if(buffer.size() < integer + sizeof(integer))
			return false;
			
		m_eventName = buffer.substr(0, integer);
		buffer.erase(0, integer);
		
		count = ntohl(* (uint32_t *) buffer.data());
		buffer.erase(0, sizeof(count));
		
		for(uint32_t i = 0; i < count; ++i)
		{
			EventAttribute attribute;
			string attributeName;
			
			if(buffer.size() < sizeof(integer))
				return false;
				
			integer = ntohl(* (uint32_t *) buffer.data());
			buffer.erase(0, sizeof(integer));
			
			if(buffer.size() < integer + 1)
				return false;
				
			attributeName = buffer.substr(0, integer);
			buffer.erase(0, integer);
			
			if(!attribute.unserialize(buffer))
				return false;
				
			m_attributes[attributeName] = attribute;
		}
		
		return true;
	}
	
	inline const uint8_t * getUid()
	{
		return m_uid;
	}
	
	string toString()
	{
		char hexUid[17];
		string event = "[\"" + m_eventName + "\":";
		
		for(unsigned int i = 0; i < sizeof(m_uid); ++i)
			sprintf(&hexUid[i * 2], "%02x", m_uid[i]);
		
		hexUid[16] = 0;
		
		event += string(hexUid) + "] { ";
		
		for(AttributeMap::iterator i = m_attributes.begin();
			i != m_attributes.end(); ++i)
		{
			event += i->first + " = \"" + i->second.toString() + "\", ";
		}
		
		event.erase(event.size() - 2);
		event += " }";
		
		return event;
	}
	
	typedef unordered_map<string, EventAttribute> AttributeMap;

	inline const AttributeMap& getAttributes()
	{ return m_attributes; }

	inline bool hasAttribute(const char * name)
	{ return m_attributes.find(name) != m_attributes.end(); }
	
private:
	AttributeMap m_attributes;
	string m_eventName;

#ifndef MAX
# define MAX(x,y) ((x) < (y) ? (y) : (x))
#endif

	uint8_t m_uid[sizeof(struct timeval) + 1];

	static uint8_t m_incrementing;
};


}

#endif // __INCLUDE_libnetworkd_Event_hpp
