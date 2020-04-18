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
 * \file esextcTextureBorderClampTexParameterIErrors.cpp
 * \brief Texture Border Clamp glTexParameterIivEXT(), glTexParameterIuivEXT() Errors (Test 2)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBorderClampTexParameterIErrors.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBorderClampTexParameterIErrorsTest::TextureBorderClampTexParameterIErrorsTest(Context&			  context,
																					 const ExtParameters& extParams,
																					 const char*		  name,
																					 const char*		  description)
	: TextureBorderClampBase(context, extParams, name, description), m_test_passed(true)
{
	/* Left blank on purpose */
}

/** Initializes GLES objects used during the test.
 *
 */
void TextureBorderClampTexParameterIErrorsTest::initTest(void)
{
	if (!m_is_texture_border_clamp_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BORDER_CLAMP_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Initialize base class */
	TextureBorderClampBase::initTest();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult TextureBorderClampTexParameterIErrorsTest::iterate(void)
{
	initTest();

	/* Make sure that the functions report GL_INVALID_ENUM error if cube-map
	 *  face or GL_TEXTURE_BUFFER_EXT texture targets (if supported) is issued as
	 *  a texture target.*/
	if (m_is_texture_buffer_supported)
	{
		VerifyGLTexParameterIiv(m_glExtTokens.TEXTURE_BUFFER, GL_TEXTURE_BASE_LEVEL, 1, GL_INVALID_ENUM);
		VerifyGLTexParameterIuiv(m_glExtTokens.TEXTURE_BUFFER, GL_TEXTURE_BASE_LEVEL, 1, GL_INVALID_ENUM);
	}

	VerifyGLTexParameterIiv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_BASE_LEVEL, 1, GL_INVALID_ENUM);
	VerifyGLTexParameterIuiv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_BASE_LEVEL, 1, GL_INVALID_ENUM);

	/* Make sure that the functions report GL_INVALID_ENUM error if
	 * GL_TEXTURE_IMMUTABLE_FORMAT is passed by pname argument.*/
	VerifyGLTexParameterIivForAll(GL_TEXTURE_IMMUTABLE_FORMAT, 1, GL_INVALID_ENUM);
	VerifyGLTexParameterIuivForAll(GL_TEXTURE_IMMUTABLE_FORMAT, 1, GL_INVALID_ENUM);

	/* Make sure that the functions report GL_INVALID_VALUE error if the following
	 * pname+value combinations are used: */

	/* GL_TEXTURE_BASE_LEVEL  -1 (iv() version only)*/
	VerifyGLTexParameterIivTextureBaseLevelForAll(GL_TEXTURE_BASE_LEVEL, -1, GL_INVALID_VALUE);
	/* GL_TEXTURE_MAX_LEVEL, -1; (iv() version only) */
	VerifyGLTexParameterIivForAll(GL_TEXTURE_MAX_LEVEL, -1, GL_INVALID_VALUE);

	/* Make sure that the functions report GL_INVALID_ENUM error if the following
	 * pname+value combinations are used: */

	/* GL_TEXTURE_COMPARE_MODE, GL_NEAREST */
	VerifyGLTexParameterIivForAll(GL_TEXTURE_COMPARE_MODE, GL_NEAREST, GL_INVALID_ENUM);
	VerifyGLTexParameterIuivForAll(GL_TEXTURE_COMPARE_MODE, GL_NEAREST, GL_INVALID_ENUM);

	/* GL_TEXTURE_COMPARE_FUNC, GL_NEAREST;*/
	VerifyGLTexParameterIivForAll(GL_TEXTURE_COMPARE_FUNC, GL_NEAREST, GL_INVALID_ENUM);
	VerifyGLTexParameterIuivForAll(GL_TEXTURE_COMPARE_FUNC, GL_NEAREST, GL_INVALID_ENUM);

	/* GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST;*/
	VerifyGLTexParameterIivForAll(GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST, GL_INVALID_ENUM);
	VerifyGLTexParameterIuivForAll(GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST, GL_INVALID_ENUM);

	/* GL_TEXTURE_MIN_FILTER, GL_RED */
	VerifyGLTexParameterIivForAll(GL_TEXTURE_MIN_FILTER, GL_RED, GL_INVALID_ENUM);
	VerifyGLTexParameterIuivForAll(GL_TEXTURE_MIN_FILTER, GL_RED, GL_INVALID_ENUM);

	/* GL_TEXTURE_SWIZZLE_R, GL_NEAREST */
	VerifyGLTexParameterIivForAll(GL_TEXTURE_SWIZZLE_R, GL_NEAREST, GL_INVALID_ENUM);
	VerifyGLTexParameterIuivForAll(GL_TEXTURE_SWIZZLE_R, GL_NEAREST, GL_INVALID_ENUM);

	/* GL_TEXTURE_SWIZZLE_G, GL_NEAREST */
	VerifyGLTexParameterIivForAll(GL_TEXTURE_SWIZZLE_G, GL_NEAREST, GL_INVALID_ENUM);
	VerifyGLTexParameterIuivForAll(GL_TEXTURE_SWIZZLE_G, GL_NEAREST, GL_INVALID_ENUM);

	/* GL_TEXTURE_SWIZZLE_B, GL_NEAREST */
	VerifyGLTexParameterIivForAll(GL_TEXTURE_SWIZZLE_B, GL_NEAREST, GL_INVALID_ENUM);
	VerifyGLTexParameterIuivForAll(GL_TEXTURE_SWIZZLE_B, GL_NEAREST, GL_INVALID_ENUM);

	/* GL_TEXTURE_SWIZZLE_A, GL_NEAREST */
	VerifyGLTexParameterIivForAll(GL_TEXTURE_SWIZZLE_A, GL_NEAREST, GL_INVALID_ENUM);
	VerifyGLTexParameterIuivForAll(GL_TEXTURE_SWIZZLE_A, GL_NEAREST, GL_INVALID_ENUM);

	/* GL_TEXTURE_WRAP_S, GL_RED */
	VerifyGLTexParameterIivForAll(GL_TEXTURE_WRAP_S, GL_RED, GL_INVALID_ENUM);
	VerifyGLTexParameterIuivForAll(GL_TEXTURE_WRAP_S, GL_RED, GL_INVALID_ENUM);

	/* GL_TEXTURE_WRAP_T, GL_RED */
	VerifyGLTexParameterIivForAll(GL_TEXTURE_WRAP_T, GL_RED, GL_INVALID_ENUM);
	VerifyGLTexParameterIuivForAll(GL_TEXTURE_WRAP_T, GL_RED, GL_INVALID_ENUM);

	/* GL_TEXTURE_WRAP_R, GL_RED */
	VerifyGLTexParameterIivForAll(GL_TEXTURE_WRAP_R, GL_RED, GL_INVALID_ENUM);
	VerifyGLTexParameterIuivForAll(GL_TEXTURE_WRAP_R, GL_RED, GL_INVALID_ENUM);

	/* Make sure that the functions report GL_INVALID_ENUM error if the following
	 * pname+value pairs are used for GL_TEXTURE_2D_MULTISAMPLE or
	 * GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES targets: */
	if (m_is_texture_storage_multisample_supported)
	{
		/* GL_TEXTURE_COMPARE_MODE, GL_NONE */
		VerifyGLTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_MODE, GL_NONE, GL_INVALID_ENUM);
		VerifyGLTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_MODE, GL_NONE, GL_INVALID_ENUM);

		/* GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL */
		VerifyGLTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL, GL_INVALID_ENUM);
		VerifyGLTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL, GL_INVALID_ENUM);

		/* GL_TEXTURE_MAG_FILTER, GL_LINEAR */
		VerifyGLTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR, GL_INVALID_ENUM);
		VerifyGLTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR, GL_INVALID_ENUM);

		/* GL_TEXTURE_MAX_LOD, 1000 */
		VerifyGLTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAX_LOD, 1000, GL_INVALID_ENUM);
		VerifyGLTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAX_LOD, 1000, GL_INVALID_ENUM);

		/* GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR */
		VerifyGLTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR,
								GL_INVALID_ENUM);
		VerifyGLTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR,
								 GL_INVALID_ENUM);

		/* GL_TEXTURE_MIN_LOD, -1000 */
		VerifyGLTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_LOD, -1000, GL_INVALID_ENUM);

		/* GL_TEXTURE_WRAP_S, GL_REPEAT */
		VerifyGLTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_REPEAT, GL_INVALID_ENUM);
		VerifyGLTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_REPEAT, GL_INVALID_ENUM);

		/* GL_TEXTURE_WRAP_T, GL_REPEAT */
		VerifyGLTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_REPEAT, GL_INVALID_ENUM);
		VerifyGLTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_REPEAT, GL_INVALID_ENUM);

		/* GL_TEXTURE_WRAP_R, GL_REPEAT */
		VerifyGLTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_R, GL_REPEAT, GL_INVALID_ENUM);
		VerifyGLTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_R, GL_REPEAT, GL_INVALID_ENUM);

	} /* if (m_is_texture_storage_multisample_supported) */

	if (m_is_texture_storage_multisample_2d_array_supported)
	{
		/* GL_TEXTURE_COMPARE_MODE, GL_NONE */
		VerifyGLTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_COMPARE_MODE, GL_NONE, GL_INVALID_ENUM);
		VerifyGLTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_COMPARE_MODE, GL_NONE,
								 GL_INVALID_ENUM);

		/* GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL */
		VerifyGLTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL,
								GL_INVALID_ENUM);
		VerifyGLTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL,
								 GL_INVALID_ENUM);

		/* GL_TEXTURE_MAG_FILTER, GL_LINEAR */
		VerifyGLTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR, GL_INVALID_ENUM);
		VerifyGLTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR,
								 GL_INVALID_ENUM);

		/* GL_TEXTURE_MAX_LOD, 1000 */
		VerifyGLTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_MAX_LOD, 1000, GL_INVALID_ENUM);
		VerifyGLTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_MAX_LOD, 1000, GL_INVALID_ENUM);

		/* GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR */
		VerifyGLTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR,
								GL_INVALID_ENUM);
		VerifyGLTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR,
								 GL_INVALID_ENUM);

		/* GL_TEXTURE_MIN_LOD, -1000 */
		VerifyGLTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_MIN_LOD, -1000, GL_INVALID_ENUM);

		/* GL_TEXTURE_WRAP_S, GL_REPEAT */
		VerifyGLTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_WRAP_S, GL_REPEAT, GL_INVALID_ENUM);
		VerifyGLTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_WRAP_S, GL_REPEAT, GL_INVALID_ENUM);

		/* GL_TEXTURE_WRAP_T, GL_REPEAT */
		VerifyGLTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_WRAP_T, GL_REPEAT, GL_INVALID_ENUM);
		VerifyGLTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_WRAP_T, GL_REPEAT, GL_INVALID_ENUM);

		/* GL_TEXTURE_WRAP_R, GL_REPEAT */
		VerifyGLTexParameterIiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_WRAP_R, GL_REPEAT, GL_INVALID_ENUM);
		VerifyGLTexParameterIuiv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_WRAP_R, GL_REPEAT, GL_INVALID_ENUM);

	} /* if (m_is_texture_storage_multisample_2d_array_supported) */

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

