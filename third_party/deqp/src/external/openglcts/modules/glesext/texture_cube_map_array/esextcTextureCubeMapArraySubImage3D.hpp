#ifndef _ESEXTCTEXTURECUBEMAPARRAYSUBIMAGE3D_HPP
#define _ESEXTCTEXTURECUBEMAPARRAYSUBIMAGE3D_HPP
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
 * \file  esextcTextureCubeMapArraySubImage3D.hpp
 * \brief Texture Cube Map Array SubImage3D (Test 5)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

namespace glcts
{
/**  Implementation of Test 5 from CTS_EXT_texture_cube_map_array. Test description follows:
 *
 *    Make sure glTexSubImage3D() and glCopyTexSubImage3D() calls work correctly
 *    for cube-map array textures.
 *
 *   Category: Functionality tests.
 *   Priority: Must-have.
 *
 *   Make sure glTexSubImage3D() and glCopyTexSubImage3D() correctly replace
 *   a region of a cube-map array texture defined with GL_RGBA32UI internalformat.
 *   Source data should be defined with the same GL_RGBA32UI internalformat.
 *
 *   Original cube-map array texture data should be defined as in test case 1.
 *   The data to be written over layer-face(s) should be distinctively different
 *   (for instance: the same values could be used with a different component
 *    ordering).
 *
 *   The test should use both client-side pointers and data defined in a BO
 *   bound to GL_PIXEL_UNPACK_BUFFER target.
 *
 *   Iterate through texture resolutions as described in test 1.
 *
 *   The following scenarios should be tested:
 *
 *   a) A single whole layer-face at index 0 should be replaced
 *      (both functions);
 *   b) A region of a layer-face at index 0 should be replaced
 *      (both functions);
 *   c) 6 layer-faces, making up a single layer, should be replaced
 *       (glTexSubImage3D() only).
 *   d) 6 layer-faces, making up two different layers (for instance: three
 *      last layer-faces of layer 1 and three first layer-faces of layer 2)
 *      should be replaced (glTexSubImage3D() only).
 *   e) c) and d), but limited to a quad defined by the following points:
 *
 *            (width/2, height/2) x (width, height)
 *
 *      (glTexSubImage3D() only)
 *
 *   Test four different cube-map array texture resolutions, as described
 *   in test 1. Both immutable and mutable textures should be checked.
 */

/* Structure holding information which region of the cube map to replace */
struct SubImage3DCopyParams
{
	SubImage3DCopyParams(void) : m_xoffset(0), m_yoffset(0), m_zoffset(0), m_width(0), m_height(0), m_depth(0)
	{
		/* Nothing to be done here */
	}

	void init(const glw::GLuint xoffset, const glw::GLuint yoffset, const glw::GLuint zoffset, const glw::GLuint width,
			  const glw::GLuint height, const glw::GLuint depth)
	{
		m_xoffset = xoffset;
		m_yoffset = yoffset;
		m_zoffset = zoffset;
		m_width   = width;
		m_height  = height;
		m_depth   = depth;
	}

	glw::GLuint m_xoffset;
	glw::GLuint m_yoffset;
	glw::GLuint m_zoffset;
	glw::GLuint m_width;
	glw::GLuint m_height;
	glw::GLuint m_depth;
};

class TextureCubeMapArraySubImage3D : public TestCaseBase
{
public:
	/* Public methods */
	TextureCubeMapArraySubImage3D(Context& context, const ExtParameters& extParams, const char* name,
								  const char* description);

	virtual ~TextureCubeMapArraySubImage3D()
	{
	}

	virtual IterateResult iterate(void);
	virtual void		  deinit(void);

	/* Public static constants */
	static const glw::GLuint m_n_components;
	static const glw::GLuint m_n_dimensions;
	static const glw::GLuint m_n_resolutions;
	static const glw::GLuint m_n_storage_type;

protected:
	/* Protected methods */
	void initTest(void);

	void configureDataBuffer(glw::GLuint width, glw::GLuint height, glw::GLuint depth,
							 const SubImage3DCopyParams& copy_params, glw::GLuint clear_value);
	void configurePixelUnpackBuffer(const SubImage3DCopyParams& copy_params);
	void configureCubeMapArrayTexture(glw::GLuint width, glw::GLuint height, glw::GLuint depth, STORAGE_TYPE storType,
									  glw::GLuint clear_value);
	void clearCubeMapArrayTexture(glw::GLuint width, glw::GLuint height, glw::GLuint depth, glw::GLuint clear_value);
	void configure2DTexture(const SubImage3DCopyParams& copy_params);

	void testTexSubImage3D(glw::GLuint width, glw::GLuint height, glw::GLuint depth,
						   const SubImage3DCopyParams& copy_params, glw::GLboolean& test_passed);
	void testCopyTexSubImage3D(glw::GLuint width, glw::GLuint height, glw::GLuint depth,
							   const SubImage3DCopyParams& copy_params, glw::GLboolean& test_passed);

	void texSubImage3D(const SubImage3DCopyParams& copy_params, const glw::GLuint* data_pointer);
	void copyTexSubImage3D(const SubImage3DCopyParams& copy_params);
	bool checkResults(glw::GLuint width, glw::GLuint height, glw::GLuint depth);

	void deletePixelUnpackBuffer();
	void deleteCubeMapArrayTexture();
	void delete2DTexture();

	typedef std::vector<glw::GLuint> DataBufferVec;

	/* Protected variables */
	DataBufferVec m_copy_data_buffer;
	DataBufferVec m_expected_data_buffer;
	glw::GLuint   m_read_fbo_id;
	glw::GLuint   m_pixel_buffer_id;
	glw::GLuint   m_tex_cube_map_array_id;
	glw::GLuint   m_tex_2d_id;
};

} // namespace glcts

#endif // _ESEXTCTEXTURECUBEMAPARRAYSUBIMAGE3D_HPP
