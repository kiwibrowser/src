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
 * \file  esextcTextureBorderClampSamplerParameterIError.cpp
 * \brief Verifies glGetSamplerParameterI*() and glSamplerParameterI*()
 *        entry-points generate errors for non-generated sampler objects
 *        as per extension specification. (Test 4)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBorderClampSamplerParameterIError.hpp"
#include "esextcTextureBorderClampBase.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{
/* Static constants */
const glw::GLuint TextureBorderClampSamplerParameterIErrorTest::m_buffer_size		 = 32;
const glw::GLuint TextureBorderClampSamplerParameterIErrorTest::m_texture_unit_index = 0;

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBorderClampSamplerParameterIErrorTest::TextureBorderClampSamplerParameterIErrorTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TextureBorderClampBase(context, extParams, name, description), m_sampler_id(0), m_test_passed(true)
{
	/* No implementation needed */
}

/** Deinitializes GLES objects created during the test.
 *
 */
void TextureBorderClampSamplerParameterIErrorTest::deinit(void)
{
	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind the default sampler object */
	gl.bindSampler(m_texture_unit_index, 0);

	/* Delete a sampler object, if one was created during test execution */
	if (0 != m_sampler_id)
	{
		gl.deleteSamplers(1, &m_sampler_id);

		m_sampler_id = 0;
	}

	/* Deinitialize base class instance */
	TestCaseBase::deinit();
}

