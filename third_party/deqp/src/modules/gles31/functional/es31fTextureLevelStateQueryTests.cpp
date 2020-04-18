/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 *//*!
 * \file
 * \brief Texture level state query tests
 *//*--------------------------------------------------------------------*/

#include "es31fTextureLevelStateQueryTests.hpp"
#include "glsStateQueryUtil.hpp"
#include "tcuTestLog.hpp"
#include "gluRenderContext.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluTextureUtil.hpp"
#include "gluStrUtil.hpp"
#include "gluContextInfo.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuFormatUtil.hpp"
#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

using namespace gls::StateQueryUtil;


static bool textureTypeHasDepth (glw::GLenum textureBindTarget)
{
	switch (textureBindTarget)
	{
		case GL_TEXTURE_2D:						return false;
		case GL_TEXTURE_3D:						return true;
		case GL_TEXTURE_2D_ARRAY:				return true;
		case GL_TEXTURE_CUBE_MAP:				return false;
		case GL_TEXTURE_2D_MULTISAMPLE:			return false;
		case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:	return true;
		case GL_TEXTURE_BUFFER:					return false;
		case GL_TEXTURE_CUBE_MAP_ARRAY:			return true;
		default:
			DE_ASSERT(DE_FALSE);
			return false;
	}
}

static bool textureTypeHasHeight (glw::GLenum textureBindTarget)
{
	switch (textureBindTarget)
	{
		case GL_TEXTURE_2D:						return true;
		case GL_TEXTURE_3D:						return true;
		case GL_TEXTURE_2D_ARRAY:				return true;
		case GL_TEXTURE_CUBE_MAP:				return true;
		case GL_TEXTURE_2D_MULTISAMPLE:			return true;
		case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:	return true;
		case GL_TEXTURE_BUFFER:					return false;
		case GL_TEXTURE_CUBE_MAP_ARRAY:			return true;
		default:
			DE_ASSERT(DE_FALSE);
			return false;
	}
}

static const char* getTextureTargetExtension (glw::GLenum target)
{
	switch (target)
	{
		case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:	return "GL_OES_texture_storage_multisample_2d_array";
		case GL_TEXTURE_BUFFER:					return "GL_EXT_texture_buffer";
		case GL_TEXTURE_CUBE_MAP_ARRAY:			return "GL_EXT_texture_cube_map_array";
		default:
			DE_ASSERT(DE_FALSE);
			return DE_NULL;
	}
}

static bool isCoreTextureTarget (glw::GLenum target, const glu::ContextType& contextType)
{
	switch (target)
	{
		case GL_TEXTURE_2D:
		case GL_TEXTURE_3D:
		case GL_TEXTURE_2D_ARRAY:
		case GL_TEXTURE_CUBE_MAP:
		case GL_TEXTURE_2D_MULTISAMPLE:
			return true;

		case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		case GL_TEXTURE_BUFFER:
		case GL_TEXTURE_CUBE_MAP_ARRAY:
			return glu::contextSupports(contextType, glu::ApiType::es(3, 2));

		default:
			return false;
	}
}

struct TextureGenerationSpec
{
	struct TextureLevelSpec
	{
		int			width;
		int			height;
		int			depth;
		int			level;
		glw::GLenum internalFormat;
		bool		compressed;

		TextureLevelSpec (void)
			: width				(0)
			, height			(0)
			, depth				(0)
			, level				(0)
			, internalFormat	(GL_RGBA)
			, compressed		(false)
		{
		}
	};

	glw::GLenum						bindTarget;
	glw::GLenum						queryTarget;
	bool							immutable;
	bool							fixedSamplePos;	// !< fixed sample pos argument for multisample textures
	int								sampleCount;
	int								texBufferDataOffset;
	int								texBufferDataSize;
	bool							bindWholeArray;
	std::vector<TextureLevelSpec>	levels;
	std::string						description;

	TextureGenerationSpec (void)
		: immutable				(true)
		, fixedSamplePos		(true)
		, sampleCount			(0)
		, texBufferDataOffset	(0)
		, texBufferDataSize		(256)
		, bindWholeArray		(false)
	{
	}
};
struct IntegerPrinter
{
	static std::string	getIntegerName	(int v)		{ return de::toString(v); }
	static std::string	getFloatName	(float v)	{ return de::toString(v); }
};

struct PixelFormatPrinter
{
	static std::string	getIntegerName	(int v)		{ return de::toString(glu::getTextureFormatStr(v));		}
	static std::string	getFloatName	(float v)	{ return de::toString(glu::getTextureFormatStr((int)v));	}
};

template <typename Printer>
static bool verifyTextureLevelParameterEqualWithPrinter (glu::CallLogWrapper& gl, glw::GLenum target, int level, glw::GLenum pname, int refValue, QueryType type)
{
	QueriedState			state;
	tcu::ResultCollector	result	(gl.getLog(), " // ERROR: ");

	gl.getLog() << tcu::TestLog::Message << "Verifying " << glu::getTextureLevelParameterStr(pname) << ", expecting " << Printer::getIntegerName(refValue) << tcu::TestLog::EndMessage;
	queryTextureLevelState(result, gl, type, target, level, pname, state);

	if (state.isUndefined())
		return false;

	verifyInteger(result, state, refValue);

	return result.getResult() == QP_TEST_RESULT_PASS;
}

static bool verifyTextureLevelParameterEqual (glu::CallLogWrapper& gl, glw::GLenum target, int level, glw::GLenum pname, int refValue, QueryType type)
{
	return verifyTextureLevelParameterEqualWithPrinter<IntegerPrinter>(gl, target, level, pname, refValue, type);
}

static bool verifyTextureLevelParameterInternalFormatEqual (glu::CallLogWrapper& gl, glw::GLenum target, int level, glw::GLenum pname, int refValue, QueryType type)
{
	return verifyTextureLevelParameterEqualWithPrinter<PixelFormatPrinter>(gl, target, level, pname, refValue, type);
}

static bool verifyTextureLevelParameterGreaterOrEqual (glu::CallLogWrapper& gl, glw::GLenum target, int level, glw::GLenum pname, int refValue, QueryType type)
{
	QueriedState			state;
	tcu::ResultCollector	result	(gl.getLog(), " // ERROR: ");

	gl.getLog() << tcu::TestLog::Message << "Verifying " << glu::getTextureLevelParameterStr(pname) << ", expecting " << refValue << " or greater" << tcu::TestLog::EndMessage;
	queryTextureLevelState(result, gl, type, target, level, pname, state);

	if (state.isUndefined())
		return false;

	verifyIntegerMin(result, state, refValue);

	return result.getResult() == QP_TEST_RESULT_PASS;
}

static bool verifyTextureLevelParameterInternalFormatAnyOf (glu::CallLogWrapper& gl, glw::GLenum target, int level, glw::GLenum pname, const int* refValues, int numRefValues, QueryType type)
{
	QueriedState			state;
	tcu::ResultCollector	result	(gl.getLog(), " // ERROR: ");

	// Log what we try to do
	{
		tcu::MessageBuilder msg(&gl.getLog());

		msg << "Verifying " << glu::getTextureLevelParameterStr(pname) << ", expecting any of {";
		for (int ndx = 0; ndx < numRefValues; ++ndx)
		{
			if (ndx != 0)
				msg << ", ";
			msg << glu::getTextureFormatStr(refValues[ndx]);
		}
		msg << "}";
		msg << tcu::TestLog::EndMessage;
	}

	queryTextureLevelState(result, gl, type, target, level, pname, state);
	if (state.isUndefined())
		return false;

	// verify
	switch (state.getType())
	{
		case DATATYPE_INTEGER:
		{
			for (int ndx = 0; ndx < numRefValues; ++ndx)
				if (state.getIntAccess() == refValues[ndx])
					return true;

			gl.getLog() << tcu::TestLog::Message << "Error: got " << state.getIntAccess() << ", (" << glu::getTextureFormatStr(state.getIntAccess()) << ")" << tcu::TestLog::EndMessage;
			return false;
		}
		case DATATYPE_FLOAT:
		{
			for (int ndx = 0; ndx < numRefValues; ++ndx)
				if (state.getFloatAccess() == (float)refValues[ndx])
					return true;

			gl.getLog() << tcu::TestLog::Message << "Error: got " << state.getFloatAccess() << ", (" << glu::getTextureFormatStr((int)state.getFloatAccess()) << ")" << tcu::TestLog::EndMessage;
			return false;
		}
		default:
			DE_ASSERT(DE_FALSE);
			return false;
	}
}

static bool isDepthFormat (const tcu::TextureFormat& fmt)
{
	return fmt.order == tcu::TextureFormat::D || fmt.order == tcu::TextureFormat::DS;
}

