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
 * \brief Shader Image Load & Store Tests.
 *//*--------------------------------------------------------------------*/

#include "es31fShaderImageLoadStoreTests.hpp"
#include "glsTextureTestUtil.hpp"
#include "gluContextInfo.hpp"
#include "gluRenderContext.hpp"
#include "gluShaderProgram.hpp"
#include "gluObjectWrapper.hpp"
#include "gluPixelTransfer.hpp"
#include "gluTextureUtil.hpp"
#include "gluStrUtil.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluProgramInterfaceQuery.hpp"
#include "gluDrawUtil.hpp"
#include "tcuTestLog.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuVector.hpp"
#include "tcuImageCompare.hpp"
#include "tcuFloat.hpp"
#include "tcuVectorUtil.hpp"
#include "deStringUtil.hpp"
#include "deSharedPtr.hpp"
#include "deUniquePtr.hpp"
#include "deRandom.hpp"
#include "deMemory.h"
#include "glwFunctions.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"

#include <vector>
#include <string>
#include <algorithm>
#include <map>

using glu::RenderContext;
using tcu::TestLog;
using tcu::Vec2;
using tcu::Vec3;
using tcu::Vec4;
using tcu::IVec2;
using tcu::IVec3;
using tcu::IVec4;
using tcu::UVec2;
using tcu::UVec3;
using tcu::UVec4;
using tcu::TextureFormat;
using tcu::ConstPixelBufferAccess;
using tcu::PixelBufferAccess;
using de::toString;
using de::SharedPtr;
using de::UniquePtr;

using std::vector;
using std::string;

