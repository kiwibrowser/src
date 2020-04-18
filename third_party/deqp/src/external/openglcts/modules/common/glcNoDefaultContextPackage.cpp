/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
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
 */ /*!
 * \file
 * \brief OpenGL Conformance Test Package that does not have predefined GL context.
 */ /*-------------------------------------------------------------------*/

#include "glcNoDefaultContextPackage.hpp"
#include "glcContextFlagsTests.hpp"
#include "glcMultipleContextsTests.hpp"
#include "glcNoErrorTests.hpp"
#include "glcRobustBufferAccessBehaviorTests.hpp"
#include "glcRobustnessTests.hpp"
#include "gluRenderContext.hpp"

namespace glcts
{
namespace nodefaultcontext
{
class TestCaseWrapper : public tcu::TestCaseExecutor
{
public:
	TestCaseWrapper(void);
	~TestCaseWrapper(void);

	void init(tcu::TestCase* testCase, const std::string& path);
	void deinit(tcu::TestCase* testCase);
	tcu::TestNode::IterateResult iterate(tcu::TestCase* testCase);
};

TestCaseWrapper::TestCaseWrapper(void)
{
}

TestCaseWrapper::~TestCaseWrapper(void)
{
}

void TestCaseWrapper::init(tcu::TestCase* testCase, const std::string&)
{
	testCase->init();
}

void TestCaseWrapper::deinit(tcu::TestCase* testCase)
{
	testCase->deinit();
}

tcu::TestNode::IterateResult TestCaseWrapper::iterate(tcu::TestCase* testCase)
{
	const tcu::TestCase::IterateResult result = testCase->iterate();

	return result;
}
} // nodefaultcontext

NoDefaultContextPackage::NoDefaultContextPackage(tcu::TestContext& testCtx, const char* name)
	: tcu::TestPackage(testCtx, name, "CTS No Default Context Package")
{
}

NoDefaultContextPackage::~NoDefaultContextPackage(void)
{
}

tcu::TestCaseExecutor* NoDefaultContextPackage::createExecutor(void) const
{
	return new nodefaultcontext::TestCaseWrapper();
}

void NoDefaultContextPackage::init(void)
{
	tcu::TestCaseGroup* gl30Group = new tcu::TestCaseGroup(getTestContext(), "gl30", "");
	gl30Group->addChild(new glcts::NoErrorTests(getTestContext(), glu::ApiType::core(3, 0)));
	addChild(gl30Group);

	tcu::TestCaseGroup* gl40Group = new tcu::TestCaseGroup(getTestContext(), "gl40", "");
	gl40Group->addChild(new glcts::MultipleContextsTests(getTestContext(), glu::ApiType::core(4, 0)));
	addChild(gl40Group);

	tcu::TestCaseGroup* gl43Group = new tcu::TestCaseGroup(getTestContext(), "gl43", "");
	gl43Group->addChild(new glcts::RobustBufferAccessBehaviorTests(getTestContext(), glu::ApiType::core(4, 3)));
	addChild(gl43Group);

	tcu::TestCaseGroup* gl45Group = new tcu::TestCaseGroup(getTestContext(), "gl45", "");
	gl45Group->addChild(new glcts::RobustnessTests(getTestContext(), glu::ApiType::core(4, 5)));
	gl45Group->addChild(new glcts::ContextFlagsTests(getTestContext(), glu::ApiType::core(4, 5)));
	addChild(gl45Group);

	tcu::TestCaseGroup* es2Group = new tcu::TestCaseGroup(getTestContext(), "es2", "");
	es2Group->addChild(new glcts::NoErrorTests(getTestContext(), glu::ApiType::es(2, 0)));
	addChild(es2Group);

	tcu::TestCaseGroup* es32Group = new tcu::TestCaseGroup(getTestContext(), "es32", "");
	es32Group->addChild(new glcts::RobustnessTests(getTestContext(), glu::ApiType::es(3, 2)));
	es32Group->addChild(new glcts::ContextFlagsTests(getTestContext(), glu::ApiType::es(3, 2)));
	es32Group->addChild(new glcts::RobustBufferAccessBehaviorTests(getTestContext(), glu::ApiType::es(3, 2)));
	addChild(es32Group);
}

} // glcts
