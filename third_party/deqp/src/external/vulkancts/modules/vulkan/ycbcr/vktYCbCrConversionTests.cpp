/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2017 Google Inc.
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
 * \brief Texture color conversion tests
 *//*--------------------------------------------------------------------*/

#include "vktYCbCrConversionTests.hpp"

#include "vktShaderExecutor.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTestGroupUtil.hpp"
#include "vktYCbCrUtil.hpp"

#include "vkImageUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkPrograms.hpp"
#include "vkRefUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkQueryUtil.hpp"

#include "tcuInterval.hpp"
#include "tcuTestLog.hpp"
#include "tcuTexture.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuVector.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuFloatFormat.hpp"
#include "tcuFloat.hpp"

#include "deRandom.hpp"
#include "deSTLUtil.hpp"
#include "deSharedPtr.hpp"

#include "deMath.h"
#include "deFloat16.h"

#include <vector>
#include <iomanip>

// \todo When defined color conversion extension is not used and conversion is performed in the shader
// #define FAKE_COLOR_CONVERSION

using tcu::Vec2;
using tcu::Vec4;

using tcu::UVec2;
using tcu::UVec3;
using tcu::UVec4;

using tcu::IVec2;
using tcu::IVec3;
using tcu::IVec4;

using tcu::TestLog;
using tcu::Interval;
using tcu::FloatFormat;

using std::vector;
using std::string;

using namespace vkt::shaderexecutor;

namespace vkt
{
namespace ycbcr
{
namespace
{
typedef de::SharedPtr<vk::Unique<vk::VkBuffer> > VkBufferSp;
typedef de::SharedPtr<vk::Allocation> AllocationSp;

// \note Used for range expansion
UVec4 getBitDepth (vk::VkFormat format)
{
	switch (format)
	{
		case vk::VK_FORMAT_G8B8G8R8_422_UNORM_KHR:
		case vk::VK_FORMAT_B8G8R8G8_422_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR:
			return UVec4(8, 8, 8, 0);

		case vk::VK_FORMAT_R10X6_UNORM_PACK16_KHR:
			return UVec4(10, 0, 0, 0);

		case vk::VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR:
			return UVec4(10, 10, 0, 0);

		case vk::VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR:
			return UVec4(10, 10, 10, 10);

		case vk::VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR:
			return UVec4(10, 10, 10, 0);

		case vk::VK_FORMAT_R12X4_UNORM_PACK16_KHR:
			return UVec4(12, 0, 0, 0);

		case vk::VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR:
			return UVec4(12, 12, 0, 0);

		case vk::VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR:
			return UVec4(12, 12, 12, 12);

		case vk::VK_FORMAT_G16B16G16R16_422_UNORM_KHR:
		case vk::VK_FORMAT_B16G16R16G16_422_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR:
			return UVec4(16, 16, 16, 0);

		default:
			return tcu::getTextureFormatBitDepth(vk::mapVkFormat(format)).cast<deUint32>();
	}
}

// \note Taken from explicit lod filtering tests
FloatFormat getFilteringPrecision (vk::VkFormat format)
{
	const FloatFormat	reallyLow	(0, 0, 6, false, tcu::YES);
	const FloatFormat	low			(0, 0, 7, false, tcu::YES);
	const FloatFormat	fp16		(-14, 15, 10, false);
	const FloatFormat	fp32		(-126, 127, 23, true);

	switch (format)
	{
		case vk::VK_FORMAT_R4G4B4A4_UNORM_PACK16:
		case vk::VK_FORMAT_B4G4R4A4_UNORM_PACK16:
		case vk::VK_FORMAT_R5G6B5_UNORM_PACK16:
		case vk::VK_FORMAT_B5G6R5_UNORM_PACK16:
		case vk::VK_FORMAT_R5G5B5A1_UNORM_PACK16:
		case vk::VK_FORMAT_B5G5R5A1_UNORM_PACK16:
		case vk::VK_FORMAT_A1R5G5B5_UNORM_PACK16:
			return reallyLow;

		case vk::VK_FORMAT_R8G8B8_UNORM:
		case vk::VK_FORMAT_B8G8R8_UNORM:
		case vk::VK_FORMAT_R8G8B8A8_UNORM:
		case vk::VK_FORMAT_B8G8R8A8_UNORM:
		case vk::VK_FORMAT_A8B8G8R8_UNORM_PACK32:
		case vk::VK_FORMAT_G8B8G8R8_422_UNORM_KHR:
		case vk::VK_FORMAT_B8G8R8G8_422_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR:
			return low;

		case vk::VK_FORMAT_A2R10G10B10_UNORM_PACK32:
		case vk::VK_FORMAT_A2B10G10R10_UNORM_PACK32:
		case vk::VK_FORMAT_R16G16B16_UNORM:
		case vk::VK_FORMAT_R16G16B16A16_UNORM:
		case vk::VK_FORMAT_R10X6_UNORM_PACK16_KHR:
		case vk::VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR:
		case vk::VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_R12X4_UNORM_PACK16_KHR:
		case vk::VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR:
		case vk::VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G16B16G16R16_422_UNORM_KHR:
		case vk::VK_FORMAT_B16G16R16G16_422_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR:
			return fp16;

		default:
			DE_FATAL("Precision not defined for format");
			return fp32;
	}
}

// \note Taken from explicit lod filtering tests
FloatFormat getConversionPrecision (vk::VkFormat format)
{
	const FloatFormat	reallyLow	(0, 0, 8, false, tcu::YES);
	const FloatFormat	fp16		(-14, 15, 10, false);
	const FloatFormat	fp32		(-126, 127, 23, true);

	switch (format)
	{
		case vk::VK_FORMAT_R4G4B4A4_UNORM_PACK16:
		case vk::VK_FORMAT_B4G4R4A4_UNORM_PACK16:
		case vk::VK_FORMAT_R5G6B5_UNORM_PACK16:
		case vk::VK_FORMAT_B5G6R5_UNORM_PACK16:
		case vk::VK_FORMAT_R5G5B5A1_UNORM_PACK16:
		case vk::VK_FORMAT_B5G5R5A1_UNORM_PACK16:
		case vk::VK_FORMAT_A1R5G5B5_UNORM_PACK16:
			return reallyLow;

		case vk::VK_FORMAT_R8G8B8_UNORM:
		case vk::VK_FORMAT_B8G8R8_UNORM:
		case vk::VK_FORMAT_R8G8B8A8_UNORM:
		case vk::VK_FORMAT_B8G8R8A8_UNORM:
		case vk::VK_FORMAT_A8B8G8R8_UNORM_PACK32:
		case vk::VK_FORMAT_G8B8G8R8_422_UNORM_KHR:
		case vk::VK_FORMAT_B8G8R8G8_422_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR:
			return reallyLow;

		case vk::VK_FORMAT_A2R10G10B10_UNORM_PACK32:
		case vk::VK_FORMAT_A2B10G10R10_UNORM_PACK32:
		case vk::VK_FORMAT_R16G16B16_UNORM:
		case vk::VK_FORMAT_R16G16B16A16_UNORM:
		case vk::VK_FORMAT_R10X6_UNORM_PACK16_KHR:
		case vk::VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR:
		case vk::VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_R12X4_UNORM_PACK16_KHR:
		case vk::VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR:
		case vk::VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G16B16G16R16_422_UNORM_KHR:
		case vk::VK_FORMAT_B16G16R16G16_422_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR:
			return fp16;

		default:
			DE_FATAL("Precision not defined for format");
			return fp32;
	}
}

class ChannelAccess
{
public:
					ChannelAccess	(tcu::TextureChannelClass	channelClass,
									 deUint8					channelSize,
									 const IVec3&				size,
									 const IVec3&				bitPitch,
									 void*						data,
									 deUint32					bitOffset);

	const IVec3&	getSize			(void) const { return m_size; }
	const IVec3&	getBitPitch		(void) const { return m_bitPitch; }
	void*			getDataPtr		(void) const { return m_data; }

	Interval		getChannel		(const FloatFormat&	conversionFormat,
									 const IVec3&		pos) const;
	deUint32		getChannelUint	(const IVec3& pos) const;
	float			getChannel		(const IVec3& pos) const;
	void			setChannel		(const IVec3& pos, deUint32 x);
	void			setChannel		(const IVec3& pos, float x);

private:
	const tcu::TextureChannelClass	m_channelClass;
	const deUint8					m_channelSize;
	const IVec3						m_size;
	const IVec3						m_bitPitch;
	void* const						m_data;
	const deInt32					m_bitOffset;

};

ChannelAccess::ChannelAccess (tcu::TextureChannelClass	channelClass,
							  deUint8					channelSize,
							  const IVec3&				size,
							  const IVec3&				bitPitch,
							  void*						data,
							  deUint32					bitOffset)
	: m_channelClass	(channelClass)
	, m_channelSize		(channelSize)
	, m_size			(size)
	, m_bitPitch		(bitPitch)

	, m_data			((deUint8*)data + (bitOffset / 8))
	, m_bitOffset		(bitOffset % 8)
{
}

//! Extend < 32b signed integer to 32b
inline deInt32 signExtend (deUint32 src, int bits)
{
	const deUint32 signBit = 1u << (bits-1);

	src |= ~((src & signBit) - 1);

	return (deInt32)src;
}

deUint32 divRoundUp (deUint32 a, deUint32 b)
{
	if (a % b == 0)
		return a / b;
	else
		return (a / b) + 1;
}

deUint32 ChannelAccess::getChannelUint (const IVec3& pos) const
{
	DE_ASSERT(pos[0] < m_size[0]);
	DE_ASSERT(pos[1] < m_size[1]);
	DE_ASSERT(pos[2] < m_size[2]);

	const deInt32			bitOffset	(m_bitOffset + tcu::dot(m_bitPitch, pos));
	const deUint8* const	firstByte	= ((const deUint8*)m_data) + (bitOffset / 8);
	const deUint32			byteCount	= divRoundUp((bitOffset + m_channelSize) - 8u * (bitOffset / 8u), 8u);
	const deUint32			mask		(m_channelSize == 32u ? ~0x0u : (0x1u << m_channelSize) - 1u);
	const deUint32			offset		= bitOffset % 8;
	deUint32				bits		= 0u;

	deMemcpy(&bits, firstByte, byteCount);

	return (bits >> offset) & mask;
}

void ChannelAccess::setChannel (const IVec3& pos, deUint32 x)
{
	DE_ASSERT(pos[0] < m_size[0]);
	DE_ASSERT(pos[1] < m_size[1]);
	DE_ASSERT(pos[2] < m_size[2]);

	const deInt32	bitOffset	(m_bitOffset + tcu::dot(m_bitPitch, pos));
	deUint8* const	firstByte	= ((deUint8*)m_data) + (bitOffset / 8);
	const deUint32	byteCount	= divRoundUp((bitOffset + m_channelSize) - 8u * (bitOffset / 8u), 8u);
	const deUint32	mask		(m_channelSize == 32u ? ~0x0u : (0x1u << m_channelSize) - 1u);
	const deUint32	offset		= bitOffset % 8;

	const deUint32	bits		= (x & mask) << offset;
	deUint32		oldBits		= 0;

	deMemcpy(&oldBits, firstByte, byteCount);

	{
		const deUint32	newBits	= bits | (oldBits & (~(mask << offset)));

		deMemcpy(firstByte, &newBits,  byteCount);
	}
}

float ChannelAccess::getChannel (const IVec3& pos) const
{
	const deUint32	bits	(getChannelUint(pos));

	switch (m_channelClass)
	{
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
			return (float)bits / (float)(m_channelSize == 32 ? ~0x0u : ((0x1u << m_channelSize) - 1u));

		case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
			return (float)bits;

		case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
			return de::max(-1.0f, (float)signExtend(bits, m_channelSize) / (float)((0x1u << (m_channelSize - 1u)) - 1u));

		case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
			return (float)signExtend(bits, m_channelSize);

		case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
			if (m_channelSize == 32)
				return tcu::Float32(bits).asFloat();
			else
			{
				DE_FATAL("Float type not supported");
				return -1.0f;
			}

		default:
			DE_FATAL("Unknown texture channel class");
			return -1.0f;
	}
}

Interval ChannelAccess::getChannel (const FloatFormat&	conversionFormat,
									const IVec3&		pos) const
{
	const deUint32	bits	(getChannelUint(pos));

	switch (m_channelClass)
	{
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
			return conversionFormat.roundOut(conversionFormat.roundOut((double)bits, false)
											/ conversionFormat.roundOut((double)(m_channelSize == 32 ? ~0x0u : ((0x1u << m_channelSize) - 1u)), false), false);

		case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
			return conversionFormat.roundOut((double)bits, false);

		case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
		{
			const Interval result (conversionFormat.roundOut(conversionFormat.roundOut((double)signExtend(bits, m_channelSize), false)
															/ conversionFormat.roundOut((double)((0x1u << (m_channelSize - 1u)) - 1u), false), false));

			return Interval(de::max(-1.0, result.lo()), de::max(-1.0, result.hi()));
		}

		case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
			return conversionFormat.roundOut((double)signExtend(bits, m_channelSize), false);

		case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
			if (m_channelSize == 32)
				return conversionFormat.roundOut(tcu::Float32(bits).asFloat(), false);
			else
			{
				DE_FATAL("Float type not supported");
				return Interval();
			}

		default:
			DE_FATAL("Unknown texture channel class");
			return Interval();
	}
}

// \todo Taken from tcuTexture.cpp
// \todo [2011-09-21 pyry] Move to tcutil?
template <typename T>
inline T convertSatRte (float f)
{
	// \note Doesn't work for 64-bit types
	DE_STATIC_ASSERT(sizeof(T) < sizeof(deUint64));
	DE_STATIC_ASSERT((-3 % 2 != 0) && (-4 % 2 == 0));

	deInt64	minVal	= std::numeric_limits<T>::min();
	deInt64 maxVal	= std::numeric_limits<T>::max();
	float	q		= deFloatFrac(f);
	deInt64 intVal	= (deInt64)(f-q);

	// Rounding.
	if (q == 0.5f)
	{
		if (intVal % 2 != 0)
			intVal++;
	}
	else if (q > 0.5f)
		intVal++;
	// else Don't add anything

	// Saturate.
	intVal = de::max(minVal, de::min(maxVal, intVal));

	return (T)intVal;
}

void ChannelAccess::setChannel (const IVec3& pos, float x)
{
	DE_ASSERT(pos[0] < m_size[0]);
	DE_ASSERT(pos[1] < m_size[1]);
	DE_ASSERT(pos[2] < m_size[2]);

	const deUint32	mask	(m_channelSize == 32u ? ~0x0u : (0x1u << m_channelSize) - 1u);

	switch (m_channelClass)
	{
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
		{
			const deUint32	maxValue	(mask);
			const deUint32	value		(de::min(maxValue, (deUint32)convertSatRte<deUint32>(x * (float)maxValue)));
			setChannel(pos, value);
			break;
		}

		case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
		{
			const deInt32	range	((0x1u << (m_channelSize - 1u)) - 1u);
			const deUint32	value	((deUint32)de::clamp<deInt32>(convertSatRte<deInt32>(x * (float)range), -range, range));
			setChannel(pos, value);
			break;
		}

		case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
		{
			const deUint32	maxValue	(mask);
			const deUint32	value		(de::min(maxValue, (deUint32)x));
			setChannel(pos, value);
			break;
		}

		case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
		{
			const deInt32	minValue	(-(deInt32)(1u << (m_channelSize - 1u)));
			const deInt32	maxValue	((deInt32)((1u << (m_channelSize - 1u)) - 1u));
			const deUint32	value		((deUint32)de::clamp((deInt32)x, minValue, maxValue));
			setChannel(pos, value);
			break;
		}

		case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
		{
			if (m_channelSize == 32)
			{
				const deUint32	value		= tcu::Float32(x).bits();
				setChannel(pos, value);
			}
			else
				DE_FATAL("Float type not supported");
			break;
		}

		default:
			DE_FATAL("Unknown texture channel class");
	}
}

ChannelAccess getChannelAccess (MultiPlaneImageData&				data,
								const vk::PlanarFormatDescription&	formatInfo,
								const UVec2&						size,
								int									channelNdx)
{
	DE_ASSERT(formatInfo.hasChannelNdx(channelNdx));

	const deUint32	planeNdx			= formatInfo.channels[channelNdx].planeNdx;
	const deUint32	valueOffsetBits		= formatInfo.channels[channelNdx].offsetBits;
	const deUint32	pixelStrideBytes	= formatInfo.channels[channelNdx].strideBytes;
	const deUint32	pixelStrideBits		= pixelStrideBytes * 8;
	const deUint8	sizeBits			= formatInfo.channels[channelNdx].sizeBits;

	DE_ASSERT(size.x() % formatInfo.planes[planeNdx].widthDivisor == 0);
	DE_ASSERT(size.y() % formatInfo.planes[planeNdx].heightDivisor == 0);

	deUint32		accessWidth			= size.x() / formatInfo.planes[planeNdx].widthDivisor;
	const deUint32	accessHeight		= size.y() / formatInfo.planes[planeNdx].heightDivisor;
	const deUint32	elementSizeBytes	= formatInfo.planes[planeNdx].elementSizeBytes;

	const deUint32	rowPitch			= formatInfo.planes[planeNdx].elementSizeBytes * accessWidth;
	const deUint32	rowPitchBits		= rowPitch * 8;

	if (pixelStrideBytes != elementSizeBytes)
	{
		DE_ASSERT(elementSizeBytes % pixelStrideBytes == 0);
		accessWidth *= elementSizeBytes/pixelStrideBytes;
	}

	return ChannelAccess((tcu::TextureChannelClass)formatInfo.channels[channelNdx].type, sizeBits, IVec3(accessWidth, accessHeight, 1u), IVec3((int)pixelStrideBits, (int)rowPitchBits, 0), data.getPlanePtr(planeNdx), (deUint32)valueOffsetBits);
}

ShaderSpec createShaderSpec (void)
{
	ShaderSpec spec;

	spec.globalDeclarations = "layout(set=" + de::toString((int)EXTRA_RESOURCES_DESCRIPTOR_SET_INDEX) + ", binding=0) uniform highp sampler2D u_sampler;";

	spec.inputs.push_back(Symbol("uv", glu::VarType(glu::TYPE_FLOAT_VEC2, glu::PRECISION_HIGHP)));
	spec.outputs.push_back(Symbol("o_color", glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP)));

