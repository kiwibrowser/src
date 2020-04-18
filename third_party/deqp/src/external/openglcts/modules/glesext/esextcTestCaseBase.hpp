#ifndef _ESEXTCTESTCASEBASE_HPP
#define _ESEXTCTESTCASEBASE_HPP
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

#include "glcContext.hpp"
#include "gluShaderUtil.hpp"
#include "tcuDefs.hpp"
#include "tcuTestCase.hpp"

#include "glcExtTokens.hpp"
#include "glwDefs.hpp"
#include "glwFunctions.hpp"
#include <map>
#include <math.h>
#include <string.h>

/* String definitions */
#define GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED "Geometry shader functionality not supported, skipping"
#define GEOMETRY_SHADER_POINT_SIZE_NOT_SUPPORTED "Geometry shader point size functionality not supported, skipping"
#define GPU_SHADER5_EXTENSION_NOT_SUPPORTED "GPU shader5 functionality not supported, skipping"
#define TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED "Tessellation shader functionality not supported, skipping"
#define TEXTURE_BORDER_CLAMP_NOT_SUPPORTED "Texture border clamp functionality not supported, skipping"
#define TEXTURE_CUBE_MAP_ARRAY_EXTENSION_NOT_SUPPORTED "Texture cube map array functionality not supported, skipping"
#define SHADER_IMAGE_ATOMIC_EXTENSION_NOT_SUPPORTED "Shader image atomic functionality not supported, skipping"
#define TEXTURE_BUFFER_EXTENSION_NOT_SUPPORTED "Texture buffer functionality not supported, skipping"
#define DRAW_BUFFERS_INDEXED_NOT_SUPPORTED "Draw buffers indexed functionality not supported, skipping"
#define VIEWPORT_ARRAY_NOT_SUPPORTED "Viewport array functionality not supported, skipping"

namespace glcts
{
/* Define allowed storage types */
enum STORAGE_TYPE
{
	ST_MUTABLE,  /* Mutable Storage */
	ST_IMMUTABLE /* Immutable Storage */
};

/* Extension names */
enum ExtensionName
{
	EXTENSIONNAME_SHADER_IMAGE_ATOMIC,
	EXTENSIONNAME_SHADER_IO_BLOCKS,
	EXTENSIONNAME_GEOMETRY_SHADER,
	EXTENSIONNAME_GEOMETRY_POINT_SIZE,
	EXTENSIONNAME_TESSELLATION_SHADER,
	EXTENSIONNAME_TESSELLATION_POINT_SIZE,
	EXTENSIONNAME_TEXTURE_BUFFER,
	EXTENSIONNAME_TEXTURE_CUBE_MAP_ARRAY,
	EXTENSIONNAME_GPU_SHADER5,
	EXTENSIONNAME_VIEWPORT_ARRAY,
};

/* Extension type */
enum ExtensionType
{
	EXTENSIONTYPE_NONE, /* Not an extension (part of this version of OpenGL or OpenGL ES) */
	EXTENSIONTYPE_EXT,  /* EXT multivendor extension */
	EXTENSIONTYPE_OES   /* OES Khronos extension */
};

enum ExtensionBehavior
{
	EXTENSIONBEHAVIOR_DISABLE, /* disable */
	EXTENSIONBEHAVIOR_WARN,	/* warn */
	EXTENSIONBEHAVIOR_ENABLE,  /* enable */
	EXTENSIONBEHAVIOR_REQUIRE  /* require */
};

using deqp::Context;

struct ExtParameters
{
	glu::GLSLVersion glslVersion;
	ExtensionType	extType;

	ExtParameters(glu::GLSLVersion _glslVersion, ExtensionType _extType) : glslVersion(_glslVersion), extType(_extType)
	{
	}
};

/**
 * Base class for tests implementations.
 */
class TestCaseBase : public tcu::TestCase
{
public:
	/* Public methods */

	/* Destructor */
	virtual ~TestCaseBase(void)
	{
	}

	static const float m_epsilon_float;

protected:
	enum LOG_TYPE
	{
		LT_SHADER_OBJECT,  /* Shader object */
		LT_PROGRAM_OBJECT, /* Program object */
		LT_PIPELINE_OBJECT /* Program pipeline object */
	};
	/* Protected type definitions */

	/* Protected methods */

	/* Constructor */
	TestCaseBase(Context& context, const ExtParameters& extParam, const char* name, const char* description);

	/* Methods that a derived test case should reimplement */
	virtual void		  deinit(void);
	virtual void		  init(void);
	virtual IterateResult iterate(void);

	/* Initializes extension function pointers */
	void initExtensions();

	/* Initializes GLSL specialization map */
	void initGLSLSpecializationMap();

	/* Function that generates an extension directive */
	std::string getGLSLExtDirective(ExtensionType type, ExtensionName name, ExtensionBehavior behavior);

	/* Sets the seed for the random generator */
	void randomSeed(const glw::GLuint seed);

	/* Returns random unsigned integer from the range [0,max) */
	glw::GLuint randomFormula(const glw::GLuint max);

	/* Helper method for verification of pixel color */
	bool comparePixel(const unsigned char* buffer, unsigned int x, unsigned int y, unsigned int width,
					  unsigned int height, unsigned int pixel_size, unsigned char expected_red = 0,
					  unsigned char expected_green = 0, unsigned char expected_blue = 0,
					  unsigned char expected_alpha = 0) const;

	/* Helper method for checking if an extension is supported*/
	bool isExtensionSupported(const std::string& extName) const;

	/* Program creation and validation helper methods */
	std::string specializeShader(const unsigned int parts, const char* const* code) const;
	void shaderSourceSpecialized(glw::GLuint shader_id, glw::GLsizei shader_count,
								 const glw::GLchar* const* shader_string);

