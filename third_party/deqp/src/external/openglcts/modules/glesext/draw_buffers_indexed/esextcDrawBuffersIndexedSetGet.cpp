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
 * \file  esextcDrawBuffersIndexedSetGet.hpp
 * \brief Draw Buffers Indexed tests 3. Set and get
 */ /*-------------------------------------------------------------------*/

#include "esextcDrawBuffersIndexedSetGet.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
DrawBuffersIndexedSetGet::DrawBuffersIndexedSetGet(Context& context, const ExtParameters& extParams, const char* name,
												   const char* description)
	: DrawBuffersIndexedBase(context, extParams, name, description)
{
	/* Left blank on purpose */
}

tcu::TestNode::IterateResult DrawBuffersIndexedSetGet::iterate()
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

	BlendMaskStateMachine state(m_context, m_testCtx.getLog(), maxDrawBuffers);

	state.SetEnable();
	state.SetColorMask(0, 0, 0, 0);
	state.SetBlendEquation(GL_FUNC_SUBTRACT);
	state.SetBlendFunc(GL_ONE_MINUS_CONSTANT_ALPHA, GL_SRC_ALPHA_SATURATE);
	if (!state.CheckAll())
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Failed to change blending and color mask state");
		return STOP;
	}

	state.SetDisablei(1);
	state.SetColorMaski(1, 1, 1, 1, 1);
	state.SetBlendEquationi(1, GL_MIN);
	state.SetBlendFunci(1, GL_ONE_MINUS_DST_COLOR, GL_CONSTANT_ALPHA);
	if (!state.CheckAll())
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Failed to change blending and color mask state");
		return STOP;
	}

	state.SetBlendEquationSeparatei(2, GL_MAX, GL_FUNC_ADD);
	state.SetBlendFuncSeparatei(2, GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_DST_COLOR);
	if (!state.CheckAll())
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Failed to change blending and color mask state");
		return STOP;
	}

	state.SetDefaults();

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
