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
 * \brief
 */ /*-------------------------------------------------------------------*/

/**
 */ /*!
 * \file  glcNoErrorTests.cpp
 * \brief Conformance tests for the GL_KHR_no_error functionality.
 */ /*--------------------------------------------------------------------*/

#include "glcNoErrorTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuCommandLine.hpp"
#include "tcuTestLog.hpp"

using namespace glu;

namespace glcts
{

/** Constructor.
	 *
	 *  @param context     Rendering context
	 *  @param name        Test name
	 *  @param description Test description
	 *  @param apiType     API version
	 */
NoErrorContextTest::NoErrorContextTest(tcu::TestContext& testCtx, glu::ApiType apiType)
	: tcu::TestCase(testCtx, "create_context", "Test verifies if it is possible to create context with "
											   "CONTEXT_FLAG_NO_ERROR_BIT_KHR flag set in CONTEXT_FLAGS")
	, m_ApiType(apiType)
{
	/* Left blank intentionally */
}

/** Tears down any GL objects set up to run the test. */
void NoErrorContextTest::deinit(void)
{
}

/** Stub init method */
void NoErrorContextTest::init(void)
{
}

/** Veriffy if no error context can be successfully created.
	 * @return True when no error context was successfully created.
	 */
bool NoErrorContextTest::verifyNoErrorContext(void)
{
	RenderConfig renderCfg(glu::ContextType(m_ApiType, glu::CONTEXT_NO_ERROR));

	const tcu::CommandLine& commandLine = m_testCtx.getCommandLine();
	glu::parseRenderConfig(&renderCfg, commandLine);

	if (commandLine.getSurfaceType() != tcu::SURFACETYPE_WINDOW)
		throw tcu::NotSupportedError("Test not supported in non-windowed context");

	RenderContext* noErrorContext = createRenderContext(m_testCtx.getPlatform(), commandLine, renderCfg);
	bool		   contextCreated = (noErrorContext != NULL);
	delete noErrorContext;

	return contextCreated;
}

/** Executes test iteration.
	 *
	 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
	 */
tcu::TestNode::IterateResult NoErrorContextTest::iterate(void)
{
	{
		glu::ContextType contextType(m_ApiType);
		deqp::Context	context(m_testCtx, contextType);

		bool noErrorExtensionExists = glu::contextSupports(contextType, glu::ApiType::core(4, 6));
		noErrorExtensionExists |= context.getContextInfo().isExtensionSupported("GL_KHR_no_error");

		if (!noErrorExtensionExists)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_KHR_no_error extension not supported");
			return STOP;
		}
	} // at this point intermediate context used to query the GL_KHR_no_error extension should be destroyed

	if (verifyNoErrorContext())
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

	m_testCtx.getLog() << tcu::TestLog::Message << "Failed to create No Error context" << tcu::TestLog::EndMessage;
	m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	return STOP;
}

/** Constructor.
	 *
	 *  @param context Rendering context.
	 */
NoErrorTests::NoErrorTests(tcu::TestContext& testCtx, glu::ApiType apiType)
	: tcu::TestCaseGroup(testCtx, "no_error", "Verify conformance of GL_KHR_no_error implementation")
	, m_ApiType(apiType)
{
}

/** Initializes the test group contents. */
void NoErrorTests::init(void)
{
	addChild(new NoErrorContextTest(m_testCtx, m_ApiType));
}
} /* glcts namespace */
