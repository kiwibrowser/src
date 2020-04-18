#ifndef _ESEXTCTEXTUREBORDERCLAMPPARAMETERTEXTUREBORDERCOLOR_HPP
#define _ESEXTCTEXTUREBORDERCLAMPPARAMETERTEXTUREBORDERCOLOR_HPP
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
 * \file  esextcTextureBorderClampParameterTextureBorderColor.hpp
 * \brief Verify that GL_TEXTURE_BORDER_COLOR_EXT state is correctly retrieved
 *        by glGetSamplerParameter*() and glGetTexParameter*() functions. (Test 6)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBorderClampBase.hpp"

namespace glcts
{
/**  Implementation of Test 6 from CTS_EXT_texture_border_clamp. Description follows
 *
 *    Verify that GL_TEXTURE_BORDER_COLOR_EXT state is correctly retrieved
 *    by glGetSamplerParameter*() and glGetTexParameter*() functions.
 *
 *    Category: Functional tests;
 *              Optional dependency on EXT_texture_cube_map_array.
 *    Priority: Must-have.
 *
 *    Verify default border color is set to (0.0, 0.0, 0.0, 0.0) and
 *    (0, 0, 0, 0).
 *
 *    Verify setting signed integer border color of (-1, -2, 3, 4) using
 *    glSamplerParameterIivEXT() / glTexParameterIivEXT() call affects the values
 *    later reported by glGetSamplerParameterIivEXT() and glGetTexParameterIivEXT().
 *    These values should match.
 *
 *    Verify setting unsigned integer border color of (1, 2, 3, 4) using
 *    glSamplerParameterIuivEXT() / glTexParameterIuivEXT() call affects the values
 *    later reported by glGetSamplerParameterIuivEXT() and glGetTexParameterIuivEXT().
 *    These values should match.
 *
 *    Verify setting floating-point border color of (0.1, 0.2, 0.3, 0.4)
 *    affects the values later reported by glGetSamplerParameterfv() /
 *    glGetTexParameterfv(). These values should match.
 *
 *    Verify setting integer border color of
 *    (0, 1, 2, 4) using glSamplerParameteriv()
 *    / glTexParameteriv() affects the values later reported by
 *    glGetSamplerParameteriv() / glGetTexParameteriv(). The returned values
 *    should correspond to the outcome of equation 2.2 from ES3.0.2 spec
 *    applied to each component.
 *
 *    All texture targets available in ES3.1, as well as
 *    GL_TEXTURE_CUBE_MAP_ARRAY_EXT (if supported).
 */
class TextureBorderClampParameterTextureBorderColor : public TextureBorderClampBase
{
public:
	/* Public methods */
	TextureBorderClampParameterTextureBorderColor(Context& context, const ExtParameters& extParams, const char* name,
												  const char* description);

	virtual ~TextureBorderClampParameterTextureBorderColor()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	void initTest(void);

	bool verifyGLGetSamplerParameterfvResult(glw::GLuint sampler_id, glw::GLenum target,
											 const glw::GLfloat* expected_data);

	bool verifyGLGetSamplerParameterivResult(glw::GLuint sampler_id, glw::GLenum target,
											 const glw::GLint* expected_data);

	bool verifyGLGetSamplerParameterIivResult(glw::GLuint sampler_id, glw::GLenum target,
											  const glw::GLint* expected_data);

	bool verifyGLGetSamplerParameterIuivResult(glw::GLuint sampler_id, glw::GLenum target,
											   const glw::GLuint* expected_data);

	bool verifyGLGetTexParameterfvResult(glw::GLenum target, const glw::GLfloat* expected_data);

	bool verifyGLGetTexParameterivResult(glw::GLenum target, const glw::GLint* expected_data);

	bool verifyGLGetTexParameterIivResult(glw::GLenum target, const glw::GLint* expected_data);

	bool verifyGLGetTexParameterIuivResult(glw::GLenum target, const glw::GLuint* expected_data);

	/* Private variables */
	glw::GLuint				 m_sampler_id;
	std::vector<glw::GLenum> m_texture_targets;
	glw::GLuint				 m_to_id;

	/* Private static constants */
	static const glw::GLuint m_buffer_length;
	static const glw::GLuint m_texture_unit_index;
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBORDERCLAMPPARAMETERTEXTUREBORDERCOLOR_HPP
