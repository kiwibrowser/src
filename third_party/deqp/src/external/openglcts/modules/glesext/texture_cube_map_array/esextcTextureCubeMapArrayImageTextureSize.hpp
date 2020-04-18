#ifndef _ESEXTCTEXTURECUBEMAPARRAYIMAGETEXTURESIZE_HPP
#define _ESEXTCTEXTURECUBEMAPARRAYIMAGETEXTURESIZE_HPP
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
 * \file  esextcTextureCubeMapArrayImageTextureSize.hpp
 * \brief texture_cube_map_array extension - Texture Size Test (Test 10)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"
#include <vector>

namespace glcts
{
/** Implementation of (Test 10) for texture_cube_map_array extension. Test description follows:
 *
 *   Make sure shaders can correctly query the size of cube-map array textures
 *   bound to active texture or image samplers.
 *
 *   Category: Coverage,
 *             Optional dependency on EXT_geometry_shader;
 *             Optional dependency on EXT_tessellation_shader;
 *   Priority: Must-have.
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
 *   Test the following cube-map array texture resolutions:
 *
 *       { 64,  64,  6},
 *       {128, 128, 12},
 *       {256, 256, 12},
 *       { 32,  32, 18}
 *
 *   Both immutable and mutable textures should be checked.
 *
 */

/* Base class for tests */
class TextureCubeMapArrayTextureSizeBase : public TestCaseBase
{
public:
	/* Public methods */
	TextureCubeMapArrayTextureSizeBase(Context& context, const ExtParameters& extParams, const char* name,
									   const char* description);

	virtual ~TextureCubeMapArrayTextureSizeBase(void)
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

	/* Public static constants  */
	static const glw::GLuint m_n_dimensions;
	static const glw::GLuint m_n_resolutions;

protected:
	/* Protected methods */
	void initTest(void);
	void createCubeMapArrayTexture(glw::GLuint& texId, glw::GLuint width, glw::GLuint height, glw::GLuint depth,
								   STORAGE_TYPE storType, glw::GLboolean shadow);

	virtual void configureProgram(void) = 0;
	virtual void deleteProgram(void)	= 0;
	virtual void configureUniforms(void);
	virtual void configureTestSpecificObjects(void) = 0;
	virtual void deleteTestSpecificObjects(void)	= 0;
	virtual void configureTextures(glw::GLuint width, glw::GLuint height, glw::GLuint depth, STORAGE_TYPE storType);
	virtual void		   deleteTextures(void);
	virtual void		   runShaders(void) = 0;
	virtual glw::GLboolean checkResults(glw::GLuint width, glw::GLuint height, glw::GLuint depth,
										STORAGE_TYPE storType) = 0;
	virtual glw::GLboolean isMutableTextureTestable(void);

	/* Protected variables */
	glw::GLuint m_po_id;
	glw::GLuint m_to_std_id;
	glw::GLuint m_to_shw_id;
	glw::GLuint m_vao_id;

	/* Protected static constants */
	static const glw::GLuint m_n_layers_per_cube;
	static const glw::GLuint m_n_storage_types;
	static const glw::GLuint m_n_texture_components;
};

/* Base class for tests using transform feedback to fetch result */
class TextureCubeMapArrayTextureSizeTFBase : public TextureCubeMapArrayTextureSizeBase
{
public:
	/* Public methods */
	TextureCubeMapArrayTextureSizeTFBase(Context& context, const ExtParameters& extParams, const char* name,
										 const char* description);

	virtual ~TextureCubeMapArrayTextureSizeTFBase(void)
	{
	}

protected:
	/* Protected methods */
	virtual void configureProgram(void) = 0;
	virtual void deleteProgram(void)	= 0;
	virtual void configureTestSpecificObjects(void);
	virtual void deleteTestSpecificObjects(void);
	virtual void configureTextures(glw::GLuint width, glw::GLuint height, glw::GLuint depth, STORAGE_TYPE storType);
	virtual void		   runShaders(void) = 0;
	virtual glw::GLboolean checkResults(glw::GLuint width, glw::GLuint height, glw::GLuint depth,
										STORAGE_TYPE storType);

	/* Protected variables */
	glw::GLuint m_tf_bo_id;

	/* Protected static constants */
	static const glw::GLsizei m_n_varyings;
	static const glw::GLuint  m_n_tf_components;
};

class TextureCubeMapArrayTextureSizeTFVertexShader : public TextureCubeMapArrayTextureSizeTFBase
{
public:
	/* Public methods */
	TextureCubeMapArrayTextureSizeTFVertexShader(Context& context, const ExtParameters& extParams, const char* name,
												 const char* description);

	virtual ~TextureCubeMapArrayTextureSizeTFVertexShader(void)
	{
	}

protected:
	/* Protected methods */
	virtual void configureProgram(void);
	virtual void deleteProgram(void);
	virtual void runShaders(void);

	const char* getVertexShaderCode(void);
	const char* getFragmentShaderCode(void);

	/* Protected variables */
	glw::GLuint m_vs_id;
	glw::GLuint m_fs_id;
};

class TextureCubeMapArrayTextureSizeTFGeometryShader : public TextureCubeMapArrayTextureSizeTFBase
{
public:
	/* Public methods */
	TextureCubeMapArrayTextureSizeTFGeometryShader(Context& context, const ExtParameters& extParams, const char* name,
												   const char* description);

