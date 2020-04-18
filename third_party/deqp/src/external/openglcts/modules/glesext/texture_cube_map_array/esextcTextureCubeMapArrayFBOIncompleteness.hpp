#ifndef _ESEXTCTEXTURECUBEMAPARRAYFBOINCOMPLETENESS_HPP
#define _ESEXTCTEXTURECUBEMAPARRAYFBOINCOMPLETENESS_HPP
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
 * \file  esextcTextureCubeMapArrayFBOIncompleteness.hpp
 * \brief texture_cube_map_array extenstion - FBO incompleteness (Test 9)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

namespace glcts
{
/**  Implementation part one of (Test 9). Test description follows:
 *
 *   Make sure FBO incompleteness is reported as per specified when at least
 *   one cube-map array texture is used as an attachment.
 *
 *   Category: Coverage.
 *   Priority: Must-have.
 *
 *   (Condition 1)
 *   Make sure that a framebuffer is considered incomplete, if a non-existing
 *   layer (whose index is larger or equal to the layer count) of a cube-map
 *   array texture is attached to any of the framebuffer's non-layered
 *   attachments;
 *
 *   (Condition 2)
 *   Make sure that a framebuffer is considered incomplete if the layer count
 *   of a cube-map array texture that has been bound to a layered framebuffer
 *   is larger than or equal to GL_MAX_FRAMEBUFFER_LAYERS_EXT.
 *
 *   (Condition 3)
 *   Make sure that a layered framebuffer is considered incomplete and its
 *   status is reported as GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT, if
 *   a cube-map array texture level has been bound to its zeroth color attachment
 *   and a 2D array texture level has been bound to its first color attachment.
 **/

class TextureCubeMapArrayFBOIncompleteness : public TestCaseBase
{
public:
	/* Public methods */
	TextureCubeMapArrayFBOIncompleteness(Context& context, const ExtParameters& extParams, const char* name,
										 const char* description);

	virtual ~TextureCubeMapArrayFBOIncompleteness(void)
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	glw::GLboolean checkState(glw::GLint expectedState, const char* message);
	void initTest();

	/* Private static constants */
	static const glw::GLint m_small_texture_depth = 6;
	static const glw::GLint m_texture_levels	  = 1;
	static const glw::GLint m_texture_height	  = 1;
	static const glw::GLint m_texture_width		  = 1;

	/* Variables for general usage */
	glw::GLuint m_fbo_id;
	glw::GLuint m_lots_of_layers_to_id;
	glw::GLuint m_non_layered_to_id;
	glw::GLuint m_small_to_id;
};

} /* glcts */

#endif // _ESEXTCTEXTURECUBEMAPARRAYFBOINCOMPLETENESS_HPP
