/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \file  gl4cSparseTextureTests.cpp
 * \brief Conformance tests for the GL_ARB_sparse_texture functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cSparseTextureTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string.h>
#include <vector>

using namespace glw;
using namespace glu;

namespace gl4cts
{

typedef std::pair<GLint, GLint> IntPair;

/** Verifies last query error and generate proper log message
 *
 * @param funcName         Verified function name
 * @param target           Target for which texture is binded
 * @param pname            Parameter name
 * @param error            Generated error code
 * @param expectedError    Expected error code
 *
 * @return Returns true if queried value is as expected, returns false otherwise
 */
bool SparseTextureUtils::verifyQueryError(std::stringstream& log, const char* funcName, GLint target, GLint pname,
										  GLint error, GLint expectedError)
{
	if (error != expectedError)
	{
		log << "QueryError [" << funcName << " return wrong error code"
			<< ", target: " << target << ", pname: " << pname << ", expected: " << expectedError
			<< ", returned: " << error << "] - ";

		return false;
	}

	return true;
}

/** Verifies last operation error and generate proper log message
 *
 * @param funcName         Verified function name
 * @param mesage           Error message
 * @param error            Generated error code
 * @param expectedError    Expected error code
 *
 * @return Returns true if queried value is as expected, returns false otherwise
 */
bool SparseTextureUtils::verifyError(std::stringstream& log, const char* funcName, GLint error, GLint expectedError)
{
	if (error != expectedError)
	{
		log << "Error [" << funcName << " return wrong error code "
			<< ", expectedError: " << expectedError << ", returnedError: " << error << "] - ";

		return false;
	}

	return true;
}

/** Get minimal depth value for target
 *
 * @param target   Texture target
 *
 * @return Returns depth value
 */
GLint SparseTextureUtils::getTargetDepth(GLint target)
{
	GLint depth;

	if (target == GL_TEXTURE_3D || target == GL_TEXTURE_1D_ARRAY || target == GL_TEXTURE_2D_ARRAY ||
		target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY || target == GL_TEXTURE_CUBE_MAP)
	{
		depth = 1;
	}
	else if (target == GL_TEXTURE_CUBE_MAP_ARRAY)
		depth = 6;
	else
		depth = 0;

	return depth;
}

/** Queries for virtual page sizes
 *
 * @param gl           GL functions
 * @param target       Texture target
 * @param format       Texture internal format
 * @param pageSizeX    Texture page size reference for X dimension
 * @param pageSizeY    Texture page size reference for X dimension
 * @param pageSizeZ    Texture page size reference for X dimension
 **/
void SparseTextureUtils::getTexturePageSizes(const glw::Functions& gl, glw::GLint target, glw::GLint format,
											 glw::GLint& pageSizeX, glw::GLint& pageSizeY, glw::GLint& pageSizeZ)
{
	gl.getInternalformativ(target, format, GL_VIRTUAL_PAGE_SIZE_X_ARB, sizeof(pageSizeX), &pageSizeX);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getInternalformativ error occurred for GL_VIRTUAL_PAGE_SIZE_X_ARB");

	gl.getInternalformativ(target, format, GL_VIRTUAL_PAGE_SIZE_Y_ARB, sizeof(pageSizeY), &pageSizeY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getInternalformativ error occurred for GL_VIRTUAL_PAGE_SIZE_Y_ARB");

	gl.getInternalformativ(target, format, GL_VIRTUAL_PAGE_SIZE_Z_ARB, sizeof(pageSizeZ), &pageSizeZ);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getInternalformativ error occurred for GL_VIRTUAL_PAGE_SIZE_Z_ARB");
}

/** Calculate texture size for specific mipmap
 *
 * @param target  GL functions
 * @param state   Texture current state
 * @param level   Texture mipmap level
 * @param width   Texture output width
 * @param height  Texture output height
 * @param depth   Texture output depth
 **/
void SparseTextureUtils::getTextureLevelSize(GLint target, TextureState& state, GLint level, GLint& width,
											 GLint& height, GLint& depth)
{
	width = state.width / (int)pow(2, level);
	if (target == GL_TEXTURE_1D || target == GL_TEXTURE_1D_ARRAY)
		height = 1;
	else
		height = state.height / (int)pow(2, level);

	if (target == GL_TEXTURE_3D)
		depth = state.depth / (int)pow(2, level);
	else if (target == GL_TEXTURE_1D_ARRAY || target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_CUBE_MAP_ARRAY)
		depth = state.depth;
	else
		depth = 1;
}

/* Texture static fields */
const GLuint Texture::m_invalid_id = -1;

/** Bind texture to target
 *
 * @param gl       GL API functions
 * @param id       Id of texture
 * @param tex_type Type of texture
 **/
void Texture::Bind(const Functions& gl, GLuint id, GLenum target)
{
	gl.bindTexture(target, id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");
}

/** Generate texture instance
 *
 * @param gl     GL functions
 * @param out_id Id of texture
 **/
void Texture::Generate(const Functions& gl, GLuint& out_id)
{
	GLuint id = m_invalid_id;

	gl.genTextures(1, &id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	if (m_invalid_id == id)
	{
		TCU_FAIL("Invalid id");
	}

	out_id = id;
}

/** Delete texture instance
 *
 * @param gl    GL functions
 * @param id    Id of texture
 **/
void Texture::Delete(const Functions& gl, GLuint& id)
{
	gl.deleteTextures(1, &id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");
}

/** Allocate storage for texture
 *
 * @param gl              GL functions
 * @param target          Texture target
 * @param levels          Number of levels
 * @param internal_format Internal format of texture
 * @param width           Width of texture
 * @param height          Height of texture
 * @param depth           Depth of texture
 **/
void Texture::Storage(const Functions& gl, GLenum target, GLsizei levels, GLenum internal_format, GLuint width,
					  GLuint height, GLuint depth)
{
	switch (target)
	{
	case GL_TEXTURE_1D:
		gl.texStorage1D(target, levels, internal_format, width);
		break;
	case GL_TEXTURE_1D_ARRAY:
		gl.texStorage2D(target, levels, internal_format, width, depth);
		break;
	case GL_TEXTURE_2D:
	case GL_TEXTURE_RECTANGLE:
	case GL_TEXTURE_CUBE_MAP:
		gl.texStorage2D(target, levels, internal_format, width, height);
		break;
	case GL_TEXTURE_3D:
	case GL_TEXTURE_2D_ARRAY:
	case GL_TEXTURE_CUBE_MAP_ARRAY:
		gl.texStorage3D(target, levels, internal_format, width, height, depth);
		break;
	case GL_TEXTURE_2D_MULTISAMPLE:
		gl.texStorage2DMultisample(target, levels /* samples */, internal_format, width, height, GL_TRUE);
		break;
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		gl.texStorage3DMultisample(target, levels /* samples */, internal_format, width, height, depth, GL_TRUE);
		break;
	default:
		TCU_FAIL("Invliad enum");
		break;
	}
}

/** Get texture data
 *
 * @param gl       GL functions
 * @param target   Texture target
 * @param format   Format of data
 * @param type     Type of data
 * @param out_data Buffer for data
 **/
void Texture::GetData(const glw::Functions& gl, glw::GLint level, glw::GLenum target, glw::GLenum format,
					  glw::GLenum type, glw::GLvoid* out_data)
{
	gl.getTexImage(target, level, format, type, out_data);
}

/** Set contents of texture
 *
 * @param gl              GL functions
 * @param target          Texture target
 * @param level           Mipmap level
 * @param x               X offset
 * @param y               Y offset
 * @param z               Z offset
 * @param width           Width of texture
 * @param height          Height of texture
 * @param depth           Depth of texture
 * @param format          Format of data
 * @param type            Type of data
 * @param pixels          Buffer with image data
 **/
void Texture::SubImage(const glw::Functions& gl, glw::GLenum target, glw::GLint level, glw::GLint x, glw::GLint y,
					   glw::GLint z, glw::GLsizei width, glw::GLsizei height, glw::GLsizei depth, glw::GLenum format,
					   glw::GLenum type, const glw::GLvoid* pixels)
{
	switch (target)
	{
	case GL_TEXTURE_1D:
		gl.texSubImage1D(target, level, x, width, format, type, pixels);
		break;
	case GL_TEXTURE_1D_ARRAY:
		gl.texSubImage2D(target, level, x, y, width, depth, format, type, pixels);
		break;
	case GL_TEXTURE_2D:
	case GL_TEXTURE_RECTANGLE:
		gl.texSubImage2D(target, level, x, y, width, height, format, type, pixels);
		break;
	case GL_TEXTURE_CUBE_MAP:
		gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, level, x, y, width, height, format, type, pixels);
		gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, level, x, y, width, height, format, type, pixels);
		gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, level, x, y, width, height, format, type, pixels);
		gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, level, x, y, width, height, format, type, pixels);
		gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, level, x, y, width, height, format, type, pixels);
		gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, level, x, y, width, height, format, type, pixels);
		break;
	case GL_TEXTURE_3D:
	case GL_TEXTURE_2D_ARRAY:
	case GL_TEXTURE_CUBE_MAP_ARRAY:
		gl.texSubImage3D(target, level, x, y, z, width, height, depth, format, type, pixels);
		break;
	default:
		TCU_FAIL("Invliad enum");
		break;
	}
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
TextureParameterQueriesTestCase::TextureParameterQueriesTestCase(deqp::Context& context)
	: TestCase(
		  context, "TextureParameterQueries",
		  "Implements all glTexParameter* and glGetTexParameter* queries tests described in CTS_ARB_sparse_texture")
{
	/* Left blank intentionally */
}