namespace deqp
{

using namespace gls::TextureTestUtil;
using namespace glu::TextureTestUtil;

namespace gles31
{
namespace Functional
{

//! Default image sizes used in most test cases.
static inline IVec3 defaultImageSize (TextureType type)
{
	switch (type)
	{
		case TEXTURETYPE_BUFFER:	return IVec3(64,	1,		1);
		case TEXTURETYPE_2D:		return IVec3(64,	64,		1);
		case TEXTURETYPE_CUBE:		return IVec3(64,	64,		1);
		case TEXTURETYPE_3D:		return IVec3(64,	64,		8);
		case TEXTURETYPE_2D_ARRAY:	return IVec3(64,	64,		8);
		default:
			DE_ASSERT(false);
			return IVec3(-1);
	}
}

template <typename T, int Size>
static string arrayStr (const T (&arr)[Size])
{
	string result = "{ ";
	for (int i = 0; i < Size; i++)
		result += (i > 0 ? ", " : "") + toString(arr[i]);
	result += " }";
	return result;
}

template <typename T, int N>
static int arrayIndexOf (const T (&arr)[N], const T& e)
{
	return (int)(std::find(DE_ARRAY_BEGIN(arr), DE_ARRAY_END(arr), e) - DE_ARRAY_BEGIN(arr));
}

static const char* getTextureTypeName (TextureType type)
{
	switch (type)
	{
		case TEXTURETYPE_BUFFER:	return "buffer";
		case TEXTURETYPE_2D:		return "2d";
		case TEXTURETYPE_CUBE:		return "cube";
		case TEXTURETYPE_3D:		return "3d";
		case TEXTURETYPE_2D_ARRAY:	return "2d_array";
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

static inline bool isFormatTypeUnsignedInteger (TextureFormat::ChannelType type)
{
	return type == TextureFormat::UNSIGNED_INT8		||
		   type == TextureFormat::UNSIGNED_INT16	||
		   type == TextureFormat::UNSIGNED_INT32;
}

static inline bool isFormatTypeSignedInteger (TextureFormat::ChannelType type)
{
	return type == TextureFormat::SIGNED_INT8	||
		   type == TextureFormat::SIGNED_INT16	||
		   type == TextureFormat::SIGNED_INT32;
}

static inline bool isFormatTypeInteger (TextureFormat::ChannelType type)
{
	return isFormatTypeUnsignedInteger(type) || isFormatTypeSignedInteger(type);
}

static inline bool isFormatTypeUnorm (TextureFormat::ChannelType type)
{
	return type == TextureFormat::UNORM_INT8	||
		   type == TextureFormat::UNORM_INT16	||
		   type == TextureFormat::UNORM_INT32;
}

static inline bool isFormatTypeSnorm (TextureFormat::ChannelType type)
{
	return type == TextureFormat::SNORM_INT8	||
		   type == TextureFormat::SNORM_INT16	||
		   type == TextureFormat::SNORM_INT32;
}

static inline bool isFormatSupportedForTextureBuffer (const TextureFormat& format)
{
	switch (format.order)
	{
		case TextureFormat::RGB:
			return format.type == TextureFormat::FLOAT				||
				   format.type == TextureFormat::SIGNED_INT32		||
				   format.type == TextureFormat::UNSIGNED_INT32;

		// \note Fallthroughs.
		case TextureFormat::R:
		case TextureFormat::RG:
		case TextureFormat::RGBA:
			return format.type == TextureFormat::UNORM_INT8			||
				   format.type == TextureFormat::HALF_FLOAT			||
				   format.type == TextureFormat::FLOAT				||
				   format.type == TextureFormat::SIGNED_INT8		||
				   format.type == TextureFormat::SIGNED_INT16		||
				   format.type == TextureFormat::SIGNED_INT32		||
				   format.type == TextureFormat::UNSIGNED_INT8		||
				   format.type == TextureFormat::UNSIGNED_INT16		||
				   format.type == TextureFormat::UNSIGNED_INT32;

		default:
			return false;
	}
}

static inline string getShaderImageFormatQualifier (const TextureFormat& format)
{
	const char* orderPart;
	const char* typePart;

	switch (format.order)
	{
		case TextureFormat::R:		orderPart = "r";		break;
		case TextureFormat::RGBA:	orderPart = "rgba";		break;
		default:
			DE_ASSERT(false);
			orderPart = DE_NULL;
	}

	switch (format.type)
	{
		case TextureFormat::FLOAT:				typePart = "32f";			break;
		case TextureFormat::HALF_FLOAT:			typePart = "16f";			break;

		case TextureFormat::UNSIGNED_INT32:		typePart = "32ui";			break;
		case TextureFormat::UNSIGNED_INT16:		typePart = "16ui";			break;
		case TextureFormat::UNSIGNED_INT8:		typePart = "8ui";			break;

		case TextureFormat::SIGNED_INT32:		typePart = "32i";			break;
		case TextureFormat::SIGNED_INT16:		typePart = "16i";			break;
		case TextureFormat::SIGNED_INT8:		typePart = "8i";			break;

		case TextureFormat::UNORM_INT16:		typePart = "16";			break;
		case TextureFormat::UNORM_INT8:			typePart = "8";				break;

		case TextureFormat::SNORM_INT16:		typePart = "16_snorm";		break;
		case TextureFormat::SNORM_INT8:			typePart = "8_snorm";		break;

		default:
			DE_ASSERT(false);
			typePart = DE_NULL;
	}

	return string() + orderPart + typePart;
}

static inline string getShaderSamplerOrImageType (TextureFormat::ChannelType formatType, TextureType textureType, bool isSampler)
{
	const char* const formatPart		= isFormatTypeUnsignedInteger(formatType)	? "u"
										: isFormatTypeSignedInteger(formatType)		? "i"
										: "";

	const char* const imageTypePart		= textureType == TEXTURETYPE_BUFFER		? "Buffer"
										: textureType == TEXTURETYPE_2D			? "2D"
										: textureType == TEXTURETYPE_3D			? "3D"
										: textureType == TEXTURETYPE_CUBE		? "Cube"
										: textureType == TEXTURETYPE_2D_ARRAY	? "2DArray"
										: DE_NULL;

	return string() + formatPart + (isSampler ? "sampler" : "image") + imageTypePart;
}

static inline string getShaderImageType (TextureFormat::ChannelType formatType, TextureType imageType)
{
	return getShaderSamplerOrImageType(formatType, imageType, false);
}

static inline string getShaderSamplerType (TextureFormat::ChannelType formatType, TextureType imageType)
{
	return getShaderSamplerOrImageType(formatType, imageType, true);
}

static inline deUint32 getGLTextureTarget (TextureType texType)
{
	switch (texType)
	{
		case TEXTURETYPE_BUFFER:	return GL_TEXTURE_BUFFER;
		case TEXTURETYPE_2D:		return GL_TEXTURE_2D;
		case TEXTURETYPE_3D:		return GL_TEXTURE_3D;
		case TEXTURETYPE_CUBE:		return GL_TEXTURE_CUBE_MAP;
		case TEXTURETYPE_2D_ARRAY:	return GL_TEXTURE_2D_ARRAY;
		default:
			DE_ASSERT(false);
			return (deUint32)-1;
	}
}

static deUint32 cubeFaceToGLFace (tcu::CubeFace face)
{
	switch (face)
	{
		case tcu::CUBEFACE_NEGATIVE_X: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
		case tcu::CUBEFACE_POSITIVE_X: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		case tcu::CUBEFACE_NEGATIVE_Y: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
		case tcu::CUBEFACE_POSITIVE_Y: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
		case tcu::CUBEFACE_NEGATIVE_Z: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
		case tcu::CUBEFACE_POSITIVE_Z: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
		default:
			DE_ASSERT(false);
			return GL_NONE;
	}
}

static inline tcu::Texture1D* newOneLevelTexture1D (const tcu::TextureFormat& format, int w)
{
	tcu::Texture1D* const res = new tcu::Texture1D(format, w);
	res->allocLevel(0);
	return res;
}

static inline tcu::Texture2D* newOneLevelTexture2D (const tcu::TextureFormat& format, int w, int h)
{
	tcu::Texture2D* const res = new tcu::Texture2D(format, w, h);
	res->allocLevel(0);
	return res;
}

static inline tcu::TextureCube* newOneLevelTextureCube (const tcu::TextureFormat& format, int size)
{
	tcu::TextureCube* const res = new tcu::TextureCube(format, size);
	for (int i = 0; i < tcu::CUBEFACE_LAST; i++)
		res->allocLevel((tcu::CubeFace)i, 0);
	return res;
}

static inline tcu::Texture3D* newOneLevelTexture3D (const tcu::TextureFormat& format, int w, int h, int d)
{
	tcu::Texture3D* const res = new tcu::Texture3D(format, w, h, d);
	res->allocLevel(0);
	return res;
}

static inline tcu::Texture2DArray* newOneLevelTexture2DArray (const tcu::TextureFormat& format, int w, int h, int d)
{
	tcu::Texture2DArray* const res = new tcu::Texture2DArray(format, w, h, d);
	res->allocLevel(0);
	return res;
}

static inline TextureType textureLayerType (TextureType entireTextureType)
{
	switch (entireTextureType)
	{
		// Single-layer types.
		// \note Fallthrough.
		case TEXTURETYPE_BUFFER:
		case TEXTURETYPE_2D:
			return entireTextureType;

		// Multi-layer types with 2d layers.
		case TEXTURETYPE_3D:
		case TEXTURETYPE_CUBE:
		case TEXTURETYPE_2D_ARRAY:
			return TEXTURETYPE_2D;

		default:
			DE_ASSERT(false);
			return TEXTURETYPE_LAST;
	}
}

static const char* const s_texBufExtString = "GL_EXT_texture_buffer";

static inline void checkTextureTypeExtensions (const glu::ContextInfo& contextInfo, TextureType type, const RenderContext& renderContext)
{
	if (type == TEXTURETYPE_BUFFER && !contextInfo.isExtensionSupported(s_texBufExtString) && !glu::contextSupports(renderContext.getType(), glu::ApiType::es(3, 2)))
		throw tcu::NotSupportedError("Test requires " + string(s_texBufExtString) + " extension");
}

static inline string textureTypeExtensionShaderRequires (TextureType type, const RenderContext& renderContext)
{
	if (!glu::contextSupports(renderContext.getType(), glu::ApiType::es(3, 2)) && (type == TEXTURETYPE_BUFFER))
		return "#extension " + string(s_texBufExtString) + " : require\n";
	else
		return "";
}

static const char* const s_imageAtomicExtString = "GL_OES_shader_image_atomic";

static inline string imageAtomicExtensionShaderRequires (const RenderContext& renderContext)
{
	if (!glu::contextSupports(renderContext.getType(), glu::ApiType::es(3, 2)))
		return "#extension " + string(s_imageAtomicExtString) + " : require\n";
	else
		return "";
}

namespace
{

enum AtomicOperation
{
	ATOMIC_OPERATION_ADD = 0,
	ATOMIC_OPERATION_MIN,
	ATOMIC_OPERATION_MAX,
	ATOMIC_OPERATION_AND,
	ATOMIC_OPERATION_OR,
	ATOMIC_OPERATION_XOR,
	ATOMIC_OPERATION_EXCHANGE,
	ATOMIC_OPERATION_COMP_SWAP,

	ATOMIC_OPERATION_LAST
};

//! An order-independent operation is one for which the end result doesn't depend on the order in which the operations are carried (i.e. is both commutative and associative).
static bool isOrderIndependentAtomicOperation (AtomicOperation op)
{
	return op == ATOMIC_OPERATION_ADD	||
		   op == ATOMIC_OPERATION_MIN	||
		   op == ATOMIC_OPERATION_MAX	||
		   op == ATOMIC_OPERATION_AND	||
		   op == ATOMIC_OPERATION_OR	||
		   op == ATOMIC_OPERATION_XOR;
}

//! Computes the result of an atomic operation where "a" is the data operated on and "b" is the parameter to the atomic function.
int computeBinaryAtomicOperationResult (AtomicOperation op, int a, int b)
{
	switch (op)
	{
		case ATOMIC_OPERATION_ADD:			return a + b;
		case ATOMIC_OPERATION_MIN:			return de::min(a, b);
		case ATOMIC_OPERATION_MAX:			return de::max(a, b);
		case ATOMIC_OPERATION_AND:			return a & b;
		case ATOMIC_OPERATION_OR:			return a | b;
		case ATOMIC_OPERATION_XOR:			return a ^ b;
		case ATOMIC_OPERATION_EXCHANGE:		return b;
		default:
			DE_ASSERT(false);
			return -1;
	}
}

//! \note For floats, only the exchange operation is supported.
float computeBinaryAtomicOperationResult (AtomicOperation op, float /*a*/, float b)
{
	switch (op)
	{
		case ATOMIC_OPERATION_EXCHANGE: return b;
		default:
			DE_ASSERT(false);
			return -1;
	}
}

static const char* getAtomicOperationCaseName (AtomicOperation op)
{
	switch (op)
	{
		case ATOMIC_OPERATION_ADD:			return "add";
		case ATOMIC_OPERATION_MIN:			return "min";
		case ATOMIC_OPERATION_MAX:			return "max";
		case ATOMIC_OPERATION_AND:			return "and";
		case ATOMIC_OPERATION_OR:			return "or";
		case ATOMIC_OPERATION_XOR:			return "xor";
		case ATOMIC_OPERATION_EXCHANGE:		return "exchange";
		case ATOMIC_OPERATION_COMP_SWAP:	return "comp_swap";
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

static const char* getAtomicOperationShaderFuncName (AtomicOperation op)
{
	switch (op)
	{
		case ATOMIC_OPERATION_ADD:			return "imageAtomicAdd";
		case ATOMIC_OPERATION_MIN:			return "imageAtomicMin";
		case ATOMIC_OPERATION_MAX:			return "imageAtomicMax";
		case ATOMIC_OPERATION_AND:			return "imageAtomicAnd";
		case ATOMIC_OPERATION_OR:			return "imageAtomicOr";
		case ATOMIC_OPERATION_XOR:			return "imageAtomicXor";
		case ATOMIC_OPERATION_EXCHANGE:		return "imageAtomicExchange";
		case ATOMIC_OPERATION_COMP_SWAP:	return "imageAtomicCompSwap";
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

//! In GLSL, when accessing cube images, the z coordinate is mapped to a cube face.
//! \note This is _not_ the same as casting the z to a tcu::CubeFace.
static inline tcu::CubeFace glslImageFuncZToCubeFace (int z)
{
	static const tcu::CubeFace faces[6] =
	{
		tcu::CUBEFACE_POSITIVE_X,
		tcu::CUBEFACE_NEGATIVE_X,
		tcu::CUBEFACE_POSITIVE_Y,
		tcu::CUBEFACE_NEGATIVE_Y,
		tcu::CUBEFACE_POSITIVE_Z,
		tcu::CUBEFACE_NEGATIVE_Z
	};

	DE_ASSERT(de::inBounds(z, 0, DE_LENGTH_OF_ARRAY(faces)));
	return faces[z];
}

class BufferMemMap
{
public:
	BufferMemMap (const glw::Functions& gl, deUint32 target, int offset, int size, deUint32 access)
		: m_gl		(gl)
		, m_target	(target)
		, m_ptr		(DE_NULL)
	{
		m_ptr = gl.mapBufferRange(target, offset, size, access);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange()");
		TCU_CHECK(m_ptr);
	}

	~BufferMemMap (void)
	{
		m_gl.unmapBuffer(m_target);
	}

	void*	getPtr		(void) const { return m_ptr; }
	void*	operator*	(void) const { return m_ptr; }

private:
							BufferMemMap			(const BufferMemMap& other);
	BufferMemMap&			operator=				(const BufferMemMap& other);

	const glw::Functions&	m_gl;
	const deUint32			m_target;
	void*					m_ptr;
};

//! Utility for more readable uniform assignment logging; logs the name of the uniform when assigning. Handles the locations, querying them the first time they're assigned
//  \note Assumes that the appropriate program is in use when assigning uniforms.
class UniformAccessLogger
{
public:
	UniformAccessLogger (const glw::Functions& gl, TestLog& log, deUint32 programGL)
		: m_gl			(gl)
		, m_log			(log)
		, m_programGL	(programGL)
	{
	}

	void						assign1i (const string& name, int x);
	void						assign3f (const string& name, float x, float y, float z);

private:
	int							getLocation (const string& name);

	const glw::Functions&		m_gl;
	TestLog&					m_log;
	const deUint32				m_programGL;

	std::map<string, int>		m_uniformLocations;
};

int UniformAccessLogger::getLocation (const string& name)
{
	if (m_uniformLocations.find(name) == m_uniformLocations.end())
	{
		const int loc = m_gl.getUniformLocation(m_programGL, name.c_str());
		TCU_CHECK(loc != -1);
		m_uniformLocations[name] = loc;
	}
	return m_uniformLocations[name];
}

void UniformAccessLogger::assign1i (const string& name, int x)
{
	const int loc = getLocation(name);
	m_log << TestLog::Message << "// Assigning to uniform " << name << ": " << x << TestLog::EndMessage;
	m_gl.uniform1i(loc, x);
}

void UniformAccessLogger::assign3f (const string& name, float x, float y, float z)
{
	const int loc = getLocation(name);
	m_log << TestLog::Message << "// Assigning to uniform " << name << ": " << Vec3(x, y, z) << TestLog::EndMessage;
	m_gl.uniform3f(loc, x, y, z);
}

//! Class containing a (single-level) texture of a given type. Supports accessing pixels with coordinate convention similar to that in imageStore() and imageLoad() in shaders; useful especially for cube maps.
class LayeredImage
{
public:
												LayeredImage				(TextureType type, const TextureFormat& format, int w, int h, int d);

	TextureType									getImageType				(void) const { return m_type; }
	const IVec3&								getSize						(void) const { return m_size; }
	const TextureFormat&						getFormat					(void) const { return m_format; }

	// \note For cube maps, set/getPixel's z parameter specifies the cube face in the same manner as in imageStore/imageLoad in GL shaders (see glslImageFuncZToCubeFace), instead of directly as a tcu::CubeFace.

	template <typename ColorT>
	void										setPixel					(int x, int y, int z, const ColorT& color) const;

	Vec4										getPixel					(int x, int y, int z) const;
	IVec4										getPixelInt					(int x, int y, int z) const;
	UVec4										getPixelUint				(int x, int y, int z) const { return getPixelInt(x, y, z).asUint(); }

	PixelBufferAccess							getAccess					(void)							{ return getAccessInternal();				}
	PixelBufferAccess							getSliceAccess				(int slice)						{ return getSliceAccessInternal(slice);		}
	PixelBufferAccess							getCubeFaceAccess			(tcu::CubeFace face)			{ return getCubeFaceAccessInternal(face);	}

	ConstPixelBufferAccess						getAccess					(void)					const	{ return getAccessInternal();				}
	ConstPixelBufferAccess						getSliceAccess				(int slice)				const	{ return getSliceAccessInternal(slice);		}
	ConstPixelBufferAccess						getCubeFaceAccess			(tcu::CubeFace face)	const	{ return getCubeFaceAccessInternal(face);	}

private:
												LayeredImage				(const LayeredImage&);
	LayeredImage&								operator=					(const LayeredImage&);

	// Some helpers to reduce code duplication between const/non-const versions of getAccess and others.
	PixelBufferAccess							getAccessInternal			(void) const;
	PixelBufferAccess							getSliceAccessInternal		(int slice) const;
	PixelBufferAccess							getCubeFaceAccessInternal	(tcu::CubeFace face) const;

	const TextureType							m_type;
	const IVec3									m_size;
	const TextureFormat							m_format;

	// \note Depending on m_type, exactly one of the following will contain non-null.
	const SharedPtr<tcu::Texture1D>				m_texBuffer;
	const SharedPtr<tcu::Texture2D>				m_tex2D;
	const SharedPtr<tcu::TextureCube>			m_texCube;
	const SharedPtr<tcu::Texture3D>				m_tex3D;
	const SharedPtr<tcu::Texture2DArray>		m_tex2DArray;
};

LayeredImage::LayeredImage (TextureType type, const TextureFormat& format, int w, int h, int d)
	: m_type		(type)
	, m_size		(w, h, d)
	, m_format		(format)
	, m_texBuffer	(type == TEXTURETYPE_BUFFER		? SharedPtr<tcu::Texture1D>			(newOneLevelTexture1D		(format, w))		: SharedPtr<tcu::Texture1D>())
	, m_tex2D		(type == TEXTURETYPE_2D			? SharedPtr<tcu::Texture2D>			(newOneLevelTexture2D		(format, w, h))		: SharedPtr<tcu::Texture2D>())
	, m_texCube		(type == TEXTURETYPE_CUBE		? SharedPtr<tcu::TextureCube>		(newOneLevelTextureCube		(format, w))		: SharedPtr<tcu::TextureCube>())
	, m_tex3D		(type == TEXTURETYPE_3D			? SharedPtr<tcu::Texture3D>			(newOneLevelTexture3D		(format, w, h, d))	: SharedPtr<tcu::Texture3D>())
	, m_tex2DArray	(type == TEXTURETYPE_2D_ARRAY	? SharedPtr<tcu::Texture2DArray>	(newOneLevelTexture2DArray	(format, w, h, d))	: SharedPtr<tcu::Texture2DArray>())
{
	DE_ASSERT(m_size.z() == 1					||
			  m_type == TEXTURETYPE_3D			||
			  m_type == TEXTURETYPE_2D_ARRAY);

	DE_ASSERT(m_size.y() == 1					||
			  m_type == TEXTURETYPE_2D			||
			  m_type == TEXTURETYPE_CUBE		||
			  m_type == TEXTURETYPE_3D			||
			  m_type == TEXTURETYPE_2D_ARRAY);

	DE_ASSERT(w == h || type != TEXTURETYPE_CUBE);

	DE_ASSERT(m_texBuffer	!= DE_NULL ||
			  m_tex2D		!= DE_NULL ||
			  m_texCube		!= DE_NULL ||
			  m_tex3D		!= DE_NULL ||
			  m_tex2DArray	!= DE_NULL);
}

template <typename ColorT>
void LayeredImage::setPixel (int x, int y, int z, const ColorT& color) const
{
	const PixelBufferAccess access = m_type == TEXTURETYPE_BUFFER		? m_texBuffer->getLevel(0)
								   : m_type == TEXTURETYPE_2D			? m_tex2D->getLevel(0)
								   : m_type == TEXTURETYPE_CUBE			? m_texCube->getLevelFace(0, glslImageFuncZToCubeFace(z))
								   : m_type == TEXTURETYPE_3D			? m_tex3D->getLevel(0)
								   : m_type == TEXTURETYPE_2D_ARRAY		? m_tex2DArray->getLevel(0)
								   : PixelBufferAccess();

	access.setPixel(color, x, y, m_type == TEXTURETYPE_CUBE ? 0 : z);
}

Vec4 LayeredImage::getPixel (int x, int y, int z) const
{
	const ConstPixelBufferAccess access = m_type == TEXTURETYPE_CUBE ? getCubeFaceAccess(glslImageFuncZToCubeFace(z)) : getAccess();
	return access.getPixel(x, y, m_type == TEXTURETYPE_CUBE ? 0 : z);
}

IVec4 LayeredImage::getPixelInt (int x, int y, int z) const
{
	const ConstPixelBufferAccess access = m_type == TEXTURETYPE_CUBE ? getCubeFaceAccess(glslImageFuncZToCubeFace(z)) : getAccess();
	return access.getPixelInt(x, y, m_type == TEXTURETYPE_CUBE ? 0 : z);
}

PixelBufferAccess LayeredImage::getAccessInternal (void) const
{
	DE_ASSERT(m_type == TEXTURETYPE_BUFFER || m_type == TEXTURETYPE_2D || m_type == TEXTURETYPE_3D || m_type == TEXTURETYPE_2D_ARRAY);

	return m_type == TEXTURETYPE_BUFFER		? m_texBuffer->getLevel(0)
		 : m_type == TEXTURETYPE_2D			? m_tex2D->getLevel(0)
		 : m_type == TEXTURETYPE_3D			? m_tex3D->getLevel(0)
		 : m_type == TEXTURETYPE_2D_ARRAY	? m_tex2DArray->getLevel(0)
		 : PixelBufferAccess();
}

PixelBufferAccess LayeredImage::getSliceAccessInternal (int slice) const
{
	const PixelBufferAccess srcAccess = getAccessInternal();
	return tcu::getSubregion(srcAccess, 0, 0, slice, srcAccess.getWidth(), srcAccess.getHeight(), 1);
}

PixelBufferAccess LayeredImage::getCubeFaceAccessInternal (tcu::CubeFace face) const
{
	DE_ASSERT(m_type == TEXTURETYPE_CUBE);
	return m_texCube->getLevelFace(0, face);
}

//! Set texture storage or, if using buffer texture, setup buffer and attach to texture.
static void setTextureStorage (glu::CallLogWrapper& glLog, TextureType imageType, deUint32 internalFormat, const IVec3& imageSize, deUint32 textureBufGL)
{
	const deUint32 textureTarget = getGLTextureTarget(imageType);

	switch (imageType)
	{
		case TEXTURETYPE_BUFFER:
		{
			const TextureFormat		format		= glu::mapGLInternalFormat(internalFormat);
			const int				numBytes	= format.getPixelSize() * imageSize.x();
			DE_ASSERT(isFormatSupportedForTextureBuffer(format));
			glLog.glBindBuffer(GL_TEXTURE_BUFFER, textureBufGL);
			glLog.glBufferData(GL_TEXTURE_BUFFER, numBytes, DE_NULL, GL_STATIC_DRAW);
			glLog.glTexBuffer(GL_TEXTURE_BUFFER, internalFormat, textureBufGL);
			DE_ASSERT(imageSize.y() == 1 && imageSize.z() == 1);
			break;
		}

		// \note Fall-throughs.

		case TEXTURETYPE_2D:
		case TEXTURETYPE_CUBE:
			glLog.glTexStorage2D(textureTarget, 1, internalFormat, imageSize.x(), imageSize.y());
			DE_ASSERT(imageSize.z() == 1);
			break;

		case TEXTURETYPE_3D:
		case TEXTURETYPE_2D_ARRAY:
			glLog.glTexStorage3D(textureTarget, 1, internalFormat, imageSize.x(), imageSize.y(), imageSize.z());
			break;

		default:
			DE_ASSERT(false);
	}
}

static void uploadTexture (glu::CallLogWrapper& glLog, const LayeredImage& src, deUint32 textureBufGL)
{
	const deUint32				internalFormat	= glu::getInternalFormat(src.getFormat());
	const glu::TransferFormat	transferFormat	= glu::getTransferFormat(src.getFormat());
	const IVec3&				imageSize		= src.getSize();

	setTextureStorage(glLog, src.getImageType(), internalFormat, imageSize, textureBufGL);

	{
		const int	pixelSize = src.getFormat().getPixelSize();
		int			unpackAlignment;

		if (deIsPowerOfTwo32(pixelSize))
			unpackAlignment = 8;
		else
			unpackAlignment = 1;

		glLog.glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
	}

	if (src.getImageType() == TEXTURETYPE_BUFFER)
	{
		glLog.glBindBuffer(GL_TEXTURE_BUFFER, textureBufGL);
		glLog.glBufferData(GL_TEXTURE_BUFFER, src.getFormat().getPixelSize() * imageSize.x(), src.getAccess().getDataPtr(), GL_STATIC_DRAW);
	}
	else if (src.getImageType() == TEXTURETYPE_2D)
		glLog.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, imageSize.x(), imageSize.y(), transferFormat.format, transferFormat.dataType, src.getAccess().getDataPtr());
	else if (src.getImageType() == TEXTURETYPE_CUBE)
	{
		for (int faceI = 0; faceI < tcu::CUBEFACE_LAST; faceI++)
		{
			const tcu::CubeFace face = (tcu::CubeFace)faceI;
			glLog.glTexSubImage2D(cubeFaceToGLFace(face), 0, 0, 0, imageSize.x(), imageSize.y(), transferFormat.format, transferFormat.dataType, src.getCubeFaceAccess(face).getDataPtr());
		}
	}
	else
	{
		DE_ASSERT(src.getImageType() == TEXTURETYPE_3D || src.getImageType() == TEXTURETYPE_2D_ARRAY);
		const deUint32 textureTarget = getGLTextureTarget(src.getImageType());
		glLog.glTexSubImage3D(textureTarget, 0, 0, 0, 0, imageSize.x(), imageSize.y(), imageSize.z(), transferFormat.format, transferFormat.dataType, src.getAccess().getDataPtr());
	}
}

static void readPixelsRGBAInteger32 (const PixelBufferAccess& dst, int originX, int originY, glu::CallLogWrapper& glLog)
{
	DE_ASSERT(dst.getDepth() == 1);

	if (isFormatTypeUnsignedInteger(dst.getFormat().type))
	{
		vector<UVec4> data(dst.getWidth()*dst.getHeight());

		glLog.glReadPixels(originX, originY, dst.getWidth(), dst.getHeight(), GL_RGBA_INTEGER, GL_UNSIGNED_INT, &data[0]);

		for (int y = 0; y < dst.getHeight(); y++)
		for (int x = 0; x < dst.getWidth(); x++)
			dst.setPixel(data[y*dst.getWidth() + x], x, y);
	}
	else if (isFormatTypeSignedInteger(dst.getFormat().type))
	{
		vector<IVec4> data(dst.getWidth()*dst.getHeight());

		glLog.glReadPixels(originX, originY, dst.getWidth(), dst.getHeight(), GL_RGBA_INTEGER, GL_INT, &data[0]);

		for (int y = 0; y < dst.getHeight(); y++)
		for (int x = 0; x < dst.getWidth(); x++)
			dst.setPixel(data[y*dst.getWidth() + x], x, y);
	}
	else
		DE_ASSERT(false);
}

//! Base for a functor for verifying and logging a 2d texture layer (2d image, cube face, 3d slice, 2d layer).
class ImageLayerVerifier
{
public:
	virtual bool	operator()				(TestLog&, const ConstPixelBufferAccess&, int sliceOrFaceNdx) const = 0;
	virtual			~ImageLayerVerifier		(void) {}
};

static void setTexParameteri (glu::CallLogWrapper& glLog, deUint32 target)
{
	if (target != GL_TEXTURE_BUFFER)
	{
		glLog.glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glLog.glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
}

//! Binds texture (one layer at a time) to color attachment of FBO and does glReadPixels(). Calls the verifier for each layer.
//! \note Not for buffer textures.
static bool readIntegerTextureViaFBOAndVerify (const RenderContext&			renderCtx,
											   glu::CallLogWrapper&			glLog,
											   deUint32						textureGL,
											   TextureType					textureType,
											   const TextureFormat&			textureFormat,
											   const IVec3&					textureSize,
											   const ImageLayerVerifier&	verifyLayer)
{
	DE_ASSERT(isFormatTypeInteger(textureFormat.type));
	DE_ASSERT(textureType != TEXTURETYPE_BUFFER);

	TestLog& log = glLog.getLog();

	const tcu::ScopedLogSection section(log, "Verification", "Result verification (bind texture layer-by-layer to FBO, read with glReadPixels())");

	const int			numSlicesOrFaces	= textureType == TEXTURETYPE_CUBE ? 6 : textureSize.z();
	const deUint32		textureTargetGL		= getGLTextureTarget(textureType);
	glu::Framebuffer	fbo					(renderCtx);
	tcu::TextureLevel	resultSlice			(textureFormat, textureSize.x(), textureSize.y());

	glLog.glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
	GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "Bind FBO");

	glLog.glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
	GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glMemoryBarrier");

	glLog.glActiveTexture(GL_TEXTURE0);
	glLog.glBindTexture(textureTargetGL, textureGL);
	setTexParameteri(glLog, textureTargetGL);

	for (int sliceOrFaceNdx = 0; sliceOrFaceNdx < numSlicesOrFaces; sliceOrFaceNdx++)
	{
		if (textureType == TEXTURETYPE_CUBE)
			glLog.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cubeFaceToGLFace(glslImageFuncZToCubeFace(sliceOrFaceNdx)), textureGL, 0);
		else if (textureType == TEXTURETYPE_2D)
			glLog.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureGL, 0);
		else if (textureType == TEXTURETYPE_3D || textureType == TEXTURETYPE_2D_ARRAY)
			glLog.glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureGL, 0, sliceOrFaceNdx);
		else
			DE_ASSERT(false);

		GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "Bind texture to framebuffer color attachment 0");

		TCU_CHECK(glLog.glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

		readPixelsRGBAInteger32(resultSlice.getAccess(), 0, 0, glLog);
		GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glReadPixels");

		if (!verifyLayer(log, resultSlice, sliceOrFaceNdx))
			return false;
	}

	return true;
}

//! Reads texture with texture() in compute shader, one layer at a time, putting values into a SSBO and reading with a mapping. Calls the verifier for each layer.
//! \note Not for buffer textures.
static bool readFloatOrNormTextureWithLookupsAndVerify (const RenderContext&		renderCtx,
														glu::CallLogWrapper&		glLog,
														deUint32					textureGL,
														TextureType					textureType,
														const TextureFormat&		textureFormat,
														const IVec3&				textureSize,
														const ImageLayerVerifier&	verifyLayer)
{
	DE_ASSERT(!isFormatTypeInteger(textureFormat.type));
	DE_ASSERT(textureType != TEXTURETYPE_BUFFER);

	TestLog& log = glLog.getLog();

	const tcu::ScopedLogSection section(log, "Verification", "Result verification (read texture layer-by-layer in compute shader with texture() into SSBO)");
	const std::string			glslVersionDeclaration = getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(renderCtx.getType()));

	const glu::ShaderProgram program(renderCtx,
		glu::ProgramSources() << glu::ComputeSource(glslVersionDeclaration + "\n"
													"layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
													"layout (binding = 0) buffer Output\n"
													"{\n"
													"	vec4 color[" + toString(textureSize.x()*textureSize.y()) + "];\n"
													"} sb_out;\n"
													"\n"
													"precision highp " + getShaderSamplerType(textureFormat.type, textureType) + ";\n"
													"\n"
													"uniform highp " + getShaderSamplerType(textureFormat.type, textureType) + " u_texture;\n"
													"uniform highp vec3 u_texCoordLD;\n"
													"uniform highp vec3 u_texCoordRD;\n"
													"uniform highp vec3 u_texCoordLU;\n"
													"uniform highp vec3 u_texCoordRU;\n"
													"\n"
													"void main (void)\n"
													"{\n"
													"	int gx = int(gl_GlobalInvocationID.x);\n"
													"	int gy = int(gl_GlobalInvocationID.y);\n"
													"	highp float s = (float(gx) + 0.5) / float(" + toString(textureSize.x()) + ");\n"
													"	highp float t = (float(gy) + 0.5) / float(" + toString(textureType == TEXTURETYPE_CUBE ? textureSize.x() : textureSize.y()) + ");\n"
													"	highp vec3 texCoord = u_texCoordLD*(1.0-s)*(1.0-t)\n"
													"	                    + u_texCoordRD*(    s)*(1.0-t)\n"
													"	                    + u_texCoordLU*(1.0-s)*(    t)\n"
													"	                    + u_texCoordRU*(    s)*(    t);\n"
													"	int ndx = gy*" + toString(textureSize.x()) + " + gx;\n"
													"	sb_out.color[ndx] = texture(u_texture, texCoord" + (textureType == TEXTURETYPE_2D ? ".xy" : "") + ");\n"
													"}\n"));

	glLog.glUseProgram(program.getProgram());

	log << program;

	if (!program.isOk())
	{
		log << TestLog::Message << "// Failure: failed to compile program" << TestLog::EndMessage;
		TCU_FAIL("Program compilation failed");
	}

	{
		const deUint32			textureTargetGL		= getGLTextureTarget(textureType);
		const glu::Buffer		outputBuffer		(renderCtx);
		UniformAccessLogger		uniforms			(renderCtx.getFunctions(), log, program.getProgram());

		// Setup texture.

		glLog.glActiveTexture(GL_TEXTURE0);
		glLog.glBindTexture(textureTargetGL, textureGL);
		setTexParameteri(glLog, textureTargetGL);

		uniforms.assign1i("u_texture", 0);

		// Setup output buffer.
		{
			const deUint32		blockIndex		= glLog.glGetProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int			blockSize		= glu::getProgramResourceInt(renderCtx.getFunctions(), program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);

			log << TestLog::Message << "// Got buffer data size = " << blockSize << TestLog::EndMessage;
			TCU_CHECK(blockSize > 0);

			glLog.glBindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer);
			glLog.glBufferData(GL_SHADER_STORAGE_BUFFER, blockSize, DE_NULL, GL_STREAM_READ);
			glLog.glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *outputBuffer);
			GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "SSB setup failed");
		}

		// Dispatch one layer at a time, read back and verify.
		{
			const int							numSlicesOrFaces	= textureType == TEXTURETYPE_CUBE ? 6 : textureSize.z();
			tcu::TextureLevel					resultSlice			(textureFormat, textureSize.x(), textureSize.y());
			const PixelBufferAccess				resultSliceAccess	= resultSlice.getAccess();
			const deUint32						blockIndex			= glLog.glGetProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int							blockSize			= glu::getProgramResourceInt(renderCtx.getFunctions(), program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);
			const deUint32						valueIndex			= glLog.glGetProgramResourceIndex(program.getProgram(), GL_BUFFER_VARIABLE, "Output.color");
			const glu::InterfaceVariableInfo	valueInfo			= glu::getProgramInterfaceVariableInfo(renderCtx.getFunctions(), program.getProgram(), GL_BUFFER_VARIABLE, valueIndex);

			TCU_CHECK(valueInfo.arraySize == (deUint32)(textureSize.x()*textureSize.y()));

			glLog.glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

			for (int sliceOrFaceNdx = 0; sliceOrFaceNdx < numSlicesOrFaces; sliceOrFaceNdx++)
			{
				if (textureType == TEXTURETYPE_CUBE)
				{
					vector<float> coords;
					computeQuadTexCoordCube(coords, glslImageFuncZToCubeFace(sliceOrFaceNdx));
					uniforms.assign3f("u_texCoordLD", coords[3*0 + 0], coords[3*0 + 1], coords[3*0 + 2]);
					uniforms.assign3f("u_texCoordRD", coords[3*2 + 0], coords[3*2 + 1], coords[3*2 + 2]);
					uniforms.assign3f("u_texCoordLU", coords[3*1 + 0], coords[3*1 + 1], coords[3*1 + 2]);
					uniforms.assign3f("u_texCoordRU", coords[3*3 + 0], coords[3*3 + 1], coords[3*3 + 2]);
				}
				else
				{
					const float z = textureType == TEXTURETYPE_3D ?
										((float)sliceOrFaceNdx + 0.5f) / (float)numSlicesOrFaces :
										(float)sliceOrFaceNdx;
					uniforms.assign3f("u_texCoordLD", 0.0f, 0.0f, z);
					uniforms.assign3f("u_texCoordRD", 1.0f, 0.0f, z);
					uniforms.assign3f("u_texCoordLU", 0.0f, 1.0f, z);
					uniforms.assign3f("u_texCoordRU", 1.0f, 1.0f, z);
				}

				glLog.glDispatchCompute(textureSize.x(), textureSize.y(), 1);

				{
					log << TestLog::Message << "// Note: mapping buffer and reading color values written" << TestLog::EndMessage;

					const BufferMemMap bufMap(renderCtx.getFunctions(), GL_SHADER_STORAGE_BUFFER, 0, blockSize, GL_MAP_READ_BIT);

					for (int y = 0; y < textureSize.y(); y++)
					for (int x = 0; x < textureSize.x(); x++)
					{
						const int				ndx			= y*textureSize.x() + x;
						const float* const		clrData		= (const float*)((const deUint8*)bufMap.getPtr() + valueInfo.offset + valueInfo.arrayStride*ndx);

						switch (textureFormat.order)
						{
							case TextureFormat::R:		resultSliceAccess.setPixel(Vec4(clrData[0]),											x, y); break;
							case TextureFormat::RGBA:	resultSliceAccess.setPixel(Vec4(clrData[0], clrData[1], clrData[2], clrData[3]),		x, y); break;
							default:
								DE_ASSERT(false);
						}
					}
				}

				if (!verifyLayer(log, resultSliceAccess, sliceOrFaceNdx))
					return false;
			}
		}

		return true;
	}
}

//! Read buffer texture by reading the corresponding buffer with a mapping.
static bool readBufferTextureWithMappingAndVerify (const RenderContext&			renderCtx,
												   glu::CallLogWrapper&			glLog,
												   deUint32						bufferGL,
												   const TextureFormat&			textureFormat,
												   int							imageSize,
												   const ImageLayerVerifier&	verifyLayer)
{
	tcu::TextureLevel			result			(textureFormat, imageSize, 1);
	const PixelBufferAccess		resultAccess	= result.getAccess();
	const int					dataSize		= imageSize * textureFormat.getPixelSize();

	const tcu::ScopedLogSection section(glLog.getLog(), "Verification", "Result verification (read texture's buffer with a mapping)");
	glLog.glBindBuffer(GL_TEXTURE_BUFFER, bufferGL);

	{
		const BufferMemMap bufMap(renderCtx.getFunctions(), GL_TEXTURE_BUFFER, 0, dataSize, GL_MAP_READ_BIT);
		deMemcpy(resultAccess.getDataPtr(), bufMap.getPtr(), dataSize);
	}

	return verifyLayer(glLog.getLog(), resultAccess, 0);
}

//! Calls the appropriate texture verification function depending on texture format or type.
static bool readTextureAndVerify (const RenderContext&			renderCtx,
								  glu::CallLogWrapper&			glLog,
								  deUint32						textureGL,
								  deUint32						bufferGL,
								  TextureType					textureType,
								  const TextureFormat&			textureFormat,
								  const IVec3&					imageSize,
								  const ImageLayerVerifier&		verifyLayer)
{
	if (textureType == TEXTURETYPE_BUFFER)
		return readBufferTextureWithMappingAndVerify(renderCtx, glLog, bufferGL, textureFormat, imageSize.x(), verifyLayer);
	else
		return isFormatTypeInteger(textureFormat.type) ? readIntegerTextureViaFBOAndVerify				(renderCtx, glLog, textureGL, textureType, textureFormat, imageSize, verifyLayer)
													   : readFloatOrNormTextureWithLookupsAndVerify		(renderCtx, glLog, textureGL, textureType, textureFormat, imageSize, verifyLayer);
}

//! An ImageLayerVerifier that simply compares the result slice to a slice in a reference image.
//! \note Holds the reference image as a reference (no pun intended) instead of a copy; caller must be aware of lifetime issues.
class ImageLayerComparer : public ImageLayerVerifier
{
public:
	ImageLayerComparer (const LayeredImage& reference,
						const IVec2& relevantRegion = IVec2(0) /* If given, only check this region of each slice. */)
		: m_reference		(reference)
		, m_relevantRegion	(relevantRegion.x() > 0 && relevantRegion.y() > 0 ? relevantRegion : reference.getSize().swizzle(0, 1))
	{
	}

