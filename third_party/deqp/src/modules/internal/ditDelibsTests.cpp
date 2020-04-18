/*-------------------------------------------------------------------------
 * drawElements Internal Test Module
 * ---------------------------------
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
 * \brief delibs self-tests.
 *//*--------------------------------------------------------------------*/

#include "ditDelibsTests.hpp"
#include "tcuTestLog.hpp"

// depool
#include "dePoolArray.h"
#include "dePoolHeap.h"
#include "dePoolHash.h"
#include "dePoolSet.h"
#include "dePoolHashSet.h"
#include "dePoolHashArray.h"
#include "dePoolMultiSet.h"

// dethread
#include "deThreadTest.h"
#include "deThread.h"

// deutil
#include "deTimerTest.h"
#include "deCommandLine.h"

// debase
#include "deInt32.h"
#include "deFloat16.h"
#include "deMath.h"
#include "deSha1.h"
#include "deMemory.h"

// decpp
#include "deBlockBuffer.hpp"
#include "deFilePath.hpp"
#include "dePoolArray.hpp"
#include "deRingBuffer.hpp"
#include "deSharedPtr.hpp"
#include "deThreadSafeRingBuffer.hpp"
#include "deUniquePtr.hpp"
#include "deRandom.hpp"
#include "deCommandLine.hpp"
#include "deArrayBuffer.hpp"
#include "deStringUtil.hpp"
#include "deSpinBarrier.hpp"
#include "deSTLUtil.hpp"
#include "deAppendList.hpp"

namespace dit
{

using tcu::TestLog;

class DepoolTests : public tcu::TestCaseGroup
{
public:
	DepoolTests (tcu::TestContext& testCtx)
		: tcu::TestCaseGroup(testCtx, "depool", "depool self-tests")
	{
	}

	void init (void)
	{
		addChild(new SelfCheckCase(m_testCtx, "array",		"dePoolArray_selfTest()",		dePoolArray_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "heap",		"dePoolHeap_selfTest()",		dePoolHeap_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "hash",		"dePoolHash_selfTest()",		dePoolHash_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "set",		"dePoolSet_selfTest()",			dePoolSet_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "hash_set",	"dePoolHashSet_selfTest()",		dePoolHashSet_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "hash_array",	"dePoolHashArray_selfTest()",	dePoolHashArray_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "multi_set",	"dePoolMultiSet_selfTest()",	dePoolMultiSet_selfTest));
	}
};

extern "C"
{
typedef deUint32	(*GetUint32Func)	(void);
}

class GetUint32Case : public tcu::TestCase
{
public:
	GetUint32Case (tcu::TestContext& testCtx, const char* name, const char* description, GetUint32Func func)
		: tcu::TestCase	(testCtx, name, description)
		, m_func		(func)
	{
	}

	IterateResult iterate (void)
	{
		m_testCtx.getLog() << TestLog::Message << getDescription() << " returned " << m_func() << TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	GetUint32Func	m_func;
};

class DethreadTests : public tcu::TestCaseGroup
{
public:
	DethreadTests (tcu::TestContext& testCtx)
		: tcu::TestCaseGroup(testCtx, "dethread", "dethread self-tests")
	{
	}

	void init (void)
	{
		addChild(new SelfCheckCase(m_testCtx, "thread",						"deThread_selfTest()",				deThread_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "mutex",						"deMutex_selfTest()",				deMutex_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "semaphore",					"deSemaphore_selfTest()",			deSemaphore_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "atomic",						"deAtomic_selfTest()",				deAtomic_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "singleton",					"deSingleton_selfTest()",			deSingleton_selfTest));
		addChild(new GetUint32Case(m_testCtx, "total_physical_cores",		"deGetNumTotalPhysicalCores()",		deGetNumTotalPhysicalCores));
		addChild(new GetUint32Case(m_testCtx, "total_logical_cores",		"deGetNumTotalLogicalCores()",		deGetNumTotalLogicalCores));
		addChild(new GetUint32Case(m_testCtx, "available_logical_cores",	"deGetNumAvailableLogicalCores()",	deGetNumAvailableLogicalCores));
	}
};

class DeutilTests : public tcu::TestCaseGroup
{
public:
	DeutilTests (tcu::TestContext& testCtx)
		: tcu::TestCaseGroup(testCtx, "deutil", "deutil self-tests")
	{
	}

	void init (void)
	{
		addChild(new SelfCheckCase(m_testCtx, "timer",			"deTimer_selfTest()",		deTimer_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "command_line",	"deCommandLine_selfTest()",	deCommandLine_selfTest));
	}
};

class DebaseTests : public tcu::TestCaseGroup
{
public:
	DebaseTests (tcu::TestContext& testCtx)
		: tcu::TestCaseGroup(testCtx, "debase", "debase self-tests")
	{
	}

	void init (void)
	{
		addChild(new SelfCheckCase(m_testCtx, "int32",		"deInt32_selfTest()",	deInt32_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "float16",	"deFloat16_selfTest()",	deFloat16_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "math",		"deMath_selfTest()",	deMath_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "sha1",		"deSha1_selfTest()",	deSha1_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "memory",		"deMemory_selfTest()",	deMemory_selfTest));
	}
};

class DecppTests : public tcu::TestCaseGroup
{
public:
	DecppTests (tcu::TestContext& testCtx)
		: tcu::TestCaseGroup(testCtx, "decpp", "decpp self-tests")
	{
	}

	void init (void)
	{
		addChild(new SelfCheckCase(m_testCtx, "block_buffer",				"de::BlockBuffer_selfTest()",			de::BlockBuffer_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "file_path",					"de::FilePath_selfTest()",				de::FilePath_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "pool_array",					"de::PoolArray_selfTest()",				de::PoolArray_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "ring_buffer",				"de::RingBuffer_selfTest()",			de::RingBuffer_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "shared_ptr",					"de::SharedPtr_selfTest()",				de::SharedPtr_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "thread_safe_ring_buffer",	"de::ThreadSafeRingBuffer_selfTest()",	de::ThreadSafeRingBuffer_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "unique_ptr",					"de::UniquePtr_selfTest()",				de::UniquePtr_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "random",						"de::Random_selfTest()",				de::Random_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "commandline",				"de::cmdline::selfTest()",				de::cmdline::selfTest));
		addChild(new SelfCheckCase(m_testCtx, "array_buffer",				"de::ArrayBuffer_selfTest()",			de::ArrayBuffer_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "string_util",				"de::StringUtil_selfTest()",			de::StringUtil_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "spin_barrier",				"de::SpinBarrier_selfTest()",			de::SpinBarrier_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "stl_util",					"de::STLUtil_selfTest()",				de::STLUtil_selfTest));
		addChild(new SelfCheckCase(m_testCtx, "append_list",				"de::AppendList_selfTest()",			de::AppendList_selfTest));
	}
};

DelibsTests::DelibsTests (tcu::TestContext& testCtx)
	: tcu::TestCaseGroup(testCtx, "delibs", "delibs Tests")
{
}

DelibsTests::~DelibsTests (void)
{
}

void DelibsTests::init (void)
{
	addChild(new DepoolTests	(m_testCtx));
	addChild(new DethreadTests	(m_testCtx));
	addChild(new DeutilTests	(m_testCtx));
	addChild(new DebaseTests	(m_testCtx));
	addChild(new DecppTests		(m_testCtx));
}

} // dit
