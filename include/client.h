/****************************************************************
 *
 * Copyright (C) 2012 Alessandro Pignotti <alessandro@leaningtech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 ***************************************************************/

#ifndef _DUETTO_CLIENT_a335cd00
#define _DUETTO_CLIENT_a335cd00

#include "duetto/types.h"
#include "duetto/clientlib.h"

#include <utility>

namespace client
{

extern Document document;

template<typename T>
String* serialize(const T& data)
{
	return data.serialize();
}

inline String* serialize(int data) [[client]]
{
	return JSON.stringify(data);
}

inline String* serialize(float data) [[client]]
{
	return JSON.stringify(data);
}

//TODO: add generic deserializer
template<typename T>
T deserialize(const String& s)
{
	return T::deserialize(s);
}

template<>
inline int deserialize<int>(const String& s) [[client]]
{
	Object* ret=JSON.parse(s);
	//TODO: Find a proper way to check for type
	return *ret;
}

//TODO: add meaningful error messages when the deserializer is missing
template<>
inline void deserialize<void>(const String& s) [[client]]
{
	return;
}


template<class Serialize, typename ...Args>
struct argumentSerializer
{
	static String* executeImpl(const Serialize& s, Args... args) [[client]]
	{
		String* ret=serialize(s)->concat(",");
		argumentSerializer<Args...> downSerializer;
		return ret->concat(downSerializer.executeImpl(std::forward<Args>(args)...));
	}
	static String* execute(const Serialize& s, Args... args) [[client]]
	{
		//We must return an array
		String* ret=new String("[");
		ret=ret->concat(executeImpl(s,std::forward<Args>(args)...));
		return ret->concat("]");
	}
};

template<class Serialize>
struct argumentSerializer<Serialize>
{
	static String* executeImpl(const Serialize& s) [[client]]
	{
		return serialize(s);
	}
	static String* execute(const Serialize& s) [[client]]
	{
		//We must return an array
		String* ret=new String("[");
		ret=ret->concat(executeImpl(s));
		return ret->concat("]");
	}
};

class Callback: public EventListener
{
public:
	Callback(void (*func)()) throw();
	template<typename Sig>
	Callback(Sig func):Callback((void (*)())func)
	{
	}
};

}

template<typename Ret, typename ...Args>
Ret clientStubImpl(const char* funcName, Args... args) [[client]]
{
	client::argumentSerializer<Args...> serializer;
	client::String* data=serializer.execute(std::forward<Args>(args)...);
	client::XMLHttpRequest* r=new client::XMLHttpRequest();
	client::String* url=new client::String("/duetto_call?f=");
	url=url->concat(funcName,"&a=",*data);
	r->open("GET",*url,false);
	r->send();
	return client::deserialize<Ret>(r->get_responseText());
}

template<typename Ret>
Ret clientStubImpl(const char* funcName) [[client]]
{
	client::XMLHttpRequest* r=new client::XMLHttpRequest();
	client::String* url=new client::String("/duetto_call?f=");
	url=url->concat(funcName,"&a=[]");
	r->open("GET",*url,false);
	r->send();
	return client::deserialize<Ret>(r->get_responseText());
}

template<typename Ret, typename ...Args>
Ret clientStub(const char* funcName, Args... args) [[client]]
{
	return clientStubImpl<Ret, Args...>(funcName, std::forward<Args>(args)...);
}

#endif
