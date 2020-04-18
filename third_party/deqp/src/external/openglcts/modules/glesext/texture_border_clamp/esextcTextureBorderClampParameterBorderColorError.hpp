#ifndef _ESEXTCTEXTUREBORDERCLAMPPARAMETERBORDERCOLORERROR_HPP
#define _ESEXTCTEXTUREBORDERCLAMPPARAMETERBORDERCOLORERROR_HPP
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
 * \file esextcTextureBorderClampParameterBorderColorError.hpp
 * \brief Texture Border Clamp Border Color Error (Test 1)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBorderClampBase.hpp"

namespace glcts
{

/** Implementation of Test 1 from CTS_EXT_texture_border_clamp. Description follows
 *
 *   Verify glTexParameterf(), glTexParameteri(), glSamplerParameterf(),
 *   glSamplerParameteri() report errors as per spec.
 *
 *   Category: Negative tests,
 *             Optional dependency on EXT_texture_cube_map_array.
 *   Priority: Must-have.
 *
 *   Make sure that the functions report GL_INVALID_ENUM if
 *   each function is called for GL_TEXTURE_BORDER_COLOR_EXT texture
 *   parameter.
 *
 *   All texture targets available in ES3.1, as well as
 *   GL_TEXTURE_CUBE_MAP_ARRAY_EXT (if supported) should be checked
 */
class TextureBorderClampParameterBorderColorErrorTest : public TextureBorderClampBase
{
public:
	/* Public functions */
	TextureBorderClampParameterBorderColorErrorTest(Context& context, const ExtParameters& extParams, const char* name,
													const char* description);

	virtual ~TextureBorderClampParameterBorderColorErrorTest()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private functions */
	void initTest(void);

	void VerifyGLTexParameterf(glw::GLenum target, glw::GLenum pname, glw::GLfloat param, glw::GLenum expected_error);

	void VerifyGLTexParameteri(glw::GLenum target, glw::GLenum pname, glw::GLint param, glw::GLenum expected_error);

	void VerifyGLSamplerParameterf(glw::GLenum pname, glw::GLfloat param, glw::GLenum expected_error);

	void VerifyGLSamplerParameteri(glw::GLenum pname, glw::GLint param, glw::GLenum expected_error);

	/* Private variables */
	glw::GLuint	m_sampler_id;
	glw::GLboolean m_test_passed;

	/* Private static variables */
	static const glw::GLuint m_texture_unit;
};

} // namespace glcts
#endif // _ESEXTCTEXTUREBORDERCLAMPPARAMETERBORDERCOLORERROR_HPP
