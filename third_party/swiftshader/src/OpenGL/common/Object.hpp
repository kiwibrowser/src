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

// Object.hpp: Defines the Object base class that provides
// lifecycle support for GL objects using the traditional BindObject scheme, but
// that need to be reference counted for correct cross-context deletion.

#ifndef gl_Object_hpp
#define gl_Object_hpp

#include "common/debug.h"
#include "Common/MutexLock.hpp"

#include <set>

typedef unsigned int GLuint;

namespace gl
{

class [[clang::lto_visibility_public]] Object
{
public:
	Object();

	virtual void addRef();
	virtual void release();

	inline bool hasSingleReference() const
	{
		return referenceCount == 1;
	}

protected:
	virtual ~Object();

	int dereference();
	void destroy();

	volatile int referenceCount;

#ifndef NDEBUG
public:
	static sw::MutexLock instances_mutex;
	static std::set<Object*> instances;   // For leak checking
#endif
};

class NamedObject : public Object
{
public:
	explicit NamedObject(GLuint name);
	virtual ~NamedObject();

	const GLuint name;
};

template<class ObjectType>
class BindingPointer
{
public:
	BindingPointer() : object(nullptr) { }

	BindingPointer(const BindingPointer<ObjectType> &other) : object(nullptr)
	{
		operator=(other.object);
	}

	~BindingPointer()
	{
		ASSERT(!object);   // Objects have to be released before the resource manager is destroyed, so they must be explicitly cleaned up. Assign null to all binding pointers to make the reference count go to zero.
	}

	ObjectType *operator=(ObjectType *newObject)
	{
		if(newObject) newObject->addRef();
		if(object) object->release();

		object = newObject;

		return object;
	}

	ObjectType *operator=(const BindingPointer<ObjectType> &other)
	{
		return operator=(other.object);
	}

	operator ObjectType*() const { return object; }
	ObjectType *operator->() const { return object; }
	GLuint name() const { return object ? object->name : 0; }
	bool operator!() const { return !object; }

private:
	ObjectType *object;
};

}

#endif   // gl_Object_hpp