/** Stub init method */
void TextureParameterQueriesTestCase::init()
{
	mSupportedTargets.push_back(GL_TEXTURE_2D);
	mSupportedTargets.push_back(GL_TEXTURE_2D_ARRAY);
	mSupportedTargets.push_back(GL_TEXTURE_CUBE_MAP);
	mSupportedTargets.push_back(GL_TEXTURE_CUBE_MAP_ARRAY);
	mSupportedTargets.push_back(GL_TEXTURE_3D);
	mSupportedTargets.push_back(GL_TEXTURE_RECTANGLE);

	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_texture2"))
	{
		mNotSupportedTargets.push_back(GL_TEXTURE_1D);
		mNotSupportedTargets.push_back(GL_TEXTURE_1D_ARRAY);
		mNotSupportedTargets.push_back(GL_TEXTURE_2D_MULTISAMPLE);
		mNotSupportedTargets.push_back(GL_TEXTURE_2D_MULTISAMPLE_ARRAY);
	}
	else
	{
		mNotSupportedTargets.push_back(GL_TEXTURE_1D);
		mNotSupportedTargets.push_back(GL_TEXTURE_1D_ARRAY);
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult TextureParameterQueriesTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_texture"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool result = true;

	GLuint texture;

	//Iterate through supported targets

	for (std::vector<glw::GLint>::const_iterator iter = mSupportedTargets.begin(); iter != mSupportedTargets.end();
		 ++iter)
	{
		const GLint& target = *iter;

		mLog.str("");

		Texture::Generate(gl, texture);
		Texture::Bind(gl, texture, target);

		result = testTextureSparseARB(gl, target) && testVirtualPageSizeIndexARB(gl, target) &&
				 testNumSparseLevelsARB(gl, target);

		Texture::Delete(gl, texture);

		if (!result)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << mLog.str() << "Fail [positive tests]"
							   << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}
	}

	//Iterate through not supported targets
	for (std::vector<glw::GLint>::const_iterator iter = mNotSupportedTargets.begin();
		 iter != mNotSupportedTargets.end(); ++iter)
	{
		const GLint& target = *iter;

		mLog.str("");

		Texture::Generate(gl, texture);
		Texture::Bind(gl, texture, target);

		result = testTextureSparseARB(gl, target, GL_INVALID_VALUE);

		Texture::Delete(gl, texture);

		if (!result)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << mLog.str() << "Fail [positive tests]"
							   << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail on negative tests");
			return STOP;
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Testing texParameter* functions for binded texture and GL_TEXTURE_SPARSE_ARB parameter name
 *
 * @param gl               GL API functions
 * @param target           Target for which texture is binded
 * @param expectedError    Expected error code (default value GL_NO_ERROR)
 *
 * @return Returns true if queried value is as expected, returns false otherwise
 */
bool TextureParameterQueriesTestCase::testTextureSparseARB(const Functions& gl, GLint target, GLint expectedError)
{
	const GLint pname = GL_TEXTURE_SPARSE_ARB;

	bool result = true;

	GLint   testValueInt;
	GLuint  testValueUInt;
	GLfloat testValueFloat;

	mLog << "Testing TEXTURE_SPARSE_ARB for target: " << target << " - ";

	//Check getTexParameter* default value
	if (expectedError == GL_NO_ERROR)
		result = checkGetTexParameter(gl, target, pname, GL_FALSE);

	//Check getTexParameter* for manually set values
	if (result)
	{
		//Query to set parameter
		gl.texParameteri(target, pname, GL_TRUE);
		if (expectedError == GL_NO_ERROR)
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri error occurred.");
			result = checkGetTexParameter(gl, target, pname, GL_TRUE);

			//If no error verification reset TEXTURE_SPARSE_ARB value
			gl.texParameteri(target, pname, GL_FALSE);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri error occurred.");
		}
		else
			result = SparseTextureUtils::verifyQueryError(mLog, "glTexParameteri", target, pname, gl.getError(),
														  expectedError);
	}

	if (result)
	{
		gl.texParameterf(target, pname, GL_TRUE);
		if (expectedError == GL_NO_ERROR)
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameterf error occurred.");
			result = checkGetTexParameter(gl, target, pname, GL_TRUE);

			gl.texParameteri(target, pname, GL_FALSE);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri error occurred.");
		}
		else
			result = SparseTextureUtils::verifyQueryError(mLog, "glTexParameterf", target, pname, gl.getError(),
														  expectedError);
	}

	if (result)
	{
		testValueInt = GL_TRUE;
		gl.texParameteriv(target, pname, &testValueInt);
		if (expectedError == GL_NO_ERROR)
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteriv error occurred.");
			result = checkGetTexParameter(gl, target, pname, GL_TRUE);

			gl.texParameteri(target, pname, GL_FALSE);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri error occurred.");
		}
		else
			result = SparseTextureUtils::verifyQueryError(mLog, "glTexParameteriv", target, pname, gl.getError(),
														  expectedError);
	}

	if (result)
	{
		testValueFloat = (GLfloat)GL_TRUE;
		gl.texParameterfv(target, pname, &testValueFloat);
		if (expectedError == GL_NO_ERROR)
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameterfv error occurred.");
			result = checkGetTexParameter(gl, target, pname, GL_TRUE);

			gl.texParameteri(target, pname, GL_FALSE);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri error occurred.");
		}
		else
			result = SparseTextureUtils::verifyQueryError(mLog, "glTexParameterfv", target, pname, gl.getError(),
														  expectedError);
	}

	if (result)
	{
		testValueInt = GL_TRUE;
		gl.texParameterIiv(target, pname, &testValueInt);
		if (expectedError == GL_NO_ERROR)
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameterIiv error occurred.");
			result = checkGetTexParameter(gl, target, pname, GL_TRUE);

			gl.texParameteri(target, pname, GL_FALSE);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri error occurred.");
		}
		else
			result = SparseTextureUtils::verifyQueryError(mLog, "glTexParameterIiv", target, pname, gl.getError(),
														  expectedError);
	}

	if (result)
	{
		testValueUInt = GL_TRUE;
		gl.texParameterIuiv(target, pname, &testValueUInt);
		if (expectedError == GL_NO_ERROR)
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameterIuiv error occurred.");
			result = checkGetTexParameter(gl, target, pname, GL_TRUE);

			gl.texParameteri(target, pname, GL_FALSE);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri error occurred.");
		}
		else
			result = SparseTextureUtils::verifyQueryError(mLog, "glTexParameterIuiv", target, pname, gl.getError(),
														  expectedError);
	}

	return result;
}

/** Testing texParameter* functions for binded texture and GL_VIRTUAL_PAGE_SIZE_INDEX_ARB parameter name
 *
 * @param gl               GL API functions
 * @param target           Target for which texture is binded
 * @param expectedError    Expected error code (default value GL_NO_ERROR)
 *
 * @return Returns true if queried value is as expected, returns false otherwise
 */
bool TextureParameterQueriesTestCase::testVirtualPageSizeIndexARB(const Functions& gl, GLint target,
																  GLint expectedError)
{
	const GLint pname = GL_VIRTUAL_PAGE_SIZE_INDEX_ARB;

	bool result = true;

	GLint   testValueInt;
	GLuint  testValueUInt;
	GLfloat testValueFloat;

	mLog << "Testing VIRTUAL_PAGE_SIZE_INDEX_ARB for target: " << target << " - ";

	//Check getTexParameter* default value
	if (expectedError == GL_NO_ERROR)
		result = checkGetTexParameter(gl, target, pname, 0);

	//Check getTexParameter* for manually set values
	if (result)
	{
		gl.texParameteri(target, pname, 1);
		if (expectedError == GL_NO_ERROR)
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri error occurred");
			result = checkGetTexParameter(gl, target, pname, 1);

			//If no error verification reset TEXTURE_SPARSE_ARB value
			gl.texParameteri(target, pname, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri error occurred");
		}
		else
			result = SparseTextureUtils::verifyQueryError(mLog, "glTexParameteri", target, pname, gl.getError(),
														  expectedError);
	}

	if (result)
	{
		gl.texParameterf(target, pname, 2.0f);
		if (expectedError == GL_NO_ERROR)
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameterf error occurred");
			result = checkGetTexParameter(gl, target, pname, 2);

			gl.texParameteri(target, pname, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri error occurred");
		}
		else
			result = SparseTextureUtils::verifyQueryError(mLog, "glTexParameterf", target, pname, gl.getError(),
														  expectedError);
	}

	if (result)
	{
		testValueInt = 8;
		gl.texParameteriv(target, pname, &testValueInt);
		if (expectedError == GL_NO_ERROR)
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteriv error occurred");
			result = checkGetTexParameter(gl, target, pname, 8);

			gl.texParameteri(target, pname, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri error occurred");
		}
		else
			result = SparseTextureUtils::verifyQueryError(mLog, "glTexParameteriv", target, pname, gl.getError(),
														  expectedError);
	}

	if (result)
	{
		testValueFloat = 10.0f;
		gl.texParameterfv(target, pname, &testValueFloat);
		if (expectedError == GL_NO_ERROR)
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameterfv error occurred");
			result = checkGetTexParameter(gl, target, pname, 10);

			gl.texParameteri(target, pname, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri error occurred");
		}
		else
			result = SparseTextureUtils::verifyQueryError(mLog, "glTexParameterfv", target, pname, gl.getError(),
														  expectedError);
	}

	if (result)
	{
		testValueInt = 6;
		gl.texParameterIiv(target, pname, &testValueInt);
		if (expectedError == GL_NO_ERROR)
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameterIiv error occurred");
			result = checkGetTexParameter(gl, target, pname, 6);

			gl.texParameteri(target, pname, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri error occurred");
		}
		else
			result = SparseTextureUtils::verifyQueryError(mLog, "glTexParameterIiv", target, pname, gl.getError(),
														  expectedError);
	}

	if (result)
	{
		testValueUInt = 16;
		gl.texParameterIuiv(target, pname, &testValueUInt);
		if (expectedError == GL_NO_ERROR)
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameterIuiv error occurred");
			result = checkGetTexParameter(gl, target, pname, 16);

			gl.texParameteri(target, pname, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri error occurred");
		}
		else
			result = SparseTextureUtils::verifyQueryError(mLog, "glTexParameterIuiv", target, pname, gl.getError(),
														  expectedError);
	}

	return result;
}

/** Testing getTexParameter* functions for binded texture and GL_NUM_SPARSE_LEVELS_ARB parameter name
 *
 * @param gl               GL API functions
 * @param target           Target for which texture is binded
 * @param expectedError    Expected error code (default value GL_NO_ERROR)
 *
 * @return Returns true if no error code was generated, throws exception otherwise
 */
