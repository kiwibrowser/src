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
 * \file  esextcTextureBufferTextureBufferRange.cpp
 * \brief Texture Buffer Range Test (Test 3)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBufferTextureBufferRange.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuPixelFormat.hpp"
#include "tcuTestLog.hpp"
#include <vector>

namespace glcts
{

/** Constructor
 *
 **/
FormatInfo::FormatInfo()
	: m_aligned_size(0)
	, m_not_aligned_size(0)
	, m_ssbo_value_size(0)
	, m_internal_format(GL_NONE)
	, m_offset_alignment(0)
	, m_n_components(0)
	, m_input_type(GL_NONE)
	, m_output_type(GL_NONE)
	, m_is_image_supported(false)
{
}

/** Constructor
 *
 *  @param internalFormat  texture internal format
 *  @param offsetAlignment value of GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT_EXT
 **/
FormatInfo::FormatInfo(glw::GLenum internalFormat, glw::GLuint offsetAlignment)
	: m_aligned_size(0)
	, m_not_aligned_size(0)
	, m_ssbo_value_size(0)
	, m_internal_format(internalFormat)
	, m_offset_alignment(offsetAlignment)
	, m_n_components(0)
	, m_input_type(GL_NONE)
	, m_output_type(GL_NONE)
	, m_is_image_supported(false)
{
	configure();
}

/** Configure test parameters according to internal format and offsetAlignment
 */
void FormatInfo::configure()
{
	/* Check internal format */
	switch (m_internal_format)
	{
	case GL_R8:
		m_n_components		 = 1;
		m_is_image_supported = false;
		m_input_type		 = GL_UNSIGNED_BYTE;
		m_output_type		 = GL_FLOAT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLubyte) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLfloat) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "";
		m_image_type_name	= "";
		m_sampler_type_name  = "samplerBuffer";
		m_output_type_name   = "float";
		m_value_selector	 = ".x";
		break;

