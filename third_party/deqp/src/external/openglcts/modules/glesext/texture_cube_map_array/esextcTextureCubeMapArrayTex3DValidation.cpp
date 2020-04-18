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
 * \file  esextcTextureCubeMapArrayTex3DValidation.cpp
 * \brief texture_cube_map_array extenstion - Tex3DValidation (Test 4)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureCubeMapArrayTex3DValidation.hpp"

#include "gluContextInfo.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cmath>

namespace glcts
{

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
TextureCubeMapArrayTex3DValidation::TextureCubeMapArrayTex3DValidation(Context& context, const ExtParameters& extParams,
																	   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_to_id(0)
{
	/* Nothing to be done here */
}

/** Deinitializes GLES objects created during the test.
 *
 */
void TextureCubeMapArrayTex3DValidation::deinit()
{
	deleteTexture();

	/* Deinitialize base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestCase::IterateResult TextureCubeMapArrayTex3DValidation::iterate()
{
	/* Check if GL_EXT_texture_cube_map_array extension is supported */
	if (!m_is_texture_cube_map_array_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_CUBE_MAP_ARRAY_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	/* Check if GL_INVALID_VALUE is not set if width and height are equal */
	const glw::GLint correctWidth  = 4;
	const glw::GLint correctHeight = 4;
	const glw::GLint correctDepth  = 6;

	createTexture();
	gl.texImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0 /* level */, GL_RGBA8 /* internalformat*/, correctWidth, correctHeight,
				  correctDepth, 0 /* border */, GL_RGBA, GL_UNSIGNED_BYTE, DE_NULL);

	if (!checkError(GL_NO_ERROR, "glTexImage3D() call failed when called with valid arguments - "))
	{
		result = false;
	}

	gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, /* levels */
					GL_RGBA8,					  /* internalformat*/
					correctWidth, correctHeight, correctDepth);

	if (!checkError(GL_NO_ERROR, "glTexStorage3D() call failed when called with valid arguments - "))
	{
		result = false;
	}

	deleteTexture();
	createTexture();

	/* Check if GL_INVALID_VALUE is set if width and height are not equal*/
	const glw::GLint incorrectWidth  = 4;
	const glw::GLint incorrectHeight = 3;

	gl.texImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0,						/* level */
				  GL_RGBA8,											/* internalformat*/
				  incorrectWidth, incorrectHeight, correctDepth, 0, /* border */
				  GL_RGBA, GL_UNSIGNED_BYTE, DE_NULL);

	if (!checkError(GL_INVALID_VALUE, "glTexImage3D() call generated an invalid error code when width != height - "))
	{
		result = false;
	}

	gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, /* levels */
					GL_RGBA8,					  /* internalformat*/
					incorrectWidth, incorrectHeight, correctDepth);

	if (!checkError(GL_INVALID_VALUE, "glTexStorage3D() call generated an invalid error code "
									  "when width != height - "))
	{
		result = false;
	}

	deleteTexture();
	createTexture();

	/* Check if GL_INVALID_VALUE is set if depth is not a multiple of six */
	const glw::GLint incorrectDepth = 7;

	gl.texImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0,					  /* level */
				  GL_RGBA8,										  /* internalformat */
				  correctWidth, correctHeight, incorrectDepth, 0, /* border */
				  GL_RGBA, GL_UNSIGNED_BYTE, DE_NULL);

	if (!checkError(GL_INVALID_VALUE, "glTexImage3D() call generated an invalid error code"
									  " when depth was not a multiple of six - "))
	{
		result = false;
	}

	deleteTexture();
	createTexture();

	gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, /* levels */
					GL_RGBA8,					  /* internalformat*/
					correctWidth, correctHeight, incorrectDepth);

	if (!checkError(GL_INVALID_VALUE, "glTexStorage3D() call generated an invalid error "
									  "code when depth was not a multiple of six - "))
	{
		result = false;
	}

	deleteTexture();
	createTexture();

	/* Check if GL_INVALID_OPERATION is set if levels argument is greater than floor(log2(max(width, height))) + 1*/
	const glw::GLfloat maxTextureSize = (const glw::GLfloat)de::max(correctWidth, correctHeight);
	const glw::GLint   maxLevels	  = (const glw::GLint)floor(log(maxTextureSize) / log(2.0f)) + 1;

	gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, maxLevels + 1, /* levels */
					GL_RGBA8,								  /* internalformat*/
					correctWidth, correctHeight, correctDepth);

	if (!checkError(GL_INVALID_OPERATION, "glTexStorage3D() call generated an invalid error code when levels argument "
										  "was greater than floor(log2(max(width, height))) + 1 - "))
	{
		result = false;
	}

	deleteTexture();

	/* Set Test Result */
	if (result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Check if user-specified error is reported by ES implementation.
 *
 *   @param expectedError anticipated error code.
 *   @param message       Message to be logged in case the error codes differ.
 *                        Must not be NULL.
 *
 *   @return              returns true if error return by glGetError() is equal expectedError, false otherwise
 */
bool TextureCubeMapArrayTex3DValidation::checkError(glw::GLint expectedError, const char* message)
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	glw::GLint			  error  = gl.getError();
	bool				  result = true;

	if (error != expectedError)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << message << " Expected error: " << glu::getErrorStr(expectedError)
						   << " Current error: " << glu::getErrorStr(error) << tcu::TestLog::EndMessage;
		result = false;
	}

	return result;
}

/** Generates a texture object and binds it to GL_TEXTURE_CUBE_MAP_ARRAY_EXT texture
 *  target.
 *
 */
void TextureCubeMapArrayTex3DValidation::createTexture()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed");

	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(),
						"Error binding the texture object to GL_TEXTURE_CUBE_MAP_ARRAY_EXT texture target!");
}

/** Delete a texture object that is being used by the test at
 *  the time of the call.
 *
 **/
void TextureCubeMapArrayTex3DValidation::deleteTexture()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_is_texture_cube_map_array_supported)
	{
		/* Unbind any bound texture to GL_TEXTURE_CUBE_MAP_ARRAY_EXT target */
		gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"glBindTexture() call failed for GL_TEXTURE_CUBE_MAP_ARRAY_EXT texture target");
	}

	/* Delete texture object */
	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures() call failed");

		m_to_id = 0;
	}
}

} /* glcts */
