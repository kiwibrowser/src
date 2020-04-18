#ifndef _ESEXTCTEXTUREBORDERCLAMPTEXPARAMETERIERRORS_HPP
#define _ESEXTCTEXTUREBORDERCLAMPTEXPARAMETERIERRORS_HPP
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
 * \file esextcTextureBorderClampTexParameterIErrors.hpp
 * \brief Texture Border Clamp glTexParameterIivEXT(), glTexParameterIuivEXT() Errors (Test 2)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBorderClampBase.hpp"

namespace glcts
{
/**  Implementation of Test 2 from CTS_EXT_texture_border_clamp. Description follows
 *
 *    Verify glTexParameterIivEXT(), glTexParameterIuivEXT() report errors as per
 *    spec.
 *
 *    Category: Negative tests,
 *              Optional dependency on EXT_texture_buffer;
 *              Optional dependency on EXT_texture_cube_map_array;
 *              Optional dependency on OES_texture_storage_multisample_2d_array.
 *    Priority: Must-have.
 *
 *    Make sure that the functions report GL_INVALID_ENUM error if cube-map
 *    face or GL_TEXTURE_BUFFER_EXT texture targets (if supported) is issued as
 *    a texture target.
 *
 *    Make sure that the functions report GL_INVALID_ENUM error if
 *    GL_TEXTURE_IMMUTABLE_FORMAT is passed by pname argument.
 *
 *    Make sure that the functions report GL_INVALID_VALUE error if the following
 *    pname+value combinations are used:
 *
 *    - GL_TEXTURE_BASE_LEVEL       + -1; (iv() version only)
 *    - GL_TEXTURE_MAX_LEVEL        + -1; (iv() version only)

 *    Make sure that the functions report GL_INVALID_ENUM error if the following
 *    pname+value combinations are used:
 *
 *    - GL_TEXTURE_COMPARE_MODE + GL_NEAREST;
 *    - GL_TEXTURE_COMPARE_FUNC + GL_NEAREST;
 *    - GL_TEXTURE_MAG_FILTER   + GL_NEAREST_MIPMAP_NEAREST;
 *    - GL_TEXTURE_MIN_FILTER   + GL_RED;
 *    - GL_TEXTURE_SWIZZLE_R    + GL_NEAREST;
 *    - GL_TEXTURE_SWIZZLE_G    + GL_NEAREST;
 *    - GL_TEXTURE_SWIZZLE_B    + GL_NEAREST;
 *    - GL_TEXTURE_SWIZZLE_A    + GL_NEAREST;
 *    - GL_TEXTURE_WRAP_S       + GL_RED;
 *    - GL_TEXTURE_WRAP_T       + GL_RED;
 *    - GL_TEXTURE_WRAP_R       + GL_RED;
 *
 *    Make sure that the functions report GL_INVALID_ENUM error if the following
 *    pname+value pairs are used for GL_TEXTURE_2D_MULTISAMPLE or
 *    GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES targets:
 *
 *    - GL_TEXTURE_COMPARE_MODE + GL_NONE;
 *    - GL_TEXTURE_COMPARE_FUNC + GL_LEQUAL;
 *    - GL_TEXTURE_MAG_FILTER   + GL_LINEAR;
 *    - GL_TEXTURE_MAX_LOD   *  + 1000;
 *    - GL_TEXTURE_MIN_FILTER   + GL_NEAREST_MIPMAP_LINEAR;
 *    - GL_TEXTURE_MIN_LOD   *  + -1000;
 *    - GL_TEXTURE_WRAP_S   *   + GL_REPEAT;
 *    - GL_TEXTURE_WRAP_T   *   + GL_REPEAT;
 *    - GL_TEXTURE_WRAP_R   *   + GL_REPEAT;
 *
 *    All texture targets available in ES3.1, as well as
 *    GL_TEXTURE_CUBE_MAP_ARRAY_EXT (if supported) should be checked (for cases
 *    2 and 3);
 */
class TextureBorderClampTexParameterIErrorsTest : public TextureBorderClampBase
{
public:
	/* Public functions */
	TextureBorderClampTexParameterIErrorsTest(Context& context, const ExtParameters& extParams, const char* name,
											  const char* description);

	virtual ~TextureBorderClampTexParameterIErrorsTest()
	{
	}

	virtual IterateResult iterate(void);

private:
	/* Private functions */
	virtual void initTest(void);

	void VerifyGLTexParameterIiv(glw::GLenum target, glw::GLenum pname, glw::GLint params, glw::GLenum expected_error);

	void VerifyGLTexParameterIivMultipleAcceptedErrors(glw::GLenum target, glw::GLenum pname, glw::GLint params,
													   glw::GLenum expected_error1, glw::GLenum expected_error2);

	void VerifyGLTexParameterIivForAll(glw::GLenum pname, glw::GLint params, glw::GLenum expected_error);

	void VerifyGLTexParameterIivTextureBaseLevelForAll(glw::GLenum pname, glw::GLint params,
													   glw::GLenum expected_error);

	void VerifyGLTexParameterIuiv(glw::GLenum target, glw::GLenum pname, glw::GLuint params,
								  glw::GLenum expected_error);

	void VerifyGLTexParameterIuivForAll(glw::GLenum pname, glw::GLuint params, glw::GLenum expected_error);

	/* Private variables */
	glw::GLboolean m_test_passed;
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBORDERCLAMPTEXPARAMETERIERRORS_HPP
