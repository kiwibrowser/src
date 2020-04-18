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

/**
 */ /*!
 * \file  gl4cGetTextureSubImageTests.cpp
 * \brief Get Texture Sub Image Tests Suite Implementation
 */ /*-------------------------------------------------------------------*/

/* Includes. */
#include "gl4cGetTextureSubImageTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluRenderContext.hpp"
#include "gluStrUtil.hpp"
#include "tcuTestLog.hpp"
#include <cstdlib>

/* Implementation */

/**************************************************************************************************
 * Tests Group Implementation                                                                     *
 **************************************************************************************************/

gl4cts::GetTextureSubImage::Tests::Tests(deqp::Context& context)
	: TestCaseGroup(context, "get_texture_sub_image", "Get Texture Sub Image Tests Suite")
{
	addChild(new GetTextureSubImage::Errors(m_context));
	addChild(new GetTextureSubImage::Functional(m_context));
}

gl4cts::GetTextureSubImage::Tests::~Tests(void)
{
}

void gl4cts::GetTextureSubImage::Tests::init(void)
{
}

/**************************************************************************************************
 * Errors Tests Implementation                                                                    *
 **************************************************************************************************/

/** Constructor of API Errors tests.
 *
 *  @return [in] context    OpenGL context in which test shall run.
 */
gl4cts::GetTextureSubImage::Errors::Errors(deqp::Context& context)
	: deqp::TestCase(context, "errors_test", "Get Texture SubImage Errors Test")
	, m_context(context)
	, m_texture_1D(0)
	, m_texture_1D_array(0)
	, m_texture_2D(0)
	, m_texture_rectangle(0)
	, m_texture_2D_compressed(0)
	, m_texture_2D_multisampled(0)
	, m_destination_buffer(DE_NULL)
	, m_gl_GetTextureSubImage(DE_NULL)
	, m_gl_GetCompressedTextureSubImage(DE_NULL)
{
}

/** Destructor of API Errors tests.
 */
gl4cts::GetTextureSubImage::Errors::~Errors(void)
{
}

/** This function iterate over API Errors tests.
 */
tcu::TestNode::IterateResult gl4cts::GetTextureSubImage::Errors::iterate(void)
{
	bool is_ok		= true;
	bool test_error = false;

	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_get_texture_sub_image = m_context.getContextInfo().isExtensionSupported("GL_ARB_get_texture_sub_image");

	try
	{
		if (is_at_least_gl_45 || is_arb_get_texture_sub_image)
		{
			/* Prepare texture objects */
			prepare();

			/* Do tests. */
			is_ok &= testExistingTextureObjectError();

			is_ok &= testBufferOrMultisampledTargetError();

			is_ok &= testNegativeOffsetError();

			is_ok &= testBoundsError();

			is_ok &= testOneDimmensionalTextureErrors();

			is_ok &= testTwoDimmensionalTextureErrors();

			is_ok &= testBufferSizeError();
		}
	}
	catch (...)
	{
		is_ok	  = false;
		test_error = true;
	}

	/* Clean up */
	clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (test_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Preparation of source textures and destination buffer.
 */
void gl4cts::GetTextureSubImage::Errors::prepare()
{
	/* OpenGL functions access point. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* If already initialized throw exception. */
	if (m_texture_1D || m_texture_1D_array || m_texture_2D || m_texture_rectangle || m_texture_2D_compressed ||
		m_texture_2D_multisampled)
	{
		throw 0;
	}

	/* Generate texture ids. */
	gl.genTextures(1, &m_texture_1D);
	gl.genTextures(1, &m_texture_1D_array);
	gl.genTextures(1, &m_texture_2D);
	gl.genTextures(1, &m_texture_rectangle);
	gl.genTextures(1, &m_texture_2D_compressed);
	gl.genTextures(1, &m_texture_2D_multisampled);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures call failed.");

	/* If one is not initialized throw exception. */
	if (!(m_texture_1D && m_texture_1D_array && m_texture_2D))
	{
		throw 0;
	}

	/* Upload texture data. */
	gl.bindTexture(GL_TEXTURE_1D, m_texture_1D);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture call failed.");

	gl.texImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, s_texture_data_width, 0, GL_RGBA, GL_UNSIGNED_BYTE, s_texture_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D call failed.");

	gl.bindTexture(GL_TEXTURE_1D_ARRAY, m_texture_1D_array);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture call failed.");

	gl.texImage2D(GL_TEXTURE_1D_ARRAY, 0, GL_RGBA8, s_texture_data_width, s_texture_data_height, 0, GL_RGBA,
				  GL_UNSIGNED_BYTE, s_texture_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D call failed.");

	gl.bindTexture(GL_TEXTURE_2D, m_texture_2D);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture call failed.");

	gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, s_texture_data_width, s_texture_data_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
				  s_texture_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D call failed.");

	gl.bindTexture(GL_TEXTURE_RECTANGLE, m_texture_rectangle);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture call failed.");

	gl.texImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA8, s_texture_data_width, s_texture_data_height, 0, GL_RGBA,
				  GL_UNSIGNED_BYTE, s_texture_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D call failed.");

	/* Upload compressed texture data. */
	gl.bindTexture(GL_TEXTURE_2D, m_texture_2D_compressed);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture call failed.");

	gl.compressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB8_ETC2, s_texture_data_compressed_width,
							s_texture_data_compressed_height, 0, s_texture_data_compressed_size,
							s_texture_data_compressed);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D call failed.");

	/* Prepare multisampled texture storage. */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_texture_2D_multisampled);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture call failed.");

	gl.texImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 1, GL_R8, s_texture_data_width, s_texture_data_height, GL_TRUE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D call failed.");

	/* Prepare function pointers. */
	m_gl_GetTextureSubImage =
		(PFNGLGETTEXTURESUBIMAGEPROC)m_context.getRenderContext().getProcAddress("glGetTextureSubImage");
	m_gl_GetCompressedTextureSubImage =
		(PFNGLGETCOMPRESSEDTEXTURESUBIMAGEPROC)m_context.getRenderContext().getProcAddress(
			"glGetCompressedTextureSubImage");

	if ((DE_NULL == m_gl_GetTextureSubImage) || (DE_NULL == m_gl_GetCompressedTextureSubImage))
	{
		throw 0;
	}

	/* Allocate destination buffer. */
	m_destination_buffer = (glw::GLubyte*)malloc(s_destination_buffer_size);

	if (DE_NULL == m_destination_buffer)
	{
		throw 0;
	}
}

/** The function checks that GL_INVALID_OPERATION error is generated by GetTextureSubImage if
 *  texture is not the name of an existing texture object. It also checks that
 *  GL_INVALID_OPERATION error is generated by GetCompressedTextureSubImage if texture is not
 *  the name of an existing texture object. For reference see the OpenGL 4.5 Core Specification
 *  chapter 8.11.4.
 *
 *  @return True if proper error values are generated, false otherwise.
 */
bool gl4cts::GetTextureSubImage::Errors::testExistingTextureObjectError()
{
	/* OpenGL functions access point. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare invalid texture name. */
	glw::GLuint invalid_texture = m_texture_2D_multisampled;

	while (gl.isTexture(++invalid_texture))
		;

	m_gl_GetTextureSubImage(invalid_texture, 0, 0, 0, 0, s_texture_data_width, s_texture_data_height, 1, GL_RGBA,
							GL_UNSIGNED_BYTE, s_destination_buffer_size, m_destination_buffer);

	glw::GLint error_value	 = gl.getError();
	glw::GLint is_proper_error = (GL_INVALID_OPERATION == error_value);

	if (!is_proper_error)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "GL_INVALID_OPERATION error is expected to be generated by "
													   "glGetTextureSubImage if texture is not the name of an existing "
													   "texture object (OpenGL 4.5 Core Specification chapter 8.11.4)."
						   << " However, the error value " << glu::getErrorName(error_value) << " was generated."
						   << tcu::TestLog::EndMessage;
	}

	m_gl_GetCompressedTextureSubImage(invalid_texture, 0, 0, 0, 0, s_texture_data_compressed_width,
									  s_texture_data_compressed_height, 1, s_destination_buffer_size,
									  m_destination_buffer);

	error_value = gl.getError();

	glw::GLint is_proper_error_compressed = (GL_INVALID_OPERATION == error_value);

	if (!is_proper_error_compressed)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "GL_INVALID_OPERATION error is expected to be generated by glGetCompressedTextureSubImage "
							  "if texture is not the name of an existing texture object (OpenGL 4.5 Core Specification "
							  "chapter 8.11.4)."
						   << " However, the error value " << glu::getErrorName(error_value) << " was generated."
						   << tcu::TestLog::EndMessage;
	}

	if (is_proper_error && is_proper_error_compressed)
	{
		return true;
	}

	return false;
}

