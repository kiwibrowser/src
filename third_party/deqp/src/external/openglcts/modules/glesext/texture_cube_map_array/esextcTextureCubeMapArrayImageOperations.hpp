#ifndef _ESEXTCTEXTURECUBEMAPARRAYIMAGEOPERATIONS_HPP
#define _ESEXTCTEXTURECUBEMAPARRAYIMAGEOPERATIONS_HPP
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
 * \file  esextcTextureCubeMapArrayImageOperations.hpp
 * \brief texture_cube_map_array extension - Image Operations (Test 8)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"
#include <map>
#include <vector>

namespace glcts
{

/** Implementation of (Test 8) for CTS_EXT_texture_cube_map_array. Test description follows:
 *
 *   Make sure cube-map array textures work correctly when used as images.
 *
 *   Category: Functionality tests,
 *             Optional dependency on EXT_geometry_shader;
 *             Optional dependency on EXT_tessellation_shader.
 *   Priority: Must-have.
 *
 *   Make sure that read and write operations performed on images bound to
 *   image units, to which cube-map array textures have been bound, work
 *   correctly from:
 *
 *   * a compute shader;                 (required)
 *   * a fragment shader;                (if supported)
 *   * a geometry shader;                (if supported)
 *   * a tessellation control shader;    (if supported)
 *   * a tessellation evaluation shader; (if supported)
 *   * a vertex shader.                  (if supported)
 *
 *   Each shader stage should read data from first image and store read data to
 *   the second image. Next test should verify if data in second image is the
 *   same as stored in the first image.
 *
 *   The test should use the following image samplers (whichever applies for
 *   image considered):
 *
 *   * iimageCubeArray;
 *   * imageCubeArray;
 *   * uimageCubeArray;
 *
 *   Following texture resolutions should be  used
 *   { 32, 32, 12},
 *   { 64, 64, 6},
 *   { 16, 16, 18},
 *   { 16, 16,  6}
 *
 *   Both immutable and mutable textures should be checked.
 *
 */

/* Define allowed test variants for shaders */
enum SHADER_TO_CHECK
{
	STC_COMPUTE_SHADER,
	STC_FRAGMENT_SHADER,
	STC_GEOMETRY_SHADER,
	STC_TESSELLATION_CONTROL_SHADER,
	STC_TESSELLATION_EVALUATION_SHADER,
	STC_VERTEX_SHADER
};

/* Location of dimensions in array StorageConfigIOC::m_dimensions */
enum DIMENSIONS_LOCATION
{
	DL_WIDTH  = 0,
	DL_HEIGHT = 1,
	DL_DEPTH  = 2
};

/* Define tested images formats */
enum IMAGE_FORMATS
{
	IF_IMAGE  = 0,
	IF_IIMAGE = 1,
	IF_UIMAGE = 2
};

class TextureCubeMapArrayImageOpCompute : public TestCaseBase
{
public:
	/* Public methods */
	TextureCubeMapArrayImageOpCompute(Context& context, const ExtParameters& extParams, const char* name,
									  const char* description, SHADER_TO_CHECK shaderToCheck);

	virtual ~TextureCubeMapArrayImageOpCompute(void)
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

	/* Public static constants */
	static const glw::GLfloat m_f_base;
	static const glw::GLint   m_i_base;
	static const glw::GLuint  m_ui_base;
	static const glw::GLuint  m_n_components;
	static const glw::GLuint  m_n_dimensions;
	static const glw::GLuint  m_n_image_formats;
	static const glw::GLuint  m_n_resolutions;
	static const glw::GLuint  m_n_storage_type;
	static const char*		  m_mutable_storage;
	static const char*		  m_immutable_storage;

private:
	/* Private methods */
	void initTest(void);
	void removeTextures(void);
	void configureProgram(void);
	void runShaders(glw::GLuint width, glw::GLuint height, glw::GLuint depth);

	const char* getComputeShaderCode(void);
	const char* getFragmentShaderCode(void);
	const char* getFragmentShaderCodeBoilerPlate(void);
	const char* getGeometryShaderCode(void);
	const char* getTessControlShaderCode(void);
	const char* getTessControlShaderCodeBoilerPlate(void);
	const char* getTessEvaluationShaderCode(void);
	const char* getTessEvaluationShaderCodeBoilerPlate(void);
	const char* getVertexShaderCode(void);
	const char* getVertexShaderCodeBoilerPlate(void);
	const char* getFloatingPointCopyShaderSource(void);

	/* Variables for general usage */
	SHADER_TO_CHECK m_shader_to_check;

	glw::GLuint m_cs_id;
	glw::GLuint m_fbo_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_tc_id;
	glw::GLuint m_te_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_id;
	glw::GLuint m_copy_po_id;
	glw::GLuint m_copy_cs_id;

	glw::GLuint m_iimage_read_to_id;
	glw::GLuint m_iimage_write_to_id;
	glw::GLuint m_image_read_to_id;
	glw::GLuint m_image_write_to_id;
	glw::GLuint m_uimage_read_to_id;
	glw::GLuint m_uimage_write_to_id;
};

} /* glcts */

#endif // _ESEXTCTEXTURECUBEMAPARRAYIMAGEOPERATIONS_HPP