static bool isColorRenderableFormat (glw::GLenum internalFormat)
{
	return	internalFormat == GL_RGB565			||
			internalFormat == GL_RGBA4			||
			internalFormat == GL_RGB5_A1		||
			internalFormat == GL_RGB10_A2		||
			internalFormat == GL_RGB10_A2UI		||
			internalFormat == GL_SRGB8_ALPHA8	||
			internalFormat == GL_R8				||
			internalFormat == GL_RG8			||
			internalFormat == GL_RGB8			||
			internalFormat == GL_RGBA8			||
			internalFormat == GL_R8I			||
			internalFormat == GL_RG8I			||
			internalFormat == GL_RGBA8I			||
			internalFormat == GL_R8UI			||
			internalFormat == GL_RG8UI			||
			internalFormat == GL_RGBA8UI		||
			internalFormat == GL_R16I			||
			internalFormat == GL_RG16I			||
			internalFormat == GL_RGBA16I		||
			internalFormat == GL_R16UI			||
			internalFormat == GL_RG16UI			||
			internalFormat == GL_RGBA16UI		||
			internalFormat == GL_R32I			||
			internalFormat == GL_RG32I			||
			internalFormat == GL_RGBA32I		||
			internalFormat == GL_R32UI			||
			internalFormat == GL_RG32UI			||
			internalFormat == GL_RGBA32UI;
}

static bool isRenderableFormat (glw::GLenum internalFormat)
{
	return	isColorRenderableFormat(internalFormat)	||
			internalFormat == GL_DEPTH_COMPONENT16	||
			internalFormat == GL_DEPTH_COMPONENT24	||
			internalFormat == GL_DEPTH_COMPONENT32F	||
			internalFormat == GL_DEPTH24_STENCIL8	||
			internalFormat == GL_DEPTH32F_STENCIL8;
}

static bool isTextureBufferFormat (glw::GLenum internalFormat)
{
	return	internalFormat == GL_R8			||
			internalFormat == GL_R16F		||
			internalFormat == GL_R32F		||
			internalFormat == GL_R8I		||
			internalFormat == GL_R16I		||
			internalFormat == GL_R32I		||
			internalFormat == GL_R8UI		||
			internalFormat == GL_R16UI		||
			internalFormat == GL_R32UI		||
			internalFormat == GL_RG8		||
			internalFormat == GL_RG16F		||
			internalFormat == GL_RG32F		||
			internalFormat == GL_RG8I		||
			internalFormat == GL_RG16I		||
			internalFormat == GL_RG32I		||
			internalFormat == GL_RG8UI		||
			internalFormat == GL_RG16UI		||
			internalFormat == GL_RG32UI		||
			internalFormat == GL_RGB32F		||
			internalFormat == GL_RGB32I		||
			internalFormat == GL_RGB32UI	||
			internalFormat == GL_RGBA8		||
			internalFormat == GL_RGBA16F	||
			internalFormat == GL_RGBA32F	||
			internalFormat == GL_RGBA8I		||
			internalFormat == GL_RGBA16I	||
			internalFormat == GL_RGBA32I	||
			internalFormat == GL_RGBA8UI	||
			internalFormat == GL_RGBA16UI	||
			internalFormat == GL_RGBA32UI;
}

static bool isLegalFormatForTarget (glw::GLenum target, glw::GLenum format)
{
	const tcu::TextureFormat fmt = glu::mapGLInternalFormat(format);

	if (target == GL_TEXTURE_3D && isDepthFormat(fmt))
		return false;
	if ((target == GL_TEXTURE_2D_MULTISAMPLE || target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY) && !isRenderableFormat(format))
		return false;
	if (target == GL_TEXTURE_BUFFER || !isTextureBufferFormat(format))
		return false;
	return true;
}

static bool isCompressionSupportedForTarget (glw::GLenum target)
{
	return target == GL_TEXTURE_2D || target == GL_TEXTURE_2D_ARRAY;
}

static bool isMultisampleTarget (glw::GLenum target)
{
	return target == GL_TEXTURE_2D_MULTISAMPLE || target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
}

static bool targetSupportsMipLevels (glw::GLenum target)
{
	return	target != GL_TEXTURE_2D_MULTISAMPLE &&
			target != GL_TEXTURE_2D_MULTISAMPLE_ARRAY &&
			target != GL_TEXTURE_BUFFER;
}

static int getPixelSize (glw::GLenum internalFormat)
{
	const tcu::TextureFormat fmt = glu::mapGLInternalFormat(internalFormat);
	return fmt.getPixelSize();
}

static void generateColorTextureGenerationGroup (std::vector<TextureGenerationSpec>& group, glw::GLenum target, int maxSamples, glw::GLenum internalFormat)
{
	const glw::GLenum queryTarget = (target == GL_TEXTURE_CUBE_MAP) ? (GL_TEXTURE_CUBE_MAP_NEGATIVE_Y) : (target);

	// initial
	{
		TextureGenerationSpec texGen;
		texGen.bindTarget		= target;
		texGen.queryTarget		= queryTarget;
		texGen.immutable		= true;
		texGen.sampleCount		= 0;
		texGen.description		= glu::getTextureTargetStr(target).toString() + ", initial values";

		group.push_back(texGen);
	}

	// ms targets
	if (isMultisampleTarget(target))
	{
		{
			TextureGenerationSpec					texGen;
			TextureGenerationSpec::TextureLevelSpec	level;

			texGen.bindTarget		= target;
			texGen.queryTarget		= queryTarget;
			texGen.immutable		= true;
			texGen.sampleCount		= 1;
			texGen.fixedSamplePos	= false;
			texGen.description		= glu::getTextureTargetStr(target).toString() + ", low sample count";

			level.width				= 16;
			level.height			= 16;
			level.depth				= (textureTypeHasDepth(texGen.bindTarget)) ? (6) : (1);
			level.level				= 0;
			level.internalFormat	= internalFormat;
			level.compressed		= false;

			texGen.levels.push_back(level);
			group.push_back(texGen);
		}
		{
			TextureGenerationSpec					texGen;
			TextureGenerationSpec::TextureLevelSpec	level;

			texGen.bindTarget		= target;
			texGen.queryTarget		= queryTarget;
			texGen.immutable		= true;
			texGen.sampleCount		= maxSamples;
			texGen.fixedSamplePos	= false;
			texGen.description		= glu::getTextureTargetStr(target).toString() + ", high sample count";

			level.width				= 32;
			level.height			= 32;
			level.depth				= (textureTypeHasDepth(texGen.bindTarget)) ? (6) : (1);
			level.level				= 0;
			level.internalFormat	= internalFormat;
			level.compressed		= false;

			texGen.levels.push_back(level);
			group.push_back(texGen);
		}
		{
			TextureGenerationSpec					texGen;
			TextureGenerationSpec::TextureLevelSpec	level;

			texGen.bindTarget		= target;
			texGen.queryTarget		= queryTarget;
			texGen.immutable		= true;
			texGen.sampleCount		= maxSamples;
			texGen.fixedSamplePos	= true;
			texGen.description		= glu::getTextureTargetStr(target).toString() + ", fixed sample positions";

			level.width				= 32;
			level.height			= 32;
			level.depth				= (textureTypeHasDepth(texGen.bindTarget)) ? (6) : (1);
			level.level				= 0;
			level.internalFormat	= internalFormat;
			level.compressed		= false;

			texGen.levels.push_back(level);
			group.push_back(texGen);
		}
	}
	else if (target == GL_TEXTURE_BUFFER)
	{
		// whole buffer
		{
			TextureGenerationSpec					texGen;
			TextureGenerationSpec::TextureLevelSpec	level;
			const int								baseSize = getPixelSize(internalFormat);

			texGen.bindTarget			= target;
			texGen.queryTarget			= queryTarget;
			texGen.immutable			= true;
			texGen.description			= glu::getTextureTargetStr(target).toString() + ", whole buffer";
			texGen.texBufferDataOffset	= 0;
			texGen.texBufferDataSize	= 32 * baseSize + (baseSize - 1);
			texGen.bindWholeArray		= true;

			level.width				= 32;
			level.height			= 1;
			level.depth				= 1;
			level.level				= 0;
			level.internalFormat	= internalFormat;
			level.compressed		= false;

			texGen.levels.push_back(level);
			group.push_back(texGen);
		}
		// partial buffer
		{
			TextureGenerationSpec					texGen;
			TextureGenerationSpec::TextureLevelSpec	level;
			const int								baseSize = getPixelSize(internalFormat);

			texGen.bindTarget			= target;
			texGen.queryTarget			= queryTarget;
			texGen.immutable			= true;
			texGen.description			= glu::getTextureTargetStr(target).toString() + ", partial buffer";
			texGen.texBufferDataOffset	= 256;
			texGen.texBufferDataSize	= 16 * baseSize + (baseSize - 1);
			texGen.bindWholeArray		= false;

			level.width				= 16;
			level.height			= 1;
			level.depth				= 1;
			level.level				= 0;
			level.internalFormat	= internalFormat;
			level.compressed		= false;

			texGen.levels.push_back(level);
			group.push_back(texGen);
		}
	}
	else
	{
		// immutable
		{
			TextureGenerationSpec					texGen;
			TextureGenerationSpec::TextureLevelSpec	level;

			texGen.bindTarget		= target;
			texGen.queryTarget		= queryTarget;
			texGen.immutable		= true;
			texGen.description		= glu::getTextureTargetStr(target).toString() + ", immutable";

			level.width				= 32;
			level.height			= 32;
			level.depth				= (textureTypeHasDepth(texGen.bindTarget)) ? (6) : (1);
			level.level				= 0;
			level.internalFormat	= internalFormat;
			level.compressed		= false;

			texGen.levels.push_back(level);
			group.push_back(texGen);
		}
		// mutable
		{
			TextureGenerationSpec					texGen;
			TextureGenerationSpec::TextureLevelSpec	level;

			texGen.bindTarget		= target;
			texGen.queryTarget		= queryTarget;
			texGen.immutable		= false;
			texGen.description		= glu::getTextureTargetStr(target).toString() + ", mutable";

			level.width				= 16;
			level.height			= (target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_CUBE_MAP_ARRAY) ? (16) : (64);
			level.depth				= (textureTypeHasDepth(texGen.bindTarget)) ? (6) : (1);
			level.level				= 0;
			level.internalFormat	= internalFormat;
			level.compressed		= false;

			texGen.levels.push_back(level);
			group.push_back(texGen);
		}
		// mip3
		{
			TextureGenerationSpec					texGen;
			TextureGenerationSpec::TextureLevelSpec	level;

			texGen.bindTarget		= target;
			texGen.queryTarget		= queryTarget;
			texGen.immutable		= false;
			texGen.description		= glu::getTextureTargetStr(target).toString() + ", mip level 3";

			level.width				= 4;
			level.height			= (target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_CUBE_MAP_ARRAY) ? (4) : (8);
			level.depth				= (textureTypeHasDepth(texGen.bindTarget)) ? (6) : (1);
			level.level				= 3;
			level.internalFormat	= internalFormat;
			level.compressed		= false;

			texGen.levels.push_back(level);
			group.push_back(texGen);
		}
	}
}