bool TextureParameterQueriesTestCase::testNumSparseLevelsARB(const Functions& gl, GLint target)
{
	const GLint pname = GL_NUM_SPARSE_LEVELS_ARB;

	bool result = true;

	GLint   value_int;
	GLuint  value_uint;
	GLfloat value_float;

	mLog << "Testing NUM_SPARSE_LEVELS_ARB for target: " << target << " - ";

	gl.getTexParameteriv(target, pname, &value_int);
	result = SparseTextureUtils::verifyError(mLog, "glGetTexParameteriv", gl.getError(), GL_NO_ERROR);

	if (result)
	{
		gl.getTexParameterfv(target, pname, &value_float);
		result = SparseTextureUtils::verifyError(mLog, "glGetTexParameterfv", gl.getError(), GL_NO_ERROR);

		if (result)
		{
			gl.getTexParameterIiv(target, pname, &value_int);
			result = SparseTextureUtils::verifyError(mLog, "glGetGexParameterIiv", gl.getError(), GL_NO_ERROR);

			if (result)
			{
				gl.getTexParameterIuiv(target, pname, &value_uint);
				result = SparseTextureUtils::verifyError(mLog, "getTexParameterIuiv", gl.getError(), GL_NO_ERROR);
			}
		}
	}

	return result;
}

/** Checking if getTexParameter* for binded texture returns value as expected
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param pname        Parameter name
 * @param expected     Expected value (int because function is designed to query only int and boolean parameters)
 *
 * @return Returns true if queried value is as expected, returns false otherwise
 */