/** The function checks that GL_INVALID_OPERATION error is generated if texture is the
 *  name of a buffer or multisample texture. For reference see OpenGL 4.5 Core Specification
 *  chapter 8.11.4.
 *
 *  @return True if proper error values are generated, false otherwise.
 */
bool gl4cts::GetTextureSubImage::Errors::testBufferOrMultisampledTargetError()
{
	/* OpenGL functions access point. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Test. */
	m_gl_GetTextureSubImage(m_texture_2D_multisampled, 0, 0, 0, 0, s_texture_data_width, s_texture_data_height, 1,
							GL_RGBA, GL_UNSIGNED_BYTE, s_destination_buffer_size, m_destination_buffer);

	glw::GLint error_value	 = gl.getError();
	glw::GLint is_proper_error = (GL_INVALID_OPERATION == error_value);

	if (!is_proper_error)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "GL_INVALID_OPERATION error is expected to be generated by "
													   "glGetTextureSubImage if texture is the name of multisample "
													   "texture (OpenGL 4.5 Core Specification chapter 8.11.4)."
						   << " However, the error value " << glu::getErrorName(error_value) << " was generated."
						   << tcu::TestLog::EndMessage;
	}

	if (is_proper_error)
	{
		return true;
	}

	return false;
}

/** The functions checks that GL_INVALID_VALUE is generated if xoffset, yoffset or
 *  zoffset are negative. For reference see OpenGL 4.5 Core Specification
 *  chapter 8.11.4.
 *
 *  @return True if proper error values are generated, false otherwise.
 */
bool gl4cts::GetTextureSubImage::Errors::testNegativeOffsetError()
{
	/* OpenGL functions access point. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Test. */
	m_gl_GetTextureSubImage(m_texture_2D, 0, -1, 0, 0, s_texture_data_width, s_texture_data_height, 1, GL_RGBA,
							GL_UNSIGNED_BYTE, s_destination_buffer_size, m_destination_buffer);

	glw::GLint error_value	 = gl.getError();
	glw::GLint is_proper_error = (GL_INVALID_VALUE == error_value);

	if (!is_proper_error)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "GL_INVALID_VALUE error is expected to be generated by glGetTextureSubImage if xoffset, "
							  "yoffset or zoffset are negative (OpenGL 4.5 Core Specification chapter 8.11.4)."
						   << " However, the error value " << glu::getErrorName(error_value) << " was generated."
						   << tcu::TestLog::EndMessage;
	}

	m_gl_GetCompressedTextureSubImage(m_texture_2D_compressed, 0, -1, 0, 0, s_texture_data_compressed_width,
									  s_texture_data_compressed_height, 1, s_destination_buffer_size,
									  m_destination_buffer);

	error_value = gl.getError();

	glw::GLint is_proper_error_compressed = (GL_INVALID_VALUE == error_value);

	if (!is_proper_error_compressed)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "GL_INVALID_VALUE error is expected to be generated by glGetCompressedTextureSubImage if "
							  "xoffset, yoffset or zoffset are negative (OpenGL 4.5 Core Specification chapter 8.11.4)."
						   << " However, the error value " << glu::getErrorName(error_value) << " was generated."
						   << tcu::TestLog::EndMessage;
	}

	if (is_proper_error && is_proper_error_compressed)
	{
		return true;
	}

	return false;
}

/** The functions checks that GL_INVALID_VALUE is generated if xoffset + width is
 *  greater than the texture's width, yoffset + height is greater than
 *  the texture's height, or zoffset + depth is greater than the
 *  texture's depth. For reference see OpenGL 4.5 Core Specification
 *  chapter 8.11.4.
 *
 *  @return True if proper error values are generated, false otherwise.
 */
bool gl4cts::GetTextureSubImage::Errors::testBoundsError()
{
	/* OpenGL functions access point. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Uncompresse texture test. */
	m_gl_GetTextureSubImage(m_texture_2D, 0, s_texture_data_width, s_texture_data_height, 0, s_texture_data_width * 2,
							s_texture_data_height * 2, 1, GL_RGBA, GL_UNSIGNED_BYTE, s_destination_buffer_size,
							m_destination_buffer);

	glw::GLint error_value	 = gl.getError();
	glw::GLint is_proper_error = (GL_INVALID_VALUE == error_value);

	if (!is_proper_error)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "GL_INVALID_VALUE error is expected to be generated by glGetTextureSubImage if xoffset + width is"
			   " greater than the texture's width, yoffset + height is greater than"
			   " the texture's height, or zoffset + depth is greater than the"
			   " texture's depth. (OpenGL 4.5 Core Specification chapter 8.11.4)."
			   " However, the error value "
			<< glu::getErrorName(error_value) << " was generated." << tcu::TestLog::EndMessage;
	}

	/* Compresse texture test. */
	m_gl_GetCompressedTextureSubImage(m_texture_2D_compressed, 0, s_texture_data_compressed_width,
									  s_texture_data_compressed_height, 0, s_texture_data_compressed_width * 2,
									  s_texture_data_compressed_height * 2, 1, s_destination_buffer_size,
									  m_destination_buffer);

	error_value = gl.getError();

	glw::GLint is_proper_error_compressed = (GL_INVALID_VALUE == error_value);

	if (!is_proper_error_compressed)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "GL_INVALID_VALUE error is expected to be generated by glGetCompressedTextureSubImage if "
							  "xoffset + width is"
							  " greater than the texture's width, yoffset + height is greater than"
							  " the texture's height, or zoffset + depth is greater than the"
							  " texture's depth. (OpenGL 4.5 Core Specification chapter 8.11.4)."
							  " However, the error value "
						   << glu::getErrorName(error_value) << " was generated." << tcu::TestLog::EndMessage;
	}

	if (is_proper_error && is_proper_error_compressed)
	{
		return true;
	}

	return false;
}

/** The functions checks that GL_INVALID_VALUE error is generated if the effective
 *  target is GL_TEXTURE_1D and either yoffset is not zero, or height
 *  is not one. For reference see OpenGL 4.5 Core Specification
 *  chapter 8.11.4.
 *
 *  @return True if proper error values are generated, false otherwise.
 */
bool gl4cts::GetTextureSubImage::Errors::testOneDimmensionalTextureErrors()
{
	/* OpenGL functions access point. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Test. */
	m_gl_GetTextureSubImage(m_texture_1D, 0, 0, 1, 0, s_texture_data_width, 2, 1, GL_RGBA, GL_UNSIGNED_BYTE,
							s_destination_buffer_size, m_destination_buffer);

	glw::GLint error_value	 = gl.getError();
	glw::GLint is_proper_error = (GL_INVALID_VALUE == error_value);

	if (!is_proper_error)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "GL_INVALID_VALUE error is expected to be generated by glGetTextureSubImage if the effective"
			   " target is GL_TEXTURE_1D and either yoffset is not zero, or height"
			   " is not one (OpenGL 4.5 Core Specification chapter 8.11.4)."
			   " However, the error value "
			<< glu::getErrorName(error_value) << " was generated." << tcu::TestLog::EndMessage;
	}

	if (is_proper_error)
	{
		return true;
	}

	return false;
}