static void generateInternalFormatTextureGenerationGroup (std::vector<TextureGenerationSpec>& group, glw::GLenum target)
{
	const glw::GLenum queryTarget = (target == GL_TEXTURE_CUBE_MAP) ? (GL_TEXTURE_CUBE_MAP_NEGATIVE_Y) : (target);

	// Internal formats
	static const glw::GLenum internalFormats[] =
	{
		GL_R8, GL_R8_SNORM, GL_RG8, GL_RG8_SNORM, GL_RGB8, GL_RGB8_SNORM, GL_RGB565, GL_RGBA4, GL_RGB5_A1,
		GL_RGBA8, GL_RGBA8_SNORM, GL_RGB10_A2, GL_RGB10_A2UI, GL_SRGB8, GL_SRGB8_ALPHA8, GL_R16F, GL_RG16F,
		GL_RGB16F, GL_RGBA16F, GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F, GL_R11F_G11F_B10F, GL_RGB9_E5, GL_R8I,
		GL_R8UI, GL_R16I, GL_R16UI, GL_R32I, GL_R32UI, GL_RG8I, GL_RG8UI, GL_RG16I, GL_RG16UI, GL_RG32I, GL_RG32UI,
		GL_RGB8I, GL_RGB8UI, GL_RGB16I, GL_RGB16UI, GL_RGB32I, GL_RGB32UI, GL_RGBA8I, GL_RGBA8UI, GL_RGBA16I,
		GL_RGBA16UI, GL_RGBA32I, GL_RGBA32UI,

		GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT16,
		GL_DEPTH32F_STENCIL8, GL_DEPTH24_STENCIL8
	};

	// initial
	{
		TextureGenerationSpec texGen;
		texGen.bindTarget		= target;
		texGen.queryTarget		= queryTarget;
		texGen.immutable		= true;
		texGen.sampleCount		= 0;
		texGen.fixedSamplePos	= true;
		texGen.description		= glu::getTextureTargetStr(target).toString() + ", initial values";

		group.push_back(texGen);
	}

	// test all formats
	for (int internalFormatNdx = 0; internalFormatNdx < DE_LENGTH_OF_ARRAY(internalFormats); ++internalFormatNdx)
	{
		if (!isLegalFormatForTarget(target, internalFormats[internalFormatNdx]))
			continue;

		const int								baseSize = getPixelSize(internalFormats[internalFormatNdx]);
		TextureGenerationSpec					texGen;
		TextureGenerationSpec::TextureLevelSpec	level;

		texGen.bindTarget		= target;
		texGen.queryTarget		= queryTarget;
		texGen.immutable		= true;
		texGen.sampleCount		= (isMultisampleTarget(target) ? (1) : (0));
		texGen.description		= glu::getTextureTargetStr(target).toString() + ", internal format " + glu::getTextureFormatName(internalFormats[internalFormatNdx]);

		if (target == GL_TEXTURE_BUFFER)
		{
			texGen.texBufferDataOffset	= 0;
			texGen.texBufferDataSize	= 32 * baseSize + (baseSize - 1);
			texGen.bindWholeArray		= true;
		}

		level.width				= 32;
		level.height			= (textureTypeHasHeight(target)) ? (32) : (1);
		level.depth				= (textureTypeHasDepth(target)) ? (6) : (1);
		level.level				= 0;
		level.internalFormat	= internalFormats[internalFormatNdx];
		level.compressed		= false;

		texGen.levels.push_back(level);
		group.push_back(texGen);
	}

	// test mutable rgba8 with mip level 3
	if (targetSupportsMipLevels(target))
	{
		TextureGenerationSpec					texGen;
		TextureGenerationSpec::TextureLevelSpec	level;

		texGen.bindTarget		= target;
		texGen.queryTarget		= queryTarget;
		texGen.immutable		= false;
		texGen.description		= glu::getTextureTargetStr(target).toString() + ", internal format GL_RGBA8, mip level 3";

		level.width				= 32;
		level.height			= 32;
		level.depth				= (textureTypeHasDepth(target)) ? (6) : (1);
		level.level				= 3;
		level.internalFormat	= GL_RGBA8;
		level.compressed		= false;

		texGen.levels.push_back(level);
		group.push_back(texGen);
	}
}

static void generateCompressedTextureGenerationGroup (std::vector<TextureGenerationSpec>& group, glw::GLenum target)
{
	const glw::GLenum queryTarget = (target == GL_TEXTURE_CUBE_MAP) ? (GL_TEXTURE_CUBE_MAP_NEGATIVE_Y) : (target);

	// initial
	{
		TextureGenerationSpec texGen;
		texGen.bindTarget	= target;
		texGen.queryTarget	= queryTarget;
		texGen.immutable	= true;
		texGen.description	= glu::getTextureTargetStr(target).toString() + ", initial values";

		group.push_back(texGen);
	}

	// compressed
	if (isCompressionSupportedForTarget(target))
	{
		TextureGenerationSpec					texGen;
		TextureGenerationSpec::TextureLevelSpec	level;

		texGen.bindTarget		= target;
		texGen.queryTarget		= queryTarget;
		texGen.immutable		= false;
		texGen.description		= glu::getTextureTargetStr(target).toString() + ", compressed";

		level.width				= 32;
		level.height			= 32;
		level.depth				= (target == GL_TEXTURE_2D_ARRAY) ? (2) : (1);
		level.level				= 0;
		level.internalFormat	= GL_COMPRESSED_RGB8_ETC2;
		level.compressed		= true;

		texGen.levels.push_back(level);
		group.push_back(texGen);
	}
}

static void generateTextureBufferGenerationGroup (std::vector<TextureGenerationSpec>& group, glw::GLenum target)
{
	const glw::GLenum queryTarget = (target == GL_TEXTURE_CUBE_MAP) ? (GL_TEXTURE_CUBE_MAP_NEGATIVE_Y) : (target);

	// initial
	{
		TextureGenerationSpec texGen;
		texGen.bindTarget		= target;
		texGen.queryTarget		= queryTarget;
		texGen.immutable		= true;
		texGen.sampleCount		= 0;
		texGen.description		= glu::getTextureTargetStr(target).toString() + ", initial values";

		group.push_back(texGen);
	}

	// actual specification tests are in texture_buffer tests, no need to do them here too
}