	case GL_R16F:
		m_n_components		 = 1;
		m_is_image_supported = false;
		m_input_type		 = GL_HALF_FLOAT;
		m_output_type		 = GL_FLOAT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLhalf) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLfloat) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "";
		m_image_type_name	= "";
		m_sampler_type_name  = "samplerBuffer";
		m_output_type_name   = "float";
		m_value_selector	 = ".x";
		break;

	case GL_R32F:
		m_n_components		 = 1;
		m_is_image_supported = true;
		m_input_type		 = GL_FLOAT;
		m_output_type		 = GL_FLOAT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLfloat) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLfloat) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "r32f";
		m_image_type_name	= "imageBuffer";
		m_sampler_type_name  = "samplerBuffer";
		m_output_type_name   = "float";
		m_value_selector	 = ".x";
		break;

	case GL_R8I:
		m_n_components		 = 1;
		m_is_image_supported = false;
		m_input_type		 = GL_BYTE;
		m_output_type		 = GL_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLbyte) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "";
		m_image_type_name	= "";
		m_sampler_type_name  = "isamplerBuffer";
		m_output_type_name   = "int";
		m_value_selector	 = ".x";
		break;

	case GL_R16I:
		m_n_components		 = 1;
		m_is_image_supported = false;
		m_input_type		 = GL_SHORT;
		m_output_type		 = GL_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLshort) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "";
		m_image_type_name	= "";
		m_sampler_type_name  = "isamplerBuffer";
		m_output_type_name   = "int";
		m_value_selector	 = ".x";
		break;

	case GL_R32I:
		m_n_components		 = 1;
		m_is_image_supported = true;
		m_input_type		 = GL_INT;
		m_output_type		 = GL_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLint) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "r32i";
		m_image_type_name	= "iimageBuffer";
		m_sampler_type_name  = "isamplerBuffer";
		m_output_type_name   = "int";
		m_value_selector	 = ".x";
		break;

	case GL_R8UI:
		m_n_components		 = 1;
		m_is_image_supported = false;
		m_input_type		 = GL_UNSIGNED_BYTE;
		m_output_type		 = GL_UNSIGNED_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLubyte) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLuint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "";
		m_image_type_name	= "";
		m_sampler_type_name  = "usamplerBuffer";
		m_output_type_name   = "uint";
		m_value_selector	 = ".x";
		break;

	case GL_R16UI:
		m_n_components		 = 1;
		m_is_image_supported = false;
		m_input_type		 = GL_UNSIGNED_SHORT;
		m_output_type		 = GL_UNSIGNED_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLushort) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLuint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "";
		m_image_type_name	= "";
		m_sampler_type_name  = "usamplerBuffer";
		m_output_type_name   = "uint";
		m_value_selector	 = ".x";
		break;

	case GL_R32UI:
		m_n_components		 = 1;
		m_is_image_supported = true;
		m_input_type		 = GL_UNSIGNED_INT;
		m_output_type		 = GL_UNSIGNED_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLuint) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLuint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "r32ui";
		m_image_type_name	= "uimageBuffer";
		m_sampler_type_name  = "usamplerBuffer";
		m_output_type_name   = "uint";
		m_value_selector	 = ".x";
		break;

	case GL_RG8:
		m_n_components		 = 2;
		m_is_image_supported = false;
		m_input_type		 = GL_UNSIGNED_BYTE;
		m_output_type		 = GL_FLOAT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLubyte) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLfloat) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "";
		m_image_type_name	= "";
		m_sampler_type_name  = "samplerBuffer";
		m_output_type_name   = "vec2";
		m_value_selector	 = ".xy";
		break;

	case GL_RG16F:
		m_n_components		 = 2;
		m_is_image_supported = false;
		m_input_type		 = GL_HALF_FLOAT;
		m_output_type		 = GL_FLOAT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLhalf) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLfloat) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "";
		m_image_type_name	= "";
		m_sampler_type_name  = "samplerBuffer";
		m_output_type_name   = "vec2";
		m_value_selector	 = ".xy";
		break;

	case GL_RG32F:
		m_n_components		 = 2;
		m_is_image_supported = false;
		m_input_type		 = GL_FLOAT;
		m_output_type		 = GL_FLOAT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLfloat) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLfloat) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "";
		m_image_type_name	= "";
		m_sampler_type_name  = "samplerBuffer";
		m_output_type_name   = "vec2";
		m_value_selector	 = ".xy";
		break;

	case GL_RG8I:
		m_n_components		 = 2;
		m_is_image_supported = false;
		m_input_type		 = GL_BYTE;
		m_output_type		 = GL_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLbyte) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "";
		m_image_type_name	= "";
		m_sampler_type_name  = "isamplerBuffer";
		m_output_type_name   = "ivec2";
		m_value_selector	 = ".xy";
		break;

	case GL_RG16I:
		m_n_components		 = 2;
		m_is_image_supported = false;
		m_input_type		 = GL_SHORT;
		m_output_type		 = GL_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLshort) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "";
		m_image_type_name	= "";
		m_sampler_type_name  = "isamplerBuffer";
		m_output_type_name   = "ivec2";
		m_value_selector	 = ".xy";
		break;

	case GL_RG32I:
		m_n_components		 = 2;
		m_is_image_supported = false;
		m_input_type		 = GL_INT;
		m_output_type		 = GL_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLint) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "";
		m_image_type_name	= "";
		m_sampler_type_name  = "isamplerBuffer";
		m_output_type_name   = "ivec2";
		m_value_selector	 = ".xy";
		break;

	case GL_RG8UI:
		m_n_components		 = 2;
		m_is_image_supported = false;
		m_input_type		 = GL_UNSIGNED_BYTE;
		m_output_type		 = GL_UNSIGNED_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLubyte) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLuint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "";
		m_image_type_name	= "";
		m_sampler_type_name  = "usamplerBuffer";
		m_output_type_name   = "uvec2";
		m_value_selector	 = ".xy";
		break;

	case GL_RG16UI:
		m_n_components		 = 2;
		m_is_image_supported = false;
		m_input_type		 = GL_UNSIGNED_SHORT;
		m_output_type		 = GL_UNSIGNED_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLushort) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLuint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "";
		m_image_type_name	= "";
		m_sampler_type_name  = "usamplerBuffer";
		m_output_type_name   = "uvec2";
		m_value_selector	 = ".xy";
		break;

	case GL_RG32UI:
		m_n_components		 = 2;
		m_is_image_supported = false;
		m_input_type		 = GL_UNSIGNED_INT;
		m_output_type		 = GL_UNSIGNED_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLuint) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLuint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "";
		m_image_type_name	= "";
		m_sampler_type_name  = "usamplerBuffer";
		m_output_type_name   = "uvec2";
		m_value_selector	 = ".xy";
		break;

	case GL_RGB32F:
		m_n_components		 = 3;
		m_is_image_supported = false;
		m_input_type		 = GL_FLOAT;
		m_output_type		 = GL_FLOAT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLfloat) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLfloat) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "";
		m_image_type_name	= "";
		m_sampler_type_name  = "samplerBuffer";
		m_output_type_name   = "vec3";
		m_value_selector	 = ".xyz";
		break;

	case GL_RGB32I:
		m_n_components		 = 3;
		m_is_image_supported = false;
		m_input_type		 = GL_INT;
		m_output_type		 = GL_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLint) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "";
		m_image_type_name	= "";
		m_sampler_type_name  = "isamplerBuffer";
		m_output_type_name   = "ivec3";
		m_value_selector	 = ".xyz";
		break;

	case GL_RGB32UI:
		m_n_components		 = 3;
		m_is_image_supported = false;
		m_input_type		 = GL_UNSIGNED_INT;
		m_output_type		 = GL_UNSIGNED_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLuint) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLuint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "";
		m_image_type_name	= "";
		m_sampler_type_name  = "usamplerBuffer";
		m_output_type_name   = "uvec3";
		m_value_selector	 = ".xyz";
		break;

	case GL_RGBA8:
		m_n_components		 = 4;
		m_is_image_supported = true;
		m_input_type		 = GL_UNSIGNED_BYTE;
		m_output_type		 = GL_FLOAT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLubyte) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLfloat) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "rgba8";
		m_image_type_name	= "imageBuffer";
		m_sampler_type_name  = "samplerBuffer";
		m_output_type_name   = "vec4";
		m_value_selector	 = "";
		break;

	case GL_RGBA16F:
		m_n_components		 = 4;
		m_is_image_supported = true;
		m_input_type		 = GL_HALF_FLOAT;
		m_output_type		 = GL_FLOAT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLhalf) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLfloat) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "rgba16f";
		m_image_type_name	= "imageBuffer";
		m_sampler_type_name  = "samplerBuffer";
		m_output_type_name   = "vec4";
		m_value_selector	 = "";
		break;

	case GL_RGBA32F:
		m_n_components		 = 4;
		m_is_image_supported = true;
		m_input_type		 = GL_FLOAT;
		m_output_type		 = GL_FLOAT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLfloat) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLfloat) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "rgba32f";
		m_image_type_name	= "imageBuffer";
		m_sampler_type_name  = "samplerBuffer";
		m_output_type_name   = "vec4";
		m_value_selector	 = "";
		break;

	case GL_RGBA8I:
		m_n_components		 = 4;
		m_is_image_supported = true;
		m_input_type		 = GL_BYTE;
		m_output_type		 = GL_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLbyte) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "rgba8i";
		m_image_type_name	= "iimageBuffer";
		m_sampler_type_name  = "isamplerBuffer";
		m_output_type_name   = "ivec4";
		m_value_selector	 = "";
		break;

	case GL_RGBA16I:
		m_n_components		 = 4;
		m_is_image_supported = true;
		m_input_type		 = GL_SHORT;
		m_output_type		 = GL_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLshort) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "rgba16i";
		m_image_type_name	= "iimageBuffer";
		m_sampler_type_name  = "isamplerBuffer";
		m_output_type_name   = "ivec4";
		m_value_selector	 = "";
		break;

	case GL_RGBA32I:
		m_n_components		 = 4;
		m_is_image_supported = true;
		m_input_type		 = GL_INT;
		m_output_type		 = GL_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLint) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "rgba32i";
		m_image_type_name	= "iimageBuffer";
		m_sampler_type_name  = "isamplerBuffer";
		m_output_type_name   = "ivec4";
		m_value_selector	 = "";
		break;

	case GL_RGBA8UI:
		m_n_components		 = 4;
		m_is_image_supported = true;
		m_input_type		 = GL_UNSIGNED_BYTE;
		m_output_type		 = GL_UNSIGNED_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLubyte) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLuint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "rgba8ui";
		m_image_type_name	= "uimageBuffer";
		m_sampler_type_name  = "usamplerBuffer";
		m_output_type_name   = "uvec4";
		m_value_selector	 = "";
		break;

	case GL_RGBA16UI:
		m_n_components		 = 4;
		m_is_image_supported = true;
		m_input_type		 = GL_UNSIGNED_SHORT;
		m_output_type		 = GL_UNSIGNED_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLushort) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLuint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "rgba16ui";
		m_image_type_name	= "uimageBuffer";
		m_sampler_type_name  = "usamplerBuffer";
		m_output_type_name   = "uvec4";
		m_value_selector	 = "";
		break;

	case GL_RGBA32UI:
		m_n_components		 = 4;
		m_is_image_supported = true;
		m_input_type		 = GL_UNSIGNED_INT;
		m_output_type		 = GL_UNSIGNED_INT;
		m_not_aligned_size   = static_cast<glw::GLuint>(sizeof(glw::GLuint) * m_n_components);
		m_ssbo_value_size	= static_cast<glw::GLuint>(sizeof(glw::GLuint) * m_n_components);
		m_aligned_size		 = countAlignedSize(m_offset_alignment, m_not_aligned_size);
		m_image_format_name  = "rgba32ui";
		m_image_type_name	= "uimageBuffer";
		m_sampler_type_name  = "usamplerBuffer";
		m_output_type_name   = "uvec4";
		m_value_selector	 = "";
		break;

	default:
		break;
	}
}