	bool operator() (TestLog& log, const tcu::ConstPixelBufferAccess& resultSlice, int sliceOrFaceNdx) const
	{
		const bool						isCube				= m_reference.getImageType() == TEXTURETYPE_CUBE;
		const ConstPixelBufferAccess	referenceSlice		= tcu::getSubregion(isCube ? m_reference.getCubeFaceAccess(glslImageFuncZToCubeFace(sliceOrFaceNdx))
																					   : m_reference.getSliceAccess(sliceOrFaceNdx),
																				0, 0, m_relevantRegion.x(), m_relevantRegion.y());

		const string comparisonName = "Comparison" + toString(sliceOrFaceNdx);
		const string comparisonDesc = "Image Comparison, "
									+ (isCube ? "face " + string(glu::getCubeMapFaceName(cubeFaceToGLFace(glslImageFuncZToCubeFace(sliceOrFaceNdx))))
											  : "slice " + toString(sliceOrFaceNdx));

		if (isFormatTypeInteger(m_reference.getFormat().type))
			return tcu::intThresholdCompare(log, comparisonName.c_str(), comparisonDesc.c_str(), referenceSlice, resultSlice, UVec4(0), tcu::COMPARE_LOG_RESULT);
		else
			return tcu::floatThresholdCompare(log, comparisonName.c_str(), comparisonDesc.c_str(), referenceSlice, resultSlice, Vec4(0.01f), tcu::COMPARE_LOG_RESULT);
	}

private:
	const LayeredImage&		m_reference;
	const IVec2				m_relevantRegion;
};

//! Case that just stores some computation results into an image.
class ImageStoreCase : public TestCase
{
public:
	enum CaseFlag
	{
		CASEFLAG_SINGLE_LAYER_BIND = 1 << 0 //!< If given, glBindImageTexture() is called with GL_FALSE <layered> argument, and for each layer the compute shader is separately dispatched.
	};

	ImageStoreCase (Context& context, const char* name, const char* description, const TextureFormat& format, TextureType textureType, deUint32 caseFlags = 0)
		: TestCase				(context, name, description)
		, m_format				(format)
		, m_textureType			(textureType)
		, m_singleLayerBind		((caseFlags & CASEFLAG_SINGLE_LAYER_BIND) != 0)
	{
	}

	void			init		(void) { checkTextureTypeExtensions(m_context.getContextInfo(), m_textureType, m_context.getRenderContext()); }
	IterateResult	iterate		(void);

private:
	const TextureFormat		m_format;
	const TextureType		m_textureType;
	const bool				m_singleLayerBind;
};

ImageStoreCase::IterateResult ImageStoreCase::iterate (void)
{
	const RenderContext&		renderCtx				= m_context.getRenderContext();
	TestLog&					log						(m_testCtx.getLog());
	glu::CallLogWrapper			glLog					(renderCtx.getFunctions(), log);
	const deUint32				internalFormatGL		= glu::getInternalFormat(m_format);
	const deUint32				textureTargetGL			= getGLTextureTarget(m_textureType);
	const IVec3&				imageSize				= defaultImageSize(m_textureType);
	const int					numSlicesOrFaces		= m_textureType == TEXTURETYPE_CUBE ? 6 : imageSize.z();
	const int					maxImageDimension		= de::max(imageSize.x(), de::max(imageSize.y(), imageSize.z()));
	const float					storeColorScale			= isFormatTypeUnorm(m_format.type) ? 1.0f / (float)(maxImageDimension - 1)
														: isFormatTypeSnorm(m_format.type) ? 2.0f / (float)(maxImageDimension - 1)
														: 1.0f;
	const float					storeColorBias			= isFormatTypeSnorm(m_format.type) ? -1.0f : 0.0f;
	const glu::Buffer			textureBuf				(renderCtx); // \note Only really used if using buffer texture.
	const glu::Texture			texture					(renderCtx);

	glLog.enableLogging(true);

	// Setup texture.

	log << TestLog::Message << "// Created a texture (name " << *texture << ")" << TestLog::EndMessage;
	if (m_textureType == TEXTURETYPE_BUFFER)
		log << TestLog::Message << "// Created a buffer for the texture (name " << *textureBuf << ")" << TestLog::EndMessage;

	glLog.glActiveTexture(GL_TEXTURE0);
	glLog.glBindTexture(textureTargetGL, *texture);
	setTexParameteri(glLog, textureTargetGL);
	setTextureStorage(glLog, m_textureType, internalFormatGL, imageSize, *textureBuf);

	// Perform image stores in compute shader.

	{
		// Generate compute shader.

		const string		shaderImageFormatStr	= getShaderImageFormatQualifier(m_format);
		const TextureType	shaderImageType			= m_singleLayerBind ? textureLayerType(m_textureType) : m_textureType;
		const string		shaderImageTypeStr		= getShaderImageType(m_format.type, shaderImageType);
		const bool			isUintFormat			= isFormatTypeUnsignedInteger(m_format.type);
		const bool			isIntFormat				= isFormatTypeSignedInteger(m_format.type);
		const string		colorBaseExpr			= string(isUintFormat ? "u" : isIntFormat ? "i" : "") + "vec4(gx^gy^gz, "
																												 "(" + toString(imageSize.x()-1) + "-gx)^gy^gz, "
																												 "gx^(" + toString(imageSize.y()-1) + "-gy)^gz, "
																												 "(" + toString(imageSize.x()-1) + "-gx)^(" + toString(imageSize.y()-1) + "-gy)^gz)";
		const string		colorExpr				= colorBaseExpr + (storeColorScale == 1.0f ? "" : "*" + toString(storeColorScale))
																	+ (storeColorBias == 0.0f ? "" : " + float(" + toString(storeColorBias) + ")");
		const std::string	glslVersionDeclaration	= glu::getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(renderCtx.getType()));

		const glu::ShaderProgram program(renderCtx,
			glu::ProgramSources() << glu::ComputeSource(glslVersionDeclaration + "\n"
														+ textureTypeExtensionShaderRequires(shaderImageType, renderCtx) +
														"\n"
														"precision highp " + shaderImageTypeStr + ";\n"
														"\n"
														"layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
														"layout (" + shaderImageFormatStr + ", binding=0) writeonly uniform " + shaderImageTypeStr + " u_image;\n"
														+ (m_singleLayerBind ? "uniform int u_layerNdx;\n" : "") +
														"\n"
														"void main (void)\n"
														"{\n"
														"	int gx = int(gl_GlobalInvocationID.x);\n"
														"	int gy = int(gl_GlobalInvocationID.y);\n"
														"	int gz = " + (m_singleLayerBind ? "u_layerNdx" : "int(gl_GlobalInvocationID.z)") + ";\n"
														+ (shaderImageType == TEXTURETYPE_BUFFER ?
															"	imageStore(u_image, gx, " + colorExpr + ");\n"
														 : shaderImageType == TEXTURETYPE_2D ?
															"	imageStore(u_image, ivec2(gx, gy), " + colorExpr + ");\n"
														 : shaderImageType == TEXTURETYPE_3D || shaderImageType == TEXTURETYPE_CUBE || shaderImageType == TEXTURETYPE_2D_ARRAY ?
															"	imageStore(u_image, ivec3(gx, gy, gz), " + colorExpr + ");\n"
														 : DE_NULL) +
														"}\n"));

		UniformAccessLogger uniforms(renderCtx.getFunctions(), log, program.getProgram());

		log << program;

		if (!program.isOk())
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Program compilation failed");
			return STOP;
		}

		// Setup and dispatch.

		glLog.glUseProgram(program.getProgram());

		if (m_singleLayerBind)
		{
			for (int layerNdx = 0; layerNdx < numSlicesOrFaces; layerNdx++)
			{
				if (layerNdx > 0)
					glLog.glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

				uniforms.assign1i("u_layerNdx", layerNdx);

				glLog.glBindImageTexture(0, *texture, 0, GL_FALSE, layerNdx, GL_WRITE_ONLY, internalFormatGL);
				GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glBindImageTexture");

				glLog.glDispatchCompute(imageSize.x(), imageSize.y(), 1);
				GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glDispatchCompute");
			}
		}
		else
		{
			glLog.glBindImageTexture(0, *texture, 0, GL_TRUE, 0, GL_WRITE_ONLY, internalFormatGL);
			GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glBindImageTexture");

			glLog.glDispatchCompute(imageSize.x(), imageSize.y(), numSlicesOrFaces);
			GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glDispatchCompute");
		}
	}

	// Create reference, read texture and compare to reference.
	{
		const int		isIntegerFormat		= isFormatTypeInteger(m_format.type);
		LayeredImage	reference			(m_textureType, m_format, imageSize.x(), imageSize.y(), imageSize.z());

		DE_ASSERT(!isIntegerFormat || (storeColorScale == 1.0f && storeColorBias == 0.0f));

		for (int z = 0; z < numSlicesOrFaces; z++)
		for (int y = 0; y < imageSize.y(); y++)
		for (int x = 0; x < imageSize.x(); x++)
		{
			const IVec4 color(x^y^z, (imageSize.x()-1-x)^y^z, x^(imageSize.y()-1-y)^z, (imageSize.x()-1-x)^(imageSize.y()-1-y)^z);

			if (isIntegerFormat)
				reference.setPixel(x, y, z, color);
			else
				reference.setPixel(x, y, z, color.asFloat()*storeColorScale + storeColorBias);
		}

		const bool compareOk = readTextureAndVerify(renderCtx, glLog, *texture, *textureBuf, m_textureType, m_format, imageSize, ImageLayerComparer(reference));

		m_testCtx.setTestResult(compareOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, compareOk ? "Pass" : "Image comparison failed");
		return STOP;
	}
}

//! Case that copies an image to another, using imageLoad() and imageStore(). Texture formats don't necessarily match image formats.
class ImageLoadAndStoreCase : public TestCase
{
public:
	enum CaseFlag
	{
		CASEFLAG_SINGLE_LAYER_BIND	= 1 << 0,	//!< If given, glBindImageTexture() is called with GL_FALSE <layered> argument, and for each layer the compute shader is separately dispatched.
		CASEFLAG_RESTRICT_IMAGES	= 1 << 1	//!< If given, images in shader will be qualified with "restrict".
	};

	ImageLoadAndStoreCase (Context& context, const char* name, const char* description, const TextureFormat& format, TextureType textureType, deUint32 caseFlags = 0)
		: TestCase				(context, name, description)
		, m_textureFormat		(format)
		, m_imageFormat			(format)
		, m_textureType			(textureType)
		, m_restrictImages		((caseFlags & CASEFLAG_RESTRICT_IMAGES)		!= 0)
		, m_singleLayerBind		((caseFlags & CASEFLAG_SINGLE_LAYER_BIND)	!= 0)
	{
	}

	ImageLoadAndStoreCase (Context& context, const char* name, const char* description, const TextureFormat& textureFormat, const TextureFormat& imageFormat, TextureType textureType, deUint32 caseFlags = 0)
		: TestCase				(context, name, description)
		, m_textureFormat		(textureFormat)
		, m_imageFormat			(imageFormat)
		, m_textureType			(textureType)
		, m_restrictImages		((caseFlags & CASEFLAG_RESTRICT_IMAGES)		!= 0)
		, m_singleLayerBind		((caseFlags & CASEFLAG_SINGLE_LAYER_BIND)	!= 0)
	{
		DE_ASSERT(textureFormat.getPixelSize() == imageFormat.getPixelSize());
	}

	void			init		(void) { checkTextureTypeExtensions(m_context.getContextInfo(), m_textureType, m_context.getRenderContext()); }
	IterateResult	iterate		(void);

private:
	template <TextureFormat::ChannelType ImageFormatType, typename TcuFloatType, typename TcuFloatStorageType>
	static void					replaceBadFloatReinterpretValues (LayeredImage& image, const TextureFormat& imageFormat);

	const TextureFormat			m_textureFormat;
	const TextureFormat			m_imageFormat;
	const TextureType			m_textureType;
	const bool					m_restrictImages;
	const bool					m_singleLayerBind;
};

template <TextureFormat::ChannelType ImageFormatType, typename TcuFloatType, typename TcuFloatTypeStorageType>
void ImageLoadAndStoreCase::replaceBadFloatReinterpretValues (LayeredImage& image, const TextureFormat& imageFormat)
{
	// Find potential bad values, such as nan or inf, and replace with something else.
	const int		pixelSize			= imageFormat.getPixelSize();
	const int		imageNumChannels	= imageFormat.order == tcu::TextureFormat::R	? 1
										: imageFormat.order == tcu::TextureFormat::RGBA	? 4
										: 0;
	const IVec3		imageSize			= image.getSize();
	const int		numSlicesOrFaces	= image.getImageType() == TEXTURETYPE_CUBE ? 6 : imageSize.z();

	DE_ASSERT(pixelSize % imageNumChannels == 0);

	for (int z = 0; z < numSlicesOrFaces; z++)
	{
		const PixelBufferAccess		sliceAccess		= image.getImageType() == TEXTURETYPE_CUBE ? image.getCubeFaceAccess((tcu::CubeFace)z) : image.getSliceAccess(z);
		const int					rowPitch		= sliceAccess.getRowPitch();
		void *const					data			= sliceAccess.getDataPtr();

		for (int y = 0; y < imageSize.y(); y++)
		for (int x = 0; x < imageSize.x(); x++)
		{
			void *const pixelData = (deUint8*)data + y*rowPitch + x*pixelSize;

			for (int c = 0; c < imageNumChannels; c++)
			{
				void *const			channelData		= (deUint8*)pixelData + c*pixelSize/imageNumChannels;
				const TcuFloatType	f				(*(TcuFloatTypeStorageType*)channelData);

				if (f.isDenorm() || f.isInf() || f.isNaN())
					*(TcuFloatTypeStorageType*)channelData = TcuFloatType(0.0f).bits();
			}
		}
	}
}