	spec.source = "o_color = texture(u_sampler, uv);\n";

	return spec;
}

void genTexCoords (std::vector<Vec2>&	coords,
				   const UVec2&			size)
{
	for (deUint32 y = 0; y < size.y() + (size.y() / 2); y++)
	for (deUint32 x = 0; x < size.x() + (size.x() / 2); x++)
	{
		const float	fx	= (float)x;
		const float	fy	= (float)y;

		const float	fw	= (float)size.x();
		const float	fh	= (float)size.y();

		const float	s	= 1.5f * ((fx * 1.5f * fw + fx) / (1.5f * fw * 1.5f * fw)) - 0.25f;
		const float	t	= 1.5f * ((fy * 1.5f * fh + fy) / (1.5f * fh * 1.5f * fh)) - 0.25f;

		coords.push_back(Vec2(s, t));
	}
}

Interval rangeExpandChroma (vk::VkSamplerYcbcrRangeKHR	range,
							const FloatFormat&			conversionFormat,
							const deUint32				bits,
							const Interval&				sample)
{
	const deUint32	values	(0x1u << bits);

	switch (range)
	{
		case vk::VK_SAMPLER_YCBCR_RANGE_ITU_FULL_KHR:
			return conversionFormat.roundOut(sample - conversionFormat.roundOut(Interval((double)(0x1u << (bits - 1u)) / (double)((0x1u << bits) - 1u)), false), false);

		case vk::VK_SAMPLER_YCBCR_RANGE_ITU_NARROW_KHR:
		{
			const Interval	a			(conversionFormat.roundOut(sample * Interval((double)(values - 1u)), false));
			const Interval	dividend	(conversionFormat.roundOut(a - Interval((double)(128u * (0x1u << (bits - 8u)))), false));
			const Interval	divisor		((double)(224u * (0x1u << (bits - 8u))));
			const Interval	result		(conversionFormat.roundOut(dividend / divisor, false));

			return result;
		}

		default:
			DE_FATAL("Unknown YCbCrRange");
			return Interval();
	}
}

Interval rangeExpandLuma (vk::VkSamplerYcbcrRangeKHR	range,
						  const FloatFormat&			conversionFormat,
						  const deUint32				bits,
						  const Interval&				sample)
{
	const deUint32	values	(0x1u << bits);

	switch (range)
	{
		case vk::VK_SAMPLER_YCBCR_RANGE_ITU_FULL_KHR:
			return conversionFormat.roundOut(sample, false);

		case vk::VK_SAMPLER_YCBCR_RANGE_ITU_NARROW_KHR:
		{
			const Interval	a			(conversionFormat.roundOut(sample * Interval((double)(values - 1u)), false));
			const Interval	dividend	(conversionFormat.roundOut(a - Interval((double)(16u * (0x1u << (bits - 8u)))), false));
			const Interval	divisor		((double)(219u * (0x1u << (bits - 8u))));
			const Interval	result		(conversionFormat.roundOut(dividend / divisor, false));

			return result;
		}

		default:
			DE_FATAL("Unknown YCbCrRange");
			return Interval();
	}
}

Interval clampMaybe (const Interval&	x,
					 double				min,
					 double				max)
{
	Interval result = x;

	DE_ASSERT(min <= max);

	if (x.lo() < min)
		result = result | Interval(min);

	if (x.hi() > max)
		result = result | Interval(max);

	return result;
}

void convertColor (vk::VkSamplerYcbcrModelConversionKHR	colorModel,
				   vk::VkSamplerYcbcrRangeKHR			range,
				   const FloatFormat&					conversionFormat,
				   const UVec4&							bitDepth,
				   const Interval						input[4],
				   Interval								output[4])
{
	switch (colorModel)
	{
		case vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY_KHR:
		{
			for (size_t ndx = 0; ndx < 4; ndx++)
				output[ndx] = input[ndx];
			break;
		}

		case vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_IDENTITY_KHR:
		{
			output[0] = clampMaybe(rangeExpandChroma(range, conversionFormat, bitDepth[0], input[0]), -0.5, 0.5);
			output[1] = clampMaybe(rangeExpandLuma(range, conversionFormat, bitDepth[1], input[1]), 0.0, 1.0);
			output[2] = clampMaybe(rangeExpandChroma(range, conversionFormat, bitDepth[2], input[2]), -0.5, 0.5);
			output[3] = input[3];
			break;
		}

		case vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601_KHR:
		{
			const Interval	y			(rangeExpandLuma(range, conversionFormat, bitDepth[1], input[1]));
			const Interval	cr			(rangeExpandChroma(range, conversionFormat, bitDepth[0], input[0]));
			const Interval	cb			(rangeExpandChroma(range, conversionFormat, bitDepth[2], input[2]));

			const Interval	yClamped	(clampMaybe(y,   0.0, 1.0));
			const Interval	crClamped	(clampMaybe(cr, -0.5, 0.5));
			const Interval	cbClamped	(clampMaybe(cb, -0.5, 0.5));

			output[0] = conversionFormat.roundOut(yClamped + conversionFormat.roundOut(1.402 * crClamped, false), false);
			output[1] = conversionFormat.roundOut(conversionFormat.roundOut(yClamped - conversionFormat.roundOut((0.202008 / 0.587) * cbClamped, false), false) - conversionFormat.roundOut((0.419198 / 0.587) * crClamped, false), false);
			output[2] = conversionFormat.roundOut(yClamped + conversionFormat.roundOut(1.772 * cbClamped, false), false);
			output[3] = input[3];
			break;
		}

		case vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709_KHR:
		{
			const Interval	y			(rangeExpandLuma(range, conversionFormat, bitDepth[1], input[1]));
			const Interval	cr			(rangeExpandChroma(range, conversionFormat, bitDepth[0], input[0]));
			const Interval	cb			(rangeExpandChroma(range, conversionFormat, bitDepth[2], input[2]));

			const Interval	yClamped	(clampMaybe(y,   0.0, 1.0));
			const Interval	crClamped	(clampMaybe(cr, -0.5, 0.5));
			const Interval	cbClamped	(clampMaybe(cb, -0.5, 0.5));

			output[0] = conversionFormat.roundOut(yClamped + conversionFormat.roundOut(1.5748 * crClamped, false), false);
			output[1] = conversionFormat.roundOut(conversionFormat.roundOut(yClamped - conversionFormat.roundOut((0.13397432 / 0.7152) * cbClamped, false), false) - conversionFormat.roundOut((0.33480248 / 0.7152) * crClamped, false), false);
			output[2] = conversionFormat.roundOut(yClamped + conversionFormat.roundOut(1.8556 * cbClamped, false), false);
			output[3] = input[3];
			break;
		}

		case vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020_KHR:
		{
			const Interval	y			(rangeExpandLuma(range, conversionFormat, bitDepth[1], input[1]));
			const Interval	cr			(rangeExpandChroma(range, conversionFormat, bitDepth[0], input[0]));
			const Interval	cb			(rangeExpandChroma(range, conversionFormat, bitDepth[2], input[2]));

			const Interval	yClamped	(clampMaybe(y,   0.0, 1.0));
			const Interval	crClamped	(clampMaybe(cr, -0.5, 0.5));
			const Interval	cbClamped	(clampMaybe(cb, -0.5, 0.5));

			output[0] = conversionFormat.roundOut(yClamped + conversionFormat.roundOut(1.4746 * crClamped, false), false);
			output[1] = conversionFormat.roundOut(conversionFormat.roundOut(yClamped - conversionFormat.roundOut(conversionFormat.roundOut(0.11156702 / 0.6780, false) * cbClamped, false), false) - conversionFormat.roundOut(conversionFormat.roundOut(0.38737742 / 0.6780, false) * crClamped, false), false);
			output[2] = conversionFormat.roundOut(yClamped + conversionFormat.roundOut(1.8814 * cbClamped, false), false);
			output[3] = input[3];
			break;
		}

		default:
			DE_FATAL("Unknown YCbCrModel");
	}

	if (colorModel != vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_IDENTITY_KHR)
	{
		for (int ndx = 0; ndx < 3; ndx++)
			output[ndx] = clampMaybe(output[ndx], 0.0, 1.0);
	}
}

int mirror (int coord)
{
	return coord >= 0 ? coord : -(1 + coord);
}

int imod (int a, int b)
{
	int m = a % b;
	return m < 0 ? m + b : m;
}

int wrap (vk::VkSamplerAddressMode	addressMode,
		  int						coord,
		  int						size)
{
	switch (addressMode)
	{
		case vk::VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
			return (size - 1) - mirror(imod(coord, 2 * size) - size);

		case vk::VK_SAMPLER_ADDRESS_MODE_REPEAT:
			return imod(coord, size);

		case vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
			return de::clamp(coord, 0, size - 1);

		case vk::VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:
			return de::clamp(mirror(coord), 0, size - 1);

		default:
			DE_FATAL("Unknown wrap mode");
			return ~0;
	}
}

Interval frac (const Interval& x)
{
	if (x.hi() - x.lo() >= 1.0)
		return Interval(0.0, 1.0);
	else
	{
		const Interval ret (deFrac(x.lo()), deFrac(x.hi()));

		return ret;
	}
}

Interval calculateUV (const FloatFormat&	coordFormat,
					  const Interval&		st,
					  const int				size)
{
	return coordFormat.roundOut(coordFormat.roundOut(st, false) * Interval((double)size), false);
}

IVec2 calculateNearestIJRange (const FloatFormat&	coordFormat,
							   const Interval&		uv)
{
	const Interval	ij	(coordFormat.roundOut(coordFormat.roundOut(uv, false) - Interval(0.5), false));

	return IVec2(deRoundToInt32(ij.lo() - coordFormat.ulp(ij.lo(), 1)), deRoundToInt32(ij.hi() + coordFormat.ulp(ij.hi(), 1)));
}

// Calculate range of pixel coordinates that can be used as lower coordinate for linear sampling
IVec2 calculateLinearIJRange (const FloatFormat&	coordFormat,
							  const Interval&		uv)
{
	const Interval	ij	(coordFormat.roundOut(uv - Interval(0.5), false));

	return IVec2(deFloorToInt32(ij.lo()), deFloorToInt32(ij.hi()));
}

Interval calculateAB (const deUint32	subTexelPrecisionBits,
					  const Interval&	uv,
					  int				ij)
{
	const deUint32	subdivisions	= 0x1u << subTexelPrecisionBits;
	const Interval	ab				(frac((uv - 0.5) & Interval((double)ij, (double)(ij + 1))));
	const Interval	gridAB			(ab * Interval(subdivisions));
	const Interval	rounded			(de::max(deFloor(gridAB.lo()) / subdivisions, 0.0) , de::min(deCeil(gridAB.hi()) / subdivisions, 1.0));

	return rounded;
}

Interval lookupWrapped (const ChannelAccess&		access,
						const FloatFormat&			conversionFormat,
						vk::VkSamplerAddressMode	addressModeU,
						vk::VkSamplerAddressMode	addressModeV,
						const IVec2&				coord)
{
	return access.getChannel(conversionFormat, IVec3(wrap(addressModeU, coord.x(), access.getSize().x()), wrap(addressModeV, coord.y(), access.getSize().y()), 0));
}

Interval linearInterpolate (const FloatFormat&	filteringFormat,
							const Interval&		a,
							const Interval&		b,
							const Interval&		p00,
							const Interval&		p10,
							const Interval&		p01,
							const Interval&		p11)
{
	const Interval p[4] =
	{
		p00,
		p10,
		p01,
		p11
	};
	Interval	result	(0.0);

	for (size_t ndx = 0; ndx < 4; ndx++)
	{
		const Interval	weightA	(filteringFormat.roundOut((ndx % 2) == 0 ? (1.0 - a) : a, false));
		const Interval	weightB	(filteringFormat.roundOut((ndx / 2) == 0 ? (1.0 - b) : b, false));
		const Interval	weight	(filteringFormat.roundOut(weightA * weightB, false));

		result = filteringFormat.roundOut(result + filteringFormat.roundOut(p[ndx] * weight, false), false);
	}

	return result;
}

Interval calculateImplicitChromaUV (const FloatFormat&		coordFormat,
									vk::VkChromaLocationKHR	offset,
									const Interval&			uv)
{
	if (offset == vk::VK_CHROMA_LOCATION_COSITED_EVEN_KHR)
		return coordFormat.roundOut(0.5 * coordFormat.roundOut(uv + 0.5, false), false);
	else
		return coordFormat.roundOut(0.5 * uv, false);
}

Interval linearSample (const ChannelAccess&		access,
					   const FloatFormat&		conversionFormat,
					   const FloatFormat&		filteringFormat,
					   vk::VkSamplerAddressMode	addressModeU,
					   vk::VkSamplerAddressMode	addressModeV,
					   const IVec2&				coord,
					   const Interval&			a,
					   const Interval&			b)
{
	return linearInterpolate(filteringFormat, a, b,
									lookupWrapped(access, conversionFormat, addressModeU, addressModeV, coord + IVec2(0, 0)),
									lookupWrapped(access, conversionFormat, addressModeU, addressModeV, coord + IVec2(1, 0)),
									lookupWrapped(access, conversionFormat, addressModeU, addressModeV, coord + IVec2(0, 1)),
									lookupWrapped(access, conversionFormat, addressModeU, addressModeV, coord + IVec2(1, 1)));
}

int divFloor (int a, int b)
{
	if (a % b == 0)
		return a / b;
	else if (a > 0)
		return a / b;
	else
		return (a / b) - 1;
}

Interval reconstructLinearXChromaSample (const FloatFormat&			filteringFormat,
										 const FloatFormat&			conversionFormat,
										 vk::VkChromaLocationKHR	offset,
										 vk::VkSamplerAddressMode	addressModeU,
										 vk::VkSamplerAddressMode	addressModeV,
										 const ChannelAccess&		access,
										 int						i,
										 int						j)
{
	const int subI	= divFloor(i, 2);

	if (offset == vk::VK_CHROMA_LOCATION_COSITED_EVEN_KHR)
	{
		if (i % 2 == 0)
			return lookupWrapped(access, conversionFormat, addressModeU, addressModeV, IVec2(subI, j));
		else
		{
			const Interval	a	(filteringFormat.roundOut(0.5 * lookupWrapped(access, conversionFormat, addressModeU, addressModeV, IVec2(subI, j)), false));
			const Interval	b	(filteringFormat.roundOut(0.5 * lookupWrapped(access, conversionFormat, addressModeU, addressModeV, IVec2(subI + 1, j)), false));

			return filteringFormat.roundOut(a + b, false);
		}
	}
	else if (offset == vk::VK_CHROMA_LOCATION_MIDPOINT_KHR)
	{
		if (i % 2 == 0)
		{
			const Interval	a	(filteringFormat.roundOut(0.25 * lookupWrapped(access, conversionFormat, addressModeU, addressModeV, IVec2(subI - 1, j)), false));
			const Interval	b	(filteringFormat.roundOut(0.75 * lookupWrapped(access, conversionFormat, addressModeU, addressModeV, IVec2(subI, j)), false));

			return filteringFormat.roundOut(a + b, false);
		}
		else
		{
			const Interval	a	(filteringFormat.roundOut(0.25 * lookupWrapped(access, conversionFormat, addressModeU, addressModeV, IVec2(subI + 1, j)), false));
			const Interval	b	(filteringFormat.roundOut(0.75 * lookupWrapped(access, conversionFormat, addressModeU, addressModeV, IVec2(subI, j)), false));

			return filteringFormat.roundOut(a + b, false);
		}
	}
	else
	{
		DE_FATAL("Unknown sample location");
		return Interval();
	}
}

Interval reconstructLinearXYChromaSample (const FloatFormat&			filteringFormat,
										  const FloatFormat&			conversionFormat,
										  vk::VkChromaLocationKHR		xOffset,
										  vk::VkChromaLocationKHR		yOffset,
										  vk::VkSamplerAddressMode		addressModeU,
										  vk::VkSamplerAddressMode		addressModeV,
										  const ChannelAccess&			access,
										  int							i,
										  int							j)
{
	const int		subI	= xOffset == vk::VK_CHROMA_LOCATION_COSITED_EVEN_KHR
							? divFloor(i, 2)
							: (i % 2 == 0 ? divFloor(i, 2) - 1 : divFloor(i, 2));
	const int		subJ	= yOffset == vk::VK_CHROMA_LOCATION_COSITED_EVEN_KHR
							? divFloor(j, 2)
							: (j % 2 == 0 ? divFloor(j, 2) - 1 : divFloor(j, 2));

	const double	a		= xOffset == vk::VK_CHROMA_LOCATION_COSITED_EVEN_KHR
							? (i % 2 == 0 ? 0.0 : 0.5)
							: (i % 2 == 0 ? 0.25 : 0.75);
	const double	b		= yOffset == vk::VK_CHROMA_LOCATION_COSITED_EVEN_KHR
							? (j % 2 == 0 ? 0.0 : 0.5)
							: (j % 2 == 0 ? 0.25 : 0.75);

	return linearInterpolate(filteringFormat, a, b,
								lookupWrapped(access, conversionFormat, addressModeU, addressModeV, IVec2(subI, subJ)),
								lookupWrapped(access, conversionFormat, addressModeU, addressModeV, IVec2(subI + 1, subJ)),
								lookupWrapped(access, conversionFormat, addressModeU, addressModeV, IVec2(subI, subJ + 1)),
								lookupWrapped(access, conversionFormat, addressModeU, addressModeV, IVec2(subI + 1, subJ + 1)));
}

const ChannelAccess& swizzle (vk::VkComponentSwizzle	swizzle,
							  const ChannelAccess&		identityPlane,
							  const ChannelAccess&		rPlane,
							  const ChannelAccess&		gPlane,
							  const ChannelAccess&		bPlane,
							  const ChannelAccess&		aPlane)
{
	switch (swizzle)
	{
		case vk::VK_COMPONENT_SWIZZLE_IDENTITY:	return identityPlane;
		case vk::VK_COMPONENT_SWIZZLE_R:		return rPlane;
		case vk::VK_COMPONENT_SWIZZLE_G:		return gPlane;
		case vk::VK_COMPONENT_SWIZZLE_B:		return bPlane;
		case vk::VK_COMPONENT_SWIZZLE_A:		return aPlane;

		default:
			DE_FATAL("Unsupported swizzle");
			return identityPlane;
	}
}

void calculateBounds (const ChannelAccess&					rPlane,
					  const ChannelAccess&					gPlane,
					  const ChannelAccess&					bPlane,
					  const ChannelAccess&					aPlane,
					  const UVec4&							bitDepth,
					  const vector<Vec2>&					sts,
					  const FloatFormat&					filteringFormat,
					  const FloatFormat&					conversionFormat,
					  const deUint32						subTexelPrecisionBits,
					  vk::VkFilter							filter,
					  vk::VkSamplerYcbcrModelConversionKHR	colorModel,
					  vk::VkSamplerYcbcrRangeKHR			range,
					  vk::VkFilter							chromaFilter,
					  vk::VkChromaLocationKHR				xChromaOffset,
					  vk::VkChromaLocationKHR				yChromaOffset,
					  const vk::VkComponentMapping&			componentMapping,
					  bool									explicitReconstruction,
					  vk::VkSamplerAddressMode				addressModeU,
					  vk::VkSamplerAddressMode				addressModeV,
					  std::vector<Vec4>&					minBounds,
					  std::vector<Vec4>&					maxBounds,
					  std::vector<Vec4>&					uvBounds,
					  std::vector<IVec4>&					ijBounds)
{
	const FloatFormat		highp			(-126, 127, 23, true,
											 tcu::MAYBE,	// subnormals
											 tcu::YES,		// infinities
											 tcu::MAYBE);	// NaN
	const FloatFormat		coordFormat		(-32, 32, 16, true);
	const ChannelAccess&	rAccess			(swizzle(componentMapping.r, rPlane, rPlane, gPlane, bPlane, aPlane));
	const ChannelAccess&	gAccess			(swizzle(componentMapping.g, gPlane, rPlane, gPlane, bPlane, aPlane));
	const ChannelAccess&	bAccess			(swizzle(componentMapping.b, bPlane, rPlane, gPlane, bPlane, aPlane));
	const ChannelAccess&	aAccess			(swizzle(componentMapping.a, aPlane, rPlane, gPlane, bPlane, aPlane));

	const bool				subsampledX		= gAccess.getSize().x() > rAccess.getSize().x();
	const bool				subsampledY		= gAccess.getSize().y() > rAccess.getSize().y();

	minBounds.resize(sts.size(), Vec4(TCU_INFINITY));
	maxBounds.resize(sts.size(), Vec4(-TCU_INFINITY));

	uvBounds.resize(sts.size(), Vec4(TCU_INFINITY, -TCU_INFINITY, TCU_INFINITY, -TCU_INFINITY));
	ijBounds.resize(sts.size(), IVec4(0x7FFFFFFF, -1 -0x7FFFFFFF, 0x7FFFFFFF, -1 -0x7FFFFFFF));

	// Chroma plane sizes must match
	DE_ASSERT(rAccess.getSize() == bAccess.getSize());

	// Luma plane sizes must match
	DE_ASSERT(gAccess.getSize() == aAccess.getSize());

	// Luma plane size must match chroma plane or be twice as big
	DE_ASSERT(rAccess.getSize().x() == gAccess.getSize().x() || 2 * rAccess.getSize().x() == gAccess.getSize().x());
	DE_ASSERT(rAccess.getSize().y() == gAccess.getSize().y() || 2 * rAccess.getSize().y() == gAccess.getSize().y());

	for (size_t ndx = 0; ndx < sts.size(); ndx++)
	{
		const Vec2	st		(sts[ndx]);
		Interval	bounds[4];

		const Interval	u	(calculateUV(coordFormat, st[0], gAccess.getSize().x()));
		const Interval	v	(calculateUV(coordFormat, st[1], gAccess.getSize().y()));

		uvBounds[ndx][0] = (float)u.lo();
		uvBounds[ndx][1] = (float)u.hi();

		uvBounds[ndx][2] = (float)v.lo();
		uvBounds[ndx][3] = (float)v.hi();

		if (filter == vk::VK_FILTER_NEAREST)
		{
			const IVec2	iRange	(calculateNearestIJRange(coordFormat, u));
			const IVec2	jRange	(calculateNearestIJRange(coordFormat, v));

			ijBounds[ndx][0] = iRange[0];
			ijBounds[ndx][1] = iRange[1];

			ijBounds[ndx][2] = jRange[0];
			ijBounds[ndx][3] = jRange[1];

			for (int j = jRange.x(); j <= jRange.y(); j++)
			for (int i = iRange.x(); i <= iRange.y(); i++)
			{
				const Interval	gValue	(lookupWrapped(gAccess, conversionFormat, addressModeU, addressModeV, IVec2(i, j)));
				const Interval	aValue	(lookupWrapped(aAccess, conversionFormat, addressModeU, addressModeV, IVec2(i, j)));

				if (subsampledX || subsampledY)
				{
					if (explicitReconstruction)
					{
						if (chromaFilter == vk::VK_FILTER_NEAREST)
						{
							// Nearest, Reconstructed chroma with explicit nearest filtering
							const int		subI		= subsampledX ? i / 2 : i;
							const int		subJ		= subsampledY ? j / 2 : j;
							const Interval	srcColor[]	=
							{
								lookupWrapped(rAccess, conversionFormat, addressModeU, addressModeV, IVec2(subI, subJ)),
								gValue,
								lookupWrapped(bAccess, conversionFormat, addressModeU, addressModeV, IVec2(subI, subJ)),
								aValue
							};
							Interval		dstColor[4];

							convertColor(colorModel, range, conversionFormat, bitDepth, srcColor, dstColor);

							for (size_t compNdx = 0; compNdx < 4; compNdx++)
								bounds[compNdx] |= highp.roundOut(dstColor[compNdx], false);
						}
						else if (chromaFilter == vk::VK_FILTER_LINEAR)
						{
							if (subsampledX && subsampledY)
							{
								// Nearest, Reconstructed both chroma samples with explicit linear filtering
								const Interval	rValue	(reconstructLinearXYChromaSample(filteringFormat, conversionFormat, xChromaOffset, yChromaOffset, addressModeU, addressModeV, rAccess, i, j));
								const Interval	bValue	(reconstructLinearXYChromaSample(filteringFormat, conversionFormat, xChromaOffset, yChromaOffset, addressModeU, addressModeV, bAccess, i, j));
								const Interval	srcColor[]	=
								{
									rValue,
									gValue,
									bValue,
									aValue
								};
								Interval		dstColor[4];

								convertColor(colorModel, range, conversionFormat, bitDepth, srcColor, dstColor);

								for (size_t compNdx = 0; compNdx < 4; compNdx++)
									bounds[compNdx] |= highp.roundOut(dstColor[compNdx], false);
							}
							else if (subsampledX)
							{
								// Nearest, Reconstructed x chroma samples with explicit linear filtering
								const Interval	rValue	(reconstructLinearXChromaSample(filteringFormat, conversionFormat, xChromaOffset, addressModeU, addressModeV, rAccess, i, j));
								const Interval	bValue	(reconstructLinearXChromaSample(filteringFormat, conversionFormat, xChromaOffset, addressModeU, addressModeV, bAccess, i, j));
								const Interval	srcColor[]	=
								{
									rValue,
									gValue,
									bValue,
									aValue
								};
								Interval		dstColor[4];

								convertColor(colorModel, range, conversionFormat, bitDepth, srcColor, dstColor);

								for (size_t compNdx = 0; compNdx < 4; compNdx++)
									bounds[compNdx] |= highp.roundOut(dstColor[compNdx], false);
							}
							else
								DE_FATAL("Unexpected chroma reconstruction");
						}
						else
							DE_FATAL("Unknown filter");
					}
					else
					{
						const Interval	chromaU	(subsampledX ? calculateImplicitChromaUV(coordFormat, xChromaOffset, u) : u);
						const Interval	chromaV	(subsampledY ? calculateImplicitChromaUV(coordFormat, yChromaOffset, v) : v);

						if (chromaFilter == vk::VK_FILTER_NEAREST)
						{
							// Nearest, reconstructed chroma samples with implicit nearest filtering
							const IVec2	chromaIRange	(subsampledX ? calculateNearestIJRange(coordFormat, chromaU) : IVec2(i, i));
							const IVec2	chromaJRange	(subsampledY ? calculateNearestIJRange(coordFormat, chromaV) : IVec2(j, j));

							for (int chromaJ = chromaJRange.x(); chromaJ <= chromaJRange.y(); chromaJ++)
							for (int chromaI = chromaIRange.x(); chromaI <= chromaIRange.x(); chromaI++)
							{
								const Interval	srcColor[]	=
								{
									lookupWrapped(rAccess, conversionFormat, addressModeU, addressModeV, IVec2(chromaI, chromaJ)),
									gValue,
									lookupWrapped(bAccess, conversionFormat, addressModeU, addressModeV, IVec2(chromaI, chromaJ)),
									aValue
								};
								Interval		dstColor[4];

								convertColor(colorModel, range, conversionFormat, bitDepth, srcColor, dstColor);

								for (size_t compNdx = 0; compNdx < 4; compNdx++)
									bounds[compNdx] |= highp.roundOut(dstColor[compNdx], false);
							}
						}
						else if (chromaFilter == vk::VK_FILTER_LINEAR)
						{
							// Nearest, reconstructed chroma samples with implicit linear filtering
							const IVec2	chromaIRange	(subsampledX ? calculateLinearIJRange(coordFormat, chromaU) : IVec2(i, i));
							const IVec2	chromaJRange	(subsampledY ? calculateLinearIJRange(coordFormat, chromaV) : IVec2(j, j));

							for (int chromaJ = chromaJRange.x(); chromaJ <= chromaJRange.y(); chromaJ++)
							for (int chromaI = chromaIRange.x(); chromaI <= chromaIRange.x(); chromaI++)
							{
								const Interval	chromaA	(calculateAB(subTexelPrecisionBits, chromaU, chromaI));
								const Interval	chromaB	(calculateAB(subTexelPrecisionBits, chromaV, chromaJ));

								const Interval	srcColor[]	=
								{
									linearSample(rAccess, conversionFormat, filteringFormat, addressModeU, addressModeV, IVec2(chromaI, chromaJ), chromaA, chromaB),
									gValue,
									linearSample(bAccess, conversionFormat, filteringFormat, addressModeU, addressModeV, IVec2(chromaI, chromaJ), chromaA, chromaB),
									aValue
								};
								Interval		dstColor[4];

								convertColor(colorModel, range, conversionFormat, bitDepth, srcColor, dstColor);

								for (size_t compNdx = 0; compNdx < 4; compNdx++)
									bounds[compNdx] |= highp.roundOut(dstColor[compNdx], false);
							}
						}
						else
							DE_FATAL("Unknown filter");
					}
				}
				else
				{
					// Linear, no chroma subsampling
					const Interval	srcColor[]	=
					{
						lookupWrapped(rAccess, conversionFormat, addressModeU, addressModeV, IVec2(i, j)),
						gValue,
						lookupWrapped(bAccess, conversionFormat, addressModeU, addressModeV, IVec2(i, j)),
						aValue
					};
					Interval dstColor[4];

					convertColor(colorModel, range, conversionFormat, bitDepth, srcColor, dstColor);

					for (size_t compNdx = 0; compNdx < 4; compNdx++)
						bounds[compNdx] |= highp.roundOut(dstColor[compNdx], false);
				}
			}
		}
		else if (filter == vk::VK_FILTER_LINEAR)
		{
			const IVec2	iRange	(calculateLinearIJRange(coordFormat, u));
			const IVec2	jRange	(calculateLinearIJRange(coordFormat, v));

			ijBounds[ndx][0] = iRange[0];
			ijBounds[ndx][1] = iRange[1];

			ijBounds[ndx][2] = jRange[0];
			ijBounds[ndx][3] = jRange[1];

			for (int j = jRange.x(); j <= jRange.y(); j++)
			for (int i = iRange.x(); i <= iRange.y(); i++)
			{
				const Interval	lumaA		(calculateAB(subTexelPrecisionBits, u, i));
				const Interval	lumaB		(calculateAB(subTexelPrecisionBits, v, j));

				const Interval	gValue		(linearSample(gAccess, conversionFormat, filteringFormat, addressModeU, addressModeV, IVec2(i, j), lumaA, lumaB));
				const Interval	aValue		(linearSample(aAccess, conversionFormat, filteringFormat, addressModeU, addressModeV, IVec2(i, j), lumaA, lumaB));

				if (subsampledX || subsampledY)
				{
					if (explicitReconstruction)
					{
						if (chromaFilter == vk::VK_FILTER_NEAREST)
						{
							const Interval	srcColor[]	=
							{
								linearInterpolate(filteringFormat, lumaA, lumaB,
																lookupWrapped(rAccess, conversionFormat, addressModeU, addressModeV, IVec2(i       / (subsampledX ? 2 : 1), j       / (subsampledY ? 2 : 1))),
																lookupWrapped(rAccess, conversionFormat, addressModeU, addressModeV, IVec2((i + 1) / (subsampledX ? 2 : 1), j       / (subsampledY ? 2 : 1))),
																lookupWrapped(rAccess, conversionFormat, addressModeU, addressModeV, IVec2(i       / (subsampledX ? 2 : 1), (j + 1) / (subsampledY ? 2 : 1))),
																lookupWrapped(rAccess, conversionFormat, addressModeU, addressModeV, IVec2((i + 1) / (subsampledX ? 2 : 1), (j + 1) / (subsampledY ? 2 : 1)))),
								gValue,
								linearInterpolate(filteringFormat, lumaA, lumaB,
																lookupWrapped(bAccess, conversionFormat, addressModeU, addressModeV, IVec2(i       / (subsampledX ? 2 : 1), j       / (subsampledY ? 2 : 1))),
																lookupWrapped(bAccess, conversionFormat, addressModeU, addressModeV, IVec2((i + 1) / (subsampledX ? 2 : 1), j       / (subsampledY ? 2 : 1))),
																lookupWrapped(bAccess, conversionFormat, addressModeU, addressModeV, IVec2(i       / (subsampledX ? 2 : 1), (j + 1) / (subsampledY ? 2 : 1))),
																lookupWrapped(bAccess, conversionFormat, addressModeU, addressModeV, IVec2((i + 1) / (subsampledX ? 2 : 1), (j + 1) / (subsampledY ? 2 : 1)))),
								aValue
							};
							Interval		dstColor[4];

							convertColor(colorModel, range, conversionFormat, bitDepth, srcColor, dstColor);

							for (size_t compNdx = 0; compNdx < 4; compNdx++)
								bounds[compNdx] |= highp.roundOut(dstColor[compNdx], false);
						}
						else if (chromaFilter == vk::VK_FILTER_LINEAR)
						{
							if (subsampledX && subsampledY)
							{
								// Linear, Reconstructed xx chroma samples with explicit linear filtering
								const Interval	rValue	(linearInterpolate(filteringFormat, lumaA, lumaB,
																			reconstructLinearXYChromaSample(filteringFormat, conversionFormat, xChromaOffset, yChromaOffset, addressModeU, addressModeV, rAccess, i, j),
																			reconstructLinearXYChromaSample(filteringFormat, conversionFormat, xChromaOffset, yChromaOffset, addressModeU, addressModeV, rAccess, i + 1, j),
																			reconstructLinearXYChromaSample(filteringFormat, conversionFormat, xChromaOffset, yChromaOffset, addressModeU, addressModeV, rAccess, i , j + 1),
																			reconstructLinearXYChromaSample(filteringFormat, conversionFormat, xChromaOffset, yChromaOffset, addressModeU, addressModeV, rAccess, i + 1, j + 1)));
								const Interval	bValue	(linearInterpolate(filteringFormat, lumaA, lumaB,
																			reconstructLinearXYChromaSample(filteringFormat, conversionFormat, xChromaOffset, yChromaOffset, addressModeU, addressModeV, bAccess, i, j),
																			reconstructLinearXYChromaSample(filteringFormat, conversionFormat, xChromaOffset, yChromaOffset, addressModeU, addressModeV, bAccess, i + 1, j),
																			reconstructLinearXYChromaSample(filteringFormat, conversionFormat, xChromaOffset, yChromaOffset, addressModeU, addressModeV, bAccess, i , j + 1),
																			reconstructLinearXYChromaSample(filteringFormat, conversionFormat, xChromaOffset, yChromaOffset, addressModeU, addressModeV, bAccess, i + 1, j + 1)));
								const Interval	srcColor[]	=
								{
									rValue,
									gValue,
									bValue,
									aValue
								};
								Interval		dstColor[4];

								convertColor(colorModel, range, conversionFormat, bitDepth, srcColor, dstColor);

								for (size_t compNdx = 0; compNdx < 4; compNdx++)
									bounds[compNdx] |= highp.roundOut(dstColor[compNdx], false);

							}
							else if (subsampledX)
							{
								// Linear, Reconstructed x chroma samples with explicit linear filtering
								const Interval	rValue	(linearInterpolate(filteringFormat, lumaA, lumaB,
																			reconstructLinearXChromaSample(filteringFormat, conversionFormat, xChromaOffset, addressModeU, addressModeV, rAccess, i, j),
																			reconstructLinearXChromaSample(filteringFormat, conversionFormat, xChromaOffset, addressModeU, addressModeV, rAccess, i + 1, j),
																			reconstructLinearXChromaSample(filteringFormat, conversionFormat, xChromaOffset, addressModeU, addressModeV, rAccess, i , j + 1),
																			reconstructLinearXChromaSample(filteringFormat, conversionFormat, xChromaOffset, addressModeU, addressModeV, rAccess, i + 1, j + 1)));
								const Interval	bValue	(linearInterpolate(filteringFormat, lumaA, lumaB,
																			reconstructLinearXChromaSample(filteringFormat, conversionFormat, xChromaOffset, addressModeU, addressModeV, bAccess, i, j),
																			reconstructLinearXChromaSample(filteringFormat, conversionFormat, xChromaOffset, addressModeU, addressModeV, bAccess, i + 1, j),
																			reconstructLinearXChromaSample(filteringFormat, conversionFormat, xChromaOffset, addressModeU, addressModeV, bAccess, i , j + 1),
																			reconstructLinearXChromaSample(filteringFormat, conversionFormat, xChromaOffset, addressModeU, addressModeV, bAccess, i + 1, j + 1)));
								const Interval	srcColor[]	=
								{
									rValue,
									gValue,
									bValue,
									aValue
								};
								Interval		dstColor[4];

								convertColor(colorModel, range, conversionFormat, bitDepth, srcColor, dstColor);

								for (size_t compNdx = 0; compNdx < 4; compNdx++)
									bounds[compNdx] |= highp.roundOut(dstColor[compNdx], false);
							}
							else
								DE_FATAL("Unknown subsampling config");
						}
						else
							DE_FATAL("Unknown chroma filter");
					}
					else
					{
						const Interval	chromaU	(subsampledX ? calculateImplicitChromaUV(coordFormat, xChromaOffset, u) : u);
						const Interval	chromaV	(subsampledY ? calculateImplicitChromaUV(coordFormat, yChromaOffset, v) : v);

						if (chromaFilter == vk::VK_FILTER_NEAREST)
						{
							const IVec2	chromaIRange	(calculateNearestIJRange(coordFormat, chromaU));
							const IVec2	chromaJRange	(calculateNearestIJRange(coordFormat, chromaV));

							for (int chromaJ = chromaJRange.x(); chromaJ <= chromaJRange.y(); chromaJ++)
							for (int chromaI = chromaIRange.x(); chromaI <= chromaIRange.x(); chromaI++)
							{
								const Interval	srcColor[]	=
								{
									lookupWrapped(rAccess, conversionFormat, addressModeU, addressModeV, IVec2(chromaI, chromaJ)),
									gValue,
									lookupWrapped(bAccess, conversionFormat, addressModeU, addressModeV, IVec2(chromaI, chromaJ)),
									aValue
								};
								Interval	dstColor[4];

								convertColor(colorModel, range, conversionFormat, bitDepth, srcColor, dstColor);

								for (size_t compNdx = 0; compNdx < 4; compNdx++)
									bounds[compNdx] |= highp.roundOut(dstColor[compNdx], false);
							}
						}
						else if (chromaFilter == vk::VK_FILTER_LINEAR)
						{
							const IVec2	chromaIRange	(calculateNearestIJRange(coordFormat, chromaU));
							const IVec2	chromaJRange	(calculateNearestIJRange(coordFormat, chromaV));

							for (int chromaJ = chromaJRange.x(); chromaJ <= chromaJRange.y(); chromaJ++)
							for (int chromaI = chromaIRange.x(); chromaI <= chromaIRange.x(); chromaI++)
							{
								const Interval	chromaA		(calculateAB(subTexelPrecisionBits, chromaU, chromaI));
								const Interval	chromaB		(calculateAB(subTexelPrecisionBits, chromaV, chromaJ));

								const Interval	rValue		(linearSample(rAccess, conversionFormat, filteringFormat, addressModeU, addressModeV, IVec2(chromaI, chromaJ), chromaA, chromaB));
								const Interval	bValue		(linearSample(bAccess, conversionFormat, filteringFormat, addressModeU, addressModeV, IVec2(chromaI, chromaJ), chromaA, chromaB));

								const Interval	srcColor[]	=
								{
									rValue,
									gValue,
									bValue,
									aValue
								};
								Interval		dstColor[4];
								convertColor(colorModel, range, conversionFormat, bitDepth, srcColor, dstColor);

								for (size_t compNdx = 0; compNdx < 4; compNdx++)
									bounds[compNdx] |= highp.roundOut(dstColor[compNdx], false);
							}
						}
						else
							DE_FATAL("Unknown chroma filter");
					}
				}
				else
				{
					const Interval	chromaA		(lumaA);
					const Interval	chromaB		(lumaB);
					const Interval	rValue		(linearSample(rAccess, conversionFormat, filteringFormat, addressModeU, addressModeV, IVec2(i, j), chromaA, chromaB));
					const Interval	bValue		(linearSample(bAccess, conversionFormat, filteringFormat, addressModeU, addressModeV, IVec2(i, j), chromaA, chromaB));
					const Interval	srcColor[]	=
					{
						rValue,
						gValue,
						bValue,
						aValue
					};
					Interval dstColor[4];

					convertColor(colorModel, range, conversionFormat, bitDepth, srcColor, dstColor);

					for (size_t compNdx = 0; compNdx < 4; compNdx++)
						bounds[compNdx] |= highp.roundOut(dstColor[compNdx], false);
				}
			}
		}
		else
			DE_FATAL("Unknown filter");

		minBounds[ndx] = Vec4((float)bounds[0].lo(), (float)bounds[1].lo(), (float)bounds[2].lo(), (float)bounds[3].lo());
		maxBounds[ndx] = Vec4((float)bounds[0].hi(), (float)bounds[1].hi(), (float)bounds[2].hi(), (float)bounds[3].hi());
	}
}

struct TestConfig
{
	TestConfig	(glu::ShaderType						shaderType_,
				 vk::VkFormat							format_,
				 vk::VkImageTiling						imageTiling_,
				 vk::VkFilter							textureFilter_,
				 vk::VkSamplerAddressMode				addressModeU_,
				 vk::VkSamplerAddressMode				addressModeV_,

				 vk::VkFilter							chromaFilter_,
				 vk::VkChromaLocationKHR				xChromaOffset_,
				 vk::VkChromaLocationKHR				yChromaOffset_,
				 bool									explicitReconstruction_,
				 bool									disjoint_,

				 vk::VkSamplerYcbcrRangeKHR				colorRange_,
				 vk::VkSamplerYcbcrModelConversionKHR	colorModel_,
				 vk::VkComponentMapping					componentMapping_)
		: shaderType				(shaderType_)
		, format					(format_)
		, imageTiling				(imageTiling_)
		, textureFilter				(textureFilter_)
		, addressModeU				(addressModeU_)
		, addressModeV				(addressModeV_)

