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
 * \file  esextcTextureBorderClampParameterTextureBorderColor.cpp
 * \brief Verify that GL_TEXTURE_BORDER_COLOR_EXT state is correctly retrieved
 *        by glGetSamplerParameter*() and glGetTexParameter*() functions. (Test 6)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBorderClampParameterTextureBorderColor.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <vector>

namespace glcts
{

/* Max number of elements in buffers allocated by the test */
const glw::GLuint TextureBorderClampParameterTextureBorderColor::m_buffer_length = 4;
/* Index of texture unit used in the test */
const glw::GLuint TextureBorderClampParameterTextureBorderColor::m_texture_unit_index = 0;

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBorderClampParameterTextureBorderColor::TextureBorderClampParameterTextureBorderColor(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TextureBorderClampBase(context, extParams, name, description), m_sampler_id(0), m_to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes GLES objects created during the test. */
void TextureBorderClampParameterTextureBorderColor::deinit(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (glw::GLuint i = 0; i < m_texture_targets.size(); ++i)
	{
		gl.bindTexture(m_texture_targets[i], 0);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");
	}

	gl.bindSampler(m_texture_unit_index, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding sampler object to texture unit!");

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);

		m_to_id = 0;
	}

	if (m_sampler_id != 0)
	{
		gl.deleteSamplers(1, &m_sampler_id);

		m_sampler_id = 0;
	}

	/* Deinitialize base class */
	TestCaseBase::deinit();
}

/** Initializes GLES objects used during the test. */
void TextureBorderClampParameterTextureBorderColor::initTest(void)
{
	/* Check whether GL_EXT_texture_border_clamp is supported */
	if (!m_is_texture_border_clamp_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BORDER_CLAMP_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Fill array with texture targets used in the test */
	m_texture_targets.push_back(GL_TEXTURE_2D);
	m_texture_targets.push_back(GL_TEXTURE_2D_ARRAY);
	m_texture_targets.push_back(GL_TEXTURE_3D);
	m_texture_targets.push_back(GL_TEXTURE_CUBE_MAP);

	/* Also consider GL_TEXTURE_CUBE_MAP_ARRAY_EXT, but only if
	 * GL_EXT_texture_cube_map_array is supported
	 */
	if (m_is_texture_cube_map_array_supported)
	{
		m_texture_targets.push_back(GL_TEXTURE_CUBE_MAP_ARRAY);
	}
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult TextureBorderClampParameterTextureBorderColor::iterate(void)
{
	initTest();

	/* Get GL entry points */
	const glw::Functions& gl		  = m_context.getRenderContext().getFunctions();
	glw::GLboolean		  test_passed = true;

	/* Iterate through all texture targets */
	for (glw::GLuint i = 0; i < m_texture_targets.size(); ++i)
	{
		gl.genTextures(1, &m_to_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating a texture object!");

		gl.bindTexture(m_texture_targets[i], m_to_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding a texture object!");

		gl.genSamplers(1, &m_sampler_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating a sampler object!");

		gl.bindSampler(m_texture_unit_index, m_sampler_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding a sampler object to a texture unit!");

		/* Verify default border color is set to (0.0, 0.0, 0.0, 0.0) and (0, 0, 0, 0) */
		std::vector<glw::GLfloat> data_fp_zeros(m_buffer_length);

		data_fp_zeros[0] = 0.0f;
		data_fp_zeros[1] = 0.0f;
		data_fp_zeros[2] = 0.0f;
		data_fp_zeros[3] = 0.0f;

		std::vector<glw::GLint> data_int_zeros(m_buffer_length);

		data_int_zeros[0] = 0;
		data_int_zeros[1] = 0;
		data_int_zeros[2] = 0;
		data_int_zeros[3] = 0;

		std::vector<glw::GLuint> data_uint_zeros(m_buffer_length);

		data_uint_zeros[0] = 0;
		data_uint_zeros[1] = 0;
		data_uint_zeros[2] = 0;
		data_uint_zeros[3] = 0;

		if (!verifyGLGetTexParameterfvResult(m_texture_targets[i], &data_fp_zeros[0]))
		{
			test_passed = false;
		}

		if (!verifyGLGetTexParameterivResult(m_texture_targets[i], &data_int_zeros[0]))
		{
			test_passed = false;
		}

		if (!verifyGLGetTexParameterIivResult(m_texture_targets[i], &data_int_zeros[0]))
		{
			test_passed = false;
		}

		if (!verifyGLGetTexParameterIuivResult(m_texture_targets[i], &data_uint_zeros[0]))
		{
			test_passed = false;
		}

		/* Verify setting signed integer border color of (-1, -2, 3, 4) using
		 * glSamplerParameterIivEXT() / glTexParameterIivEXT() call affects the values
		 * later reported by glGetSamplerParameterIivEXT() and glGetTexParameterIivEXT().
		 * These values should match.
		 */
		std::vector<glw::GLint> data_int(m_buffer_length);

		data_int[0] = -1;
		data_int[1] = -2;
		data_int[2] = 3;
		data_int[3] = 4;

		gl.texParameterIiv(m_texture_targets[i], m_glExtTokens.TEXTURE_BORDER_COLOR, &data_int[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error setting GL_TEXTURE_BORDER_COLOR_EXT parameter using glTexParameterIivEXT()");

		if (!verifyGLGetTexParameterIivResult(m_texture_targets[i], &data_int[0]))
		{
			test_passed = false;
		}

		if (!verifyGLGetSamplerParameterIivResult(m_sampler_id, m_texture_targets[i], &data_int_zeros[0]))
		{
			test_passed = false;
		}

		gl.texParameterIiv(m_texture_targets[i], m_glExtTokens.TEXTURE_BORDER_COLOR, &data_int_zeros[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error setting GL_TEXTURE_BORDER_COLOR_EXT parameter using glTexParameterIivEXT()");

		gl.samplerParameterIiv(m_sampler_id, m_glExtTokens.TEXTURE_BORDER_COLOR, &data_int[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error setting GL_TEXTURE_BORDER_COLOR_EXT parameter using glSamplerParameterIivEXT()");

		if (!verifyGLGetTexParameterIivResult(m_texture_targets[i], &data_int_zeros[0]))
		{
			test_passed = false;
		}

		if (!verifyGLGetSamplerParameterIivResult(m_sampler_id, m_texture_targets[i], &data_int[0]))
		{
			test_passed = false;
		}

		gl.samplerParameterIiv(m_sampler_id, m_glExtTokens.TEXTURE_BORDER_COLOR, &data_int_zeros[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error setting GL_TEXTURE_BORDER_COLOR_EXT parameter using glSamplerParameterIivEXT()");

		/* Verify setting unsigned integer border color of (1, 2, 3, 4) using
		 * glSamplerParameterIuivEXT() / glTexParameterIuivEXT() call affects the values
		 * later reported by glGetSamplerParameterIuivEXT() and glGetTexParameterIuivEXT().
		 * These values should match.
		 */
		std::vector<glw::GLuint> data_uint(m_buffer_length);

		data_uint[0] = 1;
		data_uint[1] = 2;
		data_uint[2] = 3;
		data_uint[3] = 4;

		gl.texParameterIuiv(m_texture_targets[i], m_glExtTokens.TEXTURE_BORDER_COLOR, &data_uint[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error setting GL_TEXTURE_BORDER_COLOR_EXT parameter using glTexParameterIuivEXT()");

		if (!verifyGLGetTexParameterIuivResult(m_texture_targets[i], &data_uint[0]))
		{
			test_passed = false;
		}

		if (!verifyGLGetSamplerParameterIuivResult(m_sampler_id, m_texture_targets[i], &data_uint_zeros[0]))
		{
			test_passed = false;
		}

		gl.texParameterIuiv(m_texture_targets[i], m_glExtTokens.TEXTURE_BORDER_COLOR, &data_uint_zeros[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error setting GL_TEXTURE_BORDER_COLOR_EXT parameter using glTexParameterIuivEXT()");

		gl.samplerParameterIuiv(m_sampler_id, m_glExtTokens.TEXTURE_BORDER_COLOR, &data_uint[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error setting GL_TEXTURE_BORDER_COLOR_EXT parameter using glSamplerParameterIuivEXT()");

		if (!verifyGLGetTexParameterIuivResult(m_texture_targets[i], &data_uint_zeros[0]))
		{
			test_passed = false;
		}

		if (!verifyGLGetSamplerParameterIuivResult(m_sampler_id, m_texture_targets[i], &data_uint[0]))
		{
			test_passed = false;
		}

		gl.samplerParameterIuiv(m_sampler_id, m_glExtTokens.TEXTURE_BORDER_COLOR, &data_uint_zeros[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error setting GL_TEXTURE_BORDER_COLOR_EXT parameter using glSamplerParameterIuivEXT()");

		/* Verify setting floating-point border color of (0.1, 0.2, 0.3, 0.4)
		 * affects the values later reported by glGetSamplerParameterfv() /
		 * glGetTexParameterfv(). These values should match.
		 */
		std::vector<glw::GLfloat> data_fp(m_buffer_length);

		data_fp[0] = 0.1f;
		data_fp[1] = 0.2f;
		data_fp[2] = 0.3f;
		data_fp[3] = 0.4f;

		gl.texParameterfv(m_texture_targets[i], m_glExtTokens.TEXTURE_BORDER_COLOR, &data_fp[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error setting GL_TEXTURE_BORDER_COLOR_EXT parameter using glTexParameterfv()");

		if (!verifyGLGetTexParameterfvResult(m_texture_targets[i], &data_fp[0]))
		{
			test_passed = false;
		}

		if (!verifyGLGetSamplerParameterfvResult(m_sampler_id, m_texture_targets[i], &data_fp_zeros[0]))
		{
			test_passed = false;
		}

		gl.texParameterfv(m_texture_targets[i], m_glExtTokens.TEXTURE_BORDER_COLOR, &data_fp_zeros[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error setting GL_TEXTURE_BORDER_COLOR_EXT parameter using glTexParameterfv()");

		gl.samplerParameterfv(m_sampler_id, m_glExtTokens.TEXTURE_BORDER_COLOR, &data_fp[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error setting GL_TEXTURE_BORDER_COLOR_EXT parameter using glSamplerParameterfv()");

		if (!verifyGLGetTexParameterfvResult(m_texture_targets[i], &data_fp_zeros[0]))
		{
			test_passed = false;
		}

		if (!verifyGLGetSamplerParameterfvResult(m_sampler_id, m_texture_targets[i], &data_fp[0]))
		{
			test_passed = false;
		}

		gl.samplerParameterfv(m_sampler_id, m_glExtTokens.TEXTURE_BORDER_COLOR, &data_fp_zeros[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error setting GL_TEXTURE_BORDER_COLOR_EXT parameter using glSamplerParameterfv()");

		/* Verify setting integer border color of
		 * (0, 1, 2, 4) using glSamplerParameteriv()
		 * / glTexParameteriv() affects the values later reported by
		 * glGetSamplerParameteriv() / glGetTexParameteriv(). The returned values
		 * should correspond to the outcome of equation 2.2 from ES3.0.2 spec
		 * applied to each component.
		 */
		data_int[0] = 0;
		data_int[1] = 1;
		data_int[2] = 2;
		data_int[3] = 4;

		gl.texParameteriv(m_texture_targets[i], m_glExtTokens.TEXTURE_BORDER_COLOR, &data_int[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error setting GL_TEXTURE_BORDER_COLOR_EXT parameter using glTexParameteriv()");

		if (!verifyGLGetTexParameterivResult(m_texture_targets[i], &data_int[0]))
		{
			test_passed = false;
		}

		if (!verifyGLGetSamplerParameterivResult(m_sampler_id, m_texture_targets[i], &data_int_zeros[0]))
		{
			test_passed = false;
		}

		gl.texParameteriv(m_texture_targets[i], m_glExtTokens.TEXTURE_BORDER_COLOR, &data_int_zeros[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error setting GL_TEXTURE_BORDER_COLOR_EXT parameter using glTexParameteriv()");

		gl.samplerParameteriv(m_sampler_id, m_glExtTokens.TEXTURE_BORDER_COLOR, &data_int[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error setting GL_TEXTURE_BORDER_COLOR_EXT parameter using glSamplerParameteriv()");

		if (!verifyGLGetTexParameterivResult(m_texture_targets[i], &data_int_zeros[0]))
		{
			test_passed = false;
		}

		if (!verifyGLGetSamplerParameterivResult(m_sampler_id, m_texture_targets[i], &data_int[0]))
		{
			test_passed = false;
		}

		gl.samplerParameteriv(m_sampler_id, m_glExtTokens.TEXTURE_BORDER_COLOR, &data_int_zeros[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error setting GL_TEXTURE_BORDER_COLOR_EXT parameter using glSamplerParameteriv()");

		/* Deinitialize the texture object */
		gl.bindTexture(m_texture_targets[i], 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");

		gl.deleteTextures(1, &m_to_id);
		m_to_id = 0;

		gl.bindSampler(m_texture_unit_index, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding sampler object to texture unit!");

		gl.deleteSamplers(1, &m_sampler_id);
		m_sampler_id = 0;
	}

	/* Has the test passed? */
	if (test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Check if glGetSamplerParameterfv() returns correct border color for GL_TEXTURE_BORDER_COLOR_EXT pname
 *
 * @param sampler_id     ID of sampler object;
 * @param target         texture target to do the call for;
 * @param expected_data  pointer to buffer with expected data
 * @return               true if both buffers are a match.
 */
bool TextureBorderClampParameterTextureBorderColor::verifyGLGetSamplerParameterfvResult(
	glw::GLuint sampler_id, glw::GLenum target, const glw::GLfloat* expected_data)
{
	std::vector<glw::GLfloat> buffer(m_buffer_length);
	std::stringstream		  expectedDataStream;
	std::stringstream		  returnedDataStream;
	const glw::Functions&	 gl = m_context.getRenderContext().getFunctions();

	memset(&buffer[0], 0, m_buffer_length * sizeof(glw::GLfloat));

	gl.getSamplerParameterfv(sampler_id, m_glExtTokens.TEXTURE_BORDER_COLOR, &buffer[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error gettnig  parameter for sampler.");

	for (glw::GLuint i = 0; i < m_buffer_length; ++i)
	{
		if (expected_data[i] != buffer[i])
		{
			for (glw::GLuint j = 0; j < m_buffer_length; ++j)
			{
				expectedDataStream << expected_data[j] << ",";
				returnedDataStream << buffer[j] << ",";
			}

			getTestContext().getLog() << tcu::TestLog::Message
									  << "Wrong value encountered when calling glGetSamplerParameterfv() with "
										 "GL_TEXTURE_BORDER_COLOR_EXT pname;"
									  << " texture target:" << getTexTargetString(target) << "\n"
									  << " expected values:[" << expectedDataStream.str() << "]"
									  << " result values:[" << returnedDataStream.str() << "]\n"
									  << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** Check if glGetSamplerParameteriv() returns correct border color for GL_TEXTURE_BORDER_COLOR_EXT pname
 *
 * @param sampler_id     ID of sampler object;
 * @param target         texture target to do the call for;
 * @param expected_data  pointer to buffer with expected data
 * @return               true if both buffers are a match.
 */
bool TextureBorderClampParameterTextureBorderColor::verifyGLGetSamplerParameterivResult(glw::GLuint		  sampler_id,
																						glw::GLenum		  target,
																						const glw::GLint* expected_data)
{
	std::vector<glw::GLint> buffer(m_buffer_length);
	std::stringstream		expectedDataStream;
	std::stringstream		returnedDataStream;
	const glw::Functions&   gl = m_context.getRenderContext().getFunctions();

	memset(&buffer[0], 0, m_buffer_length * sizeof(glw::GLint));

	gl.getSamplerParameteriv(sampler_id, m_glExtTokens.TEXTURE_BORDER_COLOR, &buffer[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting  parameter for sampler.");

	for (glw::GLuint i = 0; i < m_buffer_length; ++i)
	{
		if (expected_data[i] != buffer[i])
		{
			for (glw::GLuint j = 0; j < m_buffer_length; ++j)
			{
				expectedDataStream << expected_data[j] << ",";
				returnedDataStream << buffer[j] << ",";
			}
			getTestContext().getLog() << tcu::TestLog::Message
									  << "Wrong value encountered when calling glGetSamplerParameteriv() with "
										 "GL_TEXTURE_BORDER_COLOR_EXT pname;"
									  << " texture target:" << getTexTargetString(target) << "\n"
									  << " expected values:[" << expectedDataStream.str() << "]"
									  << " result values:[" << returnedDataStream.str() << "]\n"
									  << tcu::TestLog::EndMessage;
			return false;
		}
	}

	return true;
}

/** Check if glGetSamplerParameterIivEXT() returns correct border color for GL_TEXTURE_BORDER_COLOR_EXT pname
 *
 * @param sampler_id     ID of sampler object;
 * @param target         texture target to do the call for;
 * @param expected_data  pointer to buffer with expected data
 * @return               true if both buffers are a match.
 */
bool TextureBorderClampParameterTextureBorderColor::verifyGLGetSamplerParameterIivResult(
	glw::GLuint sampler_id, glw::GLenum target, const glw::GLint* expected_data)
{
	std::vector<glw::GLint> buffer(m_buffer_length);
	std::stringstream		expectedDataStream;
	std::stringstream		returnedDataStream;
	const glw::Functions&   gl = m_context.getRenderContext().getFunctions();

	memset(&buffer[0], 0, m_buffer_length * sizeof(glw::GLint));

	gl.getSamplerParameterIiv(sampler_id, m_glExtTokens.TEXTURE_BORDER_COLOR, &buffer[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting  parameter for sampler.");

	for (glw::GLuint i = 0; i < m_buffer_length; ++i)
	{
		if (expected_data[i] != buffer[i])
		{
			for (glw::GLuint j = 0; j < m_buffer_length; ++j)
			{
				expectedDataStream << expected_data[j] << ",";
				returnedDataStream << buffer[j] << ",";
			}
			getTestContext().getLog() << tcu::TestLog::Message
									  << "Wrong value encountered when calling glGetSamplerParameterIivEXT() with "
										 "GL_TEXTURE_BORDER_COLOR_EXT pname;"
									  << " texture target:" << getTexTargetString(target) << "\n"
									  << " expected values:[" << expectedDataStream.str() << "]"
									  << " result values:[" << returnedDataStream.str() << "]\n"
									  << tcu::TestLog::EndMessage;
			return false;
		}
	}
	return true;
}

/** Check if glGetSamplerParameterIuivEXT() returns correct border color for GL_TEXTURE_BORDER_COLOR_EXT pname
 *
 * @param sampler_id     ID of sampler object;
 * @param target         texture target to do the call for;
 * @param expected_data  pointer to buffer with expected data
 * @return               true if both buffers are a match.
 */
bool TextureBorderClampParameterTextureBorderColor::verifyGLGetSamplerParameterIuivResult(
	glw::GLuint sampler_id, glw::GLenum target, const glw::GLuint* expected_data)
{
	std::vector<glw::GLuint> buffer(m_buffer_length);
	std::stringstream		 expectedDataStream;
	std::stringstream		 returnedDataStream;
	const glw::Functions&	gl = m_context.getRenderContext().getFunctions();

	memset(&buffer[0], 0, m_buffer_length * sizeof(glw::GLuint));

	gl.getSamplerParameterIuiv(sampler_id, m_glExtTokens.TEXTURE_BORDER_COLOR, &buffer[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting  parameter for sampler.");

	for (glw::GLuint i = 0; i < m_buffer_length; ++i)
	{
		if (expected_data[i] != buffer[i])
		{
			for (glw::GLuint j = 0; j < m_buffer_length; ++j)
			{
				expectedDataStream << expected_data[j] << ",";
				returnedDataStream << buffer[j] << ",";
			}
			getTestContext().getLog() << tcu::TestLog::Message
									  << "Wrong value encountered when calling glGetSamplerParameterIuivEXT() with "
										 "GL_TEXTURE_BORDER_COLOR_EXT pname;"
									  << " texture target:" << getTexTargetString(target) << "\n"
									  << " expected values:[" << expectedDataStream.str() << "]"
									  << " result values:[" << returnedDataStream.str() << "]\n"
									  << tcu::TestLog::EndMessage;
			return false;
		}
	}
	return true;
}

/** Check if glGetTexParameterfv() returns correct border color for GL_TEXTURE_BORDER_COLOR_EXT pname
 *
 * @param target         texture target to do the call for;
 * @param expected_data  pointer to buffer with expected data
 * @return               true if both buffers are a match.
 */
bool TextureBorderClampParameterTextureBorderColor::verifyGLGetTexParameterfvResult(glw::GLenum			target,
																					const glw::GLfloat* expected_data)
{
	std::vector<glw::GLfloat> buffer(m_buffer_length);
	std::stringstream		  expectedDataStream;
	std::stringstream		  returnedDataStream;
	const glw::Functions&	 gl = m_context.getRenderContext().getFunctions();

	memset(&buffer[0], 0, m_buffer_length * sizeof(glw::GLfloat));

	gl.getTexParameterfv(target, m_glExtTokens.TEXTURE_BORDER_COLOR, &buffer[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting parameter for texture.");

	for (glw::GLuint i = 0; i < m_buffer_length; ++i)
	{
		if (de::abs(expected_data[i] - buffer[i]) > TestCaseBase::m_epsilon_float)
		{
			for (glw::GLuint j = 0; j < m_buffer_length; ++j)
			{
				expectedDataStream << expected_data[j] << ",";
				returnedDataStream << buffer[j] << ",";
			}
			getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Wrong value encountered when calling glGetTexParameterfv() with GL_TEXTURE_BORDER_COLOR_EXT pname;"
				<< " texture target:" << getTexTargetString(target) << "\n"
				<< " expected values:[" << expectedDataStream.str() << "]"
				<< " result values:[" << returnedDataStream.str() << "]\n"
				<< tcu::TestLog::EndMessage;
			return false;
		}
	}
	return true;
}

/** Check if glGetTexParameteriv() returns correct border color for GL_TEXTURE_BORDER_COLOR_EXT pname
 *
 * @param target         texture target to do the call for;
 * @param expected_data  pointer to buffer with expected data
 * @return               true if both buffers are a match.
 */
bool TextureBorderClampParameterTextureBorderColor::verifyGLGetTexParameterivResult(glw::GLenum		  target,
																					const glw::GLint* expected_data)
{
	std::vector<glw::GLint> buffer(m_buffer_length);
	std::stringstream		expectedDataStream;
	std::stringstream		returnedDataStream;
	const glw::Functions&   gl = m_context.getRenderContext().getFunctions();

	memset(&buffer[0], 0, m_buffer_length * sizeof(glw::GLint));

	gl.getTexParameteriv(target, m_glExtTokens.TEXTURE_BORDER_COLOR, &buffer[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting  parameter for texture.");

	for (glw::GLuint i = 0; i < m_buffer_length; ++i)
	{
		if (expected_data[i] != buffer[i])
		{
			for (glw::GLuint j = 0; j < m_buffer_length; ++j)
			{
				expectedDataStream << expected_data[j] << ",";
				returnedDataStream << buffer[j] << ",";
			}
			getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Wrong value encountered when calling glGetTexParameteriv() with GL_TEXTURE_BORDER_COLOR_EXT pname;"
				<< " texture target:" << getTexTargetString(target) << "\n"
				<< " expected values:[" << expectedDataStream.str() << "]"
				<< " result values:[" << returnedDataStream.str() << "]\n"
				<< tcu::TestLog::EndMessage;

			return false;
		}
	}
	return true;
}

/** Check if glGetTexParameterIivEXT() returns correct border color for GL_TEXTURE_BORDER_COLOR_EXT pname
 *
 * @param target         texture target to do the call for;
 * @param expected_data  pointer to buffer with expected data
 * @return               true if both buffers are a match.
 */
bool TextureBorderClampParameterTextureBorderColor::verifyGLGetTexParameterIivResult(glw::GLenum	   target,
																					 const glw::GLint* expected_data)
{
	std::vector<glw::GLint> buffer(m_buffer_length);
	std::stringstream		expectedDataStream;
	std::stringstream		returnedDataStream;
	const glw::Functions&   gl = m_context.getRenderContext().getFunctions();

	memset(&buffer[0], 0, m_buffer_length * sizeof(glw::GLint));

	gl.getTexParameterIiv(target, m_glExtTokens.TEXTURE_BORDER_COLOR, &buffer[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting  parameter for texture.");

	for (glw::GLuint i = 0; i < m_buffer_length; ++i)
	{
		if (expected_data[i] != buffer[i])
		{
			for (glw::GLuint j = 0; j < m_buffer_length; ++j)
			{
				expectedDataStream << expected_data[j] << ",";
				returnedDataStream << buffer[j] << ",";
			}
			getTestContext().getLog() << tcu::TestLog::Message
									  << "Wrong value encountered when calling glGetTexParameterIivEXT() with "
										 "GL_TEXTURE_BORDER_COLOR_EXT pname;"
									  << " texture target:" << getTexTargetString(target) << "\n"
									  << " expected values:[" << expectedDataStream.str() << "]"
									  << " result values:[" << returnedDataStream.str() << "]\n"
									  << tcu::TestLog::EndMessage;

			return false;
		}
	}
	return true;
}

/** Check if glGetTexParameterIuivEXT() returns correct border color for GL_TEXTURE_BORDER_COLOR_EXT pname
 *
 * @param target         texture target to do the call for;
 * @param expected_data  pointer to buffer with expected data
 * @return               true if both buffers are a match.
 */
bool TextureBorderClampParameterTextureBorderColor::verifyGLGetTexParameterIuivResult(glw::GLenum		 target,
																					  const glw::GLuint* expected_data)
{
	std::vector<glw::GLuint> buffer(m_buffer_length);
	std::stringstream		 expectedDataStream;
	std::stringstream		 returnedDataStream;
	const glw::Functions&	gl = m_context.getRenderContext().getFunctions();

	memset(&buffer[0], 0, m_buffer_length * sizeof(glw::GLuint));

	gl.getTexParameterIuiv(target, m_glExtTokens.TEXTURE_BORDER_COLOR, &buffer[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting  parameter for texture.");

	for (glw::GLuint i = 0; i < m_buffer_length; ++i)
	{
		if (expected_data[i] != buffer[i])
		{
			for (glw::GLuint j = 0; j < m_buffer_length; ++j)
			{
				expectedDataStream << expected_data[j] << ",";
				returnedDataStream << buffer[j] << ",";
			}
			getTestContext().getLog() << tcu::TestLog::Message
									  << "Wrong value encountered when calling glGetTexParameterIuivEXT() with "
										 "GL_TEXTURE_BORDER_COLOR_EXT pname;"
									  << " texture target:" << getTexTargetString(target) << "\n"
									  << " expected values:[" << expectedDataStream.str() << "]"
									  << " result values:[" << returnedDataStream.str() << "]\n"
									  << tcu::TestLog::EndMessage;

			return false;
		}
	}
	return true;
}

} // namespace glcts
