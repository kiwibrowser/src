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
 * \file  esextcTextureBufferErrors.hpp
 * \brief TexBufferEXT and TexBufferRangeEXT errors (Test 7)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBufferErrors.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstddef>

namespace glcts
{

/* Size of buffer object's data store */
const glw::GLint TextureBufferErrors::m_bo_size =
	256 /* number of texels */ * 4 /* number of components */ * sizeof(glw::GLint);

/** Constructor
 *
 * @param context     Test context
 * @param name        Test case's name
 * @param description Test case's description
 **/
TextureBufferErrors::TextureBufferErrors(Context& context, const ExtParameters& extParams, const char* name,
										 const char* description)
	: TestCaseBase(context, extParams, name, description), m_bo_id(0), m_tex_id(0)
{
}

/** Deinitializes all GLES objects created for the test. */
void TextureBufferErrors::deinit(void)
{
	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES state */
	gl.bindTexture(m_glExtTokens.TEXTURE_BUFFER, 0);
	gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, 0);

	/* Delete GLES objects */
	if (m_tex_id != 0)
	{
		gl.deleteTextures(1, &m_tex_id);
		m_tex_id = 0;
	}

	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);
		m_bo_id = 0;
	}

	/* Deinitialize base class */
	TestCaseBase::deinit();
}

