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
 * \file  esextcDrawBuffersIndexedDefaultState.hpp
 * \brief Draw Buffers Indexed tests 2. Default State
 */ /*-------------------------------------------------------------------*/

#include "esextcDrawBuffersIndexedDefaultState.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
DrawBuffersIndexedDefaultState::DrawBuffersIndexedDefaultState(Context& context, const ExtParameters& extParams,
															   const char* name, const char* description)
	: DrawBuffersIndexedBase(context, extParams, name, description)
{
	/* Left blank on purpose */
}

tcu::TestNode::IterateResult DrawBuffersIndexedDefaultState::iterate()
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
	if (!state.CheckAll())
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Default state verification failed");
		return STOP;
	}

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
