/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 * Copyright (c) 2016 The Android Open Source Project
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
 * \brief Texture access and query function tests.
 *//*--------------------------------------------------------------------*/

#include "vktShaderRenderTextureFunctionTests.hpp"
#include "vktShaderRender.hpp"
#include "gluTextureUtil.hpp"
#include "tcuTexture.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuTestLog.hpp"
#include "glwEnums.hpp"
#include "deMath.h"
#include "vkImageUtil.hpp"
#include "vkQueryUtil.hpp"

namespace vkt
{
namespace sr
{
namespace
{

using tcu::Vec2;
using tcu::Vec3;
using tcu::Vec4;
using tcu::IVec2;
using tcu::IVec3;
using tcu::IVec4;

using std::vector;

enum Function
{
	FUNCTION_TEXTURE = 0,		//!< texture(), textureOffset()
	FUNCTION_TEXTUREPROJ,		//!< textureProj(), textureProjOffset()
	FUNCTION_TEXTUREPROJ2,		//!< textureProj(sampler1D, vec2)
	FUNCTION_TEXTUREPROJ3,		//!< textureProj(sampler2D, vec3)
	FUNCTION_TEXTURELOD,		// ...
	FUNCTION_TEXTUREPROJLOD,
	FUNCTION_TEXTUREPROJLOD2,	//!< textureProjLod(sampler1D, vec2)
	FUNCTION_TEXTUREPROJLOD3,	//!< textureProjLod(sampler2D, vec3)
	FUNCTION_TEXTUREGRAD,
	FUNCTION_TEXTUREPROJGRAD,
	FUNCTION_TEXTUREPROJGRAD2,	//!< textureProjGrad(sampler1D, vec2)
	FUNCTION_TEXTUREPROJGRAD3,	//!< textureProjGrad(sampler2D, vec3)
	FUNCTION_TEXELFETCH,

	FUNCTION_LAST
};

inline bool functionHasAutoLod (glu::ShaderType shaderType, Function function)
{
	return shaderType == glu::SHADERTYPE_FRAGMENT &&
		   (function == FUNCTION_TEXTURE		||
			function == FUNCTION_TEXTUREPROJ	||
			function == FUNCTION_TEXTUREPROJ2	||
			function == FUNCTION_TEXTUREPROJ3);
}

inline bool functionHasProj (Function function)
{
	return function == FUNCTION_TEXTUREPROJ		||
		   function == FUNCTION_TEXTUREPROJ2	||
		   function == FUNCTION_TEXTUREPROJ3	||
		   function == FUNCTION_TEXTUREPROJLOD	||
		   function == FUNCTION_TEXTUREPROJLOD2	||
		   function == FUNCTION_TEXTUREPROJLOD3	||
		   function == FUNCTION_TEXTUREPROJGRAD	||
		   function == FUNCTION_TEXTUREPROJGRAD2||
		   function == FUNCTION_TEXTUREPROJGRAD3;
}

inline bool functionHasGrad (Function function)
{
	return function == FUNCTION_TEXTUREGRAD		||
		   function == FUNCTION_TEXTUREPROJGRAD	||
		   function == FUNCTION_TEXTUREPROJGRAD2||
		   function == FUNCTION_TEXTUREPROJGRAD3;
}

inline bool functionHasLod (Function function)
{
	return function == FUNCTION_TEXTURELOD		||
		   function == FUNCTION_TEXTUREPROJLOD	||
		   function == FUNCTION_TEXTUREPROJLOD2	||
		   function == FUNCTION_TEXTUREPROJLOD3	||
		   function == FUNCTION_TEXELFETCH;
}

struct TextureLookupSpec
{
	Function		function;

	tcu::Vec4		minCoord;
	tcu::Vec4		maxCoord;

	// Bias
	bool			useBias;

	// Bias or Lod for *Lod* functions
	float			minLodBias;
	float			maxLodBias;

	// For *Grad* functions
	tcu::Vec3		minDX;
	tcu::Vec3		maxDX;
	tcu::Vec3		minDY;
	tcu::Vec3		maxDY;

	bool			useOffset;
	tcu::IVec3		offset;

	TextureLookupSpec (void)
		: function		(FUNCTION_LAST)
		, minCoord		(0.0f)
		, maxCoord		(1.0f)
		, useBias		(false)
		, minLodBias	(0.0f)
		, maxLodBias	(0.0f)
		, minDX			(0.0f)
		, maxDX			(0.0f)
		, minDY			(0.0f)
		, maxDY			(0.0f)
		, useOffset		(false)
		, offset		(0)
	{
	}

	TextureLookupSpec (Function				function_,
					   const tcu::Vec4&		minCoord_,
					   const tcu::Vec4&		maxCoord_,
					   bool					useBias_,
					   float				minLodBias_,
					   float				maxLodBias_,
					   const tcu::Vec3&		minDX_,
					   const tcu::Vec3&		maxDX_,
					   const tcu::Vec3&		minDY_,
					   const tcu::Vec3&		maxDY_,
					   bool					useOffset_,
					   const tcu::IVec3&	offset_)
		: function		(function_)
		, minCoord		(minCoord_)
		, maxCoord		(maxCoord_)
		, useBias		(useBias_)
		, minLodBias	(minLodBias_)
		, maxLodBias	(maxLodBias_)
		, minDX			(minDX_)
		, maxDX			(maxDX_)
		, minDY			(minDY_)
		, maxDY			(maxDY_)
		, useOffset		(useOffset_)
		, offset		(offset_)
	{
	}
};

enum TextureType
{
	TEXTURETYPE_1D = 0,
	TEXTURETYPE_2D,
	TEXTURETYPE_3D,
	TEXTURETYPE_CUBE_MAP,
	TEXTURETYPE_1D_ARRAY,
	TEXTURETYPE_2D_ARRAY,
	TEXTURETYPE_CUBE_ARRAY,

	TEXTURETYPE_LAST
};

struct TextureSpec
{
	TextureType			type;		//!< Texture type (2D, cubemap, ...)
	deUint32			format;		//!< Internal format.
	int					width;
	int					height;
	int					depth;
	int					numLevels;
	tcu::Sampler		sampler;

	TextureSpec (void)
		: type			(TEXTURETYPE_LAST)
		, format		(GL_NONE)
		, width			(0)
		, height		(0)
		, depth			(0)
		, numLevels		(0)
	{
	}

	TextureSpec (TextureType			type_,
				 deUint32				format_,
				 int					width_,
				 int					height_,
				 int					depth_,
				 int					numLevels_,
				 const tcu::Sampler&	sampler_)
		: type			(type_)
		, format		(format_)
		, width			(width_)
		, height		(height_)
		, depth			(depth_)
		, numLevels		(numLevels_)
		, sampler		(sampler_)
	{
	}
};

struct TexLookupParams
{
	float				lod;
	tcu::IVec3			offset;
	tcu::Vec4			scale;
	tcu::Vec4			bias;

	TexLookupParams (void)
		: lod		(0.0f)
		, offset	(0)
		, scale		(1.0f)
		, bias		(0.0f)
	{
	}
};

// \note LodMode and computeLodFromDerivates functions are copied from glsTextureTestUtil
namespace TextureTestUtil
{

enum LodMode
{
	LODMODE_EXACT = 0,		//!< Ideal lod computation.
	LODMODE_MIN_BOUND,		//!< Use estimation range minimum bound.
	LODMODE_MAX_BOUND,		//!< Use estimation range maximum bound.

	LODMODE_LAST
};

// 1D lookup LOD computation.

float computeLodFromDerivates (LodMode mode, float dudx, float dudy)
{
	float p = 0.0f;
	switch (mode)
	{
		// \note [mika] Min and max bounds equal to exact with 1D textures
		case LODMODE_EXACT:
		case LODMODE_MIN_BOUND:
		case LODMODE_MAX_BOUND:
			p = de::max(deFloatAbs(dudx), deFloatAbs(dudy));
			break;

		default:
			DE_ASSERT(DE_FALSE);
	}

	return deFloatLog2(p);
}

// 2D lookup LOD computation.

float computeLodFromDerivates (LodMode mode, float dudx, float dvdx, float dudy, float dvdy)
{
	float p = 0.0f;
	switch (mode)
	{
		case LODMODE_EXACT:
			p = de::max(deFloatSqrt(dudx*dudx + dvdx*dvdx), deFloatSqrt(dudy*dudy + dvdy*dvdy));
			break;

		case LODMODE_MIN_BOUND:
		case LODMODE_MAX_BOUND:
		{
			float mu = de::max(deFloatAbs(dudx), deFloatAbs(dudy));
			float mv = de::max(deFloatAbs(dvdx), deFloatAbs(dvdy));

			p = mode == LODMODE_MIN_BOUND ? de::max(mu, mv) : mu + mv;
			break;
		}

		default:
			DE_ASSERT(DE_FALSE);
	}

	return deFloatLog2(p);
}

// 3D lookup LOD computation.

float computeLodFromDerivates (LodMode mode, float dudx, float dvdx, float dwdx, float dudy, float dvdy, float dwdy)
{
	float p = 0.0f;
	switch (mode)
	{
		case LODMODE_EXACT:
			p = de::max(deFloatSqrt(dudx*dudx + dvdx*dvdx + dwdx*dwdx), deFloatSqrt(dudy*dudy + dvdy*dvdy + dwdy*dwdy));
			break;

		case LODMODE_MIN_BOUND:
		case LODMODE_MAX_BOUND:
		{
			float mu = de::max(deFloatAbs(dudx), deFloatAbs(dudy));
			float mv = de::max(deFloatAbs(dvdx), deFloatAbs(dvdy));
			float mw = de::max(deFloatAbs(dwdx), deFloatAbs(dwdy));

			p = mode == LODMODE_MIN_BOUND ? de::max(de::max(mu, mv), mw) : (mu + mv + mw);
			break;
		}

		default:
			DE_ASSERT(DE_FALSE);
	}

	return deFloatLog2(p);
}

} // TextureTestUtil

using namespace TextureTestUtil;

static const LodMode DEFAULT_LOD_MODE = LODMODE_EXACT;

inline float computeLodFromGrad2D (const ShaderEvalContext& c)
{
	float w = (float)c.textures[0].tex2D->getWidth();
	float h = (float)c.textures[0].tex2D->getHeight();
	return computeLodFromDerivates(DEFAULT_LOD_MODE, c.in[1].x()*w, c.in[1].y()*h, c.in[2].x()*w, c.in[2].y()*h);
}

inline float computeLodFromGrad2DArray (const ShaderEvalContext& c)
{
	float w = (float)c.textures[0].tex2DArray->getWidth();
	float h = (float)c.textures[0].tex2DArray->getHeight();
	return computeLodFromDerivates(DEFAULT_LOD_MODE, c.in[1].x()*w, c.in[1].y()*h, c.in[2].x()*w, c.in[2].y()*h);
}

inline float computeLodFromGrad3D (const ShaderEvalContext& c)
{
	float w = (float)c.textures[0].tex3D->getWidth();
	float h = (float)c.textures[0].tex3D->getHeight();
	float d = (float)c.textures[0].tex3D->getDepth();
	return computeLodFromDerivates(DEFAULT_LOD_MODE, c.in[1].x()*w, c.in[1].y()*h, c.in[1].z()*d, c.in[2].x()*w, c.in[2].y()*h, c.in[2].z()*d);
}

inline float computeLodFromGradCube (const ShaderEvalContext& c)
{
	// \note Major axis is always -Z or +Z
	float m = de::abs(c.in[0].z());
	float d = (float)c.textures[0].texCube->getSize();
	float s = d/(2.0f*m);
	float t = d/(2.0f*m);
	return computeLodFromDerivates(DEFAULT_LOD_MODE, c.in[1].x()*s, c.in[1].y()*t, c.in[2].x()*s, c.in[2].y()*t);
}

inline float computeLodFromGrad1D (const ShaderEvalContext& c)
{
	float w = (float)c.textures[0].tex1D->getWidth();
	return computeLodFromDerivates(DEFAULT_LOD_MODE, c.in[1].x()*w, c.in[2].x()*w);
}

inline float computeLodFromGrad1DArray (const ShaderEvalContext& c)
{
	float w = (float)c.textures[0].tex1DArray->getWidth();
	return computeLodFromDerivates(DEFAULT_LOD_MODE, c.in[1].x()*w, c.in[2].x()*w);
}

inline float computeLodFromGradCubeArray (const ShaderEvalContext& c)
{
	// \note Major axis is always -Z or +Z
	float m = de::abs(c.in[0].z());
	float d = (float)c.textures[0].texCubeArray->getSize();
	float s = d/(2.0f*m);
	float t = d/(2.0f*m);
	return computeLodFromDerivates(DEFAULT_LOD_MODE, c.in[1].x()*s, c.in[1].y()*t, c.in[2].x()*s, c.in[2].y()*t);
}

typedef void (*TexEvalFunc) (ShaderEvalContext& c, const TexLookupParams& lookupParams);

inline Vec4 texture2D			(const ShaderEvalContext& c, float s, float t, float lod)					{ return c.textures[0].tex2D->sample(c.textures[0].sampler, s, t, lod);					}
inline Vec4 textureCube			(const ShaderEvalContext& c, float s, float t, float r, float lod)			{ return c.textures[0].texCube->sample(c.textures[0].sampler, s, t, r, lod);			}
inline Vec4 texture2DArray		(const ShaderEvalContext& c, float s, float t, float r, float lod)			{ return c.textures[0].tex2DArray->sample(c.textures[0].sampler, s, t, r, lod);			}
inline Vec4 texture3D			(const ShaderEvalContext& c, float s, float t, float r, float lod)			{ return c.textures[0].tex3D->sample(c.textures[0].sampler, s, t, r, lod);				}
inline Vec4 texture1D			(const ShaderEvalContext& c, float s, float lod)							{ return c.textures[0].tex1D->sample(c.textures[0].sampler, s, lod);					}
inline Vec4 texture1DArray		(const ShaderEvalContext& c, float s, float t, float lod)					{ return c.textures[0].tex1DArray->sample(c.textures[0].sampler, s, t, lod);			}
inline Vec4 textureCubeArray	(const ShaderEvalContext& c, float s, float t, float r, float q, float lod)	{ return c.textures[0].texCubeArray->sample(c.textures[0].sampler, s, t, r, q, lod);	}

inline float texture2DShadow		(const ShaderEvalContext& c, float ref, float s, float t, float lod)					{ return c.textures[0].tex2D->sampleCompare(c.textures[0].sampler, ref, s, t, lod);					}
inline float textureCubeShadow		(const ShaderEvalContext& c, float ref, float s, float t, float r, float lod)			{ return c.textures[0].texCube->sampleCompare(c.textures[0].sampler, ref, s, t, r, lod);			}
inline float texture2DArrayShadow	(const ShaderEvalContext& c, float ref, float s, float t, float r, float lod)			{ return c.textures[0].tex2DArray->sampleCompare(c.textures[0].sampler, ref, s, t, r, lod);			}
inline float texture1DShadow		(const ShaderEvalContext& c, float ref, float s, float lod)								{ return c.textures[0].tex1D->sampleCompare(c.textures[0].sampler, ref, s, lod);					}
inline float texture1DArrayShadow	(const ShaderEvalContext& c, float ref, float s, float t, float lod)					{ return c.textures[0].tex1DArray->sampleCompare(c.textures[0].sampler, ref, s, t, lod);			}
inline float textureCubeArrayShadow	(const ShaderEvalContext& c, float ref, float s, float t, float r, float q, float lod)	{ return c.textures[0].texCubeArray->sampleCompare(c.textures[0].sampler, ref, s, t, r, q, lod);	}

inline Vec4 texture2DOffset			(const ShaderEvalContext& c, float s, float t, float lod, IVec2 offset)				{ return c.textures[0].tex2D->sampleOffset(c.textures[0].sampler, s, t, lod, offset);			}
inline Vec4 texture2DArrayOffset	(const ShaderEvalContext& c, float s, float t, float r, float lod, IVec2 offset)	{ return c.textures[0].tex2DArray->sampleOffset(c.textures[0].sampler, s, t, r, lod, offset);	}
inline Vec4 texture3DOffset			(const ShaderEvalContext& c, float s, float t, float r, float lod, IVec3 offset)	{ return c.textures[0].tex3D->sampleOffset(c.textures[0].sampler, s, t, r, lod, offset);		}
inline Vec4 texture1DOffset			(const ShaderEvalContext& c, float s, float lod, deInt32 offset)					{ return c.textures[0].tex1D->sampleOffset(c.textures[0].sampler, s, lod, offset);				}
inline Vec4 texture1DArrayOffset	(const ShaderEvalContext& c, float s, float t, float lod, deInt32 offset)			{ return c.textures[0].tex1DArray->sampleOffset(c.textures[0].sampler, s, t, lod, offset);		}

inline float texture2DShadowOffset		(const ShaderEvalContext& c, float ref, float s, float t, float lod, IVec2 offset)			{ return c.textures[0].tex2D->sampleCompareOffset(c.textures[0].sampler, ref, s, t, lod, offset);			}
inline float texture2DArrayShadowOffset	(const ShaderEvalContext& c, float ref, float s, float t, float r, float lod, IVec2 offset)	{ return c.textures[0].tex2DArray->sampleCompareOffset(c.textures[0].sampler, ref, s, t, r, lod, offset);	}
inline float texture1DShadowOffset		(const ShaderEvalContext& c, float ref, float s, float lod, deInt32 offset)					{ return c.textures[0].tex1D->sampleCompareOffset(c.textures[0].sampler, ref, s, lod, offset);				}
inline float texture1DArrayShadowOffset	(const ShaderEvalContext& c, float ref, float s, float t, float lod, deInt32 offset)		{ return c.textures[0].tex1DArray->sampleCompareOffset(c.textures[0].sampler, ref, s, t, lod, offset);		}

// Eval functions.
static void		evalTexture2D			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2D(c, c.in[0].x(), c.in[0].y(), p.lod)*p.scale + p.bias; }
static void		evalTextureCube			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = textureCube(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), p.lod)*p.scale + p.bias; }
static void		evalTexture2DArray		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DArray(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), p.lod)*p.scale + p.bias; }
static void		evalTexture3D			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture3D(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), p.lod)*p.scale + p.bias; }
static void		evalTexture1D			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1D(c, c.in[0].x(), p.lod)*p.scale + p.bias; }
static void		evalTexture1DArray		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DArray(c, c.in[0].x(), c.in[0].y(), p.lod)*p.scale + p.bias; }
static void		evalTextureCubeArray	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = textureCubeArray(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), c.in[0].w(), p.lod)*p.scale + p.bias; }

static void		evalTexture2DBias		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2D(c, c.in[0].x(), c.in[0].y(), p.lod+c.in[1].x())*p.scale + p.bias; }
static void		evalTextureCubeBias		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = textureCube(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), p.lod+c.in[1].x())*p.scale + p.bias; }
static void		evalTexture2DArrayBias	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DArray(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), p.lod+c.in[1].x())*p.scale + p.bias; }
static void		evalTexture3DBias		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture3D(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), p.lod+c.in[1].x())*p.scale + p.bias; }
static void		evalTexture1DBias		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1D(c, c.in[0].x(), p.lod+c.in[1].x())*p.scale + p.bias; }
static void		evalTexture1DArrayBias	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DArray(c, c.in[0].x(), c.in[0].y(), p.lod+c.in[1].x())*p.scale + p.bias; }
static void		evalTextureCubeArrayBias(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = textureCubeArray(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), c.in[0].w(), p.lod+c.in[1].x())*p.scale + p.bias; }

static void		evalTexture2DProj3		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2D(c, c.in[0].x()/c.in[0].z(), c.in[0].y()/c.in[0].z(), p.lod)*p.scale + p.bias; }
static void		evalTexture2DProj3Bias	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2D(c, c.in[0].x()/c.in[0].z(), c.in[0].y()/c.in[0].z(), p.lod+c.in[1].x())*p.scale + p.bias; }
static void		evalTexture2DProj		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2D(c, c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), p.lod)*p.scale + p.bias; }
static void		evalTexture2DProjBias	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2D(c, c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), p.lod+c.in[1].x())*p.scale + p.bias; }
static void		evalTexture3DProj		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture3D(c, c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), c.in[0].z()/c.in[0].w(), p.lod)*p.scale + p.bias; }
static void		evalTexture3DProjBias	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture3D(c, c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), c.in[0].z()/c.in[0].w(), p.lod+c.in[1].x())*p.scale + p.bias; }
static void		evalTexture1DProj2		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1D(c, c.in[0].x()/c.in[0].y(), p.lod)*p.scale + p.bias; }
static void		evalTexture1DProj2Bias	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1D(c, c.in[0].x()/c.in[0].y(), p.lod+c.in[1].x())*p.scale + p.bias; }
static void		evalTexture1DProj		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1D(c, c.in[0].x()/c.in[0].w(), p.lod)*p.scale + p.bias; }
static void		evalTexture1DProjBias	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1D(c, c.in[0].x()/c.in[0].w(), p.lod+c.in[1].x())*p.scale + p.bias; }

static void		evalTexture2DLod		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2D(c, c.in[0].x(), c.in[0].y(), c.in[1].x())*p.scale + p.bias; }
static void		evalTextureCubeLod		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = textureCube(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), c.in[1].x())*p.scale + p.bias; }
static void		evalTexture2DArrayLod	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DArray(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), c.in[1].x())*p.scale + p.bias; }
static void		evalTexture3DLod		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture3D(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), c.in[1].x())*p.scale + p.bias; }
static void		evalTexture1DLod		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1D(c, c.in[0].x(), c.in[1].x())*p.scale + p.bias; }
static void		evalTexture1DArrayLod	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DArray(c, c.in[0].x(), c.in[0].y(), c.in[1].x())*p.scale + p.bias; }
static void		evalTextureCubeArrayLod	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = textureCubeArray(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), c.in[0].w(), c.in[1].x())*p.scale + p.bias; }

static void		evalTexture2DProjLod3	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2D(c, c.in[0].x()/c.in[0].z(), c.in[0].y()/c.in[0].z(), c.in[1].x())*p.scale + p.bias; }
static void		evalTexture2DProjLod	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2D(c, c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), c.in[1].x())*p.scale + p.bias; }
static void		evalTexture3DProjLod	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture3D(c, c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), c.in[0].z()/c.in[0].w(), c.in[1].x())*p.scale + p.bias; }
static void		evalTexture1DProjLod2	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1D(c, c.in[0].x()/c.in[0].y(), c.in[1].x())*p.scale + p.bias; }
static void		evalTexture1DProjLod	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1D(c, c.in[0].x()/c.in[0].w(), c.in[1].x())*p.scale + p.bias; }

// Offset variants

static void		evalTexture2DOffset				(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DOffset(c, c.in[0].x(), c.in[0].y(), p.lod, p.offset.swizzle(0,1))*p.scale + p.bias; }
static void		evalTexture2DArrayOffset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DArrayOffset(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), p.lod, p.offset.swizzle(0,1))*p.scale + p.bias; }
static void		evalTexture3DOffset				(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture3DOffset(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), p.lod, p.offset)*p.scale + p.bias; }
static void		evalTexture1DOffset				(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DOffset(c, c.in[0].x(), p.lod, p.offset.x())*p.scale + p.bias; }
static void		evalTexture1DArrayOffset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DArrayOffset(c, c.in[0].x(), c.in[0].y(), p.lod, p.offset.x())*p.scale + p.bias; }

static void		evalTexture2DOffsetBias			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DOffset(c, c.in[0].x(), c.in[0].y(), p.lod+c.in[1].x(), p.offset.swizzle(0,1))*p.scale + p.bias; }
static void		evalTexture2DArrayOffsetBias	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DArrayOffset(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), p.lod+c.in[1].x(), p.offset.swizzle(0,1))*p.scale + p.bias; }
static void		evalTexture3DOffsetBias			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture3DOffset(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), p.lod+c.in[1].x(), p.offset)*p.scale + p.bias; }
static void		evalTexture1DOffsetBias			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DOffset(c, c.in[0].x(), p.lod+c.in[1].x(), p.offset.x())*p.scale + p.bias; }
static void		evalTexture1DArrayOffsetBias	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DArrayOffset(c, c.in[0].x(), c.in[0].y(), p.lod+c.in[1].x(), p.offset.x())*p.scale + p.bias; }

static void		evalTexture2DLodOffset			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DOffset(c, c.in[0].x(), c.in[0].y(), c.in[1].x(), p.offset.swizzle(0,1))*p.scale + p.bias; }
static void		evalTexture2DArrayLodOffset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DArrayOffset(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), c.in[1].x(), p.offset.swizzle(0,1))*p.scale + p.bias; }
static void		evalTexture3DLodOffset			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture3DOffset(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), c.in[1].x(), p.offset)*p.scale + p.bias; }
static void		evalTexture1DLodOffset			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DOffset(c, c.in[0].x(), c.in[1].x(), p.offset.x())*p.scale + p.bias; }
static void		evalTexture1DArrayLodOffset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DArrayOffset(c, c.in[0].x(), c.in[0].y(), c.in[1].x(), p.offset.x())*p.scale + p.bias; }

static void		evalTexture2DProj3Offset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DOffset(c, c.in[0].x()/c.in[0].z(), c.in[0].y()/c.in[0].z(), p.lod, p.offset.swizzle(0,1))*p.scale + p.bias; }
static void		evalTexture2DProj3OffsetBias	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DOffset(c, c.in[0].x()/c.in[0].z(), c.in[0].y()/c.in[0].z(), p.lod+c.in[1].x(), p.offset.swizzle(0,1))*p.scale + p.bias; }
static void		evalTexture2DProjOffset			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DOffset(c, c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), p.lod, p.offset.swizzle(0,1))*p.scale + p.bias; }
static void		evalTexture2DProjOffsetBias		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DOffset(c, c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), p.lod+c.in[1].x(), p.offset.swizzle(0,1))*p.scale + p.bias; }
static void		evalTexture3DProjOffset			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture3DOffset(c, c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), c.in[0].z()/c.in[0].w(), p.lod, p.offset)*p.scale + p.bias; }
static void		evalTexture3DProjOffsetBias		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture3DOffset(c, c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), c.in[0].z()/c.in[0].w(), p.lod+c.in[1].x(), p.offset)*p.scale + p.bias; }
static void		evalTexture1DProj2Offset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DOffset(c, c.in[0].x()/c.in[0].y(), p.lod, p.offset.x())*p.scale + p.bias; }
static void		evalTexture1DProj2OffsetBias	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DOffset(c, c.in[0].x()/c.in[0].y(), p.lod+c.in[1].x(), p.offset.x())*p.scale + p.bias; }
static void		evalTexture1DProjOffset			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DOffset(c, c.in[0].x()/c.in[0].w(), p.lod, p.offset.x())*p.scale + p.bias; }
static void		evalTexture1DProjOffsetBias		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DOffset(c, c.in[0].x()/c.in[0].w(), p.lod+c.in[1].x(), p.offset.x())*p.scale + p.bias; }

static void		evalTexture2DProjLod3Offset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DOffset(c, c.in[0].x()/c.in[0].z(), c.in[0].y()/c.in[0].z(), c.in[1].x(), p.offset.swizzle(0,1))*p.scale + p.bias; }
static void		evalTexture2DProjLodOffset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DOffset(c, c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), c.in[1].x(), p.offset.swizzle(0,1))*p.scale + p.bias; }
static void		evalTexture3DProjLodOffset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture3DOffset(c, c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), c.in[0].z()/c.in[0].w(), c.in[1].x(), p.offset)*p.scale + p.bias; }
static void		evalTexture1DProjLod2Offset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DOffset(c, c.in[0].x()/c.in[0].y(), c.in[1].x(), p.offset.x())*p.scale + p.bias; }
static void		evalTexture1DProjLodOffset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DOffset(c, c.in[0].x()/c.in[0].w(), c.in[1].x(), p.offset.x())*p.scale + p.bias; }

// Shadow variants

static void		evalTexture2DShadow				(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture2DShadow(c, c.in[0].z(), c.in[0].x(), c.in[0].y(), p.lod); }
static void		evalTexture2DShadowBias			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture2DShadow(c, c.in[0].z(), c.in[0].x(), c.in[0].y(), p.lod+c.in[1].x()); }

static void		evalTextureCubeShadow			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = textureCubeShadow(c, c.in[0].w(), c.in[0].x(), c.in[0].y(), c.in[0].z(), p.lod); }
static void		evalTextureCubeShadowBias		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = textureCubeShadow(c, c.in[0].w(), c.in[0].x(), c.in[0].y(), c.in[0].z(), p.lod+c.in[1].x()); }

static void		evalTexture2DArrayShadow		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture2DArrayShadow(c, c.in[0].w(), c.in[0].x(), c.in[0].y(), c.in[0].z(), p.lod); }
static void		evalTexture1DShadow				(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture1DShadow(c, c.in[0].z(), c.in[0].x(), p.lod); }
static void		evalTexture1DShadowBias			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture1DShadow(c, c.in[0].z(), c.in[0].x(), p.lod+c.in[1].x()); }
static void		evalTexture1DArrayShadow		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture1DArrayShadow(c, c.in[0].z(), c.in[0].x(), c.in[0].y(), p.lod); }
static void		evalTexture1DArrayShadowBias	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture1DArrayShadow(c, c.in[0].z(), c.in[0].x(), c.in[0].y(), p.lod+c.in[1].x()); }
static void		evalTextureCubeArrayShadow		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = textureCubeArrayShadow(c, c.in[0].w(), c.in[0].x(), c.in[0].y(), c.in[0].z(), c.in[0].w(), p.lod); }

static void		evalTexture2DShadowLod				(ShaderEvalContext& c, const TexLookupParams&)		{ c.color.x() = texture2DShadow(c, c.in[0].z(), c.in[0].x(), c.in[0].y(), c.in[1].x()); }
static void		evalTexture2DShadowLodOffset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture2DShadowOffset(c, c.in[0].z(), c.in[0].x(), c.in[0].y(), c.in[1].x(), p.offset.swizzle(0,1)); }
static void		evalTexture1DShadowLod				(ShaderEvalContext& c, const TexLookupParams&)		{ c.color.x() = texture1DShadow(c, c.in[0].z(), c.in[0].x(), c.in[1].x()); }
static void		evalTexture1DShadowLodOffset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture1DShadowOffset(c, c.in[0].z(), c.in[0].x(), c.in[1].x(), p.offset.x()); }
static void		evalTexture1DArrayShadowLod			(ShaderEvalContext& c, const TexLookupParams&)		{ c.color.x() = texture1DArrayShadow(c, c.in[0].z(), c.in[0].x(), c.in[0].y(), c.in[1].x()); }
static void		evalTexture1DArrayShadowLodOffset	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture1DArrayShadowOffset(c, c.in[0].z(), c.in[0].x(), c.in[0].y(), c.in[1].x(), p.offset.x()); }

static void		evalTexture2DShadowProj				(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture2DShadow(c, c.in[0].z()/c.in[0].w(), c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), p.lod); }
static void		evalTexture2DShadowProjBias			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture2DShadow(c, c.in[0].z()/c.in[0].w(), c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), p.lod+c.in[1].x()); }
static void		evalTexture1DShadowProj				(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture1DShadow(c, c.in[0].z()/c.in[0].w(), c.in[0].x()/c.in[0].w(), p.lod); }
static void		evalTexture1DShadowProjBias			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture1DShadow(c, c.in[0].z()/c.in[0].w(), c.in[0].x()/c.in[0].w(), p.lod+c.in[1].x()); }

static void		evalTexture2DShadowProjLod			(ShaderEvalContext& c, const TexLookupParams&)		{ c.color.x() = texture2DShadow(c, c.in[0].z()/c.in[0].w(), c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), c.in[1].x()); }
static void		evalTexture2DShadowProjLodOffset	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture2DShadowOffset(c, c.in[0].z()/c.in[0].w(), c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), c.in[1].x(), p.offset.swizzle(0,1)); }
static void		evalTexture1DShadowProjLod			(ShaderEvalContext& c, const TexLookupParams&)		{ c.color.x() = texture1DShadow(c, c.in[0].z()/c.in[0].w(), c.in[0].x()/c.in[0].w(), c.in[1].x()); }
static void		evalTexture1DShadowProjLodOffset	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture1DShadowOffset(c, c.in[0].z()/c.in[0].w(), c.in[0].x()/c.in[0].w(), c.in[1].x(), p.offset.x()); }

static void		evalTexture2DShadowOffset			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture2DShadowOffset(c, c.in[0].z(), c.in[0].x(), c.in[0].y(), p.lod, p.offset.swizzle(0,1)); }
static void		evalTexture2DShadowOffsetBias		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture2DShadowOffset(c, c.in[0].z(), c.in[0].x(), c.in[0].y(), p.lod+c.in[1].x(), p.offset.swizzle(0,1)); }
static void		evalTexture2DArrayShadowOffset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture2DArrayShadowOffset(c, c.in[0].w(), c.in[0].x(), c.in[0].y(), c.in[0].z(), p.lod, p.offset.swizzle(0,1)); }
static void		evalTexture1DShadowOffset			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture1DShadowOffset(c, c.in[0].z(), c.in[0].x(), p.lod, p.offset.x()); }
static void		evalTexture1DShadowOffsetBias		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture1DShadowOffset(c, c.in[0].z(), c.in[0].x(), p.lod+c.in[1].x(), p.offset.x()); }
static void		evalTexture1DArrayShadowOffset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture1DArrayShadowOffset(c, c.in[0].z(), c.in[0].x(), c.in[0].y(), p.lod, p.offset.x()); }
static void		evalTexture1DArrayShadowOffsetBias	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture1DArrayShadowOffset(c, c.in[0].z(), c.in[0].x(), c.in[0].y(), p.lod+c.in[1].x(), p.offset.x()); }

static void		evalTexture2DShadowProjOffset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture2DShadowOffset(c, c.in[0].z()/c.in[0].w(), c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), p.lod, p.offset.swizzle(0,1)); }
static void		evalTexture2DShadowProjOffsetBias	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture2DShadowOffset(c, c.in[0].z()/c.in[0].w(), c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), p.lod+c.in[1].x(), p.offset.swizzle(0,1)); }
static void		evalTexture1DShadowProjOffset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture1DShadowOffset(c, c.in[0].z()/c.in[0].w(), c.in[0].x()/c.in[0].w(), p.lod, p.offset.x()); }
static void		evalTexture1DShadowProjOffsetBias	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture1DShadowOffset(c, c.in[0].z()/c.in[0].w(), c.in[0].x()/c.in[0].w(), p.lod+c.in[1].x(), p.offset.x()); }

// Gradient variarts

static void		evalTexture2DGrad			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2D(c, c.in[0].x(), c.in[0].y(), computeLodFromGrad2D(c))*p.scale + p.bias; }
static void		evalTextureCubeGrad			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = textureCube(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), computeLodFromGradCube(c))*p.scale + p.bias; }
static void		evalTexture2DArrayGrad		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DArray(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), computeLodFromGrad2DArray(c))*p.scale + p.bias; }
static void		evalTexture3DGrad			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture3D(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), computeLodFromGrad3D(c))*p.scale + p.bias; }
static void		evalTexture1DGrad			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1D(c, c.in[0].x(), computeLodFromGrad1D(c))*p.scale + p.bias; }
static void		evalTexture1DArrayGrad		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DArray(c, c.in[0].x(), c.in[0].y(), computeLodFromGrad1DArray(c))*p.scale + p.bias; }
static void		evalTextureCubeArrayGrad	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = textureCubeArray(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), c.in[0].w(), computeLodFromGradCubeArray(c))*p.scale + p.bias; }

static void		evalTexture2DShadowGrad			(ShaderEvalContext& c, const TexLookupParams&)		{ c.color.x() = texture2DShadow(c, c.in[0].z(), c.in[0].x(), c.in[0].y(), computeLodFromGrad2D(c)); }
static void		evalTextureCubeShadowGrad		(ShaderEvalContext& c, const TexLookupParams&)		{ c.color.x() = textureCubeShadow(c, c.in[0].w(), c.in[0].x(), c.in[0].y(), c.in[0].z(), computeLodFromGradCube(c)); }
static void		evalTexture2DArrayShadowGrad	(ShaderEvalContext& c, const TexLookupParams&)		{ c.color.x() = texture2DArrayShadow(c, c.in[0].w(), c.in[0].x(), c.in[0].y(), c.in[0].z(), computeLodFromGrad2DArray(c)); }
static void		evalTexture1DShadowGrad			(ShaderEvalContext& c, const TexLookupParams&)		{ c.color.x() = texture1DShadow(c, c.in[0].z(), c.in[0].x(), computeLodFromGrad1D(c)); }
static void		evalTexture1DArrayShadowGrad	(ShaderEvalContext& c, const TexLookupParams&)		{ c.color.x() = texture1DArrayShadow(c, c.in[0].z(), c.in[0].x(), c.in[0].y(), computeLodFromGrad1DArray(c)); }

static void		evalTexture2DGradOffset			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DOffset(c, c.in[0].x(), c.in[0].y(), computeLodFromGrad2D(c), p.offset.swizzle(0,1))*p.scale + p.bias; }
static void		evalTexture2DArrayGradOffset	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DArrayOffset(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), computeLodFromGrad2DArray(c), p.offset.swizzle(0,1))*p.scale + p.bias; }
static void		evalTexture3DGradOffset			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture3DOffset(c, c.in[0].x(), c.in[0].y(), c.in[0].z(), computeLodFromGrad3D(c), p.offset)*p.scale + p.bias; }
static void		evalTexture1DGradOffset			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DOffset(c, c.in[0].x(), computeLodFromGrad1D(c), p.offset.x())*p.scale + p.bias; }
static void		evalTexture1DArrayGradOffset	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DArrayOffset(c, c.in[0].x(), c.in[0].y(), computeLodFromGrad1DArray(c), p.offset.x())*p.scale + p.bias; }

