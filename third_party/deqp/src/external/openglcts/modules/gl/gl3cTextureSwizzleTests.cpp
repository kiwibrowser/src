/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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

/**
 * \file  gl3TextureSwizzleTests.cpp
 * \brief Implements conformance tests for "Texture Swizzle" functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl3cTextureSwizzleTests.hpp"
#include "gluContextInfo.hpp"
#include "gluStrUtil.hpp"
#include "glwFunctions.hpp"
#include "tcuFloat.hpp"
#include "tcuTestLog.hpp"

#include "deMath.h"
#include <cmath>

#define ENABLE_DEBUG 0							   /* Selects if debug callback is installed */
#define FUNCTIONAL_TEST_ALL_FORMATS 1			   /* Selects if all formats should be tested */
#define FUNCTIONAL_TEST_ALL_TARGETS 1			   /* Selects if all targets should be tested */
#define FUNCTIONAL_TEST_ALL_ACCESS_ROUTINES 0	  /* Selects if all texture access routines should be tested */
#define FUNCTIONAL_TEST_ALL_SWIZZLE_COMBINATIONS 0 /* Selects if all swizzle combinations should be tested */

namespace gl3cts
{
namespace TextureSwizzle
{
/* Static constants use by tests */
/* One and zero */
static const glw::GLhalf   data_float16_one[]  = { tcu::Float16(1.0).bits() };
static const glw::GLhalf   data_float16_zero[] = { tcu::Float16(0.0).bits() };
static const glw::GLfloat  data_float32_one[]  = { 1.0f };
static const glw::GLfloat  data_float32_zero[] = { 0.0f };
static const glw::GLbyte   data_snorm8_zero[]  = { 0 };
static const glw::GLbyte   data_snorm8_one[]   = { 127 };
static const glw::GLshort  data_snorm16_one[]  = { 32767 };
static const glw::GLshort  data_snorm16_zero[] = { 0 };
static const glw::GLubyte  data_unorm8_one[]   = { 255 };
static const glw::GLubyte  data_unorm8_zero[]  = { 0 };
static const glw::GLushort data_unorm16_one[]  = { 65535 };
static const glw::GLushort data_unorm16_zero[] = { 0 };
static const glw::GLbyte   data_sint8_zero[]   = { 0 };
static const glw::GLbyte   data_sint8_one[]	= { 1 };
static const glw::GLshort  data_sint16_one[]   = { 1 };
static const glw::GLshort  data_sint16_zero[]  = { 0 };
static const glw::GLint	data_sint32_one[]   = { 1 };
static const glw::GLint	data_sint32_zero[]  = { 0 };
static const glw::GLubyte  data_uint8_one[]	= { 1 };
static const glw::GLubyte  data_uint8_zero[]   = { 0 };
static const glw::GLushort data_uint16_one[]   = { 1 };
static const glw::GLushort data_uint16_zero[]  = { 0 };
static const glw::GLuint   data_uint32_one[]   = { 1 };
static const glw::GLuint   data_uint32_zero[]  = { 0 };

/* Source and expected data */
static const glw::GLubyte  src_data_r8[]		   = { 123 };
static const glw::GLbyte   src_data_r8_snorm[]	 = { -123 };
static const glw::GLushort src_data_r16[]		   = { 12345 };
static const glw::GLshort  src_data_r16_snorm[]	= { -12345 };
static const glw::GLubyte  src_data_rg8[]		   = { 123, 231 };
static const glw::GLbyte   src_data_rg8_snorm[]	= { -123, -23 };
static const glw::GLushort src_data_rg16[]		   = { 12345, 54321 };
static const glw::GLshort  src_data_rg16_snorm[]   = { -12345, -23451 };
static const glw::GLubyte  src_data_r3_g3_b2[]	 = { 236 };   /* 255, 109, 0 */
static const glw::GLushort src_data_rgb4[]		   = { 64832 }; /* 5_6_5: 255, 170, 0 */
static const glw::GLushort src_data_rgb5[]		   = { 64832 };
static const glw::GLubyte  src_data_rgb8[]		   = { 3, 2, 1 };
static const glw::GLbyte   src_data_rgb8_snorm[]   = { -1, -2, -3 };
static const glw::GLushort src_data_rgb16[]		   = { 65535, 32767, 16383 };
static const glw::GLshort  src_data_rgb16_snorm[]  = { -32767, -16383, -8191 };
static const glw::GLushort src_data_rgba4[]		   = { 64005 }; /* 255, 170, 0, 85 */
static const glw::GLushort src_data_rgb5_a1[]	  = { 64852 };
static const glw::GLubyte  src_data_rgba8[]		   = { 0, 64, 128, 255 };
static const glw::GLbyte   src_data_rgba8_snorm[]  = { -127, -63, -32, -16 };
static const glw::GLuint   src_data_rgb10_a2[]	 = { 4291823615u };
static const glw::GLushort exp_data_rgb10_a2ui[]   = { 1023, 256, 511, 3 };
static const glw::GLushort src_data_rgba16[]	   = { 65535, 32767, 16383, 8191 };
static const glw::GLshort  src_data_rgba16_snorm[] = { -32767, -16383, -8191, -4091 };
static const glw::GLubyte  exp_data_srgb8_alpha8[] = { 13, 1, 255, 32 }; /* See 4.5 core 8.24 */
static const glw::GLubyte  src_data_srgb8_alpha8[] = { 64, 8, 255, 32 };
static const glw::GLhalf   src_data_r16f[]		   = { tcu::Float16(1.0).bits() };
static const glw::GLhalf   src_data_rg16f[]		   = { tcu::Float16(1.0).bits(), tcu::Float16(-1.0).bits() };
static const glw::GLhalf   src_data_rgb16f[]	   = { tcu::Float16(1.0).bits(), tcu::Float16(-1.0).bits(),
											   tcu::Float16(2.0).bits() };
static const glw::GLhalf src_data_rgba16f[] = { tcu::Float16(1.0).bits(), tcu::Float16(-1.0).bits(),
												tcu::Float16(2.0).bits(), tcu::Float16(-2.0).bits() };
static const glw::GLfloat src_data_r32f[]	= { 1.0f };
static const glw::GLfloat src_data_rg32f[]   = { 1.0f, -1.0f };
static const glw::GLfloat src_data_rgb32f[]  = { 1.0f, -1.0f, 2.0f };
static const glw::GLfloat src_data_rgba32f[] = { 1.0f, -1.0f, 2.0f, -2.0f };

static const tcu::Float<deUint32, 5, 6, 15, tcu::FLOAT_SUPPORT_DENORM> r11f(0.5);
static const tcu::Float<deUint32, 5, 6, 15, tcu::FLOAT_SUPPORT_DENORM> g11f(0.75);
static const tcu::Float<deUint32, 5, 5, 15, tcu::FLOAT_SUPPORT_DENORM> b10f(0.25);

static const glw::GLhalf exp_data_r11f_g11f_b10f[] = { tcu::Float16(0.5).bits(), tcu::Float16(0.75).bits(),
													   tcu::Float16(0.25).bits() };
static const glw::GLuint   src_data_r11f_g11f_b10f[]	= { (r11f.bits()) | (g11f.bits() << 11) | (b10f.bits() << 22) };
static const glw::GLfloat  exp_data_rgb9_e5[]			= { 31.0f, 23.0f, 32.0f };
static const glw::GLuint   src_data_rgb9_e5[]			= { 2885775608u };
static const glw::GLbyte   src_data_r8i[]				= { -127 };
static const glw::GLubyte  src_data_r8ui[]				= { 128 };
static const glw::GLshort  src_data_r16i[]				= { -32767 };
static const glw::GLushort src_data_r16ui[]				= { 32768 };
static const glw::GLint	src_data_r32i[]				= { -1 };
static const glw::GLuint   src_data_r32ui[]				= { 1 };
static const glw::GLbyte   src_data_rg8i[]				= { -127, -126 };
static const glw::GLubyte  src_data_rg8ui[]				= { 128, 129 };
static const glw::GLshort  src_data_rg16i[]				= { -32767, -32766 };
static const glw::GLushort src_data_rg16ui[]			= { 32768, 32769 };
static const glw::GLint	src_data_rg32i[]				= { -1, -2 };
static const glw::GLuint   src_data_rg32ui[]			= { 1, 2 };
static const glw::GLbyte   src_data_rgb8i[]				= { -127, -126, -125 };
static const glw::GLubyte  src_data_rgb8ui[]			= { 128, 129, 130 };
static const glw::GLshort  src_data_rgb16i[]			= { -32767, -32766, -32765 };
static const glw::GLushort src_data_rgb16ui[]			= { 32768, 32769, 32770 };
static const glw::GLint	src_data_rgb32i[]			= { -1, -2, -3 };
static const glw::GLuint   src_data_rgb32ui[]			= { 1, 2, 3 };
static const glw::GLbyte   src_data_rgba8i[]			= { -127, -126, -125, -124 };
static const glw::GLubyte  src_data_rgba8ui[]			= { 128, 129, 130, 131 };
static const glw::GLshort  src_data_rgba16i[]			= { -32767, -32766, -32765, -32764 };
static const glw::GLushort src_data_rgba16ui[]			= { 32768, 32769, 32770, 32771 };
static const glw::GLint	src_data_rgba32i[]			= { -1, -2, -3, -4 };
static const glw::GLuint   src_data_rgba32ui[]			= { 1, 2, 3, 4 };
static const glw::GLushort src_data_depth_component16[] = { 32768 };
static const glw::GLfloat  exp_data_depth_component32[] = { 1.0f };
static const glw::GLuint   src_data_depth_component32[] = { 4294967295u };
static const glw::GLfloat  src_data_depth_component32f[] = { 0.75f };
static const glw::GLuint   src_data_depth24_stencil8[]   = { 4294967041u /* 1.0, 1 */ };
static const glw::GLuint   src_data_depth32f_stencil8[]  = { 1065353216, 1 /* 1.0f, 1 */ };

/* Texture channels */
static const glw::GLchar* channels[] = { "r", "g", "b", "a" };

/* Enumerations of cube map faces */
static const glw::GLenum cube_map_faces[] = { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
											  GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, GL_TEXTURE_CUBE_MAP_POSITIVE_X,
											  GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z };
static const size_t n_cube_map_faces = sizeof(cube_map_faces) / sizeof(cube_map_faces[0]);

/* Swizzle states */
static const glw::GLenum states[] = { GL_TEXTURE_SWIZZLE_R, GL_TEXTURE_SWIZZLE_G, GL_TEXTURE_SWIZZLE_B,
									  GL_TEXTURE_SWIZZLE_A };
static const size_t n_states = sizeof(states) / sizeof(states[0]);

/* Sampler descriptor */
struct _sampler
{
	const glw::GLchar* m_basic_type;
	const glw::GLchar* m_sampler_prefix;
};
static const _sampler isampler = { "int", "i" };
static const _sampler usampler = { "uint", "u" };
static const _sampler sampler  = { "float", "" };

/* Output channel descriptor */
struct _out_ch_desc
{
	glw::GLenum		   m_internal_format;
	const glw::GLvoid* m_expected_data;
};

/* Output channel descriptors for one and zero */
static const _out_ch_desc zero_ch	  = { GL_ZERO, 0 };
static const _out_ch_desc one_ch	   = { GL_ONE, 0 };
static const _out_ch_desc float16_zero = { GL_R16F, data_float16_zero };
static const _out_ch_desc float16_one  = { GL_R16F, data_float16_one };
static const _out_ch_desc float32_zero = { GL_R32F, data_float32_zero };
static const _out_ch_desc float32_one  = { GL_R32F, data_float32_one };
static const _out_ch_desc sint8_zero   = { GL_R8I, data_sint8_zero };
static const _out_ch_desc sint8_one	= { GL_R8I, data_sint8_one };
static const _out_ch_desc sint16_zero  = { GL_R16I, data_sint16_zero };
static const _out_ch_desc sint16_one   = { GL_R16I, data_sint16_one };
static const _out_ch_desc sint32_zero  = { GL_R32I, data_sint32_zero };
static const _out_ch_desc sint32_one   = { GL_R32I, data_sint32_one };
static const _out_ch_desc snorm8_zero  = { GL_R8_SNORM, data_snorm8_zero };
static const _out_ch_desc snorm8_one   = { GL_R8_SNORM, data_snorm8_one };
static const _out_ch_desc snorm16_zero = { GL_R16_SNORM, data_snorm16_zero };
static const _out_ch_desc snorm16_one  = { GL_R16_SNORM, data_snorm16_one };
static const _out_ch_desc uint8_zero   = { GL_R8UI, data_uint8_zero };
static const _out_ch_desc uint8_one	= { GL_R8UI, data_uint8_one };
static const _out_ch_desc uint16_zero  = { GL_R16UI, data_uint16_zero };
static const _out_ch_desc uint16_one   = { GL_R16UI, data_uint16_one };
static const _out_ch_desc uint32_zero  = { GL_R32UI, data_uint32_zero };
static const _out_ch_desc uint32_one   = { GL_R32UI, data_uint32_one };
static const _out_ch_desc unorm8_zero  = { GL_R8, data_unorm8_zero };
static const _out_ch_desc unorm8_one   = { GL_R8, data_unorm8_one };
static const _out_ch_desc unorm16_zero = { GL_R16, data_unorm16_zero };
static const _out_ch_desc unorm16_one  = { GL_R16, data_unorm16_one };

/* Texture format descriptor. Maps texture format with output channel descriptors, source data and sampler descriptor */
struct _texture_format
{
	const glu::ApiType  m_minimum_gl_context;
	glw::GLenum			m_internal_format;
	glw::GLenum			m_format;
	glw::GLenum			m_type;
	glw::GLenum			m_ds_mode;
	const _sampler&		m_sampler;
	_out_ch_desc		m_red_ch;
	_out_ch_desc		m_green_ch;
	_out_ch_desc		m_blue_ch;
	_out_ch_desc		m_alpha_ch;
	const glw::GLvoid*  m_source_data;
	const _out_ch_desc& m_zero_ch;
	const _out_ch_desc& m_one_ch;
};
static const _texture_format texture_formats[] = {
	/* 0  */ { glu::ApiType::core(3, 0),
			   GL_R8,
			   GL_RED,
			   GL_UNSIGNED_BYTE,
			   GL_DEPTH_COMPONENT,
			   sampler,
			   { GL_R8, DE_NULL },
			   zero_ch,
			   zero_ch,
			   one_ch,
			   src_data_r8,
			   unorm8_zero,
			   unorm8_one },
	{ glu::ApiType::core(3, 1),
	  GL_R8_SNORM,
	  GL_RED,
	  GL_BYTE,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R8_SNORM, DE_NULL },
	  zero_ch,
	  zero_ch,
	  one_ch,
	  src_data_r8_snorm,
	  snorm8_zero,
	  snorm8_one },
	{ glu::ApiType::core(3, 0),
	  GL_R16,
	  GL_RED,
	  GL_UNSIGNED_SHORT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R16, DE_NULL },
	  zero_ch,
	  zero_ch,
	  one_ch,
	  src_data_r16,
	  unorm16_zero,
	  unorm16_one },
	{ glu::ApiType::core(3, 1),
	  GL_R16_SNORM,
	  GL_RED,
	  GL_SHORT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R16_SNORM, DE_NULL },
	  zero_ch,
	  zero_ch,
	  one_ch,
	  src_data_r16_snorm,
	  snorm16_zero,
	  snorm16_one },
	{ glu::ApiType::core(3, 0),
	  GL_RG8,
	  GL_RG,
	  GL_UNSIGNED_BYTE,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  zero_ch,
	  one_ch,
	  src_data_rg8,
	  unorm8_zero,
	  unorm8_one },
	{ glu::ApiType::core(3, 1),
	  GL_RG8_SNORM,
	  GL_RG,
	  GL_BYTE,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R8_SNORM, DE_NULL },
	  { GL_R8_SNORM, DE_NULL },
	  zero_ch,
	  one_ch,
	  src_data_rg8_snorm,
	  snorm8_zero,
	  snorm8_one },
	{ glu::ApiType::core(3, 0),
	  GL_RG16,
	  GL_RG,
	  GL_UNSIGNED_SHORT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R16, DE_NULL },
	  { GL_R16, DE_NULL },
	  zero_ch,
	  one_ch,
	  src_data_rg16,
	  unorm16_zero,
	  unorm16_one },
	{ glu::ApiType::core(3, 1),
	  GL_RG16_SNORM,
	  GL_RG,
	  GL_SHORT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R16_SNORM, DE_NULL },
	  { GL_R16_SNORM, DE_NULL },
	  zero_ch,
	  one_ch,
	  src_data_rg16_snorm,
	  snorm16_zero,
	  snorm16_one },
	/* 8  */ { glu::ApiType::core(4, 4),
			   GL_R3_G3_B2,
			   GL_RGB,
			   GL_UNSIGNED_BYTE_3_3_2,
			   GL_DEPTH_COMPONENT,
			   sampler,
			   { GL_R8, DE_NULL },
			   { GL_R8, DE_NULL },
			   { GL_R8, DE_NULL },
			   one_ch,
			   src_data_r3_g3_b2,
			   unorm8_zero,
			   unorm8_one },
	{ glu::ApiType::core(4, 4),
	  GL_RGB4,
	  GL_RGB,
	  GL_UNSIGNED_SHORT_5_6_5,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  one_ch,
	  src_data_rgb4,
	  unorm8_zero,
	  unorm8_one },
	{ glu::ApiType::core(4, 4),
	  GL_RGB5,
	  GL_RGB,
	  GL_UNSIGNED_SHORT_5_6_5,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  one_ch,
	  src_data_rgb5,
	  unorm8_zero,
	  unorm8_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGB8,
	  GL_RGB,
	  GL_UNSIGNED_BYTE,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  one_ch,
	  src_data_rgb8,
	  unorm8_zero,
	  unorm8_one },
	{ glu::ApiType::core(3, 1),
	  GL_RGB8_SNORM,
	  GL_RGB,
	  GL_BYTE,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R8_SNORM, DE_NULL },
	  { GL_R8_SNORM, DE_NULL },
	  { GL_R8_SNORM, DE_NULL },
	  one_ch,
	  src_data_rgb8_snorm,
	  snorm8_zero,
	  snorm8_one },
	{ glu::ApiType::core(4, 4),
	  GL_RGB10,
	  GL_RGB,
	  GL_UNSIGNED_SHORT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R16, DE_NULL },
	  { GL_R16, DE_NULL },
	  { GL_R16, DE_NULL },
	  one_ch,
	  src_data_rgb16,
	  unorm16_zero,
	  unorm16_one },
	{ glu::ApiType::core(4, 4),
	  GL_RGB12,
	  GL_RGB,
	  GL_UNSIGNED_SHORT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R16, DE_NULL },
	  { GL_R16, DE_NULL },
	  { GL_R16, DE_NULL },
	  one_ch,
	  src_data_rgb16,
	  unorm16_zero,
	  unorm16_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGB16,
	  GL_RGB,
	  GL_UNSIGNED_SHORT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R16, DE_NULL },
	  { GL_R16, DE_NULL },
	  { GL_R16, DE_NULL },
	  one_ch,
	  src_data_rgb16,
	  unorm16_zero,
	  unorm16_one },
	/* 16 */ { glu::ApiType::core(3, 1),
			   GL_RGB16_SNORM,
			   GL_RGB,
			   GL_SHORT,
			   GL_DEPTH_COMPONENT,
			   sampler,
			   { GL_R16_SNORM, DE_NULL },
			   { GL_R16_SNORM, DE_NULL },
			   { GL_R16_SNORM, DE_NULL },
			   one_ch,
			   src_data_rgb16_snorm,
			   snorm16_zero,
			   snorm16_one },
	{ glu::ApiType::core(4, 4),
	  GL_RGBA2,
	  GL_RGBA,
	  GL_UNSIGNED_SHORT_4_4_4_4,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  src_data_rgba4,
	  unorm8_zero,
	  unorm8_one },
	{ glu::ApiType::core(4, 2),
	  GL_RGBA4,
	  GL_RGBA,
	  GL_UNSIGNED_SHORT_4_4_4_4,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  src_data_rgba4,
	  unorm8_zero,
	  unorm8_one },
	{ glu::ApiType::core(4, 2),
	  GL_RGB5_A1,
	  GL_RGBA,
	  GL_UNSIGNED_SHORT_5_5_5_1,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  src_data_rgb5_a1,
	  unorm8_zero,
	  unorm8_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGBA8,
	  GL_RGBA,
	  GL_UNSIGNED_BYTE,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  { GL_R8, DE_NULL },
	  src_data_rgba8,
	  unorm8_zero,
	  unorm8_one },
	{ glu::ApiType::core(3, 1),
	  GL_RGBA8_SNORM,
	  GL_RGBA,
	  GL_BYTE,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R8_SNORM, DE_NULL },
	  { GL_R8_SNORM, DE_NULL },
	  { GL_R8_SNORM, DE_NULL },
	  { GL_R8_SNORM, DE_NULL },
	  src_data_rgba8_snorm,
	  snorm8_zero,
	  snorm8_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGB10_A2,
	  GL_RGBA,
	  GL_UNSIGNED_INT_10_10_10_2,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R16, DE_NULL },
	  { GL_R16, DE_NULL },
	  { GL_R16, DE_NULL },
	  { GL_R16, DE_NULL },
	  src_data_rgb10_a2,
	  unorm16_zero,
	  unorm16_one },
	{ glu::ApiType::core(3, 3),
	  GL_RGB10_A2UI,
	  GL_RGBA_INTEGER,
	  GL_UNSIGNED_INT_10_10_10_2,
	  GL_DEPTH_COMPONENT,
	  usampler,
	  { GL_R16UI, exp_data_rgb10_a2ui + 0 },
	  { GL_R16UI, exp_data_rgb10_a2ui + 1 },
	  { GL_R16UI, exp_data_rgb10_a2ui + 2 },
	  { GL_R16UI, exp_data_rgb10_a2ui + 3 },
	  src_data_rgb10_a2,
	  uint16_zero,
	  uint16_one },
	/* 24 */ { glu::ApiType::core(4, 4),
			   GL_RGBA12,
			   GL_RGBA,
			   GL_UNSIGNED_SHORT,
			   GL_DEPTH_COMPONENT,
			   sampler,
			   { GL_R16, DE_NULL },
			   { GL_R16, DE_NULL },
			   { GL_R16, DE_NULL },
			   { GL_R16, DE_NULL },
			   src_data_rgba16,
			   unorm16_zero,
			   unorm16_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGBA16,
	  GL_RGBA,
	  GL_UNSIGNED_SHORT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R16, DE_NULL },
	  { GL_R16, DE_NULL },
	  { GL_R16, DE_NULL },
	  { GL_R16, DE_NULL },
	  src_data_rgba16,
	  unorm16_zero,
	  unorm16_one },
	{ glu::ApiType::core(3, 1),
	  GL_RGBA16_SNORM,
	  GL_RGBA,
	  GL_SHORT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R16_SNORM, DE_NULL },
	  { GL_R16_SNORM, DE_NULL },
	  { GL_R16_SNORM, DE_NULL },
	  { GL_R16_SNORM, src_data_rgba16_snorm + 3 },
	  src_data_rgba16_snorm,
	  snorm16_zero,
	  snorm16_one },
	{ glu::ApiType::core(3, 0),
	  GL_SRGB8,
	  GL_RGB,
	  GL_UNSIGNED_BYTE,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R8, exp_data_srgb8_alpha8 + 0 },
	  { GL_R8, exp_data_srgb8_alpha8 + 1 },
	  { GL_R8, exp_data_srgb8_alpha8 + 2 },
	  one_ch,
	  src_data_srgb8_alpha8,
	  unorm8_zero,
	  unorm8_one },
	{ glu::ApiType::core(3, 0),
	  GL_SRGB8_ALPHA8,
	  GL_RGBA,
	  GL_UNSIGNED_BYTE,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R8, exp_data_srgb8_alpha8 + 0 },
	  { GL_R8, exp_data_srgb8_alpha8 + 1 },
	  { GL_R8, exp_data_srgb8_alpha8 + 2 },
	  { GL_R8, exp_data_srgb8_alpha8 + 3 },
	  src_data_srgb8_alpha8,
	  unorm8_zero,
	  unorm8_one },
	{ glu::ApiType::core(3, 0),
	  GL_R16F,
	  GL_RED,
	  GL_HALF_FLOAT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R16F, src_data_r16f + 0 },
	  zero_ch,
	  zero_ch,
	  one_ch,
	  src_data_r16f,
	  float16_zero,
	  float16_one },
	{ glu::ApiType::core(3, 0),
	  GL_RG16F,
	  GL_RG,
	  GL_HALF_FLOAT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R16F, src_data_rg16f + 0 },
	  { GL_R16F, src_data_rg16f + 1 },
	  zero_ch,
	  one_ch,
	  src_data_rg16f,
	  float16_zero,
	  float16_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGB16F,
	  GL_RGB,
	  GL_HALF_FLOAT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R16F, src_data_rgb16f + 0 },
	  { GL_R16F, src_data_rgb16f + 1 },
	  { GL_R16F, src_data_rgb16f + 2 },
	  one_ch,
	  src_data_rgb16f,
	  float16_zero,
	  float16_one },
	/* 32 */ { glu::ApiType::core(3, 0),
			   GL_RGBA16F,
			   GL_RGBA,
			   GL_HALF_FLOAT,
			   GL_DEPTH_COMPONENT,
			   sampler,
			   { GL_R16F, src_data_rgba16f + 0 },
			   { GL_R16F, src_data_rgba16f + 1 },
			   { GL_R16F, src_data_rgba16f + 2 },
			   { GL_R16F, src_data_rgba16f + 3 },
			   src_data_rgba16f,
			   float16_zero,
			   float16_one },
	{ glu::ApiType::core(3, 0),
	  GL_R32F,
	  GL_RED,
	  GL_FLOAT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R32F, src_data_r32f + 0 },
	  zero_ch,
	  zero_ch,
	  one_ch,
	  src_data_r32f,
	  float32_zero,
	  float32_one },
	{ glu::ApiType::core(3, 0),
	  GL_RG32F,
	  GL_RG,
	  GL_FLOAT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R32F, src_data_rg32f + 0 },
	  { GL_R32F, src_data_rg32f + 1 },
	  zero_ch,
	  one_ch,
	  src_data_rg32f,
	  float32_zero,
	  float32_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGB32F,
	  GL_RGB,
	  GL_FLOAT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R32F, src_data_rgb32f + 0 },
	  { GL_R32F, src_data_rgb32f + 1 },
	  { GL_R32F, src_data_rgb32f + 2 },
	  one_ch,
	  src_data_rgb32f,
	  float32_zero,
	  float32_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGBA32F,
	  GL_RGBA,
	  GL_FLOAT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R32F, src_data_rgba32f + 0 },
	  { GL_R32F, src_data_rgba32f + 1 },
	  { GL_R32F, src_data_rgba32f + 2 },
	  { GL_R32F, src_data_rgba32f + 3 },
	  src_data_rgba32f,
	  float32_zero,
	  float32_one },
	{ glu::ApiType::core(3, 0),
	  GL_R11F_G11F_B10F,
	  GL_RGB,
	  GL_UNSIGNED_INT_10F_11F_11F_REV,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R16F, exp_data_r11f_g11f_b10f + 0 },
	  { GL_R16F, exp_data_r11f_g11f_b10f + 1 },
	  { GL_R16F, exp_data_r11f_g11f_b10f + 2 },
	  one_ch,
	  src_data_r11f_g11f_b10f,
	  float16_zero,
	  float16_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGB9_E5,
	  GL_RGB,
	  GL_UNSIGNED_INT_5_9_9_9_REV,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R32F, exp_data_rgb9_e5 + 0 },
	  { GL_R32F, exp_data_rgb9_e5 + 1 },
	  { GL_R32F, exp_data_rgb9_e5 + 2 },
	  one_ch,
	  src_data_rgb9_e5,
	  float32_zero,
	  float32_one },
	{ glu::ApiType::core(3, 0),
	  GL_R8I,
	  GL_RED_INTEGER,
	  GL_BYTE,
	  GL_DEPTH_COMPONENT,
	  isampler,
	  { GL_R8I, src_data_r8i },
	  zero_ch,
	  zero_ch,
	  one_ch,
	  src_data_r8i,
	  sint8_zero,
	  sint8_one },
	/* 40 */ { glu::ApiType::core(3, 0),
			   GL_R8UI,
			   GL_RED_INTEGER,
			   GL_UNSIGNED_BYTE,
			   GL_DEPTH_COMPONENT,
			   usampler,
			   { GL_R8UI, src_data_r8ui },
			   zero_ch,
			   zero_ch,
			   one_ch,
			   src_data_r8ui,
			   uint8_zero,
			   uint8_one },
	{ glu::ApiType::core(3, 0),
	  GL_R16I,
	  GL_RED_INTEGER,
	  GL_SHORT,
	  GL_DEPTH_COMPONENT,
	  isampler,
	  { GL_R16I, src_data_r16i },
	  zero_ch,
	  zero_ch,
	  one_ch,
	  src_data_r16i,
	  sint16_zero,
	  sint16_one },
	{ glu::ApiType::core(3, 0),
	  GL_R16UI,
	  GL_RED_INTEGER,
	  GL_UNSIGNED_SHORT,
	  GL_DEPTH_COMPONENT,
	  usampler,
	  { GL_R16UI, src_data_r16ui },
	  zero_ch,
	  zero_ch,
	  one_ch,
	  src_data_r16ui,
	  uint16_zero,
	  uint16_one },
	{ glu::ApiType::core(3, 0),
	  GL_R32I,
	  GL_RED_INTEGER,
	  GL_INT,
	  GL_DEPTH_COMPONENT,
	  isampler,
	  { GL_R32I, src_data_r32i },
	  zero_ch,
	  zero_ch,
	  one_ch,
	  src_data_r32i,
	  sint32_zero,
	  sint32_one },
	{ glu::ApiType::core(3, 0),
	  GL_R32UI,
	  GL_RED_INTEGER,
	  GL_UNSIGNED_INT,
	  GL_DEPTH_COMPONENT,
	  usampler,
	  { GL_R32UI, src_data_r32ui },
	  zero_ch,
	  zero_ch,
	  one_ch,
	  src_data_r32ui,
	  uint32_zero,
	  uint32_one },
	{ glu::ApiType::core(3, 0),
	  GL_RG8I,
	  GL_RG_INTEGER,
	  GL_BYTE,
	  GL_DEPTH_COMPONENT,
	  isampler,
	  { GL_R8I, src_data_rg8i + 0 },
	  { GL_R8I, src_data_rg8i + 1 },
	  zero_ch,
	  one_ch,
	  src_data_rg8i,
	  sint8_zero,
	  sint8_one },
	{ glu::ApiType::core(3, 0),
	  GL_RG8UI,
	  GL_RG_INTEGER,
	  GL_UNSIGNED_BYTE,
	  GL_DEPTH_COMPONENT,
	  usampler,
	  { GL_R8UI, src_data_rg8ui + 0 },
	  { GL_R8UI, src_data_rg8ui + 1 },
	  zero_ch,
	  one_ch,
	  src_data_rg8ui,
	  uint8_zero,
	  uint8_one },
	{ glu::ApiType::core(3, 0),
	  GL_RG16I,
	  GL_RG_INTEGER,
	  GL_SHORT,
	  GL_DEPTH_COMPONENT,
	  isampler,
	  { GL_R16I, src_data_rg16i + 0 },
	  { GL_R16I, src_data_rg16i + 1 },
	  zero_ch,
	  one_ch,
	  src_data_rg16i,
	  sint16_zero,
	  sint16_one },
	/* 48 */ { glu::ApiType::core(3, 0),
			   GL_RG16UI,
			   GL_RG_INTEGER,
			   GL_UNSIGNED_SHORT,
			   GL_DEPTH_COMPONENT,
			   usampler,
			   { GL_R16UI, src_data_rg16ui + 0 },
			   { GL_R16UI, src_data_rg16ui + 1 },
			   zero_ch,
			   one_ch,
			   src_data_rg16ui,
			   uint16_zero,
			   uint16_one },
	{ glu::ApiType::core(3, 0),
	  GL_RG32I,
	  GL_RG_INTEGER,
	  GL_INT,
	  GL_DEPTH_COMPONENT,
	  isampler,
	  { GL_R32I, src_data_rg32i + 0 },
	  { GL_R32I, src_data_rg32i + 1 },
	  zero_ch,
	  one_ch,
	  src_data_rg32i,
	  sint32_zero,
	  sint32_one },
	{ glu::ApiType::core(3, 0),
	  GL_RG32UI,
	  GL_RG_INTEGER,
	  GL_UNSIGNED_INT,
	  GL_DEPTH_COMPONENT,
	  usampler,
	  { GL_R32UI, src_data_rg32ui + 0 },
	  { GL_R32UI, src_data_rg32ui + 1 },
	  zero_ch,
	  one_ch,
	  src_data_rg32ui,
	  uint32_zero,
	  uint32_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGB8I,
	  GL_RGB_INTEGER,
	  GL_BYTE,
	  GL_DEPTH_COMPONENT,
	  isampler,
	  { GL_R8I, src_data_rgb8i + 0 },
	  { GL_R8I, src_data_rgb8i + 1 },
	  { GL_R8I, src_data_rgb8i + 2 },
	  one_ch,
	  src_data_rgb8i,
	  sint8_zero,
	  sint8_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGB8UI,
	  GL_RGB_INTEGER,
	  GL_UNSIGNED_BYTE,
	  GL_DEPTH_COMPONENT,
	  usampler,
	  { GL_R8UI, src_data_rgb8ui + 0 },
	  { GL_R8UI, src_data_rgb8ui + 1 },
	  { GL_R8UI, src_data_rgb8ui + 2 },
	  one_ch,
	  src_data_rgb8ui,
	  uint8_zero,
	  uint8_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGB16I,
	  GL_RGB_INTEGER,
	  GL_SHORT,
	  GL_DEPTH_COMPONENT,
	  isampler,
	  { GL_R16I, src_data_rgb16i + 0 },
	  { GL_R16I, src_data_rgb16i + 1 },
	  { GL_R16I, src_data_rgb16i + 2 },
	  one_ch,
	  src_data_rgb16i,
	  sint16_zero,
	  sint16_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGB16UI,
	  GL_RGB_INTEGER,
	  GL_UNSIGNED_SHORT,
	  GL_DEPTH_COMPONENT,
	  usampler,
	  { GL_R16UI, src_data_rgb16ui + 0 },
	  { GL_R16UI, src_data_rgb16ui + 1 },
	  { GL_R16UI, src_data_rgb16ui + 2 },
	  one_ch,
	  src_data_rgb16ui,
	  uint16_zero,
	  uint16_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGB32I,
	  GL_RGB_INTEGER,
	  GL_INT,
	  GL_DEPTH_COMPONENT,
	  isampler,
	  { GL_R32I, src_data_rgb32i + 0 },
	  { GL_R32I, src_data_rgb32i + 1 },
	  { GL_R32I, src_data_rgb32i + 2 },
	  one_ch,
	  src_data_rgb32i,
	  sint32_zero,
	  sint32_one },
	/* 56 */ { glu::ApiType::core(3, 0),
			   GL_RGB32UI,
			   GL_RGB_INTEGER,
			   GL_UNSIGNED_INT,
			   GL_DEPTH_COMPONENT,
			   usampler,
			   { GL_R32UI, src_data_rgb32ui + 0 },
			   { GL_R32UI, src_data_rgb32ui + 1 },
			   { GL_R32UI, src_data_rgb32ui + 2 },
			   one_ch,
			   src_data_rgb32ui,
			   uint32_zero,
			   uint32_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGBA8I,
	  GL_RGBA_INTEGER,
	  GL_BYTE,
	  GL_DEPTH_COMPONENT,
	  isampler,
	  { GL_R8I, src_data_rgba8i + 0 },
	  { GL_R8I, src_data_rgba8i + 1 },
	  { GL_R8I, src_data_rgba8i + 2 },
	  { GL_R8I, src_data_rgba8i + 3 },
	  src_data_rgba8i,
	  sint8_zero,
	  sint8_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGBA8UI,
	  GL_RGBA_INTEGER,
	  GL_UNSIGNED_BYTE,
	  GL_DEPTH_COMPONENT,
	  usampler,
	  { GL_R8UI, src_data_rgba8ui + 0 },
	  { GL_R8UI, src_data_rgba8ui + 1 },
	  { GL_R8UI, src_data_rgba8ui + 2 },
	  { GL_R8UI, src_data_rgba8ui + 3 },
	  src_data_rgba8ui,
	  uint8_zero,
	  uint8_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGBA16I,
	  GL_RGBA_INTEGER,
	  GL_SHORT,
	  GL_DEPTH_COMPONENT,
	  isampler,
	  { GL_R16I, src_data_rgba16i + 0 },
	  { GL_R16I, src_data_rgba16i + 1 },
	  { GL_R16I, src_data_rgba16i + 2 },
	  { GL_R16I, src_data_rgba16i + 3 },
	  src_data_rgba16i,
	  sint16_zero,
	  sint16_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGBA16UI,
	  GL_RGBA_INTEGER,
	  GL_UNSIGNED_SHORT,
	  GL_DEPTH_COMPONENT,
	  usampler,
	  { GL_R16UI, src_data_rgba16ui + 0 },
	  { GL_R16UI, src_data_rgba16ui + 1 },
	  { GL_R16UI, src_data_rgba16ui + 2 },
	  { GL_R16UI, src_data_rgba16ui + 3 },
	  src_data_rgba16ui,
	  uint16_zero,
	  uint16_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGBA32I,
	  GL_RGBA_INTEGER,
	  GL_INT,
	  GL_DEPTH_COMPONENT,
	  isampler,
	  { GL_R32I, src_data_rgba32i + 0 },
	  { GL_R32I, src_data_rgba32i + 1 },
	  { GL_R32I, src_data_rgba32i + 2 },
	  { GL_R32I, src_data_rgba32i + 3 },
	  src_data_rgba32i,
	  sint32_zero,
	  sint32_one },
	{ glu::ApiType::core(3, 0),
	  GL_RGBA32UI,
	  GL_RGBA_INTEGER,
	  GL_UNSIGNED_INT,
	  GL_DEPTH_COMPONENT,
	  usampler,
	  { GL_R32UI, src_data_rgba32ui + 0 },
	  { GL_R32UI, src_data_rgba32ui + 1 },
	  { GL_R32UI, src_data_rgba32ui + 2 },
	  { GL_R32UI, src_data_rgba32ui + 3 },
	  src_data_rgba32ui,
	  uint32_zero,
	  uint32_one },
	{ glu::ApiType::core(3, 0),
	  GL_DEPTH_COMPONENT16,
	  GL_DEPTH_COMPONENT,
	  GL_UNSIGNED_SHORT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R16, src_data_depth_component16 },
	  zero_ch,
	  zero_ch,
	  one_ch,
	  src_data_depth_component16,
	  unorm16_zero,
	  unorm16_one },
	/* 64 */ { glu::ApiType::core(3, 0),
			   GL_DEPTH_COMPONENT24,
			   GL_DEPTH_COMPONENT,
			   GL_UNSIGNED_INT,
			   GL_DEPTH_COMPONENT,
			   sampler,
			   { GL_R32F, exp_data_depth_component32 },
			   zero_ch,
			   zero_ch,
			   one_ch,
			   src_data_depth_component32,
			   float32_zero,
			   float32_one },
	{ glu::ApiType::core(3, 0),
	  GL_DEPTH_COMPONENT32,
	  GL_DEPTH_COMPONENT,
	  GL_UNSIGNED_INT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R32F, exp_data_depth_component32 },
	  zero_ch,
	  zero_ch,
	  one_ch,
	  src_data_depth_component32,
	  float32_zero,
	  float32_one },
	{ glu::ApiType::core(3, 0),
	  GL_DEPTH_COMPONENT32F,
	  GL_DEPTH_COMPONENT,
	  GL_FLOAT,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R32F, src_data_depth_component32f },
	  zero_ch,
	  zero_ch,
	  one_ch,
	  src_data_depth_component32f,
	  float32_zero,
	  float32_one },
	{ glu::ApiType::core(3, 0),
	  GL_DEPTH24_STENCIL8,
	  GL_DEPTH_STENCIL,
	  GL_UNSIGNED_INT_24_8,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R32F, exp_data_depth_component32 },
	  zero_ch,
	  zero_ch,
	  one_ch,
	  src_data_depth24_stencil8,
	  float32_zero,
	  float32_one },
	{ glu::ApiType::core(3, 0),
	  GL_DEPTH32F_STENCIL8,
	  GL_DEPTH_STENCIL,
	  GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
	  GL_DEPTH_COMPONENT,
	  sampler,
	  { GL_R32F, exp_data_depth_component32 },
	  zero_ch,
	  zero_ch,
	  one_ch,
	  src_data_depth32f_stencil8,
	  float32_zero,
	  float32_one },
	{ glu::ApiType::core(4, 3), GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, GL_STENCIL_INDEX, usampler,
	  one_ch, zero_ch, zero_ch, one_ch, src_data_depth24_stencil8, uint8_zero, uint8_one },
	{ glu::ApiType::core(4, 3), GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
	  GL_STENCIL_INDEX, usampler, one_ch, zero_ch, zero_ch, one_ch, src_data_depth32f_stencil8, uint8_zero, uint8_one }
};
static const size_t n_texture_formats = sizeof(texture_formats) / sizeof(texture_formats[0]);

