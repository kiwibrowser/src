#ifndef _ESEXTCTEXTUREBORDERCLAMPBASE_HPP
#define _ESEXTCTEXTUREBORDERCLAMPBASE_HPP
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
 * \file  esextcTextureBorderClampBase.hpp
 * \brief Base Class for Texture Border Clamp extension tests 1-6.
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"
#include <string.h>
#include <vector>

namespace glcts
{
/** Base Class for Texture Border Clamp Tests 1-6
 *  Takes care of initializing all texture objects necessary
 *  to run these tests.
 **/
class TextureBorderClampBase : public TestCaseBase
{
public:
	/* Public methods */
	TextureBorderClampBase(Context& context, const ExtParameters& extParams, const char* name, const char* description);

	virtual ~TextureBorderClampBase()
	{
	}

	virtual void deinit(void);
	virtual void initTest(void);

protected:
	/* Protected methods */
	void deinitAllTextures(void);
	void initAllTextures(void);

	const char* getPNameString(glw::GLenum pname);
	const char* getTexTargetString(glw::GLenum target);

	/* Protected variables */
	glw::GLuint m_texture_2D_array_id;
	glw::GLuint m_texture_2D_id;
	glw::GLuint m_texture_2D_multisample_array_id;
	glw::GLuint m_texture_2D_multisample_id;
	glw::GLuint m_texture_3D_id;
	glw::GLuint m_texture_buffer_id;
	glw::GLuint m_texture_cube_map_id;
	glw::GLuint m_texture_cube_map_array_id;

	std::vector<glw::GLenum> m_texture_target_list;
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBORDERCLAMPBASE_HPP
