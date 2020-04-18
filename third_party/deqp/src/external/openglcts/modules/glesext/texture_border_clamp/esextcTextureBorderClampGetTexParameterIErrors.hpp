#ifndef _ESEXTCTEXTUREBORDERCLAMPGETTEXPARAMETERIERRORS_HPP
#define _ESEXTCTEXTUREBORDERCLAMPGETTEXPARAMETERIERRORS_HPP
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
 * \file esextcTextureBorderClampGetTexParameterIErrors.hpp
 * \brief Texture Border Clamp glGetTexParameterIivEXT(), glGetTexParameterIuivEXT() Errors (Test 3)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBorderClampBase.hpp"
#include <vector>

namespace glcts
{

/**  Implementation of Test 3 from CTS_EXT_texture_border_clamp. Description follows
 *
 *    Verify glGetTexParameterIivEXT() and glGetTexParameterIuivEXT() report errors
 *    as per spec.
 *
 *    Category: Negative tests,
 *              Optional dependency on EXT_texture_buffer,
 *              Optional dependency on OES_texture_storage_multisample_2d_array.
 *    Priority: Must-have.
 *
 *    Make sure the functions report GL_INVALID_ENUM error if cube-map face
 *    texture targets or GL_TEXTURE_BUFFER_EXT (if supported) are used as
 *    a texture target.
 *
 */
class TextureBorderClampGetTexParameterIErrorsTest : public TextureBorderClampBase
{
public:
	/* Public methods */
	TextureBorderClampGetTexParameterIErrorsTest(Context& context, const ExtParameters& extParams, const char* name,
												 const char* description);

	virtual ~TextureBorderClampGetTexParameterIErrorsTest()
	{
	}

	virtual IterateResult iterate(void);

private:
	/* Private methods */
	virtual void initTest(void);

	void CheckAllNames(glw::GLenum target, glw::GLenum expected_error);
	void VerifyGLGetTexParameterIiv(glw::GLenum target, glw::GLenum pname, glw::GLenum expected_error);
	void VerifyGLGetTexParameterIuiv(glw::GLenum target, glw::GLenum pname, glw::GLenum expected_error);

	/* Private variables */
	glw::GLboolean m_test_passed;

	/* Private static variables */
	static const glw::GLuint m_buffer_size;

	std::vector<glw::GLenum> m_pname_list;
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBORDERCLAMPGETTEXPARAMETERIERRORS_HPP