bool TextureParameterQueriesTestCase::checkGetTexParameter(const Functions& gl, GLint target, GLint pname,
														   GLint expected)
{
	bool result = true;

	GLint   value_int;
	GLuint  value_uint;
	GLfloat value_float;

	mLog << "Testing GetTexParameter for target: " << target << " - ";

	gl.getTexParameteriv(target, pname, &value_int);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexParameteriv error occurred");
	if (value_int != expected)
	{
		mLog << "glGetTexParameteriv return wrong value"
			 << ", target: " << target << ", pname: " << pname << ", expected: " << expected
			 << ", returned: " << value_int << " - ";

		result = false;
	}

	gl.getTexParameterfv(target, pname, &value_float);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexParameterfv error occurred");
	if ((GLint)value_float != expected)
	{
		mLog << "glGetTexParameterfv return wrong value"
			 << ", target: " << target << ", pname: " << pname << ", expected: " << expected
			 << ", returned: " << (GLint)value_float << " - ";

		result = false;
	}

	gl.getTexParameterIiv(target, pname, &value_int);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetGexParameterIiv error occurred");
	if (value_int != expected)
	{
		mLog << "glGetGexParameterIiv return wrong value"
			 << ", target: " << target << ", pname: " << pname << ", expected: " << expected
			 << ", returned: " << value_int << " - ";

		result = false;
	}

	gl.getTexParameterIuiv(target, pname, &value_uint);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetGexParameterIui error occurred");
	if ((GLint)value_uint != expected)
	{
		mLog << "glGetGexParameterIui return wrong value"
			 << ", target: " << target << ", pname: " << pname << ", expected: " << expected
			 << ", returned: " << (GLint)value_uint << " - ";

		result = false;
	}

	return result;
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
InternalFormatQueriesTestCase::InternalFormatQueriesTestCase(deqp::Context& context)
	: TestCase(context, "InternalFormatQueries",
			   "Implements GetInternalformat query tests described in CTS_ARB_sparse_texture")
{
	/* Left blank intentionally */
}

/** Stub init method */
void InternalFormatQueriesTestCase::init()
{
	mSupportedTargets.push_back(GL_TEXTURE_1D);
	mSupportedTargets.push_back(GL_TEXTURE_1D_ARRAY);
	mSupportedTargets.push_back(GL_TEXTURE_2D);
	mSupportedTargets.push_back(GL_TEXTURE_2D_ARRAY);
	mSupportedTargets.push_back(GL_TEXTURE_3D);
	mSupportedTargets.push_back(GL_TEXTURE_CUBE_MAP);
	mSupportedTargets.push_back(GL_TEXTURE_CUBE_MAP_ARRAY);
	mSupportedTargets.push_back(GL_TEXTURE_RECTANGLE);
	mSupportedTargets.push_back(GL_TEXTURE_BUFFER);
	mSupportedTargets.push_back(GL_RENDERBUFFER);
	mSupportedTargets.push_back(GL_TEXTURE_2D_MULTISAMPLE);
	mSupportedTargets.push_back(GL_TEXTURE_2D_MULTISAMPLE_ARRAY);

	mSupportedInternalFormats.push_back(GL_R8);
	mSupportedInternalFormats.push_back(GL_R8_SNORM);
	mSupportedInternalFormats.push_back(GL_R16);
	mSupportedInternalFormats.push_back(GL_R16_SNORM);
	mSupportedInternalFormats.push_back(GL_RG8);
	mSupportedInternalFormats.push_back(GL_RG8_SNORM);
	mSupportedInternalFormats.push_back(GL_RG16);
	mSupportedInternalFormats.push_back(GL_RG16_SNORM);
	mSupportedInternalFormats.push_back(GL_RGB565);
	mSupportedInternalFormats.push_back(GL_RGBA8);
	mSupportedInternalFormats.push_back(GL_RGBA8_SNORM);
	mSupportedInternalFormats.push_back(GL_RGB10_A2);
	mSupportedInternalFormats.push_back(GL_RGB10_A2UI);
	mSupportedInternalFormats.push_back(GL_RGBA16);
	mSupportedInternalFormats.push_back(GL_RGBA16_SNORM);
	mSupportedInternalFormats.push_back(GL_R16F);
	mSupportedInternalFormats.push_back(GL_RG16F);
	mSupportedInternalFormats.push_back(GL_RGBA16F);
	mSupportedInternalFormats.push_back(GL_R32F);
	mSupportedInternalFormats.push_back(GL_RG32F);
	mSupportedInternalFormats.push_back(GL_RGBA32F);
	mSupportedInternalFormats.push_back(GL_R11F_G11F_B10F);
	mSupportedInternalFormats.push_back(GL_RGB9_E5);
	mSupportedInternalFormats.push_back(GL_R8I);
	mSupportedInternalFormats.push_back(GL_R8UI);
	mSupportedInternalFormats.push_back(GL_R16I);
	mSupportedInternalFormats.push_back(GL_R16UI);
	mSupportedInternalFormats.push_back(GL_R32I);
	mSupportedInternalFormats.push_back(GL_R32UI);
	mSupportedInternalFormats.push_back(GL_RG8I);
	mSupportedInternalFormats.push_back(GL_RG8UI);
	mSupportedInternalFormats.push_back(GL_RG16I);
	mSupportedInternalFormats.push_back(GL_RG16UI);
	mSupportedInternalFormats.push_back(GL_RG32I);
	mSupportedInternalFormats.push_back(GL_RG32UI);
	mSupportedInternalFormats.push_back(GL_RGBA8I);
	mSupportedInternalFormats.push_back(GL_RGBA8UI);
	mSupportedInternalFormats.push_back(GL_RGBA16I);
	mSupportedInternalFormats.push_back(GL_RGBA16UI);
	mSupportedInternalFormats.push_back(GL_RGBA32I);
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult InternalFormatQueriesTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_texture"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool result = true;

	mLog << "Testing getInternalformativ - ";

	for (std::vector<glw::GLint>::const_iterator iter = mSupportedTargets.begin(); iter != mSupportedTargets.end();
		 ++iter)
	{
		const GLint& target = *iter;

		for (std::vector<glw::GLint>::const_iterator formIter = mSupportedInternalFormats.begin();
			 formIter != mSupportedInternalFormats.end(); ++formIter)
		{
			const GLint& format = *formIter;
			GLint		 value;

			gl.getInternalformativ(target, format, GL_NUM_VIRTUAL_PAGE_SIZES_ARB, sizeof(value), &value);
			GLU_EXPECT_NO_ERROR(gl.getError(), "getInternalformativ error occurred for GL_NUM_VIRTUAL_PAGE_SIZES_ARB");
			if (value == 0)
			{
				mLog << "getInternalformativ for GL_NUM_VIRTUAL_PAGE_SIZES_ARB, target: " << target
					 << ", format: " << format << " returns wrong value: " << value << " - ";

				result = false;
			}

			if (result)
			{
				GLint pageSizeX;
				GLint pageSizeY;
				GLint pageSizeZ;
				SparseTextureUtils::getTexturePageSizes(gl, target, format, pageSizeX, pageSizeY, pageSizeZ);
			}
			else
			{
				m_testCtx.getLog() << tcu::TestLog::Message << mLog.str() << "Fail" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
				return STOP;
			}
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
SimpleQueriesTestCase::SimpleQueriesTestCase(deqp::Context& context)
	: TestCase(context, "SimpleQueries", "Implements Get* queries tests described in CTS_ARB_sparse_texture")
{
	/* Left blank intentionally */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SimpleQueriesTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_texture"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	const Functions& gl = m_context.getRenderContext().getFunctions();

	testSipmleQueries(gl, GL_MAX_SPARSE_TEXTURE_SIZE_ARB);
	testSipmleQueries(gl, GL_MAX_SPARSE_3D_TEXTURE_SIZE_ARB);
	testSipmleQueries(gl, GL_MAX_SPARSE_ARRAY_TEXTURE_LAYERS_ARB);
	testSipmleQueries(gl, GL_SPARSE_TEXTURE_FULL_ARRAY_CUBE_MIPMAPS_ARB);

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

void SimpleQueriesTestCase::testSipmleQueries(const Functions& gl, GLint pname)
{
	std::stringstream log;
	log << "Testing simple query for pname: " << pname << " - ";

	bool result = true;

	GLint	 value_int;
	GLint64   value_int64;
	GLfloat   value_float;
	GLdouble  value_double;
	GLboolean value_bool;

	gl.getIntegerv(pname, &value_int);
	result = SparseTextureUtils::verifyError(log, "getIntegerv", gl.getError(), GL_NO_ERROR);

	if (result)
	{
		gl.getInteger64v(pname, &value_int64);
		result = SparseTextureUtils::verifyError(log, "getInteger64v", gl.getError(), GL_NO_ERROR);

		if (result)
		{
			gl.getFloatv(pname, &value_float);
			result = SparseTextureUtils::verifyError(log, "getFloatv", gl.getError(), GL_NO_ERROR);

			if (result)
			{
				gl.getDoublev(pname, &value_double);
				result = SparseTextureUtils::verifyError(log, "getDoublev", gl.getError(), GL_NO_ERROR);

				if (result)
				{
					gl.getBooleanv(pname, &value_bool);
					result = SparseTextureUtils::verifyError(log, "getBooleanv", gl.getError(), GL_NO_ERROR);
				}
			}
		}
	}

	if (!result)
	{
		TCU_FAIL(log.str().c_str());
	}
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
SparseTextureAllocationTestCase::SparseTextureAllocationTestCase(deqp::Context& context)
	: TestCase(context, "SparseTextureAllocation", "Verifies TexStorage* functionality added in CTS_ARB_sparse_texture")
{
	/* Left blank intentionally */
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
SparseTextureAllocationTestCase::SparseTextureAllocationTestCase(deqp::Context& context, const char* name,
																 const char* description)
	: TestCase(context, name, description)
{
	/* Left blank intentionally */
}

/** Initializes the test group contents. */
void SparseTextureAllocationTestCase::init()
{
	mSupportedTargets.push_back(GL_TEXTURE_2D);
	mSupportedTargets.push_back(GL_TEXTURE_2D_ARRAY);
	mSupportedTargets.push_back(GL_TEXTURE_CUBE_MAP);
	mSupportedTargets.push_back(GL_TEXTURE_CUBE_MAP_ARRAY);
	mSupportedTargets.push_back(GL_TEXTURE_3D);
	mSupportedTargets.push_back(GL_TEXTURE_RECTANGLE);

	mFullArrayTargets.push_back(GL_TEXTURE_1D_ARRAY);
	mFullArrayTargets.push_back(GL_TEXTURE_2D_ARRAY);
	mFullArrayTargets.push_back(GL_TEXTURE_CUBE_MAP);
	mFullArrayTargets.push_back(GL_TEXTURE_CUBE_MAP_ARRAY);

	mSupportedInternalFormats.push_back(GL_R8);
	mSupportedInternalFormats.push_back(GL_R8_SNORM);
	mSupportedInternalFormats.push_back(GL_R16);
	mSupportedInternalFormats.push_back(GL_R16_SNORM);
	mSupportedInternalFormats.push_back(GL_RG8);
	mSupportedInternalFormats.push_back(GL_RG8_SNORM);
	mSupportedInternalFormats.push_back(GL_RG16);
	mSupportedInternalFormats.push_back(GL_RG16_SNORM);
	mSupportedInternalFormats.push_back(GL_RGB565);
	mSupportedInternalFormats.push_back(GL_RGBA8);
	mSupportedInternalFormats.push_back(GL_RGBA8_SNORM);
	mSupportedInternalFormats.push_back(GL_RGB10_A2);
	mSupportedInternalFormats.push_back(GL_RGB10_A2UI);
	mSupportedInternalFormats.push_back(GL_RGBA16);
	mSupportedInternalFormats.push_back(GL_RGBA16_SNORM);
	mSupportedInternalFormats.push_back(GL_R16F);
	mSupportedInternalFormats.push_back(GL_RG16F);
	mSupportedInternalFormats.push_back(GL_RGBA16F);
	mSupportedInternalFormats.push_back(GL_R32F);
	mSupportedInternalFormats.push_back(GL_RG32F);
	mSupportedInternalFormats.push_back(GL_RGBA32F);
	mSupportedInternalFormats.push_back(GL_R11F_G11F_B10F);
	mSupportedInternalFormats.push_back(GL_RGB9_E5);
	mSupportedInternalFormats.push_back(GL_R8I);
	mSupportedInternalFormats.push_back(GL_R8UI);
	mSupportedInternalFormats.push_back(GL_R16I);
	mSupportedInternalFormats.push_back(GL_R16UI);
	mSupportedInternalFormats.push_back(GL_R32I);
	mSupportedInternalFormats.push_back(GL_R32UI);
	mSupportedInternalFormats.push_back(GL_RG8I);
	mSupportedInternalFormats.push_back(GL_RG8UI);
	mSupportedInternalFormats.push_back(GL_RG16I);
	mSupportedInternalFormats.push_back(GL_RG16UI);
	mSupportedInternalFormats.push_back(GL_RG32I);
	mSupportedInternalFormats.push_back(GL_RG32UI);
	mSupportedInternalFormats.push_back(GL_RGBA8I);
	mSupportedInternalFormats.push_back(GL_RGBA8UI);
	mSupportedInternalFormats.push_back(GL_RGBA16I);
	mSupportedInternalFormats.push_back(GL_RGBA16UI);
	mSupportedInternalFormats.push_back(GL_RGBA32I);
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SparseTextureAllocationTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_texture"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool result = true;

	for (std::vector<glw::GLint>::const_iterator iter = mSupportedTargets.begin(); iter != mSupportedTargets.end();
		 ++iter)
	{
		const GLint& target = *iter;

		for (std::vector<glw::GLint>::const_iterator formIter = mSupportedInternalFormats.begin();
			 formIter != mSupportedInternalFormats.end(); ++formIter)
		{
			const GLint& format = *formIter;

			mLog.str("");
			mLog << "Testing sparse texture allocation for target: " << target << ", format: " << format << " - ";

			result = positiveTesting(gl, target, format) && verifyTexParameterErrors(gl, target, format) &&
					 verifyTexStorageVirtualPageSizeIndexError(gl, target, format) &&
					 verifyTexStorageFullArrayCubeMipmapsError(gl, target, format) &&
					 verifyTexStorageInvalidValueErrors(gl, target, format);

			if (!result)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << mLog.str() << "Fail" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
				return STOP;
			}
		}
	}

	for (std::vector<glw::GLint>::const_iterator iter = mFullArrayTargets.begin(); iter != mFullArrayTargets.end();
		 ++iter)
	{
		const GLint& target = *iter;

		for (std::vector<glw::GLint>::const_iterator formIter = mSupportedInternalFormats.begin();
			 formIter != mSupportedInternalFormats.end(); ++formIter)
		{
			const GLint& format = *formIter;

			mLog.str("");
			mLog << "Testing sparse texture allocation for target [full array]: " << target << ", format: " << format
				 << " - ";

			result = verifyTexStorageFullArrayCubeMipmapsError(gl, target, format) &&
					 verifyTexStorageInvalidValueErrors(gl, target, format);

			if (!result)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << mLog.str() << "Fail" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
				return STOP;
			}
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Testing if texStorage* functionality added in ARB_sparse_texture extension works properly for given target and internal format
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 *
 * @return Returns true if no errors occurred, false otherwise.
 **/
bool SparseTextureAllocationTestCase::positiveTesting(const Functions& gl, GLint target, GLint format)
{
	mLog << "Positive Testing - ";

	GLuint texture;

	Texture::Generate(gl, texture);
	Texture::Bind(gl, texture, target);

	GLint pageSizeX;
	GLint pageSizeY;
	GLint pageSizeZ;
	GLint depth = SparseTextureUtils::getTargetDepth(target);
	SparseTextureUtils::getTexturePageSizes(gl, target, format, pageSizeX, pageSizeY, pageSizeZ);

	gl.texParameteri(target, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
	if (!SparseTextureUtils::verifyError(mLog, "texParameteri", gl.getError(), GL_NO_ERROR))
	{
		Texture::Delete(gl, texture);
		return false;
	}

	//The <width> and <height> has to be equal for cube map textures
	if (target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_CUBE_MAP_ARRAY)
	{
		if (pageSizeX > pageSizeY)
			pageSizeY = pageSizeX;
		else if (pageSizeX < pageSizeY)
			pageSizeX = pageSizeY;
	}

	Texture::Storage(gl, target, 1, format, pageSizeX, pageSizeY, depth * pageSizeZ);
	if (!SparseTextureUtils::verifyError(mLog, "Texture::Storage", gl.getError(), GL_NO_ERROR))
	{
		Texture::Delete(gl, texture);
		return false;
	}

	Texture::Delete(gl, texture);
	return true;
}

/** Verifies if texParameter* generate proper errors for given target and internal format.
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 *
 * @return Returns true if errors are as expected, false otherwise.
 */
bool SparseTextureAllocationTestCase::verifyTexParameterErrors(const Functions& gl, GLint target, GLint format)
{
	mLog << "Verify TexParameter errors - ";

	bool result = true;

	GLuint texture;
	GLint  depth;

	Texture::Generate(gl, texture);
	Texture::Bind(gl, texture, target);

	depth = SparseTextureUtils::getTargetDepth(target);

	Texture::Storage(gl, target, 1, format, 8, 8, depth);
	if (!SparseTextureUtils::verifyError(mLog, "TexStorage", gl.getError(), GL_NO_ERROR))
	{
		Texture::Delete(gl, texture);
		return false;
	}

	GLint immutableFormat;

	gl.getTexParameteriv(target, GL_TEXTURE_IMMUTABLE_FORMAT, &immutableFormat);
	if (!SparseTextureUtils::verifyQueryError(mLog, "getTexParameteriv", target, GL_TEXTURE_IMMUTABLE_FORMAT,
											  gl.getError(), GL_NO_ERROR))
	{
		Texture::Delete(gl, texture);
		return false;
	}

	// Test error only if texture is immutable format, otherwise skip
	if (immutableFormat == GL_TRUE)
	{
		std::vector<IntPair> params;
		params.push_back(IntPair(GL_TEXTURE_SPARSE_ARB, GL_TRUE));
		params.push_back(IntPair(GL_VIRTUAL_PAGE_SIZE_INDEX_ARB, 1));

		for (std::vector<IntPair>::const_iterator iter = params.begin(); iter != params.end(); ++iter)
		{
			const IntPair& param = *iter;

			if (result)
			{
				gl.texParameteri(target, param.first, param.second);
				result = SparseTextureUtils::verifyQueryError(mLog, "glTexParameteri", target, param.first,
															  gl.getError(), GL_INVALID_OPERATION);
			}

			if (result)
			{
				gl.texParameterf(target, param.first, (GLfloat)param.second);
				result = SparseTextureUtils::verifyQueryError(mLog, "glTexParameterf", target, param.first,
															  gl.getError(), GL_INVALID_OPERATION);
			}

			if (result)
			{
				GLint value = param.second;
				gl.texParameteriv(target, param.first, &value);
				result = SparseTextureUtils::verifyQueryError(mLog, "glTexParameteriv", target, param.first,
															  gl.getError(), GL_INVALID_OPERATION);
			}

			if (result)
			{
				GLfloat value = (GLfloat)param.second;
				gl.texParameterfv(target, param.first, &value);
				result = SparseTextureUtils::verifyQueryError(mLog, "glTexParameterfv", target, param.first,
															  gl.getError(), GL_INVALID_OPERATION);
			}

			if (result)
			{
				GLint value = param.second;
				gl.texParameterIiv(target, param.first, &value);
				result = SparseTextureUtils::verifyQueryError(mLog, "glTexParameterIiv", target, param.first,
															  gl.getError(), GL_INVALID_OPERATION);
			}

			if (result)
			{
				GLuint value = param.second;
				gl.texParameterIuiv(target, param.first, &value);
				result = SparseTextureUtils::verifyQueryError(mLog, "glTexParameterIuiv", target, param.first,
															  gl.getError(), GL_INVALID_OPERATION);
			}
		}
	}

	Texture::Delete(gl, texture);
	return result;
}

/** Verifies if texStorage* generate proper error for given target and internal format when
 *  VIRTUAL_PAGE_SIZE_INDEX_ARB value is greater than NUM_VIRTUAL_PAGE_SIZES_ARB.
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 *
 * @return Returns true if errors are as expected, false otherwise.
 */
bool SparseTextureAllocationTestCase::verifyTexStorageVirtualPageSizeIndexError(const Functions& gl, GLint target,
																				GLint format)
{
	mLog << "Verify VirtualPageSizeIndex errors - ";

	GLuint texture;
	GLint  depth;
	GLint  numPageSizes;

	Texture::Generate(gl, texture);
	Texture::Bind(gl, texture, target);

	gl.texParameteri(target, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
	if (!SparseTextureUtils::verifyQueryError(mLog, "texParameteri", target, GL_TEXTURE_SPARSE_ARB, gl.getError(),
											  GL_NO_ERROR))
	{
		Texture::Delete(gl, texture);
		return false;
	}

	gl.getInternalformativ(target, format, GL_NUM_VIRTUAL_PAGE_SIZES_ARB, sizeof(numPageSizes), &numPageSizes);
	if (!SparseTextureUtils::verifyQueryError(mLog, "getInternalformativ", target, GL_NUM_VIRTUAL_PAGE_SIZES_ARB,
											  gl.getError(), GL_NO_ERROR))
	{
		Texture::Delete(gl, texture);
		return false;
	}

	numPageSizes += 1;
	gl.texParameteri(target, GL_VIRTUAL_PAGE_SIZE_INDEX_ARB, numPageSizes);
	if (!SparseTextureUtils::verifyQueryError(mLog, "texParameteri", target, GL_VIRTUAL_PAGE_SIZE_INDEX_ARB,
											  gl.getError(), GL_NO_ERROR))
	{
		Texture::Delete(gl, texture);
		return false;
	}

	depth = SparseTextureUtils::getTargetDepth(target);

	Texture::Storage(gl, target, 1, format, 8, 8, depth);
	if (!SparseTextureUtils::verifyError(mLog, "TexStorage", gl.getError(), GL_INVALID_OPERATION))
	{
		Texture::Delete(gl, texture);
		return false;
	}

	Texture::Delete(gl, texture);
	return true;
}

/** Verifies if texStorage* generate proper errors for given target and internal format and
 *  SPARSE_TEXTURE_FULL_ARRAY_CUBE_MIPMAPS_ARB value set to FALSE.
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 *
 * @return Returns true if errors are as expected, false otherwise.
 */
bool SparseTextureAllocationTestCase::verifyTexStorageFullArrayCubeMipmapsError(const Functions& gl, GLint target,
																				GLint format)
{
	mLog << "Verify FullArrayCubeMipmaps errors - ";

	bool result = true;

	GLuint texture;
	GLint  depth;

	depth = SparseTextureUtils::getTargetDepth(target);

	GLboolean fullArrayCubeMipmaps;

	gl.getBooleanv(GL_SPARSE_TEXTURE_FULL_ARRAY_CUBE_MIPMAPS_ARB, &fullArrayCubeMipmaps);
	if (!SparseTextureUtils::verifyQueryError(
			mLog, "getBooleanv", target, GL_SPARSE_TEXTURE_FULL_ARRAY_CUBE_MIPMAPS_ARB, gl.getError(), GL_NO_ERROR))
		return false;

	if (fullArrayCubeMipmaps == GL_FALSE)
	{
		if (target != GL_TEXTURE_1D_ARRAY && target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_CUBE_MAP &&
			target != GL_TEXTURE_CUBE_MAP_ARRAY)
		{
			// Case 1: test GL_TEXTURE_SPARSE_ARB
			Texture::Generate(gl, texture);
			Texture::Bind(gl, texture, target);

			gl.texParameteri(target, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
			if (!SparseTextureUtils::verifyQueryError(mLog, "texParameteri", target, GL_TEXTURE_SPARSE_ARB,
													  gl.getError(), GL_NO_ERROR))
				return false;

			Texture::Storage(gl, target, 1, format, 8, 8, depth);
			if (!SparseTextureUtils::verifyError(mLog, "TexStorage [sparse texture]", gl.getError(),
												 GL_INVALID_OPERATION))
			{
				Texture::Delete(gl, texture);
				return false;
			}

			// Case 2: test wrong texture size
			Texture::Generate(gl, texture);
			Texture::Bind(gl, texture, target);

			GLint pageSizeX;
			GLint pageSizeY;
			GLint pageSizeZ;
			SparseTextureUtils::getTexturePageSizes(gl, target, format, pageSizeX, pageSizeY, pageSizeZ);

			GLint levels = 4;
			GLint width  = pageSizeX * (int)pow(2, levels - 1);
			GLint height = pageSizeY * (int)pow(2, levels - 1);

			// Check 2 different cases:
			// 1) wrong width
			// 2) wrong height
			Texture::Storage(gl, target, levels, format, width + pageSizeX, height, depth);
			result =
				SparseTextureUtils::verifyError(mLog, "TexStorage [wrong width]", gl.getError(), GL_INVALID_OPERATION);

			if (result)
			{
				Texture::Storage(gl, target, levels, format, width, height + pageSizeY, depth);
				result = SparseTextureUtils::verifyError(mLog, "TexStorage [wrong height]", gl.getError(),
														 GL_INVALID_OPERATION);
			}

			Texture::Delete(gl, texture);
		}
		else
		{
			// Case 3: test full array mipmaps targets
			Texture::Generate(gl, texture);
			Texture::Bind(gl, texture, target);

			if (target == GL_TEXTURE_1D_ARRAY)
				Texture::Storage(gl, target, 1, format, 8, depth, 0);
			else
				Texture::Storage(gl, target, 1, format, 8, 8, depth);

			result = SparseTextureUtils::verifyError(mLog, "TexStorage [case 3]", gl.getError(), GL_INVALID_OPERATION);

			Texture::Delete(gl, texture);
		}
	}

	return result;
}

/** Verifies if texStorage* generate proper errors for given target and internal format when
 *  texture size are set greater than allowed.
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 *
 * @return Returns true if errors are as expected, false otherwise.
 */
bool SparseTextureAllocationTestCase::verifyTexStorageInvalidValueErrors(const Functions& gl, GLint target,
																		 GLint format)
{
	mLog << "Verify Invalid Value errors - ";

	GLuint texture;
	GLint  pageSizeX;
	GLint  pageSizeY;
	GLint  pageSizeZ;
	SparseTextureUtils::getTexturePageSizes(gl, target, format, pageSizeX, pageSizeY, pageSizeZ);

	Texture::Generate(gl, texture);
	Texture::Bind(gl, texture, target);

	GLint width  = pageSizeX;
	GLint height = pageSizeY;
	GLint depth  = SparseTextureUtils::getTargetDepth(target) * pageSizeZ;

	if (target == GL_TEXTURE_3D)
	{
		GLint max3DTextureSize;

		gl.getIntegerv(GL_MAX_SPARSE_3D_TEXTURE_SIZE_ARB, &max3DTextureSize);
		if (!SparseTextureUtils::verifyQueryError(mLog, "getIntegerv", target, GL_MAX_SPARSE_3D_TEXTURE_SIZE_ARB,
												  gl.getError(), GL_NO_ERROR))
		{
			Texture::Delete(gl, texture);
			return false;
		}

		// Check 3 different cases:
		// 1) wrong width
		// 2) wrong height
		// 3) wrong depth
		Texture::Storage(gl, target, 1, format, width + max3DTextureSize, height, depth);
		if (!SparseTextureUtils::verifyError(mLog, "TexStorage [GL_TEXTURE_3D wrong width]", gl.getError(),
											 GL_INVALID_VALUE))
		{
			Texture::Delete(gl, texture);
			return false;
		}

		Texture::Storage(gl, target, 1, format, width, height + max3DTextureSize, depth);
		if (!SparseTextureUtils::verifyError(mLog, "TexStorage [GL_TEXTURE_3D wrong height]", gl.getError(),
											 GL_INVALID_VALUE))
		{
			Texture::Delete(gl, texture);
			return false;
		}

		Texture::Storage(gl, target, 1, format, width, height, depth + max3DTextureSize);
		if (!SparseTextureUtils::verifyError(mLog, "TexStorage [GL_TEXTURE_3D wrong depth]", gl.getError(),
											 GL_INVALID_VALUE))
		{
			Texture::Delete(gl, texture);
			return false;
		}
	}
	else
	{
		GLint maxTextureSize;

		gl.getIntegerv(GL_MAX_SPARSE_TEXTURE_SIZE_ARB, &maxTextureSize);
		if (!SparseTextureUtils::verifyQueryError(mLog, "getIntegerv", target, GL_MAX_SPARSE_TEXTURE_SIZE_ARB,
												  gl.getError(), GL_NO_ERROR))
		{
			Texture::Delete(gl, texture);
			return false;
		}

		// Check 3 different cases:
		// 1) wrong width
		// 2) wrong height
		Texture::Storage(gl, target, 1, format, width + maxTextureSize, height, depth);
		if (!SparseTextureUtils::verifyError(mLog, "TexStorage [!GL_TEXTURE_3D wrong width]", gl.getError(),
											 GL_INVALID_VALUE))
		{
			Texture::Delete(gl, texture);
			return false;
		}

		if (target != GL_TEXTURE_1D_ARRAY)
		{
			Texture::Storage(gl, target, 1, format, width, height + maxTextureSize, depth);
			if (!SparseTextureUtils::verifyError(mLog, "TexStorage [!GL_TEXTURE_3D wrong height]", gl.getError(),
												 GL_INVALID_VALUE))
			{
				Texture::Delete(gl, texture);
				return false;
			}
		}

		GLint maxArrayTextureLayers;

		gl.getIntegerv(GL_MAX_SPARSE_ARRAY_TEXTURE_LAYERS_ARB, &maxArrayTextureLayers);
		if (!SparseTextureUtils::verifyQueryError(mLog, "getIntegerv", target, GL_MAX_SPARSE_ARRAY_TEXTURE_LAYERS_ARB,
												  gl.getError(), GL_NO_ERROR))
		{
			Texture::Delete(gl, texture);
			return false;
		}

		if (target == GL_TEXTURE_1D_ARRAY || target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_CUBE_MAP_ARRAY)
		{
			Texture::Storage(gl, target, 1, format, width, height, depth + maxArrayTextureLayers);
			if (!SparseTextureUtils::verifyError(mLog, "TexStorage [ARRAY wrong depth]", gl.getError(),
												 GL_INVALID_VALUE))
			{
				Texture::Delete(gl, texture);
				return false;
			}
		}
	}

	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_texture2"))
	{
		if (pageSizeX > 1)
		{
			Texture::Storage(gl, target, 1, format, 1, height, depth);
			if (!SparseTextureUtils::verifyError(mLog, "TexStorage [wrong width]", gl.getError(), GL_INVALID_VALUE))
			{
				Texture::Delete(gl, texture);
				return false;
			}
		}

		if (pageSizeY > 1)
		{
			Texture::Storage(gl, target, 1, format, width, 1, depth);
			if (!SparseTextureUtils::verifyError(mLog, "TexStorage [wrong height]", gl.getError(), GL_INVALID_VALUE))
			{
				Texture::Delete(gl, texture);
				return false;
			}
		}

		if (pageSizeZ > 1)
		{
			Texture::Storage(gl, target, 1, format, width, height, SparseTextureUtils::getTargetDepth(target));
			if (!SparseTextureUtils::verifyError(mLog, "TexStorage [wrong depth]", gl.getError(), GL_INVALID_VALUE))
			{
				Texture::Delete(gl, texture);
				return false;
			}
		}
	}

	Texture::Delete(gl, texture);
	return true;
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
SparseTextureCommitmentTestCase::SparseTextureCommitmentTestCase(deqp::Context& context)
	: TestCase(context, "SparseTextureCommitment",
			   "Verifies TexPageCommitmentARB functionality added in CTS_ARB_sparse_texture")
	, mState()
{
	/* Left blank intentionally */
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
SparseTextureCommitmentTestCase::SparseTextureCommitmentTestCase(deqp::Context& context, const char* name,
																 const char* description)
	: TestCase(context, name, description), mState()
{
	/* Left blank intentionally */
}

/** Initializes the test case. */
void SparseTextureCommitmentTestCase::init()
{
	mSupportedTargets.push_back(GL_TEXTURE_2D);
	mSupportedTargets.push_back(GL_TEXTURE_2D_ARRAY);
	mSupportedTargets.push_back(GL_TEXTURE_CUBE_MAP);
	mSupportedTargets.push_back(GL_TEXTURE_CUBE_MAP_ARRAY);
	mSupportedTargets.push_back(GL_TEXTURE_3D);
	mSupportedTargets.push_back(GL_TEXTURE_RECTANGLE);

	mSupportedInternalFormats.push_back(GL_R8);
	mSupportedInternalFormats.push_back(GL_R8_SNORM);
	mSupportedInternalFormats.push_back(GL_R16);
	mSupportedInternalFormats.push_back(GL_R16_SNORM);
	mSupportedInternalFormats.push_back(GL_RG8);
	mSupportedInternalFormats.push_back(GL_RG8_SNORM);
	mSupportedInternalFormats.push_back(GL_RG16);
	mSupportedInternalFormats.push_back(GL_RG16_SNORM);
	mSupportedInternalFormats.push_back(GL_RGB565);
	mSupportedInternalFormats.push_back(GL_RGBA8);
	mSupportedInternalFormats.push_back(GL_RGBA8_SNORM);
	mSupportedInternalFormats.push_back(GL_RGB10_A2);
	mSupportedInternalFormats.push_back(GL_RGB10_A2UI);
	mSupportedInternalFormats.push_back(GL_RGBA16);
	mSupportedInternalFormats.push_back(GL_RGBA16_SNORM);
	mSupportedInternalFormats.push_back(GL_R16F);
	mSupportedInternalFormats.push_back(GL_RG16F);
	mSupportedInternalFormats.push_back(GL_RGBA16F);
	mSupportedInternalFormats.push_back(GL_R32F);
	mSupportedInternalFormats.push_back(GL_RG32F);
	mSupportedInternalFormats.push_back(GL_RGBA32F);
	mSupportedInternalFormats.push_back(GL_R11F_G11F_B10F);
	mSupportedInternalFormats.push_back(GL_RGB9_E5);
	mSupportedInternalFormats.push_back(GL_R8I);
	mSupportedInternalFormats.push_back(GL_R8UI);
	mSupportedInternalFormats.push_back(GL_R16I);
	mSupportedInternalFormats.push_back(GL_R16UI);
	mSupportedInternalFormats.push_back(GL_R32I);
	mSupportedInternalFormats.push_back(GL_R32UI);
	mSupportedInternalFormats.push_back(GL_RG8I);
	mSupportedInternalFormats.push_back(GL_RG8UI);
	mSupportedInternalFormats.push_back(GL_RG16I);
	mSupportedInternalFormats.push_back(GL_RG16UI);
	mSupportedInternalFormats.push_back(GL_RG32I);
	mSupportedInternalFormats.push_back(GL_RG32UI);
	mSupportedInternalFormats.push_back(GL_RGBA8I);
	mSupportedInternalFormats.push_back(GL_RGBA8UI);
	mSupportedInternalFormats.push_back(GL_RGBA16I);
	mSupportedInternalFormats.push_back(GL_RGBA16UI);
	mSupportedInternalFormats.push_back(GL_RGBA32I);
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SparseTextureCommitmentTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_texture"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool result = true;

	GLuint texture;

	for (std::vector<glw::GLint>::const_iterator iter = mSupportedTargets.begin(); iter != mSupportedTargets.end();
		 ++iter)
	{
		const GLint& target = *iter;

		for (std::vector<glw::GLint>::const_iterator formIter = mSupportedInternalFormats.begin();
			 formIter != mSupportedInternalFormats.end(); ++formIter)
		{
			const GLint& format = *formIter;

			if (!caseAllowed(target, format))
				continue;

			mLog.str("");
			mLog << "Testing sparse texture commitment for target: " << target << ", format: " << format << " - ";

			//Checking if written data into not committed region generates no error
			sparseAllocateTexture(gl, target, format, texture, 3);
			for (int l = 0; l < mState.levels; ++l)
				writeDataToTexture(gl, target, format, texture, l);

			//Checking if written data into committed region is as expected
			for (int l = 0; l < mState.levels; ++l)
			{
				if (commitTexturePage(gl, target, format, texture, l))
				{
					writeDataToTexture(gl, target, format, texture, l);
					result = verifyTextureData(gl, target, format, texture, l);
				}

				if (!result)
					break;
			}

			Texture::Delete(gl, texture);

			//verify errors
			result = result && verifyInvalidOperationErrors(gl, target, format, texture);
			result = result && verifyInvalidValueErrors(gl, target, format, texture);

			if (!result)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << mLog.str() << "Fail" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
				return STOP;
			}
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Bind texPageCommitmentARB function
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 * @param xOffset      Texture commitment x offset
 * @param yOffset      Texture commitment y offset
 * @param zOffset      Texture commitment z offset
 * @param width        Texture commitment width
 * @param height       Texture commitment height
 * @param depth        Texture commitment depth
 * @param commit       Commit or de-commit indicator
 **/
void SparseTextureCommitmentTestCase::texPageCommitment(const glw::Functions& gl, glw::GLint target, glw::GLint format,
														glw::GLuint& texture, GLint level, GLint xOffset, GLint yOffset,
														GLint zOffset, GLint width, GLint height, GLint depth,
														GLboolean commit)
{
	DE_UNREF(format);
	Texture::Bind(gl, texture, target);

	gl.texPageCommitmentARB(target, level, xOffset, yOffset, zOffset, width, height, depth, commit);
}

/** Check if specific combination of target and format is allowed
 *
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 *
 * @return Returns true if target/format combination is allowed, false otherwise.
 */
bool SparseTextureCommitmentTestCase::caseAllowed(GLint target, GLint format)
{
	DE_UNREF(target);
	DE_UNREF(format);
	return true;
}

/** Preparing texture
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 *
 * @return Returns true if no error occurred, otherwise throws an exception.
 */
bool SparseTextureCommitmentTestCase::prepareTexture(const Functions& gl, GLint target, GLint format, GLuint& texture)
{
	Texture::Generate(gl, texture);
	Texture::Bind(gl, texture, target);

	mState.minDepth = SparseTextureUtils::getTargetDepth(target);
	SparseTextureUtils::getTexturePageSizes(gl, target, format, mState.pageSizeX, mState.pageSizeY, mState.pageSizeZ);

	//The <width> and <height> has to be equal for cube map textures
	if (target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_CUBE_MAP_ARRAY)
	{
		if (mState.pageSizeX > mState.pageSizeY)
			mState.pageSizeY = mState.pageSizeX;
		else if (mState.pageSizeX < mState.pageSizeY)
			mState.pageSizeX = mState.pageSizeY;
	}

	mState.width  = 2 * mState.pageSizeX;
	mState.height = 2 * mState.pageSizeY;
	mState.depth  = 2 * mState.pageSizeZ * mState.minDepth;

	mState.format = glu::mapGLInternalFormat(format);

	return true;
}

/** Allocating sparse texture memory using texStorage* function
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 * @param levels       Texture mipmaps level
 *
 * @return Returns true if no error occurred, otherwise throws an exception.
 */
bool SparseTextureCommitmentTestCase::sparseAllocateTexture(const Functions& gl, GLint target, GLint format,
															GLuint& texture, GLint levels)
{
	mLog << "Sparse Allocate [levels: " << levels << "] - ";

	prepareTexture(gl, target, format, texture);

	gl.texParameteri(target, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texParameteri error occurred for GL_TEXTURE_SPARSE_ARB");

	// GL_TEXTURE_RECTANGLE can have only one level
	if (target != GL_TEXTURE_RECTANGLE)
	{
		gl.getTexParameteriv(target, GL_NUM_SPARSE_LEVELS_ARB, &mState.levels);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexParameteriv");

		mState.levels = deMin32(mState.levels, levels);
	}
	else
		mState.levels = 1;

	Texture::Storage(gl, target, mState.levels, format, mState.width, mState.height, mState.depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage");

	return true;
}

/** Allocating texture memory using texStorage* function
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 * @param levels       Texture mipmaps level
 *
 * @return Returns true if no error occurred, otherwise throws an exception.
 */
bool SparseTextureCommitmentTestCase::allocateTexture(const Functions& gl, GLint target, GLint format, GLuint& texture,
													  GLint levels)
{
	mLog << "Allocate [levels: " << levels << "] - ";

	prepareTexture(gl, target, format, texture);

	//GL_TEXTURE_RECTANGLE can have only one level
	if (target != GL_TEXTURE_RECTANGLE)
		mState.levels = levels;
	else
		mState.levels = 1;

	Texture::Storage(gl, target, mState.levels, format, mState.width, mState.height, mState.depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage");

	return true;
}

/** Writing data to generated texture
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 *
 * @return Returns true if no error occurred, otherwise throws an exception.
 */
bool SparseTextureCommitmentTestCase::writeDataToTexture(const Functions& gl, GLint target, GLint format,
														 GLuint& texture, GLint level)
{
	DE_UNREF(format);
	DE_UNREF(texture);

	mLog << "Fill texture [level: " << level << "] - ";

	if (level > mState.levels - 1)
		TCU_FAIL("Invalid level");

	TransferFormat transferFormat = glu::getTransferFormat(mState.format);

	GLint width;
	GLint height;
	GLint depth;
	SparseTextureUtils::getTextureLevelSize(target, mState, level, width, height, depth);

	if (width > 0 && height > 0 && depth >= mState.minDepth)
	{
		GLint texSize = width * height * depth * mState.format.getPixelSize();

		std::vector<GLubyte> vecData;
		vecData.resize(texSize);
		GLubyte* data = vecData.data();

		deMemset(data, 16 + 16 * level, texSize);

		Texture::SubImage(gl, target, level, 0, 0, 0, width, height, depth, transferFormat.format,
						  transferFormat.dataType, (GLvoid*)data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "SubImage");
	}

	return true;
}

/** Verify if data stored in texture is as expected
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 * @param level        Texture mipmap level
 *
 * @return Returns true if data is as expected, false if not, throws an exception if error occurred.
 */
bool SparseTextureCommitmentTestCase::verifyTextureData(const Functions& gl, GLint target, GLint format,
														GLuint& texture, GLint level)
{
	DE_UNREF(format);
	DE_UNREF(texture);

	mLog << "Verify Texture [level: " << level << "] - ";

	if (level > mState.levels - 1)
		TCU_FAIL("Invalid level");

	TransferFormat transferFormat = glu::getTransferFormat(mState.format);

	GLint width;
	GLint height;
	GLint depth;
	SparseTextureUtils::getTextureLevelSize(target, mState, level, width, height, depth);

	//Committed region is limited to 1/2 of width
	GLint widthCommitted = width / 2;

	if (widthCommitted == 0 || height == 0 || depth < mState.minDepth)
		return true;

	bool result = true;

	if (target != GL_TEXTURE_CUBE_MAP)
	{
		GLint texSize = width * height * depth * mState.format.getPixelSize();

		std::vector<GLubyte> vecExpData;
		std::vector<GLubyte> vecOutData;
		vecExpData.resize(texSize);
		vecOutData.resize(texSize);
		GLubyte* exp_data = vecExpData.data();
		GLubyte* out_data = vecOutData.data();

		deMemset(exp_data, 16 + 16 * level, texSize);
		deMemset(out_data, 255, texSize);

		Texture::GetData(gl, level, target, transferFormat.format, transferFormat.dataType, (GLvoid*)out_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::GetData");

		//Verify only committed region
		for (GLint x = 0; x < widthCommitted; ++x)
			for (GLint y = 0; y < height; ++y)
				for (GLint z = 0; z < depth; ++z)
				{
					int		 pixelSize	 = mState.format.getPixelSize();
					GLubyte* dataRegion	= exp_data + ((x + y * width) * pixelSize);
					GLubyte* outDataRegion = out_data + ((x + y * width) * pixelSize);
					if (deMemCmp(dataRegion, outDataRegion, pixelSize) != 0)
						result = false;
				}
	}
	else
	{
		std::vector<GLint> subTargets;

		subTargets.push_back(GL_TEXTURE_CUBE_MAP_POSITIVE_X);
		subTargets.push_back(GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
		subTargets.push_back(GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
		subTargets.push_back(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
		subTargets.push_back(GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
		subTargets.push_back(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);

		GLint texSize = width * height * mState.format.getPixelSize();

		std::vector<GLubyte> vecExpData;
		std::vector<GLubyte> vecOutData;
		vecExpData.resize(texSize);
		vecOutData.resize(texSize);
		GLubyte* exp_data = vecExpData.data();
		GLubyte* out_data = vecOutData.data();

		deMemset(exp_data, 16 + 16 * level, texSize);
		deMemset(out_data, 255, texSize);

		for (size_t i = 0; i < subTargets.size(); ++i)
		{
			GLint subTarget = subTargets[i];

			mLog << "Verify Subtarget [subtarget: " << subTarget << "] - ";

			deMemset(out_data, 255, texSize);

			Texture::GetData(gl, level, subTarget, transferFormat.format, transferFormat.dataType, (GLvoid*)out_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::GetData");

			//Verify only committed region
			for (GLint x = 0; x < widthCommitted; ++x)
				for (GLint y = 0; y < height; ++y)
					for (GLint z = 0; z < depth; ++z)
					{
						int		 pixelSize	 = mState.format.getPixelSize();
						GLubyte* dataRegion	= exp_data + ((x + y * width) * pixelSize);
						GLubyte* outDataRegion = out_data + ((x + y * width) * pixelSize);
						if (deMemCmp(dataRegion, outDataRegion, pixelSize) != 0)
							result = false;
					}

			if (!result)
				break;
		}
	}

	return result;
}

/** Commit texture page using texPageCommitment function
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 * @param level        Texture mipmap level
 *
 * @return Returns true if commitment is done properly, false if commitment is not allowed or throws exception if error occurred.
 */
bool SparseTextureCommitmentTestCase::commitTexturePage(const Functions& gl, GLint target, GLint format,
														GLuint& texture, GLint level)
{
	mLog << "Commit Region [level: " << level << "] - ";

	if (level > mState.levels - 1)
		TCU_FAIL("Invalid level");

	// Avoid not allowed commitments
	if (!isInPageSizesRange(target, level) || !isPageSizesMultiplication(target, level))
	{
		mLog << "Skip commitment [level: " << level << "] - ";
		return false;
	}

	GLint width;
	GLint height;
	GLint depth;
	SparseTextureUtils::getTextureLevelSize(target, mState, level, width, height, depth);

	if (target == GL_TEXTURE_CUBE_MAP)
		depth = 6 * depth;

	GLint widthCommitted = width / 2;

	Texture::Bind(gl, texture, target);
	texPageCommitment(gl, target, format, texture, level, 0, 0, 0, widthCommitted, height, depth, GL_TRUE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texPageCommitment");

	return true;
}

/** Check if current texture size for level is greater or equal page size in a corresponding direction
 *
 * @param target  Target for which texture is binded
 * @param level   Texture mipmap level
 *
 * @return Returns true if the texture size condition is fulfilled, false otherwise.
 */
bool SparseTextureCommitmentTestCase::isInPageSizesRange(GLint target, GLint level)
{
	GLint width;
	GLint height;
	GLint depth;
	SparseTextureUtils::getTextureLevelSize(target, mState, level, width, height, depth);

	if (target == GL_TEXTURE_CUBE_MAP)
		depth = 6 * depth;

	GLint widthCommitted = width / 2;
	if (widthCommitted >= mState.pageSizeX && height >= mState.pageSizeY &&
		(mState.minDepth == 0 || depth >= mState.pageSizeZ))
	{
		return true;
	}

	return false;
}

/** Check if current texture size for level is page size multiplication in a corresponding direction
 *
 * @param target  Target for which texture is binded
 * @param level   Texture mipmap level
 *
 * @return Returns true if the texture size condition is fulfilled, false otherwise.
 */
bool SparseTextureCommitmentTestCase::isPageSizesMultiplication(GLint target, GLint level)
{
	GLint width;
	GLint height;
	GLint depth;
	SparseTextureUtils::getTextureLevelSize(target, mState, level, width, height, depth);

	if (target == GL_TEXTURE_CUBE_MAP)
		depth = 6 * depth;

	GLint widthCommitted = width / 2;
	if ((widthCommitted % mState.pageSizeX) == 0 && (height % mState.pageSizeY) == 0 && (depth % mState.pageSizeZ) == 0)
	{
		return true;
	}

	return false;
}

/** Verifies if gltexPageCommitment generates INVALID_OPERATION error in expected use cases
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 *
 * @return Returns true if no error occurred, otherwise throws an exception.
 */
bool SparseTextureCommitmentTestCase::verifyInvalidOperationErrors(const Functions& gl, GLint target, GLint format,
																   GLuint& texture)
{
	mLog << "Verify INVALID_OPERATION Errors - ";

	bool result = true;

	// Case 1 - texture is not GL_TEXTURE_IMMUTABLE_FORMAT
	Texture::Generate(gl, texture);
	Texture::Bind(gl, texture, target);

	gl.texParameteri(target, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texParameteri error occurred for GL_TEXTURE_SPARSE_ARB");

	GLint immutableFormat;

	gl.getTexParameteriv(target, GL_TEXTURE_IMMUTABLE_FORMAT, &immutableFormat);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getTexParameteriv error occurred for GL_TEXTURE_IMMUTABLE_FORMAT");

	if (immutableFormat == GL_FALSE)
	{
		texPageCommitment(gl, target, format, texture, 0, 0, 0, 0, mState.pageSizeX, mState.pageSizeY, mState.pageSizeZ,
						  GL_TRUE);
		result = SparseTextureUtils::verifyError(mLog, "texPageCommitment [GL_TEXTURE_IMMUTABLE_FORMAT texture]",
												 gl.getError(), GL_INVALID_OPERATION);
		if (!result)
			goto verifing_invalid_operation_end;
	}

	Texture::Delete(gl, texture);

	// Case 2 - texture is not TEXTURE_SPARSE_ARB
	allocateTexture(gl, target, format, texture, 1);

	texPageCommitment(gl, target, format, texture, 0, 0, 0, 0, mState.pageSizeX, mState.pageSizeY, mState.pageSizeZ,
					  GL_TRUE);
	result = SparseTextureUtils::verifyError(mLog, "texPageCommitment [not TEXTURE_SPARSE_ARB texture]", gl.getError(),
											 GL_INVALID_OPERATION);
	if (!result)
		goto verifing_invalid_operation_end;

	// Sparse allocate texture
	Texture::Delete(gl, texture);
	sparseAllocateTexture(gl, target, format, texture, 1);

	// Case 3 - commitment sizes greater than expected
	texPageCommitment(gl, target, format, texture, 0, 0, 0, 0, mState.width + mState.pageSizeX, mState.height,
					  mState.depth, GL_TRUE);
	result = SparseTextureUtils::verifyError(mLog, "texPageCommitment [commitment width greater than expected]",
											 gl.getError(), GL_INVALID_OPERATION);
	if (!result)
		goto verifing_invalid_operation_end;

	texPageCommitment(gl, target, format, texture, 0, 0, 0, 0, mState.width, mState.height + mState.pageSizeY,
					  mState.depth, GL_TRUE);
	result = SparseTextureUtils::verifyError(mLog, "texPageCommitment [commitment height greater than expected]",
											 gl.getError(), GL_INVALID_OPERATION);
	if (!result)
		goto verifing_invalid_operation_end;

	if (target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_CUBE_MAP_ARRAY)
	{
		texPageCommitment(gl, target, format, texture, 0, 0, 0, 0, mState.width, mState.height,
						  mState.depth + mState.pageSizeZ, GL_TRUE);
		result = SparseTextureUtils::verifyError(mLog, "texPageCommitment [commitment depth greater than expected]",
												 gl.getError(), GL_INVALID_OPERATION);
		if (!result)
			goto verifing_invalid_operation_end;
	}

	// Case 4 - commitment sizes not multiple of corresponding page sizes
	if (mState.pageSizeX > 1)
	{
		texPageCommitment(gl, target, format, texture, 0, 0, 0, 0, 1, mState.pageSizeY, mState.pageSizeZ, GL_TRUE);
		result =
			SparseTextureUtils::verifyError(mLog, "texPageCommitment [commitment width not multiple of page sizes X]",
											gl.getError(), GL_INVALID_OPERATION);
		if (!result)
			goto verifing_invalid_operation_end;
	}

	if (mState.pageSizeY > 1)
	{
		texPageCommitment(gl, target, format, texture, 0, 0, 0, 0, mState.pageSizeX, 1, mState.pageSizeZ, GL_TRUE);
		result =
			SparseTextureUtils::verifyError(mLog, "texPageCommitment [commitment height not multiple of page sizes Y]",
											gl.getError(), GL_INVALID_OPERATION);
		if (!result)
			goto verifing_invalid_operation_end;
	}

	if (mState.pageSizeZ > 1)
	{
		if (target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_CUBE_MAP_ARRAY)
		{
			texPageCommitment(gl, target, format, texture, 0, 0, 0, 0, mState.pageSizeX, mState.pageSizeY,
							  mState.minDepth, GL_TRUE);
			result = SparseTextureUtils::verifyError(
				mLog, "texPageCommitment [commitment depth not multiple of page sizes Z]", gl.getError(),
				GL_INVALID_OPERATION);
			if (!result)
				goto verifing_invalid_operation_end;
		}
	}

verifing_invalid_operation_end:

	Texture::Delete(gl, texture);

	return result;
}

/** Verifies if texPageCommitment generates INVALID_VALUE error in expected use cases
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 *
 * @return Returns true if no error occurred, otherwise throws an exception.
 */
bool SparseTextureCommitmentTestCase::verifyInvalidValueErrors(const Functions& gl, GLint target, GLint format,
															   GLuint& texture)
{
	mLog << "Verify INVALID_VALUE Errors - ";

	bool result = true;

	sparseAllocateTexture(gl, target, format, texture, 1);

	// Case 1 - commitment offset not multiple of page size in corresponding dimension
	if (mState.pageSizeX > 1)
	{
		texPageCommitment(gl, target, format, texture, 0, 1, 0, 0, mState.pageSizeX, mState.pageSizeY, mState.pageSizeZ,
						  GL_TRUE);
		result =
			SparseTextureUtils::verifyError(mLog, "texPageCommitment [commitment offsetX not multiple of page size X]",
											gl.getError(), GL_INVALID_VALUE);
		if (!result)
			goto verifing_invalid_value_end;
	}
	if (mState.pageSizeY > 1)
	{
		texPageCommitment(gl, target, format, texture, 0, 0, 1, 0, mState.pageSizeX, mState.pageSizeY, mState.pageSizeZ,
						  GL_TRUE);
		result =
			SparseTextureUtils::verifyError(mLog, "texPageCommitment [commitment offsetY not multiple of page size Y]",
											gl.getError(), GL_INVALID_VALUE);
		if (!result)
			goto verifing_invalid_value_end;
	}
	if ((target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_CUBE_MAP_ARRAY) &&
		(mState.minDepth % mState.pageSizeZ))
	{
		texPageCommitment(gl, target, format, texture, 0, 0, 0, mState.minDepth, mState.pageSizeX, mState.pageSizeY,
						  mState.pageSizeZ, GL_TRUE);
		result =
			SparseTextureUtils::verifyError(mLog, "texPageCommitment [commitment offsetZ not multiple of page size Z]",
											gl.getError(), GL_INVALID_VALUE);
		if (!result)
			goto verifing_invalid_value_end;
	}

verifing_invalid_value_end:

	Texture::Delete(gl, texture);

	return result;
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
SparseDSATextureCommitmentTestCase::SparseDSATextureCommitmentTestCase(deqp::Context& context)
	: SparseTextureCommitmentTestCase(context, "SparseDSATextureCommitment",
									  "Verifies texturePageCommitmentEXT functionality added in CTS_ARB_sparse_texture")
{
	/* Left blank intentionally */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SparseDSATextureCommitmentTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_texture"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	if (!m_context.getContextInfo().isExtensionSupported("GL_EXT_direct_state_access"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_EXT_direct_state_access extension is not supported.");
		return STOP;
	}

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool result = true;

	GLuint texture;

	for (std::vector<glw::GLint>::const_iterator iter = mSupportedTargets.begin(); iter != mSupportedTargets.end();
		 ++iter)
	{
		const GLint& target = *iter;

		for (std::vector<glw::GLint>::const_iterator formIter = mSupportedInternalFormats.begin();
			 formIter != mSupportedInternalFormats.end(); ++formIter)
		{
			const GLint& format = *formIter;

			mLog.str("");
			mLog << "Testing DSA sparse texture commitment for target: " << target << ", format: " << format << " - ";

			//Checking if written data into committed region is as expected
			sparseAllocateTexture(gl, target, format, texture, 3);
			for (int l = 0; l < mState.levels; ++l)
			{
				if (commitTexturePage(gl, target, format, texture, l))
				{
					writeDataToTexture(gl, target, format, texture, l);
					result = verifyTextureData(gl, target, format, texture, l);
				}

				if (!result)
					break;
			}

			Texture::Delete(gl, texture);

			//verify errors
			result = result && verifyInvalidOperationErrors(gl, target, format, texture);
			result = result && verifyInvalidValueErrors(gl, target, format, texture);

			if (!result)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << mLog.str() << "Fail" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
				return STOP;
			}
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Bind DSA texturePageCommitmentEXT function
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 * @param xOffset      Texture commitment x offset
 * @param yOffset      Texture commitment y offset
 * @param zOffset      Texture commitment z offset
 * @param width        Texture commitment width
 * @param height       Texture commitment height
 * @param depth        Texture commitment depth
 * @param commit       Commit or de-commit indicator
 **/
void SparseDSATextureCommitmentTestCase::texPageCommitment(const glw::Functions& gl, glw::GLint target,
														   glw::GLint format, glw::GLuint& texture, GLint level,
														   GLint xOffset, GLint yOffset, GLint zOffset, GLint width,
														   GLint height, GLint depth, GLboolean commit)
{
	DE_UNREF(target);
	DE_UNREF(format);
	gl.texturePageCommitmentEXT(texture, level, xOffset, yOffset, zOffset, width, height, depth, commit);
}

/** Constructor.
 *
 *  @param context Rendering context.
 */
SparseTextureTests::SparseTextureTests(deqp::Context& context)
	: TestCaseGroup(context, "sparse_texture_tests", "Verify conformance of CTS_ARB_sparse_texture implementation")
{
}

/** Initializes the test group contents. */
void SparseTextureTests::init()
{
	addChild(new TextureParameterQueriesTestCase(m_context));
	addChild(new InternalFormatQueriesTestCase(m_context));
	addChild(new SimpleQueriesTestCase(m_context));
	addChild(new SparseTextureAllocationTestCase(m_context));
	addChild(new SparseTextureCommitmentTestCase(m_context));
	addChild(new SparseDSATextureCommitmentTestCase(m_context));
}

} /* gl4cts namespace */