/* Texture access routine descriptors */
struct _texture_access
{
	const glw::GLchar* m_name;
	size_t			   m_n_coordinates;
	bool			   m_use_derivaties;
	bool			   m_use_integral_coordinates;
	bool			   m_use_lod;
	bool			   m_use_offsets;
	bool			   m_support_multisampling;
};
static const _texture_access texture_access[] = { { "texture", 0, false, false, false, false, false },
												  { "textureProj", 1, false, false, false, false, false },
												  { "textureLod", 0, false, false, true, false, false },
												  { "textureOffset", 0, false, false, false, true, false },
												  { "texelFetch", 0, false, true, true, false, true },
												  { "texelFetchOffset", 0, false, true, true, true, false },
												  { "textureProjOffset", 1, false, false, false, true, false },
												  { "textureLodOffset", 0, false, false, true, true, false },
												  { "textureProjLod", 1, false, false, true, false, false },
												  { "textureProjLodOffset", 1, false, false, true, true, false },
												  { "textureGrad", 0, true, false, false, false, false },
												  { "textureGradOffset", 0, true, false, false, true, false },
												  { "textureProjGrad", 1, true, false, false, false, false },
												  { "textureProjGradOffset", 1, true, false, false, true, false } };
static const size_t n_texture_access = sizeof(texture_access) / sizeof(texture_access[0]);

