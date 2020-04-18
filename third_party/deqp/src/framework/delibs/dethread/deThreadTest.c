/*-------------------------------------------------------------------------
 * drawElements Thread Library
 * ---------------------------
 *
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Thread library tests.
 *//*--------------------------------------------------------------------*/

#include "deThreadTest.h"
#include "deThread.h"
#include "deMutex.h"
#include "deSemaphore.h"
#include "deMemory.h"
#include "deRandom.h"
#include "deAtomic.h"
#include "deThreadLocal.h"
#include "deSingleton.h"
#include "deMemPool.h"
#include "dePoolArray.h"

static void threadTestThr1 (void* arg)
{
	deInt32 val = *((deInt32*)arg);
	DE_TEST_ASSERT(val == 123);
}

static void threadTestThr2 (void* arg)
{
	DE_UNREF(arg);
	deSleep(100);
}

typedef struct ThreadData3_s
{
	deUint8		bytes[16];
} ThreadData3;

static void threadTestThr3 (void* arg)
{
	ThreadData3* data = (ThreadData3*)arg;
	int ndx;

	for (ndx = 0; ndx < (int)DE_LENGTH_OF_ARRAY(data->bytes); ndx++)
		DE_TEST_ASSERT(data->bytes[ndx] == 0);

	for (ndx = 0; ndx < (int)DE_LENGTH_OF_ARRAY(data->bytes); ndx++)
		data->bytes[ndx] = 0xff;
}

static void threadTestThr4 (void* arg)
{
	deThreadLocal tls = *(deThreadLocal*)arg;
	deThreadLocal_set(tls, DE_NULL);
}

#if defined(DE_THREAD_LOCAL)

static DE_THREAD_LOCAL int tls_testVar = 123;

static void tlsTestThr (void* arg)
{
	DE_UNREF(arg);
	DE_TEST_ASSERT(tls_testVar == 123);
	tls_testVar = 104;
	DE_TEST_ASSERT(tls_testVar == 104);
}

#endif

void deThread_selfTest (void)
{
	/* Test sleep & yield. */
	deSleep(0);
	deSleep(100);
	deYield();

	/* Thread test 1. */
	{
		deInt32		val		= 123;
		deBool		ret;
		deThread	thread	= deThread_create(threadTestThr1, &val, DE_NULL);
		DE_TEST_ASSERT(thread);

		ret = deThread_join(thread);
		DE_TEST_ASSERT(ret);

		deThread_destroy(thread);
	}

	/* Thread test 2. */
	{
		deThread	thread	= deThread_create(threadTestThr2, DE_NULL, DE_NULL);
		deInt32		ret;
		DE_TEST_ASSERT(thread);

		ret = deThread_join(thread);
		DE_TEST_ASSERT(ret);

		deThread_destroy(thread);
	}

	/* Thread test 3. */
	{
		ThreadData3	data;
		deThread	thread;
		deBool		ret;
		int			ndx;

		deMemset(&data, 0, sizeof(ThreadData3));

		thread = deThread_create(threadTestThr3, &data, DE_NULL);
		DE_TEST_ASSERT(thread);

		ret = deThread_join(thread);
		DE_TEST_ASSERT(ret);

		for (ndx = 0; ndx < (int)DE_LENGTH_OF_ARRAY(data.bytes); ndx++)
			DE_TEST_ASSERT(data.bytes[ndx] == 0xff);

		deThread_destroy(thread);
	}

	/* Test tls. */
	{
		deThreadLocal	tls;
		deThread		thread;

		tls = deThreadLocal_create();
		DE_TEST_ASSERT(tls);

		deThreadLocal_set(tls, (void*)(deUintptr)0xff);

		thread = deThread_create(threadTestThr4, &tls, DE_NULL);
		deThread_join(thread);
		deThread_destroy(thread);

		DE_TEST_ASSERT((deUintptr)deThreadLocal_get(tls) == 0xff);
		deThreadLocal_destroy(tls);
	}

#if defined(DE_THREAD_LOCAL)
	{
		deThread thread;

		DE_TEST_ASSERT(tls_testVar == 123);
		tls_testVar = 1;
		DE_TEST_ASSERT(tls_testVar == 1);

		thread = deThread_create(tlsTestThr, DE_NULL, DE_NULL);
		deThread_join(thread);
		deThread_destroy(thread);

		DE_TEST_ASSERT(tls_testVar == 1);
		tls_testVar = 123;
	}
#endif
}

