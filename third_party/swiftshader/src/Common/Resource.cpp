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

#include "Resource.hpp"

#include "Memory.hpp"

namespace sw
{
	Resource::Resource(size_t bytes) : size(bytes)
	{
		blocked = 0;

		accessor = PUBLIC;
		count = 0;
		orphaned = false;

		buffer = allocate(bytes);
	}

	Resource::~Resource()
	{
		deallocate(buffer);
	}

	void *Resource::lock(Accessor claimer)
	{
		criticalSection.lock();

		while(count > 0 && accessor != claimer)
		{
			blocked++;
			criticalSection.unlock();

			unblock.wait();

			criticalSection.lock();
			blocked--;
		}

		accessor = claimer;
		count++;

		criticalSection.unlock();

		return buffer;
	}

	void *Resource::lock(Accessor relinquisher, Accessor claimer)
	{
		criticalSection.lock();

		// Release
		while(count > 0 && accessor == relinquisher)
		{
			count--;

			if(count == 0)
			{
				if(blocked)
				{
					unblock.signal();
				}
				else if(orphaned)
				{
					criticalSection.unlock();

					delete this;

					return 0;
				}
			}
		}

		// Acquire
		while(count > 0 && accessor != claimer)
		{
			blocked++;
			criticalSection.unlock();

			unblock.wait();

			criticalSection.lock();
			blocked--;
		}

		accessor = claimer;
		count++;

		criticalSection.unlock();

		return buffer;
	}

	void Resource::unlock()
	{
		criticalSection.lock();

		count--;

		if(count == 0)
		{
			if(blocked)
			{
				unblock.signal();
			}
			else if(orphaned)
			{
				criticalSection.unlock();

				delete this;

				return;
			}
		}

		criticalSection.unlock();
	}

	void Resource::unlock(Accessor relinquisher)
	{
		criticalSection.lock();

		while(count > 0 && accessor == relinquisher)
		{
			count--;

			if(count == 0)
			{
				if(blocked)
				{
					unblock.signal();
				}
				else if(orphaned)
				{
					criticalSection.unlock();

					delete this;

					return;
				}
			}
		}

		criticalSection.unlock();
	}

	void Resource::destruct()
	{
		criticalSection.lock();

		if(count == 0 && !blocked)
		{
			criticalSection.unlock();

			delete this;

			return;
		}

		orphaned = true;

		criticalSection.unlock();
	}

	const void *Resource::data() const
	{
		return buffer;
	}
}