/** The functions checks that GL_INVALID_VALUE error is generated if the effective
 *  target is GL_TEXTURE_1D, GL_TEXTURE_1D_ARRAY, GL_TEXTURE_2D or
 *  GL_TEXTURE_RECTANGLE and either zoffset is not zero, or depth
 *  is not one. For reference see OpenGL 4.5 Core Specification
 *  chapter 8.11.4.
 *
 *  @return True if proper error values are generated, false otherwise.
 */
bool gl4cts::GetTextureSubImage::Errors::testTwoDimmensionalTextureErrors()
{
	/* OpenGL functions access point. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Test. */
	const struct
	{
		glw::GLuint		   id;
		const glw::GLchar* target_name;
	} test_textures[] = { { m_texture_1D, "GL_TEXTURE_1D" },
						  { m_texture_1D_array, "GL_TEXTURE_1D_ARRAY" },
						  { m_texture_2D, "GL_TEXTURE_2D" } };

	static const glw::GLuint test_textures_size = sizeof(test_textures) / sizeof(test_textures[0]);

	glw::GLint is_error = true;

	for (glw::GLuint i = 0; i < test_textures_size; ++i)
	{
		m_gl_GetTextureSubImage(test_textures[i].id, 0, 0, 0, 1, s_texture_data_width,
								(test_textures[i].id == m_texture_1D) ? 1 : s_texture_data_height, 2, GL_RGBA,
								GL_UNSIGNED_BYTE, s_destination_buffer_size, m_destination_buffer);

		glw::GLint error_value	 = gl.getError();
		glw::GLint is_proper_error = (GL_INVALID_VALUE == error_value);

		if (!is_proper_error)
		{
			is_error = false;

			m_testCtx.getLog()
				<< tcu::TestLog::Message
				<< "GL_INVALID_VALUE error is expected to be generated by glGetTextureSubImage if the effective"
				   " target is "
				<< test_textures[i].target_name << " and either zoffset is not zero, or depth"
												   " is not one. (OpenGL 4.5 Core Specification chapter 8.11.4)."
												   " However, the error value "
				<< glu::getErrorName(error_value) << " was generated." << tcu::TestLog::EndMessage;
		}
	}

	/* Test (compressed textures). */
	const struct
	{
		glw::GLuint		   id;
		const glw::GLchar* target_name;
	} test_compressed_textures[] = { { m_texture_2D_compressed, "GL_TEXTURE_2D" } };

	static const glw::GLuint test_compressed_textures_size =
		sizeof(test_compressed_textures) / sizeof(test_compressed_textures[0]);

	for (glw::GLuint i = 0; i < test_compressed_textures_size; ++i)
	{
		m_gl_GetCompressedTextureSubImage(test_compressed_textures[i].id, 0, 0, 0, 1, s_texture_data_compressed_width,
										  s_texture_data_compressed_height, 2, s_destination_buffer_size,
										  m_destination_buffer);

		glw::GLint error_value = gl.getError();

		glw::GLint is_proper_error_compressed = (GL_INVALID_VALUE == error_value);

		if (!is_proper_error_compressed)
		{
			is_error = false;

			m_testCtx.getLog() << tcu::TestLog::Message << "GL_INVALID_VALUE error is expected to be generated by "
														   "glGetCompressedTextureSubImage if the effective"
														   " target is "
							   << test_compressed_textures[i].target_name
							   << " and either zoffset is not zero, or depth"
								  " is not one. (OpenGL 4.5 Core Specification chapter 8.11.4)."
								  " However, the error value "
							   << glu::getErrorName(error_value) << " was generated." << tcu::TestLog::EndMessage;
		}
	}

	if (is_error)
	{
		return true;
	}

	return false;
}

/** The functions checks that GL_INVALID_OPERATION error is generated if the buffer
 *  size required to store the requested data is greater than bufSize.
 *  For reference see OpenGL 4.5 Core Specification chapter 8.11.4.
 *
 *  @return True if proper error values are generated, false otherwise.
 */
bool gl4cts::GetTextureSubImage::Errors::testBufferSizeError()
{
	/* OpenGL functions access point. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Test for uncompressed texture. */
	m_gl_GetTextureSubImage(m_texture_2D, 0, 0, 0, 0, s_texture_data_width, s_texture_data_height, 1, GL_RGBA,
							GL_UNSIGNED_BYTE, 1, m_destination_buffer);

	glw::GLint error_value	 = gl.getError();
	glw::GLint is_proper_error = (GL_INVALID_OPERATION == error_value);

	if (!is_proper_error)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "GL_INVALID_OPERATION error is expected to be generated by glGetTextureSubImage if the buffer"
			   " size required to store the requested data is greater than bufSize. (OpenGL 4.5 Core Specification "
			   "chapter 8.11.4)."
			   " However, the error value "
			<< glu::getErrorName(error_value) << " was generated." << tcu::TestLog::EndMessage;
	}

	/* Test for compressed texture. */
	m_gl_GetCompressedTextureSubImage(m_texture_2D_compressed, 0, 0, 0, 0, s_texture_data_compressed_width,
									  s_texture_data_compressed_height, 1, 1, m_destination_buffer);

	error_value = gl.getError();

	glw::GLint is_proper_error_compressed = (GL_INVALID_OPERATION == error_value);

	if (!is_proper_error_compressed)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "GL_INVALID_OPERATION error is expected to be generated by glGetCompressedTextureSubImage if the buffer"
			   " size required to store the requested data is greater than bufSize. (OpenGL 4.5 Core Specification "
			   "chapter 8.11.4)."
			   " However, the error value "
			<< glu::getErrorName(error_value) << " was generated." << tcu::TestLog::EndMessage;
	}

	/* Return results. */
	if (is_proper_error && is_proper_error_compressed)
	{
		return true;
	}

	return false;
}

void gl4cts::GetTextureSubImage::Errors::clean()
{
	/* OpenGL functions access point. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/*Textures cleanup. */
	if (m_texture_1D)
	{
		gl.deleteTextures(1, &m_texture_1D);
		m_texture_1D = 0;
	}

	if (m_texture_1D_array)
	{
		gl.deleteTextures(1, &m_texture_1D_array);
		m_texture_1D_array = 0;
	}

	if (m_texture_2D)
	{
		gl.deleteTextures(1, &m_texture_2D);
		m_texture_2D = 0;
	}
	if (m_texture_rectangle)
	{
		gl.deleteTextures(1, &m_texture_rectangle);
		m_texture_rectangle = 0;
	}

	if (m_texture_2D_compressed)
	{
		gl.deleteTextures(1, &m_texture_2D_compressed);
		m_texture_2D_compressed = 0;
	}

	if (m_texture_2D_multisampled)
	{
		gl.deleteTextures(1, &m_texture_2D_multisampled);
		m_texture_2D_multisampled = 0;
	}

	/* CPU buffers */
	if (m_destination_buffer)
	{
		free(m_destination_buffer);
		m_destination_buffer = DE_NULL;
	}
}

/* Uncompressed source texture 2x2 pixels */

const glw::GLubyte gl4cts::GetTextureSubImage::Errors::s_texture_data[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
}; //<! uncompressed texture

const glw::GLuint gl4cts::GetTextureSubImage::Errors::s_texture_data_size =
	sizeof(s_texture_data); //<! uncompressed texture size

const glw::GLuint gl4cts::GetTextureSubImage::Errors::s_texture_data_width = 2; //<! uncompressed texture width

const glw::GLuint gl4cts::GetTextureSubImage::Errors::s_texture_data_height = 2; //<! uncompressed texture height

/* ETC2 compressed texture (4x4 pixels === 1 block) */

