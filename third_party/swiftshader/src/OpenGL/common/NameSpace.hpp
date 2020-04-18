// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// NameSpace.h: Defines the NameSpace class, which is used to
// allocate GL object names.

#ifndef gl_NameSpace_hpp
#define gl_NameSpace_hpp

#include "Object.hpp"
#include "debug.h"

#include <map>

namespace gl
{

template<class ObjectType, GLuint baseName = 1>
class NameSpace
{
public:
	NameSpace() : freeName(baseName)
	{
	}

	~NameSpace()
	{
		ASSERT(empty());
	}

	bool empty()
	{
		return map.empty();
	}

	GLuint firstName()
	{
		return map.begin()->first;
	}

	GLuint lastName()
	{
		return map.rbegin()->first;
	}

	GLuint allocate(ObjectType *object = nullptr)
	{
		GLuint name = freeName;

		while(isReserved(name))
		{
			name++;
		}

		map.insert({name, object});
		freeName = name + 1;

		return name;
	}

	bool isReserved(GLuint name) const
	{
		return map.find(name) != map.end();
	}

	void insert(GLuint name, ObjectType *object)
	{
		map[name] = object;

		if(name == freeName)
		{
			freeName++;
		}
	}

	ObjectType *remove(GLuint name)
	{
		auto element = map.find(name);

		if(element != map.end())
		{
			ObjectType *object = element->second;
			map.erase(element);

			if(name < freeName)
			{
				freeName = name;
			}

			return object;
		}

		return nullptr;
	}

	ObjectType *find(GLuint name) const
	{
		auto element = map.find(name);

		if(element == map.end())
		{
			return nullptr;
		}

		return element->second;
	}

private:
	typedef std::map<GLuint, ObjectType*> Map;
	Map map;

	GLuint freeName;   // Lowest known potentially free name
};

}

#endif   // gl_NameSpace_hpp