/** Computes aligned size
 *
 * @param offsetAlignment value of GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT_EXT parameter
 * @param totalSize       total size of input data per texel
 *
 * @return                properly aligned size
 */
glw::GLuint FormatInfo::countAlignedSize(glw::GLuint offsetAlignment, glw::GLuint totalSize)
{
	if (totalSize > offsetAlignment)
	{
		glw::GLuint rest = totalSize % offsetAlignment;
		if (0 == rest)
		{
			return totalSize;
		}
		return totalSize + offsetAlignment - rest;
	}
	else
	{
		return offsetAlignment;
	}
}

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBufferTextureBufferRange::TextureBufferTextureBufferRange(Context& context, const ExtParameters& extParams,
																 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_cs_id(0)
	, m_cs_po_id(0)
	, m_ssbo_size_id(0)
	, m_ssbo_value_id(0)
	, m_tbo_id(0)
	, m_tbo_tex_id(0)
	, m_texture_buffer_offset_alignment(0)
	, m_vao_id(0)
	, m_vs_id(0)
	, m_fs_id(0)
	, m_vsfs_po_id(0)
	, m_tf_size_buffer_id(0)
	, m_tf_value_buffer_id(0)
	, m_buffer_total_size(0)
{
	/* Nothing to be done here */
}

/** Initializes GLES objects used during the test.
 *
 */
void TextureBufferTextureBufferRange::initTest(void)
{
	/* Check if texture buffer extension is supported */
	if (!m_is_texture_buffer_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BUFFER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.getIntegerv(m_glExtTokens.TEXTURE_BUFFER_OFFSET_ALIGNMENT, &m_texture_buffer_offset_alignment);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT_EXT parameter value!");

	/* Create list of textures internal formats */
	std::vector<glw::GLenum> formats;

	formats.push_back(GL_R8);
	formats.push_back(GL_R16F);
	formats.push_back(GL_R32F);
	formats.push_back(GL_R8I);
	formats.push_back(GL_R16I);
	formats.push_back(GL_R32I);
	formats.push_back(GL_R8UI);
	formats.push_back(GL_R16UI);
	formats.push_back(GL_R32UI);
	formats.push_back(GL_RG8);
	formats.push_back(GL_RG16F);
	formats.push_back(GL_RG32F);
	formats.push_back(GL_RG8I);
	formats.push_back(GL_RG16I);
	formats.push_back(GL_RG32I);
	formats.push_back(GL_RG8UI);
	formats.push_back(GL_RG16UI);
	formats.push_back(GL_RG32UI);
	formats.push_back(GL_RGB32F);
	formats.push_back(GL_RGB32I);
	formats.push_back(GL_RGB32UI);
	formats.push_back(GL_RGBA8);
	formats.push_back(GL_RGBA16F);
	formats.push_back(GL_RGBA32F);
	formats.push_back(GL_RGBA8I);
	formats.push_back(GL_RGBA16I);
	formats.push_back(GL_RGBA32I);
	formats.push_back(GL_RGBA8UI);
	formats.push_back(GL_RGBA16UI);
	formats.push_back(GL_RGBA32UI);

	/* Create configuration for internal formats and add them to the map */
	for (glw::GLuint i = 0; i < formats.size(); ++i)
	{
		m_configurations[formats[i]] = FormatInfo(formats[i], m_texture_buffer_offset_alignment);
		m_buffer_total_size += m_configurations[formats[i]].get_aligned_size();
	}

	std::vector<glw::GLubyte> buffer(m_buffer_total_size);
	memset(&buffer[0], 0, m_buffer_total_size);

	glw::GLuint offset = 0;
	for (std::map<glw::GLenum, FormatInfo>::iterator it = m_configurations.begin(); it != m_configurations.end(); ++it)
	{
		FormatInfo& info = it->second;
		fillInputData(&buffer[0], offset, info);
		offset += info.get_aligned_size();
	}

	/* Set up a vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	gl.bindVertexArray(m_vao_id);

	/* Create buffer object */
	gl.genBuffers(1, &m_tbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");
	gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, m_tbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object !");
	gl.bufferData(m_glExtTokens.TEXTURE_BUFFER, m_buffer_total_size, &buffer[0], GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating buffer object's data store!");
}

/** Returns Compute shader Code
 *
 * @return pointer to literal with Compute Shader Code
 */
std::string TextureBufferTextureBufferRange::getComputeShaderCode(FormatInfo& info) const
{
	std::stringstream result;

	result << "${VERSION}\n"
			  "\n"
			  "${TEXTURE_BUFFER_REQUIRE}\n"
			  "\n"
			  "precision highp float;\n"
			  "\n"
			  "layout("
		   << info.get_image_format_name().c_str() << ", binding = 0) uniform highp readonly "
		   << info.get_image_type_name().c_str() << " image_buffer;\n"
													"\n"
													"layout(binding = 0 ) buffer ComputeSSBOSize\n"
													"{\n"
													"    int outImageSize;\n"
													"} computeSSBOSize;\n"
													"layout(binding = 1 ) buffer ComputeSSBOSizeValue\n"
													"{\n"
													"    "
		   << info.get_output_type_name().c_str() << " outValue;\n"
													 "} computeSSBOValue;\n"
													 "\n"
													 "layout (local_size_x = 1 ) in;\n"
													 "\n"
													 "void main(void)\n"
													 "{\n"
													 "    computeSSBOValue.outValue    = imageLoad( image_buffer, 0)"
		   << info.get_value_selector().c_str() << ";\n"
												   "    computeSSBOSize.outImageSize = imageSize( image_buffer );\n"
												   "}\n";

	return result.str();
}

/** Returns Fragment shader Code
 *
 * @return pointer to literal with Fragment Shader Code
 */
std::string TextureBufferTextureBufferRange::getFragmentShaderCode(FormatInfo& info) const
{
	DE_UNREF(info);

	std::stringstream result;

	result << "${VERSION}\n"
			  "\n"
			  "${TEXTURE_BUFFER_REQUIRE}\n"
			  "\n"
			  "precision highp float;\n"
			  "\n"
			  "void main(void)\n"
			  "{\n"
			  "}\n";

	return result.str();
}

/** Returns Vertex shader Code
 *
 * @return pointer to literal with Vertex Shader Code
 */
std::string TextureBufferTextureBufferRange::getVertexShaderCode(FormatInfo& info) const
{
	std::stringstream result;

	const char* pszInterpolationQualifier = "";

	if (info.get_output_type() == GL_UNSIGNED_INT || info.get_output_type() == GL_INT)
	{
		pszInterpolationQualifier = "flat ";
	}

	result << "${VERSION}\n"
			  "\n"
			  "${TEXTURE_BUFFER_REQUIRE}\n"
			  "\n"
			  "precision highp float;\n"
			  "\n"
			  "uniform highp "
		   << info.get_sampler_type_name().c_str() << " sampler_buffer;\n"
													  "\n"
													  "layout(location = 0 ) out flat int outTextureSize;\n"
													  "layout(location = 1 ) out "
		   << pszInterpolationQualifier << info.get_output_type_name().c_str()
		   << " outValue;\n"
			  "void main(void)\n"
			  "{\n"
			  "    outValue       = texelFetch ( sampler_buffer, 0)"
		   << info.get_value_selector().c_str() << ";\n"
												   "    outTextureSize = textureSize( sampler_buffer );\n"
												   "    gl_Position    = vec4(0.0f, 0.0f, 0.0f, 0.0f);\n"
												   "}\n";

	return result.str();
}

/** Delete GLES objects created for iteration */
void TextureBufferTextureBufferRange::cleanIteration()
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES state */
	gl.useProgram(0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, 0);
	gl.bindTexture(m_glExtTokens.TEXTURE_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error reseting GLES state after iteration!");

	/* Delete GLES objects */
	if (0 != m_vsfs_po_id)
	{
		gl.deleteProgram(m_vsfs_po_id);
		m_vsfs_po_id = 0;
	}

	if (0 != m_vs_id)
	{
		gl.deleteShader(m_vs_id);
		m_vs_id = 0;
	}

	if (0 != m_fs_id)
	{
		gl.deleteShader(m_fs_id);
		m_fs_id = 0;
	}

	if (0 != m_tf_size_buffer_id)
	{
		gl.deleteBuffers(1, &m_tf_size_buffer_id);
		m_tf_size_buffer_id = 0;
	}

	if (0 != m_tf_value_buffer_id)
	{
		gl.deleteBuffers(1, &m_tf_value_buffer_id);
		m_tf_value_buffer_id = 0;
	}

	if (0 != m_cs_po_id)
	{
		gl.deleteProgram(m_cs_po_id);
		m_cs_po_id = 0;
	}

	if (0 != m_cs_id)
	{
		gl.deleteShader(m_cs_id);
		m_cs_id = 0;
	}

	if (0 != m_ssbo_size_id)
	{
		gl.deleteBuffers(1, &m_ssbo_size_id);
		m_ssbo_size_id = 0;
	}

	if (0 != m_ssbo_value_id)
	{
		gl.deleteBuffers(1, &m_ssbo_value_id);
		m_ssbo_value_id = 0;
	}

	if (0 != m_tbo_tex_id)
	{
		gl.deleteTextures(1, &m_tbo_tex_id);
		m_tbo_tex_id = 0;
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error removing iteration resources!");
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult TextureBufferTextureBufferRange::iterate(void)
{
	/* Initialize */
	initTest();

	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool		testResult = true;
	glw::GLuint offset	 = 0;

	for (std::map<glw::GLenum, FormatInfo>::iterator it = m_configurations.begin(); it != m_configurations.end(); ++it)
	{
		FormatInfo& info = it->second;

		/* Create texture object */
		gl.genTextures(1, &m_tbo_tex_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating texture object!");
		gl.activeTexture(GL_TEXTURE0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active texture unit!");
		gl.bindTexture(m_glExtTokens.TEXTURE_BUFFER, m_tbo_tex_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture buffer object!");
		gl.texBufferRange(m_glExtTokens.TEXTURE_BUFFER, info.get_internal_format(), m_tbo_id, offset,
						  info.get_not_aligned_size());
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting buffer object as data store for texture buffer!");

		/* Create transform feedback buffer objects */
		gl.genBuffers(1, &m_tf_size_buffer_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");
		gl.bindBuffer(GL_ARRAY_BUFFER, m_tf_size_buffer_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object!");
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(glw::GLint), DE_NULL, GL_DYNAMIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating buffer object's data store!");

		std::vector<glw::GLubyte> buffer(info.get_ssbo_value_size());
		memset(&buffer[0], 0, info.get_ssbo_value_size());

		gl.genBuffers(1, &m_tf_value_buffer_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");
		gl.bindBuffer(GL_ARRAY_BUFFER, m_tf_value_buffer_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object!");
		gl.bufferData(GL_ARRAY_BUFFER, info.get_ssbo_value_size(), &buffer[0], GL_DYNAMIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating buffer object's data store!");

		/* Configure program object using Vertex Shader and Fragment Shader */
		m_vsfs_po_id = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating program object!");

		const char* varyings[] = { "outTextureSize", "outValue" };

		gl.transformFeedbackVaryings(m_vsfs_po_id, 2, varyings, GL_SEPARATE_ATTRIBS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting transform feedback varyings!");

		m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating shader object!");

		m_vs_id = gl.createShader(GL_VERTEX_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating shader object!");

		std::string fsSource = getFragmentShaderCode(info);
		std::string vsSource = getVertexShaderCode(info);
		const char* fsCode   = fsSource.c_str();
		const char* vsCode   = vsSource.c_str();

		if (!buildProgram(m_vsfs_po_id, m_fs_id, 1, &fsCode, m_vs_id, 1, &vsCode))
		{
			TCU_FAIL("Could not build program from valid vertex/fragment shader code!");
		}

		gl.useProgram(m_vsfs_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active program object!");

		glw::GLint sampler_location = gl.getUniformLocation(m_vsfs_po_id, "sampler_buffer");
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting uniform location!");
		if (sampler_location == -1)
		{
			TCU_FAIL("Failed to get uniform location for valid uniform variable");
		}

		gl.uniform1i(sampler_location, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting value for uniform location!");

		/* Configure transform feedback */
		gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_tf_size_buffer_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object to transform feedback binding point.");
		gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, m_tf_value_buffer_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object to transform feedback binding point.");

		gl.beginTransformFeedback(GL_POINTS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error beginning transform feedback.");

		/* Render */
		gl.drawArrays(GL_POINTS, 0, 1);
		glw::GLenum glerr = gl.getError();

		gl.endTransformFeedback();
		GLU_EXPECT_NO_ERROR(glerr, "Error drawing arrays");
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error ending transform feedback.");

		gl.memoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting memory barrier.");

		/* Check results */
		if (!checkResult(info, "Vertex/Fragment shader", true))
		{
			testResult = false;
		}

		/* Run compute shader if internal format is supported by image_load_store */
		if (info.get_is_image_supported())
		{
			/* Create Shader Storage Buffer Object */
			gl.genBuffers(1, &m_ssbo_size_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");
			gl.bindBuffer(GL_ARRAY_BUFFER, m_ssbo_size_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding  buffer object!");
			gl.bufferData(GL_ARRAY_BUFFER, sizeof(glw::GLint), 0, GL_DYNAMIC_COPY);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating buffer object's data store!");

			gl.genBuffers(1, &m_ssbo_value_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");
			gl.bindBuffer(GL_ARRAY_BUFFER, m_ssbo_value_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding  buffer object!");
			gl.bufferData(GL_ARRAY_BUFFER, info.get_ssbo_value_size(), &buffer[0], GL_DYNAMIC_COPY);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating buffer object's data store!");

			/* Create compute shader program */
			m_cs_po_id = gl.createProgram();
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating program object!");

			m_cs_id = gl.createShader(GL_COMPUTE_SHADER);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating shader object!");

			std::string csSource = getComputeShaderCode(info);
			const char* csCode   = csSource.c_str();

			if (!buildProgram(m_cs_po_id, m_cs_id, 1, &csCode))
			{
				TCU_FAIL("Could not build program from valid compute shader code!");
			}

			gl.useProgram(m_cs_po_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active program object!");

			/* Configure SSBO objects */
			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssbo_size_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object to shader storage binding point!");
			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_ssbo_value_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object to shader storage binding point!");

			/* Bind texture to image unit*/
			gl.bindImageTexture(0, m_tbo_tex_id, 0, GL_FALSE, 0, GL_READ_ONLY, info.get_internal_format());
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit 0!");

			/* Run compute shader */
			gl.dispatchCompute(1, 1, 1);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error running compute shader!");

			gl.memoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting memory barrier!");

			if (!checkResult(info, "Compute shader", false))
			{
				testResult = false;
			}
		}

		offset += info.get_aligned_size();
		cleanIteration();
	}

	if (testResult)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Check test results
 *
 *   @param info              test configuration
 *   @param phase             name of test phase
 *   @param transformFeedback contains information if verification method should use transformfeedback, otherwise it uses SSBO
 *
 *   @return                  return true if result values are es expected, otherwise return false
 */
bool TextureBufferTextureBufferRange::checkResult(FormatInfo& info, const char* phase, bool transformFeedback)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLenum bufferTarget  = GL_NONE;
	glw::GLuint bufferSizeId  = GL_NONE;
	glw::GLuint bufferValueId = GL_NONE;

	if (transformFeedback)
	{
		bufferTarget  = GL_TRANSFORM_FEEDBACK_BUFFER;
		bufferSizeId  = m_tf_size_buffer_id;
		bufferValueId = m_tf_value_buffer_id;
	}
	else
	{
		bufferTarget  = GL_SHADER_STORAGE_BUFFER;
		bufferSizeId  = m_ssbo_size_id;
		bufferValueId = m_ssbo_value_id;
	}

	bool testResult = true;

	/* Get result data */
	gl.bindBuffer(bufferTarget, bufferSizeId);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer!");

	glw::GLint* size = (glw::GLint*)gl.mapBufferRange(bufferTarget, 0, sizeof(glw::GLint), GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error mapping buffer!");

	const glw::GLint expectedSize = 1;

	/* Reagrding test specification size should be equal 1*/
	/* Log error if expected and result data are not equal */
	if (*size != expectedSize)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Texture size is wrong for " << phase << " phase \n"
						   << "Internal Format: " << glu::getUncompressedTextureFormatName(info.get_internal_format())
						   << "\n"
						   << "Expected size:   " << expectedSize << "\n"
						   << "Result size:     " << size << "\n"
						   << tcu::TestLog::EndMessage;
	}

	gl.unmapBuffer(bufferTarget);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error unmapping buffer!");

	gl.bindBuffer(bufferTarget, bufferValueId);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer!");

	std::stringstream expVal;
	std::stringstream resVal;

	/* Log error if expected and result data are not equal */
	switch (info.get_output_type())
	{
	case GL_FLOAT:
	{
		glw::GLfloat* res =
			(glw::GLfloat*)gl.mapBufferRange(bufferTarget, 0, info.get_ssbo_value_size(), GL_MAP_READ_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error mapping buffer!");

		std::vector<glw::GLfloat> expected(info.get_n_components());

		fillOutputData(reinterpret_cast<glw::GLubyte*>(&expected[0]), info);

		for (glw::GLuint i = 0; i < info.get_n_components(); ++i)
		{
			if (de::abs(res[i] - expected[i]) > m_epsilon_float)
			{
				expVal.str("");
				resVal.str("");
				expVal << expected[i];
				resVal << res[i];

				logError(phase, glu::getUncompressedTextureFormatName(info.get_internal_format()), i,
						 expVal.str().c_str(), resVal.str().c_str());

				testResult = false;
			}
		}
	}
	break;

	case GL_INT:
	{
		glw::GLint* res = (glw::GLint*)gl.mapBufferRange(bufferTarget, 0, info.get_ssbo_value_size(), GL_MAP_READ_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error mapping buffer!");

		std::vector<glw::GLint> expected(info.get_n_components());

		fillOutputData(reinterpret_cast<glw::GLubyte*>(&expected[0]), info);

		for (glw::GLuint i = 0; i < info.get_n_components(); ++i)
		{
			if (res[i] != expected[i])
			{
				expVal.str("");
				resVal.str("");
				expVal << expected[i];
				resVal << res[i];

				logError(phase, glu::getUncompressedTextureFormatName(info.get_internal_format()), i,
						 expVal.str().c_str(), resVal.str().c_str());

				testResult = false;
			}
		}
	}
	break;

	case GL_UNSIGNED_INT:
	{
		glw::GLuint* res =
			(glw::GLuint*)gl.mapBufferRange(bufferTarget, 0, info.get_ssbo_value_size(), GL_MAP_READ_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error mapping buffer!");

		std::vector<glw::GLuint> expected(info.get_n_components());

		fillOutputData(reinterpret_cast<glw::GLubyte*>(&expected[0]), info);

		for (glw::GLuint i = 0; i < info.get_n_components(); ++i)
		{
			if (res[i] != expected[i])
			{
				expVal.str("");
				resVal.str("");
				expVal << expected[i];
				resVal << res[i];

				logError(phase, glu::getUncompressedTextureFormatName(info.get_internal_format()), i,
						 expVal.str().c_str(), resVal.str().c_str());

				testResult = false;
			}
		}
	}
	break;

	default:
	{
		gl.unmapBuffer(bufferTarget);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error ummapping buffer!");

		TCU_FAIL("Not allowed output type");
	}
	}

	gl.unmapBuffer(bufferTarget);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error ummapping buffer!");

	return testResult;
}

/** Create error message and log it
 *
 * @param phase          name of phase
 * @param internalFormat name of internal format
 * @param component      component position
 * @param exptectedValue string with expected value
 * @param resultValue    string with result   value
 *
 */
void TextureBufferTextureBufferRange::logError(const char* phase, const char* internalFormat, glw::GLuint component,
											   const char* exptectedValue, const char* resultValue)
{
	m_testCtx.getLog() << tcu::TestLog::Message << "Result is different for " << phase << " phase \n"
					   << "Internal Format: " << internalFormat << "\n"
					   << "Component Position: " << component << "\n"
					   << "Expected value: " << exptectedValue << "\n"
					   << "Result value: " << resultValue << "\n"
					   << tcu::TestLog::EndMessage;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void TextureBufferTextureBufferRange::deinit(void)
{
	/* Call base class' deinit() */
	TestCaseBase::deinit();

	/* Check if texture buffer extension is supported */
	if (!m_is_texture_buffer_supported)
	{
		return;
	}

	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES state */
	gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, 0);
	gl.bindVertexArray(0);

	/* Delete GLES objects */
	if (0 != m_tbo_id)
	{
		gl.deleteBuffers(1, &m_tbo_id);
		m_tbo_id = 0;
	}

	if (0 != m_vao_id)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
	}

	cleanIteration();
}

/** Fill buffer with input data for texture buffer
 *
 * @param buffer buffer where data will be stored
 * @param offset offset into buffer
 * @param info   test configuration
 *
 */
void TextureBufferTextureBufferRange::fillInputData(glw::GLubyte* buffer, glw::GLuint offset, FormatInfo& info)
{
	/* Initial value should give value equal to 1 on output */

	/* Check input data type */
	switch (info.get_input_type())
	{
	case GL_FLOAT:
	{
		glw::GLfloat* pointer = reinterpret_cast<glw::GLfloat*>(&buffer[offset]);
		for (glw::GLuint i = 0; i < info.get_n_components(); ++i)
		{
			*pointer++ = 1.0f;
		}
	}
	break;

	case GL_HALF_FLOAT:
	{
		glw::GLhalf* pointer = reinterpret_cast<glw::GLhalf*>(&buffer[offset]);

		glw::GLfloat val = 1.0f;

		/* Convert float to half float */
		glw::GLuint	temp32   = 0;
		glw::GLuint	temp32_2 = 0;
		unsigned short temp16   = 0;
		unsigned short temp16_2 = 0;

		memcpy(&temp32, &val, sizeof(glw::GLfloat));
		temp32_2 = temp32 >> 31;
		temp16   = (unsigned short)((temp32_2 >> 31) << 5);
		temp32_2 = temp32 >> 23;
		temp16_2 = temp32_2 & 0xff;
		temp16_2 = (unsigned short)((temp16_2 - 0x70) & ((glw::GLuint)((glw::GLint)(0x70 - temp16_2) >> 4) >> 27));
		temp16   = (unsigned short)((temp16 | temp16_2) << 10);
		temp32_2 = temp32 >> 13;
		temp16   = (unsigned short)(temp16 | (temp32_2 & 0x3ff));

		for (glw::GLuint i = 0; i < info.get_n_components(); ++i)
		{
			*pointer++ = temp16;
		}
	}
	break;

	case GL_UNSIGNED_BYTE:
	{
		glw::GLubyte* pointer = reinterpret_cast<glw::GLubyte*>(&buffer[offset]);
		glw::GLubyte  val	 = 0;

		/* Normalized Values */
		if (info.get_internal_format() == GL_R8 || info.get_internal_format() == GL_RG8 ||
			info.get_internal_format() == GL_RGBA8)
		{
			val = 255;
		}
		else
		{
			val = 1;
		}
		for (glw::GLuint i = 0; i < info.get_n_components(); ++i)
		{
			*pointer++ = val;
		}
	}
	break;

	case GL_UNSIGNED_SHORT:
	{
		glw::GLushort* pointer = reinterpret_cast<glw::GLushort*>(&buffer[offset]);

		for (glw::GLuint i = 0; i < info.get_n_components(); ++i)
		{
			*pointer++ = (glw::GLushort)1;
		}
	}
	break;
	case GL_UNSIGNED_INT:
	{
		glw::GLuint* pointer = reinterpret_cast<glw::GLuint*>(&buffer[offset]);

		for (glw::GLuint i = 0; i < info.get_n_components(); ++i)
		{
			*pointer++ = 1;
		}
	}
	break;
	case GL_INT:
	{
		glw::GLint* pointer = reinterpret_cast<glw::GLint*>(&buffer[offset]);
		for (glw::GLuint i = 0; i < info.get_n_components(); ++i)
		{
			*pointer++ = 1;
		}
	}
	break;
	case GL_SHORT:
	{
		glw::GLshort* pointer = reinterpret_cast<glw::GLshort*>(&buffer[offset]);
		for (glw::GLuint i = 0; i < info.get_n_components(); ++i)
		{
			*pointer++ = (glw::GLshort)1;
		}
	}
	break;
	case GL_BYTE:
	{
		glw::GLbyte* pointer = reinterpret_cast<glw::GLbyte*>(&buffer[offset]);
		for (glw::GLuint i = 0; i < info.get_n_components(); ++i)
		{
			*pointer++ = (glw::GLbyte)1;
		}
	}
	break;
	default:
		TCU_FAIL("Not allowed input type");
	}
}

/** Fill buffer with output data for texture buffer
 *
 * @param buffer buffer where data will be stored
 * @param info   test configuration
 *
 */
void TextureBufferTextureBufferRange::fillOutputData(glw::GLubyte* buffer, FormatInfo& info)
{
	switch (info.get_output_type())
	{
	case GL_FLOAT:
	{
		glw::GLfloat* pointer = reinterpret_cast<glw::GLfloat*>(&buffer[0]);
		for (unsigned int i = 0; i < info.get_n_components(); ++i)
		{
			*pointer++ = 1.0f;
		}
	}
	break;

	case GL_INT:
	{
		glw::GLint* pointer = reinterpret_cast<glw::GLint*>(&buffer[0]);
		for (unsigned int i = 0; i < info.get_n_components(); ++i)
		{
			*pointer++ = 1;
		}
	}
	break;

	case GL_UNSIGNED_INT:
	{
		glw::GLuint* pointer = reinterpret_cast<glw::GLuint*>(&buffer[0]);
		for (unsigned int i = 0; i < info.get_n_components(); ++i)
		{
			*pointer++ = 1;
		}
	}
	break;

	default:
		TCU_FAIL("Not allowed output type");
	}
}

} // namespace glcts