ImageLoadAndStoreCase::IterateResult ImageLoadAndStoreCase::iterate (void)
{
	const RenderContext&		renderCtx					= m_context.getRenderContext();
	TestLog&					log							(m_testCtx.getLog());
	glu::CallLogWrapper			glLog						(renderCtx.getFunctions(), log);
	const deUint32				textureInternalFormatGL		= glu::getInternalFormat(m_textureFormat);
	const deUint32				imageInternalFormatGL		= glu::getInternalFormat(m_imageFormat);
	const deUint32				textureTargetGL				= getGLTextureTarget(m_textureType);
	const IVec3&				imageSize					= defaultImageSize(m_textureType);
	const int					maxImageDimension			= de::max(imageSize.x(), de::max(imageSize.y(), imageSize.z()));
	const float					storeColorScale				= isFormatTypeUnorm(m_textureFormat.type) ? 1.0f / (float)(maxImageDimension - 1)
															: isFormatTypeSnorm(m_textureFormat.type) ? 2.0f / (float)(maxImageDimension - 1)
															: 1.0f;
	const float					storeColorBias				= isFormatTypeSnorm(m_textureFormat.type) ? -1.0f : 0.0f;
	const int					numSlicesOrFaces			= m_textureType == TEXTURETYPE_CUBE ? 6 : imageSize.z();
	const bool					isIntegerTextureFormat		= isFormatTypeInteger(m_textureFormat.type);
	const glu::Buffer			texture0Buf					(renderCtx);
	const glu::Buffer			texture1Buf					(renderCtx);
	const glu::Texture			texture0					(renderCtx);
	const glu::Texture			texture1					(renderCtx);
	LayeredImage				reference					(m_textureType, m_textureFormat, imageSize.x(), imageSize.y(), imageSize.z());

	glLog.enableLogging(true);

	// Setup textures.

	log << TestLog::Message << "// Created 2 textures (names " << *texture0 << " and " << *texture1 << ")" << TestLog::EndMessage;
	if (m_textureType == TEXTURETYPE_BUFFER)
		log << TestLog::Message << "// Created buffers for the textures (names " << *texture0Buf << " and " << *texture1Buf << ")" << TestLog::EndMessage;

	// First, fill reference with (a fairly arbitrary) initial pattern. This will be used as texture upload source data as well as for actual reference computation later on.

	DE_ASSERT(!isIntegerTextureFormat || (storeColorScale == 1.0f && storeColorBias == 0.0f));

	for (int z = 0; z < numSlicesOrFaces; z++)
	for (int y = 0; y < imageSize.y(); y++)
	for (int x = 0; x < imageSize.x(); x++)
	{
		const IVec4 color(x^y^z, (imageSize.x()-1-x)^y^z, x^(imageSize.y()-1-y)^z, (imageSize.x()-1-x)^(imageSize.y()-1-y)^z);

		if (isIntegerTextureFormat)
			reference.setPixel(x, y, z, color);
		else
			reference.setPixel(x, y, z, color.asFloat()*storeColorScale + storeColorBias);
	}

	// If re-interpreting the texture contents as floating point values, need to get rid of inf, nan etc.
	if (m_imageFormat.type == TextureFormat::HALF_FLOAT && m_textureFormat.type != TextureFormat::HALF_FLOAT)
		replaceBadFloatReinterpretValues<TextureFormat::HALF_FLOAT, tcu::Float16, deUint16>(reference, m_imageFormat);
	else if (m_imageFormat.type == TextureFormat::FLOAT && m_textureFormat.type != TextureFormat::FLOAT)
		replaceBadFloatReinterpretValues<TextureFormat::FLOAT, tcu::Float32, deUint32>(reference, m_imageFormat);

	// Upload initial pattern to texture 0.

	glLog.glActiveTexture(GL_TEXTURE0);
	glLog.glBindTexture(textureTargetGL, *texture0);
	setTexParameteri(glLog, textureTargetGL);

	log << TestLog::Message << "// Filling texture " << *texture0 << " with xor pattern" << TestLog::EndMessage;

	uploadTexture(glLog, reference, *texture0Buf);

	// Set storage for texture 1.

	glLog.glActiveTexture(GL_TEXTURE1);
	glLog.glBindTexture(textureTargetGL, *texture1);
	setTexParameteri(glLog, textureTargetGL);
	setTextureStorage(glLog, m_textureType, textureInternalFormatGL, imageSize, *texture1Buf);

	// Perform image loads and stores in compute shader and finalize reference computation.

	{
		// Generate compute shader.

		const char* const		maybeRestrict			= m_restrictImages ? "restrict" : "";
		const string			shaderImageFormatStr	= getShaderImageFormatQualifier(m_imageFormat);
		const TextureType		shaderImageType			= m_singleLayerBind ? textureLayerType(m_textureType) : m_textureType;
		const string			shaderImageTypeStr		= getShaderImageType(m_imageFormat.type, shaderImageType);
		const std::string		glslVersionDeclaration	= glu::getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(renderCtx.getType()));

		const glu::ShaderProgram program(renderCtx,
			glu::ProgramSources() << glu::ComputeSource(glslVersionDeclaration + "\n"
														+ textureTypeExtensionShaderRequires(shaderImageType, renderCtx) +
														"\n"
														"precision highp " + shaderImageTypeStr + ";\n"
														"\n"
														"layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
														"layout (" + shaderImageFormatStr + ", binding=0) " + maybeRestrict + " readonly uniform " + shaderImageTypeStr + " u_image0;\n"
														"layout (" + shaderImageFormatStr + ", binding=1) " + maybeRestrict + " writeonly uniform " + shaderImageTypeStr + " u_image1;\n"
														"\n"
														"void main (void)\n"
														"{\n"
														+ (shaderImageType == TEXTURETYPE_BUFFER ?
															"	int pos = int(gl_GlobalInvocationID.x);\n"
															"	imageStore(u_image1, pos, imageLoad(u_image0, " + toString(imageSize.x()-1) + "-pos));\n"
														 : shaderImageType == TEXTURETYPE_2D ?
															"	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);\n"
															"	imageStore(u_image1, pos, imageLoad(u_image0, ivec2(" + toString(imageSize.x()-1) + "-pos.x, pos.y)));\n"
														 : shaderImageType == TEXTURETYPE_3D || shaderImageType == TEXTURETYPE_CUBE || shaderImageType == TEXTURETYPE_2D_ARRAY ?
															"	ivec3 pos = ivec3(gl_GlobalInvocationID);\n"
															"	imageStore(u_image1, pos, imageLoad(u_image0, ivec3(" + toString(imageSize.x()-1) + "-pos.x, pos.y, pos.z)));\n"
														 : DE_NULL) +
														"}\n"));

		UniformAccessLogger uniforms(renderCtx.getFunctions(), log, program.getProgram());

		log << program;

		if (!program.isOk())
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Program compilation failed");
			return STOP;
		}

		// Setup and dispatch.

		glLog.glUseProgram(program.getProgram());

		if (m_singleLayerBind)
		{
			for (int layerNdx = 0; layerNdx < numSlicesOrFaces; layerNdx++)
			{
				if (layerNdx > 0)
					glLog.glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

				glLog.glBindImageTexture(0, *texture0, 0, GL_FALSE, layerNdx, GL_READ_ONLY, imageInternalFormatGL);
				GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glBindImageTexture");

				glLog.glBindImageTexture(1, *texture1, 0, GL_FALSE, layerNdx, GL_WRITE_ONLY, imageInternalFormatGL);
				GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glBindImageTexture");

				glLog.glDispatchCompute(imageSize.x(), imageSize.y(), 1);
				GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glDispatchCompute");
			}
		}
		else
		{
			glLog.glBindImageTexture(0, *texture0, 0, GL_TRUE, 0, GL_READ_ONLY, imageInternalFormatGL);
			GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glBindImageTexture");

			glLog.glBindImageTexture(1, *texture1, 0, GL_TRUE, 0, GL_WRITE_ONLY, imageInternalFormatGL);
			GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glBindImageTexture");

			glLog.glDispatchCompute(imageSize.x(), imageSize.y(), numSlicesOrFaces);
			GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glDispatchCompute");
		}

		// Finalize reference.

		if (m_textureFormat != m_imageFormat)
		{
			// Format re-interpretation case. Read data with image format and write back, with the same image format.
			// We do this because the data may change a little during lookups (e.g. unorm8 -> float; not all unorms can be exactly represented as floats).

			const int					pixelSize		= m_imageFormat.getPixelSize();
			tcu::TextureLevel			scratch			(m_imageFormat, 1, 1);
			const PixelBufferAccess		scratchAccess	= scratch.getAccess();

			for (int z = 0; z < numSlicesOrFaces; z++)
			{
				const PixelBufferAccess		sliceAccess		= m_textureType == TEXTURETYPE_CUBE ? reference.getCubeFaceAccess((tcu::CubeFace)z) : reference.getSliceAccess(z);
				const int					rowPitch		= sliceAccess.getRowPitch();
				void *const					data			= sliceAccess.getDataPtr();

				for (int y = 0; y < imageSize.y(); y++)
				for (int x = 0; x < imageSize.x(); x++)
				{
					void *const pixelData = (deUint8*)data + y*rowPitch + x*pixelSize;

					deMemcpy(scratchAccess.getDataPtr(), pixelData, pixelSize);

					if (isFormatTypeInteger(m_imageFormat.type))
						scratchAccess.setPixel(scratchAccess.getPixelUint(0, 0), 0, 0);
					else
						scratchAccess.setPixel(scratchAccess.getPixel(0, 0), 0, 0);

					deMemcpy(pixelData, scratchAccess.getDataPtr(), pixelSize);
				}
			}
		}

		for (int z = 0; z < numSlicesOrFaces; z++)
		for (int y = 0; y < imageSize.y(); y++)
		for (int x = 0; x < imageSize.x()/2; x++)
		{
			if (isIntegerTextureFormat)
			{
				const UVec4 temp = reference.getPixelUint(imageSize.x()-1-x, y, z);
				reference.setPixel(imageSize.x()-1-x, y, z, reference.getPixelUint(x, y, z));
				reference.setPixel(x, y, z, temp);
			}
			else
			{
				const Vec4 temp = reference.getPixel(imageSize.x()-1-x, y, z);
				reference.setPixel(imageSize.x()-1-x, y, z, reference.getPixel(x, y, z));
				reference.setPixel(x, y, z, temp);
			}
		}
	}

	// Read texture 1 and compare to reference.

	const bool compareOk = readTextureAndVerify(renderCtx, glLog, *texture1, *texture1Buf, m_textureType, m_textureFormat, imageSize, ImageLayerComparer(reference));

	m_testCtx.setTestResult(compareOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, compareOk ? "Pass" : "Image comparison failed");
	return STOP;
}

enum AtomicOperationCaseType
{
	ATOMIC_OPERATION_CASE_TYPE_END_RESULT = 0,	//!< Atomic case checks the end result of the operations, and not the return values.
	ATOMIC_OPERATION_CASE_TYPE_RETURN_VALUES,	//!< Atomic case checks the return values of the atomic function, and not the end result.

	ATOMIC_OPERATION_CASE_TYPE_LAST
};

/*--------------------------------------------------------------------*//*!
 * \brief Binary atomic operation case.
 *
 * Case that performs binary atomic operations (i.e. any but compSwap) and
 * verifies according to the given AtomicOperationCaseType.
 *
 * For the "end result" case type, a single texture (and image) is created,
 * upon which the atomic operations operate. A compute shader is dispatched
 * with dimensions equal to the image size, except with a bigger X size
 * so that every pixel is operated on by multiple invocations. The end
 * results are verified in BinaryAtomicOperationCase::EndResultVerifier.
 * The return values of the atomic function calls are ignored.
 *
 * For the "return value" case type, the case does much the same operations
 * as in the "end result" case, but also creates an additional texture,
 * of size equal to the dispatch size, into which the return values of the
 * atomic functions are stored (with imageStore()). The return values are
 * verified in BinaryAtomicOperationCase::ReturnValueVerifier.
 * The end result values are not checked.
 *
 * The compute shader invocations contributing to a pixel (X, Y, Z) in the
 * end result image are the invocations with global IDs
 * (X, Y, Z), (X+W, Y, Z), (X+2*W, Y, Z), ..., (X+(N-1)*W, Y, W), where W
 * is the width of the end result image and N is
 * BinaryAtomicOperationCase::NUM_INVOCATIONS_PER_PIXEL.
 *//*--------------------------------------------------------------------*/
class BinaryAtomicOperationCase : public TestCase
{
public:
									BinaryAtomicOperationCase		(Context& context, const char* name, const char* description, const TextureFormat& format, TextureType imageType, AtomicOperation operation, AtomicOperationCaseType caseType)
		: TestCase		(context, name, description)
		, m_format		(format)
		, m_imageType	(imageType)
		, m_operation	(operation)
		, m_caseType	(caseType)
	{
		DE_ASSERT(m_format == TextureFormat(TextureFormat::R, TextureFormat::UNSIGNED_INT32)	||
				  m_format == TextureFormat(TextureFormat::R, TextureFormat::SIGNED_INT32)		||
				  (m_format == TextureFormat(TextureFormat::R, TextureFormat::FLOAT) && m_operation == ATOMIC_OPERATION_EXCHANGE));

		DE_ASSERT(m_operation != ATOMIC_OPERATION_COMP_SWAP);
	}

	void							init							(void);
	IterateResult					iterate							(void);

private:
	class EndResultVerifier;
	class ReturnValueVerifier;

	static int						getOperationInitialValue		(AtomicOperation op); //!< Appropriate value with which to initialize the texture.
	//! Compute the argument given to the atomic function at the given invocation ID, when the entire dispatch has the given width and height.
	static int						getAtomicFuncArgument			(AtomicOperation op, const IVec3& invocationID, const IVec2& dispatchSizeXY);
	//! Generate the shader expression for the argument given to the atomic function. x, y and z are the identifiers for the invocation ID components.
	static string					getAtomicFuncArgumentShaderStr	(AtomicOperation op, const string& x, const string& y, const string& z, const IVec2& dispatchSizeXY);

	static const int				NUM_INVOCATIONS_PER_PIXEL = 5;

	const TextureFormat				m_format;
	const TextureType				m_imageType;
	const AtomicOperation			m_operation;
	const AtomicOperationCaseType	m_caseType;
};

int BinaryAtomicOperationCase::getOperationInitialValue (AtomicOperation op)
{
	switch (op)
	{
		// \note 18 is just an arbitrary small nonzero value.
		case ATOMIC_OPERATION_ADD:			return 18;
		case ATOMIC_OPERATION_MIN:			return (1<<15) - 1;
		case ATOMIC_OPERATION_MAX:			return 18;
		case ATOMIC_OPERATION_AND:			return (1<<15) - 1;
		case ATOMIC_OPERATION_OR:			return 18;
		case ATOMIC_OPERATION_XOR:			return 18;
		case ATOMIC_OPERATION_EXCHANGE:		return 18;
		default:
			DE_ASSERT(false);
			return -1;
	}
}

int BinaryAtomicOperationCase::getAtomicFuncArgument (AtomicOperation op, const IVec3& invocationID, const IVec2& dispatchSizeXY)
{
	const int x		= invocationID.x();
	const int y		= invocationID.y();
	const int z		= invocationID.z();
	const int wid	= dispatchSizeXY.x();
	const int hei	= dispatchSizeXY.y();

	switch (op)
	{
		// \note Fall-throughs.
		case ATOMIC_OPERATION_ADD:
		case ATOMIC_OPERATION_MIN:
		case ATOMIC_OPERATION_MAX:
		case ATOMIC_OPERATION_AND:
		case ATOMIC_OPERATION_OR:
		case ATOMIC_OPERATION_XOR:
			return x*x + y*y + z*z;

		case ATOMIC_OPERATION_EXCHANGE:
			return (z*wid + x)*hei + y;

		default:
			DE_ASSERT(false);
			return -1;
	}
}

