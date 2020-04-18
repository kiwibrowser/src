/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */ /*!
 * \file  glcTextureRepeatModeTests.cpp
 * \brief
 */ /*-------------------------------------------------------------------*/

#include "glcTextureRepeatModeTests.hpp"
#include "deMath.h"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluShaderProgram.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"

using namespace glw;

namespace glcts
{

union InternalFormatBits {
	struct Bits
	{
		int red;	   /* red bits */
		int green;	 /* green bits */
		int blue;	  /* blue bits */
		int alpha;	 /* alpha bits */
		int intensity; /* intensity bits */
		int luminance; /* luminance bits */
		int depth;	 /* depth bits */
		int stencil;   /* stencil bits */
		int exponent;  /* shared exponent bits */
	} bits;
	int array[9]; /* all the bits */
};

enum InternalFormatSamplerType
{
	SAMPLER_UNORM = 0, /* unsigned normalized */
	SAMPLER_NORM,	  /* normalized */
	SAMPLER_UINT,	  /* unsigned integer */
	SAMPLER_INT,	   /* integer */
	SAMPLER_FLOAT	  /* floating-point */
};

enum InternalFormatFlag
{
	NO_FLAG			  = 0,
	FLAG_PACKED		  = 1,									   /* Packed pixel format. */
	FLAG_COMPRESSED   = 2,									   /* Compressed format. */
	FLAG_REQ_RBO_GL42 = 4,									   /* Required RBO & tex format in OpenGL 4.2. */
	FLAG_REQ_RBO_ES30 = 8,									   /* Required RBO & tex format in OpenGL ES 3.0. */
	FLAG_REQ_RBO	  = FLAG_REQ_RBO_GL42 | FLAG_REQ_RBO_ES30, /* Required RBO & tex format in both. */
};

#define MAX_PIXEL_SIZE 16

/* Note that internal representation is in little endian - tests will fail on big endian, in particular RGB565 will fail */
struct FormatInfo
{
	/* internal format, indicating tested format */
	GLenum internalformat;

	const char* name;

	/* number of bytes per pixel */
	GLsizei pixelSize;

	/* RGBW colors' representation, specific for each internalformat */
	GLubyte internalred[MAX_PIXEL_SIZE];
	GLubyte internalgreen[MAX_PIXEL_SIZE];
	GLubyte internalblue[MAX_PIXEL_SIZE];
	GLubyte internalwhite[MAX_PIXEL_SIZE];