const glw::GLubyte gl4cts::GetTextureSubImage::Errors::s_texture_data_compressed[] = {
	0x15, 0x90, 0x33, 0x6f, 0xaf, 0xcc, 0x16, 0x98
}; //<! ETC2 compressed texture

const glw::GLuint gl4cts::GetTextureSubImage::Errors::s_texture_data_compressed_size =
	sizeof(s_texture_data_compressed); //<! ETC2 compressed texture size

const glw::GLuint gl4cts::GetTextureSubImage::Errors::s_texture_data_compressed_width =
	4; //<! ETC2 compressed texture width

const glw::GLuint gl4cts::GetTextureSubImage::Errors::s_texture_data_compressed_height =
	4; //<! ETC2 compressed texture height

const glw::GLuint gl4cts::GetTextureSubImage::Errors::s_destination_buffer_size =
	(s_texture_data_size > s_texture_data_compressed_size) ?
		s_texture_data_size :
		s_texture_data_compressed_size; //<! size of the destination buffer (for fetched data)

/*****************************************************************************************************
 * Functional Test Implementation                                                                    *
 *****************************************************************************************************/

/** Constructor of the functional test.
 *
 *  @param [in] context     OpenGL context in which test shall run.
 */
gl4cts::GetTextureSubImage::Functional::Functional(deqp::Context& context)
	: deqp::TestCase(context, "functional_test", "Get Texture SubImage Functional Test")
	, m_context(context)
	, m_texture(0)
{
}

/** Destructor of the functional test.
 */
gl4cts::GetTextureSubImage::Functional::~Functional(void)
{
}

/** Iterate over functional test cases.
 */
tcu::TestNode::IterateResult gl4cts::GetTextureSubImage::Functional::iterate(void)
{
	bool is_ok		= true;
	bool test_error = false;

	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_get_texture_sub_image = m_context.getContextInfo().isExtensionSupported("GL_ARB_get_texture_sub_image");

	/* Bind function pointers. */
	m_gl_GetCompressedTextureSubImage =
		(PFNGLGETCOMPRESSEDTEXTURESUBIMAGEPROC)m_context.getRenderContext().getProcAddress(
			"glGetCompressedTextureSubImage");
	m_gl_GetTextureSubImage =
		(PFNGLGETTEXTURESUBIMAGEPROC)m_context.getRenderContext().getProcAddress("glGetTextureSubImage");

	try
	{
		/* Report error when function pointers are not available. */
		if ((DE_NULL == m_gl_GetCompressedTextureSubImage) || (DE_NULL == m_gl_GetTextureSubImage))
		{
			m_testCtx.getLog()
				<< tcu::TestLog::Message
				<< "Cannot obtain glGetCompressedTextureSubImage or glGetTextureSubImage function pointer."
				<< tcu::TestLog::EndMessage;
			throw 0;
		}

		/* Run tests. */
		if (is_at_least_gl_45 || is_arb_get_texture_sub_image)
		{
			/* Tested targets. */
			glw::GLenum targets[] = { GL_TEXTURE_1D,		GL_TEXTURE_1D_ARRAY, GL_TEXTURE_2D,
									  GL_TEXTURE_RECTANGLE, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_ARRAY,
									  GL_TEXTURE_2D_ARRAY,  GL_TEXTURE_3D };

			glw::GLuint targets_count = sizeof(targets) / sizeof(targets[0]);

			for (glw::GLuint i = 0; i < targets_count; ++i)
			{
				prepare(targets[i], false);

				is_ok &= check(targets[i], false);

				clean();
			}

			/* Compressed textures tested targets. */
			glw::GLenum compressed_targets[] = { GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_ARRAY,
												 GL_TEXTURE_2D_ARRAY };

			glw::GLuint compressed_targets_count = sizeof(compressed_targets) / sizeof(compressed_targets[0]);

			for (glw::GLuint i = 0; i < compressed_targets_count; ++i)
			{
				prepare(compressed_targets[i], true);

				is_ok &= check(compressed_targets[i], true);

				clean();
			}
		}
	}
	catch (...)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Test error has occured." << tcu::TestLog::EndMessage;

		is_ok	  = false;
		test_error = true;

		clean();
	}

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (test_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Prepare source texture for the test.
 *
 *  @param [in] target          Target of the texture to be prepared.
 *  @param [in] is_compressed   Flag indicating that texture shall be compressed (true) or uncompressed (false).
 */
void gl4cts::GetTextureSubImage::Functional::prepare(glw::GLenum target, bool is_compressed)
{
	/* OpenGL functions access point. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate and bind texture. */
	gl.genTextures(1, &m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures call failed.");

	gl.bindTexture(target, m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture call failed.");

	/* Upload data to the texture. */
	if (is_compressed)
	{
		/* Upload compressed texture. */
		switch (target)
		{
		case GL_TEXTURE_2D:
			gl.compressedTexImage2D(target, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, s_texture_data_width, s_texture_data_height, 0,
									s_texture_data_compressed_size / s_texture_data_depth, s_texture_data_compressed);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompressedTexImage2D call failed.");
			break;
		case GL_TEXTURE_CUBE_MAP:
			gl.compressedTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, s_texture_data_width, s_texture_data_height,
				0, s_texture_data_compressed_size / s_texture_data_depth,
				&s_texture_data_compressed[0 * s_texture_data_compressed_size / s_texture_data_depth]);
			gl.compressedTexImage2D(
				GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, s_texture_data_width, s_texture_data_height,
				0, s_texture_data_compressed_size / s_texture_data_depth,
				&s_texture_data_compressed[1 * s_texture_data_compressed_size / s_texture_data_depth]);
			gl.compressedTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, s_texture_data_width, s_texture_data_height,
				0, s_texture_data_compressed_size / s_texture_data_depth,
				&s_texture_data_compressed[2 * s_texture_data_compressed_size / s_texture_data_depth]);
			gl.compressedTexImage2D(
				GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, s_texture_data_width, s_texture_data_height,
				0, s_texture_data_compressed_size / s_texture_data_depth,
				&s_texture_data_compressed[3 * s_texture_data_compressed_size / s_texture_data_depth]);
			gl.compressedTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, s_texture_data_width, s_texture_data_height,
				0, s_texture_data_compressed_size / s_texture_data_depth,
				&s_texture_data_compressed[4 * s_texture_data_compressed_size / s_texture_data_depth]);
			gl.compressedTexImage2D(
				GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, s_texture_data_width, s_texture_data_height,
				0, s_texture_data_compressed_size / s_texture_data_depth,
				&s_texture_data_compressed[5 * s_texture_data_compressed_size / s_texture_data_depth]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompressedTexImage2D call failed.");
			break;
		case GL_TEXTURE_CUBE_MAP_ARRAY:
			gl.compressedTexImage3D(target, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, s_texture_data_width, s_texture_data_height, 6,
									0, s_texture_data_compressed_size / s_texture_data_depth * 6,
									s_texture_data_compressed);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompressedTexImage3D call failed.");
			break;
		case GL_TEXTURE_2D_ARRAY:
			gl.compressedTexImage3D(target, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, s_texture_data_width, s_texture_data_height,
									s_texture_data_depth, 0, s_texture_data_compressed_size, s_texture_data_compressed);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompressedTexImage3D call failed.");
			break;
		default:
			throw 0;
		};
	}
	else
	{
		/* Upload uncompressed texture. */
		switch (target)
		{
		case GL_TEXTURE_1D:
			gl.texImage1D(target, 0, GL_RGBA8, s_texture_data_width, 0, GL_RGBA, GL_UNSIGNED_BYTE, s_texture_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D call failed.");
			break;
		case GL_TEXTURE_1D_ARRAY:
		case GL_TEXTURE_2D:
		case GL_TEXTURE_RECTANGLE:
			gl.texImage2D(target, 0, GL_RGBA8, s_texture_data_width, s_texture_data_height, 0, GL_RGBA,
						  GL_UNSIGNED_BYTE, s_texture_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D call failed.");
			break;
		case GL_TEXTURE_CUBE_MAP:
			gl.texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA8, s_texture_data_width, s_texture_data_height, 0,
						  GL_RGBA, GL_UNSIGNED_BYTE,
						  &s_texture_data[0 * s_texture_data_width * s_texture_data_height * 4 /* RGBA */]);
			gl.texImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA8, s_texture_data_width, s_texture_data_height, 0,
						  GL_RGBA, GL_UNSIGNED_BYTE,
						  &s_texture_data[1 * s_texture_data_width * s_texture_data_height * 4 /* RGBA */]);
			gl.texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA8, s_texture_data_width, s_texture_data_height, 0,
						  GL_RGBA, GL_UNSIGNED_BYTE,
						  &s_texture_data[2 * s_texture_data_width * s_texture_data_height * 4 /* RGBA */]);
			gl.texImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA8, s_texture_data_width, s_texture_data_height, 0,
						  GL_RGBA, GL_UNSIGNED_BYTE,
						  &s_texture_data[3 * s_texture_data_width * s_texture_data_height * 4 /* RGBA */]);
			gl.texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA8, s_texture_data_width, s_texture_data_height, 0,
						  GL_RGBA, GL_UNSIGNED_BYTE,
						  &s_texture_data[4 * s_texture_data_width * s_texture_data_height * 4 /* RGBA */]);
			gl.texImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA8, s_texture_data_width, s_texture_data_height, 0,
						  GL_RGBA, GL_UNSIGNED_BYTE,
						  &s_texture_data[5 * s_texture_data_width * s_texture_data_height * 4 /* RGBA */]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D call failed.");
			break;
		case GL_TEXTURE_CUBE_MAP_ARRAY:
			gl.texImage3D(target, 0, GL_RGBA8, s_texture_data_width, s_texture_data_height, 6, 0, GL_RGBA,
						  GL_UNSIGNED_BYTE, s_texture_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage3D call failed.");
			break;
		case GL_TEXTURE_2D_ARRAY:
		case GL_TEXTURE_3D:
			gl.texImage3D(target, 0, GL_RGBA8, s_texture_data_width, s_texture_data_height, s_texture_data_depth, 0,
						  GL_RGBA, GL_UNSIGNED_BYTE, s_texture_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage3D call failed.");
			break;
		default:
			throw 0;
		};
	}
}