string BinaryAtomicOperationCase::getAtomicFuncArgumentShaderStr (AtomicOperation op, const string& x, const string& y, const string& z, const IVec2& dispatchSizeXY)
{
	switch (op)
	{
		// \note Fall-throughs.
		case ATOMIC_OPERATION_ADD:
		case ATOMIC_OPERATION_MIN:
		case ATOMIC_OPERATION_MAX:
		case ATOMIC_OPERATION_AND:
		case ATOMIC_OPERATION_OR:
		case ATOMIC_OPERATION_XOR:
			return "("+ x+"*"+x +" + "+ y+"*"+y +" + "+ z+"*"+z +")";

		case ATOMIC_OPERATION_EXCHANGE:
			return "((" + z + "*" + toString(dispatchSizeXY.x()) + " + " + x + ")*" + toString(dispatchSizeXY.y()) + " + " + y + ")";

		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

class BinaryAtomicOperationCase::EndResultVerifier : public ImageLayerVerifier
{
public:
	EndResultVerifier (AtomicOperation operation, TextureType imageType) : m_operation(operation), m_imageType(imageType) {}

	bool operator() (TestLog& log, const ConstPixelBufferAccess& resultSlice, int sliceOrFaceNdx) const
	{
		const bool		isIntegerFormat		= isFormatTypeInteger(resultSlice.getFormat().type);
		const IVec2		dispatchSizeXY		(NUM_INVOCATIONS_PER_PIXEL*resultSlice.getWidth(), resultSlice.getHeight());

		log << TestLog::Image("EndResults" + toString(sliceOrFaceNdx),
							  "Result Values, " + (m_imageType == TEXTURETYPE_CUBE ? "face " + string(glu::getCubeMapFaceName(cubeFaceToGLFace(glslImageFuncZToCubeFace(sliceOrFaceNdx))))
																				   : "slice " + toString(sliceOrFaceNdx)),
							  resultSlice);

		for (int y = 0; y < resultSlice.getHeight(); y++)
		for (int x = 0; x < resultSlice.getWidth(); x++)
		{
			union
			{
				int		i;
				float	f;
			} result;

			if (isIntegerFormat)
				result.i = resultSlice.getPixelInt(x, y).x();
			else
				result.f = resultSlice.getPixel(x, y).x();

			// Compute the arguments that were given to the atomic function in the invocations that contribute to this pixel.

			IVec3	invocationGlobalIDs[NUM_INVOCATIONS_PER_PIXEL];
			int		atomicArgs[NUM_INVOCATIONS_PER_PIXEL];

			for (int i = 0; i < NUM_INVOCATIONS_PER_PIXEL; i++)
			{
				const IVec3 gid(x + i*resultSlice.getWidth(), y, sliceOrFaceNdx);

				invocationGlobalIDs[i]	= gid;
				atomicArgs[i]			= getAtomicFuncArgument(m_operation, gid, dispatchSizeXY);
			}

			if (isOrderIndependentAtomicOperation(m_operation))
			{
				// Just accumulate the atomic args (and the initial value) according to the operation, and compare.

				DE_ASSERT(isIntegerFormat);

				int reference = getOperationInitialValue(m_operation);

				for (int i = 0; i < NUM_INVOCATIONS_PER_PIXEL; i++)
					reference = computeBinaryAtomicOperationResult(m_operation, reference, atomicArgs[i]);

				if (result.i != reference)
				{
					log << TestLog::Message << "// Failure: end result at pixel " << IVec2(x, y) << " of current layer is " << result.i << TestLog::EndMessage
						<< TestLog::Message << "// Note: relevant shader invocation global IDs are " << arrayStr(invocationGlobalIDs) << TestLog::EndMessage
						<< TestLog::Message << "// Note: data expression values for the IDs are " << arrayStr(atomicArgs) << TestLog::EndMessage
						<< TestLog::Message << "// Note: reference value is " << reference << TestLog::EndMessage;
					return false;
				}
			}
			else if (m_operation == ATOMIC_OPERATION_EXCHANGE)
			{
				// Check that the end result equals one of the atomic args.

				bool matchFound = false;

				for (int i = 0; i < NUM_INVOCATIONS_PER_PIXEL && !matchFound; i++)
					matchFound = isIntegerFormat ? result.i == atomicArgs[i]
												 : de::abs(result.f - (float)atomicArgs[i]) <= 0.01f;

				if (!matchFound)
				{
					log << TestLog::Message << "// Failure: invalid value at pixel " << IVec2(x, y) << ": got " << (isIntegerFormat ? toString(result.i) : toString(result.f)) << TestLog::EndMessage
											<< TestLog::Message << "// Note: expected one of " << arrayStr(atomicArgs) << TestLog::EndMessage;

					return false;
				}
			}
			else
				DE_ASSERT(false);
		}

		return true;
	}

private:
	const AtomicOperation	m_operation;
	const TextureType		m_imageType;
};

class BinaryAtomicOperationCase::ReturnValueVerifier : public ImageLayerVerifier
{
public:
	//! \note endResultImageLayerSize is (width, height) of the image operated on by the atomic ops, and not the size of the image where the return values are stored.
	ReturnValueVerifier (AtomicOperation operation, TextureType imageType, const IVec2& endResultImageLayerSize) : m_operation(operation), m_imageType(imageType), m_endResultImageLayerSize(endResultImageLayerSize) {}

	bool operator() (TestLog& log, const ConstPixelBufferAccess& resultSlice, int sliceOrFaceNdx) const
	{
		const bool		isIntegerFormat		(isFormatTypeInteger(resultSlice.getFormat().type));
		const IVec2		dispatchSizeXY	(resultSlice.getWidth(), resultSlice.getHeight());

		DE_ASSERT(resultSlice.getWidth()	== NUM_INVOCATIONS_PER_PIXEL*m_endResultImageLayerSize.x()	&&
				  resultSlice.getHeight()	== m_endResultImageLayerSize.y()							&&
				  resultSlice.getDepth()	== 1);

		log << TestLog::Image("ReturnValues" + toString(sliceOrFaceNdx),
							  "Per-Invocation Return Values, "
								   + (m_imageType == TEXTURETYPE_CUBE ? "face " + string(glu::getCubeMapFaceName(cubeFaceToGLFace(glslImageFuncZToCubeFace(sliceOrFaceNdx))))
																	  : "slice " + toString(sliceOrFaceNdx)),
							  resultSlice);

		for (int y = 0; y < m_endResultImageLayerSize.y(); y++)
		for (int x = 0; x < m_endResultImageLayerSize.x(); x++)
		{
			union IntFloatArr
			{
				int		i[NUM_INVOCATIONS_PER_PIXEL];
				float	f[NUM_INVOCATIONS_PER_PIXEL];
			};

			// Get the atomic function args and return values for all the invocations that contribute to the pixel (x, y) in the current end result slice.

			IntFloatArr		returnValues;
			IntFloatArr		atomicArgs;
			IVec3			invocationGlobalIDs[NUM_INVOCATIONS_PER_PIXEL];
			IVec2			pixelCoords[NUM_INVOCATIONS_PER_PIXEL];

			for (int i = 0; i < NUM_INVOCATIONS_PER_PIXEL; i++)
			{
				const IVec2 pixCoord	(x + i*m_endResultImageLayerSize.x(), y);
				const IVec3 gid			(pixCoord.x(), pixCoord.y(), sliceOrFaceNdx);

				invocationGlobalIDs[i]	= gid;
				pixelCoords[i]			= pixCoord;

				if (isIntegerFormat)
				{
					returnValues.i[i]	= resultSlice.getPixelInt(gid.x(), y).x();
					atomicArgs.i[i]		= getAtomicFuncArgument(m_operation, gid, dispatchSizeXY);
				}
				else
				{
					returnValues.f[i]	= resultSlice.getPixel(gid.x(), y).x();
					atomicArgs.f[i]		= (float)getAtomicFuncArgument(m_operation, gid, dispatchSizeXY);
				}
			}

			// Verify that the return values form a valid sequence.

			{
				const bool success = isIntegerFormat ? verifyOperationAccumulationIntermediateValues(m_operation,
																									 getOperationInitialValue(m_operation),
																									 atomicArgs.i,
																									 returnValues.i)

													 : verifyOperationAccumulationIntermediateValues(m_operation,
																									 (float)getOperationInitialValue(m_operation),
																									 atomicArgs.f,
																									 returnValues.f);

				if (!success)
				{
					log << TestLog::Message << "// Failure: intermediate return values at pixels " << arrayStr(pixelCoords) << " of current layer are "
											<< (isIntegerFormat ? arrayStr(returnValues.i) : arrayStr(returnValues.f)) << TestLog::EndMessage
						<< TestLog::Message << "// Note: relevant shader invocation global IDs are " << arrayStr(invocationGlobalIDs) << TestLog::EndMessage
						<< TestLog::Message << "// Note: data expression values for the IDs are "
											<< (isIntegerFormat ? arrayStr(atomicArgs.i) : arrayStr(atomicArgs.f))
											<< "; return values are not a valid result for any order of operations" << TestLog::EndMessage;
					return false;
				}
			}
		}

		return true;
	}

private:
	const AtomicOperation	m_operation;
	const TextureType		m_imageType;
	const IVec2				m_endResultImageLayerSize;

	//! Check whether there exists an ordering of args such that { init*A", init*A*B, ..., init*A*B*...*LAST } is the "returnValues" sequence, where { A, B, ..., LAST } is args, and * denotes the operation.
	//	That is, whether "returnValues" is a valid sequence of intermediate return values when "operation" has been accumulated on "args" (and "init") in some arbitrary order.
	template <typename T>
	static bool verifyOperationAccumulationIntermediateValues (AtomicOperation operation, T init, const T (&args)[NUM_INVOCATIONS_PER_PIXEL], const T (&returnValues)[NUM_INVOCATIONS_PER_PIXEL])
	{
		bool argsUsed[NUM_INVOCATIONS_PER_PIXEL] = { false };

		return verifyRecursive(operation, 0, init, argsUsed, args, returnValues);
	}

	static bool compare (int a, int b)		{ return a == b; }
	static bool compare (float a, float b)	{ return de::abs(a - b) <= 0.01f; }

	//! Depth-first search for verifying the return value sequence.
	template <typename T>
	static bool verifyRecursive (AtomicOperation operation, int index, T valueSoFar, bool (&argsUsed)[NUM_INVOCATIONS_PER_PIXEL], const T (&args)[NUM_INVOCATIONS_PER_PIXEL], const T (&returnValues)[NUM_INVOCATIONS_PER_PIXEL])
	{
		if (index < NUM_INVOCATIONS_PER_PIXEL)
		{
			for (int i = 0; i < NUM_INVOCATIONS_PER_PIXEL; i++)
			{
				if (!argsUsed[i] && compare(returnValues[i], valueSoFar))
				{
					argsUsed[i] = true;
					if (verifyRecursive(operation, index+1, computeBinaryAtomicOperationResult(operation, valueSoFar, args[i]), argsUsed, args, returnValues))
						return true;
					argsUsed[i] = false;
				}
			}

			return false;
		}
		else
			return true;
	}
};

void BinaryAtomicOperationCase::init (void)
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_OES_shader_image_atomic") && !glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		throw tcu::NotSupportedError("Test requires OES_shader_image_atomic extension");

	checkTextureTypeExtensions(m_context.getContextInfo(), m_imageType, m_context.getRenderContext());
}

BinaryAtomicOperationCase::IterateResult BinaryAtomicOperationCase::iterate (void)
{
	const RenderContext&		renderCtx				= m_context.getRenderContext();
	TestLog&					log						(m_testCtx.getLog());
	glu::CallLogWrapper			glLog					(renderCtx.getFunctions(), log);
	const deUint32				internalFormatGL		= glu::getInternalFormat(m_format);
	const deUint32				textureTargetGL			= getGLTextureTarget(m_imageType);
	const IVec3&				imageSize				= defaultImageSize(m_imageType);
	const int					numSlicesOrFaces		= m_imageType == TEXTURETYPE_CUBE ? 6 : imageSize.z();
	const bool					isUintFormat			= isFormatTypeUnsignedInteger(m_format.type);
	const bool					isIntFormat				= isFormatTypeSignedInteger(m_format.type);
	const glu::Buffer			endResultTextureBuf		(renderCtx);
	const glu::Buffer			returnValueTextureBuf	(renderCtx);
	const glu::Texture			endResultTexture		(renderCtx); //!< Texture for the final result; i.e. the texture on which the atomic operations are done. Size imageSize.
	const glu::Texture			returnValueTexture		(renderCtx); //!< Texture into which the return values are stored if m_caseType == CASETYPE_RETURN_VALUES.
																	 //	  Size imageSize*IVec3(N, 1, 1) or, for cube maps, imageSize*IVec3(N, N, 1) where N is NUM_INVOCATIONS_PER_PIXEL.

	glLog.enableLogging(true);

	// Setup textures.

	log << TestLog::Message << "// Created a texture (name " << *endResultTexture << ") to act as the target of atomic operations" << TestLog::EndMessage;
	if (m_imageType == TEXTURETYPE_BUFFER)
		log << TestLog::Message << "// Created a buffer for the texture (name " << *endResultTextureBuf << ")" << TestLog::EndMessage;

	if (m_caseType == ATOMIC_OPERATION_CASE_TYPE_RETURN_VALUES)
	{
		log << TestLog::Message << "// Created a texture (name " << *returnValueTexture << ") to which the intermediate return values of the atomic operation are stored" << TestLog::EndMessage;
		if (m_imageType == TEXTURETYPE_BUFFER)
			log << TestLog::Message << "// Created a buffer for the texture (name " << *returnValueTextureBuf << ")" << TestLog::EndMessage;
	}

	// Fill endResultTexture with initial pattern.

	{
		const LayeredImage imageData(m_imageType, m_format, imageSize.x(), imageSize.y(), imageSize.z());

		{
			const IVec4 initial(getOperationInitialValue(m_operation));

			for (int z = 0; z < numSlicesOrFaces; z++)
			for (int y = 0; y < imageSize.y(); y++)
			for (int x = 0; x < imageSize.x(); x++)
				imageData.setPixel(x, y, z, initial);
		}

		// Upload initial pattern to endResultTexture and bind to image.

		glLog.glActiveTexture(GL_TEXTURE0);
		glLog.glBindTexture(textureTargetGL, *endResultTexture);
		setTexParameteri(glLog, textureTargetGL);

		log << TestLog::Message << "// Filling end-result texture with initial pattern (initial value " << getOperationInitialValue(m_operation) << ")" << TestLog::EndMessage;

		uploadTexture(glLog, imageData, *endResultTextureBuf);
	}

	glLog.glBindImageTexture(0, *endResultTexture, 0, GL_TRUE, 0, GL_READ_WRITE, internalFormatGL);
	GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glBindImageTexture");

	if (m_caseType == ATOMIC_OPERATION_CASE_TYPE_RETURN_VALUES)
	{
		// Set storage for returnValueTexture and bind to image.

		glLog.glActiveTexture(GL_TEXTURE1);
		glLog.glBindTexture(textureTargetGL, *returnValueTexture);
		setTexParameteri(glLog, textureTargetGL);

		log << TestLog::Message << "// Setting storage of return-value texture" << TestLog::EndMessage;
		setTextureStorage(glLog, m_imageType, internalFormatGL, imageSize * (m_imageType == TEXTURETYPE_CUBE ? IVec3(NUM_INVOCATIONS_PER_PIXEL, NUM_INVOCATIONS_PER_PIXEL,	1)
																											 : IVec3(NUM_INVOCATIONS_PER_PIXEL, 1,							1)),
						  *returnValueTextureBuf);

		glLog.glBindImageTexture(1, *returnValueTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, internalFormatGL);
		GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glBindImageTexture");
	}

	// Perform image stores in compute shader and finalize reference computation.

	{
		// Generate compute shader.

		const string colorVecTypeName		= string(isUintFormat ? "u" : isIntFormat ? "i" : "") + "vec4";
		const string atomicCoord			= m_imageType == TEXTURETYPE_BUFFER		? "gx % " + toString(imageSize.x())
											: m_imageType == TEXTURETYPE_2D			? "ivec2(gx % " + toString(imageSize.x()) + ", gy)"
											: "ivec3(gx % " + toString(imageSize.x()) + ", gy, gz)";
		const string invocationCoord		= m_imageType == TEXTURETYPE_BUFFER		? "gx"
											: m_imageType == TEXTURETYPE_2D			? "ivec2(gx, gy)"
											: "ivec3(gx, gy, gz)";
		const string atomicArgExpr			= (isUintFormat		? "uint"
											 : isIntFormat		? ""
											 : "float")
												+ getAtomicFuncArgumentShaderStr(m_operation, "gx", "gy", "gz", IVec2(NUM_INVOCATIONS_PER_PIXEL*imageSize.x(), imageSize.y()));
		const string atomicInvocation		= string() + getAtomicOperationShaderFuncName(m_operation) + "(u_results, " + atomicCoord + ", " + atomicArgExpr + ")";
		const string shaderImageFormatStr	= getShaderImageFormatQualifier(m_format);
		const string shaderImageTypeStr		= getShaderImageType(m_format.type, m_imageType);
		const std::string		glslVersionDeclaration	= glu::getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(renderCtx.getType()));

		const glu::ShaderProgram program(renderCtx,
			glu::ProgramSources() << glu::ComputeSource(glslVersionDeclaration + "\n"
														+ imageAtomicExtensionShaderRequires(renderCtx)
														+ textureTypeExtensionShaderRequires(m_imageType, renderCtx) +
														"\n"
														"precision highp " + shaderImageTypeStr + ";\n"
														"\n"
														"layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
														"layout (" + shaderImageFormatStr + ", binding=0) coherent uniform " + shaderImageTypeStr + " u_results;\n"
														+ (m_caseType == ATOMIC_OPERATION_CASE_TYPE_RETURN_VALUES ?
															  "layout (" + shaderImageFormatStr + ", binding=1) writeonly uniform " + shaderImageTypeStr + " u_returnValues;\n"
															: "") +
														"\n"
														"void main (void)\n"
														"{\n"
														"	int gx = int(gl_GlobalInvocationID.x);\n"
														"	int gy = int(gl_GlobalInvocationID.y);\n"
														"	int gz = int(gl_GlobalInvocationID.z);\n"
														+ (m_caseType == ATOMIC_OPERATION_CASE_TYPE_RETURN_VALUES ?
															"	imageStore(u_returnValues, " + invocationCoord + ", " + colorVecTypeName + "(" + atomicInvocation + "));\n"
														 : m_caseType == ATOMIC_OPERATION_CASE_TYPE_END_RESULT ?
															"	" + atomicInvocation + ";\n"
														 : DE_NULL) +
														"}\n"));

		UniformAccessLogger uniforms(renderCtx.getFunctions(), log, program.getProgram());

		log << program;

		if (!program.isOk())
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Program compilation failed");
			return STOP;
		}

		// Setup and dispatch.

		glLog.glUseProgram(program.getProgram());

		glLog.glDispatchCompute(NUM_INVOCATIONS_PER_PIXEL*imageSize.x(), imageSize.y(), numSlicesOrFaces);
		GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glDispatchCompute");
	}

	// Read texture and check.

	{
		const deUint32								textureToCheckGL	= m_caseType == ATOMIC_OPERATION_CASE_TYPE_END_RESULT		? *endResultTexture
																		: m_caseType == ATOMIC_OPERATION_CASE_TYPE_RETURN_VALUES	? *returnValueTexture
																		: (deUint32)-1;
		const deUint32								textureToCheckBufGL	= m_caseType == ATOMIC_OPERATION_CASE_TYPE_END_RESULT		? *endResultTextureBuf
																		: m_caseType == ATOMIC_OPERATION_CASE_TYPE_RETURN_VALUES	? *returnValueTextureBuf
																		: (deUint32)-1;

		const IVec3									textureToCheckSize	= imageSize * IVec3(m_caseType == ATOMIC_OPERATION_CASE_TYPE_END_RESULT ? 1 : NUM_INVOCATIONS_PER_PIXEL, 1, 1);
		const UniquePtr<const ImageLayerVerifier>	verifier			(m_caseType == ATOMIC_OPERATION_CASE_TYPE_END_RESULT		? new EndResultVerifier(m_operation, m_imageType)
																	   : m_caseType == ATOMIC_OPERATION_CASE_TYPE_RETURN_VALUES		? new ReturnValueVerifier(m_operation, m_imageType, imageSize.swizzle(0, 1))
																	   : (ImageLayerVerifier*)DE_NULL);

		if (readTextureAndVerify(renderCtx, glLog, textureToCheckGL, textureToCheckBufGL, m_imageType, m_format, textureToCheckSize, *verifier))
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		else
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");

		return STOP;
	}
}

/*--------------------------------------------------------------------*//*!
 * \brief Atomic compSwap operation case.
 *
 * Similar in principle to BinaryAtomicOperationCase, but separated for
 * convenience, since the atomic function is somewhat different. Like
 * BinaryAtomicOperationCase, this has separate cases for checking end
 * result and return values.
 *//*--------------------------------------------------------------------*/
class AtomicCompSwapCase : public TestCase
{
public:
									AtomicCompSwapCase		(Context& context, const char* name, const char* description, const TextureFormat& format, TextureType imageType, AtomicOperationCaseType caseType)
		: TestCase		(context, name, description)
		, m_format		(format)
		, m_imageType	(imageType)
		, m_caseType	(caseType)
	{
		DE_ASSERT(m_format == TextureFormat(TextureFormat::R, TextureFormat::UNSIGNED_INT32)	||
				  m_format == TextureFormat(TextureFormat::R, TextureFormat::SIGNED_INT32));
	}

	void							init					(void);
	IterateResult					iterate					(void);

private:
	class EndResultVerifier;
	class ReturnValueVerifier;

	static int						getCompareArg			(const IVec3& invocationID, int imageWidth);
	static int						getAssignArg			(const IVec3& invocationID, int imageWidth);
	static string					getCompareArgShaderStr	(const string& x, const string& y, const string& z, int imageWidth);
	static string					getAssignArgShaderStr	(const string& x, const string& y, const string& z, int imageWidth);

	static const int				NUM_INVOCATIONS_PER_PIXEL = 5;

	const TextureFormat				m_format;
	const TextureType				m_imageType;
	const AtomicOperationCaseType	m_caseType;
};

int AtomicCompSwapCase::getCompareArg (const IVec3& invocationID, int imageWidth)
{
	const int x							= invocationID.x();
	const int y							= invocationID.y();
	const int z							= invocationID.z();
	const int wrapX						= x % imageWidth;
	const int curPixelInvocationNdx		= x / imageWidth;

	return wrapX*wrapX + y*y + z*z + curPixelInvocationNdx*42;
}

int AtomicCompSwapCase::getAssignArg (const IVec3& invocationID, int imageWidth)
{
	return getCompareArg(IVec3(invocationID.x() + imageWidth, invocationID.y(), invocationID.z()), imageWidth);
}

string AtomicCompSwapCase::getCompareArgShaderStr (const string& x, const string& y, const string& z, int imageWidth)
{
	const string wrapX					= "(" + x + "%" + toString(imageWidth) + ")";
	const string curPixelInvocationNdx	= "(" + x + "/" + toString(imageWidth) + ")";

	return "(" +wrapX+"*"+wrapX+ " + " +y+"*"+y+ " + " +z+"*"+z+ " + " + curPixelInvocationNdx + "*42)";
}

string AtomicCompSwapCase::getAssignArgShaderStr (const string& x, const string& y, const string& z, int imageWidth)
{
	const string wrapX					= "(" + x + "%" + toString(imageWidth) + ")";
	const string curPixelInvocationNdx	= "(" + x + "/" + toString(imageWidth) + " + 1)";

	return "(" +wrapX+"*"+wrapX+ " + " +y+"*"+y+ " + " +z+"*"+z+ " + " + curPixelInvocationNdx + "*42)";
}

void AtomicCompSwapCase::init (void)
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_OES_shader_image_atomic") && !glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		throw tcu::NotSupportedError("Test requires OES_shader_image_atomic extension");

	checkTextureTypeExtensions(m_context.getContextInfo(), m_imageType, m_context.getRenderContext());
}

class AtomicCompSwapCase::EndResultVerifier : public ImageLayerVerifier
{
public:
	EndResultVerifier (TextureType imageType, int imageWidth) : m_imageType(imageType), m_imageWidth(imageWidth) {}

