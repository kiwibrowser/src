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
 * \file  esextcTextureCubeMapArrayGetterCalls.cpp
 * \brief Texture Cube Map Array Getter Calls (Test 6)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureCubeMapArrayGetterCalls.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <string.h>

namespace glcts
{

/* Properties of a cube-map array texture used by the test */
const glw::GLuint  TextureCubeMapArrayGetterCalls::m_depth		  = 6;
const glw::GLsizei TextureCubeMapArrayGetterCalls::m_height		  = 64;
const glw::GLsizei TextureCubeMapArrayGetterCalls::m_width		  = 64;
const glw::GLuint  TextureCubeMapArrayGetterCalls::m_n_components = 4;

/* Name strings for GetTexParameter*() pnames */
const char* TextureCubeMapArrayGetterCalls::getStringForGetTexParameterPname(glw::GLenum pname)
{
	if (pname == GL_TEXTURE_BASE_LEVEL)
		return "GL_TEXTURE_BASE_LEVEL";
	if (pname == GL_TEXTURE_MAX_LEVEL)
		return "GL_TEXTURE_MAX_LEVEL";
	if (pname == GL_TEXTURE_MIN_FILTER)
		return "GL_TEXTURE_MIN_FILTER";
	if (pname == GL_TEXTURE_MAG_FILTER)
		return "GL_TEXTURE_MAG_FILTER";
	if (pname == GL_TEXTURE_MIN_LOD)
		return "GL_TEXTURE_MIN_LOD";
	if (pname == GL_TEXTURE_MAX_LOD)
		return "GL_TEXTURE_MAX_LOD";
	if (pname == GL_TEXTURE_SWIZZLE_R)
		return "GL_TEXTURE_SWIZZLE_R";
	if (pname == GL_TEXTURE_SWIZZLE_G)
		return "GL_TEXTURE_SWIZZLE_G";
	if (pname == GL_TEXTURE_SWIZZLE_B)
		return "GL_TEXTURE_SWIZZLE_B";
	if (pname == GL_TEXTURE_SWIZZLE_A)
		return "GL_TEXTURE_SWIZZLE_A";
	if (pname == GL_TEXTURE_WRAP_S)
		return "GL_TEXTURE_WRAP_S";
	if (pname == GL_TEXTURE_WRAP_T)
		return "GL_TEXTURE_WRAP_T";
	if (pname == GL_TEXTURE_WRAP_R)
		return "GL_TEXTURE_WRAP_R";

	return "UNKNOWN PARAMETER NAME";
}

/* Name strings for GetTexLevelParameter*() pnames */
const char* TextureCubeMapArrayGetterCalls::getStringForGetTexLevelParameterPname(glw::GLenum pname)
{
	if (pname == GL_TEXTURE_COMPRESSED)
		return "GL_TEXTURE_COMPRESSED";
	if (pname == GL_TEXTURE_ALPHA_SIZE)
		return "GL_TEXTURE_ALPHA_SIZE";
	if (pname == GL_TEXTURE_BLUE_SIZE)
		return "GL_TEXTURE_BLUE_SIZE";
	if (pname == GL_TEXTURE_GREEN_SIZE)
		return "GL_TEXTURE_GREEN_SIZE";
	if (pname == GL_TEXTURE_RED_SIZE)
		return "GL_TEXTURE_RED_SIZE";
	if (pname == GL_TEXTURE_DEPTH_SIZE)
		return "GL_TEXTURE_DEPTH_SIZE";
	if (pname == GL_TEXTURE_SHARED_SIZE)
		return "GL_TEXTURE_SHARED_SIZE";
	if (pname == GL_TEXTURE_STENCIL_SIZE)
		return "GL_TEXTURE_STENCIL_SIZE";
	if (pname == GL_TEXTURE_ALPHA_TYPE)
		return "GL_TEXTURE_ALPHA_TYPE";
	if (pname == GL_TEXTURE_BLUE_TYPE)
		return "GL_TEXTURE_BLUE_TYPE";
	if (pname == GL_TEXTURE_GREEN_TYPE)
		return "GL_TEXTURE_GREEN_TYPE";
	if (pname == GL_TEXTURE_RED_TYPE)
		return "GL_TEXTURE_RED_TYPE";
	if (pname == GL_TEXTURE_DEPTH_TYPE)
		return "GL_TEXTURE_DEPTH_TYPE";
	if (pname == GL_TEXTURE_INTERNAL_FORMAT)
		return "GL_TEXTURE_INTERNAL_FORMAT";
	if (pname == GL_TEXTURE_WIDTH)
		return "GL_TEXTURE_WIDTH";
	if (pname == GL_TEXTURE_HEIGHT)
		return "GL_TEXTURE_HEIGHT";
	if (pname == GL_TEXTURE_DEPTH)
		return "GL_TEXTURE_DEPTH";

	return "UNKNOWN PARAMETER NAME";
}

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureCubeMapArrayGetterCalls::TextureCubeMapArrayGetterCalls(Context& context, const ExtParameters& extParams,
															   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_to_id(0)
	, m_test_passed(true)
	, m_expected_alpha_size(0)
	, m_expected_alpha_type(0)
	, m_expected_blue_size(0)
	, m_expected_blue_type(0)
	, m_expected_compressed(0)
	, m_expected_depth_size(0)
	, m_expected_depth_type(0)
	, m_expected_green_size(0)
	, m_expected_green_type(0)
	, m_expected_red_size(0)
	, m_expected_red_type(0)
	, m_expected_shared_size(0)
	, m_expected_stencil_size(0)
	, m_expected_texture_internal_format(0)
{
	/* Nothing to be done here */
}

/** Deinitializes GLES objects created during the test.
 *
 */
void TextureCubeMapArrayGetterCalls::deinit(void)
{
	/* Retrieve ES entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset ES state */
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);

	/* Release any objects that may have been created during test execution */
	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);

