#ifndef _ESEXTCTEXTUREBUFFERPRECISION_HPP
#define _ESEXTCTEXTUREBUFFERPRECISION_HPP
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
 * \file  esextcTextureBufferPrecision.hpp
 * \brief Texture Buffer Precision Qualifier (Test 10)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"
#include "glcTestCase.hpp"
#include "gluShaderUtil.hpp"
#include "tcuDefs.hpp"
#include <map>

namespace glcts
{

/** Implementation of (Test 10) from CTS_EXT_texture_buffer. Description follows:
 *
 *  Test whether shaders that don't include a precision qualifier for each of the
 *  *samplerBuffer and *imageBuffer uniform declarations fail to compile.
 *
 *  Category: API.
 *
 *  Write a compute shader that defines
 *
 *  layout (r32f) uniform imageBuffer image_buffer;
 *
 *  buffer ComputeSSBO
 *  {
 *    float value;
 *  } computeSSBO;
 *
 *  In the compute shader execute:
 *
 *  computeSSBO.value = imageLoad( image_buffer, 0 ).r;
 *
 *  Try to compile the shader. The shader should fail to compile.
 *
 *  Include a precision qualifier for the uniform declaration:
 *
 *  layout (r32f) uniform highp imageBuffer image_buffer;
 *
 *  This time the shader should compile without error.
 *
 *  Instead of including a precision qualifier for the uniform
 *  declaration specify the default precision qualifier:
 *
 *  precision highp imageBuffer;
 *
 *  This time the shader should also compile without error.
 *
 *  Repeat the above tests with the iimageBuffer and uimageBuffer types
 *  (instead of imageBuffer).
 *
 *  ---------------------------------------------------------------------------
 *
 *  Write a fragment shader that defines:
 *
 *  uniform samplerBuffer sampler_buffer;
 *
 *  layout(location = 0) out vec4 color;
 *
 *  In the fragment shader execute:
 *
 *  color = texelFetch( sampler_buffer, 0 );
 *
 *  Try to compile the shader. The shader should fail to compile.
 *
 *  Include a precision qualifier for the uniform declaration:
 *
 *  uniform highp samplerBuffer sampler_buffer;
 *
 *  This time the shader should compile without error.
 *
 *  Instead of including a precision qualifier for the uniform
 *  declaration specify the default precision qualifier:
 *
 *  precision highp samplerBuffer;
 *
 *  This time the shader should also compile without error.
 *
 *  Repeat the above tests with the isamplerBuffer and usamplerBuffer types
 *  (instead of samplerBuffer).
 */
class TextureBufferPrecision : public TestCaseBase
{
public:
	/* Public methods */
	TextureBufferPrecision(Context& context, const ExtParameters& extParams, const char* name, const char* description);

	virtual ~TextureBufferPrecision()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	glw::GLboolean verifyShaderCompilationStatus(glw::GLenum shader_type, const char** sh_code_parts,
												 const glw::GLuint parts_count, const glw::GLint expected_status);

	/* Private variables */
	glw::GLuint m_po_id; /* Program object */
	glw::GLuint m_sh_id; /* Shader object */

	static const char* const m_cs_code_head; /* Head of the compute shader. */
	static const char* const m_cs_code_declaration_without_precision
		[3]; /* Variables declarations for the compute shader without usage of the precision qualifier. */
	static const char* const m_cs_code_declaration_with_precision
		[3]; /* Variables declarations for the compute shader with usage of the precision qualifier. */
	static const char* const
							 m_cs_code_global_precision[3]; /* The default precision qualifier declarations for the compute shader. */
	static const char* const m_cs_code_body;				/* The compute shader body. */

	static const char* const m_fs_code_head; /* Head of the fragment shader. */
	static const char* const m_fs_code_declaration_without_precision
		[3]; /* Variables declarations for the fragment shader without usage of the precision qualifier. */
	static const char* const m_fs_code_declaration_with_precision
		[3]; /* Variables declarations for the fragment shader with usage of the precision qualifier. */
	static const char* const
							 m_fs_code_global_precision[3]; /* The default precision qualifier declarations for the fragment shader. */
	static const char* const m_fs_code_body;				/* The fragment shader body. */
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBUFFERPRECISION_HPP