static void		evalTexture2DShadowGradOffset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture2DShadowOffset(c, c.in[0].z(), c.in[0].x(), c.in[0].y(), computeLodFromGrad2D(c), p.offset.swizzle(0,1)); }
static void		evalTexture2DArrayShadowGradOffset	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture2DArrayShadowOffset(c, c.in[0].w(), c.in[0].x(), c.in[0].y(), c.in[0].z(), computeLodFromGrad2DArray(c), p.offset.swizzle(0,1)); }
static void		evalTexture1DShadowGradOffset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture1DShadowOffset(c, c.in[0].z(), c.in[0].x(), computeLodFromGrad1D(c), p.offset.x()); }
static void		evalTexture1DArrayShadowGradOffset	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture1DArrayShadowOffset(c, c.in[0].z(), c.in[0].x(), c.in[0].y(), computeLodFromGrad1DArray(c), p.offset.x()); }

static void		evalTexture2DShadowProjGrad			(ShaderEvalContext& c, const TexLookupParams&)		{ c.color.x() = texture2DShadow(c, c.in[0].z()/c.in[0].w(), c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), computeLodFromGrad2D(c)); }
static void		evalTexture2DShadowProjGradOffset	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture2DShadowOffset(c, c.in[0].z()/c.in[0].w(), c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), computeLodFromGrad2D(c), p.offset.swizzle(0,1)); }
static void		evalTexture1DShadowProjGrad			(ShaderEvalContext& c, const TexLookupParams&)		{ c.color.x() = texture1DShadow(c, c.in[0].z()/c.in[0].w(), c.in[0].x()/c.in[0].w(), computeLodFromGrad1D(c)); }
static void		evalTexture1DShadowProjGradOffset	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color.x() = texture1DShadowOffset(c, c.in[0].z()/c.in[0].w(), c.in[0].x()/c.in[0].w(), computeLodFromGrad1D(c), p.offset.x()); }

static void		evalTexture2DProjGrad3			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2D(c, c.in[0].x()/c.in[0].z(), c.in[0].y()/c.in[0].z(), computeLodFromGrad2D(c))*p.scale + p.bias; }
static void		evalTexture2DProjGrad			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2D(c, c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), computeLodFromGrad2D(c))*p.scale + p.bias; }
static void		evalTexture3DProjGrad			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture3D(c, c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), c.in[0].z()/c.in[0].w(), computeLodFromGrad3D(c))*p.scale + p.bias; }
static void		evalTexture1DProjGrad2			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1D(c, c.in[0].x()/c.in[0].y(), computeLodFromGrad1D(c))*p.scale + p.bias; }
static void		evalTexture1DProjGrad			(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1D(c, c.in[0].x()/c.in[0].w(), computeLodFromGrad1D(c))*p.scale + p.bias; }

static void		evalTexture2DProjGrad3Offset	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DOffset(c, c.in[0].x()/c.in[0].z(), c.in[0].y()/c.in[0].z(), computeLodFromGrad2D(c), p.offset.swizzle(0,1))*p.scale + p.bias; }
static void		evalTexture2DProjGradOffset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture2DOffset(c, c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), computeLodFromGrad2D(c), p.offset.swizzle(0,1))*p.scale + p.bias; }
static void		evalTexture3DProjGradOffset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture3DOffset(c, c.in[0].x()/c.in[0].w(), c.in[0].y()/c.in[0].w(), c.in[0].z()/c.in[0].w(), computeLodFromGrad3D(c), p.offset)*p.scale + p.bias; }
static void		evalTexture1DProjGrad2Offset	(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DOffset(c, c.in[0].x()/c.in[0].y(), computeLodFromGrad1D(c), p.offset.x())*p.scale + p.bias; }
static void		evalTexture1DProjGradOffset		(ShaderEvalContext& c, const TexLookupParams& p)	{ c.color = texture1DOffset(c, c.in[0].x()/c.in[0].w(), computeLodFromGrad1D(c), p.offset.x())*p.scale + p.bias; }

// Texel fetch variants

static void evalTexelFetch2D (ShaderEvalContext& c, const TexLookupParams& p)
{
	int	x	= deChopFloatToInt32(c.in[0].x())+p.offset.x();
	int	y	= deChopFloatToInt32(c.in[0].y())+p.offset.y();
	int	lod = deChopFloatToInt32(c.in[1].x());
	c.color = c.textures[0].tex2D->getLevel(lod).getPixel(x, y)*p.scale + p.bias;
}

static void evalTexelFetch2DArray (ShaderEvalContext& c, const TexLookupParams& p)
{
	int	x	= deChopFloatToInt32(c.in[0].x())+p.offset.x();
	int	y	= deChopFloatToInt32(c.in[0].y())+p.offset.y();
	int	l	= deChopFloatToInt32(c.in[0].z());
	int	lod = deChopFloatToInt32(c.in[1].x());
	c.color = c.textures[0].tex2DArray->getLevel(lod).getPixel(x, y, l)*p.scale + p.bias;
}

static void evalTexelFetch3D (ShaderEvalContext& c, const TexLookupParams& p)
{
	int	x	= deChopFloatToInt32(c.in[0].x())+p.offset.x();
	int	y	= deChopFloatToInt32(c.in[0].y())+p.offset.y();
	int	z	= deChopFloatToInt32(c.in[0].z())+p.offset.z();
	int	lod = deChopFloatToInt32(c.in[1].x());
	c.color = c.textures[0].tex3D->getLevel(lod).getPixel(x, y, z)*p.scale + p.bias;
}

static void evalTexelFetch1D (ShaderEvalContext& c, const TexLookupParams& p)
{
	int	x	= deChopFloatToInt32(c.in[0].x())+p.offset.x();
	int	lod = deChopFloatToInt32(c.in[1].x());
	c.color = c.textures[0].tex1D->getLevel(lod).getPixel(x, 0)*p.scale + p.bias;
}

static void evalTexelFetch1DArray (ShaderEvalContext& c, const TexLookupParams& p)
{
	int	x	= deChopFloatToInt32(c.in[0].x())+p.offset.x();
	int	l	= deChopFloatToInt32(c.in[0].y());
	int	lod = deChopFloatToInt32(c.in[1].x());
	c.color = c.textures[0].tex1DArray->getLevel(lod).getPixel(x, l)*p.scale + p.bias;
}

class TexLookupEvaluator : public ShaderEvaluator
{
public:
							TexLookupEvaluator		(TexEvalFunc evalFunc, const TexLookupParams& lookupParams) : m_evalFunc(evalFunc), m_lookupParams(lookupParams) {}
	virtual					~TexLookupEvaluator		(void) {}

	virtual void			evaluate				(ShaderEvalContext& ctx) const { m_evalFunc(ctx, m_lookupParams); }

private:
	TexEvalFunc				m_evalFunc;
	const TexLookupParams&	m_lookupParams;
};

static void checkDeviceFeatures (Context& context, TextureType textureType)
{
	if (textureType == TEXTURETYPE_CUBE_ARRAY)
	{
		const vk::VkPhysicalDeviceFeatures&	deviceFeatures	= context.getDeviceFeatures();

		if (!deviceFeatures.imageCubeArray)
			TCU_THROW(NotSupportedError, "Cube array is not supported");
	}
}

class ShaderTextureFunctionInstance : public ShaderRenderCaseInstance
{
public:
								ShaderTextureFunctionInstance		(Context&					context,
																	 const bool					isVertexCase,
																	 const ShaderEvaluator&		evaluator,
																	 const UniformSetup&		uniformSetup,
																	 const TextureLookupSpec&	lookupSpec,
																	 const TextureSpec&			textureSpec,
																	 const TexLookupParams&		lookupParams,
																	 const ImageBackingMode		imageBackingMode = IMAGE_BACKING_MODE_REGULAR);
	virtual						~ShaderTextureFunctionInstance		(void);

protected:
	virtual void				setupUniforms						(const tcu::Vec4&);
	void						initTexture							(void);
private:
	const TextureLookupSpec&	m_lookupSpec;
	const TextureSpec&			m_textureSpec;
	const TexLookupParams&		m_lookupParams;
};

ShaderTextureFunctionInstance::ShaderTextureFunctionInstance (Context&						context,
															  const bool					isVertexCase,
															  const ShaderEvaluator&		evaluator,
															  const UniformSetup&			uniformSetup,
															  const TextureLookupSpec&		lookupSpec,
															  const TextureSpec&			textureSpec,
															  const TexLookupParams&		lookupParams,
															  const ImageBackingMode		imageBackingMode)
	: ShaderRenderCaseInstance	(context, isVertexCase, evaluator, uniformSetup, DE_NULL, imageBackingMode)
	, m_lookupSpec				(lookupSpec)
	, m_textureSpec				(textureSpec)
	, m_lookupParams			(lookupParams)
{
	checkDeviceFeatures(m_context, m_textureSpec.type);

	{
		// Base coord scale & bias
		Vec4 s = m_lookupSpec.maxCoord-m_lookupSpec.minCoord;
		Vec4 b = m_lookupSpec.minCoord;

		float baseCoordTrans[] =
		{
			s.x(),		0.0f,		0.f,	b.x(),
			0.f,		s.y(),		0.f,	b.y(),
			s.z()/2.f,	-s.z()/2.f,	0.f,	s.z()/2.f + b.z(),
			-s.w()/2.f,	s.w()/2.f,	0.f,	s.w()/2.f + b.w()
		};

		m_userAttribTransforms.push_back(tcu::Mat4(baseCoordTrans));

		useAttribute(4u, A_IN0);
	}

	bool hasLodBias	= functionHasLod(m_lookupSpec.function) || m_lookupSpec.useBias;
	bool isGrad		= functionHasGrad(m_lookupSpec.function);
	DE_ASSERT(!isGrad || !hasLodBias);

	if (hasLodBias)
	{
		float s = m_lookupSpec.maxLodBias-m_lookupSpec.minLodBias;
		float b = m_lookupSpec.minLodBias;
		float lodCoordTrans[] =
		{
			s/2.0f,		s/2.0f,		0.f,	b,
			0.0f,		0.0f,		0.0f,	0.0f,
			0.0f,		0.0f,		0.0f,	0.0f,
			0.0f,		0.0f,		0.0f,	0.0f
		};

		m_userAttribTransforms.push_back(tcu::Mat4(lodCoordTrans));

		useAttribute(5u, A_IN1);
	}
	else if (isGrad)
	{
		Vec3 sx = m_lookupSpec.maxDX-m_lookupSpec.minDX;
		Vec3 sy = m_lookupSpec.maxDY-m_lookupSpec.minDY;
		float gradDxTrans[] =
		{
			sx.x()/2.0f,	sx.x()/2.0f,	0.f,	m_lookupSpec.minDX.x(),
			sx.y()/2.0f,	sx.y()/2.0f,	0.0f,	m_lookupSpec.minDX.y(),
			sx.z()/2.0f,	sx.z()/2.0f,	0.0f,	m_lookupSpec.minDX.z(),
			0.0f,			0.0f,			0.0f,	0.0f
		};
		float gradDyTrans[] =
		{
			-sy.x()/2.0f,	-sy.x()/2.0f,	0.f,	m_lookupSpec.maxDY.x(),
			-sy.y()/2.0f,	-sy.y()/2.0f,	0.0f,	m_lookupSpec.maxDY.y(),
			-sy.z()/2.0f,	-sy.z()/2.0f,	0.0f,	m_lookupSpec.maxDY.z(),
			0.0f,			0.0f,			0.0f,	0.0f
		};

		m_userAttribTransforms.push_back(tcu::Mat4(gradDxTrans));
		m_userAttribTransforms.push_back(tcu::Mat4(gradDyTrans));

		useAttribute(5u, A_IN1);
		useAttribute(6u, A_IN2);
	}

	initTexture();
}

ShaderTextureFunctionInstance::~ShaderTextureFunctionInstance (void)
{
}

void ShaderTextureFunctionInstance::setupUniforms (const tcu::Vec4&)
{
	useSampler(0u, 0u);
	addUniform(1u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(tcu::Vec4), m_lookupParams.scale.getPtr());
	addUniform(2u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(tcu::Vec4), m_lookupParams.bias.getPtr());
}

void ShaderTextureFunctionInstance::initTexture (void)
{
	static const IVec4		texCubeSwz[] =
	{
		IVec4(0,0,1,1),
		IVec4(1,1,0,0),
		IVec4(0,1,0,1),
		IVec4(1,0,1,0),
		IVec4(0,1,1,0),
		IVec4(1,0,0,1)
	};
	DE_STATIC_ASSERT(DE_LENGTH_OF_ARRAY(texCubeSwz) == tcu::CUBEFACE_LAST);

	tcu::TextureFormat		texFmt			= glu::mapGLInternalFormat(m_textureSpec.format);
	tcu::TextureFormatInfo	fmtInfo			= tcu::getTextureFormatInfo(texFmt);
	tcu::UVec2				viewportSize	= getViewportSize();
	bool					isProj			= functionHasProj(m_lookupSpec.function);
	bool					isAutoLod		= functionHasAutoLod(m_isVertexCase ? glu::SHADERTYPE_VERTEX : glu::SHADERTYPE_FRAGMENT,
																 m_lookupSpec.function); // LOD can vary significantly
	float					proj			= isProj ? 1.0f/m_lookupSpec.minCoord[m_lookupSpec.function == FUNCTION_TEXTUREPROJ2 ? 1 : m_lookupSpec.function == FUNCTION_TEXTUREPROJ3 ? 2 : 3] : 1.0f;
	TexLookupParams			lookupParams;

	switch (m_textureSpec.type)
	{
		case TEXTURETYPE_2D:
		{
			float								levelStep		= isAutoLod ? 0.0f : 1.0f / (float)de::max(1, m_textureSpec.numLevels-1);
			Vec4								cScale			= fmtInfo.valueMax-fmtInfo.valueMin;
			Vec4								cBias			= fmtInfo.valueMin;
			int									baseCellSize	= de::min(m_textureSpec.width/4, m_textureSpec.height/4);
			de::MovePtr<tcu::Texture2D>			texture2D;

			texture2D = de::MovePtr<tcu::Texture2D>(new tcu::Texture2D(texFmt, m_textureSpec.width, m_textureSpec.height));

			for (int level = 0; level < m_textureSpec.numLevels; level++)
			{
				float	fA		= float(level)*levelStep;
				float	fB		= 1.0f-fA;
				Vec4	colorA	= cBias + cScale*Vec4(fA, fB, fA, fB);
				Vec4	colorB	= cBias + cScale*Vec4(fB, fA, fB, fA);

				texture2D->allocLevel(level);
				tcu::fillWithGrid(texture2D->getLevel(level), de::max(1, baseCellSize>>level), colorA, colorB);
			}

			// Compute LOD.
			float	dudx	= (m_lookupSpec.maxCoord[0]-m_lookupSpec.minCoord[0])*proj*(float)m_textureSpec.width	/ (float)viewportSize[0];
			float	dvdy	= (m_lookupSpec.maxCoord[1]-m_lookupSpec.minCoord[1])*proj*(float)m_textureSpec.height	/ (float)viewportSize[1];
			lookupParams.lod = computeLodFromDerivates(DEFAULT_LOD_MODE, dudx, 0.0f, 0.0f, dvdy);

			// Append to texture list.
			m_textures.push_back(TextureBindingSp(new TextureBinding(texture2D.release(), m_textureSpec.sampler)));
			break;
		}

		case TEXTURETYPE_CUBE_MAP:
		{
			float								levelStep		= isAutoLod ? 0.0f : 1.0f / (float)de::max(1, m_textureSpec.numLevels-1);
			Vec4								cScale			= fmtInfo.valueMax-fmtInfo.valueMin;
			Vec4								cBias			= fmtInfo.valueMin;
			Vec4								cCorner			= cBias + cScale*0.5f;
			int									baseCellSize	= de::min(m_textureSpec.width/4, m_textureSpec.height/4);
			de::MovePtr<tcu::TextureCube>		textureCube;

			DE_ASSERT(m_textureSpec.width == m_textureSpec.height);
			textureCube = de::MovePtr<tcu::TextureCube>(new tcu::TextureCube(texFmt, m_textureSpec.width));

			for (int level = 0; level < m_textureSpec.numLevels; level++)
			{
				float	fA		= float(level)*levelStep;
				float	fB		= 1.0f-fA;
				Vec2	f		(fA, fB);

				for (int face = 0; face < tcu::CUBEFACE_LAST; face++)
				{
					const IVec4&	swzA	= texCubeSwz[face];
					IVec4			swzB	= 1-swzA;
					Vec4			colorA	= cBias + cScale*f.swizzle(swzA[0], swzA[1], swzA[2], swzA[3]);
					Vec4			colorB	= cBias + cScale*f.swizzle(swzB[0], swzB[1], swzB[2], swzB[3]);

					textureCube->allocLevel((tcu::CubeFace)face, level);

					{
						const tcu::PixelBufferAccess	access		= textureCube->getLevelFace(level, (tcu::CubeFace)face);
						const int						lastPix		= access.getWidth()-1;

						tcu::fillWithGrid(access, de::max(1, baseCellSize>>level), colorA, colorB);

						// Ensure all corners have identical colors in order to avoid dealing with ambiguous corner texel filtering
						access.setPixel(cCorner, 0, 0);
						access.setPixel(cCorner, 0, lastPix);
						access.setPixel(cCorner, lastPix, 0);
						access.setPixel(cCorner, lastPix, lastPix);
					}
				}
			}

			// Compute LOD \note Assumes that only single side is accessed and R is constant major axis.
			DE_ASSERT(de::abs(m_lookupSpec.minCoord[2] - m_lookupSpec.maxCoord[2]) < 0.005);
			DE_ASSERT(de::abs(m_lookupSpec.minCoord[0]) < de::abs(m_lookupSpec.minCoord[2]) && de::abs(m_lookupSpec.maxCoord[0]) < de::abs(m_lookupSpec.minCoord[2]));
			DE_ASSERT(de::abs(m_lookupSpec.minCoord[1]) < de::abs(m_lookupSpec.minCoord[2]) && de::abs(m_lookupSpec.maxCoord[1]) < de::abs(m_lookupSpec.minCoord[2]));

			tcu::CubeFaceFloatCoords	c00		= tcu::getCubeFaceCoords(Vec3(m_lookupSpec.minCoord[0]*proj, m_lookupSpec.minCoord[1]*proj, m_lookupSpec.minCoord[2]*proj));
			tcu::CubeFaceFloatCoords	c10		= tcu::getCubeFaceCoords(Vec3(m_lookupSpec.maxCoord[0]*proj, m_lookupSpec.minCoord[1]*proj, m_lookupSpec.minCoord[2]*proj));
			tcu::CubeFaceFloatCoords	c01		= tcu::getCubeFaceCoords(Vec3(m_lookupSpec.minCoord[0]*proj, m_lookupSpec.maxCoord[1]*proj, m_lookupSpec.minCoord[2]*proj));
			float						dudx	= (c10.s - c00.s)*(float)m_textureSpec.width	/ (float)viewportSize[0];
			float						dvdy	= (c01.t - c00.t)*(float)m_textureSpec.height	/ (float)viewportSize[1];
			lookupParams.lod = computeLodFromDerivates(DEFAULT_LOD_MODE, dudx, 0.0f, 0.0f, dvdy);

			// Append to texture list.
			m_textures.push_back(TextureBindingSp(new TextureBinding(textureCube.release(), m_textureSpec.sampler)));
			break;
		}

		case TEXTURETYPE_2D_ARRAY:
		{
			float								layerStep		= 1.0f / (float)m_textureSpec.depth;
			float								levelStep		= isAutoLod ? 0.0f : 1.0f / (float)(de::max(1, m_textureSpec.numLevels-1)*m_textureSpec.depth);
			Vec4								cScale			= fmtInfo.valueMax-fmtInfo.valueMin;
			Vec4								cBias			= fmtInfo.valueMin;
			int									baseCellSize	= de::min(m_textureSpec.width/4, m_textureSpec.height/4);
			de::MovePtr<tcu::Texture2DArray>	texture2DArray;

			texture2DArray = de::MovePtr<tcu::Texture2DArray>(new tcu::Texture2DArray(texFmt, m_textureSpec.width, m_textureSpec.height, m_textureSpec.depth));

			for (int level = 0; level < m_textureSpec.numLevels; level++)
			{
				texture2DArray->allocLevel(level);
				tcu::PixelBufferAccess levelAccess = texture2DArray->getLevel(level);

				for (int layer = 0; layer < levelAccess.getDepth(); layer++)
				{
					float	fA		= (float)layer*layerStep + (float)level*levelStep;
					float	fB		= 1.0f-fA;
					Vec4	colorA	= cBias + cScale*Vec4(fA, fB, fA, fB);
					Vec4	colorB	= cBias + cScale*Vec4(fB, fA, fB, fA);

					tcu::fillWithGrid(tcu::getSubregion(levelAccess, 0, 0, layer, levelAccess.getWidth(), levelAccess.getHeight(), 1), de::max(1, baseCellSize>>level), colorA, colorB);
				}
			}

			// Compute LOD.
			float	dudx	= (m_lookupSpec.maxCoord[0]-m_lookupSpec.minCoord[0])*proj*(float)m_textureSpec.width	/ (float)viewportSize[0];
			float	dvdy	= (m_lookupSpec.maxCoord[1]-m_lookupSpec.minCoord[1])*proj*(float)m_textureSpec.height	/ (float)viewportSize[1];
			lookupParams.lod = computeLodFromDerivates(DEFAULT_LOD_MODE, dudx, 0.0f, 0.0f, dvdy);

			// Append to texture list.
			m_textures.push_back(TextureBindingSp(new TextureBinding(texture2DArray.release(), m_textureSpec.sampler)));
			break;
		}

		case TEXTURETYPE_3D:
		{
			float								levelStep		= isAutoLod ? 0.0f : 1.0f / (float)de::max(1, m_textureSpec.numLevels-1);
			Vec4								cScale			= fmtInfo.valueMax-fmtInfo.valueMin;
			Vec4								cBias			= fmtInfo.valueMin;
			int									baseCellSize	= de::min(de::min(m_textureSpec.width/2, m_textureSpec.height/2), m_textureSpec.depth/2);
			de::MovePtr<tcu::Texture3D>			texture3D;

			texture3D = de::MovePtr<tcu::Texture3D>(new tcu::Texture3D(texFmt, m_textureSpec.width, m_textureSpec.height, m_textureSpec.depth));

			for (int level = 0; level < m_textureSpec.numLevels; level++)
			{
				float	fA		= (float)level*levelStep;
				float	fB		= 1.0f-fA;
				Vec4	colorA	= cBias + cScale*Vec4(fA, fB, fA, fB);
				Vec4	colorB	= cBias + cScale*Vec4(fB, fA, fB, fA);

				texture3D->allocLevel(level);
				tcu::fillWithGrid(texture3D->getLevel(level), de::max(1, baseCellSize>>level), colorA, colorB);
			}

			// Compute LOD.
			float	dudx	= (m_lookupSpec.maxCoord[0]-m_lookupSpec.minCoord[0])*proj*(float)m_textureSpec.width		/ (float)viewportSize[0];
			float	dvdy	= (m_lookupSpec.maxCoord[1]-m_lookupSpec.minCoord[1])*proj*(float)m_textureSpec.height		/ (float)viewportSize[1];
			float	dwdx	= (m_lookupSpec.maxCoord[2]-m_lookupSpec.minCoord[2])*0.5f*proj*(float)m_textureSpec.depth	/ (float)viewportSize[0];
			float	dwdy	= (m_lookupSpec.maxCoord[2]-m_lookupSpec.minCoord[2])*0.5f*proj*(float)m_textureSpec.depth	/ (float)viewportSize[1];
			lookupParams.lod = computeLodFromDerivates(DEFAULT_LOD_MODE, dudx, 0.0f, dwdx, 0.0f, dvdy, dwdy);

			// Append to texture list.
			m_textures.push_back(TextureBindingSp(new TextureBinding(texture3D.release(), m_textureSpec.sampler)));
			break;
		}

		case TEXTURETYPE_1D:
		{
			float								levelStep		= isAutoLod ? 0.0f : 1.0f / (float)de::max(1, m_textureSpec.numLevels-1);
			Vec4								cScale			= fmtInfo.valueMax-fmtInfo.valueMin;
			Vec4								cBias			= fmtInfo.valueMin;
			int									baseCellSize	= m_textureSpec.width/4;
			de::MovePtr<tcu::Texture1D>			texture1D;

			texture1D = de::MovePtr<tcu::Texture1D>(new tcu::Texture1D(texFmt, m_textureSpec.width));

			for (int level = 0; level < m_textureSpec.numLevels; level++)
			{
				float	fA		= float(level)*levelStep;
				float	fB		= 1.0f-fA;
				Vec4	colorA	= cBias + cScale*Vec4(fA, fB, fA, fB);
				Vec4	colorB	= cBias + cScale*Vec4(fB, fA, fB, fA);

				texture1D->allocLevel(level);
				tcu::fillWithGrid(texture1D->getLevel(level), de::max(1, baseCellSize>>level), colorA, colorB);
			}

			// Compute LOD.
			float	dudx	= (m_lookupSpec.maxCoord[0]-m_lookupSpec.minCoord[0])*proj*(float)m_textureSpec.width	/ (float)viewportSize[0];
			lookupParams.lod = computeLodFromDerivates(DEFAULT_LOD_MODE, dudx, 0.0f);

			// Append to texture list.
			m_textures.push_back(TextureBindingSp(new TextureBinding(texture1D.release(), m_textureSpec.sampler)));
			break;
		}

		case TEXTURETYPE_1D_ARRAY:
		{
			float								layerStep		= 1.0f / (float)m_textureSpec.depth;
			float								levelStep		= isAutoLod ? 0.0f : 1.0f / (float)(de::max(1, m_textureSpec.numLevels-1)*m_textureSpec.depth);
			Vec4								cScale			= fmtInfo.valueMax-fmtInfo.valueMin;
			Vec4								cBias			= fmtInfo.valueMin;
			int									baseCellSize	= m_textureSpec.width/4;
			de::MovePtr<tcu::Texture1DArray>	texture1DArray;

			texture1DArray = de::MovePtr<tcu::Texture1DArray>(new tcu::Texture1DArray(texFmt, m_textureSpec.width, m_textureSpec.depth));

			for (int level = 0; level < m_textureSpec.numLevels; level++)
			{
				texture1DArray->allocLevel(level);
				tcu::PixelBufferAccess levelAccess = texture1DArray->getLevel(level);

				for (int layer = 0; layer < levelAccess.getHeight(); layer++)
				{
					float	fA		= (float)layer*layerStep + (float)level*levelStep;
					float	fB		= 1.0f-fA;
					Vec4	colorA	= cBias + cScale*Vec4(fA, fB, fA, fB);
					Vec4	colorB	= cBias + cScale*Vec4(fB, fA, fB, fA);

					tcu::fillWithGrid(tcu::getSubregion(levelAccess, 0, layer, 0, levelAccess.getWidth(), 1, 1), de::max(1, baseCellSize>>level), colorA, colorB);
				}
			}

			// Compute LOD.
			float	dudx	= (m_lookupSpec.maxCoord[0]-m_lookupSpec.minCoord[0])*proj*(float)m_textureSpec.width	/ (float)viewportSize[0];
			lookupParams.lod = computeLodFromDerivates(DEFAULT_LOD_MODE, dudx, 0.0f);

			// Append to texture list.
			m_textures.push_back(TextureBindingSp(new TextureBinding(texture1DArray.release(), m_textureSpec.sampler)));
			break;
		}

		case TEXTURETYPE_CUBE_ARRAY:
		{
			float								layerStep			= 1.0f / (float)(m_textureSpec.depth/6);
			float								levelStep			= isAutoLod ? 0.0f : 1.0f / (float)(de::max(1, m_textureSpec.numLevels-1)*(m_textureSpec.depth/6));
			Vec4								cScale				= fmtInfo.valueMax-fmtInfo.valueMin;
			Vec4								cBias				= fmtInfo.valueMin;
			Vec4								cCorner				= cBias + cScale*0.5f;
			int									baseCellSize		= de::min(m_textureSpec.width/4, m_textureSpec.height/4);
			de::MovePtr<tcu::TextureCubeArray>	textureCubeArray;

			DE_ASSERT(m_textureSpec.width == m_textureSpec.height);
			DE_ASSERT(m_textureSpec.depth % 6 == 0);

			textureCubeArray = de::MovePtr<tcu::TextureCubeArray>(new tcu::TextureCubeArray(texFmt, m_textureSpec.width, m_textureSpec.depth));

			for (int level = 0; level < m_textureSpec.numLevels; level++)
			{
				float	fA		= float(level)*levelStep;
				float	fB		= 1.0f-fA;
				Vec2	f		(fA, fB);

				textureCubeArray->allocLevel(level);
				tcu::PixelBufferAccess levelAccess = textureCubeArray->getLevel(level);

				for (int layer = 0; layer < m_textureSpec.depth/6; layer++)
				{
					float layerCorr = 1.0f-(float)layer*layerStep;

					for (int face = 0; face < tcu::CUBEFACE_LAST; face++)
					{
						const IVec4&	swzA	= texCubeSwz[face];
						IVec4			swzB	= 1-swzA;
						Vec4			colorA	= cBias + cScale*f.swizzle(swzA[0], swzA[1], swzA[2], swzA[3])*layerCorr;
						Vec4			colorB	= cBias + cScale*f.swizzle(swzB[0], swzB[1], swzB[2], swzB[3])*layerCorr;

						{
							const tcu::PixelBufferAccess	access		= tcu::getSubregion(levelAccess, 0, 0, (layer*6)+face, levelAccess.getWidth(), levelAccess.getHeight(), 1);
							const int						lastPix		= access.getWidth()-1;

							tcu::fillWithGrid(access, de::max(1, baseCellSize>>level), colorA, colorB);

							// Ensure all corners have identical colors in order to avoid dealing with ambiguous corner texel filtering
							access.setPixel(cCorner, 0, 0);
							access.setPixel(cCorner, 0, lastPix);
							access.setPixel(cCorner, lastPix, 0);
							access.setPixel(cCorner, lastPix, lastPix);
						}
					}
				}
			}

			// Compute LOD \note Assumes that only single side is accessed and R is constant major axis.
			DE_ASSERT(de::abs(m_lookupSpec.minCoord[2] - m_lookupSpec.maxCoord[2]) < 0.005);
			DE_ASSERT(de::abs(m_lookupSpec.minCoord[0]) < de::abs(m_lookupSpec.minCoord[2]) && de::abs(m_lookupSpec.maxCoord[0]) < de::abs(m_lookupSpec.minCoord[2]));
			DE_ASSERT(de::abs(m_lookupSpec.minCoord[1]) < de::abs(m_lookupSpec.minCoord[2]) && de::abs(m_lookupSpec.maxCoord[1]) < de::abs(m_lookupSpec.minCoord[2]));

			tcu::CubeFaceFloatCoords	c00		= tcu::getCubeFaceCoords(Vec3(m_lookupSpec.minCoord[0]*proj, m_lookupSpec.minCoord[1]*proj, m_lookupSpec.minCoord[2]*proj));
			tcu::CubeFaceFloatCoords	c10		= tcu::getCubeFaceCoords(Vec3(m_lookupSpec.maxCoord[0]*proj, m_lookupSpec.minCoord[1]*proj, m_lookupSpec.minCoord[2]*proj));
			tcu::CubeFaceFloatCoords	c01		= tcu::getCubeFaceCoords(Vec3(m_lookupSpec.minCoord[0]*proj, m_lookupSpec.maxCoord[1]*proj, m_lookupSpec.minCoord[2]*proj));
			float						dudx	= (c10.s - c00.s)*(float)m_textureSpec.width	/ (float)viewportSize[0];
			float						dvdy	= (c01.t - c00.t)*(float)m_textureSpec.height	/ (float)viewportSize[1];
			lookupParams.lod = computeLodFromDerivates(DEFAULT_LOD_MODE, dudx, 0.0f, 0.0f, dvdy);

			// Append to texture list.
			m_textures.push_back(TextureBindingSp(new TextureBinding(textureCubeArray.release(), m_textureSpec.sampler)));
			break;
		}

		default:
			DE_ASSERT(DE_FALSE);
	}

	// Set lookup scale & bias
	lookupParams.scale		= fmtInfo.lookupScale;
	lookupParams.bias		= fmtInfo.lookupBias;
	lookupParams.offset		= m_lookupSpec.offset;

	// \todo [dirnerakos] Avoid const cast somehow
	const_cast<TexLookupParams&>(m_lookupParams) = lookupParams;
}

class ShaderTextureFunctionCase : public ShaderRenderCase
{
public:
								ShaderTextureFunctionCase		(tcu::TestContext&				testCtx,
																 const std::string&				name,
																 const std::string&				desc,
																 const TextureLookupSpec&		lookup,
																 const TextureSpec&				texture,
																 TexEvalFunc					evalFunc,
																 bool							isVertexCase);
	virtual						~ShaderTextureFunctionCase		(void);

	virtual TestInstance*		createInstance					(Context& context) const;

protected:
	const TextureLookupSpec		m_lookupSpec;
	const TextureSpec			m_textureSpec;
	const TexLookupParams		m_lookupParams;

	void						initShaderSources				(void);
};

ShaderTextureFunctionCase::ShaderTextureFunctionCase (tcu::TestContext&				testCtx,
													  const std::string&			name,
													  const std::string&			desc,
													  const TextureLookupSpec&		lookup,
													  const TextureSpec&			texture,
													  TexEvalFunc					evalFunc,
													  bool							isVertexCase)
	: ShaderRenderCase		(testCtx, name, desc, isVertexCase, new TexLookupEvaluator(evalFunc, m_lookupParams), NULL, NULL)
	, m_lookupSpec			(lookup)
	, m_textureSpec			(texture)
{
	initShaderSources();
}

ShaderTextureFunctionCase::~ShaderTextureFunctionCase (void)
{
}

TestInstance* ShaderTextureFunctionCase::createInstance (Context& context) const
{
	DE_ASSERT(m_evaluator != DE_NULL);
	DE_ASSERT(m_uniformSetup != DE_NULL);
	return new ShaderTextureFunctionInstance(context, m_isVertexCase, *m_evaluator, *m_uniformSetup, m_lookupSpec, m_textureSpec, m_lookupParams);
}