	bool buildProgram(glw::GLuint po_id, glw::GLuint sh1_shader_id, unsigned int n_sh1_body_parts,
					  const char* const* sh1_body_parts, bool* out_has_compilation_failed = NULL);

	bool buildProgram(glw::GLuint po_id, glw::GLuint sh1_shader_id, unsigned int n_sh1_body_parts,
					  const char* const* sh1_body_parts, glw::GLuint sh2_shader_id, unsigned int n_sh2_body_parts,
					  const char* const* sh2_body_parts, bool* out_has_compilation_failed = NULL);

	bool buildProgram(glw::GLuint po_id, glw::GLuint sh1_shader_id, unsigned int n_sh1_body_parts,
					  const char* const* sh1_body_parts, glw::GLuint sh2_shader_id, unsigned int n_sh2_body_parts,
					  const char* const* sh2_body_parts, glw::GLuint sh3_shader_id, unsigned int n_sh3_body_parts,
					  const char* const* sh3_body_parts, bool* out_has_compilation_failed = NULL);

	bool buildProgram(glw::GLuint po_id, glw::GLuint sh1_shader_id, unsigned int n_sh1_body_parts,
					  const char* const* sh1_body_parts, glw::GLuint sh2_shader_id, unsigned int n_sh2_body_parts,
					  const char* const* sh2_body_parts, glw::GLuint sh3_shader_id, unsigned int n_sh3_body_parts,
					  const char* const* sh3_body_parts, glw::GLuint sh4_shader_id, unsigned int n_sh4_body_parts,
					  const char* const* sh4_body_parts, bool* out_has_compilation_failed = NULL);

	bool buildProgram(glw::GLuint po_id, glw::GLuint sh1_shader_id, unsigned int n_sh1_body_parts,
					  const char* const* sh1_body_parts, glw::GLuint sh2_shader_id, unsigned int n_sh2_body_parts,
					  const char* const* sh2_body_parts, glw::GLuint sh3_shader_id, unsigned int n_sh3_body_parts,
					  const char* const* sh3_body_parts, glw::GLuint sh4_shader_id, unsigned int n_sh4_body_parts,
					  const char* const* sh4_body_parts, glw::GLuint sh5_shader_id, unsigned int n_sh5_body_parts,
					  const char* const* sh5_body_parts, bool* out_has_compilation_failed = NULL);

	bool doesProgramBuild(unsigned int n_fs_body_parts, const char* const* fs_body_parts, unsigned int n_gs_body_parts,
						  const char* const* gs_body_parts, unsigned int n_vs_body_parts,
						  const char* const* vs_body_parts);

	std::string getShaderSource(glw::GLuint shader_id);
	std::string getCompilationInfoLog(glw::GLuint shader_id);
	std::string getLinkingInfoLog(glw::GLuint po_id);
	std::string getPipelineInfoLog(glw::GLuint ppo_id);

	/* Helper method for setting up a fraembuffer with texture as color attachment */
	bool setupFramebufferWithTextureAsAttachment(glw::GLuint framebuffer_object_id, glw::GLuint color_texture_id,
												 glw::GLenum texture_format, glw::GLuint width,
												 glw::GLuint height) const;

	void checkFramebufferStatus(glw::GLenum framebuffer) const;

	/* Protected variables */
	Context&		 m_context;
	glu::GLSLVersion m_glslVersion;
	ExtensionType	m_extType;
	std::map<std::string, std::string> m_specializationMap;

	bool m_is_framebuffer_no_attachments_supported;
	bool m_is_geometry_shader_extension_supported;
	bool m_is_geometry_shader_point_size_supported;
	bool m_is_gpu_shader5_supported;
	bool m_is_program_interface_query_supported;
	bool m_is_shader_image_load_store_supported;
	bool m_is_shader_image_atomic_supported;
	bool m_is_texture_storage_multisample_supported;
	bool m_is_texture_storage_multisample_2d_array_supported;
	bool m_is_tessellation_shader_supported;
	bool m_is_tessellation_shader_point_size_supported;
	bool m_is_texture_cube_map_array_supported;
	bool m_is_texture_border_clamp_supported;
	bool m_is_texture_buffer_supported;
	bool m_is_viewport_array_supported;

	/* Predefined shader strings */
	static const char* m_boilerplate_vs_code;

	/* GL tokens that are diferent for functionalities
	 * enabled by extensions and for functionalities that
	 * are present in the core. */
	deqp::GLExtTokens m_glExtTokens;

private:
	/* Private functions */
	bool buildProgramVA(glw::GLuint po_id, bool* out_has_compilation_failed, unsigned int sh_stages, ...);
	std::string getInfoLog(LOG_TYPE log_type, glw::GLuint id);

	/* Private variables */
	glw::GLuint seed_value;

	friend class TessellationShaderUtils;
};

/* Test Case Group that tracks GLSL version and extension type */
class TestCaseGroupBase : public tcu::TestCaseGroup
{
public:
	TestCaseGroupBase(Context& context, const ExtParameters& extParam, const char* name, const char* description);

	virtual ~TestCaseGroupBase(void)
	{
	}

	Context& getContext(void)
	{
		return m_context;
	}

protected:
	Context&	  m_context;
	ExtParameters m_extParams;
};

inline TestCaseGroupBase::TestCaseGroupBase(Context& context, const ExtParameters& extParams, const char* name,
											const char* description)
	: tcu::TestCaseGroup(context.getTestContext(), name, description), m_context(context), m_extParams(extParams)
{
}

} // namespace glcts

#endif // _ESEXTCTESTCASEBASE_HPP
