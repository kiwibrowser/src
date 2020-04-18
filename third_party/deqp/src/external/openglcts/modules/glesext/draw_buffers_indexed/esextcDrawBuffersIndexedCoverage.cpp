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
 * \file  esextcDrawBuffersIndexedCoverage.hpp
 * \brief Draw Buffers Indexed tests 1. Coverage
 */ /*-------------------------------------------------------------------*/

#include "esextcDrawBuffersIndexedCoverage.hpp"
#include "glwEnums.hpp"

namespace glcts
{

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
DrawBuffersIndexedCoverage::DrawBuffersIndexedCoverage(Context& context, const ExtParameters& extParams,
													   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
{
	/* Left blank on purpose */
}

void DrawBuffersIndexedCoverage::init()
{
	if (!isExtensionSupported("GL_OES_draw_buffers_indexed"))
	{
		throw tcu::NotSupportedError(DRAW_BUFFERS_INDEXED_NOT_SUPPORTED);
	}
}

tcu::TestNode::IterateResult DrawBuffersIndexedCoverage::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	// Check number of available draw buffers
	glw::GLint maxDrawBuffers = 0;
	gl.getIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
	if (maxDrawBuffers < 4)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Minimum number of draw buffers too low");
		return STOP;
	}

	// Check that all functions executed properly
	glw::GLint	 valueI;
	glw::GLint64   valueLI;
	glw::GLboolean valueB;
	for (int i = 0; i < maxDrawBuffers; ++i)
	{
		gl.enablei(GL_BLEND, i);
		gl.disablei(GL_BLEND, i);

		gl.isEnabledi(GL_BLEND, i);
		gl.colorMaski(i, GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
		gl.blendEquationi(i, GL_MIN);
		gl.blendEquationSeparatei(i, GL_MAX, GL_FUNC_SUBTRACT);
		gl.blendFunci(i, GL_ZERO, GL_SRC_COLOR);
		gl.blendFuncSeparatei(i, GL_CONSTANT_COLOR, GL_DST_ALPHA, GL_SRC_ALPHA, GL_ONE);

		gl.getIntegeri_v(GL_BLEND_DST_ALPHA, i, &valueI);
		gl.getInteger64i_v(GL_BLEND_DST_ALPHA, i, &valueLI);
		gl.getBooleani_v(GL_BLEND_SRC_RGB, i, &valueB);
	}

	// Restore default state
	gl.colorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	gl.blendEquation(GL_FUNC_ADD);
	gl.blendFunc(GL_ONE, GL_ZERO);

	// Check for error
	glw::GLenum error_code = gl.getError();
	if (error_code != GL_NO_ERROR)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Some functions generated error");
		return STOP;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

} // namespace glcts