		m_to_id = 0;
	}

	/* Deinitialize base class */
	TestCaseBase::deinit();
}

/** Initializes GLES objects used during the test.
 *
 */
void TextureCubeMapArrayGetterCalls::initTest(void)
{
	/* Only execute if GL_EXT_texture_cube_map_array extension is supported */
	if (!m_is_texture_cube_map_array_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_CUBE_MAP_ARRAY_EXTENSION_NOT_SUPPORTED);
	}

	pnames_for_gettexparameter_default[GL_TEXTURE_BASE_LEVEL] = 0;
	pnames_for_gettexparameter_default[GL_TEXTURE_MAX_LEVEL]  = 1000;
	pnames_for_gettexparameter_default[GL_TEXTURE_MIN_FILTER] = GL_NEAREST_MIPMAP_LINEAR;
	pnames_for_gettexparameter_default[GL_TEXTURE_MAG_FILTER] = GL_LINEAR;
	pnames_for_gettexparameter_default[GL_TEXTURE_MIN_LOD]	= -1000;
	pnames_for_gettexparameter_default[GL_TEXTURE_MAX_LOD]	= 1000;
	pnames_for_gettexparameter_default[GL_TEXTURE_SWIZZLE_R]  = GL_RED;
	pnames_for_gettexparameter_default[GL_TEXTURE_SWIZZLE_G]  = GL_GREEN;
	pnames_for_gettexparameter_default[GL_TEXTURE_SWIZZLE_B]  = GL_BLUE;
	pnames_for_gettexparameter_default[GL_TEXTURE_SWIZZLE_A]  = GL_ALPHA;
	pnames_for_gettexparameter_default[GL_TEXTURE_WRAP_S]	 = GL_REPEAT;
	pnames_for_gettexparameter_default[GL_TEXTURE_WRAP_T]	 = GL_REPEAT;
	pnames_for_gettexparameter_default[GL_TEXTURE_WRAP_R]	 = GL_REPEAT;

	pnames_for_gettexparameter_modified[GL_TEXTURE_BASE_LEVEL] = 1;
	pnames_for_gettexparameter_modified[GL_TEXTURE_MAX_LEVEL]  = 1;
	pnames_for_gettexparameter_modified[GL_TEXTURE_MIN_FILTER] = GL_NEAREST;
	pnames_for_gettexparameter_modified[GL_TEXTURE_MAG_FILTER] = GL_NEAREST;
	pnames_for_gettexparameter_modified[GL_TEXTURE_MIN_LOD]	= -10;
	pnames_for_gettexparameter_modified[GL_TEXTURE_MAX_LOD]	= 10;
	pnames_for_gettexparameter_modified[GL_TEXTURE_SWIZZLE_R]  = GL_GREEN;
	pnames_for_gettexparameter_modified[GL_TEXTURE_SWIZZLE_G]  = GL_BLUE;
	pnames_for_gettexparameter_modified[GL_TEXTURE_SWIZZLE_B]  = GL_ALPHA;
	pnames_for_gettexparameter_modified[GL_TEXTURE_SWIZZLE_A]  = GL_RED;
	pnames_for_gettexparameter_modified[GL_TEXTURE_WRAP_S]	 = GL_CLAMP_TO_EDGE;
	pnames_for_gettexparameter_modified[GL_TEXTURE_WRAP_T]	 = GL_MIRRORED_REPEAT;
	pnames_for_gettexparameter_modified[GL_TEXTURE_WRAP_R]	 = GL_CLAMP_TO_EDGE;

	pnames_for_gettexlevelparameter.push_back(GL_TEXTURE_COMPRESSED);
	pnames_for_gettexlevelparameter.push_back(GL_TEXTURE_ALPHA_SIZE);
	pnames_for_gettexlevelparameter.push_back(GL_TEXTURE_BLUE_SIZE);
	pnames_for_gettexlevelparameter.push_back(GL_TEXTURE_GREEN_SIZE);
	pnames_for_gettexlevelparameter.push_back(GL_TEXTURE_RED_SIZE);
	pnames_for_gettexlevelparameter.push_back(GL_TEXTURE_DEPTH_SIZE);
	pnames_for_gettexlevelparameter.push_back(GL_TEXTURE_SHARED_SIZE);
	pnames_for_gettexlevelparameter.push_back(GL_TEXTURE_STENCIL_SIZE);
	pnames_for_gettexlevelparameter.push_back(GL_TEXTURE_ALPHA_TYPE);
	pnames_for_gettexlevelparameter.push_back(GL_TEXTURE_BLUE_TYPE);
	pnames_for_gettexlevelparameter.push_back(GL_TEXTURE_GREEN_TYPE);
	pnames_for_gettexlevelparameter.push_back(GL_TEXTURE_RED_TYPE);
	pnames_for_gettexlevelparameter.push_back(GL_TEXTURE_DEPTH_TYPE);
	pnames_for_gettexlevelparameter.push_back(GL_TEXTURE_INTERNAL_FORMAT);
	pnames_for_gettexlevelparameter.push_back(GL_TEXTURE_WIDTH);
	pnames_for_gettexlevelparameter.push_back(GL_TEXTURE_HEIGHT);
	pnames_for_gettexlevelparameter.push_back(GL_TEXTURE_DEPTH);
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult TextureCubeMapArrayGetterCalls::iterate(void)
{
	initTest();

	/* Retrieve ES entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate a texture object */
	gl.genTextures(1, &m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate a texture object!");

	/* Bind the texture object to GL_TEXTURE_CUBE_MAP_ARRAY_EXT texture target */
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind a texture object!");

	glw::GLubyte texture_data_ubyte[m_width * m_height * m_depth * m_n_components];
	memset(texture_data_ubyte, 0, m_width * m_height * m_depth * m_n_components * sizeof(glw::GLubyte));

	/* Set up mutable texture storage */
	gl.texImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, /* target */
				  0,						 /* level */
				  GL_RGBA8,					 /* internal format */
				  m_width,					 /* width */
				  m_height,					 /* height */
				  m_depth,					 /* depth */
				  0,						 /* border */
				  GL_RGBA,					 /* format */
				  GL_UNSIGNED_BYTE,			 /* type */
				  texture_data_ubyte);		 /* texel data */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create mutable texture storage!");

	gl.texImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, /* target */
				  1,						 /* level */
				  GL_RGBA8,					 /* internal format */
				  m_width,					 /* width */
				  m_height,					 /* height */
				  m_depth,					 /* depth */
				  0,						 /* border */
				  GL_RGBA,					 /* format */
				  GL_UNSIGNED_BYTE,			 /* type */
				  texture_data_ubyte);		 /* texel data */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create mutable texture storage!");

	/* Configure expected values for all properties we will be checking */
	m_expected_compressed			   = GL_FALSE;
	m_expected_alpha_size			   = 8;
	m_expected_alpha_type			   = GL_UNSIGNED_NORMALIZED;
	m_expected_blue_size			   = 8;
	m_expected_blue_type			   = GL_UNSIGNED_NORMALIZED;
	m_expected_green_size			   = 8;
	m_expected_green_type			   = GL_UNSIGNED_NORMALIZED;
	m_expected_red_size				   = 8;
	m_expected_red_type				   = GL_UNSIGNED_NORMALIZED;
	m_expected_depth_size			   = 0;
	m_expected_depth_type			   = GL_NONE;
	m_expected_shared_size			   = 0;
	m_expected_stencil_size			   = 0;
	m_expected_texture_internal_format = GL_RGBA8;

	/* Verify the texture bindings have been updated */
	verifyTextureBindings();

	/* Verify texture parameter values reported by glGetTexParameter*() functions are valid. */
	PNamesMap::iterator pnames_iter = pnames_for_gettexparameter_default.begin();
	PNamesMap::iterator pnames_end  = pnames_for_gettexparameter_default.end();
	for (; pnames_iter != pnames_end; ++pnames_iter)
	{
		verifyGetTexParameter(pnames_iter->first, pnames_iter->second);
	}

	pnames_iter = pnames_for_gettexparameter_modified.begin();
	pnames_end  = pnames_for_gettexparameter_modified.end();
	for (; pnames_iter != pnames_end; ++pnames_iter)
	{
		/* Set property value(s) using glGetTexParameteriv() */
		gl.texParameteriv(GL_TEXTURE_CUBE_MAP_ARRAY, pnames_iter->first, &pnames_iter->second);

		if (gl.getError() != GL_NO_ERROR)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "glTexParameteriv() call failed for pname: "
							   << getStringForGetTexParameterPname(pnames_iter->first)
							   << " and value: " << pnames_iter->second << tcu::TestLog::EndMessage;

			TCU_FAIL("glTexParameteriv() call failed");
		}

		verifyGetTexParameter(pnames_iter->first, pnames_iter->second);
	}

	/* Verify texture level parameter values reported by glGetTexLevelParameter*()
	 * functions are valid. */
	verifyGetTexLevelParameters();

	/* Delete a texture object */
	gl.deleteTextures(1, &m_to_id);
	m_to_id = 0;

	/* Generate a texture object */
	gl.genTextures(1, &m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate a texture object!");

	/* Bind the texture object to GL_TEXTURE_CUBE_MAP_ARRAY_EXT texture target */
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind a texture object!");

	/* Set up immutable texture storage */
	gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 2, /* levels */
					GL_RGBA32F,					  /* internal format */
					m_width,					  /* width */
					m_height,					  /* height */
					m_depth);					  /* depth */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create an immutable texture storage!");

	/* Update expected values for all the properties we will be checking */
	m_expected_compressed			   = GL_FALSE;
	m_expected_alpha_size			   = 32;
	m_expected_alpha_type			   = GL_FLOAT;
	m_expected_blue_size			   = 32;
	m_expected_blue_type			   = GL_FLOAT;
	m_expected_green_size			   = 32;
	m_expected_green_type			   = GL_FLOAT;
	m_expected_red_size				   = 32;
	m_expected_red_type				   = GL_FLOAT;
	m_expected_depth_size			   = 0;
	m_expected_depth_type			   = GL_NONE;
	m_expected_shared_size			   = 0;
	m_expected_stencil_size			   = 0;
	m_expected_texture_internal_format = GL_RGBA32F;

	/* Verify that texture bindings have been updated */
	verifyTextureBindings();

	/* Verify texture parameter values reported by glGetTexParameter*() functions are valid. */
	pnames_iter = pnames_for_gettexparameter_default.begin();
	pnames_end  = pnames_for_gettexparameter_default.end();
	for (; pnames_iter != pnames_end; ++pnames_iter)
	{
		verifyGetTexParameter(pnames_iter->first, pnames_iter->second);
	}

	pnames_iter = pnames_for_gettexparameter_modified.begin();
	pnames_end  = pnames_for_gettexparameter_modified.end();
	for (; pnames_iter != pnames_end; ++pnames_iter)
	{
		/* Set property value(s) using glGetTexParameteriv() */
		gl.texParameteriv(GL_TEXTURE_CUBE_MAP_ARRAY, pnames_iter->first, &pnames_iter->second);

		if (gl.getError() != GL_NO_ERROR)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "glTexParameteriv() call failed for pname: "
							   << getStringForGetTexParameterPname(pnames_iter->first)
							   << " and value: " << pnames_iter->second << tcu::TestLog::EndMessage;

			TCU_FAIL("glTexParameteriv() call failed");
		}

		verifyGetTexParameter(pnames_iter->first, pnames_iter->second);
	}

	/* Verify texture level parameter values reported by glGetTexLevelParameter*()
	 * functions are valid. */
	verifyGetTexLevelParameters();

	if (m_test_passed)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

