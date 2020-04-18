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
 * \file esextcTextureBorderClampBase.cpp
 * \brief Base Class for Texture Border Clamp extension tests 1-6.
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBorderClampBase.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
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
TextureBorderClampBase::TextureBorderClampBase(Context& context, const ExtParameters& extParams, const char* name,
											   const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_texture_2D_array_id(0)
	, m_texture_2D_id(0)
	, m_texture_2D_multisample_array_id(0)
	, m_texture_2D_multisample_id(0)
	, m_texture_3D_id(0)
	, m_texture_buffer_id(0)
	, m_texture_cube_map_id(0)
	, m_texture_cube_map_array_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes GLES objects created during the tests.
 *
 */
void TextureBorderClampBase::deinit(void)
{
	deinitAllTextures();

	/* Deinitializes base class */
	TestCaseBase::deinit();
}

/** Initializes GLES objects used during the tests.
 *
 */
void TextureBorderClampBase::initTest(void)
{
	initAllTextures();
}

/** Deinitializes all texture objects */
void TextureBorderClampBase::deinitAllTextures(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.bindTexture(GL_TEXTURE_3D, 0);
	gl.bindTexture(GL_TEXTURE_2D_ARRAY, 0);
	gl.bindTexture(GL_TEXTURE_CUBE_MAP, 0);

	if (m_is_texture_storage_multisample_supported)
	{
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	}

	if (m_is_texture_storage_multisample_2d_array_supported)
	{
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 0);
	}

	if (m_is_texture_cube_map_array_supported)
	{
		gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
	}

	if (m_is_texture_buffer_supported)
	{
		gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, 0);
	}

	if (0 != m_texture_2D_id)
	{
		gl.deleteTextures(1, &m_texture_2D_id);

		m_texture_2D_id = 0;
	}

	if (0 != m_texture_3D_id)
	{
		gl.deleteTextures(1, &m_texture_3D_id);

		m_texture_3D_id = 0;
	}

	if (0 != m_texture_2D_array_id)
	{
		gl.deleteTextures(1, &m_texture_2D_array_id);

		m_texture_2D_array_id = 0;
	}

	if (0 != m_texture_cube_map_id)
	{
		gl.deleteTextures(1, &m_texture_cube_map_id);

		m_texture_cube_map_id = 0;
	}

	if (0 != m_texture_cube_map_array_id)
	{
		gl.deleteTextures(1, &m_texture_cube_map_array_id);

		m_texture_cube_map_array_id = 0;
	}

	if (0 != m_texture_2D_multisample_id)
	{
		gl.deleteTextures(1, &m_texture_2D_multisample_id);

		m_texture_2D_multisample_id = 0;
	}

	if (0 != m_texture_2D_multisample_array_id)
	{
		gl.deleteTextures(1, &m_texture_2D_multisample_array_id);

		m_texture_2D_multisample_array_id = 0;
	}

	if (0 != m_texture_buffer_id)
	{
		gl.deleteBuffers(1, &m_texture_buffer_id);

		m_texture_buffer_id = 0;
	}
}