void ShaderTextureFunctionCase::initShaderSources (void)
{
	Function			function			= m_lookupSpec.function;
	bool				isVtxCase			= m_isVertexCase;
	bool				isProj				= functionHasProj(function);
	bool				isGrad				= functionHasGrad(function);
	bool				isShadow			= m_textureSpec.sampler.compare != tcu::Sampler::COMPAREMODE_NONE;
	bool				is2DProj4			= !isShadow && m_textureSpec.type == TEXTURETYPE_2D && (function == FUNCTION_TEXTUREPROJ || function == FUNCTION_TEXTUREPROJLOD || function == FUNCTION_TEXTUREPROJGRAD);
	bool				is1DProj4			= !isShadow && m_textureSpec.type == TEXTURETYPE_1D && (function == FUNCTION_TEXTUREPROJ || function == FUNCTION_TEXTUREPROJLOD || function == FUNCTION_TEXTUREPROJGRAD);
	bool				isIntCoord			= function == FUNCTION_TEXELFETCH;
	bool				hasLodBias			= functionHasLod(m_lookupSpec.function) || m_lookupSpec.useBias;
	int					texCoordComps		= m_textureSpec.type == TEXTURETYPE_1D ? 1 :
											  m_textureSpec.type == TEXTURETYPE_1D_ARRAY || m_textureSpec.type == TEXTURETYPE_2D ? 2 :
											  m_textureSpec.type == TEXTURETYPE_CUBE_ARRAY ? 4 :
											  3;
	int					extraCoordComps		= (isProj ? (is2DProj4 ? 2 : (is1DProj4 ? 3 : 1)) : 0) + (isShadow ? (m_textureSpec.type == TEXTURETYPE_1D ? 2 : 1) : 0);
	const bool			isCubeArrayShadow	= isShadow && m_textureSpec.type == TEXTURETYPE_CUBE_ARRAY;
	glu::DataType		coordType			= glu::getDataTypeFloatVec(isCubeArrayShadow ? 4 : texCoordComps+extraCoordComps);
	glu::Precision		coordPrec			= glu::PRECISION_HIGHP;
	const char*			coordTypeName		= glu::getDataTypeName(coordType);
	const char*			coordPrecName		= glu::getPrecisionName(coordPrec);
	tcu::TextureFormat	texFmt				= glu::mapGLInternalFormat(m_textureSpec.format);
	glu::DataType		samplerType			= glu::TYPE_LAST;
	glu::DataType		gradType			= m_textureSpec.type == TEXTURETYPE_1D || m_textureSpec.type == TEXTURETYPE_1D_ARRAY ? glu::TYPE_FLOAT :
											  m_textureSpec.type == TEXTURETYPE_3D || m_textureSpec.type == TEXTURETYPE_CUBE_MAP || m_textureSpec.type == TEXTURETYPE_CUBE_ARRAY ? glu::TYPE_FLOAT_VEC3 :
											  glu::TYPE_FLOAT_VEC2;
	const char*			gradTypeName		= glu::getDataTypeName(gradType);
	const char*			baseFuncName		= DE_NULL;

	DE_ASSERT(!isGrad || !hasLodBias);

	switch (m_textureSpec.type)
	{
		case TEXTURETYPE_2D:			samplerType = isShadow ? glu::TYPE_SAMPLER_2D_SHADOW			: glu::getSampler2DType(texFmt);		break;
		case TEXTURETYPE_CUBE_MAP:		samplerType = isShadow ? glu::TYPE_SAMPLER_CUBE_SHADOW			: glu::getSamplerCubeType(texFmt);		break;
		case TEXTURETYPE_2D_ARRAY:		samplerType = isShadow ? glu::TYPE_SAMPLER_2D_ARRAY_SHADOW		: glu::getSampler2DArrayType(texFmt);	break;
		case TEXTURETYPE_3D:			DE_ASSERT(!isShadow); samplerType = glu::getSampler3DType(texFmt);										break;
		case TEXTURETYPE_1D:			samplerType = isShadow ? glu::TYPE_SAMPLER_1D_SHADOW			: glu::getSampler1DType(texFmt);		break;
		case TEXTURETYPE_1D_ARRAY:		samplerType = isShadow ? glu::TYPE_SAMPLER_1D_ARRAY_SHADOW		: glu::getSampler1DArrayType(texFmt);	break;
		case TEXTURETYPE_CUBE_ARRAY:	samplerType = isShadow ? glu::TYPE_SAMPLER_CUBE_ARRAY_SHADOW	: glu::getSamplerCubeArrayType(texFmt);	break;
		default:
			DE_ASSERT(DE_FALSE);
	}

	switch (m_lookupSpec.function)
	{
		case FUNCTION_TEXTURE:			baseFuncName = "texture";			break;
		case FUNCTION_TEXTUREPROJ:		baseFuncName = "textureProj";		break;
		case FUNCTION_TEXTUREPROJ2:		baseFuncName = "textureProj";		break;
		case FUNCTION_TEXTUREPROJ3:		baseFuncName = "textureProj";		break;
		case FUNCTION_TEXTURELOD:		baseFuncName = "textureLod";		break;
		case FUNCTION_TEXTUREPROJLOD:	baseFuncName = "textureProjLod";	break;
		case FUNCTION_TEXTUREPROJLOD2:	baseFuncName = "textureProjLod";	break;
		case FUNCTION_TEXTUREPROJLOD3:	baseFuncName = "textureProjLod";	break;
		case FUNCTION_TEXTUREGRAD:		baseFuncName = "textureGrad";		break;
		case FUNCTION_TEXTUREPROJGRAD:	baseFuncName = "textureProjGrad";	break;
		case FUNCTION_TEXTUREPROJGRAD2:	baseFuncName = "textureProjGrad";	break;
		case FUNCTION_TEXTUREPROJGRAD3:	baseFuncName = "textureProjGrad";	break;
		case FUNCTION_TEXELFETCH:		baseFuncName = "texelFetch";		break;
		default:
			DE_ASSERT(DE_FALSE);
	}

	std::ostringstream	vert;
	std::ostringstream	frag;
	std::ostringstream&	op		= isVtxCase ? vert : frag;
	glu::GLSLVersion	version	= glu::GLSL_VERSION_LAST;

	switch (m_textureSpec.type)
	{
		case TEXTURETYPE_2D:
		case TEXTURETYPE_3D:
		case TEXTURETYPE_CUBE_MAP:
		case TEXTURETYPE_2D_ARRAY:
			version = glu::GLSL_VERSION_310_ES;
			break;

		case TEXTURETYPE_1D:
		case TEXTURETYPE_1D_ARRAY:
		case TEXTURETYPE_CUBE_ARRAY:
			version = glu::GLSL_VERSION_420;
			break;

		default:
			DE_ASSERT(DE_FALSE);
			break;
	}

	vert << glu::getGLSLVersionDeclaration(version) << "\n"
		 << "layout(location = 0) in highp vec4 a_position;\n"
		 << "layout(location = 4) in " << coordPrecName << " " << coordTypeName << " a_in0;\n";

	if (isGrad)
	{
		vert << "layout(location = 5) in " << coordPrecName << " " << gradTypeName << " a_in1;\n";
		vert << "layout(location = 6) in " << coordPrecName << " " << gradTypeName << " a_in2;\n";
	}
	else if (hasLodBias)
		vert << "layout(location = 5) in " << coordPrecName << " float a_in1;\n";

	frag << glu::getGLSLVersionDeclaration(version) << "\n"
		 << "layout(location = 0) out mediump vec4 o_color;\n";

	if (isVtxCase)
	{
		vert << "layout(location = 0) out mediump vec4 v_color;\n";
		frag << "layout(location = 0) in mediump vec4 v_color;\n";
	}
	else
	{
		vert << "layout(location = 0) out " << coordPrecName << " " << coordTypeName << " v_texCoord;\n";
		frag << "layout(location = 0) in " << coordPrecName << " " << coordTypeName << " v_texCoord;\n";

		if (isGrad)
		{
			vert << "layout(location = 1) out " << coordPrecName << " " << gradTypeName << " v_gradX;\n";
			vert << "layout(location = 2) out " << coordPrecName << " " << gradTypeName << " v_gradY;\n";
			frag << "layout(location = 1) in " << coordPrecName << " " << gradTypeName << " v_gradX;\n";
			frag << "layout(location = 2) in " << coordPrecName << " " << gradTypeName << " v_gradY;\n";
		}
		else if (hasLodBias)
		{
			vert << "layout(location = 1) out " << coordPrecName << " float v_lodBias;\n";
			frag << "layout(location = 1) in " << coordPrecName << " float v_lodBias;\n";
		}
	}

	// Uniforms
	op << "layout(set = 0, binding = 0) uniform highp " << glu::getDataTypeName(samplerType) << " u_sampler;\n"
	   << "layout(set = 0, binding = 1) uniform buf0 { highp vec4 u_scale; };\n"
	   << "layout(set = 0, binding = 2) uniform buf1 { highp vec4 u_bias; };\n";

	if (version != glu::GLSL_VERSION_310_ES)
		vert << "out gl_PerVertex {\n"
			 << "\tvec4 gl_Position;\n"
			 << "};\n";

	vert << "\nvoid main()\n{\n"
		 << "\tgl_Position = a_position;\n";
	frag << "\nvoid main()\n{\n";

	if (isVtxCase)
		vert << "\tv_color = ";
	else
		frag << "\to_color = ";

	// Op.
	{
		const char*	texCoord	= isVtxCase ? "a_in0" : "v_texCoord";
		const char* gradX		= isVtxCase ? "a_in1" : "v_gradX";
		const char* gradY		= isVtxCase ? "a_in2" : "v_gradY";
		const char*	lodBias		= isVtxCase ? "a_in1" : "v_lodBias";

		op << "vec4(" << baseFuncName;
		if (m_lookupSpec.useOffset)
			op << "Offset";
		op << "(u_sampler, ";

		if (isIntCoord)
			op << glu::getDataTypeName(glu::getDataTypeIntVec(texCoordComps+extraCoordComps)) << "(";

		op << texCoord;

		if (isIntCoord)
			op << ")";

		if (isGrad)
			op << ", " << gradX << ", " << gradY;

		if (functionHasLod(function))
		{
			if (isIntCoord)
				op << ", int(" << lodBias << ")";
			else
				op << ", " << lodBias;
		}

		if (m_lookupSpec.useOffset)
		{
			int offsetComps = m_textureSpec.type == TEXTURETYPE_1D || m_textureSpec.type == TEXTURETYPE_1D_ARRAY ? 1 :
							  m_textureSpec.type == TEXTURETYPE_3D ? 3 : 2;

			op << ", " << glu::getDataTypeName(glu::getDataTypeIntVec(offsetComps)) << "(";
			for (int ndx = 0; ndx < offsetComps; ndx++)
			{
				if (ndx != 0)
					op << ", ";
				op << m_lookupSpec.offset[ndx];
			}
			op << ")";
		}

		if (isCubeArrayShadow && m_lookupSpec.function == FUNCTION_TEXTURE)
			op << ", " << texCoord << ".w";

		if (m_lookupSpec.useBias)
			op << ", " << lodBias;

		op << ")";

		if (isShadow)
			op << ", 0.0, 0.0, 1.0)";
		else
			op << ")*u_scale + u_bias";

		op << ";\n";
	}

	if (isVtxCase)
		frag << "\to_color = v_color;\n";
	else
	{
		vert << "\tv_texCoord = a_in0;\n";

		if (isGrad)
		{
			vert << "\tv_gradX = a_in1;\n";
			vert << "\tv_gradY = a_in2;\n";
		}
		else if (hasLodBias)
			vert << "\tv_lodBias = a_in1;\n";
	}

	vert << "}\n";
	frag << "}\n";

	m_vertShaderSource = vert.str();
	m_fragShaderSource = frag.str();
}

enum QueryFunction
{
	QUERYFUNCTION_TEXTURESIZE = 0,
	QUERYFUNCTION_TEXTUREQUERYLOD,
	QUERYFUNCTION_TEXTUREQUERYLEVELS,
	QUERYFUNCTION_TEXTURESAMPLES,

	QUERYFUNCTION_LAST
};

class TextureQueryInstance : public ShaderRenderCaseInstance
{
public:
								TextureQueryInstance			(Context&					context,
																 const bool					isVertexCase,
																 const TextureSpec&			textureSpec);
	virtual						~TextureQueryInstance			(void);

protected:
	virtual void				setupDefaultInputs				(void);
	virtual void				setupUniforms					(const tcu::Vec4&);

	void						render							(void);

protected:
	const TextureSpec&			m_textureSpec;
};

TextureQueryInstance::TextureQueryInstance (Context&				context,
											const bool				isVertexCase,
											const TextureSpec&		textureSpec)
	: ShaderRenderCaseInstance	(context, isVertexCase, DE_NULL, DE_NULL, DE_NULL)
	, m_textureSpec				(textureSpec)
{
	m_colorFormat = vk::VK_FORMAT_R32G32B32A32_SFLOAT;

	checkDeviceFeatures(m_context, m_textureSpec.type);
}

TextureQueryInstance::~TextureQueryInstance (void)
{
}

void TextureQueryInstance::setupDefaultInputs (void)
{
	const deUint32		numVertices		= 4;
	const float			positions[]		=
	{
		-1.0f, -1.0f, 0.0f, 1.0f,
		-1.0f,  1.0f, 0.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f,
		 1.0f,  1.0f, 0.0f, 1.0f
	};

	addAttribute(0u, vk::VK_FORMAT_R32G32B32A32_SFLOAT, 4 * (deUint32)sizeof(float), numVertices, positions);
}

void TextureQueryInstance::setupUniforms (const tcu::Vec4&)
{
	useSampler(0u, 0u);
}

void TextureQueryInstance::render (void)
{
	const deUint32		numVertices		= 4;
	const deUint32		numTriangles	= 2;
	const deUint16		indices[6]		= { 0, 1, 2, 2, 1, 3 };

	ShaderRenderCaseInstance::setup();

	ShaderRenderCaseInstance::render(numVertices, numTriangles, indices);
}

static int getMaxTextureSize (TextureType type, const tcu::IVec3& textureSize)
{
	int		maxSize		= 0;

	switch (type)
	{
		case TEXTURETYPE_1D:
		case TEXTURETYPE_1D_ARRAY:
			maxSize = textureSize.x();
			break;

		case TEXTURETYPE_2D:
		case TEXTURETYPE_2D_ARRAY:
		case TEXTURETYPE_CUBE_MAP:
		case TEXTURETYPE_CUBE_ARRAY:
			maxSize = de::max(textureSize.x(), textureSize.y());
			break;

		case TEXTURETYPE_3D:
			maxSize = de::max(textureSize.x(), de::max(textureSize.y(), textureSize.z()));
			break;

		default:
			DE_ASSERT(false);
	}

	return maxSize;
}

static std::string getTextureSizeString (TextureType type, const tcu::IVec3& textureSize)
{
	std::ostringstream	str;

	switch (type)
	{
		case TEXTURETYPE_1D:
			str << textureSize.x() << "x1";
			break;

		case TEXTURETYPE_2D:
		case TEXTURETYPE_CUBE_MAP:
			str << textureSize.x() << "x" << textureSize.y();
			break;

		case TEXTURETYPE_3D:
			str << textureSize.x() << "x" << textureSize.y() << "x" << textureSize.z();
			break;

		case TEXTURETYPE_1D_ARRAY:
			str << textureSize.x() << "x1 with " << textureSize.z() << " layer(s)";
			break;

		case TEXTURETYPE_2D_ARRAY:
		case TEXTURETYPE_CUBE_ARRAY:
			str << textureSize.x() << "x" << textureSize.y() << " with " << textureSize.z() << " layers(s)";
			break;

		default:
			DE_ASSERT(false);
			break;
	}

	return str.str();
}

static bool isValidCase (TextureType type, const tcu::IVec3& textureSize, int lod, int lodBase)
{
	const bool		isSquare		= textureSize.x() == textureSize.y();
	const bool		isCubeArray		= isSquare && (textureSize.z() % 6) == 0;
	const int		maxSize			= getMaxTextureSize(type, textureSize);
	const bool		isLodValid		= (maxSize >> (lod + lodBase)) != 0;

	if (!isLodValid)
		return false;
	if (type == TEXTURETYPE_CUBE_MAP && !isSquare)
		return false;
	if (type == TEXTURETYPE_CUBE_ARRAY && !isCubeArray)
		return false;

	return true;
}

static TextureBindingSp createEmptyTexture (deUint32				format,
											TextureType				type,
											const tcu::IVec3&		textureSize,
											int						numLevels,
											int						lodBase,
											const tcu::Sampler&		sampler)
{
	const tcu::TextureFormat			texFmt				= glu::mapGLInternalFormat(format);
	const TextureBinding::Parameters	params				(lodBase);
	TextureBindingSp					textureBinding;

	switch (type)
	{

		case TEXTURETYPE_1D:
		{
			de::MovePtr<tcu::Texture1D>			texture		(new tcu::Texture1D(texFmt, textureSize.x()));

			for (int level = 0; level < numLevels; level++)
				texture->allocLevel(level);

			textureBinding = TextureBindingSp(new TextureBinding(texture.release(), sampler));
			break;
		}

		case TEXTURETYPE_2D:
		{
			de::MovePtr<tcu::Texture2D>			texture		(new tcu::Texture2D(texFmt, textureSize.x(), textureSize.y()));

			for (int level = 0; level < numLevels; level++)
				texture->allocLevel(level);

			textureBinding = TextureBindingSp(new TextureBinding(texture.release(), sampler));
			break;
		}

		case TEXTURETYPE_3D:
		{
			de::MovePtr<tcu::Texture3D>			texture		(new tcu::Texture3D(texFmt, textureSize.x(), textureSize.y(), textureSize.z()));

			for (int level = 0; level < numLevels; level++)
				texture->allocLevel(level);

			textureBinding = TextureBindingSp(new TextureBinding(texture.release(), sampler));
			break;
		}

		case TEXTURETYPE_CUBE_MAP:
		{
			de::MovePtr<tcu::TextureCube>		texture		(new tcu::TextureCube(texFmt, textureSize.x()));

			for (int level = 0; level < numLevels; level++)
				for (int face = 0; face < tcu::CUBEFACE_LAST; face++)
					texture->allocLevel((tcu::CubeFace)face, level);

			textureBinding = TextureBindingSp(new TextureBinding(texture.release(), sampler));
			break;
		}

		case TEXTURETYPE_1D_ARRAY:
		{
			de::MovePtr<tcu::Texture1DArray>	texture		(new tcu::Texture1DArray(texFmt, textureSize.x(), textureSize.z()));

			for (int level = 0; level < numLevels; level++)
				texture->allocLevel(level);

			textureBinding = TextureBindingSp(new TextureBinding(texture.release(), sampler));
			break;
		}

		case TEXTURETYPE_2D_ARRAY:
		{
			de::MovePtr<tcu::Texture2DArray>	texture		(new tcu::Texture2DArray(texFmt, textureSize.x(), textureSize.y(), textureSize.z()));

			for (int level = 0; level < numLevels; level++)
				texture->allocLevel(level);

			textureBinding = TextureBindingSp(new TextureBinding(texture.release(), sampler));
			break;
		}

		case TEXTURETYPE_CUBE_ARRAY:
		{
			de::MovePtr<tcu::TextureCubeArray>	texture		(new tcu::TextureCubeArray(texFmt, textureSize.x(), textureSize.z()));

			for (int level = 0; level < numLevels; level++)
				texture->allocLevel(level);

			textureBinding = TextureBindingSp(new TextureBinding(texture.release(), sampler));
			break;
		}

		default:
			DE_ASSERT(false);
			break;
	}

	textureBinding->setParameters(params);
	return textureBinding;
}

static inline glu::DataType getTextureSizeFuncResultType (TextureType textureType)
{
	switch (textureType)
	{
		case TEXTURETYPE_1D:
			return glu::TYPE_INT;

		case TEXTURETYPE_2D:
		case TEXTURETYPE_CUBE_MAP:
		case TEXTURETYPE_1D_ARRAY:
			return glu::TYPE_INT_VEC2;

		case TEXTURETYPE_3D:
		case TEXTURETYPE_2D_ARRAY:
		case TEXTURETYPE_CUBE_ARRAY:
			return glu::TYPE_INT_VEC3;

		default:
			DE_ASSERT(false);
			return glu::TYPE_LAST;
	}
}

class TextureSizeInstance : public TextureQueryInstance
{
public:
								TextureSizeInstance				(Context&					context,
																 const bool					isVertexCase,
																 const TextureSpec&			textureSpec);
	virtual						~TextureSizeInstance			(void);

	virtual tcu::TestStatus		iterate							(void);

protected:
	virtual void				setupUniforms					(const tcu::Vec4& constCoords);

private:
	struct TestSize
	{
		tcu::IVec3	textureSize;
		int			lod;
		int			lodBase;
		tcu::IVec3	expectedSize;
	};

	void						initTexture						(void);
	bool						testTextureSize					(void);

	TestSize					m_testSize;
	tcu::IVec3					m_expectedSize;
	int							m_iterationCounter;
};

TextureSizeInstance::TextureSizeInstance (Context&					context,
										  const bool				isVertexCase,
										  const TextureSpec&		textureSpec)
	: TextureQueryInstance		(context, isVertexCase, textureSpec)
	, m_testSize				()
	, m_expectedSize			()
	, m_iterationCounter		(0)
{
	deMemset(&m_testSize, 0, sizeof(TestSize));

	m_renderSize = tcu::UVec2(1, 1);
}

TextureSizeInstance::~TextureSizeInstance (void)
{
}

void TextureSizeInstance::setupUniforms (const tcu::Vec4& constCoords)
{
	TextureQueryInstance::setupUniforms(constCoords);
	addUniform(1u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(int), &m_testSize.lod);
}

void TextureSizeInstance::initTexture (void)
{
	tcu::TestLog&			log					= m_context.getTestContext().getLog();
	const int				numLevels			= m_testSize.lod + m_testSize.lodBase + 1;
	TextureBindingSp		textureBinding;

	log << tcu::TestLog::Message << "Testing image size " << getTextureSizeString(m_textureSpec.type, m_testSize.textureSize) << tcu::TestLog::EndMessage;
	log << tcu::TestLog::Message << "Lod: " << m_testSize.lod << ", base level: " << m_testSize.lodBase << tcu::TestLog::EndMessage;

	switch (m_textureSpec.type)
	{
		case TEXTURETYPE_3D:
			log << tcu::TestLog::Message << "Expecting: " << m_testSize.expectedSize.x() << "x" << m_testSize.expectedSize.y() << "x" << m_testSize.expectedSize.z() << tcu::TestLog::EndMessage;
			break;

		case TEXTURETYPE_2D:
			log << tcu::TestLog::Message << "Expecting: " << m_testSize.expectedSize.x() << "x" << m_testSize.expectedSize.y() << tcu::TestLog::EndMessage;
			break;

		case TEXTURETYPE_CUBE_MAP:
			log << tcu::TestLog::Message << "Expecting: " << m_testSize.expectedSize.x() << "x" << m_testSize.expectedSize.y() << tcu::TestLog::EndMessage;
			break;

		case TEXTURETYPE_2D_ARRAY:
			log << tcu::TestLog::Message << "Expecting: " << m_testSize.expectedSize.x() << "x" << m_testSize.expectedSize.y() << " and " << m_testSize.textureSize.z() << " layer(s)" << tcu::TestLog::EndMessage;
			break;

		case TEXTURETYPE_CUBE_ARRAY:
			log << tcu::TestLog::Message << "Expecting: " << m_testSize.expectedSize.x() << "x" << m_testSize.expectedSize.y() << " and " << (m_testSize.textureSize.z() / 6) << " cube(s)" << tcu::TestLog::EndMessage;
			break;

		case TEXTURETYPE_1D:
			log << tcu::TestLog::Message << "Expecting: " << m_testSize.expectedSize.x() << tcu::TestLog::EndMessage;
			break;

		case TEXTURETYPE_1D_ARRAY:
			log << tcu::TestLog::Message << "Expecting: " << m_testSize.expectedSize.x() << " and " << m_testSize.textureSize.z() << " layer(s)" << tcu::TestLog::EndMessage;
			break;

		default:
			DE_ASSERT(false);
			break;
	}

	textureBinding = createEmptyTexture(m_textureSpec.format, m_textureSpec.type, m_testSize.textureSize, numLevels, m_testSize.lodBase, m_textureSpec.sampler);

	m_textures.clear();
	m_textures.push_back(textureBinding);
}

tcu::TestStatus TextureSizeInstance::iterate (void)
{
	const TestSize testSizes[] =
	{
		{ tcu::IVec3(1, 2, 1),			0,		0,	tcu::IVec3(1, 2, 1)			},
		{ tcu::IVec3(1, 2, 1),			1,		0,	tcu::IVec3(1, 1, 1)			},

		{ tcu::IVec3(1, 3, 2),			0,		0,	tcu::IVec3(1, 3, 2)			},
		{ tcu::IVec3(1, 3, 2),			1,		0,	tcu::IVec3(1, 1, 1)			},

		{ tcu::IVec3(100, 31, 18),		0,		0,	tcu::IVec3(100, 31, 18)		},
		{ tcu::IVec3(100, 31, 18),		1,		0,	tcu::IVec3(50, 15, 9)		},
		{ tcu::IVec3(100, 31, 18),		2,		0,	tcu::IVec3(25, 7, 4)		},
		{ tcu::IVec3(100, 31, 18),		3,		0,	tcu::IVec3(12, 3, 2)		},
		{ tcu::IVec3(100, 31, 18),		4,		0,	tcu::IVec3(6, 1, 1)			},
		{ tcu::IVec3(100, 31, 18),		5,		0,	tcu::IVec3(3, 1, 1)			},
		{ tcu::IVec3(100, 31, 18),		6,		0,	tcu::IVec3(1, 1, 1)			},

		{ tcu::IVec3(100, 128, 32),		0,		0,	tcu::IVec3(100, 128, 32)	},
		{ tcu::IVec3(100, 128, 32),		1,		0,	tcu::IVec3(50, 64, 16)		},
		{ tcu::IVec3(100, 128, 32),		2,		0,	tcu::IVec3(25, 32, 8)		},
		{ tcu::IVec3(100, 128, 32),		3,		0,	tcu::IVec3(12, 16, 4)		},
		{ tcu::IVec3(100, 128, 32),		4,		0,	tcu::IVec3(6, 8, 2)			},
		{ tcu::IVec3(100, 128, 32),		5,		0,	tcu::IVec3(3, 4, 1)			},
		{ tcu::IVec3(100, 128, 32),		6,		0,	tcu::IVec3(1, 2, 1)			},
		{ tcu::IVec3(100, 128, 32),		7,		0,	tcu::IVec3(1, 1, 1)			},

		// pow 2
		{ tcu::IVec3(128, 64, 32),		0,		0,	tcu::IVec3(128, 64, 32)		},
		{ tcu::IVec3(128, 64, 32),		1,		0,	tcu::IVec3(64, 32, 16)		},
		{ tcu::IVec3(128, 64, 32),		2,		0,	tcu::IVec3(32, 16, 8)		},
		{ tcu::IVec3(128, 64, 32),		3,		0,	tcu::IVec3(16, 8, 4)		},
		{ tcu::IVec3(128, 64, 32),		4,		0,	tcu::IVec3(8, 4, 2)			},
		{ tcu::IVec3(128, 64, 32),		5,		0,	tcu::IVec3(4, 2, 1)			},
		{ tcu::IVec3(128, 64, 32),		6,		0,	tcu::IVec3(2, 1, 1)			},
		{ tcu::IVec3(128, 64, 32),		7,		0,	tcu::IVec3(1, 1, 1)			},

		// w == h
		{ tcu::IVec3(1, 1, 1),			0,		0,	tcu::IVec3(1, 1, 1)			},
		{ tcu::IVec3(64, 64, 64),		0,		0,	tcu::IVec3(64, 64, 64)		},
		{ tcu::IVec3(64, 64, 64),		1,		0,	tcu::IVec3(32, 32, 32)		},
		{ tcu::IVec3(64, 64, 64),		2,		0,	tcu::IVec3(16, 16, 16)		},
		{ tcu::IVec3(64, 64, 64),		3,		0,	tcu::IVec3(8, 8, 8)			},
		{ tcu::IVec3(64, 64, 64),		4,		0,	tcu::IVec3(4, 4, 4)			},

		// with lod base
		{ tcu::IVec3(100, 31, 18),		3,		1,	tcu::IVec3(6, 1, 1)			},
		{ tcu::IVec3(128, 64, 32),		3,		1,	tcu::IVec3(8, 4, 2)			},
		{ tcu::IVec3(64, 64, 64),		1,		1,	tcu::IVec3(16, 16, 16)		},

		// w == h and d % 6 == 0 (for cube array)
		{ tcu::IVec3(1, 1, 6),			0,		0,	tcu::IVec3(1, 1, 6)			},
		{ tcu::IVec3(32, 32, 12),		0,		0,	tcu::IVec3(32, 32, 12)		},
		{ tcu::IVec3(32, 32, 12),		0,		1,	tcu::IVec3(16, 16, 6)		},
		{ tcu::IVec3(32, 32, 12),		1,		0,	tcu::IVec3(16, 16, 6)		},
		{ tcu::IVec3(32, 32, 12),		2,		0,	tcu::IVec3(8, 8, 3)			},
		{ tcu::IVec3(32, 32, 12),		3,		0,	tcu::IVec3(4, 4, 1)			},
		{ tcu::IVec3(32, 32, 12),		4,		0,	tcu::IVec3(2, 2, 1)			},
		{ tcu::IVec3(32, 32, 12),		5,		0,	tcu::IVec3(1, 1, 1)			},
	};
	const int lastIterationIndex = DE_LENGTH_OF_ARRAY(testSizes) + 1;

	m_iterationCounter++;

	if (m_iterationCounter == lastIterationIndex)
		return tcu::TestStatus::pass("Pass");
	else
	{
		// set current test size
		m_testSize = testSizes[m_iterationCounter - 1];

		if (!testTextureSize())
			return tcu::TestStatus::fail("Got unexpected result");

		return tcu::TestStatus::incomplete();
	}
}

bool TextureSizeInstance::testTextureSize (void)
{
	tcu::TestLog&			log				= m_context.getTestContext().getLog();
	bool					success			= true;

	// skip incompatible cases
	if (!isValidCase(m_textureSpec.type, m_testSize.textureSize, m_testSize.lod, m_testSize.lodBase))
		return true;

	// setup texture
	initTexture();

	// determine expected texture size
	switch (m_textureSpec.type)
	{
		case TEXTURETYPE_1D:
		case TEXTURETYPE_2D:
		case TEXTURETYPE_3D:
		case TEXTURETYPE_CUBE_MAP:
			m_expectedSize = m_testSize.expectedSize;
			break;

		case TEXTURETYPE_1D_ARRAY:
			m_expectedSize = tcu::IVec3(m_testSize.expectedSize.x(), m_testSize.textureSize.z(), 0);
			break;

		case TEXTURETYPE_2D_ARRAY:
			m_expectedSize = tcu::IVec3(m_testSize.expectedSize.x(), m_testSize.expectedSize.y(), m_testSize.textureSize.z());
			break;

		case TEXTURETYPE_CUBE_ARRAY:
			m_expectedSize = tcu::IVec3(m_testSize.expectedSize.x(), m_testSize.expectedSize.y(), m_testSize.textureSize.z() / 6);
			break;

		default:
			DE_ASSERT(false);
			break;
	}

	// render
	TextureQueryInstance::render();

	// test
	{
		const tcu::TextureLevel&	result				= getResultImage();
		tcu::IVec4					output				= result.getAccess().getPixelInt(0, 0);
		const int					resultComponents	= glu::getDataTypeScalarSize(getTextureSizeFuncResultType(m_textureSpec.type));

		for (int ndx = 0; ndx < resultComponents; ndx++)
		{
			if (output[ndx] != m_expectedSize[ndx])
			{
				success = false;
				break;
			}
		}

		if (success)
		{
			// success
			log << tcu::TestLog::Message << "Passed" << tcu::TestLog::EndMessage;
		}
		else
		{
			// failure
			std::stringstream	resultSizeStr;
			switch (resultComponents)
			{
				case 1:
					resultSizeStr << output[0];
					break;
				case 2:
					resultSizeStr << output.toWidth<2>();
					break;
				case 3:
					resultSizeStr << output.toWidth<3>();
					break;
				default:
					DE_ASSERT(false);
					break;
			}
			log << tcu::TestLog::Message << "Result: " << resultSizeStr.str() << tcu::TestLog::EndMessage;
			log << tcu::TestLog::Message << "Failed" << tcu::TestLog::EndMessage;
		}
	}

	log << tcu::TestLog::Message << tcu::TestLog::EndMessage;

	return success;
}

static vk::VkImageType getVkImageType (TextureType type)
{
	switch (type)
	{
		case TEXTURETYPE_1D:
		case TEXTURETYPE_1D_ARRAY:
			return vk::VK_IMAGE_TYPE_1D;

		case TEXTURETYPE_2D:
		case TEXTURETYPE_2D_ARRAY:
		case TEXTURETYPE_CUBE_MAP:
		case TEXTURETYPE_CUBE_ARRAY:
			return vk::VK_IMAGE_TYPE_2D;

		case TEXTURETYPE_3D:
			return vk::VK_IMAGE_TYPE_3D;

		default:
			DE_ASSERT(false);
			return (vk::VkImageType)0;
	}
}

class TextureSamplesInstance : public TextureQueryInstance
{
public:
								TextureSamplesInstance			(Context&					context,
																 const bool					isVertexCase,
																 const TextureSpec&			textureSpec);
	virtual						~TextureSamplesInstance			(void);

	virtual tcu::TestStatus		iterate							(void);

private:
	void						initTexture						(void);

	int										m_iterationCounter;
	vector<vk::VkSampleCountFlagBits>		m_iterations;
};

TextureSamplesInstance::TextureSamplesInstance (Context&				context,
												const bool				isVertexCase,
												const TextureSpec&		textureSpec)
	: TextureQueryInstance		(context, isVertexCase, textureSpec)
	, m_iterationCounter		(0)
{
	m_renderSize = tcu::UVec2(1, 1);

	// determine available sample counts
	{
		const vk::VkFormat						format			= vk::mapTextureFormat(glu::mapGLInternalFormat(m_textureSpec.format));
		const vk::VkImageType					imageType		= getVkImageType(m_textureSpec.type);
		vk::VkImageFormatProperties				properties;

		if (m_context.getInstanceInterface().getPhysicalDeviceImageFormatProperties(m_context.getPhysicalDevice(),
																					format,
																					imageType,
																					vk::VK_IMAGE_TILING_OPTIMAL,
																					vk::VK_IMAGE_USAGE_SAMPLED_BIT | vk::VK_IMAGE_USAGE_TRANSFER_DST_BIT,
																					(vk::VkImageCreateFlags)0,
																					&properties) == vk::VK_ERROR_FORMAT_NOT_SUPPORTED)
			TCU_THROW(NotSupportedError, "Format not supported");

		// NOTE: The test case initializes MS images (for all supported N of samples), runs a program
		//       which invokes OpImageQuerySamples against the image and checks the result.
		//
		//       Now, in the SPIR-V spec for the very operation we have the following language:
		//
		//       OpImageQuerySamples
		//       Query the number of samples available per texel fetch in a multisample image.
		//       Result Type must be a scalar integer type.
		//       The result is the number of samples.
		//       Image must be an object whose type is OpTypeImage.
		//       Its Dim operand must be one of 2D and **MS of 1(multisampled).
		//
		//       "MS of 1" implies the image must not be single-sample, meaning we must exclude
		//       VK_SAMPLE_COUNT_1_BIT in the sampleFlags array below, and may have to skip further testing.
		static const vk::VkSampleCountFlagBits	sampleFlags[]	=
		{
			vk::VK_SAMPLE_COUNT_2_BIT,
			vk::VK_SAMPLE_COUNT_4_BIT,
			vk::VK_SAMPLE_COUNT_8_BIT,
			vk::VK_SAMPLE_COUNT_16_BIT,
			vk::VK_SAMPLE_COUNT_32_BIT,
			vk::VK_SAMPLE_COUNT_64_BIT
		};

		for (int samplesNdx = 0; samplesNdx < DE_LENGTH_OF_ARRAY(sampleFlags); samplesNdx++)
		{
			const vk::VkSampleCountFlagBits&	flag			= sampleFlags[samplesNdx];

			if ((properties.sampleCounts & flag) != 0)
				m_iterations.push_back(flag);
		}

		if (m_iterations.empty())
		{
			// Sampled images of integer formats may support only 1 sample. Exit the test with "Not supported" in these cases.
			if (tcu::getTextureChannelClass(mapVkFormat(format).type) == tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER ||
				tcu::getTextureChannelClass(mapVkFormat(format).type) == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER)
			{
				TCU_THROW(NotSupportedError, "Skipping validation of integer formats as only VK_SAMPLE_COUNT_1_BIT is supported.");
			}

			DE_ASSERT(false);
		}
	}

	// setup texture
	initTexture();
}

TextureSamplesInstance::~TextureSamplesInstance (void)
{
}

tcu::TestStatus TextureSamplesInstance::iterate (void)
{
	tcu::TestLog&		log		= m_context.getTestContext().getLog();

	// update samples count
	{
		DE_ASSERT(m_textures.size() == 1);

		TextureBinding::Parameters	params	= m_textures[0]->getParameters();

		params.initialization	= TextureBinding::INIT_CLEAR;
		params.samples			= m_iterations[m_iterationCounter];
		log << tcu::TestLog::Message << "Expected samples: " << m_iterations[m_iterationCounter] << tcu::TestLog::EndMessage;

		m_textures[0]->setParameters(params);
	}

	// render
	TextureQueryInstance::render();

	// test
	{
		const tcu::TextureLevel&	result				= getResultImage();
		tcu::IVec4					output				= result.getAccess().getPixelInt(0, 0);

		if (output.x() == (int)m_iterations[m_iterationCounter])
		{
			// success
			log << tcu::TestLog::Message << "Passed" << tcu::TestLog::EndMessage;
		}
		else
		{
			// failure
			log << tcu::TestLog::Message << "Result: " << output.x() << tcu::TestLog::EndMessage;
			log << tcu::TestLog::Message << "Failed" << tcu::TestLog::EndMessage;
			return tcu::TestStatus::fail("Got unexpected result");
		}

		m_iterationCounter++;
		if (m_iterationCounter == (int)m_iterations.size())
			return tcu::TestStatus::pass("Pass");
		else
			return tcu::TestStatus::incomplete();
	}
}

void TextureSamplesInstance::initTexture (void)
{
	tcu::TestLog&			log					= m_context.getTestContext().getLog();
	tcu::IVec3				textureSize			(m_textureSpec.width, m_textureSpec.height, m_textureSpec.depth);
	TextureBindingSp		textureBinding;

	DE_ASSERT(m_textures.empty());
	DE_ASSERT(m_textureSpec.type == TEXTURETYPE_2D || m_textureSpec.type == TEXTURETYPE_2D_ARRAY);

	log << tcu::TestLog::Message << "Image size: " << getTextureSizeString(m_textureSpec.type, textureSize) << tcu::TestLog::EndMessage;

	textureBinding = createEmptyTexture(m_textureSpec.format, m_textureSpec.type, textureSize, m_textureSpec.numLevels, 0 /* lodBase */, m_textureSpec.sampler);

	m_textures.push_back(textureBinding);
}

class TextureQueryLevelsInstance : public TextureQueryInstance
{
public:
								TextureQueryLevelsInstance		(Context&					context,
																 const bool					isVertexCase,
																 const TextureSpec&			textureSpec);
	virtual						~TextureQueryLevelsInstance		(void);

	virtual tcu::TestStatus		iterate							(void);

private:
	struct TestSize
	{
		tcu::IVec3	textureSize;
		int			lodBase;
	};

	void						initTexture						(void);
	bool						testTextureLevels				(void);

	TestSize					m_testSize;
	int							m_levels;
	int							m_iterationCounter;
};

TextureQueryLevelsInstance::TextureQueryLevelsInstance (Context&				context,
														const bool				isVertexCase,
														const TextureSpec&		textureSpec)
	: TextureQueryInstance		(context, isVertexCase, textureSpec)
	, m_testSize				()
	, m_levels					(0)
	, m_iterationCounter		(0)
{
	deMemset(&m_testSize, 0, sizeof(TestSize));

	m_renderSize = tcu::UVec2(1, 1);
}

TextureQueryLevelsInstance::~TextureQueryLevelsInstance (void)
{
}