/** Test that Get(Compressed)TextureSubImage resturns expected texture data.
 *
 *  @param [in] target          Target of the texture to be prepared.
 *  @param [in] is_compressed   Flag indicating that texture shall be compressed (true) or uncompressed (false).
 *
 *  @return True if test succeeded, false otherwise.
 */
bool gl4cts::GetTextureSubImage::Functional::check(glw::GLenum target, bool is_compressed)
{
	/* OpenGL functions access point. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Arguments setup (depends on dimmension) */
	glw::GLint x_offset = s_texture_data_width / 2 /* half */;
	glw::GLint width	= s_texture_data_width / 2 /* half */;

	glw::GLint y_offset = 0;
	glw::GLint height   = 1;

	glw::GLint z_offset = 0;
	glw::GLint depth	= 1;

	/* For 2 and 3 -dimensional textures setup y-direction. */
	if (GL_TEXTURE_1D != target)
	{
		y_offset = s_texture_data_height / 2 /* half */;
		height   = s_texture_data_height / 2 /* half */;
	}

	/* For 3-dimensional textures setup z-direction. */
	if ((GL_TEXTURE_3D == target) || (GL_TEXTURE_2D_ARRAY == target))
	{
		z_offset = s_texture_data_depth / 2 /* half */;
		depth	= s_texture_data_depth / 2 /* half */;
	}

	/* For cube-map texture stup 6-cube faces. */
	if ((GL_TEXTURE_CUBE_MAP == target) || (GL_TEXTURE_CUBE_MAP_ARRAY == target))
	{
		z_offset = 3; /* half of cube map */
		depth	= 3; /* half of cube map */
	}

	/* Setup number of components. */
	glw::GLint number_of_components = 0;

	if (is_compressed)
	{
		number_of_components = 16; /* 128 bit block of 4x4 compressed pixels. */
	}
	else
	{
		number_of_components = 4; /* RGBA components. */
	}

	/* Setup size. */
	glw::GLsizei size = 0;

	/* Iterate over pixels. */
	glw::GLint x_block = 1;
	glw::GLint y_block = 1;

	if (is_compressed)
	{
		/* Iterate over 4x4 compressed pixel block. */
		x_block = 4;
		y_block = 4;

		size = static_cast<glw::GLsizei>((width / x_block) * (height / y_block) * depth * number_of_components *
										 sizeof(glw::GLubyte));
	}
	else
	{
		size = static_cast<glw::GLsizei>(width * height * depth * number_of_components * sizeof(glw::GLubyte));
	}

	/* Storage for fetched texturte. */
	glw::GLubyte* texture_data = new glw::GLubyte[size];

	if (DE_NULL == texture_data)
	{
		throw 0;
	}

	/* Fetching texture. */
	if (is_compressed)
	{
		m_gl_GetCompressedTextureSubImage(m_texture, 0, x_offset, y_offset, z_offset, width, height, depth, size,
										  texture_data);
	}
	else
	{
		m_gl_GetTextureSubImage(m_texture, 0, x_offset, y_offset, z_offset, width, height, depth, GL_RGBA,
								GL_UNSIGNED_BYTE, size, texture_data);
	}

	/* Comprae fetched texture with reference. */
	glw::GLint error = gl.getError();

	bool is_ok = true;

	if (GL_NO_ERROR == error)
	{
		for (glw::GLint k = 0; k < depth; ++k)
		{
			for (glw::GLint j = 0; j < height / y_block; ++j)
			{
				for (glw::GLint i = 0; i < width / x_block; ++i)
				{
					for (glw::GLint c = 0; c < number_of_components; ++c) /* RGBA components iterating */
					{
						glw::GLuint reference_data_position =
							(i + (x_offset / x_block)) * number_of_components +
							(j + (y_offset / y_block)) * s_texture_data_width / x_block * number_of_components +
							(k + z_offset) * s_texture_data_width / x_block * s_texture_data_height / y_block *
								number_of_components +
							c;

						glw::GLuint tested_data_position =
							i * number_of_components + j * width / x_block * number_of_components +
							k * width / x_block * height / y_block * number_of_components + c;

						glw::GLubyte reference_value = (is_compressed) ?
														   s_texture_data_compressed[reference_data_position] :
														   s_texture_data[reference_data_position];
						glw::GLubyte tested_value = texture_data[tested_data_position];

						if (reference_value != tested_value)
						{
							is_ok = false;
							break;
						}
					}
				}
			}
		}
	}
	else
	{
		/* GL error. */
		delete[] texture_data;
		throw 0;
	}

	/* Cleanup. */
	delete[] texture_data;

	/* Error reporting. */
	if (!is_ok)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Functional test of "
						   << ((is_compressed) ? "glGetCompressedTextureSubImage " : "glGetTextureSubImage ")
						   << "function has failed with target " << glu::getTextureTargetStr(target) << "."
						   << tcu::TestLog::EndMessage;
	}

	/* Return result. */
	return is_ok;
}

