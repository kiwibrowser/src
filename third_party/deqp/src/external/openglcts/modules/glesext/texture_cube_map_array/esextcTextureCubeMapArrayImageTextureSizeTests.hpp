#ifndef _ESEXTCTEXTURECUBEMAPARRAYIMAGETEXTURESIZETESTS_HPP
#define _ESEXTCTEXTURECUBEMAPARRAYIMAGETEXTURESIZETESTS_HPP
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
 * \file  esextcTextureCubeMapArrayTextureSizeTests.hpp
 * \brief texture_cube_map_array extension - Texture Size Test (Test 10)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureCubeMapArrayImageTextureSize.hpp"
#include "glcContext.hpp"
#include "glcTestCase.hpp"

namespace glcts
{
/** Implementation of (Test 10) for texture_cube_map_array extension. Test description follows:
 *   Make sure shaders can correctly query the size of cube-map array textures
 *   bound to active texture or image samplers.
 *
 *   Category:           Coverage,
 *                       Optional dependency on EXT_geometry_shader;
 *                       Optional dependency on EXT_tessellation_shader;
 *   Suggested priority: Must-have for the texture part;
 *                       Must-have for the image part;
 *
 *   Make sure textureSize() (both shadow and normal cases) GLSL functions
 *   return correct values for cube-map array textures.
 *   Make sure imageSize() returns correct values for image units, to which
 *   cube-map array textures have been bound.
 *
 *   All supported shader types should be considered for the purpose of the
 *   test.
 *
 *   The results should be XFBed back to test implementation for verification
 *   in case of geometry & tessellation shaders & vertex shaders.
 *   For compute shaders, the size should be stored to an image of
 *   1x1 resolution and of GL_RGBA32UI internalformat.
 *   Fragment shaders should store the texture size by writing it to
 *   an output variable defined in the fragment shader. A GL_RGBA32UI
 *   texture of 1x1 resolution should be used as a draw buffer.
 *
 *   The following texture resolutions should be used
 *       { 64,  64,  6},
 *       {128, 128, 12},
 *       {256, 256, 12},
 *       { 32,  32, 18}
 *   Both immutable and mutable textures should be checked.
 *
 */
class TextureCubeMapArrayImageTextureSizeTests : public glcts::TestCaseGroupBase
{
public:
	/* Public methods */
	TextureCubeMapArrayImageTextureSizeTests(glcts::Context& context, const ExtParameters& extParams, const char* name,
											 const char* description);

	virtual ~TextureCubeMapArrayImageTextureSizeTests()
	{
	}

	virtual void init(void);
	virtual void deinit(void);
};

} // namespace glcts

#endif // _ESEXTCTEXTURECUBEMAPARRAYIMAGETEXTURESIZETESTS_HPP