/* Texture target descriptor */
struct _texture_target
{
	size_t			   m_n_array_coordinates;
	size_t			   m_n_coordinates;
	size_t			   m_n_derivatives;
	const glw::GLchar* m_sampler_type;
	bool			   m_support_integral_coordinates;
	bool			   m_support_lod;
	bool			   m_support_offset;
	bool			   m_supports_proj;
	bool			   m_require_multisampling;
	glw::GLenum		   m_target;
};

static const _texture_target texture_targets[] = {
	{ 0, 1, 1, "1D", true, true, true, true, false, GL_TEXTURE_1D },
	{ 0, 2, 2, "2D", true, true, true, true, false, GL_TEXTURE_2D },
	{ 0, 3, 3, "3D", true, true, true, true, false, GL_TEXTURE_3D },
	{ 1, 1, 1, "1DArray", true, true, true, false, false, GL_TEXTURE_1D_ARRAY },
	{ 1, 2, 2, "2DArray", true, true, true, false, false, GL_TEXTURE_2D_ARRAY },
	{ 0, 2, 2, "2DRect", true, false, true, true, false, GL_TEXTURE_RECTANGLE },
	{ 0, 3, 3, "Cube", false, true, false, false, false, GL_TEXTURE_CUBE_MAP },
	{ 0, 2, 2, "2DMS", true, false, true, true, true, GL_TEXTURE_2D_MULTISAMPLE },
	{ 1, 2, 2, "2DMSArray", true, false, true, true, true, GL_TEXTURE_2D_MULTISAMPLE_ARRAY },
};
static const size_t n_texture_targets = sizeof(texture_targets) / sizeof(texture_targets[0]);

/* Swizzle valid values */
static const glw::GLint valid_values[] = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA, GL_ONE, GL_ZERO };
static const size_t		n_valid_values = sizeof(valid_values) / sizeof(valid_values[0]);

/* Prototypes */
const _out_ch_desc& get_descriptor_for_channel(const _texture_format& format, const size_t channel);

#if ENABLE_DEBUG

/** Debuging procedure. Logs parameters.
 *
 * @param source   As specified in GL spec.
 * @param type     As specified in GL spec.
 * @param id       As specified in GL spec.
 * @param severity As specified in GL spec.
 * @param ignored
 * @param message  As specified in GL spec.
 * @param info     Pointer to instance of deqp::Context used by test.
 */
void GLW_APIENTRY debug_proc(glw::GLenum source, glw::GLenum type, glw::GLuint id, glw::GLenum severity,
							 glw::GLsizei /* length */, const glw::GLchar* message, void* info)
{
	Context* ctx = (Context*)info;

	const glw::GLchar* source_str   = "Unknown";
	const glw::GLchar* type_str		= "Unknown";
	const glw::GLchar* severity_str = "Unknown";

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:
		source_str = "API";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		source_str = "APP";
		break;
	case GL_DEBUG_SOURCE_OTHER:
		source_str = "OTR";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		source_str = "COM";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		source_str = "3RD";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		source_str = "WS";
		break;
	default:
		break;
	}

	switch (type)
	{
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		type_str = "DEPRECATED_BEHAVIOR";
		break;
	case GL_DEBUG_TYPE_ERROR:
		type_str = "ERROR";
		break;
	case GL_DEBUG_TYPE_MARKER:
		type_str = "MARKER";
		break;
	case GL_DEBUG_TYPE_OTHER:
		type_str = "OTHER";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		type_str = "PERFORMANCE";
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		type_str = "POP_GROUP";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		type_str = "PORTABILITY";
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		type_str = "PUSH_GROUP";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		type_str = "UNDEFINED_BEHAVIOR";
		break;
	default:
		break;
	}

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		severity_str = "H";
		break;
	case GL_DEBUG_SEVERITY_LOW:
		severity_str = "L";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		severity_str = "M";
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		severity_str = "N";
		break;
	default:
		break;
	}

	ctx->getTestContext().getLog() << tcu::TestLog::Message << "DEBUG_INFO: " << std::setw(3) << source_str << "|"
								   << severity_str << "|" << std::setw(18) << type_str << "|" << std::setw(12) << id
								   << ": " << message << tcu::TestLog::EndMessage;
}

#endif /* ENABLE_DEBUG */

/** Extracts value of each channel from source data of given format
 *
 * @param format_idx  Index of format
 * @param out_ch_rgba Storage for values
 **/