static void mutexTestThr1 (void* arg)
{
	deMutex		mutex	= *((deMutex*)arg);

	deMutex_lock(mutex);
	deMutex_unlock(mutex);
}

typedef struct MutexData2_s
{
	deMutex		mutex;
	deInt32		counter;
	deInt32		counter2;
	deInt32		maxVal;
} MutexData2;

static void mutexTestThr2 (void* arg)
{
	MutexData2* data = (MutexData2*)arg;
	deInt32 numIncremented = 0;

	for (;;)
	{
		deInt32 localCounter;
		deMutex_lock(data->mutex);

		if (data->counter >= data->maxVal)
		{
			deMutex_unlock(data->mutex);
			break;
		}

		localCounter = data->counter;
		deYield();

		DE_TEST_ASSERT(localCounter == data->counter);
		localCounter += 1;
		data->counter = localCounter;

		deMutex_unlock(data->mutex);

		numIncremented++;
	}

	deMutex_lock(data->mutex);
	data->counter2 += numIncremented;
	deMutex_unlock(data->mutex);
}

void mutexTestThr3 (void* arg)
{
	deMutex mutex = *((deMutex*)arg);
	deBool	ret;

	ret = deMutex_tryLock(mutex);
	DE_TEST_ASSERT(!ret);
}

void deMutex_selfTest (void)
{
	/* Default mutex from single thread. */
	{
		deMutex mutex = deMutex_create(DE_NULL);
		deBool	ret;
		DE_TEST_ASSERT(mutex);

		deMutex_lock(mutex);
		deMutex_unlock(mutex);

		/* Should succeed. */
		ret = deMutex_tryLock(mutex);
		DE_TEST_ASSERT(ret);
		deMutex_unlock(mutex);

		deMutex_destroy(mutex);
	}

	/* Recursive mutex. */
	{
		deMutexAttributes	attrs;
		deMutex				mutex;
		int					ndx;
		int					numLocks	= 10;

		deMemset(&attrs, 0, sizeof(attrs));

		attrs.flags = DE_MUTEX_RECURSIVE;

		mutex = deMutex_create(&attrs);
		DE_TEST_ASSERT(mutex);

		for (ndx = 0; ndx < numLocks; ndx++)
			deMutex_lock(mutex);

		for (ndx = 0; ndx < numLocks; ndx++)
			deMutex_unlock(mutex);

		deMutex_destroy(mutex);
	}

	/* Mutex and threads. */
	{
		deMutex		mutex;
		deThread	thread;

		mutex = deMutex_create(DE_NULL);
		DE_TEST_ASSERT(mutex);

		deMutex_lock(mutex);

		thread = deThread_create(mutexTestThr1, &mutex, DE_NULL);
		DE_TEST_ASSERT(thread);

		deSleep(100);
		deMutex_unlock(mutex);

		deMutex_lock(mutex);
		deMutex_unlock(mutex);

		deThread_join(thread);

		deThread_destroy(thread);
		deMutex_destroy(mutex);
	}

	/* A bit more complex mutex test. */
	{
		MutexData2	data;
		deThread	threads[2];
		int			ndx;

		data.mutex	= deMutex_create(DE_NULL);
		DE_TEST_ASSERT(data.mutex);

		data.counter	= 0;
		data.counter2	= 0;
		data.maxVal		= 1000;

		deMutex_lock(data.mutex);

		for (ndx = 0; ndx < (int)DE_LENGTH_OF_ARRAY(threads); ndx++)
		{
			threads[ndx] = deThread_create(mutexTestThr2, &data, DE_NULL);
			DE_TEST_ASSERT(threads[ndx]);
		}

		deMutex_unlock(data.mutex);

		for (ndx = 0; ndx < (int)DE_LENGTH_OF_ARRAY(threads); ndx++)
		{
			deBool ret = deThread_join(threads[ndx]);
			DE_TEST_ASSERT(ret);
			deThread_destroy(threads[ndx]);
		}

		DE_TEST_ASSERT(data.counter == data.counter2);
		DE_TEST_ASSERT(data.maxVal == data.counter);

		deMutex_destroy(data.mutex);
	}

	/* tryLock() deadlock test. */
	{
		deThread	thread;
		deMutex		mutex	= deMutex_create(DE_NULL);
		deBool		ret;
		DE_TEST_ASSERT(mutex);

		deMutex_lock(mutex);

		thread = deThread_create(mutexTestThr3, &mutex, DE_NULL);
		DE_TEST_ASSERT(mutex);

		ret = deThread_join(thread);
		DE_TEST_ASSERT(ret);

		deMutex_unlock(mutex);
		deMutex_destroy(mutex);
		deThread_destroy(thread);
	}
}

