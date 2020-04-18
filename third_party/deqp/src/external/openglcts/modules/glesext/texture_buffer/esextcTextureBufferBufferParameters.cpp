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
 * \file  esextcTextureBufferBufferParameters.cpp
 * \brief GetBufferParameteriv and GetBufferPointerv test (Test 9)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBufferBufferParameters.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <stddef.h>

namespace glcts
{

/* Size of buffer object's data store */
const glw::GLint TextureBufferBufferParameters::m_bo_size =
	256 /* number of texels */ * sizeof(glw::GLubyte) /* size of one texel */;

/** Constructor
 *
 * @param context     Test context
 * @param name        Test case's name
 * @param description Test case's description
 **/
TextureBufferBufferParameters::TextureBufferBufferParameters(Context& context, const ExtParameters& extParams,
															 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_bo_id(0), m_buffer_pointer(DE_NULL)
{
}

/** Deinitializes all GLES objects created for the test. */
void TextureBufferBufferParameters::deinit(void)
{
	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES state */
	gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, 0);
	gl.unmapBuffer(m_glExtTokens.TEXTURE_BUFFER);
	gl.getError();
	m_buffer_pointer = DE_NULL;

	/* Delete GLES objects */
	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);
		m_bo_id = 0;
	}

	/* Deinitialize base class */
	TestCaseBase::deinit();
}

