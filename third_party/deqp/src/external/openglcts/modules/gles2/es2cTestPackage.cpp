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
 * \brief OpenGL ES 2 Test Package.
 */ /*-------------------------------------------------------------------*/

#include "es2cTestPackage.hpp"
#include "es2cTexture3DTests.hpp"
#include "glcAggressiveShaderOptimizationsTests.hpp"
#include "glcInfoTests.hpp"
#include "glcInternalformatTests.hpp"
#include "glcShaderNegativeTests.hpp"
#include "gluRenderContext.hpp"
#include "gluStateReset.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace es2cts
{

class TestCaseWrapper : public tcu::TestCaseExecutor
{
public:
	TestCaseWrapper(TestPackage& package);
	~TestCaseWrapper(void);

	void init(tcu::TestCase* testCase, const std::string& path);
	void deinit(tcu::TestCase* testCase);
	tcu::TestNode::IterateResult iterate(tcu::TestCase* testCase);

private:
	es2cts::TestPackage& m_testPackage;
};

TestCaseWrapper::TestCaseWrapper(TestPackage& package) : m_testPackage(package)
{
}

TestCaseWrapper::~TestCaseWrapper(void)
{
}

void TestCaseWrapper::init(tcu::TestCase* testCase, const std::string&)
{
	glu::resetState(m_testPackage.getContext().getRenderContext(), m_testPackage.getContext().getContextInfo());

	testCase->init();
}

void TestCaseWrapper::deinit(tcu::TestCase* testCase)
{
	testCase->deinit();

	glu::resetState(m_testPackage.getContext().getRenderContext(), m_testPackage.getContext().getContextInfo());
}

tcu::TestNode::IterateResult TestCaseWrapper::iterate(tcu::TestCase* testCase)
{
	tcu::TestContext&			 testCtx   = m_testPackage.getContext().getTestContext();
	glu::RenderContext&			 renderCtx = m_testPackage.getContext().getRenderContext();
	tcu::TestCase::IterateResult result;

	// Clear to black
	{
		const glw::Functions& gl = renderCtx.getFunctions();
		gl.clearColor(0.0f, 0.0f, 0.0f, 1.f);
		gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	result = testCase->iterate();

	// Call implementation specific post-iterate routine (usually handles native events and swaps buffers)
	try
	{
		renderCtx.postIterate();
		return result;
	}
	catch (const tcu::ResourceError& e)
	{
		testCtx.getLog() << e;
		testCtx.setTestResult(QP_TEST_RESULT_RESOURCE_ERROR, "Resource error in context post-iteration routine");
		testCtx.setTerminateAfter(true);
		return tcu::TestNode::STOP;
	}
	catch (const std::exception& e)
	{
		testCtx.getLog() << e;
		testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Error in context post-iteration routine");
		return tcu::TestNode::STOP;
	}
}

class ShaderTests : public deqp::TestCaseGroup
{
public:
	ShaderTests(deqp::Context& context) : TestCaseGroup(context, "shaders", "Shading Language Tests")
	{
	}

	void init(void)
	{
		addChild(new deqp::ShaderNegativeTests(m_context, glu::GLSL_VERSION_100_ES));
		addChild(new glcts::AggressiveShaderOptimizationsTests(m_context));
	}
};

TestPackage::TestPackage(tcu::TestContext& testCtx, const char* packageName)
	: deqp::TestPackage(testCtx, packageName, "OpenGL ES 2 Conformance Tests", glu::ContextType(glu::ApiType::es(2, 0)),
						"gl_cts/data/gles2/")
{
}

TestPackage::~TestPackage(void)
{
}

void TestPackage::init(void)
{
	// Call init() in parent - this creates context.
	deqp::TestPackage::init();

	try
	{
		addChild(new ShaderTests(getContext()));
		addChild(new Texture3DTests(getContext()));
		tcu::TestCaseGroup* coreGroup = new tcu::TestCaseGroup(getTestContext(), "core", "core tests");
		coreGroup->addChild(new glcts::InternalformatTests(getContext()));
		addChild(coreGroup);
	}
	catch (...)
	{
		// Destroy context.
		deqp::TestPackage::deinit();
		throw;
	}
}

tcu::TestCaseExecutor* TestPackage::createExecutor(void) const
{
	return new TestCaseWrapper(const_cast<TestPackage&>(*this));
}

} // es2cts