	bool operator() (TestLog& log, const ConstPixelBufferAccess& resultSlice, int sliceOrFaceNdx) const
	{
		DE_ASSERT(isFormatTypeInteger(resultSlice.getFormat().type));
		DE_ASSERT(resultSlice.getWidth() == m_imageWidth);

		log << TestLog::Image("EndResults" + toString(sliceOrFaceNdx),
							  "Result Values, " + (m_imageType == TEXTURETYPE_CUBE ? "face " + string(glu::getCubeMapFaceName(cubeFaceToGLFace(glslImageFuncZToCubeFace(sliceOrFaceNdx))))
																				   : "slice " + toString(sliceOrFaceNdx)),
							  resultSlice);

		for (int y = 0; y < resultSlice.getHeight(); y++)
		for (int x = 0; x < resultSlice.getWidth(); x++)
		{
			// Compute the value-to-assign arguments that were given to the atomic function in the invocations that contribute to this pixel.
			// One of those should be the result.

			const int	result = resultSlice.getPixelInt(x, y).x();
			IVec3		invocationGlobalIDs[NUM_INVOCATIONS_PER_PIXEL];
			int			assignArgs[NUM_INVOCATIONS_PER_PIXEL];

			for (int i = 0; i < NUM_INVOCATIONS_PER_PIXEL; i++)
			{
				const IVec3 gid(x + i*resultSlice.getWidth(), y, sliceOrFaceNdx);

				invocationGlobalIDs[i]	= gid;
				assignArgs[i]			= getAssignArg(gid, m_imageWidth);
			}

			{
				bool matchFound = false;
				for (int i = 0; i < NUM_INVOCATIONS_PER_PIXEL && !matchFound; i++)
					matchFound = result == assignArgs[i];

				if (!matchFound)
				{
					log << TestLog::Message << "// Failure: invalid value at pixel " << IVec2(x, y) << ": got " << result << TestLog::EndMessage
						<< TestLog::Message << "// Note: relevant shader invocation global IDs are " << arrayStr(invocationGlobalIDs) << TestLog::EndMessage
						<< TestLog::Message << "// Note: expected one of " << arrayStr(assignArgs)
											<< " (those are the values given as the 'data' argument in the invocations that contribute to this pixel)"
											<< TestLog::EndMessage;
					return false;
				}
			}
		}

		return true;
	}

private:
	const TextureType	m_imageType;
	const int			m_imageWidth;
};

class AtomicCompSwapCase::ReturnValueVerifier : public ImageLayerVerifier
{
public:
	//! \note endResultImageLayerSize is (width, height) of the image operated on by the atomic ops, and not the size of the image where the return values are stored.
	ReturnValueVerifier (TextureType imageType, int endResultImageWidth) : m_imageType(imageType), m_endResultImageWidth(endResultImageWidth) {}

	bool operator() (TestLog& log, const ConstPixelBufferAccess& resultSlice, int sliceOrFaceNdx) const
	{
		DE_ASSERT(isFormatTypeInteger(resultSlice.getFormat().type));
		DE_ASSERT(resultSlice.getWidth() == NUM_INVOCATIONS_PER_PIXEL*m_endResultImageWidth);

		log << TestLog::Image("ReturnValues" + toString(sliceOrFaceNdx),
							  "Per-Invocation Return Values, "
								   + (m_imageType == TEXTURETYPE_CUBE ? "face " + string(glu::getCubeMapFaceName(cubeFaceToGLFace(glslImageFuncZToCubeFace(sliceOrFaceNdx))))
																	  : "slice " + toString(sliceOrFaceNdx)),
							  resultSlice);

		for (int y = 0; y < resultSlice.getHeight(); y++)
		for (int x = 0; x < m_endResultImageWidth; x++)
		{
			// Get the atomic function args and return values for all the invocations that contribute to the pixel (x, y) in the current end result slice.

			int		returnValues[NUM_INVOCATIONS_PER_PIXEL];
			int		compareArgs[NUM_INVOCATIONS_PER_PIXEL];
			IVec3	invocationGlobalIDs[NUM_INVOCATIONS_PER_PIXEL];
			IVec2	pixelCoords[NUM_INVOCATIONS_PER_PIXEL];

			for (int i = 0; i < NUM_INVOCATIONS_PER_PIXEL; i++)
			{
				const IVec2 pixCoord	(x + i*m_endResultImageWidth, y);
				const IVec3 gid			(pixCoord.x(), pixCoord.y(), sliceOrFaceNdx);

				pixelCoords[i]			= pixCoord;
				invocationGlobalIDs[i]	= gid;
				returnValues[i]			= resultSlice.getPixelInt(gid.x(), y).x();
				compareArgs[i]			= getCompareArg(gid, m_endResultImageWidth);
			}

			// Verify that the return values form a valid sequence.
			// Due to the way the compare and assign arguments to the atomic calls are organized
			// among the different invocations contributing to the same pixel -- i.e. one invocation
			// compares to A and assigns B, another compares to B and assigns C, and so on, where
			// A<B<C etc -- the first value in the return value sequence must be A, and each following
			// value must be either the same as or the smallest value (among A, B, C, ...) bigger than
			// the one just before it. E.g. sequences A A A A A A A A, A B C D E F G H and
			// A A B B B C D E are all valid sequences (if there were 8 invocations contributing
			// to each pixel).

			{
				int failingNdx = -1;

				{
					int currentAtomicValueNdx = 0;
					for (int i = 0; i < NUM_INVOCATIONS_PER_PIXEL; i++)
					{
						if (returnValues[i] == compareArgs[currentAtomicValueNdx])
							continue;
						if (i > 0 && returnValues[i] == compareArgs[currentAtomicValueNdx+1])
						{
							currentAtomicValueNdx++;
							continue;
						}
						failingNdx = i;
						break;
					}
				}

				if (failingNdx >= 0)
				{
					log << TestLog::Message << "// Failure: intermediate return values at pixels " << arrayStr(pixelCoords) << " of current layer are "
											<< arrayStr(returnValues) << TestLog::EndMessage
						<< TestLog::Message << "// Note: relevant shader invocation global IDs are " << arrayStr(invocationGlobalIDs) << TestLog::EndMessage
						<< TestLog::Message << "// Note: 'compare' argument values for the IDs are " << arrayStr(compareArgs) << TestLog::EndMessage
						<< TestLog::Message << "// Note: expected the return value sequence to fulfill the following conditions:\n"
											<< "// - first value is " << compareArgs[0] << "\n"
											<< "// - each value other than the first is either the same as the one just before it, or the smallest value (in the sequence "
											<< arrayStr(compareArgs) << ") bigger than the one just before it" << TestLog::EndMessage;
					if (failingNdx == 0)
						log << TestLog::Message << "// Note: the first return value (" << returnValues[0] << ") isn't " << compareArgs[0] << TestLog::EndMessage;
					else
						log << TestLog::Message << "// Note: the return value at index " << failingNdx << " (value " << returnValues[failingNdx] << ") "
												<< "is neither " << returnValues[failingNdx-1] << " (the one just before it) "
												<< "nor " << compareArgs[arrayIndexOf(compareArgs, returnValues[failingNdx-1])+1] << " (the smallest value bigger than the one just before it)"
												<< TestLog::EndMessage;

					return false;
				}
			}
		}

		return true;
	}

private:
	const TextureType	m_imageType;
	const int			m_endResultImageWidth;
};

AtomicCompSwapCase::IterateResult AtomicCompSwapCase::iterate (void)
{
	const RenderContext&		renderCtx				= m_context.getRenderContext();
	TestLog&					log						(m_testCtx.getLog());
	glu::CallLogWrapper			glLog					(renderCtx.getFunctions(), log);
	const deUint32				internalFormatGL		= glu::getInternalFormat(m_format);
	const deUint32				textureTargetGL			= getGLTextureTarget(m_imageType);
	const IVec3&				imageSize				= defaultImageSize(m_imageType);
	const int					numSlicesOrFaces		= m_imageType == TEXTURETYPE_CUBE ? 6 : imageSize.z();
	const bool					isUintFormat			= isFormatTypeUnsignedInteger(m_format.type);
	const bool					isIntFormat				= isFormatTypeSignedInteger(m_format.type);
	const glu::Buffer			endResultTextureBuf		(renderCtx);
	const glu::Buffer			returnValueTextureBuf	(renderCtx);
	const glu::Texture			endResultTexture		(renderCtx); //!< Texture for the final result; i.e. the texture on which the atomic operations are done. Size imageSize.
	const glu::Texture			returnValueTexture		(renderCtx); //!< Texture into which the return values are stored if m_caseType == CASETYPE_RETURN_VALUES.
																	 //	  Size imageSize*IVec3(N, 1, 1) or, for cube maps, imageSize*IVec3(N, N, 1) where N is NUM_INVOCATIONS_PER_PIXEL.

	DE_ASSERT(isUintFormat || isIntFormat);

	glLog.enableLogging(true);

	// Setup textures.

	log << TestLog::Message << "// Created a texture (name " << *endResultTexture << ") to act as the target of atomic operations" << TestLog::EndMessage;
	if (m_imageType == TEXTURETYPE_BUFFER)
		log << TestLog::Message << "// Created a buffer for the texture (name " << *endResultTextureBuf << ")" << TestLog::EndMessage;

	if (m_caseType == ATOMIC_OPERATION_CASE_TYPE_RETURN_VALUES)
	{
		log << TestLog::Message << "// Created a texture (name " << *returnValueTexture << ") to which the intermediate return values of the atomic operation are stored" << TestLog::EndMessage;
		if (m_imageType == TEXTURETYPE_BUFFER)
			log << TestLog::Message << "// Created a buffer for the texture (name " << *returnValueTextureBuf << ")" << TestLog::EndMessage;
	}

	// Fill endResultTexture with initial pattern.

	{
		const LayeredImage imageData(m_imageType, m_format, imageSize.x(), imageSize.y(), imageSize.z());

		{
			for (int z = 0; z < numSlicesOrFaces; z++)
			for (int y = 0; y < imageSize.y(); y++)
			for (int x = 0; x < imageSize.x(); x++)
				imageData.setPixel(x, y, z, IVec4(getCompareArg(IVec3(x, y, z), imageSize.x())));
		}

		// Upload initial pattern to endResultTexture and bind to image.

		glLog.glActiveTexture(GL_TEXTURE0);
		glLog.glBindTexture(textureTargetGL, *endResultTexture);
		setTexParameteri(glLog, textureTargetGL);

		log << TestLog::Message << "// Filling end-result texture with initial pattern" << TestLog::EndMessage;

		uploadTexture(glLog, imageData, *endResultTextureBuf);
	}

	glLog.glBindImageTexture(0, *endResultTexture, 0, GL_TRUE, 0, GL_READ_WRITE, internalFormatGL);
	GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glBindImageTexture");

	if (m_caseType == ATOMIC_OPERATION_CASE_TYPE_RETURN_VALUES)
	{
		// Set storage for returnValueTexture and bind to image.

		glLog.glActiveTexture(GL_TEXTURE1);
		glLog.glBindTexture(textureTargetGL, *returnValueTexture);
		setTexParameteri(glLog, textureTargetGL);

		log << TestLog::Message << "// Setting storage of return-value texture" << TestLog::EndMessage;
		setTextureStorage(glLog, m_imageType, internalFormatGL, imageSize * (m_imageType == TEXTURETYPE_CUBE ? IVec3(NUM_INVOCATIONS_PER_PIXEL, NUM_INVOCATIONS_PER_PIXEL,	1)
																											 : IVec3(NUM_INVOCATIONS_PER_PIXEL, 1,							1)),
						  *returnValueTextureBuf);

		glLog.glBindImageTexture(1, *returnValueTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, internalFormatGL);
		GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glBindImageTexture");
	}

	// Perform atomics in compute shader.

	{
		// Generate compute shader.

		const string colorScalarTypeName	= isUintFormat ? "uint" : isIntFormat ? "int" : DE_NULL;
		const string colorVecTypeName		= string(isUintFormat ? "u" : isIntFormat ? "i" : DE_NULL) + "vec4";
		const string atomicCoord			= m_imageType == TEXTURETYPE_BUFFER		? "gx % " + toString(imageSize.x())
											: m_imageType == TEXTURETYPE_2D			? "ivec2(gx % " + toString(imageSize.x()) + ", gy)"
											: "ivec3(gx % " + toString(imageSize.x()) + ", gy, gz)";
		const string invocationCoord		= m_imageType == TEXTURETYPE_BUFFER		? "gx"
											: m_imageType == TEXTURETYPE_2D			? "ivec2(gx, gy)"
											: "ivec3(gx, gy, gz)";
		const string shaderImageFormatStr	= getShaderImageFormatQualifier(m_format);
		const string shaderImageTypeStr		= getShaderImageType(m_format.type, m_imageType);
		const string glslVersionDeclaration	= glu::getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(renderCtx.getType()));

		const glu::ShaderProgram program(renderCtx,
			glu::ProgramSources() << glu::ComputeSource(glslVersionDeclaration + "\n"
														+ imageAtomicExtensionShaderRequires(renderCtx)
														+ textureTypeExtensionShaderRequires(m_imageType, renderCtx) +
														"\n"
														"precision highp " + shaderImageTypeStr + ";\n"
														"\n"
														"layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
														"layout (" + shaderImageFormatStr + ", binding=0) coherent uniform " + shaderImageTypeStr + " u_results;\n"
														+ (m_caseType == ATOMIC_OPERATION_CASE_TYPE_RETURN_VALUES ?
															  "layout (" + shaderImageFormatStr + ", binding=1) writeonly uniform " + shaderImageTypeStr + " u_returnValues;\n"
															: "") +
														"\n"
														"void main (void)\n"
														"{\n"
														"	int gx = int(gl_GlobalInvocationID.x);\n"
														"	int gy = int(gl_GlobalInvocationID.y);\n"
														"	int gz = int(gl_GlobalInvocationID.z);\n"
														"	" + colorScalarTypeName + " compare = " + colorScalarTypeName + getCompareArgShaderStr("gx", "gy", "gz", imageSize.x()) + ";\n"
														"	" + colorScalarTypeName + " data    = " + colorScalarTypeName + getAssignArgShaderStr("gx", "gy", "gz", imageSize.x()) + ";\n"
														"	" + colorScalarTypeName + " status  = " + colorScalarTypeName + "(-1);\n"
														"	status = imageAtomicCompSwap(u_results, " + atomicCoord + ", compare, data);\n"
														+ (m_caseType == ATOMIC_OPERATION_CASE_TYPE_RETURN_VALUES ?
															"	imageStore(u_returnValues, " + invocationCoord + ", " + colorVecTypeName + "(status));\n" :
															"") +
														"}\n"));

		UniformAccessLogger uniforms(renderCtx.getFunctions(), log, program.getProgram());

		log << program;

		if (!program.isOk())
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Program compilation failed");
			return STOP;
		}

		// Setup and dispatch.

		glLog.glUseProgram(program.getProgram());

		glLog.glDispatchCompute(NUM_INVOCATIONS_PER_PIXEL*imageSize.x(), imageSize.y(), numSlicesOrFaces);
		GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glDispatchCompute");
	}

	// Create reference, read texture and compare.

	{
		const deUint32								textureToCheckGL	= m_caseType == ATOMIC_OPERATION_CASE_TYPE_END_RESULT		? *endResultTexture
																		: m_caseType == ATOMIC_OPERATION_CASE_TYPE_RETURN_VALUES	? *returnValueTexture
																		: (deUint32)-1;

		const deUint32								textureToCheckBufGL	= m_caseType == ATOMIC_OPERATION_CASE_TYPE_END_RESULT		? *endResultTextureBuf
																		: m_caseType == ATOMIC_OPERATION_CASE_TYPE_RETURN_VALUES	? *returnValueTextureBuf
																		: (deUint32)-1;

		// The relevant region of the texture being checked (potentially
		// different from actual texture size for cube maps, because cube maps
		// may have unused pixels due to square size restriction).
		const IVec3									relevantRegion		= imageSize * (m_caseType == ATOMIC_OPERATION_CASE_TYPE_END_RESULT	? IVec3(1,							1,							1)
																					 :														  IVec3(NUM_INVOCATIONS_PER_PIXEL,	1,							1));

		const UniquePtr<const ImageLayerVerifier>	verifier			(m_caseType == ATOMIC_OPERATION_CASE_TYPE_END_RESULT		? new EndResultVerifier(m_imageType, imageSize.x())
																	   : m_caseType == ATOMIC_OPERATION_CASE_TYPE_RETURN_VALUES		? new ReturnValueVerifier(m_imageType, imageSize.x())
																	   : (ImageLayerVerifier*)DE_NULL);

		if (readTextureAndVerify(renderCtx, glLog, textureToCheckGL, textureToCheckBufGL, m_imageType, m_format, relevantRegion, *verifier))
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		else
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");

		return STOP;
	}
}

//! Case testing the "coherent" or "volatile" qualifier, along with memoryBarrier() and barrier().
class CoherenceCase : public TestCase
{
public:
	enum Qualifier
	{
		QUALIFIER_COHERENT = 0,
		QUALIFIER_VOLATILE,

		QUALIFIER_LAST
	};

	CoherenceCase (Context& context, const char* name, const char* description, const TextureFormat& format, TextureType imageType, Qualifier qualifier)
		: TestCase		(context, name, description)
		, m_format		(format)
		, m_imageType	(imageType)
		, m_qualifier	(qualifier)
	{
		DE_STATIC_ASSERT(DE_LENGTH_OF_ARRAY(CoherenceCase::SHADER_READ_OFFSETS_Y) == DE_LENGTH_OF_ARRAY(CoherenceCase::SHADER_READ_OFFSETS_X) &&
						 DE_LENGTH_OF_ARRAY(CoherenceCase::SHADER_READ_OFFSETS_Z) == DE_LENGTH_OF_ARRAY(CoherenceCase::SHADER_READ_OFFSETS_X));

		DE_ASSERT(qualifier == QUALIFIER_COHERENT || qualifier == QUALIFIER_VOLATILE);

		DE_ASSERT(m_format == TextureFormat(TextureFormat::R, TextureFormat::UNSIGNED_INT32)	||
				  m_format == TextureFormat(TextureFormat::R, TextureFormat::SIGNED_INT32)		||
				  m_format == TextureFormat(TextureFormat::R, TextureFormat::FLOAT));
	}

	void			init		(void) { checkTextureTypeExtensions(m_context.getContextInfo(), m_imageType, m_context.getRenderContext()); }
	IterateResult	iterate		(void);

private:
	static const int			SHADER_READ_OFFSETS_X[4];
	static const int			SHADER_READ_OFFSETS_Y[4];
	static const int			SHADER_READ_OFFSETS_Z[4];
	static const char* const	SHADER_READ_OFFSETS_X_STR;
	static const char* const	SHADER_READ_OFFSETS_Y_STR;
	static const char* const	SHADER_READ_OFFSETS_Z_STR;

	const TextureFormat		m_format;
	const TextureType		m_imageType;
	const Qualifier			m_qualifier;
};

const int			CoherenceCase::SHADER_READ_OFFSETS_X[4]		=		{ 1, 4, 7, 10 };
const int			CoherenceCase::SHADER_READ_OFFSETS_Y[4]		=		{ 2, 5, 8, 11 };
const int			CoherenceCase::SHADER_READ_OFFSETS_Z[4]		=		{ 3, 6, 9, 12 };
const char* const	CoherenceCase::SHADER_READ_OFFSETS_X_STR	= "int[]( 1, 4, 7, 10 )";
const char* const	CoherenceCase::SHADER_READ_OFFSETS_Y_STR	= "int[]( 2, 5, 8, 11 )";
const char* const	CoherenceCase::SHADER_READ_OFFSETS_Z_STR	= "int[]( 3, 6, 9, 12 )";