		, chromaFilter				(chromaFilter_)
		, xChromaOffset				(xChromaOffset_)
		, yChromaOffset				(yChromaOffset_)
		, explicitReconstruction	(explicitReconstruction_)
		, disjoint					(disjoint_)

		, colorRange				(colorRange_)
		, colorModel				(colorModel_)
		, componentMapping			(componentMapping_)
	{
	}

	glu::ShaderType							shaderType;
	vk::VkFormat							format;
	vk::VkImageTiling						imageTiling;
	vk::VkFilter							textureFilter;
	vk::VkSamplerAddressMode				addressModeU;
	vk::VkSamplerAddressMode				addressModeV;

	vk::VkFilter							chromaFilter;
	vk::VkChromaLocationKHR					xChromaOffset;
	vk::VkChromaLocationKHR					yChromaOffset;
	bool									explicitReconstruction;
	bool									disjoint;

	vk::VkSamplerYcbcrRangeKHR				colorRange;
	vk::VkSamplerYcbcrModelConversionKHR	colorModel;
	vk::VkComponentMapping					componentMapping;
};

vk::Move<vk::VkDescriptorSetLayout> createDescriptorSetLayout (const vk::DeviceInterface&	vkd,
															   vk::VkDevice					device,
															   vk::VkSampler				sampler)
{
	const vk::VkDescriptorSetLayoutBinding		layoutBindings[]	=
	{
		{
			0u,
			vk::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1u,
			vk::VK_SHADER_STAGE_ALL,
			&sampler
		}
	};
	const vk::VkDescriptorSetLayoutCreateInfo	layoutCreateInfo	=
	{
		vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		DE_NULL,

		0u,
		DE_LENGTH_OF_ARRAY(layoutBindings),
		layoutBindings
	};

	return vk::createDescriptorSetLayout(vkd, device, &layoutCreateInfo);
}

vk::Move<vk::VkDescriptorPool> createDescriptorPool (const vk::DeviceInterface&	vkd,
													 vk::VkDevice				device)
{
	const vk::VkDescriptorPoolSize			poolSizes[]					=
	{
		{ vk::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, }
	};
	const vk::VkDescriptorPoolCreateInfo	descriptorPoolCreateInfo	=
	{
		vk::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		DE_NULL,
		vk::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,

		1u,
		DE_LENGTH_OF_ARRAY(poolSizes),
		poolSizes
	};

	return createDescriptorPool(vkd, device, &descriptorPoolCreateInfo);
}

vk::Move<vk::VkDescriptorSet> createDescriptorSet (const vk::DeviceInterface&	vkd,
												   vk::VkDevice					device,
												   vk::VkDescriptorPool			descriptorPool,
												   vk::VkDescriptorSetLayout	layout,
												   vk::VkSampler				sampler,
												   vk::VkImageView				imageView)
{
	const vk::VkDescriptorSetAllocateInfo		descriptorSetAllocateInfo	=
	{
		vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		DE_NULL,

		descriptorPool,
		1u,
		&layout
	};
	vk::Move<vk::VkDescriptorSet>	descriptorSet	(vk::allocateDescriptorSet(vkd, device, &descriptorSetAllocateInfo));
	const vk::VkDescriptorImageInfo	imageInfo		=
	{
		sampler,
		imageView,
		vk::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	{
		const vk::VkWriteDescriptorSet	writes[]	=
		{
			{
				vk::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				DE_NULL,

				*descriptorSet,
				0u,
				0u,
				1u,
				vk::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				&imageInfo,
				DE_NULL,
				DE_NULL
			}
		};

		vkd.updateDescriptorSets(device, DE_LENGTH_OF_ARRAY(writes), writes, 0u, DE_NULL);
	}

	return descriptorSet;
}

vk::Move<vk::VkSampler> createSampler (const vk::DeviceInterface&		vkd,
									   vk::VkDevice						device,
									   vk::VkFilter						textureFilter,
									   vk::VkSamplerAddressMode			addressModeU,
									   vk::VkSamplerAddressMode			addressModeV,
									   vk::VkSamplerYcbcrConversionKHR	conversion)
{
#if !defined(FAKE_COLOR_CONVERSION)
	const vk::VkSamplerYcbcrConversionInfoKHR	samplerConversionInfo	=
	{
		vk::VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO_KHR,
		DE_NULL,
		conversion
	};
#else
	DE_UNREF(conversion);
#endif
	const vk::VkSamplerCreateInfo	createInfo	=
	{
		vk::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
#if !defined(FAKE_COLOR_CONVERSION)
		&samplerConversionInfo,
#else
		DE_NULL,
#endif

		0u,
		textureFilter,
		textureFilter,
		vk::VK_SAMPLER_MIPMAP_MODE_NEAREST,
		addressModeU,
		addressModeV,
		vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		0.0f,
		VK_FALSE,
		1.0f,
		VK_FALSE,
		vk::VK_COMPARE_OP_ALWAYS,
		0.0f,
		0.0f,
		vk::VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
		VK_FALSE,
	};

	return createSampler(vkd, device, &createInfo);
}

vk::Move<vk::VkImage> createImage (const vk::DeviceInterface&	vkd,
								   vk::VkDevice					device,
								   vk::VkFormat					format,
								   const UVec2&					size,
								   bool							disjoint,
								   vk::VkImageTiling			tiling)
{
	const vk::VkImageCreateInfo createInfo =
	{
		vk::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		DE_NULL,
		disjoint ? (vk::VkImageCreateFlags)vk::VK_IMAGE_CREATE_DISJOINT_BIT_KHR : (vk::VkImageCreateFlags)0u,

		vk::VK_IMAGE_TYPE_2D,
		format,
		vk::makeExtent3D(size.x(), size.y(), 1u),
		1u,
		1u,
		vk::VK_SAMPLE_COUNT_1_BIT,
		tiling,
		vk::VK_IMAGE_USAGE_TRANSFER_DST_BIT | vk::VK_IMAGE_USAGE_SAMPLED_BIT,
		vk::VK_SHARING_MODE_EXCLUSIVE,
		0u,
		(const deUint32*)DE_NULL,
		vk::VK_IMAGE_LAYOUT_UNDEFINED,
	};

	return vk::createImage(vkd, device, &createInfo);
}

vk::Move<vk::VkImageView> createImageView (const vk::DeviceInterface&		vkd,
										   vk::VkDevice						device,
										   vk::VkImage						image,
										   vk::VkFormat						format,
										   vk::VkSamplerYcbcrConversionKHR	conversion)
{
#if !defined(FAKE_COLOR_CONVERSION)
	const vk::VkSamplerYcbcrConversionInfoKHR	conversionInfo	=
	{
		vk::VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO_KHR,
		DE_NULL,
		conversion
	};
#else
	DE_UNREF(conversion);
#endif
	const vk::VkImageViewCreateInfo				viewInfo		=
	{
		vk::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
#if defined(FAKE_COLOR_CONVERSION)
		DE_NULL,
#else
		&conversionInfo,
#endif
		(vk::VkImageViewCreateFlags)0,

		image,
		vk::VK_IMAGE_VIEW_TYPE_2D,
		format,
		{
			vk::VK_COMPONENT_SWIZZLE_IDENTITY,
			vk::VK_COMPONENT_SWIZZLE_IDENTITY,
			vk::VK_COMPONENT_SWIZZLE_IDENTITY,
			vk::VK_COMPONENT_SWIZZLE_IDENTITY,
		},
		{ vk::VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u },
	};

	return vk::createImageView(vkd, device, &viewInfo);
}

vk::Move<vk::VkSamplerYcbcrConversionKHR> createConversion (const vk::DeviceInterface&				vkd,
															vk::VkDevice							device,
															vk::VkFormat							format,
															vk::VkSamplerYcbcrModelConversionKHR	colorModel,
															vk::VkSamplerYcbcrRangeKHR				colorRange,
															vk::VkChromaLocationKHR					xChromaOffset,
															vk::VkChromaLocationKHR					yChromaOffset,
															vk::VkFilter							chromaFilter,
															const vk::VkComponentMapping&			componentMapping,
															bool									explicitReconstruction)
{
	const vk::VkSamplerYcbcrConversionCreateInfoKHR	conversionInfo	=
	{
		vk::VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO_KHR,
		DE_NULL,

		format,
		colorModel,
		colorRange,
		componentMapping,
		xChromaOffset,
		yChromaOffset,
		chromaFilter,
		explicitReconstruction ? VK_TRUE : VK_FALSE
	};

	return vk::createSamplerYcbcrConversionKHR(vkd, device, &conversionInfo);
}

void evalShader (Context&								context,
				 glu::ShaderType						shaderType,
				 const MultiPlaneImageData&				imageData,
				 const UVec2&							size,
				 vk::VkFormat							format,
				 vk::VkImageTiling						imageTiling,
				 bool									disjoint,
				 vk::VkFilter							textureFilter,
				 vk::VkSamplerAddressMode				addressModeU,
				 vk::VkSamplerAddressMode				addressModeV,
				 vk::VkSamplerYcbcrModelConversionKHR	colorModel,
				 vk::VkSamplerYcbcrRangeKHR				colorRange,
				 vk::VkChromaLocationKHR				xChromaOffset,
				 vk::VkChromaLocationKHR				yChromaOffset,
				 vk::VkFilter							chromaFilter,
				 const vk::VkComponentMapping&			componentMapping,
				 bool									explicitReconstruction,
				 const vector<Vec2>&					sts,
				 vector<Vec4>&							results)
{
	const vk::DeviceInterface&							vkd					(context.getDeviceInterface());
	const vk::VkDevice									device				(context.getDevice());
#if !defined(FAKE_COLOR_CONVERSION)
	const vk::Unique<vk::VkSamplerYcbcrConversionKHR>	conversion			(createConversion(vkd, device, format, colorModel, colorRange, xChromaOffset, yChromaOffset, chromaFilter, componentMapping, explicitReconstruction));
	const vk::Unique<vk::VkSampler>						sampler				(createSampler(vkd, device, textureFilter, addressModeU, addressModeV, *conversion));
#else
	DE_UNREF(colorModel);
	DE_UNREF(colorRange);
	DE_UNREF(xChromaOffset);
	DE_UNREF(yChromaOffset);
	DE_UNREF(chromaFilter);
	DE_UNREF(explicitReconstruction);
	DE_UNREF(componentMapping);
	DE_UNREF(createConversion);
	const vk::Unique<vk::VkSampler>						sampler				(createSampler(vkd, device, textureFilter, addressModeU, addressModeV, (vk::VkSamplerYcbcrConversionKHR)0u));
#endif
	const vk::Unique<vk::VkImage>						image				(createImage(vkd, device, format, size, disjoint, imageTiling));
	const vk::MemoryRequirement							memoryRequirement	(imageTiling == vk::VK_IMAGE_TILING_OPTIMAL
																			? vk::MemoryRequirement::Any
																			: vk::MemoryRequirement::HostVisible);
	const vk::VkImageCreateFlags						createFlags			(disjoint ? vk::VK_IMAGE_CREATE_DISJOINT_BIT_KHR : (vk::VkImageCreateFlagBits)0u);
	const vector<AllocationSp>							imageMemory			(allocateAndBindImageMemory(vkd, device, context.getDefaultAllocator(), *image, format, createFlags, memoryRequirement));
#if defined(FAKE_COLOR_CONVERSION)
	const vk::Unique<vk::VkImageView>					imageView			(createImageView(vkd, device, *image, format, (vk::VkSamplerYcbcrConversionKHR)0));
#else
	const vk::Unique<vk::VkImageView>					imageView			(createImageView(vkd, device, *image, format, *conversion));
#endif

	const vk::Unique<vk::VkDescriptorSetLayout>			layout				(createDescriptorSetLayout(vkd, device, *sampler));
	const vk::Unique<vk::VkDescriptorPool>				descriptorPool		(createDescriptorPool(vkd, device));
	const vk::Unique<vk::VkDescriptorSet>				descriptorSet		(createDescriptorSet(vkd, device, *descriptorPool, *layout, *sampler, *imageView));

	const ShaderSpec									spec				(createShaderSpec());
	const de::UniquePtr<ShaderExecutor>					executor			(createExecutor(context, shaderType, spec, *layout));

	if (imageTiling == vk::VK_IMAGE_TILING_OPTIMAL)
		uploadImage(vkd, device, context.getUniversalQueueFamilyIndex(), context.getDefaultAllocator(), *image, imageData, vk::VK_ACCESS_SHADER_READ_BIT, vk::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	else
		fillImageMemory(vkd, device, context.getUniversalQueueFamilyIndex(), *image, imageMemory, imageData, vk::VK_ACCESS_SHADER_READ_BIT, vk::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	results.resize(sts.size());

	{
		const void* const	inputs[]	=
		{
			&sts[0]
		};
		void* const			outputs[]	=
		{
			&results[0]
		};

		executor->execute((int)sts.size(), inputs, outputs, *descriptorSet);
	}
}

bool isXChromaSubsampled (vk::VkFormat format)
{
	switch (format)
	{
		case vk::VK_FORMAT_G8B8G8R8_422_UNORM_KHR:
		case vk::VK_FORMAT_B8G8R8G8_422_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G16B16G16R16_422_UNORM_KHR:
		case vk::VK_FORMAT_B16G16R16G16_422_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR:
			return true;

		default:
			return false;
	}
}

bool isYChromaSubsampled (vk::VkFormat format)
{
	switch (format)
	{
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR:
			return true;

		default:
			return false;
	}
}

void logTestCaseInfo (TestLog& log, const TestConfig& config)
{
	log << TestLog::Message << "ShaderType: " << config.shaderType << TestLog::EndMessage;
	log << TestLog::Message << "Format: "  << config.format << TestLog::EndMessage;
	log << TestLog::Message << "ImageTiling: " << config.imageTiling << TestLog::EndMessage;
	log << TestLog::Message << "TextureFilter: " << config.textureFilter << TestLog::EndMessage;
	log << TestLog::Message << "AddressModeU: " << config.addressModeU << TestLog::EndMessage;
	log << TestLog::Message << "AddressModeV: " << config.addressModeV << TestLog::EndMessage;
	log << TestLog::Message << "ChromaFilter: " << config.chromaFilter << TestLog::EndMessage;
	log << TestLog::Message << "XChromaOffset: " << config.xChromaOffset << TestLog::EndMessage;
	log << TestLog::Message << "YChromaOffset: " << config.yChromaOffset << TestLog::EndMessage;
	log << TestLog::Message << "ExplicitReconstruction: " << (config.explicitReconstruction ? "true" : "false") << TestLog::EndMessage;
	log << TestLog::Message << "Disjoint: " << (config.disjoint ? "true" : "false") << TestLog::EndMessage;
	log << TestLog::Message << "ColorRange: " << config.colorRange << TestLog::EndMessage;
	log << TestLog::Message << "ColorModel: " << config.colorModel << TestLog::EndMessage;
	log << TestLog::Message << "ComponentMapping: " << config.componentMapping << TestLog::EndMessage;
}


tcu::TestStatus textureConversionTest (Context& context, const TestConfig config)
{
	const FloatFormat	filteringPrecision		(getFilteringPrecision(config.format));
	const FloatFormat	conversionPrecision		(getConversionPrecision(config.format));
	const deUint32		subTexelPrecisionBits	(vk::getPhysicalDeviceProperties(context.getInstanceInterface(), context.getPhysicalDevice()).limits.subTexelPrecisionBits);
	const tcu::UVec4	bitDepth				(getBitDepth(config.format));
	const UVec2			size					(isXChromaSubsampled(config.format) ? 12 : 7,
												 isYChromaSubsampled(config.format) ?  8 : 13);
	TestLog&			log						(context.getTestContext().getLog());
	bool				explicitReconstruction	= config.explicitReconstruction;
	bool				isOk					= true;

	logTestCaseInfo(log, config);

#if !defined(FAKE_COLOR_CONVERSION)
	if (!de::contains(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), string("VK_KHR_sampler_ycbcr_conversion")))
		TCU_THROW(NotSupportedError, "Extension VK_KHR_sampler_ycbcr_conversion not supported");

	try
	{
		const vk::VkFormatProperties	properties	(vk::getPhysicalDeviceFormatProperties(context.getInstanceInterface(), context.getPhysicalDevice(), config.format));
		const vk::VkFormatFeatureFlags	features	(config.imageTiling == vk::VK_IMAGE_TILING_OPTIMAL
													? properties.optimalTilingFeatures
													: properties.linearTilingFeatures);

		if ((features & (vk::VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT_KHR | vk::VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT_KHR)) == 0)
			TCU_THROW(NotSupportedError, "Format doesn't support YCbCr conversions");

		if ((features & vk::VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) == 0)
			TCU_THROW(NotSupportedError, "Format doesn't support sampling");

		if (config.textureFilter == vk::VK_FILTER_LINEAR && ((features & vk::VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT_KHR) == 0))
			TCU_THROW(NotSupportedError, "Format doesn't support YCbCr linear chroma reconstruction");

		if (config.chromaFilter == vk::VK_FILTER_LINEAR && ((features & vk::VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT_KHR) == 0))
			TCU_THROW(NotSupportedError, "Format doesn't support YCbCr linear chroma reconstruction");

		if (config.chromaFilter != config.textureFilter && ((features & vk::VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_SEPARATE_RECONSTRUCTION_FILTER_BIT_KHR) == 0))
			TCU_THROW(NotSupportedError, "Format doesn't support different chroma and texture filters");

		if (config.explicitReconstruction && ((features & vk::VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_FORCEABLE_BIT_KHR) == 0))
			TCU_THROW(NotSupportedError, "Format doesn't support explicit chroma reconstruction");

		if (config.disjoint && ((features & vk::VK_FORMAT_FEATURE_DISJOINT_BIT_KHR) == 0))
			TCU_THROW(NotSupportedError, "Format doesn't disjoint planes");

		if (isXChromaSubsampled(config.format) && (config.xChromaOffset == vk::VK_CHROMA_LOCATION_COSITED_EVEN_KHR) && ((features & vk::VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT_KHR) == 0))
			TCU_THROW(NotSupportedError, "Format doesn't support cosited chroma samples");

		if (isXChromaSubsampled(config.format) && (config.xChromaOffset == vk::VK_CHROMA_LOCATION_MIDPOINT_KHR) && ((features & vk::VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT_KHR) == 0))
			TCU_THROW(NotSupportedError, "Format doesn't support midpoint chroma samples");

		if (isYChromaSubsampled(config.format) && (config.yChromaOffset == vk::VK_CHROMA_LOCATION_COSITED_EVEN_KHR) && ((features & vk::VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT_KHR) == 0))
			TCU_THROW(NotSupportedError, "Format doesn't support cosited chroma samples");

		if (isYChromaSubsampled(config.format) && (config.yChromaOffset == vk::VK_CHROMA_LOCATION_MIDPOINT_KHR) && ((features & vk::VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT_KHR) == 0))
			TCU_THROW(NotSupportedError, "Format doesn't support midpoint chroma samples");

		if ((features & vk::VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_BIT_KHR) != 0)
			explicitReconstruction = true;

		log << TestLog::Message << "FormatFeatures: " << vk::getFormatFeatureFlagsStr(features) << TestLog::EndMessage;
	}
	catch (const vk::Error& err)
	{
		if (err.getError() == vk::VK_ERROR_FORMAT_NOT_SUPPORTED)
			TCU_THROW(NotSupportedError, "Format not supported");

		throw;
	}
#endif

	{
		const vk::PlanarFormatDescription	planeInfo			(vk::getPlanarFormatDescription(config.format));
		MultiPlaneImageData					src					(config.format, size);

		deUint32							nullAccessData		(0u);
		ChannelAccess						nullAccess			(tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT, 1u, IVec3(size.x(), size.y(), 1), IVec3(0, 0, 0), &nullAccessData, 0u);
		deUint32							nullAccessAlphaData	(~0u);
		ChannelAccess						nullAccessAlpha		(tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT, 1u, IVec3(size.x(), size.y(), 1), IVec3(0, 0, 0), &nullAccessAlphaData, 0u);
		ChannelAccess						rChannelAccess		(planeInfo.hasChannelNdx(0) ? getChannelAccess(src, planeInfo, size, 0) : nullAccess);
		ChannelAccess						gChannelAccess		(planeInfo.hasChannelNdx(1) ? getChannelAccess(src, planeInfo, size, 1) : nullAccess);
		ChannelAccess						bChannelAccess		(planeInfo.hasChannelNdx(2) ? getChannelAccess(src, planeInfo, size, 2) : nullAccess);
		ChannelAccess						aChannelAccess		(planeInfo.hasChannelNdx(3) ? getChannelAccess(src, planeInfo, size, 3) : nullAccessAlpha);

		vector<Vec2>						sts;
		vector<Vec4>						results;
		vector<Vec4>						minBounds;
		vector<Vec4>						maxBounds;
		vector<Vec4>						uvBounds;
		vector<IVec4>						ijBounds;

		for (deUint32 planeNdx = 0; planeNdx < planeInfo.numPlanes; planeNdx++)
			deMemset(src.getPlanePtr(planeNdx), 0u, src.getPlaneSize(planeNdx));

		// \todo Limit values to only values that produce defined values using selected colorRange and colorModel? The verification code handles those cases already correctly.
		if (planeInfo.hasChannelNdx(0))
		{
			for (int y = 0; y < rChannelAccess.getSize().y(); y++)
			for (int x = 0; x < rChannelAccess.getSize().x(); x++)
				rChannelAccess.setChannel(IVec3(x, y, 0), (float)x / (float)rChannelAccess.getSize().x());
		}

		if (planeInfo.hasChannelNdx(1))
		{
			for (int y = 0; y < gChannelAccess.getSize().y(); y++)
			for (int x = 0; x < gChannelAccess.getSize().x(); x++)
				gChannelAccess.setChannel(IVec3(x, y, 0), (float)y / (float)gChannelAccess.getSize().y());
		}

		if (planeInfo.hasChannelNdx(2))
		{
			for (int y = 0; y < bChannelAccess.getSize().y(); y++)
			for (int x = 0; x < bChannelAccess.getSize().x(); x++)
				bChannelAccess.setChannel(IVec3(x, y, 0), (float)(x + y) / (float)(bChannelAccess.getSize().x() + bChannelAccess.getSize().y()));
		}

		if (planeInfo.hasChannelNdx(3))
		{
			for (int y = 0; y < aChannelAccess.getSize().y(); y++)
			for (int x = 0; x < aChannelAccess.getSize().x(); x++)
				aChannelAccess.setChannel(IVec3(x, y, 0), (float)(x * y) / (float)(aChannelAccess.getSize().x() * aChannelAccess.getSize().y()));
		}

		genTexCoords(sts, size);

		calculateBounds(rChannelAccess, gChannelAccess, bChannelAccess, aChannelAccess, bitDepth, sts, filteringPrecision, conversionPrecision, subTexelPrecisionBits, config.textureFilter, config.colorModel, config.colorRange, config.chromaFilter, config.xChromaOffset, config.yChromaOffset, config.componentMapping, explicitReconstruction, config.addressModeU, config.addressModeV, minBounds, maxBounds, uvBounds, ijBounds);

		if (vk::isYCbCrFormat(config.format))
		{
			tcu::TextureLevel	rImage	(tcu::TextureFormat(tcu::TextureFormat::R, tcu::TextureFormat::FLOAT), rChannelAccess.getSize().x(), rChannelAccess.getSize().y());
			tcu::TextureLevel	gImage	(tcu::TextureFormat(tcu::TextureFormat::R, tcu::TextureFormat::FLOAT), gChannelAccess.getSize().x(), gChannelAccess.getSize().y());
			tcu::TextureLevel	bImage	(tcu::TextureFormat(tcu::TextureFormat::R, tcu::TextureFormat::FLOAT), bChannelAccess.getSize().x(), bChannelAccess.getSize().y());
			tcu::TextureLevel	aImage	(tcu::TextureFormat(tcu::TextureFormat::R, tcu::TextureFormat::FLOAT), aChannelAccess.getSize().x(), aChannelAccess.getSize().y());

			for (int y = 0; y < (int)rChannelAccess.getSize().y(); y++)
			for (int x = 0; x < (int)rChannelAccess.getSize().x(); x++)
				rImage.getAccess().setPixel(Vec4(rChannelAccess.getChannel(IVec3(x, y, 0))), x, y);

			for (int y = 0; y < (int)gChannelAccess.getSize().y(); y++)
			for (int x = 0; x < (int)gChannelAccess.getSize().x(); x++)
				gImage.getAccess().setPixel(Vec4(gChannelAccess.getChannel(IVec3(x, y, 0))), x, y);

			for (int y = 0; y < (int)bChannelAccess.getSize().y(); y++)
			for (int x = 0; x < (int)bChannelAccess.getSize().x(); x++)
				bImage.getAccess().setPixel(Vec4(bChannelAccess.getChannel(IVec3(x, y, 0))), x, y);

			for (int y = 0; y < (int)aChannelAccess.getSize().y(); y++)
			for (int x = 0; x < (int)aChannelAccess.getSize().x(); x++)
				aImage.getAccess().setPixel(Vec4(aChannelAccess.getChannel(IVec3(x, y, 0))), x, y);

			{
				const Vec4	scale	(1.0f);
				const Vec4	bias	(0.0f);

				log << TestLog::Image("SourceImageR", "SourceImageR", rImage.getAccess(), scale, bias);
				log << TestLog::Image("SourceImageG", "SourceImageG", gImage.getAccess(), scale, bias);
				log << TestLog::Image("SourceImageB", "SourceImageB", bImage.getAccess(), scale, bias);
				log << TestLog::Image("SourceImageA", "SourceImageA", aImage.getAccess(), scale, bias);
			}
		}
		else
		{
			tcu::TextureLevel	srcImage	(vk::mapVkFormat(config.format), size.x(), size.y());

			for (int y = 0; y < (int)size.y(); y++)
			for (int x = 0; x < (int)size.x(); x++)
			{
				const IVec3 pos (x, y, 0);
				srcImage.getAccess().setPixel(Vec4(rChannelAccess.getChannel(pos), gChannelAccess.getChannel(pos), bChannelAccess.getChannel(pos), aChannelAccess.getChannel(pos)), x, y);
			}

			log << TestLog::Image("SourceImage", "SourceImage", srcImage.getAccess());
		}

		evalShader(context, config.shaderType, src, size, config.format, config.imageTiling, config.disjoint, config.textureFilter, config.addressModeU, config.addressModeV, config.colorModel, config.colorRange, config.xChromaOffset, config.yChromaOffset, config.chromaFilter, config.componentMapping, config.explicitReconstruction, sts, results);

		{
			tcu::TextureLevel	minImage	(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::FLOAT), size.x() + (size.x() / 2), size.y() + (size.y() / 2));
			tcu::TextureLevel	maxImage	(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::FLOAT), size.x() + (size.x() / 2), size.y() + (size.y() / 2));
			tcu::TextureLevel	resImage	(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::FLOAT), size.x() + (size.x() / 2), size.y() + (size.y() / 2));

			for (int y = 0; y < (int)(size.y() + (size.y() / 2)); y++)
			for (int x = 0; x < (int)(size.x() + (size.x() / 2)); x++)
			{
				const int ndx = x + y * (int)(size.x() + (size.x() / 2));
				minImage.getAccess().setPixel(minBounds[ndx], x, y);
				maxImage.getAccess().setPixel(maxBounds[ndx], x, y);
			}

			for (int y = 0; y < (int)(size.y() + (size.y() / 2)); y++)
			for (int x = 0; x < (int)(size.x() + (size.x() / 2)); x++)
			{
				const int ndx = x + y * (int)(size.x() + (size.x() / 2));
				resImage.getAccess().setPixel(results[ndx], x, y);
			}

			{
				const Vec4	scale	(1.0f);
				const Vec4	bias	(0.0f);

				log << TestLog::Image("MinBoundImage", "MinBoundImage", minImage.getAccess(), scale, bias);
				log << TestLog::Image("MaxBoundImage", "MaxBoundImage", maxImage.getAccess(), scale, bias);
				log << TestLog::Image("ResultImage", "ResultImage", resImage.getAccess(), scale, bias);
			}
		}

		size_t errorCount = 0;

		for (size_t ndx = 0; ndx < sts.size(); ndx++)
		{
			if (tcu::boolAny(tcu::lessThan(results[ndx], minBounds[ndx])) || tcu::boolAny(tcu::greaterThan(results[ndx], maxBounds[ndx])))
			{
				log << TestLog::Message << "Fail: " << sts[ndx] << " " << results[ndx] << TestLog::EndMessage;
				log << TestLog::Message << "  Min : " << minBounds[ndx] << TestLog::EndMessage;
				log << TestLog::Message << "  Max : " << maxBounds[ndx] << TestLog::EndMessage;
				log << TestLog::Message << "  Threshold: " << (maxBounds[ndx] - minBounds[ndx]) << TestLog::EndMessage;
				log << TestLog::Message << "  UMin : " << uvBounds[ndx][0] << TestLog::EndMessage;
				log << TestLog::Message << "  UMax : " << uvBounds[ndx][1] << TestLog::EndMessage;
				log << TestLog::Message << "  VMin : " << uvBounds[ndx][2] << TestLog::EndMessage;
				log << TestLog::Message << "  VMax : " << uvBounds[ndx][3] << TestLog::EndMessage;
				log << TestLog::Message << "  IMin : " << ijBounds[ndx][0] << TestLog::EndMessage;
				log << TestLog::Message << "  IMax : " << ijBounds[ndx][1] << TestLog::EndMessage;
				log << TestLog::Message << "  JMin : " << ijBounds[ndx][2] << TestLog::EndMessage;
				log << TestLog::Message << "  JMax : " << ijBounds[ndx][3] << TestLog::EndMessage;

				if (isXChromaSubsampled(config.format))
				{
					log << TestLog::Message << "  LumaAlphaValues : " << TestLog::EndMessage;
					log << TestLog::Message << "    Offset : (" << ijBounds[ndx][0] << ", " << ijBounds[ndx][2] << ")" << TestLog::EndMessage;

					for (deInt32 j = ijBounds[ndx][2]; j <= ijBounds[ndx][3] + (config.textureFilter == vk::VK_FILTER_LINEAR ? 1 : 0); j++)
					{
						const deInt32		wrappedJ	= wrap(config.addressModeV, j, gChannelAccess.getSize().y());
						bool				first		= true;
						std::ostringstream	line;

						for (deInt32 i = ijBounds[ndx][0]; i <= ijBounds[ndx][1] + (config.textureFilter == vk::VK_FILTER_LINEAR ? 1 : 0); i++)
						{
							const deInt32	wrappedI	= wrap(config.addressModeU, i, gChannelAccess.getSize().x());

							if (!first)
							{
								line << ", ";
								first = false;
							}

							line << "(" << std::setfill(' ') << std::setw(5) << gChannelAccess.getChannelUint(IVec3(wrappedI, wrappedJ, 0))
								<< ", " << std::setfill(' ') << std::setw(5) << aChannelAccess.getChannelUint(IVec3(wrappedI, wrappedJ, 0)) << ")";
						}
						log << TestLog::Message << "    " << line.str() << TestLog::EndMessage;
					}

					{
						const IVec2 chromaIRange	(divFloor(ijBounds[ndx][0], 2) - 1, divFloor(ijBounds[ndx][1] + (config.textureFilter == vk::VK_FILTER_LINEAR ? 1 : 0), 2) + 1);
						const IVec2 chromaJRange	(isYChromaSubsampled(config.format)
													? IVec2(divFloor(ijBounds[ndx][2], 2) - 1, divFloor(ijBounds[ndx][3] + (config.textureFilter == vk::VK_FILTER_LINEAR ? 1 : 0), 2) + 1)
													: IVec2(ijBounds[ndx][2], ijBounds[ndx][3] + (config.textureFilter == vk::VK_FILTER_LINEAR ? 1 : 0)));

						log << TestLog::Message << "  ChromaValues : " << TestLog::EndMessage;
						log << TestLog::Message << "    Offset : (" << chromaIRange[0] << ", " << chromaJRange[0] << ")" << TestLog::EndMessage;

						for (deInt32 j = chromaJRange[0]; j <= chromaJRange[1]; j++)
						{
							const deInt32		wrappedJ	= wrap(config.addressModeV, j, rChannelAccess.getSize().y());
							bool				first		= true;
							std::ostringstream	line;

							for (deInt32 i = chromaIRange[0]; i <= chromaIRange[1]; i++)
							{
								const deInt32	wrappedI	= wrap(config.addressModeU, i, rChannelAccess.getSize().x());

								if (!first)
								{
									line << ", ";
									first = false;
								}

								line << "(" << std::setfill(' ') << std::setw(5) << rChannelAccess.getChannelUint(IVec3(wrappedI, wrappedJ, 0))
									<< ", " << std::setfill(' ') << std::setw(5) << bChannelAccess.getChannelUint(IVec3(wrappedI, wrappedJ, 0)) << ")";
							}
							log << TestLog::Message << "    " << line.str() << TestLog::EndMessage;
						}
					}
				}
				else
				{
					log << TestLog::Message << "  Values : " << TestLog::EndMessage;
					log << TestLog::Message << "    Offset : (" << ijBounds[ndx][0] << ", " << ijBounds[ndx][2] << ")" << TestLog::EndMessage;

					for (deInt32 j = ijBounds[ndx][2]; j <= ijBounds[ndx][3] + (config.textureFilter == vk::VK_FILTER_LINEAR ? 1 : 0); j++)
					{
						const deInt32		wrappedJ	= wrap(config.addressModeV, j, rChannelAccess.getSize().y());
						bool				first		= true;
						std::ostringstream	line;

						for (deInt32 i = ijBounds[ndx][0]; i <= ijBounds[ndx][1] + (config.textureFilter == vk::VK_FILTER_LINEAR ? 1 : 0); i++)
						{
							const deInt32	wrappedI	= wrap(config.addressModeU, i, rChannelAccess.getSize().x());

							if (!first)
							{
								line << ", ";
								first = false;
							}

							line << "(" << std::setfill(' ') << std::setw(5) << rChannelAccess.getChannelUint(IVec3(wrappedI, wrappedJ, 0))
								<< ", " << std::setfill(' ') << std::setw(5) << gChannelAccess.getChannelUint(IVec3(wrappedI, wrappedJ, 0))
								<< ", " << std::setfill(' ') << std::setw(5) << bChannelAccess.getChannelUint(IVec3(wrappedI, wrappedJ, 0))
								<< ", " << std::setfill(' ') << std::setw(5) << aChannelAccess.getChannelUint(IVec3(wrappedI, wrappedJ, 0)) << ")";
						}
						log << TestLog::Message << "    " << line.str() << TestLog::EndMessage;
					}
				}

				errorCount++;
				isOk = false;

				if (errorCount > 30)
				{
					log << TestLog::Message << "Encountered " << errorCount << " errors. Omitting rest of the per result logs." << TestLog::EndMessage;
					break;
				}
			}
		}
	}