/** Verifies GL_TEXTURE_CUBE_MAP_ARRAY_EXT texture binding is reported
 *  correctly by glGetBooleanv(), glGetFloatv() and glGetIntegerv().
 *
 *  It is assumed that texture object of id m_to_id is bound to
 *  GL_TEXTURE_CUBE_MAP_ARRAY_EXT texture binding point at the time
 *  of the call.
 *
 **/
void TextureCubeMapArrayGetterCalls::verifyTextureBindings(void)
{
	const glw::Functions& gl		  = m_context.getRenderContext().getFunctions();
	glw::GLboolean		  bool_value  = GL_FALSE;
	const float			  epsilon	 = 1e-5f;
	glw::GLfloat		  float_value = 0.0f;
	glw::GLint			  int_value   = 0;

	/* Check glGetBooleanv() reports a correct value */
	gl.getBooleanv(GL_TEXTURE_BINDING_CUBE_MAP_ARRAY, &bool_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetBooleanv() failed for GL_TEXTURE_BINDING_CUBE_MAP_ARRAY_EXT pname");

	if ((m_to_id == 0 && bool_value != GL_FALSE) || (m_to_id != 0 && bool_value != GL_TRUE))
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "glGetBooleanv() reported an invalid value for GL_TEXTURE_BINDING_CUBE_MAP_ARRAY_EXT pname"
			<< tcu::TestLog::EndMessage;
		m_test_passed = false;
	}

	/* Check glGetFloatv() reports a correct value */
	gl.getFloatv(GL_TEXTURE_BINDING_CUBE_MAP_ARRAY, &float_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFloatv() failed for GL_TEXTURE_BINDING_CUBE_MAP_ARRAY_EXT pname");

	if (de::abs(float_value - static_cast<float>(m_to_id)) > epsilon)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "glGetFloatv() reported an invalid value for GL_TEXTURE_BINDING_CUBE_MAP_ARRAY_EXT pname"
						   << tcu::TestLog::EndMessage;
		m_test_passed = false;
	}

	/* Check glGetIntegerv() reports a correct value */
	gl.getIntegerv(GL_TEXTURE_BINDING_CUBE_MAP_ARRAY, &int_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_TEXTURE_BINDING_CUBE_MAP_ARRAY_EXT pname");

	if (de::abs(float_value - static_cast<float>(m_to_id)) > epsilon)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "glGetIntegerv() reported an invalid value for GL_TEXTURE_BINDING_CUBE_MAP_ARRAY_EXT pname"
			<< tcu::TestLog::EndMessage;
		m_test_passed = false;
	}
}

