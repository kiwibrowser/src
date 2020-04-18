/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 2.0 Module
 * -------------------------------------------------
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
 * \brief drawElements Internal Test Package
 *//*--------------------------------------------------------------------*/

#include "ditTestPackage.hpp"
#include "ditBuildInfoTests.hpp"
#include "ditDelibsTests.hpp"
#include "ditFrameworkTests.hpp"
#include "ditImageIOTests.hpp"
#include "ditImageCompareTests.hpp"
#include "ditTestLogTests.hpp"
#include "ditSeedBuilderTests.hpp"
#include "ditSRGB8ConversionTest.hpp"

namespace dit
{
namespace
{

class TextureTests : public tcu::TestCaseGroup
{
public:
	TextureTests (tcu::TestContext& testCtx)
		: tcu::TestCaseGroup(testCtx, "texture", "Tests for tcu::Texture and utils.")
	{
	}

	void init (void)
	{
		addChild(createSRGB8ConversionTest(m_testCtx));
	}
};

class DeqpTests : public tcu::TestCaseGroup
{
public:
	DeqpTests (tcu::TestContext& testCtx)
		: tcu::TestCaseGroup(testCtx, "deqp", "dEQP Test Framework Self-tests")
	{
	}

	void init (void)
	{
		addChild(new TestLogTests		(m_testCtx));
		addChild(new ImageIOTests		(m_testCtx));
		addChild(new ImageCompareTests	(m_testCtx));
		addChild(new TextureTests		(m_testCtx));
		addChild(createSeedBuilderTests	(m_testCtx));
	}
};

} // anonymous

class TestCaseExecutor : public tcu::TestCaseExecutor
{
public:
	TestCaseExecutor (void)
	{
	}

	~TestCaseExecutor (void)
	{
	}

	void init (tcu::TestCase* testCase, const std::string&)
	{
		testCase->init();
	}

	void deinit (tcu::TestCase* testCase)
	{
		testCase->deinit();
	}

	tcu::TestNode::IterateResult iterate (tcu::TestCase* testCase)
	{
		return testCase->iterate();
	}
};

TestPackage::TestPackage (tcu::TestContext& testCtx)
	: tcu::TestPackage(testCtx, "dE-IT", "drawElements Internal Tests")
{
}

TestPackage::~TestPackage (void)
{
}

void TestPackage::init (void)
{
	addChild(new BuildInfoTests	(m_testCtx));
	addChild(new DelibsTests	(m_testCtx));
	addChild(new FrameworkTests	(m_testCtx));
	addChild(new DeqpTests		(m_testCtx));
}

tcu::TestCaseExecutor* TestPackage::createExecutor (void) const
{
	return new TestCaseExecutor();
}

} // dit