	if (isOk)
		return tcu::TestStatus::pass("Pass");
	else
		return tcu::TestStatus::fail("Result comparison failed");
}

#if defined(FAKE_COLOR_CONVERSION)
const char* swizzleToCompName (const char* identity, vk::VkComponentSwizzle swizzle)
{
	switch (swizzle)
	{
		case vk::VK_COMPONENT_SWIZZLE_IDENTITY:	return identity;
		case vk::VK_COMPONENT_SWIZZLE_R:		return "r";
		case vk::VK_COMPONENT_SWIZZLE_G:		return "g";
		case vk::VK_COMPONENT_SWIZZLE_B:		return "b";
		case vk::VK_COMPONENT_SWIZZLE_A:		return "a";
		default:
			DE_FATAL("Unsupported swizzle");
			return DE_NULL;
	}
}
#endif

void createTestShaders (vk::SourceCollections& dst, TestConfig config)
{
#if !defined(FAKE_COLOR_CONVERSION)
	const ShaderSpec spec (createShaderSpec());

	generateSources(config.shaderType, spec, dst);
#else
	const UVec4	bits	(getBitDepth(config.format));
	ShaderSpec	spec;

	spec.globalDeclarations = "layout(set=" + de::toString((int)EXTRA_RESOURCES_DESCRIPTOR_SET_INDEX) + ", binding=0) uniform highp sampler2D u_sampler;";

	spec.inputs.push_back(Symbol("uv", glu::VarType(glu::TYPE_FLOAT_VEC2, glu::PRECISION_HIGHP)));
	spec.outputs.push_back(Symbol("o_color", glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP)));