typedef struct TestBuffer_s
{
	deUint32		buffer[32];
	deSemaphore		empty;
	deSemaphore		fill;

	deUint32		producerHash;
	deUint32		consumerHash;
} TestBuffer;

void producerThread (void* arg)
{
	TestBuffer* buffer = (TestBuffer*)arg;
	deRandom	random;
	int			ndx;
	int			numToProduce	= 10000;
	int			writePos		= 0;

	deRandom_init(&random, 123);

	for (ndx = 0; ndx <= numToProduce; ndx++)
	{
		deUint32 val;

		if (ndx == numToProduce)
		{
			val = 0u; /* End. */
		}
		else
		{
			val = deRandom_getUint32(&random);
			val = val ? val : 1u;
		}

		deSemaphore_decrement(buffer->empty);

		buffer->buffer[writePos] = val;
		writePos = (writePos + 1) % DE_LENGTH_OF_ARRAY(buffer->buffer);

		deSemaphore_increment(buffer->fill);

		buffer->producerHash ^= val;
	}
}

void consumerThread (void* arg)
{
	TestBuffer*	buffer	= (TestBuffer*)arg;
	int			readPos	= 0;

	for (;;)
	{
		deInt32 val;

		deSemaphore_decrement(buffer->fill);

		val = buffer->buffer[readPos];
		readPos = (readPos + 1) % DE_LENGTH_OF_ARRAY(buffer->buffer);

		deSemaphore_increment(buffer->empty);

		buffer->consumerHash ^= val;

		if (val == 0)
			break;
	}
}

void deSemaphore_selfTest (void)
{
	/* Basic test. */
	{
		deSemaphore	semaphore	= deSemaphore_create(1, DE_NULL);
		DE_TEST_ASSERT(semaphore);

		deSemaphore_increment(semaphore);
		deSemaphore_decrement(semaphore);
		deSemaphore_decrement(semaphore);

		deSemaphore_destroy(semaphore);
	}

	/* Producer-consumer test. */
	{
		TestBuffer	testBuffer;
		deThread	producer;
		deThread	consumer;
		deBool		ret;

		deMemset(&testBuffer, 0, sizeof(testBuffer));

		testBuffer.empty	= deSemaphore_create(DE_LENGTH_OF_ARRAY(testBuffer.buffer), DE_NULL);
		testBuffer.fill		= deSemaphore_create(0, DE_NULL);

		DE_TEST_ASSERT(testBuffer.empty && testBuffer.fill);

		consumer	= deThread_create(consumerThread, &testBuffer, DE_NULL);
		producer	= deThread_create(producerThread, &testBuffer, DE_NULL);

		DE_TEST_ASSERT(consumer && producer);

		ret = deThread_join(consumer) &&
			  deThread_join(producer);
		DE_TEST_ASSERT(ret);

		deThread_destroy(producer);
		deThread_destroy(consumer);

		deSemaphore_destroy(testBuffer.empty);
		deSemaphore_destroy(testBuffer.fill);
		DE_TEST_ASSERT(testBuffer.producerHash == testBuffer.consumerHash);
	}
}