/** Clean texture. */
void gl4cts::GetTextureSubImage::Functional::clean()
{
	/* OpenGL functions access point. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_texture)
	{
		gl.deleteTextures(1, &m_texture);

		m_texture = 0;
	}
}

/** RGBA 8x8x8 pixels in size source texture for testing GetTextureSubImage. */
const glw::GLubyte gl4cts::GetTextureSubImage::Functional::s_texture_data[] = {
	0,   0,   0,   0,  0,   0,   32,  1,  0,   0,   64,  2,  0,   0,   96,  3,
	0,   0,   128, 4,  0,   0,   160, 5,  0,   0,   192, 6,  0,   0,   224, 7,

	0,   32,  0,   1,  0,   32,  32,  2,  0,   32,  64,  3,  0,   32,  96,  4,
	0,   32,  128, 5,  0,   32,  160, 6,  0,   32,  192, 7,  0,   32,  224, 8,

	0,   64,  0,   2,  0,   64,  32,  3,  0,   64,  64,  4,  0,   64,  96,  5,
	0,   64,  128, 6,  0,   64,  160, 7,  0,   64,  192, 8,  0,   64,  224, 9,

	0,   96,  0,   3,  0,   96,  32,  4,  0,   96,  64,  5,  0,   96,  96,  6,
	0,   96,  128, 7,  0,   96,  160, 8,  0,   96,  192, 9,  0,   96,  224, 10,

	0,   128, 0,   4,  0,   128, 32,  5,  0,   128, 64,  6,  0,   128, 96,  7,
	0,   128, 128, 8,  0,   128, 160, 9,  0,   128, 192, 10, 0,   128, 224, 11,

	0,   160, 0,   5,  0,   160, 32,  6,  0,   160, 64,  7,  0,   160, 96,  8,
	0,   160, 128, 9,  0,   160, 160, 10, 0,   160, 192, 11, 0,   160, 224, 12,

	0,   192, 0,   6,  0,   192, 32,  7,  0,   192, 64,  8,  0,   192, 96,  9,
	0,   192, 128, 10, 0,   192, 160, 11, 0,   192, 192, 12, 0,   192, 224, 13,

	0,   224, 0,   7,  0,   224, 32,  8,  0,   224, 64,  9,  0,   224, 96,  10,
	0,   224, 128, 11, 0,   224, 160, 12, 0,   224, 192, 13, 0,   224, 224, 14,

	32,  0,   0,   1,  32,  0,   32,  2,  32,  0,   64,  3,  32,  0,   96,  4,
	32,  0,   128, 5,  32,  0,   160, 6,  32,  0,   192, 7,  32,  0,   224, 8,

	32,  32,  0,   2,  32,  32,  32,  3,  32,  32,  64,  4,  32,  32,  96,  5,
	32,  32,  128, 6,  32,  32,  160, 7,  32,  32,  192, 8,  32,  32,  224, 9,

	32,  64,  0,   3,  32,  64,  32,  4,  32,  64,  64,  5,  32,  64,  96,  6,
	32,  64,  128, 7,  32,  64,  160, 8,  32,  64,  192, 9,  32,  64,  224, 10,

	32,  96,  0,   4,  32,  96,  32,  5,  32,  96,  64,  6,  32,  96,  96,  7,
	32,  96,  128, 8,  32,  96,  160, 9,  32,  96,  192, 10, 32,  96,  224, 11,

	32,  128, 0,   5,  32,  128, 32,  6,  32,  128, 64,  7,  32,  128, 96,  8,
	32,  128, 128, 9,  32,  128, 160, 10, 32,  128, 192, 11, 32,  128, 224, 12,

	32,  160, 0,   6,  32,  160, 32,  7,  32,  160, 64,  8,  32,  160, 96,  9,
	32,  160, 128, 10, 32,  160, 160, 11, 32,  160, 192, 12, 32,  160, 224, 13,

	32,  192, 0,   7,  32,  192, 32,  8,  32,  192, 64,  9,  32,  192, 96,  10,
	32,  192, 128, 11, 32,  192, 160, 12, 32,  192, 192, 13, 32,  192, 224, 14,

	32,  224, 0,   8,  32,  224, 32,  9,  32,  224, 64,  10, 32,  224, 96,  11,
	32,  224, 128, 12, 32,  224, 160, 13, 32,  224, 192, 14, 32,  224, 224, 15,

	64,  0,   0,   2,  64,  0,   32,  3,  64,  0,   64,  4,  64,  0,   96,  5,
	64,  0,   128, 6,  64,  0,   160, 7,  64,  0,   192, 8,  64,  0,   224, 9,

	64,  32,  0,   3,  64,  32,  32,  4,  64,  32,  64,  5,  64,  32,  96,  6,
	64,  32,  128, 7,  64,  32,  160, 8,  64,  32,  192, 9,  64,  32,  224, 10,

	64,  64,  0,   4,  64,  64,  32,  5,  64,  64,  64,  6,  64,  64,  96,  7,
	64,  64,  128, 8,  64,  64,  160, 9,  64,  64,  192, 10, 64,  64,  224, 11,

	64,  96,  0,   5,  64,  96,  32,  6,  64,  96,  64,  7,  64,  96,  96,  8,
	64,  96,  128, 9,  64,  96,  160, 10, 64,  96,  192, 11, 64,  96,  224, 12,

	64,  128, 0,   6,  64,  128, 32,  7,  64,  128, 64,  8,  64,  128, 96,  9,
	64,  128, 128, 10, 64,  128, 160, 11, 64,  128, 192, 12, 64,  128, 224, 13,

	64,  160, 0,   7,  64,  160, 32,  8,  64,  160, 64,  9,  64,  160, 96,  10,
	64,  160, 128, 11, 64,  160, 160, 12, 64,  160, 192, 13, 64,  160, 224, 14,

	64,  192, 0,   8,  64,  192, 32,  9,  64,  192, 64,  10, 64,  192, 96,  11,
	64,  192, 128, 12, 64,  192, 160, 13, 64,  192, 192, 14, 64,  192, 224, 15,

	64,  224, 0,   9,  64,  224, 32,  10, 64,  224, 64,  11, 64,  224, 96,  12,
	64,  224, 128, 13, 64,  224, 160, 14, 64,  224, 192, 15, 64,  224, 224, 16,

	96,  0,   0,   3,  96,  0,   32,  4,  96,  0,   64,  5,  96,  0,   96,  6,
	96,  0,   128, 7,  96,  0,   160, 8,  96,  0,   192, 9,  96,  0,   224, 10,

	96,  32,  0,   4,  96,  32,  32,  5,  96,  32,  64,  6,  96,  32,  96,  7,
	96,  32,  128, 8,  96,  32,  160, 9,  96,  32,  192, 10, 96,  32,  224, 11,

	96,  64,  0,   5,  96,  64,  32,  6,  96,  64,  64,  7,  96,  64,  96,  8,
	96,  64,  128, 9,  96,  64,  160, 10, 96,  64,  192, 11, 96,  64,  224, 12,

	96,  96,  0,   6,  96,  96,  32,  7,  96,  96,  64,  8,  96,  96,  96,  9,
	96,  96,  128, 10, 96,  96,  160, 11, 96,  96,  192, 12, 96,  96,  224, 13,

	96,  128, 0,   7,  96,  128, 32,  8,  96,  128, 64,  9,  96,  128, 96,  10,
	96,  128, 128, 11, 96,  128, 160, 12, 96,  128, 192, 13, 96,  128, 224, 14,

	96,  160, 0,   8,  96,  160, 32,  9,  96,  160, 64,  10, 96,  160, 96,  11,
	96,  160, 128, 12, 96,  160, 160, 13, 96,  160, 192, 14, 96,  160, 224, 15,

	96,  192, 0,   9,  96,  192, 32,  10, 96,  192, 64,  11, 96,  192, 96,  12,
	96,  192, 128, 13, 96,  192, 160, 14, 96,  192, 192, 15, 96,  192, 224, 16,

	96,  224, 0,   10, 96,  224, 32,  11, 96,  224, 64,  12, 96,  224, 96,  13,
	96,  224, 128, 14, 96,  224, 160, 15, 96,  224, 192, 16, 96,  224, 224, 17,

	128, 0,   0,   4,  128, 0,   32,  5,  128, 0,   64,  6,  128, 0,   96,  7,
	128, 0,   128, 8,  128, 0,   160, 9,  128, 0,   192, 10, 128, 0,   224, 11,

	128, 32,  0,   5,  128, 32,  32,  6,  128, 32,  64,  7,  128, 32,  96,  8,
	128, 32,  128, 9,  128, 32,  160, 10, 128, 32,  192, 11, 128, 32,  224, 12,

	128, 64,  0,   6,  128, 64,  32,  7,  128, 64,  64,  8,  128, 64,  96,  9,
	128, 64,  128, 10, 128, 64,  160, 11, 128, 64,  192, 12, 128, 64,  224, 13,

	128, 96,  0,   7,  128, 96,  32,  8,  128, 96,  64,  9,  128, 96,  96,  10,
	128, 96,  128, 11, 128, 96,  160, 12, 128, 96,  192, 13, 128, 96,  224, 14,

	128, 128, 0,   8,  128, 128, 32,  9,  128, 128, 64,  10, 128, 128, 96,  11,
	128, 128, 128, 12, 128, 128, 160, 13, 128, 128, 192, 14, 128, 128, 224, 15,

	128, 160, 0,   9,  128, 160, 32,  10, 128, 160, 64,  11, 128, 160, 96,  12,
	128, 160, 128, 13, 128, 160, 160, 14, 128, 160, 192, 15, 128, 160, 224, 16,

	128, 192, 0,   10, 128, 192, 32,  11, 128, 192, 64,  12, 128, 192, 96,  13,
	128, 192, 128, 14, 128, 192, 160, 15, 128, 192, 192, 16, 128, 192, 224, 17,

	128, 224, 0,   11, 128, 224, 32,  12, 128, 224, 64,  13, 128, 224, 96,  14,
	128, 224, 128, 15, 128, 224, 160, 16, 128, 224, 192, 17, 128, 224, 224, 18,

	160, 0,   0,   5,  160, 0,   32,  6,  160, 0,   64,  7,  160, 0,   96,  8,
	160, 0,   128, 9,  160, 0,   160, 10, 160, 0,   192, 11, 160, 0,   224, 12,

	160, 32,  0,   6,  160, 32,  32,  7,  160, 32,  64,  8,  160, 32,  96,  9,
	160, 32,  128, 10, 160, 32,  160, 11, 160, 32,  192, 12, 160, 32,  224, 13,

	160, 64,  0,   7,  160, 64,  32,  8,  160, 64,  64,  9,  160, 64,  96,  10,
	160, 64,  128, 11, 160, 64,  160, 12, 160, 64,  192, 13, 160, 64,  224, 14,

	160, 96,  0,   8,  160, 96,  32,  9,  160, 96,  64,  10, 160, 96,  96,  11,
	160, 96,  128, 12, 160, 96,  160, 13, 160, 96,  192, 14, 160, 96,  224, 15,

	160, 128, 0,   9,  160, 128, 32,  10, 160, 128, 64,  11, 160, 128, 96,  12,
	160, 128, 128, 13, 160, 128, 160, 14, 160, 128, 192, 15, 160, 128, 224, 16,

	160, 160, 0,   10, 160, 160, 32,  11, 160, 160, 64,  12, 160, 160, 96,  13,
	160, 160, 128, 14, 160, 160, 160, 15, 160, 160, 192, 16, 160, 160, 224, 17,

	160, 192, 0,   11, 160, 192, 32,  12, 160, 192, 64,  13, 160, 192, 96,  14,
	160, 192, 128, 15, 160, 192, 160, 16, 160, 192, 192, 17, 160, 192, 224, 18,

	160, 224, 0,   12, 160, 224, 32,  13, 160, 224, 64,  14, 160, 224, 96,  15,
	160, 224, 128, 16, 160, 224, 160, 17, 160, 224, 192, 18, 160, 224, 224, 19,

	192, 0,   0,   6,  192, 0,   32,  7,  192, 0,   64,  8,  192, 0,   96,  9,
	192, 0,   128, 10, 192, 0,   160, 11, 192, 0,   192, 12, 192, 0,   224, 13,

	192, 32,  0,   7,  192, 32,  32,  8,  192, 32,  64,  9,  192, 32,  96,  10,
	192, 32,  128, 11, 192, 32,  160, 12, 192, 32,  192, 13, 192, 32,  224, 14,

	192, 64,  0,   8,  192, 64,  32,  9,  192, 64,  64,  10, 192, 64,  96,  11,
	192, 64,  128, 12, 192, 64,  160, 13, 192, 64,  192, 14, 192, 64,  224, 15,

	192, 96,  0,   9,  192, 96,  32,  10, 192, 96,  64,  11, 192, 96,  96,  12,
	192, 96,  128, 13, 192, 96,  160, 14, 192, 96,  192, 15, 192, 96,  224, 16,

	192, 128, 0,   10, 192, 128, 32,  11, 192, 128, 64,  12, 192, 128, 96,  13,
	192, 128, 128, 14, 192, 128, 160, 15, 192, 128, 192, 16, 192, 128, 224, 17,

	192, 160, 0,   11, 192, 160, 32,  12, 192, 160, 64,  13, 192, 160, 96,  14,
	192, 160, 128, 15, 192, 160, 160, 16, 192, 160, 192, 17, 192, 160, 224, 18,

	192, 192, 0,   12, 192, 192, 32,  13, 192, 192, 64,  14, 192, 192, 96,  15,
	192, 192, 128, 16, 192, 192, 160, 17, 192, 192, 192, 18, 192, 192, 224, 19,

	192, 224, 0,   13, 192, 224, 32,  14, 192, 224, 64,  15, 192, 224, 96,  16,
	192, 224, 128, 17, 192, 224, 160, 18, 192, 224, 192, 19, 192, 224, 224, 20,

	224, 0,   0,   7,  224, 0,   32,  8,  224, 0,   64,  9,  224, 0,   96,  10,
	224, 0,   128, 11, 224, 0,   160, 12, 224, 0,   192, 13, 224, 0,   224, 14,

	224, 32,  0,   8,  224, 32,  32,  9,  224, 32,  64,  10, 224, 32,  96,  11,
	224, 32,  128, 12, 224, 32,  160, 13, 224, 32,  192, 14, 224, 32,  224, 15,

	224, 64,  0,   9,  224, 64,  32,  10, 224, 64,  64,  11, 224, 64,  96,  12,
	224, 64,  128, 13, 224, 64,  160, 14, 224, 64,  192, 15, 224, 64,  224, 16,

	224, 96,  0,   10, 224, 96,  32,  11, 224, 96,  64,  12, 224, 96,  96,  13,
	224, 96,  128, 14, 224, 96,  160, 15, 224, 96,  192, 16, 224, 96,  224, 17,

	224, 128, 0,   11, 224, 128, 32,  12, 224, 128, 64,  13, 224, 128, 96,  14,
	224, 128, 128, 15, 224, 128, 160, 16, 224, 128, 192, 17, 224, 128, 224, 18,

	224, 160, 0,   12, 224, 160, 32,  13, 224, 160, 64,  14, 224, 160, 96,  15,
	224, 160, 128, 16, 224, 160, 160, 17, 224, 160, 192, 18, 224, 160, 224, 19,

	224, 192, 0,   13, 224, 192, 32,  14, 224, 192, 64,  15, 224, 192, 96,  16,
	224, 192, 128, 17, 224, 192, 160, 18, 224, 192, 192, 19, 224, 192, 224, 20,

	224, 224, 0,   14, 224, 224, 32,  15, 224, 224, 64,  16, 224, 224, 96,  17,
	224, 224, 128, 18, 224, 224, 160, 19, 224, 224, 192, 20, 224, 224, 224, 21
};

const glw::GLsizei gl4cts::GetTextureSubImage::Functional::s_texture_data_size =
	sizeof(s_texture_data); //!< Size of the uncompressed texture.

const glw::GLsizei gl4cts::GetTextureSubImage::Functional::s_texture_data_width =
	8; //!< Width  of compressed and uncompressed textures.
const glw::GLsizei gl4cts::GetTextureSubImage::Functional::s_texture_data_height =
	8; //!< Height of compressed and uncompressed textures.
const glw::GLsizei gl4cts::GetTextureSubImage::Functional::s_texture_data_depth =
	8; //!< Depth  of compressed and uncompressed textures.

/** ETC2 8x8x8(layers) pixels in size source texture for testing GetCompressedTextureSubImage. */
const glw::GLubyte gl4cts::GetTextureSubImage::Functional::s_texture_data_compressed[] = {
	/* Layer 0 */
	0x80, 0x80, 0x4, 0x2, 0x1, 0x0, 0x10, 0x0, 0x80, 0x81, 0x4, 0x2, 0x1, 0xf8, 0x10, 0x20,
	0x81, 0x80, 0x4, 0x2, 0x81,	0x0, 0x1f, 0xc0, 0x81, 0x81, 0x4, 0x2, 0x81, 0xf8, 0x1f, 0xe0,
	0x80, 0x80, 0x5, 0x2, 0x1, 0x0, 0x10, 0x0, 0x80, 0x81, 0x5, 0x2, 0x1, 0xf8, 0x10, 0x20,
	0x81, 0x80, 0x5, 0x2, 0x81,	0x0, 0x1f, 0xc0, 0x81, 0x81, 0x5, 0x2, 0x81, 0xf8, 0x1f, 0xe0,

	/* Layer 1 */
	0x90, 0x80, 0x4, 0x12, 0x1, 0x1, 0x10, 0x0, 0x90, 0x81, 0x4, 0x12, 0x1, 0xf9, 0x10, 0x20,
	0x91, 0x80, 0x4, 0x12, 0x81, 0x1, 0x1f, 0xc0, 0x91, 0x81, 0x4, 0x12, 0x81, 0xf9, 0x1f, 0xe0,
	0x90, 0x80, 0x5, 0x12, 0x1, 0x1, 0x10, 0x0, 0x90, 0x81, 0x5, 0x12, 0x1, 0xf9, 0x10, 0x20,
	0x91, 0x80, 0x5, 0x12, 0x81, 0x1, 0x1f, 0xc0, 0x91, 0x81, 0x5, 0x12, 0x81, 0xf9, 0x1f, 0xe0,

	/* Layer 2 */
	0xa0, 0x80, 0x4, 0x22, 0x1, 0x2, 0x10, 0x0, 0xa0, 0x81, 0x4, 0x22, 0x1, 0xfa, 0x10, 0x20,
	0xa1, 0x80, 0x4, 0x22, 0x81, 0x2, 0x1f, 0xc0, 0xa1, 0x81, 0x4, 0x22, 0x81, 0xfa, 0x1f, 0xe0,
	0xa0, 0x80, 0x5, 0x22, 0x1, 0x2, 0x10, 0x0, 0xa0, 0x81, 0x5, 0x22, 0x1, 0xfa, 0x10, 0x20,
	0xa1, 0x80, 0x5, 0x22, 0x81, 0x2, 0x1f, 0xc0, 0xa1, 0x81, 0x5, 0x22, 0x81, 0xfa, 0x1f, 0xe0,

	/* Layer 3 */
	0xb0, 0x80, 0x4, 0x32, 0x1, 0x3, 0x10, 0x0, 0xb0, 0x81, 0x4, 0x32, 0x1, 0xfb, 0x10, 0x20,
	0xb1, 0x80, 0x4, 0x32, 0x81, 0x3, 0x1f, 0xc0, 0xb1, 0x81, 0x4, 0x32, 0x81, 0xfb, 0x1f, 0xe0,
	0xb0, 0x80, 0x5, 0x32, 0x1, 0x3, 0x10, 0x0, 0xb0, 0x81, 0x5, 0x32, 0x1, 0xfb, 0x10, 0x20,
	0xb1, 0x80, 0x5, 0x32, 0x81, 0x3, 0x1f, 0xc0, 0xb1, 0x81, 0x5, 0x32, 0x81, 0xfb, 0x1f, 0xe0,

	/* Layer 4 */
	0x40, 0x80, 0x4, 0x42, 0x1, 0x4, 0x10, 0x0, 0x40, 0x81, 0x4, 0x42, 0x1, 0xfc, 0x10, 0x20,
	0x41, 0x80, 0x4, 0x42, 0x81, 0x4, 0x1f, 0xc0, 0x41, 0x81, 0x4, 0x42, 0x81, 0xfc, 0x1f, 0xe0,
	0x40, 0x80, 0x5, 0x42, 0x1, 0x4, 0x10, 0x0, 0x40, 0x81, 0x5, 0x42, 0x1, 0xfc, 0x10, 0x20,
	0x41, 0x80, 0x5, 0x42, 0x81, 0x4, 0x1f, 0xc0, 0x41, 0x81, 0x5, 0x42, 0x81, 0xfc, 0x1f, 0xe0,

	/* Layer 5 */
	0x50, 0x80, 0x4, 0x52, 0x1, 0x5, 0x10, 0x0, 0x50, 0x81, 0x4, 0x52, 0x1, 0xfd, 0x10, 0x20,
	0x51, 0x80, 0x4, 0x52, 0x81, 0x5, 0x1f, 0xc0, 0x51, 0x81, 0x4, 0x52, 0x81, 0xfd, 0x1f, 0xe0,
	0x50, 0x80, 0x5, 0x52, 0x1, 0x5, 0x10, 0x0, 0x50, 0x81, 0x5, 0x52, 0x1, 0xfd, 0x10, 0x20,
	0x51, 0x80, 0x5, 0x52, 0x81, 0x5, 0x1f, 0xc0, 0x51, 0x81, 0x5, 0x52, 0x81, 0xfd, 0x1f, 0xe0,

	/* Layer 6 */
	0x5e, 0x80, 0x4, 0x5f, 0x1, 0x5, 0xf0, 0x0, 0x5e, 0x81, 0x4, 0x5f, 0x1, 0xfd, 0xf0, 0x20,
	0x5f, 0x80, 0x4, 0x5f, 0x81, 0x5, 0xff, 0xc0, 0x5f, 0x81, 0x4, 0x5f, 0x81, 0xfd, 0xff, 0xe0,
	0x5e, 0x80, 0x5, 0x5f, 0x1, 0x5, 0xf0, 0x0, 0x5e, 0x81, 0x5, 0x5f, 0x1, 0xfd, 0xf0, 0x20,
	0x5f, 0x80, 0x5, 0x5f, 0x81, 0x5, 0xff, 0xc0, 0x5f, 0x81, 0x5, 0x5f, 0x81, 0xfd, 0xff, 0xe0,

	/* Layer 7 */
	0x6e, 0x80, 0x4, 0x6f, 0x1, 0x6, 0xf0, 0x0, 0x6e, 0x81, 0x4, 0x6f, 0x1, 0xfe, 0xf0, 0x20,
	0x6f, 0x80, 0x4, 0x6f, 0x81, 0x6, 0xff, 0xc0, 0x6f, 0x81, 0x4, 0x6f, 0x81, 0xfe, 0xff, 0xe0,
	0x6e, 0x80, 0x5, 0x6f, 0x1, 0x6, 0xf0, 0x0, 0x6e, 0x81, 0x5, 0x6f, 0x1, 0xfe, 0xf0, 0x20,
	0x6f, 0x80, 0x5, 0x6f, 0x81, 0x6, 0xff, 0xc0, 0x6f, 0x81, 0x5, 0x6f, 0x81, 0xfe, 0xff, 0xe0
};

const glw::GLsizei gl4cts::GetTextureSubImage::Functional::s_texture_data_compressed_size =
	sizeof(s_texture_data_compressed); //!< Size of compressed texture.