/** Verifies that all texture parameter values reported by corresponding
 *  getter functions are as defined for a texture object currently bound to
 *  GL_TEXTURE_CUBE_MAP_ARRAY_EXT binding point
 **/
void TextureCubeMapArrayGetterCalls::verifyGetTexParameter(glw::GLenum pname, glw::GLint expected_value)
{
	/* Retrieve ES function pointers */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint int_value = 0;

	/* Retrieve property value(s) using glGetTexParameteriv() */
	gl.getTexParameteriv(GL_TEXTURE_CUBE_MAP_ARRAY, pname, &int_value);

	if (gl.getError() != GL_NO_ERROR)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "glGetTexParameteriv() call failed for pname: " << getStringForGetTexParameterPname(pname)
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("glGetTexLevelParameteriv() call failed");
	}

	if (int_value != expected_value)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "glGetTexParameteriv() called for pname: "
													   "["
						   << getStringForGetTexParameterPname(pname) << "]"
																		 " returned an invalid value of:"
																		 "["
						   << int_value << "]"
										   ", expected:"
										   "["
						   << expected_value << "]" << tcu::TestLog::EndMessage;

		m_test_passed = false;
	}
}

/** Verifies that all texture level parameter values reported by corresponding
 *  getter functions are as defined for a texture object currently bound to
 *  GL_TEXTURE_CUBE_MAP_ARRAY_EXT binding point
 **/
