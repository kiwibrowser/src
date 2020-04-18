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

#ifndef sw_Thread_hpp
#define sw_Thread_hpp

#if defined(_WIN32)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
	#include <intrin.h>
#else
	#include <pthread.h>
	#include <sched.h>
	#include <unistd.h>
	#define TLS_OUT_OF_INDEXES (pthread_key_t)(~0)
#endif

#include <stdlib.h>

#if defined(__clang__)
#if __has_include(<atomic>) // clang has an explicit check for the availability of atomic
#define USE_STD_ATOMIC 1
#endif
// atomic is available in C++11 or newer, and in Visual Studio 2012 or newer
#elif (defined(_MSC_VER) && (_MSC_VER >= 1700)) || (__cplusplus >= 201103L)
#define USE_STD_ATOMIC 1
#endif

#if USE_STD_ATOMIC
#include <atomic>
#endif

namespace sw
{
	class Event;

	class Thread
	{
	public:
		Thread(void (*threadFunction)(void *parameters), void *parameters);

		~Thread();

		void join();

		static void yield();
		static void sleep(int milliseconds);

		#if defined(_WIN32)
			typedef DWORD LocalStorageKey;
		#else
			typedef pthread_key_t LocalStorageKey;
		#endif

		static LocalStorageKey allocateLocalStorageKey(void (*destructor)(void *storage) = free);
		static void freeLocalStorageKey(LocalStorageKey key);
		static void *allocateLocalStorage(LocalStorageKey key, size_t size);
		static void *getLocalStorage(LocalStorageKey key);
		static void freeLocalStorage(LocalStorageKey key);

	private:
		struct Entry
		{
			void (*const threadFunction)(void *parameters);
			void *threadParameters;
			Event *init;
		};

		#if defined(_WIN32)
			static unsigned long __stdcall startFunction(void *parameters);
			HANDLE handle;
		#else
			static void *startFunction(void *parameters);
			pthread_t handle;
		#endif

		bool hasJoined = false;
	};

	class Event
	{
		friend class Thread;

	public:
		Event();

		~Event();

		void signal();
		void wait();

	private:
		#if defined(_WIN32)
			HANDLE handle;
		#else
			pthread_cond_t handle;
			pthread_mutex_t mutex;
			volatile bool signaled;
		#endif
	};

	#if PERF_PROFILE
	int64_t atomicExchange(int64_t volatile *target, int64_t value);
	#endif

	int atomicExchange(int volatile *target, int value);
	int atomicIncrement(int volatile *value);
	int atomicDecrement(int volatile *value);
	int atomicAdd(int volatile *target, int value);
	void nop();
}

namespace sw
{
	inline void Thread::yield()
	{
		#if defined(_WIN32)
			Sleep(0);
		#elif defined(__APPLE__)
			pthread_yield_np();
		#else
			sched_yield();
		#endif
	}

	inline void Thread::sleep(int milliseconds)
	{
		#if defined(_WIN32)
			Sleep(milliseconds);
		#else
			usleep(1000 * milliseconds);
		#endif
	}

	inline Thread::LocalStorageKey Thread::allocateLocalStorageKey(void (*destructor)(void *storage))
	{
		#if defined(_WIN32)
			return TlsAlloc();
		#else
			LocalStorageKey key;
			pthread_key_create(&key, destructor);
			return key;
		#endif
	}

	inline void Thread::freeLocalStorageKey(LocalStorageKey key)
	{
		#if defined(_WIN32)
			TlsFree(key);
		#else
			pthread_key_delete(key);   // Using an invalid key is an error but not undefined behavior.
		#endif
	}

	inline void *Thread::allocateLocalStorage(LocalStorageKey key, size_t size)
	{
		if(key == TLS_OUT_OF_INDEXES)
		{
			return nullptr;
		}

		freeLocalStorage(key);

		void *storage = malloc(size);

		#if defined(_WIN32)
			TlsSetValue(key, storage);
		#else
			pthread_setspecific(key, storage);
		#endif

		return storage;
	}

	inline void *Thread::getLocalStorage(LocalStorageKey key)
	{
		#if defined(_WIN32)
			return TlsGetValue(key);
		#else
			if(key == TLS_OUT_OF_INDEXES)   // Avoid undefined behavior.
			{
				return nullptr;
			}

			return pthread_getspecific(key);
		#endif
	}

