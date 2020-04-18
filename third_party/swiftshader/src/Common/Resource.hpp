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

#ifndef sw_Resource_hpp
#define sw_Resource_hpp

#include "MutexLock.hpp"

namespace sw
{
	enum Accessor
	{
		PUBLIC,    // Application/API access
		PRIVATE,   // Renderer access, shared by multiple threads if read-only
		MANAGED,   // Renderer access, shared read/write access if partitioned
		EXCLUSIVE
	};

	class Resource
	{
	public:
		Resource(size_t bytes);

		void destruct();   // Asynchronous destructor

		void *lock(Accessor claimer);
		void *lock(Accessor relinquisher, Accessor claimer);
		void unlock();
		void unlock(Accessor relinquisher);

		const void *data() const;
		const size_t size;

	private:
		~Resource();   // Always call destruct() instead

		MutexLock criticalSection;
		Event unblock;
		volatile int blocked;

		volatile Accessor accessor;
		volatile int count;
		bool orphaned;

		void *buffer;
	};
}

#endif   // sw_Resource_hpp
