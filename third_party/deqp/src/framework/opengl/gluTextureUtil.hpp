#ifndef _GLUTEXTUREUTIL_HPP
#define _GLUTEXTUREUTIL_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES Utilities
 * ------------------------------------------------
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
 * \brief Texture format utilities.
 *//*--------------------------------------------------------------------*/

#include "gluDefs.hpp"
#include "tcuTexture.hpp"
#include "tcuCompressedTexture.hpp"
#include "gluShaderUtil.hpp"
#include "deInt32.h"

namespace glu
{

class RenderContext;
class ContextInfo;
class TextureBuffer;

/*--------------------------------------------------------------------*//*!
 * \brief GL pixel transfer format.
 *//*--------------------------------------------------------------------*/
struct TransferFormat
{
	deUint32	format;		//!< Pixel format.
	deUint32	dataType;	//!< Data type.

	TransferFormat (void)
		: format	(0)
		, dataType	(0)
	{
	}

	TransferFormat (deUint32 format_, deUint32 dataType_)
		: format	(format_)
		, dataType	(dataType_)
	{
	}
} DE_WARN_UNUSED_TYPE;

tcu::TextureFormat				mapGLTransferFormat					(deUint32 format, deUint32 dataType);
tcu::TextureFormat				mapGLInternalFormat					(deUint32 internalFormat);
tcu::CompressedTexFormat		mapGLCompressedTexFormat			(deUint32 format);
bool							isGLInternalColorFormatFilterable	(deUint32 internalFormat);
tcu::Sampler					mapGLSampler						(deUint32 wrapS, deUint32 minFilter, deUint32 magFilter);
tcu::Sampler					mapGLSampler						(deUint32 wrapS, deUint32 wrapT, deUint32 minFilter, deUint32 magFilter);
tcu::Sampler					mapGLSampler						(deUint32 wrapS, deUint32 wrapT, deUint32 wrapR, deUint32 minFilter, deUint32 magFilter);
tcu::Sampler::CompareMode		mapGLCompareFunc					(deUint32 mode);

TransferFormat					getTransferFormat					(tcu::TextureFormat format);
deUint32						getInternalFormat					(tcu::TextureFormat format);
deUint32						getGLFormat							(tcu::CompressedTexFormat format);

deUint32						getGLWrapMode						(tcu::Sampler::WrapMode wrapMode);
deUint32						getGLFilterMode						(tcu::Sampler::FilterMode filterMode);
deUint32						getGLCompareFunc					(tcu::Sampler::CompareMode compareMode);

deUint32						getGLCubeFace						(tcu::CubeFace face);
tcu::CubeFace					getCubeFaceFromGL					(deUint32 face);

DataType						getSampler1DType					(tcu::TextureFormat format);
DataType						getSampler2DType					(tcu::TextureFormat format);
DataType						getSamplerCubeType					(tcu::TextureFormat format);
DataType						getSampler1DArrayType				(tcu::TextureFormat format);
DataType						getSampler2DArrayType				(tcu::TextureFormat format);
DataType						getSampler3DType					(tcu::TextureFormat format);
DataType						getSamplerCubeArrayType				(tcu::TextureFormat format);

bool							isSizedFormatColorRenderable		(const RenderContext& renderCtx, const ContextInfo& contextInfo, deUint32 sizedFormat);
bool							isCompressedFormat					(deUint32 internalFormat);

const tcu::IVec2				(&getDefaultGatherOffsets			(void))[4];

tcu::PixelBufferAccess			getTextureBufferEffectiveRefTexture	(TextureBuffer& buffer, int maxTextureBufferSize);
tcu::ConstPixelBufferAccess		getTextureBufferEffectiveRefTexture	(const TextureBuffer& buffer, int maxTextureBufferSize);

} // glu

#endif // _GLUTEXTUREUTIL_HPP