CoherenceCase::IterateResult CoherenceCase::iterate (void)
{
	const RenderContext&		renderCtx					= m_context.getRenderContext();
	TestLog&					log							(m_testCtx.getLog());
	glu::CallLogWrapper			glLog						(renderCtx.getFunctions(), log);
	const deUint32				internalFormatGL			= glu::getInternalFormat(m_format);
	const deUint32				textureTargetGL				= getGLTextureTarget(m_imageType);
	const IVec3&				imageSize					= defaultImageSize(m_imageType);
	const int					numSlicesOrFaces			= m_imageType == TEXTURETYPE_CUBE ? 6 : imageSize.z();
	const bool					isUintFormat				= isFormatTypeUnsignedInteger(m_format.type);
	const bool					isIntFormat					= isFormatTypeSignedInteger(m_format.type);
	const char* const			qualifierName				= m_qualifier == QUALIFIER_COHERENT ? "coherent"
															: m_qualifier == QUALIFIER_VOLATILE ? "volatile"
															: DE_NULL;
	const glu::Buffer			textureBuf					(renderCtx);
	const glu::Texture			texture						(renderCtx);
	const IVec3					numGroups					= IVec3(16, de::min(16, imageSize.y()), de::min(2, numSlicesOrFaces));
	const IVec3					workItemSize				= IVec3(imageSize.x(), imageSize.y(), numSlicesOrFaces);
	const IVec3					localSize					= workItemSize / numGroups;
	const IVec3					minReqMaxLocalSize			= IVec3(128, 128, 64);
	const int					minReqMaxLocalInvocations	= 128;

	DE_ASSERT(workItemSize == localSize*numGroups);
	DE_ASSERT(tcu::boolAll(tcu::lessThanEqual(localSize, minReqMaxLocalSize)));
	DE_ASSERT(localSize.x()*localSize.y()*localSize.z() <= minReqMaxLocalInvocations);
	DE_UNREF(minReqMaxLocalSize);
	DE_UNREF(minReqMaxLocalInvocations);

	glLog.enableLogging(true);

	// Setup texture.

	log << TestLog::Message << "// Created a texture (name " << *texture << ")" << TestLog::EndMessage;
	if (m_imageType == TEXTURETYPE_BUFFER)
		log << TestLog::Message << "// Created a buffer for the texture (name " << *textureBuf << ")" << TestLog::EndMessage;

	glLog.glActiveTexture(GL_TEXTURE0);
	glLog.glBindTexture(textureTargetGL, *texture);
	setTexParameteri(glLog, textureTargetGL);
	setTextureStorage(glLog, m_imageType, internalFormatGL, imageSize, *textureBuf);
	glLog.glBindImageTexture(0, *texture, 0, GL_TRUE, 0, GL_READ_WRITE, internalFormatGL);
	GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glBindImageTexture");

	// Perform computations in compute shader.

	{
		// Generate compute shader.

		const string		colorVecTypeName		= string(isUintFormat ? "u" : isIntFormat ? "i" : "") + "vec4";
		const char* const	colorScalarTypeName		= isUintFormat ? "uint" : isIntFormat ? "int" : "float";
		const string		invocationCoord			= m_imageType == TEXTURETYPE_BUFFER		? "gx"
													: m_imageType == TEXTURETYPE_2D			? "ivec2(gx, gy)"
													: "ivec3(gx, gy, gz)";
		const string		shaderImageFormatStr	= getShaderImageFormatQualifier(m_format);
		const string		shaderImageTypeStr		= getShaderImageType(m_format.type, m_imageType);
		const string		localSizeX				= de::toString(localSize.x());
		const string		localSizeY				= de::toString(localSize.y());
		const string		localSizeZ				= de::toString(localSize.z());
		const std::string	glslVersionDeclaration	= glu::getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(renderCtx.getType()));


		const glu::ShaderProgram program(renderCtx,
			glu::ProgramSources() << glu::ComputeSource(glslVersionDeclaration + "\n"
														+ textureTypeExtensionShaderRequires(m_imageType, renderCtx) +
														"\n"
														"precision highp " + shaderImageTypeStr + ";\n"
														"\n"
														"layout (local_size_x = " + localSizeX
															+ ", local_size_y = " + localSizeY
															+ ", local_size_z = " + localSizeZ
															+ ") in;\n"
														"layout (" + shaderImageFormatStr + ", binding=0) " + qualifierName + " uniform " + shaderImageTypeStr + " u_image;\n"
														"void main (void)\n"
														"{\n"
														"	int gx = int(gl_GlobalInvocationID.x);\n"
														"	int gy = int(gl_GlobalInvocationID.y);\n"
														"	int gz = int(gl_GlobalInvocationID.z);\n"
														"	imageStore(u_image, " + invocationCoord + ", " + colorVecTypeName + "(gx^gy^gz));\n"
														"\n"
														"	memoryBarrier();\n"
														"	barrier();\n"
														"\n"
														"	" + colorScalarTypeName + " sum = " + colorScalarTypeName + "(0);\n"
														"	int groupBaseX = gx/" + localSizeX + "*" + localSizeX + ";\n"
														"	int groupBaseY = gy/" + localSizeY + "*" + localSizeY + ";\n"
														"	int groupBaseZ = gz/" + localSizeZ + "*" + localSizeZ + ";\n"
														"	int xOffsets[] = " + SHADER_READ_OFFSETS_X_STR + ";\n"
														"	int yOffsets[] = " + SHADER_READ_OFFSETS_Y_STR + ";\n"
														"	int zOffsets[] = " + SHADER_READ_OFFSETS_Z_STR + ";\n"
														"	for (int i = 0; i < " + toString(DE_LENGTH_OF_ARRAY(SHADER_READ_OFFSETS_X)) + "; i++)\n"
														"	{\n"
														"		int readX = groupBaseX + (gx + xOffsets[i]) % " + localSizeX + ";\n"
														"		int readY = groupBaseY + (gy + yOffsets[i]) % " + localSizeY + ";\n"
														"		int readZ = groupBaseZ + (gz + zOffsets[i]) % " + localSizeZ + ";\n"
														"		sum += imageLoad(u_image, " + (m_imageType == TEXTURETYPE_BUFFER	? "readX"
																							 : m_imageType == TEXTURETYPE_2D		? "ivec2(readX, readY)"
																							 : "ivec3(readX, readY, readZ)") + ").x;\n"
														"	}\n"
														"\n"
														"	memoryBarrier();\n"
														"	barrier();\n"
														"\n"
														"	imageStore(u_image, " + invocationCoord + ", " + colorVecTypeName + "(sum));\n"
														"}\n"));

		UniformAccessLogger uniforms(renderCtx.getFunctions(), log, program.getProgram());

		log << program;

		if (!program.isOk())
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Program compilation failed");
			return STOP;
		}

		// Setup and dispatch.

		glLog.glUseProgram(program.getProgram());

		glLog.glDispatchCompute(numGroups.x(), numGroups.y(), numGroups.z());
		GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glDispatchCompute");
	}

	// Create reference, read texture and compare.

	{
		LayeredImage reference(m_imageType, m_format, imageSize.x(), imageSize.y(), imageSize.z());

		{
			LayeredImage base(m_imageType, m_format, imageSize.x(), imageSize.y(), imageSize.z());
			for (int z = 0; z < numSlicesOrFaces; z++)
			for (int y = 0; y < imageSize.y(); y++)
			for (int x = 0; x < imageSize.x(); x++)
				base.setPixel(x, y, z, IVec4(x^y^z));

			for (int z = 0; z < numSlicesOrFaces; z++)
			for (int y = 0; y < imageSize.y(); y++)
			for (int x = 0; x < imageSize.x(); x++)
			{
				const int	groupBaseX	= x / localSize.x() * localSize.x();
				const int	groupBaseY	= y / localSize.y() * localSize.y();
				const int	groupBaseZ	= z / localSize.z() * localSize.z();
				int			sum			= 0;
				for (int i = 0; i < DE_LENGTH_OF_ARRAY(SHADER_READ_OFFSETS_X); i++)
					sum += base.getPixelInt(groupBaseX + (x + SHADER_READ_OFFSETS_X[i]) % localSize.x(),
											groupBaseY + (y + SHADER_READ_OFFSETS_Y[i]) % localSize.y(),
											groupBaseZ + (z + SHADER_READ_OFFSETS_Z[i]) % localSize.z()).x();

				reference.setPixel(x, y, z, IVec4(sum));
			}
		}

		if (readTextureAndVerify(renderCtx, glLog, *texture, *textureBuf, m_imageType, m_format, imageSize, ImageLayerComparer(reference)))
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		else
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image comparison failed");

		return STOP;
	}
}

class R32UIImageSingleValueVerifier : public ImageLayerVerifier
{
public:
	R32UIImageSingleValueVerifier (const deUint32 value)					: m_min(value),	m_max(value)	{}
	R32UIImageSingleValueVerifier (const deUint32 min, const deUint32 max)	: m_min(min),	m_max(max)		{}

	bool operator() (TestLog& log, const ConstPixelBufferAccess& resultSlice, int) const
	{
		DE_ASSERT(resultSlice.getWidth() == 1 && resultSlice.getHeight() == 1 && resultSlice.getDepth() == 1);
		DE_ASSERT(resultSlice.getFormat() == TextureFormat(TextureFormat::R, TextureFormat::UNSIGNED_INT32));

		log << TestLog::Message << "// Note: expecting to get value " << (m_min == m_max ? toString(m_min) : "in range [" + toString(m_min) + ", " + toString(m_max) + "]") << TestLog::EndMessage;

		const deUint32 resultValue = resultSlice.getPixelUint(0, 0).x();
		if (!de::inRange(resultValue, m_min, m_max))
		{
			log << TestLog::Message << "// Failure: got value " << resultValue << TestLog::EndMessage;
			return false;
		}
		else
		{
			log << TestLog::Message << "// Success: got value " << resultValue << TestLog::EndMessage;
			return true;
		}
	}

private:
	const deUint32 m_min;
	const deUint32 m_max;
};

//! Tests the imageSize() GLSL function. Stores result in a 1x1 R32UI image. The image with which imageSize() is called isn't read or written, and
//  can thus be qualifier readonly, writeonly, or both.
class ImageSizeCase : public TestCase
{
public:
	enum ImageAccess
	{
		IMAGEACCESS_READ_ONLY = 0,
		IMAGEACCESS_WRITE_ONLY,
		IMAGEACCESS_READ_ONLY_WRITE_ONLY,

		IMAGEACCESS_LAST
	};

	ImageSizeCase (Context& context, const char* name, const char* description, const TextureFormat& format, TextureType imageType, const IVec3& size, ImageAccess imageAccess)
		: TestCase			(context, name, description)
		, m_format			(format)
		, m_imageType		(imageType)
		, m_imageSize		(size)
		, m_imageAccess		(imageAccess)
	{
	}

	void			init		(void) { checkTextureTypeExtensions(m_context.getContextInfo(), m_imageType, m_context.getRenderContext()); }
	IterateResult	iterate		(void);

private:
	const TextureFormat		m_format;
	const TextureType		m_imageType;
	const IVec3				m_imageSize;
	const ImageAccess		m_imageAccess;
};

ImageSizeCase::IterateResult ImageSizeCase::iterate (void)
{
	const RenderContext&		renderCtx				= m_context.getRenderContext();
	TestLog&					log						(m_testCtx.getLog());
	glu::CallLogWrapper			glLog					(renderCtx.getFunctions(), log);
	const deUint32				internalFormatGL		= glu::getInternalFormat(m_format);
	const deUint32				textureTargetGL			= getGLTextureTarget(m_imageType);
	const glu::Buffer			mainTextureBuf			(renderCtx);
	const glu::Texture			mainTexture				(renderCtx);
	const glu::Texture			shaderOutResultTexture	(renderCtx);

	glLog.enableLogging(true);

	// Setup textures.

	log << TestLog::Message << "// Created a texture (name " << *mainTexture << ")" << TestLog::EndMessage;
	if (m_imageType == TEXTURETYPE_BUFFER)
		log << TestLog::Message << "// Created a buffer for the texture (name " << *mainTextureBuf << ")" << TestLog::EndMessage;
	log << TestLog::Message << "// Created a texture (name " << *shaderOutResultTexture << ") for storing the shader output" << TestLog::EndMessage;

	glLog.glActiveTexture(GL_TEXTURE0);
	glLog.glBindTexture(textureTargetGL, *mainTexture);
	setTexParameteri(glLog, textureTargetGL);
	setTextureStorage(glLog, m_imageType, internalFormatGL, m_imageSize, *mainTextureBuf);
	glLog.glBindImageTexture(0, *mainTexture, 0, GL_TRUE, 0, GL_READ_WRITE, internalFormatGL);
	GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glBindImageTexture");

	glLog.glActiveTexture(GL_TEXTURE1);
	glLog.glBindTexture(GL_TEXTURE_2D, *shaderOutResultTexture);
	setTexParameteri(glLog, GL_TEXTURE_2D);
	setTextureStorage(glLog, TEXTURETYPE_2D, GL_R32UI, IVec3(1, 1, 1), 0 /* always 2d texture, no buffer needed */);
	glLog.glBindImageTexture(1, *shaderOutResultTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);
	GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glBindImageTexture");

	// Read texture size in compute shader.

	{
		// Generate compute shader.

		const char* const	shaderImageAccessStr	= m_imageAccess == IMAGEACCESS_READ_ONLY			? "readonly"
													: m_imageAccess == IMAGEACCESS_WRITE_ONLY			? "writeonly"
													: m_imageAccess == IMAGEACCESS_READ_ONLY_WRITE_ONLY	? "readonly writeonly"
													: DE_NULL;
		const string		shaderImageFormatStr	= getShaderImageFormatQualifier(m_format);
		const string		shaderImageTypeStr		= getShaderImageType(m_format.type, m_imageType);
		const string		glslVersionDeclaration	= glu::getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(renderCtx.getType()));

		const glu::ShaderProgram program(renderCtx,
			glu::ProgramSources() << glu::ComputeSource(glslVersionDeclaration + "\n"
														+ textureTypeExtensionShaderRequires(m_imageType, renderCtx) +
														"\n"
														"precision highp " + shaderImageTypeStr + ";\n"
														"precision highp uimage2D;\n"
														"\n"
														"layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
														"layout (" + shaderImageFormatStr + ", binding=0) " + shaderImageAccessStr + " uniform " + shaderImageTypeStr + " u_image;\n"
														"layout (r32ui, binding=1) writeonly uniform uimage2D u_result;\n"
														"void main (void)\n"
														"{\n"
														+ (m_imageType == TEXTURETYPE_BUFFER ?
															"	int result = imageSize(u_image);\n"
														 : m_imageType == TEXTURETYPE_2D || m_imageType == TEXTURETYPE_CUBE ?
															"	ivec2 size = imageSize(u_image);\n"
															"	int result = size.y*1000 + size.x;\n"
														 : m_imageType == TEXTURETYPE_3D || m_imageType == TEXTURETYPE_2D_ARRAY ?
															"	ivec3 size = imageSize(u_image);\n"
															"	int result = size.z*1000000 + size.y*1000 + size.x;\n"
														 : DE_NULL) +
														"	imageStore(u_result, ivec2(0, 0), uvec4(result));\n"
														"}\n"));

		UniformAccessLogger uniforms(renderCtx.getFunctions(), log, program.getProgram());

		log << program;

		if (!program.isOk())
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Program compilation failed");
			return STOP;
		}

		// Setup and dispatch.

		glLog.glUseProgram(program.getProgram());

		glLog.glDispatchCompute(1, 1, 1);
		GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glDispatchCompute");
	}

	// Read texture and compare to reference.

	{
		const deUint32	referenceOutput		= m_imageType == TEXTURETYPE_BUFFER										? (deUint32)(												  m_imageSize.x())
											: m_imageType == TEXTURETYPE_2D || m_imageType == TEXTURETYPE_CUBE		? (deUint32)(						   m_imageSize.y()*1000 + m_imageSize.x())
											: m_imageType == TEXTURETYPE_3D || m_imageType == TEXTURETYPE_2D_ARRAY	? (deUint32)(m_imageSize.z()*1000000 + m_imageSize.y()*1000 + m_imageSize.x())
											: (deUint32)-1;

		if (readIntegerTextureViaFBOAndVerify(renderCtx, glLog, *shaderOutResultTexture, TEXTURETYPE_2D, TextureFormat(TextureFormat::R, TextureFormat::UNSIGNED_INT32),
											  IVec3(1, 1, 1), R32UIImageSingleValueVerifier(referenceOutput)))
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		else
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got wrong value");

		return STOP;
	}
}

//! Case testing the control over early/late fragment tests.
class EarlyFragmentTestsCase : public TestCase
{
public:
	enum TestType
	{
		TESTTYPE_DEPTH = 0,
		TESTTYPE_STENCIL,

		TESTTYPE_LAST
	};

	enum RenderTargetType
	{
		RENDERTARGET_DEFAULT = 0,
		RENDERTARGET_FBO,
		RENDERTARGET_FBO_WITHOUT_TEST_ATTACHMENT,

		RENDERTARGET_LAST
	};


	EarlyFragmentTestsCase (Context& context, const char* name, const char* description, TestType type, bool useEarlyTests, RenderTargetType renderTarget)
		: TestCase			(context, name, description)
		, m_type			(type)
		, m_useEarlyTests	(useEarlyTests)
		, m_renderTarget	(renderTarget)
	{
	}

	void init (void)
	{
		if (m_context.getContextInfo().getInt(GL_MAX_FRAGMENT_IMAGE_UNIFORMS) == 0)
			throw tcu::NotSupportedError("GL_MAX_FRAGMENT_IMAGE_UNIFORMS is zero");

		if (!m_context.getContextInfo().isExtensionSupported("GL_OES_shader_image_atomic") && !glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
			throw tcu::NotSupportedError("Test requires OES_shader_image_atomic extension");

		if (m_type == TESTTYPE_DEPTH				&&
			m_renderTarget == RENDERTARGET_DEFAULT	&&
			m_context.getRenderTarget().getDepthBits() == 0)
		{
			throw tcu::NotSupportedError("Test requires depth buffer");
		}

		if (m_type == TESTTYPE_STENCIL				&&
			m_renderTarget == RENDERTARGET_DEFAULT	&&
			m_context.getRenderTarget().getStencilBits() == 0)
		{
			throw tcu::NotSupportedError("Test requires stencil buffer");
		}

		if (m_renderTarget == RENDERTARGET_DEFAULT	&&
			(m_context.getRenderTarget().getWidth() < RENDER_SIZE || m_context.getRenderTarget().getHeight() < RENDER_SIZE))
			throw tcu::NotSupportedError("Render target must have at least " + toString(RENDER_SIZE) + " width and height");
	}

	IterateResult iterate (void);

private:
	static const int		RENDER_SIZE;

	const TestType			m_type;
	const bool				m_useEarlyTests;
	const RenderTargetType	m_renderTarget;
};

const int EarlyFragmentTestsCase::RENDER_SIZE = 32;

EarlyFragmentTestsCase::IterateResult EarlyFragmentTestsCase::iterate (void)
{
	const RenderContext&			renderCtx			= m_context.getRenderContext();
	TestLog&						log					(m_testCtx.getLog());
	glu::CallLogWrapper				glLog				(renderCtx.getFunctions(), log);
	de::Random						rnd					(deStringHash(getName()));
	const bool						expectPartialResult	= m_useEarlyTests && m_renderTarget != RENDERTARGET_FBO_WITHOUT_TEST_ATTACHMENT;
	const int						viewportWidth		= RENDER_SIZE;
	const int						viewportHeight		= RENDER_SIZE;
	const int						viewportX			= (m_renderTarget == RENDERTARGET_DEFAULT) ? (rnd.getInt(0, renderCtx.getRenderTarget().getWidth() - viewportWidth))	: (0);
	const int						viewportY			= (m_renderTarget == RENDERTARGET_DEFAULT) ? (rnd.getInt(0, renderCtx.getRenderTarget().getHeight() - viewportHeight))	: (0);
	const glu::Texture				texture				(renderCtx);
	de::MovePtr<glu::Framebuffer>	fbo;
	de::MovePtr<glu::Renderbuffer>	colorAttachment;
	de::MovePtr<glu::Renderbuffer>	testAttachment;

	glLog.enableLogging(true);

	// Setup texture.

	log << TestLog::Message << "// Created a texture (name " << *texture << ")" << TestLog::EndMessage;

	glLog.glActiveTexture(GL_TEXTURE0);
	glLog.glBindTexture(GL_TEXTURE_2D, *texture);
	setTexParameteri(glLog, GL_TEXTURE_2D);
	{
		LayeredImage src(TEXTURETYPE_2D, TextureFormat(TextureFormat::R, TextureFormat::UNSIGNED_INT32), 1, 1, 1);
		src.setPixel(0, 0, 0, IVec4(0));
		uploadTexture(glLog, src, 0 /* always 2d texture, no buffer needed */);
	}
	glLog.glBindImageTexture(0, *texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
	GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "glBindImageTexture");

	// Set up framebuffer
	if (m_renderTarget == RENDERTARGET_FBO ||
		m_renderTarget == RENDERTARGET_FBO_WITHOUT_TEST_ATTACHMENT)
	{
		fbo				= de::MovePtr<glu::Framebuffer>(new glu::Framebuffer(renderCtx));
		colorAttachment	= de::MovePtr<glu::Renderbuffer>(new glu::Renderbuffer(renderCtx));
		testAttachment	= de::MovePtr<glu::Renderbuffer>(new glu::Renderbuffer(renderCtx));

		glLog.glBindRenderbuffer(GL_RENDERBUFFER, **colorAttachment);
		glLog.glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, RENDER_SIZE, RENDER_SIZE);
		GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "gen color attachment rb");

