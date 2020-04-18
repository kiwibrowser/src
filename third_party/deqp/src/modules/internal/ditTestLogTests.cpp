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
 * \brief Test log output tests.
 *//*--------------------------------------------------------------------*/

#include "ditTestLogTests.hpp"
#include "tcuTestLog.hpp"

#include <limits>

namespace dit
{

using tcu::TestLog;

// \todo [2014-02-25 pyry] Extend with:
//  - output of all element types
//  - nested element cases (sections, image sets)
//  - parse results and verify

class BasicSampleListCase : public tcu::TestCase
{
public:
	BasicSampleListCase (tcu::TestContext& testCtx)
		: TestCase(testCtx, "sample_list", "Basic sample list usage")
	{
	}

	IterateResult iterate (void)
	{
		TestLog& log = m_testCtx.getLog();

		log << TestLog::SampleList("TestSamples", "Test Sample List")
			<< TestLog::SampleInfo
			<< TestLog::ValueInfo("NumDrawCalls",	"Number of draw calls",		"",		QP_SAMPLE_VALUE_TAG_PREDICTOR)
			<< TestLog::ValueInfo("NumOps",			"Number of ops in shader",	"op",	QP_SAMPLE_VALUE_TAG_PREDICTOR)
			<< TestLog::ValueInfo("RenderTime",		"Rendering time",			"ms",	QP_SAMPLE_VALUE_TAG_RESPONSE)
			<< TestLog::EndSampleInfo;

		log << TestLog::Sample << 1 << 2 << 2.3 << TestLog::EndSample
			<< TestLog::Sample << 0 << 0 << 0 << TestLog::EndSample
			<< TestLog::Sample << 421 << -23 << 0.00001 << TestLog::EndSample
			<< TestLog::Sample << 2 << 9 << -1e9 << TestLog::EndSample
			<< TestLog::Sample << std::numeric_limits<deInt64>::max() << std::numeric_limits<deInt64>::min() << -0.0f << TestLog::EndSample
			<< TestLog::Sample << 0x3355 << 0xf24 << std::numeric_limits<double>::max() << TestLog::EndSample;

		log << TestLog::EndSampleList;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}
};

TestLogTests::TestLogTests (tcu::TestContext& testCtx)
	: TestCaseGroup(testCtx, "testlog", "Test Log Tests")
{
}

TestLogTests::~TestLogTests (void)
{
}

void TestLogTests::init (void)
{
	addChild(new BasicSampleListCase(m_testCtx));
}

} // dit
