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

#ifndef sw_MutexLock_hpp
#define sw_MutexLock_hpp

#include "Thread.hpp"

#if defined(__linux__)
// Use a pthread mutex on Linux. Since many processes may use SwiftShader
// at the same time it's best to just have the scheduler overhead.
#include <pthread.h>

namespace sw
{
	class MutexLock
	{
	public:
		MutexLock()
		{
			pthread_mutex_init(&mutex, NULL);
		}

		~MutexLock()
		{
			pthread_mutex_destroy(&mutex);
		}

		bool attemptLock()
		{
			return pthread_mutex_trylock(&mutex) == 0;
		}

		void lock()
		{
			pthread_mutex_lock(&mutex);
		}

		void unlock()
		{
			pthread_mutex_unlock(&mutex);
		}

	private:
		pthread_mutex_t mutex;
	};
}

#else   // !__linux__

#include <atomic>

namespace sw
{
	class BackoffLock
	{
	public:
		BackoffLock()
		{
			mutex = 0;
		}

		bool attemptLock()
		{
			if(!isLocked())
			{
				if(mutex.exchange(true) == false)
				{
					return true;
				}
			}

			return false;
		}

		void lock()
		{
			int backoff = 1;

			while(!attemptLock())
			{
				if(backoff <= 64)
				{
					for(int i = 0; i < backoff; i++)
					{
						nop();
						nop();
						nop();
						nop();
						nop();

						nop();
						nop();
						nop();
						nop();
						nop();

						nop();
						nop();
						nop();
						nop();
						nop();

						nop();
						nop();
						nop();
						nop();
						nop();

						nop();
						nop();
						nop();
						nop();
						nop();

						nop();
						nop();
						nop();
						nop();
						nop();

						nop();
						nop();
						nop();
						nop();
						nop();
					}

					backoff *= 2;
				}
				else
				{
					Thread::yield();

					backoff = 1;
				}
			};
		}

		void unlock()
		{
			mutex.store(false, std::memory_order_release);
		}

		bool isLocked()
		{
			return mutex.load(std::memory_order_acquire);
		}

	private:
		struct
		{
			// Ensure that the mutex variable is on its own 64-byte cache line to avoid false sharing
			// Padding must be public to avoid compiler warnings
			volatile int padding1[16];
			std::atomic<bool> mutex;
			volatile int padding2[15];
		};
	};

	using MutexLock = BackoffLock;
}

#endif   // !__ANDROID__

class LockGuard
{
public:
	explicit LockGuard(sw::MutexLock &mutex) : mutex(mutex)
	{
		mutex.lock();
	}

	~LockGuard()
	{
		mutex.unlock();
	}

protected:
	sw::MutexLock &mutex;
};

#endif   // sw_MutexLock_hpp