/** Initializes all GLES objects and reference values for the test. */
void TextureBufferErrors::initTest(void)
{
	/* Skip if required extensions are not supported. */
	if (!m_is_texture_buffer_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BUFFER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate texture object!");

	gl.bindTexture(m_glExtTokens.TEXTURE_BUFFER, m_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind texture object!");

	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate buffer object!");

	gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object!");

	gl.bufferData(m_glExtTokens.TEXTURE_BUFFER, m_bo_size, DE_NULL, GL_STATIC_READ);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not allocate buffer object's data store");

	m_texture_targets.push_back(GL_TEXTURE_2D);
	m_texture_targets.push_back(GL_TEXTURE_2D_ARRAY);
	m_texture_targets.push_back(GL_TEXTURE_3D);
	m_texture_targets.push_back(GL_TEXTURE_CUBE_MAP);

	if (m_is_texture_cube_map_array_supported)
	{
		m_texture_targets.push_back(GL_TEXTURE_CUBE_MAP_ARRAY);
	}

	if (m_is_texture_storage_multisample_supported)
	{
		m_texture_targets.push_back(GL_TEXTURE_2D_MULTISAMPLE);
	}

	if (m_is_texture_storage_multisample_2d_array_supported)
	{
		m_texture_targets.push_back(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES);
	}
}
/** Test if the error code returned by glGetError is the same as expected.
 *  If the error is different from expected description is logged.
 *
 * @param expected_error    GLenum error which is expected
 * @param description       Log message in the case of failure.
 *
 * @return true if error is equal to expected, false otherwise.
 */
glw::GLboolean TextureBufferErrors::verifyError(const glw::GLenum expected_error, const char* description)
{
	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLboolean test_passed = true;
	glw::GLenum	error_code  = gl.getError();

	if (error_code != expected_error)
	{
		test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << description << tcu::TestLog::EndMessage;
	}

	return test_passed;
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult TextureBufferErrors::iterate(void)
{
	/* Initialization */
	initTest();

	/* Retrieve ES entry/state points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLboolean test_passed = true;

	/*An INVALID_ENUM error should be generated if target parameter is not TEXTURE_BUFFER_EXT.*/
	for (TargetsVector::iterator iter = m_texture_targets.begin(); iter != m_texture_targets.end(); ++iter)
	{
		gl.texBuffer(*iter, GL_RGBA32I, m_bo_id);
		test_passed = verifyError(GL_INVALID_ENUM, "Expected GL_INVALID_ENUM was not returned "
												   "when wrong texture target was used in glTexBufferEXT.") &&
					  test_passed;

		gl.texBufferRange(*iter, GL_RGBA32I, m_bo_id, 0, m_bo_size);
		test_passed = verifyError(GL_INVALID_ENUM, "Expected GL_INVALID_ENUM was not returned "
												   "when wrong texture target was used in glTexBufferRangeEXT.") &&
					  test_passed;
	}

	/* An INVALID_ENUM error should be generated if internal format parameter
	 * is not one of the sized internal formats specified in the extension
	 * specification. One of the formats that should generate INVALID_ENUM
	 * error is GL_DEPTH_COMPONENT32F. */
	gl.texBuffer(m_glExtTokens.TEXTURE_BUFFER, GL_DEPTH_COMPONENT32F, m_bo_id);
	test_passed = verifyError(GL_INVALID_ENUM, "Expected GL_INVALID_ENUM was not returned "
											   "when wrong sized internal format was used "
											   "in glTexBufferEXT.") &&
				  test_passed;

	gl.texBufferRange(m_glExtTokens.TEXTURE_BUFFER, GL_DEPTH_COMPONENT32F, m_bo_id, 0, m_bo_size);
	test_passed = verifyError(GL_INVALID_ENUM, "Expected GL_INVALID_ENUM was not returned "
											   "when wrong sized internal format was used "
											   "in glTexBufferRangeEXT.") &&
				  test_passed;

	/* An INVALID_OPERATION error should be generated if buffer parameter
	 * is non-zero, but is not the name of an existing buffer object. */
	gl.texBuffer(m_glExtTokens.TEXTURE_BUFFER, GL_RGBA32I, m_bo_id + 1);
	test_passed = verifyError(GL_INVALID_OPERATION, "Expected GL_INVALID_OPERATION was not returned "
													"when non-existing buffer object was used in glTexBufferEXT.") &&
				  test_passed;

	gl.texBufferRange(m_glExtTokens.TEXTURE_BUFFER, GL_RGBA32I, m_bo_id + 1, 0, m_bo_size);
	test_passed =
		verifyError(GL_INVALID_OPERATION, "Expected GL_INVALID_OPERATION was not returned "
										  "when non-existing buffer object was used in glTexBufferRangeEXT.") &&
		test_passed;

	/* INVALID_VALUE error should be generated if offset parameter is negative */
	gl.texBufferRange(m_glExtTokens.TEXTURE_BUFFER, GL_RGBA32I, m_bo_id, -16, m_bo_size);
	test_passed = verifyError(GL_INVALID_VALUE, "Expected INVALID_VALUE was not returned "
												"when negative offset was used in glTexBufferRangeEXT.") &&
				  test_passed;

	/* INVALID_VALUE error should be generated if size parameter is less than or equal to zero */
	gl.texBufferRange(m_glExtTokens.TEXTURE_BUFFER, GL_RGBA32I, m_bo_id, 0, 0);
	test_passed = verifyError(GL_INVALID_VALUE, "Expected INVALID_VALUE was not returned "
												"when 0 size was used in glTexBufferRangeEXT.") &&
				  test_passed;

	gl.texBufferRange(m_glExtTokens.TEXTURE_BUFFER, GL_RGBA32I, m_bo_id, 0, -1);
	test_passed = verifyError(GL_INVALID_VALUE, "Expected INVALID_VALUE was not returned "
												"when negative size was used in glTexBufferRangeEXT.") &&
				  test_passed;

	/* INVALID_VALUE error should be generated if offset + size is greater than the value of BUFFER_SIZE */
	gl.texBufferRange(m_glExtTokens.TEXTURE_BUFFER, GL_RGBA32I, m_bo_id, m_bo_size / 2, m_bo_size);
	test_passed = verifyError(GL_INVALID_VALUE, "Expected INVALID_VALUE was not returned "
												"when offset + size is greater than BUFFER_SIZE") &&
				  test_passed;

	/* INVALID_VALUE error should be generated if offset parameter is not an integer multiple of
	 * value(TEXTURE_BUFFER_OFFSET_ALIGNMENT_EXT). */
	glw::GLint texture_buffer_offset_alignment = 0;
	gl.getIntegerv(m_glExtTokens.TEXTURE_BUFFER_OFFSET_ALIGNMENT, &texture_buffer_offset_alignment);

	if (texture_buffer_offset_alignment > 1)
	{
		gl.texBufferRange(m_glExtTokens.TEXTURE_BUFFER, GL_RGBA32I, m_bo_id, texture_buffer_offset_alignment / 2,
						  m_bo_size - texture_buffer_offset_alignment / 2);
		test_passed = verifyError(GL_INVALID_VALUE, "Expected INVALID_VALUE was not returned "
													"when improperly aligned offset was used "
													"in glTexBufferRangeEXT.") &&
					  test_passed;
	}

	/* All done */
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

} /* namespace glcts */