	std::ostringstream	source;

	source << "highp vec4 inputColor = texture(u_sampler, uv);\n";

	source << "highp float r = inputColor." << swizzleToCompName("r", config.componentMapping.r) << ";\n";
	source << "highp float g = inputColor." << swizzleToCompName("g", config.componentMapping.g) << ";\n";
	source << "highp float b = inputColor." << swizzleToCompName("b", config.componentMapping.b) << ";\n";
	source << "highp float a = inputColor." << swizzleToCompName("a", config.componentMapping.a) << ";\n";

	switch (config.colorRange)
	{
		case vk::VK_SAMPLER_YCBCR_RANGE_ITU_FULL_KHR:
			source << "highp float cr = r - (float(" << (0x1u << (bits[0] - 0x1u)) << ") / float(" << ((0x1u << bits[0]) - 1u) << "));\n";
			source << "highp float y  = g;\n";
			source << "highp float cb = b - (float(" << (0x1u << (bits[2] - 0x1u)) << ") / float(" << ((0x1u << bits[2]) - 1u) << "));\n";
			break;

		case vk::VK_SAMPLER_YCBCR_RANGE_ITU_NARROW_KHR:
			source << "highp float cr = (r * float(" << ((0x1u << bits[0]) - 1u) << ") - float(" << (128u * (0x1u << (bits[0] - 8))) << ")) / float(" << (224u * (0x1u << (bits[0] - 8))) << ");\n";
			source << "highp float y  = (g * float(" << ((0x1u << bits[1]) - 1u) << ") - float(" << (16u * (0x1u << (bits[1] - 8))) << ")) / float(" << (219u * (0x1u << (bits[1] - 8))) << ");\n";
			source << "highp float cb = (b * float(" << ((0x1u << bits[2]) - 1u) << ") - float(" << (128u * (0x1u << (bits[2] - 8))) << ")) / float(" << (224u * (0x1u << (bits[2] - 8))) << ");\n";
			break;

		default:
			DE_FATAL("Unknown color range");
	}

