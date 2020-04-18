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

// Object.cpp: Defines the Object base class that provides
// lifecycle support for GL objects using the traditional BindObject scheme, but
// that need to be reference counted for correct cross-context deletion.

#include "Object.hpp"

#include "Common/Thread.hpp"

namespace gl
{
#ifndef NDEBUG
sw::MutexLock Object::instances_mutex;
std::set<Object*> Object::instances;
#endif

Object::Object()
{
	referenceCount = 0;

	#ifndef NDEBUG
		LockGuard instances_lock(instances_mutex);
		instances.insert(this);
	#endif
}

Object::~Object()
{
	ASSERT(referenceCount == 0);

	#ifndef NDEBUG
		LockGuard instances_lock(instances_mutex);
		ASSERT(instances.find(this) != instances.end());   // Check for double deletion
		instances.erase(this);
	#endif
}

void Object::addRef()
{
	sw::atomicIncrement(&referenceCount);
}

void Object::release()
{
	if(dereference() == 0)
	{
		delete this;
	}
}

int Object::dereference()
{
	ASSERT(referenceCount > 0);

	if(referenceCount > 0)
	{
		return sw::atomicDecrement(&referenceCount);
	}

	return 0;
}

void Object::destroy()
{
	referenceCount = 0;
	delete this;
}

NamedObject::NamedObject(GLuint name) : name(name)
{
}

NamedObject::~NamedObject()
{
}

#ifndef NDEBUG
struct ObjectLeakCheck
{
	~ObjectLeakCheck()
	{
		LockGuard instances_lock(Object::instances_mutex);
		ASSERT(Object::instances.empty());   // Check for GL object leak at termination
	}
};

static ObjectLeakCheck objectLeakCheck;
#endif

}