/** Initializes GLES objects used during the test. */
void TextureBorderClampSamplerParameterIErrorTest::initTest(void)
{
	if (!m_is_texture_border_clamp_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BORDER_CLAMP_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate a sampler object */
	gl.genSamplers(1, &m_sampler_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating sampler object");

	/* Bind the sampler object to a texture unit */
	gl.bindSampler(m_texture_unit_index, m_sampler_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding sampler object");

	/* Fill list with property name + property value pairs that should
	 * be used to verify that glGetSamplerParameterI*() and glSamplerParameterI*()
	 * entry-points generate errors for non-generated sampler objects
	 * as per extension specification.
	 */
	m_pnames_list.push_back(PnameParams(GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	m_pnames_list.push_back(PnameParams(GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR));
	m_pnames_list.push_back(PnameParams(GL_TEXTURE_MIN_LOD, 1));
	m_pnames_list.push_back(PnameParams(GL_TEXTURE_MAX_LOD, 1));
	m_pnames_list.push_back(PnameParams(GL_TEXTURE_WRAP_S, GL_REPEAT));
	m_pnames_list.push_back(PnameParams(GL_TEXTURE_WRAP_T, GL_REPEAT));
	m_pnames_list.push_back(PnameParams(GL_TEXTURE_WRAP_R, GL_REPEAT));
	m_pnames_list.push_back(PnameParams(m_glExtTokens.TEXTURE_BORDER_COLOR, 0, 0, 0, 0));
	m_pnames_list.push_back(PnameParams(GL_TEXTURE_COMPARE_MODE, GL_NONE));
	m_pnames_list.push_back(PnameParams(GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL));
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult TextureBorderClampSamplerParameterIErrorTest::iterate(void)
{
	initTest();

	/* Verify glGetSamplerParameterIivEXT() and glGetSamplerParameterIuivEXT(),
	 * report GL_NO_ERROR if called for a generated sampler object*/
	VerifyGLGetCallsForAllPNames(m_sampler_id, GL_NO_ERROR);

	/* Verify glGetSamplerParameterIivEXT() and glGetSamplerParameterIuivEXT(),
	 * report GL_INVALID_OPERATION if called for a non-existent sampler object*/
	VerifyGLGetCallsForAllPNames(m_sampler_id + 1, GL_INVALID_OPERATION);

	/* Verify glSamplerParameterIivEXT() and glSamplerParameterIuivEXT(),
	 * report GL_NO_ERROR if called for a generated sampler object*/
	VerifyGLSamplerCallsForAllPNames(m_sampler_id, GL_NO_ERROR);

	/* Verify glSamplerParameterIivEXT() and glSamplerParameterIuivEXT(),
	 * report GL_INVALID_OPERATION if called for a non-existent sampler object*/
	VerifyGLSamplerCallsForAllPNames(m_sampler_id + 2, GL_INVALID_OPERATION);

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
 * Checks if glGetSamplerParameterIivEXT() reports an user-specified GL error, if
 * called with specific arguments.
 *
 * Should the check fail, the function will set m_test_passed to false.
 *
 * @param sampler_id     id of sampler object to use for the call;
 * @param pname          pname to use for the call;
 * @param expected_error expected GL error code.
 */
void TextureBorderClampSamplerParameterIErrorTest::VerifyGLGetSamplerParameterIiv(glw::GLuint sampler_id,
																				  glw::GLenum pname,
																				  glw::GLenum expected_error)
{
	glw::GLenum				error_code = GL_NO_ERROR;
	const glw::Functions&   gl		   = m_context.getRenderContext().getFunctions();
	std::vector<glw::GLint> params(m_buffer_size);

	gl.getSamplerParameterIiv(sampler_id, pname, &params[0]);

	error_code = gl.getError();
	if (expected_error != error_code)
	{
		m_test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glGetSamplerParameterIivEXT() failed:["
						   << "sampler id:" << sampler_id << ", pname:" << getPNameString(pname)
						   << "] reported error code:[" << glu::getErrorStr(error_code) << "] expected error code:["
						   << glu::getErrorStr(expected_error) << "]\n"
						   << tcu::TestLog::EndMessage;
	}
}

/**
 * Checks if glGetSamplerParameterIuivEXT() reports an user-specified GL error, if
 * called with specific arguments.
 *
 * Should the check fail, the function will set m_test_passed to false.
 *
 * @param sampler_id     id of sampler object to use for the call;
 * @param pname          pname to use for the call;
 * @param expected_error expected GL error code.
 */
void TextureBorderClampSamplerParameterIErrorTest::VerifyGLGetSamplerParameterIuiv(glw::GLuint sampler_id,
																				   glw::GLenum pname,
																				   glw::GLenum expected_error)
{
	glw::GLenum				 error_code = GL_NO_ERROR;
	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	std::vector<glw::GLuint> params(m_buffer_size);

	gl.getSamplerParameterIuiv(sampler_id, pname, &params[0]);

	error_code = gl.getError();
	if (expected_error != error_code)
	{
		m_test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glGetSamplerParameterIuivEXT() failed:["
						   << "sampler id:" << sampler_id << ", pname:" << getPNameString(pname)
						   << "] reported error code:[" << glu::getErrorStr(error_code) << "] expected error code:["
						   << glu::getErrorStr(expected_error) << "]\n"
						   << tcu::TestLog::EndMessage;
	}
}

/** Verifies all pnames stored in m_pnames_list generate an user-specified error when used with
 *  paired value for glGetSamplerParameterIivEXT() and glGetSamplerParameterIuivEXT() functions.
 *
 * @param sampler_id     id of a sampler object to use for the calls;
 * @param expected_error expected GL error code.
 */
void TextureBorderClampSamplerParameterIErrorTest::VerifyGLGetCallsForAllPNames(glw::GLuint sampler_id,
																				glw::GLenum expected_error)
{
	for (glw::GLuint i = 0; i < m_pnames_list.size(); ++i)
	{
		/* Check glGetSamplerParameterIivEXT() */
		VerifyGLGetSamplerParameterIiv(sampler_id, m_pnames_list[i].pname, expected_error);

		/* Check glGetSamplerParameterIuivEXT() */
		VerifyGLGetSamplerParameterIuiv(sampler_id, m_pnames_list[i].pname, expected_error);
	} /* for (all pname+value pairs) */
}

/**
 * Checks if glSamplerParameterIivEXT() reports an user-specified GL error, if
 * called with specific arguments.
 *
 * Should the check fail, the function will set m_test_passed to false.
 *
 * @param sampler_id     id of sampler object to use for the call;
 * @param pname          pname argument value to use for the call;
 * @param params         params argument value to use for the call;
 * @param expected_error expected GL error code.
 */
void TextureBorderClampSamplerParameterIErrorTest::VerifyGLSamplerParameterIiv(glw::GLuint sampler_id,
																			   glw::GLenum pname, glw::GLint* params,
																			   glw::GLenum expected_error)
{
	glw::GLenum			  error_code = GL_NO_ERROR;
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();

	gl.samplerParameterIiv(sampler_id, pname, params);

	error_code = gl.getError();
	if (expected_error != error_code)
	{
		m_test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glSamplerParameterIivEXT() failed:["
						   << "sampler id:" << sampler_id << ", pname:" << getPNameString(pname)
						   << "] reported error code:[" << glu::getErrorStr(error_code) << "] expected error code:["
						   << glu::getErrorStr(expected_error) << "]\n"
						   << tcu::TestLog::EndMessage;
	}
}

/**
 * Checks if glSamplerParameterIuivEXT() reports an user-specified GL error, if
 * called with specific arguments.
 *
 * Should the check fail, the function will set m_test_passed to false.
 *
 * @param sampler_id     id of sampler object to use for the call;
 * @param pname          pname argument value to use for the call;
 * @param params         params argument value to use for the call;
 * @param expected_error expected GL error code.
 **/
void TextureBorderClampSamplerParameterIErrorTest::VerifyGLSamplerParameterIuiv(glw::GLuint sampler_id,
																				glw::GLenum pname, glw::GLuint* params,
																				glw::GLenum expected_error)
{
	glw::GLenum			  error_code = GL_NO_ERROR;
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();

	gl.samplerParameterIuiv(sampler_id, pname, params);

	error_code = gl.getError();
	if (expected_error != error_code)
	{
		m_test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glSamplerParameterIuivEXT() failed:["
						   << "sampler id:" << sampler_id << ", pname:" << getPNameString(pname)
						   << "] reported error code:[" << glu::getErrorStr(error_code) << "] expected error code:["
						   << glu::getErrorStr(expected_error) << "]\n"
						   << tcu::TestLog::EndMessage;
	}
}

/** Verifies all pnames stored in m_pnames_list generate an user-specified error when used with
 *  paired value for glSamplerParameterIivEXT() and glSamplerParameterIuivEXT() functions.
 *
 * @param sampler_id     id of a sampler object to use for the calls;
 * @param expected_error expected GL error code.
 */
void TextureBorderClampSamplerParameterIErrorTest::VerifyGLSamplerCallsForAllPNames(glw::GLuint sampler_id,
																					glw::GLenum expected_error)
{
	for (glw::GLuint i = 0; i < m_pnames_list.size(); ++i)
	{
		/* Check glSamplerParameterIivEXT() */
		VerifyGLSamplerParameterIiv(sampler_id, m_pnames_list[i].pname, (glw::GLint*)m_pnames_list[i].params,
									expected_error);
		/* Check glSamplerParameterIuivEXT() */
		VerifyGLSamplerParameterIuiv(sampler_id, m_pnames_list[i].pname, (glw::GLuint*)m_pnames_list[i].params,
									 expected_error);
	} /* for (all pname+value pairs) */
}

} // namespace glcts
