#ifndef _ESEXTCTEXTURECUBEMAPARRAYGETTERCALLS_HPP
#define _ESEXTCTEXTURECUBEMAPARRAYGETTERCALLS_HPP
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
 * \file  esextcTextureCubeMapArrayGetterCalls.hpp
 * \brief Texture Cube Map Array Getter Calls (Test 6)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"
#include <map>
#include <vector>

namespace glcts
{
/*   Implementation of Test 6 from CTS_EXT_texture_cube_map_array. Test description follows:
 *
 *   Make sure getter calls work correctly in regard to cube-map array textures.
 *
 *   Category: API;
 *   Priority: Must-have.
 *
 *   Consider a cube-map array texture. Make sure that getter calls operating
 *   on the texture object report valid default properties for a newly created
 *   and initialized cube-map array texture.
 *   Modify texture properties of the object and make sure valid values are
 *   reported by the getter calls.
 *   Getter calls to consider: glGetTexParameterfv(), glGetTexParameteriv().
 *
 *   Make sure that GL_TEXTURE_CUBE_MAP_ARRAY_EXT texture binding is reported
 *   correctly by glGetBooleanv(), glGetFloatv() and glGetIntegerv().
 *
 *   Make sure that glGetTexLevelParameterfv() and glGetTexLevelParameteriv()
 *   do not report an error, when all relevant properties defined for the
 *   GL_TEXTURE_CUBE_MAP_ARRAY_EXT texture target are queried.
 *
 *   Both immutable and mutable textures should be checked
 */
class TextureCubeMapArrayGetterCalls : public TestCaseBase
{
public:
	/* Public functions */
	TextureCubeMapArrayGetterCalls(Context& context, const ExtParameters& extParams, const char* name,
								   const char* description);

	virtual ~TextureCubeMapArrayGetterCalls()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private functions */
	void initTest(void);
	void verifyTextureBindings(void);
	void verifyGetTexParameter(glw::GLenum pname, glw::GLint expected_value);
	void verifyGetTexLevelParameters(void);

	static const char* getStringForGetTexParameterPname(glw::GLenum pname);
	static const char* getStringForGetTexLevelParameterPname(glw::GLenum pname);

	/* Private variables */
	glw::GLuint	m_to_id;
	glw::GLboolean m_test_passed;

	static const glw::GLuint  m_depth;
	static const glw::GLsizei m_height;
	static const glw::GLsizei m_width;
	static const glw::GLuint  m_n_components;

	/* Values represented by variables below are used by
	 * verifyTextureLevelParameterValues() */
	glw::GLint m_expected_alpha_size;
	glw::GLint m_expected_alpha_type;
	glw::GLint m_expected_blue_size;
	glw::GLint m_expected_blue_type;
	glw::GLint m_expected_compressed;
	glw::GLint m_expected_depth_size;
	glw::GLint m_expected_depth_type;
	glw::GLint m_expected_green_size;
	glw::GLint m_expected_green_type;
	glw::GLint m_expected_red_size;
	glw::GLint m_expected_red_type;
	glw::GLint m_expected_shared_size;
	glw::GLint m_expected_stencil_size;
	glw::GLint m_expected_texture_internal_format;

	typedef std::map<glw::GLenum, glw::GLint> PNamesMap;
	typedef std::vector<glw::GLenum> PNamesVec;

	PNamesMap pnames_for_gettexparameter_default;
	PNamesMap pnames_for_gettexparameter_modified;
	PNamesVec pnames_for_gettexlevelparameter;
};

} // namespace glcts

#endif // _ESEXTCTEXTURECUBEMAPARRAYGETTERCALLS_HPP
