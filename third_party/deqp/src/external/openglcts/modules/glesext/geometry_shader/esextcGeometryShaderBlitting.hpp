#ifndef _ESEXTCGEOMETRYSHADERBLITTING_HPP
#define _ESEXTCGEOMETRYSHADERBLITTING_HPP
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

#include "../esextcTestCaseBase.hpp"
#include "glwEnums.hpp"

namespace glcts
{

/** Base class for implementation of "Group 9" tests from CTS_EXT_geometry_shader.*/
class GeometryShaderBlitting : public TestCaseBase
{
public:
	/* Public methods */
	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

protected:
	GeometryShaderBlitting(Context& context, const ExtParameters& extParams, const char* name, const char* description);

	virtual ~GeometryShaderBlitting(void)
	{
	}

	/* Protected abstract methods */
	virtual void setUpFramebuffersForRendering(glw::GLuint fbo_draw_id, glw::GLuint fbo_read_id, glw::GLint to_draw,
											   glw::GLint to_read) = 0;

private:
	/* Private variables */
	glw::GLenum m_draw_fbo_completeness;
	glw::GLenum m_read_fbo_completeness;
	glw::GLuint m_fbo_draw_id;
	glw::GLuint m_fbo_read_id;
	glw::GLuint m_to_draw;
	glw::GLuint m_to_read;
	glw::GLuint m_vao_id;
};

/**
 * 1. Only layer zero should be used when blitting from a layered read
 *    framebuffer to a non-layered draw framebuffer.
 *
 *    Category: API;
 *              Functional Test.
 *
 *    A 3D texture object A of resolution 4x4x4 and of internalformat GL_RGBA8
 *    should be created:
 *
 *    - First slice should be filled with  (255, 0,   0,   0);
 *    - Second slice should be filled with (0,   255, 0,   0);
 *    - Third slice should be filled with  (0,   0,   255, 0);
 *    - Fourth slice should be filled with (0,   0,   0,   255);
 *
 *    Source FBO should use:
 *
 *    - glFramebufferTextureEXT() to bind level 0 of 3D texture A for color
 *      attachment zero;
 *
 *    For destination FBO, create texture object A' of exactly the same type
 *    as A. The difference should be in content - all layers and slices should
 *    be filled with (0, 0, 0, 0).
 *
 *    Destination FBO should use:
 *
 *    - glFramebufferTextureLayer() to bind level 0 of texture A'.
 *
 *    After blitting, the test should confirm that only zero layer/slice of
 *    destination FBO's attachments have been changed. Remaining layers and
 *    slices should remain black.
 */
class GeometryShaderBlittingLayeredToNonLayered : public GeometryShaderBlitting
{
public:
	/* Public methods */
	GeometryShaderBlittingLayeredToNonLayered(Context& context, const ExtParameters& extParams, const char* name,
											  const char* description);

	virtual ~GeometryShaderBlittingLayeredToNonLayered(void)
	{
	}

protected:
	/* Protected methods */
	void setUpFramebuffersForRendering(glw::GLuint fbo_draw_id, glw::GLuint fbo_read_id, glw::GLint to_draw,
									   glw::GLint to_read);
};

/**
 * 2. Only layer zero should be used when blitting from a non-layered read
 *    framebuffer to a layered draw framebuffer.
 *
 *    Category: API;
 *              Functional Test.
 *
 *    Modify test case 9.1 so that:
 *
 *    - Original source FBO becomes a destination FBO;
 *    - Original destination FBO becomes a source FBO;
 *    - Texture objects A' and B' are filled with data as original textures
 *      A and B;
 *    - Texture objects A and B are filled with data as original textures
 *      A' and B'.
 */
class GeometryShaderBlittingNonLayeredToLayered : public GeometryShaderBlitting
{
public:
	/* Public methods */
	GeometryShaderBlittingNonLayeredToLayered(Context& context, const ExtParameters& extParams, const char* name,
											  const char* description);

	virtual ~GeometryShaderBlittingNonLayeredToLayered(void)
	{
	}

protected:
	/* Protected methods */
	void setUpFramebuffersForRendering(glw::GLuint fbo_draw_id, glw::GLuint fbo_read_id, glw::GLint to_draw,
									   glw::GLint to_read);
};

/**
 * 3. Only layer zero should be used when blitting from layered read framebuffer
 *    to a layered draw framebuffer.
 *
 *    Category: API;
 *              Functional Test.
 *
 *    Modify test case 9.1 so that:
 *
 *    - Texture objects A' and B' should be configured in exactly the same way
 *      as A and B (they should be layered!), however they should still carry
 *      the same data as described in test case 9.1.
 */
class GeometryShaderBlittingLayeredToLayered : public GeometryShaderBlitting
{
public:
	/* Public methods */
	GeometryShaderBlittingLayeredToLayered(Context& context, const ExtParameters& extParams, const char* name,
										   const char* description);

	virtual ~GeometryShaderBlittingLayeredToLayered(void)
	{
	}

protected:
	/* Protected methods */
	void setUpFramebuffersForRendering(glw::GLuint fbo_draw_id, glw::GLuint fbo_read_id, glw::GLint to_draw,
									   glw::GLint to_read);
};

} /* namespace glcts */

#endif // _ESEXTCGEOMETRYSHADERBLITTING_HPP
