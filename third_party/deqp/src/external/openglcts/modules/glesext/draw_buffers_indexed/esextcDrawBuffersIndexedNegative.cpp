/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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

/*!
 * \file  esextcDrawBuffersIndexedNegative.hpp
 * \brief Draw Buffers Indexed tests 6. Negative
 */ /*-------------------------------------------------------------------*/

#include "esextcDrawBuffersIndexedNegative.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
DrawBuffersIndexedNegative::DrawBuffersIndexedNegative(Context& context, const ExtParameters& extParams,
													   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
{
	/* Left blank on purpose */
}

void DrawBuffersIndexedNegative::init()
{
	if (!isExtensionSupported("GL_OES_draw_buffers_indexed"))
	{
		throw tcu::NotSupportedError(DRAW_BUFFERS_INDEXED_NOT_SUPPORTED);
	}
}

tcu::TestNode::IterateResult DrawBuffersIndexedNegative::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint	 data;
	glw::GLint64   data64;
	glw::GLboolean bData;
	glw::GLint	 maxDrawBuffers;
	gl.getIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);

	bool success = true;

	// INVALID_VALUE
	gl.blendEquationi(maxDrawBuffers, GL_MIN);
	success &= ExpectedError(GL_INVALID_VALUE, "glBlendEquationi(0, maxDrawBuffers)");

	gl.blendEquationSeparatei(maxDrawBuffers, GL_MIN, GL_MIN);
	success &= ExpectedError(GL_INVALID_VALUE, "glBlendEquationSeparatei(0, maxDrawBuffers)");

	gl.blendFuncSeparatei(maxDrawBuffers, GL_CONSTANT_COLOR, GL_DST_ALPHA, GL_SRC_ALPHA, GL_ONE);
	success &=
		ExpectedError(GL_INVALID_VALUE,
					  "glBlendFuncSeparatei(maxDrawBuffers, GL_CONSTANT_COLOR, GL_DST_ALPHA, GL_SRC_ALPHA, GL_ONE)");

	gl.blendFunci(maxDrawBuffers, GL_ZERO, GL_SRC_COLOR);
	success &= ExpectedError(GL_INVALID_VALUE, "glBlendFunci(maxDrawBuffers, GL_ZERO, GL_SRC_COLOR)");

	gl.getIntegeri_v(GL_BLEND_EQUATION_RGB, maxDrawBuffers, &data);
	success &= ExpectedError(GL_INVALID_VALUE, "glGetIntegeri_v(GL_BLEND_EQUATION_RGB, maxDrawBuffers, &data)");

	gl.getIntegeri_v(GL_BLEND_EQUATION_ALPHA, maxDrawBuffers, &data);
	success &= ExpectedError(GL_INVALID_VALUE, "glGetIntegeri_v(GL_BLEND_EQUATION_ALPHA, maxDrawBuffers, &data)");

	gl.getIntegeri_v(GL_BLEND_SRC_RGB, maxDrawBuffers, &data);
	success &= ExpectedError(GL_INVALID_VALUE, "glGetIntegeri_v(GL_BLEND_SRC_RGB, maxDrawBuffers, &data)");

	gl.getIntegeri_v(GL_BLEND_SRC_ALPHA, maxDrawBuffers, &data);
	success &= ExpectedError(GL_INVALID_VALUE, "glGetIntegeri_v(GL_BLEND_SRC_ALPHA, maxDrawBuffers, &data)");

	gl.getIntegeri_v(GL_BLEND_DST_RGB, maxDrawBuffers, &data);
	success &= ExpectedError(GL_INVALID_VALUE, "glGetIntegeri_v(GL_BLEND_DST_RGB, maxDrawBuffers, &data)");

	gl.getIntegeri_v(GL_BLEND_DST_ALPHA, maxDrawBuffers, &data);
	success &= ExpectedError(GL_INVALID_VALUE, "glGetIntegeri_v(GL_BLEND_DST_ALPHA, maxDrawBuffers, &data)");

	gl.getInteger64i_v(GL_BLEND_EQUATION_RGB, maxDrawBuffers, &data64);
	success &= ExpectedError(GL_INVALID_VALUE, "glGetInteger64i_v(GL_BLEND_EQUATION_RGB, maxDrawBuffers, &data64)");

	gl.getInteger64i_v(GL_BLEND_EQUATION_ALPHA, maxDrawBuffers, &data64);
	success &= ExpectedError(GL_INVALID_VALUE, "glGetInteger64i_v(GL_BLEND_EQUATION_ALPHA, maxDrawBuffers, &data64)");

	gl.getInteger64i_v(GL_BLEND_SRC_RGB, maxDrawBuffers, &data64);
	success &= ExpectedError(GL_INVALID_VALUE, "glGetInteger64i_v(GL_BLEND_SRC_RGB, maxDrawBuffers, &data64)");

	gl.getInteger64i_v(GL_BLEND_SRC_ALPHA, maxDrawBuffers, &data64);
	success &= ExpectedError(GL_INVALID_VALUE, "glGetInteger64i_v(GL_BLEND_SRC_ALPHA, maxDrawBuffers, &data64)");

	gl.getInteger64i_v(GL_BLEND_DST_RGB, maxDrawBuffers, &data64);
	success &= ExpectedError(GL_INVALID_VALUE, "glGetInteger64i_v(GL_BLEND_DST_RGB, maxDrawBuffers, &data64)");

	gl.getInteger64i_v(GL_BLEND_DST_ALPHA, maxDrawBuffers, &data64);
	success &= ExpectedError(GL_INVALID_VALUE, "glGetInteger64i_v(GL_BLEND_DST_ALPHA, maxDrawBuffers, &data64)");

	gl.enablei(GL_BLEND, maxDrawBuffers);
	success &= ExpectedError(GL_INVALID_VALUE, "glEnablei(GL_BLEND, maxDrawBuffers)");

	gl.disablei(GL_BLEND, maxDrawBuffers);
	success &= ExpectedError(GL_INVALID_VALUE, "glDisablei(GL_BLEND, maxDrawBuffers)");

	gl.isEnabledi(GL_BLEND, maxDrawBuffers);
	success &= ExpectedError(GL_INVALID_VALUE, "glIsEnabledi(GL_BLEND, maxDrawBuffers)");

	gl.colorMaski(maxDrawBuffers, GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
	success &= ExpectedError(GL_INVALID_VALUE, "glColorMaski(maxDrawBuffers, GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE)");

	gl.getBooleani_v(GL_COLOR_WRITEMASK, maxDrawBuffers, &bData);
	success &= ExpectedError(GL_INVALID_VALUE, "glGetBooleani_v(GL_COLOR_WRITEMASK, maxDrawBuffers, &bData)");

	// INVALID_ENUM
	gl.blendFunci(0, GL_MIN, GL_ONE_MINUS_SRC_ALPHA);
	success &= ExpectedError(GL_INVALID_ENUM, "glBlendFunci(0, GL_MIN, GL_ONE_MINUS_SRC_ALPHA)");

	gl.blendFunci(0, GL_SRC_ALPHA, GL_MIN);
	success &= ExpectedError(GL_INVALID_ENUM, "glBlendFunci(0, GL_SRC_ALPHA, GL_MIN)");

	gl.blendFuncSeparatei(0, GL_MIN, GL_ONE, GL_ONE, GL_ONE);
	success &= ExpectedError(GL_INVALID_ENUM, "glBlendFuncSeparatei(0, GL_MIN, GL_ONE, GL_ONE, GL_ONE)");

	gl.blendFuncSeparatei(0, GL_ONE, GL_MIN, GL_ONE, GL_ONE);
	success &= ExpectedError(GL_INVALID_ENUM, "glBlendFuncSeparatei(0, GL_ONE, GL_MIN, GL_ONE, GL_ONE)");

	gl.blendFuncSeparatei(0, GL_ONE, GL_ONE, GL_MIN, GL_ONE);
	success &= ExpectedError(GL_INVALID_ENUM, "glBlendFuncSeparatei(0, GL_ONE, GL_ONE, GL_MIN, GL_ONE)");

	gl.blendFuncSeparatei(0, GL_ONE, GL_ONE, GL_ONE, GL_MIN);
	success &= ExpectedError(GL_INVALID_ENUM, "glBlendFuncSeparatei(0, GL_ONE, GL_ONE, GL_ONE, GL_MIN)");

	gl.blendEquationi(0, GL_SRC_ALPHA);
	success &= ExpectedError(GL_INVALID_ENUM, "glBlendEquationi(0, GL_SRC_ALPHA)");

	gl.blendEquationSeparatei(0, GL_SRC_ALPHA, GL_MIN);
	success &= ExpectedError(GL_INVALID_ENUM, "glBlendEquationSeparatei(0, GL_SRC_ALPHA, GL_MIN)");

	gl.blendEquationSeparatei(0, GL_MIN, GL_SRC_ALPHA);
	success &= ExpectedError(GL_INVALID_ENUM, "glBlendEquationSeparatei(0, GL_MIN, GL_SRC_ALPHA)");

	gl.enablei(GL_FUNC_ADD, 0);
	success &= ExpectedError(GL_INVALID_ENUM, "glEnablei(GL_FUNC_ADD, 0)");

	gl.disablei(GL_FUNC_ADD, 0);
	success &= ExpectedError(GL_INVALID_ENUM, "glDisablei(GL_FUNC_ADD, 0)");

	gl.isEnabledi(GL_FUNC_ADD, 0);
	success &= ExpectedError(GL_INVALID_ENUM, "glIsEnabledi(GL_FUNC_ADD, 0)");

	if (success)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Negative test failed");
	}
	return STOP;
}

bool DrawBuffersIndexedNegative::ExpectedError(glw::GLenum expectedResult, const char* call)
{
	glw::GLenum error = m_context.getRenderContext().getFunctions().getError();

	if (expectedResult != error)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Error on " << call << " call.\n"
						   << "Expected " << expectedResult << " but found " << error << tcu::TestLog::EndMessage;
		return false;
	}
	return true;
}

} // namespace glcts