		glLog.glBindFramebuffer(GL_FRAMEBUFFER, **fbo);
		glLog.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, **colorAttachment);
		GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "set fbo color attachment");

		if (m_renderTarget == RENDERTARGET_FBO && m_type == TESTTYPE_DEPTH)
		{
			glLog.glBindRenderbuffer(GL_RENDERBUFFER, **testAttachment);
			glLog.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, RENDER_SIZE, RENDER_SIZE);
			GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "gen depth attachment rb");

			glLog.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, **testAttachment);
			GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "set fbo depth attachment");
		}
		else if (m_renderTarget == RENDERTARGET_FBO && m_type == TESTTYPE_STENCIL)
		{
			glLog.glBindRenderbuffer(GL_RENDERBUFFER, **testAttachment);
			glLog.glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, RENDER_SIZE, RENDER_SIZE);
			GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "gen stencil attachment rb");

			glLog.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, **testAttachment);
			GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "set fbo stencil attachment");
		}

		glLog.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, **colorAttachment);
		GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "setup fbo");
		TCU_CHECK(glLog.glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	}

	// Set up appropriate conditions for the test.

	glLog.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glLog.glClear(GL_COLOR_BUFFER_BIT);

	if (m_type == TESTTYPE_DEPTH)
	{
		glLog.glClearDepthf(0.5f);
		glLog.glClear(GL_DEPTH_BUFFER_BIT);
		glLog.glEnable(GL_DEPTH_TEST);
	}
	else if (m_type == TESTTYPE_STENCIL)
	{
		glLog.glClearStencil(0);
		glLog.glClear(GL_STENCIL_BUFFER_BIT);
		glLog.glScissor(viewportX, viewportY, viewportWidth/2, viewportHeight);
		glLog.glEnable(GL_SCISSOR_TEST);
		glLog.glClearStencil(1);
		glLog.glClear(GL_STENCIL_BUFFER_BIT);
		glLog.glDisable(GL_SCISSOR_TEST);
		glLog.glStencilFunc(GL_EQUAL, 1, 1);
		glLog.glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glLog.glEnable(GL_STENCIL_TEST);
	}
	else
		DE_ASSERT(false);

	// Perform image stores in fragment shader.

	{
		const std::string glslVersionDeclaration = glu::getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(renderCtx.getType()));

		// Generate fragment shader.

		const glu::ShaderProgram program(renderCtx,
			glu::ProgramSources() << glu::VertexSource(		glslVersionDeclaration + "\n"
															"\n"
															"highp in vec3 a_position;\n"
															"\n"
															"void main (void)\n"
															"{\n"
															"	gl_Position = vec4(a_position, 1.0);\n"
															"}\n")

								  << glu::FragmentSource(	glslVersionDeclaration + "\n"
															+ imageAtomicExtensionShaderRequires(renderCtx) +
															"\n"
															+ string(m_useEarlyTests ? "layout (early_fragment_tests) in;\n\n" : "") +
															"layout (location = 0) out highp vec4 o_color;\n"
															"\n"
															"precision highp uimage2D;\n"
															"\n"
															"layout (r32ui, binding=0) coherent uniform uimage2D u_image;\n"
															"\n"
															"void main (void)\n"
															"{\n"
															"	imageAtomicAdd(u_image, ivec2(0, 0), uint(1));\n"
															"	o_color = vec4(1.0);\n"
															"}\n"));

		UniformAccessLogger uniforms(renderCtx.getFunctions(), log, program.getProgram());

		log << program;

		if (!program.isOk())
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Program compilation failed");
			return STOP;
		}

		// Setup and draw full-viewport quad.

		glLog.glUseProgram(program.getProgram());

		{
			static const float vertexPositions[4*3] =
			{
				-1.0, -1.0, -1.0f,
				 1.0, -1.0,  0.0f,
				-1.0,  1.0,  0.0f,
				 1.0,  1.0,  1.0f,
			};

			static const deUint16 indices[6] = { 0, 1, 2, 2, 1, 3 };

			const glu::VertexArrayBinding attrBindings[] =
			{
				glu::va::Float("a_position", 3, 4, 0, &vertexPositions[0])
			};

			glLog.glViewport(viewportX, viewportY, viewportWidth, viewportHeight);

			glu::draw(renderCtx, program.getProgram(), DE_LENGTH_OF_ARRAY(attrBindings), &attrBindings[0],
				glu::pr::Triangles(DE_LENGTH_OF_ARRAY(indices), &indices[0]));
			GLU_EXPECT_NO_ERROR(renderCtx.getFunctions().getError(), "Draw failed");
		}
	}

	// Log rendered result for convenience.
	{
		tcu::Surface rendered(viewportWidth, viewportHeight);
		glu::readPixels(renderCtx, viewportX, viewportY, rendered.getAccess());
		log << TestLog::Image("Rendered", "Rendered image", rendered);
	}

	// Read counter value and check.
	{
		const int numSamples		= de::max(1, renderCtx.getRenderTarget().getNumSamples());
		const int expectedCounter	= expectPartialResult ? viewportWidth*viewportHeight/2				: viewportWidth*viewportHeight;
		const int tolerance			= expectPartialResult ? de::max(viewportWidth, viewportHeight)*3	: 0;
		const int expectedMin		= de::max(0, expectedCounter - tolerance);
		const int expectedMax		= (expectedCounter + tolerance) * numSamples;

		if (readIntegerTextureViaFBOAndVerify(renderCtx, glLog, *texture, TEXTURETYPE_2D, TextureFormat(TextureFormat::R, TextureFormat::UNSIGNED_INT32),
											  IVec3(1, 1, 1), R32UIImageSingleValueVerifier(expectedMin, expectedMax)))
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		else
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got wrong value");

		return STOP;
	}
}

} // anonymous

ShaderImageLoadStoreTests::ShaderImageLoadStoreTests (Context& context)
	: TestCaseGroup(context, "image_load_store", "Shader Image Load & Store Tests")
{
}

ShaderImageLoadStoreTests::~ShaderImageLoadStoreTests (void)
{
}

void ShaderImageLoadStoreTests::init (void)
{
	// Per-image-type tests.

	{
		static const TextureType imageTypes[] =
		{
			TEXTURETYPE_2D,
			TEXTURETYPE_CUBE,
			TEXTURETYPE_3D,
			TEXTURETYPE_2D_ARRAY,
			TEXTURETYPE_BUFFER
		};

		static const TextureFormat formats[] =
		{
			TextureFormat(TextureFormat::RGBA,	TextureFormat::FLOAT),
			TextureFormat(TextureFormat::RGBA,	TextureFormat::HALF_FLOAT),
			TextureFormat(TextureFormat::R,		TextureFormat::FLOAT),

			TextureFormat(TextureFormat::RGBA,	TextureFormat::UNSIGNED_INT32),
			TextureFormat(TextureFormat::RGBA,	TextureFormat::UNSIGNED_INT16),
			TextureFormat(TextureFormat::RGBA,	TextureFormat::UNSIGNED_INT8),
			TextureFormat(TextureFormat::R,		TextureFormat::UNSIGNED_INT32),

			TextureFormat(TextureFormat::RGBA,	TextureFormat::SIGNED_INT32),
			TextureFormat(TextureFormat::RGBA,	TextureFormat::SIGNED_INT16),
			TextureFormat(TextureFormat::RGBA,	TextureFormat::SIGNED_INT8),
			TextureFormat(TextureFormat::R,		TextureFormat::SIGNED_INT32),

			TextureFormat(TextureFormat::RGBA,	TextureFormat::UNORM_INT8),

			TextureFormat(TextureFormat::RGBA,	TextureFormat::SNORM_INT8)
		};

		for (int imageTypeNdx = 0; imageTypeNdx < DE_LENGTH_OF_ARRAY(imageTypes); imageTypeNdx++)
		{
			const TextureType		imageType			= imageTypes[imageTypeNdx];
			TestCaseGroup* const	imageTypeGroup		= new TestCaseGroup(m_context, getTextureTypeName(imageType), "");
			addChild(imageTypeGroup);

			TestCaseGroup* const	storeGroup			= new TestCaseGroup(m_context, "store",					"Plain imageStore() cases");
			TestCaseGroup* const	loadStoreGroup		= new TestCaseGroup(m_context, "load_store",			"Cases with imageLoad() followed by imageStore()");
			TestCaseGroup* const	atomicGroup			= new TestCaseGroup(m_context, "atomic",				"Atomic image operation cases");
			TestCaseGroup* const	qualifierGroup		= new TestCaseGroup(m_context, "qualifiers",			"Coherent, volatile and restrict");
			TestCaseGroup* const	reinterpretGroup	= new TestCaseGroup(m_context, "format_reinterpret",	"Cases with differing texture and image formats");
			TestCaseGroup* const	imageSizeGroup		= new TestCaseGroup(m_context, "image_size",			"imageSize() cases");
			imageTypeGroup->addChild(storeGroup);
			imageTypeGroup->addChild(loadStoreGroup);
			imageTypeGroup->addChild(atomicGroup);
			imageTypeGroup->addChild(qualifierGroup);
			imageTypeGroup->addChild(reinterpretGroup);
			imageTypeGroup->addChild(imageSizeGroup);

			for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); formatNdx++)
			{
				const TextureFormat&	format		= formats[formatNdx];
				const string			formatName	= getShaderImageFormatQualifier(formats[formatNdx]);

				if (imageType == TEXTURETYPE_BUFFER && !isFormatSupportedForTextureBuffer(format))
					continue;

				// Store cases.

				storeGroup->addChild(new ImageStoreCase(m_context, formatName.c_str(), "", format, imageType));
				if (textureLayerType(imageType) != imageType)
					storeGroup->addChild(new ImageStoreCase(m_context, (formatName + "_single_layer").c_str(), "", format, imageType, ImageStoreCase::CASEFLAG_SINGLE_LAYER_BIND));

				// Load & store.

				loadStoreGroup->addChild(new ImageLoadAndStoreCase(m_context, formatName.c_str(), "", format, imageType));
				if (textureLayerType(imageType) != imageType)
					loadStoreGroup->addChild(new ImageLoadAndStoreCase(m_context, (formatName + "_single_layer").c_str(), "", format, imageType, ImageLoadAndStoreCase::CASEFLAG_SINGLE_LAYER_BIND));

				if (format.order == TextureFormat::R)
				{
					// Atomic operations.

					for (int operationI = 0; operationI < ATOMIC_OPERATION_LAST; operationI++)
					{
						for (int atomicCaseTypeI = 0; atomicCaseTypeI < ATOMIC_OPERATION_CASE_TYPE_LAST; atomicCaseTypeI++)
						{
							const AtomicOperation operation = (AtomicOperation)operationI;

							if (format.type == TextureFormat::FLOAT && operation != ATOMIC_OPERATION_EXCHANGE)
								continue;

							const AtomicOperationCaseType	caseType		= (AtomicOperationCaseType)atomicCaseTypeI;
							const string					caseTypeName	= caseType == ATOMIC_OPERATION_CASE_TYPE_END_RESULT		? "result"
																			: caseType == ATOMIC_OPERATION_CASE_TYPE_RETURN_VALUES	? "return_value"
																			: DE_NULL;
							const string					caseName		= string() + getAtomicOperationCaseName(operation) + "_" + formatName + "_" + caseTypeName;

							if (operation == ATOMIC_OPERATION_COMP_SWAP)
								atomicGroup->addChild(new AtomicCompSwapCase(m_context, caseName.c_str(), "", format, imageType, caseType));
							else
								atomicGroup->addChild(new BinaryAtomicOperationCase(m_context, caseName.c_str(), "", format, imageType, operation, caseType));
						}
					}

					// Coherence.

					for (int coherenceQualifierI = 0; coherenceQualifierI < CoherenceCase::QUALIFIER_LAST; coherenceQualifierI++)
					{
						const CoherenceCase::Qualifier	coherenceQualifier		= (CoherenceCase::Qualifier)coherenceQualifierI;
						const char* const				coherenceQualifierName	= coherenceQualifier == CoherenceCase::QUALIFIER_COHERENT ? "coherent"
																				: coherenceQualifier == CoherenceCase::QUALIFIER_VOLATILE ? "volatile"
																				: DE_NULL;
						const string					caseName				= string() + coherenceQualifierName + "_" + formatName;

						qualifierGroup->addChild(new CoherenceCase(m_context, caseName.c_str(), "", format, imageType, coherenceQualifier));
					}
				}
			}

			// Restrict.
			qualifierGroup->addChild(new ImageLoadAndStoreCase(m_context, "restrict", "", TextureFormat(TextureFormat::RGBA, TextureFormat::UNSIGNED_INT32), imageType, ImageLoadAndStoreCase::CASEFLAG_RESTRICT_IMAGES));

			// Format re-interpretation.

			for (int texFmtNdx = 0; texFmtNdx < DE_LENGTH_OF_ARRAY(formats); texFmtNdx++)
			for (int imgFmtNdx = 0; imgFmtNdx < DE_LENGTH_OF_ARRAY(formats); imgFmtNdx++)
			{
				const TextureFormat& texFmt = formats[texFmtNdx];
				const TextureFormat& imgFmt = formats[imgFmtNdx];

				if (imageType == TEXTURETYPE_BUFFER && !isFormatSupportedForTextureBuffer(texFmt))
					continue;

				if (texFmt != imgFmt && texFmt.getPixelSize() == imgFmt.getPixelSize())
					reinterpretGroup->addChild(new ImageLoadAndStoreCase(m_context,
																		 (getShaderImageFormatQualifier(texFmt) + "_" + getShaderImageFormatQualifier(imgFmt)).c_str(), "",
																		 texFmt, imgFmt, imageType));
			}

			// imageSize().

			{
				static const IVec3 baseImageSizes[] =
				{
					IVec3(32, 32, 32),
					IVec3(12, 34, 56),
					IVec3(1,   1,  1),
					IVec3(7,   1,  1)
				};

				for (int imageAccessI = 0; imageAccessI < ImageSizeCase::IMAGEACCESS_LAST; imageAccessI++)
				{
					const ImageSizeCase::ImageAccess	imageAccess		= (ImageSizeCase::ImageAccess)imageAccessI;
					const char* const					imageAccessStr	= imageAccess == ImageSizeCase::IMAGEACCESS_READ_ONLY				? "readonly"
																		: imageAccess == ImageSizeCase::IMAGEACCESS_WRITE_ONLY				? "writeonly"
																		: imageAccess == ImageSizeCase::IMAGEACCESS_READ_ONLY_WRITE_ONLY	? "readonly_writeonly"
																		: DE_NULL;

					for (int imageSizeNdx = 0; imageSizeNdx < DE_LENGTH_OF_ARRAY(baseImageSizes); imageSizeNdx++)
					{
						const IVec3&	baseSize	= baseImageSizes[imageSizeNdx];
						const IVec3		imageSize	= imageType == TEXTURETYPE_BUFFER		? IVec3(baseSize.x(), 1, 1)
													: imageType == TEXTURETYPE_2D			? IVec3(baseSize.x(), baseSize.y(), 1)
													: imageType == TEXTURETYPE_CUBE			? IVec3(baseSize.x(), baseSize.x(), 1)
													: imageType == TEXTURETYPE_3D			? baseSize
													: imageType == TEXTURETYPE_2D_ARRAY		? baseSize
													: IVec3(-1, -1, -1);

						const string	sizeStr		= imageType == TEXTURETYPE_BUFFER		? toString(imageSize.x())
													: imageType == TEXTURETYPE_2D			? toString(imageSize.x()) + "x" + toString(imageSize.y())
													: imageType == TEXTURETYPE_CUBE			? toString(imageSize.x()) + "x" + toString(imageSize.y())
													: imageType == TEXTURETYPE_3D			? toString(imageSize.x()) + "x" + toString(imageSize.y()) + "x" + toString(imageSize.z())
													: imageType == TEXTURETYPE_2D_ARRAY		? toString(imageSize.x()) + "x" + toString(imageSize.y()) + "x" + toString(imageSize.z())
													: DE_NULL;

						const string	caseName	= string() + imageAccessStr + "_" + sizeStr;

						imageSizeGroup->addChild(new ImageSizeCase(m_context, caseName.c_str(), "", TextureFormat(TextureFormat::RGBA, TextureFormat::FLOAT), imageType, imageSize, imageAccess));
					}
				}
			}
		}
	}

	// early_fragment_tests cases.

	{
		TestCaseGroup* const earlyTestsGroup = new TestCaseGroup(m_context, "early_fragment_tests", "");
		addChild(earlyTestsGroup);

		for (int testRenderTargetI = 0; testRenderTargetI < EarlyFragmentTestsCase::RENDERTARGET_LAST; testRenderTargetI++)
		for (int useEarlyTestsI = 0; useEarlyTestsI <= 1; useEarlyTestsI++)
		for (int testTypeI = 0; testTypeI < EarlyFragmentTestsCase::TESTTYPE_LAST; testTypeI++)
		{
			const EarlyFragmentTestsCase::RenderTargetType	targetType		= (EarlyFragmentTestsCase::RenderTargetType)testRenderTargetI;
			const bool										useEarlyTests	= useEarlyTestsI != 0;
			const EarlyFragmentTestsCase::TestType			testType		= (EarlyFragmentTestsCase::TestType)testTypeI;

			const string									testTypeName	= testType == EarlyFragmentTestsCase::TESTTYPE_DEPTH	? "depth"
																			: testType == EarlyFragmentTestsCase::TESTTYPE_STENCIL	? "stencil"
																			: DE_NULL;

			const string									targetName		= targetType == EarlyFragmentTestsCase::RENDERTARGET_FBO							? (std::string("_fbo"))
																			: targetType == EarlyFragmentTestsCase::RENDERTARGET_FBO_WITHOUT_TEST_ATTACHMENT	? (std::string("_fbo_with_no_") + testTypeName)
																			: std::string("");

			const string									caseName		= string(useEarlyTests ? "" : "no_") + "early_fragment_tests_" + testTypeName + targetName;

			const string									caseDesc		= string(useEarlyTests ? "Specify" : "Don't specify")
																			+ " early_fragment_tests, use the " + testTypeName + " test"
																			+ ((targetType == EarlyFragmentTestsCase::RENDERTARGET_FBO)								? (", render to fbo")
																			   : (targetType == EarlyFragmentTestsCase::RENDERTARGET_FBO_WITHOUT_TEST_ATTACHMENT)	? (", render to fbo without relevant buffer")
																			   : (""));

			earlyTestsGroup->addChild(new EarlyFragmentTestsCase(m_context, caseName.c_str(), caseDesc.c_str(), testType, useEarlyTests, targetType));
		}
	}
}

} // Functional
} // gles31
} // deqp
