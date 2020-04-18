#ifndef _ESEXTCTEXTURECUBEMAPARRAYETC2SUPPORT_HPP
#define _ESEXTCTEXTURECUBEMAPARRAYETC2SUPPORT_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
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
 * \file  esextcTextureCubeMapArrayETC2Support.hpp
 * \brief texture_cube_map_array extension - ETC2 support (Test 11)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

namespace glcts
{
class TextureCubeMapArrayETC2Support : public TestCaseBase
{
public:
	/* Public methods */
	TextureCubeMapArrayETC2Support(glcts::Context& context, const ExtParameters& extParams, const char* name,
								   const char* description);

	virtual ~TextureCubeMapArrayETC2Support()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

protected:
	void prepareFramebuffer();
	void prepareVertexArrayObject();
	void prepareProgram();
	void prepareTexture();
	void draw();
	bool isRenderedImageValid();
	void clean();

private:
	glw::GLuint m_fbo;
	glw::GLuint m_rbo;
	glw::GLuint m_vao;
	glw::GLuint m_texture;
	glw::GLuint m_program;

	static const glw::GLchar  s_vertex_shader[];
	static const glw::GLchar  s_fragment_shader[];
	static const glw::GLubyte s_RGB_texture_data[];
	static const glw::GLsizei s_RGB_texture_data_size;
	static const glw::GLubyte s_compressed_RGB_texture_data[];
	static const glw::GLsizei s_compressed_RGB_texture_data_size;
};

} // namespace glcts

#endif // _ESEXTCTEXTURECUBEMAPARRAYETC2SUPPORT_HPP
