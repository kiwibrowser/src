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
 * \file  gl4cSpirvExtensionsTests.cpp
 * \brief Conformance tests for the GL_ARB_spirv_extensions functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cSpirvExtensionsTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

using namespace glw;
using namespace glu;

namespace gl4cts
{

/** Constructor.
 *
 *  @param context     Rendering context
 */
SpirvExtensionsQueriesTestCase::SpirvExtensionsQueriesTestCase(deqp::Context& context)
	: TestCase(context, "spirv_extensions_queries",
			   "Verifies if queries for GL_ARB_spirv_extension tokens works as expected")
{
	/* Left blank intentionally */
}

/** Stub init method */
void SpirvExtensionsQueriesTestCase::init()
{
	glu::ContextType contextType = m_context.getRenderContext().getType();
	if (!glu::contextSupports(contextType, glu::ApiType::core(4, 6)) &&
		!m_context.getContextInfo().isExtensionSupported("GL_ARB_spirv_extensions"))
	{
		TCU_THROW(NotSupportedError, "GL_ARB_spirv_extensions not supported");
	}

	if (!glu::contextSupports(contextType, glu::ApiType::core(4, 6)) &&
		!m_context.getContextInfo().isExtensionSupported("GL_ARB_gl_spirv"))
	{
		TCU_THROW(NotSupportedError, "GL_ARB_gl_spirv not supported");
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SpirvExtensionsQueriesTestCase::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	GLint numSpirvExtensions;
	gl.getIntegerv(GL_NUM_SPIR_V_EXTENSIONS, &numSpirvExtensions);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getIntegerv");

	m_testCtx.getLog() << tcu::TestLog::Message << "GL_NUM_SPIR_V_EXTENSIONS = " << numSpirvExtensions << "\n"
					   << tcu::TestLog::EndMessage;
	for (GLint i = 0; i < numSpirvExtensions; ++i)
	{
		const GLubyte* spirvExtension = DE_NULL;

		spirvExtension = gl.getStringi(GL_SPIR_V_EXTENSIONS, i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getStringi");

		if (!spirvExtension || strlen((const char*)spirvExtension) == 0)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Failed to fetch GL_SPIRV_EXTENSIONS string for index: " << i
							   << "\n"
							   << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}
		else
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "GL_SPIR_V_EXTENSION " << i << ": " << spirvExtension << "\n"
							   << tcu::TestLog::EndMessage;
		}
	}

	// Test out of bound query
	gl.getStringi(GL_SPIR_V_EXTENSIONS, numSpirvExtensions);
	GLenum err = gl.getError();
	if (err != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "GetStringi query for GL_SPIRV_EXTENSIONS with index: " << numSpirvExtensions
						   << " should generate INVALID_VALUE error instead of " << glu::getErrorName(err) << "\n"
						   << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 */
SpirvExtensionsTests::SpirvExtensionsTests(deqp::Context& context)
	: TestCaseGroup(context, "spirv_extensions", "Verify conformance of GL_ARB_spirv_extensions implementation")
{
}

/** Initializes the test group contents. */
void SpirvExtensionsTests::init()
{
	addChild(new SpirvExtensionsQueriesTestCase(m_context));
}

} /* gl4cts namespace */