	source << "highp vec4 color;\n";

	switch (config.colorModel)
	{
		case vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY_KHR:
			source << "color = vec4(r, g, b, a);\n";
			break;

		case vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_IDENTITY_KHR:
			source << "color = vec4(cr, y, cb, a);\n";
			break;

		case vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601_KHR:
			source << "color = vec4(y + 1.402 * cr, y - float(" << (0.202008 / 0.587) << ") * cb - float(" << (0.419198 / 0.587) << ") * cr, y + 1.772 * cb, a);\n";
			break;

		case vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709_KHR:
			source << "color = vec4(y + 1.5748 * cr, y - float(" << (0.13397432 / 0.7152) << ") * cb - float(" << (0.33480248 / 0.7152) << ") * cr, y + 1.8556 * cb, a);\n";
			break;

		case vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020_KHR:
			source << "color = vec4(y + 1.4746 * cr, (y - float(" << (0.11156702 / 0.6780) << ") * cb) - float(" << (0.38737742 / 0.6780) << ") * cr, y + 1.8814 * cb, a);\n";
			break;

		default:
			DE_FATAL("Unknown color model");
	};

	source << "o_color = color;\n";

	spec.source = source.str();
	generateSources(config.shaderType, spec, dst);
#endif
}

deUint32 getFormatChannelCount (vk::VkFormat format)
{
	switch (format)
	{
		case vk::VK_FORMAT_A1R5G5B5_UNORM_PACK16:
		case vk::VK_FORMAT_A2B10G10R10_UNORM_PACK32:
		case vk::VK_FORMAT_A2R10G10B10_UNORM_PACK32:
		case vk::VK_FORMAT_A8B8G8R8_UNORM_PACK32:
		case vk::VK_FORMAT_B4G4R4A4_UNORM_PACK16:
		case vk::VK_FORMAT_B5G5R5A1_UNORM_PACK16:
		case vk::VK_FORMAT_B8G8R8A8_UNORM:
		case vk::VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_R16G16B16A16_UNORM:
		case vk::VK_FORMAT_R4G4B4A4_UNORM_PACK16:
		case vk::VK_FORMAT_R5G5B5A1_UNORM_PACK16:
		case vk::VK_FORMAT_R8G8B8A8_UNORM:
			return 4;

		case vk::VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_B16G16R16G16_422_UNORM_KHR:
		case vk::VK_FORMAT_B5G6R5_UNORM_PACK16:
		case vk::VK_FORMAT_B8G8R8G8_422_UNORM_KHR:
		case vk::VK_FORMAT_B8G8R8_UNORM:
		case vk::VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G16B16G16R16_422_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR:
		case vk::VK_FORMAT_G8B8G8R8_422_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR:
		case vk::VK_FORMAT_R16G16B16_UNORM:
		case vk::VK_FORMAT_R5G6B5_UNORM_PACK16:
		case vk::VK_FORMAT_R8G8B8_UNORM:
			return 3;

		case vk::VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR:
		case vk::VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR:
			return 2;

		case vk::VK_FORMAT_R10X6_UNORM_PACK16_KHR:
		case vk::VK_FORMAT_R12X4_UNORM_PACK16_KHR:
			return 1;

		default:
			DE_FATAL("Unknown number of channels");
			return -1;
	}
}

struct RangeNamePair
{
	const char*					name;
	vk::VkSamplerYcbcrRangeKHR	value;
};


struct ChromaLocationNamePair
{
	const char*				name;
	vk::VkChromaLocationKHR	value;
};