tcu::TestStatus TextureQueryLevelsInstance::iterate (void)
{
	const TestSize testSizes[] =
	{
		{ tcu::IVec3(1, 2, 1),			0	},
		{ tcu::IVec3(1, 2, 1),			1	},

		{ tcu::IVec3(1, 3, 2),			0	},
		{ tcu::IVec3(1, 3, 2),			1	},

		{ tcu::IVec3(100, 31, 18),		0	},
		{ tcu::IVec3(100, 31, 18),		1	},
		{ tcu::IVec3(100, 31, 18),		2	},
		{ tcu::IVec3(100, 31, 18),		3	},
		{ tcu::IVec3(100, 31, 18),		4	},
		{ tcu::IVec3(100, 31, 18),		5	},
		{ tcu::IVec3(100, 31, 18),		6	},

		{ tcu::IVec3(100, 128, 32),		0	},
		{ tcu::IVec3(100, 128, 32),		1	},
		{ tcu::IVec3(100, 128, 32),		2	},
		{ tcu::IVec3(100, 128, 32),		3	},
		{ tcu::IVec3(100, 128, 32),		4	},
		{ tcu::IVec3(100, 128, 32),		5	},
		{ tcu::IVec3(100, 128, 32),		6	},
		{ tcu::IVec3(100, 128, 32),		7	},

		// pow 2
		{ tcu::IVec3(128, 64, 32),		0	},
		{ tcu::IVec3(128, 64, 32),		1	},
		{ tcu::IVec3(128, 64, 32),		2	},
		{ tcu::IVec3(128, 64, 32),		3	},
		{ tcu::IVec3(128, 64, 32),		4	},
		{ tcu::IVec3(128, 64, 32),		5	},
		{ tcu::IVec3(128, 64, 32),		6	},
		{ tcu::IVec3(128, 64, 32),		7	},

		// w == h
		{ tcu::IVec3(1, 1, 1),			0	},
		{ tcu::IVec3(64, 64, 64),		0	},
		{ tcu::IVec3(64, 64, 64),		1	},
		{ tcu::IVec3(64, 64, 64),		2	},
		{ tcu::IVec3(64, 64, 64),		3	},
		{ tcu::IVec3(64, 64, 64),		4	},
		{ tcu::IVec3(64, 64, 64),		5	},
		{ tcu::IVec3(64, 64, 64),		6	},

		// w == h and d % 6 == 0 (for cube array)
		{ tcu::IVec3(1, 1, 6),			0	},
		{ tcu::IVec3(32, 32, 12),		0	},
		{ tcu::IVec3(32, 32, 12),		1	},
		{ tcu::IVec3(32, 32, 12),		2	},
		{ tcu::IVec3(32, 32, 12),		3	},
		{ tcu::IVec3(32, 32, 12),		4	},
		{ tcu::IVec3(32, 32, 12),		5	},
	};
	const int lastIterationIndex = DE_LENGTH_OF_ARRAY(testSizes) + 1;

	m_iterationCounter++;

	if (m_iterationCounter == lastIterationIndex)
		return tcu::TestStatus::pass("Pass");
	else
	{
		// set current test size
		m_testSize = testSizes[m_iterationCounter - 1];

		if (!testTextureLevels())
			return tcu::TestStatus::fail("Got unexpected result");

		return tcu::TestStatus::incomplete();
	}
}

bool TextureQueryLevelsInstance::testTextureLevels (void)
{
	tcu::TestLog&			log				= m_context.getTestContext().getLog();
	bool					success			= true;

	// skip incompatible cases
	if (!isValidCase(m_textureSpec.type, m_testSize.textureSize, 0, m_testSize.lodBase))
		return true;

	// setup texture
	initTexture();

	// calculate accessible levels
	{
		const int	mipLevels	= deLog2Floor32(getMaxTextureSize(m_textureSpec.type, m_testSize.textureSize)) + 1;

		m_levels = mipLevels - m_testSize.lodBase;
		DE_ASSERT(m_levels > 0);

		log << tcu::TestLog::Message << "Expected levels: " << m_levels << tcu::TestLog::EndMessage;
	}

	// render
	TextureQueryInstance::render();

	// test
	{
		const tcu::TextureLevel&	result				= getResultImage();
		tcu::IVec4					output				= result.getAccess().getPixelInt(0, 0);

		if (output.x() == m_levels)
		{
			// success
			log << tcu::TestLog::Message << "Passed" << tcu::TestLog::EndMessage;
		}
		else
		{
			// failure
			log << tcu::TestLog::Message << "Result: " << output.x() << tcu::TestLog::EndMessage;
			log << tcu::TestLog::Message << "Failed" << tcu::TestLog::EndMessage;
			success = false;
		}
	}

	log << tcu::TestLog::Message << tcu::TestLog::EndMessage;

	return success;
}

void TextureQueryLevelsInstance::initTexture (void)
{
	tcu::TestLog&			log					= m_context.getTestContext().getLog();
	int						numLevels			= m_testSize.lodBase + 1;
	TextureBindingSp		textureBinding;

	log << tcu::TestLog::Message << "Image size: " << getTextureSizeString(m_textureSpec.type, m_testSize.textureSize) << tcu::TestLog::EndMessage;
	log << tcu::TestLog::Message << "Base level: " << m_testSize.lodBase << tcu::TestLog::EndMessage;

	textureBinding = createEmptyTexture(m_textureSpec.format, m_textureSpec.type, m_testSize.textureSize, numLevels, m_testSize.lodBase, m_textureSpec.sampler);

	m_textures.clear();
	m_textures.push_back(textureBinding);
}

static int getQueryLodFuncTextCoordComps (TextureType type)
{
	switch (type)
	{
		case TEXTURETYPE_1D:
		case TEXTURETYPE_1D_ARRAY:
			return 1;

		case TEXTURETYPE_2D:
		case TEXTURETYPE_2D_ARRAY:
			return 2;

		case TEXTURETYPE_3D:
		case TEXTURETYPE_CUBE_MAP:
		case TEXTURETYPE_CUBE_ARRAY:
			return 3;

		default:
			DE_ASSERT(false);
			return 0;
	}
}

class TextureQueryLodInstance : public TextureQueryInstance
{
public:
								TextureQueryLodInstance			(Context&					context,
																 const bool					isVertexCase,
																 const TextureSpec&			textureSpec);
	virtual						~TextureQueryLodInstance		(void);

	virtual tcu::TestStatus		iterate							(void);

protected:
	virtual void				setupDefaultInputs				(void);

private:
	void						initTexture						(void);
	float						computeLevelFromLod				(float computedLod) const;
	vector<float>				computeQuadTexCoord				(void) const;

	tcu::Vec4					m_minCoord;
	tcu::Vec4					m_maxCoord;
	tcu::Vec2					m_lodBounds;
	tcu::Vec2					m_levelBounds;
};

TextureQueryLodInstance::TextureQueryLodInstance (Context&					context,
												  const bool				isVertexCase,
												  const TextureSpec&		textureSpec)
	: TextureQueryInstance		(context, isVertexCase, textureSpec)
	, m_minCoord				()
	, m_maxCoord				()
	, m_lodBounds				()
	, m_levelBounds				()
{
	// setup texture
	initTexture();

	// init min/max coords
	switch (m_textureSpec.type)
	{
		case TEXTURETYPE_1D:
		case TEXTURETYPE_1D_ARRAY:
			m_minCoord		= Vec4(-0.2f,  0.0f,  0.0f,  0.0f);
			m_maxCoord		= Vec4( 1.5f,  0.0f,  0.0f,  0.0f);
			break;

		case TEXTURETYPE_2D:
		case TEXTURETYPE_2D_ARRAY:
			m_minCoord		= Vec4(-0.2f, -0.4f,  0.0f,  0.0f);
			m_maxCoord		= Vec4( 1.5f,  2.3f,  0.0f,  0.0f);
			break;

		case TEXTURETYPE_3D:
			m_minCoord		= Vec4(-1.2f, -1.4f,  0.1f,  0.0f);
			m_maxCoord		= Vec4( 1.5f,  2.3f,  2.3f,  0.0f);
			break;

		case TEXTURETYPE_CUBE_MAP:
		case TEXTURETYPE_CUBE_ARRAY:
			m_minCoord		= Vec4(-1.0f, -1.0f,  1.01f,  0.0f);
			m_maxCoord		= Vec4( 1.0f,  1.0f,  1.01f,  0.0f);
			break;

		default:
			DE_ASSERT(false);
			break;
	}

	// calculate lod and accessed level
	{
		const tcu::UVec2&		viewportSize		= getViewportSize();
		const float				lodEps				= (1.0f / float(1u << m_context.getDeviceProperties().limits.mipmapPrecisionBits)) + 0.008f;

		switch (m_textureSpec.type)
		{
			case TEXTURETYPE_1D:
			case TEXTURETYPE_1D_ARRAY:
			{
				const float	dudx	= (m_maxCoord[0]-m_minCoord[0])*(float)m_textureSpec.width	/ (float)viewportSize[0];

				m_lodBounds[0]		= computeLodFromDerivates(LODMODE_MIN_BOUND, dudx, 0.0f)-lodEps;
				m_lodBounds[1]		= computeLodFromDerivates(LODMODE_MAX_BOUND, dudx, 0.0f)+lodEps;
				break;
			}

			case TEXTURETYPE_2D:
			case TEXTURETYPE_2D_ARRAY:
			{
				const float	dudx	= (m_maxCoord[0]-m_minCoord[0])*(float)m_textureSpec.width	/ (float)viewportSize[0];
				const float	dvdy	= (m_maxCoord[1]-m_minCoord[1])*(float)m_textureSpec.height	/ (float)viewportSize[1];

				m_lodBounds[0]		= computeLodFromDerivates(LODMODE_MIN_BOUND, dudx, 0.0f, 0.0f, dvdy)-lodEps;
				m_lodBounds[1]		= computeLodFromDerivates(LODMODE_MAX_BOUND, dudx, 0.0f, 0.0f, dvdy)+lodEps;
				break;
			}

			case TEXTURETYPE_CUBE_MAP:
			case TEXTURETYPE_CUBE_ARRAY:
			{
				// Compute LOD \note Assumes that only single side is accessed and R is constant major axis.
				DE_ASSERT(de::abs(m_minCoord[2] - m_maxCoord[2]) < 0.005);
				DE_ASSERT(de::abs(m_minCoord[0]) < de::abs(m_minCoord[2]) && de::abs(m_maxCoord[0]) < de::abs(m_minCoord[2]));
				DE_ASSERT(de::abs(m_minCoord[1]) < de::abs(m_minCoord[2]) && de::abs(m_maxCoord[1]) < de::abs(m_minCoord[2]));

				tcu::CubeFaceFloatCoords	c00		= tcu::getCubeFaceCoords(Vec3(m_minCoord[0], m_minCoord[1], m_minCoord[2]));
				tcu::CubeFaceFloatCoords	c10		= tcu::getCubeFaceCoords(Vec3(m_maxCoord[0], m_minCoord[1], m_minCoord[2]));
				tcu::CubeFaceFloatCoords	c01		= tcu::getCubeFaceCoords(Vec3(m_minCoord[0], m_maxCoord[1], m_minCoord[2]));
				float						dudx	= (c10.s - c00.s)*(float)m_textureSpec.width	/ (float)viewportSize[0];
				float						dvdy	= (c01.t - c00.t)*(float)m_textureSpec.height	/ (float)viewportSize[1];

				m_lodBounds[0]		= computeLodFromDerivates(LODMODE_MIN_BOUND, dudx, 0.0f, 0.0f, dvdy)-lodEps;
				m_lodBounds[1]		= computeLodFromDerivates(LODMODE_MAX_BOUND, dudx, 0.0f, 0.0f, dvdy)+lodEps;
				break;
			}

			case TEXTURETYPE_3D:
			{
				const float	dudx	= (m_maxCoord[0]-m_minCoord[0])*(float)m_textureSpec.width		/ (float)viewportSize[0];
				const float	dvdy	= (m_maxCoord[1]-m_minCoord[1])*(float)m_textureSpec.height		/ (float)viewportSize[1];
				const float	dwdx	= (m_maxCoord[2]-m_minCoord[2])*0.5f*(float)m_textureSpec.depth	/ (float)viewportSize[0];
				const float	dwdy	= (m_maxCoord[2]-m_minCoord[2])*0.5f*(float)m_textureSpec.depth	/ (float)viewportSize[1];

				m_lodBounds[0]		= computeLodFromDerivates(LODMODE_MIN_BOUND, dudx, 0.0f, dwdx, 0.0f, dvdy, dwdy)-lodEps;
				m_lodBounds[1]		= computeLodFromDerivates(LODMODE_MAX_BOUND, dudx, 0.0f, dwdx, 0.0f, dvdy, dwdy)+lodEps;
				break;
			}

			default:
				DE_ASSERT(false);
				break;
		}

		m_levelBounds[0] = computeLevelFromLod(m_lodBounds[0]);
		m_levelBounds[1] = computeLevelFromLod(m_lodBounds[1]);
	}
}

TextureQueryLodInstance::~TextureQueryLodInstance (void)
{
}

tcu::TestStatus TextureQueryLodInstance::iterate (void)
{
	tcu::TestLog&		log		= m_context.getTestContext().getLog();

	log << tcu::TestLog::Message << "Expected: level in range " << m_levelBounds << ", lod in range " << m_lodBounds << tcu::TestLog::EndMessage;

	// render
	TextureQueryInstance::render();

	// test
	{
		const tcu::TextureLevel&	result		= getResultImage();
		const tcu::Vec4				output		= result.getAccess().getPixel(0, 0);
		const float					resLevel	= output.x();
		const float					resLod		= output.y();

		if (de::inRange(resLevel, m_levelBounds[0], m_levelBounds[1]) && de::inRange(resLod, m_lodBounds[0], m_lodBounds[1]))
		{
			// success
			log << tcu::TestLog::Message << "Passed" << tcu::TestLog::EndMessage;
			return tcu::TestStatus::pass("Pass");
		}
		else
		{
			// failure
			log << tcu::TestLog::Message << "Result: level: " << resLevel << ", lod: " << resLod << tcu::TestLog::EndMessage;
			log << tcu::TestLog::Message << "Failed" << tcu::TestLog::EndMessage;
			return tcu::TestStatus::fail("Got unexpected result");
		}
	}
}

void TextureQueryLodInstance::setupDefaultInputs (void)
{
	TextureQueryInstance::setupDefaultInputs();

	const deUint32			numVertices			= 4;
	const vector<float>		texCoord			= computeQuadTexCoord();
	const int				texCoordComps		= getQueryLodFuncTextCoordComps(m_textureSpec.type);
	const vk::VkFormat		coordFormats[]		=
	{
		vk::VK_FORMAT_R32_SFLOAT,
		vk::VK_FORMAT_R32G32_SFLOAT,
		vk::VK_FORMAT_R32G32B32_SFLOAT
	};

	DE_ASSERT(de::inRange(texCoordComps, 1, 3));
	DE_ASSERT((int)texCoord.size() == texCoordComps * 4);

	addAttribute(1u, coordFormats[texCoordComps - 1], (deUint32)(texCoordComps * sizeof(float)), numVertices, texCoord.data());
}

void TextureQueryLodInstance::initTexture (void)
{
	tcu::TestLog&			log					= m_context.getTestContext().getLog();
	tcu::IVec3				textureSize			(m_textureSpec.width, m_textureSpec.height, m_textureSpec.depth);
	TextureBindingSp		textureBinding;

	DE_ASSERT(m_textures.empty());

	log << tcu::TestLog::Message << "Image size: " << getTextureSizeString(m_textureSpec.type, textureSize) << tcu::TestLog::EndMessage;

	textureBinding = createEmptyTexture(m_textureSpec.format, m_textureSpec.type, textureSize, m_textureSpec.numLevels, 0 /* lodBase */, m_textureSpec.sampler);

	m_textures.push_back(textureBinding);
}

float TextureQueryLodInstance::computeLevelFromLod (float computedLod) const
{
	const int	maxAccessibleLevel	= m_textureSpec.numLevels - 1;

	// Clamp the computed LOD to the range of accessible levels.
	computedLod = deFloatClamp(computedLod, 0.0f, (float)maxAccessibleLevel);

	// Return a value according to the min filter.
	switch (m_textureSpec.sampler.minFilter)
	{
		case tcu::Sampler::LINEAR:
		case tcu::Sampler::NEAREST:
			return 0.0f;

		case tcu::Sampler::NEAREST_MIPMAP_NEAREST:
		case tcu::Sampler::LINEAR_MIPMAP_NEAREST:
			return deFloatClamp(deFloatCeil(computedLod + 0.5f) - 1.0f, 0.0f, (float)maxAccessibleLevel);

		case tcu::Sampler::NEAREST_MIPMAP_LINEAR:
		case tcu::Sampler::LINEAR_MIPMAP_LINEAR:
			return computedLod;

		default:
			DE_ASSERT(false);
			return 0.0f;
	}
}

vector<float> TextureQueryLodInstance::computeQuadTexCoord (void) const
{
	vector<float>	res;
	tcu::Mat4		coordTransMat;

	{
		Vec4 s = m_maxCoord - m_minCoord;
		Vec4 b = m_minCoord;

		float baseCoordTrans[] =
		{
			s.x(),		0.0f,		0.f,	b.x(),
			0.f,		s.y(),		0.f,	b.y(),
			s.z()/2.f,	-s.z()/2.f,	0.f,	s.z()/2.f + b.z(),
			-s.w()/2.f,	s.w()/2.f,	0.f,	s.w()/2.f + b.w()
		};

		coordTransMat = tcu::Mat4(baseCoordTrans);
	}

	const int		texCoordComps	= getQueryLodFuncTextCoordComps(m_textureSpec.type);
	Vec4			coords[4]		=
	{
		coordTransMat * tcu::Vec4(0, 0, 0, 1),
		coordTransMat * tcu::Vec4(0, 1, 0, 1),
		coordTransMat * tcu::Vec4(1, 0, 0, 1),
		coordTransMat * tcu::Vec4(1, 1, 0, 1)
	};

	res.resize(4 * texCoordComps);

	for (int ndx = 0; ndx < 4; ndx++)
		deMemcpy(&res[ndx * texCoordComps], coords[ndx].getPtr(), texCoordComps * sizeof(float));

	return res;
}

class TextureQueryCase : public ShaderRenderCase
{
public:
								TextureQueryCase				(tcu::TestContext&			testCtx,
																 const std::string&			name,
																 const std::string&			desc,
																 const std::string&			samplerType,
																 const TextureSpec&			texture,
																 bool						isVertexCase,
																 QueryFunction				function);
	virtual						~TextureQueryCase				(void);

	virtual TestInstance*		createInstance					(Context& context) const;

protected:
	void						initShaderSources				(void);

	const std::string			m_samplerTypeStr;
	const TextureSpec			m_textureSpec;
	const QueryFunction			m_function;
};

TextureQueryCase::TextureQueryCase (tcu::TestContext&		testCtx,
									const std::string&		name,
									const std::string&		desc,
									const std::string&		samplerType,
									const TextureSpec&		texture,
									bool					isVertexCase,
									QueryFunction			function)
	: ShaderRenderCase	(testCtx, name, desc, isVertexCase, (ShaderEvaluator*)DE_NULL, DE_NULL, DE_NULL)
	, m_samplerTypeStr	(samplerType)
	, m_textureSpec		(texture)
	, m_function		(function)
{
	initShaderSources();
}

TextureQueryCase::~TextureQueryCase (void)
{
}