bool applyTextureGenerationSpec (glu::CallLogWrapper& gl, const TextureGenerationSpec& spec, glw::GLuint& texBuffer)
{
	bool allOk = true;

	DE_ASSERT(!(spec.immutable && spec.levels.size() > 1));		// !< immutable textures have only one level

	for (int levelNdx = 0; levelNdx < (int)spec.levels.size(); ++levelNdx)
	{
		const glu::TransferFormat transferFormat = (spec.levels[levelNdx].compressed) ? (glu::TransferFormat()) : (glu::getTransferFormat(glu::mapGLInternalFormat(spec.levels[levelNdx].internalFormat)));

		if (spec.immutable && !spec.levels[levelNdx].compressed && spec.bindTarget == GL_TEXTURE_2D)
			gl.glTexStorage2D(spec.bindTarget, 1, spec.levels[levelNdx].internalFormat, spec.levels[levelNdx].width, spec.levels[levelNdx].height);
		else if (spec.immutable && !spec.levels[levelNdx].compressed && spec.bindTarget == GL_TEXTURE_3D)
			gl.glTexStorage3D(spec.bindTarget, 1, spec.levels[levelNdx].internalFormat, spec.levels[levelNdx].width, spec.levels[levelNdx].height, spec.levels[levelNdx].depth);
		else if (spec.immutable && !spec.levels[levelNdx].compressed && spec.bindTarget == GL_TEXTURE_2D_ARRAY)
			gl.glTexStorage3D(spec.bindTarget, 1, spec.levels[levelNdx].internalFormat, spec.levels[levelNdx].width, spec.levels[levelNdx].height, spec.levels[levelNdx].depth);
		else if (spec.immutable && !spec.levels[levelNdx].compressed && spec.bindTarget == GL_TEXTURE_CUBE_MAP)
			gl.glTexStorage2D(spec.bindTarget, 1, spec.levels[levelNdx].internalFormat, spec.levels[levelNdx].width, spec.levels[levelNdx].height);
		else if (spec.immutable && !spec.levels[levelNdx].compressed && spec.bindTarget == GL_TEXTURE_2D_MULTISAMPLE)
			gl.glTexStorage2DMultisample(spec.bindTarget, spec.sampleCount, spec.levels[levelNdx].internalFormat, spec.levels[levelNdx].width, spec.levels[levelNdx].height, (spec.fixedSamplePos) ? (GL_TRUE) : (GL_FALSE));
		else if (spec.immutable && !spec.levels[levelNdx].compressed && spec.bindTarget == GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
			gl.glTexStorage3DMultisample(spec.bindTarget, spec.sampleCount, spec.levels[levelNdx].internalFormat, spec.levels[levelNdx].width, spec.levels[levelNdx].height, spec.levels[levelNdx].depth, (spec.fixedSamplePos) ? (GL_TRUE) : (GL_FALSE));
		else if (spec.immutable && !spec.levels[levelNdx].compressed && spec.bindTarget == GL_TEXTURE_CUBE_MAP_ARRAY)
			gl.glTexStorage3D(spec.bindTarget, 1, spec.levels[levelNdx].internalFormat, spec.levels[levelNdx].width, spec.levels[levelNdx].height, spec.levels[levelNdx].depth);
		else if (!spec.immutable && !spec.levels[levelNdx].compressed && spec.bindTarget == GL_TEXTURE_2D)
			gl.glTexImage2D(spec.bindTarget, spec.levels[levelNdx].level, spec.levels[levelNdx].internalFormat, spec.levels[levelNdx].width, spec.levels[levelNdx].height, 0, transferFormat.format, transferFormat.dataType, DE_NULL);
		else if (!spec.immutable && !spec.levels[levelNdx].compressed && spec.bindTarget == GL_TEXTURE_3D)
			gl.glTexImage3D(spec.bindTarget, spec.levels[levelNdx].level, spec.levels[levelNdx].internalFormat, spec.levels[levelNdx].width, spec.levels[levelNdx].height, spec.levels[levelNdx].depth, 0, transferFormat.format, transferFormat.dataType, DE_NULL);
		else if (!spec.immutable && !spec.levels[levelNdx].compressed && spec.bindTarget == GL_TEXTURE_2D_ARRAY)
			gl.glTexImage3D(spec.bindTarget, spec.levels[levelNdx].level, spec.levels[levelNdx].internalFormat, spec.levels[levelNdx].width, spec.levels[levelNdx].height, spec.levels[levelNdx].depth, 0, transferFormat.format, transferFormat.dataType, DE_NULL);
		else if (!spec.immutable && !spec.levels[levelNdx].compressed && spec.bindTarget == GL_TEXTURE_CUBE_MAP)
			gl.glTexImage2D(spec.queryTarget, spec.levels[levelNdx].level, spec.levels[levelNdx].internalFormat, spec.levels[levelNdx].width, spec.levels[levelNdx].height, 0, transferFormat.format, transferFormat.dataType, DE_NULL);
		else if (!spec.immutable && !spec.levels[levelNdx].compressed && spec.bindTarget == GL_TEXTURE_CUBE_MAP_ARRAY)
			gl.glTexImage3D(spec.bindTarget, spec.levels[levelNdx].level, spec.levels[levelNdx].internalFormat, spec.levels[levelNdx].width, spec.levels[levelNdx].height, spec.levels[levelNdx].depth, 0, transferFormat.format, transferFormat.dataType, DE_NULL);
		else if (!spec.immutable && spec.levels[levelNdx].compressed && spec.bindTarget == GL_TEXTURE_2D)
		{
			DE_ASSERT(spec.levels[levelNdx].width == 32);
			DE_ASSERT(spec.levels[levelNdx].height == 32);
			DE_ASSERT(spec.levels[levelNdx].internalFormat == GL_COMPRESSED_RGB8_ETC2);

			static const deUint8 buffer[64 * 8] = { 0 };
			gl.glCompressedTexImage2D(spec.bindTarget, spec.levels[levelNdx].level, spec.levels[levelNdx].internalFormat, spec.levels[levelNdx].width, spec.levels[levelNdx].height, 0, sizeof(buffer), buffer);
		}
		else if (!spec.immutable && spec.levels[levelNdx].compressed && spec.bindTarget == GL_TEXTURE_2D_ARRAY)
		{
			DE_ASSERT(spec.levels[levelNdx].width == 32);
			DE_ASSERT(spec.levels[levelNdx].height == 32);
			DE_ASSERT(spec.levels[levelNdx].depth == 2);
			DE_ASSERT(spec.levels[levelNdx].internalFormat == GL_COMPRESSED_RGB8_ETC2);

			static const deUint8 buffer[64 * 8 * 2] = { 0 };
			gl.glCompressedTexImage3D(spec.bindTarget, spec.levels[levelNdx].level, spec.levels[levelNdx].internalFormat, spec.levels[levelNdx].width, spec.levels[levelNdx].height, spec.levels[levelNdx].depth, 0, sizeof(buffer), buffer);
		}
		else if (spec.immutable && !spec.levels[levelNdx].compressed && spec.bindTarget == GL_TEXTURE_BUFFER)
		{
			gl.glGenBuffers(1, &texBuffer);
			gl.glBindBuffer(GL_TEXTURE_BUFFER, texBuffer);

			if (spec.bindWholeArray)
			{
				gl.glBufferData(GL_TEXTURE_BUFFER, spec.texBufferDataSize, DE_NULL, GL_STATIC_DRAW);
				gl.glTexBuffer(GL_TEXTURE_BUFFER, spec.levels[levelNdx].internalFormat, texBuffer);
			}
			else
			{
				gl.glBufferData(GL_TEXTURE_BUFFER, spec.texBufferDataOffset + spec.texBufferDataSize, DE_NULL, GL_STATIC_DRAW);
				gl.glTexBufferRange(GL_TEXTURE_BUFFER, spec.levels[levelNdx].internalFormat, texBuffer, spec.texBufferDataOffset, spec.texBufferDataSize);
			}
		}
		else
			DE_ASSERT(DE_FALSE);

		{
			const glw::GLenum err = gl.glGetError();
			if (err != GL_NO_ERROR)
			{
				gl.getLog()	<< tcu::TestLog::Message
							<< "Texture specification failed, got " + glu::getErrorStr(err).toString()
							<< tcu::TestLog::EndMessage;
				allOk = false;
			}
		}
	}

	return allOk;
}

class TextureLevelCase : public TestCase
{
public:
										TextureLevelCase		(Context& ctx, const char* name, const char* desc, glw::GLenum target, QueryType type);
										~TextureLevelCase		(void);

	void								init					(void);
	void								deinit					(void);
	IterateResult						iterate					(void);

protected:
	void								getFormatSamples		(glw::GLenum internalFormat, std::vector<int>& samples);
	bool								testConfig				(const TextureGenerationSpec& spec);
	virtual bool						checkTextureState		(glu::CallLogWrapper& gl, const TextureGenerationSpec& spec) = 0;
	virtual void						generateTestIterations	(std::vector<TextureGenerationSpec>& iterations) = 0;

	const QueryType						m_type;
	const glw::GLenum					m_target;
	glw::GLuint							m_texture;
	glw::GLuint							m_texBuffer;

private:
	int									m_iteration;
	std::vector<TextureGenerationSpec>	m_iterations;
	bool								m_allIterationsOk;
	std::vector<int>					m_failedIterations;
};

TextureLevelCase::TextureLevelCase (Context& ctx, const char* name, const char* desc, glw::GLenum target, QueryType type)
	: TestCase			(ctx, name, desc)
	, m_type			(type)
	, m_target			(target)
	, m_texture			(0)
	, m_texBuffer		(0)
	, m_iteration		(0)
	, m_allIterationsOk	(true)
{
}

TextureLevelCase::~TextureLevelCase (void)
{
	deinit();
}

void TextureLevelCase::init (void)
{
	if (!isCoreTextureTarget(m_target, m_context.getRenderContext().getType()))
	{
		const char* const targetExtension = getTextureTargetExtension(m_target);

		if (!m_context.getContextInfo().isExtensionSupported(targetExtension))
			throw tcu::NotSupportedError("Test requires " + std::string(targetExtension) + " extension");
	}

	generateTestIterations(m_iterations);

	for (int iterationNdx = 0; iterationNdx < (int)m_iterations.size(); ++iterationNdx)
		DE_ASSERT(m_iterations[iterationNdx].bindTarget == m_target);
}

void TextureLevelCase::deinit (void)
{
	if (m_texture)
	{
		m_context.getRenderContext().getFunctions().deleteTextures(1, &m_texture);
		m_texture = 0;
	}
	if (m_texBuffer)
	{
		m_context.getRenderContext().getFunctions().deleteTextures(1, &m_texBuffer);
		m_texBuffer = 0;
	}
}

void TextureLevelCase::getFormatSamples (glw::GLenum internalFormat, std::vector<int>& samples)
{
	const glw::Functions	gl			= m_context.getRenderContext().getFunctions();
	int						sampleCount	= -1;

	if (!isMultisampleTarget(m_target))
		return;

	gl.getInternalformativ(m_target, internalFormat, GL_NUM_SAMPLE_COUNTS, 1, &sampleCount);

	if (sampleCount < 0)
		throw tcu::TestError("internal format query failed");

	samples.resize(sampleCount);

	if (sampleCount > 0)
	{
		gl.getInternalformativ(m_target, internalFormat, GL_SAMPLES, sampleCount, &samples[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "get max samples");
	}
}

TextureLevelCase::IterateResult TextureLevelCase::iterate (void)
{
	const bool result = testConfig(m_iterations[m_iteration]);

	if (!result)
	{
		m_failedIterations.push_back(m_iteration);
		m_allIterationsOk = false;
	}

	if (++m_iteration < (int)m_iterations.size())
		return CONTINUE;

	if (m_allIterationsOk)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
	{
		tcu::MessageBuilder msg(&m_testCtx.getLog());

		msg << "Following iteration(s) failed: ";
		for (int ndx = 0; ndx < (int)m_failedIterations.size(); ++ndx)
		{
			if (ndx)
				msg << ", ";
			msg << (m_failedIterations[ndx] + 1);
		}
		msg << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "One or more iterations failed");
	}
	return STOP;
}

bool TextureLevelCase::testConfig (const TextureGenerationSpec& spec)
{
	const tcu::ScopedLogSection section(m_testCtx.getLog(), "Iteration", std::string() + "Iteration " + de::toString(m_iteration+1) + "/" + de::toString((int)m_iterations.size()) + " - " + spec.description);
	glu::CallLogWrapper			gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	bool						result;

	gl.enableLogging(true);

	gl.glGenTextures(1, &m_texture);
	gl.glBindTexture(spec.bindTarget, m_texture);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen tex");

	// Set the state
	applyTextureGenerationSpec(gl, spec, m_texBuffer);

	// Verify the state
	result = checkTextureState(gl, spec);

	gl.glDeleteTextures(1, &m_texture);
	m_texture = 0;

	if (m_texBuffer)
	{
		gl.glDeleteBuffers(1, &m_texBuffer);
		m_texture = 0;
	}

	return result;
}

/*--------------------------------------------------------------------*//*!
 * \brief Test texture target
 *//*--------------------------------------------------------------------*/
class TextureLevelCommonCase : public TextureLevelCase
{
public:
						TextureLevelCommonCase	(Context& ctx, const char* name, const char* desc, glw::GLenum target, QueryType type);

protected:
	virtual void		generateTestIterations	(std::vector<TextureGenerationSpec>& iterations);
};

TextureLevelCommonCase::TextureLevelCommonCase (Context& ctx, const char* name, const char* desc, glw::GLenum target, QueryType type)
	: TextureLevelCase(ctx, name, desc, target, type)
{
}

void TextureLevelCommonCase::generateTestIterations (std::vector<TextureGenerationSpec>& iterations)
{
	const glw::GLenum	internalFormat = GL_RGBA8;
	int					maxSamples;
	std::vector<int>	samples;

	getFormatSamples(internalFormat, samples);
	if (samples.empty())
		maxSamples = -1;
	else
		maxSamples = samples[0];

	generateColorTextureGenerationGroup(iterations, m_target, maxSamples, internalFormat);
}

class TextureLevelSampleCase : public TextureLevelCommonCase
{
public:
	TextureLevelSampleCase (Context& ctx, const char* name, const char* desc, glw::GLenum target, QueryType type)
		: TextureLevelCommonCase(ctx, name, desc, target, type)
	{
	}

private:
	bool checkTextureState (glu::CallLogWrapper& gl, const TextureGenerationSpec& spec)
	{
		const int queryLevel	= (spec.levels.empty()) ? (0) : (spec.levels[0].level);
		const int refValue		= (spec.levels.empty()) ? (0) : (spec.sampleCount);

		return verifyTextureLevelParameterGreaterOrEqual(gl, spec.queryTarget, queryLevel, GL_TEXTURE_SAMPLES, refValue, m_type);
	}
};

class TextureLevelFixedSamplesCase : public TextureLevelCommonCase
{
public:
	TextureLevelFixedSamplesCase (Context& ctx, const char* name, const char* desc, glw::GLenum target, QueryType type)
		: TextureLevelCommonCase(ctx, name, desc, target, type)
	{
	}

private:
	bool checkTextureState (glu::CallLogWrapper& gl, const TextureGenerationSpec& spec)
	{
		const int queryLevel	= (spec.levels.empty()) ? (0) : (spec.levels[0].level);
		const int refValue		= (spec.levels.empty()) ? (1) : ((spec.fixedSamplePos) ? (1) : (0));

		return verifyTextureLevelParameterEqual(gl, spec.queryTarget, queryLevel, GL_TEXTURE_FIXED_SAMPLE_LOCATIONS, refValue, m_type);
	}
};

class TextureLevelWidthCase : public TextureLevelCommonCase
{
public:
	TextureLevelWidthCase (Context& ctx, const char* name, const char* desc, glw::GLenum target, QueryType type)
		: TextureLevelCommonCase(ctx, name, desc, target, type)
	{
	}

private:
	bool checkTextureState (glu::CallLogWrapper& gl, const TextureGenerationSpec& spec)
	{
		const int	initialValue	= 0;
		bool		allOk			= true;

		if (spec.levels.empty())
		{
			const int queryLevel	= 0;
			const int refValue		= initialValue;

			allOk &= verifyTextureLevelParameterEqual(gl, spec.queryTarget, queryLevel, GL_TEXTURE_WIDTH, refValue, m_type);
		}
		else
		{
			for (int levelNdx = 0; levelNdx < (int)spec.levels.size(); ++levelNdx)
			{
				const int queryLevel	= spec.levels[levelNdx].level;
				const int refValue		= spec.levels[levelNdx].width;

				allOk &= verifyTextureLevelParameterEqual(gl, spec.queryTarget, queryLevel, GL_TEXTURE_WIDTH, refValue, m_type);
			}
		}

		return allOk;
	}
};

class TextureLevelHeightCase : public TextureLevelCommonCase
{
public:
	TextureLevelHeightCase (Context& ctx, const char* name, const char* desc, glw::GLenum target, QueryType type)
		: TextureLevelCommonCase(ctx, name, desc, target, type)
	{
	}

private:
	bool checkTextureState (glu::CallLogWrapper& gl, const TextureGenerationSpec& spec)
	{
		const int	initialValue	= 0;
		bool		allOk			= true;

		if (spec.levels.empty())
		{
			const int queryLevel	= 0;
			const int refValue		= initialValue;

			allOk &= verifyTextureLevelParameterEqual(gl, spec.queryTarget, queryLevel, GL_TEXTURE_HEIGHT, refValue, m_type);
		}
		else
		{
			for (int levelNdx = 0; levelNdx < (int)spec.levels.size(); ++levelNdx)
			{
				const int queryLevel	= spec.levels[levelNdx].level;
				const int refValue		= spec.levels[levelNdx].height;

				allOk &= verifyTextureLevelParameterEqual(gl, spec.queryTarget, queryLevel, GL_TEXTURE_HEIGHT, refValue, m_type);
			}
		}

		return allOk;
	}
};

class TextureLevelDepthCase : public TextureLevelCommonCase
{
public:
	TextureLevelDepthCase (Context& ctx, const char* name, const char* desc, glw::GLenum target, QueryType type)
		: TextureLevelCommonCase(ctx, name, desc, target, type)
	{
	}

private:

	bool checkTextureState (glu::CallLogWrapper& gl, const TextureGenerationSpec& spec)
	{
		const int	initialValue	= 0;
		bool		allOk			= true;

		if (spec.levels.empty())
		{
			const int queryLevel	= 0;
			const int refValue		= initialValue;

			allOk &= verifyTextureLevelParameterEqual(gl, spec.queryTarget, queryLevel, GL_TEXTURE_DEPTH, refValue, m_type);
		}
		else
		{
			for (int levelNdx = 0; levelNdx < (int)spec.levels.size(); ++levelNdx)
			{
				const int queryLevel	= spec.levels[levelNdx].level;
				const int refValue		= spec.levels[levelNdx].depth;

				allOk &= verifyTextureLevelParameterEqual(gl, spec.queryTarget, queryLevel, GL_TEXTURE_DEPTH, refValue, m_type);
			}
		}

		return allOk;
	}
};

class TextureLevelInternalFormatCase : public TextureLevelCase
{
public:
	TextureLevelInternalFormatCase (Context& ctx, const char* name, const char* desc, glw::GLenum target, QueryType type)
		: TextureLevelCase(ctx, name, desc, target, type)
	{
	}

private:
	void generateTestIterations (std::vector<TextureGenerationSpec>& iterations)
	{
		generateInternalFormatTextureGenerationGroup(iterations, m_target);
	}

	bool checkTextureState (glu::CallLogWrapper& gl, const TextureGenerationSpec& spec)
	{
		bool allOk = true;

		if (spec.levels.empty())
		{
			const int queryLevel		= 0;
			const int initialValues[2]	= { GL_RGBA, GL_R8 };

			allOk &= verifyTextureLevelParameterInternalFormatAnyOf(gl, spec.queryTarget, queryLevel, GL_TEXTURE_INTERNAL_FORMAT, initialValues, DE_LENGTH_OF_ARRAY(initialValues), m_type);
		}
		else
		{
			for (int levelNdx = 0; levelNdx < (int)spec.levels.size(); ++levelNdx)
			{
				const int queryLevel	= spec.levels[levelNdx].level;
				const int refValue		= spec.levels[levelNdx].internalFormat;

				allOk &= verifyTextureLevelParameterInternalFormatEqual(gl, spec.queryTarget, queryLevel, GL_TEXTURE_INTERNAL_FORMAT, refValue, m_type);
			}
		}

		return allOk;
	}
};

class TextureLevelSizeCase : public TextureLevelCase
{
public:
						TextureLevelSizeCase			(Context& ctx, const char* name, const char* desc, glw::GLenum target, QueryType type, glw::GLenum pname);

private:
	void				generateTestIterations			(std::vector<TextureGenerationSpec>& iterations);
	bool				checkTextureState				(glu::CallLogWrapper& gl, const TextureGenerationSpec& spec);
	int					getMinimumComponentResolution	(glw::GLenum internalFormat);

	const glw::GLenum	m_pname;
};

TextureLevelSizeCase::TextureLevelSizeCase (Context& ctx, const char* name, const char* desc, glw::GLenum target, QueryType type, glw::GLenum pname)
	: TextureLevelCase	(ctx, name, desc, target, type)
	, m_pname			(pname)
{
}

void TextureLevelSizeCase::generateTestIterations (std::vector<TextureGenerationSpec>& iterations)
{
	generateInternalFormatTextureGenerationGroup(iterations, m_target);
}

bool TextureLevelSizeCase::checkTextureState (glu::CallLogWrapper& gl, const TextureGenerationSpec& spec)
{
	bool allOk = true;

	if (spec.levels.empty())
	{
		allOk &= verifyTextureLevelParameterEqual(gl, spec.queryTarget, 0, m_pname, 0, m_type);
	}
	else
	{
		for (int levelNdx = 0; levelNdx < (int)spec.levels.size(); ++levelNdx)
		{
			const int queryLevel	= spec.levels[levelNdx].level;
			const int refValue		= getMinimumComponentResolution(spec.levels[levelNdx].internalFormat);

			allOk &= verifyTextureLevelParameterGreaterOrEqual(gl, spec.queryTarget, queryLevel, m_pname, refValue, m_type);
		}
	}

	return allOk;
}

int TextureLevelSizeCase::getMinimumComponentResolution (glw::GLenum internalFormat)
{
	const tcu::TextureFormat	format			= glu::mapGLInternalFormat(internalFormat);
	const tcu::IVec4			channelBitDepth	= tcu::getTextureFormatBitDepth(format);

	switch (m_pname)
	{
		case GL_TEXTURE_RED_SIZE:
			if (format.order == tcu::TextureFormat::R		||
				format.order == tcu::TextureFormat::RG		||
				format.order == tcu::TextureFormat::RGB		||
				format.order == tcu::TextureFormat::RGBA	||
				format.order == tcu::TextureFormat::BGRA	||
				format.order == tcu::TextureFormat::ARGB	||
				format.order == tcu::TextureFormat::sRGB	||
				format.order == tcu::TextureFormat::sRGBA)
				return channelBitDepth[0];
			else
				return 0;

		case GL_TEXTURE_GREEN_SIZE:
			if (format.order == tcu::TextureFormat::RG		||
				format.order == tcu::TextureFormat::RGB		||
				format.order == tcu::TextureFormat::RGBA	||
				format.order == tcu::TextureFormat::BGRA	||
				format.order == tcu::TextureFormat::ARGB	||
				format.order == tcu::TextureFormat::sRGB	||
				format.order == tcu::TextureFormat::sRGBA)
				return channelBitDepth[1];
			else
				return 0;

		case GL_TEXTURE_BLUE_SIZE:
			if (format.order == tcu::TextureFormat::RGB		||
				format.order == tcu::TextureFormat::RGBA	||
				format.order == tcu::TextureFormat::BGRA	||
				format.order == tcu::TextureFormat::ARGB	||
				format.order == tcu::TextureFormat::sRGB	||
				format.order == tcu::TextureFormat::sRGBA)
				return channelBitDepth[2];
			else
				return 0;

		case GL_TEXTURE_ALPHA_SIZE:
			if (format.order == tcu::TextureFormat::RGBA	||
				format.order == tcu::TextureFormat::BGRA	||
				format.order == tcu::TextureFormat::ARGB	||
				format.order == tcu::TextureFormat::sRGBA)
				return channelBitDepth[3];
			else
				return 0;

		case GL_TEXTURE_DEPTH_SIZE:
			if (format.order == tcu::TextureFormat::D	||
				format.order == tcu::TextureFormat::DS)
				return channelBitDepth[0];
			else
				return 0;

		case GL_TEXTURE_STENCIL_SIZE:
			if (format.order == tcu::TextureFormat::DS)
				return channelBitDepth[3];
			else
				return 0;

		case GL_TEXTURE_SHARED_SIZE:
			if (internalFormat == GL_RGB9_E5)
				return 5;
			else
				return 0;
		default:
			DE_ASSERT(DE_FALSE);
			return 0;
	}
}

class TextureLevelTypeCase : public TextureLevelCase
{
public:
						TextureLevelTypeCase			(Context& ctx, const char* name, const char* desc, glw::GLenum target, QueryType type, glw::GLenum pname);

private:
	void				generateTestIterations			(std::vector<TextureGenerationSpec>& iterations);
	bool				checkTextureState				(glu::CallLogWrapper& gl, const TextureGenerationSpec& spec);
	int					getComponentType				(glw::GLenum internalFormat);

	const glw::GLenum	m_pname;
};

TextureLevelTypeCase::TextureLevelTypeCase (Context& ctx, const char* name, const char* desc, glw::GLenum target, QueryType type, glw::GLenum pname)
	: TextureLevelCase	(ctx, name, desc, target, type)
	, m_pname			(pname)
{
}

void TextureLevelTypeCase::generateTestIterations (std::vector<TextureGenerationSpec>& iterations)
{
	generateInternalFormatTextureGenerationGroup(iterations, m_target);
}

bool TextureLevelTypeCase::checkTextureState (glu::CallLogWrapper& gl, const TextureGenerationSpec& spec)
{
	bool allOk = true;

	if (spec.levels.empty())
	{
		allOk &= verifyTextureLevelParameterEqual(gl, spec.queryTarget, 0, m_pname, GL_NONE, m_type);
	}
	else
	{
		for (int levelNdx = 0; levelNdx < (int)spec.levels.size(); ++levelNdx)
		{
			const int queryLevel	= spec.levels[levelNdx].level;
			const int refValue		= getComponentType(spec.levels[levelNdx].internalFormat);

			allOk &= verifyTextureLevelParameterEqual(gl, spec.queryTarget, queryLevel, m_pname, refValue, m_type);
		}
	}

	return allOk;
}

int TextureLevelTypeCase::getComponentType (glw::GLenum internalFormat)
{
	const tcu::TextureFormat		format			= glu::mapGLInternalFormat(internalFormat);
	const tcu::TextureChannelClass	channelClass	= tcu::getTextureChannelClass(format.type);
	glw::GLenum						channelType		= GL_NONE;

	// depth-stencil special cases
	if (format.type == tcu::TextureFormat::UNSIGNED_INT_24_8)
	{
		if (m_pname == GL_TEXTURE_DEPTH_TYPE)
			return GL_UNSIGNED_NORMALIZED;
		else
			return GL_NONE;
	}
	else if (format.type == tcu::TextureFormat::FLOAT_UNSIGNED_INT_24_8_REV)
	{
		if (m_pname == GL_TEXTURE_DEPTH_TYPE)
			return GL_FLOAT;
		else
			return GL_NONE;
	}
	else
	{
		switch (channelClass)
		{
			case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:		channelType = GL_SIGNED_NORMALIZED;		break;
			case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:		channelType = GL_UNSIGNED_NORMALIZED;	break;
			case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:			channelType = GL_INT;					break;
			case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:			channelType = GL_UNSIGNED_INT;			break;
			case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:			channelType = GL_FLOAT;					break;
			default:
				DE_ASSERT(DE_FALSE);
		}
	}

	switch (m_pname)
	{
		case GL_TEXTURE_RED_TYPE:
			if (format.order == tcu::TextureFormat::R		||
				format.order == tcu::TextureFormat::RG		||
				format.order == tcu::TextureFormat::RGB		||
				format.order == tcu::TextureFormat::RGBA	||
				format.order == tcu::TextureFormat::BGRA	||
				format.order == tcu::TextureFormat::ARGB	||
				format.order == tcu::TextureFormat::sRGB	||
				format.order == tcu::TextureFormat::sRGBA)
				return channelType;
			else
				return GL_NONE;

		case GL_TEXTURE_GREEN_TYPE:
			if (format.order == tcu::TextureFormat::RG		||
				format.order == tcu::TextureFormat::RGB		||
				format.order == tcu::TextureFormat::RGBA	||
				format.order == tcu::TextureFormat::BGRA	||
				format.order == tcu::TextureFormat::ARGB	||
				format.order == tcu::TextureFormat::sRGB	||
				format.order == tcu::TextureFormat::sRGBA)
				return channelType;
			else
				return GL_NONE;

		case GL_TEXTURE_BLUE_TYPE:
			if (format.order == tcu::TextureFormat::RGB		||
				format.order == tcu::TextureFormat::RGBA	||
				format.order == tcu::TextureFormat::BGRA	||
				format.order == tcu::TextureFormat::ARGB	||
				format.order == tcu::TextureFormat::sRGB	||
				format.order == tcu::TextureFormat::sRGBA)
				return channelType;
			else
				return GL_NONE;

		case GL_TEXTURE_ALPHA_TYPE:
			if (format.order == tcu::TextureFormat::RGBA	||
				format.order == tcu::TextureFormat::BGRA	||
				format.order == tcu::TextureFormat::ARGB	||
				format.order == tcu::TextureFormat::sRGBA)
				return channelType;
			else
				return GL_NONE;

		case GL_TEXTURE_DEPTH_TYPE:
			if (format.order == tcu::TextureFormat::D	||
				format.order == tcu::TextureFormat::DS)
				return channelType;
			else
				return GL_NONE;

		default:
			DE_ASSERT(DE_FALSE);
			return 0;
	}
}

class TextureLevelCompressedCase : public TextureLevelCase
{
public:
	TextureLevelCompressedCase (Context& ctx, const char* name, const char* desc, glw::GLenum target, QueryType type)
		: TextureLevelCase(ctx, name, desc, target, type)
	{
	}

private:
	void generateTestIterations (std::vector<TextureGenerationSpec>& iterations)
	{
		generateCompressedTextureGenerationGroup(iterations, m_target);
	}

	bool checkTextureState (glu::CallLogWrapper& gl, const TextureGenerationSpec& spec)
	{
		bool allOk = true;

		if (spec.levels.empty())
		{
			allOk &= verifyTextureLevelParameterEqual(gl, spec.queryTarget, 0, GL_TEXTURE_COMPRESSED, 0, m_type);
		}
		else
		{
			for (int levelNdx = 0; levelNdx < (int)spec.levels.size(); ++levelNdx)
			{
				const int queryLevel	= spec.levels[levelNdx].level;
				const int refValue		= (spec.levels[levelNdx].compressed) ? (1) : (0);

				allOk &= verifyTextureLevelParameterEqual(gl, spec.queryTarget, queryLevel, GL_TEXTURE_COMPRESSED, refValue, m_type);
			}
		}

		return allOk;
	}
};

class TextureLevelBufferDataStoreCase : public TextureLevelCase
{
public:
	TextureLevelBufferDataStoreCase (Context& ctx, const char* name, const char* desc, glw::GLenum target, QueryType type)
		: TextureLevelCase(ctx, name, desc, target, type)
	{
	}

private:
	void init (void)
	{
		if (!m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_buffer") &&
			!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
			throw tcu::NotSupportedError("Test requires GL_EXT_texture_buffer extension");
		TextureLevelCase::init();
	}

	void generateTestIterations (std::vector<TextureGenerationSpec>& iterations)
	{
		generateTextureBufferGenerationGroup(iterations, m_target);
	}

	bool checkTextureState (glu::CallLogWrapper& gl, const TextureGenerationSpec& spec)
	{
		bool allOk = true;

		if (spec.levels.empty())
		{
			allOk &= verifyTextureLevelParameterEqual(gl, spec.queryTarget, 0, GL_TEXTURE_BUFFER_DATA_STORE_BINDING, 0, m_type);
		}
		else
		{
			allOk &= verifyTextureLevelParameterEqual(gl, spec.queryTarget, 0, GL_TEXTURE_BUFFER_DATA_STORE_BINDING, m_texBuffer, m_type);
		}

		return allOk;
	}
};

class TextureLevelBufferDataOffsetCase : public TextureLevelCase
{
public:
	TextureLevelBufferDataOffsetCase (Context& ctx, const char* name, const char* desc, glw::GLenum target, QueryType type)
		: TextureLevelCase(ctx, name, desc, target, type)
	{
	}

private:
	void init (void)
	{
		if (!m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_buffer") &&
			!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
			throw tcu::NotSupportedError("Test requires GL_EXT_texture_buffer extension");
		TextureLevelCase::init();
	}

	void generateTestIterations (std::vector<TextureGenerationSpec>& iterations)
	{
		generateTextureBufferGenerationGroup(iterations, m_target);
	}

	bool checkTextureState (glu::CallLogWrapper& gl, const TextureGenerationSpec& spec)
	{
		bool allOk = true;

		if (spec.levels.empty())
		{
			allOk &= verifyTextureLevelParameterEqual(gl, spec.queryTarget, 0, GL_TEXTURE_BUFFER_OFFSET, 0, m_type);
		}
		else
		{
			const int refValue = spec.texBufferDataOffset;

			allOk &= verifyTextureLevelParameterEqual(gl, spec.queryTarget, 0, GL_TEXTURE_BUFFER_OFFSET, refValue, m_type);
		}

		return allOk;
	}
};

class TextureLevelBufferDataSizeCase : public TextureLevelCase
{
public:
	TextureLevelBufferDataSizeCase (Context& ctx, const char* name, const char* desc, glw::GLenum target, QueryType type)
		: TextureLevelCase(ctx, name, desc, target, type)
	{
	}

private:
	void init (void)
	{
		if (!m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_buffer") &&
			!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
			throw tcu::NotSupportedError("Test requires GL_EXT_texture_buffer extension");
		TextureLevelCase::init();
	}

	void generateTestIterations (std::vector<TextureGenerationSpec>& iterations)
	{
		generateTextureBufferGenerationGroup(iterations, m_target);
	}

	bool checkTextureState (glu::CallLogWrapper& gl, const TextureGenerationSpec& spec)
	{
		bool allOk = true;

		if (spec.levels.empty())
		{
			allOk &= verifyTextureLevelParameterEqual(gl, spec.queryTarget, 0, GL_TEXTURE_BUFFER_SIZE, 0, m_type);
		}
		else
		{
			const int refValue = spec.texBufferDataSize;

			allOk &= verifyTextureLevelParameterEqual(gl, spec.queryTarget, 0, GL_TEXTURE_BUFFER_SIZE, refValue, m_type);
		}

		return allOk;
	}
};

} // anonymous

static const char* getVerifierSuffix (QueryType type)
{
	switch (type)
	{
		case QUERY_TEXTURE_LEVEL_FLOAT:		return "_float";
		case QUERY_TEXTURE_LEVEL_INTEGER:	return "_integer";
		default:
			DE_ASSERT(DE_FALSE);
			return DE_NULL;
	}
}

TextureLevelStateQueryTests::TextureLevelStateQueryTests (Context& context)
	: TestCaseGroup(context, "texture_level", "GetTexLevelParameter tests")
{
}

TextureLevelStateQueryTests::~TextureLevelStateQueryTests (void)
{
}

void TextureLevelStateQueryTests::init (void)
{
	static const QueryType verifiers[] =
	{
		QUERY_TEXTURE_LEVEL_INTEGER,
		QUERY_TEXTURE_LEVEL_FLOAT,
	};

#define FOR_EACH_VERIFIER(X) \
	for (int verifierNdx = 0; verifierNdx < DE_LENGTH_OF_ARRAY(verifiers); ++verifierNdx)	\
	{																						\
		const std::string verifierSuffix = getVerifierSuffix(verifiers[verifierNdx]);		\
		const QueryType verifier = verifiers[verifierNdx];									\
		targetGroup->addChild(X);															\
	}
	static const struct
	{
		const char*	name;
		glw::GLenum	target;
	} textureTargets[] =
	{
		{ "texture_2d",						GL_TEXTURE_2D,						},
		{ "texture_3d",						GL_TEXTURE_3D,						},
		{ "texture_2d_array",				GL_TEXTURE_2D_ARRAY,				},
		{ "texture_cube_map",				GL_TEXTURE_CUBE_MAP,				},
		{ "texture_2d_multisample",			GL_TEXTURE_2D_MULTISAMPLE,			},
		{ "texture_2d_multisample_array",	GL_TEXTURE_2D_MULTISAMPLE_ARRAY,	}, // GL_OES_texture_storage_multisample_2d_array
		{ "texture_buffer",					GL_TEXTURE_BUFFER,					}, // GL_EXT_texture_buffer
		{ "texture_cube_array",				GL_TEXTURE_CUBE_MAP_ARRAY,			}, // GL_EXT_texture_cube_map_array
	};

	for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(textureTargets); ++targetNdx)
	{
		tcu::TestCaseGroup* const targetGroup = new tcu::TestCaseGroup(m_testCtx, textureTargets[targetNdx].name, textureTargets[targetNdx].name);
		addChild(targetGroup);

		FOR_EACH_VERIFIER(new TextureLevelSampleCase			(m_context, ("samples" + verifierSuffix).c_str(),					"Verify TEXTURE_SAMPLES",					textureTargets[targetNdx].target,	verifier));
		FOR_EACH_VERIFIER(new TextureLevelFixedSamplesCase		(m_context, ("fixed_sample_locations" + verifierSuffix).c_str(),	"Verify TEXTURE_FIXED_SAMPLE_LOCATIONS",	textureTargets[targetNdx].target,	verifier));
		FOR_EACH_VERIFIER(new TextureLevelWidthCase				(m_context, ("width" + verifierSuffix).c_str(),						"Verify TEXTURE_WIDTH",						textureTargets[targetNdx].target,	verifier));
		FOR_EACH_VERIFIER(new TextureLevelHeightCase			(m_context, ("height" + verifierSuffix).c_str(),					"Verify TEXTURE_HEIGHT",					textureTargets[targetNdx].target,	verifier));
		FOR_EACH_VERIFIER(new TextureLevelDepthCase				(m_context, ("depth" + verifierSuffix).c_str(),						"Verify TEXTURE_DEPTH",						textureTargets[targetNdx].target,	verifier));
		FOR_EACH_VERIFIER(new TextureLevelInternalFormatCase	(m_context, ("internal_format" + verifierSuffix).c_str(),			"Verify TEXTURE_INTERNAL_FORMAT",			textureTargets[targetNdx].target,	verifier));
		FOR_EACH_VERIFIER(new TextureLevelSizeCase				(m_context, ("red_size" + verifierSuffix).c_str(),					"Verify TEXTURE_RED_SIZE",					textureTargets[targetNdx].target,	verifier,	GL_TEXTURE_RED_SIZE));
		FOR_EACH_VERIFIER(new TextureLevelSizeCase				(m_context, ("green_size" + verifierSuffix).c_str(),				"Verify TEXTURE_GREEN_SIZE",				textureTargets[targetNdx].target,	verifier,	GL_TEXTURE_GREEN_SIZE));
		FOR_EACH_VERIFIER(new TextureLevelSizeCase				(m_context, ("blue_size" + verifierSuffix).c_str(),					"Verify TEXTURE_BLUE_SIZE",					textureTargets[targetNdx].target,	verifier,	GL_TEXTURE_BLUE_SIZE));
		FOR_EACH_VERIFIER(new TextureLevelSizeCase				(m_context, ("alpha_size" + verifierSuffix).c_str(),				"Verify TEXTURE_ALPHA_SIZE",				textureTargets[targetNdx].target,	verifier,	GL_TEXTURE_ALPHA_SIZE));
		FOR_EACH_VERIFIER(new TextureLevelSizeCase				(m_context, ("depth_size" + verifierSuffix).c_str(),				"Verify TEXTURE_DEPTH_SIZE",				textureTargets[targetNdx].target,	verifier,	GL_TEXTURE_DEPTH_SIZE));
		FOR_EACH_VERIFIER(new TextureLevelSizeCase				(m_context, ("stencil_size" + verifierSuffix).c_str(),				"Verify TEXTURE_STENCIL_SIZE",				textureTargets[targetNdx].target,	verifier,	GL_TEXTURE_STENCIL_SIZE));
		FOR_EACH_VERIFIER(new TextureLevelSizeCase				(m_context, ("shared_size" + verifierSuffix).c_str(),				"Verify TEXTURE_SHARED_SIZE",				textureTargets[targetNdx].target,	verifier,	GL_TEXTURE_SHARED_SIZE));
		FOR_EACH_VERIFIER(new TextureLevelTypeCase				(m_context, ("red_type" + verifierSuffix).c_str(),					"Verify TEXTURE_RED_TYPE",					textureTargets[targetNdx].target,	verifier,	GL_TEXTURE_RED_TYPE));
		FOR_EACH_VERIFIER(new TextureLevelTypeCase				(m_context, ("green_type" + verifierSuffix).c_str(),				"Verify TEXTURE_GREEN_TYPE",				textureTargets[targetNdx].target,	verifier,	GL_TEXTURE_GREEN_TYPE));
		FOR_EACH_VERIFIER(new TextureLevelTypeCase				(m_context, ("blue_type" + verifierSuffix).c_str(),					"Verify TEXTURE_BLUE_TYPE",					textureTargets[targetNdx].target,	verifier,	GL_TEXTURE_BLUE_TYPE));
		FOR_EACH_VERIFIER(new TextureLevelTypeCase				(m_context, ("alpha_type" + verifierSuffix).c_str(),				"Verify TEXTURE_ALPHA_TYPE",				textureTargets[targetNdx].target,	verifier,	GL_TEXTURE_ALPHA_TYPE));
		FOR_EACH_VERIFIER(new TextureLevelTypeCase				(m_context, ("depth_type" + verifierSuffix).c_str(),				"Verify TEXTURE_DEPTH_TYPE",				textureTargets[targetNdx].target,	verifier,	GL_TEXTURE_DEPTH_TYPE));
		FOR_EACH_VERIFIER(new TextureLevelCompressedCase		(m_context, ("compressed" + verifierSuffix).c_str(),				"Verify TEXTURE_COMPRESSED",				textureTargets[targetNdx].target,	verifier));
		FOR_EACH_VERIFIER(new TextureLevelBufferDataStoreCase	(m_context, ("buffer_data_store_binding" + verifierSuffix).c_str(),	"Verify TEXTURE_BUFFER_DATA_STORE_BINDING",	textureTargets[targetNdx].target,	verifier));
		FOR_EACH_VERIFIER(new TextureLevelBufferDataOffsetCase	(m_context, ("buffer_offset" + verifierSuffix).c_str(),				"Verify TEXTURE_BUFFER_OFFSET",				textureTargets[targetNdx].target,	verifier));
		FOR_EACH_VERIFIER(new TextureLevelBufferDataSizeCase	(m_context, ("buffer_size" + verifierSuffix).c_str(),				"Verify TEXTURE_BUFFER_SIZE",				textureTargets[targetNdx].target,	verifier));
	}

#undef FOR_EACH_VERIFIER
}

} // Functional
} // gles31
} // deqp
