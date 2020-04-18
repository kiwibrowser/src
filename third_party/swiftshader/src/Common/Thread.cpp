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

#include "Thread.hpp"

namespace sw
{
	Thread::Thread(void (*threadFunction)(void *parameters), void *parameters)
	{
		Event init;
		Entry entry = {threadFunction, parameters, &init};

		#if defined(_WIN32)
			handle = CreateThread(NULL, 1024 * 1024, startFunction, &entry, 0, NULL);
		#else
			pthread_create(&handle, NULL, startFunction, &entry);
		#endif

		init.wait();
	}

	Thread::~Thread()
	{
		join();   // Make threads exit before deleting them to not block here
	}

	void Thread::join()
	{
		if(!hasJoined)
		{
			#if defined(_WIN32)
				WaitForSingleObject(handle, INFINITE);
				CloseHandle(handle);
			#else
				pthread_join(handle, NULL);
			#endif

			hasJoined = true;
		}
	}

	#if defined(_WIN32)
		unsigned long __stdcall Thread::startFunction(void *parameters)
		{
			Entry entry = *(Entry*)parameters;
			entry.init->signal();
			entry.threadFunction(entry.threadParameters);
			return 0;
		}
	#else
		void *Thread::startFunction(void *parameters)
		{
			Entry entry = *(Entry*)parameters;
			entry.init->signal();
			entry.threadFunction(entry.threadParameters);
			return nullptr;
		}
	#endif

	Event::Event()
	{
		#if defined(_WIN32)
			handle = CreateEvent(NULL, FALSE, FALSE, NULL);
		#else
			pthread_cond_init(&handle, NULL);
			pthread_mutex_init(&mutex, NULL);
			signaled = false;
		#endif
	}

	Event::~Event()
	{
		#if defined(_WIN32)
			CloseHandle(handle);
		#else
			pthread_cond_destroy(&handle);
			pthread_mutex_destroy(&mutex);
		#endif
	}
}