TestInstance* TextureQueryCase::createInstance (Context& context) const
{
	switch (m_function)
	{
		case QUERYFUNCTION_TEXTURESIZE:				return new TextureSizeInstance(context, m_isVertexCase, m_textureSpec);
		case QUERYFUNCTION_TEXTUREQUERYLOD:			return new TextureQueryLodInstance(context, m_isVertexCase, m_textureSpec);
		case QUERYFUNCTION_TEXTUREQUERYLEVELS:		return new TextureQueryLevelsInstance(context, m_isVertexCase, m_textureSpec);
		case QUERYFUNCTION_TEXTURESAMPLES:			return new TextureSamplesInstance(context, m_isVertexCase, m_textureSpec);
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

void TextureQueryCase::initShaderSources (void)
{
	std::ostringstream		vert;
	std::ostringstream		frag;
	std::ostringstream&		op			= m_isVertexCase ? vert : frag;
	glu::GLSLVersion		version		= glu::GLSL_VERSION_LAST;

	DE_ASSERT(m_function != QUERYFUNCTION_TEXTUREQUERYLOD || !m_isVertexCase);

	switch (m_function)
	{
		case QUERYFUNCTION_TEXTURESIZE:
			if (m_textureSpec.type == TEXTURETYPE_1D || m_textureSpec.type == TEXTURETYPE_1D_ARRAY || m_textureSpec.type == TEXTURETYPE_CUBE_ARRAY)
				version = glu::GLSL_VERSION_420;
			else
				version = glu::GLSL_VERSION_310_ES;
			break;

		case QUERYFUNCTION_TEXTUREQUERYLOD:
			version = glu::GLSL_VERSION_420;
			break;

		case QUERYFUNCTION_TEXTUREQUERYLEVELS:
			version = glu::GLSL_VERSION_430;
			break;

		case QUERYFUNCTION_TEXTURESAMPLES:
			version = glu::GLSL_VERSION_450;
			break;

		default:
			DE_ASSERT(false);
			break;
	}

	vert << glu::getGLSLVersionDeclaration(version) << "\n"
		 << "layout(location = 0) in highp vec4 a_position;\n";

	frag << glu::getGLSLVersionDeclaration(version) << "\n"
		 << "layout(location = 0) out mediump vec4 o_color;\n";

	if (m_isVertexCase)
	{
		vert << "layout(location = 0) out mediump vec4 v_color;\n";
		frag << "layout(location = 0) in mediump vec4 v_color;\n";
	}

	if (m_function == QUERYFUNCTION_TEXTUREQUERYLOD)
	{
		const int		texCoordComps	= getQueryLodFuncTextCoordComps(m_textureSpec.type);
		const char*		coordTypeName	= glu::getDataTypeName(glu::getDataTypeFloatVec(texCoordComps));

		vert << "layout (location = 1) in highp " << coordTypeName << " a_texCoord;\n";
		vert << "layout (location = 0) out highp " << coordTypeName << " v_texCoord;\n";
		frag << "layout (location = 0) in highp " << coordTypeName << " v_texCoord;\n";
	}

	// uniforms
	op << "layout(set = 0, binding = 0) uniform highp " << m_samplerTypeStr << " u_sampler;\n";
	if (m_function == QUERYFUNCTION_TEXTURESIZE)
		op << "layout(set = 0, binding = 1) uniform buf0 { highp int u_lod; };\n";

	if (version != glu::GLSL_VERSION_310_ES)
		vert << "out gl_PerVertex {\n"
			 << "\tvec4 gl_Position;\n"
			 << "};\n";

	vert << "\nvoid main()\n{\n"
		 << "\tgl_Position = a_position;\n";
	frag << "\nvoid main()\n{\n";

	if (m_isVertexCase)
		vert << "\tv_color = ";
	else
		frag << "\to_color = ";

	// op
	{
		op << "vec4(";

		switch (m_function)
		{
			case QUERYFUNCTION_TEXTURESIZE:
			{
				const int		resultComponents	= glu::getDataTypeScalarSize(getTextureSizeFuncResultType(m_textureSpec.type));

				op << "textureSize(u_sampler, u_lod)";
				for (int ndx = 0; ndx < 3 - resultComponents; ndx++)
					op << ", 0.0";
				op << ", 1.0";

				break;
			}

			case QUERYFUNCTION_TEXTUREQUERYLOD:
				op << "textureQueryLod(u_sampler, v_texCoord), 0.0, 1.0";
				break;

			case QUERYFUNCTION_TEXTUREQUERYLEVELS:
				op << "textureQueryLevels(u_sampler), 0.0, 0.0, 1.0";
				break;

			case QUERYFUNCTION_TEXTURESAMPLES:
				op << "textureSamples(u_sampler), 0.0, 0.0, 1.0";
				break;

			default:
				DE_ASSERT(false);
				break;
		}

		op << ");\n";
	}

	if (m_isVertexCase)
		frag << "\to_color = v_color;\n";

	if (m_function == QUERYFUNCTION_TEXTUREQUERYLOD)
		vert << "\tv_texCoord = a_texCoord;\n";

	vert << "}\n";
	frag << "}\n";

	m_vertShaderSource = vert.str();
	m_fragShaderSource = frag.str();
}

class ShaderTextureFunctionTests : public tcu::TestCaseGroup
{
public:
									ShaderTextureFunctionTests		(tcu::TestContext& context);
	virtual							~ShaderTextureFunctionTests		(void);
	virtual void					init							(void);

private:
									ShaderTextureFunctionTests		(const ShaderTextureFunctionTests&);		// not allowed!
	ShaderTextureFunctionTests&		operator=						(const ShaderTextureFunctionTests&);		// not allowed!
};

ShaderTextureFunctionTests::ShaderTextureFunctionTests (tcu::TestContext& context)
	: TestCaseGroup(context, "texture_functions", "Texture Access Function Tests")
{
}

ShaderTextureFunctionTests::~ShaderTextureFunctionTests (void)
{
}

enum CaseFlags
{
	VERTEX		= (1<<0),
	FRAGMENT	= (1<<1),
	BOTH		= VERTEX|FRAGMENT
};

struct TexFuncCaseSpec
{
	const char*			name;
	TextureLookupSpec	lookupSpec;
	TextureSpec			texSpec;
	TexEvalFunc			evalFunc;
	deUint32			flags;
};

#define CASE_SPEC(NAME, FUNC, MINCOORD, MAXCOORD, USEBIAS, MINLOD, MAXLOD, USEOFFSET, OFFSET, TEXSPEC, EVALFUNC, FLAGS) \
	{ #NAME, TextureLookupSpec(FUNC, MINCOORD, MAXCOORD, USEBIAS, MINLOD, MAXLOD, tcu::Vec3(0.0f), tcu::Vec3(0.0f), tcu::Vec3(0.0f), tcu::Vec3(0.0f), USEOFFSET, OFFSET), TEXSPEC, EVALFUNC, FLAGS }
#define GRAD_CASE_SPEC(NAME, FUNC, MINCOORD, MAXCOORD, MINDX, MAXDX, MINDY, MAXDY, USEOFFSET, OFFSET, TEXSPEC, EVALFUNC, FLAGS) \
	{ #NAME, TextureLookupSpec(FUNC, MINCOORD, MAXCOORD, false, 0.0f, 0.0f, MINDX, MAXDX, MINDY, MAXDY, USEOFFSET, OFFSET), TEXSPEC, EVALFUNC, FLAGS }

class SparseShaderTextureFunctionInstance : public ShaderTextureFunctionInstance
{
public:
				SparseShaderTextureFunctionInstance		(Context&					context,
														const bool					isVertexCase,
														const ShaderEvaluator&		evaluator,
														const UniformSetup&			uniformSetup,
														const TextureLookupSpec&	lookupSpec,
														const TextureSpec&			textureSpec,
														const TexLookupParams&		lookupParams,
														const ImageBackingMode		imageBackingMode = IMAGE_BACKING_MODE_SPARSE);
	virtual		~SparseShaderTextureFunctionInstance	(void);
};

SparseShaderTextureFunctionInstance::SparseShaderTextureFunctionInstance (Context&					context,
																		 const bool					isVertexCase,
																		 const ShaderEvaluator&		evaluator,
																		 const UniformSetup&		uniformSetup,
																		 const TextureLookupSpec&	lookupSpec,
																		 const TextureSpec&			textureSpec,
																		 const TexLookupParams&		lookupParams,
																		 const ImageBackingMode		imageBackingMode)
	: ShaderTextureFunctionInstance (context, isVertexCase, evaluator, uniformSetup, lookupSpec, textureSpec, lookupParams, imageBackingMode)
{
}

SparseShaderTextureFunctionInstance::~SparseShaderTextureFunctionInstance (void)
{
}

class SparseShaderTextureFunctionCase : public ShaderTextureFunctionCase
{
public:
							SparseShaderTextureFunctionCase		(tcu::TestContext&			testCtx,
																const std::string&			name,
																const std::string&			desc,
																const TextureLookupSpec&	lookup,
																const TextureSpec&			texture,
																TexEvalFunc					evalFunc,
																bool						isVertexCase);

	virtual					~SparseShaderTextureFunctionCase	(void);

	virtual	TestInstance*	createInstance						(Context& context) const;
protected:
	void					initShaderSources					(void);
};

SparseShaderTextureFunctionCase::SparseShaderTextureFunctionCase (tcu::TestContext&				testCtx,
																  const std::string&			name,
																  const std::string&			desc,
																  const TextureLookupSpec&		lookup,
																  const TextureSpec&			texture,
																  TexEvalFunc					evalFunc,
																  bool							isVertexCase)
	: ShaderTextureFunctionCase		(testCtx, name, desc, lookup, texture, evalFunc, isVertexCase)
{
	initShaderSources();
}

void SparseShaderTextureFunctionCase::initShaderSources (void)
{
	const Function				function			= m_lookupSpec.function;
	const bool					isVtxCase			= m_isVertexCase;
	const bool					isProj				= functionHasProj(function);
	const bool					isGrad				= functionHasGrad(function);
	const bool					isShadow			= m_textureSpec.sampler.compare != tcu::Sampler::COMPAREMODE_NONE;
	const bool					is2DProj4			= !isShadow && m_textureSpec.type == TEXTURETYPE_2D && (function == FUNCTION_TEXTUREPROJ || function == FUNCTION_TEXTUREPROJLOD || function == FUNCTION_TEXTUREPROJGRAD);
	const bool					isIntCoord			= function == FUNCTION_TEXELFETCH;
	const bool					hasLodBias			= functionHasLod(m_lookupSpec.function) || m_lookupSpec.useBias;
	const int					texCoordComps		= m_textureSpec.type == TEXTURETYPE_2D ? 2 : 3;
	const int					extraCoordComps		= (isProj ? (is2DProj4 ? 2 : 1) : 0) + (isShadow ? 1 : 0);
	const glu::DataType			coordType			= glu::getDataTypeFloatVec(texCoordComps+extraCoordComps);
	const glu::Precision		coordPrec			= glu::PRECISION_HIGHP;
	const char*					coordTypeName		= glu::getDataTypeName(coordType);
	const char*					coordPrecName		= glu::getPrecisionName(coordPrec);
	const tcu::TextureFormat	texFmt				= glu::mapGLInternalFormat(m_textureSpec.format);
	glu::DataType				samplerType			= glu::TYPE_LAST;
	const glu::DataType			gradType			= (m_textureSpec.type == TEXTURETYPE_CUBE_MAP || m_textureSpec.type == TEXTURETYPE_3D) ? glu::TYPE_FLOAT_VEC3 : glu::TYPE_FLOAT_VEC2;
	const char*					gradTypeName		= glu::getDataTypeName(gradType);
	const char*					baseFuncName		= DE_NULL;

	DE_ASSERT(!isGrad || !hasLodBias);

	switch (m_textureSpec.type)
	{
		case TEXTURETYPE_2D:		samplerType = isShadow ? glu::TYPE_SAMPLER_2D_SHADOW		: glu::getSampler2DType(texFmt);		break;
		case TEXTURETYPE_CUBE_MAP:	samplerType = isShadow ? glu::TYPE_SAMPLER_CUBE_SHADOW		: glu::getSamplerCubeType(texFmt);		break;
		case TEXTURETYPE_2D_ARRAY:	samplerType = isShadow ? glu::TYPE_SAMPLER_2D_ARRAY_SHADOW	: glu::getSampler2DArrayType(texFmt);	break;
		case TEXTURETYPE_3D:		DE_ASSERT(!isShadow); samplerType = glu::getSampler3DType(texFmt);									break;
		default:
			DE_ASSERT(DE_FALSE);
	}

	// Not supported cases
	switch (m_lookupSpec.function)
	{
		case FUNCTION_TEXTURE:			baseFuncName = "sparseTexture";			break;
		case FUNCTION_TEXTURELOD:		baseFuncName = "sparseTextureLod";		break;
		case FUNCTION_TEXTUREGRAD:		baseFuncName = "sparseTextureGrad";		break;
		case FUNCTION_TEXELFETCH:		baseFuncName = "sparseTexelFetch";		break;
		default:
			DE_ASSERT(DE_FALSE);
	}

	std::ostringstream	vert;
	std::ostringstream	frag;
	std::ostringstream&	op		= isVtxCase ? vert : frag;

	vert << "#version 450\n"
		 << "#extension GL_ARB_sparse_texture2 : require\n"
		 << "layout(location = 0) in highp vec4 a_position;\n"
		 << "layout(location = 4) in " << coordPrecName << " " << coordTypeName << " a_in0;\n";

	if (isGrad)
	{
		vert << "layout(location = 5) in " << coordPrecName << " " << gradTypeName << " a_in1;\n";
		vert << "layout(location = 6) in " << coordPrecName << " " << gradTypeName << " a_in2;\n";
	}
	else if (hasLodBias)
		vert << "layout(location = 5) in " << coordPrecName << " float a_in1;\n";

	frag << "#version 450\n"
		 << "#extension GL_ARB_sparse_texture2 : require\n"
		 << "layout(location = 0) out mediump vec4 o_color;\n";

	if (isVtxCase)
	{
		vert << "layout(location = 0) out mediump vec4 v_color;\n";
		frag << "layout(location = 0) in mediump vec4 v_color;\n";
	}
	else
	{
		vert << "layout(location = 0) out " << coordPrecName << " " << coordTypeName << " v_texCoord;\n";
		frag << "layout(location = 0) in " << coordPrecName << " " << coordTypeName << " v_texCoord;\n";

		if (isGrad)
		{
			vert << "layout(location = 1) out " << coordPrecName << " " << gradTypeName << " v_gradX;\n";
			vert << "layout(location = 2) out " << coordPrecName << " " << gradTypeName << " v_gradY;\n";
			frag << "layout(location = 1) in " << coordPrecName << " " << gradTypeName << " v_gradX;\n";
			frag << "layout(location = 2) in " << coordPrecName << " " << gradTypeName << " v_gradY;\n";
		}
		else if (hasLodBias)
		{
			vert << "layout(location = 1) out " << coordPrecName << " float v_lodBias;\n";
			frag << "layout(location = 1) in " << coordPrecName << " float v_lodBias;\n";
		}
	}

	// Uniforms
	op << "layout(set = 0, binding = 0) uniform highp " << glu::getDataTypeName(samplerType) << " u_sampler;\n"
	   << "layout(set = 0, binding = 1) uniform buf0 { highp vec4 u_scale; };\n"
	   << "layout(set = 0, binding = 2) uniform buf1 { highp vec4 u_bias; };\n";

	vert << "out gl_PerVertex {\n"
		 << "	vec4 gl_Position;\n"
		 << "};\n";
	vert << "\nvoid main()\n{\n"
		 << "\tgl_Position = a_position;\n";
	frag << "\nvoid main()\n{\n";

	// Op.
	{
		// Texel declaration
		if (isShadow)
			op << "\tfloat texel;\n";
		else
			op << "\tvec4 texel;\n";

		const char*	const texCoord	= isVtxCase ? "a_in0" : "v_texCoord";
		const char* const gradX		= isVtxCase ? "a_in1" : "v_gradX";
		const char* const gradY		= isVtxCase ? "a_in2" : "v_gradY";
		const char*	const lodBias	= isVtxCase ? "a_in1" : "v_lodBias";

		op << "\tint success = " << baseFuncName;

		if (m_lookupSpec.useOffset)
			op << "Offset";

		op << "ARB(u_sampler, ";

		if (isIntCoord)
			op << "ivec" << (texCoordComps+extraCoordComps) << "(";

		op << texCoord;

		if (isIntCoord)
			op << ")";

		if (isGrad)
			op << ", " << gradX << ", " << gradY;

		if (functionHasLod(function))
		{
			if (isIntCoord)
				op << ", int(" << lodBias << ")";
			else
				op << ", " << lodBias;
		}

		if (m_lookupSpec.useOffset)
		{
			int offsetComps = m_textureSpec.type == TEXTURETYPE_3D ? 3 : 2;

			op << ", ivec" << offsetComps << "(";
			for (int ndx = 0; ndx < offsetComps; ndx++)
			{
				if (ndx != 0)
					op << ", ";
				op << m_lookupSpec.offset[ndx];
			}
			op << ")";
		}

		op << ", texel";

		if (m_lookupSpec.useBias)
			op << ", " << lodBias;

		op << ");\n";

		// Check sparse validity, and handle each case
		op << "\tif (sparseTexelsResidentARB(success))\n";

		if (isVtxCase)
			vert << "\t\tv_color = ";
		else
			frag << "\t\to_color = ";

		if (isShadow)
			op << "vec4(texel, 0.0, 0.0, 1.0);\n";
		else
			op << "vec4(texel * u_scale + u_bias);\n";

		op << "\telse\n";

		// This color differs from the used colors
		if (isVtxCase)
			vert << "\t\tv_color = vec4(0.54117647058, 0.16862745098, 0.8862745098, 1.0);\n";
		else
			frag << "\t\to_color = vec4(0.54117647058, 0.16862745098, 0.8862745098, 1.0);\n";
	}

	if (isVtxCase)
		frag << "\to_color = v_color;\n";
	else
	{
		vert << "\tv_texCoord = a_in0;\n";

		if (isGrad)
		{
			vert << "\tv_gradX = a_in1;\n";
			vert << "\tv_gradY = a_in2;\n";
		}
		else if (hasLodBias)
			vert << "\tv_lodBias = a_in1;\n";
	}

	vert << "}\n";
	frag << "}\n";

	m_vertShaderSource = vert.str();
	m_fragShaderSource = frag.str();
}

SparseShaderTextureFunctionCase::~SparseShaderTextureFunctionCase ()
{
}

TestInstance* SparseShaderTextureFunctionCase::createInstance (Context& context) const
{
	DE_ASSERT(m_evaluator != DE_NULL);
	DE_ASSERT(m_uniformSetup != DE_NULL);
	return new SparseShaderTextureFunctionInstance(context, m_isVertexCase, *m_evaluator, *m_uniformSetup, m_lookupSpec, m_textureSpec, m_lookupParams);
}

static void createCaseGroup (tcu::TestCaseGroup* parent, const char* groupName, const char* groupDesc, const TexFuncCaseSpec* cases, int numCases)
{
	de::MovePtr<tcu::TestCaseGroup>	group	(new tcu::TestCaseGroup(parent->getTestContext(), groupName, groupDesc));

	for (int ndx = 0; ndx < numCases; ndx++)
	{
		std::string	name			= cases[ndx].name;
		bool		sparseSupported	= !functionHasProj(cases[ndx].lookupSpec.function)		&&
									  TEXTURETYPE_1D			!= cases[ndx].texSpec.type	&&
									  TEXTURETYPE_1D_ARRAY		!= cases[ndx].texSpec.type	&&
									  TEXTURETYPE_CUBE_ARRAY	!= cases[ndx].texSpec.type;

		if (cases[ndx].flags & VERTEX)
		{
			if (sparseSupported)
				group->addChild(new SparseShaderTextureFunctionCase(parent->getTestContext(), ("sparse_" + name + "_vertex"),   "", cases[ndx].lookupSpec, cases[ndx].texSpec, cases[ndx].evalFunc, true ));

			group->addChild(new ShaderTextureFunctionCase(parent->getTestContext(), (name + "_vertex"),   "", cases[ndx].lookupSpec, cases[ndx].texSpec, cases[ndx].evalFunc, true ));
		}

		if (cases[ndx].flags & FRAGMENT)
		{
			if (sparseSupported)
				group->addChild(new SparseShaderTextureFunctionCase(parent->getTestContext(), ("sparse_" + name + "_fragment"), "", cases[ndx].lookupSpec, cases[ndx].texSpec, cases[ndx].evalFunc, false));

			group->addChild(new ShaderTextureFunctionCase(parent->getTestContext(), (name + "_fragment"), "", cases[ndx].lookupSpec, cases[ndx].texSpec, cases[ndx].evalFunc, false));
		}
	}

	parent->addChild(group.release());
}

void ShaderTextureFunctionTests::init (void)
{
	// Samplers
	static const tcu::Sampler	samplerNearestNoMipmap	(tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL,
														 tcu::Sampler::NEAREST, tcu::Sampler::NEAREST,
														 0.0f /* LOD threshold */, true /* normalized coords */, tcu::Sampler::COMPAREMODE_NONE,
														 0 /* cmp channel */, tcu::Vec4(0.0f) /* border color */, true /* seamless cube map */);
	static const tcu::Sampler	samplerLinearNoMipmap	(tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL,
														 tcu::Sampler::LINEAR, tcu::Sampler::LINEAR,
														 0.0f /* LOD threshold */, true /* normalized coords */, tcu::Sampler::COMPAREMODE_NONE,
														 0 /* cmp channel */, tcu::Vec4(0.0f) /* border color */, true /* seamless cube map */);
	static const tcu::Sampler	samplerNearestMipmap	(tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL,
														 tcu::Sampler::NEAREST_MIPMAP_NEAREST, tcu::Sampler::NEAREST,
														 0.0f /* LOD threshold */, true /* normalized coords */, tcu::Sampler::COMPAREMODE_NONE,
														 0 /* cmp channel */, tcu::Vec4(0.0f) /* border color */, true /* seamless cube map */);
	static const tcu::Sampler	samplerLinearMipmap		(tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL,
														 tcu::Sampler::LINEAR_MIPMAP_NEAREST, tcu::Sampler::LINEAR,
														 0.0f /* LOD threshold */, true /* normalized coords */, tcu::Sampler::COMPAREMODE_NONE,
														 0 /* cmp channel */, tcu::Vec4(0.0f) /* border color */, true /* seamless cube map */);

	static const tcu::Sampler	samplerShadowNoMipmap	(tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL,
														 tcu::Sampler::NEAREST, tcu::Sampler::NEAREST,
														 0.0f /* LOD threshold */, true /* normalized coords */, tcu::Sampler::COMPAREMODE_LESS,
														 0 /* cmp channel */, tcu::Vec4(0.0f) /* border color */, true /* seamless cube map */);
	static const tcu::Sampler	samplerShadowMipmap		(tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL,
														 tcu::Sampler::NEAREST_MIPMAP_NEAREST, tcu::Sampler::NEAREST,
														 0.0f /* LOD threshold */, true /* normalized coords */, tcu::Sampler::COMPAREMODE_LESS,
														 0 /* cmp channel */, tcu::Vec4(0.0f) /* border color */, true /* seamless cube map */);

	static const tcu::Sampler	samplerTexelFetch		(tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL,
														 tcu::Sampler::NEAREST_MIPMAP_NEAREST, tcu::Sampler::NEAREST,
														 00.0f /* LOD threshold */, true /* normalized coords */, tcu::Sampler::COMPAREMODE_NONE,
														 0 /* cmp channel */, tcu::Vec4(0.0f) /* border color */, true /* seamless cube map */);

	// Default textures.
	//												Type					Format					W		H		D	L	Sampler
	static const TextureSpec tex2DFixed				(TEXTURETYPE_2D,		GL_RGBA8,				256,	256,	1,	1,	samplerLinearNoMipmap);
	static const TextureSpec tex2DFloat				(TEXTURETYPE_2D,		GL_RGBA16F,				256,	256,	1,	1,	samplerLinearNoMipmap);
	static const TextureSpec tex2DInt				(TEXTURETYPE_2D,		GL_RGBA8I,				256,	256,	1,	1,	samplerNearestNoMipmap);
	static const TextureSpec tex2DUint				(TEXTURETYPE_2D,		GL_RGBA8UI,				256,	256,	1,	1,	samplerNearestNoMipmap);
	static const TextureSpec tex2DMipmapFixed		(TEXTURETYPE_2D,		GL_RGBA8,				256,	256,	1,	9,	samplerLinearMipmap);
	static const TextureSpec tex2DMipmapFloat		(TEXTURETYPE_2D,		GL_RGBA16F,				256,	256,	1,	9,	samplerLinearMipmap);
	static const TextureSpec tex2DMipmapInt			(TEXTURETYPE_2D,		GL_RGBA8I,				256,	256,	1,	9,	samplerNearestMipmap);
	static const TextureSpec tex2DMipmapUint		(TEXTURETYPE_2D,		GL_RGBA8UI,				256,	256,	1,	9,	samplerNearestMipmap);

	static const TextureSpec tex2DShadow			(TEXTURETYPE_2D,		GL_DEPTH_COMPONENT16,	256,	256,	1,	1,	samplerShadowNoMipmap);
	static const TextureSpec tex2DMipmapShadow		(TEXTURETYPE_2D,		GL_DEPTH_COMPONENT16,	256,	256,	1,	9,	samplerShadowMipmap);

	static const TextureSpec tex2DTexelFetchFixed	(TEXTURETYPE_2D,		GL_RGBA8,				256,	256,	1,	9,	samplerTexelFetch);
	static const TextureSpec tex2DTexelFetchFloat	(TEXTURETYPE_2D,		GL_RGBA16F,				256,	256,	1,	9,	samplerTexelFetch);
	static const TextureSpec tex2DTexelFetchInt		(TEXTURETYPE_2D,		GL_RGBA8I,				256,	256,	1,	9,	samplerTexelFetch);
	static const TextureSpec tex2DTexelFetchUint	(TEXTURETYPE_2D,		GL_RGBA8UI,				256,	256,	1,	9,	samplerTexelFetch);

	static const TextureSpec texCubeFixed			(TEXTURETYPE_CUBE_MAP,	GL_RGBA8,	256,	256,	1,	1,	samplerLinearNoMipmap);
	static const TextureSpec texCubeFloat			(TEXTURETYPE_CUBE_MAP,	GL_RGBA16F,	256,	256,	1,	1,	samplerLinearNoMipmap);
	static const TextureSpec texCubeInt				(TEXTURETYPE_CUBE_MAP,	GL_RGBA8I,	256,	256,	1,	1,	samplerNearestNoMipmap);
	static const TextureSpec texCubeUint			(TEXTURETYPE_CUBE_MAP,	GL_RGBA8UI,	256,	256,	1,	1,	samplerNearestNoMipmap);
	static const TextureSpec texCubeMipmapFixed		(TEXTURETYPE_CUBE_MAP,	GL_RGBA8,	256,	256,	1,	9,	samplerLinearMipmap);
	static const TextureSpec texCubeMipmapFloat		(TEXTURETYPE_CUBE_MAP,	GL_RGBA16F,	128,	128,	1,	8,	samplerLinearMipmap);
	static const TextureSpec texCubeMipmapInt		(TEXTURETYPE_CUBE_MAP,	GL_RGBA8I,	256,	256,	1,	9,	samplerNearestMipmap);
	static const TextureSpec texCubeMipmapUint		(TEXTURETYPE_CUBE_MAP,	GL_RGBA8UI,	256,	256,	1,	9,	samplerNearestMipmap);

	static const TextureSpec texCubeShadow			(TEXTURETYPE_CUBE_MAP,	GL_DEPTH_COMPONENT16,	256,	256,	1,	1,	samplerShadowNoMipmap);
	static const TextureSpec texCubeMipmapShadow	(TEXTURETYPE_CUBE_MAP,	GL_DEPTH_COMPONENT16,	256,	256,	1,	9,	samplerShadowMipmap);

	static const TextureSpec tex2DArrayFixed		(TEXTURETYPE_2D_ARRAY,	GL_RGBA8,	128,	128,	4,	1,	samplerLinearNoMipmap);
	static const TextureSpec tex2DArrayFloat		(TEXTURETYPE_2D_ARRAY,	GL_RGBA16F,	128,	128,	4,	1,	samplerLinearNoMipmap);
	static const TextureSpec tex2DArrayInt			(TEXTURETYPE_2D_ARRAY,	GL_RGBA8I,	128,	128,	4,	1,	samplerNearestNoMipmap);
	static const TextureSpec tex2DArrayUint			(TEXTURETYPE_2D_ARRAY,	GL_RGBA8UI,	128,	128,	4,	1,	samplerNearestNoMipmap);
	static const TextureSpec tex2DArrayMipmapFixed	(TEXTURETYPE_2D_ARRAY,	GL_RGBA8,	128,	128,	4,	8,	samplerLinearMipmap);
	static const TextureSpec tex2DArrayMipmapFloat	(TEXTURETYPE_2D_ARRAY,	GL_RGBA16F,	128,	128,	4,	8,	samplerLinearMipmap);
	static const TextureSpec tex2DArrayMipmapInt	(TEXTURETYPE_2D_ARRAY,	GL_RGBA8I,	128,	128,	4,	8,	samplerNearestMipmap);
	static const TextureSpec tex2DArrayMipmapUint	(TEXTURETYPE_2D_ARRAY,	GL_RGBA8UI,	128,	128,	4,	8,	samplerNearestMipmap);

	static const TextureSpec tex2DArrayShadow		(TEXTURETYPE_2D_ARRAY,	GL_DEPTH_COMPONENT16,	128,	128,	4,	1,	samplerShadowNoMipmap);
	static const TextureSpec tex2DArrayMipmapShadow	(TEXTURETYPE_2D_ARRAY,	GL_DEPTH_COMPONENT16,	128,	128,	4,	8,	samplerShadowMipmap);

	static const TextureSpec tex2DArrayTexelFetchFixed	(TEXTURETYPE_2D_ARRAY,	GL_RGBA8,	128,	128,	4,	8,	samplerTexelFetch);
	static const TextureSpec tex2DArrayTexelFetchFloat	(TEXTURETYPE_2D_ARRAY,	GL_RGBA16F,	128,	128,	4,	8,	samplerTexelFetch);
	static const TextureSpec tex2DArrayTexelFetchInt	(TEXTURETYPE_2D_ARRAY,	GL_RGBA8I,	128,	128,	4,	8,	samplerTexelFetch);
	static const TextureSpec tex2DArrayTexelFetchUint	(TEXTURETYPE_2D_ARRAY,	GL_RGBA8UI,	128,	128,	4,	8,	samplerTexelFetch);

	static const TextureSpec tex3DFixed				(TEXTURETYPE_3D,		GL_RGBA8,	64,		32,		32,	1,	samplerLinearNoMipmap);
	static const TextureSpec tex3DFloat				(TEXTURETYPE_3D,		GL_RGBA16F,	64,		32,		32,	1,	samplerLinearNoMipmap);
	static const TextureSpec tex3DInt				(TEXTURETYPE_3D,		GL_RGBA8I,	64,		32,		32,	1,	samplerNearestNoMipmap);
	static const TextureSpec tex3DUint				(TEXTURETYPE_3D,		GL_RGBA8UI,	64,		32,		32,	1,	samplerNearestNoMipmap);
	static const TextureSpec tex3DMipmapFixed		(TEXTURETYPE_3D,		GL_RGBA8,	64,		32,		32,	7,	samplerLinearMipmap);
	static const TextureSpec tex3DMipmapFloat		(TEXTURETYPE_3D,		GL_RGBA16F,	64,		32,		32,	7,	samplerLinearMipmap);
	static const TextureSpec tex3DMipmapInt			(TEXTURETYPE_3D,		GL_RGBA8I,	64,		32,		32,	7,	samplerNearestMipmap);
	static const TextureSpec tex3DMipmapUint		(TEXTURETYPE_3D,		GL_RGBA8UI,	64,		32,		32,	7,	samplerNearestMipmap);

	static const TextureSpec tex3DTexelFetchFixed	(TEXTURETYPE_3D,		GL_RGBA8,	64,		32,		32,	7,	samplerTexelFetch);
	static const TextureSpec tex3DTexelFetchFloat	(TEXTURETYPE_3D,		GL_RGBA16F,	64,		32,		32,	7,	samplerTexelFetch);
	static const TextureSpec tex3DTexelFetchInt		(TEXTURETYPE_3D,		GL_RGBA8I,	64,		32,		32,	7,	samplerTexelFetch);
	static const TextureSpec tex3DTexelFetchUint	(TEXTURETYPE_3D,		GL_RGBA8UI,	64,		32,		32,	7,	samplerTexelFetch);

	static const TextureSpec tex1DFixed				(TEXTURETYPE_1D,		GL_RGBA8,				256,	1,	1,	1,	samplerLinearNoMipmap);
	static const TextureSpec tex1DFloat				(TEXTURETYPE_1D,		GL_RGBA16F,				256,	1,	1,	1,	samplerLinearNoMipmap);
	static const TextureSpec tex1DInt				(TEXTURETYPE_1D,		GL_RGBA8I,				256,	1,	1,	1,	samplerNearestNoMipmap);
	static const TextureSpec tex1DUint				(TEXTURETYPE_1D,		GL_RGBA8UI,				256,	1,	1,	1,	samplerNearestNoMipmap);
	static const TextureSpec tex1DMipmapFixed		(TEXTURETYPE_1D,		GL_RGBA8,				256,	1,	1,	9,	samplerLinearMipmap);
	static const TextureSpec tex1DMipmapFloat		(TEXTURETYPE_1D,		GL_RGBA16F,				256,	1,	1,	9,	samplerLinearMipmap);
	static const TextureSpec tex1DMipmapInt			(TEXTURETYPE_1D,		GL_RGBA8I,				256,	1,	1,	9,	samplerNearestMipmap);
	static const TextureSpec tex1DMipmapUint		(TEXTURETYPE_1D,		GL_RGBA8UI,				256,	1,	1,	9,	samplerNearestMipmap);

	static const TextureSpec tex1DShadow			(TEXTURETYPE_1D,		GL_DEPTH_COMPONENT16,	256,	1,	1,	1,	samplerShadowNoMipmap);
	static const TextureSpec tex1DMipmapShadow		(TEXTURETYPE_1D,		GL_DEPTH_COMPONENT16,	256,	1,	1,	9,	samplerShadowMipmap);

	static const TextureSpec tex1DTexelFetchFixed	(TEXTURETYPE_1D,		GL_RGBA8,				256,	1,	1,	9,	samplerTexelFetch);
	static const TextureSpec tex1DTexelFetchFloat	(TEXTURETYPE_1D,		GL_RGBA16F,				256,	1,	1,	9,	samplerTexelFetch);
	static const TextureSpec tex1DTexelFetchInt		(TEXTURETYPE_1D,		GL_RGBA8I,				256,	1,	1,	9,	samplerTexelFetch);
	static const TextureSpec tex1DTexelFetchUint	(TEXTURETYPE_1D,		GL_RGBA8UI,				256,	1,	1,	9,	samplerTexelFetch);

	static const TextureSpec tex1DArrayFixed		(TEXTURETYPE_1D_ARRAY,	GL_RGBA8,	256,	1,	4,	1,	samplerLinearNoMipmap);
	static const TextureSpec tex1DArrayFloat		(TEXTURETYPE_1D_ARRAY,	GL_RGBA16F,	256,	1,	4,	1,	samplerLinearNoMipmap);
	static const TextureSpec tex1DArrayInt			(TEXTURETYPE_1D_ARRAY,	GL_RGBA8I,	256,	1,	4,	1,	samplerNearestNoMipmap);
	static const TextureSpec tex1DArrayUint			(TEXTURETYPE_1D_ARRAY,	GL_RGBA8UI,	256,	1,	4,	1,	samplerNearestNoMipmap);
	static const TextureSpec tex1DArrayMipmapFixed	(TEXTURETYPE_1D_ARRAY,	GL_RGBA8,	256,	1,	4,	9,	samplerLinearMipmap);
	static const TextureSpec tex1DArrayMipmapFloat	(TEXTURETYPE_1D_ARRAY,	GL_RGBA16F,	256,	1,	4,	9,	samplerLinearMipmap);
	static const TextureSpec tex1DArrayMipmapInt	(TEXTURETYPE_1D_ARRAY,	GL_RGBA8I,	256,	1,	4,	9,	samplerNearestMipmap);
	static const TextureSpec tex1DArrayMipmapUint	(TEXTURETYPE_1D_ARRAY,	GL_RGBA8UI,	256,	1,	4,	9,	samplerNearestMipmap);

	static const TextureSpec tex1DArrayShadow		(TEXTURETYPE_1D_ARRAY,	GL_DEPTH_COMPONENT16,	256,	1,	4,	1,	samplerShadowNoMipmap);
	static const TextureSpec tex1DArrayMipmapShadow	(TEXTURETYPE_1D_ARRAY,	GL_DEPTH_COMPONENT16,	256,	1,	4,	9,	samplerShadowMipmap);

	static const TextureSpec tex1DArrayTexelFetchFixed	(TEXTURETYPE_1D_ARRAY,	GL_RGBA8,	256,	1,	4,	9,	samplerTexelFetch);
	static const TextureSpec tex1DArrayTexelFetchFloat	(TEXTURETYPE_1D_ARRAY,	GL_RGBA16F,	256,	1,	4,	9,	samplerTexelFetch);
	static const TextureSpec tex1DArrayTexelFetchInt	(TEXTURETYPE_1D_ARRAY,	GL_RGBA8I,	256,	1,	4,	9,	samplerTexelFetch);
	static const TextureSpec tex1DArrayTexelFetchUint	(TEXTURETYPE_1D_ARRAY,	GL_RGBA8UI,	256,	1,	4,	9,	samplerTexelFetch);

	static const TextureSpec texCubeArrayFixed			(TEXTURETYPE_CUBE_ARRAY,	GL_RGBA8,	64,		64,		12,	1,	samplerLinearNoMipmap);
	static const TextureSpec texCubeArrayFloat			(TEXTURETYPE_CUBE_ARRAY,	GL_RGBA16F,	64,		64,		12,	1,	samplerLinearNoMipmap);
	static const TextureSpec texCubeArrayInt			(TEXTURETYPE_CUBE_ARRAY,	GL_RGBA8I,	64,		64,		12,	1,	samplerNearestNoMipmap);
	static const TextureSpec texCubeArrayUint			(TEXTURETYPE_CUBE_ARRAY,	GL_RGBA8UI,	64,		64,		12,	1,	samplerNearestNoMipmap);
	static const TextureSpec texCubeArrayMipmapFixed	(TEXTURETYPE_CUBE_ARRAY,	GL_RGBA8,	64,		64,		12,	7,	samplerLinearMipmap);
	static const TextureSpec texCubeArrayMipmapFloat	(TEXTURETYPE_CUBE_ARRAY,	GL_RGBA16F,	64,		64,		12,	7,	samplerLinearMipmap);
	static const TextureSpec texCubeArrayMipmapInt		(TEXTURETYPE_CUBE_ARRAY,	GL_RGBA8I,	64,		64,		12,	7,	samplerNearestMipmap);
	static const TextureSpec texCubeArrayMipmapUint		(TEXTURETYPE_CUBE_ARRAY,	GL_RGBA8UI,	64,		64,		12,	7,	samplerNearestMipmap);

	static const TextureSpec texCubeArrayShadow			(TEXTURETYPE_CUBE_ARRAY,	GL_DEPTH_COMPONENT16,	64,		64,		12,	1,	samplerShadowNoMipmap);
	static const TextureSpec texCubeArrayMipmapShadow	(TEXTURETYPE_CUBE_ARRAY,	GL_DEPTH_COMPONENT16,	64,		64,		12,	7,	samplerShadowMipmap);

	// texture() cases
	static const TexFuncCaseSpec textureCases[] =
	{
		//		  Name							Function			MinCoord							MaxCoord							Bias?	MinLod	MaxLod	Offset?	Offset		Format					EvalFunc				Flags
		CASE_SPEC(sampler2d_fixed,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DFixed,				evalTexture2D,			VERTEX),
		CASE_SPEC(sampler2d_fixed,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DMipmapFixed,		evalTexture2D,			FRAGMENT),
		CASE_SPEC(sampler2d_float,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DFloat,				evalTexture2D,			VERTEX),
		CASE_SPEC(sampler2d_float,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DMipmapFloat,		evalTexture2D,			FRAGMENT),
		CASE_SPEC(isampler2d,					FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DInt,				evalTexture2D,			VERTEX),
		CASE_SPEC(isampler2d,					FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DMipmapInt,			evalTexture2D,			FRAGMENT),
		CASE_SPEC(usampler2d,					FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DUint,				evalTexture2D,			VERTEX),
		CASE_SPEC(usampler2d,					FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DMipmapUint,		evalTexture2D,			FRAGMENT),

		CASE_SPEC(sampler2d_bias_fixed,			FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex2DMipmapFixed,		evalTexture2DBias,		FRAGMENT),
		CASE_SPEC(sampler2d_bias_float,			FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex2DMipmapFloat,		evalTexture2DBias,		FRAGMENT),
		CASE_SPEC(isampler2d_bias,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex2DMipmapInt,			evalTexture2DBias,		FRAGMENT),
		CASE_SPEC(usampler2d_bias,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex2DMipmapUint,		evalTexture2DBias,		FRAGMENT),

		CASE_SPEC(samplercube_fixed,			FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f,  1.01f,  0.0f),	Vec4( 1.0f,  1.0f,  1.01f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeFixed,			evalTextureCube,		VERTEX),
		CASE_SPEC(samplercube_fixed,			FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f,  1.01f,  0.0f),	Vec4( 1.0f,  1.0f,  1.01f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeMipmapFixed,		evalTextureCube,		FRAGMENT),
		CASE_SPEC(samplercube_float,			FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f, -1.01f,  0.0f),	Vec4( 1.0f,  1.0f, -1.01f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeFloat,			evalTextureCube,		VERTEX),
		CASE_SPEC(samplercube_float,			FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f, -1.01f,  0.0f),	Vec4( 1.0f,  1.0f, -1.01f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeMipmapFloat,		evalTextureCube,		FRAGMENT),
		CASE_SPEC(isamplercube,					FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f,  1.01f,  0.0f),	Vec4( 1.0f,  1.0f,  1.01f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeInt,				evalTextureCube,		VERTEX),
		CASE_SPEC(isamplercube,					FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f,  1.01f,  0.0f),	Vec4( 1.0f,  1.0f,  1.01f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeMipmapInt,		evalTextureCube,		FRAGMENT),
		CASE_SPEC(usamplercube,					FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f, -1.01f,  0.0f),	Vec4( 1.0f,  1.0f, -1.01f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeUint,			evalTextureCube,		VERTEX),
		CASE_SPEC(usamplercube,					FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f, -1.01f,  0.0f),	Vec4( 1.0f,  1.0f, -1.01f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeMipmapUint,		evalTextureCube,		FRAGMENT),

		CASE_SPEC(samplercube_bias_fixed,		FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f,  1.01f,  0.0f),	Vec4( 1.0f,  1.0f,  1.01f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	texCubeMipmapFixed,		evalTextureCubeBias,	FRAGMENT),
		CASE_SPEC(samplercube_bias_float,		FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f, -1.01f,  0.0f),	Vec4( 1.0f,  1.0f, -1.01f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	texCubeMipmapFloat,		evalTextureCubeBias,	FRAGMENT),
		CASE_SPEC(isamplercube_bias,			FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f,  1.01f,  0.0f),	Vec4( 1.0f,  1.0f,  1.01f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	texCubeMipmapInt,		evalTextureCubeBias,	FRAGMENT),
		CASE_SPEC(usamplercube_bias,			FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f, -1.01f,  0.0f),	Vec4( 1.0f,  1.0f, -1.01f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	texCubeMipmapUint,		evalTextureCubeBias,	FRAGMENT),

		CASE_SPEC(sampler2darray_fixed,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DArrayFixed,		evalTexture2DArray,		VERTEX),
		CASE_SPEC(sampler2darray_fixed,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DArrayMipmapFixed,	evalTexture2DArray,		FRAGMENT),
		CASE_SPEC(sampler2darray_float,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DArrayFloat,		evalTexture2DArray,		VERTEX),
		CASE_SPEC(sampler2darray_float,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DArrayMipmapFloat,	evalTexture2DArray,		FRAGMENT),
		CASE_SPEC(isampler2darray,				FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DArrayInt,			evalTexture2DArray,		VERTEX),
		CASE_SPEC(isampler2darray,				FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DArrayMipmapInt,	evalTexture2DArray,		FRAGMENT),
		CASE_SPEC(usampler2darray,				FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DArrayUint,			evalTexture2DArray,		VERTEX),
		CASE_SPEC(usampler2darray,				FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DArrayMipmapUint,	evalTexture2DArray,		FRAGMENT),

		CASE_SPEC(sampler2darray_bias_fixed,	FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex2DArrayMipmapFixed,	evalTexture2DArrayBias,	FRAGMENT),
		CASE_SPEC(sampler2darray_bias_float,	FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex2DArrayMipmapFloat,	evalTexture2DArrayBias,	FRAGMENT),
		CASE_SPEC(isampler2darray_bias,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex2DArrayMipmapInt,	evalTexture2DArrayBias,	FRAGMENT),
		CASE_SPEC(usampler2darray_bias,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex2DArrayMipmapUint,	evalTexture2DArrayBias,	FRAGMENT),

		CASE_SPEC(sampler3d_fixed,				FUNCTION_TEXTURE,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex3DFixed,				evalTexture3D,			VERTEX),
		CASE_SPEC(sampler3d_fixed,				FUNCTION_TEXTURE,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex3DMipmapFixed,		evalTexture3D,			FRAGMENT),
		CASE_SPEC(sampler3d_float,				FUNCTION_TEXTURE,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex3DFloat,				evalTexture3D,			VERTEX),
		CASE_SPEC(sampler3d_float,				FUNCTION_TEXTURE,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex3DMipmapFloat,		evalTexture3D,			FRAGMENT),
		CASE_SPEC(isampler3d,					FUNCTION_TEXTURE,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex3DInt,				evalTexture3D,			VERTEX),
		CASE_SPEC(isampler3d,					FUNCTION_TEXTURE,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex3DMipmapInt,			evalTexture3D,			FRAGMENT),
		CASE_SPEC(usampler3d,					FUNCTION_TEXTURE,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex3DUint,				evalTexture3D,			VERTEX),
		CASE_SPEC(usampler3d,					FUNCTION_TEXTURE,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex3DMipmapUint,		evalTexture3D,			FRAGMENT),

		CASE_SPEC(sampler3d_bias_fixed,			FUNCTION_TEXTURE,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	true,	-2.0f,	1.0f,	false,	IVec3(0),	tex3DMipmapFixed,		evalTexture3DBias,		FRAGMENT),
		CASE_SPEC(sampler3d_bias_float,			FUNCTION_TEXTURE,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	true,	-2.0f,	1.0f,	false,	IVec3(0),	tex3DMipmapFloat,		evalTexture3DBias,		FRAGMENT),
		CASE_SPEC(isampler3d_bias,				FUNCTION_TEXTURE,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex3DMipmapInt,			evalTexture3DBias,		FRAGMENT),
		CASE_SPEC(usampler3d_bias,				FUNCTION_TEXTURE,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex3DMipmapUint,		evalTexture3DBias,		FRAGMENT),

		CASE_SPEC(sampler1d_fixed,				FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DFixed,				evalTexture1D,			VERTEX),
		CASE_SPEC(sampler1d_fixed,				FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DMipmapFixed,		evalTexture1D,			FRAGMENT),
		CASE_SPEC(sampler1d_float,				FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DFloat,				evalTexture1D,			VERTEX),
		CASE_SPEC(sampler1d_float,				FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DMipmapFloat,		evalTexture1D,			FRAGMENT),
		CASE_SPEC(isampler1d,					FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DInt,				evalTexture1D,			VERTEX),
		CASE_SPEC(isampler1d,					FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DMipmapInt,			evalTexture1D,			FRAGMENT),
		CASE_SPEC(usampler1d,					FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DUint,				evalTexture1D,			VERTEX),
		CASE_SPEC(usampler1d,					FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DMipmapUint,		evalTexture1D,			FRAGMENT),

		CASE_SPEC(sampler1d_bias_fixed,			FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DMipmapFixed,		evalTexture1DBias,		FRAGMENT),
		CASE_SPEC(sampler1d_bias_float,			FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DMipmapFloat,		evalTexture1DBias,		FRAGMENT),
		CASE_SPEC(isampler1d_bias,				FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DMipmapInt,			evalTexture1DBias,		FRAGMENT),
		CASE_SPEC(usampler1d_bias,				FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DMipmapUint,		evalTexture1DBias,		FRAGMENT),

		CASE_SPEC(sampler1darray_fixed,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DArrayFixed,		evalTexture1DArray,		VERTEX),
		CASE_SPEC(sampler1darray_fixed,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DArrayMipmapFixed,	evalTexture1DArray,		FRAGMENT),
		CASE_SPEC(sampler1darray_float,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DArrayFloat,		evalTexture1DArray,		VERTEX),
		CASE_SPEC(sampler1darray_float,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DArrayMipmapFloat,	evalTexture1DArray,		FRAGMENT),
		CASE_SPEC(isampler1darray,				FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DArrayInt,			evalTexture1DArray,		VERTEX),
		CASE_SPEC(isampler1darray,				FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DArrayMipmapInt,	evalTexture1DArray,		FRAGMENT),
		CASE_SPEC(usampler1darray,				FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DArrayUint,			evalTexture1DArray,		VERTEX),
		CASE_SPEC(usampler1darray,				FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DArrayMipmapUint,	evalTexture1DArray,		FRAGMENT),

		CASE_SPEC(sampler1darray_bias_fixed,	FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DArrayMipmapFixed,	evalTexture1DArrayBias,	FRAGMENT),
		CASE_SPEC(sampler1darray_bias_float,	FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DArrayMipmapFloat,	evalTexture1DArrayBias,	FRAGMENT),
		CASE_SPEC(isampler1darray_bias,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DArrayMipmapInt,	evalTexture1DArrayBias,	FRAGMENT),
		CASE_SPEC(usampler1darray_bias,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DArrayMipmapUint,	evalTexture1DArrayBias,	FRAGMENT),

		CASE_SPEC(samplercubearray_fixed,		FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f,  1.01f, -0.5f),	Vec4( 1.0f,  1.0f,  1.01f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeArrayFixed,			evalTextureCubeArray,		VERTEX),
		CASE_SPEC(samplercubearray_fixed,		FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f,  1.01f, -0.5f),	Vec4( 1.0f,  1.0f,  1.01f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeArrayMipmapFixed,	evalTextureCubeArray,		FRAGMENT),
		CASE_SPEC(samplercubearray_float,		FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f, -1.01f, -0.5f),	Vec4( 1.0f,  1.0f, -1.01f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeArrayFloat,			evalTextureCubeArray,		VERTEX),
		CASE_SPEC(samplercubearray_float,		FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f, -1.01f, -0.5f),	Vec4( 1.0f,  1.0f, -1.01f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeArrayMipmapFloat,	evalTextureCubeArray,		FRAGMENT),
		CASE_SPEC(isamplercubearray,			FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f,  1.01f, -0.5f),	Vec4( 1.0f,  1.0f,  1.01f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeArrayInt,			evalTextureCubeArray,		VERTEX),
		CASE_SPEC(isamplercubearray,			FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f,  1.01f, -0.5f),	Vec4( 1.0f,  1.0f,  1.01f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeArrayMipmapInt,		evalTextureCubeArray,		FRAGMENT),
		CASE_SPEC(usamplercubearray,			FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f, -1.01f, -0.5f),	Vec4( 1.0f,  1.0f, -1.01f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeArrayUint,			evalTextureCubeArray,		VERTEX),
		CASE_SPEC(usamplercubearray,			FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f, -1.01f, -0.5f),	Vec4( 1.0f,  1.0f, -1.01f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeArrayMipmapUint,		evalTextureCubeArray,		FRAGMENT),

		CASE_SPEC(samplercubearray_bias_fixed,	FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f,  1.01f, -0.5f),	Vec4( 1.0f,  1.0f,  1.01f,  1.5f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	texCubeArrayMipmapFixed,	evalTextureCubeArrayBias,	FRAGMENT),
		CASE_SPEC(samplercubearray_bias_float,	FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f, -1.01f, -0.5f),	Vec4( 1.0f,  1.0f, -1.01f,  1.5f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	texCubeArrayMipmapFloat,	evalTextureCubeArrayBias,	FRAGMENT),
		CASE_SPEC(isamplercubearray_bias,		FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f,  1.01f, -0.5f),	Vec4( 1.0f,  1.0f,  1.01f,  1.5f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	texCubeArrayMipmapInt,		evalTextureCubeArrayBias,	FRAGMENT),
		CASE_SPEC(usamplercubearray_bias,		FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f, -1.01f, -0.5f),	Vec4( 1.0f,  1.0f, -1.01f,  1.5f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	texCubeArrayMipmapUint,		evalTextureCubeArrayBias,	FRAGMENT),

		CASE_SPEC(sampler2dshadow,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  1.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DShadow,			evalTexture2DShadow,			VERTEX),
		CASE_SPEC(sampler2dshadow,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  1.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DMipmapShadow,		evalTexture2DShadow,			FRAGMENT),
		CASE_SPEC(sampler2dshadow_bias,			FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  1.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex2DMipmapShadow,		evalTexture2DShadowBias,		FRAGMENT),

		CASE_SPEC(samplercubeshadow,			FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f,  1.01f,  0.0f),	Vec4( 1.0f,  1.0f,  1.01f,  1.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeShadow,			evalTextureCubeShadow,			VERTEX),
		CASE_SPEC(samplercubeshadow,			FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f,  1.01f,  0.0f),	Vec4( 1.0f,  1.0f,  1.01f,  1.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeMipmapShadow,	evalTextureCubeShadow,			FRAGMENT),
		CASE_SPEC(samplercubeshadow_bias,		FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f,  1.01f,  0.0f),	Vec4( 1.0f,  1.0f,  1.01f,  1.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	texCubeMipmapShadow,	evalTextureCubeShadowBias,		FRAGMENT),

		CASE_SPEC(sampler2darrayshadow,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  1.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DArrayShadow,		evalTexture2DArrayShadow,		VERTEX),
		CASE_SPEC(sampler2darrayshadow,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  1.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DArrayMipmapShadow,	evalTexture2DArrayShadow,		FRAGMENT),

		CASE_SPEC(sampler1dshadow,				FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  1.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DShadow,			evalTexture1DShadow,			VERTEX),
		CASE_SPEC(sampler1dshadow,				FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  1.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DMipmapShadow,		evalTexture1DShadow,			FRAGMENT),
		CASE_SPEC(sampler1dshadow_bias,			FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  1.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DMipmapShadow,		evalTexture1DShadowBias,		FRAGMENT),

		CASE_SPEC(sampler1darrayshadow,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  1.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DArrayShadow,		evalTexture1DArrayShadow,		VERTEX),
		CASE_SPEC(sampler1darrayshadow,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  1.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DArrayMipmapShadow,	evalTexture1DArrayShadow,		FRAGMENT),
		CASE_SPEC(sampler1darrayshadow_bias,	FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  1.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DArrayMipmapShadow,	evalTexture1DArrayShadowBias,	FRAGMENT),

		CASE_SPEC(samplercubearrayshadow,		FUNCTION_TEXTURE,	Vec4(-1.0f, -1.0f,  1.01f, -0.5f),	Vec4( 1.0f,  1.0f,  1.01f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	texCubeArrayMipmapShadow,	evalTextureCubeArrayShadow,		FRAGMENT),
	};
	createCaseGroup(this, "texture", "texture() Tests", textureCases, DE_LENGTH_OF_ARRAY(textureCases));

	// textureOffset() cases
	// \note _bias variants are not using mipmap thanks to wide allowed range for LOD computation
	static const TexFuncCaseSpec textureOffsetCases[] =
	{
		//		  Name							Function			MinCoord							MaxCoord							Bias?	MinLod	MaxLod	Offset?	Offset				Format					EvalFunc						Flags
		CASE_SPEC(sampler2d_fixed,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DFixed,				evalTexture2DOffset,			VERTEX),
		CASE_SPEC(sampler2d_fixed,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapFixed,		evalTexture2DOffset,			FRAGMENT),
		CASE_SPEC(sampler2d_float,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DFloat,				evalTexture2DOffset,			VERTEX),
		CASE_SPEC(sampler2d_float,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapFloat,		evalTexture2DOffset,			FRAGMENT),
		CASE_SPEC(isampler2d,					FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DInt,				evalTexture2DOffset,			VERTEX),
		CASE_SPEC(isampler2d,					FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapInt,			evalTexture2DOffset,			FRAGMENT),
		CASE_SPEC(usampler2d,					FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DUint,				evalTexture2DOffset,			VERTEX),
		CASE_SPEC(usampler2d,					FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapUint,		evalTexture2DOffset,			FRAGMENT),

		CASE_SPEC(sampler2d_bias_fixed,			FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 7, 0),	tex2DFixed,				evalTexture2DOffsetBias,		FRAGMENT),
		CASE_SPEC(sampler2d_bias_float,			FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(7, -8, 0),	tex2DFloat,				evalTexture2DOffsetBias,		FRAGMENT),
		CASE_SPEC(isampler2d_bias,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 7, 0),	tex2DInt,				evalTexture2DOffsetBias,		FRAGMENT),
		CASE_SPEC(usampler2d_bias,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(7, -8, 0),	tex2DUint,				evalTexture2DOffsetBias,		FRAGMENT),

		CASE_SPEC(sampler2darray_fixed,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DArrayFixed,		evalTexture2DArrayOffset,		VERTEX),
		CASE_SPEC(sampler2darray_fixed,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DArrayMipmapFixed,	evalTexture2DArrayOffset,		FRAGMENT),
		CASE_SPEC(sampler2darray_float,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DArrayFloat,		evalTexture2DArrayOffset,		VERTEX),
		CASE_SPEC(sampler2darray_float,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DArrayMipmapFloat,	evalTexture2DArrayOffset,		FRAGMENT),
		CASE_SPEC(isampler2darray,				FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DArrayInt,			evalTexture2DArrayOffset,		VERTEX),
		CASE_SPEC(isampler2darray,				FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DArrayMipmapInt,	evalTexture2DArrayOffset,		FRAGMENT),
		CASE_SPEC(usampler2darray,				FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DArrayUint,			evalTexture2DArrayOffset,		VERTEX),
		CASE_SPEC(usampler2darray,				FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DArrayMipmapUint,	evalTexture2DArrayOffset,		FRAGMENT),

		CASE_SPEC(sampler2darray_bias_fixed,	FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 7, 0),	tex2DArrayFixed,		evalTexture2DArrayOffsetBias,	FRAGMENT),
		CASE_SPEC(sampler2darray_bias_float,	FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(7, -8, 0),	tex2DArrayFloat,		evalTexture2DArrayOffsetBias,	FRAGMENT),
		CASE_SPEC(isampler2darray_bias,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 7, 0),	tex2DArrayInt,			evalTexture2DArrayOffsetBias,	FRAGMENT),
		CASE_SPEC(usampler2darray_bias,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(7, -8, 0),	tex2DArrayUint,			evalTexture2DArrayOffsetBias,	FRAGMENT),

		CASE_SPEC(sampler3d_fixed,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 3),	tex3DFixed,				evalTexture3DOffset,			VERTEX),
		CASE_SPEC(sampler3d_fixed,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7, 3, -8),	tex3DMipmapFixed,		evalTexture3DOffset,			FRAGMENT),
		CASE_SPEC(sampler3d_float,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(3, -8, 7),	tex3DFloat,				evalTexture3DOffset,			VERTEX),
		CASE_SPEC(sampler3d_float,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 3),	tex3DMipmapFloat,		evalTexture3DOffset,			FRAGMENT),
		CASE_SPEC(isampler3d,					FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7, 3, -8),	tex3DInt,				evalTexture3DOffset,			VERTEX),
		CASE_SPEC(isampler3d,					FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(3, -8, 7),	tex3DMipmapInt,			evalTexture3DOffset,			FRAGMENT),
		CASE_SPEC(usampler3d,					FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 3),	tex3DUint,				evalTexture3DOffset,			VERTEX),
		CASE_SPEC(usampler3d,					FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7, 3, -8),	tex3DMipmapUint,		evalTexture3DOffset,			FRAGMENT),

		CASE_SPEC(sampler3d_bias_fixed,			FUNCTION_TEXTURE,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	true,	-2.0f,	1.0f,	true,	IVec3(-8, 7, 3),	tex3DFixed,				evalTexture3DOffsetBias,		FRAGMENT),
		CASE_SPEC(sampler3d_bias_float,			FUNCTION_TEXTURE,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	true,	-2.0f,	1.0f,	true,	IVec3(7, 3, -8),	tex3DFloat,				evalTexture3DOffsetBias,		FRAGMENT),
		CASE_SPEC(isampler3d_bias,				FUNCTION_TEXTURE,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(3, -8, 7),	tex3DInt,				evalTexture3DOffsetBias,		FRAGMENT),
		CASE_SPEC(usampler3d_bias,				FUNCTION_TEXTURE,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 7, 3),	tex3DUint,				evalTexture3DOffsetBias,		FRAGMENT),

		CASE_SPEC(sampler1d_fixed,				FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DFixed,				evalTexture1DOffset,			VERTEX),
		CASE_SPEC(sampler1d_fixed,				FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3( 7, 0, 0),	tex1DMipmapFixed,		evalTexture1DOffset,			FRAGMENT),
		CASE_SPEC(sampler1d_float,				FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DFloat,				evalTexture1DOffset,			VERTEX),
		CASE_SPEC(sampler1d_float,				FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3( 7, 0, 0),	tex1DMipmapFloat,		evalTexture1DOffset,			FRAGMENT),
		CASE_SPEC(isampler1d,					FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DInt,				evalTexture1DOffset,			VERTEX),
		CASE_SPEC(isampler1d,					FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3( 7, 0, 0),	tex1DMipmapInt,			evalTexture1DOffset,			FRAGMENT),
		CASE_SPEC(usampler1d,					FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DUint,				evalTexture1DOffset,			VERTEX),
		CASE_SPEC(usampler1d,					FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3( 7, 0, 0),	tex1DMipmapUint,		evalTexture1DOffset,			FRAGMENT),

		CASE_SPEC(sampler1d_bias_fixed,			FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 0, 0),	tex1DFixed,				evalTexture1DOffsetBias,		FRAGMENT),
		CASE_SPEC(sampler1d_bias_float,			FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3( 7, 0, 0),	tex1DFloat,				evalTexture1DOffsetBias,		FRAGMENT),
		CASE_SPEC(isampler1d_bias,				FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 0, 0),	tex1DInt,				evalTexture1DOffsetBias,		FRAGMENT),
		CASE_SPEC(usampler1d_bias,				FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3( 7, 0, 0),	tex1DUint,				evalTexture1DOffsetBias,		FRAGMENT),

		CASE_SPEC(sampler1darray_fixed,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DArrayFixed,		evalTexture1DArrayOffset,		VERTEX),
		CASE_SPEC(sampler1darray_fixed,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3( 7, 0, 0),	tex1DArrayMipmapFixed,	evalTexture1DArrayOffset,		FRAGMENT),
		CASE_SPEC(sampler1darray_float,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DArrayFloat,		evalTexture1DArrayOffset,		VERTEX),
		CASE_SPEC(sampler1darray_float,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3( 7, 0, 0),	tex1DArrayMipmapFloat,	evalTexture1DArrayOffset,		FRAGMENT),
		CASE_SPEC(isampler1darray,				FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DArrayInt,			evalTexture1DArrayOffset,		VERTEX),
		CASE_SPEC(isampler1darray,				FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3( 7, 0, 0),	tex1DArrayMipmapInt,	evalTexture1DArrayOffset,		FRAGMENT),
		CASE_SPEC(usampler1darray,				FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DArrayUint,			evalTexture1DArrayOffset,		VERTEX),
		CASE_SPEC(usampler1darray,				FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3( 7, 0, 0),	tex1DArrayMipmapUint,	evalTexture1DArrayOffset,		FRAGMENT),

		CASE_SPEC(sampler1darray_bias_fixed,	FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f, -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 0, 0),	tex1DArrayFixed,		evalTexture1DArrayOffsetBias,	FRAGMENT),
		CASE_SPEC(sampler1darray_bias_float,	FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3( 7, 0, 0),	tex1DArrayFloat,		evalTexture1DArrayOffsetBias,	FRAGMENT),
		CASE_SPEC(isampler1darray_bias,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 0, 0),	tex1DArrayInt,			evalTexture1DArrayOffsetBias,	FRAGMENT),
		CASE_SPEC(usampler1darray_bias,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3( 7, 0, 0),	tex1DArrayUint,			evalTexture1DArrayOffsetBias,	FRAGMENT),

		CASE_SPEC(sampler2dshadow,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  1.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DShadow,			evalTexture2DShadowOffset,			VERTEX),
		CASE_SPEC(sampler2dshadow,				FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  1.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapShadow,		evalTexture2DShadowOffset,			FRAGMENT),
		CASE_SPEC(sampler2dshadow_bias,			FUNCTION_TEXTURE,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  1.0f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 7, 0),	tex2DShadow,			evalTexture2DShadowOffsetBias,		FRAGMENT),
		CASE_SPEC(sampler2darrayshadow,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  1.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DArrayShadow,		evalTexture2DArrayShadowOffset,		VERTEX),
		CASE_SPEC(sampler2darrayshadow,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  1.0f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DArrayMipmapShadow,	evalTexture2DArrayShadowOffset,		FRAGMENT),
		CASE_SPEC(sampler1dshadow,				FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  1.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DShadow,			evalTexture1DShadowOffset,			VERTEX),
		CASE_SPEC(sampler1dshadow,				FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  1.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3( 7, 0, 0),	tex1DMipmapShadow,		evalTexture1DShadowOffset,			FRAGMENT),
		CASE_SPEC(sampler1dshadow_bias,			FUNCTION_TEXTURE,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  1.0f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 0, 0),	tex1DShadow,			evalTexture1DShadowOffsetBias,		FRAGMENT),
		CASE_SPEC(sampler1darrayshadow,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  1.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DArrayShadow,		evalTexture1DArrayShadowOffset,		VERTEX),
		CASE_SPEC(sampler1darrayshadow,			FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  1.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3( 7, 0, 0),	tex1DArrayMipmapShadow,	evalTexture1DArrayShadowOffset,		FRAGMENT),
		CASE_SPEC(sampler1darrayshadow_bias,	FUNCTION_TEXTURE,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  1.0f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 0, 0),	tex1DArrayShadow,		evalTexture1DArrayShadowOffsetBias,	FRAGMENT),
	};
	createCaseGroup(this, "textureoffset", "textureOffset() Tests", textureOffsetCases, DE_LENGTH_OF_ARRAY(textureOffsetCases));

	// textureProj() cases
	// \note Currently uses constant divider!
	static const TexFuncCaseSpec textureProjCases[] =
	{
		//		  Name							Function				MinCoord							MaxCoord							Bias?	MinLod	MaxLod	Offset?	Offset		Format					EvalFunc				Flags
		CASE_SPEC(sampler2d_vec3_fixed,			FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DFixed,				evalTexture2DProj3,		VERTEX),
		CASE_SPEC(sampler2d_vec3_fixed,			FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DMipmapFixed,		evalTexture2DProj3,		FRAGMENT),
		CASE_SPEC(sampler2d_vec3_float,			FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DFloat,				evalTexture2DProj3,		VERTEX),
		CASE_SPEC(sampler2d_vec3_float,			FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DMipmapFloat,		evalTexture2DProj3,		FRAGMENT),
		CASE_SPEC(isampler2d_vec3,				FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DInt,				evalTexture2DProj3,		VERTEX),
		CASE_SPEC(isampler2d_vec3,				FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DMipmapInt,			evalTexture2DProj3,		FRAGMENT),
		CASE_SPEC(usampler2d_vec3,				FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DUint,				evalTexture2DProj3,		VERTEX),
		CASE_SPEC(usampler2d_vec3,				FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DMipmapUint,		evalTexture2DProj3,		FRAGMENT),

		CASE_SPEC(sampler2d_vec3_bias_fixed,	FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex2DMipmapFixed,		evalTexture2DProj3Bias,	FRAGMENT),
		CASE_SPEC(sampler2d_vec3_bias_float,	FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex2DMipmapFloat,		evalTexture2DProj3Bias,	FRAGMENT),
		CASE_SPEC(isampler2d_vec3_bias,			FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex2DMipmapInt,			evalTexture2DProj3Bias,	FRAGMENT),
		CASE_SPEC(usampler2d_vec3_bias,			FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex2DMipmapUint,		evalTexture2DProj3Bias,	FRAGMENT),

		CASE_SPEC(sampler2d_vec4_fixed,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DFixed,				evalTexture2DProj,		VERTEX),
		CASE_SPEC(sampler2d_vec4_fixed,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DMipmapFixed,		evalTexture2DProj,		FRAGMENT),
		CASE_SPEC(sampler2d_vec4_float,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DFloat,				evalTexture2DProj,		VERTEX),
		CASE_SPEC(sampler2d_vec4_float,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DMipmapFloat,		evalTexture2DProj,		FRAGMENT),
		CASE_SPEC(isampler2d_vec4,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DInt,				evalTexture2DProj,		VERTEX),
		CASE_SPEC(isampler2d_vec4,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DMipmapInt,			evalTexture2DProj,		FRAGMENT),
		CASE_SPEC(usampler2d_vec4,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DUint,				evalTexture2DProj,		VERTEX),
		CASE_SPEC(usampler2d_vec4,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DMipmapUint,		evalTexture2DProj,		FRAGMENT),

		CASE_SPEC(sampler2d_vec4_bias_fixed,	FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex2DMipmapFixed,		evalTexture2DProjBias,	FRAGMENT),
		CASE_SPEC(sampler2d_vec4_bias_float,	FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex2DMipmapFloat,		evalTexture2DProjBias,	FRAGMENT),
		CASE_SPEC(isampler2d_vec4_bias,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex2DMipmapInt,			evalTexture2DProjBias,	FRAGMENT),
		CASE_SPEC(usampler2d_vec4_bias,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex2DMipmapUint,		evalTexture2DProjBias,	FRAGMENT),

		CASE_SPEC(sampler3d_fixed,				FUNCTION_TEXTUREPROJ,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex3DFixed,				evalTexture3DProj,		VERTEX),
		CASE_SPEC(sampler3d_fixed,				FUNCTION_TEXTUREPROJ,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex3DMipmapFixed,		evalTexture3DProj,		FRAGMENT),
		CASE_SPEC(sampler3d_float,				FUNCTION_TEXTUREPROJ,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex3DFloat,				evalTexture3DProj,		VERTEX),
		CASE_SPEC(sampler3d_float,				FUNCTION_TEXTUREPROJ,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex3DMipmapFloat,		evalTexture3DProj,		FRAGMENT),
		CASE_SPEC(isampler3d,					FUNCTION_TEXTUREPROJ,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex3DInt,				evalTexture3DProj,		VERTEX),
		CASE_SPEC(isampler3d,					FUNCTION_TEXTUREPROJ,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex3DMipmapInt,			evalTexture3DProj,		FRAGMENT),
		CASE_SPEC(usampler3d,					FUNCTION_TEXTUREPROJ,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex3DUint,				evalTexture3DProj,		VERTEX),
		CASE_SPEC(usampler3d,					FUNCTION_TEXTUREPROJ,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex3DMipmapUint,		evalTexture3DProj,		FRAGMENT),

		CASE_SPEC(sampler3d_bias_fixed,			FUNCTION_TEXTUREPROJ,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	true,	-2.0f,	1.0f,	false,	IVec3(0),	tex3DMipmapFixed,		evalTexture3DProjBias,	FRAGMENT),
		CASE_SPEC(sampler3d_bias_float,			FUNCTION_TEXTUREPROJ,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	true,	-2.0f,	1.0f,	false,	IVec3(0),	tex3DMipmapFloat,		evalTexture3DProjBias,	FRAGMENT),
		CASE_SPEC(isampler3d_bias,				FUNCTION_TEXTUREPROJ,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex3DMipmapInt,			evalTexture3DProjBias,	FRAGMENT),
		CASE_SPEC(usampler3d_bias,				FUNCTION_TEXTUREPROJ,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex3DMipmapUint,		evalTexture3DProjBias,	FRAGMENT),

		CASE_SPEC(sampler1d_vec2_fixed,			FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DFixed,				evalTexture1DProj2,		VERTEX),
		CASE_SPEC(sampler1d_vec2_fixed,			FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DMipmapFixed,		evalTexture1DProj2,		FRAGMENT),
		CASE_SPEC(sampler1d_vec2_float,			FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DFloat,				evalTexture1DProj2,		VERTEX),
		CASE_SPEC(sampler1d_vec2_float,			FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DMipmapFloat,		evalTexture1DProj2,		FRAGMENT),
		CASE_SPEC(isampler1d_vec2,				FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DInt,				evalTexture1DProj2,		VERTEX),
		CASE_SPEC(isampler1d_vec2,				FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DMipmapInt,			evalTexture1DProj2,		FRAGMENT),
		CASE_SPEC(usampler1d_vec2,				FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DUint,				evalTexture1DProj2,		VERTEX),
		CASE_SPEC(usampler1d_vec2,				FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DMipmapUint,		evalTexture1DProj2,		FRAGMENT),

		CASE_SPEC(sampler1d_vec2_bias_fixed,	FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DMipmapFixed,		evalTexture1DProj2Bias,	FRAGMENT),
		CASE_SPEC(sampler1d_vec2_bias_float,	FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DMipmapFloat,		evalTexture1DProj2Bias,	FRAGMENT),
		CASE_SPEC(isampler1d_vec2_bias,			FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DMipmapInt,			evalTexture1DProj2Bias,	FRAGMENT),
		CASE_SPEC(usampler1d_vec2_bias,			FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DMipmapUint,		evalTexture1DProj2Bias,	FRAGMENT),

		CASE_SPEC(sampler1d_vec4_fixed,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DFixed,				evalTexture1DProj,		VERTEX),
		CASE_SPEC(sampler1d_vec4_fixed,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DMipmapFixed,		evalTexture1DProj,		FRAGMENT),
		CASE_SPEC(sampler1d_vec4_float,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DFloat,				evalTexture1DProj,		VERTEX),
		CASE_SPEC(sampler1d_vec4_float,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DMipmapFloat,		evalTexture1DProj,		FRAGMENT),
		CASE_SPEC(isampler1d_vec4,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DInt,				evalTexture1DProj,		VERTEX),
		CASE_SPEC(isampler1d_vec4,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DMipmapInt,			evalTexture1DProj,		FRAGMENT),
		CASE_SPEC(usampler1d_vec4,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DUint,				evalTexture1DProj,		VERTEX),
		CASE_SPEC(usampler1d_vec4,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DMipmapUint,		evalTexture1DProj,		FRAGMENT),

		CASE_SPEC(sampler1d_vec4_bias_fixed,	FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DMipmapFixed,		evalTexture1DProjBias,	FRAGMENT),
		CASE_SPEC(sampler1d_vec4_bias_float,	FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DMipmapFloat,		evalTexture1DProjBias,	FRAGMENT),
		CASE_SPEC(isampler1d_vec4_bias,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DMipmapInt,			evalTexture1DProjBias,	FRAGMENT),
		CASE_SPEC(usampler1d_vec4_bias,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DMipmapUint,		evalTexture1DProjBias,	FRAGMENT),

		CASE_SPEC(sampler2dshadow,				FUNCTION_TEXTUREPROJ,	Vec4( 0.2f, 0.6f,  0.0f,  1.5f),	Vec4(-2.25f, -3.45f, 1.5f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DShadow,			evalTexture2DShadowProj,		VERTEX),
		CASE_SPEC(sampler2dshadow,				FUNCTION_TEXTUREPROJ,	Vec4( 0.2f, 0.6f,  0.0f,  1.5f),	Vec4(-2.25f, -3.45f, 1.5f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DMipmapShadow,		evalTexture2DShadowProj,		FRAGMENT),
		CASE_SPEC(sampler2dshadow_bias,			FUNCTION_TEXTUREPROJ,	Vec4( 0.2f, 0.6f,  0.0f,  1.5f),	Vec4(-2.25f, -3.45f, 1.5f,  1.5f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex2DMipmapShadow,		evalTexture2DShadowProjBias,	FRAGMENT),
		CASE_SPEC(sampler1dshadow,				FUNCTION_TEXTUREPROJ,	Vec4( 0.2f, 0.0f,  0.0f,  1.5f),	Vec4(-2.25f,   0.0f, 1.5f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DShadow,			evalTexture1DShadowProj,		VERTEX),
		CASE_SPEC(sampler1dshadow,				FUNCTION_TEXTUREPROJ,	Vec4( 0.2f, 0.0f,  0.0f,  1.5f),	Vec4(-2.25f,   0.0f, 1.5f,  1.5f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DMipmapShadow,		evalTexture1DShadowProj,		FRAGMENT),
		CASE_SPEC(sampler1dshadow_bias,			FUNCTION_TEXTUREPROJ,	Vec4( 0.2f, 0.0f,  0.0f,  1.5f),	Vec4(-2.25f,   0.0f, 1.5f,  1.5f),	true,	-2.0f,	2.0f,	false,	IVec3(0),	tex1DMipmapShadow,		evalTexture1DShadowProjBias,	FRAGMENT),
	};
	createCaseGroup(this, "textureproj", "textureProj() Tests", textureProjCases, DE_LENGTH_OF_ARRAY(textureProjCases));

	// textureProjOffset() cases
	// \note Currently uses constant divider!
	static const TexFuncCaseSpec textureProjOffsetCases[] =
	{
		//		  Name							Function				MinCoord							MaxCoord							Bias?	MinLod	MaxLod	Offset?	Offset				Format					EvalFunc						Flags
		CASE_SPEC(sampler2d_vec3_fixed,			FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DFixed,				evalTexture2DProj3Offset,		VERTEX),
		CASE_SPEC(sampler2d_vec3_fixed,			FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapFixed,		evalTexture2DProj3Offset,		FRAGMENT),
		CASE_SPEC(sampler2d_vec3_float,			FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DFloat,				evalTexture2DProj3Offset,		VERTEX),
		CASE_SPEC(sampler2d_vec3_float,			FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapFloat,		evalTexture2DProj3Offset,		FRAGMENT),
		CASE_SPEC(isampler2d_vec3,				FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DInt,				evalTexture2DProj3Offset,		VERTEX),
		CASE_SPEC(isampler2d_vec3,				FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapInt,			evalTexture2DProj3Offset,		FRAGMENT),
		CASE_SPEC(usampler2d_vec3,				FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DUint,				evalTexture2DProj3Offset,		VERTEX),
		CASE_SPEC(usampler2d_vec3,				FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapUint,		evalTexture2DProj3Offset,		FRAGMENT),

		CASE_SPEC(sampler2d_vec3_bias_fixed,	FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 7, 0),	tex2DFixed,				evalTexture2DProj3OffsetBias,	FRAGMENT),
		CASE_SPEC(sampler2d_vec3_bias_float,	FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(7, -8, 0),	tex2DFloat,				evalTexture2DProj3OffsetBias,	FRAGMENT),
		CASE_SPEC(isampler2d_vec3_bias,			FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 7, 0),	tex2DInt,				evalTexture2DProj3OffsetBias,	FRAGMENT),
		CASE_SPEC(usampler2d_vec3_bias,			FUNCTION_TEXTUREPROJ3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(7, -8, 0),	tex2DUint,				evalTexture2DProj3OffsetBias,	FRAGMENT),

		CASE_SPEC(sampler2d_vec4_fixed,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DFixed,				evalTexture2DProjOffset,		VERTEX),
		CASE_SPEC(sampler2d_vec4_fixed,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapFixed,		evalTexture2DProjOffset,		FRAGMENT),
		CASE_SPEC(sampler2d_vec4_float,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DFloat,				evalTexture2DProjOffset,		VERTEX),
		CASE_SPEC(sampler2d_vec4_float,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapFloat,		evalTexture2DProjOffset,		FRAGMENT),
		CASE_SPEC(isampler2d_vec4,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DInt,				evalTexture2DProjOffset,		VERTEX),
		CASE_SPEC(isampler2d_vec4,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapInt,			evalTexture2DProjOffset,		FRAGMENT),
		CASE_SPEC(usampler2d_vec4,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DUint,				evalTexture2DProjOffset,		VERTEX),
		CASE_SPEC(usampler2d_vec4,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapUint,		evalTexture2DProjOffset,		FRAGMENT),

		CASE_SPEC(sampler2d_vec4_bias_fixed,	FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 7, 0),	tex2DFixed,				evalTexture2DProjOffsetBias,	FRAGMENT),
		CASE_SPEC(sampler2d_vec4_bias_float,	FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	true,	-2.0f,	2.0f,	true,	IVec3(7, -8, 0),	tex2DFloat,				evalTexture2DProjOffsetBias,	FRAGMENT),
		CASE_SPEC(isampler2d_vec4_bias,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 7, 0),	tex2DInt,				evalTexture2DProjOffsetBias,	FRAGMENT),
		CASE_SPEC(usampler2d_vec4_bias,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	true,	-2.0f,	2.0f,	true,	IVec3(7, -8, 0),	tex2DUint,				evalTexture2DProjOffsetBias,	FRAGMENT),

		CASE_SPEC(sampler3d_fixed,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  2.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 3),	tex3DFixed,				evalTexture3DProjOffset,		VERTEX),
		CASE_SPEC(sampler3d_fixed,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  2.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(7, 3, -8),	tex3DMipmapFixed,		evalTexture3DProjOffset,		FRAGMENT),
		CASE_SPEC(sampler3d_float,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  2.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(3, -8, 7),	tex3DFloat,				evalTexture3DProjOffset,		VERTEX),
		CASE_SPEC(sampler3d_float,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  2.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 3),	tex3DMipmapFloat,		evalTexture3DProjOffset,		FRAGMENT),
		CASE_SPEC(isampler3d,					FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  2.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(7, 3, -8),	tex3DInt,				evalTexture3DProjOffset,		VERTEX),
		CASE_SPEC(isampler3d,					FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  2.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(3, -8, 7),	tex3DMipmapInt,			evalTexture3DProjOffset,		FRAGMENT),
		CASE_SPEC(usampler3d,					FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  2.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 3),	tex3DUint,				evalTexture3DProjOffset,		VERTEX),
		CASE_SPEC(usampler3d,					FUNCTION_TEXTUREPROJ,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  2.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(7, 3, -8),	tex3DMipmapUint,		evalTexture3DProjOffset,		FRAGMENT),

		CASE_SPEC(sampler3d_bias_fixed,			FUNCTION_TEXTUREPROJ,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 7, 3),	tex3DFixed,				evalTexture3DProjOffsetBias,	FRAGMENT),
		CASE_SPEC(sampler3d_bias_float,			FUNCTION_TEXTUREPROJ,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	true,	-2.0f,	2.0f,	true,	IVec3(7, 3, -8),	tex3DFloat,				evalTexture3DProjOffsetBias,	FRAGMENT),
		CASE_SPEC(isampler3d_bias,				FUNCTION_TEXTUREPROJ,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	true,	-2.0f,	2.0f,	true,	IVec3(3, -8, 7),	tex3DInt,				evalTexture3DProjOffsetBias,	FRAGMENT),
		CASE_SPEC(usampler3d_bias,				FUNCTION_TEXTUREPROJ,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 7, 3),	tex3DUint,				evalTexture3DProjOffsetBias,	FRAGMENT),

		CASE_SPEC(sampler1d_vec2_fixed,			FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DFixed,				evalTexture1DProj2Offset,		VERTEX),
		CASE_SPEC(sampler1d_vec2_fixed,			FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7,  0, 0),	tex1DMipmapFixed,		evalTexture1DProj2Offset,		FRAGMENT),
		CASE_SPEC(sampler1d_vec2_float,			FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DFloat,				evalTexture1DProj2Offset,		VERTEX),
		CASE_SPEC(sampler1d_vec2_float,			FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7,  0, 0),	tex1DMipmapFloat,		evalTexture1DProj2Offset,		FRAGMENT),
		CASE_SPEC(isampler1d_vec2,				FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DInt,				evalTexture1DProj2Offset,		VERTEX),
		CASE_SPEC(isampler1d_vec2,				FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7,  0, 0),	tex1DMipmapInt,			evalTexture1DProj2Offset,		FRAGMENT),
		CASE_SPEC(usampler1d_vec2,				FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DUint,				evalTexture1DProj2Offset,		VERTEX),
		CASE_SPEC(usampler1d_vec2,				FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(7,  0, 0),	tex1DMipmapUint,		evalTexture1DProj2Offset,		FRAGMENT),

		CASE_SPEC(sampler1d_vec2_bias_fixed,	FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 0, 0),	tex1DFixed,				evalTexture1DProj2OffsetBias,	FRAGMENT),
		CASE_SPEC(sampler1d_vec2_bias_float,	FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(7,  0, 0),	tex1DFloat,				evalTexture1DProj2OffsetBias,	FRAGMENT),
		CASE_SPEC(isampler1d_vec2_bias,			FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 0, 0),	tex1DInt,				evalTexture1DProj2OffsetBias,	FRAGMENT),
		CASE_SPEC(usampler1d_vec2_bias,			FUNCTION_TEXTUREPROJ2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	true,	-2.0f,	2.0f,	true,	IVec3(7,  0, 0),	tex1DUint,				evalTexture1DProj2OffsetBias,	FRAGMENT),

		CASE_SPEC(sampler1d_vec4_fixed,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DFixed,				evalTexture1DProjOffset,		VERTEX),
		CASE_SPEC(sampler1d_vec4_fixed,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(7,  0, 0),	tex1DMipmapFixed,		evalTexture1DProjOffset,		FRAGMENT),
		CASE_SPEC(sampler1d_vec4_float,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DFloat,				evalTexture1DProjOffset,		VERTEX),
		CASE_SPEC(sampler1d_vec4_float,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(7,  0, 0),	tex1DMipmapFloat,		evalTexture1DProjOffset,		FRAGMENT),
		CASE_SPEC(isampler1d_vec4,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DInt,				evalTexture1DProjOffset,		VERTEX),
		CASE_SPEC(isampler1d_vec4,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(7,  0, 0),	tex1DMipmapInt,			evalTexture1DProjOffset,		FRAGMENT),
		CASE_SPEC(usampler1d_vec4,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DUint,				evalTexture1DProjOffset,		VERTEX),
		CASE_SPEC(usampler1d_vec4,				FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(7,  0, 0),	tex1DMipmapUint,		evalTexture1DProjOffset,		FRAGMENT),

		CASE_SPEC(sampler1d_vec4_bias_fixed,	FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 0, 0),	tex1DFixed,				evalTexture1DProjOffsetBias,	FRAGMENT),
		CASE_SPEC(sampler1d_vec4_bias_float,	FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	true,	-2.0f,	2.0f,	true,	IVec3(7,  0, 0),	tex1DFloat,				evalTexture1DProjOffsetBias,	FRAGMENT),
		CASE_SPEC(isampler1d_vec4_bias,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 0, 0),	tex1DInt,				evalTexture1DProjOffsetBias,	FRAGMENT),
		CASE_SPEC(usampler1d_vec4_bias,			FUNCTION_TEXTUREPROJ,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	true,	-2.0f,	2.0f,	true,	IVec3(7,  0, 0),	tex1DUint,				evalTexture1DProjOffsetBias,	FRAGMENT),

		CASE_SPEC(sampler2dshadow,				FUNCTION_TEXTUREPROJ,	Vec4( 0.2f, 0.6f,  0.0f,  1.5f),	Vec4(-2.25f, -3.45f, 1.5f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DShadow,			evalTexture2DShadowProjOffset,		VERTEX),
		CASE_SPEC(sampler2dshadow,				FUNCTION_TEXTUREPROJ,	Vec4( 0.2f, 0.6f,  0.0f,  1.5f),	Vec4(-2.25f, -3.45f, 1.5f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapShadow,		evalTexture2DShadowProjOffset,		FRAGMENT),
		CASE_SPEC(sampler2dshadow_bias,			FUNCTION_TEXTUREPROJ,	Vec4( 0.2f, 0.6f,  0.0f,  1.5f),	Vec4(-2.25f, -3.45f, 1.5f,  1.5f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 7, 0),	tex2DShadow,			evalTexture2DShadowProjOffsetBias,	FRAGMENT),
		CASE_SPEC(sampler1dshadow,				FUNCTION_TEXTUREPROJ,	Vec4( 0.2f, 0.0f,  0.0f,  1.5f),	Vec4(-2.25f,   0.0f, 1.5f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DShadow,			evalTexture1DShadowProjOffset,		VERTEX),
		CASE_SPEC(sampler1dshadow,				FUNCTION_TEXTUREPROJ,	Vec4( 0.2f, 0.0f,  0.0f,  1.5f),	Vec4(-2.25f,   0.0f, 1.5f,  1.5f),	false,	0.0f,	0.0f,	true,	IVec3(7,  0, 0),	tex1DMipmapShadow,		evalTexture1DShadowProjOffset,		FRAGMENT),
		CASE_SPEC(sampler1dshadow_bias,			FUNCTION_TEXTUREPROJ,	Vec4( 0.2f, 0.0f,  0.0f,  1.5f),	Vec4(-2.25f,   0.0f, 1.5f,  1.5f),	true,	-2.0f,	2.0f,	true,	IVec3(-8, 0, 0),	tex1DShadow,			evalTexture1DShadowProjOffsetBias,	FRAGMENT),
	};
	createCaseGroup(this, "textureprojoffset", "textureOffsetProj() Tests", textureProjOffsetCases, DE_LENGTH_OF_ARRAY(textureProjOffsetCases));

	// textureLod() cases
	static const TexFuncCaseSpec textureLodCases[] =
	{
		//		  Name							Function				MinCoord							MaxCoord							Bias?	MinLod	MaxLod	Offset?	Offset		Format					EvalFunc				Flags
		CASE_SPEC(sampler2d_fixed,				FUNCTION_TEXTURELOD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex2DMipmapFixed,		evalTexture2DLod,		BOTH),
		CASE_SPEC(sampler2d_float,				FUNCTION_TEXTURELOD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex2DMipmapFloat,		evalTexture2DLod,		BOTH),
		CASE_SPEC(isampler2d,					FUNCTION_TEXTURELOD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex2DMipmapInt,			evalTexture2DLod,		BOTH),
		CASE_SPEC(usampler2d,					FUNCTION_TEXTURELOD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex2DMipmapUint,		evalTexture2DLod,		BOTH),

		CASE_SPEC(samplercube_fixed,			FUNCTION_TEXTURELOD,	Vec4(-1.0f, -1.0f,  1.01f,  0.0f),	Vec4( 1.0f,  1.0f,  1.01f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	texCubeMipmapFixed,		evalTextureCubeLod,		BOTH),
		CASE_SPEC(samplercube_float,			FUNCTION_TEXTURELOD,	Vec4(-1.0f, -1.0f, -1.01f,  0.0f),	Vec4( 1.0f,  1.0f, -1.01f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	texCubeMipmapFloat,		evalTextureCubeLod,		BOTH),
		CASE_SPEC(isamplercube,					FUNCTION_TEXTURELOD,	Vec4(-1.0f, -1.0f,  1.01f,  0.0f),	Vec4( 1.0f,  1.0f,  1.01f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	texCubeMipmapInt,		evalTextureCubeLod,		BOTH),
		CASE_SPEC(usamplercube,					FUNCTION_TEXTURELOD,	Vec4(-1.0f, -1.0f, -1.01f,  0.0f),	Vec4( 1.0f,  1.0f, -1.01f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	texCubeMipmapUint,		evalTextureCubeLod,		BOTH),

		CASE_SPEC(sampler2darray_fixed,			FUNCTION_TEXTURELOD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	-1.0f,	8.0f,	false,	IVec3(0),	tex2DArrayMipmapFixed,	evalTexture2DArrayLod,	BOTH),
		CASE_SPEC(sampler2darray_float,			FUNCTION_TEXTURELOD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	-1.0f,	8.0f,	false,	IVec3(0),	tex2DArrayMipmapFloat,	evalTexture2DArrayLod,	BOTH),
		CASE_SPEC(isampler2darray,				FUNCTION_TEXTURELOD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	-1.0f,	8.0f,	false,	IVec3(0),	tex2DArrayMipmapInt,	evalTexture2DArrayLod,	BOTH),
		CASE_SPEC(usampler2darray,				FUNCTION_TEXTURELOD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	-1.0f,	8.0f,	false,	IVec3(0),	tex2DArrayMipmapUint,	evalTexture2DArrayLod,	BOTH),

		CASE_SPEC(sampler3d_fixed,				FUNCTION_TEXTURELOD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	-1.0f,	7.0f,	false,	IVec3(0),	tex3DMipmapFixed,		evalTexture3DLod,		BOTH),
		CASE_SPEC(sampler3d_float,				FUNCTION_TEXTURELOD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	-1.0f,	7.0f,	false,	IVec3(0),	tex3DMipmapFloat,		evalTexture3DLod,		BOTH),
		CASE_SPEC(isampler3d,					FUNCTION_TEXTURELOD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	-1.0f,	7.0f,	false,	IVec3(0),	tex3DMipmapInt,			evalTexture3DLod,		BOTH),
		CASE_SPEC(usampler3d,					FUNCTION_TEXTURELOD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	-1.0f,	7.0f,	false,	IVec3(0),	tex3DMipmapUint,		evalTexture3DLod,		BOTH),

		CASE_SPEC(sampler1d_fixed,				FUNCTION_TEXTURELOD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DMipmapFixed,		evalTexture1DLod,		BOTH),
		CASE_SPEC(sampler1d_float,				FUNCTION_TEXTURELOD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DMipmapFloat,		evalTexture1DLod,		BOTH),
		CASE_SPEC(isampler1d,					FUNCTION_TEXTURELOD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DMipmapInt,			evalTexture1DLod,		BOTH),
		CASE_SPEC(usampler1d,					FUNCTION_TEXTURELOD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DMipmapUint,		evalTexture1DLod,		BOTH),

		CASE_SPEC(sampler1darray_fixed,			FUNCTION_TEXTURELOD,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DArrayMipmapFixed,	evalTexture1DArrayLod,	BOTH),
		CASE_SPEC(sampler1darray_float,			FUNCTION_TEXTURELOD,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DArrayMipmapFloat,	evalTexture1DArrayLod,	BOTH),
		CASE_SPEC(isampler1darray,				FUNCTION_TEXTURELOD,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DArrayMipmapInt,	evalTexture1DArrayLod,	BOTH),
		CASE_SPEC(usampler1darray,				FUNCTION_TEXTURELOD,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DArrayMipmapUint,	evalTexture1DArrayLod,	BOTH),

		CASE_SPEC(samplercubearray_fixed,		FUNCTION_TEXTURELOD,	Vec4(-1.0f, -1.0f,  1.01f, -0.5f),	Vec4( 1.0f,  1.0f,  1.01f,  1.5f),	false,	-1.0f,	7.0f,	false,	IVec3(0),	texCubeArrayMipmapFixed,	evalTextureCubeArrayLod,	BOTH),
		CASE_SPEC(samplercubearray_float,		FUNCTION_TEXTURELOD,	Vec4(-1.0f, -1.0f, -1.01f, -0.5f),	Vec4( 1.0f,  1.0f, -1.01f,  1.5f),	false,	-1.0f,	7.0f,	false,	IVec3(0),	texCubeArrayMipmapFloat,	evalTextureCubeArrayLod,	BOTH),
		CASE_SPEC(isamplercubearray,			FUNCTION_TEXTURELOD,	Vec4(-1.0f, -1.0f,  1.01f, -0.5f),	Vec4( 1.0f,  1.0f,  1.01f,  1.5f),	false,	-1.0f,	7.0f,	false,	IVec3(0),	texCubeArrayMipmapInt,		evalTextureCubeArrayLod,	BOTH),
		CASE_SPEC(usamplercubearray,			FUNCTION_TEXTURELOD,	Vec4(-1.0f, -1.0f, -1.01f, -0.5f),	Vec4( 1.0f,  1.0f, -1.01f,  1.5f),	false,	-1.0f,	7.0f,	false,	IVec3(0),	texCubeArrayMipmapUint,		evalTextureCubeArrayLod,	BOTH),

		CASE_SPEC(sampler2dshadow,				FUNCTION_TEXTURELOD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  1.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex2DMipmapShadow,		evalTexture2DShadowLod,			BOTH),
		CASE_SPEC(sampler1dshadow,				FUNCTION_TEXTURELOD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  1.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DMipmapShadow,		evalTexture1DShadowLod,			BOTH),
		CASE_SPEC(sampler1darrayshadow,			FUNCTION_TEXTURELOD,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  1.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DArrayMipmapShadow,	evalTexture1DArrayShadowLod,	BOTH),
	};
	createCaseGroup(this, "texturelod", "textureLod() Tests", textureLodCases, DE_LENGTH_OF_ARRAY(textureLodCases));

	// textureLodOffset() cases
	static const TexFuncCaseSpec textureLodOffsetCases[] =
	{
		//		  Name							Function				MinCoord							MaxCoord							Bias?	MinLod	MaxLod	Offset?	Offset				Format					EvalFunc						Flags
		CASE_SPEC(sampler2d_fixed,				FUNCTION_TEXTURELOD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(-8, 7, 0),	tex2DMipmapFixed,		evalTexture2DLodOffset,			BOTH),
		CASE_SPEC(sampler2d_float,				FUNCTION_TEXTURELOD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapFloat,		evalTexture2DLodOffset,			BOTH),
		CASE_SPEC(isampler2d,					FUNCTION_TEXTURELOD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(-8, 7, 0),	tex2DMipmapInt,			evalTexture2DLodOffset,			BOTH),
		CASE_SPEC(usampler2d,					FUNCTION_TEXTURELOD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapUint,		evalTexture2DLodOffset,			BOTH),

		CASE_SPEC(sampler2darray_fixed,			FUNCTION_TEXTURELOD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	-1.0f,	8.0f,	true,	IVec3(-8, 7, 0),	tex2DArrayMipmapFixed,	evalTexture2DArrayLodOffset,	BOTH),
		CASE_SPEC(sampler2darray_float,			FUNCTION_TEXTURELOD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	-1.0f,	8.0f,	true,	IVec3(7, -8, 0),	tex2DArrayMipmapFloat,	evalTexture2DArrayLodOffset,	BOTH),
		CASE_SPEC(isampler2darray,				FUNCTION_TEXTURELOD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	-1.0f,	8.0f,	true,	IVec3(-8, 7, 0),	tex2DArrayMipmapInt,	evalTexture2DArrayLodOffset,	BOTH),
		CASE_SPEC(usampler2darray,				FUNCTION_TEXTURELOD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	false,	-1.0f,	8.0f,	true,	IVec3(7, -8, 0),	tex2DArrayMipmapUint,	evalTexture2DArrayLodOffset,	BOTH),

		CASE_SPEC(sampler3d_fixed,				FUNCTION_TEXTURELOD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	-1.0f,	7.0f,	true,	IVec3(-8, 7, 3),	tex3DMipmapFixed,		evalTexture3DLodOffset,			BOTH),
		CASE_SPEC(sampler3d_float,				FUNCTION_TEXTURELOD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	-1.0f,	7.0f,	true,	IVec3(7, 3, -8),	tex3DMipmapFloat,		evalTexture3DLodOffset,			BOTH),
		CASE_SPEC(isampler3d,					FUNCTION_TEXTURELOD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	-1.0f,	7.0f,	true,	IVec3(3, -8, 7),	tex3DMipmapInt,			evalTexture3DLodOffset,			BOTH),
		CASE_SPEC(usampler3d,					FUNCTION_TEXTURELOD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	false,	-1.0f,	7.0f,	true,	IVec3(-8, 7, 3),	tex3DMipmapUint,		evalTexture3DLodOffset,			BOTH),

		CASE_SPEC(sampler1d_fixed,				FUNCTION_TEXTURELOD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(-8, 0, 0),	tex1DMipmapFixed,		evalTexture1DLodOffset,			BOTH),
		CASE_SPEC(sampler1d_float,				FUNCTION_TEXTURELOD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(7,  0, 0),	tex1DMipmapFloat,		evalTexture1DLodOffset,			BOTH),
		CASE_SPEC(isampler1d,					FUNCTION_TEXTURELOD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(-8, 0, 0),	tex1DMipmapInt,			evalTexture1DLodOffset,			BOTH),
		CASE_SPEC(usampler1d,					FUNCTION_TEXTURELOD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(7,  0, 0),	tex1DMipmapUint,		evalTexture1DLodOffset,			BOTH),

		CASE_SPEC(sampler1darray_fixed,			FUNCTION_TEXTURELOD,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(-8, 0, 0),	tex1DArrayMipmapFixed,	evalTexture1DArrayLodOffset,	BOTH),
		CASE_SPEC(sampler1darray_float,			FUNCTION_TEXTURELOD,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(7,  0, 0),	tex1DArrayMipmapFloat,	evalTexture1DArrayLodOffset,	BOTH),
		CASE_SPEC(isampler1darray,				FUNCTION_TEXTURELOD,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(-8, 0, 0),	tex1DArrayMipmapInt,	evalTexture1DArrayLodOffset,	BOTH),
		CASE_SPEC(usampler1darray,				FUNCTION_TEXTURELOD,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(7,  0, 0),	tex1DArrayMipmapUint,	evalTexture1DArrayLodOffset,	BOTH),

		CASE_SPEC(sampler2dshadow,				FUNCTION_TEXTURELOD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  1.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(-8, 7, 0),	tex2DMipmapShadow,		evalTexture2DShadowLodOffset,		BOTH),
		CASE_SPEC(sampler1dshadow,				FUNCTION_TEXTURELOD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  1.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(-8, 0, 0),	tex1DMipmapShadow,		evalTexture1DShadowLodOffset,		BOTH),
		CASE_SPEC(sampler1darrayshadow,			FUNCTION_TEXTURELOD,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  1.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(7,  0, 0),	tex1DArrayMipmapShadow,	evalTexture1DArrayShadowLodOffset,	BOTH),
	};
	createCaseGroup(this, "texturelodoffset", "textureLodOffset() Tests", textureLodOffsetCases, DE_LENGTH_OF_ARRAY(textureLodOffsetCases));

	// textureProjLod() cases
	static const TexFuncCaseSpec textureProjLodCases[] =
	{
		//		  Name							Function					MinCoord							MaxCoord							Bias?	MinLod	MaxLod	Offset?	Offset		Format					EvalFunc					Flags
		CASE_SPEC(sampler2d_vec3_fixed,			FUNCTION_TEXTUREPROJLOD3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex2DMipmapFixed,		evalTexture2DProjLod3,		BOTH),
		CASE_SPEC(sampler2d_vec3_float,			FUNCTION_TEXTUREPROJLOD3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex2DMipmapFloat,		evalTexture2DProjLod3,		BOTH),
		CASE_SPEC(isampler2d_vec3,				FUNCTION_TEXTUREPROJLOD3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex2DMipmapInt,			evalTexture2DProjLod3,		BOTH),
		CASE_SPEC(usampler2d_vec3,				FUNCTION_TEXTUREPROJLOD3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex2DMipmapUint,		evalTexture2DProjLod3,		BOTH),

		CASE_SPEC(sampler2d_vec4_fixed,			FUNCTION_TEXTUREPROJLOD,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex2DMipmapFixed,		evalTexture2DProjLod,		BOTH),
		CASE_SPEC(sampler2d_vec4_float,			FUNCTION_TEXTUREPROJLOD,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex2DMipmapFloat,		evalTexture2DProjLod,		BOTH),
		CASE_SPEC(isampler2d_vec4,				FUNCTION_TEXTUREPROJLOD,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex2DMipmapInt,			evalTexture2DProjLod,		BOTH),
		CASE_SPEC(usampler2d_vec4,				FUNCTION_TEXTUREPROJLOD,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex2DMipmapUint,		evalTexture2DProjLod,		BOTH),

		CASE_SPEC(sampler3d_fixed,				FUNCTION_TEXTUREPROJLOD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	false,	-1.0f,	7.0f,	false,	IVec3(0),	tex3DMipmapFixed,		evalTexture3DProjLod,		BOTH),
		CASE_SPEC(sampler3d_float,				FUNCTION_TEXTUREPROJLOD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	false,	-1.0f,	7.0f,	false,	IVec3(0),	tex3DMipmapFloat,		evalTexture3DProjLod,		BOTH),
		CASE_SPEC(isampler3d,					FUNCTION_TEXTUREPROJLOD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	false,	-1.0f,	7.0f,	false,	IVec3(0),	tex3DMipmapInt,			evalTexture3DProjLod,		BOTH),
		CASE_SPEC(usampler3d,					FUNCTION_TEXTUREPROJLOD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	false,	-1.0f,	7.0f,	false,	IVec3(0),	tex3DMipmapUint,		evalTexture3DProjLod,		BOTH),

		CASE_SPEC(sampler1d_vec2_fixed,			FUNCTION_TEXTUREPROJLOD2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DMipmapFixed,		evalTexture1DProjLod2,		BOTH),
		CASE_SPEC(sampler1d_vec2_float,			FUNCTION_TEXTUREPROJLOD2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DMipmapFloat,		evalTexture1DProjLod2,		BOTH),
		CASE_SPEC(isampler1d_vec2,				FUNCTION_TEXTUREPROJLOD2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DMipmapInt,			evalTexture1DProjLod2,		BOTH),
		CASE_SPEC(usampler1d_vec2,				FUNCTION_TEXTUREPROJLOD2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DMipmapUint,		evalTexture1DProjLod2,		BOTH),

		CASE_SPEC(sampler1d_vec4_fixed,			FUNCTION_TEXTUREPROJLOD,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DMipmapFixed,		evalTexture1DProjLod,		BOTH),
		CASE_SPEC(sampler1d_vec4_float,			FUNCTION_TEXTUREPROJLOD,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DMipmapFloat,		evalTexture1DProjLod,		BOTH),
		CASE_SPEC(isampler1d_vec4,				FUNCTION_TEXTUREPROJLOD,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DMipmapInt,			evalTexture1DProjLod,		BOTH),
		CASE_SPEC(usampler1d_vec4,				FUNCTION_TEXTUREPROJLOD,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DMipmapUint,		evalTexture1DProjLod,		BOTH),

		CASE_SPEC(sampler2dshadow,				FUNCTION_TEXTUREPROJLOD,	Vec4( 0.2f, 0.6f,  0.0f,  1.5f),	Vec4(-2.25f, -3.45f, 1.5f,  1.5f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex2DMipmapShadow,		evalTexture2DShadowProjLod,	BOTH),
		CASE_SPEC(sampler1dshadow,				FUNCTION_TEXTUREPROJLOD,	Vec4( 0.2f, 0.0f,  0.0f,  1.5f),	Vec4(-2.25f,  0.0f,  0.0f,  1.5f),	false,	-1.0f,	9.0f,	false,	IVec3(0),	tex1DMipmapShadow,		evalTexture1DShadowProjLod,	BOTH),
	};
	createCaseGroup(this, "textureprojlod", "textureProjLod() Tests", textureProjLodCases, DE_LENGTH_OF_ARRAY(textureProjLodCases));

	// textureProjLodOffset() cases
	static const TexFuncCaseSpec textureProjLodOffsetCases[] =
	{
		//		  Name							Function					MinCoord							MaxCoord							Bias?	MinLod	MaxLod	Offset?	Offset				Format					EvalFunc								Flags
		CASE_SPEC(sampler2d_vec3_fixed,			FUNCTION_TEXTUREPROJLOD3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(-8, 7, 0),	tex2DMipmapFixed,		evalTexture2DProjLod3Offset,	BOTH),
		CASE_SPEC(sampler2d_vec3_float,			FUNCTION_TEXTUREPROJLOD3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapFloat,		evalTexture2DProjLod3Offset,	BOTH),
		CASE_SPEC(isampler2d_vec3,				FUNCTION_TEXTUREPROJLOD3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(-8, 7, 0),	tex2DMipmapInt,			evalTexture2DProjLod3Offset,	BOTH),
		CASE_SPEC(usampler2d_vec3,				FUNCTION_TEXTUREPROJLOD3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapUint,		evalTexture2DProjLod3Offset,	BOTH),

		CASE_SPEC(sampler2d_vec4_fixed,			FUNCTION_TEXTUREPROJLOD,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	-1.0f,	9.0f,	true,	IVec3(-8, 7, 0),	tex2DMipmapFixed,		evalTexture2DProjLodOffset,		BOTH),
		CASE_SPEC(sampler2d_vec4_float,			FUNCTION_TEXTUREPROJLOD,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	-1.0f,	9.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapFloat,		evalTexture2DProjLodOffset,		BOTH),
		CASE_SPEC(isampler2d_vec4,				FUNCTION_TEXTUREPROJLOD,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	-1.0f,	9.0f,	true,	IVec3(-8, 7, 0),	tex2DMipmapInt,			evalTexture2DProjLodOffset,		BOTH),
		CASE_SPEC(usampler2d_vec4,				FUNCTION_TEXTUREPROJLOD,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	false,	-1.0f,	9.0f,	true,	IVec3(7, -8, 0),	tex2DMipmapUint,		evalTexture2DProjLodOffset,		BOTH),

		CASE_SPEC(sampler3d_fixed,				FUNCTION_TEXTUREPROJLOD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	false,	-1.0f,	7.0f,	true,	IVec3(-8, 7, 3),	tex3DMipmapFixed,		evalTexture3DProjLodOffset,		BOTH),
		CASE_SPEC(sampler3d_float,				FUNCTION_TEXTUREPROJLOD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	false,	-1.0f,	7.0f,	true,	IVec3(7, 3, -8),	tex3DMipmapFloat,		evalTexture3DProjLodOffset,		BOTH),
		CASE_SPEC(isampler3d,					FUNCTION_TEXTUREPROJLOD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	false,	-1.0f,	7.0f,	true,	IVec3(3, -8, 7),	tex3DMipmapInt,			evalTexture3DProjLodOffset,		BOTH),
		CASE_SPEC(usampler3d,					FUNCTION_TEXTUREPROJLOD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	false,	-1.0f,	7.0f,	true,	IVec3(-8, 7, 3),	tex3DMipmapUint,		evalTexture3DProjLodOffset,		BOTH),

		CASE_SPEC(sampler1d_vec2_fixed,			FUNCTION_TEXTUREPROJLOD2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(-8, 0, 0),	tex1DMipmapFixed,		evalTexture1DProjLod2Offset,	BOTH),
		CASE_SPEC(sampler1d_vec2_float,			FUNCTION_TEXTUREPROJLOD2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(7,  0, 0),	tex1DMipmapFloat,		evalTexture1DProjLod2Offset,	BOTH),
		CASE_SPEC(isampler1d_vec2,				FUNCTION_TEXTUREPROJLOD2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(-8, 0, 0),	tex1DMipmapInt,			evalTexture1DProjLod2Offset,	BOTH),
		CASE_SPEC(usampler1d_vec2,				FUNCTION_TEXTUREPROJLOD2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	false,	-1.0f,	9.0f,	true,	IVec3(7,  0, 0),	tex1DMipmapUint,		evalTexture1DProjLod2Offset,	BOTH),

		CASE_SPEC(sampler1d_vec4_fixed,			FUNCTION_TEXTUREPROJLOD,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	-1.0f,	9.0f,	true,	IVec3(-8, 0, 0),	tex1DMipmapFixed,		evalTexture1DProjLodOffset,		BOTH),
		CASE_SPEC(sampler1d_vec4_float,			FUNCTION_TEXTUREPROJLOD,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	-1.0f,	9.0f,	true,	IVec3(7,  0, 0),	tex1DMipmapFloat,		evalTexture1DProjLodOffset,		BOTH),
		CASE_SPEC(isampler1d_vec4,				FUNCTION_TEXTUREPROJLOD,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	-1.0f,	9.0f,	true,	IVec3(-8, 0, 0),	tex1DMipmapInt,			evalTexture1DProjLodOffset,		BOTH),
		CASE_SPEC(usampler1d_vec4,				FUNCTION_TEXTUREPROJLOD,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	false,	-1.0f,	9.0f,	true,	IVec3(7,  0, 0),	tex1DMipmapUint,		evalTexture1DProjLodOffset,		BOTH),

		CASE_SPEC(sampler2dshadow,				FUNCTION_TEXTUREPROJLOD,	Vec4( 0.2f, 0.6f,  0.0f,  1.5f),	Vec4(-2.25f, -3.45f, 1.5f,  1.5f),	false,	-1.0f,	9.0f,	true,	IVec3(-8, 7, 0),	tex2DMipmapShadow,		evalTexture2DShadowProjLodOffset,	BOTH),
		CASE_SPEC(sampler1dshadow,				FUNCTION_TEXTUREPROJLOD,	Vec4( 0.2f, 0.0f,  0.0f,  1.5f),	Vec4(-2.25f,  0.0f,  1.5f,  1.5f),	false,	-1.0f,	9.0f,	true,	IVec3(7,  0, 0),	tex1DMipmapShadow,		evalTexture1DShadowProjLodOffset,	BOTH),
	};
	createCaseGroup(this, "textureprojlodoffset", "textureProjLodOffset() Tests", textureProjLodOffsetCases, DE_LENGTH_OF_ARRAY(textureProjLodOffsetCases));

	// textureGrad() cases
	// \note Only one of dudx, dudy, dvdx, dvdy is non-zero since spec allows approximating p from derivates by various methods.
	static const TexFuncCaseSpec textureGradCases[] =
	{
		//		  Name							Function				MinCoord							MaxCoord							MinDx						MaxDx						MinDy						MaxDy						Offset?	Offset		Format					EvalFunc				Flags
		GRAD_CASE_SPEC(sampler2d_fixed,			FUNCTION_TEXTUREGRAD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex2DMipmapFixed,		evalTexture2DGrad,		BOTH),
		GRAD_CASE_SPEC(sampler2d_float,			FUNCTION_TEXTUREGRAD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f, -0.2f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex2DMipmapFloat,		evalTexture2DGrad,		BOTH),
		GRAD_CASE_SPEC(isampler2d,				FUNCTION_TEXTUREGRAD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	false,	IVec3(0),	tex2DMipmapInt,			evalTexture2DGrad,		BOTH),
		GRAD_CASE_SPEC(usampler2d,				FUNCTION_TEXTUREGRAD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.2f,  0.0f),	false,	IVec3(0),	tex2DMipmapUint,		evalTexture2DGrad,		BOTH),

		GRAD_CASE_SPEC(samplercube_fixed,		FUNCTION_TEXTUREGRAD,	Vec4(-1.0f, -1.0f,  1.01f,  0.0f),	Vec4( 1.0f,  1.0f,  1.01f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	texCubeMipmapFixed,		evalTextureCubeGrad,	BOTH),
		GRAD_CASE_SPEC(samplercube_float,		FUNCTION_TEXTUREGRAD,	Vec4(-1.0f, -1.0f, -1.01f,  0.0f),	Vec4( 1.0f,  1.0f, -1.01f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f, -0.2f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	texCubeMipmapFloat,		evalTextureCubeGrad,	BOTH),
		GRAD_CASE_SPEC(isamplercube,			FUNCTION_TEXTUREGRAD,	Vec4(-1.0f, -1.0f,  1.01f,  0.0f),	Vec4( 1.0f,  1.0f,  1.01f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	false,	IVec3(0),	texCubeMipmapInt,		evalTextureCubeGrad,	BOTH),
		GRAD_CASE_SPEC(usamplercube,			FUNCTION_TEXTUREGRAD,	Vec4(-1.0f, -1.0f, -1.01f,  0.0f),	Vec4( 1.0f,  1.0f, -1.01f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.2f,  0.0f),	false,	IVec3(0),	texCubeMipmapUint,		evalTextureCubeGrad,	BOTH),

		GRAD_CASE_SPEC(sampler2darray_fixed,	FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex2DArrayMipmapFixed,	evalTexture2DArrayGrad,	BOTH),
		GRAD_CASE_SPEC(sampler2darray_float,	FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f, -0.2f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex2DArrayMipmapFloat,	evalTexture2DArrayGrad,	BOTH),
		GRAD_CASE_SPEC(isampler2darray,			FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	false,	IVec3(0),	tex2DArrayMipmapInt,	evalTexture2DArrayGrad,	BOTH),
		GRAD_CASE_SPEC(usampler2darray,			FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.2f,  0.0f),	false,	IVec3(0),	tex2DArrayMipmapUint,	evalTexture2DArrayGrad,	BOTH),

		GRAD_CASE_SPEC(sampler3d_fixed,			FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex3DMipmapFixed,		evalTexture3DGrad,		BOTH),
		GRAD_CASE_SPEC(sampler3d_float,			FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f, -0.2f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex3DMipmapFloat,		evalTexture3DGrad,		VERTEX),
		GRAD_CASE_SPEC(sampler3d_float,			FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.2f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex3DMipmapFloat,		evalTexture3DGrad,		FRAGMENT),
		GRAD_CASE_SPEC(isampler3d,				FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	false,	IVec3(0),	tex3DMipmapInt,			evalTexture3DGrad,		BOTH),
		GRAD_CASE_SPEC(usampler3d,				FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.2f,  0.0f),	false,	IVec3(0),	tex3DMipmapUint,		evalTexture3DGrad,		VERTEX),
		GRAD_CASE_SPEC(usampler3d,				FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f, -0.2f),	false,	IVec3(0),	tex3DMipmapUint,		evalTexture3DGrad,		FRAGMENT),

		GRAD_CASE_SPEC(sampler1d_fixed,			FUNCTION_TEXTUREGRAD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DMipmapFixed,		evalTexture1DGrad,		BOTH),
		GRAD_CASE_SPEC(sampler1d_float,			FUNCTION_TEXTUREGRAD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DMipmapFloat,		evalTexture1DGrad,		BOTH),
		GRAD_CASE_SPEC(isampler1d,				FUNCTION_TEXTUREGRAD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DMipmapInt,			evalTexture1DGrad,		BOTH),
		GRAD_CASE_SPEC(usampler1d,				FUNCTION_TEXTUREGRAD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DMipmapUint,		evalTexture1DGrad,		BOTH),

		GRAD_CASE_SPEC(sampler1darray_fixed,	FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DArrayMipmapFixed,	evalTexture1DArrayGrad,	BOTH),
		GRAD_CASE_SPEC(sampler1darray_float,	FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DArrayMipmapFloat,	evalTexture1DArrayGrad,	BOTH),
		GRAD_CASE_SPEC(isampler1darray,			FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DArrayMipmapInt,	evalTexture1DArrayGrad,	BOTH),
		GRAD_CASE_SPEC(usampler1darray,			FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DArrayMipmapUint,	evalTexture1DArrayGrad,	BOTH),

		GRAD_CASE_SPEC(samplercubearray_fixed,	FUNCTION_TEXTUREGRAD,	Vec4(-1.0f, -1.0f,  1.01f, -0.5f),	Vec4( 1.0f,  1.0f,  1.01f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	texCubeArrayMipmapFixed,	evalTextureCubeArrayGrad,	BOTH),
		GRAD_CASE_SPEC(samplercubearray_float,	FUNCTION_TEXTUREGRAD,	Vec4(-1.0f, -1.0f, -1.01f, -0.5f),	Vec4( 1.0f,  1.0f, -1.01f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f, -0.2f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	texCubeArrayMipmapFloat,	evalTextureCubeArrayGrad,	BOTH),
		GRAD_CASE_SPEC(isamplercubearray,		FUNCTION_TEXTUREGRAD,	Vec4(-1.0f, -1.0f,  1.01f, -0.5f),	Vec4( 1.0f,  1.0f,  1.01f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	false,	IVec3(0),	texCubeArrayMipmapInt,		evalTextureCubeArrayGrad,	BOTH),
		GRAD_CASE_SPEC(usamplercubearray,		FUNCTION_TEXTUREGRAD,	Vec4(-1.0f, -1.0f, -1.01f, -0.5f),	Vec4( 1.0f,  1.0f, -1.01f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.2f,  0.0f),	false,	IVec3(0),	texCubeArrayMipmapUint,		evalTextureCubeArrayGrad,	BOTH),

		GRAD_CASE_SPEC(sampler2dshadow,			FUNCTION_TEXTUREGRAD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  1.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex2DMipmapShadow,		evalTexture2DShadowGrad,		BOTH),
		GRAD_CASE_SPEC(samplercubeshadow,		FUNCTION_TEXTUREGRAD,	Vec4(-1.0f, -1.0f,  1.01f,  0.0f),	Vec4( 1.0f,  1.0f,  1.01f,  1.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.2f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	texCubeMipmapShadow,	evalTextureCubeShadowGrad,		BOTH),
		GRAD_CASE_SPEC(sampler2darrayshadow,	FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  1.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	false,	IVec3(0),	tex2DArrayMipmapShadow,	evalTexture2DArrayShadowGrad,	VERTEX),
		GRAD_CASE_SPEC(sampler2darrayshadow,	FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  1.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f, -0.2f,  0.0f),	false,	IVec3(0),	tex2DArrayMipmapShadow,	evalTexture2DArrayShadowGrad,	FRAGMENT),
		GRAD_CASE_SPEC(sampler1dshadow,			FUNCTION_TEXTUREGRAD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  1.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DMipmapShadow,		evalTexture1DShadowGrad,		BOTH),
		GRAD_CASE_SPEC(sampler1darrayshadow,	FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  1.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DArrayMipmapShadow,	evalTexture1DArrayShadowGrad,	VERTEX),
		GRAD_CASE_SPEC(sampler1darrayshadow,	FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  1.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DArrayMipmapShadow,	evalTexture1DArrayShadowGrad,	FRAGMENT),
	};
	createCaseGroup(this, "texturegrad", "textureGrad() Tests", textureGradCases, DE_LENGTH_OF_ARRAY(textureGradCases));

	// textureGradOffset() cases
	static const TexFuncCaseSpec textureGradOffsetCases[] =
	{
		//		  Name							Function				MinCoord							MaxCoord							MinDx						MaxDx						MinDy						MaxDy						Offset?	Offset				Format					EvalFunc							Flags
		GRAD_CASE_SPEC(sampler2d_fixed,			FUNCTION_TEXTUREGRAD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 0),	tex2DMipmapFixed,		evalTexture2DGradOffset,			BOTH),
		GRAD_CASE_SPEC(sampler2d_float,			FUNCTION_TEXTUREGRAD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f, -0.2f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(7, -8, 0),	tex2DMipmapFloat,		evalTexture2DGradOffset,			BOTH),
		GRAD_CASE_SPEC(isampler2d,				FUNCTION_TEXTUREGRAD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 0),	tex2DMipmapInt,			evalTexture2DGradOffset,			BOTH),
		GRAD_CASE_SPEC(usampler2d,				FUNCTION_TEXTUREGRAD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.2f,  0.0f),	true,	IVec3(7, -8, 0),	tex2DMipmapUint,		evalTexture2DGradOffset,			BOTH),

		GRAD_CASE_SPEC(sampler2darray_fixed,	FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 0),	tex2DArrayMipmapFixed,	evalTexture2DArrayGradOffset,		BOTH),
		GRAD_CASE_SPEC(sampler2darray_float,	FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f, -0.2f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(7, -8, 0),	tex2DArrayMipmapFloat,	evalTexture2DArrayGradOffset,		BOTH),
		GRAD_CASE_SPEC(isampler2darray,			FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 0),	tex2DArrayMipmapInt,	evalTexture2DArrayGradOffset,		BOTH),
		GRAD_CASE_SPEC(usampler2darray,			FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.2f,  0.0f),	true,	IVec3(7, -8, 0),	tex2DArrayMipmapUint,	evalTexture2DArrayGradOffset,		BOTH),

		GRAD_CASE_SPEC(sampler3d_fixed,			FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 3),	tex3DMipmapFixed,		evalTexture3DGradOffset,			BOTH),
		GRAD_CASE_SPEC(sampler3d_float,			FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f, -0.2f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(7, 3, -8),	tex3DMipmapFloat,		evalTexture3DGradOffset,			VERTEX),
		GRAD_CASE_SPEC(sampler3d_float,			FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.2f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(3, -8, 7),	tex3DMipmapFloat,		evalTexture3DGradOffset,			FRAGMENT),
		GRAD_CASE_SPEC(isampler3d,				FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 3),	tex3DMipmapInt,			evalTexture3DGradOffset,			BOTH),
		GRAD_CASE_SPEC(usampler3d,				FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.2f,  0.0f),	true,	IVec3(7, 3, -8),	tex3DMipmapUint,		evalTexture3DGradOffset,			VERTEX),
		GRAD_CASE_SPEC(usampler3d,				FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -1.4f,  0.1f,  0.0f),	Vec4( 1.5f,  2.3f,  2.3f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f, -0.2f),	true,	IVec3(3, -8, 7),	tex3DMipmapUint,		evalTexture3DGradOffset,			FRAGMENT),

		GRAD_CASE_SPEC(sampler1d_fixed,			FUNCTION_TEXTUREGRAD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(-8, 0, 0),	tex1DMipmapFixed,		evalTexture1DGradOffset,			BOTH),
		GRAD_CASE_SPEC(sampler1d_float,			FUNCTION_TEXTUREGRAD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(7,  0, 0),	tex1DMipmapFloat,		evalTexture1DGradOffset,			BOTH),
		GRAD_CASE_SPEC(isampler1d,				FUNCTION_TEXTUREGRAD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	true,	IVec3(-8, 0, 0),	tex1DMipmapInt,			evalTexture1DGradOffset,			BOTH),
		GRAD_CASE_SPEC(usampler1d,				FUNCTION_TEXTUREGRAD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	true,	IVec3(7,  0, 0),	tex1DMipmapUint,		evalTexture1DGradOffset,			BOTH),

		GRAD_CASE_SPEC(sampler1darray_fixed,	FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(-8, 0, 0),	tex1DArrayMipmapFixed,	evalTexture1DArrayGradOffset,		BOTH),
		GRAD_CASE_SPEC(sampler1darray_float,	FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(7,  0, 0),	tex1DArrayMipmapFloat,	evalTexture1DArrayGradOffset,		BOTH),
		GRAD_CASE_SPEC(isampler1darray,			FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	true,	IVec3(-8, 0, 0),	tex1DArrayMipmapInt,	evalTexture1DArrayGradOffset,		BOTH),
		GRAD_CASE_SPEC(usampler1darray,			FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.5f,   0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	true,	IVec3(7,  0, 0),	tex1DArrayMipmapUint,	evalTexture1DArrayGradOffset,		BOTH),

		GRAD_CASE_SPEC(sampler2dshadow,			FUNCTION_TEXTUREGRAD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  1.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 0),	tex2DMipmapShadow,		evalTexture2DShadowGradOffset,		VERTEX),
		GRAD_CASE_SPEC(sampler2dshadow,			FUNCTION_TEXTUREGRAD,	Vec4(-0.2f, -0.4f,  0.0f,  0.0f),	Vec4( 1.5f,  2.3f,  1.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.2f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(7, -8, 0),	tex2DMipmapShadow,		evalTexture2DShadowGradOffset,		FRAGMENT),
		GRAD_CASE_SPEC(sampler2darrayshadow,	FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  1.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 0),	tex2DArrayMipmapShadow,	evalTexture2DArrayShadowGradOffset,	VERTEX),
		GRAD_CASE_SPEC(sampler2darrayshadow,	FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.4f,  -0.5f,  0.0f),	Vec4( 1.5f,  2.3f,  3.5f,  1.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f, -0.2f,  0.0f),	true,	IVec3(7, -8, 0),	tex2DArrayMipmapShadow,	evalTexture2DArrayShadowGradOffset,	FRAGMENT),
		GRAD_CASE_SPEC(sampler1dshadow,			FUNCTION_TEXTUREGRAD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  1.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(-8, 0, 0),	tex1DMipmapShadow,		evalTexture1DShadowGradOffset,		VERTEX),
		GRAD_CASE_SPEC(sampler1dshadow,			FUNCTION_TEXTUREGRAD,	Vec4(-0.2f,  0.0f,  0.0f,  0.0f),	Vec4( 1.5f,  0.0f,  1.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(7,  0, 0),	tex1DMipmapShadow,		evalTexture1DShadowGradOffset,		FRAGMENT),
		GRAD_CASE_SPEC(sampler1darrayshadow,	FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  1.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	true,	IVec3(-8, 0, 0),	tex1DArrayMipmapShadow,	evalTexture1DArrayShadowGradOffset,	VERTEX),
		GRAD_CASE_SPEC(sampler1darrayshadow,	FUNCTION_TEXTUREGRAD,	Vec4(-1.2f, -0.5f,  0.0f,  0.0f),	Vec4( 1.5f,  3.5f,  1.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	true,	IVec3(7,  0, 0),	tex1DArrayMipmapShadow,	evalTexture1DArrayShadowGradOffset,	FRAGMENT),
	};
	createCaseGroup(this, "texturegradoffset", "textureGradOffset() Tests", textureGradOffsetCases, DE_LENGTH_OF_ARRAY(textureGradOffsetCases));

	// textureProjGrad() cases
	static const TexFuncCaseSpec textureProjGradCases[] =
	{
		//		  Name							Function					MinCoord							MaxCoord							MinDx						MaxDx						MinDy						MaxDy						Offset?	Offset		Format					EvalFunc					Flags
		GRAD_CASE_SPEC(sampler2d_vec3_fixed,	FUNCTION_TEXTUREPROJGRAD3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex2DMipmapFixed,		evalTexture2DProjGrad3,		BOTH),
		GRAD_CASE_SPEC(sampler2d_vec3_float,	FUNCTION_TEXTUREPROJGRAD3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f, -0.2f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex2DMipmapFloat,		evalTexture2DProjGrad3,		BOTH),
		GRAD_CASE_SPEC(isampler2d_vec3,			FUNCTION_TEXTUREPROJGRAD3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	false,	IVec3(0),	tex2DMipmapInt,			evalTexture2DProjGrad3,		BOTH),
		GRAD_CASE_SPEC(usampler2d_vec3,			FUNCTION_TEXTUREPROJGRAD3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.2f,  0.0f),	false,	IVec3(0),	tex2DMipmapUint,		evalTexture2DProjGrad3,		BOTH),

		GRAD_CASE_SPEC(sampler2d_vec4_fixed,	FUNCTION_TEXTUREPROJGRAD,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex2DMipmapFixed,		evalTexture2DProjGrad,		BOTH),
		GRAD_CASE_SPEC(sampler2d_vec4_float,	FUNCTION_TEXTUREPROJGRAD,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f, -0.2f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex2DMipmapFloat,		evalTexture2DProjGrad,		BOTH),
		GRAD_CASE_SPEC(isampler2d_vec4,			FUNCTION_TEXTUREPROJGRAD,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	false,	IVec3(0),	tex2DMipmapInt,			evalTexture2DProjGrad,		BOTH),
		GRAD_CASE_SPEC(usampler2d_vec4,			FUNCTION_TEXTUREPROJGRAD,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.2f,  0.0f),	false,	IVec3(0),	tex2DMipmapUint,		evalTexture2DProjGrad,		BOTH),

		GRAD_CASE_SPEC(sampler3d_fixed,			FUNCTION_TEXTUREPROJGRAD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex3DMipmapFixed,		evalTexture3DProjGrad,		BOTH),
		GRAD_CASE_SPEC(sampler3d_float,			FUNCTION_TEXTUREPROJGRAD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f, -0.2f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex3DMipmapFloat,		evalTexture3DProjGrad,		VERTEX),
		GRAD_CASE_SPEC(sampler3d_float,			FUNCTION_TEXTUREPROJGRAD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.2f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex3DMipmapFloat,		evalTexture3DProjGrad,		FRAGMENT),
		GRAD_CASE_SPEC(isampler3d,				FUNCTION_TEXTUREPROJGRAD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	false,	IVec3(0),	tex3DMipmapInt,			evalTexture3DProjGrad,		BOTH),
		GRAD_CASE_SPEC(usampler3d,				FUNCTION_TEXTUREPROJGRAD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.2f,  0.0f),	false,	IVec3(0),	tex3DMipmapUint,		evalTexture3DProjGrad,		VERTEX),
		GRAD_CASE_SPEC(usampler3d,				FUNCTION_TEXTUREPROJGRAD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f, -0.2f),	false,	IVec3(0),	tex3DMipmapUint,		evalTexture3DProjGrad,		FRAGMENT),

		GRAD_CASE_SPEC(sampler1d_vec2_fixed,	FUNCTION_TEXTUREPROJGRAD2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DMipmapFixed,		evalTexture1DProjGrad2,		BOTH),
		GRAD_CASE_SPEC(sampler1d_vec2_float,	FUNCTION_TEXTUREPROJGRAD2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DMipmapFloat,		evalTexture1DProjGrad2,		BOTH),
		GRAD_CASE_SPEC(isampler1d_vec2,			FUNCTION_TEXTUREPROJGRAD2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DMipmapInt,			evalTexture1DProjGrad2,		BOTH),
		GRAD_CASE_SPEC(usampler1d_vec2,			FUNCTION_TEXTUREPROJGRAD2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DMipmapUint,		evalTexture1DProjGrad2,		BOTH),

		GRAD_CASE_SPEC(sampler1d_vec4_fixed,	FUNCTION_TEXTUREPROJGRAD,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DMipmapFixed,		evalTexture1DProjGrad,		BOTH),
		GRAD_CASE_SPEC(sampler1d_vec4_float,	FUNCTION_TEXTUREPROJGRAD,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DMipmapFloat,		evalTexture1DProjGrad,		BOTH),
		GRAD_CASE_SPEC(isampler1d_vec4,			FUNCTION_TEXTUREPROJGRAD,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DMipmapInt,			evalTexture1DProjGrad,		BOTH),
		GRAD_CASE_SPEC(usampler1d_vec4,			FUNCTION_TEXTUREPROJGRAD,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DMipmapUint,		evalTexture1DProjGrad,		BOTH),

		GRAD_CASE_SPEC(sampler2dshadow,			FUNCTION_TEXTUREPROJGRAD,	Vec4( 0.2f, 0.6f,  0.0f,  -1.5f),	Vec4(-2.25f, -3.45f, -1.5f, -1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex2DMipmapShadow,		evalTexture2DShadowProjGrad,	VERTEX),
		GRAD_CASE_SPEC(sampler2dshadow,			FUNCTION_TEXTUREPROJGRAD,	Vec4( 0.2f, 0.6f,  0.0f,  -1.5f),	Vec4(-2.25f, -3.45f, -1.5f, -1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f, -0.2f,  0.0f),	false,	IVec3(0),	tex2DMipmapShadow,		evalTexture2DShadowProjGrad,	FRAGMENT),
		GRAD_CASE_SPEC(sampler1dshadow,			FUNCTION_TEXTUREPROJGRAD,	Vec4( 0.2f, 0.0f,  0.0f,  -1.5f),	Vec4(-2.25f,   0.0f, -1.5f, -1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DMipmapShadow,		evalTexture1DShadowProjGrad,	VERTEX),
		GRAD_CASE_SPEC(sampler1dshadow,			FUNCTION_TEXTUREPROJGRAD,	Vec4( 0.2f, 0.0f,  0.0f,  -1.5f),	Vec4(-2.25f,   0.0f, -1.5f, -1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	false,	IVec3(0),	tex1DMipmapShadow,		evalTexture1DShadowProjGrad,	FRAGMENT),
	};
	createCaseGroup(this, "textureprojgrad", "textureProjGrad() Tests", textureProjGradCases, DE_LENGTH_OF_ARRAY(textureProjGradCases));

	// textureProjGradOffset() cases
	static const TexFuncCaseSpec textureProjGradOffsetCases[] =
	{
		//		  Name							Function					MinCoord							MaxCoord							MinDx						MaxDx						MinDy						MaxDy						Offset?	Offset				Format					EvalFunc							Flags
		GRAD_CASE_SPEC(sampler2d_vec3_fixed,	FUNCTION_TEXTUREPROJGRAD3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 0),	tex2DMipmapFixed,		evalTexture2DProjGrad3Offset,		BOTH),
		GRAD_CASE_SPEC(sampler2d_vec3_float,	FUNCTION_TEXTUREPROJGRAD3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f, -0.2f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(7, -8, 0),	tex2DMipmapFloat,		evalTexture2DProjGrad3Offset,		BOTH),
		GRAD_CASE_SPEC(isampler2d_vec3,			FUNCTION_TEXTUREPROJGRAD3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 0),	tex2DMipmapInt,			evalTexture2DProjGrad3Offset,		BOTH),
		GRAD_CASE_SPEC(usampler2d_vec3,			FUNCTION_TEXTUREPROJGRAD3,	Vec4(-0.3f, -0.6f,  1.5f,  0.0f),	Vec4(2.25f, 3.45f,  1.5f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.2f,  0.0f),	true,	IVec3(7, -8, 0),	tex2DMipmapUint,		evalTexture2DProjGrad3Offset,		BOTH),

		GRAD_CASE_SPEC(sampler2d_vec4_fixed,	FUNCTION_TEXTUREPROJGRAD,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 0),	tex2DMipmapFixed,		evalTexture2DProjGradOffset,		BOTH),
		GRAD_CASE_SPEC(sampler2d_vec4_float,	FUNCTION_TEXTUREPROJGRAD,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f, -0.2f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(7, -8, 0),	tex2DMipmapFloat,		evalTexture2DProjGradOffset,		BOTH),
		GRAD_CASE_SPEC(isampler2d_vec4,			FUNCTION_TEXTUREPROJGRAD,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 0),	tex2DMipmapInt,			evalTexture2DProjGradOffset,		BOTH),
		GRAD_CASE_SPEC(usampler2d_vec4,			FUNCTION_TEXTUREPROJGRAD,	Vec4(-0.3f, -0.6f,  0.0f,  1.5f),	Vec4(2.25f, 3.45f,  0.0f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.2f,  0.0f),	true,	IVec3(7, -8, 0),	tex2DMipmapUint,		evalTexture2DProjGradOffset,		BOTH),

		GRAD_CASE_SPEC(sampler3d_fixed,			FUNCTION_TEXTUREPROJGRAD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 3),	tex3DMipmapFixed,		evalTexture3DProjGradOffset,		BOTH),
		GRAD_CASE_SPEC(sampler3d_float,			FUNCTION_TEXTUREPROJGRAD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f, -0.2f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(7, 3, -8),	tex3DMipmapFloat,		evalTexture3DProjGradOffset,		VERTEX),
		GRAD_CASE_SPEC(sampler3d_float,			FUNCTION_TEXTUREPROJGRAD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.2f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(3, -8, 7),	tex3DMipmapFloat,		evalTexture3DProjGradOffset,		FRAGMENT),
		GRAD_CASE_SPEC(isampler3d,				FUNCTION_TEXTUREPROJGRAD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 3),	tex3DMipmapInt,			evalTexture3DProjGradOffset,		BOTH),
		GRAD_CASE_SPEC(usampler3d,				FUNCTION_TEXTUREPROJGRAD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.2f,  0.0f),	true,	IVec3(7, 3, -8),	tex3DMipmapUint,		evalTexture3DProjGradOffset,		VERTEX),
		GRAD_CASE_SPEC(usampler3d,				FUNCTION_TEXTUREPROJGRAD,	Vec4(0.9f, 1.05f, -0.08f, -0.75f),	Vec4(-1.13f, -1.7f, -1.7f, -0.75f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f, -0.2f),	true,	IVec3(3, -8, 7),	tex3DMipmapUint,		evalTexture3DProjGradOffset,		FRAGMENT),

		GRAD_CASE_SPEC(sampler1d_vec2_fixed,	FUNCTION_TEXTUREPROJGRAD2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 0),	tex1DMipmapFixed,		evalTexture1DProjGrad2Offset,		BOTH),
		GRAD_CASE_SPEC(sampler1d_vec2_float,	FUNCTION_TEXTUREPROJGRAD2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(7, -8, 0),	tex1DMipmapFloat,		evalTexture1DProjGrad2Offset,		BOTH),
		GRAD_CASE_SPEC(isampler1d_vec2,			FUNCTION_TEXTUREPROJGRAD2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 0),	tex1DMipmapInt,			evalTexture1DProjGrad2Offset,		BOTH),
		GRAD_CASE_SPEC(usampler1d_vec2,			FUNCTION_TEXTUREPROJGRAD2,	Vec4(-0.3f,  1.5f,  0.0f,  0.0f),	Vec4(2.25f,  1.5f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	true,	IVec3(7, -8, 0),	tex1DMipmapUint,		evalTexture1DProjGrad2Offset,		BOTH),

		GRAD_CASE_SPEC(sampler1d_vec4_fixed,	FUNCTION_TEXTUREPROJGRAD,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 0),	tex1DMipmapFixed,		evalTexture1DProjGradOffset,		BOTH),
		GRAD_CASE_SPEC(sampler1d_vec4_float,	FUNCTION_TEXTUREPROJGRAD,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(7, -8, 0),	tex1DMipmapFloat,		evalTexture1DProjGradOffset,		BOTH),
		GRAD_CASE_SPEC(isampler1d_vec4,			FUNCTION_TEXTUREPROJGRAD,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 0),	tex1DMipmapInt,			evalTexture1DProjGradOffset,		BOTH),
		GRAD_CASE_SPEC(usampler1d_vec4,			FUNCTION_TEXTUREPROJGRAD,	Vec4(-0.3f,  0.0f,  0.0f,  1.5f),	Vec4(2.25f,  0.0f,  0.0f,  1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	true,	IVec3(7, -8, 0),	tex1DMipmapUint,		evalTexture1DProjGradOffset,		BOTH),

		GRAD_CASE_SPEC(sampler2dshadow,			FUNCTION_TEXTUREPROJGRAD,	Vec4( 0.2f, 0.6f,  0.0f,  -1.5f),	Vec4(-2.25f, -3.45f, -1.5f, -1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 0),	tex2DMipmapShadow,		evalTexture2DShadowProjGradOffset,	VERTEX),
		GRAD_CASE_SPEC(sampler2dshadow,			FUNCTION_TEXTUREPROJGRAD,	Vec4( 0.2f, 0.6f,  0.0f,  -1.5f),	Vec4(-2.25f, -3.45f, -1.5f, -1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f, -0.2f,  0.0f),	true,	IVec3(7, -8, 0),	tex2DMipmapShadow,		evalTexture2DShadowProjGradOffset,	FRAGMENT),
		GRAD_CASE_SPEC(sampler1dshadow,			FUNCTION_TEXTUREPROJGRAD,	Vec4( 0.2f, 0.0f,  0.0f,  -1.5f),	Vec4(-2.25f,   0.0f, -1.5f, -1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.2f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	true,	IVec3(-8, 7, 0),	tex1DMipmapShadow,		evalTexture1DShadowProjGradOffset,	VERTEX),
		GRAD_CASE_SPEC(sampler1dshadow,			FUNCTION_TEXTUREPROJGRAD,	Vec4( 0.2f, 0.0f,  0.0f,  -1.5f),	Vec4(-2.25f,   0.0f, -1.5f, -1.5f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3( 0.0f,  0.0f,  0.0f),	Vec3(-0.2f,  0.0f,  0.0f),	true,	IVec3(7, -8, 0),	tex1DMipmapShadow,		evalTexture1DShadowProjGradOffset,	FRAGMENT),
	};
	createCaseGroup(this, "textureprojgradoffset", "textureProjGradOffset() Tests", textureProjGradOffsetCases, DE_LENGTH_OF_ARRAY(textureProjGradOffsetCases));

	// texelFetch() cases
	// \note Level is constant across quad
	static const TexFuncCaseSpec texelFetchCases[] =
	{
		//		  Name							Function				MinCoord							MaxCoord						Bias?	MinLod	MaxLod	Offset?	Offset		Format						EvalFunc				Flags
		CASE_SPEC(sampler2d_fixed,				FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4(255.9f, 255.9f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DTexelFetchFixed,		evalTexelFetch2D,		BOTH),
		CASE_SPEC(sampler2d_float,				FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4(127.9f, 127.9f,  0.0f,  0.0f),	false,	1.0f,	1.0f,	false,	IVec3(0),	tex2DTexelFetchFloat,		evalTexelFetch2D,		BOTH),
		CASE_SPEC(isampler2d,					FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4( 63.9f,  63.9f,  0.0f,  0.0f),	false,	2.0f,	2.0f,	false,	IVec3(0),	tex2DTexelFetchInt,			evalTexelFetch2D,		BOTH),
		CASE_SPEC(usampler2d,					FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4( 15.9f,  15.9f,  0.0f,  0.0f),	false,	4.0f,	4.0f,	false,	IVec3(0),	tex2DTexelFetchUint,		evalTexelFetch2D,		BOTH),

		CASE_SPEC(sampler2darray_fixed,			FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4(127.9f, 127.9f,  3.9f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex2DArrayTexelFetchFixed,	evalTexelFetch2DArray,	BOTH),
		CASE_SPEC(sampler2darray_float,			FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4( 63.9f,  63.9f,  3.9f,  0.0f),	false,	1.0f,	1.0f,	false,	IVec3(0),	tex2DArrayTexelFetchFloat,	evalTexelFetch2DArray,	BOTH),
		CASE_SPEC(isampler2darray,				FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4( 31.9f,  31.9f,  3.9f,  0.0f),	false,	2.0f,	2.0f,	false,	IVec3(0),	tex2DArrayTexelFetchInt,	evalTexelFetch2DArray,	BOTH),
		CASE_SPEC(usampler2darray,				FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4( 15.9f,  15.9f,  3.9f,  0.0f),	false,	3.0f,	3.0f,	false,	IVec3(0),	tex2DArrayTexelFetchUint,	evalTexelFetch2DArray,	BOTH),

		CASE_SPEC(sampler3d_fixed,				FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4(63.9f,  31.9f,  31.9f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex3DTexelFetchFixed,		evalTexelFetch3D,		BOTH),
		CASE_SPEC(sampler3d_float,				FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4(31.9f,  15.9f,  15.9f,  0.0f),	false,	1.0f,	1.0f,	false,	IVec3(0),	tex3DTexelFetchFloat,		evalTexelFetch3D,		BOTH),
		CASE_SPEC(isampler3d,					FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4(15.9f,   7.9f,   7.9f,  0.0f),	false,	2.0f,	2.0f,	false,	IVec3(0),	tex3DTexelFetchInt,			evalTexelFetch3D,		BOTH),
		CASE_SPEC(usampler3d,					FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4(63.9f,  31.9f,  31.9f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex3DTexelFetchUint,		evalTexelFetch3D,		BOTH),

		CASE_SPEC(sampler1d_fixed,				FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4(255.9f,   0.0f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DTexelFetchFixed,		evalTexelFetch1D,		BOTH),
		CASE_SPEC(sampler1d_float,				FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4(127.9f,   0.0f,  0.0f,  0.0f),	false,	1.0f,	1.0f,	false,	IVec3(0),	tex1DTexelFetchFloat,		evalTexelFetch1D,		BOTH),
		CASE_SPEC(isampler1d,					FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4( 63.9f,   0.0f,  0.0f,  0.0f),	false,	2.0f,	2.0f,	false,	IVec3(0),	tex1DTexelFetchInt,			evalTexelFetch1D,		BOTH),
		CASE_SPEC(usampler1d,					FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4( 15.9f,   0.0f,  0.0f,  0.0f),	false,	4.0f,	4.0f,	false,	IVec3(0),	tex1DTexelFetchUint,		evalTexelFetch1D,		BOTH),

		CASE_SPEC(sampler1darray_fixed,			FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4(255.9f,   3.9f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	false,	IVec3(0),	tex1DArrayTexelFetchFixed,	evalTexelFetch1DArray,	BOTH),
		CASE_SPEC(sampler1darray_float,			FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4(127.9f,   3.9f,  0.0f,  0.0f),	false,	1.0f,	1.0f,	false,	IVec3(0),	tex1DArrayTexelFetchFloat,	evalTexelFetch1DArray,	BOTH),
		CASE_SPEC(isampler1darray,				FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4( 63.9f,   3.9f,  0.0f,  0.0f),	false,	2.0f,	2.0f,	false,	IVec3(0),	tex1DArrayTexelFetchInt,	evalTexelFetch1DArray,	BOTH),
		CASE_SPEC(usampler1darray,				FUNCTION_TEXELFETCH,	Vec4(0.0f, 0.0f, 0.0f, 0.0f),	Vec4( 15.9f,   3.9f,  0.0f,  0.0f),	false,	4.0f,	4.0f,	false,	IVec3(0),	tex1DArrayTexelFetchUint,	evalTexelFetch1DArray,	BOTH),
	};
	createCaseGroup(this, "texelfetch", "texelFetch() Tests", texelFetchCases, DE_LENGTH_OF_ARRAY(texelFetchCases));

	// texelFetchOffset() cases
	static const TexFuncCaseSpec texelFetchOffsetCases[] =
	{
		//		  Name							Function				MinCoord							MaxCoord						Bias?	MinLod	MaxLod	Offset?	Offset		Format						EvalFunc				Flags
		CASE_SPEC(sampler2d_fixed,				FUNCTION_TEXELFETCH,	Vec4( 8.0f, -7.0f, 0.0f, 0.0f),	Vec4(263.9f, 248.9f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DTexelFetchFixed,		evalTexelFetch2D,		BOTH),
		CASE_SPEC(sampler2d_float,				FUNCTION_TEXELFETCH,	Vec4(-7.0f,  8.0f, 0.0f, 0.0f),	Vec4(120.9f, 135.9f,  0.0f,  0.0f),	false,	1.0f,	1.0f,	true,	IVec3(7, -8, 0),	tex2DTexelFetchFloat,		evalTexelFetch2D,		BOTH),
		CASE_SPEC(isampler2d,					FUNCTION_TEXELFETCH,	Vec4( 8.0f, -7.0f, 0.0f, 0.0f),	Vec4( 71.9f,  56.9f,  0.0f,  0.0f),	false,	2.0f,	2.0f,	true,	IVec3(-8, 7, 0),	tex2DTexelFetchInt,			evalTexelFetch2D,		BOTH),
		CASE_SPEC(usampler2d,					FUNCTION_TEXELFETCH,	Vec4(-7.0f,  8.0f, 0.0f, 0.0f),	Vec4(  8.9f,  23.9f,  0.0f,  0.0f),	false,	4.0f,	4.0f,	true,	IVec3(7, -8, 0),	tex2DTexelFetchUint,		evalTexelFetch2D,		BOTH),

		CASE_SPEC(sampler2darray_fixed,			FUNCTION_TEXELFETCH,	Vec4( 8.0f, -7.0f, 0.0f, 0.0f),	Vec4(135.9f, 120.9f,  3.9f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 0),	tex2DArrayTexelFetchFixed,	evalTexelFetch2DArray,	BOTH),
		CASE_SPEC(sampler2darray_float,			FUNCTION_TEXELFETCH,	Vec4(-7.0f,  8.0f, 0.0f, 0.0f),	Vec4( 56.9f,  71.9f,  3.9f,  0.0f),	false,	1.0f,	1.0f,	true,	IVec3(7, -8, 0),	tex2DArrayTexelFetchFloat,	evalTexelFetch2DArray,	BOTH),
		CASE_SPEC(isampler2darray,				FUNCTION_TEXELFETCH,	Vec4( 8.0f, -7.0f, 0.0f, 0.0f),	Vec4( 39.9f,  24.9f,  3.9f,  0.0f),	false,	2.0f,	2.0f,	true,	IVec3(-8, 7, 0),	tex2DArrayTexelFetchInt,	evalTexelFetch2DArray,	BOTH),
		CASE_SPEC(usampler2darray,				FUNCTION_TEXELFETCH,	Vec4(-7.0f,  8.0f, 0.0f, 0.0f),	Vec4(  8.9f,  23.9f,  3.9f,  0.0f),	false,	3.0f,	3.0f,	true,	IVec3(7, -8, 0),	tex2DArrayTexelFetchUint,	evalTexelFetch2DArray,	BOTH),

		CASE_SPEC(sampler3d_fixed,				FUNCTION_TEXELFETCH,	Vec4( 8.0f, -7.0f, -3.0f, 0.0f),Vec4(71.9f,  24.9f,  28.9f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 3),	tex3DTexelFetchFixed,		evalTexelFetch3D,		BOTH),
		CASE_SPEC(sampler3d_float,				FUNCTION_TEXELFETCH,	Vec4(-7.0f, -3.0f,  8.0f, 0.0f),Vec4(24.9f,  12.9f,  23.9f,  0.0f),	false,	1.0f,	1.0f,	true,	IVec3(7, 3, -8),	tex3DTexelFetchFloat,		evalTexelFetch3D,		BOTH),
		CASE_SPEC(isampler3d,					FUNCTION_TEXELFETCH,	Vec4(-3.0f,  8.0f, -7.0f, 0.0f),Vec4(12.9f,  15.9f,   0.9f,  0.0f),	false,	2.0f,	2.0f,	true,	IVec3(3, -8, 7),	tex3DTexelFetchInt,			evalTexelFetch3D,		BOTH),
		CASE_SPEC(usampler3d,					FUNCTION_TEXELFETCH,	Vec4( 8.0f, -7.0f, -3.0f, 0.0f),Vec4(71.9f,  24.9f,  28.9f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 7, 3),	tex3DTexelFetchUint,		evalTexelFetch3D,		BOTH),

		CASE_SPEC(sampler1d_fixed,				FUNCTION_TEXELFETCH,	Vec4( 8.0f,  0.0f, 0.0f, 0.0f),	Vec4(263.9f,   0.0f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DTexelFetchFixed,		evalTexelFetch1D,		BOTH),
		CASE_SPEC(sampler1d_float,				FUNCTION_TEXELFETCH,	Vec4(-7.0f,  0.0f, 0.0f, 0.0f),	Vec4(120.9f,   0.0f,  0.0f,  0.0f),	false,	1.0f,	1.0f,	true,	IVec3(7,  0, 0),	tex1DTexelFetchFloat,		evalTexelFetch1D,		BOTH),
		CASE_SPEC(isampler1d,					FUNCTION_TEXELFETCH,	Vec4( 8.0f,  0.0f, 0.0f, 0.0f),	Vec4( 71.9f,   0.0f,  0.0f,  0.0f),	false,	2.0f,	2.0f,	true,	IVec3(-8, 0, 0),	tex1DTexelFetchInt,			evalTexelFetch1D,		BOTH),
		CASE_SPEC(usampler1d,					FUNCTION_TEXELFETCH,	Vec4(-7.0f,  0.0f, 0.0f, 0.0f),	Vec4(  8.9f,   0.0f,  0.0f,  0.0f),	false,	4.0f,	4.0f,	true,	IVec3(7,  0, 0),	tex1DTexelFetchUint,		evalTexelFetch1D,		BOTH),

		CASE_SPEC(sampler1darray_fixed,			FUNCTION_TEXELFETCH,	Vec4( 8.0f,  0.0f, 0.0f, 0.0f),	Vec4(135.9f,   3.9f,  0.0f,  0.0f),	false,	0.0f,	0.0f,	true,	IVec3(-8, 0, 0),	tex1DArrayTexelFetchFixed,	evalTexelFetch1DArray,	BOTH),
		CASE_SPEC(sampler1darray_float,			FUNCTION_TEXELFETCH,	Vec4(-7.0f,  0.0f, 0.0f, 0.0f),	Vec4( 56.9f,   3.9f,  0.0f,  0.0f),	false,	1.0f,	1.0f,	true,	IVec3(7,  0, 0),	tex1DArrayTexelFetchFloat,	evalTexelFetch1DArray,	BOTH),
		CASE_SPEC(isampler1darray,				FUNCTION_TEXELFETCH,	Vec4( 8.0f,  0.0f, 0.0f, 0.0f),	Vec4( 39.9f,   3.9f,  0.0f,  0.0f),	false,	2.0f,	2.0f,	true,	IVec3(-8, 0, 0),	tex1DArrayTexelFetchInt,	evalTexelFetch1DArray,	BOTH),
		CASE_SPEC(usampler1darray,				FUNCTION_TEXELFETCH,	Vec4(-7.0f,  0.0f, 0.0f, 0.0f),	Vec4(  8.9f,   3.9f,  0.0f,  0.0f),	false,	3.0f,	3.0f,	true,	IVec3(7,  0, 0),	tex1DArrayTexelFetchUint,	evalTexelFetch1DArray,	BOTH),
	};
	createCaseGroup(this, "texelfetchoffset", "texelFetchOffset() Tests", texelFetchOffsetCases, DE_LENGTH_OF_ARRAY(texelFetchOffsetCases));

	// texture query functions
	{
		struct TexQueryFuncCaseSpec
		{
			const char*		name;
			const char*		samplerName;
			TextureSpec		textureSpec;
		};

		de::MovePtr<tcu::TestCaseGroup>			queryGroup	(new tcu::TestCaseGroup(m_testCtx, "query", "Texture query function tests"));

		// textureSize() cases
		{
			const TexQueryFuncCaseSpec textureSizeCases[] =
			{
				{ "sampler2d_fixed",			"sampler2D",				tex2DFixed			},
				{ "sampler2d_float",			"sampler2D",				tex2DFloat			},
				{ "isampler2d",					"isampler2D",				tex2DInt			},
				{ "usampler2d",					"usampler2D",				tex2DUint			},
				{ "sampler2dshadow",			"sampler2DShadow",			tex2DShadow			},
				{ "sampler3d_fixed",			"sampler3D",				tex3DFixed			},
				{ "sampler3d_float",			"sampler3D",				tex3DFloat			},
				{ "isampler3d",					"isampler3D",				tex3DInt			},
				{ "usampler3d",					"usampler3D",				tex3DUint			},
				{ "samplercube_fixed",			"samplerCube",				texCubeFixed		},
				{ "samplercube_float",			"samplerCube",				texCubeFloat		},
				{ "isamplercube",				"isamplerCube",				texCubeInt			},
				{ "usamplercube",				"usamplerCube",				texCubeUint			},
				{ "samplercubeshadow",			"samplerCubeShadow",		texCubeShadow		},
				{ "sampler2darray_fixed",		"sampler2DArray",			tex2DArrayFixed		},
				{ "sampler2darray_float",		"sampler2DArray",			tex2DArrayFloat		},
				{ "isampler2darray",			"isampler2DArray",			tex2DArrayInt		},
				{ "usampler2darray",			"usampler2DArray",			tex2DArrayUint		},
				{ "sampler2darrayshadow",		"sampler2DArrayShadow",		tex2DArrayShadow	},
				{ "samplercubearray_fixed",		"samplerCubeArray",			texCubeArrayFixed	},
				{ "samplercubearray_float",		"samplerCubeArray",			texCubeArrayFloat	},
				{ "isamplercubearray",			"isamplerCubeArray",		texCubeArrayInt		},
				{ "usamplercubearray",			"usamplerCubeArray",		texCubeArrayUint	},
				{ "samplercubearrayshadow",		"samplerCubeArrayShadow",	texCubeArrayShadow	},
				{ "sampler1d_fixed",			"sampler1D",				tex1DFixed			},
				{ "sampler1d_float",			"sampler1D",				tex1DFloat			},
				{ "isampler1d",					"isampler1D",				tex1DInt			},
				{ "usampler1d",					"usampler1D",				tex1DUint			},
				{ "sampler1dshadow",			"sampler1DShadow",			tex1DShadow			},
				{ "sampler1darray_fixed",		"sampler1DArray",			tex1DArrayFixed		},
				{ "sampler1darray_float",		"sampler1DArray",			tex1DArrayFloat		},
				{ "isampler1darray",			"isampler1DArray",			tex1DArrayInt		},
				{ "usampler1darray",			"usampler1DArray",			tex1DArrayUint		},
				{ "sampler1darrayshadow",		"sampler1DArrayShadow",		tex1DArrayShadow	},
			};

			de::MovePtr<tcu::TestCaseGroup>		group		(new tcu::TestCaseGroup(m_testCtx, "texturesize", "textureSize() Tests"));

			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(textureSizeCases); ++ndx)
			{
				const TexQueryFuncCaseSpec&		caseSpec	= textureSizeCases[ndx];

				group->addChild(new TextureQueryCase(m_testCtx, (std::string(caseSpec.name) + "_vertex"),   "", caseSpec.samplerName, caseSpec.textureSpec, true,  QUERYFUNCTION_TEXTURESIZE));
				group->addChild(new TextureQueryCase(m_testCtx, (std::string(caseSpec.name) + "_fragment"), "", caseSpec.samplerName, caseSpec.textureSpec, false, QUERYFUNCTION_TEXTURESIZE));
			}

			queryGroup->addChild(group.release());
		}

		// textureSamples() cases
		{
			const TexQueryFuncCaseSpec textureSamplesCases[] =
			{
				{ "sampler2dms_fixed",			"sampler2DMS",				tex2DFixed			},
				{ "sampler2dms_float",			"sampler2DMS",				tex2DFloat			},
				{ "isampler2dms",				"isampler2DMS",				tex2DInt			},
				{ "usampler2dms",				"usampler2DMS",				tex2DUint			},
				{ "sampler2dmsarray_fixed",		"sampler2DMSArray",			tex2DArrayFixed		},
				{ "sampler2dmsarray_float",		"sampler2DMSArray",			tex2DArrayFloat		},
				{ "isampler2dmsarray",			"isampler2DMSArray",		tex2DArrayInt		},
				{ "usampler2dmsarray",			"usampler2DMSArray",		tex2DArrayUint		},
			};

			de::MovePtr<tcu::TestCaseGroup>		group		(new tcu::TestCaseGroup(m_testCtx, "texturesamples", "textureSamples() Tests"));

			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(textureSamplesCases); ++ndx)
			{
				const TexQueryFuncCaseSpec&		caseSpec	= textureSamplesCases[ndx];

				group->addChild(new TextureQueryCase(m_testCtx, (std::string(caseSpec.name) + "_vertex"),   "", caseSpec.samplerName, caseSpec.textureSpec, true,  QUERYFUNCTION_TEXTURESAMPLES));
				group->addChild(new TextureQueryCase(m_testCtx, (std::string(caseSpec.name) + "_fragment"), "", caseSpec.samplerName, caseSpec.textureSpec, false, QUERYFUNCTION_TEXTURESAMPLES));
			}

			queryGroup->addChild(group.release());
		}

		// textureQueryLevels() cases
		{
			const TexQueryFuncCaseSpec textureQueryLevelsCases[] =
			{
				{ "sampler2d_fixed",			"sampler2D",				tex2DFixed			},
				{ "sampler2d_float",			"sampler2D",				tex2DFloat			},
				{ "isampler2d",					"isampler2D",				tex2DInt			},
				{ "usampler2d",					"usampler2D",				tex2DUint			},
				{ "sampler2dshadow",			"sampler2DShadow",			tex2DShadow			},
				{ "sampler3d_fixed",			"sampler3D",				tex3DFixed			},
				{ "sampler3d_float",			"sampler3D",				tex3DFloat			},
				{ "isampler3d",					"isampler3D",				tex3DInt			},
				{ "usampler3d",					"usampler3D",				tex3DUint			},
				{ "samplercube_fixed",			"samplerCube",				texCubeFixed		},
				{ "samplercube_float",			"samplerCube",				texCubeFloat		},
				{ "isamplercube",				"isamplerCube",				texCubeInt			},
				{ "usamplercube",				"usamplerCube",				texCubeUint			},
				{ "samplercubeshadow",			"samplerCubeShadow",		texCubeShadow		},
				{ "sampler2darray_fixed",		"sampler2DArray",			tex2DArrayFixed		},
				{ "sampler2darray_float",		"sampler2DArray",			tex2DArrayFloat		},
				{ "isampler2darray",			"isampler2DArray",			tex2DArrayInt		},
				{ "usampler2darray",			"usampler2DArray",			tex2DArrayUint		},
				{ "sampler2darrayshadow",		"sampler2DArrayShadow",		tex2DArrayShadow	},
				{ "samplercubearray_fixed",		"samplerCubeArray",			texCubeArrayFixed	},
				{ "samplercubearray_float",		"samplerCubeArray",			texCubeArrayFloat	},
				{ "isamplercubearray",			"isamplerCubeArray",		texCubeArrayInt		},
				{ "usamplercubearray",			"usamplerCubeArray",		texCubeArrayUint	},
				{ "samplercubearrayshadow",		"samplerCubeArrayShadow",	texCubeArrayShadow	},
				{ "sampler1d_fixed",			"sampler1D",				tex1DFixed			},
				{ "sampler1d_float",			"sampler1D",				tex1DFloat			},
				{ "isampler1d",					"isampler1D",				tex1DInt			},
				{ "usampler1d",					"usampler1D",				tex1DUint			},
				{ "sampler1dshadow",			"sampler1DShadow",			tex1DShadow			},
				{ "sampler1darray_fixed",		"sampler1DArray",			tex1DArrayFixed		},
				{ "sampler1darray_float",		"sampler1DArray",			tex1DArrayFloat		},
				{ "isampler1darray",			"isampler1DArray",			tex1DArrayInt		},
				{ "usampler1darray",			"usampler1DArray",			tex1DArrayUint		},
				{ "sampler1darrayshadow",		"sampler1DArrayShadow",		tex1DArrayShadow	},
			};

			de::MovePtr<tcu::TestCaseGroup>		group		(new tcu::TestCaseGroup(m_testCtx, "texturequerylevels", "textureQueryLevels() Tests"));

			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(textureQueryLevelsCases); ++ndx)
			{
				const TexQueryFuncCaseSpec&		caseSpec	= textureQueryLevelsCases[ndx];

				group->addChild(new TextureQueryCase(m_testCtx, (std::string(caseSpec.name) + "_vertex"),   "", caseSpec.samplerName, caseSpec.textureSpec, true,  QUERYFUNCTION_TEXTUREQUERYLEVELS));
				group->addChild(new TextureQueryCase(m_testCtx, (std::string(caseSpec.name) + "_fragment"), "", caseSpec.samplerName, caseSpec.textureSpec, false, QUERYFUNCTION_TEXTUREQUERYLEVELS));
			}

			queryGroup->addChild(group.release());
		}

		// textureQueryLod() cases
		{
			const TexQueryFuncCaseSpec textureQueryLodCases[] =
			{
				{ "sampler2d_fixed",			"sampler2D",				tex2DMipmapFixed			},
				{ "sampler2d_float",			"sampler2D",				tex2DMipmapFloat			},
				{ "isampler2d",					"isampler2D",				tex2DMipmapInt				},
				{ "usampler2d",					"usampler2D",				tex2DMipmapUint				},
				{ "sampler2dshadow",			"sampler2DShadow",			tex2DMipmapShadow			},
				{ "sampler3d_fixed",			"sampler3D",				tex3DMipmapFixed			},
				{ "sampler3d_float",			"sampler3D",				tex3DMipmapFloat			},
				{ "isampler3d",					"isampler3D",				tex3DMipmapInt				},
				{ "usampler3d",					"usampler3D",				tex3DMipmapUint				},
				{ "samplercube_fixed",			"samplerCube",				texCubeMipmapFixed			},
				{ "samplercube_float",			"samplerCube",				texCubeMipmapFloat			},
				{ "isamplercube",				"isamplerCube",				texCubeMipmapInt			},
				{ "usamplercube",				"usamplerCube",				texCubeMipmapUint			},
				{ "samplercubeshadow",			"samplerCubeShadow",		texCubeMipmapShadow			},
				{ "sampler2darray_fixed",		"sampler2DArray",			tex2DArrayMipmapFixed		},
				{ "sampler2darray_float",		"sampler2DArray",			tex2DArrayMipmapFloat		},
				{ "isampler2darray",			"isampler2DArray",			tex2DArrayMipmapInt			},
				{ "usampler2darray",			"usampler2DArray",			tex2DArrayMipmapUint		},
				{ "sampler2darrayshadow",		"sampler2DArrayShadow",		tex2DArrayMipmapShadow		},
				{ "samplercubearray_fixed",		"samplerCubeArray",			texCubeArrayMipmapFixed		},
				{ "samplercubearray_float",		"samplerCubeArray",			texCubeArrayMipmapFloat		},
				{ "isamplercubearray",			"isamplerCubeArray",		texCubeArrayMipmapInt		},
				{ "usamplercubearray",			"usamplerCubeArray",		texCubeArrayMipmapUint		},
				{ "samplercubearrayshadow",		"samplerCubeArrayShadow",	texCubeArrayMipmapShadow	},
				{ "sampler1d_fixed",			"sampler1D",				tex1DMipmapFixed			},
				{ "sampler1d_float",			"sampler1D",				tex1DMipmapFloat			},
				{ "isampler1d",					"isampler1D",				tex1DMipmapInt				},
				{ "usampler1d",					"usampler1D",				tex1DMipmapUint				},
				{ "sampler1dshadow",			"sampler1DShadow",			tex1DMipmapShadow			},
				{ "sampler1darray_fixed",		"sampler1DArray",			tex1DArrayMipmapFixed		},
				{ "sampler1darray_float",		"sampler1DArray",			tex1DArrayMipmapFloat		},
				{ "isampler1darray",			"isampler1DArray",			tex1DArrayMipmapInt			},
				{ "usampler1darray",			"usampler1DArray",			tex1DArrayMipmapUint		},
				{ "sampler1darrayshadow",		"sampler1DArrayShadow",		tex1DArrayMipmapShadow		},
			};

			de::MovePtr<tcu::TestCaseGroup>		group		(new tcu::TestCaseGroup(m_testCtx, "texturequerylod", "textureQueryLod() Tests"));

			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(textureQueryLodCases); ++ndx)
			{
				const TexQueryFuncCaseSpec&		caseSpec	= textureQueryLodCases[ndx];

				// available only in fragment shader
				group->addChild(new TextureQueryCase(m_testCtx, (std::string(caseSpec.name) + "_fragment"), "", caseSpec.samplerName, caseSpec.textureSpec, false, QUERYFUNCTION_TEXTUREQUERYLOD));
			}

			queryGroup->addChild(group.release());
		}

		addChild(queryGroup.release());
	}
}

} // anonymous

tcu::TestCaseGroup* createTextureFunctionTests (tcu::TestContext& testCtx)
{
	return new ShaderTextureFunctionTests(testCtx);
}

} // sr
} // vkt
