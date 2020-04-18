#ifndef _ESEXTCTEXTURECUBEMAPARRAYSTENCILATTACHMENTS_HPP
#define _ESEXTCTEXTURECUBEMAPARRAYSTENCILATTACHMENTS_HPP
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
 * \file  esextcTextureCubeMapArrayStencilAttachments.hpp
 * \brief Texture Cube Map Array Stencil Attachments (Test 3)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

namespace glcts
{

/** Class used to store configuration and data for different texture cube map array textures
 */
class CubeMapArrayDataStorage
{
public:
	/* Public functions */
	CubeMapArrayDataStorage();
	~CubeMapArrayDataStorage();

	void deinit(void);
	void init(const glw::GLuint width, const glw::GLuint height, const glw::GLuint depth,
			  glw::GLubyte initial_value = 0);

	inline glw::GLuint getDepth() const
	{
		return m_depth;
	}
	inline glw::GLuint getHeight() const
	{
		return m_height;
	}
	inline glw::GLuint getWidth() const
	{
		return m_width;
	}

	glw::GLuint   getArraySize() const;
	glw::GLubyte* getDataArray() const;

protected:
	/* Protected variables */
	glw::GLubyte* m_data_array;
	glw::GLuint   m_depth;
	glw::GLuint   m_height;
	glw::GLuint   m_width;
};

/**  Implementation of Test 3 from CTS_EXT_texture_cube_map_array.
 *   Test description follows:
 *
 *   Make sure cube-map texture layers can be used as stencil attachments.
 *
 *   Category: Functionality tests,
 *             Optional dependency on EXT_geometry_shader;
 *   Priority: Must-have.
 *
 *   Verify that a cube-map array texture layers carrying stencil data
 *   can be used as a stencil attachment for a framebuffer object.
 *
 *   Clear the color buffer with (1, 0, 0, 1) color. Fill the upper half of
 *   the stencil buffer with 1 and the bottom half of the stencil buffer with 0.
 *
 *   Configure the stencil test so that it the test passes only if the stencil
 *   buffer data for each fragment is larger than 0.
 *   The fragment shader should set the only output variable to (0, 1, 1, 0).
 *
 *   The test should render a full-screen quad and then check the outcome of
 *   the operation. The test passes if top half is filled with (0, 1, 1, 0)
 *   and bottom half is set to (1, 0, 0, 1).
 *
 *   Rendering to both layered (if supported) and non-layered framebuffer
 *   objects should be verified.
 *
 *   Test four different cube-map array texture resolutions, as described
 *   in test 1. Both immutable and mutable textures should be checked.
 */
class TextureCubeMapArrayStencilAttachments : public TestCaseBase
{
public:
	/* Public functions */
	TextureCubeMapArrayStencilAttachments(Context& context, const ExtParameters& extParams, const char* name,
										  const char* description, glw::GLboolean immutable_storage,
										  glw::GLboolean fbo_layered);

	virtual ~TextureCubeMapArrayStencilAttachments()
	{
	}

	virtual IterateResult iterate(void);
	virtual void		  deinit(void);

	/* Public variables */
	static const glw::GLuint m_n_components;
	static const glw::GLuint m_n_cube_map_array_configurations;
	static const glw::GLuint m_n_vertices_gs;

private:
	/* Private variables */
	static const char* m_fragment_shader_code;
	static const char* m_vertex_shader_code;

	glw::GLboolean m_fbo_layered;
	glw::GLboolean m_immutable_storage;

	glw::GLuint m_fbo_draw_id;
	glw::GLuint m_fbo_read_id;
	glw::GLuint m_fragment_shader_id;
	glw::GLuint m_geometry_shader_id;
	glw::GLuint m_program_id;
	glw::GLuint m_texture_cube_array_stencil_id;
	glw::GLuint m_texture_cube_array_color_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vbo_id;
	glw::GLuint m_vertex_shader_id;

	CubeMapArrayDataStorage* m_cube_map_array_data;
	glw::GLubyte*			 m_result_data;

	/* Private functions */
	void buildAndUseProgram(glw::GLuint test_index);
	void checkFramebufferStatus(glw::GLenum framebuffer_status);
	void cleanAfterTest(void);
	void createImmutableCubeArrayColor(glw::GLuint test_index);
	void createImmutableCubeArrayStencil(glw::GLuint test_index);
	void createMutableCubeArrayColor(glw::GLuint test_index);
	void createMutableCubeArrayStencil(glw::GLuint test_index);
	void fillStencilData(glw::GLuint test_index);
	std::string getGeometryShaderCode(const std::string& max_vertices, const std::string& n_layers);
	void initTest(void);
	void initTestIteration(glw::GLuint test_index);
	bool readPixelsAndCompareWithExpectedResult(glw::GLuint test_index);
	void setupLayeredFramebuffer(void);
	void setupNonLayeredFramebuffer(void);
};

} // namespace glcts

#endif // _ESEXTCTEXTURECUBEMAPARRAYSTENCILATTACHMENTS_HPP