/** Initializes all GLES objects and reference values for the test. */
void TextureBufferBufferParameters::initTest(void)
{
	/* Skip if required extensions are not supported. */
	if (!m_is_texture_buffer_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BUFFER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate buffer object!");

	gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object!");
}

/** Query the data using glGetBufferParameteriv and compare against expected_data.
 *
 * @param target            buffer target
 * @param pname             queried parameter name
 * @param expected_data     expected return from OpenGL
 *
 * @return true if queried data is equal to expected_data, false otherwise.
 */
glw::GLboolean TextureBufferBufferParameters::queryBufferParameteriv(glw::GLenum target, glw::GLenum pname,
																	 glw::GLint expected_data)
{
	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint	 result	  = -1;
	glw::GLboolean test_passed = true;

	gl.getBufferParameteriv(target, pname, &result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query for value of buffer object's parameter!");

	if (result != expected_data)
	{
		test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glGetBufferParameteriv(pname == " << pname << ") returned "
						   << result << " which is not equal to expected value " << expected_data << "."
						   << tcu::TestLog::EndMessage;
	}

	return test_passed;
}

/** Query the data using glGetBufferParameteri64v and compare against expected_data.
 *
 * @param target            buffer target
 * @param pname             queried parameter name
 * @param expected_data     expected return from OpenGL
 *
 * @return true if queried data is equal to expected_data, false otherwise.
 */
glw::GLboolean TextureBufferBufferParameters::queryBufferParameteri64v(glw::GLenum target, glw::GLenum pname,
																	   glw::GLint64 expected_data)
{
	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint64   result	  = -1;
	glw::GLboolean test_passed = true;

	gl.getBufferParameteri64v(target, pname, &result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query for value of buffer object's parameter!");

	if (result != expected_data)
	{
		test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glGetBufferParameteri64v(pname == " << pname << ") returned "
						   << result << " which is not equal to expected value " << expected_data << "."
						   << tcu::TestLog::EndMessage;
	}

	return test_passed;
}

/** Query the mapped pointer using glGetBufferPointerv and compare against expected_pointer.
 *
 * @param target            buffer target
 * @param pname             queried parameter name
 * @param expected_pointer  expected return pointer from OpenGL
 *
 * @return true if queried pointer is equal to expected_params, false otherwise.
 */
glw::GLboolean TextureBufferBufferParameters::queryBufferPointerv(glw::GLenum target, glw::GLenum pname,
																  glw::GLvoid* expected_pointer)
{
	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLvoid*   result_pointer = DE_NULL;
	glw::GLboolean test_passed	= true;

	gl.getBufferPointerv(target, pname, &result_pointer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not get the mapped buffer's pointer using getBufferPointerv!");

	if (result_pointer != expected_pointer)
	{
		test_passed = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "glGetBufferPointerv(pname == " << pname << ") returned pointer "
						   << result_pointer << " which is not equal to expected pointer " << expected_pointer << "."
						   << tcu::TestLog::EndMessage;
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
tcu::TestNode::IterateResult TextureBufferBufferParameters::iterate(void)
{
	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initialization */
	initTest();

	glw::GLboolean test_passed = true;

	/* Query GL_BUFFER_SIZE without buffer object's data store initialized */
	test_passed =
		queryBufferParameteriv(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_SIZE, 0 /* expected size */) && test_passed;
	test_passed =
		queryBufferParameteri64v(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_SIZE, 0 /* expected size */) && test_passed;

	gl.bufferData(m_glExtTokens.TEXTURE_BUFFER, m_bo_size, DE_NULL, GL_STATIC_READ);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not initialize buffer object's data store!");

	/* Query GL_BUFFER_SIZE with buffer object's data store initialized */
	test_passed = queryBufferParameteriv(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_SIZE, m_bo_size /* expected size */) &&
				  test_passed;
	test_passed = queryBufferParameteri64v(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_SIZE,
										   (glw::GLint64)m_bo_size /* expected size */) &&
				  test_passed;

	/* Query GL_BUFFER_USAGE */
	test_passed = queryBufferParameteriv(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_USAGE, GL_STATIC_READ) && test_passed;

	/* Query GL_BUFFER_MAP... pnames without buffer object's data store being mapped */
	test_passed = queryBufferParameteriv(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_MAPPED, GL_FALSE) && test_passed;

	test_passed = queryBufferParameteriv(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_MAP_OFFSET, 0 /* expected offset */) &&
				  test_passed;
	test_passed =
		queryBufferParameteri64v(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_MAP_OFFSET, 0 /* expected offset */) &&
		test_passed;

	test_passed = queryBufferParameteriv(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_MAP_LENGTH, 0 /* expected size */) &&
				  test_passed;
	test_passed = queryBufferParameteri64v(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_MAP_LENGTH, 0 /* expected size */) &&
				  test_passed;

	test_passed =
		queryBufferPointerv(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_MAP_POINTER, (glw::GLvoid*)DE_NULL) && test_passed;

	/* Mapping whole buffer object's data store */
	m_buffer_pointer = (glw::GLubyte*)gl.mapBufferRange(m_glExtTokens.TEXTURE_BUFFER, 0, m_bo_size, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not map buffer object's data store to client's address space!");

	/* Query GL_BUFFER_MAP... pnames with buffer object's data store being mapped */
	test_passed = queryBufferParameteriv(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_MAPPED, GL_TRUE) && test_passed;

	test_passed = queryBufferParameteriv(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_MAP_OFFSET, 0 /* expected offset */) &&
				  test_passed;
	test_passed =
		queryBufferParameteri64v(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_MAP_OFFSET, 0 /* expected offset */) &&
		test_passed;

	test_passed =
		queryBufferParameteriv(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_MAP_LENGTH, m_bo_size /* expected size */) &&
		test_passed;
	test_passed = queryBufferParameteri64v(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_MAP_LENGTH,
										   (glw::GLint64)m_bo_size /* expected size */) &&
				  test_passed;

	test_passed =
		queryBufferParameteriv(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_ACCESS_FLAGS, GL_MAP_READ_BIT) && test_passed;

	test_passed =
		queryBufferPointerv(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_MAP_POINTER, (glw::GLvoid*)m_buffer_pointer) &&
		test_passed;

	/* Unmapping buffer object's data store */
	m_buffer_pointer = DE_NULL;
	gl.unmapBuffer(m_glExtTokens.TEXTURE_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Unmapping buffer failed");

	/* Mapping part of buffer object's data store */
	m_buffer_pointer = (glw::GLubyte*)gl.mapBufferRange(m_glExtTokens.TEXTURE_BUFFER, m_bo_size / 2 /* offset */,
														m_bo_size / 2 /* size */, GL_MAP_WRITE_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not map buffer object's data store to client's address space!");

	/* Query GL_BUFFER_MAP... pnames with buffer object's data store being mapped */
	test_passed = queryBufferParameteriv(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_MAPPED, GL_TRUE) && test_passed;

	test_passed = queryBufferParameteriv(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_MAP_OFFSET,
										 (m_bo_size / 2) /* expected offset */) &&
				  test_passed;
	test_passed = queryBufferParameteri64v(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_MAP_OFFSET,
										   (glw::GLint64)(m_bo_size / 2) /* expected offset */) &&
				  test_passed;

	test_passed = queryBufferParameteriv(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_MAP_LENGTH,
										 (m_bo_size / 2) /* expected size */) &&
				  test_passed;
	test_passed = queryBufferParameteri64v(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_MAP_LENGTH,
										   (glw::GLint64)(m_bo_size / 2) /* expected size */) &&
				  test_passed;

	test_passed =
		queryBufferParameteriv(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_ACCESS_FLAGS, GL_MAP_WRITE_BIT) && test_passed;

	test_passed =
		queryBufferPointerv(m_glExtTokens.TEXTURE_BUFFER, GL_BUFFER_MAP_POINTER, (glw::GLvoid*)m_buffer_pointer) &&
		test_passed;

	/* Unmapping buffer object's data store */
	m_buffer_pointer = DE_NULL;
	gl.unmapBuffer(m_glExtTokens.TEXTURE_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Unmapping buffer failed");

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
