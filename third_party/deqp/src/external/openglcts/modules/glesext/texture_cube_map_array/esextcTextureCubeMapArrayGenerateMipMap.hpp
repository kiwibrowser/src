#ifndef _ESEXTCTEXTURECUBEMAPARRAYGENERATEMIPMAP_HPP
#define _ESEXTCTEXTURECUBEMAPARRAYGENERATEMIPMAP_HPP
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
 * \file  esextcTextureCubeMapArrayGenerateMipMap.hpp
 * \brief texture_cube_map_array extenstion - glGenerateMipmap() (Test 7)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"
#include "gluStrUtil.hpp"
#include <vector>

namespace glcts
{
/* Represents a specific texture storage configuration */
struct StorageConfig
{
	glw::GLuint m_depth;
	glw::GLuint m_height;
	glw::GLuint m_levels;
	glw::GLuint m_to_id;
	glw::GLuint m_width;
};

/**  Implementation of the first four paragraphs of Texture Cube Map Array Test 7.
 *   Description follows:
 *
 *   Make sure glGenerateMipmap() works as specified for cube-map array
 *   textures.
 *
 *   Category: Functionality tests, Coverage.
 *   Priority: Must-have.
 *
 *   Make sure that mip-map generation works correctly for cube-map array
 *   textures. Use texture resolutions:
 *
 *         [width x height x depth]
 *      1)   64   x   64   x  18;
 *      2)  117   x  117   x   6;
 *      3)  256   x  256   x   6;
 *      4)  173   x  173   x  12;
 *
 *   Both mutable and immutable textures should be checked (except as noted below).
 *
 *   1. For each layer-face, the test should initialize a base mip-map of iteration-
 *      -specific resolution with a checkerboard pattern filled with two colors.
 *
 *      The color pair used for the pattern should be different for each
 *      layer-face considered. The test should then call glGenerateMipmap().
 *      This part of the test passes if base mip-map was left unchanged and the
 *      lowest level mip-map is not set to either of the colors used.
 **/

class TextureCubeMapArrayGenerateMipMapFilterable : public TestCaseBase
{
public:
	/* Public methods */
	TextureCubeMapArrayGenerateMipMapFilterable(Context& context, const ExtParameters& extParams, const char* name,
												const char* description, STORAGE_TYPE storageType);

	virtual ~TextureCubeMapArrayGenerateMipMapFilterable(void)
	{
	}

	void		  deinit(void);
	void		  init(void);
	IterateResult iterate(void);

private:
	/* Private method */
	void generateTestData(int face, unsigned char* data, int width, int height);
	void initTest();

	/* Private constants */
	static const int		   m_n_colors_per_layer_face = 2;
	static const int		   m_n_components			 = 4;
	static const int		   m_n_max_faces			 = 18;
	static const unsigned char m_layer_face_data[m_n_max_faces][m_n_colors_per_layer_face][m_n_components];

	/* Variables for general usage */
	glw::GLuint				   m_fbo_id;
	std::vector<StorageConfig> m_storage_configs;
	STORAGE_TYPE			   m_storage_type;

	unsigned char* m_reference_data_ptr;
	unsigned char* m_rendered_data_ptr;
};

/** Implementats the last paragraph of Texture Cube Map Array Test 7.
 *  Description follows:
 *
 *  3. Make sure that GL_INVALID_OPERATION error is generated if the texture
 *     bound to target is not cube array complete. To check this, configure
 *     a cube-map array texture object so that its faces use a non-filterable
 *     internalformat, and the magnification filter parameter for the object
 *     is set to LINEAR. This test should be executed for both mutable and
 *     immutable textures.
 */
class TextureCubeMapArrayGenerateMipMapNonFilterable : public TestCaseBase
{
public:
	/* Public methods */
	TextureCubeMapArrayGenerateMipMapNonFilterable(Context& context, const ExtParameters& extParams, const char* name,
												   const char* description, STORAGE_TYPE storageType);

	virtual ~TextureCubeMapArrayGenerateMipMapNonFilterable(void)
	{
	}

	void		  deinit(void);
	void		  init(void);
	void		  initTest(void);
	IterateResult iterate(void);

private:
	/* Private methods */
	void generateTestData(int face, unsigned char* data, int size);

	/* Variables for general usage */
	std::vector<StorageConfig> m_non_filterable_texture_configs;
	STORAGE_TYPE			   m_storage_type;
};

} /* glcts */

#endif // _ESEXTCTEXTURECUBEMAPARRAYGENERATEMIPMAP_HPP