	inline void Thread::freeLocalStorage(LocalStorageKey key)
	{
		free(getLocalStorage(key));

		#if defined(_WIN32)
			TlsSetValue(key, nullptr);
		#else
			pthread_setspecific(key, nullptr);
		#endif
	}

	inline void Event::signal()
	{
		#if defined(_WIN32)
			SetEvent(handle);
		#else
			pthread_mutex_lock(&mutex);
			signaled = true;
			pthread_cond_signal(&handle);
			pthread_mutex_unlock(&mutex);
		#endif
	}

	inline void Event::wait()
	{
		#if defined(_WIN32)
			WaitForSingleObject(handle, INFINITE);
		#else
			pthread_mutex_lock(&mutex);
			while(!signaled) pthread_cond_wait(&handle, &mutex);
			signaled = false;
			pthread_mutex_unlock(&mutex);
		#endif
	}

	#if PERF_PROFILE
	inline int64_t atomicExchange(volatile int64_t *target, int64_t value)
	{
		#if defined(_WIN32)
			return InterlockedExchange64(target, value);
		#else
			int ret;
			__asm__ __volatile__("lock; xchg8 %0,(%1)" : "=r" (ret) :"r" (target), "0" (value) : "memory" );
			return ret;
		#endif
	}
	#endif

	inline int atomicExchange(volatile int *target, int value)
	{
		#if defined(_WIN32)
			return InterlockedExchange((volatile long*)target, (long)value);
		#else
			int ret;
			__asm__ __volatile__("lock; xchgl %0,(%1)" : "=r" (ret) :"r" (target), "0" (value) : "memory" );
			return ret;
		#endif
	}

	inline int atomicIncrement(volatile int *value)
	{
		#if defined(_WIN32)
			return InterlockedIncrement((volatile long*)value);
		#else
			return __sync_add_and_fetch(value, 1);
		#endif
	}

	inline int atomicDecrement(volatile int *value)
	{
		#if defined(_WIN32)
			return InterlockedDecrement((volatile long*)value);
		#else
			return __sync_sub_and_fetch(value, 1);
		#endif
	}

	inline int atomicAdd(volatile int* target, int value)
	{
		#if defined(_WIN32)
			return InterlockedExchangeAdd((volatile long*)target, value) + value;
		#else
			return __sync_add_and_fetch(target, value);
		#endif
	}

	inline void nop()
	{
		#if defined(_WIN32)
			__nop();
		#else
			__asm__ __volatile__ ("nop");
		#endif
	}

	#if USE_STD_ATOMIC
		class AtomicInt
		{
		public:
			AtomicInt() : ai() {}
			AtomicInt(int i) : ai(i) {}

			inline operator int() const { return ai.load(std::memory_order_acquire); }
			inline void operator=(const AtomicInt& i) { ai.store(i.ai.load(std::memory_order_acquire), std::memory_order_release); }
			inline void operator=(int i) { ai.store(i, std::memory_order_release); }
			inline void operator--() { ai.fetch_sub(1, std::memory_order_acq_rel); }
			inline void operator++() { ai.fetch_add(1, std::memory_order_acq_rel); }
			inline int operator--(int) { return ai.fetch_sub(1, std::memory_order_acq_rel) - 1; }
			inline int operator++(int) { return ai.fetch_add(1, std::memory_order_acq_rel) + 1; }
			inline void operator-=(int i) { ai.fetch_sub(i, std::memory_order_acq_rel); }
			inline void operator+=(int i) { ai.fetch_add(i, std::memory_order_acq_rel); }
		private:
			std::atomic<int> ai;
		};
	#else
		class AtomicInt
		{
		public:
			AtomicInt() {}
			AtomicInt(int i) : vi(i) {}

			inline operator int() const { return vi; } // Note: this isn't a guaranteed atomic operation
			inline void operator=(const AtomicInt& i) { sw::atomicExchange(&vi, i.vi); }
			inline void operator=(int i) { sw::atomicExchange(&vi, i); }
			inline void operator--() { sw::atomicDecrement(&vi); }
			inline void operator++() { sw::atomicIncrement(&vi); }
			inline int operator--(int) { return sw::atomicDecrement(&vi); }
			inline int operator++(int) { return sw::atomicIncrement(&vi); }
			inline void operator-=(int i) { sw::atomicAdd(&vi, -i); }
			inline void operator+=(int i) { sw::atomicAdd(&vi, i); }
		private:
			volatile int vi;
		};
	#endif
}

#endif   // sw_Thread_hpp