void deAtomic_selfTest (void)
{
	/* Single-threaded tests. */
	{
		volatile deInt32 a = 11;
		DE_TEST_ASSERT(deAtomicIncrementInt32(&a) == 12);
		DE_TEST_ASSERT(a == 12);
		DE_TEST_ASSERT(deAtomicIncrementInt32(&a) == 13);
		DE_TEST_ASSERT(a == 13);

		a = -2;
		DE_TEST_ASSERT(deAtomicIncrementInt32(&a) == -1);
		DE_TEST_ASSERT(a == -1);
		DE_TEST_ASSERT(deAtomicIncrementInt32(&a) == 0);
		DE_TEST_ASSERT(a == 0);

		a = 11;
		DE_TEST_ASSERT(deAtomicDecrementInt32(&a) == 10);
		DE_TEST_ASSERT(a == 10);
		DE_TEST_ASSERT(deAtomicDecrementInt32(&a) == 9);
		DE_TEST_ASSERT(a == 9);

		a = 0;
		DE_TEST_ASSERT(deAtomicDecrementInt32(&a) == -1);
		DE_TEST_ASSERT(a == -1);
		DE_TEST_ASSERT(deAtomicDecrementInt32(&a) == -2);
		DE_TEST_ASSERT(a == -2);

		a = 0x7fffffff;
		DE_TEST_ASSERT(deAtomicIncrementInt32(&a) == (int)0x80000000);
		DE_TEST_ASSERT(a == (int)0x80000000);
		DE_TEST_ASSERT(deAtomicDecrementInt32(&a) == (int)0x7fffffff);
		DE_TEST_ASSERT(a == 0x7fffffff);
	}

	{
		volatile deUint32 a = 11;
		DE_TEST_ASSERT(deAtomicIncrementUint32(&a) == 12);
		DE_TEST_ASSERT(a == 12);
		DE_TEST_ASSERT(deAtomicIncrementUint32(&a) == 13);
		DE_TEST_ASSERT(a == 13);

		a = 0x7fffffff;
		DE_TEST_ASSERT(deAtomicIncrementUint32(&a) == 0x80000000);
		DE_TEST_ASSERT(a == 0x80000000);
		DE_TEST_ASSERT(deAtomicDecrementUint32(&a) == 0x7fffffff);
		DE_TEST_ASSERT(a == 0x7fffffff);

		a = 0xfffffffe;
		DE_TEST_ASSERT(deAtomicIncrementUint32(&a) == 0xffffffff);
		DE_TEST_ASSERT(a == 0xffffffff);
		DE_TEST_ASSERT(deAtomicDecrementUint32(&a) == 0xfffffffe);
		DE_TEST_ASSERT(a == 0xfffffffe);
	}

	{
		volatile deUint32 p;

		p = 0;
		DE_TEST_ASSERT(deAtomicCompareExchange32(&p, 0, 1) == 0);
		DE_TEST_ASSERT(p == 1);

		DE_TEST_ASSERT(deAtomicCompareExchange32(&p, 0, 2) == 1);
		DE_TEST_ASSERT(p == 1);

		p = 7;
		DE_TEST_ASSERT(deAtomicCompareExchange32(&p, 6, 8) == 7);
		DE_TEST_ASSERT(p == 7);

		DE_TEST_ASSERT(deAtomicCompareExchange32(&p, 7, 8) == 7);
		DE_TEST_ASSERT(p == 8);
	}

#if (DE_PTR_SIZE == 8)
	{
		volatile deInt64 a = 11;
		DE_TEST_ASSERT(deAtomicIncrementInt64(&a) == 12);
		DE_TEST_ASSERT(a == 12);
		DE_TEST_ASSERT(deAtomicIncrementInt64(&a) == 13);
		DE_TEST_ASSERT(a == 13);

		a = -2;
		DE_TEST_ASSERT(deAtomicIncrementInt64(&a) == -1);
		DE_TEST_ASSERT(a == -1);
		DE_TEST_ASSERT(deAtomicIncrementInt64(&a) == 0);
		DE_TEST_ASSERT(a == 0);

		a = 11;
		DE_TEST_ASSERT(deAtomicDecrementInt64(&a) == 10);
		DE_TEST_ASSERT(a == 10);
		DE_TEST_ASSERT(deAtomicDecrementInt64(&a) == 9);
		DE_TEST_ASSERT(a == 9);

		a = 0;
		DE_TEST_ASSERT(deAtomicDecrementInt64(&a) == -1);
		DE_TEST_ASSERT(a == -1);
		DE_TEST_ASSERT(deAtomicDecrementInt64(&a) == -2);
		DE_TEST_ASSERT(a == -2);

		a = (deInt64)((1ull << 63) - 1ull);
		DE_TEST_ASSERT(deAtomicIncrementInt64(&a) == (deInt64)(1ull << 63));
		DE_TEST_ASSERT(a == (deInt64)(1ull << 63));
		DE_TEST_ASSERT(deAtomicDecrementInt64(&a) == (deInt64)((1ull << 63) - 1));
		DE_TEST_ASSERT(a == (deInt64)((1ull << 63) - 1));
	}
#endif /* (DE_PTR_SIZE == 8) */

	/* \todo [2012-10-26 pyry] Implement multi-threaded tests. */
}