	/* RGBW colors' mapped to RGBA, that are read from framebuffer */
	GLubyte RGBAred[4];
	GLubyte RGBAgreen[4];
	GLubyte RGBAblue[4];
	GLubyte RGBAwhite[4];
};

static const FormatInfo testedFormats[] = {
	{
		GL_R8,
		"r8",
		1,
		{ 0xFF },
		{ 0xC7 },
		{ 0x30 },
		{ 0x00 },
		/* expected values */
		{ 0xFF, 0x00, 0x00, 0xFF },
		{ 0xC7, 0x00, 0x00, 0xFF },
		{ 0x30, 0x00, 0x00, 0xFF },
		{ 0x00, 0x00, 0x00, 0xFF },
	},
	{
		GL_RGB565,
		"rgb565",
		2,
		{ 0x00, 0xF8 },
		{ 0xE0, 0x07 },
		{ 0x1F, 0x00 },
		{ 0xFF, 0xFF },
		/* expected values */
		{ 0xFF, 0x00, 0x00, 0xFF },
		{ 0x00, 0xFF, 0x00, 0xFF },
		{ 0x00, 0x00, 0xFF, 0xFF },
		{ 0xFF, 0xFF, 0xFF, 0xFF },
	},
	{
		GL_RGB8,
		"rgb8",
		3,
		{ 0xFF, 0x00, 0x00 },
		{ 0x00, 0xFF, 0x00 },
		{ 0x00, 0x00, 0xFF },
		{ 0xFF, 0xFF, 0xFF },
		/* expected values */
		{ 0xFF, 0x00, 0x00, 0xFF },
		{ 0x00, 0xFF, 0x00, 0xFF },
		{ 0x00, 0x00, 0xFF, 0xFF },
		{ 0xFF, 0xFF, 0xFF, 0xFF },
	},
	{
		GL_RGB10_A2,
		"rgb10_a2",
		4,
		{ 0xFF, 0x03, 0x00, 0xC0 },
		{ 0x00, 0xFC, 0x0F, 0xC0 },
		{ 0x00, 0x00, 0xF0, 0xFF },
		{ 0xFF, 0xFF, 0xFF, 0xFF },
		/* expected values */
		{ 0xFF, 0x00, 0x00, 0xFF },
		{ 0x00, 0xFF, 0x00, 0xFF },
		{ 0x00, 0x00, 0xFF, 0xFF },
		{ 0xFF, 0xFF, 0xFF, 0xFF },
	},
	{
		/* unsigned integer texture formats : require an unsigned texture sampler in the fragment shader */
		GL_R32UI,
		"r32ui",
		4,
		{ 0xFF, 0x00, 0x00, 0x00 },
		{ 0x51, 0x00, 0x00, 0x00 },
		{ 0xF3, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		/* expected values */
		{ 0xFF, 0x00, 0x00, 0xFF },
		{ 0x51, 0x00, 0x00, 0xFF },
		{ 0xF3, 0x00, 0x00, 0xFF },
		{ 0x00, 0x00, 0x00, 0xFF },
	},
	{
		GL_RG32UI,
		"rg32ui",
		8,
		{ 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
		/* expected values */
		{ 0xFF, 0x00, 0x00, 0xFF },
		{ 0x00, 0xFF, 0x00, 0xFF },
		{ 0x00, 0x00, 0x00, 0xFF },
		{ 0xFF, 0xFF, 0x00, 0xFF },
	},
	{
		GL_RGBA32UI,
		"rgba32ui",
		16,
		{ 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF },
		{ 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
		{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
		/* expected values */
		{ 0xFF, 0x00, 0x00, 0xFF },
		{ 0x00, 0xFF, 0x00, 0xFF },
		{ 0x00, 0x00, 0xFF, 0xFF },
		{ 0xFF, 0xFF, 0xFF, 0xFF },
	},
	{
		/* DEPTH formats are tested by comparing with reference values hard-coded in shaders */
		GL_DEPTH_COMPONENT16,
		"depth_component16",
		2,
		{ 0xFF, 0xE8 }, /* ~ 0.91 */
		{ 0xFF, 0xAB }, /* ~ 0.67 */
		{ 0x00, 0x78 }, /* ~ 0.46 */
		{ 0x00, 0x3C }, /* ~ 0.23 */
		/* expected values */
		{ 0x00, 0x00, 0x00, 0xff },
		{ 0x00, 0x00, 0xff, 0xff },
		{ 0x00, 0xff, 0xff, 0xff },
		{ 0xff, 0xff, 0xff, 0xff },
	},
	{
		/* little-endian order, so the bytes are in reverse order: stencil first, then depth */
		GL_DEPTH24_STENCIL8,
		"depth24_stencil8",
		4,
		{ 0x00, 0x00, 0xE8, 0xFF }, /* ~ 0.99 */
		{ 0x88, 0x00, 0xAB, 0xAF }, /* ~ 0.68 */
		{ 0xBB, 0x28, 0x55, 0x7F }, /* ~ 0.49 */
		{ 0xFF, 0x78, 0x80, 0x02 }, /* ~ 0.01 */
		/* expected values */
		{ 0x00, 0x00, 0x00, 0xff },
		{ 0x00, 0x00, 0xff, 0xff },
		{ 0x00, 0xff, 0xff, 0xff },
		{ 0xff, 0xff, 0xff, 0xff },
	}
};

struct InternalFormat
{
	GLenum					  sizedFormat;
	GLenum					  baseFormat;
	GLenum					  format;
	GLenum					  type;
	InternalFormatSamplerType sampler;
	InternalFormatBits		  bits;
	GLuint					  flags; // InternalFormatFlag
};

static const InternalFormat glInternalFormats[] =
{
	{ GL_DEPTH_COMPONENT,	GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT,		SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 0,16, 0, NO_FLAG } }, NO_FLAG },
	{ GL_DEPTH_STENCIL,		GL_DEPTH_STENCIL,   GL_DEPTH_STENCIL,   GL_UNSIGNED_INT,		SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 0,24, 8, NO_FLAG } }, NO_FLAG },

	{ GL_RED,				GL_RED,			GL_RED,				GL_UNSIGNED_BYTE,			SAMPLER_UNORM,  { { 8, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RG,				GL_RG,			GL_RG,				GL_UNSIGNED_BYTE,			SAMPLER_UNORM,  { { 8, 8, 0, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },

	// Table 3.12
	{ GL_R8,				GL_RED,			GL_RED,				GL_UNSIGNED_BYTE,			SAMPLER_UNORM,  { { 8, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_R8_SNORM,			GL_RED,			GL_RED,				GL_BYTE,					SAMPLER_NORM,   { { 8, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_R16,				GL_RED,			GL_RED,				GL_UNSIGNED_SHORT,			SAMPLER_UNORM,  { {16, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO_GL42 },
	{ GL_R16_SNORM,			GL_RED,			GL_RED,				GL_SHORT,					SAMPLER_NORM,   { {16, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RG8,				GL_RG,			GL_RG,				GL_UNSIGNED_BYTE,			SAMPLER_UNORM,  { { 8, 8, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RG8_SNORM,			GL_RG,			GL_RG,				GL_BYTE,					SAMPLER_NORM,   { { 8, 8, 0, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RG16,				GL_RG,			GL_RG,				GL_UNSIGNED_SHORT,			SAMPLER_UNORM,  { {16,16, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO_GL42 },
	{ GL_RG16_SNORM,		GL_RG,			GL_RG,				GL_SHORT,					SAMPLER_NORM,   { {16,16, 0, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_R3_G3_B2,			GL_RGB,			GL_RGB,				GL_UNSIGNED_BYTE_3_3_2,		SAMPLER_UNORM,  { { 3, 3, 2, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_PACKED },
	{ GL_RGB4,				GL_RGB,			GL_RGB,				GL_UNSIGNED_BYTE,			SAMPLER_UNORM,  { { 4, 4, 4, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGB5,				GL_RGB,			GL_RGB,				GL_UNSIGNED_BYTE,			SAMPLER_UNORM,  { { 5, 5, 5, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGB565,			GL_RGB,			GL_RGB,				GL_UNSIGNED_SHORT_5_6_5,	SAMPLER_UNORM,  { { 5, 6, 5, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_PACKED|FLAG_REQ_RBO },
	{ GL_RGB8,				GL_RGB,			GL_RGB,				GL_UNSIGNED_BYTE,			SAMPLER_UNORM,  { { 8, 8, 8, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO_ES30 },
	{ GL_RGB8_SNORM,		GL_RGB,			GL_RGB,				GL_BYTE,					SAMPLER_NORM,   { { 8, 8, 8, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGB10,				GL_RGB,			GL_RGB,				GL_UNSIGNED_SHORT,			SAMPLER_UNORM,  { {10,10,10, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGB12,				GL_RGB,			GL_RGB,				GL_UNSIGNED_SHORT,			SAMPLER_UNORM,  { {12,12,12, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGB16,				GL_RGB,			GL_RGB,				GL_UNSIGNED_SHORT,			SAMPLER_UNORM,  { {16,16,16, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGB16_SNORM,		GL_RGB,			GL_RGB,				GL_SHORT,					SAMPLER_NORM,   { {16,16,16, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGBA2,				GL_RGBA,		GL_RGBA,			GL_UNSIGNED_BYTE,			SAMPLER_UNORM,  { { 2, 2, 2, 2, 0, 0, 0, 0, NO_FLAG } }, 0 },

	{ GL_RGBA4,				GL_RGBA,		GL_RGBA,			GL_UNSIGNED_SHORT_4_4_4_4,	SAMPLER_UNORM,  { { 4, 4, 4, 4, 0, 0, 0, 0, NO_FLAG } }, FLAG_PACKED|FLAG_REQ_RBO },
	{ GL_RGB5_A1,			GL_RGBA,		GL_RGBA,			GL_UNSIGNED_SHORT_5_5_5_1,	SAMPLER_UNORM,  { { 5, 5, 5, 1, 0, 0, 0, 0, NO_FLAG } }, FLAG_PACKED|FLAG_REQ_RBO },

	{ GL_RGBA8,				GL_RGBA,		GL_RGBA,			GL_UNSIGNED_BYTE,			SAMPLER_UNORM,  { { 8, 8, 8, 8, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RGBA8_SNORM,		GL_RGBA,		GL_RGBA,			GL_BYTE,					SAMPLER_NORM,   { { 8, 8, 8, 8, 0, 0, 0, 0, NO_FLAG } }, 0 },

	{ GL_RGB10_A2,			GL_RGBA,		GL_RGBA,			GL_UNSIGNED_INT_2_10_10_10_REV, SAMPLER_UNORM,  { {10,10,10, 2, 0, 0, 0, 0, NO_FLAG } }, FLAG_PACKED|FLAG_REQ_RBO_GL42 },
	{ GL_RGB10_A2UI,		GL_RGBA,		GL_RGBA_INTEGER,	GL_UNSIGNED_INT_2_10_10_10_REV, SAMPLER_UINT,   { {10,10,10, 2, 0, 0, 0, 0, NO_FLAG } }, FLAG_PACKED|FLAG_REQ_RBO_GL42 },

	{ GL_RGBA12,			GL_RGBA,		GL_RGBA,			GL_UNSIGNED_SHORT,			SAMPLER_UNORM,  { {12,12,12,12, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGBA16,			GL_RGBA,		GL_RGBA,			GL_UNSIGNED_SHORT,			SAMPLER_UNORM,  { {16,16,16,16, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO_GL42 },
	{ GL_RGBA16_SNORM,		GL_RGBA,		GL_RGBA,			GL_SHORT,					SAMPLER_NORM,   { {16,16,16,16, 0, 0, 0, 0, NO_FLAG } }, 0 },

	{ GL_SRGB8,				GL_RGB,			GL_RGB,				GL_UNSIGNED_BYTE,			SAMPLER_UNORM,  { { 8, 8, 8, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_SRGB8_ALPHA8,		GL_RGBA,		GL_RGBA,			GL_UNSIGNED_BYTE,			SAMPLER_UNORM,  { { 8, 8, 8, 8, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },

	{ GL_R16F,				GL_RED,		GL_RED,		GL_HALF_FLOAT,					 SAMPLER_FLOAT,  { {16, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO_GL42 },
	{ GL_RG16F,				GL_RG,		GL_RG,		GL_HALF_FLOAT,					 SAMPLER_FLOAT,  { {16,16, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO_GL42 },
	{ GL_RGB16F,			GL_RGB,		GL_RGB,		GL_HALF_FLOAT,					 SAMPLER_FLOAT,  { {16,16,16, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGBA16F,			GL_RGBA,	GL_RGBA,	GL_HALF_FLOAT,					 SAMPLER_FLOAT,  { {16,16,16,16, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO_GL42 },
	{ GL_R32F,				GL_RED,		GL_RED,		GL_FLOAT,						 SAMPLER_FLOAT,  { {32, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO_GL42 },
	{ GL_RG32F,				GL_RG,		GL_RG,		GL_FLOAT,						 SAMPLER_FLOAT,  { {32,32, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO_GL42 },
	{ GL_RGB32F,			GL_RGB,		GL_RGB,		GL_FLOAT,						 SAMPLER_FLOAT,  { {32,32,32, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGBA32F,			GL_RGBA,	GL_RGBA,	GL_FLOAT,						 SAMPLER_FLOAT,  { {32,32,32,32, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO_GL42 },
	{ GL_R11F_G11F_B10F,	GL_RGB,		GL_RGB,		GL_UNSIGNED_INT_10F_11F_11F_REV, SAMPLER_FLOAT,  { {11,11,10, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_PACKED|FLAG_REQ_RBO_GL42 },

	{ GL_RGB9_E5,			GL_RGB,		GL_RGB,		GL_UNSIGNED_INT_5_9_9_9_REV,	 SAMPLER_FLOAT,  { { 9, 9, 9, 0, 0, 0, 0, 0, 5 } }, FLAG_PACKED },

	{ GL_R8I,		GL_RED,		GL_RED_INTEGER,	 GL_BYTE,			SAMPLER_INT,	{ { 8, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_R8UI,		GL_RED,		GL_RED_INTEGER,	 GL_UNSIGNED_BYTE,	SAMPLER_UINT,   { { 8, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_R16I,		GL_RED,		GL_RED_INTEGER,	 GL_SHORT,			SAMPLER_INT,	{ {16, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_R16UI,		GL_RED,		GL_RED_INTEGER,	 GL_UNSIGNED_SHORT,	SAMPLER_UINT,   { {16, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_R32I,		GL_RED,		GL_RED_INTEGER,	 GL_INT,			SAMPLER_INT,	{ {32, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_R32UI,		GL_RED,		GL_RED_INTEGER,	 GL_UNSIGNED_INT,	SAMPLER_UINT,   { {32, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RG8I,		GL_RG,		GL_RG_INTEGER,	 GL_BYTE,			SAMPLER_INT,	{ { 8, 8, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RG8UI,		GL_RG,		GL_RG_INTEGER,	 GL_UNSIGNED_BYTE,	SAMPLER_UINT,   { { 8, 8, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RG16I,		GL_RG,		GL_RG_INTEGER,	 GL_SHORT,			SAMPLER_INT,	{ {16,16, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RG16UI,	GL_RG,		GL_RG_INTEGER,	 GL_UNSIGNED_SHORT,	SAMPLER_UINT,   { {16,16, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RG32I,		GL_RG,		GL_RG_INTEGER,	 GL_INT,			SAMPLER_INT,	{ {32,32, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RG32UI,	GL_RG,		GL_RG_INTEGER,	 GL_UNSIGNED_INT,	SAMPLER_UINT,   { {32,32, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RGB8I,		GL_RGB,		GL_RGB_INTEGER,	 GL_BYTE,			SAMPLER_INT,	{ { 8, 8, 8, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGB8UI,	GL_RGB,		GL_RGB_INTEGER,	 GL_UNSIGNED_BYTE,	SAMPLER_UINT,   { { 8, 8, 8, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGB16I,	GL_RGB,		GL_RGB_INTEGER,	 GL_SHORT,			SAMPLER_INT,	{ {16,16,16, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGB16UI,	GL_RGB,		GL_RGB_INTEGER,	 GL_UNSIGNED_SHORT,	SAMPLER_UINT,   { {16,16,16, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGB32I,	GL_RGB,		GL_RGB_INTEGER,	 GL_INT,			SAMPLER_INT,	{ {32,32,32, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGB32UI,	GL_RGB,		GL_RGB_INTEGER,	 GL_UNSIGNED_INT,	SAMPLER_UINT,   { {32,32,32, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGBA8I,	GL_RGBA,	GL_RGBA_INTEGER, GL_BYTE,			SAMPLER_INT,	{ { 8, 8, 8, 8, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RGBA8UI,	GL_RGBA,	GL_RGBA_INTEGER, GL_UNSIGNED_BYTE,	SAMPLER_UINT,   { { 8, 8, 8, 8, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RGBA16I,	GL_RGBA,	GL_RGBA_INTEGER, GL_SHORT,			SAMPLER_INT,	{ {16,16,16,16, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RGBA16UI,	GL_RGBA,	GL_RGBA_INTEGER, GL_UNSIGNED_SHORT,	SAMPLER_UINT,   { {16,16,16,16, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RGBA32I,	GL_RGBA,	GL_RGBA_INTEGER, GL_INT,			SAMPLER_INT,	{ {32,32,32,32, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RGBA32UI,	GL_RGBA,	GL_RGBA_INTEGER, GL_UNSIGNED_INT,	SAMPLER_UINT,   { {32,32,32,32, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },

	// Table 3.13
	{ GL_DEPTH_COMPONENT16,	 GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT,					SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 0,16, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_DEPTH_COMPONENT24,	 GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,					SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 0,24, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_DEPTH_COMPONENT32,	 GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,					SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 0,32, 0, NO_FLAG } }, 0 },
	{ GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT,							SAMPLER_FLOAT,  { { 0, 0, 0, 0, 0, 0,32, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_DEPTH24_STENCIL8,	 GL_DEPTH_STENCIL,   GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8,				SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 0,24, 8, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_DEPTH32F_STENCIL8,	 GL_DEPTH_STENCIL,   GL_DEPTH_STENCIL,   GL_FLOAT_32_UNSIGNED_INT_24_8_REV, SAMPLER_FLOAT,  { { 0, 0, 0, 0, 0, 0,32, 8, NO_FLAG } }, FLAG_PACKED|FLAG_REQ_RBO },

	// Table 3.14
	{ GL_COMPRESSED_RED,			  GL_RED,	GL_RED,	  GL_UNSIGNED_BYTE,   SAMPLER_UNORM,  { { 8, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_COMPRESSED },
	{ GL_COMPRESSED_RG,				  GL_RG,	GL_RG,	  GL_UNSIGNED_BYTE,   SAMPLER_UNORM,  { { 8, 8, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_COMPRESSED },
	{ GL_COMPRESSED_RGB,			  GL_RGB,	GL_RGB,	  GL_UNSIGNED_BYTE,   SAMPLER_UNORM,  { { 8, 8, 8, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_COMPRESSED },
	{ GL_COMPRESSED_RGBA,			  GL_RGBA,	GL_RGBA,  GL_UNSIGNED_BYTE,   SAMPLER_UNORM,  { { 8, 8, 8, 8, 0, 0, 0, 0, NO_FLAG } }, FLAG_COMPRESSED },
	{ GL_COMPRESSED_SRGB,			  GL_RGB,	GL_RGB,	  GL_UNSIGNED_BYTE,   SAMPLER_UNORM,  { { 8, 8, 8, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_COMPRESSED },
	{ GL_COMPRESSED_SRGB_ALPHA,		  GL_RGBA,	GL_RGBA,  GL_UNSIGNED_BYTE,   SAMPLER_UNORM,  { { 8, 8, 8, 8, 0, 0, 0, 0, NO_FLAG } }, FLAG_COMPRESSED },
	{ GL_COMPRESSED_RED_RGTC1,		  GL_RED,	GL_RED,	  GL_UNSIGNED_BYTE,   SAMPLER_UNORM,  { { 8, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_COMPRESSED },
	{ GL_COMPRESSED_SIGNED_RED_RGTC1, GL_RED,	GL_RED,	  GL_BYTE,			  SAMPLER_NORM,   { { 8, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_COMPRESSED },
	{ GL_COMPRESSED_RG_RGTC2,		  GL_RG,	GL_RG,	  GL_UNSIGNED_BYTE,   SAMPLER_UNORM,  { { 8, 8, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_COMPRESSED },
	{ GL_COMPRESSED_SIGNED_RG_RGTC2,  GL_RG,	GL_RG,	  GL_BYTE,			  SAMPLER_NORM,   { { 8, 8, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_COMPRESSED },
};

static const InternalFormat esInternalFormats[] =
{
	// Table 3.3
	{ GL_LUMINANCE,			GL_LUMINANCE,		GL_LUMINANCE,		GL_UNSIGNED_BYTE,	SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 8, 0, 0, NO_FLAG } }, 0 },
	{ GL_ALPHA,				GL_ALPHA,			GL_ALPHA,			GL_UNSIGNED_BYTE,	SAMPLER_UNORM,  { { 0, 0, 0, 8, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_LUMINANCE_ALPHA,	GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,	SAMPLER_UNORM,  { { 0, 0, 0, 8, 0, 8, 0, 0, NO_FLAG } }, 0 },

	{ GL_RGB,				GL_RGB,				GL_RGB,				GL_UNSIGNED_BYTE,	SAMPLER_UNORM,  { { 8, 8, 8, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGBA,				GL_RGBA,			GL_RGBA,			GL_UNSIGNED_BYTE,	SAMPLER_UNORM,  { { 8, 8, 8, 8, 0, 0, 0, 0, NO_FLAG } }, 0 },

	// Table 3.12
	{ GL_R8,				GL_RED,			GL_RED,			GL_UNSIGNED_BYTE,			SAMPLER_UNORM,  { { 8, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_R8_SNORM,			GL_RED,			GL_RED,			GL_BYTE,					SAMPLER_NORM,   { { 8, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RG8,				GL_RG,			GL_RG,			GL_UNSIGNED_BYTE,			SAMPLER_UNORM,  { { 8, 8, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RG8_SNORM,			GL_RG,			GL_RG,			GL_BYTE,					SAMPLER_NORM,   { { 8, 8, 0, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGB8,				GL_RGB,			GL_RGB,			GL_UNSIGNED_BYTE,			SAMPLER_UNORM,  { { 8, 8, 8, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO_ES30 },
	{ GL_RGB8_SNORM,		GL_RGB,			GL_RGB,			GL_BYTE,					SAMPLER_NORM,   { { 8, 8, 8, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGB565,			GL_RGB,			GL_RGB,			GL_UNSIGNED_SHORT_5_6_5,	SAMPLER_UNORM,  { { 5, 6, 5, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_PACKED|FLAG_REQ_RBO },

	{ GL_RGBA4,				GL_RGBA,		GL_RGBA,		GL_UNSIGNED_SHORT_4_4_4_4,  SAMPLER_UNORM,  { { 4, 4, 4, 4, 0, 0, 0, 0, NO_FLAG } }, FLAG_PACKED|FLAG_REQ_RBO },
	{ GL_RGB5_A1,			GL_RGBA,		GL_RGBA,		GL_UNSIGNED_SHORT_5_5_5_1,  SAMPLER_UNORM,  { { 5, 5, 5, 1, 0, 0, 0, 0, NO_FLAG } }, FLAG_PACKED|FLAG_REQ_RBO },

	{ GL_RGBA8,				GL_RGBA,		GL_RGBA,		GL_UNSIGNED_BYTE,			SAMPLER_UNORM,  { { 8, 8, 8, 8, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RGBA8_SNORM,		GL_RGBA,		GL_RGBA,		GL_BYTE,					SAMPLER_NORM,   { { 8, 8, 8, 8, 0, 0, 0, 0, NO_FLAG } }, 0 },

	{ GL_RGB10_A2,			GL_RGBA,		GL_RGBA,		 GL_UNSIGNED_INT_2_10_10_10_REV, SAMPLER_UNORM,  { {10,10,10, 2, 0, 0, 0, 0, NO_FLAG } }, FLAG_PACKED|FLAG_REQ_RBO_ES30 },
	{ GL_RGB10_A2UI,		GL_RGBA,		GL_RGBA_INTEGER, GL_UNSIGNED_INT_2_10_10_10_REV, SAMPLER_UINT,   { {10,10,10, 2, 0, 0, 0, 0, NO_FLAG } }, FLAG_PACKED|FLAG_REQ_RBO_ES30 },

	{ GL_SRGB8,				GL_RGB,			GL_RGB,			GL_UNSIGNED_BYTE,			SAMPLER_UNORM,  { { 8, 8, 8, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_SRGB8_ALPHA8,		GL_RGBA,		GL_RGBA,		GL_UNSIGNED_BYTE,			SAMPLER_UNORM,  { { 8, 8, 8, 8, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },

	{ GL_R16F,				GL_RED,			GL_RED,			GL_HALF_FLOAT,					 SAMPLER_FLOAT,  { {16, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO_GL42 },
	{ GL_RG16F,				GL_RG,			GL_RG,			GL_HALF_FLOAT,					 SAMPLER_FLOAT,  { {16,16, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO_GL42 },
	{ GL_RGB16F,			GL_RGB,			GL_RGB,			GL_HALF_FLOAT,					 SAMPLER_FLOAT,  { {16,16,16, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGBA16F,			GL_RGBA,		GL_RGBA,		GL_HALF_FLOAT,					 SAMPLER_FLOAT,  { {16,16,16,16, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO_GL42 },
	{ GL_R32F,				GL_RED,			GL_RED,			GL_FLOAT,						 SAMPLER_FLOAT,  { {32, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO_GL42 },
	{ GL_RG32F,				GL_RG,			GL_RG,			GL_FLOAT,						 SAMPLER_FLOAT,  { {32,32, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO_GL42 },
	{ GL_RGB32F,			GL_RGB,			GL_RGB,			GL_FLOAT,						 SAMPLER_FLOAT,  { {32,32,32, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGBA32F,			GL_RGBA,		GL_RGBA,		GL_FLOAT,						 SAMPLER_FLOAT,  { {32,32,32,32, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO_GL42 },
	{ GL_R11F_G11F_B10F,	GL_RGB,			GL_RGB,			GL_UNSIGNED_INT_10F_11F_11F_REV, SAMPLER_FLOAT,  { {11,11,10, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_PACKED|FLAG_REQ_RBO_GL42 },

	{ GL_RGB9_E5,			GL_RGB,			GL_RGB,			GL_UNSIGNED_INT_5_9_9_9_REV,	SAMPLER_FLOAT,  { { 9, 9, 9, 0, 0, 0, 0, 0, 5 } }, FLAG_PACKED },

	{ GL_R8I,				GL_RED,			GL_RED_INTEGER,		GL_BYTE,					SAMPLER_INT,	{ { 8, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_R8UI,				GL_RED,			GL_RED_INTEGER,		GL_UNSIGNED_BYTE,			SAMPLER_UINT,   { { 8, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_R16I,				GL_RED,			GL_RED_INTEGER,		GL_SHORT,					SAMPLER_INT,	{ {16, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_R16UI,				GL_RED,			GL_RED_INTEGER,		GL_UNSIGNED_SHORT,			SAMPLER_UINT,   { {16, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_R32I,				GL_RED,			GL_RED_INTEGER,		GL_INT,						SAMPLER_INT,	{ {32, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_R32UI,				GL_RED,			GL_RED_INTEGER,		GL_UNSIGNED_INT,			SAMPLER_UINT,   { {32, 0, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RG8I,				GL_RG,			GL_RG_INTEGER,		GL_BYTE,					SAMPLER_INT,	{ { 8, 8, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RG8UI,				GL_RG,			GL_RG_INTEGER,		GL_UNSIGNED_BYTE,			SAMPLER_UINT,   { { 8, 8, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RG16I,				GL_RG,			GL_RG_INTEGER,		GL_SHORT,					SAMPLER_INT,	{ {16,16, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RG16UI,			GL_RG,			GL_RG_INTEGER,		GL_UNSIGNED_SHORT,			SAMPLER_UINT,   { {16,16, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RG32I,				GL_RG,			GL_RG_INTEGER,		GL_INT,						SAMPLER_INT,	{ {32,32, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RG32UI,			GL_RG,			GL_RG_INTEGER,		GL_UNSIGNED_INT,			SAMPLER_UINT,   { {32,32, 0, 0, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RGB8I,				GL_RGB,			GL_RGB_INTEGER,		GL_BYTE,					SAMPLER_INT,	{ { 8, 8, 8, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGB8UI,			GL_RGB,			GL_RGB_INTEGER,		GL_UNSIGNED_BYTE,			SAMPLER_UINT,   { { 8, 8, 8, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGB16I,			GL_RGB,			GL_RGB_INTEGER,		GL_SHORT,					SAMPLER_INT,	{ {16,16,16, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGB16UI,			GL_RGB,			GL_RGB_INTEGER,		GL_UNSIGNED_SHORT,			SAMPLER_UINT,   { {16,16,16, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGB32I,			GL_RGB,			GL_RGB_INTEGER,		GL_INT,						SAMPLER_INT,	{ {32,32,32, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGB32UI,			GL_RGB,			GL_RGB_INTEGER,		GL_UNSIGNED_INT,			SAMPLER_UINT,   { {32,32,32, 0, 0, 0, 0, 0, NO_FLAG } }, 0 },
	{ GL_RGBA8I,			GL_RGBA,		GL_RGBA_INTEGER,	GL_BYTE,					SAMPLER_INT,	{ { 8, 8, 8, 8, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RGBA8UI,			GL_RGBA,		GL_RGBA_INTEGER,	GL_UNSIGNED_BYTE,			SAMPLER_UINT,   { { 8, 8, 8, 8, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RGBA16I,			GL_RGBA,		GL_RGBA_INTEGER,	GL_SHORT,					SAMPLER_INT,	{ {16,16,16,16, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RGBA16UI,			GL_RGBA,		GL_RGBA_INTEGER,	GL_UNSIGNED_SHORT,			SAMPLER_UINT,   { {16,16,16,16, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RGBA32I,			GL_RGBA,		GL_RGBA_INTEGER,	GL_INT,						SAMPLER_INT,	{ {32,32,32,32, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_RGBA32UI,			GL_RGBA,		GL_RGBA_INTEGER,	GL_UNSIGNED_INT,			SAMPLER_UINT,   { {32,32,32,32, 0, 0, 0, 0, NO_FLAG } }, FLAG_REQ_RBO },

	// Table 3.13
	{ GL_DEPTH_COMPONENT16,	 GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT,					SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 0,16, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_DEPTH_COMPONENT24,	 GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,					SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 0,24, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT,							SAMPLER_FLOAT,  { { 0, 0, 0, 0, 0, 0,32, 0, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_DEPTH24_STENCIL8,	 GL_DEPTH_STENCIL,   GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8,				SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 0,24, 8, NO_FLAG } }, FLAG_REQ_RBO },
	{ GL_DEPTH32F_STENCIL8,	 GL_DEPTH_STENCIL,   GL_DEPTH_STENCIL,   GL_FLOAT_32_UNSIGNED_INT_24_8_REV, SAMPLER_FLOAT,  { { 0, 0, 0, 0, 0, 0,32, 8, NO_FLAG } }, FLAG_PACKED|FLAG_REQ_RBO },
};

const char* basic_vs = "precision mediump float;\n"
					   "out vec2 texCoord;\n"
					   "void main(void)\n"
					   "{\n"
					   "	switch(gl_VertexID)\n"
					   "	{\n"
					   "		case 0:\n"
					   "			gl_Position = vec4( 1.0,-1.0, 0.0, 1.0);\n"
					   "			texCoord = vec2(2.0, -1.0);\n"
					   "			break;\n"
					   "		case 1:\n"
					   "			gl_Position = vec4( 1.0, 1.0, 0.0, 1.0);\n"
					   "			texCoord = vec2(2.0, 2.0);\n"
					   "			break;\n"
					   "		case 2:\n"
					   "			gl_Position = vec4(-1.0,-1.0, 0.0, 1.0);\n"
					   "			texCoord = vec2(-1.0, -1.0);\n"
					   "			break;\n"
					   "		case 3:\n"
					   "			gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);\n"
					   "			texCoord = vec2(-1.0, 2.0);\n"
					   "			break;\n"
					   "	}\n"
					   "}";

const char* basic_fs = "precision mediump float;\n"
					   "uniform sampler2D texture0;\n"
					   "in vec2 texCoord;\n"
					   "out vec4 color;\n"
					   "void main(void)\n"
					   "{\n"
					   "	color = texture(texture0, texCoord);\n"
					   "}";

const char* shadow_fs = "precision mediump float;\n"
						"uniform mediump sampler2DShadow texture0;\n"
						"in  vec2 texCoord;\n"
						"out vec4 color;\n"
						"void main(void)\n"
						"{\n"
						"	float r = texture(texture0, vec3(texCoord.xy, 0.3));\n"
						"	float g = texture(texture0, vec3(texCoord.xy, 0.5));\n"
						"	float b = texture(texture0, vec3(texCoord.xy, 0.8));\n"
						"	color = vec4(r, g, b, 1.0);\n"
						"}\n";

const char* integer_fs = "precision mediump float;\n"
						 "uniform mediump usampler2D texture0;\n"
						 "in vec2 texCoord;\n"
						 "out vec4 color;\n"
						 "void main(void)\n"
						 "{\n"
						 "	highp uvec4 ci = texture(texture0, texCoord);\n"
						 "	color = vec4(ci) / 255.0; // we are using an integer texture format - so convert to float\n"
						 "	if (ci.a > 0u)\n"
						 "		color.a = 1.0;\n"
						 "	else\n"
						 "		color.a = 0.0;\n"
						 "}";

struct TestArea
{
	GLsizei left;
	GLsizei right;
	GLsizei top;
	GLsizei bottom;
};

class TestClampModeForInternalFormat : public deqp::TestCase
{
public:
	/* Public methods */
	TestClampModeForInternalFormat(deqp::Context& context, const std::string& name, GLenum internalFormat,
								   GLenum clampMode, GLint lodLevel, GLsizei width, GLsizei height);
	virtual ~TestClampModeForInternalFormat()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

private:
	/* Private methods */
	const FormatInfo* findFormat(GLenum internalformat) const;
	const InternalFormat& findInternalFormat(GLenum internalformat) const;
	void clearTextures(GLenum target, GLsizei width, GLsizei height, GLint lod, GLenum internalformat, GLenum type,
					   GLenum format) const;
	void fillTextureWithColor(GLubyte* texture_data, GLsizei tex_width, GLsizei tex_height,
							  GLenum internalformat) const;
	void calcTextureEpsilon(const GLsizei textureBits[4], GLfloat textureEpsilon[4]) const;
	GLsizei proportion(GLsizei a, GLsizei b, GLsizei c) const;
	bool isEqual(const GLubyte* color1, const GLubyte* color2, GLubyte tolerance) const;

	bool verifyClampMode(GLubyte* buf, GLsizei width, GLsizei height, GLenum clampMode, GLenum internalformat) const;
	bool verifyClampToEdge(GLubyte* buf, GLsizei width_init, GLsizei height_init, GLsizei width, GLsizei height,
						   GLenum internalformat) const;
	bool verifyRepeat(GLubyte* buf, GLsizei width, GLsizei height, GLenum internalformat) const;
	bool verifyMirroredRepeat(GLubyte* buf, GLsizei width, GLsizei height, GLenum internalformat) const;

private:
	/* Private attributes */
	GLenum  m_internalFormat;
	GLenum  m_clampMode;
	GLint   m_lodLevel;
	GLsizei m_width;
	GLsizei m_height;
};

/** Constructor.
 *
 * @param context Rendering context.
 **/
TestClampModeForInternalFormat::TestClampModeForInternalFormat(deqp::Context& context, const std::string& name,
															   GLenum internalFormat, GLenum clampMode, GLint lodLevel,
															   GLsizei width, GLsizei height)
	: deqp::TestCase(context, name.c_str(), "")
	, m_internalFormat(internalFormat)
	, m_clampMode(clampMode)
	, m_lodLevel(lodLevel)
	, m_width(width)
	, m_height(height)
{
	/* Left blank intentionally */
}

const InternalFormat& TestClampModeForInternalFormat::findInternalFormat(GLenum internalformat) const
{
	const InternalFormat* internalFormats	  = glInternalFormats;
	GLsizei				  internalFormatsCount = DE_LENGTH_OF_ARRAY(glInternalFormats);

	if (glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		internalFormats		 = esInternalFormats;
		internalFormatsCount = DE_LENGTH_OF_ARRAY(esInternalFormats);
	}

	for (GLsizei i = 0; i < internalFormatsCount; i++)
	{
		if (internalFormats[i].sizedFormat == internalformat)
			return internalFormats[i];
	}

	TCU_FAIL("Internal format not found");
}

const FormatInfo* TestClampModeForInternalFormat::findFormat(GLenum internalformat) const
{
	for (GLsizei i = 0; i < DE_LENGTH_OF_ARRAY(testedFormats); i++)
		if (testedFormats[i].internalformat == internalformat)
			return &testedFormats[i];
	return NULL;
}

bool TestClampModeForInternalFormat::isEqual(const GLubyte* color1, const GLubyte* color2, GLubyte tolerance) const
{
	for (int i = 0; i < 4; i++)
	{
		if (de::abs((int)color1[i] - (int)color2[i]) > tolerance)
			return false;
	}
	return true;
}

/** Fill texture with RGBW colors, according to the scheme:
 *  R R R G G G
 *  R R R G G G
 *  R R R G G G
 *  B B B W W W
 *  B B B W W W
 *  B B B W W W
 *
 *  NOTE: width of the red and blue rectangles would be less than green and white ones for odd texture's widths
 *		height of the red and green rectangles would be less than blue and white ones for odd texture's heights
 */
void TestClampModeForInternalFormat::fillTextureWithColor(GLubyte* texture_data, GLsizei tex_width, GLsizei tex_height,
														  GLenum internalformat) const
{
	const FormatInfo* testedFormat = findFormat(internalformat);
	const GLubyte*	red		   = testedFormat->internalred;
	const GLubyte*	green		   = testedFormat->internalgreen;
	const GLubyte*	blue		   = testedFormat->internalblue;
	const GLubyte*	white		   = testedFormat->internalwhite;
	const GLsizei	 size		   = testedFormat->pixelSize;

	GLint i = 0;
	GLint j = 0;
	for (j = 0; j < tex_height / 2; j++)
	{
		for (i = 0; i < tex_width / 2; i++)
		{
			deMemcpy(texture_data, red, size);
			texture_data += size;
		}
		for (; i < tex_width; i++)
		{
			deMemcpy(texture_data, green, size);
			texture_data += size;
		}
	}
	for (; j < tex_height; j++)
	{
		for (i = 0; i < tex_width / 2; i++)
		{
			deMemcpy(texture_data, blue, size);
			texture_data += size;
		}
		for (; i < tex_width; i++)
		{
			deMemcpy(texture_data, white, size);
			texture_data += size;
		}
	}
}

void TestClampModeForInternalFormat::clearTextures(GLenum target, GLsizei width, GLsizei height, GLint lod,
												   GLenum internalformat, GLenum type, GLenum format) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLint level;
	for (level = lod; level > 0; level--)
	{
		width *= 2;
		height *= 2;
	}

	bool				 continueLoop = true;
	std::vector<GLubyte> tex_buf(width * height * MAX_PIXEL_SIZE, 0);
	do
	{
		if (level != lod)
			gl.texImage2D(target, level, internalformat, width, height, 0, format, type, &tex_buf[0]);
		level++;

		continueLoop = !((height == 1) && (width == 1));
		if (width > 1)
			width /= 2;
		if (height > 1)
			height /= 2;
	} while (continueLoop);
}

/* Calculate error epsilons to the least accurate of either
** frame buffer or texture precision. RGBA epsilons are
** returned in textureEpsilon[]. target must be a valid
** texture target.
*/
void TestClampModeForInternalFormat::calcTextureEpsilon(const GLsizei textureBits[4], GLfloat textureEpsilon[4]) const
{
	GLint i, bits;
	GLint bufferBits[4];

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	gl.getIntegerv(GL_RED_BITS, &bufferBits[0]);
	gl.getIntegerv(GL_GREEN_BITS, &bufferBits[1]);
	gl.getIntegerv(GL_BLUE_BITS, &bufferBits[2]);
	gl.getIntegerv(GL_ALPHA_BITS, &bufferBits[3]);

	for (i = 0; i < 4; i++)
	{

		if (textureBits[i] == 0)
		{
			/* 'exact' */
			bits = 1000;
		}
		else
		{
			bits = textureBits[i];
		}

		/* Some tests fail on RGB10_A2 pixelformat, because framebuffer precision calculated here is 10-bit, but these tests use 8-bit data types
		   for pixel transfers and 8-bit textures. Because of that, the required precision is limited to 8-bit only. */
		if (bits > 8)
		{
			bits = 8;
		}

		/* frame buffer bits */
		if (bits > bufferBits[i])
		{
			bits = bufferBits[i];
		}

		if (bits == 0)
		{
			/* infinity */
			textureEpsilon[i] = 2.0f;
		}
		else
		{
			const float zeroEpsilon = deFloatLdExp(1.0f, -13);
			textureEpsilon[i]		= (1.0f / (deFloatLdExp(1.0f, bits) - 1.0f)) + zeroEpsilon;
			textureEpsilon[i]		= deMin(1.0f, textureEpsilon[i]);
		}

		/* If we have 8 bits framebuffer, we should hit the right value within one intensity level. */
		if (bits == 8 && bufferBits[i] != 0)
		{
			textureEpsilon[i] /= 2.0;
		}
	}
}

/* calculate (a * b) / c with rounding */
GLsizei TestClampModeForInternalFormat::proportion(GLsizei a, GLsizei b, GLsizei c) const
{
	float result = (float)a * b;
	return (GLsizei)(0.5f + result / c);
}

/* check out the read-back values for GL_CLAMP_TO_EDGE mode
 * r r r g g g
 * r r r g g g
 * r r R G g g
 * b b B W w w
 * b b b w w w
 * b b b w w w

   width_init, height_init arguments refer to the basic pattern texture, which describes proportions
   between colors in the rendered rectangle (tex_width, tex_height)
 */
bool TestClampModeForInternalFormat::verifyClampToEdge(GLubyte* buf, GLsizei width_init, GLsizei height_init,
													   GLsizei width, GLsizei height, GLenum internalformat) const
{
	GLint			  i, h;
	const FormatInfo* testedFormat = findFormat(internalformat);
	const GLubyte*	red		   = testedFormat->RGBAred;
	const GLubyte*	green		   = testedFormat->RGBAgreen;
	const GLubyte*	blue		   = testedFormat->RGBAblue;
	const GLubyte*	white		   = testedFormat->RGBAwhite;
	const GLsizei	 size		   = 4;
	const GLsizei	 skip		   = 6;

	GLsizei red_w   = proportion(width, width_init / 2, width_init);
	GLsizei red_h   = proportion(height, height_init / 2, height_init);
	GLsizei bits[4] = { 8, 8, 8, 8 };
	GLfloat epsilonf[4];
	GLubyte epsilons[4];

	TestArea check_area = { 0, width, 0, height };

	calcTextureEpsilon(bits, epsilonf);
	for (i = 0; i < 4; ++i)
	{
		epsilons[i] = (GLubyte)(epsilonf[i] * 255.0f);
	}

	for (h = 0; h < red_h - skip / 2; h++)
	{
		for (i = 0; i < red_w - skip / 2; i++)
		{
			/* skip over corner pixel to avoid issues with mipmap selection */
			if (i >= check_area.left || h >= check_area.top)
				if (!isEqual(buf, red, epsilons[0]))
					TCU_FAIL("verifyClampToEdge failed");
			buf += size;
		}

		/* skip over border pixels to avoid issues with rounding */
		i += skip;
		buf += skip * size;

		for (; i < width; i++)
		{
			/* skip over corner pixel to avoid issues with mipmap selection */
			if (i < check_area.right || h >= check_area.top)
				if (!isEqual(buf, green, epsilons[1]))
					TCU_FAIL("verifyClampToEdge failed");
			buf += size;
		}
	}

	/* skip over border pixels to avoid issues with rounding */
	h += skip;
	buf += skip * width * size;

	for (; h < height; h++)
	{
		for (i = 0; i < red_w - skip / 2; i++)
		{
			/* skip over corner pixel to avoid issues with mipmap selection */
			if (i >= check_area.left || h < check_area.bottom)
				if (!isEqual(buf, blue, epsilons[2]))
					TCU_FAIL("verifyClampToEdge failed");
			buf += size;
		}

		/* skip over border pixels to avoid issues with rounding */
		i += skip;
		buf += skip * size;

		for (; i < width; i++)
		{
			/* skip over corner pixel to avoid issues with mipmap selection */
			if (i < check_area.right || h < check_area.bottom)
				if (!isEqual(buf, white, epsilons[0]))
					TCU_FAIL("verifyClampToEdge failed");
			buf += size;
		}
	}

	m_context.getTestContext().getLog() << tcu::TestLog::Message << " verifyClampToEdge succeeded."
										<< tcu::TestLog::EndMessage;
	return true;
}

/* check out the read-back values for GL_REPEAT mode
 * r g r g r g
 * b w b w b w
 * r g R G r g
 * b w B W b w
 * r g r g r g
 * b w b w b w
 */
bool TestClampModeForInternalFormat::verifyRepeat(GLubyte* buf, GLsizei width, GLsizei height,
												  GLenum internalformat) const
{
	GLint			  i, j, g, h;
	const FormatInfo* testedFormat = findFormat(internalformat);

	const GLubyte* red		 = testedFormat->RGBAred;
	const GLubyte* green	 = testedFormat->RGBAgreen;
	const GLubyte* blue		 = testedFormat->RGBAblue;
	const GLubyte* white	 = testedFormat->RGBAwhite;
	const GLsizei  size		 = 4;
	const GLubyte  tolerance = 0;

	GLsizei tex_w = width / 3;
	GLsizei tex_h = height / 3;

	GLsizei red_w = tex_w / 2;
	GLsizei red_h = tex_h / 2;

	GLsizei green_w = tex_w - red_w;

	GLsizei blue_w = red_w;
	GLsizei blue_h = tex_h - red_h;

	GLsizei white_w = green_w;

	for (g = 0; g < 3; g++)
	{
		for (h = 0; h < red_h; h++)
		{
			for (j = 0; j < 3; j++)
			{
				for (i = 0; i < red_w; i++)
				{
					if (!isEqual(buf, red, tolerance))
						TCU_FAIL("verifyRepeat failed");

					buf += size;
				}
				for (i = 0; i < green_w; i++)
				{
					if (!isEqual(buf, green, tolerance))
						TCU_FAIL("verifyRepeat failed");

					buf += size;
				}
			}
		}
		for (h = 0; h < blue_h; h++)
		{
			for (j = 0; j < 3; j++)
			{
				for (i = 0; i < blue_w; i++)
				{
					if (!isEqual(buf, blue, tolerance))
						TCU_FAIL("verifyRepeat failed");

					buf += size;
				}
				for (i = 0; i < white_w; i++)
				{
					if (!isEqual(buf, white, tolerance))
						TCU_FAIL("verifyRepeat failed");

					buf += size;
				}
			}
		}
	}

	m_context.getTestContext().getLog() << tcu::TestLog::Message << " verifyRepeat succeeded."
										<< tcu::TestLog::EndMessage;
	return true;
}

/* check out the read-back values for GL_MIRRORED_REPEAT mode
 * w b b w w b
 * g r r g g r
 * r g R G r g
 * w b B W w b
 * w b b w w b
 * g r r g g r
 */
bool TestClampModeForInternalFormat::verifyMirroredRepeat(GLubyte* buf, GLsizei width, GLsizei height,
														  GLenum internalformat) const
{
	GLint			  i, j, g, h;
	const FormatInfo* testedFormat = findFormat(internalformat);

	const GLubyte* red   = testedFormat->RGBAred;
	const GLubyte* green = testedFormat->RGBAgreen;
	const GLubyte* blue  = testedFormat->RGBAblue;
	const GLubyte* white = testedFormat->RGBAwhite;

	const GLsizei size		= 4;
	const GLubyte tolerance = 0;

	GLsizei tex_w = width / 3;
	GLsizei tex_h = height / 3;

	GLsizei red_w = tex_w / 2;
	GLsizei red_h = tex_h / 2;

	GLsizei green_w = tex_w - red_w;
	GLsizei green_h = red_h;

	GLsizei blue_w = red_w;
	GLsizei blue_h = tex_h - red_h;

	GLsizei white_w = green_w;
	GLsizei white_h = blue_h;

	for (g = 0; g < 3; g++)
	{
		if (g % 2 == 0)
		{
			for (h = 0; h < white_h; h++)
			{
				for (j = 0; j < 3; j++)
				{
					if (j % 2 == 0)
					{
						for (i = 0; i < white_w; i++)
						{
							if (!isEqual(buf, white, tolerance))
								TCU_FAIL("verifyMirroredRepeat failed");

							buf += size;
						}
						for (i = 0; i < blue_w; i++)
						{
							if (!isEqual(buf, blue, tolerance))
								TCU_FAIL("verifyMirroredRepeat failed");

							buf += size;
						}
					}
					else
					{
						for (i = 0; i < blue_w; i++)
						{
							if (!isEqual(buf, blue, tolerance))
								TCU_FAIL("verifyMirroredRepeat failed");

							buf += size;
						}
						for (i = 0; i < white_w; i++)
						{
							if (!isEqual(buf, white, tolerance))
								TCU_FAIL("verifyMirroredRepeat failed");

							buf += size;
						}
					}
				}
			}
			for (h = 0; h < green_h; h++)
			{
				for (j = 0; j < 3; j++)
				{
					if (j % 2 == 0)
					{
						for (i = 0; i < green_w; i++)
						{
							if (!isEqual(buf, green, tolerance))
								TCU_FAIL("verifyMirroredRepeat failed");

							buf += size;
						}
						for (i = 0; i < red_w; i++)
						{
							if (!isEqual(buf, red, tolerance))
								TCU_FAIL("verifyMirroredRepeat failed");

							buf += size;
						}
					}
					else
					{
						for (i = 0; i < red_w; i++)
						{
							if (!isEqual(buf, red, tolerance))
								TCU_FAIL("verifyMirroredRepeat failed");

							buf += size;
						}
						for (i = 0; i < green_w; i++)
						{
							if (!isEqual(buf, green, tolerance))
								TCU_FAIL("verifyMirroredRepeat failed");

							buf += size;
						}
					}
				}
			}
		}
		else
		{
			for (h = 0; h < green_h; h++)
			{
				for (j = 0; j < 3; j++)
				{
					if (j % 2 == 0)
					{
						for (i = 0; i < green_w; i++)
						{
							if (!isEqual(buf, green, tolerance))
								TCU_FAIL("verifyMirroredRepeat failed");

							buf += size;
						}
						for (i = 0; i < red_w; i++)
						{
							if (!isEqual(buf, red, tolerance))
								TCU_FAIL("verifyMirroredRepeat failed");

							buf += size;
						}
					}
					else
					{
						for (i = 0; i < red_w; i++)
						{
							if (!isEqual(buf, red, tolerance))
								TCU_FAIL("verifyMirroredRepeat failed");

							buf += size;
						}
						for (i = 0; i < green_w; i++)
						{
							if (!isEqual(buf, green, tolerance))
								TCU_FAIL("verifyMirroredRepeat failed");

							buf += size;
						}
					}
				}
			}
			for (h = 0; h < white_h; h++)
			{
				for (j = 0; j < 3; j++)
				{
					if (j % 2 == 0)
					{
						for (i = 0; i < white_w; i++)
						{
							if (!isEqual(buf, white, tolerance))
								TCU_FAIL("verifyMirroredRepeat failed");

							buf += size;
						}
						for (i = 0; i < blue_w; i++)
						{
							if (!isEqual(buf, blue, tolerance))
								TCU_FAIL("verifyMirroredRepeat failed");

							buf += size;
						}
					}
					else
					{
						for (i = 0; i < blue_w; i++)
						{
							if (!isEqual(buf, blue, tolerance))
								TCU_FAIL("verifyMirroredRepeat failed");

							buf += size;
						}
						for (i = 0; i < white_w; i++)
						{
							if (!isEqual(buf, white, tolerance))
								TCU_FAIL("verifyMirroredRepeat failed");

							buf += size;
						}
					}
				}
			}
		}
	}

	m_context.getTestContext().getLog() << tcu::TestLog::Message << " verifyMirroredRepeat succeeded."
										<< tcu::TestLog::EndMessage;
	return true;
}

bool TestClampModeForInternalFormat::verifyClampMode(GLubyte* buf, GLsizei width, GLsizei height, GLenum clampMode,
													 GLenum internalformat) const
{
	switch (clampMode)
	{
	case GL_CLAMP_TO_EDGE:
		return verifyClampToEdge(buf, width, height, width, height, internalformat);
	case GL_REPEAT:
		return verifyRepeat(buf, width, height, internalformat);
	case GL_MIRRORED_REPEAT:
		return verifyMirroredRepeat(buf, width, height, internalformat);
	}
	return false;
}

/** Execute test
 *
 * Upload the texture, set up a quad 3 times the size of the
 * texture. Coordinates should be integers to avoid spatial rounding
 * differences. Set texture coordinates as shown below.
 *
 * (-1,  2, 0) --- (2,  2, 0)
 * |						|
 * |						|
 * (-1, -1, 0) --- (2, -1, 0)
 *
 * Set TEXTURE_MIN_FILTER for the texture to NEAREST_MIPMAP_NEAREST.
 *
 * Repeat the test for each repeat mode, i.e., set the repeat mode to
 * one of CLAMP_TO_EDGE, REPEAT, and MIRRORED_REPEAT for both S and
 * T, depending on the iteration.
 *
 * For vertex shader, just pass on the vertices and texture
 * coordinates for interpolation. For fragment shader, look up the
 * fragment corresponding to given texture coordinates.
 *
 * Render the quad.
 *
 * Read back the pixels covering the quad.
 *
 * For CLAMP_TO_EDGE result should be (Each character has dimension
 * half the original texture, original texture in capitals):
 *
 * rrrggg
 * rrrggg
 * rrRGgg
 * bbBWww
 * bbbwww
 * bbbwww
 *
 * For REPEAT:
 *
 * rgrgrg
 * bwbwbw
 * rgRGrg
 * bwBWbw
 * rgrgrg
 * bwbwbw
 *
 * For MIRRORED_REPEAT
 *
 * wbbwwb
 * grrggr
 * grRGgr
 * wbBWwb
 * wbbwwb
 * grrggr
 *
 * If implementation under test is for OpenGL 3.2 Core specification,
 * the test includes repeat mode of CLAMP_TO_BORDER. For this case,
 * the TEXTURE_BORDER_COLOR is set to black (0, 0, 0, 1) (RGBA). Then
 * the result will be (0 meaning black):
 *
 * 000000
 * 000000
 * 00RG00
 * 00BW00
 * 000000
 * 000000
 *
 * Procedure:
 * - allocate large enough memory buffer for the texture data - tex_width * tex_height * tex_depth * MAX_PIXEL_SIZE
 * - fill the buffer with the pattern
 * - upload the texture with glTexImage2D() to the requested level
 * - upload black texture to other LOD levels
 * - render a quad with size matching the texture LOD level
 * - read back pixels
 * - verify the results
 * - free the buffer
**/
tcu::TestNode::IterateResult TestClampModeForInternalFormat::iterate(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	/* Retrieve properties of the tested format */
	const InternalFormat& internalFormatStruct = findInternalFormat(m_internalFormat);
	GLenum				  format			   = internalFormatStruct.format;
	GLenum				  type				   = internalFormatStruct.type;
	GLenum				  internalformat	   = internalFormatStruct.sizedFormat;
	GLenum				  sampler			   = internalFormatStruct.sampler;
	GLsizei				  viewWidth			   = 3 * m_width;
	GLsizei				  viewHeight		   = 3 * m_height;

	/* Allocate buffers for texture data */
	GLsizei				 resultTextureSize = viewWidth * viewHeight;
	std::vector<GLubyte> textureData(resultTextureSize * MAX_PIXEL_SIZE, 0);
	std::fill(textureData.begin(), textureData.end(), 0);

	/* Set pixel storage modes */
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 1);
	gl.pixelStorei(GL_PACK_ALIGNMENT, 1);

	/* Generate and bind the texture object that will store test result*/
	GLuint resultTextureId;
	gl.genTextures(1, &resultTextureId);
	gl.bindTexture(GL_TEXTURE_2D, resultTextureId);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, viewWidth, viewHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, &textureData[0]);

	/* Reuse textureData buffer to initialize source texture.
	 * Fill the buffer according to the color scheme */
	fillTextureWithColor(&textureData[0], m_width, m_height, internalformat);

	/* Generate and bind the texture object that will store color scheme*/
	GLuint sourceTextureId;
	gl.genTextures(1, &sourceTextureId);
	gl.bindTexture(GL_TEXTURE_2D, sourceTextureId);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texImage2D(GL_TEXTURE_2D, m_lodLevel, internalformat, m_width, m_height, 0, format, type, &textureData[0]);

	/* Set compare function used by shadow samplers */
	if ((GL_DEPTH_COMPONENT == format) || (GL_DEPTH_STENCIL == format))
	{
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GEQUAL);
	}

	/* Clear the other source texture levels with black */
	clearTextures(GL_TEXTURE_2D, m_width, m_height, m_lodLevel, internalformat, type, format);

	/* Create and bind the FBO */
	GLuint fbId;
	gl.genFramebuffers(1, &fbId);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbId);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resultTextureId, 0);

	/* Construct shaders */
	glu::ContextType contextType = m_context.getRenderContext().getType();
	glu::GLSLVersion glslVersion = glu::getContextTypeGLSLVersion(contextType);
	std::string		 version	 = glu::getGLSLVersionDeclaration(glslVersion) + std::string("\n");
	std::string		 vs			 = version + basic_vs;
	std::string		 fs			 = version;
	if ((GL_DEPTH_COMPONENT == format) || (GL_DEPTH_STENCIL == format))
		fs += shadow_fs;
	else if ((SAMPLER_UINT == sampler) || (SAMPLER_INT == sampler))
		fs += integer_fs;
	else
		fs += basic_fs;
	glu::ProgramSources sources = glu::makeVtxFragSources(vs, fs);

	/* Create program object */
	glu::ShaderProgram program(m_context.getRenderContext(), sources);
	if (!program.isOk())
		TCU_FAIL("Compile failed");
	GLint programId		  = program.getProgram();
	GLint samplerLocation = gl.getUniformLocation(programId, "texture0");
	if (-1 == samplerLocation)
		TCU_FAIL("Fragment shader does not have texture0 input.");
	gl.useProgram(programId);
	gl.uniform1i(samplerLocation, 0);

	m_context.getTestContext().getLog() << tcu::TestLog::Message << "NPOT [" << m_internalFormat << "] (" << viewWidth
										<< " x " << viewHeight << "), Level: " << m_lodLevel
										<< tcu::TestLog::EndMessage;

	/* Set the wrap parameters for texture coordinates s and t to the current clamp mode */
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_clampMode);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_clampMode);

	/* Set viewport's width and height based on an overview */
	gl.viewport(0, 0, viewWidth, viewHeight);

	/* Draw rectangle */
	GLuint vaoId;
	gl.genVertexArrays(1, &vaoId);
	gl.bindVertexArray(vaoId);
	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	gl.bindVertexArray(0);
	gl.deleteVertexArrays(1, &vaoId);

	/* Read back pixels and verify that they have the proper values */
	std::vector<GLubyte> buffer(resultTextureSize * 4, 0);
	gl.readPixels(0, 0, viewWidth, viewHeight, GL_RGBA, GL_UNSIGNED_BYTE, (void*)&(buffer[0]));
	if (verifyClampMode(&buffer[0], viewWidth, viewHeight, m_clampMode, internalformat))
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Cleanup */
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.deleteTextures(1, &sourceTextureId);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbId);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	gl.deleteFramebuffers(1, &fbId);

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 */
TextureRepeatModeTests::TextureRepeatModeTests(deqp::Context& context)
	: TestCaseGroup(context, "texture_repeat_mode", "Texture repeat mode tests")
{
}

/** Initializes the test group contents. */
void TextureRepeatModeTests::init()
{
	/* Texture sizes to test */
	const struct TexSize
	{
		GLsizei width;
		GLsizei height;
	} textureSizes[] = {
		{ 49, 23 }, { 11, 131 },
	};

	/* LOD levels to test */
	const GLint levelsOfDetail[] = { 0, 1, 2 };

	for (GLint sizeIndex = 0; sizeIndex < DE_LENGTH_OF_ARRAY(textureSizes); sizeIndex++)
	{
		const TexSize& ts = textureSizes[sizeIndex];
		for (GLint formatIndex = 0; formatIndex < DE_LENGTH_OF_ARRAY(testedFormats); formatIndex++)
		{
			const FormatInfo& formatInfo	 = testedFormats[formatIndex];
			GLenum			  internalFormat = formatInfo.internalformat;
			for (GLsizei lodIndex = 0; lodIndex < DE_LENGTH_OF_ARRAY(levelsOfDetail); lodIndex++)
			{
				GLint			  lod = levelsOfDetail[lodIndex];
				std::stringstream testBaseName;
				testBaseName << formatInfo.name << "_" << ts.width << "x" << ts.height << "_" << lod << "_";
				std::string names[] = { testBaseName.str() + "clamp_to_edge", testBaseName.str() + "repeat",
										testBaseName.str() + "mirrored_repeat" };
				addChild(new TestClampModeForInternalFormat(m_context, names[0], internalFormat, GL_CLAMP_TO_EDGE, lod,
															ts.width, ts.height));
				addChild(new TestClampModeForInternalFormat(m_context, names[1], internalFormat, GL_REPEAT, lod,
															ts.width, ts.height));
				addChild(new TestClampModeForInternalFormat(m_context, names[2], internalFormat, GL_MIRRORED_REPEAT,
															lod, ts.width, ts.height));
			}
		}
	}
}
} /* glcts namespace */
