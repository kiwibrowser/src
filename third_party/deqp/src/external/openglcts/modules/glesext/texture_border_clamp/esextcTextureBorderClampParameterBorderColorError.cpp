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
 * \file  esextcTextureBorderClampParameterBorderColorError.Cpp
 * \brief Texture Border Clamp Border Color Error (Test 1)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBorderClampParameterBorderColorError.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

/* Texture unit number */
const glw::GLuint TextureBorderClampParameterBorderColorErrorTest::m_texture_unit = 0;

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
TextureBorderClampParameterBorderColorErrorTest::TextureBorderClampParameterBorderColorErrorTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TextureBorderClampBase(context, extParams, name, description), m_sampler_id(0), m_test_passed(true)
{
	/* Left blank on purpose */
}

/** Deinitializes GLES objects created during the test.
 *
 */
void TextureBorderClampParameterBorderColorErrorTest::deinit(void)
{
	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release sampler object */
	gl.bindSampler(m_texture_unit, 0);

	if (0 != m_sampler_id)
	{
		gl.deleteSamplers(1, &m_sampler_id);

		m_sampler_id = 0;
	}

	/* Deinitializes base class */
	TextureBorderClampBase::deinit();
}

/** Initializes GLES objects used during the test.
 *
 */
void TextureBorderClampParameterBorderColorErrorTest::initTest(void)
{
	if (!m_is_texture_border_clamp_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BORDER_CLAMP_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Initialize base class implementation */
	TextureBorderClampBase::initTest();

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate a sampler object */
	gl.genSamplers(1, &m_sampler_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating a sampler object");

	/* Bind a sampler object to the texture unit we will use for the test*/
	gl.bindSampler(m_texture_unit, m_sampler_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding a sampler object to texture unit");
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult TextureBorderClampParameterBorderColorErrorTest::iterate(void)
{
	/* Initialize all ES objects necessary to run the test */
	initTest();

	for (unsigned int i = 0; i < m_texture_target_list.size(); ++i)
	{
		/* Check if function glTexParameterf works properly for all the texture targets supported
		 * in ES3.1.
		 */
		VerifyGLTexParameterf(m_texture_target_list[i], GL_TEXTURE_BASE_LEVEL, 0.0f, /* param */
							  GL_NO_ERROR /* expected_error */);

		/* Make sure that the functions report GL_INVALID_ENUM if
		 * any of them attempts to modify GL_TEXTURE_BORDER_COLOR_EXT
		 * parameter.
		 */
		VerifyGLTexParameterf(m_texture_target_list[i], m_glExtTokens.TEXTURE_BORDER_COLOR, 0.0f, /* param */
							  GL_INVALID_ENUM /* expected_error */);

		/* Check that glTexParameteri() accepts all ES3.1 texture targets */
		VerifyGLTexParameteri(m_texture_target_list[i], GL_TEXTURE_BASE_LEVEL, 0, /* param */
							  GL_NO_ERROR /* expected_error */);

		/* Check that glTexParameteri() reports GL_INVALID_ENUM if used for
		 * GL_TEXTURE_BORDER_COLOR_EXT pname */
		VerifyGLTexParameteri(m_texture_target_list[i], m_glExtTokens.TEXTURE_BORDER_COLOR, 0, /* param */
							  GL_INVALID_ENUM /* expected_error */);
	}

	/* Check that glSamplerParameter{f,i} functions correctly handle
	 * GL_TEXTURE_MIN_FILTER pname.
	 **/
	VerifyGLSamplerParameterf(GL_TEXTURE_MIN_FILTER, GL_NEAREST, /* param */
							  GL_NO_ERROR /* expected_error */);

	VerifyGLSamplerParameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST, /* param */
							  GL_NO_ERROR /* expected_error */);

	/* Make sure that glSamplerParameter*() functions report GL_INVALID_ENUM
	 * if called with GL_TEXTURE_BORDER_COLOR_EXT pname
	 */
	VerifyGLSamplerParameterf(m_glExtTokens.TEXTURE_BORDER_COLOR, 0.0f, /* param */
							  GL_INVALID_ENUM /* expected_error */);

	VerifyGLSamplerParameteri(m_glExtTokens.TEXTURE_BORDER_COLOR, 0, /* param */
							  GL_INVALID_ENUM /* expected_error */);

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

/** Check if calling glTexParameterf() with user-provided set of arguments causes
 *  a specified GL error.
 *
 *  Should the error codes differ, m_test_passed will be set to false.
 *
 * @param target         texture target to use for the call;
 * @param pname          property name to use for the call;
 * @param param          property value to use for the call;
 * @param expected_error expected error code.
 */
void TextureBorderClampParameterBorderColorErrorTest::VerifyGLTexParameterf(glw::GLenum target, glw::GLenum pname,
																			glw::GLfloat param,
																			glw::GLenum  expected_error)
{
	glw::GLenum			  error_code = GL_NO_ERROR;
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();

	gl.texParameterf(target, pname, param);

	error_code = gl.getError();
	if (expected_error != error_code)
	{
		m_test_passed = false;
		m_testCtx.getLog() << tcu::TestLog::Message << "glTexParameterf() failed:["
						   << "target:" << getTexTargetString(target) << ", pname:" << getPNameString(pname)
						   << "] reported error code:[" << glu::getErrorStr(error_code) << "] expected error code:["
						   << glu::getErrorStr(expected_error) << "]\n"
						   << tcu::TestLog::EndMessage;
	}
}

/** Check if calling glTexParameteri() with user-provided set of arguments causes
 *  a specified GL error.
 *
 *  Should the error codes differ, m_test_passed will be set to false.
 *
 * @param target         texture target to use for the call;
 * @param pname          property name to use for the call;
 * @param param          property value to use for the call;
 * @param expected_error expected error code.
 */
void TextureBorderClampParameterBorderColorErrorTest::VerifyGLTexParameteri(glw::GLenum target, glw::GLenum pname,
																			glw::GLint  param,
																			glw::GLenum expected_error)
{
	glw::GLenum			  error_code = GL_NO_ERROR;
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();

	gl.texParameteri(target, pname, param);

	error_code = gl.getError();
	if (expected_error != error_code)
	{
		m_test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glTexParameteri() failed:["
						   << "target:" << getTexTargetString(target) << ", pname:" << getPNameString(pname)
						   << "] reported error code:[" << glu::getErrorStr(error_code) << "] expected error code:["
						   << glu::getErrorStr(expected_error) << "]\n"
						   << tcu::TestLog::EndMessage;
	}
}

/** Check if calling glSamplerParameterf() with user-provided set of arguments causes
 *  a specified GL error.
 *
 *  Should the error codes differ, m_test_passed will be set to false.
 *
 * @param pname          property name to use for the call;
 * @param param          property value to use for the call;
 * @param expected_error expected error code.
 */
void TextureBorderClampParameterBorderColorErrorTest::VerifyGLSamplerParameterf(glw::GLenum pname, glw::GLfloat param,
																				glw::GLenum expected_error)
{
	glw::GLenum			  error_code = GL_NO_ERROR;
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();

	gl.samplerParameterf(m_sampler_id, pname, param);

	error_code = gl.getError();
	if (expected_error != error_code)
	{
		m_test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glSamplerParameterf() failed:["
						   << ", pname:" << getPNameString(pname) << "] reported error code:["
						   << glu::getErrorStr(error_code) << "] expected error code:["
						   << glu::getErrorStr(expected_error) << "]\n"
						   << tcu::TestLog::EndMessage;
	}
}

/** Check if calling glSamplerParameteri() with user-provided set of arguments causes
 *  a specified GL error.
 *
 *  Should the error codes differ, m_test_passed will be set to false.
 *
 * @param pname          property name to use for the call;
 * @param param          property value to use for the call;
 * @param expected_error expected error code.
 */
void TextureBorderClampParameterBorderColorErrorTest::VerifyGLSamplerParameteri(glw::GLenum pname, glw::GLint param,
																				glw::GLenum expected_error)
{
	glw::GLenum			  error_code = GL_NO_ERROR;
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();

	gl.samplerParameteri(m_sampler_id, pname, param);

	error_code = gl.getError();
	if (expected_error != error_code)
	{
		m_test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glSamplerParameteri() failed:["
						   << ", pname:" << getPNameString(pname) << "] reported error code:["
						   << glu::getErrorStr(error_code) << "] expected error code:["
						   << glu::getErrorStr(expected_error) << "]\n"
						   << tcu::TestLog::EndMessage;
	}
}

} // namespace glcts
