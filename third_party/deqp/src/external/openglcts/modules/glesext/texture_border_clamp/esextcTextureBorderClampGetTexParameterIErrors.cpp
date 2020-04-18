/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014-2016 The Khronos Group Inc.
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
 * \file esextcTextureBorderClampGetTexParameterIErrors.cpp
 * \brief Texture Border Clamp glGetTexParameterIivEXT(), glGetTexParameterIuivEXT() Errors (Test 3)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBorderClampGetTexParameterIErrors.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <vector>

namespace glcts
{

/* Amount of entries to pre-allocate for vectors which will be used by
 * glGetTexParameterIivEXT() and glGetTexParameterIuivEXT() functions.
 */
const glw::GLuint TextureBorderClampGetTexParameterIErrorsTest::m_buffer_size = 32;

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBorderClampGetTexParameterIErrorsTest::TextureBorderClampGetTexParameterIErrorsTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TextureBorderClampBase(context, extParams, name, description), m_test_passed(true)
{
	/* Left blank on purpose */
}

/** Initializes GLES objects used during the test.  */
void TextureBorderClampGetTexParameterIErrorsTest::initTest(void)
{
	if (!m_is_texture_border_clamp_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BORDER_CLAMP_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Initialize base class */
	TextureBorderClampBase::initTest();

	m_pname_list.push_back(GL_DEPTH_STENCIL_TEXTURE_MODE);
	m_pname_list.push_back(GL_TEXTURE_BASE_LEVEL);
	m_pname_list.push_back(m_glExtTokens.TEXTURE_BORDER_COLOR);
	m_pname_list.push_back(GL_TEXTURE_COMPARE_FUNC);
	m_pname_list.push_back(GL_TEXTURE_IMMUTABLE_FORMAT);
	m_pname_list.push_back(GL_TEXTURE_IMMUTABLE_LEVELS);
	m_pname_list.push_back(GL_TEXTURE_MAG_FILTER);
	m_pname_list.push_back(GL_TEXTURE_MAX_LEVEL);
	m_pname_list.push_back(GL_TEXTURE_MAX_LOD);
	m_pname_list.push_back(GL_TEXTURE_MIN_FILTER);
	m_pname_list.push_back(GL_TEXTURE_MIN_LOD);
	m_pname_list.push_back(GL_TEXTURE_SWIZZLE_R);
	m_pname_list.push_back(GL_TEXTURE_SWIZZLE_G);
	m_pname_list.push_back(GL_TEXTURE_SWIZZLE_B);
	m_pname_list.push_back(GL_TEXTURE_SWIZZLE_A);
	m_pname_list.push_back(GL_TEXTURE_WRAP_S);
	m_pname_list.push_back(GL_TEXTURE_WRAP_T);
	m_pname_list.push_back(GL_TEXTURE_WRAP_R);
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult TextureBorderClampGetTexParameterIErrorsTest::iterate(void)
{
	initTest();

	/* Make sure the tested functions report GL_NO_ERROR if used with
	 * cube map texture target. */
	CheckAllNames(GL_TEXTURE_CUBE_MAP, GL_NO_ERROR);

	/* Make sure the tested functions report GL_INVALID_ENUM error
	 * if used for cube-map face texture targets. */
	CheckAllNames(GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_INVALID_ENUM);

	if (m_is_texture_buffer_supported)
	{
		/* Make sure the tested functions report GL_INVALID_ENUM error
		 * if used for GL_TEXTURE_BUFFER_EXT texture target. */
		CheckAllNames(GL_TEXTURE_BUFFER_EXT, GL_INVALID_ENUM);
	}

	if (m_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/**
 * Verifies that all pnames are correctly handled by glGetTexParameterIivEXT() and
 * glGetTexParameterIuivEXT() functions for a particular texture target.
 *
 * @param target          texture target to be used for the calls;
 * @param expected_error  GL error code to expect;
 */
void TextureBorderClampGetTexParameterIErrorsTest::CheckAllNames(glw::GLenum target, glw::GLenum expected_error)
{
	for (glw::GLuint i = 0; i < m_pname_list.size(); ++i)
	{
		/* Check glGetTexParameterIivEXT() */
		VerifyGLGetTexParameterIiv(target, m_pname_list[i] /* pname*/, expected_error);
		/* Check glGetTexParameterIuivEXT() */
		VerifyGLGetTexParameterIuiv(target, m_pname_list[i] /* pname*/, expected_error);
	}
}

/** Verifies that calling glGetTexParameterIivEXT() with user-specified arguments
 *  generates user-defined error code.
 *
 * @param target         texture target to be used for the call;
 * @param pname          pname to be used for the call;
 * @param expected_error anticipated GL error code;
 */
void TextureBorderClampGetTexParameterIErrorsTest::VerifyGLGetTexParameterIiv(glw::GLenum target, glw::GLenum pname,
																			  glw::GLenum expected_error)
{
	glw::GLenum				error_code = GL_NO_ERROR;
	const glw::Functions&   gl		   = m_context.getRenderContext().getFunctions();
	std::vector<glw::GLint> params(m_buffer_size);

	gl.getTexParameterIiv(target, pname, &params[0]);

	error_code = gl.getError();
	if (expected_error != error_code)
	{
		m_test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glGetTexParameterIivEXT() failed:["
						   << "target:" << getTexTargetString(target) << ", pname:" << getPNameString(pname)
						   << "] reported error code:[" << glu::getErrorStr(error_code) << "] expected error code:["
						   << glu::getErrorStr(expected_error) << "]\n"
						   << tcu::TestLog::EndMessage;
	}
}

/** Verifies that calling glGetTexParameterIuivEXT() with user-specified arguments
 *  generates user-defined error code.
 *
 * @param target         texture target to be used for the call;
 * @param pname          pname to be used for the call;
 * @param expected_error anticipated GL error code;
 */
void TextureBorderClampGetTexParameterIErrorsTest::VerifyGLGetTexParameterIuiv(glw::GLenum target, glw::GLenum pname,
																			   glw::GLenum expected_error)
{
	glw::GLenum				 error_code = GL_NO_ERROR;
	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	std::vector<glw::GLuint> params(m_buffer_size);

	gl.getTexParameterIuiv(target, pname, &params[0]);

	error_code = gl.getError();
	if (expected_error != error_code)
	{
		m_test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glGetTexParameterIuivEXT() failed:["
						   << "target:" << getTexTargetString(target) << ", pname:" << getPNameString(pname)
						   << "] reported error code:[" << glu::getErrorStr(error_code) << "] expected error code:["
						   << glu::getErrorStr(expected_error) << "]\n"
						   << tcu::TestLog::EndMessage;
	}
}

} // namespace glcts
