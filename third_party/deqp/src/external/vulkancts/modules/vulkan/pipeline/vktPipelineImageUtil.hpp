#ifndef _VKTPIPELINEIMAGEUTIL_HPP
#define _VKTPIPELINEIMAGEUTIL_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Imagination Technologies Ltd.
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
 * \brief Utilities for images.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "vkDefs.hpp"
#include "vkDefs.hpp"
#include "vkPlatform.hpp"
#include "vkMemUtil.hpp"
#include "vkRef.hpp"
#include "tcuTexture.hpp"
#include "tcuCompressedTexture.hpp"
#include "deSharedPtr.hpp"

namespace vkt
{
namespace pipeline
{

class TestTexture;

enum BorderColor
{
	BORDER_COLOR_OPAQUE_BLACK,
	BORDER_COLOR_OPAQUE_WHITE,
	BORDER_COLOR_TRANSPARENT_BLACK,

	BORDER_COLOR_COUNT
};

bool							isSupportedSamplableFormat	(const vk::InstanceInterface&	instanceInterface,
															 vk::VkPhysicalDevice			device,
															 vk::VkFormat					format);
bool							isLinearFilteringSupported	(const vk::InstanceInterface&	instanceInterface,
															 vk::VkPhysicalDevice			device,
															 vk::VkFormat					format,
															 vk::VkImageTiling				tiling);

bool							isMinMaxFilteringSupported	(const vk::InstanceInterface&	instanceInterface,
															 vk::VkPhysicalDevice			device,
															 vk::VkFormat					format,
															 vk::VkImageTiling				tiling);

vk::VkBorderColor				getFormatBorderColor		(BorderColor color, vk::VkFormat format);

void							getLookupScaleBias			(vk::VkFormat					format,
															 tcu::Vec4&						lookupScale,
															 tcu::Vec4&						lookupBias);

/*--------------------------------------------------------------------*//*!
 * Gets a tcu::TextureLevel initialized with data from a VK color
 * attachment.
 *
 * The VkImage must be non-multisampled and able to be used as a source
 * operand for transfer operations.
 *//*--------------------------------------------------------------------*/
de::MovePtr<tcu::TextureLevel>	readColorAttachment			 (const vk::DeviceInterface&	vk,
															  vk::VkDevice					device,
															  vk::VkQueue					queue,
															  deUint32						queueFamilyIndex,
															  vk::Allocator&				allocator,
															  vk::VkImage					image,
															  vk::VkFormat					format,
															  const tcu::UVec2&				renderSize);

/*--------------------------------------------------------------------*//*!
 * Uploads data from a test texture to a destination VK image.
 *
 * The VkImage must be non-multisampled and able to be used as a
 * destination operand for transfer operations.
 *//*--------------------------------------------------------------------*/
void							uploadTestTexture			(const vk::DeviceInterface&		vk,
															 vk::VkDevice					device,
															 vk::VkQueue					queue,
															 deUint32						queueFamilyIndex,
															 vk::Allocator&					allocator,
															 const TestTexture&				testTexture,
															 vk::VkImage					destImage);

/*--------------------------------------------------------------------*//*!
 * Uploads data from a test texture to a destination VK image using sparse
 * binding.
 *
 * The VkImage must be non-multisampled and able to be used as a
 * destination operand for transfer operations.
 *//*--------------------------------------------------------------------*/
void							uploadTestTextureSparse		(const vk::DeviceInterface&						vk,
															 vk::VkDevice									device,
															 const vk::VkPhysicalDevice						physicalDevice,
															 const vk::InstanceInterface&					instance,
															 const vk::VkImageCreateInfo&					imageCreateInfo,
															 vk::VkQueue									universalQueue,
															 deUint32										universalQueueFamilyIndex,
															 vk::VkQueue									sparseQueue,
															 vk::Allocator&									allocator,
															 std::vector<de::SharedPtr<vk::Allocation> >&	allocations,
															 const TestTexture&								srcTexture,
															 vk::VkImage									destImage);

/*--------------------------------------------------------------------*//*!
 * Allocates memory for a sparse image and handles the memory binding.
 *//*--------------------------------------------------------------------*/
void							allocateAndBindSparseImage	(const vk::DeviceInterface&						vk,
															 vk::VkDevice									device,
															 const vk::VkPhysicalDevice						physicalDevice,
															 const vk::InstanceInterface&					instance,
															 const vk::VkImageCreateInfo&					imageCreateInfo,
															 const vk::VkSemaphore&							signalSemaphore,
															 vk::VkQueue									queue,
															 vk::Allocator&									allocator,
															 std::vector<de::SharedPtr<vk::Allocation> >&	allocations,
															 tcu::TextureFormat								format,
															 vk::VkImage									destImage);

class TestTexture
{
public:
												TestTexture					(const tcu::TextureFormat& format, int width, int height, int depth);
												TestTexture					(const tcu::CompressedTexFormat& format, int width, int height, int depth);
	virtual										~TestTexture				(void);

