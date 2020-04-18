#ifndef _ESEXTCTEXTUREBORDERCLAMPSAMPLERPARAMETERIWITHWRONGPNAME_HPP
#define _ESEXTCTEXTUREBORDERCLAMPSAMPLERPARAMETERIWITHWRONGPNAME_HPP
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
 * \file  esextcTextureBorderClampSamplerParameterIWithWrongPname.hpp
 * \brief Verifies glGetSamplerParameterI*() and glSamplerParameterI*()
 *        entry-points generate GL_INVALID_ENUM error if invalid pnames
 *        are used. (Test 5)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBorderClampBase.hpp"

namespace glcts
{

/**  Implementation of Test 5 from CTS_EXT_texture_border_clamp. Description follows
 *
 *    Verify glGetSamplerParameterIivEXT(), glGetSamplerParameterIuivEXT(),
 *    glSamplerParameterIivEXT() and glSamplerParameterIuivEXT() generate
 *    GL_INVALID_ENUM error if invalid pnames are used.
 *
 *    Category: Negative tests;
 *    Priority: Must-have.
 *
 *    The test should verify the error is generated, when each function is
 *    called for the following pnames:
 *
 *    - GL_TEXTURE_IMMUTABLE_FORMAT;
 *    - GL_TEXTURE_BASE_LEVEL;
 *    - GL_TEXTURE_MAX_LEVEL;
 *    - GL_TEXTURE_SWIZZLE_R;
 *    - GL_TEXTURE_SWIZZLE_G;
 *    - GL_TEXTURE_SWIZZLE_B;
 *    - GL_TEXTURE_SWIZZLE_A;
 *
 */
class TextureBorderClampSamplerParameterIWithWrongPnameTest : public TextureBorderClampBase
{
public:
	/* Public functions */
	TextureBorderClampSamplerParameterIWithWrongPnameTest(Context& context, const ExtParameters& extParams,
														  const char* name, const char* description);

	virtual ~TextureBorderClampSamplerParameterIWithWrongPnameTest()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private functions */
	void initTest(void);

	void VerifyGLGetSamplerParameterIiv(glw::GLuint sampler_id, glw::GLenum pname, glw::GLenum expected_error);

	void VerifyGLGetSamplerParameterIuiv(glw::GLuint sampler_id, glw::GLenum pname, glw::GLenum expected_error);

	void VerifyGLSamplerParameterIiv(glw::GLuint sampler_id, glw::GLenum pname, glw::GLenum expected_error);

	void VerifyGLSamplerParameterIuiv(glw::GLuint sampler_id, glw::GLenum pname, glw::GLenum expected_error);

	/* Private variables */
	glw::GLuint	m_sampler_id;
	glw::GLboolean m_test_passed;

	std::vector<glw::GLenum> m_pnames_list;

	/* Private static constants */
	static const glw::GLuint m_buffer_size;
	static const glw::GLuint m_texture_unit_index;
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBORDERCLAMPSAMPLERPARAMETERIWITHWRONGPNAME_HPP