void calculate_values_from_source(size_t format_idx, double out_ch_rgba[4])
{
	const _texture_format& format = texture_formats[format_idx];

	/*  */
	double  ch_rgba[4] = { 0.0, 0.0, 0.0, 0.0 };
	double& ch_r	   = ch_rgba[0];
	double& ch_g	   = ch_rgba[1];
	double& ch_b	   = ch_rgba[2];
	double& ch_a	   = ch_rgba[3];
	size_t  n_channels = 0;
	bool	is_norm	= true;

	/* Select n_channels and is_norm */
	switch (format.m_format)
	{
	case GL_RED_INTEGER:
		is_norm = false;
	/* fall through */

	case GL_RED:
		n_channels = 1;

		break;

	case GL_RG_INTEGER:
		is_norm = false;
	/* fall through */

	case GL_RG:
		n_channels = 2;

		break;

	case GL_RGB_INTEGER:
		is_norm = false;
	/* fall through */

	case GL_RGB:
		n_channels = 3;

		break;

	case GL_RGBA_INTEGER:
		is_norm = false;
	/* fall through */

	case GL_RGBA:
		n_channels = 4;

		break;

	default:
		TCU_FAIL("Unsupported format");
	}

	/* Calculate rgba values */
	if ((GL_SRGB8 == format.m_internal_format) || (GL_SRGB8_ALPHA8 == format.m_internal_format))
	{
		const glw::GLubyte* ptr = (const glw::GLubyte*)src_data_srgb8_alpha8;
		const glw::GLubyte  r   = ptr[0];
		const glw::GLubyte  g   = ptr[1];
		const glw::GLubyte  b   = ptr[2];
		const glw::GLubyte  a   = ptr[3];

		ch_r = r;
		ch_g = g;
		ch_b = b;
		ch_a = a;

		ch_r /= 255.0;
		ch_g /= 255.0;
		ch_b /= 255.0;
		ch_a /= 255.0;
	}
	else if (GL_UNSIGNED_BYTE_3_3_2 == format.m_type)
	{
		const glw::GLubyte* ptr = (const glw::GLubyte*)format.m_source_data;
		const glw::GLubyte  r   = (glw::GLubyte)((*ptr) >> 5);
		const glw::GLubyte  g   = ((*ptr) >> 2) & 7;
		const glw::GLubyte  b   = (*ptr) & 3;

		ch_r = r;
		ch_g = g;
		ch_b = b;

		ch_r /= 7.0;
		ch_g /= 7.0;
		ch_b /= 3.0;
	}
	else if (GL_UNSIGNED_SHORT_5_6_5 == format.m_type)
	{
		const glw::GLushort* ptr = (const glw::GLushort*)format.m_source_data;
		const glw::GLubyte   r   = (glw::GLubyte)((*ptr) >> 11);
		const glw::GLubyte   g   = (glw::GLubyte)((*ptr) >> 5) & 63;
		const glw::GLubyte   b   = (*ptr) & 31;

		ch_r = r;
		ch_g = g;
		ch_b = b;

		ch_r /= 31.0;
		ch_g /= 63.0;
		ch_b /= 31.0;
	}
	else if (GL_UNSIGNED_SHORT_4_4_4_4 == format.m_type)
	{
		const glw::GLushort* ptr = (const glw::GLushort*)format.m_source_data;
		const glw::GLubyte   r   = (glw::GLubyte)((*ptr) >> 12);
		const glw::GLubyte   g   = (glw::GLubyte)(((*ptr) >> 8) & 15);
		const glw::GLubyte   b   = (glw::GLubyte)(((*ptr) >> 4) & 15);
		const glw::GLubyte   a   = (glw::GLubyte)((*ptr) & 15);

		ch_r = r;
		ch_g = g;
		ch_b = b;
		ch_a = a;

		ch_r /= 15.0;
		ch_g /= 15.0;
		ch_b /= 15.0;
		ch_a /= 15.0;
	}
	else if (GL_UNSIGNED_SHORT_5_5_5_1 == format.m_type)
	{
		const glw::GLushort* ptr = (const glw::GLushort*)format.m_source_data;
		const glw::GLubyte   r   = (glw::GLubyte)((*ptr) >> 11);
		const glw::GLubyte   g   = ((*ptr) >> 6) & 31;
		const glw::GLubyte   b   = ((*ptr) >> 1) & 31;
		const glw::GLubyte   a   = (*ptr) & 1;

		ch_r = r;
		ch_g = g;
		ch_b = b;
		ch_a = a;

		ch_r /= 31.0;
		ch_g /= 31.0;
		ch_b /= 31.0;
		ch_a /= 1.0;
	}
	else if (GL_UNSIGNED_INT_10_10_10_2 == format.m_type)
	{
		const glw::GLuint*  ptr = (const glw::GLuint*)format.m_source_data;
		const glw::GLushort r   = (glw::GLushort)((*ptr) >> 22);
		const glw::GLushort g   = ((*ptr) >> 12) & 1023;
		const glw::GLushort b   = ((*ptr) >> 2) & 1023;
		const glw::GLushort a   = (*ptr) & 3;

		ch_r = r;
		ch_g = g;
		ch_b = b;
		ch_a = a;

		if (true == is_norm)
		{
			ch_r /= 1023.0;
			ch_g /= 1023.0;
			ch_b /= 1023.0;
			ch_a /= 3.0;
		}
	}
	else if (GL_UNSIGNED_INT_10F_11F_11F_REV == format.m_type)
	{
		ch_r = r11f.asDouble();
		ch_g = g11f.asDouble();
		ch_b = b10f.asDouble();
	}
	else if (GL_UNSIGNED_INT_5_9_9_9_REV == format.m_type)
	{
		TCU_FAIL("Not supported: GL_UNSIGNED_INT_5_9_9_9_REV");
	}
	else if (GL_UNSIGNED_INT_24_8 == format.m_type)
	{
		TCU_FAIL("Not supported: GL_UNSIGNED_INT_24_8");
	}
	else if (GL_FLOAT_32_UNSIGNED_INT_24_8_REV == format.m_type)
	{
		TCU_FAIL("Not supported: GL_FLOAT_32_UNSIGNED_INT_24_8_REV");
	}
	else if (GL_BYTE == format.m_type)
	{
		const glw::GLbyte* ptr = (const glw::GLbyte*)format.m_source_data;

		for (size_t i = 0; i < n_channels; ++i)
		{
			const glw::GLbyte val = ptr[i];
			double&			  ch  = ch_rgba[i];

			ch = val;
			if (true == is_norm)
				ch /= 127.0;
		}
	}
	else if (GL_UNSIGNED_BYTE == format.m_type)
	{
		const glw::GLubyte* ptr = (const glw::GLubyte*)format.m_source_data;

		for (size_t i = 0; i < n_channels; ++i)
		{
			const glw::GLubyte val = ptr[i];
			double&			   ch  = ch_rgba[i];

			ch = val;
			if (true == is_norm)
				ch /= 255.0;
		}
	}
	else if (GL_SHORT == format.m_type)
	{
		const glw::GLshort* ptr = (const glw::GLshort*)format.m_source_data;

		for (size_t i = 0; i < n_channels; ++i)
		{
			const glw::GLshort val = ptr[i];
			double&			   ch  = ch_rgba[i];

			ch = val;
			if (true == is_norm)
				ch /= 32767.0;
		}
	}
	else if (GL_UNSIGNED_SHORT == format.m_type)
	{
		const glw::GLushort* ptr = (const glw::GLushort*)format.m_source_data;

		for (size_t i = 0; i < n_channels; ++i)
		{
			const glw::GLushort val = ptr[i];
			double&				ch  = ch_rgba[i];

			ch = val;
			if (true == is_norm)
				ch /= 65535.0;
		}
	}
	else if (GL_INT == format.m_type)
	{
		const glw::GLint* ptr = (const glw::GLint*)format.m_source_data;

		for (size_t i = 0; i < n_channels; ++i)
		{
			const glw::GLint val = ptr[i];
			double&			 ch  = ch_rgba[i];

			ch = val;
			if (true == is_norm)
				ch /= 2147483647.0;
		}
	}
	else if (GL_UNSIGNED_INT == format.m_type)
	{
		const glw::GLuint* ptr = (const glw::GLuint*)format.m_source_data;

		for (size_t i = 0; i < n_channels; ++i)
		{
			const glw::GLuint val = ptr[i];
			double&			  ch  = ch_rgba[i];

			ch = val;
			if (true == is_norm)
				ch /= 4294967295.0;
		}
	}
	else if (GL_FLOAT == format.m_type)
	{
		const glw::GLfloat* ptr = (const glw::GLfloat*)format.m_source_data;

		for (size_t i = 0; i < n_channels; ++i)
		{
			const glw::GLfloat val = ptr[i];
			double&			   ch  = ch_rgba[i];

			ch = val;
		}
	}
	else if (GL_HALF_FLOAT == format.m_type)
	{
		const glw::GLhalf* ptr = (const glw::GLhalf*)format.m_source_data;

		for (size_t i = 0; i < n_channels; ++i)
		{
			const glw::GLhalf val = ptr[i];
			double&			  ch  = ch_rgba[i];

			tcu::Float16 f16(val);
			ch = f16.asDouble();
		}
	}
	else
	{
		TCU_FAIL("Invalid enum");
	}

	/* Store results */
	memcpy(out_ch_rgba, ch_rgba, 4 * sizeof(double));
}

/** Calculate maximum uint value for given size of storage
 *
 * @param size Size of storage in bits
 *
 * @return Calculated max
 **/
double calculate_max_for_size(size_t size)
{
	double power = pow(2.0, double(size));

	return power - 1.0;
}

/** Converts from double to given T
 *
 * @tparam Requested type of value
 *
 * @param out_expected_data Storage for converted value
 * @param value             Value to be converted
 **/
template <typename T>
void convert(void* out_expected_data, double value)
{
	T* ptr = (T*)out_expected_data;

	*ptr = T(value);
}

/** Calcualte range of expected values
 *
 * @param source_format_idx         Index of source format
 * @param output_format_idx         Index of output format
 * @param index_of_swizzled_channel Index of swizzled channel
 * @param source_size               Size of source storage in bits
 * @param output_size               Size of output storage in bits
 * @param out_expected_data_low     Lowest acceptable value
 * @param out_expected_data_top     Highest acceptable value
 * @param out_expected_data_size    Number of bytes used to store out values
 **/
void calculate_expected_value(size_t source_format_idx, size_t output_format_idx, size_t index_of_swizzled_channel,
							  glw::GLint source_size, glw::GLint output_size, void* out_expected_data_low,
							  void* out_expected_data_top, size_t& out_expected_data_size)
{
	const _texture_format& output_format = texture_formats[output_format_idx];
	const _texture_format& source_format = texture_formats[source_format_idx];
	const _out_ch_desc&	desc			 = get_descriptor_for_channel(source_format, index_of_swizzled_channel);
	const glw::GLvoid*	 expected_data = desc.m_expected_data;
	bool				   is_signed	 = false;
	double				   range_low	 = 0.0f;
	double				   range_top	 = 0.0f;
	size_t				   texel_size	= 0;

	/* Select range, texel size and is_signed */
	switch (output_format.m_type)
	{
	case GL_BYTE:
		is_signed = true;

		range_low = -127.0;
		range_top = 127.0;

		texel_size = 1;

		break;

	case GL_UNSIGNED_BYTE:
		range_low = 0.0;
		range_top = 255.0;

		texel_size = 1;

		break;

	case GL_SHORT:
		is_signed = true;

		range_low = -32767.0;
		range_top = 32767.0;

		texel_size = 2;

		break;

	case GL_UNSIGNED_SHORT:
		range_low = 0.0;
		range_top = 65535.0;

		texel_size = 2;

		break;

	case GL_HALF_FLOAT:
		texel_size = 2;

		/* Halfs are not calculated, range will not be used */

		break;

	case GL_INT:
		is_signed = true;

		range_low = -2147483647.0;
		range_top = 2147483647.0;

		texel_size = 4;

		break;

	case GL_UNSIGNED_INT:
		range_low = 0.0;
		range_top = 4294967295.0;

		texel_size = 4;

		break;

	case GL_FLOAT:
		texel_size = 4;

		/* Float are not calculated, range will not be used */

		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	/* Signed formats use one bit less */
	if (true == is_signed)
	{
		source_size -= 1;
		output_size -= 1;
	}

	/* If expected data is hardcoded just copy data to low and top */
	if (DE_NULL != expected_data)
	{
		memcpy(out_expected_data_top, expected_data, texel_size);
		memcpy(out_expected_data_low, expected_data, texel_size);
		out_expected_data_size = texel_size;
	}
	else
	{
		/* Get source values */
		double ch_rgba[4];
		calculate_values_from_source(source_format_idx, ch_rgba);

		/* Calculate expected value */
		const float max_internal  = float(calculate_max_for_size(source_size));
		const float max_output	= float(calculate_max_for_size(output_size));
		const float temp_internal = float(ch_rgba[index_of_swizzled_channel]) * max_internal;
		const float stor_internal_low =
			deFloatFloor(temp_internal - 1.0f); /* Offset by 1 to avoid rounding done by FPU */
		const float stor_internal_top =
			deFloatCeil(temp_internal + 1.0f); /* Offset by 1 to avoid rounding done by FPU */
		const float read_internal_low = stor_internal_low / max_internal;
		const float read_internal_top = stor_internal_top / max_internal;
		const float temp_output_low   = read_internal_low * max_output;
		const float temp_output_top   = read_internal_top * max_output;
		double		stor_output_low   = floor(temp_output_low);
		double		stor_output_top   = ceil(temp_output_top);

		/* Clamp to limits of output format */
		stor_output_low = de::clamp(stor_output_low, range_low, range_top);
		stor_output_top = de::clamp(stor_output_top, range_low, range_top);

		/* Store resuts */
		switch (output_format.m_type)
		{
		case GL_BYTE:
			convert<glw::GLbyte>(out_expected_data_low, stor_output_low);
			convert<glw::GLbyte>(out_expected_data_top, stor_output_top);
			break;
		case GL_UNSIGNED_BYTE:
			convert<glw::GLubyte>(out_expected_data_low, stor_output_low);
			convert<glw::GLubyte>(out_expected_data_top, stor_output_top);
			break;
		case GL_SHORT:
			convert<glw::GLshort>(out_expected_data_low, stor_output_low);
			convert<glw::GLshort>(out_expected_data_top, stor_output_top);
			break;
		case GL_UNSIGNED_SHORT:
			convert<glw::GLushort>(out_expected_data_low, stor_output_low);
			convert<glw::GLushort>(out_expected_data_top, stor_output_top);
			break;
		case GL_INT:
			convert<glw::GLint>(out_expected_data_low, stor_output_low);
			convert<glw::GLint>(out_expected_data_top, stor_output_top);
			break;
		case GL_UNSIGNED_INT:
			convert<glw::GLuint>(out_expected_data_low, stor_output_low);
			convert<glw::GLuint>(out_expected_data_top, stor_output_top);
			break;
		default:
			TCU_FAIL("Invalid enum");
			break;
		}
		out_expected_data_size = texel_size;
	}
}

/** Gets index of internal format in texture_fomrats
 *
 * @param internal_format Internal format to be found
 *
 * @return Found index. -1 when format is not available. 0 when GL_ZERO is requested.
 **/
size_t get_index_of_format(glw::GLenum internal_format)
{
	if (GL_ZERO == internal_format)
	{
		return 0;
	}

	for (size_t i = 0; i < n_texture_formats; ++i)
	{
		if (texture_formats[i].m_internal_format == internal_format)
		{
			return i;
		}
	}

	TCU_FAIL("Unknown internal format");
	return -1;
}

/** Gets index of target in texture_targets
 *
 * @param target target to be found
 *
 * @return Found index. -1 when format is not available. 0 when GL_ZERO is requested.
 **/
size_t get_index_of_target(glw::GLenum target)
{
	if (GL_ZERO == target)
	{
		return 0;
	}

	for (size_t i = 0; i < n_texture_targets; ++i)
	{
		if (texture_targets[i].m_target == target)
		{
			return i;
		}
	}

	TCU_FAIL("Unknown texture target");
	return -1;
}

/* Constants used by get_swizzled_channel_idx */
static const size_t CHANNEL_INDEX_ONE  = 4;
static const size_t CHANNEL_INDEX_ZERO = 5;

/** Get index of channel that will be accessed after "swizzle" is applied
 *
 * @param channel_idx Index of channel before "swizzle" is applied
 * @param swizzle_set Set of swizzle states
 *
 * @return Index of "swizzled" channel
 */
size_t get_swizzled_channel_idx(const size_t channel_idx, const glw::GLint swizzle_set[4])
{
	const glw::GLint swizzle = swizzle_set[channel_idx];

	size_t channel = 0;

	switch (swizzle)
	{
	case GL_RED:
		channel = 0;
		break;
	case GL_GREEN:
		channel = 1;
		break;
	case GL_BLUE:
		channel = 2;
		break;
	case GL_ALPHA:
		channel = 3;
		break;
	case GL_ONE:
		channel = CHANNEL_INDEX_ONE;
		break;
	case GL_ZERO:
		channel = CHANNEL_INDEX_ZERO;
		break;
	default:
		TCU_FAIL("Invalid value");
		break;
	}

	return channel;
}

/** Gets descriptor of output channel from texture format descriptor
 *
 * @param format  Format descriptor
 * @param channel Index of "swizzled" channel
 *
 * @return Descriptor of output channel
 **/
const _out_ch_desc& get_descriptor_for_channel(const _texture_format& format, const size_t channel)
{
	const _out_ch_desc* desc = 0;

	switch (channel)
	{
	case CHANNEL_INDEX_ONE:
		desc = &format.m_one_ch;
		break;
	case CHANNEL_INDEX_ZERO:
		desc = &format.m_zero_ch;
		break;
	case 0:
		desc = &format.m_red_ch;
		break;
	case 1:
		desc = &format.m_green_ch;
		break;
	case 2:
		desc = &format.m_blue_ch;
		break;
	case 3:
		desc = &format.m_alpha_ch;
		break;
	default:
		TCU_FAIL("Invalid value");
		break;
	};

	switch (desc->m_internal_format)
	{
	case GL_ONE:
		desc = &format.m_one_ch;
		break;
	case GL_ZERO:
		desc = &format.m_zero_ch;
		break;
	default:
		break;
	}

	return *desc;
}

/** Gets internal_format of output channel for given texture format
 *
 * @param format  Format descriptor
 * @param channel Index of "swizzled" channel
 *
 * @return Internal format
 **/
glw::GLenum get_internal_format_for_channel(const _texture_format& format, const size_t channel)
{
	return get_descriptor_for_channel(format, channel).m_internal_format;
}

/** Constructor
 *
 * @param context Test context
 **/
Utils::programInfo::programInfo(deqp::Context& context)
	: m_context(context), m_fragment_shader_id(0), m_program_object_id(0), m_vertex_shader_id(0)
{
	/* Nothing to be done here */
}

/** Destructor
 *
 **/
Utils::programInfo::~programInfo()
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Make sure program object is no longer used by GL */
	gl.useProgram(0);

	/* Clean program object */
	if (0 != m_program_object_id)
	{
		gl.deleteProgram(m_program_object_id);
		m_program_object_id = 0;
	}

	/* Clean shaders */
	if (0 != m_fragment_shader_id)
	{
		gl.deleteShader(m_fragment_shader_id);
		m_fragment_shader_id = 0;
	}

	if (0 != m_vertex_shader_id)
	{
		gl.deleteShader(m_vertex_shader_id);
		m_vertex_shader_id = 0;
	}
}