	virtual int									getNumLevels				(void) const = 0;
	virtual deUint32							getSize						(void) const;
	virtual int									getArraySize				(void) const { return 1; }

	virtual bool								isCompressed				(void) const { return !m_compressedLevels.empty(); }
	virtual deUint32							getCompressedSize			(void) const;

	virtual tcu::PixelBufferAccess				getLevel					(int level, int layer) = 0;
	virtual const tcu::ConstPixelBufferAccess	getLevel					(int level, int layer) const = 0;

	virtual tcu::CompressedTexture&				getCompressedLevel			(int level, int layer);
	virtual const tcu::CompressedTexture&		getCompressedLevel			(int level, int layer) const;

	virtual std::vector<vk::VkBufferImageCopy>	getBufferCopyRegions		(void) const;
	virtual void								write						(deUint8* destPtr) const;
	virtual de::MovePtr<TestTexture>			copy						(const tcu::TextureFormat) const = 0;

	virtual const tcu::TextureFormat&			getTextureFormat			(void) const = 0;
	virtual tcu::UVec3							getTextureDimension			(void) const = 0;

protected:
	void										populateLevels				(const std::vector<tcu::PixelBufferAccess>& levels);
	void										populateCompressedLevels	(tcu::CompressedTexFormat format, const std::vector<tcu::PixelBufferAccess>& decompressedLevels);

	static void									fillWithGradient			(const tcu::PixelBufferAccess& levelAccess);

	void										copyToTexture				(TestTexture&) const;

protected:
	std::vector<tcu::CompressedTexture*>		m_compressedLevels;
};

class TestTexture1D : public TestTexture
{
private:
	tcu::Texture1D								m_texture;

public:
												TestTexture1D		(const tcu::TextureFormat& format, int width);
												TestTexture1D		(const tcu::CompressedTexFormat& format, int width);
	virtual										~TestTexture1D		(void);

	virtual int getNumLevels (void) const;
	virtual tcu::PixelBufferAccess				getLevel			(int level, int layer);
	virtual const tcu::ConstPixelBufferAccess	getLevel			(int level, int layer) const;
	virtual const tcu::Texture1D&				getTexture			(void) const;
	virtual tcu::Texture1D&						getTexture			(void);
	virtual const tcu::TextureFormat&			getTextureFormat	(void) const { return m_texture.getFormat(); }
	virtual tcu::UVec3							getTextureDimension	(void) const { return tcu::UVec3(m_texture.getWidth(), 1, 1); }

	virtual de::MovePtr<TestTexture>			copy				(const tcu::TextureFormat) const;
};

class TestTexture1DArray : public TestTexture
{
private:
	tcu::Texture1DArray							m_texture;

public:
												TestTexture1DArray	(const tcu::TextureFormat& format, int width, int arraySize);
												TestTexture1DArray	(const tcu::CompressedTexFormat& format, int width, int arraySize);
	virtual										~TestTexture1DArray	(void);

	virtual int									getNumLevels		(void) const;
	virtual tcu::PixelBufferAccess				getLevel			(int level, int layer);
	virtual const tcu::ConstPixelBufferAccess	getLevel			(int level, int layer) const;
	virtual const tcu::Texture1DArray&			getTexture			(void) const;
	virtual tcu::Texture1DArray&				getTexture			(void);
	virtual int									getArraySize		(void) const;
	virtual const tcu::TextureFormat&			getTextureFormat	(void) const { return m_texture.getFormat(); }
	virtual tcu::UVec3							getTextureDimension	(void) const { return tcu::UVec3(m_texture.getWidth(), 1, 1); }

	virtual de::MovePtr<TestTexture>			copy				(const tcu::TextureFormat) const;
};

class TestTexture2D : public TestTexture
{
private:
	tcu::Texture2D								m_texture;

public:
												TestTexture2D		(const tcu::TextureFormat& format, int width, int height);
												TestTexture2D		(const tcu::CompressedTexFormat& format, int width, int height);
	virtual										~TestTexture2D		(void);

	virtual int									getNumLevels		(void) const;
	virtual tcu::PixelBufferAccess				getLevel			(int level, int layer);
	virtual const tcu::ConstPixelBufferAccess	getLevel			(int level, int layer) const;
	virtual const tcu::Texture2D&				getTexture			(void) const;
	virtual tcu::Texture2D&						getTexture			(void);
	virtual const tcu::TextureFormat&			getTextureFormat	(void) const { return m_texture.getFormat(); }
	virtual tcu::UVec3							getTextureDimension	(void) const { return tcu::UVec3(m_texture.getWidth(), m_texture.getHeight(), 1); }

