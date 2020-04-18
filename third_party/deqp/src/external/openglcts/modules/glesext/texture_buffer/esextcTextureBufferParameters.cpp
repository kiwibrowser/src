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
 * \file  esextcTextureBufferParameters.cpp
 * \brief Texture Buffer GetTexLevelParameter and GetIntegerv test (Test 6)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBufferParameters.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <stddef.h>

namespace glcts
{

const glw::GLuint TextureBufferParameters::m_n_texels_phase_one = 128;
const glw::GLuint TextureBufferParameters::m_n_texels_phase_two = 256;

/** Constructor
 *
 * @param context     Test context
 * @param name        Test case's name
 * @param description Test case's description
 **/
TextureBufferParameters::TextureBufferParameters(Context& context, const ExtParameters& extParams, const char* name,
												 const char* description)
	: TestCaseBase(context, extParams, name, description), m_tbo_id(0), m_to_id(0)
{
}

/** Initializes all GLES objects and reference values for the test. */
void TextureBufferParameters::initTest(void)
{
	/* Skip if required extensions are not supported. */
	if (!m_is_texture_buffer_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BUFFER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	m_internal_formats[GL_R8]		= sizeof(glw::GLubyte) * 1 /* components */;
	m_internal_formats[GL_R16F]		= sizeof(glw::GLhalf) * 1 /* components */;
	m_internal_formats[GL_R32F]		= sizeof(glw::GLfloat) * 1 /* components */;
	m_internal_formats[GL_R8I]		= sizeof(glw::GLbyte) * 1 /* components */;
	m_internal_formats[GL_R16I]		= sizeof(glw::GLshort) * 1 /* components */;
	m_internal_formats[GL_R32I]		= sizeof(glw::GLint) * 1 /* components */;
	m_internal_formats[GL_R8UI]		= sizeof(glw::GLubyte) * 1 /* components */;
	m_internal_formats[GL_R16UI]	= sizeof(glw::GLushort) * 1 /* components */;
	m_internal_formats[GL_R32UI]	= sizeof(glw::GLuint) * 1 /* components */;
	m_internal_formats[GL_RG8]		= sizeof(glw::GLubyte) * 2 /* components */;
	m_internal_formats[GL_RG16F]	= sizeof(glw::GLhalf) * 2 /* components */;
	m_internal_formats[GL_RG32F]	= sizeof(glw::GLfloat) * 2 /* components */;
	m_internal_formats[GL_RG8I]		= sizeof(glw::GLbyte) * 2 /* components */;
	m_internal_formats[GL_RG16I]	= sizeof(glw::GLshort) * 2 /* components */;
	m_internal_formats[GL_RG32I]	= sizeof(glw::GLint) * 2 /* components */;
	m_internal_formats[GL_RG8UI]	= sizeof(glw::GLubyte) * 2 /* components */;
	m_internal_formats[GL_RG16UI]   = sizeof(glw::GLushort) * 2 /* components */;
	m_internal_formats[GL_RG32UI]   = sizeof(glw::GLuint) * 2 /* components */;
	m_internal_formats[GL_RGB32F]   = sizeof(glw::GLfloat) * 3 /* components */;
	m_internal_formats[GL_RGB32I]   = sizeof(glw::GLint) * 3 /* components */;
	m_internal_formats[GL_RGB32UI]  = sizeof(glw::GLuint) * 3 /* components */;
	m_internal_formats[GL_RGBA8]	= sizeof(glw::GLubyte) * 4 /* components */;
	m_internal_formats[GL_RGBA16F]  = sizeof(glw::GLhalf) * 4 /* components */;
	m_internal_formats[GL_RGBA32F]  = sizeof(glw::GLfloat) * 4 /* components */;
	m_internal_formats[GL_RGBA8I]   = sizeof(glw::GLbyte) * 4 /* components */;
	m_internal_formats[GL_RGBA16I]  = sizeof(glw::GLshort) * 4 /* components */;
	m_internal_formats[GL_RGBA32I]  = sizeof(glw::GLint) * 4 /* components */;
	m_internal_formats[GL_RGBA8UI]  = sizeof(glw::GLubyte) * 4 /* components */;
	m_internal_formats[GL_RGBA16UI] = sizeof(glw::GLushort) * 4 /* components */;
	m_internal_formats[GL_RGBA32UI] = sizeof(glw::GLuint) * 4 /* components */;

	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate texture object!");

	gl.bindTexture(m_glExtTokens.TEXTURE_BUFFER, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind texture object!");

	gl.genBuffers(1, &m_tbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate buffer object!");

	gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, m_tbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object!");
}

/** Deinitializes GLES objects created during the test */
void TextureBufferParameters::deinit(void)
{
	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES state */
	gl.bindTexture(m_glExtTokens.TEXTURE_BUFFER, 0);
	gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, 0);

	/* Delete GLEs objects */
	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);
		m_to_id = 0;
	}

	if (m_tbo_id != 0)
	{
		gl.deleteBuffers(1, &m_tbo_id);
		m_tbo_id = 0;
	}

	/* Deinitialize base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult TextureBufferParameters::iterate(void)
{
	/* Initialization */
	initTest();

	/* Retrieve GLEs entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLboolean test_passed = true;

	/* Check GL_TEXTURE_BINDING_BUFFER_EXT */
	test_passed = test_passed && queryTextureBufferBinding(m_to_id);

	/* Check GL_TEXTURE_BUFFER_BINDING_EXT */
	test_passed = test_passed && queryTextureBindingBuffer(m_tbo_id);

	/* For each GL_TEXTURE_INTERNAL_FORMAT */
	for (InternalFormatsMap::iterator iter = m_internal_formats.begin(); iter != m_internal_formats.end(); ++iter)
	{
		std::vector<glw::GLubyte> data_phase_one(m_n_texels_phase_one * iter->second, 0);
		gl.bufferData(m_glExtTokens.TEXTURE_BUFFER, m_n_texels_phase_one * iter->second, &data_phase_one[0],
					  GL_STATIC_READ);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not allocate buffer object's data store!");

		gl.texBuffer(m_glExtTokens.TEXTURE_BUFFER, iter->first, m_tbo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set buffer object as data source for texture buffer!");

		/* Check GL_TEXTURE_BUFFER_DATA_STORE_BINDING_EXT */
		test_passed = test_passed && queryTextureBufferDataStoreBinding(m_tbo_id);

		/* Check GL_TEXTURE_INTERNAL_FORMAT */
		test_passed = test_passed && queryTextureInternalFormat(iter->first);

		/* Check GL_TEXTURE_BUFFER_OFFSET_EXT */
		test_passed = test_passed && queryTextureBufferOffset(0);

		/* Ckeck GL_TEXTURE_BUFFER_SIZE_EXT */
		test_passed = test_passed && queryTextureBufferSize(m_n_texels_phase_one * iter->second);

		/* Ckeck wrong lod level */
		test_passed = test_passed && queryTextureInvalidLevel();

		/* Get texture buffer offset alignment */
		glw::GLint offset_alignment = 0;
		gl.getIntegerv(m_glExtTokens.TEXTURE_BUFFER_OFFSET_ALIGNMENT, &offset_alignment);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not get texture buffer offset alignment!");

		/* Resize buffer object */
		std::vector<glw::GLubyte> data_phase_two(m_n_texels_phase_two * iter->second + offset_alignment, 0);
		gl.bufferData(m_glExtTokens.TEXTURE_BUFFER, m_n_texels_phase_two * iter->second + offset_alignment,
					  &data_phase_two[0], GL_STATIC_READ);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not allocate buffer object's data store!");

		/* Check GL_TEXTURE_BUFFER_OFFSET_EXT */
		test_passed = test_passed && queryTextureBufferOffset(0);

		/* Check GL_TEXTURE_BUFFER_SIZE_EXT */
		test_passed = test_passed && queryTextureBufferSize(m_n_texels_phase_two * iter->second + offset_alignment);

		gl.texBufferRange(m_glExtTokens.TEXTURE_BUFFER, iter->first, m_tbo_id, offset_alignment,
						  m_n_texels_phase_two * iter->second);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set buffer object as data source for texture buffer!");

		/* Check GL_TEXTURE_BUFFER_OFFSET_EXT */
		test_passed = test_passed && queryTextureBufferOffset(offset_alignment);

		/* Check GL_TEXTURE_BUFFER_SIZE_EXT */
		test_passed = test_passed && queryTextureBufferSize(m_n_texels_phase_two * iter->second);

		gl.texBuffer(m_glExtTokens.TEXTURE_BUFFER, iter->first, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not reset buffer object binding!");
	}

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

/** Query GL_TEXTURE_BUFFER_BINDING_EXT and compare with the expected value.
 *
 *  Note - the function throws exception should an error occur!
 *
 *  @param expected     Expected value used for comparison.
 *
 *  @return true  if the comparison has passed,
 *          false if the comparison has failed.
 **/
glw::GLboolean TextureBufferParameters::queryTextureBindingBuffer(glw::GLint expected)
{
	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint	 result	  = -1;
	glw::GLboolean test_passed = true;

	gl.getIntegerv(m_glExtTokens.TEXTURE_BUFFER_BINDING, &result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query value of GL_TEXTURE_BUFFER_BINDING_EXT");

	if (result != expected)
	{
		test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glGetIntegerv(GL_TEXTURE_BINDING_BUFFER_EXT) returned "
						   << result << " which is not equal to expected buffer object id == " << expected << ".\n"
						   << tcu::TestLog::EndMessage;
	}

	return test_passed;
}

/** Query GL_TEXTURE_BINDING_BUFFER_EXT and compare with the expected value.
 *
 *  Note - the function throws exception should an error occur!
 *
 *  @param expected     Expected value used for comparison.
 *
 *  @return true  if the comparison has passed,
 *          false if the comparison has failed.
 **/
glw::GLboolean TextureBufferParameters::queryTextureBufferBinding(glw::GLint expected)
{
	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint	 result	  = -1;
	glw::GLboolean test_passed = true;

	gl.getIntegerv(m_glExtTokens.TEXTURE_BINDING_BUFFER, &result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query value of GL_TEXTURE_BINDING_BUFFER_EXT");

	if (result != expected)
	{
		test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glGetIntegerv(GL_TEXTURE_BUFFER_BINDING_EXT) returned "
						   << result << " which is not equal to expected texture object id == " << expected << ".\n"
						   << tcu::TestLog::EndMessage;
	}

	return test_passed;
}

/** Query GL_TEXTURE_BUFFER_DATA_STORE_BINDING_EXT and compare with the expected value.
 *
 *  Note - the function throws exception should an error occur!
 *
 *  @param expected     Expected value used for comparison.
 *
 *  @return true  if the comparison has passed,
 *          false if the comparison has failed.
 **/
glw::GLboolean TextureBufferParameters::queryTextureBufferDataStoreBinding(glw::GLint expected)
{
	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint	 result	  = -1;
	glw::GLboolean test_passed = true;

	gl.getTexLevelParameteriv(m_glExtTokens.TEXTURE_BUFFER, 0, m_glExtTokens.TEXTURE_BUFFER_DATA_STORE_BINDING,
							  &result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query value of GL_TEXTURE_BUFFER_DATA_STORE_BINDING_EXT");

	if (result != expected)
	{
		test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message
						   << "glGetTexLevelParameteriv(GL_TEXTURE_BUFFER_DATA_STORE_BINDING_EXT) returned " << result
						   << " which is not equal to expected buffer object id == " << expected << ".\n"
						   << tcu::TestLog::EndMessage;
	}

	return test_passed;
}

/** Query GL_TEXTURE_BUFFER_OFFSET_EXT and compare with the expected value.
 *
 *  Note - the function throws exception should an error occur!
 *
 *  @param expected     Expected value used for comparison.
 *
 *  @return true  if the comparison has passed,
 *          false if the comparison has failed.
 **/
glw::GLboolean TextureBufferParameters::queryTextureBufferOffset(glw::GLint expected)
{
	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint	 result	  = -1;
	glw::GLboolean test_passed = true;

	gl.getTexLevelParameteriv(m_glExtTokens.TEXTURE_BUFFER, 0, m_glExtTokens.TEXTURE_BUFFER_OFFSET, &result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query value of GL_TEXTURE_BUFFER_OFFSET_EXT");

	if (result != expected)
	{
		test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message
						   << "glGetTexLevelParameteriv(GL_TEXTURE_BUFFER_OFFSET_EXT) returned " << result
						   << " which is not equal to expected offset " << expected << ".\n"
						   << tcu::TestLog::EndMessage;
	}

	return test_passed;
}

/** Query GL_TEXTURE_BUFFER_SIZE_EXT and compare with the expected value.
 *
 *  Note - the function throws exception should an error occur!
 *
 *  @param expected     Expected value used for comparison.
 *
 *  @return true  if the comparison has passed,
 *          false if the comparison has failed.
 **/
glw::GLboolean TextureBufferParameters::queryTextureBufferSize(glw::GLint expected)
{
	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint	 result	  = -1;
	glw::GLboolean test_passed = true;

	gl.getTexLevelParameteriv(m_glExtTokens.TEXTURE_BUFFER, 0, m_glExtTokens.TEXTURE_BUFFER_SIZE, &result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query value of GL_TEXTURE_BUFFER_SIZE_EXT");

	if (result != expected)
	{
		test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glGetTexLevelParameteriv(GL_TEXTURE_BUFFER_SIZE_EXT) returned "
						   << result << " which is not equal to expected size " << expected << ".\n"
						   << tcu::TestLog::EndMessage;
	}

	return test_passed;
}

/** Query GL_TEXTURE_INTERNAL_FORMAT and compare with the expected value.
 *
 *  Note - the function throws exception should an error occur!
 *
 *  @param expected     Expected value used for comparison.
 *
 *  @return true  if the comparison has passed,
 *          false if the comparison has failed.
 **/
glw::GLboolean TextureBufferParameters::queryTextureInternalFormat(glw::GLint expected)
{
	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint	 result	  = -1;
	glw::GLboolean test_passed = true;

	gl.getTexLevelParameteriv(m_glExtTokens.TEXTURE_BUFFER, 0, GL_TEXTURE_INTERNAL_FORMAT, &result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query value of GL_TEXTURE_INTERNAL_FORMAT");

	if (result != expected)
	{
		test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glGetTexLevelParameteriv(GL_TEXTURE_INTERNAL_FORMAT) returned "
						   << result << " which is not equal to expected internal format " << expected
						   << tcu::TestLog::EndMessage;
	}

	return test_passed;
}

/** Query GL_TEXTURE_BUFFER_SIZE_EXT with invalid texture level
 *   and checks for GL_INVALID_VALUE error.
 *
 *  Note - the function throws exception should an error occur!
 *
 *  @return true  if the GL_INVALID_VALUE error was generated,
 *          false if the GL_INVALID_VALUE error was not generated.
 **/
glw::GLboolean TextureBufferParameters::queryTextureInvalidLevel()
{
	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint	 result	  = -1;
	glw::GLboolean test_passed = true;

	gl.getTexLevelParameteriv(m_glExtTokens.TEXTURE_BUFFER, 1, m_glExtTokens.TEXTURE_BUFFER_SIZE, &result);
	glw::GLenum error_code = gl.getError();

	if (error_code != GL_INVALID_VALUE)
	{
		test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message
						   << "glGetTexLevelParameteriv() called for GL_TEXTURE_BUFFER_EXT texture target "
						   << "with lod level different than 0 did not generate an GL_INVALID_VALUE error.\n"
						   << tcu::TestLog::EndMessage;
	}

	return test_passed;
}

} /* namespace glcts */