/* Singleton self-test. */

DE_DECLARE_POOL_ARRAY(deThreadArray, deThread);

static volatile	deSingletonState	s_testSingleton				= DE_SINGLETON_STATE_NOT_INITIALIZED;
static volatile int					s_testSingletonInitCount	= 0;
static deBool						s_testSingletonInitialized	= DE_FALSE;
static volatile deBool				s_singletonInitLock			= DE_FALSE;

static void waitForSingletonInitLock (void)
{
	for (;;)
	{
		deMemoryReadWriteFence();

		if (s_singletonInitLock)
			break;
	}
}

static void initTestSingleton (void* arg)
{
	int initTimeMs = *(const int*)arg;

	if (initTimeMs >= 0)
		deSleep((deUint32)initTimeMs);

	deAtomicIncrement32(&s_testSingletonInitCount);
	s_testSingletonInitialized = DE_TRUE;
}

static void singletonTestThread (void* arg)
{
	waitForSingletonInitLock();

	deInitSingleton(&s_testSingleton, initTestSingleton, arg);
	DE_TEST_ASSERT(s_testSingletonInitialized);
}

static void resetTestState (void)
{
	s_testSingleton				= DE_SINGLETON_STATE_NOT_INITIALIZED;
	s_testSingletonInitCount	= 0;
	s_testSingletonInitialized	= DE_FALSE;
	s_singletonInitLock			= DE_FALSE;
}

static void runSingletonThreadedTest (int numThreads, int initTimeMs)
{
	deMemPool*		tmpPool		= deMemPool_createRoot(DE_NULL, 0);
	deThreadArray*	threads		= tmpPool ? deThreadArray_create(tmpPool) : DE_NULL;
	int				threadNdx;

	resetTestState();

	for (threadNdx = 0; threadNdx < numThreads; threadNdx++)
	{
		deThread thread = deThread_create(singletonTestThread, &initTimeMs, DE_NULL);
		DE_TEST_ASSERT(thread);
		DE_TEST_ASSERT(deThreadArray_pushBack(threads, thread));
	}

	/* All threads created - let them do initialization. */
	deMemoryReadWriteFence();
	s_singletonInitLock = DE_TRUE;
	deMemoryReadWriteFence();

	for (threadNdx = 0; threadNdx < numThreads; threadNdx++)
	{
		deThread thread = deThreadArray_get(threads, threadNdx);
		DE_TEST_ASSERT(deThread_join(thread));
		deThread_destroy(thread);
	}

	/* Verify results. */
	DE_TEST_ASSERT(s_testSingletonInitialized);
	DE_TEST_ASSERT(s_testSingletonInitCount == 1);

	deMemPool_destroy(tmpPool);
}

void deSingleton_selfTest (void)
{
	const struct
	{
		int		numThreads;
		int		initTimeMs;
		int		repeatCount;
	} cases[] =
	{
	/*	#threads	time	#repeat	*/
		{ 1,		-1,		5	},
		{ 1,		1,		5	},
		{ 2,		-1,		20	},
		{ 2,		1,		20	},
		{ 4,		-1,		20	},
		{ 4,		1,		20	},
		{ 4,		5,		20	}
	};
	int caseNdx;

	for (caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(cases); caseNdx++)
	{
		int		numThreads		= cases[caseNdx].numThreads;
		int		initTimeMs		= cases[caseNdx].initTimeMs;
		int		repeatCount		= cases[caseNdx].repeatCount;
		int		subCaseNdx;

		for (subCaseNdx = 0; subCaseNdx < repeatCount; subCaseNdx++)
			runSingletonThreadedTest(numThreads, initTimeMs);
	}
}