	virtual ~TextureCubeMapArrayTextureSizeTFGeometryShader(void)
	{
	}

protected:
	/* Protected methods */
	virtual void configureProgram(void);
	virtual void deleteProgram(void);
	virtual void runShaders(void);

	const char* getVertexShaderCode(void);
	const char* getGeometryShaderCode(void);
	const char* getFragmentShaderCode(void);

	/* Protected variables */
	glw::GLuint m_vs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_fs_id;
};

class TextureCubeMapArrayTextureSizeTFTessControlShader : public TextureCubeMapArrayTextureSizeTFBase
{
public:
	/* Public methods */
	TextureCubeMapArrayTextureSizeTFTessControlShader(Context& context, const ExtParameters& extParams,
													  const char* name, const char* description);

	virtual ~TextureCubeMapArrayTextureSizeTFTessControlShader(void)
	{
	}

protected:
	/* Protected methods */
	virtual void configureProgram(void);
	virtual void deleteProgram(void);
	virtual void runShaders(void);

	const char*			getVertexShaderCode(void);
	virtual const char* getTessellationControlShaderCode(void);
	virtual const char* getTessellationEvaluationShaderCode(void);
	const char*			getFragmentShaderCode(void);

	/* Protected variables */
	glw::GLuint m_vs_id;
	glw::GLuint m_tcs_id;
	glw::GLuint m_tes_id;
	glw::GLuint m_fs_id;
};

class TextureCubeMapArrayTextureSizeTFTessEvaluationShader : public TextureCubeMapArrayTextureSizeTFTessControlShader
{
public:
	/* Public methods */
	TextureCubeMapArrayTextureSizeTFTessEvaluationShader(Context& context, const ExtParameters& extParams,
														 const char* name, const char* description);

	virtual ~TextureCubeMapArrayTextureSizeTFTessEvaluationShader(void)
	{
	}

protected:
	/* Protected methods */
	virtual const char* getTessellationControlShaderCode(void);
	virtual const char* getTessellationEvaluationShaderCode(void);
};

/* Base class for tests using rendering to texture to fetch result */
class TextureCubeMapArrayTextureSizeRTBase : public TextureCubeMapArrayTextureSizeBase
{
public:
	/* Public methods */
	TextureCubeMapArrayTextureSizeRTBase(Context& context, const ExtParameters& extParams, const char* name,
										 const char* description);

	virtual ~TextureCubeMapArrayTextureSizeRTBase(void)
	{
	}

protected:
	/* Protected methods */
	virtual void		   configureProgram(void) = 0;
	virtual void		   deleteProgram(void)	= 0;
	virtual void		   configureTestSpecificObjects(void);
	virtual void		   deleteTestSpecificObjects(void);
	virtual void		   runShaders(void) = 0;
	virtual glw::GLboolean checkResults(glw::GLuint width, glw::GLuint height, glw::GLuint depth,
										STORAGE_TYPE storType);
	void checkFramebufferStatus(glw::GLenum framebuffer);

	/* Protected variables */
	glw::GLuint m_read_fbo_id;
	glw::GLuint m_rt_std_id;
	glw::GLuint m_rt_shw_id;

	/* Protected static constants */
	static const glw::GLuint m_n_rt_components;
};

class TextureCubeMapArrayTextureSizeRTFragmentShader : public TextureCubeMapArrayTextureSizeRTBase
{
public:
	/* Public methods */
	TextureCubeMapArrayTextureSizeRTFragmentShader(Context& context, const ExtParameters& extParams, const char* name,
												   const char* description);

	virtual ~TextureCubeMapArrayTextureSizeRTFragmentShader(void)
	{
	}

protected:
	/* Protected methods */
	virtual void configureProgram(void);
	virtual void deleteProgram(void);
	virtual void configureTestSpecificObjects(void);
	virtual void deleteTestSpecificObjects(void);
	virtual void runShaders(void);

	const char* getVertexShaderCode(void);
	const char* getFragmentShaderCode(void);

	/* Protected variables */
	glw::GLuint m_draw_fbo_id;
	glw::GLuint m_vs_id;
	glw::GLuint m_fs_id;
};

class TextureCubeMapArrayTextureSizeRTComputeShader : public TextureCubeMapArrayTextureSizeRTBase
{
public:
	/* Public methods */
	TextureCubeMapArrayTextureSizeRTComputeShader(Context& context, const ExtParameters& extParams, const char* name,
												  const char* description);

	virtual ~TextureCubeMapArrayTextureSizeRTComputeShader(void)
	{
	}

private:
	/* Private methods */
	virtual void configureProgram(void);
	virtual void deleteProgram(void);
	virtual void configureTestSpecificObjects(void);
	virtual void deleteTestSpecificObjects(void);
	virtual void configureTextures(glw::GLuint width, glw::GLuint height, glw::GLuint depth, STORAGE_TYPE storType);
	virtual void		   deleteTextures(void);
	virtual void		   runShaders(void);
	virtual glw::GLboolean checkResults(glw::GLuint width, glw::GLuint height, glw::GLuint depth,
										STORAGE_TYPE storType);
	virtual glw::GLboolean isMutableTextureTestable(void);

	const char* getComputeShaderCode(void);

	/* Private variables */
	glw::GLuint m_cs_id;
	glw::GLuint m_to_img_id;
	glw::GLuint m_rt_img_id;
};

} /* glcts */

#endif // _ESEXTCTEXTURECUBEMAPARRAYIMAGETEXTURESIZE_HPP