/** Check if glTexParameterIivEXT() reports an user-specified error if called with
 *  provided arguments.
 *
 *  @param target         texture target to use for the call;
 *  @param pname          value of parameter name to use for the call;
 *  @param param          parameter value to use for the call;
 *  @param expected_error expected GL error code.
 */
void TextureBorderClampTexParameterIErrorsTest::VerifyGLTexParameterIiv(glw::GLenum target, glw::GLenum pname,
																		glw::GLint params, glw::GLenum expected_error)
{
	glw::GLenum			  error_code = GL_NO_ERROR;
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();

	gl.texParameterIiv(target, pname, &params);

	error_code = gl.getError();
	if (expected_error != error_code)
	{
		m_test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glTexParameterIivEXT() failed:["
						   << "target:" << getTexTargetString(target) << ", pname:" << getPNameString(pname)
						   << "] reported error code:[" << glu::getErrorStr(error_code) << "] expected error code:["
						   << glu::getErrorStr(expected_error) << "]\n"
						   << tcu::TestLog::EndMessage;
	}
}

/** Check if glTexParameterIivEXT() reports one of two user specified errors if called with
 *  provided arguments.
 *
 *  @param target          texture target to use for the call;
 *  @param pname           value of parameter name to use for the call;
 *  @param param           parameter value to use for the call;
 *  @param expected_error1 one of the expected GL error codes.
 *  @param expected_error2 one of the expected GL error codes.
 */
void TextureBorderClampTexParameterIErrorsTest::VerifyGLTexParameterIivMultipleAcceptedErrors(
	glw::GLenum target, glw::GLenum pname, glw::GLint params, glw::GLenum expected_error1, glw::GLenum expected_error2)
{
	glw::GLenum			  error_code = GL_NO_ERROR;
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();

	gl.texParameterIiv(target, pname, &params);

	error_code = gl.getError();
	if (expected_error1 != error_code && expected_error2 != error_code)
	{
		m_test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glTexParameterIivEXT() failed:["
						   << "target:" << getTexTargetString(target) << ", pname:" << getPNameString(pname)
						   << "] reported error code:[" << glu::getErrorStr(error_code) << "] expected error code:["
						   << glu::getErrorStr(expected_error1) << "] or error code ["
						   << glu::getErrorStr(expected_error2) << "]\n"
						   << tcu::TestLog::EndMessage;
	}
}

/** Check if glTexParameterIuivEXT() reports an user-specified error if called with
 *  provided arguments.
 *
 *  @param target         texture target to use for the call;
 *  @param pname          value of parameter name to use for the call;
 *  @param param          parameter value to use for the call;
 *  @param expected_error expected GL error code.
 */
void TextureBorderClampTexParameterIErrorsTest::VerifyGLTexParameterIuiv(glw::GLenum target, glw::GLenum pname,
																		 glw::GLuint params, glw::GLenum expected_error)
{
	glw::GLenum			  error_code = GL_NO_ERROR;
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();

	gl.texParameterIuiv(target, pname, &params);

	error_code = gl.getError();
	if (expected_error != error_code)
	{
		m_test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glTexParameterIuivEXT() failed:["
						   << "target:" << getTexTargetString(target) << ", pname:" << getPNameString(pname)
						   << "] reported error code:[" << glu::getErrorStr(error_code) << "] expected error code:["
						   << glu::getErrorStr(expected_error) << "]\n"
						   << tcu::TestLog::EndMessage;
	}
}

/** Check if glTexParameterIuivEXT() reports an user-specified error if called with
 *  provided arguments. Checks all supported texture targets.
 *
 *  @param pname          value of parameter name to use for the call;
 *  @param params         parameter value to use for the call;
 *  @param expected_error expected GL error code.
 */
void TextureBorderClampTexParameterIErrorsTest::VerifyGLTexParameterIivForAll(glw::GLenum pname, glw::GLint params,
																			  glw::GLenum expected_error)
{
	for (glw::GLuint i = 0; i < m_texture_target_list.size(); ++i)
	{
		VerifyGLTexParameterIiv(m_texture_target_list[i], /* target texture */
								pname, params, expected_error);
	}
}

/** Check if glTexParameterIuivEXT() reports an user-specified error if called with
 * GL_TEXTURE_BASE_LEVEL as the pname. Checks all supported texture targets. For
 * multisample targets a param different from 0 should give invalid operation error,
 * but for any target a negative param should give invalid value error, so for
 * multisample targets and a negative param, either invalid operation or invalid
 * value should be accepted.
 *
 *  @param pname          value of parameter name to use for the call;
 *  @param params         parameter value to use for the call;
 *  @param expected_error expected GL error code.
 */
void TextureBorderClampTexParameterIErrorsTest::VerifyGLTexParameterIivTextureBaseLevelForAll(
	glw::GLenum pname, glw::GLint params, glw::GLenum expected_error)
{
	for (glw::GLuint i = 0; i < m_texture_target_list.size(); ++i)
	{
		if (GL_TEXTURE_2D_MULTISAMPLE == m_texture_target_list[i] ||
			GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES == m_texture_target_list[i])
		{
			/* Negative params can give either invalid operation or invalid value for multisample textures. */
			if (0 > params)
			{
				VerifyGLTexParameterIivMultipleAcceptedErrors(m_texture_target_list[i], /* target texture */
															  pname, params, GL_INVALID_OPERATION, GL_INVALID_VALUE);
			}
			/* Non-zero params give invalid operation for multisample textures. */
			else
			{
				VerifyGLTexParameterIiv(m_texture_target_list[i], /* target texture */
										pname, params, GL_INVALID_OPERATION);
			}
		}
		else
		{
			VerifyGLTexParameterIiv(m_texture_target_list[i], /* target texture */
									pname, params, expected_error);
		}
	}
}

/** Check if glTexParameterIuivEXT() reports an user-specified error if called with
 *  provided arguments. Checks all supported texture targets.
 *
 *  @param pname          value of parameter name to use for the call;
 *  @param params         parameter value to use for the call;
 *  @param expected_error expected GL error code.
 */
void TextureBorderClampTexParameterIErrorsTest::VerifyGLTexParameterIuivForAll(glw::GLenum pname, glw::GLuint params,
																			   glw::GLenum expected_error)
{
	for (glw::GLuint i = 0; i < m_texture_target_list.size(); ++i)
	{
		VerifyGLTexParameterIuiv(m_texture_target_list[i] /* target texture */, pname, params, expected_error);
	}
}

} // namespace glcts