void initTests (tcu::TestCaseGroup* testGroup)
{
	const vk::VkFormat noChromaSubsampledFormats[] =
	{
		vk::VK_FORMAT_R4G4B4A4_UNORM_PACK16,
		vk::VK_FORMAT_B4G4R4A4_UNORM_PACK16,
		vk::VK_FORMAT_R5G6B5_UNORM_PACK16,
		vk::VK_FORMAT_B5G6R5_UNORM_PACK16,
		vk::VK_FORMAT_R5G5B5A1_UNORM_PACK16,
		vk::VK_FORMAT_B5G5R5A1_UNORM_PACK16,
		vk::VK_FORMAT_A1R5G5B5_UNORM_PACK16,
		vk::VK_FORMAT_R8G8B8_UNORM,
		vk::VK_FORMAT_B8G8R8_UNORM,
		vk::VK_FORMAT_R8G8B8A8_UNORM,
		vk::VK_FORMAT_B8G8R8A8_UNORM,
		vk::VK_FORMAT_A8B8G8R8_UNORM_PACK32,
		vk::VK_FORMAT_A2R10G10B10_UNORM_PACK32,
		vk::VK_FORMAT_A2B10G10R10_UNORM_PACK32,
		vk::VK_FORMAT_R16G16B16_UNORM,
		vk::VK_FORMAT_R16G16B16A16_UNORM,
		vk::VK_FORMAT_R10X6_UNORM_PACK16_KHR,
		vk::VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR,
		vk::VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR,
		vk::VK_FORMAT_R12X4_UNORM_PACK16_KHR,
		vk::VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR,
		vk::VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR,
		vk::VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR,
		vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR
	};
	const vk::VkFormat xChromaSubsampledFormats[] =
	{
		vk::VK_FORMAT_G8B8G8R8_422_UNORM_KHR,
		vk::VK_FORMAT_B8G8R8G8_422_UNORM_KHR,
		vk::VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR,
		vk::VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR,

		vk::VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR,
		vk::VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR,
		vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR,
		vk::VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR,
		vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G16B16G16R16_422_UNORM_KHR,
		vk::VK_FORMAT_B16G16R16G16_422_UNORM_KHR,
		vk::VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR,
		vk::VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR,
	};
	const vk::VkFormat xyChromaSubsampledFormats[] =
	{
		vk::VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR,
		vk::VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR,
		vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR,
		vk::VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR,
	};
	const struct
	{
		const char* const							name;
		const vk::VkSamplerYcbcrModelConversionKHR	value;
	} colorModels[] =
	{
		{ "rgb_identity",	vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY_KHR		},
		{ "ycbcr_identity",	vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_IDENTITY_KHR	},
		{ "ycbcr_709",		vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709_KHR			},
		{ "ycbcr_601",		vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601_KHR			},
		{ "ycbcr_2020",		vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020_KHR		}
	};
	const RangeNamePair colorRanges[]	=
	{
		{ "itu_full",		vk::VK_SAMPLER_YCBCR_RANGE_ITU_FULL_KHR		},
		{ "itu_narrow",		vk::VK_SAMPLER_YCBCR_RANGE_ITU_NARROW_KHR	}
	};
	const ChromaLocationNamePair chromaLocations[] =
	{
		{ "cosited",		vk::VK_CHROMA_LOCATION_COSITED_EVEN_KHR	},
		{ "midpoint",		vk::VK_CHROMA_LOCATION_MIDPOINT_KHR		}
	};
	const struct
	{
		const char* const	name;
		vk::VkFilter		value;
	} textureFilters[] =
	{
		{ "linear",			vk::VK_FILTER_LINEAR	},
		{ "nearest",		vk::VK_FILTER_NEAREST	}
	};
	// Used by the chroma reconstruction tests
	const vk::VkSamplerYcbcrModelConversionKHR	defaultColorModel		(vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY_KHR);
	const vk::VkSamplerYcbcrRangeKHR			defaultColorRange		(vk::VK_SAMPLER_YCBCR_RANGE_ITU_FULL_KHR);
	const vk::VkComponentMapping				identitySwizzle			=
	{
		vk::VK_COMPONENT_SWIZZLE_IDENTITY,
		vk::VK_COMPONENT_SWIZZLE_IDENTITY,
		vk::VK_COMPONENT_SWIZZLE_IDENTITY,
		vk::VK_COMPONENT_SWIZZLE_IDENTITY
	};
	const vk::VkComponentMapping				swappedChromaSwizzle	=
	{
		vk::VK_COMPONENT_SWIZZLE_B,
		vk::VK_COMPONENT_SWIZZLE_IDENTITY,
		vk::VK_COMPONENT_SWIZZLE_R,
		vk::VK_COMPONENT_SWIZZLE_IDENTITY
	};
	const glu::ShaderType						shaderTypes[]			=
	{
		glu::SHADERTYPE_VERTEX,
		glu::SHADERTYPE_FRAGMENT,
		glu::SHADERTYPE_COMPUTE
	};
	const struct
	{
		const char*			name;
		vk::VkImageTiling	value;
	}											imageTilings[]			=
	{
		{ "tiling_linear",	vk::VK_IMAGE_TILING_LINEAR },
		{ "tiling_optimal",	vk::VK_IMAGE_TILING_OPTIMAL }
	};
	tcu::TestContext&							testCtx					(testGroup->getTestContext());
	de::Random									rng						(1978765638u);

	// Test formats without chroma reconstruction
	for (size_t formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(noChromaSubsampledFormats); formatNdx++)
	{
		const vk::VkFormat				format		(noChromaSubsampledFormats[formatNdx]);
		const std::string				formatName	(de::toLower(std::string(getFormatName(format)).substr(10)));
		de::MovePtr<tcu::TestCaseGroup>	formatGroup	(new tcu::TestCaseGroup(testCtx, formatName.c_str(), ("Tests for color conversion using format " + formatName).c_str()));

		for (size_t modelNdx = 0; modelNdx < DE_LENGTH_OF_ARRAY(colorModels); modelNdx++)
		{
			const char* const							colorModelName	(colorModels[modelNdx].name);
			const vk::VkSamplerYcbcrModelConversionKHR	colorModel		(colorModels[modelNdx].value);

			if (colorModel != vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY_KHR && getFormatChannelCount(format) < 3)
				continue;

			de::MovePtr<tcu::TestCaseGroup>				colorModelGroup	(new tcu::TestCaseGroup(testCtx, colorModelName, ("Tests for color model " + string(colorModelName)).c_str()));

			if (colorModel == vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY_KHR)
			{
				for (size_t textureFilterNdx = 0; textureFilterNdx < DE_LENGTH_OF_ARRAY(textureFilters); textureFilterNdx++)
				{
					const char* const					textureFilterName	(textureFilters[textureFilterNdx].name);
					const vk::VkFilter					textureFilter		(textureFilters[textureFilterNdx].value);

					for (size_t tilingNdx = 0; tilingNdx < DE_LENGTH_OF_ARRAY(imageTilings); tilingNdx++)
					{
						const vk::VkImageTiling				tiling				(imageTilings[tilingNdx].value);
						const char* const					tilingName			(imageTilings[tilingNdx].name);
						const glu::ShaderType				shaderType			(rng.choose<glu::ShaderType>(DE_ARRAY_BEGIN(shaderTypes), DE_ARRAY_END(shaderTypes)));
						const vk::VkSamplerYcbcrRangeKHR	colorRange			(rng.choose<RangeNamePair, const RangeNamePair*>(DE_ARRAY_BEGIN(colorRanges), DE_ARRAY_END(colorRanges)).value);
						const vk::VkChromaLocationKHR		chromaLocation		(rng.choose<ChromaLocationNamePair, const ChromaLocationNamePair*>(DE_ARRAY_BEGIN(chromaLocations), DE_ARRAY_END(chromaLocations)).value);

						const TestConfig					config				(shaderType, format, tiling, textureFilter, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																					textureFilter, chromaLocation, chromaLocation, false, false,
																					colorRange, colorModel, identitySwizzle);

						addFunctionCaseWithPrograms(colorModelGroup.get(), std::string(textureFilterName) + "_" + tilingName, "", createTestShaders, textureConversionTest, config);
					}
				}
			}
			else
			{
				for (size_t rangeNdx = 0; rangeNdx < DE_LENGTH_OF_ARRAY(colorRanges); rangeNdx++)
				{
					const char* const					colorRangeName	(colorRanges[rangeNdx].name);
					const vk::VkSamplerYcbcrRangeKHR	colorRange		(colorRanges[rangeNdx].value);

					// Narrow range doesn't really work with formats that have less than 8 bits
					if (colorRange == vk::VK_SAMPLER_YCBCR_RANGE_ITU_NARROW_KHR)
					{
						const UVec4	bitDepth	(getBitDepth(format));

						if (bitDepth[0] < 8 || bitDepth[1] < 8 || bitDepth[2] < 8)
							continue;
					}

					de::MovePtr<tcu::TestCaseGroup>		colorRangeGroup	(new tcu::TestCaseGroup(testCtx, colorRangeName, ("Tests for color range " + string(colorRangeName)).c_str()));

					for (size_t textureFilterNdx = 0; textureFilterNdx < DE_LENGTH_OF_ARRAY(textureFilters); textureFilterNdx++)
					{
						const char* const				textureFilterName	(textureFilters[textureFilterNdx].name);
						const vk::VkFilter				textureFilter		(textureFilters[textureFilterNdx].value);

						for (size_t tilingNdx = 0; tilingNdx < DE_LENGTH_OF_ARRAY(imageTilings); tilingNdx++)
						{
							const vk::VkImageTiling			tiling				(imageTilings[tilingNdx].value);
							const char* const				tilingName			(imageTilings[tilingNdx].name);
							const glu::ShaderType			shaderType			(rng.choose<glu::ShaderType>(DE_ARRAY_BEGIN(shaderTypes), DE_ARRAY_END(shaderTypes)));
							const vk::VkChromaLocationKHR	chromaLocation		(rng.choose<ChromaLocationNamePair, const ChromaLocationNamePair*>(DE_ARRAY_BEGIN(chromaLocations), DE_ARRAY_END(chromaLocations)).value);
							const TestConfig				config				(shaderType, format, tiling, textureFilter, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																					textureFilter, chromaLocation, chromaLocation, false, false,
																					colorRange, colorModel, identitySwizzle);

							addFunctionCaseWithPrograms(colorRangeGroup.get(), std::string(textureFilterName) + "_" + tilingName, "", createTestShaders, textureConversionTest, config);
						}
					}

					colorModelGroup->addChild(colorRangeGroup.release());
				}
			}

			formatGroup->addChild(colorModelGroup.release());
		}

		testGroup->addChild(formatGroup.release());
	}

	// Test formats with x chroma reconstruction
	for (size_t formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(xChromaSubsampledFormats); formatNdx++)
	{
		const vk::VkFormat				format		(xChromaSubsampledFormats[formatNdx]);
		const std::string				formatName	(de::toLower(std::string(getFormatName(format)).substr(10)));
		de::MovePtr<tcu::TestCaseGroup>	formatGroup	(new tcu::TestCaseGroup(testCtx, formatName.c_str(), ("Tests for color conversion using format " + formatName).c_str()));

		// Color conversion tests
		{
			de::MovePtr<tcu::TestCaseGroup>	conversionGroup	(new tcu::TestCaseGroup(testCtx, "color_conversion", ""));

			for (size_t xChromaOffsetNdx = 0; xChromaOffsetNdx < DE_LENGTH_OF_ARRAY(chromaLocations); xChromaOffsetNdx++)
			{
				const char* const				xChromaOffsetName	(chromaLocations[xChromaOffsetNdx].name);
				const vk::VkChromaLocationKHR	xChromaOffset		(chromaLocations[xChromaOffsetNdx].value);

				for (size_t modelNdx = 0; modelNdx < DE_LENGTH_OF_ARRAY(colorModels); modelNdx++)
				{
					const char* const							colorModelName	(colorModels[modelNdx].name);
					const vk::VkSamplerYcbcrModelConversionKHR	colorModel		(colorModels[modelNdx].value);

					if (colorModel != vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY_KHR && getFormatChannelCount(format) < 3)
						continue;


					if (colorModel == vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY_KHR)
					{
						for (size_t tilingNdx = 0; tilingNdx < DE_LENGTH_OF_ARRAY(imageTilings); tilingNdx++)
						{
							const vk::VkImageTiling				tiling			(imageTilings[tilingNdx].value);
							const char* const					tilingName		(imageTilings[tilingNdx].name);
							const vk::VkSamplerYcbcrRangeKHR	colorRange		(rng.choose<RangeNamePair, const RangeNamePair*>(DE_ARRAY_BEGIN(colorRanges), DE_ARRAY_END(colorRanges)).value);
							const glu::ShaderType				shaderType		(rng.choose<glu::ShaderType>(DE_ARRAY_BEGIN(shaderTypes), DE_ARRAY_END(shaderTypes)));
							const vk::VkChromaLocationKHR		yChromaOffset	(rng.choose<ChromaLocationNamePair, const ChromaLocationNamePair*>(DE_ARRAY_BEGIN(chromaLocations), DE_ARRAY_END(chromaLocations)).value);
							const TestConfig					config			(shaderType, format, tiling, vk::VK_FILTER_NEAREST, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																				 vk::VK_FILTER_NEAREST, xChromaOffset, yChromaOffset, false, false,
																				 colorRange, colorModel, identitySwizzle);

							addFunctionCaseWithPrograms(conversionGroup.get(), std::string(colorModelName) + "_" + tilingName + "_" + xChromaOffsetName, "", createTestShaders, textureConversionTest, config);
						}
					}
					else
					{
						for (size_t rangeNdx = 0; rangeNdx < DE_LENGTH_OF_ARRAY(colorRanges); rangeNdx++)
						{
							const char* const					colorRangeName	(colorRanges[rangeNdx].name);
							const vk::VkSamplerYcbcrRangeKHR	colorRange		(colorRanges[rangeNdx].value);

							// Narrow range doesn't really work with formats that have less than 8 bits
							if (colorRange == vk::VK_SAMPLER_YCBCR_RANGE_ITU_NARROW_KHR)
							{
								const UVec4	bitDepth	(getBitDepth(format));

								if (bitDepth[0] < 8 || bitDepth[1] < 8 || bitDepth[2] < 8)
									continue;
							}

							for (size_t tilingNdx = 0; tilingNdx < DE_LENGTH_OF_ARRAY(imageTilings); tilingNdx++)
							{
								const vk::VkImageTiling			tiling			(imageTilings[tilingNdx].value);
								const char* const				tilingName		(imageTilings[tilingNdx].name);
								const glu::ShaderType			shaderType		(rng.choose<glu::ShaderType>(DE_ARRAY_BEGIN(shaderTypes), DE_ARRAY_END(shaderTypes)));
								const vk::VkChromaLocationKHR	yChromaOffset	(rng.choose<ChromaLocationNamePair, const ChromaLocationNamePair*>(DE_ARRAY_BEGIN(chromaLocations), DE_ARRAY_END(chromaLocations)).value);
								const TestConfig				config			(shaderType, format, tiling, vk::VK_FILTER_NEAREST, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																				vk::VK_FILTER_NEAREST, xChromaOffset, yChromaOffset, false, false,
																				colorRange, colorModel, identitySwizzle);

								addFunctionCaseWithPrograms(conversionGroup.get(), (string(colorModelName) + "_" + colorRangeName + "_" + tilingName + "_" + xChromaOffsetName).c_str(), "", createTestShaders, textureConversionTest, config);
							}
						}
					}
				}
			}

			formatGroup->addChild(conversionGroup.release());
		}

		// Chroma reconstruction tests
		{
			de::MovePtr<tcu::TestCaseGroup>	reconstrucGroup	(new tcu::TestCaseGroup(testCtx, "chroma_reconstruction", ""));

			for (size_t textureFilterNdx = 0; textureFilterNdx < DE_LENGTH_OF_ARRAY(textureFilters); textureFilterNdx++)
			{
				const char* const				textureFilterName	(textureFilters[textureFilterNdx].name);
				const vk::VkFilter				textureFilter		(textureFilters[textureFilterNdx].value);
				de::MovePtr<tcu::TestCaseGroup>	textureFilterGroup	(new tcu::TestCaseGroup(testCtx, textureFilterName, textureFilterName));

				for (size_t explicitReconstructionNdx = 0; explicitReconstructionNdx < 2; explicitReconstructionNdx++)
				{
					const bool	explicitReconstruction	(explicitReconstructionNdx == 1);

					for (size_t disjointNdx = 0; disjointNdx < 2; disjointNdx++)
					{
						const bool	disjoint	(disjointNdx == 1);

						for (size_t xChromaOffsetNdx = 0; xChromaOffsetNdx < DE_LENGTH_OF_ARRAY(chromaLocations); xChromaOffsetNdx++)
						{
							const vk::VkChromaLocationKHR	xChromaOffset		(chromaLocations[xChromaOffsetNdx].value);
							const char* const				xChromaOffsetName	(chromaLocations[xChromaOffsetNdx].name);

							for (size_t tilingNdx = 0; tilingNdx < DE_LENGTH_OF_ARRAY(imageTilings); tilingNdx++)
							{
								const vk::VkImageTiling	tiling		(imageTilings[tilingNdx].value);
								const char* const		tilingName	(imageTilings[tilingNdx].name);

								{
									const glu::ShaderType			shaderType		(rng.choose<glu::ShaderType>(DE_ARRAY_BEGIN(shaderTypes), DE_ARRAY_END(shaderTypes)));
									const vk::VkChromaLocationKHR	yChromaOffset	(rng.choose<ChromaLocationNamePair, const ChromaLocationNamePair*>(DE_ARRAY_BEGIN(chromaLocations), DE_ARRAY_END(chromaLocations)).value);
									const TestConfig				config			(shaderType, format, tiling, textureFilter, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																						vk::VK_FILTER_LINEAR, xChromaOffset, yChromaOffset, explicitReconstruction, disjoint,
																						defaultColorRange, defaultColorModel, identitySwizzle);

									addFunctionCaseWithPrograms(textureFilterGroup.get(), string(explicitReconstruction ? "explicit_linear_" : "default_linear_") + xChromaOffsetName + "_" + tilingName + (disjoint ? "_disjoint" : ""), "", createTestShaders, textureConversionTest, config);
								}

								{
									const glu::ShaderType			shaderType		(rng.choose<glu::ShaderType>(DE_ARRAY_BEGIN(shaderTypes), DE_ARRAY_END(shaderTypes)));
									const vk::VkChromaLocationKHR	yChromaOffset	(rng.choose<ChromaLocationNamePair, const ChromaLocationNamePair*>(DE_ARRAY_BEGIN(chromaLocations), DE_ARRAY_END(chromaLocations)).value);
									const TestConfig				config			(shaderType, format, tiling, textureFilter, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																						vk::VK_FILTER_LINEAR, xChromaOffset, yChromaOffset, explicitReconstruction, disjoint,
																						defaultColorRange, defaultColorModel, swappedChromaSwizzle);

									addFunctionCaseWithPrograms(textureFilterGroup.get(), string(explicitReconstruction ? "explicit_linear_" : "default_linear_") + xChromaOffsetName + "_" + tilingName + (disjoint ? "_disjoint" : "") + "_swapped_chroma", "", createTestShaders, textureConversionTest, config);
								}

								if (!explicitReconstruction)
								{
									{
										const glu::ShaderType			shaderType		(rng.choose<glu::ShaderType>(DE_ARRAY_BEGIN(shaderTypes), DE_ARRAY_END(shaderTypes)));
										const vk::VkChromaLocationKHR	yChromaOffset	(rng.choose<ChromaLocationNamePair, const ChromaLocationNamePair*>(DE_ARRAY_BEGIN(chromaLocations), DE_ARRAY_END(chromaLocations)).value);
										const TestConfig				config			(shaderType, format, tiling, textureFilter, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																							vk::VK_FILTER_NEAREST, xChromaOffset, yChromaOffset, explicitReconstruction, disjoint,
																							defaultColorRange, defaultColorModel, identitySwizzle);

										addFunctionCaseWithPrograms(textureFilterGroup.get(), string("default_nearest_") + xChromaOffsetName + "_" + tilingName + (disjoint ? "_disjoint" : ""), "", createTestShaders, textureConversionTest, config);
									}

									{
										const glu::ShaderType			shaderType		(rng.choose<glu::ShaderType>(DE_ARRAY_BEGIN(shaderTypes), DE_ARRAY_END(shaderTypes)));
										const vk::VkChromaLocationKHR	yChromaOffset	(rng.choose<ChromaLocationNamePair, const ChromaLocationNamePair*>(DE_ARRAY_BEGIN(chromaLocations), DE_ARRAY_END(chromaLocations)).value);
										const TestConfig				config			(shaderType, format, tiling, textureFilter, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																							vk::VK_FILTER_NEAREST, xChromaOffset, yChromaOffset, explicitReconstruction, disjoint,
																							defaultColorRange, defaultColorModel, swappedChromaSwizzle);

										addFunctionCaseWithPrograms(textureFilterGroup.get(), string("default_nearest_") + xChromaOffsetName + "_" + tilingName + (disjoint ? "_disjoint" : "") + "_swapped_chroma", "", createTestShaders, textureConversionTest, config);
									}
								}
							}
						}

						if (explicitReconstruction)
						{
							for (size_t tilingNdx = 0; tilingNdx < DE_LENGTH_OF_ARRAY(imageTilings); tilingNdx++)
							{
								const vk::VkImageTiling	tiling		(imageTilings[tilingNdx].value);
								const char* const		tilingName	(imageTilings[tilingNdx].name);
								{
									const glu::ShaderType			shaderType		(rng.choose<glu::ShaderType>(DE_ARRAY_BEGIN(shaderTypes), DE_ARRAY_END(shaderTypes)));
									const vk::VkChromaLocationKHR	chromaLocation	(rng.choose<ChromaLocationNamePair, const ChromaLocationNamePair*>(DE_ARRAY_BEGIN(chromaLocations), DE_ARRAY_END(chromaLocations)).value);
									const TestConfig				config			(shaderType, format, tiling, textureFilter, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																						vk::VK_FILTER_NEAREST, chromaLocation, chromaLocation, explicitReconstruction, disjoint,
																						defaultColorRange, defaultColorModel, identitySwizzle);

									addFunctionCaseWithPrograms(textureFilterGroup.get(), string("explicit_nearest") + "_" + tilingName + (disjoint ? "_disjoint" : ""), "", createTestShaders, textureConversionTest, config);
								}

								{
									const glu::ShaderType			shaderType		(rng.choose<glu::ShaderType>(DE_ARRAY_BEGIN(shaderTypes), DE_ARRAY_END(shaderTypes)));
									const vk::VkChromaLocationKHR	chromaLocation	(rng.choose<ChromaLocationNamePair, const ChromaLocationNamePair*>(DE_ARRAY_BEGIN(chromaLocations), DE_ARRAY_END(chromaLocations)).value);
									const TestConfig				config			(shaderType, format, tiling, textureFilter, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																						vk::VK_FILTER_NEAREST, chromaLocation, chromaLocation, explicitReconstruction, disjoint,
																						defaultColorRange, defaultColorModel, swappedChromaSwizzle);

									addFunctionCaseWithPrograms(textureFilterGroup.get(), string("explicit_nearest") + "_" + tilingName + (disjoint ? "_disjoint" : "") + "_swapped_chroma", "", createTestShaders, textureConversionTest, config);
								}
							}
						}
					}
				}

				reconstrucGroup->addChild(textureFilterGroup.release());
			}

			formatGroup->addChild(reconstrucGroup.release());
		}

		testGroup->addChild(formatGroup.release());
	}

	// Test formats with xy chroma reconstruction
	for (size_t formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(xyChromaSubsampledFormats); formatNdx++)
	{
		const vk::VkFormat				format		(xyChromaSubsampledFormats[formatNdx]);
		const std::string				formatName	(de::toLower(std::string(getFormatName(format)).substr(10)));
		de::MovePtr<tcu::TestCaseGroup>	formatGroup	(new tcu::TestCaseGroup(testCtx, formatName.c_str(), ("Tests for color conversion using format " + formatName).c_str()));

		// Color conversion tests
		{
			de::MovePtr<tcu::TestCaseGroup>	conversionGroup	(new tcu::TestCaseGroup(testCtx, "color_conversion", ""));

			for (size_t chromaOffsetNdx = 0; chromaOffsetNdx < DE_LENGTH_OF_ARRAY(chromaLocations); chromaOffsetNdx++)
			{
				const char* const				chromaOffsetName	(chromaLocations[chromaOffsetNdx].name);
				const vk::VkChromaLocationKHR	chromaOffset		(chromaLocations[chromaOffsetNdx].value);

				for (size_t modelNdx = 0; modelNdx < DE_LENGTH_OF_ARRAY(colorModels); modelNdx++)
				{
					const char* const							colorModelName	(colorModels[modelNdx].name);
					const vk::VkSamplerYcbcrModelConversionKHR	colorModel		(colorModels[modelNdx].value);

					if (colorModel != vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY_KHR && getFormatChannelCount(format) < 3)
						continue;

					if (colorModel == vk::VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY_KHR)
					{
						for (size_t tilingNdx = 0; tilingNdx < DE_LENGTH_OF_ARRAY(imageTilings); tilingNdx++)
						{
							const vk::VkImageTiling				tiling			(imageTilings[tilingNdx].value);
							const char* const					tilingName		(imageTilings[tilingNdx].name);
							const vk::VkSamplerYcbcrRangeKHR	colorRange		(rng.choose<RangeNamePair, const RangeNamePair*>(DE_ARRAY_BEGIN(colorRanges), DE_ARRAY_END(colorRanges)).value);
							const glu::ShaderType				shaderType		(rng.choose<glu::ShaderType>(DE_ARRAY_BEGIN(shaderTypes), DE_ARRAY_END(shaderTypes)));
							const TestConfig					config			(shaderType, format, tiling, vk::VK_FILTER_NEAREST, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																				 vk::VK_FILTER_NEAREST, chromaOffset, chromaOffset, false, false,
																				 colorRange, colorModel, identitySwizzle);

							addFunctionCaseWithPrograms(conversionGroup.get(), std::string(colorModelName) + "_" + tilingName + "_" + chromaOffsetName, "", createTestShaders, textureConversionTest, config);
						}
					}
					else
					{
						for (size_t rangeNdx = 0; rangeNdx < DE_LENGTH_OF_ARRAY(colorRanges); rangeNdx++)
						{
							const char* const					colorRangeName	(colorRanges[rangeNdx].name);
							const vk::VkSamplerYcbcrRangeKHR	colorRange		(colorRanges[rangeNdx].value);

							// Narrow range doesn't really work with formats that have less than 8 bits
							if (colorRange == vk::VK_SAMPLER_YCBCR_RANGE_ITU_NARROW_KHR)
							{
								const UVec4	bitDepth	(getBitDepth(format));

								if (bitDepth[0] < 8 || bitDepth[1] < 8 || bitDepth[2] < 8)
									continue;
							}

							for (size_t tilingNdx = 0; tilingNdx < DE_LENGTH_OF_ARRAY(imageTilings); tilingNdx++)
							{
								const vk::VkImageTiling			tiling			(imageTilings[tilingNdx].value);
								const char* const				tilingName		(imageTilings[tilingNdx].name);
								const glu::ShaderType			shaderType		(rng.choose<glu::ShaderType>(DE_ARRAY_BEGIN(shaderTypes), DE_ARRAY_END(shaderTypes)));
								const TestConfig				config			(shaderType, format, tiling, vk::VK_FILTER_NEAREST, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																					vk::VK_FILTER_NEAREST, chromaOffset, chromaOffset, false, false,
																					colorRange, colorModel, identitySwizzle);

								addFunctionCaseWithPrograms(conversionGroup.get(), (string(colorModelName) + "_" + colorRangeName + "_" + tilingName + "_" + chromaOffsetName).c_str(), "", createTestShaders, textureConversionTest, config);
							}
						}
					}
				}
			}

			formatGroup->addChild(conversionGroup.release());
		}

		// Chroma reconstruction tests
		{
			de::MovePtr<tcu::TestCaseGroup>	reconstrucGroup	(new tcu::TestCaseGroup(testCtx, "chroma_reconstruction", ""));

			for (size_t textureFilterNdx = 0; textureFilterNdx < DE_LENGTH_OF_ARRAY(textureFilters); textureFilterNdx++)
			{
				const char* const				textureFilterName	(textureFilters[textureFilterNdx].name);
				const vk::VkFilter				textureFilter		(textureFilters[textureFilterNdx].value);
				de::MovePtr<tcu::TestCaseGroup>	textureFilterGroup	(new tcu::TestCaseGroup(testCtx, textureFilterName, textureFilterName));

				for (size_t explicitReconstructionNdx = 0; explicitReconstructionNdx < 2; explicitReconstructionNdx++)
				{
					const bool	explicitReconstruction	(explicitReconstructionNdx == 1);

					for (size_t disjointNdx = 0; disjointNdx < 2; disjointNdx++)
					{
						const bool	disjoint	(disjointNdx == 1);

						for (size_t xChromaOffsetNdx = 0; xChromaOffsetNdx < DE_LENGTH_OF_ARRAY(chromaLocations); xChromaOffsetNdx++)
						for (size_t yChromaOffsetNdx = 0; yChromaOffsetNdx < DE_LENGTH_OF_ARRAY(chromaLocations); yChromaOffsetNdx++)
						{
							const vk::VkChromaLocationKHR	xChromaOffset		(chromaLocations[xChromaOffsetNdx].value);
							const char* const				xChromaOffsetName	(chromaLocations[xChromaOffsetNdx].name);

							const vk::VkChromaLocationKHR	yChromaOffset		(chromaLocations[yChromaOffsetNdx].value);
							const char* const				yChromaOffsetName	(chromaLocations[yChromaOffsetNdx].name);

							for (size_t tilingNdx = 0; tilingNdx < DE_LENGTH_OF_ARRAY(imageTilings); tilingNdx++)
							{
								const vk::VkImageTiling	tiling		(imageTilings[tilingNdx].value);
								const char* const		tilingName	(imageTilings[tilingNdx].name);
								{
									const glu::ShaderType	shaderType	(rng.choose<glu::ShaderType>(DE_ARRAY_BEGIN(shaderTypes), DE_ARRAY_END(shaderTypes)));
									const TestConfig		config		(shaderType, format, tiling, textureFilter, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																			vk::VK_FILTER_LINEAR, xChromaOffset, yChromaOffset, explicitReconstruction, disjoint,
																			defaultColorRange, defaultColorModel, identitySwizzle);

									addFunctionCaseWithPrograms(textureFilterGroup.get(), string(explicitReconstruction ? "explicit_linear_" : "default_linear_") + xChromaOffsetName + "_" + yChromaOffsetName + "_" + tilingName + (disjoint ? "_disjoint" : ""), "", createTestShaders, textureConversionTest, config);
								}

								{
									const glu::ShaderType	shaderType	(rng.choose<glu::ShaderType>(DE_ARRAY_BEGIN(shaderTypes), DE_ARRAY_END(shaderTypes)));
									const TestConfig		config		(shaderType, format, tiling, textureFilter, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																			vk::VK_FILTER_LINEAR, xChromaOffset, yChromaOffset, explicitReconstruction, disjoint,
																			defaultColorRange, defaultColorModel, swappedChromaSwizzle);

									addFunctionCaseWithPrograms(textureFilterGroup.get(), string(explicitReconstruction ? "explicit_linear_" : "default_linear_") + xChromaOffsetName + "_" + yChromaOffsetName + "_" + tilingName + (disjoint ? "_disjoint" : "") + "_swapped_chroma", "", createTestShaders, textureConversionTest, config);
								}

								if (!explicitReconstruction)
								{
									{
										const glu::ShaderType	shaderType	(rng.choose<glu::ShaderType>(DE_ARRAY_BEGIN(shaderTypes), DE_ARRAY_END(shaderTypes)));
										const TestConfig		config		(shaderType, format, tiling, textureFilter, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																				vk::VK_FILTER_NEAREST, xChromaOffset, yChromaOffset, explicitReconstruction, disjoint,
																				defaultColorRange, defaultColorModel, identitySwizzle);

										addFunctionCaseWithPrograms(textureFilterGroup.get(), string("default_nearest_") + xChromaOffsetName + "_" + yChromaOffsetName + "_" + tilingName + (disjoint ? "_disjoint" : ""), "", createTestShaders, textureConversionTest, config);
									}

									{
										const glu::ShaderType	shaderType	(rng.choose<glu::ShaderType>(DE_ARRAY_BEGIN(shaderTypes), DE_ARRAY_END(shaderTypes)));
										const TestConfig		config		(shaderType, format, tiling, textureFilter, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																				vk::VK_FILTER_NEAREST, xChromaOffset, yChromaOffset, explicitReconstruction, disjoint,
																				defaultColorRange, defaultColorModel, swappedChromaSwizzle);

										addFunctionCaseWithPrograms(textureFilterGroup.get(), string("default_nearest_") + xChromaOffsetName + "_" + yChromaOffsetName + "_" + tilingName + (disjoint ? "_disjoint" : "") + "_swapped_chroma", "", createTestShaders, textureConversionTest, config);
									}
								}
							}
						}

						if (explicitReconstruction)
						{
							for (size_t tilingNdx = 0; tilingNdx < DE_LENGTH_OF_ARRAY(imageTilings); tilingNdx++)
							{
								const vk::VkImageTiling	tiling		(imageTilings[tilingNdx].value);
								const char* const		tilingName	(imageTilings[tilingNdx].name);
								{
									const glu::ShaderType			shaderType		(rng.choose<glu::ShaderType>(DE_ARRAY_BEGIN(shaderTypes), DE_ARRAY_END(shaderTypes)));
									const vk::VkChromaLocationKHR	chromaLocation	(rng.choose<ChromaLocationNamePair, const ChromaLocationNamePair*>(DE_ARRAY_BEGIN(chromaLocations), DE_ARRAY_END(chromaLocations)).value);
									const TestConfig				config			(shaderType, format, tiling, textureFilter, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																						vk::VK_FILTER_NEAREST, chromaLocation, chromaLocation, explicitReconstruction, disjoint,
																						defaultColorRange, defaultColorModel, identitySwizzle);

									addFunctionCaseWithPrograms(textureFilterGroup.get(), string("explicit_nearest") + "_" + tilingName + (disjoint ? "_disjoint" : ""), "", createTestShaders, textureConversionTest, config);
								}

								{
									const glu::ShaderType			shaderType		(rng.choose<glu::ShaderType>(DE_ARRAY_BEGIN(shaderTypes), DE_ARRAY_END(shaderTypes)));
									const vk::VkChromaLocationKHR	chromaLocation	(rng.choose<ChromaLocationNamePair, const ChromaLocationNamePair*>(DE_ARRAY_BEGIN(chromaLocations), DE_ARRAY_END(chromaLocations)).value);
									const TestConfig				config			(shaderType, format, tiling, textureFilter, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
																						vk::VK_FILTER_NEAREST, chromaLocation, chromaLocation, explicitReconstruction, disjoint,
																						defaultColorRange, defaultColorModel, swappedChromaSwizzle);

									addFunctionCaseWithPrograms(textureFilterGroup.get(), string("explicit_nearest") + "_" + tilingName + (disjoint ? "_disjoint" : "") + "_swapped_chroma", "", createTestShaders, textureConversionTest, config);
								}
							}
						}
					}
				}

				reconstrucGroup->addChild(textureFilterGroup.release());
			}

			formatGroup->addChild(reconstrucGroup.release());
		}

		testGroup->addChild(formatGroup.release());
	}
}

} // anonymous

tcu::TestCaseGroup* createConversionTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "conversion", "Sampler YCbCr Conversion Tests", initTests);
}

} // ycbcr
} // vkt