/** Initializes all texture objects */
void TextureBorderClampBase::initAllTextures(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create and bind a 2D texture object */
	gl.genTextures(1, &m_texture_2D_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating a texture object");

	gl.bindTexture(GL_TEXTURE_2D, m_texture_2D_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding a texture object to GL_TEXTURE_2D texture target");

	m_texture_target_list.push_back(GL_TEXTURE_2D);

	/* Create and bind a 3D texture object */
	gl.genTextures(1, &m_texture_3D_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating a texture object");

	gl.bindTexture(GL_TEXTURE_3D, m_texture_3D_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding a texture object to GL_TEXTURE_3D texture target");

	m_texture_target_list.push_back(GL_TEXTURE_3D);

	/* Create and bind a 2D array texture object */
	gl.genTextures(1, &m_texture_2D_array_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating a texture object");

	gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_texture_2D_array_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding a texture object to GL_TEXTURE_2D_ARRAY texture target");

	m_texture_target_list.push_back(GL_TEXTURE_2D_ARRAY);

	/* Create and bind a cube map texture object */
	gl.genTextures(1, &m_texture_cube_map_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating a texture object");

	gl.bindTexture(GL_TEXTURE_CUBE_MAP, m_texture_cube_map_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding a texture object to GL_TEXTURE_CUBE_MAP texture target");

	m_texture_target_list.push_back(GL_TEXTURE_CUBE_MAP);

	if (m_is_texture_cube_map_array_supported)
	{
		/* Create and bind a cube map array texture object */
		gl.genTextures(1, &m_texture_cube_map_array_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating a texture object");

		gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_texture_cube_map_array_id);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error binding a texture object to GL_TEXTURE_CUBE_MAP_ARRAY_EXT texture target");

		m_texture_target_list.push_back(GL_TEXTURE_CUBE_MAP_ARRAY);
	}

	if (m_is_texture_storage_multisample_supported)
	{
		/* Create and bind a 2D multisample texture object */
		gl.genTextures(1, &m_texture_2D_multisample_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating a texture object");

		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_texture_2D_multisample_id);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error binding a texture object to GL_TEXTURE_2D_MULTISAMPLE texture target");

		m_texture_target_list.push_back(GL_TEXTURE_2D_MULTISAMPLE);
	}

	if (m_is_texture_storage_multisample_2d_array_supported)
	{
		/* Create and bind a 2D multisample array texture object */
		gl.genTextures(1, &m_texture_2D_multisample_array_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating a texture object");

		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, m_texture_2D_multisample_array_id);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Error binding a texture object to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target");

		m_texture_target_list.push_back(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES);
	}

	if (m_is_texture_buffer_supported)
	{
		/* Create and bind a buffer texture object */
		gl.genBuffers(1, &m_texture_buffer_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating a texture object");

		gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, m_texture_buffer_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding a texture object to GL_TEXTURE_BUFFER_EXT texture target");
	}
}

/** Returns "texture border clamp"-specific pname as literals.
 *
 * @param  pname GL property name.
 *
 * @return pointer to literal with pname
 */
const char* TextureBorderClampBase::getPNameString(glw::GLenum pname)
{
	static const char* str_GL_TEXTURE_BASE_LEVEL		 = "GL_TEXTURE_BASE_LEVEL";
	static const char* str_GL_TEXTURE_BORDER_COLOR_EXT   = "GL_TEXTURE_BORDER_COLOR_EXT";
	static const char* str_GL_TEXTURE_MIN_FILTER		 = "GL_TEXTURE_MIN_FILTER";
	static const char* str_GL_TEXTURE_IMMUTABLE_FORMAT   = "GL_TEXTURE_IMMUTABLE_FORMAT";
	static const char* str_GL_TEXTURE_COMPARE_MODE		 = "GL_TEXTURE_COMPARE_MODE";
	static const char* str_GL_TEXTURE_COMPARE_FUNC		 = "GL_TEXTURE_COMPARE_FUNC";
	static const char* str_GL_TEXTURE_MAG_FILTER		 = "GL_TEXTURE_MAG_FILTER";
	static const char* str_GL_TEXTURE_MAX_LEVEL			 = "GL_TEXTURE_MAX_LEVEL";
	static const char* str_GL_TEXTURE_SWIZZLE_R			 = "GL_TEXTURE_SWIZZLE_R";
	static const char* str_GL_TEXTURE_SWIZZLE_G			 = "GL_TEXTURE_SWIZZLE_G";
	static const char* str_GL_TEXTURE_SWIZZLE_B			 = "GL_TEXTURE_SWIZZLE_B";
	static const char* str_GL_TEXTURE_SWIZZLE_A			 = "GL_TEXTURE_SWIZZLE_A";
	static const char* str_GL_TEXTURE_WRAP_S			 = "GL_TEXTURE_WRAP_S";
	static const char* str_GL_TEXTURE_WRAP_T			 = "GL_TEXTURE_WRAP_T";
	static const char* str_GL_TEXTURE_WRAP_R			 = "GL_TEXTURE_WRAP_R";
	static const char* str_GL_DEPTH_STENCIL_TEXTURE_MODE = "GL_DEPTH_STENCIL_TEXTURE_MODE";
	static const char* str_GL_TEXTURE_IMMUTABLE_LEVELS   = "GL_TEXTURE_IMMUTABLE_LEVELS";
	static const char* str_GL_TEXTURE_MAX_LOD			 = "GL_TEXTURE_MAX_LOD";
	static const char* str_GL_TEXTURE_MIN_LOD			 = "GL_TEXTURE_MIN_LOD";
	static const char* str_UNKNOWN						 = "UNKNOWN";

	if (pname == m_glExtTokens.TEXTURE_BORDER_COLOR)
	{
		return str_GL_TEXTURE_BORDER_COLOR_EXT;
	}

	switch (pname)
	{
	case GL_TEXTURE_BASE_LEVEL:
		return str_GL_TEXTURE_BASE_LEVEL;
	case GL_TEXTURE_MIN_FILTER:
		return str_GL_TEXTURE_MIN_FILTER;
	case GL_TEXTURE_IMMUTABLE_FORMAT:
		return str_GL_TEXTURE_IMMUTABLE_FORMAT;
	case GL_TEXTURE_COMPARE_MODE:
		return str_GL_TEXTURE_COMPARE_MODE;
	case GL_TEXTURE_COMPARE_FUNC:
		return str_GL_TEXTURE_COMPARE_FUNC;
	case GL_TEXTURE_MAG_FILTER:
		return str_GL_TEXTURE_MAG_FILTER;
	case GL_TEXTURE_MAX_LEVEL:
		return str_GL_TEXTURE_MAX_LEVEL;
	case GL_TEXTURE_SWIZZLE_R:
		return str_GL_TEXTURE_SWIZZLE_R;
	case GL_TEXTURE_SWIZZLE_G:
		return str_GL_TEXTURE_SWIZZLE_G;
	case GL_TEXTURE_SWIZZLE_B:
		return str_GL_TEXTURE_SWIZZLE_B;
	case GL_TEXTURE_SWIZZLE_A:
		return str_GL_TEXTURE_SWIZZLE_A;
	case GL_TEXTURE_WRAP_S:
		return str_GL_TEXTURE_WRAP_S;
	case GL_TEXTURE_WRAP_T:
		return str_GL_TEXTURE_WRAP_T;
	case GL_TEXTURE_WRAP_R:
		return str_GL_TEXTURE_WRAP_R;
	case GL_DEPTH_STENCIL_TEXTURE_MODE:
		return str_GL_DEPTH_STENCIL_TEXTURE_MODE;
	case GL_TEXTURE_IMMUTABLE_LEVELS:
		return str_GL_TEXTURE_IMMUTABLE_LEVELS;
	case GL_TEXTURE_MAX_LOD:
		return str_GL_TEXTURE_MAX_LOD;
	case GL_TEXTURE_MIN_LOD:
		return str_GL_TEXTURE_MIN_LOD;
	default:
		return str_UNKNOWN;
	}
}

/** Returns texture target as literals.
 *
 * @param  target ES texture target.
 *
 * @return requested literal
 */
const char* TextureBorderClampBase::getTexTargetString(glw::GLenum target)
{
	static const char* str_GL_TEXTURE_2D					   = "GL_TEXTURE_2D";
	static const char* str_GL_TEXTURE_2D_ARRAY				   = "GL_TEXTURE_2D_ARRAY";
	static const char* str_GL_TEXTURE_2D_MULTISAMPLE		   = "GL_TEXTURE_2D_MULTISAMPLE";
	static const char* str_GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES = "GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES";
	static const char* str_GL_TEXTURE_3D					   = "GL_TEXTURE_3D";
	static const char* str_GL_TEXTURE_BUFFER_EXT			   = "GL_TEXTURE_BUFFER_EXT";
	static const char* str_GL_TEXTURE_CUBE_MAP				   = "GL_TEXTURE_CUBE_MAP";
	static const char* str_GL_TEXTURE_CUBE_MAP_ARRAY_EXT	   = "GL_TEXTURE_CUBE_MAP_ARRAY_EXT";
	static const char* str_GL_TEXTURE_CUBE_MAP_POSITIVE_X	  = "GL_TEXTURE_CUBE_MAP_POSITIVE_X";
	static const char* str_UNKNOWN							   = "UNKNOWN";

	if (target == m_glExtTokens.TEXTURE_BORDER_COLOR)
	{
		return str_GL_TEXTURE_BUFFER_EXT;
	}

	switch (target)
	{
	case GL_TEXTURE_2D:
		return str_GL_TEXTURE_2D;
	case GL_TEXTURE_3D:
		return str_GL_TEXTURE_3D;
	case GL_TEXTURE_2D_ARRAY:
		return str_GL_TEXTURE_2D_ARRAY;
	case GL_TEXTURE_CUBE_MAP:
		return str_GL_TEXTURE_CUBE_MAP;
	case GL_TEXTURE_2D_MULTISAMPLE:
		return str_GL_TEXTURE_2D_MULTISAMPLE;
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES:
		return str_GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES;
	case GL_TEXTURE_CUBE_MAP_ARRAY:
		return str_GL_TEXTURE_CUBE_MAP_ARRAY_EXT;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
		return str_GL_TEXTURE_CUBE_MAP_POSITIVE_X;
	default:
		return str_UNKNOWN;
	}
}

} // namespace glcts