/** Build program
 *
 * @param fragment_shader_code Fragment shader source code
 * @param vertex_shader_code   Vertex shader source code
 **/
void Utils::programInfo::build(const glw::GLchar* fragment_shader_code, const glw::GLchar* vertex_shader_code)
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create shader objects and compile */
	if (0 != fragment_shader_code)
	{
		m_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_fragment_shader_id, fragment_shader_code);
	}

	if (0 != vertex_shader_code)
	{
		m_vertex_shader_id = gl.createShader(GL_VERTEX_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_vertex_shader_id, vertex_shader_code);
	}

	/* Create program object */
	m_program_object_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "CreateProgram");

	/* Link program */
	link();
}

/** Compile shader
 *
 * @param shader_id   Shader object id
 * @param shader_code Shader source code
 **/
void Utils::programInfo::compile(glw::GLuint shader_id, const glw::GLchar* shader_code) const
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Compilation status */
	glw::GLint status = GL_FALSE;

	/* Set source code */
	gl.shaderSource(shader_id, 1 /* count */, &shader_code, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ShaderSource");

	/* Compile */
	gl.compileShader(shader_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "CompileShader");

	/* Get compilation status */
	gl.getShaderiv(shader_id, GL_COMPILE_STATUS, &status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderiv");

	/* Log compilation error */
	if (GL_TRUE != status)
	{
		glw::GLint				 length = 0;
		std::vector<glw::GLchar> message;

		/* Error log length */
		gl.getShaderiv(shader_id, GL_INFO_LOG_LENGTH, &length);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderiv");

		/* Prepare storage */
		message.resize(length);

		/* Get error log */
		gl.getShaderInfoLog(shader_id, length, 0, &message[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderInfoLog");

		/* Log */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Failed to compile shader:\n"
											<< &message[0] << "\nShader source\n"
											<< shader_code << tcu::TestLog::EndMessage;

		TCU_FAIL("Failed to compile shader");
	}
}

/** Attach shaders and link program
 *
 **/
void Utils::programInfo::link() const
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Link status */
	glw::GLint status = GL_FALSE;

	/* Attach shaders */
	if (0 != m_fragment_shader_id)
	{
		gl.attachShader(m_program_object_id, m_fragment_shader_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
	}

	if (0 != m_vertex_shader_id)
	{
		gl.attachShader(m_program_object_id, m_vertex_shader_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
	}

	/* Link */
	gl.linkProgram(m_program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "LinkProgram");

	/* Get link status */
	gl.getProgramiv(m_program_object_id, GL_LINK_STATUS, &status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

	/* Log link error */
	if (GL_TRUE != status)
	{
		glw::GLint				 length = 0;
		std::vector<glw::GLchar> message;

		/* Get error log length */
		gl.getProgramiv(m_program_object_id, GL_INFO_LOG_LENGTH, &length);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

		message.resize(length);

		/* Get error log */
		gl.getProgramInfoLog(m_program_object_id, length, 0, &message[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramInfoLog");

		/* Log */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Failed to link program:\n"
											<< &message[0] << tcu::TestLog::EndMessage;

		TCU_FAIL("Failed to link program");
	}
}

/** Replace first occurance of <token> with <text> in <string> starting at <search_posistion>
 *
 * @param token           Token string
 * @param search_position Position at which find will start, it is updated to position at which replaced text ends
 * @param text            String that will be used as replacement for <token>
 * @param string          String to work on
 **/
void Utils::replaceToken(const glw::GLchar* token, size_t& search_position, const glw::GLchar* text,
						 std::string& string)
{
	const size_t text_length	= strlen(text);
	const size_t token_length   = strlen(token);
	const size_t token_position = string.find(token, search_position);

	string.replace(token_position, token_length, text, text_length);

	search_position = token_position + text_length;
}

/** Constructor.
 *
 * @param context Rendering context.
 **/
APIErrorsTest::APIErrorsTest(deqp::Context& context)
	: TestCase(context, "api_errors", "Verifies that errors are generated as specified"), m_id(0)
{
	/* Left blank intentionally */
}

/** Deinitialization **/
void APIErrorsTest::deinit()
{
	if (0 != m_id)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteTextures(1, &m_id);
		m_id = 0;
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP.
 */
tcu::TestNode::IterateResult APIErrorsTest::iterate()
{
	static const glw::GLint invalid_values[] = { 0x1902, 0x1907, -1, 2 };
	static const size_t		n_invalid_values = sizeof(invalid_values) / sizeof(invalid_values[0]);

	/*  */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	gl.bindTexture(GL_TEXTURE_CUBE_MAP, m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	/*
	 * - INVALID_ENUM is generated by TexParameter* routines when <pname> is
	 * one of [TEXTURE_SWIZZLE_R, TEXTURE_SWIZZLE_G, TEXTURE_SWIZZLE_B,
	 * TEXTURE_SWIZZLE_A] and <param> is not one of [RED, GREEN, BLUE, ALPHA, ZERO,
	 * ONE];
	 */
	for (size_t i = 0; i < n_states; ++i)
	{
		for (size_t j = 0; j < n_valid_values; ++j)
		{
			const glw::GLenum state = states[i];
			const glw::GLint  value = valid_values[j];

			gl.texParameteri(GL_TEXTURE_CUBE_MAP, state, value);
			verifyError(GL_NO_ERROR);
		}

		for (size_t j = 0; j < n_invalid_values; ++j)
		{
			const glw::GLenum state = states[i];
			const glw::GLint  value = invalid_values[j];

			gl.texParameteri(GL_TEXTURE_CUBE_MAP, state, value);
			verifyError(GL_INVALID_ENUM);
		}
	}

	/*
	 * - INVALID_ENUM is generated by TexParameter*v routines when <pname> is
	 * TEXTURE_SWIZZLE_RGBA and any of four values pointed by <param> is not one of
	 * [RED, GREEN, BLUE, ALPHA, ZERO, ONE].
	 */
	for (size_t i = 0; i < 4 /* number of channels */; ++i)
	{
		for (size_t j = 0; j < n_valid_values; ++j)
		{
			const glw::GLint value = valid_values[j];

			glw::GLint param[4] = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA };

			param[i] = value;

			gl.texParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_SWIZZLE_RGBA, param);
			verifyError(GL_NO_ERROR);
		}

		for (size_t j = 0; j < n_invalid_values; ++j)
		{
			const glw::GLint value = invalid_values[j];

			glw::GLint param[4] = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA };

			param[i] = value;

			gl.texParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_SWIZZLE_RGBA, param);
			verifyError(GL_INVALID_ENUM);
		}
	}

	/* Set result - exceptions are thrown in case of any error */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	return STOP;
}

/** Verifies that proper error was generated
 *
 * @param expected_error
 **/
void APIErrorsTest::verifyError(const glw::GLenum expected_error)
{
	/*  */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	const glw::GLenum error = gl.getError();

	if (expected_error != error)
	{
		TCU_FAIL("Got invalid error");
	}
}

/** Constructor.
 *
 * @param context Rendering context.
 **/
IntialStateTest::IntialStateTest(deqp::Context& context)
	: TestCase(context, "intial_state", "Verifies that initial states are as specified"), m_id(0)
{
	/* Left blank intentionally */
}

/** Deinitialization **/
void IntialStateTest::deinit()
{
	if (0 != m_id)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteTextures(1, &m_id);
		m_id = 0;
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP.
 */
tcu::TestNode::IterateResult IntialStateTest::iterate()
{
	/*  */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	for (size_t tex_tgt_idx = 0; tex_tgt_idx < n_texture_targets; ++tex_tgt_idx)
	{
		const glw::GLenum target = texture_targets[tex_tgt_idx].m_target;

		gl.bindTexture(target, m_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

		verifyValues(target);

		deinit();
	}

	/* Set result - exceptions are thrown in case of any error */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	return STOP;
}

/** Verifies that proper error was generated
 *
 * @param expected_error
 **/
void IntialStateTest::verifyValues(const glw::GLenum texture_target)
{
	/*  */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint red		= 0;
	glw::GLint green	= 0;
	glw::GLint blue		= 0;
	glw::GLint alpha	= 0;
	glw::GLint param[4] = { 0 };

	gl.getTexParameterIiv(texture_target, GL_TEXTURE_SWIZZLE_R, &red);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexParameterIiv");
	gl.getTexParameterIiv(texture_target, GL_TEXTURE_SWIZZLE_G, &green);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexParameterIiv");
	gl.getTexParameterIiv(texture_target, GL_TEXTURE_SWIZZLE_B, &blue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexParameterIiv");
	gl.getTexParameterIiv(texture_target, GL_TEXTURE_SWIZZLE_A, &alpha);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexParameterIiv");
	gl.getTexParameterIiv(texture_target, GL_TEXTURE_SWIZZLE_RGBA, param);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexParameterIiv");

	if (GL_RED != red)
	{
		TCU_FAIL("Got invalid initial state for TEXTURE_SWIZZLE_R");
	}
	if (GL_GREEN != green)
	{
		TCU_FAIL("Got invalid initial state for TEXTURE_SWIZZLE_G");
	}
	if (GL_BLUE != blue)
	{
		TCU_FAIL("Got invalid initial state for TEXTURE_SWIZZLE_B");
	}
	if (GL_ALPHA != alpha)
	{
		TCU_FAIL("Got invalid initial state for TEXTURE_SWIZZLE_A");
	}

	if (GL_RED != param[0])
	{
		TCU_FAIL("Got invalid initial red state for TEXTURE_SWIZZLE_RGBA");
	}
	if (GL_GREEN != param[1])
	{
		TCU_FAIL("Got invalid initial green state for TEXTURE_SWIZZLE_RGBA");
	}
	if (GL_BLUE != param[2])
	{
		TCU_FAIL("Got invalid initial blue state for TEXTURE_SWIZZLE_RGBA");
	}
	if (GL_ALPHA != param[3])
	{
		TCU_FAIL("Got invalid initial alpha state for TEXTURE_SWIZZLE_RGBA");
	}
}

/* Constants used by SmokeTest */
const glw::GLsizei SmokeTest::m_depth		  = 1;
const glw::GLsizei SmokeTest::m_height		  = 1;
const glw::GLsizei SmokeTest::m_width		  = 1;
const glw::GLsizei SmokeTest::m_output_height = 8;
const glw::GLsizei SmokeTest::m_output_width  = 8;

/** Constructor.
 *
 * @param context Rendering context.
 **/
SmokeTest::SmokeTest(deqp::Context& context)
	: TestCase(context, "smoke", "Verifies that all swizzle combinations work with all texture access routines")
	, m_is_ms_supported(false)
	, m_prepare_fbo_id(0)
	, m_out_tex_id(0)
	, m_source_tex_id(0)
	, m_test_fbo_id(0)
	, m_vao_id(0)
{
	/* Left blank intentionally */
}

/** Constructor.
 *
 * @param context Rendering context.
 **/
SmokeTest::SmokeTest(deqp::Context& context, const glw::GLchar* name, const glw::GLchar* description)
	: TestCase(context, name, description)
	, m_is_ms_supported(false)
	, m_prepare_fbo_id(0)
	, m_out_tex_id(0)
	, m_source_tex_id(0)
	, m_test_fbo_id(0)
	, m_vao_id(0)
{
	/* Left blank intentionally */
}

/** Deinitialization **/
void SmokeTest::deinit()
{
	deinitTextures();

	if (m_prepare_fbo_id != 0)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();
		gl.deleteFramebuffers(1, &m_prepare_fbo_id);

		m_prepare_fbo_id = 0;
	}

	if (m_test_fbo_id != 0)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();
		gl.deleteFramebuffers(1, &m_test_fbo_id);

		m_test_fbo_id = 0;
	}

	if (m_vao_id != 0)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP.
 */
tcu::TestNode::IterateResult SmokeTest::iterate()
{
	static const glw::GLenum tested_format = GL_RGBA32UI;
	static const glw::GLenum tested_target = GL_TEXTURE_2D_ARRAY;

	const size_t format_idx = get_index_of_format(tested_format);
	const size_t tgt_idx	= get_index_of_target(tested_target);

	glw::GLint source_channel_sizes[4] = { 0 };

	/*  */
	testInit();

	if (false == isTargetSupported(tgt_idx))
	{
		throw tcu::NotSupportedError("Texture target is not support by implementation", "", __FILE__, __LINE__);
	}

	/* Prepare and fill source texture */
	prepareSourceTexture(format_idx, tgt_idx, source_channel_sizes);
	if (false == fillSourceTexture(format_idx, tgt_idx))
	{
		TCU_FAIL("Failed to prepare source texture");
	}

	/* Iterate over all cases */
	for (size_t access_idx = 0; access_idx < n_texture_access; ++access_idx)
	{
		/* Skip invalid cases */
		if (false == isTargetSuppByAccess(access_idx, tgt_idx))
		{
			continue;
		}

		for (size_t r = 0; r < n_valid_values; ++r)
		{
			for (size_t g = 0; g < n_valid_values; ++g)
			{
				for (size_t b = 0; b < n_valid_values; ++b)
				{
					for (size_t a = 0; a < n_valid_values; ++a)
					{
						for (size_t channel_idx = 0; channel_idx < 4; ++channel_idx)
						{
							const testCase test_case = { channel_idx,
														 format_idx,
														 tgt_idx,
														 access_idx,
														 valid_values[r],
														 valid_values[g],
														 valid_values[b],
														 valid_values[a],
														 { source_channel_sizes[0], source_channel_sizes[1],
														   source_channel_sizes[2], source_channel_sizes[3] } };

							executeTestCase(test_case);

							deinitOutputTexture();
						} /* iteration over channels */
					}	 /* iteration over swizzle combinations */
				}
			}
		}
	} /* iteration over access routines */

	/* Set result - exceptions are thrown in case of any error */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	return STOP;
}

/** Deinitialization of output texture **/
void SmokeTest::deinitOutputTexture()
{
	if (m_out_tex_id != 0)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();
		gl.deleteTextures(1, &m_out_tex_id);

		m_out_tex_id = 0;
	}
}

/** Deinitialization of textures **/
void SmokeTest::deinitTextures()
{
	deinitOutputTexture();

	if (m_source_tex_id != 0)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();
		gl.deleteTextures(1, &m_source_tex_id);

		m_source_tex_id = 0;
	}
}

/** Captures and verifies contents of output texture
 *
 * @param test_case                 Test case instance
 * @param output_format_index       Index of format used by output texture
 * @parma output_channel_size       Size of storage used by output texture in bits
 * @param index_of_swizzled_channel Index of swizzled channel
 */
void SmokeTest::captureAndVerify(const testCase& test_case, size_t output_format_index, glw::GLint output_channel_size,
								 size_t index_of_swizzled_channel)
{
	const _texture_format& output_format = texture_formats[output_format_index];

	/*  */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Storage for image data */
	glw::GLubyte result_image[m_output_width * m_output_height * 4 /* channles */ * sizeof(glw::GLuint)];

	/* Get image data */
	gl.bindTexture(GL_TEXTURE_2D, m_out_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	gl.getTexImage(GL_TEXTURE_2D, 0 /* level */, output_format.m_format, output_format.m_type, result_image);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getTexImage");

	/* Unbind output texture */
	gl.bindTexture(GL_TEXTURE_2D, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	/* Verification */
	verifyOutputImage(test_case, output_format_index, output_channel_size, index_of_swizzled_channel, result_image);
}

/** Draws four points
 *
 * @param target          Target of source texture
 * @param texture_swizzle Set of texture swizzle values
 * @param use_rgba_enum   If texture swizzle states should be set with RGBA enum or separe calls
 **/
void SmokeTest::draw(glw::GLenum target, const glw::GLint* texture_swizzle, bool use_rgba_enum)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare source texture */
	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ActiveTexture");

	gl.bindTexture(target, m_source_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	/* Set texture swizzle */
	if (true == use_rgba_enum)
	{
		gl.texParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, texture_swizzle);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteriv");
	}
	else
	{
		gl.texParameteri(target, GL_TEXTURE_SWIZZLE_R, texture_swizzle[0]);
		gl.texParameteri(target, GL_TEXTURE_SWIZZLE_G, texture_swizzle[1]);
		gl.texParameteri(target, GL_TEXTURE_SWIZZLE_B, texture_swizzle[2]);
		gl.texParameteri(target, GL_TEXTURE_SWIZZLE_A, texture_swizzle[3]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");
	}

	/* Clear */
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Clear");

	/* Draw */
	gl.drawArrays(GL_TRIANGLE_STRIP, 0 /* first */, 4 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

	/* Revert texture swizzle */
	gl.texParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_RED);
	gl.texParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
	gl.texParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
	gl.texParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");

	/* Unbind source texture */
	gl.bindTexture(target, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");
}

/** Executes test case
 *
 * @param test_case Test case instance
 **/
void SmokeTest::executeTestCase(const testCase& test_case)
{
	const _texture_format& source_format	   = texture_formats[test_case.m_source_texture_format_index];
	const glw::GLint	   red				   = test_case.m_texture_swizzle_red;
	const glw::GLint	   green			   = test_case.m_texture_swizzle_green;
	const glw::GLint	   blue				   = test_case.m_texture_swizzle_blue;
	const glw::GLint	   alpha			   = test_case.m_texture_swizzle_alpha;
	const glw::GLint	   param[4]			   = { red, green, blue, alpha };
	const size_t		   channel			   = get_swizzled_channel_idx(test_case.m_channel_index, param);
	glw::GLint			   out_channel_size	= 0;
	const glw::GLenum	  out_internal_format = get_internal_format_for_channel(source_format, channel);
	const size_t		   out_format_idx	  = get_index_of_format(out_internal_format);

	/*  */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare output */
	prepareOutputTexture(out_format_idx);

	gl.bindTexture(GL_TEXTURE_2D, m_out_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_test_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindFramebuffer");

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_out_tex_id, 0 /* level */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "FramebufferTexture2D");

	/* Set Viewport */
	gl.viewport(0 /* x */, 0 /* y */, m_output_width, m_output_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Viewport");

	/* Get internal storage size of output texture */
	gl.getTexLevelParameteriv(GL_TEXTURE_2D, 0 /* level */, GL_TEXTURE_RED_SIZE, &out_channel_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexLevelParameteriv");

	/* Unbind output texture */
	gl.bindTexture(GL_TEXTURE_2D, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	prepareAndTestProgram(test_case, out_format_idx, out_channel_size, channel, true);
	prepareAndTestProgram(test_case, out_format_idx, out_channel_size, channel, false);

	/* Unbind FBO */
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindFramebuffer");
}

/** Fills source texture
 *
 * @param format_idx Index of format
 * @param target_idx Index of target
 *
 * @return True if operation was successful, false other wise
 **/
bool SmokeTest::fillSourceTexture(size_t format_idx, size_t target_idx)
{
	static const glw::GLuint rgba32ui[4] = { 0x3fffffff, 0x7fffffff, 0xbfffffff, 0xffffffff };

	const glw::GLenum	  target		  = texture_targets[target_idx].m_target;
	const _texture_format& texture_format = texture_formats[format_idx];

	/*  */
	const glw::Functions& gl   = m_context.getRenderContext().getFunctions();
	const glw::GLvoid*	data = 0;

	/* Bind texture and FBO */
	gl.bindTexture(target, m_source_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	/* Set color */
	switch (texture_format.m_internal_format)
	{
	case GL_RGBA32UI:
		data = (const glw::GLubyte*)rgba32ui;
		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	/* Attach texture */
	switch (target)
	{
	case GL_TEXTURE_2D_ARRAY:
		gl.texSubImage3D(target, 0 /* level */, 0 /* x */, 0 /* y */, 0 /* z */, m_width, m_height, m_depth,
						 texture_format.m_format, texture_format.m_type, data);
		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	/* Unbind */
	gl.bindTexture(target, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	/* Done */
	return true;
}

/** Gets source of fragment shader
 *
 * @param test_case           Test case instance
 * @param output_format_index Index of output format
 * @param is_tested_stage     Selects if fragment or vertex shader makes texture access
 *
 * @return Source of shader
 **/
std::string SmokeTest::getFragmentShader(const testCase& test_case, size_t output_format_index, bool is_tested_stage)
{
	static const glw::GLchar* fs_blank_template = "#version 330 core\n"
												  "\n"
												  "flat in BASIC_TYPE result;\n"
												  "\n"
												  "out BASIC_TYPE out_color;\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    out_color = result;\n"
												  "}\n"
												  "\n";

	static const glw::GLchar* fs_test_template = "#version 330 core\n"
												 "\n"
												 "uniform PREFIXsamplerSAMPLER_TYPE sampler;\n"
												 "\n"
												 "out BASIC_TYPE out_color;\n"
												 "\n"
												 "void main()\n"
												 "{\n"
												 "    BASIC_TYPE result = TEXTURE_ACCESS(sampler, ARGUMENTS).CHANNEL;\n"
												 "\n"
												 "    out_color = result;\n"
												 "}\n"
												 "\n";

	/* */
	const std::string&	 arguments	 = prepareArguments(test_case);
	const _texture_access& access		 = texture_access[test_case.m_texture_access_index];
	const glw::GLchar*	 channel		 = channels[test_case.m_channel_index];
	const _texture_format& output_format = texture_formats[output_format_index];
	const _texture_format& source_format = texture_formats[test_case.m_source_texture_format_index];
	const _texture_target& target		 = texture_targets[test_case.m_source_texture_target_index];

	std::string fs;
	size_t		position = 0;

	if (is_tested_stage)
	{
		fs = fs_test_template;

		Utils::replaceToken("PREFIX", position, source_format.m_sampler.m_sampler_prefix, fs);
		Utils::replaceToken("SAMPLER_TYPE", position, target.m_sampler_type, fs);
		Utils::replaceToken("BASIC_TYPE", position, output_format.m_sampler.m_basic_type, fs);
		Utils::replaceToken("BASIC_TYPE", position, source_format.m_sampler.m_basic_type, fs);
		Utils::replaceToken("TEXTURE_ACCESS", position, access.m_name, fs);
		Utils::replaceToken("ARGUMENTS", position, arguments.c_str(), fs);
		Utils::replaceToken("CHANNEL", position, channel, fs);
	}
	else
	{
		fs = fs_blank_template;

		Utils::replaceToken("BASIC_TYPE", position, source_format.m_sampler.m_basic_type, fs);
		Utils::replaceToken("BASIC_TYPE", position, output_format.m_sampler.m_basic_type, fs);
	}

	return fs;
}

/** Gets source of vertex shader
 *
 * @param test_case           Test case instance
 * @param is_tested_stage     Selects if vertex or fragment shader makes texture access
 *
 * @return Source of shader
 **/
std::string SmokeTest::getVertexShader(const testCase& test_case, bool is_tested_stage)
{
	static const glw::GLchar* vs_blank_template = "#version 330 core\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    switch (gl_VertexID)\n"
												  "    {\n"
												  "      case 0: gl_Position = vec4(-1.0, 1.0, 0.0, 1.0); break; \n"
												  "      case 1: gl_Position = vec4( 1.0, 1.0, 0.0, 1.0); break; \n"
												  "      case 2: gl_Position = vec4(-1.0,-1.0, 0.0, 1.0); break; \n"
												  "      case 3: gl_Position = vec4( 1.0,-1.0, 0.0, 1.0); break; \n"
												  "    }\n"
												  "}\n"
												  "\n";

	static const glw::GLchar* vs_test_template = "#version 330 core\n"
												 "\n"
												 "uniform PREFIXsamplerSAMPLER_TYPE sampler;\n"
												 "\n"
												 "flat out BASIC_TYPE result;\n"
												 "\n"
												 "void main()\n"
												 "{\n"
												 "    result = TEXTURE_ACCESS(sampler, ARGUMENTS).CHANNEL;\n"
												 "\n"
												 "    switch (gl_VertexID)\n"
												 "    {\n"
												 "      case 0: gl_Position = vec4(-1.0, 1.0, 0.0, 1.0); break; \n"
												 "      case 1: gl_Position = vec4( 1.0, 1.0, 0.0, 1.0); break; \n"
												 "      case 2: gl_Position = vec4(-1.0,-1.0, 0.0, 1.0); break; \n"
												 "      case 3: gl_Position = vec4( 1.0,-1.0, 0.0, 1.0); break; \n"
												 "    }\n"
												 "}\n"
												 "\n";

	std::string vs;

	if (is_tested_stage)
	{
		/* */
		const std::string&	 arguments	 = prepareArguments(test_case);
		const _texture_access& access		 = texture_access[test_case.m_texture_access_index];
		const glw::GLchar*	 channel		 = channels[test_case.m_channel_index];
		const _texture_format& source_format = texture_formats[test_case.m_source_texture_format_index];
		const _texture_target& target		 = texture_targets[test_case.m_source_texture_target_index];

		size_t position = 0;

		vs = vs_test_template;

		Utils::replaceToken("PREFIX", position, source_format.m_sampler.m_sampler_prefix, vs);
		Utils::replaceToken("SAMPLER_TYPE", position, target.m_sampler_type, vs);
		Utils::replaceToken("BASIC_TYPE", position, source_format.m_sampler.m_basic_type, vs);
		Utils::replaceToken("TEXTURE_ACCESS", position, access.m_name, vs);
		Utils::replaceToken("ARGUMENTS", position, arguments.c_str(), vs);
		Utils::replaceToken("CHANNEL", position, channel, vs);
	}
	else
	{
		vs = vs_blank_template;
	}

	return vs;
}

/** Check if target is supported
 *
 * @param target_idx Index of target
 *
 * @return true if target is supported, false otherwise
 **/
bool SmokeTest::isTargetSupported(size_t target_idx)
{
	const _texture_target& target = texture_targets[target_idx];

	bool is_supported = true;

	switch (target.m_target)
	{
	case GL_TEXTURE_2D_MULTISAMPLE:
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		is_supported = m_is_ms_supported;
		break;

	default:
		break;
	}

	return is_supported;
}

/** Check if target is supported by access routine
 *
 * @param access_idx Index of access routine
 * @param target_idx Index of target
 *
 * @return true if target is supported, false otherwise
 **/
bool SmokeTest::isTargetSuppByAccess(size_t access_idx, size_t target_idx)
{
	const _texture_access& access		 = texture_access[access_idx];
	const _texture_target& source_target = texture_targets[target_idx];

	if ((false == source_target.m_support_integral_coordinates) && (true == access.m_use_integral_coordinates))
	{
		/* Cases are not valid, texelFetch* is not supported by the target */
		return false;
	}

	if ((false == source_target.m_support_offset) && (true == access.m_use_offsets))
	{
		/* Cases are not valid, texture*Offset is not supported by the target */
		return false;
	}

	if ((false == source_target.m_support_lod) && (true == access.m_use_lod))
	{
		/* Access is one of texture*Lod* or texelFetch* */
		/* Target is one of MS or rect */

		if ((true == source_target.m_require_multisampling) && (true == access.m_support_multisampling))
		{
			/* texelFetch */
			/* One of MS targets */
			return true;
		}

		/* Cases are not valid, either lod or sample is required but target does not supported that */
		return false;
	}

	if ((false == source_target.m_supports_proj) && (1 == access.m_n_coordinates))
	{
		/* Cases are not valid, textureProj* is not supported by the target */
		return false;
	}

	if ((true == source_target.m_require_multisampling) && (false == access.m_support_multisampling))
	{
		/* Cases are not valid, texelFetch* is not supported by the target */
		return false;
	}

	return true;
}

/** Check if target is supported by format
 *
 * @param format_idx Index of format
 * @param target_idx Index of target
 *
 * @return true if target is supported, false otherwise
 **/
bool SmokeTest::isTargetSuppByFormat(size_t format_idx, size_t target_idx)
{
	const _texture_format& format		 = texture_formats[format_idx];
	const _texture_target& source_target = texture_targets[target_idx];

	bool is_supported = true;

	switch (format.m_internal_format)
	{
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32:
	case GL_DEPTH_COMPONENT32F:
	case GL_DEPTH24_STENCIL8:
	case GL_DEPTH32F_STENCIL8:
		switch (source_target.m_target)
		{
		case GL_TEXTURE_3D:
		case GL_TEXTURE_2D_MULTISAMPLE:
		case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
			is_supported = false;
			break;
		default:
			break;
		}
		break;

	case GL_RGB9_E5:
		switch (source_target.m_target)
		{
		case GL_TEXTURE_2D_MULTISAMPLE:
		case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
			is_supported = false;
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}

	return is_supported;
}

/** Logs details of test case
 *
 * @parma test_case Test case instance
 **/
void SmokeTest::logTestCaseDetials(const testCase& test_case)
{
	const glw::GLenum	  target			   = texture_targets[test_case.m_source_texture_target_index].m_target;
	const _texture_format& source_format	   = texture_formats[test_case.m_source_texture_format_index];
	const glw::GLint	   red				   = test_case.m_texture_swizzle_red;
	const glw::GLint	   green			   = test_case.m_texture_swizzle_green;
	const glw::GLint	   blue				   = test_case.m_texture_swizzle_blue;
	const glw::GLint	   alpha			   = test_case.m_texture_swizzle_alpha;
	const glw::GLint	   param[4]			   = { red, green, blue, alpha };
	const size_t		   channel			   = get_swizzled_channel_idx(test_case.m_channel_index, param);
	const glw::GLenum	  out_internal_format = get_internal_format_for_channel(source_format, channel);
	const size_t		   out_format_idx	  = get_index_of_format(out_internal_format);
	const _texture_format& output_format	   = texture_formats[out_format_idx];

	m_context.getTestContext().getLog() << tcu::TestLog::Message << "Test case details. Source texture: Target: "
										<< glu::getTextureTargetStr(target)
										<< ". Format: " << glu::getTextureFormatName(source_format.m_internal_format)
										<< ", " << glu::getTextureFormatName(source_format.m_format) << ", "
										<< glu::getTypeStr(source_format.m_type)
										<< ", DS: " << glu::getTextureDepthStencilModeName(source_format.m_ds_mode)
										<< ". Swizzle: [" << glu::getTextureSwizzleStr(red) << ", "
										<< glu::getTextureSwizzleStr(green) << ", " << glu::getTextureSwizzleStr(blue)
										<< ", " << glu::getTextureSwizzleStr(alpha)
										<< "]. Channel: " << channels[test_case.m_channel_index]
										<< ". Access: " << texture_access[test_case.m_texture_access_index].m_name
										<< ". Output texture: Format: "
										<< glu::getTextureFormatName(output_format.m_internal_format) << ", "
										<< glu::getTextureFormatName(output_format.m_format) << ", "
										<< glu::getTypeStr(output_format.m_type) << "." << tcu::TestLog::EndMessage;
}

/** Prepares program then draws and verifies resutls for both ways of setting texture swizzle
 *
 * @param test_case                 Test case instance
 * @param output_format_index       Index of format used by output texture
 * @parma output_channel_size       Size of storage used by output texture in bits
 * @param index_of_swizzled_channel Index of swizzled channel
 * @param test_vertex_stage         Selects if vertex or fragment shader should execute texture access
 **/
void SmokeTest::prepareAndTestProgram(const testCase& test_case, size_t output_format_index,
									  glw::GLint output_channel_size, size_t index_of_swizzled_channel,
									  bool test_vertex_stage)
{
	const _texture_target& source_target = texture_targets[test_case.m_source_texture_target_index];
	const glw::GLint	   red			 = test_case.m_texture_swizzle_red;
	const glw::GLint	   green		 = test_case.m_texture_swizzle_green;
	const glw::GLint	   blue			 = test_case.m_texture_swizzle_blue;
	const glw::GLint	   alpha		 = test_case.m_texture_swizzle_alpha;
	const glw::GLint	   param[4]		 = { red, green, blue, alpha };

	/*  */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare program */
	const std::string& fs = getFragmentShader(test_case, output_format_index, !test_vertex_stage);
	const std::string& vs = getVertexShader(test_case, test_vertex_stage);

	Utils::programInfo program(m_context);
	program.build(fs.c_str(), vs.c_str());

	gl.useProgram(program.m_program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");

	/* Prepare sampler */
	glw::GLint location = gl.getUniformLocation(program.m_program_object_id, "sampler");
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformLocation");

	if (-1 == location)
	{
		TCU_FAIL("Uniform is not available");
	}

	gl.uniform1i(location, 0 /* texture unit */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");

	draw(source_target.m_target, param, false);
	captureAndVerify(test_case, output_format_index, output_channel_size, index_of_swizzled_channel);

	draw(source_target.m_target, param, true);
	captureAndVerify(test_case, output_format_index, output_channel_size, index_of_swizzled_channel);
}

/** Prepares arguments for texture access routine call
 *
 * @param test_case Test case instance
 *
 * @return Source code
 **/
std::string SmokeTest::prepareArguments(const testCase& test_case)
{
	const _texture_access& access = texture_access[test_case.m_texture_access_index];
	const _texture_target& target = texture_targets[test_case.m_source_texture_target_index];

	std::string		   arguments   = "COORDINATESLODDERIVATIVESOFFSETSSAMPLE";
	const std::string& coordinates = prepareCoordinates(test_case);

	size_t position = 0;

	Utils::replaceToken("COORDINATES", position, coordinates.c_str(), arguments);

	if ((true == access.m_use_lod) && (true == target.m_support_lod))
	{
		Utils::replaceToken("LODDERIVATIVES", position, ", int(0)", arguments);
	}
	else if (true == access.m_use_derivaties)
	{
		const std::string& derivatives_0 = prepareDerivatives(test_case, 0);
		const std::string& derivatives_1 = prepareDerivatives(test_case, 1);
		const size_t	   start_pos	 = position;

		Utils::replaceToken("LODDERIVATIVES", position, ", XXXXX, XXXXX", arguments);
		position = start_pos + 2;
		Utils::replaceToken("XXXXX", position, derivatives_0.c_str(), arguments);
		Utils::replaceToken("XXXXX", position, derivatives_1.c_str(), arguments);
	}
	else
	{
		Utils::replaceToken("LODDERIVATIVES", position, "", arguments);
	}

	if (true == access.m_use_offsets)
	{
		const std::string& offsets   = prepareOffsets(test_case);
		const size_t	   start_pos = position;

		Utils::replaceToken("OFFSETS", position, ", XXXXX", arguments);
		position = start_pos + 2;
		Utils::replaceToken("XXXXX", position, offsets.c_str(), arguments);
	}
	else
	{
		Utils::replaceToken("OFFSETS", position, "", arguments);
	}

	if ((true == target.m_require_multisampling) && (true == access.m_support_multisampling))
	{
		const std::string& sample	= prepareSample();
		const size_t	   start_pos = position;

		Utils::replaceToken("SAMPLE", position, ", XX", arguments);
		position = start_pos + 2;
		Utils::replaceToken("XX", position, sample.c_str(), arguments);
	}
	else
	{
		Utils::replaceToken("SAMPLE", position, "", arguments);
	}

	return arguments;
}

/** Prepares coordinate for texture access routine call
 *
 * @param test_case Test case instance
 *
 * @return Source code
 **/
std::string SmokeTest::prepareCoordinates(const testCase& test_case)
{
	const _texture_access& access = texture_access[test_case.m_texture_access_index];
	const _texture_target& target = texture_targets[test_case.m_source_texture_target_index];

	const glw::GLchar* type = 0;

	std::string coordinates = "TYPE(VAL_LIST)";

	if (false == access.m_use_integral_coordinates)
	{
		switch (access.m_n_coordinates + target.m_n_array_coordinates + target.m_n_coordinates)
		{
		case 1:
			type = "float";
			break;
		case 2:
			type = "vec2";
			break;
		case 3:
			type = "vec3";
			break;
		case 4:
			type = "vec4";
			break;
		default:
			TCU_FAIL("Invalid value");
			break;
		}
	}
	else
	{
		switch (access.m_n_coordinates + target.m_n_array_coordinates + target.m_n_coordinates)
		{
		case 1:
			type = "int";
			break;
		case 2:
			type = "ivec2";
			break;
		case 3:
			type = "ivec3";
			break;
		case 4:
			type = "ivec4";
			break;
		default:
			TCU_FAIL("Invalid value");
			break;
		}
	}

	size_t position = 0;

	Utils::replaceToken("TYPE", position, type, coordinates);

	for (size_t i = 0; i < target.m_n_coordinates; ++i)
	{
		size_t start_position = position;

		Utils::replaceToken("VAL_LIST", position, "0, VAL_LIST", coordinates);

		position = start_position + 1;
	}

	for (size_t i = 0; i < target.m_n_array_coordinates; ++i)
	{
		size_t start_position = position;

		Utils::replaceToken("VAL_LIST", position, "0, VAL_LIST", coordinates);

		position = start_position + 1;
	}

	for (size_t i = 0; i < access.m_n_coordinates; ++i)
	{
		size_t start_position = position;

		Utils::replaceToken("VAL_LIST", position, "1, VAL_LIST", coordinates);

		position = start_position + 1;
	}

	Utils::replaceToken(", VAL_LIST", position, "", coordinates);

	return coordinates;
}

/** Prepares derivatives for texture access routine call
 *
 * @param test_case Test case instance
 *
 * @return Source code
 **/
std::string SmokeTest::prepareDerivatives(const testCase& test_case, size_t index)
{
	const _texture_target& target = texture_targets[test_case.m_source_texture_target_index];

	const glw::GLchar* type = 0;

	std::string derivatives = "TYPE(VAL_LIST)";

	switch (target.m_n_derivatives)
	{
	case 1:
		type = "float";
		break;
	case 2:
		type = "vec2";
		break;
	case 3:
		type = "vec3";
		break;
	case 4:
		type = "vec4";
		break;
	default:
		TCU_FAIL("Invalid value");
		break;
	}

	size_t position = 0;

	Utils::replaceToken("TYPE", position, type, derivatives);

	for (size_t i = 0; i < target.m_n_derivatives; ++i)
	{
		size_t start_position = position;

		if (index == i)
		{
			Utils::replaceToken("VAL_LIST", position, "1.0, VAL_LIST", derivatives);
		}
		else
		{
			Utils::replaceToken("VAL_LIST", position, "0.0, VAL_LIST", derivatives);
		}

		position = start_position + 1;
	}

	Utils::replaceToken(", VAL_LIST", position, "", derivatives);

	return derivatives;
}

/** Prepares offsets for texture access routine call
 *
 * @param test_case Test case instance
 *
 * @return Source code
 **/
std::string SmokeTest::prepareOffsets(const testCase& test_case)
{
	const _texture_target& target = texture_targets[test_case.m_source_texture_target_index];

	const glw::GLchar* type = DE_NULL;

	std::string offsets = "TYPE(VAL_LIST)";

	switch (target.m_n_derivatives)
	{
	case 1:
		type = "int";
		break;
	case 2:
		type = "ivec2";
		break;
	case 3:
		type = "ivec3";
		break;
	case 4:
		type = "ivec4";
		break;
	default:
		TCU_FAIL("Invalid value");
		break;
	}

	size_t position = 0;

	Utils::replaceToken("TYPE", position, type, offsets);

	for (size_t i = 0; i < target.m_n_coordinates; ++i)
	{
		size_t start_position = position;

		Utils::replaceToken("VAL_LIST", position, "0, VAL_LIST", offsets);

		position = start_position + 1;
	}

	Utils::replaceToken(", VAL_LIST", position, "", offsets);

	return offsets;
}

/** Prepares output texture
 *
 * @param format_idx Index of texture format
 **/
void SmokeTest::prepareOutputTexture(size_t format_idx)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	const _texture_format& format = texture_formats[format_idx];

	gl.genTextures(1, &m_out_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	gl.bindTexture(GL_TEXTURE_2D, m_out_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	gl.texImage2D(GL_TEXTURE_2D, 0 /* level */, format.m_internal_format, m_output_width, m_output_height,
				  0 /* border */, format.m_format, format.m_type, 0 /* pixels */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexImage2D");

	gl.bindTexture(GL_TEXTURE_2D, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");
}

/** Prepares sample for texture access routine call
 *
 * @return Source code
 **/
std::string SmokeTest::prepareSample()
{
	glw::GLsizei	  samples = 1;
	std::stringstream stream;

	/* Get max number of samples */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.getIntegerv(GL_MAX_SAMPLES, &samples);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	stream << samples - 1;

	return stream.str();
}

/** Prepares source texture
 *
 * @param format_idx Index of texture format
 * @param target_idx Index of texture target
 * @param out_sizes  Sizes of storage used for texture channels
 **/
void SmokeTest::prepareSourceTexture(size_t format_idx, size_t target_idx, glw::GLint out_sizes[4])
{
	static const glw::GLint border = 0;
	static const glw::GLint level  = 0;

	/* */
	const glw::GLenum	  target		  = texture_targets[target_idx].m_target;
	const _texture_format& texture_format = texture_formats[format_idx];

	/* */
	glw::GLenum		   error		   = 0;
	const glw::GLenum  format		   = texture_format.m_format;
	const glw::GLchar* function_name   = "unknown";
	const glw::GLenum  internal_format = texture_format.m_internal_format;
	glw::GLsizei	   samples		   = 1;
	glw::GLenum		   target_get_prm  = target;
	const glw::GLenum  type			   = texture_format.m_type;

	/*  */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get max number of samples */
	gl.getIntegerv(GL_MAX_SAMPLES, &samples);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/* Generate and bind */
	gl.genTextures(1, &m_source_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	gl.bindTexture(target, m_source_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	/* Allocate storage */
	switch (target)
	{
	case GL_TEXTURE_1D:

		gl.texImage1D(target, level, internal_format, m_width, border, format, type, 0 /* pixels */);
		error		  = gl.getError();
		function_name = "TexImage1D";

		break;

	case GL_TEXTURE_1D_ARRAY:
	case GL_TEXTURE_2D:
	case GL_TEXTURE_RECTANGLE:
		gl.texImage2D(target, level, internal_format, m_width, m_height, border, format, type, 0 /* pixels */);
		error		  = gl.getError();
		function_name = "TexImage2D";

		break;

	case GL_TEXTURE_2D_ARRAY:
	case GL_TEXTURE_3D:
		gl.texImage3D(target, level, internal_format, m_width, m_height, m_depth, border, format, type, 0 /* pixels */);
		error		  = gl.getError();
		function_name = "TexImage3D";

		break;

	case GL_TEXTURE_CUBE_MAP:
		for (size_t i = 0; i < n_cube_map_faces; ++i)
		{
			gl.texImage2D(cube_map_faces[i], level, internal_format, m_width, m_height, border, format, type,
						  0 /* pixels */);
		}
		error		  = gl.getError();
		function_name = "TexImage2D";

		target_get_prm = cube_map_faces[0];

		break;

	case GL_TEXTURE_2D_MULTISAMPLE:
		gl.texImage2DMultisample(target, samples, internal_format, m_width, m_height,
								 GL_FALSE /* fixedsamplelocation */);
		error		  = gl.getError();
		function_name = "TexImage2DMultisample";

		break;

	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		gl.texImage3DMultisample(target, samples, internal_format, m_width, m_height, m_depth,
								 GL_FALSE /* fixedsamplelocation */);
		error		  = gl.getError();
		function_name = "TexImage3DMultisample";

		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	/* Log error */
	GLU_EXPECT_NO_ERROR(error, function_name);

	/* Make texture complete and set ds texture mode */
	if ((GL_TEXTURE_2D_MULTISAMPLE != target) && (GL_TEXTURE_2D_MULTISAMPLE_ARRAY != target) &&
		(GL_TEXTURE_RECTANGLE != target))
	{
		gl.texParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");

		gl.texParameteri(target, GL_TEXTURE_MAX_LEVEL, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");

		gl.texParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");

		gl.texParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");
	}

	if (texture_format.m_ds_mode == GL_STENCIL_INDEX)
	{
		gl.texParameteri(target, GL_DEPTH_STENCIL_TEXTURE_MODE, texture_format.m_ds_mode);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");
	}

	/* Get internal storage sizes */
	switch (internal_format)
	{
	case GL_DEPTH24_STENCIL8:
	case GL_DEPTH32F_STENCIL8:

		gl.getTexLevelParameteriv(target_get_prm, 0 /* level */, GL_TEXTURE_STENCIL_SIZE, out_sizes + 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexLevelParameteriv");

	/* Fall through */

	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32:
	case GL_DEPTH_COMPONENT32F:

		gl.getTexLevelParameteriv(target_get_prm, 0 /* level */, GL_TEXTURE_DEPTH_SIZE, out_sizes + 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexLevelParameteriv");

		break;

	default:

		gl.getTexLevelParameteriv(target_get_prm, 0 /* level */, GL_TEXTURE_RED_SIZE, out_sizes + 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexLevelParameteriv");

		gl.getTexLevelParameteriv(target_get_prm, 0 /* level */, GL_TEXTURE_GREEN_SIZE, out_sizes + 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexLevelParameteriv");

		gl.getTexLevelParameteriv(target_get_prm, 0 /* level */, GL_TEXTURE_BLUE_SIZE, out_sizes + 2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexLevelParameteriv");

		gl.getTexLevelParameteriv(target_get_prm, 0 /* level */, GL_TEXTURE_ALPHA_SIZE, out_sizes + 3);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexLevelParameteriv");

		break;
	}

	/* Unbind texture */
	gl.bindTexture(target, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");
}

/** Initializes frame buffer and vertex array
 *
 **/
void SmokeTest::testInit()
{
	/*  */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint major = 0;
	glw::GLint minor = 0;

	gl.getIntegerv(GL_MAJOR_VERSION, &major);
	gl.getIntegerv(GL_MINOR_VERSION, &minor);

	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	if (4 < major)
	{
		m_is_ms_supported = true;
	}
	else if (4 == major)
	{
		if (3 <= minor)
		{
			m_is_ms_supported = true;
		}
	}

	/* Context is below 4.3 */
	if (false == m_is_ms_supported)
	{
		m_is_ms_supported = m_context.getContextInfo().isExtensionSupported("GL_ARB_texture_storage_multisample");
	}

#if ENABLE_DEBUG

	gl.debugMessageCallback(debug_proc, &m_context);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");

#endif /* ENABLE_DEBUG */

	/* Prepare FBOs */
	gl.genFramebuffers(1, &m_prepare_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenFramebuffers");

	gl.genFramebuffers(1, &m_test_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenFramebuffers");

	/* Prepare blank VAO */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genVertexArrays");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindVertexArray");
}

/** Verifies contents of output image
 *
 * @param test_case                 Test case instance
 * @param ignored
 * @param ignored
 * @param index_of_swizzled_channel Index of swizzled channel
 * @param data                      Image contents
 **/
void SmokeTest::verifyOutputImage(const testCase& test_case, size_t /* output_format_index */,
								  glw::GLint /* output_channel_size */, size_t index_of_swizzled_channel,
								  const glw::GLubyte* data)
{
	static const glw::GLuint rgba32ui[6] = { 0x3fffffff, 0x7fffffff, 0xbfffffff, 0xffffffff, 1, 0 };

	const _texture_format& source_format = texture_formats[test_case.m_source_texture_format_index];

	/* Set color */
	switch (source_format.m_internal_format)
	{
	case GL_RGBA32UI:
	{
		glw::GLuint		   expected_value = rgba32ui[index_of_swizzled_channel];
		const glw::GLuint* image		  = (glw::GLuint*)data;

		for (size_t i = 0; i < m_output_width * m_output_height; ++i)
		{
			if (image[i] != expected_value)
			{
				TCU_FAIL("Found pixel with wrong value");
			}
		}
	}
	break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}
}

/** Constructor.
 *
 * @param context Rendering context.
 **/
FunctionalTest::FunctionalTest(deqp::Context& context)
	: SmokeTest(context, "functional",
				"Verifies that swizzle is respected for textures of different formats and targets")
{
	/* Left blank intentionally */
}

/** Executes test iteration.
 *
 *  @return Returns STOP.
 */
tcu::TestNode::IterateResult FunctionalTest::iterate()
{

#if FUNCTIONAL_TEST_ALL_FORMATS == 0

	static const glw::GLenum tested_formats[] = { GL_R8,	  GL_R3_G3_B2,		   GL_RGBA16, GL_R11F_G11F_B10F,
												  GL_RGB9_E5, GL_DEPTH32F_STENCIL8 };
	static const size_t n_tested_formats = sizeof(tested_formats) / sizeof(tested_formats[0]);

#endif /* FUNCTIONAL_TEST_ALL_FORMATS == 0 */

#if FUNCTIONAL_TEST_ALL_TARGETS == 0

	static const glw::GLenum tested_targets[] = { GL_TEXTURE_1D, GL_TEXTURE_2D_MULTISAMPLE_ARRAY };
	static const size_t		 n_tested_targets = sizeof(tested_targets) / sizeof(tested_targets[0]);

#endif /* FUNCTIONAL_TEST_ALL_TARGETS == 0 */

#if FUNCTIONAL_TEST_ALL_ACCESS_ROUTINES == 0

	static const size_t access_idx = 4; /* 4 - index of "texelFetch" entry in texture_access_routines */

#endif /* FUNCTIONAL_TEST_ALL_ACCESS_ROUTINES == 0 */

#if FUNCTIONAL_TEST_ALL_SWIZZLE_COMBINATIONS == 0

	static const size_t tested_swizzle_combinations[][4] = { { 3, 2, 1,
															   0 }, /* values are indices of entries in valid_values */
															 { 5, 4, 0, 3 },
															 { 5, 5, 5, 5 },
															 { 4, 4, 4, 4 },
															 { 2, 2, 2, 2 } };
	static const size_t n_tested_swizzle_combinations =
		sizeof(tested_swizzle_combinations) / sizeof(tested_swizzle_combinations[0]);

#endif /* FUNCTIONAL_TEST_ALL_SWIZZLE_COMBINATIONS == 0 */

	/*  */
	bool	   test_result			   = true;
	glw::GLint source_channel_sizes[4] = { 0 };

	/*  */
	testInit();

/* Iterate over all cases */

#if FUNCTIONAL_TEST_ALL_FORMATS

	for (size_t format_idx = 0; format_idx < n_texture_formats; ++format_idx)
	{

		/* Check that format is supported by context. */
		if (!glu::contextSupports(m_context.getRenderContext().getType(),
								  texture_formats[format_idx].m_minimum_gl_context))
		{
			continue;
		}

#else /* FUNCTIONAL_TEST_ALL_FORMATS */

	for (size_t tested_format_idx = 0; tested_format_idx < n_tested_formats; ++tested_format_idx)
	{
		const size_t format_idx = get_index_of_format(tested_formats[tested_format_idx]);

#endif /* FUNCTIONAL_TEST_ALL_FORMATS */

#if FUNCTIONAL_TEST_ALL_TARGETS

		for (size_t tgt_idx = 0; tgt_idx < n_texture_targets; ++tgt_idx)
		{

#else /* FUNCTIONAL_TEST_ALL_TARGETS */

		for (size_t tested_tgt_idx = 0; tested_tgt_idx < n_tested_targets; ++tested_tgt_idx)
		{
			const size_t tgt_idx = get_index_of_target(tested_targets[tested_tgt_idx]);

#endif /* FUNCTIONAL_TEST_ALL_TARGETS */

			/* Skip not supported targets */
			if (false == isTargetSupported(tgt_idx))
			{
				continue;
			}

			/* Skip invalid cases */
			if (false == isTargetSuppByFormat(format_idx, tgt_idx))
			{
				continue;
			}

			try
			{
				prepareSourceTexture(format_idx, tgt_idx, source_channel_sizes);

				/* Skip formats not supported by FBO */
				if (false == fillSourceTexture(format_idx, tgt_idx))
				{
					deinitTextures();
					continue;
				}

#if FUNCTIONAL_TEST_ALL_ACCESS_ROUTINES

				for (size_t access_idx = 0; access_idx < n_texture_access; ++access_idx)
				{
					/* Skip invalid cases */
					if (false == isTargetSuppByAccess(access_idx, tgt_idx))
					{
						continue;
					}
#else /* FUNCTIONAL_TEST_ALL_ACCESS_ROUTINES */
					/* Skip invalid cases */
					if (false == isTargetSuppByAccess(access_idx, tgt_idx))
					{
						deinitTextures();
						continue;
					}
#endif /* FUNCTIONAL_TEST_ALL_ACCESS_ROUTINES */

#if FUNCTIONAL_TEST_ALL_SWIZZLE_COMBINATIONS

					for (size_t r = 0; r < n_valid_values; ++r)
					{
						for (size_t g = 0; g < n_valid_values; ++g)
						{
							for (size_t b = 0; b < n_valid_values; ++b)
							{
								for (size_t a = 0; a < n_valid_values; ++a)
								{

#else /* FUNCTIONAL_TEST_ALL_SWIZZLE_COMBINATIONS */

				for (size_t tested_swizzle_idx = 0; tested_swizzle_idx < n_tested_swizzle_combinations;
					 ++tested_swizzle_idx)
				{
					const size_t r = tested_swizzle_combinations[tested_swizzle_idx][0];
					const size_t g = tested_swizzle_combinations[tested_swizzle_idx][1];
					const size_t b = tested_swizzle_combinations[tested_swizzle_idx][2];
					const size_t a = tested_swizzle_combinations[tested_swizzle_idx][3];

#endif /* FUNCTIONAL_TEST_ALL_SWIZZLE_COMBINATIONS */

									for (size_t channel_idx = 0; channel_idx < 4; ++channel_idx)
									{
										const testCase test_case = { channel_idx,
																	 format_idx,
																	 tgt_idx,
																	 access_idx,
																	 valid_values[r],
																	 valid_values[g],
																	 valid_values[b],
																	 valid_values[a],
																	 { source_channel_sizes[0], source_channel_sizes[1],
																	   source_channel_sizes[2],
																	   source_channel_sizes[3] } };

										executeTestCase(test_case);

										deinitOutputTexture();
									} /* iteration over channels */

#if FUNCTIONAL_TEST_ALL_SWIZZLE_COMBINATIONS

									/* iteration over swizzle combinations */
								}
							}
						}
					}

#else /* FUNCTIONAL_TEST_ALL_SWIZZLE_COMBINATIONS */

				} /* iteration over swizzle combinations */

#endif /* FUNCTIONAL_TEST_ALL_SWIZZLE_COMBINATIONS */

#if FUNCTIONAL_TEST_ALL_ACCESS_ROUTINES

				} /* iteration over access routines - only when enabled */

#endif /* FUNCTIONAL_TEST_ALL_ACCESS_ROUTINES */
				deinitTextures();
			} /* try */
			catch (wrongResults& exc)
			{
				logTestCaseDetials(exc.m_test_case);
				m_context.getTestContext().getLog() << tcu::TestLog::Message << exc.what() << tcu::TestLog::EndMessage;

				test_result = false;
				deinitTextures();
			}
			catch (...)
			{
				deinitTextures();
				throw;
			}
		} /* iteration over texture targets */

	} /* iteration over texture formats */

	/* Set result */
	if (true == test_result)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return STOP;
}

/** Fills multisampled source texture
 *
 * @param format_idx Index of format
 * @param target_idx Index of target
 *
 * @return True if operation was successful, false other wise
 **/
bool FunctionalTest::fillMSTexture(size_t format_idx, size_t target_idx)
{
	const glw::GLenum target = texture_targets[target_idx].m_target;

	/*  */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind FBO */
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_prepare_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindFramebuffer");

	/* Attach texture */
	switch (target)
	{
	case GL_TEXTURE_2D_MULTISAMPLE:

		gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, m_source_tex_id, 0 /* level */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "FramebufferTexture2D");

		break;

	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:

		gl.framebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_source_tex_id, 0 /* level */, 0 /* layer */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "FramebufferTexture2D");

		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	/* Verify status */
	const glw::GLenum status = gl.checkFramebufferStatus(GL_FRAMEBUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "CheckFramebufferStatus");

	if (GL_FRAMEBUFFER_UNSUPPORTED == status)
	{
		/* Unbind */
		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindFramebuffer");

		return false;
	}
	else if (GL_FRAMEBUFFER_COMPLETE != status)
	{
		TCU_FAIL("Framebuffer is incomplete. Format is supported");
	}

	/* Set Viewport */
	gl.viewport(0 /* x */, 0 /* y */, m_output_width, m_output_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Viewport");

	Utils::programInfo program(m_context);
	prepareProgram(format_idx, program);

	gl.useProgram(program.m_program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");

	/* Clear */
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Clear");

	/* Draw */
	gl.drawArrays(GL_TRIANGLE_STRIP, 0 /* first */, 4 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

	/* Unbind FBO */
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindFramebuffer");

	/* Done */
	return true;
}

/** Fills source texture
 *
 * @param format_idx Index of format
 * @param target_idx Index of target
 *
 * @return True if operation was successful, false other wise
 **/
bool FunctionalTest::fillSourceTexture(size_t format_idx, size_t target_idx)
{
	const glw::GLenum	  target = texture_targets[target_idx].m_target;
	const _texture_format& format = texture_formats[format_idx];

	/*  */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result */
	bool result = true;

	/* Bind texture */
	gl.bindTexture(target, m_source_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	/* Attach texture */
	switch (target)
	{
	case GL_TEXTURE_1D:
		gl.texSubImage1D(target, 0 /* level */, 0 /* x */, m_width, format.m_format, format.m_type,
						 format.m_source_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexSubImage1D");

		break;

	case GL_TEXTURE_1D_ARRAY:
	case GL_TEXTURE_2D:
	case GL_TEXTURE_RECTANGLE:
		gl.texSubImage2D(target, 0 /* level */, 0 /* x */, 0 /* y */, m_width, m_height, format.m_format, format.m_type,
						 format.m_source_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexSubImage2D");

		break;

	case GL_TEXTURE_2D_ARRAY:
	case GL_TEXTURE_3D:
		gl.texSubImage3D(target, 0 /* level */, 0 /* x */, 0 /* y */, 0 /* z */, m_width, m_height, m_depth,
						 format.m_format, format.m_type, format.m_source_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexSubImage3D");

		break;

	case GL_TEXTURE_CUBE_MAP:
		for (size_t i = 0; i < n_cube_map_faces; ++i)
		{
			gl.texSubImage2D(cube_map_faces[i], 0 /* level */, 0 /* x */, 0 /* y */, m_width, m_height, format.m_format,
							 format.m_type, format.m_source_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "TexSubImage2D");
		}

		break;

	case GL_TEXTURE_2D_MULTISAMPLE:
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		result = fillMSTexture(format_idx, target_idx);

		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	/* Unbind */
	gl.bindTexture(target, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	/* Done */
	return result;
}

/** Prepares program used to fill multisampled texture
 *
 * @param format_idx Index of texture format
 * @param program    Instance of program that will be prepared
 **/
void FunctionalTest::prepareProgram(size_t format_idx, Utils::programInfo& program)
{
	static const glw::GLchar* fs_template = "#version 330 core\n"
											"\n"
											"out PREFIXvec4 out_color;\n"
											"\n"
											"void main()\n"
											"{\n"
											"    out_color = PREFIXvec4(VALUES);\n"
											"}\n"
											"\n";

	const _texture_format& format = texture_formats[format_idx];
	const std::string&	 values = prepareValues(format_idx);
	const std::string&	 vs	 = getVertexShader(testCase(), false); /* Get blank VS */

	std::string fs		 = fs_template;
	size_t		position = 0;

	Utils::replaceToken("PREFIX", position, format.m_sampler.m_sampler_prefix, fs);
	Utils::replaceToken("PREFIX", position, format.m_sampler.m_sampler_prefix, fs);
	Utils::replaceToken("VALUES", position, values.c_str(), fs);

	program.build(fs.c_str(), vs.c_str());
}

/** Prepares hardcoded values used by program to fill multisampled textures
 *
 * @param format_idx Index of texture format
 *
 * @return Shader source
 **/
std::string FunctionalTest::prepareValues(size_t format_idx)
{
	double ch_rgba[4] = { 0.0, 0.0, 0.0, 0.0 };

	calculate_values_from_source(format_idx, ch_rgba);

	/* Prepare string */
	std::stringstream stream;
	stream << ch_rgba[0] << ", " << ch_rgba[1] << ", " << ch_rgba[2] << ", " << ch_rgba[3];

	return stream.str();
}

/** Verifies if value is in <low:top> range
 *
 * @tparam T Type oo values
 *
 * @param value Value to check
 * @param low   Lowest acceptable value
 * @param top   Highest acceptable value
 *
 * @return true if value is in range, false otherwise
 **/
template <typename T>
bool isInRange(const void* value, const void* low, const void* top)
{
	const T* v_ptr = (const T*)value;
	const T* l_ptr = (const T*)low;
	const T* t_ptr = (const T*)top;

	if ((*v_ptr > *t_ptr) || (*v_ptr < *l_ptr))
	{
		return false;
	}

	return true;
}

/** Verifies contents of output image
 *
 * @param test_case                 Test case instance
 * @param output_format_index       Index of format used by output texture
 * @parma output_channel_size       Size of storage used by output texture in bits
 * @param index_of_swizzled_channel Index of swizzled channel
 * @param data                      Image contents
 **/
void FunctionalTest::verifyOutputImage(const testCase& test_case, size_t output_format_index,
									   glw::GLint output_channel_size, size_t index_of_swizzled_channel,
									   const glw::GLubyte* data)
{
	const _texture_format& output_format = texture_formats[output_format_index];

	glw::GLubyte expected_data_low[8] = { 0 };
	glw::GLubyte expected_data_top[8] = { 0 };
	size_t		 texel_size			  = 0;

	calculate_expected_value(test_case.m_source_texture_format_index, output_format_index, index_of_swizzled_channel,
							 test_case.m_texture_sizes[index_of_swizzled_channel], output_channel_size,
							 expected_data_low, expected_data_top, texel_size);

	for (size_t i = 0; i < m_output_height * m_output_width; ++i)
	{
		const size_t	   offset  = i * texel_size;
		const glw::GLvoid* pointer = data + offset;

		bool res = false;

		switch (output_format.m_type)
		{
		case GL_BYTE:
			res = isInRange<glw::GLbyte>(pointer, expected_data_low, expected_data_top);
			break;
		case GL_UNSIGNED_BYTE:
			res = isInRange<glw::GLubyte>(pointer, expected_data_low, expected_data_top);
			break;
		case GL_SHORT:
			res = isInRange<glw::GLshort>(pointer, expected_data_low, expected_data_top);
			break;
		case GL_UNSIGNED_SHORT:
			res = isInRange<glw::GLushort>(pointer, expected_data_low, expected_data_top);
			break;
		case GL_HALF_FLOAT:
			res = isInRange<glw::GLbyte>(pointer, expected_data_low, expected_data_top);
			break;
		case GL_INT:
			res = isInRange<glw::GLint>(pointer, expected_data_low, expected_data_top);
			break;
		case GL_UNSIGNED_INT:
			res = isInRange<glw::GLhalf>(pointer, expected_data_low, expected_data_top);
			break;
		case GL_FLOAT:
			res = isInRange<glw::GLfloat>(pointer, expected_data_low, expected_data_top);
			break;
		default:
			TCU_FAIL("Invalid enum");
			break;
		}

		if (false == res)
		{
			throw wrongResults(test_case);
		}
	}
}
}

/** Constructor.
 *
 *  @param context Rendering context.
 **/
TextureSwizzleTests::TextureSwizzleTests(deqp::Context& context)
	: TestCaseGroup(context, "texture_swizzle", "Verifies \"texture_swizzle\" functionality")
{
	/* Left blank on purpose */
}

/** Initializes a texture_storage_multisample test group.
 *
 **/
void TextureSwizzleTests::init(void)
{
	addChild(new TextureSwizzle::APIErrorsTest(m_context));
	addChild(new TextureSwizzle::IntialStateTest(m_context));
	addChild(new TextureSwizzle::SmokeTest(m_context));
	addChild(new TextureSwizzle::FunctionalTest(m_context));
}
} /* glcts namespace */
