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
 * \file  esextcTextureBorderClampSamplerParameterIWithWrongPname.hpp
 * \brief Verifies glGetSamplerParameterI*() and glSamplerParameterI*()
 *        entry-points generate GL_INVALID_ENUM error if invalid pnames
 *        are used. (Test 5)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBorderClampSamplerParameterIWithWrongPname.hpp"
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
const glw::GLuint TextureBorderClampSamplerParameterIWithWrongPnameTest::m_buffer_size		  = 32;
const glw::GLuint TextureBorderClampSamplerParameterIWithWrongPnameTest::m_texture_unit_index = 0;

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBorderClampSamplerParameterIWithWrongPnameTest::TextureBorderClampSamplerParameterIWithWrongPnameTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TextureBorderClampBase(context, extParams, name, description), m_sampler_id(0), m_test_passed(true)
{
	/* No implementation needed */
}

/** Deinitializes GLES objects created during the test.
 *
 */
void TextureBorderClampSamplerParameterIWithWrongPnameTest::deinit(void)
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

/** Initializes GLES objects used during the test.
 *
 */
void TextureBorderClampSamplerParameterIWithWrongPnameTest::initTest(void)
{
	/* Check if EXT_texture_border_clamp is supported */
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

	/* Store all enums that should generate GL_INVALID_ENUM error when used
	 * as parameter names for glGetSamplerParameterI*() and glSamplerParameterI*()
	 * calls.
	 */
	m_pnames_list.push_back(GL_TEXTURE_BASE_LEVEL);
	m_pnames_list.push_back(GL_TEXTURE_IMMUTABLE_FORMAT);
	m_pnames_list.push_back(GL_TEXTURE_MAX_LEVEL);
	m_pnames_list.push_back(GL_TEXTURE_SWIZZLE_R);
	m_pnames_list.push_back(GL_TEXTURE_SWIZZLE_G);
	m_pnames_list.push_back(GL_TEXTURE_SWIZZLE_B);
	m_pnames_list.push_back(GL_TEXTURE_SWIZZLE_A);
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult TextureBorderClampSamplerParameterIWithWrongPnameTest::iterate(void)
{
	initTest();

	for (glw::GLuint i = 0; i < m_pnames_list.size(); ++i)
	{
		/* Check glGetSamplerParameterIivEXT() */
		VerifyGLGetSamplerParameterIiv(m_sampler_id, m_pnames_list[i], GL_INVALID_ENUM);

		/* Check glGetSamplerParameterIuivEXT() */
		VerifyGLGetSamplerParameterIuiv(m_sampler_id, m_pnames_list[i], GL_INVALID_ENUM);

		/* Check glSamplerParameterIivEXT() */
		VerifyGLSamplerParameterIiv(m_sampler_id, m_pnames_list[i], GL_INVALID_ENUM);

		/* Check glSamplerParameterIuivEXT() */
		VerifyGLSamplerParameterIuiv(m_sampler_id, m_pnames_list[i], GL_INVALID_ENUM);
	} /* for (all pnames) */

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
void TextureBorderClampSamplerParameterIWithWrongPnameTest::VerifyGLGetSamplerParameterIiv(glw::GLuint sampler_id,
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
void TextureBorderClampSamplerParameterIWithWrongPnameTest::VerifyGLGetSamplerParameterIuiv(glw::GLuint sampler_id,
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

/**
 * Checks if glSamplerParameterIivEXT() reports an user-specified GL error, if
 * called with specific arguments.
 *
 * Should the check fail, the function will set m_test_passed to false.
 *
 * @param sampler_id     id of sampler object to use for the call;
 * @param pname          pname argument value to use for the call;
 * @param expected_error expected GL error code.
 */
void TextureBorderClampSamplerParameterIWithWrongPnameTest::VerifyGLSamplerParameterIiv(glw::GLuint sampler_id,
																						glw::GLenum pname,
																						glw::GLenum expected_error)
{
	glw::GLenum				error_code = GL_NO_ERROR;
	const glw::Functions&   gl		   = m_context.getRenderContext().getFunctions();
	std::vector<glw::GLint> params(m_buffer_size);

	gl.samplerParameterIiv(sampler_id, pname, &params[0]);

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
 * @param expected_error expected GL error code.
 **/
void TextureBorderClampSamplerParameterIWithWrongPnameTest::VerifyGLSamplerParameterIuiv(glw::GLuint sampler_id,
																						 glw::GLenum pname,
																						 glw::GLenum expected_error)
{
	glw::GLenum				 error_code = GL_NO_ERROR;
	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	std::vector<glw::GLuint> params(m_buffer_size);

	gl.samplerParameterIuiv(sampler_id, pname, &params[0]);

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

} // namespace glcts