	virtual de::MovePtr<TestTexture>			copy				(const tcu::TextureFormat) const;
};

class TestTexture2DArray : public TestTexture
{
private:
	tcu::Texture2DArray	m_texture;

public:
												TestTexture2DArray	(const tcu::TextureFormat& format, int width, int height, int arraySize);
												TestTexture2DArray	(const tcu::CompressedTexFormat& format, int width, int height, int arraySize);
	virtual										~TestTexture2DArray	(void);

	virtual int									getNumLevels		(void) const;
	virtual tcu::PixelBufferAccess				getLevel			(int level, int layer);
	virtual const tcu::ConstPixelBufferAccess	getLevel			(int level, int layer) const;
	virtual const tcu::Texture2DArray&			getTexture			(void) const;
	virtual tcu::Texture2DArray&				getTexture			(void);
	virtual int									getArraySize		(void) const;
	virtual const tcu::TextureFormat&			getTextureFormat	(void) const { return m_texture.getFormat(); }
	virtual tcu::UVec3							getTextureDimension	(void) const { return tcu::UVec3(m_texture.getWidth(), m_texture.getHeight(), 1); }

	virtual de::MovePtr<TestTexture>			copy				(const tcu::TextureFormat) const;
};

class TestTexture3D : public TestTexture
{
private:
	tcu::Texture3D	m_texture;

public:
												TestTexture3D		(const tcu::TextureFormat& format, int width, int height, int depth);
												TestTexture3D		(const tcu::CompressedTexFormat& format, int width, int height, int depth);
	virtual										~TestTexture3D		(void);

	virtual int									getNumLevels		(void) const;
	virtual tcu::PixelBufferAccess				getLevel			(int level, int layer);
	virtual const tcu::ConstPixelBufferAccess	getLevel			(int level, int layer) const;
	virtual const tcu::Texture3D&				getTexture			(void) const;
	virtual tcu::Texture3D&						getTexture			(void);
	virtual const tcu::TextureFormat&			getTextureFormat	(void) const { return m_texture.getFormat(); }
	virtual tcu::UVec3							getTextureDimension	(void) const { return tcu::UVec3(m_texture.getWidth(), m_texture.getHeight(), m_texture.getDepth()); }

	virtual de::MovePtr<TestTexture>			copy				(const tcu::TextureFormat) const;
};

class TestTextureCube : public TestTexture
{
private:
	tcu::TextureCube							m_texture;

public:
												TestTextureCube			(const tcu::TextureFormat& format, int size);
												TestTextureCube			(const tcu::CompressedTexFormat& format, int size);
	virtual										~TestTextureCube		(void);

	virtual int									getNumLevels			(void) const;
	virtual tcu::PixelBufferAccess				getLevel				(int level, int layer);
	virtual const tcu::ConstPixelBufferAccess	getLevel				(int level, int layer) const;
	virtual int									getArraySize			(void) const;
	virtual const tcu::TextureCube&				getTexture				(void) const;
	virtual tcu::TextureCube&					getTexture				(void);
	virtual const tcu::TextureFormat&			getTextureFormat		(void) const { return m_texture.getFormat(); }
	virtual tcu::UVec3							getTextureDimension		(void) const { return tcu::UVec3(m_texture.getSize(), m_texture.getSize(), 1); }

	virtual de::MovePtr<TestTexture>			copy					(const tcu::TextureFormat) const;
};

class TestTextureCubeArray: public TestTexture
{
private:
	tcu::TextureCubeArray						m_texture;

public:
												TestTextureCubeArray	(const tcu::TextureFormat& format, int size, int arraySize);
												TestTextureCubeArray	(const tcu::CompressedTexFormat& format, int size, int arraySize);
	virtual										~TestTextureCubeArray	(void);

	virtual int									getNumLevels			(void) const;
	virtual tcu::PixelBufferAccess				getLevel				(int level, int layer);
	virtual const tcu::ConstPixelBufferAccess	getLevel				(int level, int layer) const;
	virtual int									getArraySize			(void) const;
	virtual const tcu::TextureCubeArray&		getTexture				(void) const;
	virtual tcu::TextureCubeArray&				getTexture				(void);
	virtual const tcu::TextureFormat&			getTextureFormat		(void) const { return m_texture.getFormat(); }
	virtual tcu::UVec3							getTextureDimension		(void) const { return tcu::UVec3(m_texture.getSize(), m_texture.getSize(), 1); }

	virtual de::MovePtr<TestTexture>			copy					(const tcu::TextureFormat) const;
};

} // pipeline
} // vkt

#endif // _VKTPIPELINEIMAGEUTIL_HPP
