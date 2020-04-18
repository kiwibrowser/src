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
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */ /*!
 * \file glcPackedPixelsTests.cpp
 * \brief
 */ /*-------------------------------------------------------------------*/

#include "glcPackedPixelsTests.hpp"
#include "deMath.h"
#include "glcMisc.hpp"
#include "gluContextInfo.hpp"
#include "gluShaderProgram.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"
#include <algorithm>
#include <cstring>
#include <limits>
#include <map>
#include <stdio.h>

using namespace glw;
using namespace glu;

namespace glcts
{

enum
{
	GRADIENT_WIDTH  = 7,
	GRADIENT_HEIGHT = 3
};

enum InputOutputOperation
{
	OUTPUT_GETTEXIMAGE,
	OUTPUT_READPIXELS,
	INPUT_TEXIMAGE,
};

enum ComponentFormat
{
	FORMAT_STENCIL,		  // stencil, unsigned int
	FORMAT_DEPTH,		  // depth, unsigned [fp|float]
	FORMAT_DEPTH_STENCIL, // depth+stencil, unsigned [fp|float]
	FORMAT_COLOR,		  // color, [signed|unsigned] fp
	FORMAT_COLOR_INTEGER, // color, [signed|unsigned] int
};

enum TypeStorage
{
	STORAGE_UNSIGNED, // unsigned fp/int
	STORAGE_SIGNED,   // signed fp/int
	STORAGE_FLOAT,	// signed/unsigned float
};

union InternalFormatBits {
	struct Bits
	{
		int red;	   // red bits
		int green;	 // green bits
		int blue;	  // blue bits
		int alpha;	 // alpha bits
		int intensity; // intensity bits
		int luminance; // luminance bits
		int depth;	 // depth bits
		int stencil;   // stencil bits
		int exponent;  // shared exponent bits
	} bits;
	int array[9]; // all the bits
};

struct PixelType
{
	GLenum			   type;
	int				   size;
	int				   storage;
	bool			   special;
	bool			   reversed;
	InternalFormatBits bits;
	bool			   clamp;
};

struct PixelFormat
{
	GLenum			   format;			// format name
	int				   components;		// number of components
	int				   componentFormat; // element meaning
	GLenum			   attachment;		// target buffer
	InternalFormatBits componentOrder;  // zero based element order, -1 for N/A
};

enum InternalFormatSamplerType
{
	SAMPLER_UNORM = 0, // unsigned normalized
	SAMPLER_NORM,	  // normalized
	SAMPLER_UINT,	  // unsigned integer
	SAMPLER_INT,	   // integer
	SAMPLER_FLOAT	  // floating-point
};

enum InternalFormatFlag
{
	FLAG_PACKED		  = 1,									   // packed pixel format
	FLAG_COMPRESSED   = 2,									   // compressed format
	FLAG_REQ_RBO_GL42 = 4,									   // required RBO & tex format in OpenGL 4.2
	FLAG_REQ_RBO_ES30 = 8,									   // required RBO & tex format in OpenGL ES 3.0
	FLAG_REQ_RBO	  = FLAG_REQ_RBO_GL42 | FLAG_REQ_RBO_ES30, // Required RBO & tex format in both
};

struct InternalFormat
{
	GLenum					  sizedFormat;
	GLenum					  baseFormat;
	GLenum					  format;
	GLenum					  type;
	InternalFormatSamplerType sampler;
	InternalFormatBits		  bits;
	int						  flags; // InternalFormatFlag
};

struct EnumFormats
{
	GLenum internalformat;
	GLenum format;
	GLenum type;
	int	size;
	bool   bRenderable;
};

#define PACK_DEFAULTI (0)
#define PACK_DEFAULTUI (0)
#define PACK_DEFAULTF (-2.0f)

static const InternalFormat coreInternalformats[] =
{
	{ GL_DEPTH_COMPONENT,			  GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT,				 SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 0,16, 0, 0 } }, 0 },
	{ GL_DEPTH_STENCIL,				  GL_DEPTH_STENCIL,   GL_DEPTH_STENCIL,   GL_UNSIGNED_INT,					 SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 0,24, 8, 0 } }, 0 },
	{ GL_RED,						  GL_RED,			  GL_RED,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 8, 0, 0, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RG,						  GL_RG,			  GL_RG,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 8, 8, 0, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_R8,						  GL_RED,			  GL_RED,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 8, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_R8_SNORM,					  GL_RED,			  GL_RED,			  GL_BYTE,							 SAMPLER_NORM,   { { 8, 0, 0, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_R16,						  GL_RED,			  GL_RED,			  GL_UNSIGNED_SHORT,				 SAMPLER_UNORM,  { {16, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO_GL42 },
	{ GL_R16_SNORM,					  GL_RED,			  GL_RED,			  GL_SHORT,							 SAMPLER_NORM,   { {16, 0, 0, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RG8,						  GL_RG,			  GL_RG,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 8, 8, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RG8_SNORM,					  GL_RG,			  GL_RG,			  GL_BYTE,							 SAMPLER_NORM,   { { 8, 8, 0, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RG16,						  GL_RG,			  GL_RG,			  GL_UNSIGNED_SHORT,				 SAMPLER_UNORM,  { {16,16, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO_GL42 },
	{ GL_RG16_SNORM,				  GL_RG,			  GL_RG,			  GL_SHORT,							 SAMPLER_NORM,   { {16,16, 0, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_R3_G3_B2,					  GL_RGB,			  GL_RGB,			  GL_UNSIGNED_BYTE_3_3_2,			 SAMPLER_UNORM,  { { 3, 3, 2, 0, 0, 0, 0, 0, 0 } }, FLAG_PACKED },
	{ GL_RGB4,						  GL_RGB,			  GL_RGB,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 4, 4, 4, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB5,						  GL_RGB,			  GL_RGB,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 5, 5, 5, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB8,						  GL_RGB,			  GL_RGB,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 8, 8, 8, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO_ES30 },
	{ GL_RGB8_SNORM,				  GL_RGB,			  GL_RGB,			  GL_BYTE,							 SAMPLER_NORM,   { { 8, 8, 8, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB10,						  GL_RGB,			  GL_RGB,			  GL_UNSIGNED_SHORT,				 SAMPLER_UNORM,  { {10,10,10, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB12,						  GL_RGB,			  GL_RGB,			  GL_UNSIGNED_SHORT,				 SAMPLER_UNORM,  { {12,12,12, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB16,						  GL_RGB,			  GL_RGB,			  GL_UNSIGNED_SHORT,				 SAMPLER_UNORM,  { {16,16,16, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB16_SNORM,				  GL_RGB,			  GL_RGB,			  GL_SHORT,							 SAMPLER_NORM,   { {16,16,16, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGBA2,						  GL_RGBA,			  GL_RGBA,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 2, 2, 2, 2, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGBA4,						  GL_RGBA,			  GL_RGBA,			  GL_UNSIGNED_SHORT_4_4_4_4,		 SAMPLER_UNORM,  { { 4, 4, 4, 4, 0, 0, 0, 0, 0 } }, FLAG_PACKED|FLAG_REQ_RBO },
	{ GL_RGB5_A1,					  GL_RGBA,			  GL_RGBA,			  GL_UNSIGNED_SHORT_5_5_5_1,		 SAMPLER_UNORM,  { { 5, 5, 5, 1, 0, 0, 0, 0, 0 } }, FLAG_PACKED|FLAG_REQ_RBO },
	{ GL_RGBA8,						  GL_RGBA,			  GL_RGBA,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 8, 8, 8, 8, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RGBA8_SNORM,				  GL_RGBA,			  GL_RGBA,			  GL_BYTE,							 SAMPLER_NORM,   { { 8, 8, 8, 8, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB10_A2,					  GL_RGBA,			  GL_RGBA,			  GL_UNSIGNED_INT_10_10_10_2,		 SAMPLER_UNORM,  { {10,10,10, 2, 0, 0, 0, 0, 0 } }, FLAG_PACKED|FLAG_REQ_RBO_GL42 },
	{ GL_RGB10_A2UI,				  GL_RGBA,			  GL_RGBA_INTEGER,	  GL_UNSIGNED_INT_10_10_10_2,		 SAMPLER_UINT,   { {10,10,10, 2, 0, 0, 0, 0, 0 } }, FLAG_PACKED|FLAG_REQ_RBO_GL42 },
	{ GL_RGBA12,					  GL_RGBA,			  GL_RGBA,			  GL_UNSIGNED_SHORT,				 SAMPLER_UNORM,  { {12,12,12,12, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGBA16,					  GL_RGBA,			  GL_RGBA,			  GL_UNSIGNED_SHORT,				 SAMPLER_UNORM,  { {16,16,16,16, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO_GL42 },
	{ GL_RGBA16_SNORM,				  GL_RGBA,			  GL_RGBA,			  GL_SHORT,							 SAMPLER_NORM,   { {16,16,16,16, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_SRGB8,						  GL_RGB,			  GL_RGB,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 8, 8, 8, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_SRGB8_ALPHA8,				  GL_RGBA,			  GL_RGBA,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 8, 8, 8, 8, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_R16F,						  GL_RED,			  GL_RED,			  GL_HALF_FLOAT,					 SAMPLER_FLOAT,  { {16, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO_GL42 },
	{ GL_RG16F,						  GL_RG,			  GL_RG,			  GL_HALF_FLOAT,					 SAMPLER_FLOAT,  { {16,16, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO_GL42 },
	{ GL_RGB16F,					  GL_RGB,			  GL_RGB,			  GL_HALF_FLOAT,					 SAMPLER_FLOAT,  { {16,16,16, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGBA16F,					  GL_RGBA,			  GL_RGBA,			  GL_HALF_FLOAT,					 SAMPLER_FLOAT,  { {16,16,16,16, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO_GL42 },
	{ GL_R32F,						  GL_RED,			  GL_RED,			  GL_FLOAT,							 SAMPLER_FLOAT,  { {32, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO_GL42 },
	{ GL_RG32F,						  GL_RG,			  GL_RG,			  GL_FLOAT,							 SAMPLER_FLOAT,  { {32,32, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO_GL42 },
	{ GL_RGB32F,					  GL_RGB,			  GL_RGB,			  GL_FLOAT,							 SAMPLER_FLOAT,  { {32,32,32, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGBA32F,					  GL_RGBA,			  GL_RGBA,			  GL_FLOAT,							 SAMPLER_FLOAT,  { {32,32,32,32, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO_GL42 },
	{ GL_R11F_G11F_B10F,			  GL_RGB,			  GL_RGB,			  GL_UNSIGNED_INT_10F_11F_11F_REV,	 SAMPLER_FLOAT,  { {11,11,10, 0, 0, 0, 0, 0, 0 } }, FLAG_PACKED|FLAG_REQ_RBO_GL42 },
	{ GL_RGB9_E5,					  GL_RGB,			  GL_RGB,			  GL_UNSIGNED_INT_5_9_9_9_REV,		 SAMPLER_FLOAT,  { { 9, 9, 9, 0, 0, 0, 0, 0, 5 } }, FLAG_PACKED },
	{ GL_R8I,						  GL_RED,			  GL_RED_INTEGER,	  GL_BYTE,							 SAMPLER_INT,	 { { 8, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_R8UI,						  GL_RED,			  GL_RED_INTEGER,	  GL_UNSIGNED_BYTE,					 SAMPLER_UINT,   { { 8, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_R16I,						  GL_RED,			  GL_RED_INTEGER,	  GL_SHORT,							 SAMPLER_INT,	 { {16, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_R16UI,						  GL_RED,			  GL_RED_INTEGER,	  GL_UNSIGNED_SHORT,				 SAMPLER_UINT,   { {16, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_R32I,						  GL_RED,			  GL_RED_INTEGER,	  GL_INT,							 SAMPLER_INT,	 { {32, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_R32UI,						  GL_RED,			  GL_RED_INTEGER,	  GL_UNSIGNED_INT,					 SAMPLER_UINT,   { {32, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RG8I,						  GL_RG,			  GL_RG_INTEGER,	  GL_BYTE,							 SAMPLER_INT,	 { { 8, 8, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RG8UI,						  GL_RG,			  GL_RG_INTEGER,	  GL_UNSIGNED_BYTE,					 SAMPLER_UINT,   { { 8, 8, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RG16I,						  GL_RG,			  GL_RG_INTEGER,	  GL_SHORT,							 SAMPLER_INT,	 { {16,16, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RG16UI,					  GL_RG,			  GL_RG_INTEGER,	  GL_UNSIGNED_SHORT,				 SAMPLER_UINT,   { {16,16, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RG32I,						  GL_RG,			  GL_RG_INTEGER,	  GL_INT,							 SAMPLER_INT,	 { {32,32, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RG32UI,					  GL_RG,			  GL_RG_INTEGER,	  GL_UNSIGNED_INT,					 SAMPLER_UINT,   { {32,32, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RGB8I,						  GL_RGB,			  GL_RGB_INTEGER,	  GL_BYTE,							 SAMPLER_INT,	 { { 8, 8, 8, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB8UI,					  GL_RGB,			  GL_RGB_INTEGER,	  GL_UNSIGNED_BYTE,					 SAMPLER_UINT,   { { 8, 8, 8, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB16I,					  GL_RGB,			  GL_RGB_INTEGER,	  GL_SHORT,							 SAMPLER_INT,	 { {16,16,16, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB16UI,					  GL_RGB,			  GL_RGB_INTEGER,	  GL_UNSIGNED_SHORT,				 SAMPLER_UINT,   { {16,16,16, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB32I,					  GL_RGB,			  GL_RGB_INTEGER,	  GL_INT,							 SAMPLER_INT,	 { {32,32,32, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB32UI,					  GL_RGB,			  GL_RGB_INTEGER,	  GL_UNSIGNED_INT,					 SAMPLER_UINT,   { {32,32,32, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGBA8I,					  GL_RGBA,			  GL_RGBA_INTEGER,	  GL_BYTE,							 SAMPLER_INT,	 { { 8, 8, 8, 8, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RGBA8UI,					  GL_RGBA,			  GL_RGBA_INTEGER,	  GL_UNSIGNED_BYTE,					 SAMPLER_UINT,   { { 8, 8, 8, 8, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RGBA16I,					  GL_RGBA,			  GL_RGBA_INTEGER,	  GL_SHORT,							 SAMPLER_INT,	 { {16,16,16,16, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RGBA16UI,					  GL_RGBA,			  GL_RGBA_INTEGER,	  GL_UNSIGNED_SHORT,				 SAMPLER_UINT,   { {16,16,16,16, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RGBA32I,					  GL_RGBA,			  GL_RGBA_INTEGER,	  GL_INT,							 SAMPLER_INT,	 { {32,32,32,32, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RGBA32UI,					  GL_RGBA,			  GL_RGBA_INTEGER,	  GL_UNSIGNED_INT,					 SAMPLER_UINT,   { {32,32,32,32, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_DEPTH_COMPONENT16,			  GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT,				 SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 0,16, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_DEPTH_COMPONENT24,			  GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,					 SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 0,24, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_DEPTH_COMPONENT32,			  GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,					 SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 0,32, 0, 0 } }, 0 },
	{ GL_DEPTH_COMPONENT32F,		  GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT,							 SAMPLER_FLOAT,  { { 0, 0, 0, 0, 0, 0,32, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_DEPTH24_STENCIL8,			  GL_DEPTH_STENCIL,   GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8,				 SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 0,24, 8, 0 } }, FLAG_REQ_RBO },
	{ GL_DEPTH32F_STENCIL8,			  GL_DEPTH_STENCIL,   GL_DEPTH_STENCIL,   GL_FLOAT_32_UNSIGNED_INT_24_8_REV, SAMPLER_FLOAT,  { { 0, 0, 0, 0, 0, 0,32, 8, 0 } }, FLAG_PACKED|FLAG_REQ_RBO },
	{ GL_COMPRESSED_RED,			  GL_RED,			  GL_RED,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 8, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_COMPRESSED },
	{ GL_COMPRESSED_RG,				  GL_RG,			  GL_RG,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 8, 8, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_COMPRESSED },
	{ GL_COMPRESSED_RGB,			  GL_RGB,			  GL_RGB,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 8, 8, 8, 0, 0, 0, 0, 0, 0 } }, FLAG_COMPRESSED },
	{ GL_COMPRESSED_RGBA,			  GL_RGBA,			  GL_RGBA,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 8, 8, 8, 8, 0, 0, 0, 0, 0 } }, FLAG_COMPRESSED },
	{ GL_COMPRESSED_SRGB,			  GL_RGB,			  GL_RGB,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 8, 8, 8, 0, 0, 0, 0, 0, 0 } }, FLAG_COMPRESSED },
	{ GL_COMPRESSED_SRGB_ALPHA,		  GL_RGBA,			  GL_RGBA,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 8, 8, 8, 8, 0, 0, 0, 0, 0 } }, FLAG_COMPRESSED },
	{ GL_COMPRESSED_RED_RGTC1,		  GL_RED,			  GL_RED,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 8, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_COMPRESSED },
	{ GL_COMPRESSED_SIGNED_RED_RGTC1, GL_RED,			  GL_RED,			  GL_BYTE,							 SAMPLER_NORM,   { { 8, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_COMPRESSED },
	{ GL_COMPRESSED_RG_RGTC2,		  GL_RG,			  GL_RG,			  GL_UNSIGNED_BYTE,					 SAMPLER_UNORM,  { { 8, 8, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_COMPRESSED },
	{ GL_COMPRESSED_SIGNED_RG_RGTC2,  GL_RG,			  GL_RG,			  GL_BYTE,							 SAMPLER_NORM,   { { 8, 8, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_COMPRESSED },
};

static InternalFormat esInternalformats[] =
{
	{ GL_LUMINANCE,			 GL_LUMINANCE,		 GL_LUMINANCE,		 GL_UNSIGNED_BYTE,					SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 8, 0, 0, 0 } }, 0 },
	{ GL_ALPHA,				 GL_ALPHA,			 GL_ALPHA,			 GL_UNSIGNED_BYTE,					SAMPLER_UNORM,  { { 0, 0, 0, 8, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_LUMINANCE_ALPHA,	 GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,					SAMPLER_UNORM,  { { 0, 0, 0, 8, 0, 8, 0, 0, 0 } }, 0 },
	{ GL_RGB,				 GL_RGB,			 GL_RGB,			 GL_UNSIGNED_BYTE,					SAMPLER_UNORM,  { { 8, 8, 8, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGBA,				 GL_RGBA,			 GL_RGBA,			 GL_UNSIGNED_BYTE,					SAMPLER_UNORM,  { { 8, 8, 8, 8, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_R8,				 GL_RED,			 GL_RED,			 GL_UNSIGNED_BYTE,					SAMPLER_UNORM,  { { 8, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_R8_SNORM,			 GL_RED,			 GL_RED,			 GL_BYTE,							SAMPLER_NORM,   { { 8, 0, 0, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RG8,				 GL_RG,				 GL_RG,				 GL_UNSIGNED_BYTE,					SAMPLER_UNORM,  { { 8, 8, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RG8_SNORM,			 GL_RG,				 GL_RG,				 GL_BYTE,							SAMPLER_NORM,   { { 8, 8, 0, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB8,				 GL_RGB,			 GL_RGB,			 GL_UNSIGNED_BYTE,					SAMPLER_UNORM,  { { 8, 8, 8, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO_ES30 },
	{ GL_RGB8_SNORM,		 GL_RGB,			 GL_RGB,			 GL_BYTE,							SAMPLER_NORM,   { { 8, 8, 8, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB565,			 GL_RGB,			 GL_RGB,			 GL_UNSIGNED_SHORT_5_6_5,			SAMPLER_UNORM,  { { 5, 6, 5, 0, 0, 0, 0, 0, 0 } }, FLAG_PACKED|FLAG_REQ_RBO },
	{ GL_RGBA4,				 GL_RGBA,			 GL_RGBA,			 GL_UNSIGNED_SHORT_4_4_4_4,			SAMPLER_UNORM,  { { 4, 4, 4, 4, 0, 0, 0, 0, 0 } }, FLAG_PACKED|FLAG_REQ_RBO },
	{ GL_RGB5_A1,			 GL_RGBA,			 GL_RGBA,			 GL_UNSIGNED_SHORT_5_5_5_1,			SAMPLER_UNORM,  { { 5, 5, 5, 1, 0, 0, 0, 0, 0 } }, FLAG_PACKED|FLAG_REQ_RBO },
	{ GL_RGBA8,				 GL_RGBA,			 GL_RGBA,			 GL_UNSIGNED_BYTE,					SAMPLER_UNORM,  { { 8, 8, 8, 8, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RGBA8_SNORM,		 GL_RGBA,			 GL_RGBA,			 GL_BYTE,							SAMPLER_NORM,   { { 8, 8, 8, 8, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB10_A2,			 GL_RGBA,			 GL_RGBA,			 GL_UNSIGNED_INT_2_10_10_10_REV,	SAMPLER_UNORM,  { {10,10,10, 2, 0, 0, 0, 0, 0 } }, FLAG_PACKED|FLAG_REQ_RBO_ES30 },
	{ GL_RGB10_A2UI,		 GL_RGBA,			 GL_RGBA_INTEGER,	 GL_UNSIGNED_INT_2_10_10_10_REV,	SAMPLER_UINT,   { {10,10,10, 2, 0, 0, 0, 0, 0 } }, FLAG_PACKED|FLAG_REQ_RBO_ES30 },
	{ GL_SRGB8,				 GL_RGB,			 GL_RGB,			 GL_UNSIGNED_BYTE,					SAMPLER_UNORM,  { { 8, 8, 8, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_SRGB8_ALPHA8,		 GL_RGBA,			 GL_RGBA,			 GL_UNSIGNED_BYTE,					SAMPLER_UNORM,  { { 8, 8, 8, 8, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_R16F,				 GL_RED,			 GL_RED,			 GL_HALF_FLOAT,						SAMPLER_FLOAT,  { {16, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO_GL42 },
	{ GL_RG16F,				 GL_RG,				 GL_RG,				 GL_HALF_FLOAT,						SAMPLER_FLOAT,  { {16,16, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO_GL42 },
	{ GL_RGB16F,			 GL_RGB,			 GL_RGB,			 GL_HALF_FLOAT,						SAMPLER_FLOAT,  { {16,16,16, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGBA16F,			 GL_RGBA,			 GL_RGBA,			 GL_HALF_FLOAT,						SAMPLER_FLOAT,  { {16,16,16,16, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO_GL42 },
	{ GL_R32F,				 GL_RED,			 GL_RED,			 GL_FLOAT,							SAMPLER_FLOAT,  { {32, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO_GL42 },
	{ GL_RG32F,				 GL_RG,				 GL_RG,				 GL_FLOAT,							SAMPLER_FLOAT,  { {32,32, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO_GL42 },
	{ GL_RGB32F,			 GL_RGB,			 GL_RGB,			 GL_FLOAT,							SAMPLER_FLOAT,  { {32,32,32, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGBA32F,			 GL_RGBA,			 GL_RGBA,			 GL_FLOAT,							SAMPLER_FLOAT,  { {32,32,32,32, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO_GL42 },
	{ GL_R11F_G11F_B10F,	 GL_RGB,			 GL_RGB,			 GL_UNSIGNED_INT_10F_11F_11F_REV,	SAMPLER_FLOAT,  { {11,11,10, 0, 0, 0, 0, 0, 0 } }, FLAG_PACKED|FLAG_REQ_RBO_GL42 },
	{ GL_RGB9_E5,			 GL_RGB,			 GL_RGB,			 GL_UNSIGNED_INT_5_9_9_9_REV,		SAMPLER_FLOAT,  { { 9, 9, 9, 0, 0, 0, 0, 0, 5 } }, FLAG_PACKED },
	{ GL_R8I,				 GL_RED,			 GL_RED_INTEGER,	 GL_BYTE,							SAMPLER_INT,	{ { 8, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_R8UI,				 GL_RED,			 GL_RED_INTEGER,	 GL_UNSIGNED_BYTE,					SAMPLER_UINT,   { { 8, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_R16I,				 GL_RED,			 GL_RED_INTEGER,	 GL_SHORT,							SAMPLER_INT,	{ {16, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_R16UI,				 GL_RED,			 GL_RED_INTEGER,	 GL_UNSIGNED_SHORT,					SAMPLER_UINT,   { {16, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_R32I,				 GL_RED,			 GL_RED_INTEGER,	 GL_INT,							SAMPLER_INT,	{ {32, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_R32UI,				 GL_RED,			 GL_RED_INTEGER,	 GL_UNSIGNED_INT,					SAMPLER_UINT,   { {32, 0, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RG8I,				 GL_RG,				 GL_RG_INTEGER,		 GL_BYTE,							SAMPLER_INT,	{ { 8, 8, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RG8UI,				 GL_RG,				 GL_RG_INTEGER,		 GL_UNSIGNED_BYTE,					SAMPLER_UINT,   { { 8, 8, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RG16I,				 GL_RG,				 GL_RG_INTEGER,		 GL_SHORT,							SAMPLER_INT,	{ {16,16, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RG16UI,			 GL_RG,				 GL_RG_INTEGER,		 GL_UNSIGNED_SHORT,					SAMPLER_UINT,   { {16,16, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RG32I,				 GL_RG,				 GL_RG_INTEGER,		 GL_INT,							SAMPLER_INT,	{ {32,32, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RG32UI,			 GL_RG,				 GL_RG_INTEGER,		 GL_UNSIGNED_INT,					SAMPLER_UINT,   { {32,32, 0, 0, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RGB8I,				 GL_RGB,			 GL_RGB_INTEGER,	 GL_BYTE,							SAMPLER_INT,	{ { 8, 8, 8, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB8UI,			 GL_RGB,			 GL_RGB_INTEGER,	 GL_UNSIGNED_BYTE,					SAMPLER_UINT,   { { 8, 8, 8, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB16I,			 GL_RGB,			 GL_RGB_INTEGER,	 GL_SHORT,							SAMPLER_INT,	{ {16,16,16, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB16UI,			 GL_RGB,			 GL_RGB_INTEGER,	 GL_UNSIGNED_SHORT,					SAMPLER_UINT,   { {16,16,16, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB32I,			 GL_RGB,			 GL_RGB_INTEGER,	 GL_INT,							SAMPLER_INT,	{ {32,32,32, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGB32UI,			 GL_RGB,			 GL_RGB_INTEGER,	 GL_UNSIGNED_INT,					SAMPLER_UINT,   { {32,32,32, 0, 0, 0, 0, 0, 0 } }, 0 },
	{ GL_RGBA8I,			 GL_RGBA,			 GL_RGBA_INTEGER,	 GL_BYTE,							SAMPLER_INT,	{ { 8, 8, 8, 8, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RGBA8UI,			 GL_RGBA,			 GL_RGBA_INTEGER,	 GL_UNSIGNED_BYTE,					SAMPLER_UINT,   { { 8, 8, 8, 8, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RGBA16I,			 GL_RGBA,			 GL_RGBA_INTEGER,	 GL_SHORT,							SAMPLER_INT,	{ {16,16,16,16, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RGBA16UI,			 GL_RGBA,			 GL_RGBA_INTEGER,	 GL_UNSIGNED_SHORT,					SAMPLER_UINT,   { {16,16,16,16, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RGBA32I,			 GL_RGBA,			 GL_RGBA_INTEGER,	 GL_INT,							SAMPLER_INT,	{ {32,32,32,32, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_RGBA32UI,			 GL_RGBA,			 GL_RGBA_INTEGER,	 GL_UNSIGNED_INT,					SAMPLER_UINT,   { {32,32,32,32, 0, 0, 0, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_DEPTH_COMPONENT16,	 GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT,					SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 0,16, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_DEPTH_COMPONENT24,	 GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,					SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 0,24, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT,							SAMPLER_FLOAT,  { { 0, 0, 0, 0, 0, 0,32, 0, 0 } }, FLAG_REQ_RBO },
	{ GL_DEPTH24_STENCIL8,	 GL_DEPTH_STENCIL,   GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8,				SAMPLER_UNORM,  { { 0, 0, 0, 0, 0, 0,24, 8, 0 } }, FLAG_REQ_RBO },
	{ GL_DEPTH32F_STENCIL8,	 GL_DEPTH_STENCIL,   GL_DEPTH_STENCIL,   GL_FLOAT_32_UNSIGNED_INT_24_8_REV, SAMPLER_FLOAT,  { { 0, 0, 0, 0, 0, 0,32, 8, 0 } }, FLAG_PACKED|FLAG_REQ_RBO },
};

static const PixelFormat coreFormats[] = {
	{ GL_STENCIL_INDEX,   1, FORMAT_STENCIL,	   GL_STENCIL_ATTACHMENT,		{ {-1,-1,-1,-1,-1,-1,-1, 0,-1} } },
	{ GL_DEPTH_COMPONENT, 1, FORMAT_DEPTH,		   GL_DEPTH_ATTACHMENT,			{ {-1,-1,-1,-1,-1,-1, 0,-1,-1} } },
	{ GL_DEPTH_STENCIL,   2, FORMAT_DEPTH_STENCIL, GL_DEPTH_STENCIL_ATTACHMENT, { {-1,-1,-1,-1,-1,-1, 0, 1,-1} } },
	{ GL_RED,			  1, FORMAT_COLOR,		   GL_COLOR_ATTACHMENT0,		{ { 0,-1,-1,-1,-1,-1,-1,-1,-1} } },
	{ GL_GREEN,			  1, FORMAT_COLOR,		   GL_COLOR_ATTACHMENT0,		{ {-1, 0,-1,-1,-1,-1,-1,-1,-1} } },
	{ GL_BLUE,			  1, FORMAT_COLOR,		   GL_COLOR_ATTACHMENT0,		{ {-1,-1, 0,-1,-1,-1,-1,-1,-1} } },
	{ GL_RG,			  2, FORMAT_COLOR,		   GL_COLOR_ATTACHMENT0,		{ { 0, 1,-1,-1,-1,-1,-1,-1,-1} } },
	{ GL_RGB,			  3, FORMAT_COLOR,		   GL_COLOR_ATTACHMENT0,		{ { 0, 1, 2,-1,-1,-1,-1,-1,-1} } },
	{ GL_RGBA,			  4, FORMAT_COLOR,		   GL_COLOR_ATTACHMENT0,		{ { 0, 1, 2, 3,-1,-1,-1,-1,-1} } },
	{ GL_BGR,			  3, FORMAT_COLOR,		   GL_COLOR_ATTACHMENT0,		{ { 2, 1, 0,-1,-1,-1,-1,-1,-1} } },
	{ GL_BGRA,			  4, FORMAT_COLOR,		   GL_COLOR_ATTACHMENT0,		{ { 2, 1, 0, 3,-1,-1,-1,-1,-1} } },
	{ GL_RED_INTEGER,	  1, FORMAT_COLOR_INTEGER, GL_COLOR_ATTACHMENT0,		{ { 0,-1,-1,-1,-1,-1,-1,-1,-1} } },
	{ GL_GREEN_INTEGER,   1, FORMAT_COLOR_INTEGER, GL_COLOR_ATTACHMENT0,		{ {-1, 0,-1,-1,-1,-1,-1,-1,-1} } },
	{ GL_BLUE_INTEGER,	  1, FORMAT_COLOR_INTEGER, GL_COLOR_ATTACHMENT0,		{ {-1,-1, 0,-1,-1,-1,-1,-1,-1} } },
	{ GL_RG_INTEGER,	  2, FORMAT_COLOR_INTEGER, GL_COLOR_ATTACHMENT0,		{ { 0, 1,-1,-1,-1,-1,-1,-1,-1} } },
	{ GL_RGB_INTEGER,	  3, FORMAT_COLOR_INTEGER, GL_COLOR_ATTACHMENT0,		{ { 0, 1, 2,-1,-1,-1,-1,-1,-1} } },
	{ GL_RGBA_INTEGER,	  4, FORMAT_COLOR_INTEGER, GL_COLOR_ATTACHMENT0,		{ { 0, 1, 2, 3,-1,-1,-1,-1,-1} } },
	{ GL_BGR_INTEGER,	  3, FORMAT_COLOR_INTEGER, GL_COLOR_ATTACHMENT0,		{ { 2, 1, 0,-1,-1,-1,-1,-1,-1} } },
	{ GL_BGRA_INTEGER,	  4, FORMAT_COLOR_INTEGER, GL_COLOR_ATTACHMENT0,		{ { 2, 1, 0, 3,-1,-1,-1,-1,-1} } },
};

static const PixelFormat esFormats[] = {
	{ GL_DEPTH_COMPONENT, 1, FORMAT_DEPTH,		   GL_DEPTH_ATTACHMENT,			{ {-1,-1,-1,-1,-1,-1, 0,-1,-1} } },
	{ GL_DEPTH_STENCIL,   2, FORMAT_DEPTH_STENCIL, GL_DEPTH_STENCIL_ATTACHMENT, { {-1,-1,-1,-1,-1,-1, 0, 1,-1} } },
	{ GL_RED,			  1, FORMAT_COLOR,		   GL_COLOR_ATTACHMENT0,		{ { 0,-1,-1,-1,-1,-1,-1,-1,-1} } },
	{ GL_RG,			  2, FORMAT_COLOR,		   GL_COLOR_ATTACHMENT0,		{ { 0, 1,-1,-1,-1,-1,-1,-1,-1} } },
	{ GL_RGB,			  3, FORMAT_COLOR,		   GL_COLOR_ATTACHMENT0,		{ { 0, 1, 2,-1,-1,-1,-1,-1,-1} } },
	{ GL_RGBA,			  4, FORMAT_COLOR,		   GL_COLOR_ATTACHMENT0,		{ { 0, 1, 2, 3,-1,-1,-1,-1,-1} } },
	{ GL_LUMINANCE,		  1, FORMAT_COLOR,		   GL_COLOR_ATTACHMENT0,		{ {-1,-1,-1,-1,-1, 0,-1,-1,-1} } },
	{ GL_ALPHA,			  1, FORMAT_COLOR,		   GL_COLOR_ATTACHMENT0,		{ {-1,-1,-1, 0,-1,-1,-1,-1,-1} } },
	{ GL_LUMINANCE_ALPHA, 2, FORMAT_COLOR,		   GL_COLOR_ATTACHMENT0,		{ {-1,-1,-1, 1,-1, 0,-1,-1,-1} } },
	{ GL_RED_INTEGER,	  1, FORMAT_COLOR_INTEGER, GL_COLOR_ATTACHMENT0,		{ { 0,-1,-1,-1,-1,-1,-1,-1,-1} } },
	{ GL_RG_INTEGER,	  2, FORMAT_COLOR_INTEGER, GL_COLOR_ATTACHMENT0,		{ { 0, 1,-1,-1,-1,-1,-1,-1,-1} } },
	{ GL_RGB_INTEGER,	  3, FORMAT_COLOR_INTEGER, GL_COLOR_ATTACHMENT0,		{ { 0, 1, 2,-1,-1,-1,-1,-1,-1} } },
	{ GL_RGBA_INTEGER,	  4, FORMAT_COLOR_INTEGER, GL_COLOR_ATTACHMENT0,		{ { 0, 1, 2, 3,-1,-1,-1,-1,-1} } },
};

static const PixelType coreTypes[] = {
	{ GL_UNSIGNED_BYTE,					 sizeof(GLubyte),				 STORAGE_UNSIGNED, false, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, true },
	{ GL_BYTE,							 sizeof(GLbyte),				 STORAGE_SIGNED,   false, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, true },
	{ GL_UNSIGNED_SHORT,				 sizeof(GLushort),				 STORAGE_UNSIGNED, false, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, true },
	{ GL_SHORT,							 sizeof(GLshort),				 STORAGE_SIGNED,   false, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, true },
	{ GL_UNSIGNED_INT,					 sizeof(GLuint),				 STORAGE_UNSIGNED, false, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, false },
	{ GL_INT,							 sizeof(GLint),					 STORAGE_SIGNED,   false, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, false },
	{ GL_HALF_FLOAT,					 sizeof(GLhalf),				 STORAGE_FLOAT,	   false, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, false },
	{ GL_FLOAT,							 sizeof(GLfloat),				 STORAGE_FLOAT,	   false, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_SHORT_5_6_5,			 sizeof(GLushort),				 STORAGE_UNSIGNED, true,  false, { { 5, 6, 5, 0, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_SHORT_4_4_4_4,		 sizeof(GLushort),				 STORAGE_UNSIGNED, true,  false, { { 4, 4, 4, 4, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_SHORT_5_5_5_1,		 sizeof(GLushort),				 STORAGE_UNSIGNED, true,  false, { { 5, 5, 5, 1, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_INT_2_10_10_10_REV,	 sizeof(GLuint),				 STORAGE_UNSIGNED, true,  true,  { {10,10,10, 2, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_INT_24_8,				 sizeof(GLuint),				 STORAGE_UNSIGNED, true,  false, { { 0, 0, 0, 0, 0, 0,24, 8, 0 } }, false },
	{ GL_UNSIGNED_INT_10F_11F_11F_REV,	 sizeof(GLuint),				 STORAGE_FLOAT,	   true,  true,  { { 6, 7, 7, 0, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_INT_5_9_9_9_REV,		 sizeof(GLuint),				 STORAGE_FLOAT,	   true,  true,  { { 9, 9, 9, 5, 0, 0, 0, 0, 0 } }, false },
	{ GL_FLOAT_32_UNSIGNED_INT_24_8_REV, sizeof(GLfloat)+sizeof(GLuint), STORAGE_FLOAT,	   true,  true,  { { 0, 0, 0, 0, 0, 0,32, 8,24 } }, false },
	{ GL_UNSIGNED_BYTE_3_3_2,			 sizeof(GLubyte),				 STORAGE_UNSIGNED, true,  false, { { 3, 3, 2, 0, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_BYTE_2_3_3_REV,		 sizeof(GLubyte),				 STORAGE_UNSIGNED, true,  true,  { { 3, 3, 2, 0, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_SHORT_5_6_5_REV,		 sizeof(GLushort),				 STORAGE_UNSIGNED, true,  true,  { { 5, 6, 5, 0, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_SHORT_4_4_4_4_REV,	 sizeof(GLushort),				 STORAGE_UNSIGNED, true,  true,  { { 4, 4, 4, 4, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_SHORT_1_5_5_5_REV,	 sizeof(GLushort),				 STORAGE_UNSIGNED, true,  true,  { { 5, 5, 5, 1, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_INT_8_8_8_8,			 sizeof(GLuint),				 STORAGE_UNSIGNED, true,  false, { { 8, 8, 8, 8, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_INT_8_8_8_8_REV,		 sizeof(GLuint),				 STORAGE_UNSIGNED, true,  true,  { { 8, 8, 8, 8, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_INT_10_10_10_2,		 sizeof(GLuint),				 STORAGE_UNSIGNED, true,  true,  { {10,10,10, 2, 0, 0, 0, 0, 0 } }, false },
};

static const PixelType esTypes[] = {
	{ GL_UNSIGNED_BYTE,					 sizeof(GLubyte),				 STORAGE_UNSIGNED, false, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, true },
	{ GL_BYTE,							 sizeof(GLbyte),				 STORAGE_SIGNED,   false, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, true },
	{ GL_UNSIGNED_SHORT,				 sizeof(GLushort),				 STORAGE_UNSIGNED, false, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, true },
	{ GL_SHORT,							 sizeof(GLshort),				 STORAGE_SIGNED,   false, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, true },
	{ GL_UNSIGNED_INT,					 sizeof(GLuint),				 STORAGE_UNSIGNED, false, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, false },
	{ GL_INT,							 sizeof(GLint),					 STORAGE_SIGNED,   false, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, false },
	{ GL_HALF_FLOAT,					 sizeof(GLhalf),				 STORAGE_FLOAT,	   false, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, false },
	{ GL_FLOAT,							 sizeof(GLfloat),				 STORAGE_FLOAT,	   false, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_SHORT_5_6_5,			 sizeof(GLushort),				 STORAGE_UNSIGNED, true,  false, { { 5, 6, 5, 0, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_SHORT_4_4_4_4,		 sizeof(GLushort),				 STORAGE_UNSIGNED, true,  false, { { 4, 4, 4, 4, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_SHORT_5_5_5_1,		 sizeof(GLushort),				 STORAGE_UNSIGNED, true,  false, { { 5, 5, 5, 1, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_INT_2_10_10_10_REV,	 sizeof(GLuint),				 STORAGE_UNSIGNED, true,  true,  { {10,10,10, 2, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_INT_24_8,				 sizeof(GLuint),				 STORAGE_UNSIGNED, true,  false, { { 0, 0, 0, 0, 0, 0,24, 8, 0 } }, false },
	{ GL_UNSIGNED_INT_10F_11F_11F_REV,	 sizeof(GLuint),				 STORAGE_FLOAT,	   true,  true,  { { 6, 7, 7, 0, 0, 0, 0, 0, 0 } }, false },
	{ GL_UNSIGNED_INT_5_9_9_9_REV,		 sizeof(GLuint),				 STORAGE_FLOAT,	   true,  true,  { { 9, 9, 9, 5, 0, 0, 0, 0, 0 } }, false },
	{ GL_FLOAT_32_UNSIGNED_INT_24_8_REV, sizeof(GLfloat)+sizeof(GLuint), STORAGE_FLOAT,	   true,  true,  { { 0, 0, 0, 0, 0, 0,32, 8,24 } }, false },
};

static const EnumFormats esValidFormats[] = {
	{ GL_RGBA8,				 GL_RGBA,			 GL_UNSIGNED_BYTE,					4, true },
	{ GL_RGB5_A1,			 GL_RGBA,			 GL_UNSIGNED_BYTE,					4, true },
	{ GL_RGBA4,				 GL_RGBA,			 GL_UNSIGNED_BYTE,					4, true },
	{ GL_SRGB8_ALPHA8,		 GL_RGBA,			 GL_UNSIGNED_BYTE,					4, true },
	{ GL_RGBA8_SNORM,		 GL_RGBA,			 GL_BYTE,							4, false },
	{ GL_RGBA4,				 GL_RGBA,			 GL_UNSIGNED_SHORT_4_4_4_4,			2, true },
	{ GL_RGB5_A1,			 GL_RGBA,			 GL_UNSIGNED_SHORT_5_5_5_1,			2, true },
	{ GL_RGB10_A2,			 GL_RGBA,			 GL_UNSIGNED_INT_2_10_10_10_REV,	4, true },
	{ GL_RGB5_A1,			 GL_RGBA,			 GL_UNSIGNED_INT_2_10_10_10_REV,	4, true },
	{ GL_RGBA16F,			 GL_RGBA,			 GL_HALF_FLOAT,						8, false },
	{ GL_RGBA32F,			 GL_RGBA,			 GL_FLOAT,						   16, false },
	{ GL_RGBA16F,			 GL_RGBA,			 GL_FLOAT,						   16, false },
	{ GL_RGBA8UI,			 GL_RGBA_INTEGER,	 GL_UNSIGNED_BYTE,					4, true },
	{ GL_RGBA8I,			 GL_RGBA_INTEGER,	 GL_BYTE,							4, true },
	{ GL_RGBA16UI,			 GL_RGBA_INTEGER,	 GL_UNSIGNED_SHORT,					8, true },
	{ GL_RGBA16I,			 GL_RGBA_INTEGER,	 GL_SHORT,							8, true },
	{ GL_RGBA32UI,			 GL_RGBA_INTEGER,	 GL_UNSIGNED_INT,				   16, true },
	{ GL_RGBA32I,			 GL_RGBA_INTEGER,	 GL_INT,						   16, true },
	{ GL_RGB10_A2UI,		 GL_RGBA_INTEGER,	 GL_UNSIGNED_INT_2_10_10_10_REV,	4, true },
	{ GL_RGB8,				 GL_RGB,			 GL_UNSIGNED_BYTE,					3, true },
	{ GL_RGB565,			 GL_RGB,			 GL_UNSIGNED_BYTE,					3, true },
	{ GL_SRGB8,				 GL_RGB,			 GL_UNSIGNED_BYTE,					3, false },
	{ GL_RGB8_SNORM,		 GL_RGB,			 GL_BYTE,							3, false },
	{ GL_RGB565,			 GL_RGB,			 GL_UNSIGNED_SHORT_5_6_5,			2, true },
	{ GL_R11F_G11F_B10F,	 GL_RGB,			 GL_UNSIGNED_INT_10F_11F_11F_REV,	4, false },
	{ GL_R11F_G11F_B10F,	 GL_RGB,			 GL_HALF_FLOAT,						6, false },
	{ GL_R11F_G11F_B10F,	 GL_RGB,			 GL_FLOAT,						   12, false },
	{ GL_RGB9_E5,			 GL_RGB,			 GL_UNSIGNED_INT_5_9_9_9_REV,		4, false },
	{ GL_RGB9_E5,			 GL_RGB,			 GL_HALF_FLOAT,						6, false },
	{ GL_RGB9_E5,			 GL_RGB,			 GL_FLOAT,						   12, false },
	{ GL_RGB16F,			 GL_RGB,			 GL_HALF_FLOAT,						6, false },
	{ GL_RGB32F,			 GL_RGB,			 GL_FLOAT,						   12, false },
	{ GL_RGB16F,			 GL_RGB,			 GL_FLOAT,						   12, false },
	{ GL_RGB8UI,			 GL_RGB_INTEGER,	 GL_UNSIGNED_BYTE,					3, false },
	{ GL_RGB8I,				 GL_RGB_INTEGER,	 GL_BYTE,							3, false },
	{ GL_RGB16UI,			 GL_RGB_INTEGER,	 GL_UNSIGNED_SHORT,					6, false },
	{ GL_RGB16I,			 GL_RGB_INTEGER,	 GL_SHORT,							6, false },
	{ GL_RGB32UI,			 GL_RGB_INTEGER,	 GL_UNSIGNED_INT,				   12, false },
	{ GL_RGB32I,			 GL_RGB_INTEGER,	 GL_INT,						   12, false },
	{ GL_RG8,				 GL_RG,				 GL_UNSIGNED_BYTE,					2, true },
	{ GL_RG8_SNORM,			 GL_RG,				 GL_BYTE,							2, false },
	{ GL_RG16F,				 GL_RG,				 GL_HALF_FLOAT,						4, false },
	{ GL_RG32F,				 GL_RG,				 GL_FLOAT,							8, false },
	{ GL_RG16F,				 GL_RG,				 GL_FLOAT,							8, false },
	{ GL_RG8UI,				 GL_RG_INTEGER,		 GL_UNSIGNED_BYTE,					2, true },
	{ GL_RG8I,				 GL_RG_INTEGER,		 GL_BYTE,							2, true },
	{ GL_RG16UI,			 GL_RG_INTEGER,		 GL_UNSIGNED_SHORT,					4, true },
	{ GL_RG16I,				 GL_RG_INTEGER,		 GL_SHORT,							4, true },
	{ GL_RG32UI,			 GL_RG_INTEGER,		 GL_UNSIGNED_INT,					8, true },
	{ GL_RG32I,				 GL_RG_INTEGER,		 GL_INT,							8, true },
	{ GL_R8,				 GL_RED,			 GL_UNSIGNED_BYTE,					1, true },
	{ GL_R8_SNORM,			 GL_RED,			 GL_BYTE,							1, false },
	{ GL_R16F,				 GL_RED,			 GL_HALF_FLOAT,						2, false },
	{ GL_R32F,				 GL_RED,			 GL_FLOAT,							4, false },
	{ GL_R16F,				 GL_RED,			 GL_FLOAT,							4, false },
	{ GL_R8UI,				 GL_RED_INTEGER,	 GL_UNSIGNED_BYTE,					1, true },
	{ GL_R8I,				 GL_RED_INTEGER,	 GL_BYTE,							1, true },
	{ GL_R16UI,				 GL_RED_INTEGER,	 GL_UNSIGNED_SHORT,					2, true },
	{ GL_R16I,				 GL_RED_INTEGER,	 GL_SHORT,							2, true },
	{ GL_R32UI,				 GL_RED_INTEGER,	 GL_UNSIGNED_INT,					4, true },
	{ GL_R32I,				 GL_RED_INTEGER,	 GL_INT,							4, true },
	{ GL_DEPTH_COMPONENT24,  GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,					4, true },
	{ GL_DEPTH_COMPONENT16,  GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,					4, true },
	{ GL_DEPTH_COMPONENT16,  GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT,					2, true },
	{ GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT,							4, true },
	{ GL_DEPTH24_STENCIL8,   GL_DEPTH_STENCIL,	 GL_UNSIGNED_INT_24_8,				4, true },
	{ GL_DEPTH32F_STENCIL8,  GL_DEPTH_STENCIL,	 GL_FLOAT_32_UNSIGNED_INT_24_8_REV, 8, true },
	{ GL_RGBA,				 GL_RGBA,			 GL_UNSIGNED_BYTE,					4, true },
	{ GL_RGBA,				 GL_RGBA,			 GL_UNSIGNED_SHORT_4_4_4_4,			2, true },
	{ GL_RGBA,				 GL_RGBA,			 GL_UNSIGNED_SHORT_5_5_5_1,			2, true },
	{ GL_RGB,				 GL_RGB,			 GL_UNSIGNED_BYTE,					3, true },
	{ GL_RGB,				 GL_RGB,			 GL_UNSIGNED_SHORT_5_6_5,			2, true },
	{ GL_LUMINANCE_ALPHA,	 GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,					2, false },
	{ GL_LUMINANCE,			 GL_LUMINANCE,		 GL_UNSIGNED_BYTE,					1, false },
	{ GL_ALPHA,				 GL_ALPHA,			 GL_UNSIGNED_BYTE,					1, false },
};

static const EnumFormats coreValidFormats[] = {
	{ GL_RGB,			GL_RGB,			  GL_UNSIGNED_BYTE_3_3_2,			 3, true },
	{ GL_RGB_INTEGER,	GL_RGB_INTEGER,   GL_UNSIGNED_BYTE_3_3_2,			 3, true },
	{ GL_RGB,			GL_RGB,			  GL_UNSIGNED_BYTE_2_3_3_REV,		 3, true },
	{ GL_RGB_INTEGER,	GL_RGB_INTEGER,   GL_UNSIGNED_BYTE_2_3_3_REV,		 3, true },
	{ GL_RGB,			GL_RGB,			  GL_UNSIGNED_SHORT_5_6_5,			 3, true },
	{ GL_RGB_INTEGER,	GL_RGB_INTEGER,   GL_UNSIGNED_SHORT_5_6_5,			 3, true },
	{ GL_RGB,			GL_RGB,			  GL_UNSIGNED_SHORT_5_6_5_REV,		 3, true },
	{ GL_RGB_INTEGER,	GL_RGB_INTEGER,   GL_UNSIGNED_SHORT_5_6_5_REV,		 3, true },
	{ GL_RGBA,			GL_RGBA,		  GL_UNSIGNED_SHORT_4_4_4_4,		 4, true },
	{ GL_RGBA,			GL_RGBA,		  GL_UNSIGNED_SHORT_4_4_4_4_REV,	 4, true },
	{ GL_RGBA_INTEGER,	GL_RGBA_INTEGER,  GL_UNSIGNED_SHORT_4_4_4_4,		 4, true },
	{ GL_RGBA_INTEGER,	GL_RGBA_INTEGER,  GL_UNSIGNED_SHORT_4_4_4_4_REV,	 4, true },
	{ GL_BGRA,			GL_BGRA,		  GL_UNSIGNED_SHORT_4_4_4_4_REV,	 4, true },
	{ GL_BGRA,			GL_BGRA,		  GL_UNSIGNED_SHORT_4_4_4_4,		 4, true },
	{ GL_BGRA_INTEGER,	GL_BGRA_INTEGER,  GL_UNSIGNED_SHORT_4_4_4_4_REV,	 4, true },
	{ GL_BGRA_INTEGER,	GL_BGRA_INTEGER,  GL_UNSIGNED_SHORT_4_4_4_4,		 4, true },
	{ GL_RGBA,			GL_RGBA,		  GL_UNSIGNED_SHORT_5_5_5_1,		 4, true },
	{ GL_BGRA,			GL_BGRA,		  GL_UNSIGNED_SHORT_5_5_5_1,		 4, true },
	{ GL_RGBA_INTEGER,  GL_RGBA_INTEGER,  GL_UNSIGNED_SHORT_5_5_5_1,		 4, true },
	{ GL_BGRA_INTEGER,  GL_BGRA_INTEGER,  GL_UNSIGNED_SHORT_5_5_5_1,		 4, true },
	{ GL_RGBA,			GL_RGBA,		  GL_UNSIGNED_SHORT_1_5_5_5_REV,	 4, true },
	{ GL_BGRA,			GL_BGRA,		  GL_UNSIGNED_SHORT_1_5_5_5_REV,	 4, true },
	{ GL_RGBA_INTEGER,  GL_RGBA_INTEGER,  GL_UNSIGNED_SHORT_1_5_5_5_REV,	 4, true },
	{ GL_BGRA_INTEGER,  GL_BGRA_INTEGER,  GL_UNSIGNED_SHORT_1_5_5_5_REV,	 4, true },
	{ GL_RGBA,			GL_RGBA,		  GL_UNSIGNED_INT_8_8_8_8,			 4, true },
	{ GL_BGRA,			GL_BGRA,		  GL_UNSIGNED_INT_8_8_8_8,			 4, true },
	{ GL_RGBA_INTEGER,  GL_RGBA_INTEGER,  GL_UNSIGNED_INT_8_8_8_8,			 4, true },
	{ GL_BGRA_INTEGER,  GL_BGRA_INTEGER,  GL_UNSIGNED_INT_8_8_8_8,			 4, true },
	{ GL_RGBA,			GL_RGBA,		  GL_UNSIGNED_INT_8_8_8_8_REV,		 4, true },
	{ GL_BGRA,			GL_BGRA,		  GL_UNSIGNED_INT_8_8_8_8_REV,		 4, true },
	{ GL_RGBA_INTEGER,  GL_RGBA_INTEGER,  GL_UNSIGNED_INT_8_8_8_8_REV,		 4, true },
	{ GL_BGRA_INTEGER,  GL_BGRA_INTEGER,  GL_UNSIGNED_INT_8_8_8_8_REV,		 4, true },
	{ GL_RGBA,			GL_RGBA,		  GL_UNSIGNED_INT_10_10_10_2,		 4, true },
	{ GL_BGRA,			GL_BGRA,		  GL_UNSIGNED_INT_10_10_10_2,		 4, true },
	{ GL_RGBA_INTEGER, GL_RGBA_INTEGER,   GL_UNSIGNED_INT_10_10_10_2,		 4, true },
	{ GL_BGRA_INTEGER, GL_BGRA_INTEGER,   GL_UNSIGNED_INT_10_10_10_2,		 4, true },
	{ GL_RGBA,			GL_RGBA,		  GL_UNSIGNED_INT_2_10_10_10_REV,	 4, true },
	{ GL_BGRA,			GL_BGRA,		  GL_UNSIGNED_INT_2_10_10_10_REV,	 4, true },
	{ GL_RGBA_INTEGER, GL_RGBA_INTEGER,   GL_UNSIGNED_INT_2_10_10_10_REV,	 4, true },
	{ GL_BGRA_INTEGER, GL_BGRA_INTEGER,   GL_UNSIGNED_INT_2_10_10_10_REV,	 4, true },
	{ GL_DEPTH_STENCIL, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,				 2, true },
	{ GL_RGB,			GL_RGB,			  GL_UNSIGNED_INT_10F_11F_11F_REV,	 3, true },
	{ GL_RGB,			GL_RGB,			  GL_UNSIGNED_INT_5_9_9_9_REV,		 4, true },
	{ GL_DEPTH_STENCIL, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, 2, true },
};

static const EnumFormats validformats_EXT_texture_type_2_10_10_10_REV[] = {
	{ GL_RGBA, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV_EXT, 4, false },
	{ GL_RGB, GL_RGB, GL_UNSIGNED_INT_2_10_10_10_REV_EXT, 3, false }
};

// Valid combinations given by GL_EXT_texture_type_2_10_10_10_REV and
// GL_OES_required_internalformat extensions
static const EnumFormats validformats_OES_required_internalformat[] = {
	{ GL_RGB8_OES, GL_RGB, GL_UNSIGNED_INT_2_10_10_10_REV_EXT, 3, true },
	{ GL_RGB565, GL_RGB, GL_UNSIGNED_INT_2_10_10_10_REV_EXT, 4, true }
};

// Companion type for GL_FLOAT_32_UNSIGNED_INT_24_8_REV. Stencil part was
// not split into 24/8 to avoid any packing related issues from compiler.
struct F_32_UINT_24_8_REV
{
	GLfloat d;
	GLuint  s;
};

// custom pixel data type. holds both float and integer pixel data. memory consuming, but
// it is not that relavant in this case. makes comparing more reliable and flexible
struct FloatPixel
{
	int i_r;
	int i_g;
	int i_b;
	int i_a;
	int i_d;
	int i_s;

	unsigned int ui_r;
	unsigned int ui_g;
	unsigned int ui_b;
	unsigned int ui_a;
	unsigned int ui_d;
	unsigned int ui_s;

	float r;
	float g;
	float b;
	float a;
	float d;
	float s;
};

static const int NUM_FLOAT_PIXEL_COUNT = sizeof(FloatPixel) / sizeof(float);

typedef int			 rawIntPixel[4];
typedef unsigned int rawUintPixel[4];
typedef float		 rawFloatPixel[4];

struct PackedPixelsBufferProperties
{
	int elementsInGroup;	  // number of elements in a group
	int rowLength;			  // number of groups in the row
	int alignment;			  // alignment (in bytes)
	int elementSize;		  // size of an element (in bytes)
	int elementsInRow;		  // row size (in elements)
	int elementsInRowNoAlign; // row size (in elements) without alignment
	int rowCount;			  // number of rows in 2D image
	int imagesCount;		  // number of 2D images in 3D image
	int skipPixels;			  // (UN)PACK_SKIP_PIXELS
	int skipRows;			  // (UN)PACK_SKIP_ROWS
	int skipImages;			  // (UN)PACK_SKIP_IMAGES
	int swapBytes;
	int lsbFirst;
};

std::string getTypeStr(GLenum type)
{
	// this function extends glu::getTypeStr by types used in this tests

	typedef std::map<GLenum, std::string> TypeMap;
	static TypeMap typeMap;
	if (typeMap.empty())
	{
		typeMap[GL_UNSIGNED_BYTE_3_3_2]		   = "GL_UNSIGNED_BYTE_3_3_2";
		typeMap[GL_UNSIGNED_BYTE_2_3_3_REV]	= "GL_UNSIGNED_BYTE_2_3_3_REV";
		typeMap[GL_UNSIGNED_SHORT_5_6_5_REV]   = "GL_UNSIGNED_SHORT_5_6_5_REV";
		typeMap[GL_UNSIGNED_SHORT_4_4_4_4_REV] = "GL_UNSIGNED_SHORT_4_4_4_4_REV";
		typeMap[GL_UNSIGNED_SHORT_1_5_5_5_REV] = "GL_UNSIGNED_SHORT_1_5_5_5_REV";
		typeMap[GL_UNSIGNED_INT_8_8_8_8]	   = "GL_UNSIGNED_INT_8_8_8_8";
		typeMap[GL_UNSIGNED_INT_8_8_8_8_REV]   = "GL_UNSIGNED_INT_8_8_8_8_REV";
		typeMap[GL_UNSIGNED_INT_10_10_10_2]	= "GL_UNSIGNED_INT_10_10_10_2";
	}

	TypeMap::iterator it = typeMap.find(type);
	if (it == typeMap.end())
	{
		// if type is not in map use glu function
		return glu::getTypeStr(type).toString();
	}
	return it->second;
}

std::string getFormatStr(GLenum format)
{
	// this function extends glu::getTextureFormatStr by types used in this tests

	typedef std::map<GLenum, std::string> FormatMap;
	static FormatMap formatMap;
	if (formatMap.empty())
	{
		formatMap[GL_GREEN]						  = "GL_GREEN";
		formatMap[GL_BLUE]						  = "GL_BLUE";
		formatMap[GL_GREEN_INTEGER]				  = "GL_GREEN_INTEGER";
		formatMap[GL_BLUE_INTEGER]				  = "GL_BLUE_INTEGER";
		formatMap[GL_BGR]						  = "GL_BGR";
		formatMap[GL_BGR_INTEGER]				  = "GL_BGR_INTEGER";
		formatMap[GL_BGRA_INTEGER]				  = "GL_BGRA_INTEGER";
		formatMap[GL_R3_G3_B2]					  = "GL_R3_G3_B2";
		formatMap[GL_RGB4]						  = "GL_RGB4";
		formatMap[GL_RGB5]						  = "GL_RGB5";
		formatMap[GL_RGB12]						  = "GL_RGB12";
		formatMap[GL_RGBA2]						  = "GL_RGBA2";
		formatMap[GL_RGBA12]					  = "GL_RGBA12";
		formatMap[GL_COMPRESSED_RED]			  = "GL_COMPRESSED_RED";
		formatMap[GL_COMPRESSED_RG]				  = "GL_COMPRESSED_RG";
		formatMap[GL_COMPRESSED_RGB]			  = "GL_COMPRESSED_RGB";
		formatMap[GL_COMPRESSED_RGBA]			  = "GL_COMPRESSED_RGBA";
		formatMap[GL_COMPRESSED_SRGB]			  = "GL_COMPRESSED_SRGB";
		formatMap[GL_COMPRESSED_SRGB_ALPHA]		  = "GL_COMPRESSED_SRGB_ALPHA";
		formatMap[GL_COMPRESSED_RED_RGTC1]		  = "GL_COMPRESSED_RED_RGTC1";
		formatMap[GL_COMPRESSED_SIGNED_RED_RGTC1] = "GL_COMPRESSED_SIGNED_RED_RGTC1";
		formatMap[GL_COMPRESSED_RG_RGTC2]		  = "GL_COMPRESSED_RG_RGTC2";
		formatMap[GL_COMPRESSED_SIGNED_RG_RGTC2]  = "GL_COMPRESSED_SIGNED_RG_RGTC2";
		formatMap[GL_STENCIL_INDEX]				  = "GL_STENCIL_INDEX";
	}

	FormatMap::iterator it = formatMap.find(format);
	if (it == formatMap.end())
	{
		// if format is not in map use glu function
		return glu::getTextureFormatStr(format).toString();
	}
	return it->second;
}

std::string getModeStr(GLenum type)
{
	typedef std::map<GLenum, std::string> ModeMap;
	static ModeMap modeMap;
	if (modeMap.empty())
	{
		modeMap[GL_UNPACK_ROW_LENGTH]   = "GL_UNPACK_ROW_LENGTH";
		modeMap[GL_UNPACK_SKIP_ROWS]	= "GL_UNPACK_SKIP_ROWS";
		modeMap[GL_UNPACK_SKIP_PIXELS]  = "GL_UNPACK_SKIP_PIXELS";
		modeMap[GL_UNPACK_ALIGNMENT]	= "GL_UNPACK_ALIGNMENT";
		modeMap[GL_UNPACK_IMAGE_HEIGHT] = "GL_UNPACK_IMAGE_HEIGHT";
		modeMap[GL_UNPACK_SKIP_IMAGES]  = "GL_UNPACK_SKIP_IMAGES";
		modeMap[GL_PACK_ROW_LENGTH]		= "GL_PACK_ROW_LENGTH";
		modeMap[GL_PACK_SKIP_ROWS]		= "GL_PACK_SKIP_ROWS";
		modeMap[GL_PACK_SKIP_PIXELS]	= "GL_PACK_SKIP_PIXELS";
		modeMap[GL_PACK_ALIGNMENT]		= "GL_PACK_ALIGNMENT";
		modeMap[GL_UNPACK_SWAP_BYTES]   = "GL_UNPACK_SWAP_BYTES";
		modeMap[GL_UNPACK_LSB_FIRST]	= "GL_UNPACK_LSB_FIRST";
		modeMap[GL_PACK_SWAP_BYTES]		= "GL_PACK_SWAP_BYTES";
		modeMap[GL_PACK_LSB_FIRST]		= "GL_PACK_LSB_FIRST";
		modeMap[GL_PACK_IMAGE_HEIGHT]   = "GL_PACK_IMAGE_HEIGHT";
		modeMap[GL_PACK_SKIP_IMAGES]	= "GL_PACK_SKIP_IMAGES";
	}

	ModeMap::iterator it = modeMap.find(type);
	if (it == modeMap.end())
		TCU_FAIL("Unknown mode name");
	return it->second;
}

class RectangleTest : public deqp::TestCase
{
public:
	RectangleTest(deqp::Context& context, std::string& name, InternalFormat internalFormat);
	virtual ~RectangleTest();

	void resetInitialStorageModes();
	void applyInitialStorageModes();
	void testAllFormatsAndTypes();

	virtual tcu::TestNode::IterateResult iterate(void);

protected:
	void createGradient();
	void swapBytes(int typeSize, std::vector<GLbyte>& dataBuffer);

	template <typename Type>
	void makeGradient(Type (*unpack)(float));

	template <typename Type>
	static Type unpackSizedComponents(float value, int s1, int s2, int s3, int s4);

	template <typename Type>
	static Type unpackSizedComponentsRev(float value, int s1, int s2, int s3, int s4);

	static GLubyte unpack_UNSIGNED_BYTE(float value);
	static GLbyte unpack_BYTE(float value);
	static GLushort unpack_UNSIGNED_SHORT(float value);
	static GLshort unpack_SHORT(float value);
	static GLuint unpack_UNSIGNED_INT(float value);
	static GLint unpack_INT(float value);
	static GLhalf unpack_HALF_FLOAT(float value);
	static GLfloat unpack_FLOAT(float value);
	static GLubyte unpack_UNSIGNED_BYTE_3_3_2(float value);
	static GLubyte unpack_UNSIGNED_BYTE_2_3_3_REV(float value);
	static GLushort unpack_UNSIGNED_SHORT_5_6_5_REV(float value);
	static GLushort unpack_UNSIGNED_SHORT_4_4_4_4_REV(float value);
	static GLushort unpack_UNSIGNED_SHORT_1_5_5_5_REV(float value);
	static GLuint unpack_UNSIGNED_INT_8_8_8_8(float value);
	static GLuint unpack_UNSIGNED_INT_8_8_8_8_REV(float value);
	static GLuint unpack_UNSIGNED_INT_10_10_10_2(float value);
	static GLushort unpack_UNSIGNED_SHORT_5_6_5(float value);
	static GLushort unpack_UNSIGNED_SHORT_4_4_4_4(float value);
	static GLushort unpack_UNSIGNED_SHORT_5_5_5_1(float value);
	static GLuint unpack_UNSIGNED_INT_2_10_10_10_REV(float value);
	static GLuint unpack_UNSIGNED_INT_24_8(float value);
	static GLuint unpack_UNSIGNED_INT_5_9_9_9_REV(float value);
	static GLuint unpack_UNSIGNED_INT_10F_11F_11F_REV(float value);
	static F_32_UINT_24_8_REV unpack_FLOAT_32_UNSIGNED_INT_24_8_REV(float value);

	bool isFormatValid(const PixelFormat& format, const PixelType& type, const struct InternalFormat& internalformat,
					   bool checkInput, bool checkOutput, int operation) const;
	bool isUnsizedFormat(GLenum format) const;
	bool isSRGBFormat(const InternalFormat& internalFormat) const;
	bool isSNORMFormat(const InternalFormat& internalFormat) const;
	bool isCopyValid(const InternalFormat& copyInternalFormat, const InternalFormat& internalFormat) const;
	bool isFBOImageAttachValid(const InternalFormat& internalformat, GLenum format, GLenum type) const;

	const PixelFormat& getPixelFormat(GLenum format) const;
	const PixelType& getPixelType(GLenum type) const;
	const EnumFormats* getCanonicalFormat(const InternalFormat& internalformat, GLenum format, GLenum type) const;
	InternalFormatSamplerType getSampler(const PixelType& type, const PixelFormat& format) const;

	GLenum readOutputData(const PixelFormat& outputFormat, const PixelType& outputType, int operation);

	bool doCopy();

	bool doCopyInner();

	bool compare(GLvoid* gradient, GLvoid* data, const PixelFormat& outputFormat, const PixelType& outputType,
				 bool isCopy) const;

	void getFloatBuffer(GLvoid* gradient, int samplerIsIntUintFloat, const PixelFormat& format, const PixelType& type,
						int elementCount, std::vector<FloatPixel>& result) const;

	void getBits(const PixelType& type, const PixelFormat& format, std::vector<int>& resultTable) const;

	template <typename Type>
	void makeBuffer(const GLvoid* gradient, const PixelFormat& format, int samplerIsIntUintFloat, int elementCount,
					int componentCount, float (*pack)(Type), std::vector<FloatPixel>& result) const;

	template <typename Type>
	void makeBufferPackedInt(const GLvoid* gradient, const PixelFormat& format, int		elementCount,
							 void (*pack)(rawIntPixel*, Type), std::vector<FloatPixel>& result) const;

	template <typename Type>
	void makeBufferPackedUint(const GLvoid* gradient, const PixelFormat& format, int	  elementCount,
							  void (*pack)(rawUintPixel*, Type), std::vector<FloatPixel>& result) const;

	template <typename Type>
	void makeBufferPackedFloat(const GLvoid* gradient, const PixelFormat& format, int		elementCount,
							   void (*pack)(rawFloatPixel*, Type), std::vector<FloatPixel>& result) const;

	FloatPixel orderComponentsInt(rawIntPixel values, const PixelFormat& format) const;
	FloatPixel orderComponentsUint(rawUintPixel values, const PixelFormat& format) const;
	FloatPixel orderComponentsFloat(rawFloatPixel values, const PixelFormat& format) const;

	unsigned int getRealBitPrecision(int bits, bool isFloat) const;

	bool stripBuffer(const PackedPixelsBufferProperties& props, const GLubyte* orginalBuffer,
					 std::vector<GLubyte>& newBuffer, bool validate) const;

	int clampSignedValue(int bits, int value) const;
	unsigned int clampUnsignedValue(int bits, unsigned int value) const;

	static float pack_UNSIGNED_BYTE(GLubyte value);
	static float pack_BYTE(GLbyte value);
	static float pack_UNSIGNED_SHORT(GLushort value);
	static float pack_SHORT(GLshort value);
	static float pack_UNSIGNED_INT(GLuint value);
	static float pack_INT(GLint value);
	static float pack_HALF_FLOAT(GLhalf value);
	static float pack_FLOAT(GLfloat value);
	static void pack_UNSIGNED_BYTE_3_3_2(rawFloatPixel* values, GLubyte value);
	static void pack_UNSIGNED_BYTE_3_3_2_UINT(rawUintPixel* values, GLubyte value);
	static void pack_UNSIGNED_BYTE_3_3_2_INT(rawIntPixel* values, GLubyte value);
	static void pack_UNSIGNED_BYTE_2_3_3_REV(rawFloatPixel* values, GLubyte value);
	static void pack_UNSIGNED_BYTE_2_3_3_REV_UINT(rawUintPixel* values, GLubyte value);
	static void pack_UNSIGNED_BYTE_2_3_3_REV_INT(rawIntPixel* values, GLubyte value);
	static void pack_UNSIGNED_SHORT_5_6_5(rawFloatPixel* values, GLushort value);
	static void pack_UNSIGNED_SHORT_5_6_5_UINT(rawUintPixel* values, GLushort value);
	static void pack_UNSIGNED_SHORT_5_6_5_INT(rawIntPixel* values, GLushort value);
	static void pack_UNSIGNED_SHORT_5_6_5_REV(rawFloatPixel* values, GLushort value);
	static void pack_UNSIGNED_SHORT_5_6_5_REV_UINT(rawUintPixel* values, GLushort value);
	static void pack_UNSIGNED_SHORT_5_6_5_REV_INT(rawIntPixel* values, GLushort value);
	static void pack_UNSIGNED_SHORT_4_4_4_4(rawFloatPixel* values, GLushort value);
	static void pack_UNSIGNED_SHORT_4_4_4_4_UINT(rawUintPixel* values, GLushort value);
	static void pack_UNSIGNED_SHORT_4_4_4_4_INT(rawIntPixel* values, GLushort value);
	static void pack_UNSIGNED_SHORT_4_4_4_4_REV(rawFloatPixel* values, GLushort value);
	static void pack_UNSIGNED_SHORT_4_4_4_4_REV_UINT(rawUintPixel* values, GLushort value);
	static void pack_UNSIGNED_SHORT_4_4_4_4_REV_INT(rawIntPixel* values, GLushort value);
	static void pack_UNSIGNED_SHORT_5_5_5_1(rawFloatPixel* values, GLushort value);
	static void pack_UNSIGNED_SHORT_5_5_5_1_UINT(rawUintPixel* values, GLushort value);
	static void pack_UNSIGNED_SHORT_5_5_5_1_INT(rawIntPixel* values, GLushort value);
	static void pack_UNSIGNED_SHORT_1_5_5_5_REV(rawFloatPixel* values, GLushort value);
	static void pack_UNSIGNED_SHORT_1_5_5_5_REV_UINT(rawUintPixel* values, GLushort value);
	static void pack_UNSIGNED_SHORT_1_5_5_5_REV_INT(rawIntPixel* values, GLushort value);
	static void pack_UNSIGNED_INT_8_8_8_8(rawFloatPixel* values, GLuint value);
	static void pack_UNSIGNED_INT_8_8_8_8_UINT(rawUintPixel* values, GLuint value);
	static void pack_UNSIGNED_INT_8_8_8_8_INT(rawIntPixel* values, GLuint value);
	static void pack_UNSIGNED_INT_8_8_8_8_REV(rawFloatPixel* values, GLuint value);
	static void pack_UNSIGNED_INT_8_8_8_8_REV_UINT(rawUintPixel* values, GLuint value);
	static void pack_UNSIGNED_INT_8_8_8_8_REV_INT(rawIntPixel* values, GLuint value);
	static void pack_UNSIGNED_INT_10_10_10_2(rawFloatPixel* values, GLuint value);
	static void pack_UNSIGNED_INT_10_10_10_2_UINT(rawUintPixel* values, GLuint value);
	static void pack_UNSIGNED_INT_10_10_10_2_INT(rawIntPixel* values, GLuint value);
	static void pack_UNSIGNED_INT_2_10_10_10_REV(rawFloatPixel* values, GLuint value);
	static void pack_UNSIGNED_INT_2_10_10_10_REV_UINT(rawUintPixel* values, GLuint value);
	static void pack_UNSIGNED_INT_2_10_10_10_REV_INT(rawIntPixel* values, GLuint value);
	static void pack_UNSIGNED_INT_24_8(rawFloatPixel* values, GLuint value);
	static void pack_UNSIGNED_INT_10F_11F_11F_REV(rawFloatPixel* values, GLuint value);
	static void pack_UNSIGNED_INT_5_9_9_9_REV(rawFloatPixel* values, GLuint value);
	static void pack_FLOAT_32_UNSIGNED_INT_24_8_REV(rawFloatPixel* values, F_32_UINT_24_8_REV value);

	bool getTexImage();
	bool getTexImageInner(const PixelFormat& outputFormat, const PixelType& outputType);

	bool doRead(GLuint texture);
	bool readPixels(bool isCopy);
	bool readPixelsInner(const PixelFormat& outputFormat, const PixelType& outputType, bool isCopy);

protected:
	const InternalFormat m_internalFormat;

	bool						 m_usePBO;
	GLenum						 m_textureTarget;
	PackedPixelsBufferProperties m_initialPackProperties;
	PackedPixelsBufferProperties m_initialUnpackProperties;

	std::vector<GLbyte> m_gradient;
	const GLubyte		m_defaultFillValue;

public:
	// debuf counters
	static int m_countReadPixels;
	static int m_countReadPixelsOK;
	static int m_countGetTexImage;
	static int m_countGetTexImageOK;
	static int m_countCompare;
	static int m_countCompareOK;

private:
	// those attribute change multiple times during test execution
	PixelFormat					 m_inputFormat;
	PixelType					 m_inputType;
	InternalFormat				 m_copyInternalFormat;
	PackedPixelsBufferProperties m_packProperties;
	PackedPixelsBufferProperties m_unpackProperties;
	std::vector<GLbyte>			 m_outputBuffer;
};

int RectangleTest::m_countReadPixels	= 0;
int RectangleTest::m_countReadPixelsOK  = 0;
int RectangleTest::m_countGetTexImage   = 0;
int RectangleTest::m_countGetTexImageOK = 0;
int RectangleTest::m_countCompare		= 0;
int RectangleTest::m_countCompareOK		= 0;

RectangleTest::RectangleTest(deqp::Context& context, std::string& name, InternalFormat internalFormat)
	: deqp::TestCase(context, name.c_str(), "")
	, m_internalFormat(internalFormat)
	, m_usePBO(false)
	, m_textureTarget(GL_TEXTURE_2D)
	, m_defaultFillValue(0xaa)
{
}

RectangleTest::~RectangleTest()
{
}

void RectangleTest::resetInitialStorageModes()
{
	m_initialPackProperties.skipPixels = 0;
	m_initialPackProperties.skipRows   = 0;
	m_initialPackProperties.rowLength  = 0;
	m_initialPackProperties.alignment  = 4;
	m_initialPackProperties.rowCount   = 0;
	m_initialPackProperties.skipImages = 0;
	m_initialPackProperties.lsbFirst   = 0;
	m_initialPackProperties.swapBytes  = 0;

	m_initialUnpackProperties.skipPixels = 0;
	m_initialUnpackProperties.skipRows   = 0;
	m_initialUnpackProperties.rowLength  = 0;
	m_initialUnpackProperties.alignment  = 4;
	m_initialUnpackProperties.rowCount   = 0;
	m_initialUnpackProperties.skipImages = 0;
	m_initialUnpackProperties.lsbFirst   = 0;
	m_initialUnpackProperties.swapBytes  = 0;
}

void RectangleTest::applyInitialStorageModes()
{
	glu::RenderContext& renderContext = m_context.getRenderContext();
	const Functions&	gl			  = renderContext.getFunctions();

	PackedPixelsBufferProperties& up = m_initialUnpackProperties;
	PackedPixelsBufferProperties& pp = m_initialPackProperties;

	m_unpackProperties = up;
	m_packProperties   = pp;

	gl.pixelStorei(GL_PACK_ROW_LENGTH, pp.rowLength);
	gl.pixelStorei(GL_PACK_SKIP_ROWS, pp.skipRows);
	gl.pixelStorei(GL_PACK_SKIP_PIXELS, pp.skipPixels);
	gl.pixelStorei(GL_PACK_ALIGNMENT, pp.alignment);

	gl.pixelStorei(GL_UNPACK_ROW_LENGTH, up.rowLength);
	gl.pixelStorei(GL_UNPACK_SKIP_ROWS, up.skipRows);
	gl.pixelStorei(GL_UNPACK_SKIP_PIXELS, up.skipPixels);
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, up.alignment);
	gl.pixelStorei(GL_UNPACK_IMAGE_HEIGHT, up.rowCount);
	gl.pixelStorei(GL_UNPACK_SKIP_IMAGES, up.skipImages);

	if (!isContextTypeES(renderContext.getType()))
	{
		gl.pixelStorei(GL_PACK_IMAGE_HEIGHT, pp.rowCount);
		gl.pixelStorei(GL_PACK_SKIP_IMAGES, pp.skipImages);

		gl.pixelStorei(GL_PACK_SWAP_BYTES, pp.swapBytes);
		gl.pixelStorei(GL_PACK_LSB_FIRST, pp.lsbFirst);

		gl.pixelStorei(GL_UNPACK_SWAP_BYTES, up.swapBytes);
		gl.pixelStorei(GL_UNPACK_LSB_FIRST, up.lsbFirst);
	}
}

void RectangleTest::swapBytes(int typeSize, std::vector<GLbyte>& dataBuffer)
{
	int bufferSize = dataBuffer.size();
	switch (typeSize)
	{
	case 1:
		break; // no swapping
	case 2:
	{
		GLushort* data = reinterpret_cast<GLushort*>(&dataBuffer[0]);
		for (int i = 0; i < bufferSize / 2; i++)
		{
			GLushort v = data[i];
			data[i]	= ((v & 0xff) << 8) + ((v & 0xff00) >> 8);
		}
		break;
	}
	case 4:
	case 8: // typeSize is 2 x 32bit, behaves the same this time
	{
		GLuint* data = reinterpret_cast<GLuint*>(&dataBuffer[0]);
		for (int i = 0; i < bufferSize / 4; i++)
		{
			GLuint v = data[i];
			data[i]  = ((v & 0xff) << 24) + ((v & 0xff00) << 8) + ((v & 0xff0000) >> 8) + ((v & 0xff000000) >> 24);
		}
		break;
	}
	default:
		TCU_FAIL("Invalid size for swapBytes");
	}
}

const PixelFormat& RectangleTest::getPixelFormat(GLenum format) const
{
	const PixelFormat* formats;
	int				   formatsCount;
	if (glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		formats		 = esFormats;
		formatsCount = DE_LENGTH_OF_ARRAY(esFormats);
	}
	else
	{
		formats		 = coreFormats;
		formatsCount = DE_LENGTH_OF_ARRAY(coreFormats);
	}

	// Look up pixel format from a GL enum
	for (int i = 0; i < formatsCount; i++)
	{
		if (formats[i].format == format)
			return formats[i];
	}

	TCU_FAIL("Unsuported format.");
	return formats[0];
}

const PixelType& RectangleTest::getPixelType(GLenum type) const
{
	const PixelType* types;
	int				 typesCount;
	if (glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		types	  = esTypes;
		typesCount = DE_LENGTH_OF_ARRAY(esTypes);
	}
	else
	{
		types	  = coreTypes;
		typesCount = DE_LENGTH_OF_ARRAY(coreTypes);
	}

	for (int i = 0; i < typesCount; i++)
	{
		if (types[i].type == type)
			return types[i];
	}

	TCU_FAIL("Unsuported type.");
	return types[0];
}

const EnumFormats* RectangleTest::getCanonicalFormat(const InternalFormat& internalformat, GLenum format,
													 GLenum type) const
{
	// function returns a canonical format from internal format. for example
	// GL_RGBA16F => { GL_RGBA, GL_FLOAT }; used mostly for GLES tests

	if (glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		for (int i = 0; i < DE_LENGTH_OF_ARRAY(esValidFormats); ++i)
		{
			if ((esValidFormats[i].internalformat == internalformat.sizedFormat) &&
				(esValidFormats[i].format == format) && (esValidFormats[i].type == type))
			{
				return &(esValidFormats[i]);
			}
		}

		const glu::ContextInfo& contextInfo = m_context.getContextInfo();
		if (contextInfo.isExtensionSupported("GL_EXT_texture_type_2_10_10_10_REV"))
		{
			for (int i = 0; i < DE_LENGTH_OF_ARRAY(validformats_EXT_texture_type_2_10_10_10_REV); ++i)
			{
				if (validformats_EXT_texture_type_2_10_10_10_REV[i].internalformat == internalformat.sizedFormat &&
					validformats_EXT_texture_type_2_10_10_10_REV[i].format == format &&
					validformats_EXT_texture_type_2_10_10_10_REV[i].type == type)
				{
					return &(validformats_EXT_texture_type_2_10_10_10_REV[i]);
				}
			}

			if (contextInfo.isExtensionSupported("GL_OES_required_internalformat"))
			{
				for (int i = 0; i < DE_LENGTH_OF_ARRAY(validformats_OES_required_internalformat); ++i)
				{
					if (validformats_OES_required_internalformat[i].internalformat == internalformat.sizedFormat &&
						validformats_OES_required_internalformat[i].format == format &&
						validformats_OES_required_internalformat[i].type == type)
					{
						return &(validformats_OES_required_internalformat[i]);
					}
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < DE_LENGTH_OF_ARRAY(coreValidFormats); ++i)
		{
			if ((coreValidFormats[i].internalformat == internalformat.sizedFormat) &&
				(coreValidFormats[i].format == format) && (coreValidFormats[i].type == type))
			{
				return &(coreValidFormats[i]);
			}
		}
	}

	return 0;
}

InternalFormatSamplerType RectangleTest::getSampler(const PixelType& type, const PixelFormat& format) const
{
	switch (type.storage)
	{
	case STORAGE_FLOAT:
		return SAMPLER_FLOAT;

	case STORAGE_UNSIGNED:
		if ((format.componentFormat == FORMAT_COLOR_INTEGER) || (format.componentFormat == FORMAT_STENCIL))
			return SAMPLER_UINT;
		return SAMPLER_UNORM;

	case STORAGE_SIGNED:
		if (format.componentFormat == FORMAT_COLOR_INTEGER)
			return SAMPLER_INT;
		return SAMPLER_NORM;

	default:
		TCU_FAIL("Invalid storage specifier");
	}
}

void RectangleTest::createGradient()
{
	switch (m_inputType.type)
	{
	case GL_UNSIGNED_BYTE:
		makeGradient(unpack_UNSIGNED_BYTE);
		break;
	case GL_BYTE:
		makeGradient<GLbyte>(unpack_BYTE);
		break;
	case GL_UNSIGNED_SHORT:
		makeGradient<GLushort>(unpack_UNSIGNED_SHORT);
		break;
	case GL_SHORT:
		makeGradient<GLshort>(unpack_SHORT);
		break;
	case GL_UNSIGNED_INT:
		makeGradient<GLuint>(unpack_UNSIGNED_INT);
		break;
	case GL_INT:
		makeGradient<GLint>(unpack_INT);
		break;
	case GL_HALF_FLOAT:
		makeGradient<GLhalf>(unpack_HALF_FLOAT);
		break;
	case GL_FLOAT:
		makeGradient<GLfloat>(unpack_FLOAT);
		break;
	case GL_UNSIGNED_SHORT_5_6_5:
		makeGradient<GLushort>(unpack_UNSIGNED_SHORT_5_6_5);
		break;
	case GL_UNSIGNED_SHORT_4_4_4_4:
		makeGradient<GLushort>(unpack_UNSIGNED_SHORT_4_4_4_4);
		break;
	case GL_UNSIGNED_SHORT_5_5_5_1:
		makeGradient<GLushort>(unpack_UNSIGNED_SHORT_5_5_5_1);
		break;
	case GL_UNSIGNED_INT_2_10_10_10_REV:
		makeGradient<GLuint>(unpack_UNSIGNED_INT_2_10_10_10_REV);
		break;
	case GL_UNSIGNED_INT_24_8:
		makeGradient<GLuint>(unpack_UNSIGNED_INT_24_8);
		break;
	case GL_UNSIGNED_INT_10F_11F_11F_REV:
		makeGradient<GLuint>(unpack_UNSIGNED_INT_10F_11F_11F_REV);
		break;
	case GL_UNSIGNED_INT_5_9_9_9_REV:
		makeGradient<GLuint>(unpack_UNSIGNED_INT_5_9_9_9_REV);
		break;
	case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
		makeGradient<F_32_UINT_24_8_REV>(unpack_FLOAT_32_UNSIGNED_INT_24_8_REV);
		break;
	case GL_UNSIGNED_BYTE_3_3_2:
		makeGradient<GLubyte>(unpack_UNSIGNED_BYTE_3_3_2);
		break;
	case GL_UNSIGNED_BYTE_2_3_3_REV:
		makeGradient<GLubyte>(unpack_UNSIGNED_BYTE_2_3_3_REV);
		break;
	case GL_UNSIGNED_SHORT_5_6_5_REV:
		makeGradient<GLushort>(unpack_UNSIGNED_SHORT_5_6_5_REV);
		break;
	case GL_UNSIGNED_SHORT_4_4_4_4_REV:
		makeGradient<GLushort>(unpack_UNSIGNED_SHORT_4_4_4_4_REV);
		break;
	case GL_UNSIGNED_SHORT_1_5_5_5_REV:
		makeGradient<GLushort>(unpack_UNSIGNED_SHORT_1_5_5_5_REV);
		break;
	case GL_UNSIGNED_INT_8_8_8_8:
		makeGradient<GLuint>(unpack_UNSIGNED_INT_8_8_8_8);
		break;
	case GL_UNSIGNED_INT_8_8_8_8_REV:
		makeGradient<GLuint>(unpack_UNSIGNED_INT_8_8_8_8_REV);
		break;
	case GL_UNSIGNED_INT_10_10_10_2:
		makeGradient<GLuint>(unpack_UNSIGNED_INT_10_10_10_2);
		break;
	default:
		TCU_FAIL("Unsupported type");
	};
}

template <typename Type>
void RectangleTest::makeGradient(Type (*unpack)(float))
{
	// number of elements in a group
	int elementsInGroup = m_inputFormat.components;
	if (m_inputType.special)
		elementsInGroup = 1;

	int rowCount = m_unpackProperties.rowCount;
	if (rowCount == 0)
		rowCount = GRADIENT_HEIGHT + m_unpackProperties.skipRows;

	// number of groups in the row
	int rowLength = m_unpackProperties.rowLength;
	if (rowLength == 0)
		rowLength = GRADIENT_WIDTH + m_unpackProperties.skipPixels;

	int elementSize = m_inputType.size;

	// row size (in elements)
	int elementsInRowNoAlign = elementsInGroup * rowLength;
	int elementsInRow		 = elementsInRowNoAlign;
	if (elementSize < m_unpackProperties.alignment)
	{
		int alignment = m_unpackProperties.alignment;
		elementsInRow = (int)(alignment * deFloatCeil(elementSize * elementsInGroup * rowLength / ((float)alignment))) /
						elementSize;
	}

	if (m_textureTarget == GL_TEXTURE_2D)
		m_unpackProperties.skipImages = 0;

	// "depth" will be 1 + skipped image layers.
	// We still want to work on a 2D-ish image later.
	int depth = 1 + m_unpackProperties.skipImages;

	m_unpackProperties.elementsInGroup		= elementsInGroup;
	m_unpackProperties.rowCount				= rowCount;
	m_unpackProperties.rowLength			= rowLength;
	m_unpackProperties.elementSize			= elementSize;
	m_unpackProperties.elementsInRowNoAlign = elementsInRowNoAlign;
	m_unpackProperties.elementsInRow		= elementsInRow;
	m_unpackProperties.imagesCount			= depth;

	// element size * elements in row * number of rows * number of 2d images
	std::size_t bufferSize = elementSize * elementsInRow * rowCount * depth;
	// need space for the skipped images as well
	bufferSize *= depth;

	m_gradient.resize(bufferSize);
	Type* data = reinterpret_cast<Type*>(&m_gradient[0]);

	std::size_t dataToSkip   = m_unpackProperties.skipImages * rowCount * elementsInRowNoAlign;
	std::size_t index		 = dataToSkip;
	const Type  defaultValue = unpack(0.5f);
	std::fill(data, data + dataToSkip, defaultValue);

	for (int k = 0; k < depth; k++)
	{
		for (int j = 0; j < rowCount; j++)
		{
			for (int i = 0; i < elementsInRow; i++)
			{
				int x = i / elementsInGroup;
				if ((k == depth - 1) && (m_unpackProperties.skipRows <= j) &&
					(j < m_unpackProperties.skipRows + GRADIENT_HEIGHT) && (m_unpackProperties.skipPixels <= x) &&
					(x < m_unpackProperties.skipPixels + GRADIENT_WIDTH))
				{
					float value   = static_cast<float>(x - m_unpackProperties.skipPixels) / GRADIENT_WIDTH;
					int   channel = i - elementsInGroup * x;
					value *= 1.0 - 0.25 * channel;
					data[index] = unpack(value);
				}
				else
				{
					data[index] = defaultValue;
				}
				index++;
			}
		}
	}
}

template <typename Type>
Type RectangleTest::unpackSizedComponents(float value, int s1, int s2, int s3, int s4)
{
	int	typeBits = sizeof(Type) * 8;
	double v		= static_cast<double>(value);
	Type   c1		= static_cast<Type>(v * 1.00 * ((1 << s1) - 1));
	Type   c2		= static_cast<Type>(v * 0.75 * ((1 << s2) - 1));
	Type   c3		= static_cast<Type>(v * 0.50 * ((1 << s3) - 1));
	Type   c4		= static_cast<Type>(v * 0.25 * ((1 << s4) - 1));
	return ((c1) << (typeBits - s1)) | ((c2) << (typeBits - s1 - s2)) | ((c3) << (typeBits - s1 - s2 - s3)) |
		   ((c4) << (typeBits - s1 - s2 - s3 - s4));
}

template <typename Type>
Type RectangleTest::unpackSizedComponentsRev(float value, int s1, int s2, int s3, int s4)
{
	int	typeBits = sizeof(Type) * 8;
	double v		= static_cast<double>(value);
	Type   c1		= static_cast<Type>(v * 1.00 * ((1 << s4) - 1));
	Type   c2		= static_cast<Type>(v * 0.75 * ((1 << s3) - 1));
	Type   c3		= static_cast<Type>(v * 0.50 * ((1 << s2) - 1));
	Type   c4		= static_cast<Type>(v * 0.25 * ((1 << s1) - 1));
	return ((c4) << (typeBits - s1)) | ((c3) << (typeBits - s1 - s2)) | ((c2) << (typeBits - s1 - s2 - s3)) |
		   ((c1) << (typeBits - s1 - s2 - s3 - s4));
}

GLubyte RectangleTest::unpack_UNSIGNED_BYTE(float value)
{
	return static_cast<GLubyte>(value * std::numeric_limits<GLubyte>::max());
}

GLbyte RectangleTest::unpack_BYTE(float value)
{
	return static_cast<GLbyte>(value * std::numeric_limits<GLbyte>::max());
}

GLushort RectangleTest::unpack_UNSIGNED_SHORT(float value)
{
	return static_cast<GLushort>(value * std::numeric_limits<GLushort>::max());
}

GLshort RectangleTest::unpack_SHORT(float value)
{
	return static_cast<GLshort>(value * std::numeric_limits<GLshort>::max());
}

GLuint RectangleTest::unpack_UNSIGNED_INT(float value)
{
	return static_cast<GLuint>(value * std::numeric_limits<GLuint>::max());
}

GLint RectangleTest::unpack_INT(float value)
{
	return static_cast<GLint>(value * std::numeric_limits<GLint>::max());
}

GLhalf RectangleTest::unpack_HALF_FLOAT(float value)
{
	return floatToHalfFloat(value);
}

GLfloat RectangleTest::unpack_FLOAT(float value)
{
	return value;
}

GLubyte RectangleTest::unpack_UNSIGNED_BYTE_3_3_2(float value)
{
	return unpackSizedComponents<GLubyte>(value, 3, 3, 2, 0);
}

GLubyte RectangleTest::unpack_UNSIGNED_BYTE_2_3_3_REV(float value)
{
	return unpackSizedComponentsRev<GLubyte>(value, 2, 3, 3, 0);
}

GLushort RectangleTest::unpack_UNSIGNED_SHORT_5_6_5_REV(float value)
{
	return unpackSizedComponentsRev<GLushort>(value, 5, 6, 5, 0);
}

GLushort RectangleTest::unpack_UNSIGNED_SHORT_4_4_4_4_REV(float value)
{
	return unpackSizedComponentsRev<GLushort>(value, 4, 4, 4, 4);
}

GLushort RectangleTest::unpack_UNSIGNED_SHORT_1_5_5_5_REV(float value)
{
	return unpackSizedComponentsRev<GLushort>(value, 1, 5, 5, 5);
}

GLuint RectangleTest::unpack_UNSIGNED_INT_8_8_8_8(float value)
{
	return unpackSizedComponents<GLuint>(value, 8, 8, 8, 8);
}

GLuint RectangleTest::unpack_UNSIGNED_INT_8_8_8_8_REV(float value)
{
	return unpackSizedComponentsRev<GLuint>(value, 8, 8, 8, 8);
}

GLuint RectangleTest::unpack_UNSIGNED_INT_10_10_10_2(float value)
{
	return unpackSizedComponents<GLuint>(value, 10, 10, 10, 2);
}

GLushort RectangleTest::unpack_UNSIGNED_SHORT_5_6_5(float value)
{
	return unpackSizedComponents<GLushort>(value, 5, 6, 5, 0);
}

GLushort RectangleTest::unpack_UNSIGNED_SHORT_4_4_4_4(float value)
{
	return unpackSizedComponents<GLushort>(value, 4, 4, 4, 4);
}

GLushort RectangleTest::unpack_UNSIGNED_SHORT_5_5_5_1(float value)
{
	return unpackSizedComponents<GLushort>(value, 5, 5, 5, 1);
}

GLuint RectangleTest::unpack_UNSIGNED_INT_2_10_10_10_REV(float value)
{
	return unpackSizedComponentsRev<GLuint>(value, 2, 10, 10, 10);
}

GLuint RectangleTest::unpack_UNSIGNED_INT_24_8(float value)
{
	return unpackSizedComponents<GLuint>(value, 24, 8, 0, 0);
}

GLuint RectangleTest::unpack_UNSIGNED_INT_5_9_9_9_REV(float value)
{
	const int N		= 9;
	const int B		= 15;
	const int E_max = 31;

	GLfloat red   = value * 1.00f;
	GLfloat green = value * 0.75f;
	GLfloat blue  = value * 0.50f;

	GLfloat sharedExpMax = (deFloatPow(2, N) - 1) / deFloatPow(2, N) * deFloatPow(2, E_max - B);

	GLfloat red_c   = deFloatMax(0, deFloatMin(sharedExpMax, red));
	GLfloat green_c = deFloatMax(0, deFloatMin(sharedExpMax, green));
	GLfloat blue_c  = deFloatMax(0, deFloatMin(sharedExpMax, blue));

	GLfloat max_c = deFloatMax(deFloatMax(red_c, green_c), blue_c);

	GLfloat exp_p = deFloatMax(-B - 1, deFloatFloor(deFloatLog2(max_c))) + 1 + B;

	GLfloat max_s = deFloatFloor(max_c / deFloatPow(2, exp_p - B - N) + 0.5);

	GLfloat exp_s;

	if (0 <= max_s && max_s < deFloatPow(2, N))
		exp_s = exp_p;
	else
		exp_s = exp_p + 1;

	GLfloat red_s   = deFloatFloor(red_c / deFloatPow(2, exp_s - B - N) + 0.5);
	GLfloat green_s = deFloatFloor(green_c / deFloatPow(2, exp_s - B - N) + 0.5);
	GLfloat blue_s  = deFloatFloor(blue_c / deFloatPow(2, exp_s - B - N) + 0.5);

	GLuint c1 = (static_cast<GLuint>(red_s)) & 511;
	GLuint c2 = (static_cast<GLuint>(green_s)) & 511;
	GLuint c3 = (static_cast<GLuint>(blue_s)) & 511;
	GLuint c4 = (static_cast<GLuint>(exp_s)) & 31;

	return (c1) | (c2 << 9) | (c3 << 18) | (c4 << 27);
}

GLuint RectangleTest::unpack_UNSIGNED_INT_10F_11F_11F_REV(float value)
{
	GLuint c1 = floatToUnisgnedF11(value * 1.00f);
	GLuint c2 = floatToUnisgnedF11(value * 0.75f);
	GLuint c3 = floatToUnisgnedF10(value * 0.50f);
	return (c3 << 22) | (c2 << 11) | (c1);
}

F_32_UINT_24_8_REV RectangleTest::unpack_FLOAT_32_UNSIGNED_INT_24_8_REV(float value)
{
	F_32_UINT_24_8_REV ret;
	ret.d = value;
	ret.s = (GLuint)(value * 255.0 * 0.75);
	ret.s &= 0xff;
	return ret;
}

bool RectangleTest::isFormatValid(const PixelFormat& format, const PixelType& type,
								  const struct InternalFormat& internalformat, bool checkInput, bool checkOutput,
								  int operation) const
{
	glu::RenderContext&		renderContext = m_context.getRenderContext();
	glu::ContextType		contextType   = renderContext.getType();
	const glu::ContextInfo& contextInfo   = m_context.getContextInfo();
	const Functions&		gl			  = renderContext.getFunctions();

	int i;

	// Test the combination of input format, input type and internalFormat
	if (glu::isContextTypeES(contextType))
	{
		if (checkInput)
		{
			// GLES30 has more restricted requirement on combination than GL for input
			for (i = 0; i < DE_LENGTH_OF_ARRAY(esValidFormats); ++i)
			{
				if (internalformat.sizedFormat == esValidFormats[i].internalformat &&
					format.format == esValidFormats[i].format && type.type == esValidFormats[i].type)
				{
					break;
				}
			}

			if (i == DE_LENGTH_OF_ARRAY(esValidFormats))
			{
				// Check for support of OES_texture_float extension
				if (((GL_LUMINANCE_ALPHA == format.format) && (GL_LUMINANCE_ALPHA == internalformat.sizedFormat)) ||
					((GL_LUMINANCE == format.format) && (GL_LUMINANCE == internalformat.sizedFormat)) ||
					((GL_ALPHA == format.format) && (GL_ALPHA == internalformat.sizedFormat)) ||
					((GL_RGBA == format.format) && (GL_RGBA == internalformat.sizedFormat)) ||
					((GL_RGB == format.format) && (GL_RGB == internalformat.sizedFormat)))
				{
					if ((contextInfo.isExtensionSupported("GL_OES_texture_float") && (GL_FLOAT == type.type)) ||
						(contextInfo.isExtensionSupported("GL_OES_texture_half_float") &&
						 (GL_HALF_FLOAT_OES == type.type)))
					{
						return true;
					}
				}

				// Check for support of EXT_texture_type_2_10_10_10_REV extension
				if (((GL_RGBA == format.format) && (GL_RGBA == internalformat.sizedFormat)) ||
					((GL_RGB == format.format) && (GL_RGB == internalformat.sizedFormat)))
				{
					if (contextInfo.isExtensionSupported("GL_EXT_texture_type_2_10_10_10_REV") &&
						((GL_UNSIGNED_INT_2_10_10_10_REV_EXT == type.type)))
						return true;
				}

				// Check for support of NV_packed_float extension
				if ((GL_RGB == format.format) && (GL_RGB == internalformat.sizedFormat))
				{
					if (contextInfo.isExtensionSupported("GL_NV_packed_float") &&
						((GL_UNSIGNED_INT_10F_11F_11F_REV == type.type)))
						return true;
				}

				// Check for support of EXT_texture_type_2_10_10_10_REV and GL_OES_required_internalformat extensions
				if (contextInfo.isExtensionSupported("GL_EXT_texture_type_2_10_10_10_REV") &&
					contextInfo.isExtensionSupported("GL_OES_required_internalformat"))
				{
					for (i = 0; i < DE_LENGTH_OF_ARRAY(validformats_OES_required_internalformat); ++i)
					{
						if (internalformat.sizedFormat == validformats_OES_required_internalformat[i].internalformat &&
							format.format == validformats_OES_required_internalformat[i].format &&
							type.type == validformats_OES_required_internalformat[i].type)
						{
							return true;
						}
					}
				}

				return false;
			}

			if ((m_textureTarget == GL_TEXTURE_3D) &&
				((format.format == GL_DEPTH_COMPONENT) || (format.format == GL_DEPTH_STENCIL)))
				return false;
		}
		else if (checkOutput)
		{
			// GLES30 has more restricted requirement on combination than GL for output
			// As stated in Section Reading Pixels
			InternalFormatSamplerType sampler = internalformat.sampler;

			const PixelFormat& inputFormat = getPixelFormat(internalformat.format);

			if (inputFormat.attachment == GL_DEPTH_ATTACHMENT && contextInfo.isExtensionSupported("GL_NV_read_depth") &&
				format.format == GL_DEPTH_COMPONENT &&
				((sampler == SAMPLER_FLOAT && type.type == GL_FLOAT) ||
				 (sampler != SAMPLER_FLOAT && (type.type == GL_UNSIGNED_SHORT || type.type == GL_UNSIGNED_INT ||
											   type.type == GL_UNSIGNED_INT_24_8))))
			{
				return true;
			}

			if (inputFormat.attachment == GL_DEPTH_STENCIL_ATTACHMENT &&
				contextInfo.isExtensionSupported("GL_NV_read_depth_stencil") &&
				((format.format == GL_DEPTH_STENCIL &&
				  ((sampler == SAMPLER_FLOAT && type.type == GL_FLOAT_32_UNSIGNED_INT_24_8_REV) ||
				   (sampler != SAMPLER_FLOAT && type.type == GL_UNSIGNED_INT_24_8))) ||
				 (format.format == GL_DEPTH_COMPONENT &&
				  ((sampler == SAMPLER_FLOAT && type.type == GL_FLOAT) ||
				   (sampler != SAMPLER_FLOAT && (type.type == GL_UNSIGNED_SHORT || type.type == GL_UNSIGNED_INT ||
												 type.type == GL_UNSIGNED_INT_24_8))))))
			{
				return true;
			}

			if (inputFormat.attachment != GL_COLOR_ATTACHMENT0)
			{
				return false;
			}

			if ((sampler == SAMPLER_UNORM) && (((type.type == GL_UNSIGNED_BYTE) && (format.format == GL_RGB) &&
												(internalformat.sizedFormat == GL_SRGB8)) ||
											   ((type.type == GL_UNSIGNED_BYTE) && (format.format == GL_RGBA) &&
												(internalformat.sizedFormat == GL_SRGB8_ALPHA8))) &&
				contextInfo.isExtensionSupported("GL_NV_sRGB_formats"))
			{
				return true;
			}

			if ((sampler == SAMPLER_NORM) && (type.type == GL_UNSIGNED_BYTE) && (format.format == GL_RGBA) &&
				((internalformat.sizedFormat == GL_R8_SNORM) || (internalformat.sizedFormat == GL_RG8_SNORM) ||
				 (internalformat.sizedFormat == GL_RGBA8_SNORM) || (internalformat.sizedFormat == GL_R16_SNORM) ||
				 (internalformat.sizedFormat == GL_RG16_SNORM) || (internalformat.sizedFormat == GL_RGBA16_SNORM)) &&
				contextInfo.isExtensionSupported("GL_EXT_render_snorm"))
			{
				return true;
			}

			GLint implementType;
			GLint implementFormat;
			gl.getIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &implementType);
			gl.getIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &implementFormat);
			GLenum err				   = gl.getError();
			GLenum implementTypeEnum   = static_cast<GLenum>(implementType);
			GLenum implementFormatEnum = static_cast<GLenum>(implementFormat);

			if (((sampler == SAMPLER_UNORM) && (type.type == GL_UNSIGNED_BYTE) && (format.format == GL_RGBA)) ||
				((sampler == SAMPLER_UINT) && (type.type == GL_UNSIGNED_INT) && (format.format == GL_RGBA_INTEGER)) ||
				((sampler == SAMPLER_INT) && (type.type == GL_INT) && (format.format == GL_RGBA_INTEGER)) ||
				((sampler == SAMPLER_FLOAT) && (type.type == GL_FLOAT) && (format.format == GL_RGBA)) ||
				((err == GL_NO_ERROR) && (type.type == implementTypeEnum) && (format.format == implementFormatEnum)) ||
				((internalformat.sizedFormat == GL_RGB10_A2) && (type.type == GL_UNSIGNED_INT_2_10_10_10_REV) &&
				 (format.format == GL_RGBA)))
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	else
	{
		if (format.format == GL_DEPTH_STENCIL)
		{
			if (type.type != GL_UNSIGNED_INT_24_8 && type.type != GL_FLOAT_32_UNSIGNED_INT_24_8_REV)
			{
				return false;
			}
		}

		if ((format.componentFormat == FORMAT_COLOR_INTEGER) && (type.type == GL_FLOAT || type.type == GL_HALF_FLOAT))
		{
			return false;
		}

		if ((internalformat.baseFormat == GL_DEPTH_STENCIL || internalformat.baseFormat == GL_STENCIL_INDEX ||
			 internalformat.baseFormat == GL_DEPTH_COMPONENT) !=
			(format.format == GL_DEPTH_STENCIL || format.format == GL_STENCIL_INDEX ||
			 format.format == GL_DEPTH_COMPONENT))
		{
			return false;
		}

		if (operation == INPUT_TEXIMAGE)
		{
			if (format.format == GL_STENCIL_INDEX || internalformat.baseFormat == GL_STENCIL_INDEX)
			{
				return false;
			}

			if ((format.format == GL_DEPTH_COMPONENT || format.format == GL_DEPTH_STENCIL) &&
				!(internalformat.baseFormat == GL_DEPTH_STENCIL || internalformat.baseFormat == GL_DEPTH_COMPONENT))
			{
				return false;
			}
		}
		else if (operation == OUTPUT_GETTEXIMAGE)
		{
			if ((format.format == GL_STENCIL_INDEX &&
				 ((internalformat.baseFormat != GL_STENCIL_INDEX && internalformat.baseFormat != GL_DEPTH_STENCIL) ||
				  !contextInfo.isExtensionSupported("GL_ARB_texture_stencil8"))))
			{
				return false;
			}

			if (format.format == GL_DEPTH_STENCIL && internalformat.baseFormat != GL_DEPTH_STENCIL)
			{
				return false;
			}
		}
		else if (operation == OUTPUT_READPIXELS)
		{
			if (format.format == GL_DEPTH_STENCIL && internalformat.baseFormat != GL_DEPTH_STENCIL)
			{
				return false;
			}

			if (format.format == GL_DEPTH_COMPONENT && internalformat.baseFormat != GL_DEPTH_STENCIL &&
				internalformat.baseFormat != GL_DEPTH_COMPONENT)
			{
				return false;
			}

			if (format.format == GL_STENCIL_INDEX && internalformat.baseFormat != GL_DEPTH_STENCIL &&
				internalformat.baseFormat != GL_STENCIL_INDEX)
			{
				return false;
			}
		}

		if (type.special == true)
		{
			bool valid = false;

			for (i = 0; i < DE_LENGTH_OF_ARRAY(coreValidFormats); ++i)
			{
				if (coreValidFormats[i].format == format.format && coreValidFormats[i].type == type.type)
				{
					valid = true;
					break;
				}
			}

			if (!valid)
				return false;
		}

		if ((format.componentFormat == FORMAT_COLOR_INTEGER) &&
			!(internalformat.sampler == SAMPLER_INT || internalformat.sampler == SAMPLER_UINT))
		{
			return false;
		}

		if (!(format.componentFormat == FORMAT_COLOR_INTEGER) &&
			(internalformat.sampler == SAMPLER_INT || internalformat.sampler == SAMPLER_UINT))
		{
			return false;
		}

		if ((m_textureTarget == GL_TEXTURE_3D) &&
			((internalformat.baseFormat == GL_DEPTH_COMPONENT) || (internalformat.baseFormat == GL_DEPTH_STENCIL) ||
			 (internalformat.sizedFormat == GL_COMPRESSED_RED_RGTC1) ||
			 (internalformat.sizedFormat == GL_COMPRESSED_SIGNED_RED_RGTC1) ||
			 (internalformat.sizedFormat == GL_COMPRESSED_RG_RGTC2) ||
			 (internalformat.sizedFormat == GL_COMPRESSED_SIGNED_RG_RGTC2)))
		{
			return false;
		}
	}

	return true;
}

bool RectangleTest::isUnsizedFormat(GLenum format) const
{
	GLenum  formats[]  = { GL_RGBA, GL_RGB, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_ALPHA };
	GLenum* formatsEnd = formats + DE_LENGTH_OF_ARRAY(formats);
	return (std::find(formats, formatsEnd, format) < formatsEnd);
}

bool RectangleTest::isSRGBFormat(const InternalFormat& internalFormat) const
{
	return (internalFormat.sizedFormat == GL_SRGB8) || (internalFormat.sizedFormat == GL_SRGB8_ALPHA8);
}

bool RectangleTest::isSNORMFormat(const InternalFormat& internalFormat) const
{
	GLenum formats[] = { GL_R8_SNORM,  GL_RG8_SNORM,  GL_RGB8_SNORM,  GL_RGBA8_SNORM,
						 GL_R16_SNORM, GL_RG16_SNORM, GL_RGB16_SNORM, GL_RGBA16_SNORM };
	GLenum* formatsEnd = formats + DE_LENGTH_OF_ARRAY(formats);
	return (std::find(formats, formatsEnd, internalFormat.sizedFormat) < formatsEnd);
}

bool RectangleTest::isCopyValid(const InternalFormat& copyInternalFormat, const InternalFormat& internalFormat) const
{
	// check if copy between two internal formats is allowed

	int b1 = getPixelFormat(internalFormat.format).components;
	int b2 = getPixelFormat(copyInternalFormat.format).components;

	if (b2 > b1)
		return false;

	//Check that the types can be converted in CopyTexImage.
	if (((copyInternalFormat.sampler == SAMPLER_UINT) && (internalFormat.sampler != SAMPLER_UINT)) ||
		((copyInternalFormat.sampler == SAMPLER_INT) && (internalFormat.sampler != SAMPLER_INT)) ||
		(((copyInternalFormat.sampler == SAMPLER_FLOAT) || (internalFormat.sampler == SAMPLER_UNORM) ||
		  (copyInternalFormat.sampler == SAMPLER_NORM)) &&
		 (!((copyInternalFormat.sampler == SAMPLER_FLOAT) || (internalFormat.sampler == SAMPLER_UNORM) ||
			(internalFormat.sampler == SAMPLER_NORM)))))
	{
		return false;
	}

	// Core GL is less restricted then ES - check it first
	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		if ((copyInternalFormat.format == GL_DEPTH_COMPONENT && internalFormat.format != GL_DEPTH_COMPONENT) ||
			(copyInternalFormat.format == GL_DEPTH_STENCIL && internalFormat.format != GL_DEPTH_STENCIL) ||
			(copyInternalFormat.format == GL_ALPHA && internalFormat.format != GL_ALPHA) ||
			(copyInternalFormat.format == GL_LUMINANCE && internalFormat.format != GL_LUMINANCE) ||
			(copyInternalFormat.format == GL_LUMINANCE_ALPHA && internalFormat.format != GL_LUMINANCE_ALPHA))
		{
			return false;
		}

		return true;
	}

	const glu::ContextInfo& contextInfo = m_context.getContextInfo();

	// GLES30 has more restricted requirement on glCopyTexImage2D
	// As stated in Table 3.15 and comment to glCopyTexImage2D
	if ((internalFormat.baseFormat == GL_DEPTH_COMPONENT) || (internalFormat.baseFormat == GL_DEPTH_STENCIL) ||
		(copyInternalFormat.baseFormat == GL_DEPTH_COMPONENT) || (copyInternalFormat.baseFormat == GL_DEPTH_STENCIL) ||
		((internalFormat.baseFormat != GL_RGBA && internalFormat.baseFormat != GL_ALPHA) &&
		 ((copyInternalFormat.baseFormat == GL_ALPHA) || (copyInternalFormat.baseFormat == GL_LUMINANCE_ALPHA))) ||
		((internalFormat.baseFormat == GL_ALPHA) &&
		 ((copyInternalFormat.baseFormat != GL_RGBA) && (copyInternalFormat.baseFormat != GL_ALPHA) &&
		  (copyInternalFormat.baseFormat != GL_LUMINANCE_ALPHA))) ||
		(isSRGBFormat(internalFormat) != isSRGBFormat(copyInternalFormat)) ||
		// GLES30 does not define ReadPixels types for signed normalized fixed point formats in Table 3.14,
		// and conversions to SNORM internalformats are not allowed by Table 3.2
		(copyInternalFormat.sampler == SAMPLER_NORM) ||
		((copyInternalFormat.sizedFormat == GL_RGB9_E5) &&
		 !contextInfo.isExtensionSupported("GL_APPLE_color_buffer_packed_float")))
	{
		/* Some formats are activated by extensions, check. */
		if (((internalFormat.baseFormat == GL_LUMINANCE && copyInternalFormat.baseFormat == GL_LUMINANCE) ||
			 (internalFormat.baseFormat == GL_ALPHA && copyInternalFormat.baseFormat == GL_ALPHA) ||
			 (internalFormat.baseFormat == GL_LUMINANCE_ALPHA &&
			  (copyInternalFormat.baseFormat == GL_LUMINANCE_ALPHA || copyInternalFormat.baseFormat == GL_LUMINANCE ||
			   copyInternalFormat.baseFormat == GL_ALPHA))) &&
			contextInfo.isExtensionSupported("GL_NV_render_luminance_alpha"))
		{
			return true;
		}
		else if (contextInfo.isExtensionSupported("GL_EXT_render_snorm") && isSNORMFormat(copyInternalFormat) &&
				 (internalFormat.sampler == copyInternalFormat.sampler) &&
				 (((copyInternalFormat.baseFormat == GL_RED) &&
				   (internalFormat.baseFormat == GL_RED || internalFormat.baseFormat == GL_RG ||
					internalFormat.baseFormat == GL_RGB || internalFormat.baseFormat == GL_RGBA ||
					copyInternalFormat.baseFormat == GL_LUMINANCE)) ||
				  ((copyInternalFormat.baseFormat == GL_RG) &&
				   (internalFormat.baseFormat == GL_RG || internalFormat.baseFormat == GL_RGB ||
					internalFormat.baseFormat == GL_RGBA)) ||
				  ((copyInternalFormat.baseFormat == GL_RGB) &&
				   (internalFormat.baseFormat == GL_RGB || internalFormat.baseFormat == GL_RGBA)) ||
				  ((copyInternalFormat.baseFormat == GL_RGBA) && (internalFormat.baseFormat == GL_RGBA))))
		{
			return true;
		}

		return false;
	}
	else
	{
		if (internalFormat.sampler != copyInternalFormat.sampler)
		{
			// You can't convert between different base types, for example NORM<->FLOAT.
			return false;
		}
		if (!isUnsizedFormat(copyInternalFormat.sizedFormat))
		{
			if ((internalFormat.bits.bits.red && copyInternalFormat.bits.bits.red &&
				 internalFormat.bits.bits.red != copyInternalFormat.bits.bits.red) ||
				(internalFormat.bits.bits.green && copyInternalFormat.bits.bits.green &&
				 internalFormat.bits.bits.green != copyInternalFormat.bits.bits.green) ||
				(internalFormat.bits.bits.blue && copyInternalFormat.bits.bits.blue &&
				 internalFormat.bits.bits.blue != copyInternalFormat.bits.bits.blue) ||
				(internalFormat.bits.bits.alpha && copyInternalFormat.bits.bits.alpha &&
				 internalFormat.bits.bits.alpha != copyInternalFormat.bits.bits.alpha))
			{
				// If the destination internalFormat is sized we don't allow component size changes.
				return false;
			}
		}
		else
		{
			if (internalFormat.sizedFormat == GL_RGB10_A2)
			{
				// Not allowed to convert from a GL_RGB10_A2 surface.
				return false;
			}
		}
	}

	return true;
}

bool RectangleTest::isFBOImageAttachValid(const InternalFormat& internalformat, GLenum format, GLenum type) const
{
	const glu::ContextInfo& contextInfo = m_context.getContextInfo();

	if (glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		const EnumFormats* validFormat = getCanonicalFormat(internalformat, format, type);
		if (validFormat != 0)
		{
			if (!validFormat->bRenderable)
			{
				/* Some formats are activated by extensions, check. */
				if ((GL_RGBA32F == validFormat->internalformat || GL_RGBA16F == validFormat->internalformat ||
					 GL_RG32F == validFormat->internalformat || GL_RG16F == validFormat->internalformat ||
					 GL_R32F == validFormat->internalformat || GL_R16F == validFormat->internalformat ||
					 GL_R11F_G11F_B10F == validFormat->internalformat) &&
					contextInfo.isExtensionSupported("GL_EXT_color_buffer_float"))
				{
					return true;
				}

				if ((GL_RGBA16F == validFormat->internalformat || GL_RGB16F == validFormat->internalformat ||
					 GL_RG16F == validFormat->internalformat || GL_R16F == validFormat->internalformat) &&
					contextInfo.isExtensionSupported("GL_EXT_color_buffer_half_float"))
				{
					return true;
				}

				if ((GL_R11F_G11F_B10F == validFormat->internalformat || GL_RGB9_E5 == validFormat->internalformat) &&
					contextInfo.isExtensionSupported("GL_APPLE_color_buffer_packed_float"))
				{
					return true;
				}

				if ((GL_LUMINANCE == validFormat->internalformat || GL_ALPHA == validFormat->internalformat ||
					 GL_LUMINANCE_ALPHA == validFormat->internalformat) &&
					contextInfo.isExtensionSupported("GL_NV_render_luminance_alpha"))
				{
					return true;
				}

				if ((GL_SRGB8 == validFormat->internalformat) && contextInfo.isExtensionSupported("GL_NV_sRGB_formats"))
				{
					return true;
				}

				if (((GL_R8_SNORM == validFormat->internalformat) || (GL_RG8_SNORM == validFormat->internalformat) ||
					 (GL_RGBA8_SNORM == validFormat->internalformat) || (GL_R16_SNORM == validFormat->internalformat) ||
					 (GL_RG16_SNORM == validFormat->internalformat) ||
					 (GL_RGBA16_SNORM == validFormat->internalformat)) &&
					contextInfo.isExtensionSupported("GL_EXT_render_snorm"))
				{
					return true;
				}
			}
			return validFormat->bRenderable;
		}
		else
		{
			// Check for NV_packed_float
			if (GL_RGB == internalformat.sizedFormat && GL_RGB == format && GL_UNSIGNED_INT_10F_11F_11F_REV == type &&
				contextInfo.isExtensionSupported("GL_NV_packed_float"))
			{
				return true;
			}
			return false;
		}
	}
	else
	{
		if (format == GL_DEPTH_STENCIL && internalformat.sizedFormat != GL_DEPTH24_STENCIL8 &&
			internalformat.sizedFormat != GL_DEPTH32F_STENCIL8)
		{
			// We can't make a complete DEPTH_STENCIL attachment with a
			// texture that does not have both DEPTH and STENCIL components.
			return false;
		}

		GLenum colorRenderableFrmats[] = { GL_RGBA32F,	GL_RGBA32I, GL_RGBA32UI,	 GL_RGBA16,
										   GL_RGBA16F,	GL_RGBA16I, GL_RGBA16UI,	 GL_RGBA8,
										   GL_RGBA8I,	 GL_RGBA8UI, GL_SRGB8_ALPHA8, GL_RGB10_A2,
										   GL_RGB10_A2UI, GL_RGB5_A1, GL_RGBA4,		   GL_R11F_G11F_B10F,
										   GL_RGB565,	 GL_RG32F,   GL_RG32I,		   GL_RG32UI,
										   GL_RG16,		  GL_RG16F,   GL_RG16I,		   GL_RG16UI,
										   GL_RG8,		  GL_RG8I,	GL_RG8UI,		   GL_R32F,
										   GL_R32I,		  GL_R32UI,   GL_R16F,		   GL_R16I,
										   GL_R16UI,	  GL_R16,	 GL_R8,		   GL_R8I,
										   GL_R8UI };
		GLenum* formatsEnd = colorRenderableFrmats + DE_LENGTH_OF_ARRAY(colorRenderableFrmats);
		if (std::find(colorRenderableFrmats, formatsEnd, internalformat.sizedFormat) < formatsEnd)
			return true;

		GLenum dsRenderableFormats[] = {
			GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT16,
			GL_DEPTH32F_STENCIL8,  GL_DEPTH24_STENCIL8,
		};

		formatsEnd = dsRenderableFormats + DE_LENGTH_OF_ARRAY(dsRenderableFormats);
		if (std::find(dsRenderableFormats, formatsEnd, internalformat.sizedFormat) < formatsEnd)
			return true;

		return false;
	}
}

bool RectangleTest::doRead(GLuint texture)
{
	glu::RenderContext& renderContext = m_context.getRenderContext();
	const Functions&	gl			  = renderContext.getFunctions();

	GLuint fboId;
	gl.genFramebuffers(1, &fboId);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fboId);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer");

	bool validImageAttach = isFBOImageAttachValid(m_internalFormat, m_inputFormat.format, m_inputType.type);
	if (m_textureTarget == GL_TEXTURE_2D)
		gl.framebufferTexture2D(GL_FRAMEBUFFER, m_inputFormat.attachment, GL_TEXTURE_2D, texture, 0);
	else if (glu::isContextTypeES(renderContext.getType()))
		gl.framebufferTextureLayer(GL_FRAMEBUFFER, m_inputFormat.attachment, texture, 0, 0);
	else
		gl.framebufferTexture3D(GL_FRAMEBUFFER, m_inputFormat.attachment, GL_TEXTURE_3D, texture, 0, 0);
	GLenum status = gl.checkFramebufferStatus(GL_FRAMEBUFFER);

	bool result = true;
	if (status == GL_FRAMEBUFFER_COMPLETE)
	{
		if (!validImageAttach && glu::isContextTypeES(renderContext.getType()))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "FBO is complete but expected incomplete with sizedFormat: "
							   << getFormatStr(m_internalFormat.sizedFormat) << tcu::TestLog::EndMessage;
			result = false;
		}
		else
		{
			result &= readPixels(false);
			result &= doCopy();
		}
	}
	else if (validImageAttach)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "FBO is not complete but expected complete with sizedFormat: "
						   << getFormatStr(m_internalFormat.sizedFormat) << tcu::TestLog::EndMessage;
		result = false;
	}

	gl.deleteFramebuffers(1, &fboId);

	return result;
}

bool RectangleTest::readPixels(bool isCopy)
{
	bool result = true;

	const PixelType*   types;
	int				   typesCount;
	const PixelFormat* formats;
	int				   formatsCount;

	if (glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		types		 = esTypes;
		typesCount   = DE_LENGTH_OF_ARRAY(esTypes);
		formats		 = esFormats;
		formatsCount = DE_LENGTH_OF_ARRAY(esFormats);
	}
	else
	{
		types		 = coreTypes;
		typesCount   = DE_LENGTH_OF_ARRAY(coreTypes);
		formats		 = coreFormats;
		formatsCount = DE_LENGTH_OF_ARRAY(coreFormats);
	}

	// for each output format
	for (int m = 0; m < formatsCount; ++m)
	{
		const PixelFormat& outputFormat = formats[m];

		// for each output type
		for (int n = 0; n < typesCount; ++n)
		{
			const PixelType& outputType = types[n];

			if (isCopy)
			{
				// continue when input format,type != canonical format,type and
				// output format,type != canonical format,type
				if ((outputFormat.format != m_copyInternalFormat.format ||
					 outputType.type != m_copyInternalFormat.type))
				{
					// invalid output format and type - skipping case
					continue;
				}
			}
			else if ((m_inputFormat.format != m_internalFormat.format || m_inputType.type != m_internalFormat.type) &&
					 (outputFormat.format != m_internalFormat.format || outputType.type != m_internalFormat.type))
			{
				// invalid output format and type - skipping case
				continue;
			}

			result &= readPixelsInner(outputFormat, outputType, isCopy);
		}
	}

	return result;
}

bool RectangleTest::readPixelsInner(const PixelFormat& outputFormat, const PixelType& outputType, bool isCopy)
{
	const char* copyStage = "Copy stage: ";

	GLenum readerror = readOutputData(outputFormat, outputType, OUTPUT_READPIXELS);
	if (m_outputBuffer.empty())
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "No buffer allocated" << tcu::TestLog::EndMessage;
		return false;
	}

	m_countReadPixels++;

	// Check if output format is valid
	bool outputFormatValid = isFormatValid(outputFormat, outputType, isCopy ? m_copyInternalFormat : m_internalFormat,
										   false, true, OUTPUT_READPIXELS);

	// Even if this is a valid glReadPixels format, we can't read non-existant components
	if (outputFormatValid && outputFormat.format == GL_DEPTH_STENCIL && m_inputFormat.format != GL_DEPTH_STENCIL)
		outputFormatValid = false;

	if (outputFormatValid)
	{
		if (readerror != GL_NO_ERROR)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << (isCopy ? copyStage : "")
							   << "Valid format used but glReadPixels failed for input = ["
							   << getFormatStr(m_inputFormat.format) << ", " << getTypeStr(m_inputType.type)
							   << "] output = [" << getFormatStr(outputFormat.format) << ", "
							   << getTypeStr(outputType.type) << "]" << tcu::TestLog::EndMessage;
			return false;
		}
		else
		{
			m_countReadPixelsOK++;
			m_countCompare++;

			// compare output gradient to input gradient
			if (!compare(&m_gradient[0], &m_outputBuffer[0], outputFormat, outputType, isCopy))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << (isCopy ? copyStage : "")
								   << "Gradient comparison failed during ReadPixels for input = ["
								   << getFormatStr(m_inputFormat.format) << ", " << getTypeStr(m_inputType.type)
								   << "] output = [" << getFormatStr(outputFormat.format) << ", "
								   << getTypeStr(outputType.type) << "]" << tcu::TestLog::EndMessage;
				return false;
			}

			m_countCompareOK++;
		}
	}
	else if (readerror == GL_NO_ERROR)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << (isCopy ? copyStage : "")
						   << "Invalid format used but glReadPixels succeeded for input = ["
						   << getFormatStr(m_inputFormat.format) << ", " << getTypeStr(m_inputType.type)
						   << "] output = [" << getFormatStr(outputFormat.format) << ", " << getTypeStr(outputType.type)
						   << "]" << tcu::TestLog::EndMessage;
		return false;
	}

	return true;
}

GLenum RectangleTest::readOutputData(const PixelFormat& outputFormat, const PixelType& outputType, int operation)
{
	// If using PBOs buffer object for GL_PIXEL_PACK_BUFFER must
	// be bound and not allocated before calling when using PBOs

	glu::RenderContext& renderContext = m_context.getRenderContext();
	const Functions&	gl			  = renderContext.getFunctions();

	PackedPixelsBufferProperties& props = m_packProperties;
	props.elementSize					= outputType.size;
	props.elementsInGroup				= outputType.special ? 1 : outputFormat.components;
	props.rowLength			   = (props.rowLength == 0) ? (GRADIENT_WIDTH + props.skipPixels) : props.rowLength;
	props.elementsInRowNoAlign = props.elementsInGroup * props.rowLength;
	props.elementsInRow		   = props.elementsInRowNoAlign;

	if (glu::isContextTypeES(renderContext.getType()))
	{
		props.rowCount   = 0;
		props.skipImages = 0;
	}
	else if ((operation == OUTPUT_READPIXELS) || (m_textureTarget == GL_TEXTURE_2D))
		props.skipImages = 0;

	if (props.rowCount == 0)
		props.rowCount = GRADIENT_HEIGHT + props.skipRows;
	props.imagesCount  = props.skipImages + 1;
	if (props.elementSize < props.alignment)
	{
		props.elementsInRow = (int)(props.alignment * deFloatCeil(props.elementSize * props.elementsInGroup *
																  props.rowLength / ((float)props.alignment))) /
							  props.elementSize;
	}

	int bufferSize =
		props.elementSize * props.elementsInRow * props.rowCount * props.imagesCount * (props.skipImages + 1);

	// The output buffer allocated should be initialized to a known value. After
	// a pack operation, any extra memory allocated for skipping should be
	// verified to be the original known value, untouched by the GL.
	const GLubyte defaultFillValue = 0xaa;
	m_outputBuffer.resize(static_cast<std::size_t>(bufferSize));
	std::fill(m_outputBuffer.begin(), m_outputBuffer.end(), defaultFillValue);

	GLuint packPBO;
	if (m_usePBO)
	{
		gl.genBuffers(1, &packPBO);
		gl.bindBuffer(GL_PIXEL_PACK_BUFFER, packPBO);
		gl.bufferData(GL_PIXEL_PACK_BUFFER, bufferSize, &m_outputBuffer[0], GL_STATIC_READ);
	}

	GLenum readError = GL_NO_ERROR;
	switch (operation)
	{
	case OUTPUT_GETTEXIMAGE:
		gl.getTexImage(m_textureTarget, 0, outputFormat.format, outputType.type, m_usePBO ? 0 : &m_outputBuffer[0]);
		break;

	case OUTPUT_READPIXELS:
		if (m_inputFormat.attachment != GL_DEPTH_ATTACHMENT &&
			m_inputFormat.attachment != GL_DEPTH_STENCIL_ATTACHMENT &&
			m_inputFormat.attachment != GL_STENCIL_ATTACHMENT)
			gl.readBuffer(m_inputFormat.attachment);

		readError = gl.getError();
		if (readError == GL_NO_ERROR)
			gl.readPixels(0, 0, GRADIENT_WIDTH, GRADIENT_HEIGHT, outputFormat.format, outputType.type,
						  m_usePBO ? 0 : &m_outputBuffer[0]);
		break;
	}

	if (readError == GL_NO_ERROR)
		readError = gl.getError();

	if (m_usePBO)
	{
		if (readError == GL_NO_ERROR)
		{
			GLvoid* mappedData = gl.mapBufferRange(GL_PIXEL_PACK_BUFFER, 0, bufferSize, GL_MAP_READ_BIT);
			if (!mappedData)
				return GL_INVALID_INDEX;

			std::memcpy(&m_outputBuffer[0], mappedData, bufferSize);
			gl.unmapBuffer(GL_PIXEL_PACK_BUFFER);
		}

		gl.bindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		gl.deleteBuffers(1, &packPBO);
	}

	if (m_packProperties.swapBytes && (readError == GL_NO_ERROR))
		swapBytes(outputType.size, m_outputBuffer);

	return readError;
}

bool RectangleTest::doCopy()
{
	bool result = true;

	const InternalFormat* copyInternalFormats;
	int					  internalFormatsCount;
	if (glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		copyInternalFormats  = esInternalformats;
		internalFormatsCount = DE_LENGTH_OF_ARRAY(esInternalformats);
	}
	else
	{
		copyInternalFormats  = coreInternalformats;
		internalFormatsCount = DE_LENGTH_OF_ARRAY(coreInternalformats);
	}

	if ((m_inputFormat.format == m_internalFormat.format) && (m_inputType.type == m_internalFormat.type))
	{
		for (int i = 0; i < internalFormatsCount; ++i)
		{
			m_copyInternalFormat = copyInternalFormats[i];
			result &= doCopyInner();
		}
	}

	return result;
}

bool RectangleTest::doCopyInner()
{
	glu::RenderContext& renderContext = m_context.getRenderContext();
	const Functions&	gl			  = renderContext.getFunctions();
	bool				result		  = true;

	const EnumFormats* copyFormatEnum =
		getCanonicalFormat(m_copyInternalFormat, m_copyInternalFormat.format, m_copyInternalFormat.type);

	if (copyFormatEnum != 0)
	{
		GLuint texture2;
		GLenum status;

		bool validcopy = isCopyValid(m_copyInternalFormat, m_internalFormat);

		gl.genTextures(1, &texture2);
		// Target is always GL_TEXTURE_2D
		gl.bindTexture(GL_TEXTURE_2D, texture2);
		GLenum error = gl.getError();

		// CopyTexImage to copy_internalformat (GL converts, but PixelStore is ignored)
		// Target is always GL_TEXTURE_2D
		gl.copyTexImage2D(GL_TEXTURE_2D, 0, m_copyInternalFormat.sizedFormat, 0, 0, GRADIENT_WIDTH, GRADIENT_HEIGHT, 0);
		error = gl.getError();

		// if this combination of copy_internalformat,internalformat is invalid
		if (validcopy == false)
		{
			// expect error and continue
			if (error != GL_NO_ERROR)
			{
				// Invalid format used and glCopyTexImage2D failed
				result = true;
			}
			else
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid format used but glCopyTexImage2D succeeded"
								   << tcu::TestLog::EndMessage;
				result = false;
			}
		}
		else // validcopy == true
		{
			// expect no error and continue
			if (error != GL_NO_ERROR)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Valid format used but glCopyTexImage2D failed"
								   << tcu::TestLog::EndMessage;
				result = false;
			}
			else
			{
				if (!glu::isContextTypeES(renderContext.getType()))
				{
					// if GetTexImage is supported we call the
					// inner function only as no loop needed
					const PixelFormat& outputFormat = getPixelFormat(copyFormatEnum->format);
					const PixelType&   outputType   = getPixelType(copyFormatEnum->type);
					result &= getTexImageInner(outputFormat, outputType);
				}

				GLint orginalFboId;
				gl.getIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &orginalFboId);
				GLU_EXPECT_NO_ERROR(gl.getError(), "getIntegerv");

				GLuint fboId;
				gl.genFramebuffers(1, &fboId);
				gl.bindFramebuffer(GL_FRAMEBUFFER, fboId);

				bool validImageAttach =
					isFBOImageAttachValid(m_copyInternalFormat, copyFormatEnum->format, copyFormatEnum->type);

				// attach copy_internalformat texture to FBO
				// Target is always GL_TEXTURE_2D
				const PixelFormat& copyFormat = getPixelFormat(copyFormatEnum->format);
				gl.framebufferTexture2D(GL_FRAMEBUFFER, copyFormat.attachment, GL_TEXTURE_2D, texture2, 0);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D");

				status = gl.checkFramebufferStatus(GL_FRAMEBUFFER);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glCheckFramebufferStatus");

				// if an unsized sizedFormat was given, then implementation chosen copy format
				// destination might be different than what's expected;
				// we cannot continue checking since ReadPixels might not choose a compatible
				// format, also the format may not be renderable as expected
				if (isUnsizedFormat(m_copyInternalFormat.sizedFormat))
				{
					result &= true;
				}
				else if (status == GL_FRAMEBUFFER_COMPLETE)
				{
					if (validImageAttach)
						result &= readPixels(true);
					else
					{
						m_testCtx.getLog()
							<< tcu::TestLog::Message << "Copy FBO is complete but expected incomplete with sizedFormat "
							<< getFormatStr(m_copyInternalFormat.sizedFormat) << ", attachement "
							<< copyFormat.attachment << tcu::TestLog::EndMessage;
						result = false;
					}
				}
				else if (validImageAttach)
				{
					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Copy FBO is not complete but expected complete with sizedFormat "
									   << getFormatStr(m_copyInternalFormat.sizedFormat) << ", attachement "
									   << copyFormat.attachment << tcu::TestLog::EndMessage;
					result = false;
				}

				// bind original FBO
				gl.bindFramebuffer(GL_FRAMEBUFFER, orginalFboId);
				gl.deleteFramebuffers(1, &fboId);
			}
		}

		gl.deleteTextures(1, &texture2);
	}

	return result;
}

bool RectangleTest::compare(GLvoid* gradient, GLvoid* data, const PixelFormat& outputFormat,
							const PixelType& outputType, bool isCopy) const
{
	// Compares the reference gradient data to the output data

	int iformatSampler = m_internalFormat.sampler;
	int outputSampler  = getSampler(outputType, outputFormat);
	int inputSampler   = getSampler(m_inputType, m_inputFormat);

	if (isCopy)
		iformatSampler = m_copyInternalFormat.sampler;

	int samplerIsIntUintFloat = 3; // 1: INT | 2: UINT | 3: FLOAT/UNORM/NORM
	if (m_internalFormat.sampler == SAMPLER_INT)
		samplerIsIntUintFloat = 1;
	else if (m_internalFormat.sampler == SAMPLER_UINT)
		samplerIsIntUintFloat = 2;

	std::vector<GLubyte> gradientStrip;
	if (!stripBuffer(m_unpackProperties, static_cast<const GLubyte*>(gradient), gradientStrip, false))
		return false;
	std::vector<GLubyte> dataStrip;
	if (!stripBuffer(m_packProperties, static_cast<const GLubyte*>(data), dataStrip, true))
		return false;

	if (gradientStrip.empty() || dataStrip.empty())
		return false;

	std::vector<FloatPixel> inputBuffer;
	getFloatBuffer(&gradientStrip[0], samplerIsIntUintFloat, m_inputFormat, m_inputType,
				   GRADIENT_WIDTH * GRADIENT_HEIGHT, inputBuffer);

	std::vector<FloatPixel> outputBuffer;
	getFloatBuffer(&dataStrip[0], samplerIsIntUintFloat, outputFormat, outputType, GRADIENT_WIDTH * GRADIENT_HEIGHT,
				   outputBuffer);

	std::vector<int> inputBitTable;
	getBits(m_inputType, m_inputFormat, inputBitTable);
	std::vector<int> outputBitTable;
	getBits(outputType, outputFormat, outputBitTable);
	const int* internalformatBitTable = reinterpret_cast<const int*>(&m_internalFormat.bits);
	const int* copyFormatBitTable	 = m_copyInternalFormat.bits.array;

	// make struct field iterable
	float* inputBufferFloat  = reinterpret_cast<float*>(&inputBuffer[0]);
	float* outputBufferFloat = reinterpret_cast<float*>(&outputBuffer[0]);

	int* inputBufferInt  = reinterpret_cast<int*>(&inputBuffer[0]);
	int* outputBufferInt = reinterpret_cast<int*>(&outputBuffer[0]);

	unsigned int* inputBufferUint  = reinterpret_cast<unsigned int*>(&inputBuffer[0]);
	unsigned int* outputBufferUint = reinterpret_cast<unsigned int*>(&outputBuffer[0]);

	for (int i = 0; i < GRADIENT_WIDTH * GRADIENT_HEIGHT; ++i)
	{
		for (int j = 0; j < NUM_FLOAT_PIXEL_COUNT / 3; ++j)
		{
			int bit1 = getRealBitPrecision(inputBitTable[j], inputSampler == SAMPLER_FLOAT);
			int bit2 = getRealBitPrecision(outputBitTable[j], outputSampler == SAMPLER_FLOAT);
			int bit3 = getRealBitPrecision(internalformatBitTable[j], iformatSampler == SAMPLER_FLOAT);
			int bitdiff;

			if (bit1 >= bit3)
			{
				if ((inputSampler == SAMPLER_UNORM && m_internalFormat.sampler == SAMPLER_NORM))
					bit3 -= 1;
			}

			if (bit2 <= bit3)
			{
				if (outputSampler == SAMPLER_NORM && m_internalFormat.sampler == SAMPLER_UNORM)
					bit2 -= 1;
			}

			if (!(m_internalFormat.flags & FLAG_REQ_RBO) && bit3 > 8 && m_internalFormat.sampler != SAMPLER_UINT &&
				m_internalFormat.sampler != SAMPLER_INT)
			{
				// If this internalFormat is not a required format there is no requirement
				// that the implementation uses exactly the bit width requested. For example,
				// it may substitute RGB10 for RGB12. The implementation can't make subtitutions
				// for integer formats.
				bit3 = 8;
			}

			bitdiff = std::min(std::min(bit1, bit2), bit3);
			if (isCopy)
			{
				bitdiff = std::min(bitdiff, copyFormatBitTable[j]);
			}

			if (bitdiff > 0)
			{
				if (samplerIsIntUintFloat == 1) // 1: INT
				{
					int inputValue  = inputBufferInt[NUM_FLOAT_PIXEL_COUNT * i + j];
					int outputValue = outputBufferInt[NUM_FLOAT_PIXEL_COUNT * i + j];

					if (inputSampler == SAMPLER_UINT)
						// If input data was unsigned, it should be clamped to fit into
						// internal format positive range (otherwise it may wrap and
						// yield negative internalformat values)
						inputValue = clampUnsignedValue(bit3 - 1, inputValue);
					;

					inputValue = clampSignedValue(bit3, inputValue);
					if (isCopy)
					{
						inputValue = clampSignedValue(copyFormatBitTable[j], inputValue);
					}
					if (outputSampler == SAMPLER_UINT)
						inputValue = clampUnsignedValue(bit2, inputValue);
					else
						inputValue = clampSignedValue(bit2, inputValue);

					if (inputValue != outputValue)
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "Integer comparison: " << i << ", " << j << ", "
										   << NUM_FLOAT_PIXEL_COUNT * i + j << ": " << inputValue
										   << " == " << outputValue << ": not equal." << tcu::TestLog::EndMessage;
						return false;
					}
				}
				else if (samplerIsIntUintFloat == 2) // 2: UINT
				{
					unsigned int inputValue  = inputBufferUint[NUM_FLOAT_PIXEL_COUNT * i + j + 6];
					unsigned int outputValue = outputBufferUint[NUM_FLOAT_PIXEL_COUNT * i + j + 6];

					inputValue = clampUnsignedValue(bit3, inputValue);
					if (isCopy)
						inputValue = clampUnsignedValue(copyFormatBitTable[j], inputValue);

					if (outputSampler == SAMPLER_UINT)
						inputValue = clampUnsignedValue(bit2, inputValue);
					else if (outputSampler == SAMPLER_INT)
						inputValue = clampUnsignedValue(bit2 - 1, inputValue);
					if (inputValue != outputValue)
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "Integer comparison: " << i << ", " << j << ", "
										   << NUM_FLOAT_PIXEL_COUNT * i + j << ": " << inputValue
										   << " == " << outputValue << ": not equal." << tcu::TestLog::EndMessage;
						return false;
					}
				}
				else if (samplerIsIntUintFloat == 3) // 3: FLOAT / UNORM / NORM
				{
					float inputValue  = inputBufferFloat[NUM_FLOAT_PIXEL_COUNT * i + j + 12];
					float outputValue = outputBufferFloat[NUM_FLOAT_PIXEL_COUNT * i + j + 12];
					float epsilon;

					if ((outputSampler == SAMPLER_UNORM) || (outputSampler == SAMPLER_NORM))
					{
						// Check if format is UNORM/NORM which needs different
						// precision since it implies a int->float conversion.
						epsilon = 1.0f / ((float)((1 << (bitdiff - 1))) - 1);
					}
					else if ((inputSampler == SAMPLER_FLOAT) != (outputSampler == SAMPLER_FLOAT))
					{
						// Allow for rounding in either direction for float->int conversions.
						epsilon = 1.0f / ((float)((1 << (bitdiff - 1))) - 1);
					}
					else
						epsilon = 1.0f / ((float)((1 << bitdiff) - 1));

					if (inputSampler == SAMPLER_FLOAT || outputSampler == SAMPLER_FLOAT)
					{
						// According to GL spec the precision requirement
						// of floating-point values is 1 part in 10^5.
						epsilon = deFloatMax(epsilon, 0.00001f);
					}

					if (m_internalFormat.flags & FLAG_COMPRESSED)
						epsilon = deFloatMax(epsilon, 0.1f);

					// Clamp input value to range of output
					if (iformatSampler == SAMPLER_UNORM || outputSampler == SAMPLER_UNORM)
						inputValue = deFloatMax(deFloatMin(inputValue, 1.0f), 0.0f);
					else if (iformatSampler == SAMPLER_NORM || outputSampler == SAMPLER_NORM)
						inputValue = deFloatMax(deFloatMin(inputValue, 1.0f), -1.0f);

					// Compare the input and output
					if (deFloatAbs(inputValue - outputValue) > epsilon)
					{
						m_testCtx.getLog()
							<< tcu::TestLog::Message << "Non-integer comparison: " << i << ", " << j << ", "
							<< NUM_FLOAT_PIXEL_COUNT * i + j << ", " << epsilon << ": " << inputValue
							<< " == " << outputValue << ": not equal." << tcu::TestLog::EndMessage;
						return false;
					}
				}
				else
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Sampler type cannot be recognised, so no comparison"
									   << tcu::TestLog::EndMessage;
					return false;
				}
			}
		}
	}

	return true;
}

void RectangleTest::getFloatBuffer(GLvoid* gradient, int samplerIsIntUintFloat, const PixelFormat& format,
								   const PixelType& type, int elementCount, std::vector<FloatPixel>& result) const
{
	int componentCount = format.components;
	switch (type.type)
	{
	case GL_UNSIGNED_BYTE:
		makeBuffer(gradient, format, samplerIsIntUintFloat, elementCount, componentCount, pack_UNSIGNED_BYTE, result);
		break;
	case GL_BYTE:
		makeBuffer(gradient, format, samplerIsIntUintFloat, elementCount, componentCount, pack_BYTE, result);
		break;
	case GL_UNSIGNED_SHORT:
		makeBuffer(gradient, format, samplerIsIntUintFloat, elementCount, componentCount, pack_UNSIGNED_SHORT, result);
		break;
	case GL_SHORT:
		makeBuffer(gradient, format, samplerIsIntUintFloat, elementCount, componentCount, pack_SHORT, result);
		break;
	case GL_UNSIGNED_INT:
		makeBuffer(gradient, format, samplerIsIntUintFloat, elementCount, componentCount, pack_UNSIGNED_INT, result);
		break;
	case GL_INT:
		makeBuffer(gradient, format, samplerIsIntUintFloat, elementCount, componentCount, pack_INT, result);
		break;
	case GL_HALF_FLOAT:
		makeBuffer(gradient, format, samplerIsIntUintFloat, elementCount, componentCount, pack_HALF_FLOAT, result);
		break;
	case GL_FLOAT:
		makeBuffer(gradient, format, samplerIsIntUintFloat, elementCount, componentCount, pack_FLOAT, result);
		break;
	case GL_UNSIGNED_SHORT_5_6_5:
		if (samplerIsIntUintFloat == 1)
			makeBufferPackedInt(gradient, format, elementCount, pack_UNSIGNED_SHORT_5_6_5_INT, result);
		else if (samplerIsIntUintFloat == 2)
			makeBufferPackedUint(gradient, format, elementCount, pack_UNSIGNED_SHORT_5_6_5_UINT, result);
		else if (samplerIsIntUintFloat == 3)
			makeBufferPackedFloat(gradient, format, elementCount, pack_UNSIGNED_SHORT_5_6_5, result);
		break;
	case GL_UNSIGNED_SHORT_4_4_4_4:
		if (samplerIsIntUintFloat == 1)
			makeBufferPackedInt(gradient, format, elementCount, pack_UNSIGNED_SHORT_4_4_4_4_INT, result);
		else if (samplerIsIntUintFloat == 2)
			makeBufferPackedUint(gradient, format, elementCount, pack_UNSIGNED_SHORT_4_4_4_4_UINT, result);
		else if (samplerIsIntUintFloat == 3)
			makeBufferPackedFloat(gradient, format, elementCount, pack_UNSIGNED_SHORT_4_4_4_4, result);
		break;
	case GL_UNSIGNED_SHORT_5_5_5_1:
		if (samplerIsIntUintFloat == 1)
			makeBufferPackedInt(gradient, format, elementCount, pack_UNSIGNED_SHORT_5_5_5_1_INT, result);
		else if (samplerIsIntUintFloat == 2)
			makeBufferPackedUint(gradient, format, elementCount, pack_UNSIGNED_SHORT_5_5_5_1_UINT, result);
		else if (samplerIsIntUintFloat == 3)
			makeBufferPackedFloat(gradient, format, elementCount, pack_UNSIGNED_SHORT_5_5_5_1, result);
		break;
	case GL_UNSIGNED_INT_2_10_10_10_REV:
		if (samplerIsIntUintFloat == 1)
			makeBufferPackedInt(gradient, format, elementCount, pack_UNSIGNED_INT_2_10_10_10_REV_INT, result);
		else if (samplerIsIntUintFloat == 2)
			makeBufferPackedUint(gradient, format, elementCount, pack_UNSIGNED_INT_2_10_10_10_REV_UINT, result);
		else if (samplerIsIntUintFloat == 3)
			makeBufferPackedFloat(gradient, format, elementCount, pack_UNSIGNED_INT_2_10_10_10_REV, result);
		break;
	case GL_UNSIGNED_INT_24_8:
		makeBufferPackedFloat(gradient, format, elementCount, pack_UNSIGNED_INT_24_8, result);
		break;
	case GL_UNSIGNED_INT_10F_11F_11F_REV:
		makeBufferPackedFloat(gradient, format, elementCount, pack_UNSIGNED_INT_10F_11F_11F_REV, result);
		break;
	case GL_UNSIGNED_INT_5_9_9_9_REV:
		makeBufferPackedFloat(gradient, format, elementCount, pack_UNSIGNED_INT_5_9_9_9_REV, result);
		break;
	case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
		makeBufferPackedFloat(gradient, format, elementCount, pack_FLOAT_32_UNSIGNED_INT_24_8_REV, result);
		break;
	case GL_UNSIGNED_BYTE_3_3_2:
		if (samplerIsIntUintFloat == 1)
			makeBufferPackedInt(gradient, format, elementCount, pack_UNSIGNED_BYTE_3_3_2_INT, result);
		else if (samplerIsIntUintFloat == 2)
			makeBufferPackedUint(gradient, format, elementCount, pack_UNSIGNED_BYTE_3_3_2_UINT, result);
		else if (samplerIsIntUintFloat == 3)
			makeBufferPackedFloat(gradient, format, elementCount, pack_UNSIGNED_BYTE_3_3_2, result);
		break;
	case GL_UNSIGNED_BYTE_2_3_3_REV:
		if (samplerIsIntUintFloat == 1)
			makeBufferPackedInt(gradient, format, elementCount, pack_UNSIGNED_BYTE_2_3_3_REV_INT, result);
		else if (samplerIsIntUintFloat == 2)
			makeBufferPackedUint(gradient, format, elementCount, pack_UNSIGNED_BYTE_2_3_3_REV_UINT, result);
		else if (samplerIsIntUintFloat == 3)
			makeBufferPackedFloat(gradient, format, elementCount, pack_UNSIGNED_BYTE_2_3_3_REV, result);
		break;
	case GL_UNSIGNED_SHORT_5_6_5_REV:
		if (samplerIsIntUintFloat == 1)
			makeBufferPackedInt(gradient, format, elementCount, pack_UNSIGNED_SHORT_5_6_5_REV_INT, result);
		else if (samplerIsIntUintFloat == 2)
			makeBufferPackedUint(gradient, format, elementCount, pack_UNSIGNED_SHORT_5_6_5_REV_UINT, result);
		else if (samplerIsIntUintFloat == 3)
			makeBufferPackedFloat(gradient, format, elementCount, pack_UNSIGNED_SHORT_5_6_5_REV, result);
		break;
	case GL_UNSIGNED_SHORT_4_4_4_4_REV:
		if (samplerIsIntUintFloat == 1)
			makeBufferPackedInt(gradient, format, elementCount, pack_UNSIGNED_SHORT_4_4_4_4_REV_INT, result);
		else if (samplerIsIntUintFloat == 2)
			makeBufferPackedUint(gradient, format, elementCount, pack_UNSIGNED_SHORT_4_4_4_4_REV_UINT, result);
		else if (samplerIsIntUintFloat == 3)
			makeBufferPackedFloat(gradient, format, elementCount, pack_UNSIGNED_SHORT_4_4_4_4_REV, result);
		break;
	case GL_UNSIGNED_SHORT_1_5_5_5_REV:
		if (samplerIsIntUintFloat == 1)
			makeBufferPackedInt(gradient, format, elementCount, pack_UNSIGNED_SHORT_1_5_5_5_REV_INT, result);
		else if (samplerIsIntUintFloat == 2)
			makeBufferPackedUint(gradient, format, elementCount, pack_UNSIGNED_SHORT_1_5_5_5_REV_UINT, result);
		else if (samplerIsIntUintFloat == 3)
			makeBufferPackedFloat(gradient, format, elementCount, pack_UNSIGNED_SHORT_1_5_5_5_REV, result);
		break;
	case GL_UNSIGNED_INT_8_8_8_8:
		if (samplerIsIntUintFloat == 1)
			makeBufferPackedInt(gradient, format, elementCount, pack_UNSIGNED_INT_8_8_8_8_INT, result);
		else if (samplerIsIntUintFloat == 2)
			makeBufferPackedUint(gradient, format, elementCount, pack_UNSIGNED_INT_8_8_8_8_UINT, result);
		else if (samplerIsIntUintFloat == 3)
			makeBufferPackedFloat(gradient, format, elementCount, pack_UNSIGNED_INT_8_8_8_8, result);
		break;
	case GL_UNSIGNED_INT_8_8_8_8_REV:
		if (samplerIsIntUintFloat == 1)
			makeBufferPackedInt(gradient, format, elementCount, pack_UNSIGNED_INT_8_8_8_8_REV_INT, result);
		else if (samplerIsIntUintFloat == 2)
			makeBufferPackedUint(gradient, format, elementCount, pack_UNSIGNED_INT_8_8_8_8_REV_UINT, result);
		else if (samplerIsIntUintFloat == 3)
			makeBufferPackedFloat(gradient, format, elementCount, pack_UNSIGNED_INT_8_8_8_8_REV, result);
		break;
	case GL_UNSIGNED_INT_10_10_10_2:
		if (samplerIsIntUintFloat == 1)
			makeBufferPackedInt(gradient, format, elementCount, pack_UNSIGNED_INT_10_10_10_2_INT, result);
		else if (samplerIsIntUintFloat == 2)
			makeBufferPackedUint(gradient, format, elementCount, pack_UNSIGNED_INT_10_10_10_2_UINT, result);
		else if (samplerIsIntUintFloat == 3)
			makeBufferPackedFloat(gradient, format, elementCount, pack_UNSIGNED_INT_10_10_10_2, result);
		break;
	default:
		TCU_FAIL("Unsuported type");
	}
}

template <typename Type>
void RectangleTest::makeBuffer(const GLvoid* gradient, const PixelFormat& format, int samplerIsIntUintFloat,
							   int elementCount, int componentCount, float (*pack)(Type),
							   std::vector<FloatPixel>& result) const
{
	const Type* sourceData = static_cast<const Type*>(gradient);
	result.resize(sizeof(FloatPixel) * elementCount);
	for (int i = 0; i < elementCount; ++i)
	{
		rawFloatPixel values;
		rawIntPixel   valuesInt;
		rawUintPixel  valuesUint;
		for (int j = 0; j < componentCount; j++)
		{
			if (samplerIsIntUintFloat == 1)
				valuesInt[j] = static_cast<Type>(sourceData[componentCount * i + j]);
			else if (samplerIsIntUintFloat == 2)
				valuesUint[j] = static_cast<Type>(sourceData[componentCount * i + j]);
			else if (samplerIsIntUintFloat == 3)
				values[j] = pack(sourceData[componentCount * i + j]);
		}
		if (samplerIsIntUintFloat == 1)
			result[i] = orderComponentsInt(valuesInt, format);
		else if (samplerIsIntUintFloat == 2)
			result[i] = orderComponentsUint(valuesUint, format);
		else if (samplerIsIntUintFloat == 3)
			result[i] = orderComponentsFloat(values, format);
	}
}

void RectangleTest::getBits(const PixelType& type, const PixelFormat& format, std::vector<int>& resultTable) const
{
	// return bit depth table based on type and format pair;
	// table is always NUM_FLOAT_PIXEL_COUNT digit long

	resultTable.resize(NUM_FLOAT_PIXEL_COUNT);
	std::fill(resultTable.begin(), resultTable.end(), 0);

	if (type.special == true)
	{
		std::memcpy(&resultTable[0], &type.bits, sizeof(int) * NUM_FLOAT_PIXEL_COUNT);
		if (type.type == GL_UNSIGNED_INT_5_9_9_9_REV)
		{
			//this type is another special case: it is always converted to 3-channel color (no A).
			//as results of this function are used for comparison of converted values we set A bits to 0.
			resultTable[3] = 0;
		}
	}
	else
	{
		int bits;

		if (type.type == GL_FLOAT)
			bits = 32;
		else if (type.type == GL_HALF_FLOAT)
			bits = 16;
		else
			bits = type.size << 3;

		if (format.format == GL_DEPTH_STENCIL || format.format == GL_DEPTH_COMPONENT)
			resultTable[4] = bits;

		if (format.format == GL_DEPTH_STENCIL || format.format == GL_STENCIL_INDEX ||
			format.format == GL_STENCIL_INDEX8)
		{
			resultTable[5] = bits;
		}

		if (format.format == GL_RED || format.format == GL_RG || format.format == GL_RGB || format.format == GL_RGBA ||
			format.format == GL_BGR || format.format == GL_BGRA || format.format == GL_RED_INTEGER ||
			format.format == GL_RG_INTEGER || format.format == GL_RGB_INTEGER || format.format == GL_RGBA_INTEGER ||
			format.format == GL_BGR_INTEGER || format.format == GL_BGRA_INTEGER)
		{
			resultTable[0] = bits;
		}

		if (format.format == GL_RG || format.format == GL_RGB || format.format == GL_RGBA || format.format == GL_BGR ||
			format.format == GL_BGRA || format.format == GL_RG_INTEGER || format.format == GL_RGB_INTEGER ||
			format.format == GL_RGBA_INTEGER || format.format == GL_BGR_INTEGER || format.format == GL_BGRA_INTEGER)
		{
			resultTable[1] = bits;
		}

		if (format.format == GL_RGB || format.format == GL_RGBA || format.format == GL_BGR ||
			format.format == GL_BGRA || format.format == GL_RGB_INTEGER || format.format == GL_RGBA_INTEGER ||
			format.format == GL_BGR_INTEGER || format.format == GL_BGRA_INTEGER)
		{
			resultTable[2] = bits;
		}

		if (format.format == GL_RGBA || format.format == GL_BGRA || format.format == GL_RGBA_INTEGER ||
			format.format == GL_BGRA_INTEGER)
		{
			resultTable[3] = bits;
		}
	}
}

template <typename Type>
void RectangleTest::makeBufferPackedInt(const GLvoid* gradient, const PixelFormat& format, int	 elementCount,
										void (*pack)(rawIntPixel*, Type), std::vector<FloatPixel>& result) const
{
	const Type* sourceData = static_cast<const Type*>(gradient);
	result.resize(sizeof(FloatPixel) * elementCount);
	for (int i = 0; i < elementCount; ++i)
	{
		rawIntPixel values;
		pack(&values, sourceData[i]);
		result[i] = orderComponentsInt(values, format);
	}
}

template <typename Type>
void RectangleTest::makeBufferPackedUint(const GLvoid* gradient, const PixelFormat& format, int		 elementCount,
										 void (*pack)(rawUintPixel*, Type), std::vector<FloatPixel>& result) const
{
	const Type* sourceData = static_cast<const Type*>(gradient);
	result.resize(sizeof(FloatPixel) * elementCount);
	for (int i = 0; i < elementCount; ++i)
	{
		rawUintPixel values;
		pack(&values, sourceData[i]);
		result[i] = orderComponentsUint(values, format);
	}
}

template <typename Type>
void RectangleTest::makeBufferPackedFloat(const GLvoid* gradient, const PixelFormat& format, int	   elementCount,
										  void (*pack)(rawFloatPixel*, Type), std::vector<FloatPixel>& result) const
{
	const Type* sourceData = static_cast<const Type*>(gradient);
	result.resize(sizeof(FloatPixel) * elementCount);
	for (int i = 0; i < elementCount; ++i)
	{
		rawFloatPixel values;
		pack(&values, sourceData[i]);
		result[i] = orderComponentsFloat(values, format);
	}
}

FloatPixel RectangleTest::orderComponentsInt(rawIntPixel values, const PixelFormat& format) const
{
	FloatPixel fp = { PACK_DEFAULTI,  PACK_DEFAULTI,  PACK_DEFAULTI,  PACK_DEFAULTI,  PACK_DEFAULTI,  PACK_DEFAULTI,
					  PACK_DEFAULTUI, PACK_DEFAULTUI, PACK_DEFAULTUI, PACK_DEFAULTUI, PACK_DEFAULTUI, PACK_DEFAULTUI,
					  PACK_DEFAULTF,  PACK_DEFAULTF,  PACK_DEFAULTF,  PACK_DEFAULTF,  PACK_DEFAULTF,  PACK_DEFAULTF };

	if (format.componentOrder.bits.red >= 0)
		fp.i_r = values[format.componentOrder.bits.red];
	if (format.componentOrder.bits.green >= 0)
		fp.i_g = values[format.componentOrder.bits.green];
	if (format.componentOrder.bits.blue >= 0)
		fp.i_b = values[format.componentOrder.bits.blue];
	if (format.componentOrder.bits.alpha >= 0)
		fp.i_a = values[format.componentOrder.bits.alpha];
	if (format.componentOrder.bits.depth >= 0)
		fp.i_d = values[format.componentOrder.bits.depth];
	if (format.componentOrder.bits.stencil >= 0)
		fp.i_s = values[format.componentOrder.bits.stencil];

	return fp;
}

FloatPixel RectangleTest::orderComponentsUint(rawUintPixel values, const PixelFormat& format) const
{
	FloatPixel fp = { PACK_DEFAULTI,  PACK_DEFAULTI,  PACK_DEFAULTI,  PACK_DEFAULTI,  PACK_DEFAULTI,  PACK_DEFAULTI,
					  PACK_DEFAULTUI, PACK_DEFAULTUI, PACK_DEFAULTUI, PACK_DEFAULTUI, PACK_DEFAULTUI, PACK_DEFAULTUI,
					  PACK_DEFAULTF,  PACK_DEFAULTF,  PACK_DEFAULTF,  PACK_DEFAULTF,  PACK_DEFAULTF,  PACK_DEFAULTF };

	if (format.componentOrder.bits.red >= 0)
		fp.ui_r = values[format.componentOrder.bits.red];
	if (format.componentOrder.bits.green >= 0)
		fp.ui_g = values[format.componentOrder.bits.green];
	if (format.componentOrder.bits.blue >= 0)
		fp.ui_b = values[format.componentOrder.bits.blue];
	if (format.componentOrder.bits.alpha >= 0)
		fp.ui_a = values[format.componentOrder.bits.alpha];
	if (format.componentOrder.bits.depth >= 0)
		fp.ui_d = values[format.componentOrder.bits.depth];
	if (format.componentOrder.bits.stencil >= 0)
		fp.ui_s = values[format.componentOrder.bits.stencil];

	return fp;
}

FloatPixel RectangleTest::orderComponentsFloat(rawFloatPixel values, const PixelFormat& format) const
{
	FloatPixel fp = { PACK_DEFAULTI,  PACK_DEFAULTI,  PACK_DEFAULTI,  PACK_DEFAULTI,  PACK_DEFAULTI,  PACK_DEFAULTI,
					  PACK_DEFAULTUI, PACK_DEFAULTUI, PACK_DEFAULTUI, PACK_DEFAULTUI, PACK_DEFAULTUI, PACK_DEFAULTUI,
					  PACK_DEFAULTF,  PACK_DEFAULTF,  PACK_DEFAULTF,  PACK_DEFAULTF,  PACK_DEFAULTF,  PACK_DEFAULTF };

	if (format.componentOrder.bits.red >= 0)
		fp.r = values[format.componentOrder.bits.red];
	if (format.componentOrder.bits.green >= 0)
		fp.g = values[format.componentOrder.bits.green];
	if (format.componentOrder.bits.blue >= 0)
		fp.b = values[format.componentOrder.bits.blue];
	if (format.componentOrder.bits.alpha >= 0)
		fp.a = values[format.componentOrder.bits.alpha];
	if (format.componentOrder.bits.depth >= 0)
		fp.d = values[format.componentOrder.bits.depth];
	if (format.componentOrder.bits.stencil >= 0)
		fp.s = values[format.componentOrder.bits.stencil];

	return fp;
}

unsigned int RectangleTest::getRealBitPrecision(int bits, bool isFloat) const
{
	if (!isFloat)
		return bits;
	switch (bits)
	{
	case 32:
		return 23;
	case 16:
		return 10;
	case 11:
		return 6;
	case 10:
		return 5;
	}
	return bits;
}

bool RectangleTest::stripBuffer(const PackedPixelsBufferProperties& props, const GLubyte* orginalBuffer,
								std::vector<GLubyte>& newBuffer, bool validate) const
{
	// Extracts pixel data from a buffer with specific
	// pixel store configuration into a flat buffer

	int newBufferSize = props.elementSize * props.elementsInRowNoAlign * GRADIENT_HEIGHT;
	if (!newBufferSize)
		return false;
	newBuffer.resize(newBufferSize);

	int skipBottom = ((props.skipImages * props.rowCount + props.skipRows) * props.elementsInRow) * props.elementSize;
	int skipTop	= (props.rowCount - GRADIENT_HEIGHT - props.skipRows) * props.elementsInRow * props.elementSize;
	int skipLeft   = props.skipPixels * props.elementsInGroup * props.elementSize;
	int skipRight  = (props.elementsInRow - GRADIENT_WIDTH * props.elementsInGroup) * props.elementSize - skipLeft;
	int copy	   = GRADIENT_WIDTH * props.elementsInGroup * props.elementSize;
	int skipAlign  = (props.elementsInRow - props.elementsInRowNoAlign) * props.elementSize;

	if (validate)
	{
		for (int i = 0; i < skipBottom; i++)
		{
			if (orginalBuffer[i] != m_defaultFillValue)
				return false;
		}
	}

	int index_src = skipBottom;
	int index_dst = 0;

	for (int j = 0; j < GRADIENT_HEIGHT; j++)
	{
		if (validate)
		{
			for (int i = 0; i < skipLeft; i++)
			{
				if (orginalBuffer[index_src + i] != m_defaultFillValue)
					return false;
			}
		}
		index_src += skipLeft;

		std::memcpy(&newBuffer[0] + index_dst, &orginalBuffer[0] + index_src, copy);
		index_src += copy;
		index_dst += copy;

		if (validate)
		{
			for (int i = skipAlign; i < skipRight; i++)
			{
				if (orginalBuffer[index_src + i] != m_defaultFillValue)
					return false;
			}
		}
		index_src += skipRight;
	}

	if (validate)
	{
		for (int i = 0; i < skipTop; i++)
		{
			if (orginalBuffer[index_src + i] != m_defaultFillValue)
				return false;
		}
	}
	index_src += skipTop;

	return true;
}

int RectangleTest::clampSignedValue(int bits, int value) const
{
	int max = 2147483647;
	int min = 0x80000000;

	if (bits < 32)
	{
		max = (1 << (bits - 1)) - 1;
		min = (1 << (bits - 1)) - (1 << bits);
	}

	if (value >= max)
		return max;
	else if (value <= min)
		return min;

	return value;
}

unsigned int RectangleTest::clampUnsignedValue(int bits, unsigned int value) const
{
	unsigned int max = 4294967295u;
	unsigned int min = 0;

	if (bits < 32)
	{
		max = (1 << bits) - 1;
	}

	if (value >= max)
		return max;
	else if (value <= min)
		return min;

	return value;
}

float RectangleTest::pack_UNSIGNED_BYTE(GLubyte value)
{
	return static_cast<GLfloat>(value) / std::numeric_limits<GLubyte>::max();
}

float RectangleTest::pack_BYTE(GLbyte value)
{
	return deFloatMax(static_cast<GLfloat>(value) / std::numeric_limits<GLbyte>::max(), -1.0f);
}

float RectangleTest::pack_UNSIGNED_SHORT(GLushort value)
{
	return static_cast<GLfloat>(value) / std::numeric_limits<GLushort>::max();
}

float RectangleTest::pack_SHORT(GLshort value)
{
	return deFloatMax(static_cast<GLfloat>(value) / std::numeric_limits<GLshort>::max(), -1.0f);
}

float RectangleTest::pack_UNSIGNED_INT(GLuint value)
{
	return static_cast<GLfloat>(value) / std::numeric_limits<GLuint>::max();
}

float RectangleTest::pack_INT(GLint value)
{
	return deFloatMax(static_cast<GLfloat>(value) / std::numeric_limits<GLint>::max(), -1.0f);
}

float RectangleTest::pack_HALF_FLOAT(GLhalf value)
{
	return halfFloatToFloat(value);
}

float RectangleTest::pack_FLOAT(GLfloat value)
{
	return value;
}

void RectangleTest::pack_UNSIGNED_BYTE_3_3_2(rawFloatPixel* values, GLubyte value)
{
	(*values)[0] = ((value >> 5) & 7) / 7.0f;
	(*values)[1] = ((value >> 2) & 7) / 7.0f;
	(*values)[2] = ((value >> 0) & 3) / 3.0f;
}

void RectangleTest::pack_UNSIGNED_BYTE_3_3_2_UINT(rawUintPixel* values, GLubyte value)
{
	(*values)[0] = (value >> 5) & 7;
	(*values)[1] = (value >> 2) & 7;
	(*values)[2] = (value >> 0) & 3;
}

void RectangleTest::pack_UNSIGNED_BYTE_3_3_2_INT(rawIntPixel* values, GLubyte value)
{
	(*values)[0] = (static_cast<GLbyte>(value) >> 5) & 7;
	(*values)[1] = (static_cast<GLbyte>(value) >> 2) & 7;
	(*values)[2] = (static_cast<GLbyte>(value) >> 0) & 3;
}

void RectangleTest::pack_UNSIGNED_BYTE_2_3_3_REV(rawFloatPixel* values, GLubyte value)
{
	(*values)[2] = ((value >> 6) & 3) / 3.0f;
	(*values)[1] = ((value >> 3) & 7) / 7.0f;
	(*values)[0] = ((value >> 0) & 7) / 7.0f;
}

void RectangleTest::pack_UNSIGNED_BYTE_2_3_3_REV_UINT(rawUintPixel* values, GLubyte value)
{
	(*values)[2] = (value >> 6) & 3;
	(*values)[1] = (value >> 3) & 7;
	(*values)[0] = (value >> 0) & 7;
}

void RectangleTest::pack_UNSIGNED_BYTE_2_3_3_REV_INT(rawIntPixel* values, GLubyte value)
{
	(*values)[2] = (static_cast<GLbyte>(value) >> 6) & 3;
	(*values)[1] = (static_cast<GLbyte>(value) >> 3) & 7;
	(*values)[0] = (static_cast<GLbyte>(value) >> 0) & 7;
}

void RectangleTest::pack_UNSIGNED_SHORT_5_6_5(rawFloatPixel* values, GLushort value)
{
	(*values)[0] = ((value >> 11) & 31) / 31.0f;
	(*values)[1] = ((value >> 5) & 63) / 63.0f;
	(*values)[2] = ((value >> 0) & 31) / 31.0f;
}

void RectangleTest::pack_UNSIGNED_SHORT_5_6_5_UINT(rawUintPixel* values, GLushort value)
{
	(*values)[0] = (value >> 11) & 31;
	(*values)[1] = (value >> 5) & 63;
	(*values)[2] = (value >> 0) & 31;
}

void RectangleTest::pack_UNSIGNED_SHORT_5_6_5_INT(rawIntPixel* values, GLushort value)
{
	(*values)[0] = (static_cast<GLshort>(value) >> 11) & 31;
	(*values)[1] = (static_cast<GLshort>(value) >> 5) & 63;
	(*values)[2] = (static_cast<GLshort>(value) >> 0) & 31;
}

void RectangleTest::pack_UNSIGNED_SHORT_5_6_5_REV(rawFloatPixel* values, GLushort value)
{
	(*values)[2] = ((value >> 11) & 31) / 31.0f;
	(*values)[1] = ((value >> 5) & 63) / 63.0f;
	(*values)[0] = ((value >> 0) & 31) / 31.0f;
}

void RectangleTest::pack_UNSIGNED_SHORT_5_6_5_REV_UINT(rawUintPixel* values, GLushort value)
{
	(*values)[2] = (value >> 11) & 31;
	(*values)[1] = (value >> 5) & 63;
	(*values)[0] = (value >> 0) & 31;
}

void RectangleTest::pack_UNSIGNED_SHORT_5_6_5_REV_INT(rawIntPixel* values, GLushort value)
{
	(*values)[2] = (static_cast<GLshort>(value) >> 11) & 31;
	(*values)[1] = (static_cast<GLshort>(value) >> 5) & 63;
	(*values)[0] = (static_cast<GLshort>(value) >> 0) & 31;
}

void RectangleTest::pack_UNSIGNED_SHORT_4_4_4_4(rawFloatPixel* values, GLushort value)
{
	(*values)[0] = ((value >> 12) & 15) / 15.0f;
	(*values)[1] = ((value >> 8) & 15) / 15.0f;
	(*values)[2] = ((value >> 4) & 15) / 15.0f;
	(*values)[3] = ((value >> 0) & 15) / 15.0f;
}

void RectangleTest::pack_UNSIGNED_SHORT_4_4_4_4_UINT(rawUintPixel* values, GLushort value)
{
	(*values)[0] = (value >> 12) & 15;
	(*values)[1] = (value >> 8) & 15;
	(*values)[2] = (value >> 4) & 15;
	(*values)[3] = (value >> 0) & 15;
}

void RectangleTest::pack_UNSIGNED_SHORT_4_4_4_4_INT(rawIntPixel* values, GLushort value)
{
	(*values)[0] = (static_cast<GLshort>(value) >> 12) & 15;
	(*values)[1] = (static_cast<GLshort>(value) >> 8) & 15;
	(*values)[2] = (static_cast<GLshort>(value) >> 4) & 15;
	(*values)[3] = (static_cast<GLshort>(value) >> 0) & 15;
}

void RectangleTest::pack_UNSIGNED_SHORT_4_4_4_4_REV(rawFloatPixel* values, GLushort value)
{
	(*values)[3] = ((value >> 12) & 15) / 15.0f;
	(*values)[2] = ((value >> 8) & 15) / 15.0f;
	(*values)[1] = ((value >> 4) & 15) / 15.0f;
	(*values)[0] = ((value >> 0) & 15) / 15.0f;
}

void RectangleTest::pack_UNSIGNED_SHORT_4_4_4_4_REV_UINT(rawUintPixel* values, GLushort value)
{
	(*values)[3] = (value >> 12) & 15;
	(*values)[2] = (value >> 8) & 15;
	(*values)[1] = (value >> 4) & 15;
	(*values)[0] = (value >> 0) & 15;
}

void RectangleTest::pack_UNSIGNED_SHORT_4_4_4_4_REV_INT(rawIntPixel* values, GLushort value)
{
	(*values)[3] = (static_cast<GLshort>(value) >> 12) & 15;
	(*values)[2] = (static_cast<GLshort>(value) >> 8) & 15;
	(*values)[1] = (static_cast<GLshort>(value) >> 4) & 15;
	(*values)[0] = (static_cast<GLshort>(value) >> 0) & 15;
}

void RectangleTest::pack_UNSIGNED_SHORT_5_5_5_1(rawFloatPixel* values, GLushort value)
{
	(*values)[0] = ((value >> 11) & 31) / 31.0f;
	(*values)[1] = ((value >> 6) & 31) / 31.0f;
	(*values)[2] = ((value >> 1) & 31) / 31.0f;
	(*values)[3] = ((value >> 0) & 1) / 1.0f;
}

void RectangleTest::pack_UNSIGNED_SHORT_5_5_5_1_UINT(rawUintPixel* values, GLushort value)
{
	(*values)[0] = (value >> 11) & 31;
	(*values)[1] = (value >> 6) & 31;
	(*values)[2] = (value >> 1) & 31;
	(*values)[3] = (value >> 0) & 1;
}

void RectangleTest::pack_UNSIGNED_SHORT_5_5_5_1_INT(rawIntPixel* values, GLushort value)
{
	(*values)[0] = (static_cast<GLshort>(value) >> 11) & 31;
	(*values)[1] = (static_cast<GLshort>(value) >> 6) & 31;
	(*values)[2] = (static_cast<GLshort>(value) >> 1) & 31;
	(*values)[3] = (static_cast<GLshort>(value) >> 0) & 1;
}

void RectangleTest::pack_UNSIGNED_SHORT_1_5_5_5_REV(rawFloatPixel* values, GLushort value)
{
	(*values)[3] = ((value >> 15) & 1) / 1.0f;
	(*values)[2] = ((value >> 10) & 31) / 31.0f;
	(*values)[1] = ((value >> 5) & 31) / 31.0f;
	(*values)[0] = ((value >> 0) & 31) / 31.0f;
}

void RectangleTest::pack_UNSIGNED_SHORT_1_5_5_5_REV_UINT(rawUintPixel* values, GLushort value)
{
	(*values)[3] = (value >> 15) & 1;
	(*values)[2] = (value >> 10) & 31;
	(*values)[1] = (value >> 5) & 31;
	(*values)[0] = (value >> 0) & 31;
}

void RectangleTest::pack_UNSIGNED_SHORT_1_5_5_5_REV_INT(rawIntPixel* values, GLushort value)
{
	(*values)[3] = (static_cast<GLshort>(value) >> 15) & 1;
	(*values)[2] = (static_cast<GLshort>(value) >> 10) & 31;
	(*values)[1] = (static_cast<GLshort>(value) >> 5) & 31;
	(*values)[0] = (static_cast<GLshort>(value) >> 0) & 31;
}

void RectangleTest::pack_UNSIGNED_INT_8_8_8_8(rawFloatPixel* values, GLuint value)
{
	(*values)[0] = ((value >> 24) & 255) / 255.0f;
	(*values)[1] = ((value >> 16) & 255) / 255.0f;
	(*values)[2] = ((value >> 8) & 255) / 255.0f;
	(*values)[3] = ((value >> 0) & 255) / 255.0f;
}

void RectangleTest::pack_UNSIGNED_INT_8_8_8_8_UINT(rawUintPixel* values, GLuint value)
{
	(*values)[0] = (value >> 24) & 255;
	(*values)[1] = (value >> 16) & 255;
	(*values)[2] = (value >> 8) & 255;
	(*values)[3] = (value >> 0) & 255;
}

void RectangleTest::pack_UNSIGNED_INT_8_8_8_8_INT(rawIntPixel* values, GLuint value)
{
	(*values)[0] = (static_cast<GLint>(value) >> 24) & 255;
	(*values)[1] = (static_cast<GLint>(value) >> 16) & 255;
	(*values)[2] = (static_cast<GLint>(value) >> 8) & 255;
	(*values)[3] = (static_cast<GLint>(value) >> 0) & 255;
}

void RectangleTest::pack_UNSIGNED_INT_8_8_8_8_REV(rawFloatPixel* values, GLuint value)
{
	(*values)[3] = ((value >> 24) & 255) / 255.0f;
	(*values)[2] = ((value >> 16) & 255) / 255.0f;
	(*values)[1] = ((value >> 8) & 255) / 255.0f;
	(*values)[0] = ((value >> 0) & 255) / 255.0f;
}

void RectangleTest::pack_UNSIGNED_INT_8_8_8_8_REV_UINT(rawUintPixel* values, GLuint value)
{
	(*values)[3] = (value >> 24) & 255;
	(*values)[2] = (value >> 16) & 255;
	(*values)[1] = (value >> 8) & 255;
	(*values)[0] = (value >> 0) & 255;
}

void RectangleTest::pack_UNSIGNED_INT_8_8_8_8_REV_INT(rawIntPixel* values, GLuint value)
{
	(*values)[3] = (static_cast<GLint>(value) >> 24) & 255;
	(*values)[2] = (static_cast<GLint>(value) >> 16) & 255;
	(*values)[1] = (static_cast<GLint>(value) >> 8) & 255;
	(*values)[0] = (static_cast<GLint>(value) >> 0) & 255;
}

void RectangleTest::pack_UNSIGNED_INT_10_10_10_2(rawFloatPixel* values, GLuint value)
{
	(*values)[0] = ((value >> 22) & 1023) / 1023.0f;
	(*values)[1] = ((value >> 12) & 1023) / 1023.0f;
	(*values)[2] = ((value >> 2) & 1023) / 1023.0f;
	(*values)[3] = ((value >> 0) & 3) / 3.0f;
}

void RectangleTest::pack_UNSIGNED_INT_10_10_10_2_UINT(rawUintPixel* values, GLuint value)
{
	(*values)[0] = ((value >> 22) & 1023);
	(*values)[1] = ((value >> 12) & 1023);
	(*values)[2] = ((value >> 2) & 1023);
	(*values)[3] = ((value >> 0) & 3);
}

void RectangleTest::pack_UNSIGNED_INT_10_10_10_2_INT(rawIntPixel* values, GLuint value)
{
	(*values)[0] = ((static_cast<GLint>(value) >> 22) & 1023);
	(*values)[1] = ((static_cast<GLint>(value) >> 12) & 1023);
	(*values)[2] = ((static_cast<GLint>(value) >> 2) & 1023);
	(*values)[3] = ((static_cast<GLint>(value) >> 0) & 3);
}

void RectangleTest::pack_UNSIGNED_INT_2_10_10_10_REV(rawFloatPixel* values, GLuint value)
{
	(*values)[3] = ((value >> 30) & 3) / 3.0f;
	(*values)[2] = ((value >> 20) & 1023) / 1023.0f;
	(*values)[1] = ((value >> 10) & 1023) / 1023.0f;
	(*values)[0] = ((value >> 0) & 1023) / 1023.0f;
}

void RectangleTest::pack_UNSIGNED_INT_2_10_10_10_REV_UINT(rawUintPixel* values, GLuint value)
{
	(*values)[3] = (value >> 30) & 3;
	(*values)[2] = (value >> 20) & 1023;
	(*values)[1] = (value >> 10) & 1023;
	(*values)[0] = (value >> 0) & 1023;
}

void RectangleTest::pack_UNSIGNED_INT_2_10_10_10_REV_INT(rawIntPixel* values, GLuint value)
{
	(*values)[3] = (static_cast<GLint>(value) >> 30) & 3;
	(*values)[2] = (static_cast<GLint>(value) >> 20) & 1023;
	(*values)[1] = (static_cast<GLint>(value) >> 10) & 1023;
	(*values)[0] = (static_cast<GLint>(value) >> 0) & 1023;
}

void RectangleTest::pack_UNSIGNED_INT_24_8(rawFloatPixel* values, GLuint value)
{
	(*values)[0] = ((value >> 8) & 16777215) / 16777215.0f;
	(*values)[1] = ((value >> 0) & 255) / 255.0f;
}

void RectangleTest::pack_UNSIGNED_INT_10F_11F_11F_REV(rawFloatPixel* values, GLuint value)
{
	(*values)[2] = unsignedF10ToFloat((value >> 22) & 1023);
	(*values)[1] = unsignedF11ToFloat((value >> 11) & 2047);
	(*values)[0] = unsignedF11ToFloat((value >> 0) & 2047);
}

void RectangleTest::pack_UNSIGNED_INT_5_9_9_9_REV(rawFloatPixel* values, GLuint value)
{
	const int B		 = 15;
	const int N		 = 9;
	GLint	 pExp   = ((value >> 27) & 31);
	GLuint	pBlue  = ((value >> 18) & 511);
	GLuint	pGreen = ((value >> 9) & 511);
	GLuint	pRed   = ((value >> 0) & 511);

	(*values)[2] = pBlue * pow(2.0, pExp - B - N);
	(*values)[1] = pGreen * pow(2.0, pExp - B - N);
	(*values)[0] = pRed * pow(2.0, pExp - B - N);
	(*values)[3] = 1.0;
}

void RectangleTest::pack_FLOAT_32_UNSIGNED_INT_24_8_REV(rawFloatPixel* values, F_32_UINT_24_8_REV value)
{
	(*values)[0] = value.d;
	(*values)[1] = (value.s & 255) / 255.0;
}

bool RectangleTest::getTexImage()
{
	// for each output format
	for (int m = 0; m < DE_LENGTH_OF_ARRAY(coreFormats); ++m)
	{
		const PixelFormat& outputFormat = coreFormats[m];

		// for each output type
		for (int n = 0; n < DE_LENGTH_OF_ARRAY(coreTypes); ++n)
		{
			const PixelType& outputType = coreTypes[n];

			if ((m_inputFormat.format != m_internalFormat.format || m_inputType.type != m_internalFormat.type) &&
				(outputFormat.format != m_internalFormat.format || outputType.type != m_internalFormat.type))
			{
				continue;
			}

			if (!getTexImageInner(outputFormat, outputType))
				return false;
		}
	}

	return true;
}

bool RectangleTest::getTexImageInner(const PixelFormat& outputFormat, const PixelType& outputType)
{
	bool outputFormatValid = isFormatValid(outputFormat, outputType, m_internalFormat, false, true, OUTPUT_GETTEXIMAGE);

	GLenum error = readOutputData(outputFormat, outputType, OUTPUT_GETTEXIMAGE);
	m_countGetTexImage++;

	if (!outputFormatValid)
	{
		if (error)
		{
			m_countGetTexImageOK++;
			return true;
		}

		m_testCtx.getLog() << tcu::TestLog::Message << "Expected error but got no GL error" << tcu::TestLog::EndMessage;
		return false;
	}
	else if (error)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Error during glGetTexImage" << tcu::TestLog::EndMessage;
		return false;
	}

	m_countGetTexImageOK++;
	m_countCompare++;

	// compare output gradient to input gradient
	if (compare(&m_gradient[0], &m_outputBuffer[0], outputFormat, outputType, false))
	{
		m_countCompareOK++;
		return true;
	}

	m_testCtx.getLog() << tcu::TestLog::Message << "Gradient comparison failed during GetTexImage for input = ["
					   << getFormatStr(m_inputFormat.format) << ", " << getTypeStr(m_inputType.type) << "] output = ["
					   << getFormatStr(outputFormat.format) << ", " << getTypeStr(outputType.type) << "]"
					   << tcu::TestLog::EndMessage;
	return false;
}

void RectangleTest::testAllFormatsAndTypes()
{
	DE_ASSERT((m_textureTarget == GL_TEXTURE_2D) || (m_textureTarget == GL_TEXTURE_3D));

	glu::RenderContext& renderContext = m_context.getRenderContext();
	const Functions&	gl			  = renderContext.getFunctions();
	bool				result		  = true;

	gl.clear(GL_COLOR_BUFFER_BIT);

	const PixelType*   types;
	int				   typesCount;
	const PixelFormat* formats;
	int				   formatsCount;
	if (glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		types		 = esTypes;
		typesCount   = DE_LENGTH_OF_ARRAY(esTypes);
		formats		 = esFormats;
		formatsCount = DE_LENGTH_OF_ARRAY(esFormats);
	}
	else
	{
		types		 = coreTypes;
		typesCount   = DE_LENGTH_OF_ARRAY(coreTypes);
		formats		 = coreFormats;
		formatsCount = DE_LENGTH_OF_ARRAY(coreFormats);
	}

	for (int inputFormatIndex = 0; inputFormatIndex < formatsCount; inputFormatIndex++)
	{
		m_inputFormat = formats[inputFormatIndex];

		for (int inputTypeIndex = 0; inputTypeIndex < typesCount; inputTypeIndex++)
		{
			GLenum error = 0;
			m_inputType  = types[inputTypeIndex];

			applyInitialStorageModes();

			// Create input gradient in format,type, with appropriate range
			createGradient();
			if (m_gradient.empty())
				TCU_FAIL("Could not create gradient.");

			if (m_unpackProperties.swapBytes)
				swapBytes(m_inputType.size, m_gradient);

			GLuint texture;
			gl.genTextures(1, &texture);
			gl.bindTexture(m_textureTarget, texture);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture");
			if (m_textureTarget == GL_TEXTURE_3D)
			{
				gl.texImage3D(GL_TEXTURE_3D, 0, m_internalFormat.sizedFormat, GRADIENT_WIDTH, GRADIENT_HEIGHT, 1, 0,
							  m_inputFormat.format, m_inputType.type, &m_gradient[0]);
			}
			else
			{
				gl.texImage2D(GL_TEXTURE_2D, 0, m_internalFormat.sizedFormat, GRADIENT_WIDTH, GRADIENT_HEIGHT, 0,
							  m_inputFormat.format, m_inputType.type, &m_gradient[0]);
			}

			if (m_unpackProperties.swapBytes)
				swapBytes(m_inputType.size, m_gradient);

			error = gl.getError();
			if (isFormatValid(m_inputFormat, m_inputType, m_internalFormat, true, false, INPUT_TEXIMAGE))
			{
				if (error == GL_NO_ERROR)
				{
					if (!glu::isContextTypeES(renderContext.getType()))
						result &= getTexImage();
					result &= doRead(texture);
				}
				else
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Valid format used but glTexImage2D/3D failed"
									   << tcu::TestLog::EndMessage;
					result = false;
				}
			}
			else if (error == GL_NO_ERROR)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid format used but glTexImage2D/3D succeeded"
								   << tcu::TestLog::EndMessage;
				result = false;
			}

			gl.deleteTextures(1, &texture);
		}
	}

	if (result)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
}

tcu::TestNode::IterateResult RectangleTest::iterate(void)
{
	resetInitialStorageModes();
	testAllFormatsAndTypes();
	return STOP;
}

class InitialValuesTest : public deqp::TestCase
{
public:
	InitialValuesTest(deqp::Context& context);
	virtual ~InitialValuesTest();

	tcu::TestNode::IterateResult iterate(void);
};

InitialValuesTest::InitialValuesTest(deqp::Context& context)
	: deqp::TestCase(context, "initial_values", "Verify if all UNPACK and PACK initial "
												"state matches the values in table 6.28 (6.23, in ES.)")
{
}

InitialValuesTest::~InitialValuesTest()
{
}

tcu::TestNode::IterateResult InitialValuesTest::iterate(void)
{
	glu::RenderContext& renderContext = m_context.getRenderContext();
	const Functions&	gl			  = renderContext.getFunctions();

	bool result = true;

	GLenum commonIntModes[] = { GL_UNPACK_ROW_LENGTH,   GL_UNPACK_SKIP_ROWS,   GL_UNPACK_SKIP_PIXELS,
								GL_UNPACK_IMAGE_HEIGHT, GL_UNPACK_SKIP_IMAGES, GL_PACK_ROW_LENGTH,
								GL_PACK_SKIP_ROWS,		GL_PACK_SKIP_PIXELS };

	// check if following eight storage modes are 0
	GLint i = 1;
	for (int mode = 0; mode < DE_LENGTH_OF_ARRAY(commonIntModes); mode++)
	{
		gl.getIntegerv(commonIntModes[mode], &i);
		result &= (i == 0);
	}

	// check if following two storage modes are 4
	gl.getIntegerv(GL_UNPACK_ALIGNMENT, &i);
	result &= (i == 4);
	gl.getIntegerv(GL_PACK_ALIGNMENT, &i);
	result &= (i == 4);

	// check storage modes available only in core GL
	if (!glu::isContextTypeES(renderContext.getType()))
	{
		// check if following four boolean modes are false
		GLboolean b = true;
		gl.getBooleanv(GL_UNPACK_SWAP_BYTES, &b);
		result &= (b == false);
		gl.getBooleanv(GL_UNPACK_LSB_FIRST, &b);
		result &= (b == false);
		gl.getBooleanv(GL_PACK_SWAP_BYTES, &b);
		result &= (b == false);
		gl.getBooleanv(GL_PACK_LSB_FIRST, &b);
		result &= (b == false);

		// check if following two modes are 0
		gl.getIntegerv(GL_PACK_IMAGE_HEIGHT, &i);
		result &= (i == 0);
		gl.getIntegerv(GL_PACK_SKIP_IMAGES, &i);
		result &= (i == 0);
	}

	// make sure that no pack/unpack buffers are bound
	gl.getIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &i);
	result &= (i == 0);
	gl.getIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &i);
	result &= (i == 0);

	if (result)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	return STOP;
}

class PBORectangleTest : public RectangleTest
{
public:
	PBORectangleTest(deqp::Context& context, std::string& name, InternalFormat internalFormat);
	virtual ~PBORectangleTest();
};

PBORectangleTest::PBORectangleTest(deqp::Context& context, std::string& name, InternalFormat internalFormat)
	: RectangleTest(context, name, internalFormat)
{
	m_usePBO = true;
}

PBORectangleTest::~PBORectangleTest()
{
}

class VariedRectangleTest : public RectangleTest
{
public:
	VariedRectangleTest(deqp::Context& context, std::string& name, InternalFormat internalFormat);
	virtual ~VariedRectangleTest();

	tcu::TestNode::IterateResult iterate(void);

protected:
	struct StoreMode
	{
		GLenum parameter;
		GLint* property;
		GLint  value;
	};
};

VariedRectangleTest::VariedRectangleTest(deqp::Context& context, std::string& name, InternalFormat internalFormat)
	: RectangleTest(context, name, internalFormat)
{
}

VariedRectangleTest::~VariedRectangleTest()
{
}

tcu::TestNode::IterateResult VariedRectangleTest::iterate(void)
{
	const int IMAGE_WIDTH_1  = 10;
	const int IMAGE_WIDTH_2  = 15;
	const int IMAGE_HEIGHT_1 = 10;
	const int IMAGE_HEIGHT_2 = 15;

	PackedPixelsBufferProperties& up = m_initialUnpackProperties;
	PackedPixelsBufferProperties& pp = m_initialPackProperties;

	StoreMode commonCases[] = {
		{ GL_UNPACK_ROW_LENGTH, &up.rowLength, 0 },
		{ GL_UNPACK_ROW_LENGTH, &up.rowLength, IMAGE_WIDTH_1 },
		{ GL_UNPACK_ROW_LENGTH, &up.rowLength, IMAGE_WIDTH_2 },
		{ GL_UNPACK_SKIP_ROWS, &up.skipRows, 0 },
		{ GL_UNPACK_SKIP_ROWS, &up.skipRows, 1 },
		{ GL_UNPACK_SKIP_ROWS, &up.skipRows, 2 },
		{ GL_UNPACK_SKIP_PIXELS, &up.skipPixels, 0 },
		{ GL_UNPACK_SKIP_PIXELS, &up.skipPixels, 1 },
		{ GL_UNPACK_SKIP_PIXELS, &up.skipPixels, 2 },
		{ GL_UNPACK_ALIGNMENT, &up.alignment, 1 },
		{ GL_UNPACK_ALIGNMENT, &up.alignment, 2 },
		{ GL_UNPACK_ALIGNMENT, &up.alignment, 4 },
		{ GL_UNPACK_ALIGNMENT, &up.alignment, 8 },
		{ GL_UNPACK_IMAGE_HEIGHT, &up.rowCount, 0 },
		{ GL_UNPACK_IMAGE_HEIGHT, &up.rowCount, IMAGE_HEIGHT_1 },
		{ GL_UNPACK_IMAGE_HEIGHT, &up.rowCount, IMAGE_HEIGHT_2 },
		{ GL_UNPACK_SKIP_IMAGES, &up.skipImages, 0 },
		{ GL_UNPACK_SKIP_IMAGES, &up.skipImages, 1 },
		{ GL_UNPACK_SKIP_IMAGES, &up.skipImages, 2 },
		{ GL_PACK_ROW_LENGTH, &pp.rowLength, 0 },
		{ GL_PACK_ROW_LENGTH, &pp.rowLength, IMAGE_WIDTH_1 },
		{ GL_PACK_ROW_LENGTH, &pp.rowLength, IMAGE_WIDTH_2 },
		{ GL_PACK_SKIP_ROWS, &pp.skipRows, 0 },
		{ GL_PACK_SKIP_ROWS, &pp.skipRows, 1 },
		{ GL_PACK_SKIP_ROWS, &pp.skipRows, 2 },
		{ GL_PACK_SKIP_PIXELS, &pp.skipPixels, 0 },
		{ GL_PACK_SKIP_PIXELS, &pp.skipPixels, 1 },
		{ GL_PACK_SKIP_PIXELS, &pp.skipPixels, 2 },
		{ GL_PACK_ALIGNMENT, &pp.alignment, 1 },
		{ GL_PACK_ALIGNMENT, &pp.alignment, 2 },
		{ GL_PACK_ALIGNMENT, &pp.alignment, 4 },
		{ GL_PACK_ALIGNMENT, &pp.alignment, 8 },
	};

	StoreMode coreCases[] = {
		{ GL_UNPACK_SWAP_BYTES, &up.swapBytes, GL_FALSE },
		{ GL_UNPACK_SWAP_BYTES, &up.swapBytes, GL_TRUE },
		{ GL_UNPACK_LSB_FIRST, &up.lsbFirst, GL_FALSE },
		{ GL_UNPACK_LSB_FIRST, &up.lsbFirst, GL_TRUE },
		{ GL_PACK_SWAP_BYTES, &pp.swapBytes, GL_FALSE },
		{ GL_PACK_SWAP_BYTES, &pp.swapBytes, GL_TRUE },
		{ GL_PACK_LSB_FIRST, &pp.lsbFirst, GL_FALSE },
		{ GL_PACK_LSB_FIRST, &pp.lsbFirst, GL_TRUE },
		{ GL_PACK_IMAGE_HEIGHT, &pp.rowCount, 0 },
		{ GL_PACK_IMAGE_HEIGHT, &pp.rowCount, IMAGE_HEIGHT_1 },
		{ GL_PACK_IMAGE_HEIGHT, &pp.rowCount, IMAGE_HEIGHT_2 },
		{ GL_PACK_SKIP_IMAGES, &pp.skipImages, 0 },
		{ GL_PACK_SKIP_IMAGES, &pp.skipImages, 1 },
		{ GL_PACK_SKIP_IMAGES, &pp.skipImages, 2 },
	};

	std::vector<StoreMode> testModes(commonCases, commonCases + DE_LENGTH_OF_ARRAY(commonCases));
	glu::RenderContext&	renderContext	   = m_context.getRenderContext();
	bool				   contextTypeIsCoreGL = !glu::isContextTypeES(renderContext.getType());
	if (contextTypeIsCoreGL)
		testModes.insert(testModes.end(), coreCases, coreCases + DE_LENGTH_OF_ARRAY(coreCases));

	std::vector<StoreMode>::iterator currentCase = testModes.begin();
	while (currentCase != testModes.end())
	{
		resetInitialStorageModes();

		GLenum parameter = currentCase->parameter;
		GLint  value	 = currentCase->value;

		*(currentCase->property) = value;

		// for some parameters an additional parameter needs to be set
		if (parameter == GL_PACK_SKIP_ROWS)
		{
			if (contextTypeIsCoreGL)
				m_initialPackProperties.rowCount = GRADIENT_HEIGHT + value;
		}
		else if (parameter == GL_PACK_SKIP_PIXELS)
			m_initialPackProperties.rowLength = GRADIENT_WIDTH + value;
		else if (parameter == GL_UNPACK_SKIP_ROWS)
			m_initialUnpackProperties.rowCount = GRADIENT_HEIGHT + value;
		else if (parameter == GL_UNPACK_SKIP_PIXELS)
			m_initialUnpackProperties.rowLength = GRADIENT_WIDTH + value;

		m_textureTarget = GL_TEXTURE_2D;
		if ((parameter == GL_PACK_IMAGE_HEIGHT) || (parameter == GL_PACK_SKIP_IMAGES) ||
			(parameter == GL_UNPACK_IMAGE_HEIGHT) || (parameter == GL_UNPACK_SKIP_IMAGES))
			m_textureTarget = GL_TEXTURE_3D;

		testAllFormatsAndTypes();

		if (m_testCtx.getTestResult() != QP_TEST_RESULT_PASS)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Case for: " << glu::getGettableStateStr(parameter).toString() << " = " << value
							   << " failed." << tcu::TestLog::EndMessage;
			return STOP;
		}

		++currentCase;
	}

	return STOP;
}

PackedPixelsTests::PackedPixelsTests(deqp::Context& context) : TestCaseGroup(context, "packed_pixels", "")
{
}

PackedPixelsTests::~PackedPixelsTests(void)
{
	m_testCtx.getLog() << tcu::TestLog::Message << "PackedPixelsTests statistics:"
					   << "\n  countReadPixels: " << RectangleTest::m_countReadPixels
					   << "\n  countReadPixelsOK: " << RectangleTest::m_countReadPixelsOK
					   << "\n  countGetTexImage: " << RectangleTest::m_countGetTexImage
					   << "\n  countGetTexImageOK: " << RectangleTest::m_countGetTexImageOK
					   << "\n  countCompare: " << RectangleTest::m_countCompare
					   << "\n  countCompareOK: " << RectangleTest::m_countCompareOK << tcu::TestLog::EndMessage;
}

void PackedPixelsTests::init(void)
{
	const InternalFormat* internalFormats;
	unsigned int		  internalFormatsCount;

	if (glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		internalFormats		 = esInternalformats;
		internalFormatsCount = DE_LENGTH_OF_ARRAY(esInternalformats);
	}
	else
	{
		internalFormats		 = coreInternalformats;
		internalFormatsCount = DE_LENGTH_OF_ARRAY(coreInternalformats);
	}

	TestCaseGroup* rectangleGroup = new deqp::TestCaseGroup(m_context, "rectangle", "");
	rectangleGroup->addChild(new InitialValuesTest(m_context));
	TestCaseGroup* pboRectangleGroup	= new deqp::TestCaseGroup(m_context, "pbo_rectangle", "");
	TestCaseGroup* variedRectangleGroup = new deqp::TestCaseGroup(m_context, "varied_rectangle", "");

	for (unsigned int internalFormatIndex = 0; internalFormatIndex < internalFormatsCount; internalFormatIndex++)
	{
		const InternalFormat& internalFormat	   = internalFormats[internalFormatIndex];
		std::string			  internalFormatString = getFormatStr(internalFormat.sizedFormat);

		std::string name = internalFormatString.substr(3);
		std::transform(name.begin(), name.end(), name.begin(), tolower);

		rectangleGroup->addChild(new RectangleTest(m_context, name, internalFormat));
		pboRectangleGroup->addChild(new PBORectangleTest(m_context, name, internalFormat));
		variedRectangleGroup->addChild(new VariedRectangleTest(m_context, name, internalFormat));
	}

	addChild(rectangleGroup);
	addChild(pboRectangleGroup);
	addChild(variedRectangleGroup);
}

} /* glcts namespace */