void TextureCubeMapArrayGetterCalls::verifyGetTexLevelParameters(void)
{
	/* Retrieve ES function pointers */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLfloat	   float_value = 0.0f;
	glw::GLint		   int_value   = 0;
	const glw::GLfloat epsilon	 = 1e-5f;

	PNamesVec::iterator pnames_end = pnames_for_gettexlevelparameter.end();
	for (PNamesVec::iterator pnames_iter = pnames_for_gettexlevelparameter.begin(); pnames_iter != pnames_end;
		 ++pnames_iter)
	{
		glw::GLenum pname = *pnames_iter;

		/* Retrieve property value(s) using glGetTexLevelParameteriv() */
		gl.getTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_ARRAY, 0, /* level */
								  pname, &int_value);

		if (gl.getError() != GL_NO_ERROR)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "glGetTexLevelParameteriv() call failed for pname: "
							   << getStringForGetTexLevelParameterPname(pname) << tcu::TestLog::EndMessage;

			TCU_FAIL("glGetTexLevelParameteriv() call failed");
		}

		/* Retrieve property value(s) using glGetTexLevelParameterfv() */
		gl.getTexLevelParameterfv(GL_TEXTURE_CUBE_MAP_ARRAY, 0, /* level */
								  pname, &float_value);

		if (gl.getError() != GL_NO_ERROR)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "glGetTexLevelParameterfv() call failed for pname: "
							   << getStringForGetTexLevelParameterPname(pname) << tcu::TestLog::EndMessage;

			TCU_FAIL("glGetTexLevelParameterfv() call failed");
		}

		/* Make sure the property values are valid */
		glw::GLboolean should_use_equal_comparison = true;
		glw::GLint	 expected_property_int_value = 0;

		switch (pname)
		{
		case GL_TEXTURE_ALPHA_SIZE:
		{
			expected_property_int_value = m_expected_alpha_size;
			should_use_equal_comparison = false;
			break;
		}

		case GL_TEXTURE_ALPHA_TYPE:
		{
			expected_property_int_value = m_expected_alpha_type;
			break;
		}

		case GL_TEXTURE_BLUE_SIZE:
		{
			expected_property_int_value = m_expected_blue_size;
			should_use_equal_comparison = false;
			break;
		}

		case GL_TEXTURE_BLUE_TYPE:
		{
			expected_property_int_value = m_expected_blue_type;
			break;
		}

		case GL_TEXTURE_COMPRESSED:
		{
			expected_property_int_value = m_expected_compressed;
			break;
		}

		case GL_TEXTURE_DEPTH:
		{
			expected_property_int_value = m_depth;
			break;
		}

		case GL_TEXTURE_DEPTH_SIZE:
		{
			expected_property_int_value = m_expected_depth_size;
			break;
		}

		case GL_TEXTURE_DEPTH_TYPE:
		{
			expected_property_int_value = m_expected_depth_type;
			break;
		}

		case GL_TEXTURE_GREEN_SIZE:
		{
			expected_property_int_value = m_expected_green_size;
			should_use_equal_comparison = false;
			break;
		}

		case GL_TEXTURE_GREEN_TYPE:
		{
			expected_property_int_value = m_expected_green_type;
			break;
		}

		case GL_TEXTURE_HEIGHT:
		{
			expected_property_int_value = m_height;
			break;
		}

		case GL_TEXTURE_INTERNAL_FORMAT:
		{
			expected_property_int_value = m_expected_texture_internal_format;
			break;
		}

		case GL_TEXTURE_RED_SIZE:
		{
			expected_property_int_value = m_expected_red_size;
			should_use_equal_comparison = false;
			break;
		}

		case GL_TEXTURE_RED_TYPE:
		{
			expected_property_int_value = m_expected_red_type;

			break;
		}

		case GL_TEXTURE_SHARED_SIZE:
		{
			expected_property_int_value = m_expected_shared_size;
			should_use_equal_comparison = false;
			break;
		}

		case GL_TEXTURE_STENCIL_SIZE:
		{
			expected_property_int_value = m_expected_stencil_size;
			break;
		}

		case GL_TEXTURE_WIDTH:
		{
			expected_property_int_value = m_width;
			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized pname");
		}
		} /* switch(pname) */

		if ((should_use_equal_comparison && (expected_property_int_value != int_value)) ||
			(!should_use_equal_comparison && (expected_property_int_value < int_value)))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "glGetTexLevelParameteriv() called for pname: "
														   "["
							   << getStringForGetTexLevelParameterPname(pname) << "]"
																				  " returned an invalid value of:"
																				  "["
							   << int_value << "]"
											   ", expected:"
											   "["
							   << expected_property_int_value << "]" << tcu::TestLog::EndMessage;

			m_test_passed = false;
		}

		glw::GLfloat expected_property_float_value = static_cast<glw::GLfloat>(expected_property_int_value);
		if ((should_use_equal_comparison && (de::abs(float_value - (expected_property_float_value)) > epsilon)) ||
			(!should_use_equal_comparison && (expected_property_float_value < float_value)))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "glGetTexLevelParameterfv() called for pname: "
														   "["
							   << getStringForGetTexLevelParameterPname(pname) << "]"
																				  " returned an invalid value of:"
																				  "["
							   << float_value << "]"
												 ", expected:"
												 "["
							   << expected_property_float_value << "]" << tcu::TestLog::EndMessage;

			m_test_passed = false;
		}
	} /* for (all property names) */
}

} // namespace glcts
