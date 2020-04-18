/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 Google Inc.
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \brief OpenGL Conformance Test Configuration List Package
 */ /*-------------------------------------------------------------------*/

#include "glcConfigPackage.hpp"
#include "glcConfigListCase.hpp"

#include "glcTestPackage.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{
namespace config
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
}

ConfigPackage::ConfigPackage(tcu::TestContext& testCtx, const char* name)
	: tcu::TestPackage(testCtx, name, "CTS Configuration List Package")
{
}

ConfigPackage::~ConfigPackage(void)
{
}

tcu::TestCaseExecutor* ConfigPackage::createExecutor(void) const
{
	return new config::TestCaseWrapper();
}

void ConfigPackage::init(void)
{
	addChild(new ConfigListCase(m_testCtx, "es2", "OpenGL ES 2 Configurations", glu::ApiType::es(2, 0)));
	addChild(new ConfigListCase(m_testCtx, "es3", "OpenGL ES 3 Configurations", glu::ApiType::es(3, 0)));
	addChild(new ConfigListCase(m_testCtx, "es31", "OpenGL ES 3.1 Configurations", glu::ApiType::es(3, 1)));
	addChild(new ConfigListCase(m_testCtx, "es32", "OpenGL ES 3.2 Configurations", glu::ApiType::es(3, 2)));
	addChild(new ConfigListCase(m_testCtx, "gl30", "OpenGL 3.0 Configurations", glu::ApiType::core(3, 0)));
	addChild(new ConfigListCase(m_testCtx, "gl31", "OpenGL 3.1 Configurations", glu::ApiType::core(3, 1)));
	addChild(new ConfigListCase(m_testCtx, "gl32", "OpenGL 3.2 Configurations", glu::ApiType::core(3, 2)));
	addChild(new ConfigListCase(m_testCtx, "gl33", "OpenGL 3.3 Configurations", glu::ApiType::core(3, 3)));
	addChild(new ConfigListCase(m_testCtx, "gl40", "OpenGL 4.0 Configurations", glu::ApiType::core(4, 0)));
	addChild(new ConfigListCase(m_testCtx, "gl41", "OpenGL 4.1 Configurations", glu::ApiType::core(4, 1)));
	addChild(new ConfigListCase(m_testCtx, "gl42", "OpenGL 4.2 Configurations", glu::ApiType::core(4, 2)));
	addChild(new ConfigListCase(m_testCtx, "gl43", "OpenGL 4.3 Configurations", glu::ApiType::core(4, 3)));
	addChild(new ConfigListCase(m_testCtx, "gl44", "OpenGL 4.4 Configurations", glu::ApiType::core(4, 4)));
	addChild(new ConfigListCase(m_testCtx, "gl45", "OpenGL 4.5 Configurations", glu::ApiType::core(4, 5)));
	addChild(new ConfigListCase(m_testCtx, "gl46", "OpenGL 4.6 Configurations", glu::ApiType::core(4, 6)));
}

} // glcts
