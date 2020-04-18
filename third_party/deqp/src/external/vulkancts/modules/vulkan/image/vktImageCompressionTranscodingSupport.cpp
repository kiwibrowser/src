/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
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
 *//*!
 * \file  vktImageCompressionTranscodingSupport.cpp
 * \brief Compression transcoding support
 *//*--------------------------------------------------------------------*/

#include "vktImageCompressionTranscodingSupport.hpp"
#include "vktImageLoadStoreUtil.hpp"

#include "deUniquePtr.hpp"
#include "deStringUtil.hpp"
#include "deSharedPtr.hpp"
#include "deRandom.hpp"

#include "vktTestCaseUtil.hpp"
#include "vkPrograms.hpp"
#include "vkImageUtil.hpp"
#include "vktImageTestsUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkQueryUtil.hpp"

#include "tcuTextureUtil.hpp"
#include "tcuTexture.hpp"
#include "tcuCompressedTexture.hpp"
#include "tcuVectorType.hpp"
#include "tcuResource.hpp"
#include "tcuImageIO.hpp"
#include "tcuImageCompare.hpp"
#include "tcuTestLog.hpp"
#include "tcuRGBA.hpp"
#include "tcuSurface.hpp"

#include <vector>
using namespace vk;
namespace vkt
{
namespace image
{
namespace
{
using std::string;
using std::vector;
using tcu::TestContext;
using tcu::TestStatus;
using tcu::UVec3;
using tcu::IVec3;
using tcu::CompressedTexFormat;
using tcu::CompressedTexture;
using tcu::Resource;
using tcu::Archive;
using tcu::ConstPixelBufferAccess;
using de::MovePtr;
using de::SharedPtr;
using de::Random;

typedef SharedPtr<MovePtr<Image> >			ImageSp;
typedef SharedPtr<Move<VkImageView> >		ImageViewSp;
typedef SharedPtr<Move<VkDescriptorSet> >	SharedVkDescriptorSet;

enum ShaderType
{
	SHADER_TYPE_COMPUTE,
	SHADER_TYPE_FRAGMENT,
	SHADER_TYPE_LAST
};

enum Operation
{
	OPERATION_IMAGE_LOAD,
	OPERATION_TEXEL_FETCH,
	OPERATION_TEXTURE,
	OPERATION_IMAGE_STORE,
	OPERATION_ATTACHMENT_READ,
	OPERATION_ATTACHMENT_WRITE,
	OPERATION_TEXTURE_READ,
	OPERATION_TEXTURE_WRITE,
	OPERATION_LAST
};

struct TestParameters
{
	Operation			operation;
	ShaderType			shader;
	UVec3				size;
	ImageType			imageType;
	VkFormat			formatCompressed;
	VkFormat			formatUncompressed;
	deUint32			imagesCount;
	VkImageUsageFlags	compressedImageUsage;
	VkImageUsageFlags	compressedImageViewUsage;
	VkImageUsageFlags	uncompressedImageUsage;
	bool				useMipmaps;
	VkFormat			formatForVerify;
};

template<typename T>
inline SharedPtr<Move<T> > makeVkSharedPtr (Move<T> move)
{
	return SharedPtr<Move<T> >(new Move<T>(move));
}

template<typename T>
inline SharedPtr<MovePtr<T> > makeVkSharedPtr (MovePtr<T> movePtr)
{
	return SharedPtr<MovePtr<T> >(new MovePtr<T>(movePtr));
}

const deUint32 SINGLE_LEVEL = 1u;
const deUint32 SINGLE_LAYER = 1u;

class BasicTranscodingTestInstance : public TestInstance
{
public:
							BasicTranscodingTestInstance	(Context&						context,
															 const TestParameters&			parameters);
	virtual TestStatus		iterate							(void) = 0;
protected:
	void					generateData					(deUint8*						toFill,
															 const size_t					size,
															 const VkFormat					format,
															 const deUint32					layer = 0u,
															 const deUint32					level = 0u);
	deUint32				getLevelCount					();
	deUint32				getLayerCount					();
	UVec3					getLayerDims					();
	vector<UVec3>			getMipLevelSizes				(UVec3							baseSize);
	vector<UVec3>			getCompressedMipLevelSizes		(const VkFormat					compressedFormat,
															 const vector<UVec3>&			uncompressedSizes);

	const TestParameters	m_parameters;
	const deUint32			m_blockWidth;
	const deUint32			m_blockHeight;
	const deUint32			m_levelCount;
	const UVec3				m_layerSize;

private:
	deUint32				findMipMapLevelCount			();
};

deUint32 BasicTranscodingTestInstance::findMipMapLevelCount ()
{
	deUint32 levelCount = 1;

	// We cannot use mipmap levels which have resolution below block size.
	// Reduce number of mipmap levels
	if (m_parameters.useMipmaps)
	{
		deUint32 w = m_parameters.size.x();
		deUint32 h = m_parameters.size.y();

		DE_ASSERT(m_blockWidth > 0u && m_blockHeight > 0u);

		while (w > m_blockWidth && h > m_blockHeight)
		{
			w >>= 1;
			h >>= 1;

			if (w > m_blockWidth && h > m_blockHeight)
				levelCount++;
		}

		DE_ASSERT((m_parameters.size.x() >> (levelCount - 1u)) >= m_blockWidth);
		DE_ASSERT((m_parameters.size.y() >> (levelCount - 1u)) >= m_blockHeight);
	}

	return levelCount;
}

BasicTranscodingTestInstance::BasicTranscodingTestInstance (Context& context, const TestParameters& parameters)
	: TestInstance	(context)
	, m_parameters	(parameters)
	, m_blockWidth	(getBlockWidth(m_parameters.formatCompressed))
	, m_blockHeight	(getBlockHeight(m_parameters.formatCompressed))
	, m_levelCount	(findMipMapLevelCount())
	, m_layerSize	(getLayerSize(m_parameters.imageType, m_parameters.size))
{
	DE_ASSERT(deLog2Floor32(m_parameters.size.x()) == deLog2Floor32(m_parameters.size.y()));
}

deUint32 BasicTranscodingTestInstance::getLevelCount()
{
	return m_levelCount;
}

deUint32 BasicTranscodingTestInstance::getLayerCount()
{
	return m_parameters.size.z();
}

UVec3 BasicTranscodingTestInstance::getLayerDims()
{
	return m_layerSize;
}

vector<UVec3> BasicTranscodingTestInstance::getMipLevelSizes (UVec3 baseSize)
{
	vector<UVec3>	levelSizes;
	const deUint32	levelCount = getLevelCount();

	DE_ASSERT(m_parameters.imageType == IMAGE_TYPE_2D || m_parameters.imageType == IMAGE_TYPE_2D_ARRAY);

	baseSize.z() = 1u;

	levelSizes.push_back(baseSize);

	while (levelSizes.size() < levelCount && (baseSize.x() != 1 || baseSize.y() != 1))
	{
		baseSize.x() = deMax32(baseSize.x() >> 1, 1);
		baseSize.y() = deMax32(baseSize.y() >> 1, 1);
		levelSizes.push_back(baseSize);
	}

	DE_ASSERT(levelSizes.size() == getLevelCount());

	return levelSizes;
}

vector<UVec3> BasicTranscodingTestInstance::getCompressedMipLevelSizes (const VkFormat compressedFormat, const vector<UVec3>& uncompressedSizes)
{
	vector<UVec3> levelSizes;
	vector<UVec3>::const_iterator it;

	for (it = uncompressedSizes.begin(); it != uncompressedSizes.end(); it++)
		levelSizes.push_back(getCompressedImageResolutionInBlocks(compressedFormat, *it));

	return levelSizes;
}

void BasicTranscodingTestInstance::generateData (deUint8*		toFill,
												 const size_t	size,
												 const VkFormat format,
												 const deUint32 layer,
												 const deUint32 level)
{
	const deUint8 pattern[] =
	{
		// 64-bit values
		0x11, 0x11, 0x11, 0x11, 0x22, 0x22, 0x22, 0x22,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
		0x7F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		// Positive infinity
		0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		// Negative infinity
		0x7F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,		// Start of a signalling NaN (NANS)
		0x7F, 0xF7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,		// End of a signalling NaN (NANS)
		0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,		// Start of a signalling NaN (NANS)
		0xFF, 0xF7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,		// End of a signalling NaN (NANS)
		0x7F, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		// Start of a quiet NaN (NANQ)
		0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,		// End of of a quiet NaN (NANQ)
		0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		// Start of a quiet NaN (NANQ)
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,		// End of a quiet NaN (NANQ)
		// 32-bit values
		0x7F, 0x80, 0x00, 0x00,								// Positive infinity
		0xFF, 0x80, 0x00, 0x00,								// Negative infinity
		0x7F, 0x80, 0x00, 0x01,								// Start of a signalling NaN (NANS)
		0x7F, 0xBF, 0xFF, 0xFF,								// End of a signalling NaN (NANS)
		0xFF, 0x80, 0x00, 0x01,								// Start of a signalling NaN (NANS)
		0xFF, 0xBF, 0xFF, 0xFF,								// End of a signalling NaN (NANS)
		0x7F, 0xC0, 0x00, 0x00,								// Start of a quiet NaN (NANQ)
		0x7F, 0xFF, 0xFF, 0xFF,								// End of of a quiet NaN (NANQ)
		0xFF, 0xC0, 0x00, 0x00,								// Start of a quiet NaN (NANQ)
		0xFF, 0xFF, 0xFF, 0xFF,								// End of a quiet NaN (NANQ)
		0xAA, 0xAA, 0xAA, 0xAA,
		0x55, 0x55, 0x55, 0x55,
	};

	deUint8*	start		= toFill;
	size_t		sizeToRnd	= size;

	// Pattern part
	if (layer == 0 && level == 0 && size >= 2 * sizeof(pattern))
	{
		// Rotated pattern
		for (size_t i = 0; i < sizeof(pattern); i++)
			start[sizeof(pattern) - i - 1] = pattern[i];

		start		+= sizeof(pattern);
		sizeToRnd	-= sizeof(pattern);

		// Direct pattern
		deMemcpy(start, pattern, sizeof(pattern));

		start		+= sizeof(pattern);
		sizeToRnd	-= sizeof(pattern);
	}

	// Random part
	{
		DE_ASSERT(sizeToRnd % sizeof(deUint32) == 0);

		deUint32*	start32		= reinterpret_cast<deUint32*>(start);
		size_t		sizeToRnd32	= sizeToRnd / sizeof(deUint32);
		deUint32	seed		= (layer << 24) ^ (level << 16) ^ static_cast<deUint32>(format);
		Random		rnd			(seed);

		for (size_t i = 0; i < sizeToRnd32; i++)
			start32[i] = rnd.getUint32();
	}

	{
		// Remove certain values that may not be preserved based on the uncompressed view format
		if (isSnormFormat(m_parameters.formatUncompressed))
		{
			for (size_t i = 0; i < size; i += 2)
			{
				// SNORM fix: due to write operation in SNORM format
				// replaces 0x00 0x80 to 0x01 0x80
				if (toFill[i] == 0x00 && toFill[i+1] == 0x80)
					toFill[i+1] = 0x81;
			}
		}
		else if (isFloatFormat(m_parameters.formatUncompressed))
		{
			tcu::TextureFormat textureFormat = mapVkFormat(m_parameters.formatUncompressed);

			if (textureFormat.type == tcu::TextureFormat::HALF_FLOAT)
			{
				for (size_t i = 0; i < size; i += 2)
				{
					// HALF_FLOAT fix: remove INF and NaN
					if ((toFill[i+1] & 0x7C) == 0x7C)
						toFill[i+1] = 0x00;
				}
			}
			else if (textureFormat.type == tcu::TextureFormat::FLOAT)
			{
				for (size_t i = 0; i < size; i += 4)
				{
					// HALF_FLOAT fix: remove INF and NaN
					if ((toFill[i+1] & 0x7C) == 0x7C)
						toFill[i+1] = 0x00;
				}

				for (size_t i = 0; i < size; i += 4)
				{
					// FLOAT fix: remove INF, NaN, and denorm
					// Little endian fix
					if (((toFill[i+3] & 0x7F) == 0x7F && (toFill[i+2] & 0x80) == 0x80) || ((toFill[i+3] & 0x7F) == 0x00 && (toFill[i+2] & 0x80) == 0x00))
						toFill[i+3] = 0x01;
					// Big endian fix
					if (((toFill[i+0] & 0x7F) == 0x7F && (toFill[i+1] & 0x80) == 0x80) || ((toFill[i+0] & 0x7F) == 0x00 && (toFill[i+1] & 0x80) == 0x00))
						toFill[i+0] = 0x01;
				}
			}
		}
	}
}

class BasicComputeTestInstance : public BasicTranscodingTestInstance
{
public:
					BasicComputeTestInstance	(Context&							context,
												const TestParameters&				parameters);
	TestStatus		iterate						(void);
protected:
	struct ImageData
	{
		deUint32			getImagesCount		(void)									{ return static_cast<deUint32>(images.size());		}
		deUint32			getImageViewCount	(void)									{ return static_cast<deUint32>(imagesViews.size());	}
		deUint32			getImageInfoCount	(void)									{ return static_cast<deUint32>(imagesInfos.size());	}
		VkImage				getImage			(const deUint32				ndx)		{ return **images[ndx]->get();						}
		VkImageView			getImageView		(const deUint32				ndx)		{ return **imagesViews[ndx];						}
		VkImageCreateInfo	getImageInfo		(const deUint32				ndx)		{ return imagesInfos[ndx];							}
		void				addImage			(MovePtr<Image>				image)		{ images.push_back(makeVkSharedPtr(image));			}
		void				addImageView		(Move<VkImageView>			imageView)	{ imagesViews.push_back(makeVkSharedPtr(imageView));}
		void				addImageInfo		(const VkImageCreateInfo	imageInfo)	{ imagesInfos.push_back(imageInfo);					}
		void				resetViews			()										{ imagesViews.clear();								}
	private:
		vector<ImageSp>				images;
		vector<ImageViewSp>			imagesViews;
		vector<VkImageCreateInfo>	imagesInfos;
	};
	void			copyDataToImage				(const VkCommandBuffer&				cmdBuffer,
												 ImageData&							imageData,
												 const vector<UVec3>&				mipMapSizes,
												 const bool							isCompressed);
	virtual void	executeShader				(const VkCommandBuffer&				cmdBuffer,
												 const VkDescriptorSetLayout&		descriptorSetLayout,
												 const VkDescriptorPool&			descriptorPool,
												vector<ImageData>&					imageData);
	bool			copyResultAndCompare		(const VkCommandBuffer&				cmdBuffer,
												 const VkImage&						uncompressed,
												 const VkDeviceSize					offset,
												 const UVec3&						size);
	void			descriptorSetUpdate			(VkDescriptorSet					descriptorSet,
												 const VkDescriptorImageInfo*		descriptorImageInfos);
	void			createImageInfos			(ImageData&							imageData,
												 const vector<UVec3>&				mipMapSizes,
												 const bool							isCompressed);
	bool			decompressImage				(const VkCommandBuffer&				cmdBuffer,
												 vector<ImageData>&					imageData,
												 const vector<UVec3>&				mipMapSizes);
	vector<deUint8>	m_data;
};


BasicComputeTestInstance::BasicComputeTestInstance (Context& context, const TestParameters& parameters)
	:BasicTranscodingTestInstance	(context, parameters)
{
}

TestStatus BasicComputeTestInstance::iterate (void)
{
	const DeviceInterface&					vk					= m_context.getDeviceInterface();
	const VkDevice							device				= m_context.getDevice();
	const deUint32							queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&								allocator			= m_context.getDefaultAllocator();
	const Unique<VkCommandPool>				cmdPool				(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>			cmdBuffer			(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const vector<UVec3>						mipMapSizes			= m_parameters.useMipmaps ? getMipLevelSizes (getLayerDims()) : vector<UVec3>(1, m_parameters.size);
	vector<ImageData>						imageData			(m_parameters.imagesCount);
	const deUint32							compressedNdx		= 0u;
	const deUint32							resultImageNdx		= m_parameters.imagesCount -1u;

	for (deUint32 imageNdx = 0u; imageNdx < m_parameters.imagesCount; ++imageNdx)
	{
		const bool isCompressed = compressedNdx == imageNdx ? true : false;
		createImageInfos(imageData[imageNdx], mipMapSizes, isCompressed);
		for (deUint32 infoNdx = 0u; infoNdx < imageData[imageNdx].getImageInfoCount(); ++infoNdx)
		{
			imageData[imageNdx].addImage(MovePtr<Image>(new Image(vk, device, allocator, imageData[imageNdx].getImageInfo(infoNdx), MemoryRequirement::Any)));
			if (isCompressed)
			{
				const VkImageViewUsageCreateInfoKHR	imageViewUsageKHR	=
				{
					VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO_KHR,				//VkStructureType		sType;
					DE_NULL,														//const void*			pNext;
					m_parameters.compressedImageUsage,								//VkImageUsageFlags		usage;
				};
				for (deUint32 mipNdx = 0u; mipNdx < mipMapSizes.size(); ++mipNdx)
				for (deUint32 layerNdx = 0u; layerNdx < getLayerCount(); ++layerNdx)
				{
					imageData[imageNdx].addImageView(makeImageView(vk, device, imageData[imageNdx].getImage(infoNdx),
														mapImageViewType(m_parameters.imageType), m_parameters.formatUncompressed,
														makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, mipNdx, 1u, layerNdx, 1u),
														&imageViewUsageKHR));
				}
			}
			else
			{
				imageData[imageNdx].addImageView(makeImageView(vk, device, imageData[imageNdx].getImage(infoNdx),
													mapImageViewType(m_parameters.imageType), m_parameters.formatUncompressed,
													makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u)));
			}
		}
	}

	{
		size_t size = 0ull;
		for(deUint32 mipNdx = 0u; mipNdx < mipMapSizes.size(); ++mipNdx)
		{
			size += static_cast<size_t>(getCompressedImageSizeInBytes(m_parameters.formatCompressed, mipMapSizes[mipNdx]) * getLayerCount());
		}
		m_data.resize(size);
		generateData (&m_data[0], m_data.size(), m_parameters.formatCompressed);
	}

	switch(m_parameters.operation)
	{
		case OPERATION_IMAGE_LOAD:
		case OPERATION_TEXEL_FETCH:
		case OPERATION_TEXTURE:
			copyDataToImage(*cmdBuffer, imageData[compressedNdx], mipMapSizes, true);
			break;
		case OPERATION_IMAGE_STORE:
			copyDataToImage(*cmdBuffer, imageData[1], mipMapSizes, false);
			break;
		default:
			DE_ASSERT(false);
			break;
	}

	{
		Move<VkDescriptorSetLayout>	descriptorSetLayout;
		Move<VkDescriptorPool>		descriptorPool;

		DescriptorSetLayoutBuilder	descriptorSetLayoutBuilder;
		DescriptorPoolBuilder		descriptorPoolBuilder;
		for (deUint32 imageNdx = 0u; imageNdx < m_parameters.imagesCount; ++imageNdx)
		{
			switch(m_parameters.operation)
			{
				case OPERATION_IMAGE_LOAD:
				case OPERATION_IMAGE_STORE:
					descriptorSetLayoutBuilder.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
					descriptorPoolBuilder.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, imageData[0].getImageViewCount());
					break;
				case OPERATION_TEXEL_FETCH:
				case OPERATION_TEXTURE:
					descriptorSetLayoutBuilder.addSingleBinding((compressedNdx == imageNdx) ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
					descriptorPoolBuilder.addType((compressedNdx == imageNdx) ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, imageData[0].getImageViewCount());
					break;
				default:
					DE_ASSERT(false);
					break;
			}
		}
		descriptorSetLayout	= descriptorSetLayoutBuilder.build(vk, device);
		descriptorPool		= descriptorPoolBuilder.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, imageData[0].getImageViewCount());
		executeShader(*cmdBuffer, *descriptorSetLayout, *descriptorPool, imageData);

		{
			VkDeviceSize offset = 0ull;
			for (deUint32 mipNdx = 0u; mipNdx < mipMapSizes.size(); ++mipNdx)
			for (deUint32 layerNdx = 0u; layerNdx < getLayerCount(); ++layerNdx)
			{
				const deUint32	imageNdx	= layerNdx + mipNdx * getLayerCount();
				const UVec3		size		= UVec3(imageData[resultImageNdx].getImageInfo(imageNdx).extent.width,
													imageData[resultImageNdx].getImageInfo(imageNdx).extent.height,
													imageData[resultImageNdx].getImageInfo(imageNdx).extent.depth);
				if (!copyResultAndCompare(*cmdBuffer, imageData[resultImageNdx].getImage(imageNdx), offset, size))
					return TestStatus::fail("Fail");
				offset += getCompressedImageSizeInBytes(m_parameters.formatCompressed, mipMapSizes[mipNdx]);
			}
		}
	};
	if (!decompressImage(*cmdBuffer, imageData, mipMapSizes))
			return TestStatus::fail("Fail");
	return TestStatus::pass("Pass");
}

void BasicComputeTestInstance::copyDataToImage (const VkCommandBuffer&	cmdBuffer,
												ImageData&				imageData,
												const vector<UVec3>&	mipMapSizes,
												const bool				isCompressed)
{
	const DeviceInterface&		vk			= m_context.getDeviceInterface();
	const VkDevice				device		= m_context.getDevice();
	const VkQueue				queue		= m_context.getUniversalQueue();
	Allocator&					allocator	= m_context.getDefaultAllocator();

	Buffer						imageBuffer	(vk, device, allocator,
												makeBufferCreateInfo(m_data.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
												MemoryRequirement::HostVisible);
	VkDeviceSize				offset		= 0ull;
	{
		const Allocation& alloc = imageBuffer.getAllocation();
		deMemcpy(alloc.getHostPtr(), &m_data[0], m_data.size());
		flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), m_data.size());
	}

	beginCommandBuffer(vk, cmdBuffer);
	const VkImageSubresourceRange	subresourceRange		=
	{
		VK_IMAGE_ASPECT_COLOR_BIT,					//VkImageAspectFlags	aspectMask
		0u,											//deUint32				baseMipLevel
		imageData.getImageInfo(0u).mipLevels,		//deUint32				levelCount
		0u,											//deUint32				baseArrayLayer
		imageData.getImageInfo(0u).arrayLayers		//deUint32				layerCount
	};

	for (deUint32 imageNdx = 0u; imageNdx < imageData.getImagesCount(); ++imageNdx)
	{
		const VkImageMemoryBarrier		preCopyImageBarrier		= makeImageMemoryBarrier(
																	0u, VK_ACCESS_TRANSFER_WRITE_BIT,
																	VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
																	imageData.getImage(imageNdx), subresourceRange);

		const VkBufferMemoryBarrier		FlushHostCopyBarrier	= makeBufferMemoryBarrier(
																	VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
																	imageBuffer.get(), 0ull, m_data.size());

		vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				(VkDependencyFlags)0, 0u, (const VkMemoryBarrier*)DE_NULL, 1u, &FlushHostCopyBarrier, 1u, &preCopyImageBarrier);

		for (deUint32 mipNdx = 0u; mipNdx < imageData.getImageInfo(imageNdx).mipLevels; ++mipNdx)
		{
			const VkExtent3D				imageExtent				= isCompressed ?
																		makeExtent3D(mipMapSizes[mipNdx]) :
																		imageData.getImageInfo(imageNdx).extent;
			const VkBufferImageCopy			copyRegion				=
			{
				offset,																												//VkDeviceSize				bufferOffset;
				0u,																													//deUint32					bufferRowLength;
				0u,																													//deUint32					bufferImageHeight;
				makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, mipNdx, 0u, imageData.getImageInfo(imageNdx).arrayLayers),	//VkImageSubresourceLayers	imageSubresource;
				makeOffset3D(0, 0, 0),																								//VkOffset3D				imageOffset;
				imageExtent,																										//VkExtent3D				imageExtent;
			};

			vk.cmdCopyBufferToImage(cmdBuffer, imageBuffer.get(), imageData.getImage(imageNdx), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &copyRegion);
			offset += getCompressedImageSizeInBytes(m_parameters.formatCompressed,
						UVec3(isCompressed ? imageExtent.width : imageExtent.width * m_blockWidth, isCompressed? imageExtent.height :imageExtent.height * m_blockHeight,imageExtent.depth)) *
						imageData.getImageInfo(imageNdx).arrayLayers;
		}
	}
	endCommandBuffer(vk, cmdBuffer);
	submitCommandsAndWait(vk, device, queue, cmdBuffer);
}

void BasicComputeTestInstance::executeShader (const VkCommandBuffer&		cmdBuffer,
											  const VkDescriptorSetLayout&	descriptorSetLayout,
											  const VkDescriptorPool&		descriptorPool,
											  vector<ImageData>&			imageData)
{
	const DeviceInterface&			vk						= m_context.getDeviceInterface();
	const VkDevice					device					= m_context.getDevice();
	const VkQueue					queue					= m_context.getUniversalQueue();
	const Unique<VkShaderModule>	shaderModule			(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0));
	vector<SharedVkDescriptorSet>	descriptorSets			(imageData[0].getImageViewCount());
	const Unique<VkPipelineLayout>	pipelineLayout			(makePipelineLayout(vk, device, descriptorSetLayout));
	const Unique<VkPipeline>		pipeline				(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule));
	Move<VkSampler>					sampler;
	{
		const VkSamplerCreateInfo createInfo =
		{
			VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,		//VkStructureType		sType;
			DE_NULL,									//const void*			pNext;
			0u,											//VkSamplerCreateFlags	flags;
			VK_FILTER_NEAREST,							//VkFilter				magFilter;
			VK_FILTER_NEAREST,							//VkFilter				minFilter;
			VK_SAMPLER_MIPMAP_MODE_NEAREST,				//VkSamplerMipmapMode	mipmapMode;
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		//VkSamplerAddressMode	addressModeU;
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		//VkSamplerAddressMode	addressModeV;
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		//VkSamplerAddressMode	addressModeW;
			0.0f,										//float					mipLodBias;
			VK_FALSE,									//VkBool32				anisotropyEnable;
			1.0f,										//float					maxAnisotropy;
			VK_FALSE,									//VkBool32				compareEnable;
			VK_COMPARE_OP_EQUAL,						//VkCompareOp			compareOp;
			0.0f,										//float					minLod;
			0.0f,										//float					maxLod;
			VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,	//VkBorderColor			borderColor;
			VK_FALSE,									//VkBool32				unnormalizedCoordinates;
		};
		sampler = createSampler(vk, device, &createInfo);
	}

	vector<VkDescriptorImageInfo>	descriptorImageInfos	(descriptorSets.size() * m_parameters.imagesCount);
	for (deUint32 viewNdx = 0u; viewNdx < descriptorSets.size(); ++viewNdx)
	{
		const deUint32 descriptorNdx = viewNdx * m_parameters.imagesCount;
		for (deUint32 imageNdx = 0; imageNdx < m_parameters.imagesCount; ++imageNdx)
		{
			descriptorImageInfos[descriptorNdx+imageNdx] = makeDescriptorImageInfo(*sampler,
															imageData[imageNdx].getImageView(viewNdx), VK_IMAGE_LAYOUT_GENERAL);
		}
	}

	for (deUint32 ndx = 0u; ndx < descriptorSets.size(); ++ndx)
		descriptorSets[ndx] = makeVkSharedPtr(makeDescriptorSet(vk, device, descriptorPool, descriptorSetLayout));

	beginCommandBuffer(vk, cmdBuffer);
	{
		const VkImageSubresourceRange	compressedRange				=
		{
			VK_IMAGE_ASPECT_COLOR_BIT,					//VkImageAspectFlags	aspectMask
			0u,											//deUint32				baseMipLevel
			imageData[0].getImageInfo(0u).mipLevels,	//deUint32				levelCount
			0u,											//deUint32				baseArrayLayer
			imageData[0].getImageInfo(0u).arrayLayers	//deUint32				layerCount
		};
		const VkImageSubresourceRange	uncompressedRange			=
		{
			VK_IMAGE_ASPECT_COLOR_BIT,					//VkImageAspectFlags	aspectMask
			0u,											//deUint32				baseMipLevel
			1u,											//deUint32				levelCount
			0u,											//deUint32				baseArrayLayer
			1u											//deUint32				layerCount
		};

		vk.cmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);

		vector<VkImageMemoryBarrier>		preShaderImageBarriers;
		preShaderImageBarriers.resize(descriptorSets.size() + 1u);
		for (deUint32 imageNdx = 0u; imageNdx < imageData[1].getImagesCount(); ++imageNdx)
		{
			preShaderImageBarriers[imageNdx]= makeImageMemoryBarrier(
												VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT,
												VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
												imageData[1].getImage(imageNdx), uncompressedRange);
		}

		preShaderImageBarriers[descriptorSets.size()] = makeImageMemoryBarrier(
															VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
															VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
															imageData[0].getImage(0), compressedRange);

		vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			(VkDependencyFlags)0, 0u, (const VkMemoryBarrier*)DE_NULL, 0u, (const VkBufferMemoryBarrier*)DE_NULL,
			static_cast<deUint32>(preShaderImageBarriers.size()), &preShaderImageBarriers[0]);

		for (deUint32 ndx = 0u; ndx <descriptorSets.size(); ++ndx)
		{
			descriptorSetUpdate (**descriptorSets[ndx], &descriptorImageInfos[ndx* m_parameters.imagesCount]);
			vk.cmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &(**descriptorSets[ndx]), 0u, DE_NULL);
			vk.cmdDispatch(cmdBuffer,	imageData[1].getImageInfo(ndx).extent.width,
										imageData[1].getImageInfo(ndx).extent.height,
										imageData[1].getImageInfo(ndx).extent.depth);
		}
	}
	endCommandBuffer(vk, cmdBuffer);
	submitCommandsAndWait(vk, device, queue, cmdBuffer);
}

bool BasicComputeTestInstance::copyResultAndCompare (const VkCommandBuffer&	cmdBuffer,
													 const VkImage&			uncompressed,
													 const VkDeviceSize		offset,
													 const UVec3&			size)
{
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const VkDevice			device				= m_context.getDevice();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	VkDeviceSize			imageResultSize		= getImageSizeBytes (tcu::IVec3(size.x(), size.y(), size.z()), m_parameters.formatUncompressed);
	Buffer					imageBufferResult	(vk, device, allocator,
													makeBufferCreateInfo(imageResultSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT),
													MemoryRequirement::HostVisible);

	beginCommandBuffer(vk, cmdBuffer);
	{
		const VkImageSubresourceRange	subresourceRange	=
		{
			VK_IMAGE_ASPECT_COLOR_BIT,											//VkImageAspectFlags	aspectMask
			0u,																	//deUint32				baseMipLevel
			1u,																	//deUint32				levelCount
			0u,																	//deUint32				baseArrayLayer
			1u																	//deUint32				layerCount
		};

		const VkBufferImageCopy			copyRegion			=
		{
			0ull,																//	VkDeviceSize				bufferOffset;
			0u,																	//	deUint32					bufferRowLength;
			0u,																	//	deUint32					bufferImageHeight;
			makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u),	//	VkImageSubresourceLayers	imageSubresource;
			makeOffset3D(0, 0, 0),												//	VkOffset3D					imageOffset;
			makeExtent3D(size),													//	VkExtent3D					imageExtent;
		};

		const VkImageMemoryBarrier prepareForTransferBarrier = makeImageMemoryBarrier(
																VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
																VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
																uncompressed, subresourceRange);

		const VkBufferMemoryBarrier copyBarrier = makeBufferMemoryBarrier(
													VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT,
													imageBufferResult.get(), 0ull, imageResultSize);

		vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1u, &prepareForTransferBarrier);
		vk.cmdCopyImageToBuffer(cmdBuffer, uncompressed, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, imageBufferResult.get(), 1u, &copyRegion);
		vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0u, (const VkMemoryBarrier*)DE_NULL, 1, &copyBarrier, 0u, (const VkImageMemoryBarrier*)DE_NULL);
	}
	endCommandBuffer(vk, cmdBuffer);
	submitCommandsAndWait(vk, device, queue, cmdBuffer);

	const Allocation& allocResult = imageBufferResult.getAllocation();
	invalidateMappedMemoryRange(vk, device, allocResult.getMemory(), allocResult.getOffset(), imageResultSize);
	if (deMemCmp((const void *)allocResult.getHostPtr(), (const void *)&m_data[static_cast<size_t>(offset)], static_cast<size_t>(imageResultSize)) == 0ull)
		return true;
	return false;
}

void BasicComputeTestInstance::descriptorSetUpdate (VkDescriptorSet descriptorSet, const VkDescriptorImageInfo* descriptorImageInfos)
{
	const DeviceInterface&		vk		= m_context.getDeviceInterface();
	const VkDevice				device	= m_context.getDevice();
	DescriptorSetUpdateBuilder	descriptorSetUpdateBuilder;

	switch(m_parameters.operation)
	{
		case OPERATION_IMAGE_LOAD:
		case OPERATION_IMAGE_STORE:
		{
			for (deUint32 bindingNdx = 0u; bindingNdx < m_parameters.imagesCount; ++bindingNdx)
				descriptorSetUpdateBuilder.writeSingle(descriptorSet, DescriptorSetUpdateBuilder::Location::binding(bindingNdx), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descriptorImageInfos[bindingNdx]);

			break;
		}

		case OPERATION_TEXEL_FETCH:
		case OPERATION_TEXTURE:
		{
			for (deUint32 bindingNdx = 0u; bindingNdx < m_parameters.imagesCount; ++bindingNdx)
			{
				descriptorSetUpdateBuilder.writeSingle(descriptorSet, DescriptorSetUpdateBuilder::Location::binding(bindingNdx),
					bindingNdx == 0u ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descriptorImageInfos[bindingNdx]);
			}

			break;
		}

		default:
			DE_ASSERT(false);
	}
	descriptorSetUpdateBuilder.update(vk, device);
}

void BasicComputeTestInstance::createImageInfos (ImageData& imageData, const vector<UVec3>& mipMapSizes, const bool isCompressed)
{
	const VkImageType			imageType			= mapImageType(m_parameters.imageType);

	if (isCompressed)
	{
		const VkExtent3D			extentCompressed	= makeExtent3D(getLayerSize(m_parameters.imageType, m_parameters.size));
		const VkImageCreateInfo compressedInfo =
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,					// VkStructureType			sType;
			DE_NULL,												// const void*				pNext;
			VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT |
			VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT_KHR |
			VK_IMAGE_CREATE_EXTENDED_USAGE_BIT_KHR,					// VkImageCreateFlags		flags;
			imageType,												// VkImageType				imageType;
			m_parameters.formatCompressed,							// VkFormat					format;
			extentCompressed,										// VkExtent3D				extent;
			static_cast<deUint32>(mipMapSizes.size()),				// deUint32					mipLevels;
			getLayerCount(),										// deUint32					arrayLayers;
			VK_SAMPLE_COUNT_1_BIT,									// VkSampleCountFlagBits	samples;
			VK_IMAGE_TILING_OPTIMAL,								// VkImageTiling			tiling;
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_STORAGE_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT,						// VkImageUsageFlags		usage;
			VK_SHARING_MODE_EXCLUSIVE,								// VkSharingMode			sharingMode;
			0u,														// deUint32					queueFamilyIndexCount;
			DE_NULL,												// const deUint32*			pQueueFamilyIndices;
			VK_IMAGE_LAYOUT_UNDEFINED,								// VkImageLayout			initialLayout;
		};
		imageData.addImageInfo(compressedInfo);
	}
	else
	{
		for (size_t mipNdx = 0ull; mipNdx < mipMapSizes.size(); ++mipNdx)
		for (size_t layerNdx = 0ull; layerNdx < getLayerCount(); ++layerNdx)
		{
			const VkExtent3D		extentUncompressed	= m_parameters.useMipmaps ?
															makeExtent3D(getCompressedImageResolutionInBlocks(m_parameters.formatCompressed, mipMapSizes[mipNdx])) :
															makeExtent3D(getCompressedImageResolutionInBlocks(m_parameters.formatCompressed, m_parameters.size));
			const VkImageCreateInfo	uncompressedInfo	=
			{
				VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,				// VkStructureType			sType;
				DE_NULL,											// const void*				pNext;
				0u,													// VkImageCreateFlags		flags;
				imageType,											// VkImageType				imageType;
				m_parameters.formatUncompressed,					// VkFormat					format;
				extentUncompressed,									// VkExtent3D				extent;
				1u,													// deUint32					mipLevels;
				1u,													// deUint32					arrayLayers;
				VK_SAMPLE_COUNT_1_BIT,								// VkSampleCountFlagBits	samples;
				VK_IMAGE_TILING_OPTIMAL,							// VkImageTiling			tiling;
				m_parameters.uncompressedImageUsage |
				VK_IMAGE_USAGE_SAMPLED_BIT,							// VkImageUsageFlags		usage;
				VK_SHARING_MODE_EXCLUSIVE,							// VkSharingMode			sharingMode;
				0u,													// deUint32					queueFamilyIndexCount;
				DE_NULL,											// const deUint32*			pQueueFamilyIndices;
				VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout			initialLayout;
			};
			imageData.addImageInfo(uncompressedInfo);
		}
	}
}

bool BasicComputeTestInstance::decompressImage (const VkCommandBuffer&	cmdBuffer,
												 vector<ImageData>&		imageData,
												 const vector<UVec3>&	mipMapSizes)
{
	const DeviceInterface&			vk						= m_context.getDeviceInterface();
	const VkDevice					device					= m_context.getDevice();
	const VkQueue					queue					= m_context.getUniversalQueue();
	Allocator&						allocator				= m_context.getDefaultAllocator();
	const Unique<VkShaderModule>	shaderModule			(createShaderModule(vk, device, m_context.getBinaryCollection().get("decompress"), 0));
	const VkImage&					compressed				= imageData[0].getImage(0);

	for (deUint32 ndx = 0u; ndx < imageData.size(); ndx++)
		imageData[ndx].resetViews();

	for (deUint32 mipNdx = 0u; mipNdx < mipMapSizes.size(); ++mipNdx)
	for (deUint32 layerNdx = 0u; layerNdx < getLayerCount(); ++layerNdx)
	{
		const bool						layoutShaderReadOnly	= (layerNdx % 2u) == 1;
		const deUint32					imageNdx				= layerNdx + mipNdx * getLayerCount();
		const VkExtent3D				extentCompressed		= makeExtent3D(mipMapSizes[mipNdx]);
		const VkImage&					uncompressed			= imageData[m_parameters.imagesCount -1].getImage(imageNdx);
		const VkExtent3D				extentUncompressed		= imageData[m_parameters.imagesCount -1].getImageInfo(imageNdx).extent;
		const VkDeviceSize				bufferSizeComp			= getCompressedImageSizeInBytes(m_parameters.formatCompressed, mipMapSizes[mipNdx]);

		const VkImageCreateInfo			decompressedImageInfo	=
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,								// VkStructureType			sType;
			DE_NULL,															// const void*				pNext;
			0u,																	// VkImageCreateFlags		flags;
			VK_IMAGE_TYPE_2D,													// VkImageType				imageType;
			VK_FORMAT_R8G8B8A8_UNORM,											// VkFormat					format;
			extentCompressed,													// VkExtent3D				extent;
			1u,																	// deUint32					mipLevels;
			1u,																	// deUint32					arrayLayers;
			VK_SAMPLE_COUNT_1_BIT,												// VkSampleCountFlagBits	samples;
			VK_IMAGE_TILING_OPTIMAL,											// VkImageTiling			tiling;
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_STORAGE_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT,									// VkImageUsageFlags		usage;
			VK_SHARING_MODE_EXCLUSIVE,											// VkSharingMode			sharingMode;
			0u,																	// deUint32					queueFamilyIndexCount;
			DE_NULL,															// const deUint32*			pQueueFamilyIndices;
			VK_IMAGE_LAYOUT_UNDEFINED,											// VkImageLayout			initialLayout;
		};

		const VkImageCreateInfo			compressedImageInfo		=
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,								// VkStructureType			sType;
			DE_NULL,															// const void*				pNext;
			0u,																	// VkImageCreateFlags		flags;
			VK_IMAGE_TYPE_2D,													// VkImageType				imageType;
			m_parameters.formatCompressed,										// VkFormat					format;
			extentCompressed,													// VkExtent3D				extent;
			1u,																	// deUint32					mipLevels;
			1u,																	// deUint32					arrayLayers;
			VK_SAMPLE_COUNT_1_BIT,												// VkSampleCountFlagBits	samples;
			VK_IMAGE_TILING_OPTIMAL,											// VkImageTiling			tiling;
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT,									// VkImageUsageFlags		usage;
			VK_SHARING_MODE_EXCLUSIVE,											// VkSharingMode			sharingMode;
			0u,																	// deUint32					queueFamilyIndexCount;
			DE_NULL,															// const deUint32*			pQueueFamilyIndices;
			VK_IMAGE_LAYOUT_UNDEFINED,											// VkImageLayout			initialLayout;
		};
		const VkImageUsageFlags				compressedViewUsageFlags	= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		const VkImageViewUsageCreateInfoKHR	compressedViewUsageCI		=
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO_KHR,					//VkStructureType		sType;
			DE_NULL,															//const void*			pNext;
			compressedViewUsageFlags,											//VkImageUsageFlags		usage;
		};
		Image							resultImage				(vk, device, allocator, decompressedImageInfo, MemoryRequirement::Any);
		Image							referenceImage			(vk, device, allocator, decompressedImageInfo, MemoryRequirement::Any);
		Image							uncompressedImage		(vk, device, allocator, compressedImageInfo, MemoryRequirement::Any);
		Move<VkImageView>				resultView				= makeImageView(vk, device, resultImage.get(), mapImageViewType(m_parameters.imageType), decompressedImageInfo.format,
																	makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, decompressedImageInfo.extent.depth, 0u, decompressedImageInfo.arrayLayers));
		Move<VkImageView>				referenceView			= makeImageView(vk, device, referenceImage.get(), mapImageViewType(m_parameters.imageType), decompressedImageInfo.format,
																	makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, decompressedImageInfo.extent.depth, 0u, decompressedImageInfo.arrayLayers));
		Move<VkImageView>				uncompressedView		= makeImageView(vk, device, uncompressedImage.get(), mapImageViewType(m_parameters.imageType), m_parameters.formatCompressed,
																	makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, compressedImageInfo.extent.depth, 0u, compressedImageInfo.arrayLayers));
		Move<VkImageView>				compressedView			= makeImageView(vk, device, compressed, mapImageViewType(m_parameters.imageType), m_parameters.formatCompressed,
																	makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, mipNdx, 1u, layerNdx, 1u), &compressedViewUsageCI);
		Move<VkDescriptorSetLayout>		descriptorSetLayout		= DescriptorSetLayoutBuilder()
																	.addSingleBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
																	.addSingleBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
																	.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
																	.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
																	.build(vk, device);
		Move<VkDescriptorPool>			descriptorPool			= DescriptorPoolBuilder()
																	.addType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, decompressedImageInfo.arrayLayers)
																	.addType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, decompressedImageInfo.arrayLayers)
																	.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, decompressedImageInfo.arrayLayers)
																	.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, decompressedImageInfo.arrayLayers)
																	.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, decompressedImageInfo.arrayLayers);

		Move<VkDescriptorSet>			descriptorSet			= makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout);
		const Unique<VkPipelineLayout>	pipelineLayout			(makePipelineLayout(vk, device, *descriptorSetLayout));
		const Unique<VkPipeline>		pipeline				(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule));
		const VkDeviceSize				bufferSize				= getImageSizeBytes(IVec3((int)extentCompressed.width, (int)extentCompressed.height, (int)extentCompressed.depth), VK_FORMAT_R8G8B8A8_UNORM);
		Buffer							resultBuffer			(vk, device, allocator,
																	makeBufferCreateInfo(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT), MemoryRequirement::HostVisible);
		Buffer							referenceBuffer			(vk, device, allocator,
																	makeBufferCreateInfo(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT), MemoryRequirement::HostVisible);
		Buffer							transferBuffer			(vk, device, allocator,
																	makeBufferCreateInfo(bufferSizeComp, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT), MemoryRequirement::HostVisible);
		Move<VkSampler>					sampler;
		{
			const VkSamplerCreateInfo createInfo	=
			{
				VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,							//VkStructureType		sType;
				DE_NULL,														//const void*			pNext;
				0u,																//VkSamplerCreateFlags	flags;
				VK_FILTER_NEAREST,												//VkFilter				magFilter;
				VK_FILTER_NEAREST,												//VkFilter				minFilter;
				VK_SAMPLER_MIPMAP_MODE_NEAREST,									//VkSamplerMipmapMode	mipmapMode;
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,							//VkSamplerAddressMode	addressModeU;
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,							//VkSamplerAddressMode	addressModeV;
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,							//VkSamplerAddressMode	addressModeW;
				0.0f,															//float					mipLodBias;
				VK_FALSE,														//VkBool32				anisotropyEnable;
				1.0f,															//float					maxAnisotropy;
				VK_FALSE,														//VkBool32				compareEnable;
				VK_COMPARE_OP_EQUAL,											//VkCompareOp			compareOp;
				0.0f,															//float					minLod;
				1.0f,															//float					maxLod;
				VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,						//VkBorderColor			borderColor;
				VK_FALSE,														//VkBool32				unnormalizedCoordinates;
			};
			sampler = createSampler(vk, device, &createInfo);
		}

		VkDescriptorImageInfo			descriptorImageInfos[]	=
		{
			makeDescriptorImageInfo(*sampler,	*uncompressedView,	layoutShaderReadOnly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL),
			makeDescriptorImageInfo(*sampler,	*compressedView,	layoutShaderReadOnly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL),
			makeDescriptorImageInfo(DE_NULL,	*resultView,		VK_IMAGE_LAYOUT_GENERAL),
			makeDescriptorImageInfo(DE_NULL,	*referenceView,		VK_IMAGE_LAYOUT_GENERAL)
		};
		DescriptorSetUpdateBuilder()
			.writeSingle(descriptorSet.get(), DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &descriptorImageInfos[0])
			.writeSingle(descriptorSet.get(), DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &descriptorImageInfos[1])
			.writeSingle(descriptorSet.get(), DescriptorSetUpdateBuilder::Location::binding(2u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descriptorImageInfos[2])
			.writeSingle(descriptorSet.get(), DescriptorSetUpdateBuilder::Location::binding(3u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descriptorImageInfos[3])
			.update(vk, device);


		beginCommandBuffer(vk, cmdBuffer);
		{
			const VkImageSubresourceRange	subresourceRange		=
			{
				VK_IMAGE_ASPECT_COLOR_BIT,											//VkImageAspectFlags			aspectMask
				0u,																	//deUint32						baseMipLevel
				1u,																	//deUint32						levelCount
				0u,																	//deUint32						baseArrayLayer
				1u																	//deUint32						layerCount
			};

			const VkImageSubresourceRange	subresourceRangeComp	=
			{
				VK_IMAGE_ASPECT_COLOR_BIT,											//VkImageAspectFlags			aspectMask
				mipNdx,																//deUint32						baseMipLevel
				1u,																	//deUint32						levelCount
				layerNdx,															//deUint32						baseArrayLayer
				1u																	//deUint32						layerCount
			};

			const VkBufferImageCopy			copyRegion				=
			{
				0ull,																//	VkDeviceSize				bufferOffset;
				0u,																	//	deUint32					bufferRowLength;
				0u,																	//	deUint32					bufferImageHeight;
				makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u),	//	VkImageSubresourceLayers	imageSubresource;
				makeOffset3D(0, 0, 0),												//	VkOffset3D					imageOffset;
				decompressedImageInfo.extent,										//	VkExtent3D					imageExtent;
			};

			const VkBufferImageCopy			compressedCopyRegion	=
			{
				0ull,																//	VkDeviceSize				bufferOffset;
				0u,																	//	deUint32					bufferRowLength;
				0u,																	//	deUint32					bufferImageHeight;
				makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u),	//	VkImageSubresourceLayers	imageSubresource;
				makeOffset3D(0, 0, 0),												//	VkOffset3D					imageOffset;
				extentUncompressed,													//	VkExtent3D					imageExtent;
			};

			{

				const VkBufferMemoryBarrier		preCopyBufferBarriers	= makeBufferMemoryBarrier(0u, VK_ACCESS_TRANSFER_WRITE_BIT,
																			transferBuffer.get(), 0ull, bufferSizeComp);

				vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					(VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1u, &preCopyBufferBarriers, 0u, (const VkImageMemoryBarrier*)DE_NULL);
			}

			vk.cmdCopyImageToBuffer(cmdBuffer, uncompressed, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, transferBuffer.get(), 1u, &compressedCopyRegion);

			{
				const VkBufferMemoryBarrier		postCopyBufferBarriers	= makeBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
																			transferBuffer.get(), 0ull, bufferSizeComp);

				const VkImageMemoryBarrier		preCopyImageBarriers	= makeImageMemoryBarrier(0u, VK_ACCESS_TRANSFER_WRITE_BIT,
																			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, uncompressedImage.get(), subresourceRange);

				vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					(VkDependencyFlags)0, 0u, (const VkMemoryBarrier*)DE_NULL, 1u, &postCopyBufferBarriers, 1u, &preCopyImageBarriers);
			}

			vk.cmdCopyBufferToImage(cmdBuffer, transferBuffer.get(), uncompressedImage.get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &copyRegion);

			vk.cmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
			vk.cmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

			{
				const VkImageMemoryBarrier		preShaderImageBarriers[]	=
				{

					makeImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layoutShaderReadOnly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL,
						uncompressedImage.get(), subresourceRange),

					makeImageMemoryBarrier(0, VK_ACCESS_SHADER_READ_BIT,
						VK_IMAGE_LAYOUT_GENERAL, layoutShaderReadOnly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL,
						compressed, subresourceRangeComp),

					makeImageMemoryBarrier(0u, VK_ACCESS_SHADER_WRITE_BIT,
						VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
						resultImage.get(), subresourceRange),

					makeImageMemoryBarrier(0u, VK_ACCESS_SHADER_WRITE_BIT,
						VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
						referenceImage.get(), subresourceRange)
				};

				vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					(VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0u, (const VkBufferMemoryBarrier*)DE_NULL,
					DE_LENGTH_OF_ARRAY(preShaderImageBarriers), preShaderImageBarriers);
			}

			vk.cmdDispatch(cmdBuffer, extentCompressed.width, extentCompressed.height, extentCompressed.depth);

			{
				const VkImageMemoryBarrier		postShaderImageBarriers[]	=
				{
					makeImageMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
					VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					resultImage.get(), subresourceRange),

					makeImageMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
						VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						referenceImage.get(), subresourceRange)
				};

				vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					(VkDependencyFlags)0, 0u, (const VkMemoryBarrier*)DE_NULL, 0u, (const VkBufferMemoryBarrier*)DE_NULL,
					DE_LENGTH_OF_ARRAY(postShaderImageBarriers), postShaderImageBarriers);
			}

			vk.cmdCopyImageToBuffer(cmdBuffer, resultImage.get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, resultBuffer.get(), 1u, &copyRegion);
			vk.cmdCopyImageToBuffer(cmdBuffer, referenceImage.get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, referenceBuffer.get(), 1u, &copyRegion);

			{
				const VkBufferMemoryBarrier		postCopyBufferBarrier[]		=
				{
					makeBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT,
						resultBuffer.get(), 0ull, bufferSize),

					makeBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT,
						referenceBuffer.get(), 0ull, bufferSize),
				};

				vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
					(VkDependencyFlags)0, 0u, (const VkMemoryBarrier*)DE_NULL, DE_LENGTH_OF_ARRAY(postCopyBufferBarrier), postCopyBufferBarrier,
					0u, (const VkImageMemoryBarrier*)DE_NULL);
			}
		}
		endCommandBuffer(vk, cmdBuffer);
		submitCommandsAndWait(vk, device, queue, cmdBuffer);

		const Allocation&		resultAlloc		= resultBuffer.getAllocation();
		const Allocation&		referenceAlloc	= referenceBuffer.getAllocation();
		invalidateMappedMemoryRange(vk, device, resultAlloc.getMemory(), resultAlloc.getOffset(), bufferSize);
		invalidateMappedMemoryRange(vk, device, referenceAlloc.getMemory(), referenceAlloc.getOffset(), bufferSize);

		if (deMemCmp(resultAlloc.getHostPtr(), referenceAlloc.getHostPtr(), (size_t)bufferSize) != 0)
		{
			ConstPixelBufferAccess	resultPixels		(mapVkFormat(decompressedImageInfo.format), decompressedImageInfo.extent.width, decompressedImageInfo.extent.height, decompressedImageInfo.extent.depth, resultAlloc.getHostPtr());
			ConstPixelBufferAccess	referencePixels		(mapVkFormat(decompressedImageInfo.format), decompressedImageInfo.extent.width, decompressedImageInfo.extent.height, decompressedImageInfo.extent.depth, referenceAlloc.getHostPtr());

			if(!fuzzyCompare(m_context.getTestContext().getLog(), "Image Comparison", "Image Comparison", resultPixels, referencePixels, 0.001f, tcu::COMPARE_LOG_EVERYTHING))
				return false;
		}
	}

	return true;
}

class ImageStoreComputeTestInstance : public BasicComputeTestInstance
{
public:
					ImageStoreComputeTestInstance	(Context&							context,
													 const TestParameters&				parameters);
protected:
	virtual void	executeShader					(const VkCommandBuffer&				cmdBuffer,
													 const VkDescriptorSetLayout&		descriptorSetLayout,
													 const VkDescriptorPool&			descriptorPool,
													 vector<ImageData>&					imageData);
private:
};

ImageStoreComputeTestInstance::ImageStoreComputeTestInstance (Context& context, const TestParameters& parameters)
	:BasicComputeTestInstance	(context, parameters)
{
}

void ImageStoreComputeTestInstance::executeShader (const VkCommandBuffer&		cmdBuffer,
												   const VkDescriptorSetLayout&	descriptorSetLayout,
												   const VkDescriptorPool&		descriptorPool,
												   vector<ImageData>&			imageData)
{
	const DeviceInterface&			vk						= m_context.getDeviceInterface();
	const VkDevice					device					= m_context.getDevice();
	const VkQueue					queue					= m_context.getUniversalQueue();
	const Unique<VkShaderModule>	shaderModule			(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0));
	vector<SharedVkDescriptorSet>	descriptorSets			(imageData[0].getImageViewCount());
	const Unique<VkPipelineLayout>	pipelineLayout			(makePipelineLayout(vk, device, descriptorSetLayout));
	const Unique<VkPipeline>		pipeline				(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule));
	Move<VkSampler>					sampler;
	{
		const VkSamplerCreateInfo createInfo =
		{
			VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,		//VkStructureType		sType;
			DE_NULL,									//const void*			pNext;
			0u,											//VkSamplerCreateFlags	flags;
			VK_FILTER_NEAREST,							//VkFilter				magFilter;
			VK_FILTER_NEAREST,							//VkFilter				minFilter;
			VK_SAMPLER_MIPMAP_MODE_NEAREST,				//VkSamplerMipmapMode	mipmapMode;
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		//VkSamplerAddressMode	addressModeU;
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		//VkSamplerAddressMode	addressModeV;
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		//VkSamplerAddressMode	addressModeW;
			0.0f,										//float					mipLodBias;
			VK_FALSE,									//VkBool32				anisotropyEnable;
			1.0f,										//float					maxAnisotropy;
			VK_FALSE,									//VkBool32				compareEnable;
			VK_COMPARE_OP_EQUAL,						//VkCompareOp			compareOp;
			0.0f,										//float					minLod;
			0.0f,										//float					maxLod;
			VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,	//VkBorderColor			borderColor;
			VK_TRUE,									//VkBool32				unnormalizedCoordinates;
		};
		sampler = createSampler(vk, device, &createInfo);
	}

	vector<VkDescriptorImageInfo>	descriptorImageInfos	(descriptorSets.size() * m_parameters.imagesCount);
	for (deUint32 viewNdx = 0u; viewNdx < descriptorSets.size(); ++viewNdx)
	{
		const deUint32 descriptorNdx = viewNdx * m_parameters.imagesCount;
		for (deUint32 imageNdx = 0u; imageNdx < m_parameters.imagesCount; ++imageNdx)
		{
			descriptorImageInfos[descriptorNdx+imageNdx] = makeDescriptorImageInfo(*sampler,
															imageData[imageNdx].getImageView(viewNdx), VK_IMAGE_LAYOUT_GENERAL);
		}
	}

	for (deUint32 ndx = 0u; ndx < descriptorSets.size(); ++ndx)
		descriptorSets[ndx] = makeVkSharedPtr(makeDescriptorSet(vk, device, descriptorPool, descriptorSetLayout));

	beginCommandBuffer(vk, cmdBuffer);
	{
		const VkImageSubresourceRange	compressedRange				=
		{
			VK_IMAGE_ASPECT_COLOR_BIT,					//VkImageAspectFlags	aspectMask
			0u,											//deUint32				baseMipLevel
			imageData[0].getImageInfo(0).mipLevels,		//deUint32				levelCount
			0u,											//deUint32				baseArrayLayer
			imageData[0].getImageInfo(0).arrayLayers	//deUint32				layerCount
		};

		const VkImageSubresourceRange	uncompressedRange			=
		{
			VK_IMAGE_ASPECT_COLOR_BIT,					//VkImageAspectFlags	aspectMask
			0u,											//deUint32				baseMipLevel
			1u,											//deUint32				levelCount
			0u,											//deUint32				baseArrayLayer
			1u											//deUint32				layerCount
		};

		vk.cmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);

		vector<VkImageMemoryBarrier>		preShaderImageBarriers	(descriptorSets.size() * 2u + 1u);
		for (deUint32 imageNdx = 0u; imageNdx < imageData[1].getImagesCount(); ++imageNdx)
		{
			preShaderImageBarriers[imageNdx]									= makeImageMemoryBarrier(
																					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT,
																					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
																					imageData[1].getImage(imageNdx), uncompressedRange);

			preShaderImageBarriers[imageNdx + imageData[1].getImagesCount()]	= makeImageMemoryBarrier(
																					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT,
																					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
																					imageData[2].getImage(imageNdx), uncompressedRange);
		}

		preShaderImageBarriers[preShaderImageBarriers.size()-1] = makeImageMemoryBarrier(
																	VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
																	VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
																	imageData[0].getImage(0u), compressedRange);

		vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			(VkDependencyFlags)0, 0u, (const VkMemoryBarrier*)DE_NULL, 0u, (const VkBufferMemoryBarrier*)DE_NULL,
			static_cast<deUint32>(preShaderImageBarriers.size()), &preShaderImageBarriers[0]);

		for (deUint32 ndx = 0u; ndx <descriptorSets.size(); ++ndx)
		{
			descriptorSetUpdate (**descriptorSets[ndx], &descriptorImageInfos[ndx* m_parameters.imagesCount]);
			vk.cmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &(**descriptorSets[ndx]), 0u, DE_NULL);
			vk.cmdDispatch(cmdBuffer,	imageData[1].getImageInfo(ndx).extent.width,
										imageData[1].getImageInfo(ndx).extent.height,
										imageData[1].getImageInfo(ndx).extent.depth);
		}
	}
	endCommandBuffer(vk, cmdBuffer);
	submitCommandsAndWait(vk, device, queue, cmdBuffer);
}

class GraphicsAttachmentsTestInstance : public BasicTranscodingTestInstance
{
public:
										GraphicsAttachmentsTestInstance	(Context& context, const TestParameters& parameters);
	virtual TestStatus					iterate							(void);

protected:
	virtual bool						isWriteToCompressedOperation	();
	VkImageCreateInfo					makeCreateImageInfo				(const VkFormat					format,
																		 const ImageType				type,
																		 const UVec3&					size,
																		 const VkImageUsageFlags		usageFlags,
																		 const VkImageCreateFlags*		createFlags,
																		 const deUint32					levels,
																		 const deUint32					layers);
	VkDeviceSize						getCompressedImageData			(const VkFormat					format,
																		 const UVec3&					size,
																		 std::vector<deUint8>&			data,
																		 const deUint32					layer,
																		 const deUint32					level);
	VkDeviceSize						getUncompressedImageData		(const VkFormat					format,
																		 const UVec3&					size,
																		 std::vector<deUint8>&			data,
																		 const deUint32					layer,
																		 const deUint32					level);
	virtual void						prepareData						();
	virtual void						prepareVertexBuffer				();
	virtual void						transcodeRead					();
	virtual void						transcodeWrite					();
	bool								verifyDecompression				(const std::vector<deUint8>&	refCompressedData,
																		 const de::MovePtr<Image>&		resCompressedImage,
																		 const deUint32					layer,
																		 const deUint32					level,
																		 const UVec3&					mipmapDims);

	typedef std::vector<deUint8>		RawDataVector;
	typedef SharedPtr<RawDataVector>	RawDataPtr;
	typedef std::vector<RawDataPtr>		LevelData;
	typedef std::vector<LevelData>		FullImageData;

	FullImageData						m_srcData;
	FullImageData						m_dstData;

	typedef SharedPtr<Image>			ImagePtr;
	typedef std::vector<ImagePtr>		LevelImages;
	typedef std::vector<LevelImages>	ImagesArray;

	ImagesArray							m_uncompressedImages;
	MovePtr<Image>						m_compressedImage;

	VkImageViewUsageCreateInfoKHR		m_imageViewUsageKHR;
	VkImageViewUsageCreateInfoKHR*		m_srcImageViewUsageKHR;
	VkImageViewUsageCreateInfoKHR*		m_dstImageViewUsageKHR;
	std::vector<tcu::UVec3>				m_compressedImageResVec;
	std::vector<tcu::UVec3>				m_uncompressedImageResVec;
	VkFormat							m_srcFormat;
	VkFormat							m_dstFormat;
	VkImageUsageFlags					m_srcImageUsageFlags;
	VkImageUsageFlags					m_dstImageUsageFlags;
	std::vector<tcu::UVec3>				m_srcImageResolutions;
	std::vector<tcu::UVec3>				m_dstImageResolutions;

	MovePtr<Buffer>						m_vertexBuffer;
	deUint32							m_vertexCount;
	VkDeviceSize						m_vertexBufferOffset;
};

GraphicsAttachmentsTestInstance::GraphicsAttachmentsTestInstance (Context& context, const TestParameters& parameters)
	: BasicTranscodingTestInstance(context, parameters)
	, m_srcData()
	, m_dstData()
	, m_uncompressedImages()
	, m_compressedImage()
	, m_imageViewUsageKHR()
	, m_srcImageViewUsageKHR()
	, m_dstImageViewUsageKHR()
	, m_compressedImageResVec()
	, m_uncompressedImageResVec()
	, m_srcFormat()
	, m_dstFormat()
	, m_srcImageUsageFlags()
	, m_dstImageUsageFlags()
	, m_srcImageResolutions()
	, m_dstImageResolutions()
	, m_vertexBuffer()
	, m_vertexCount(0u)
	, m_vertexBufferOffset(0ull)
{
}

TestStatus GraphicsAttachmentsTestInstance::iterate (void)
{
	prepareData();
	prepareVertexBuffer();

	for (deUint32 levelNdx = 0; levelNdx < getLevelCount(); ++levelNdx)
		for (deUint32 layerNdx = 0; layerNdx < getLayerCount(); ++layerNdx)
			DE_ASSERT(m_srcData[levelNdx][layerNdx]->size() == m_dstData[levelNdx][layerNdx]->size());

	if (isWriteToCompressedOperation())
		transcodeWrite();
	else
		transcodeRead();

	for (deUint32 levelNdx = 0; levelNdx < getLevelCount(); ++levelNdx)
		for (deUint32 layerNdx = 0; layerNdx < getLayerCount(); ++layerNdx)
			if (isWriteToCompressedOperation())
			{
				if (!verifyDecompression(*m_srcData[levelNdx][layerNdx], m_compressedImage, levelNdx, layerNdx, m_compressedImageResVec[levelNdx]))
					return TestStatus::fail("Images difference detected");
			}
			else
			{
				if (!verifyDecompression(*m_dstData[levelNdx][layerNdx], m_compressedImage, levelNdx, layerNdx, m_compressedImageResVec[levelNdx]))
					return TestStatus::fail("Images difference detected");
			}

	return TestStatus::pass("Pass");
}

void GraphicsAttachmentsTestInstance::prepareData ()
{
	VkImageViewUsageCreateInfoKHR*	imageViewUsageKHRNull	= (VkImageViewUsageCreateInfoKHR*)DE_NULL;

	m_imageViewUsageKHR			= makeImageViewUsageCreateInfo(m_parameters.compressedImageViewUsage);

	m_srcImageViewUsageKHR		= isWriteToCompressedOperation() ? imageViewUsageKHRNull : &m_imageViewUsageKHR;
	m_dstImageViewUsageKHR		= isWriteToCompressedOperation() ? &m_imageViewUsageKHR : imageViewUsageKHRNull;

	m_srcFormat					= isWriteToCompressedOperation() ? m_parameters.formatUncompressed : m_parameters.formatCompressed;
	m_dstFormat					= isWriteToCompressedOperation() ? m_parameters.formatCompressed : m_parameters.formatUncompressed;

	m_srcImageUsageFlags		= isWriteToCompressedOperation() ? m_parameters.uncompressedImageUsage : m_parameters.compressedImageUsage;
	m_dstImageUsageFlags		= isWriteToCompressedOperation() ? m_parameters.compressedImageUsage : m_parameters.uncompressedImageUsage;

	m_compressedImageResVec		= getMipLevelSizes(getLayerDims());
	m_uncompressedImageResVec	= getCompressedMipLevelSizes(m_parameters.formatCompressed, m_compressedImageResVec);

	m_srcImageResolutions		= isWriteToCompressedOperation() ? m_uncompressedImageResVec : m_compressedImageResVec;
	m_dstImageResolutions		= isWriteToCompressedOperation() ? m_compressedImageResVec : m_uncompressedImageResVec;

	m_srcData.resize(getLevelCount());
	m_dstData.resize(getLevelCount());
	m_uncompressedImages.resize(getLevelCount());

	for (deUint32 levelNdx = 0; levelNdx < getLevelCount(); ++levelNdx)
	{
		m_srcData[levelNdx].resize(getLayerCount());
		m_dstData[levelNdx].resize(getLayerCount());
		m_uncompressedImages[levelNdx].resize(getLayerCount());

		for (deUint32 layerNdx = 0; layerNdx < getLayerCount(); ++layerNdx)
		{
			m_srcData[levelNdx][layerNdx] = SharedPtr<RawDataVector>(new RawDataVector);
			m_dstData[levelNdx][layerNdx] = SharedPtr<RawDataVector>(new RawDataVector);

			if (isWriteToCompressedOperation())
			{
				getUncompressedImageData(m_srcFormat, m_srcImageResolutions[levelNdx], *m_srcData[levelNdx][layerNdx], layerNdx, levelNdx);

				m_dstData[levelNdx][layerNdx]->resize((size_t)getCompressedImageSizeInBytes(m_dstFormat, m_dstImageResolutions[levelNdx]));
			}
			else
			{
				getCompressedImageData(m_srcFormat, m_srcImageResolutions[levelNdx], *m_srcData[levelNdx][layerNdx], layerNdx, levelNdx);

				m_dstData[levelNdx][layerNdx]->resize((size_t)getUncompressedImageSizeInBytes(m_dstFormat, m_dstImageResolutions[levelNdx]));
			}

			DE_ASSERT(m_srcData[levelNdx][layerNdx]->size() == m_dstData[levelNdx][layerNdx]->size());
		}
	}
}

void GraphicsAttachmentsTestInstance::prepareVertexBuffer ()
{
	const DeviceInterface&			vk						= m_context.getDeviceInterface();
	const VkDevice					device					= m_context.getDevice();
	Allocator&						allocator				= m_context.getDefaultAllocator();

	const std::vector<tcu::Vec4>	vertexArray				= createFullscreenQuad();
	const size_t					vertexBufferSizeInBytes	= vertexArray.size() * sizeof(vertexArray[0]);

	m_vertexCount	= static_cast<deUint32>(vertexArray.size());
	m_vertexBuffer	= MovePtr<Buffer>(new Buffer(vk, device, allocator, makeBufferCreateInfo(vertexBufferSizeInBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), MemoryRequirement::HostVisible));

	// Upload vertex data
	const Allocation&	vertexBufferAlloc	= m_vertexBuffer->getAllocation();
	deMemcpy(vertexBufferAlloc.getHostPtr(), &vertexArray[0], vertexBufferSizeInBytes);
	flushMappedMemoryRange(vk, device, vertexBufferAlloc.getMemory(), vertexBufferAlloc.getOffset(), vertexBufferSizeInBytes);
}

void GraphicsAttachmentsTestInstance::transcodeRead ()
{
	const DeviceInterface&				vk						= m_context.getDeviceInterface();
	const VkDevice						device					= m_context.getDevice();
	const deUint32						queueFamilyIndex		= m_context.getUniversalQueueFamilyIndex();
	const VkQueue						queue					= m_context.getUniversalQueue();
	Allocator&							allocator				= m_context.getDefaultAllocator();

	const VkImageCreateFlags*			imgCreateFlagsOverride	= DE_NULL;

	const VkImageCreateInfo				srcImageCreateInfo		= makeCreateImageInfo(m_srcFormat, m_parameters.imageType, m_srcImageResolutions[0], m_srcImageUsageFlags, imgCreateFlagsOverride, getLevelCount(), getLayerCount());
	MovePtr<Image>						srcImage				(new Image(vk, device, allocator, srcImageCreateInfo, MemoryRequirement::Any));

	const Unique<VkShaderModule>		vertShaderModule		(createShaderModule(vk, device, m_context.getBinaryCollection().get("vert"), 0));
	const Unique<VkShaderModule>		fragShaderModule		(createShaderModule(vk, device, m_context.getBinaryCollection().get("frag"), 0));

	const Unique<VkRenderPass>			renderPass				(makeRenderPass(vk, device, m_parameters.formatUncompressed, m_parameters.formatUncompressed));

	const Move<VkDescriptorSetLayout>	descriptorSetLayout		(DescriptorSetLayoutBuilder()
																	.addSingleBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
																	.build(vk, device));
	const Move<VkDescriptorPool>		descriptorPool			(DescriptorPoolBuilder()
																	.addType(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
																	.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));
	const Move<VkDescriptorSet>			descriptorSet			(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));

	const VkExtent2D					renderSizeDummy			(makeExtent2D(1u, 1u));
	const Unique<VkPipelineLayout>		pipelineLayout			(makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkPipeline>			pipeline				(makeGraphicsPipeline(vk, device, *pipelineLayout, *renderPass, *vertShaderModule, *fragShaderModule, renderSizeDummy, 1u, true));

	const Unique<VkCommandPool>			cmdPool					(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>		cmdBuffer				(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	for (deUint32 levelNdx = 0; levelNdx < getLevelCount(); ++levelNdx)
	{
		const UVec3&				uncompressedImageRes	= m_uncompressedImageResVec[levelNdx];
		const UVec3&				srcImageResolution		= m_srcImageResolutions[levelNdx];
		const UVec3&				dstImageResolution		= m_dstImageResolutions[levelNdx];
		const size_t				srcImageSizeInBytes		= m_srcData[levelNdx][0]->size();
		const size_t				dstImageSizeInBytes		= m_dstData[levelNdx][0]->size();
		const UVec3					srcImageResBlocked		= getCompressedImageResolutionBlockCeil(m_parameters.formatCompressed, srcImageResolution);

		const VkImageCreateInfo		dstImageCreateInfo		= makeCreateImageInfo(m_dstFormat, m_parameters.imageType, dstImageResolution, m_dstImageUsageFlags, imgCreateFlagsOverride, SINGLE_LEVEL, SINGLE_LAYER);

		const VkBufferCreateInfo	srcImageBufferInfo		= makeBufferCreateInfo(srcImageSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		const MovePtr<Buffer>		srcImageBuffer			= MovePtr<Buffer>(new Buffer(vk, device, allocator, srcImageBufferInfo, MemoryRequirement::HostVisible));

		const VkBufferCreateInfo	dstImageBufferInfo		= makeBufferCreateInfo(dstImageSizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		MovePtr<Buffer>				dstImageBuffer			= MovePtr<Buffer>(new Buffer(vk, device, allocator, dstImageBufferInfo, MemoryRequirement::HostVisible));

		const VkExtent2D			renderSize				(makeExtent2D(uncompressedImageRes.x(), uncompressedImageRes.y()));
		const VkViewport			viewport				= makeViewport(0.0f, 0.0f, static_cast<float>(renderSize.width), static_cast<float>(renderSize.height), 0.0f, 1.0f);
		const VkRect2D				scissor					= makeScissor(renderSize.width, renderSize.height);

		for (deUint32 layerNdx = 0; layerNdx < getLayerCount(); ++layerNdx)
		{
			const VkImageSubresourceRange	srcSubresourceRange		= makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, levelNdx, SINGLE_LEVEL, layerNdx, SINGLE_LAYER);
			const VkImageSubresourceRange	dstSubresourceRange		= makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, SINGLE_LEVEL, 0u, SINGLE_LAYER);

			Move<VkImageView>				srcImageView			(makeImageView(vk, device, srcImage->get(), mapImageViewType(m_parameters.imageType), m_parameters.formatUncompressed, srcSubresourceRange, m_srcImageViewUsageKHR));

			de::MovePtr<Image>				dstImage				(new Image(vk, device, allocator, dstImageCreateInfo, MemoryRequirement::Any));
			Move<VkImageView>				dstImageView			(makeImageView(vk, device, dstImage->get(), mapImageViewType(m_parameters.imageType), m_parameters.formatUncompressed, dstSubresourceRange, m_dstImageViewUsageKHR));

			const VkBufferImageCopy			srcCopyRegion			= makeBufferImageCopy(srcImageResolution.x(), srcImageResolution.y(), levelNdx, layerNdx, srcImageResBlocked.x(), srcImageResBlocked.y());
			const VkBufferMemoryBarrier		srcCopyBufferBarrierPre	= makeBufferMemoryBarrier(VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, srcImageBuffer->get(), 0ull, srcImageSizeInBytes);
			const VkImageMemoryBarrier		srcCopyImageBarrierPre	= makeImageMemoryBarrier(0u, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, srcImage->get(), srcSubresourceRange);
			const VkImageMemoryBarrier		srcCopyImageBarrierPost	= makeImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, srcImage->get(), srcSubresourceRange);
			const VkBufferImageCopy			dstCopyRegion			= makeBufferImageCopy(dstImageResolution.x(), dstImageResolution.y());
			const VkImageMemoryBarrier		dstInitImageBarrier		= makeImageMemoryBarrier(0u, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, dstImage->get(), dstSubresourceRange);

			const VkImageView				attachmentBindInfos[]	= { *srcImageView, *dstImageView };
			const VkExtent2D				framebufferSize			(makeExtent2D(dstImageResolution[0], dstImageResolution[1]));
			const Move<VkFramebuffer>		framebuffer				(makeFramebuffer(vk, device, *renderPass, DE_LENGTH_OF_ARRAY(attachmentBindInfos), attachmentBindInfos, framebufferSize, SINGLE_LAYER));

			// Upload source image data
			const Allocation& alloc = srcImageBuffer->getAllocation();
			deMemcpy(alloc.getHostPtr(), &m_srcData[levelNdx][layerNdx]->at(0), srcImageSizeInBytes);
			flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), srcImageSizeInBytes);

			beginCommandBuffer(vk, *cmdBuffer);
			vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);

			// Copy buffer to image
			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1u, &srcCopyBufferBarrierPre, 1u, &srcCopyImageBarrierPre);
			vk.cmdCopyBufferToImage(*cmdBuffer, srcImageBuffer->get(), srcImage->get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &srcCopyRegion);
			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0u, DE_NULL, 1u, &srcCopyImageBarrierPost);

			// Define destination image layout
			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0u, DE_NULL, 1u, &dstInitImageBarrier);

			beginRenderPass(vk, *cmdBuffer, *renderPass, *framebuffer, renderSize);

			const VkDescriptorImageInfo	descriptorSrcImageInfo(makeDescriptorImageInfo(DE_NULL, *srcImageView, VK_IMAGE_LAYOUT_GENERAL));
			DescriptorSetUpdateBuilder()
				.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &descriptorSrcImageInfo)
				.update(vk, device);

			vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);
			vk.cmdBindVertexBuffers(*cmdBuffer, 0u, 1u, &m_vertexBuffer->get(), &m_vertexBufferOffset);

			vk.cmdSetViewport(*cmdBuffer, 0u, 1u, &viewport);
			vk.cmdSetScissor(*cmdBuffer, 0u, 1u, &scissor);

			vk.cmdDraw(*cmdBuffer, (deUint32)m_vertexCount, 1, 0, 0);

			vk.cmdEndRenderPass(*cmdBuffer);

			const VkImageMemoryBarrier prepareForTransferBarrier = makeImageMemoryBarrier(
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
				dstImage->get(), dstSubresourceRange);

			const VkBufferMemoryBarrier copyBarrier = makeBufferMemoryBarrier(
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT,
				dstImageBuffer->get(), 0ull, dstImageSizeInBytes);

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &prepareForTransferBarrier);
			vk.cmdCopyImageToBuffer(*cmdBuffer, dstImage->get(), VK_IMAGE_LAYOUT_GENERAL, dstImageBuffer->get(), 1u, &dstCopyRegion);
			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &copyBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);

			endCommandBuffer(vk, *cmdBuffer);

			submitCommandsAndWait(vk, device, queue, *cmdBuffer);

			const Allocation& dstImageBufferAlloc = dstImageBuffer->getAllocation();
			invalidateMappedMemoryRange(vk, device, dstImageBufferAlloc.getMemory(), dstImageBufferAlloc.getOffset(), dstImageSizeInBytes);
			deMemcpy(&m_dstData[levelNdx][layerNdx]->at(0), dstImageBufferAlloc.getHostPtr(), dstImageSizeInBytes);
		}
	}

	m_compressedImage = srcImage;
}

void GraphicsAttachmentsTestInstance::transcodeWrite ()
{
	const DeviceInterface&				vk						= m_context.getDeviceInterface();
	const VkDevice						device					= m_context.getDevice();
	const deUint32						queueFamilyIndex		= m_context.getUniversalQueueFamilyIndex();
	const VkQueue						queue					= m_context.getUniversalQueue();
	Allocator&							allocator				= m_context.getDefaultAllocator();

	const VkImageCreateFlags*			imgCreateFlagsOverride	= DE_NULL;

	const VkImageCreateInfo				dstImageCreateInfo		= makeCreateImageInfo(m_dstFormat, m_parameters.imageType, m_dstImageResolutions[0], m_dstImageUsageFlags, imgCreateFlagsOverride, getLevelCount(), getLayerCount());
	MovePtr<Image>						dstImage				(new Image(vk, device, allocator, dstImageCreateInfo, MemoryRequirement::Any));

	const Unique<VkShaderModule>		vertShaderModule		(createShaderModule(vk, device, m_context.getBinaryCollection().get("vert"), 0));
	const Unique<VkShaderModule>		fragShaderModule		(createShaderModule(vk, device, m_context.getBinaryCollection().get("frag"), 0));

	const Unique<VkRenderPass>			renderPass				(makeRenderPass(vk, device, m_parameters.formatUncompressed, m_parameters.formatUncompressed));

	const Move<VkDescriptorSetLayout>	descriptorSetLayout		(DescriptorSetLayoutBuilder()
																	.addSingleBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
																	.build(vk, device));
	const Move<VkDescriptorPool>		descriptorPool			(DescriptorPoolBuilder()
																	.addType(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
																	.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));
	const Move<VkDescriptorSet>			descriptorSet			(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));

	const VkExtent2D					renderSizeDummy			(makeExtent2D(1u, 1u));
	const Unique<VkPipelineLayout>		pipelineLayout			(makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkPipeline>			pipeline				(makeGraphicsPipeline(vk, device, *pipelineLayout, *renderPass, *vertShaderModule, *fragShaderModule, renderSizeDummy, 1u, true));

	const Unique<VkCommandPool>			cmdPool					(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>		cmdBuffer				(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	for (deUint32 levelNdx = 0; levelNdx < getLevelCount(); ++levelNdx)
	{
		const UVec3&				uncompressedImageRes	= m_uncompressedImageResVec[levelNdx];
		const UVec3&				srcImageResolution		= m_srcImageResolutions[levelNdx];
		const UVec3&				dstImageResolution		= m_dstImageResolutions[levelNdx];
		const UVec3					dstImageResBlocked		= getCompressedImageResolutionBlockCeil(m_parameters.formatCompressed, dstImageResolution);
		const size_t				srcImageSizeInBytes		= m_srcData[levelNdx][0]->size();
		const size_t				dstImageSizeInBytes		= m_dstData[levelNdx][0]->size();

		const VkImageCreateInfo		srcImageCreateInfo		= makeCreateImageInfo(m_srcFormat, m_parameters.imageType, srcImageResolution, m_srcImageUsageFlags, imgCreateFlagsOverride, SINGLE_LEVEL, SINGLE_LAYER);

		const VkExtent2D			renderSize				(makeExtent2D(uncompressedImageRes.x(), uncompressedImageRes.y()));
		const VkViewport			viewport				= makeViewport(0.0f, 0.0f, static_cast<float>(renderSize.width), static_cast<float>(renderSize.height), 0.0f, 1.0f);
		const VkRect2D				scissor					= makeScissor(renderSize.width, renderSize.height);

		for (deUint32 layerNdx = 0; layerNdx < getLayerCount(); ++layerNdx)
		{
			const VkBufferCreateInfo		srcImageBufferInfo		= makeBufferCreateInfo(srcImageSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
			const MovePtr<Buffer>			srcImageBuffer			= MovePtr<Buffer>(new Buffer(vk, device, allocator, srcImageBufferInfo, MemoryRequirement::HostVisible));

			const VkBufferCreateInfo		dstImageBufferInfo		= makeBufferCreateInfo(dstImageSizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
			MovePtr<Buffer>					dstImageBuffer			= MovePtr<Buffer>(new Buffer(vk, device, allocator, dstImageBufferInfo, MemoryRequirement::HostVisible));

			const VkImageSubresourceRange	srcSubresourceRange		= makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, SINGLE_LEVEL, 0u, SINGLE_LAYER);
			const VkImageSubresourceRange	dstSubresourceRange		= makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, levelNdx, SINGLE_LEVEL, layerNdx, SINGLE_LAYER);

			Move<VkImageView>				dstImageView			(makeImageView(vk, device, dstImage->get(), mapImageViewType(m_parameters.imageType), m_parameters.formatUncompressed, dstSubresourceRange, m_dstImageViewUsageKHR));

			de::MovePtr<Image>				srcImage				(new Image(vk, device, allocator, srcImageCreateInfo, MemoryRequirement::Any));
			Move<VkImageView>				srcImageView			(makeImageView(vk, device, srcImage->get(), mapImageViewType(m_parameters.imageType), m_parameters.formatUncompressed, srcSubresourceRange, m_srcImageViewUsageKHR));

			const VkBufferImageCopy			srcCopyRegion			= makeBufferImageCopy(srcImageResolution.x(), srcImageResolution.y(), 0u, 0u);
			const VkBufferMemoryBarrier		srcCopyBufferBarrierPre	= makeBufferMemoryBarrier(VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, srcImageBuffer->get(), 0ull, srcImageSizeInBytes);
			const VkImageMemoryBarrier		srcCopyImageBarrierPre	= makeImageMemoryBarrier(0u, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, srcImage->get(), srcSubresourceRange);
			const VkImageMemoryBarrier		srcCopyImageBarrierPost	= makeImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, srcImage->get(), srcSubresourceRange);
			const VkBufferImageCopy			dstCopyRegion			= makeBufferImageCopy(dstImageResolution.x(), dstImageResolution.y(), levelNdx, layerNdx, dstImageResBlocked.x(), dstImageResBlocked.y());
			const VkImageMemoryBarrier		dstInitImageBarrier		= makeImageMemoryBarrier(0u, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, dstImage->get(), dstSubresourceRange);

			const VkImageView				attachmentBindInfos[]	= { *srcImageView, *dstImageView };
			const VkExtent2D				framebufferSize			(renderSize);
			const Move<VkFramebuffer>		framebuffer				(makeFramebuffer(vk, device, *renderPass, DE_LENGTH_OF_ARRAY(attachmentBindInfos), attachmentBindInfos, framebufferSize, SINGLE_LAYER));

			// Upload source image data
			const Allocation& alloc = srcImageBuffer->getAllocation();
			deMemcpy(alloc.getHostPtr(), &m_srcData[levelNdx][layerNdx]->at(0), srcImageSizeInBytes);
			flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), srcImageSizeInBytes);

			beginCommandBuffer(vk, *cmdBuffer);
			vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);

			// Copy buffer to image
			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1u, &srcCopyBufferBarrierPre, 1u, &srcCopyImageBarrierPre);
			vk.cmdCopyBufferToImage(*cmdBuffer, srcImageBuffer->get(), srcImage->get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &srcCopyRegion);
			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0u, DE_NULL, 1u, &srcCopyImageBarrierPost);

			// Define destination image layout
			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0u, DE_NULL, 1u, &dstInitImageBarrier);

			beginRenderPass(vk, *cmdBuffer, *renderPass, *framebuffer, renderSize);

			const VkDescriptorImageInfo	descriptorSrcImageInfo(makeDescriptorImageInfo(DE_NULL, *srcImageView, VK_IMAGE_LAYOUT_GENERAL));
			DescriptorSetUpdateBuilder()
				.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &descriptorSrcImageInfo)
				.update(vk, device);

			vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);
			vk.cmdBindVertexBuffers(*cmdBuffer, 0u, 1u, &m_vertexBuffer->get(), &m_vertexBufferOffset);

			vk.cmdSetViewport(*cmdBuffer, 0u, 1u, &viewport);
			vk.cmdSetScissor(*cmdBuffer, 0u, 1u, &scissor);

			vk.cmdDraw(*cmdBuffer, (deUint32)m_vertexCount, 1, 0, 0);

			vk.cmdEndRenderPass(*cmdBuffer);

			const VkImageMemoryBarrier prepareForTransferBarrier = makeImageMemoryBarrier(
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
				dstImage->get(), dstSubresourceRange);

			const VkBufferMemoryBarrier copyBarrier = makeBufferMemoryBarrier(
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT,
				dstImageBuffer->get(), 0ull, dstImageSizeInBytes);

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &prepareForTransferBarrier);
			vk.cmdCopyImageToBuffer(*cmdBuffer, dstImage->get(), VK_IMAGE_LAYOUT_GENERAL, dstImageBuffer->get(), 1u, &dstCopyRegion);
			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &copyBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);

			endCommandBuffer(vk, *cmdBuffer);

			submitCommandsAndWait(vk, device, queue, *cmdBuffer);

			const Allocation& dstImageBufferAlloc = dstImageBuffer->getAllocation();
			invalidateMappedMemoryRange(vk, device, dstImageBufferAlloc.getMemory(), dstImageBufferAlloc.getOffset(), dstImageSizeInBytes);
			deMemcpy(&m_dstData[levelNdx][layerNdx]->at(0), dstImageBufferAlloc.getHostPtr(), dstImageSizeInBytes);
		}
	}

	m_compressedImage = dstImage;
}

bool GraphicsAttachmentsTestInstance::isWriteToCompressedOperation ()
{
	return (m_parameters.operation == OPERATION_ATTACHMENT_WRITE);
}

VkImageCreateInfo GraphicsAttachmentsTestInstance::makeCreateImageInfo (const VkFormat				format,
																	    const ImageType				type,
																	    const UVec3&				size,
																	    const VkImageUsageFlags		usageFlags,
																	    const VkImageCreateFlags*	createFlags,
																	    const deUint32				levels,
																	    const deUint32				layers)
{
	const VkImageType			imageType				= mapImageType(type);
	const VkImageCreateFlags	imageCreateFlagsBase	= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
	const VkImageCreateFlags	imageCreateFlagsAddOn	= isCompressedFormat(format) ? VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT_KHR | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT_KHR : 0;
	const VkImageCreateFlags	imageCreateFlags		= (createFlags != DE_NULL) ? *createFlags : (imageCreateFlagsBase | imageCreateFlagsAddOn);

	const VkImageCreateInfo createImageInfo =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,			// VkStructureType			sType;
		DE_NULL,										// const void*				pNext;
		imageCreateFlags,								// VkImageCreateFlags		flags;
		imageType,										// VkImageType				imageType;
		format,											// VkFormat					format;
		makeExtent3D(getLayerSize(type, size)),			// VkExtent3D				extent;
		levels,											// deUint32					mipLevels;
		layers,											// deUint32					arrayLayers;
		VK_SAMPLE_COUNT_1_BIT,							// VkSampleCountFlagBits	samples;
		VK_IMAGE_TILING_OPTIMAL,						// VkImageTiling			tiling;
		usageFlags,										// VkImageUsageFlags		usage;
		VK_SHARING_MODE_EXCLUSIVE,						// VkSharingMode			sharingMode;
		0u,												// deUint32					queueFamilyIndexCount;
		DE_NULL,										// const deUint32*			pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,						// VkImageLayout			initialLayout;
	};

	return createImageInfo;
}

VkDeviceSize GraphicsAttachmentsTestInstance::getCompressedImageData (const VkFormat			format,
																	  const UVec3&				size,
																	  std::vector<deUint8>&		data,
																	  const deUint32			layer,
																	  const deUint32			level)
{
	VkDeviceSize	sizeBytes	= getCompressedImageSizeInBytes(format, size);

	data.resize((size_t)sizeBytes);
	generateData(&data[0], data.size(), format, layer, level);

	return sizeBytes;
}

VkDeviceSize GraphicsAttachmentsTestInstance::getUncompressedImageData (const VkFormat			format,
																		const UVec3&			size,
																		std::vector<deUint8>&	data,
																		const deUint32			layer,
																		const deUint32			level)
{
	tcu::IVec3				sizeAsIVec3	= tcu::IVec3(static_cast<int>(size[0]), static_cast<int>(size[1]), static_cast<int>(size[2]));
	VkDeviceSize			sizeBytes	= getImageSizeBytes(sizeAsIVec3, format);

	data.resize((size_t)sizeBytes);
	generateData(&data[0], data.size(), format, layer, level);

	return sizeBytes;
}

bool GraphicsAttachmentsTestInstance::verifyDecompression (const std::vector<deUint8>&	refCompressedData,
														   const de::MovePtr<Image>&	resCompressedImage,
														   const deUint32				level,
														   const deUint32				layer,
														   const UVec3&					mipmapDims)
{
	const DeviceInterface&				vk							= m_context.getDeviceInterface();
	const VkDevice						device						= m_context.getDevice();
	const deUint32						queueFamilyIndex			= m_context.getUniversalQueueFamilyIndex();
	const VkQueue						queue						= m_context.getUniversalQueue();
	Allocator&							allocator					= m_context.getDefaultAllocator();

	const bool							layoutShaderReadOnly		= (layer % 2u) == 1;
	const UVec3							mipmapDimsBlocked			= getCompressedImageResolutionBlockCeil(m_parameters.formatCompressed, mipmapDims);

	const VkImageSubresourceRange		subresourceRange			= makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, SINGLE_LEVEL, 0u, SINGLE_LAYER);
	const VkImageSubresourceRange		resSubresourceRange			= makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, level, SINGLE_LEVEL, layer, SINGLE_LAYER);

	const VkDeviceSize					dstBufferSize				= getUncompressedImageSizeInBytes(m_parameters.formatForVerify, mipmapDims);
	const VkImageUsageFlags				refSrcImageUsageFlags		= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	const VkBufferCreateInfo			refSrcImageBufferInfo		(makeBufferCreateInfo(refCompressedData.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT));
	const MovePtr<Buffer>				refSrcImageBuffer			= MovePtr<Buffer>(new Buffer(vk, device, allocator, refSrcImageBufferInfo, MemoryRequirement::HostVisible));

	const VkImageCreateFlags			refSrcImageCreateFlags		= 0;
	const VkImageCreateInfo				refSrcImageCreateInfo		= makeCreateImageInfo(m_parameters.formatCompressed, m_parameters.imageType, mipmapDimsBlocked, refSrcImageUsageFlags, &refSrcImageCreateFlags, SINGLE_LEVEL, SINGLE_LAYER);
	const MovePtr<Image>				refSrcImage					(new Image(vk, device, allocator, refSrcImageCreateInfo, MemoryRequirement::Any));
	Move<VkImageView>					refSrcImageView				(makeImageView(vk, device, refSrcImage->get(), mapImageViewType(m_parameters.imageType), m_parameters.formatCompressed, subresourceRange));

	const VkImageUsageFlags				resSrcImageUsageFlags		= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	const VkImageViewUsageCreateInfoKHR	resSrcImageViewUsageKHR		= makeImageViewUsageCreateInfo(resSrcImageUsageFlags);
	Move<VkImageView>					resSrcImageView				(makeImageView(vk, device, resCompressedImage->get(), mapImageViewType(m_parameters.imageType), m_parameters.formatCompressed, resSubresourceRange, &resSrcImageViewUsageKHR));

	const VkImageCreateFlags			refDstImageCreateFlags		= 0;
	const VkImageUsageFlags				refDstImageUsageFlags		= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	const VkImageCreateInfo				refDstImageCreateInfo		= makeCreateImageInfo(m_parameters.formatForVerify, m_parameters.imageType, mipmapDims, refDstImageUsageFlags, &refDstImageCreateFlags, SINGLE_LEVEL, SINGLE_LAYER);
	const MovePtr<Image>				refDstImage					(new Image(vk, device, allocator, refDstImageCreateInfo, MemoryRequirement::Any));
	const Move<VkImageView>				refDstImageView				(makeImageView(vk, device, refDstImage->get(), mapImageViewType(m_parameters.imageType), m_parameters.formatForVerify, subresourceRange));
	const VkImageMemoryBarrier			refDstInitImageBarrier		= makeImageMemoryBarrier(0u, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, refDstImage->get(), subresourceRange);
	const VkBufferCreateInfo			refDstBufferInfo			(makeBufferCreateInfo(dstBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT));
	const MovePtr<Buffer>				refDstBuffer				= MovePtr<Buffer>(new Buffer(vk, device, allocator, refDstBufferInfo, MemoryRequirement::HostVisible));

	const VkImageCreateFlags			resDstImageCreateFlags		= 0;
	const VkImageUsageFlags				resDstImageUsageFlags		= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	const VkImageCreateInfo				resDstImageCreateInfo		= makeCreateImageInfo(m_parameters.formatForVerify, m_parameters.imageType, mipmapDims, resDstImageUsageFlags, &resDstImageCreateFlags, SINGLE_LEVEL, SINGLE_LAYER);
	const MovePtr<Image>				resDstImage					(new Image(vk, device, allocator, resDstImageCreateInfo, MemoryRequirement::Any));
	const Move<VkImageView>				resDstImageView				(makeImageView(vk, device, resDstImage->get(), mapImageViewType(m_parameters.imageType), m_parameters.formatForVerify, subresourceRange));
	const VkImageMemoryBarrier			resDstInitImageBarrier		= makeImageMemoryBarrier(0u, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, resDstImage->get(), subresourceRange);
	const VkBufferCreateInfo			resDstBufferInfo			(makeBufferCreateInfo(dstBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT));
	const MovePtr<Buffer>				resDstBuffer				= MovePtr<Buffer>(new Buffer(vk, device, allocator, resDstBufferInfo, MemoryRequirement::HostVisible));

	const Unique<VkShaderModule>		vertShaderModule			(createShaderModule(vk, device, m_context.getBinaryCollection().get("vert"), 0));
	const Unique<VkShaderModule>		fragShaderModule			(createShaderModule(vk, device, m_context.getBinaryCollection().get("frag_verify"), 0));

	const Unique<VkRenderPass>			renderPass					(makeRenderPass(vk, device));

	const Move<VkDescriptorSetLayout>	descriptorSetLayout			(DescriptorSetLayoutBuilder()
																		.addSingleBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
																		.addSingleBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
																		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT)
																		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT)
																		.build(vk, device));
	const Move<VkDescriptorPool>		descriptorPool				(DescriptorPoolBuilder()
																		.addType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
																		.addType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
																		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
																		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
																		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));
	const Move<VkDescriptorSet>			descriptorSet				(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));
	const VkSamplerCreateInfo			refSrcSamplerInfo			(makeSamplerCreateInfo());
	const Move<VkSampler>				refSrcSampler				= vk::createSampler(vk, device, &refSrcSamplerInfo);
	const VkSamplerCreateInfo			resSrcSamplerInfo			(makeSamplerCreateInfo());
	const Move<VkSampler>				resSrcSampler				= vk::createSampler(vk, device, &resSrcSamplerInfo);
	const VkDescriptorImageInfo			descriptorRefSrcImage		(makeDescriptorImageInfo(*refSrcSampler, *refSrcImageView, layoutShaderReadOnly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL));
	const VkDescriptorImageInfo			descriptorResSrcImage		(makeDescriptorImageInfo(*resSrcSampler, *resSrcImageView, layoutShaderReadOnly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL));
	const VkDescriptorImageInfo			descriptorRefDstImage		(makeDescriptorImageInfo(DE_NULL, *refDstImageView, VK_IMAGE_LAYOUT_GENERAL));
	const VkDescriptorImageInfo			descriptorResDstImage		(makeDescriptorImageInfo(DE_NULL, *resDstImageView, VK_IMAGE_LAYOUT_GENERAL));

	const VkExtent2D					renderSize					(makeExtent2D(mipmapDims.x(), mipmapDims.y()));
	const Unique<VkPipelineLayout>		pipelineLayout				(makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkPipeline>			pipeline					(makeGraphicsPipeline(vk, device, *pipelineLayout, *renderPass, *vertShaderModule, *fragShaderModule, renderSize, 0u));
	const Unique<VkCommandPool>			cmdPool						(createCommandPool(vk, device, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>		cmdBuffer					(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	const VkBufferImageCopy				copyBufferToImageRegion		= makeBufferImageCopy(mipmapDimsBlocked.x(), mipmapDimsBlocked.y(), 0u, 0u, mipmapDimsBlocked.x(), mipmapDimsBlocked.y());
	const VkBufferImageCopy				copyRegion					= makeBufferImageCopy(mipmapDims.x(), mipmapDims.y(), 0u, 0u);
	const VkBufferMemoryBarrier			refSrcCopyBufferBarrierPre	= makeBufferMemoryBarrier(VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, refSrcImageBuffer->get(), 0ull, refCompressedData.size());
	const VkImageMemoryBarrier			refSrcCopyImageBarrierPre	= makeImageMemoryBarrier(0u, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, refSrcImage->get(), subresourceRange);
	const VkImageMemoryBarrier			refSrcCopyImageBarrierPost	= makeImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, layoutShaderReadOnly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL, refSrcImage->get(), subresourceRange);
	const VkImageMemoryBarrier			resCompressedImageBarrier	= makeImageMemoryBarrier(0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, layoutShaderReadOnly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL, resCompressedImage->get(), resSubresourceRange);

	const Move<VkFramebuffer>			framebuffer					(makeFramebuffer(vk, device, *renderPass, 0, DE_NULL, renderSize, getLayerCount()));

	// Upload source image data
	{
		const Allocation& refSrcImageBufferAlloc = refSrcImageBuffer->getAllocation();
		deMemcpy(refSrcImageBufferAlloc.getHostPtr(), &refCompressedData[0], refCompressedData.size());
		flushMappedMemoryRange(vk, device, refSrcImageBufferAlloc.getMemory(), refSrcImageBufferAlloc.getOffset(), refCompressedData.size());
	}

	beginCommandBuffer(vk, *cmdBuffer);
	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);

	// Copy buffer to image
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1u, &refSrcCopyBufferBarrierPre, 1u, &refSrcCopyImageBarrierPre);
	vk.cmdCopyBufferToImage(*cmdBuffer, refSrcImageBuffer->get(), refSrcImage->get(), VK_IMAGE_LAYOUT_GENERAL, 1u, &copyBufferToImageRegion);
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, DE_NULL, 1u, &refSrcCopyImageBarrierPost);

	// Make reference and result images readable
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0u, DE_NULL, 1u, &refDstInitImageBarrier);
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0u, DE_NULL, 1u, &resDstInitImageBarrier);
	{
		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0u, DE_NULL, 1u, &resCompressedImageBarrier);
	}

	beginRenderPass(vk, *cmdBuffer, *renderPass, *framebuffer, renderSize);
	{
		DescriptorSetUpdateBuilder()
			.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &descriptorRefSrcImage)
			.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &descriptorResSrcImage)
			.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(2u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descriptorRefDstImage)
			.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(3u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descriptorResDstImage)
			.update(vk, device);

		vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);
		vk.cmdBindVertexBuffers(*cmdBuffer, 0, 1, &m_vertexBuffer->get(), &m_vertexBufferOffset);
		vk.cmdDraw(*cmdBuffer, m_vertexCount, 1, 0, 0);
	}
	vk.cmdEndRenderPass(*cmdBuffer);

	// Decompress reference image
	{
		const VkImageMemoryBarrier refDstImageBarrier = makeImageMemoryBarrier(
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
			refDstImage->get(), subresourceRange);

		const VkBufferMemoryBarrier refDstBufferBarrier = makeBufferMemoryBarrier(
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT,
			refDstBuffer->get(), 0ull, dstBufferSize);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &refDstImageBarrier);
		vk.cmdCopyImageToBuffer(*cmdBuffer, refDstImage->get(), VK_IMAGE_LAYOUT_GENERAL, refDstBuffer->get(), 1u, &copyRegion);
		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &refDstBufferBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);
	}

	// Decompress result image
	{
		const VkImageMemoryBarrier resDstImageBarrier = makeImageMemoryBarrier(
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
			resDstImage->get(), subresourceRange);

		const VkBufferMemoryBarrier resDstBufferBarrier = makeBufferMemoryBarrier(
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT,
			resDstBuffer->get(), 0ull, dstBufferSize);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &resDstImageBarrier);
		vk.cmdCopyImageToBuffer(*cmdBuffer, resDstImage->get(), VK_IMAGE_LAYOUT_GENERAL, resDstBuffer->get(), 1u, &copyRegion);
		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &resDstBufferBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);
	}

	endCommandBuffer(vk, *cmdBuffer);

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Compare decompressed pixel data in reference and result images
	{
		const Allocation&	refDstBufferAlloc	= refDstBuffer->getAllocation();
		invalidateMappedMemoryRange(vk, device, refDstBufferAlloc.getMemory(), refDstBufferAlloc.getOffset(), dstBufferSize);

		const Allocation&	resDstBufferAlloc	= resDstBuffer->getAllocation();
		invalidateMappedMemoryRange(vk, device, resDstBufferAlloc.getMemory(), resDstBufferAlloc.getOffset(), dstBufferSize);

		if (deMemCmp(refDstBufferAlloc.getHostPtr(), resDstBufferAlloc.getHostPtr(), (size_t)dstBufferSize) != 0)
		{
			// Do fuzzy to log error mask
			invalidateMappedMemoryRange(vk, device, resDstBufferAlloc.getMemory(), resDstBufferAlloc.getOffset(), dstBufferSize);
			invalidateMappedMemoryRange(vk, device, refDstBufferAlloc.getMemory(), refDstBufferAlloc.getOffset(), dstBufferSize);

			tcu::ConstPixelBufferAccess	resPixels	(mapVkFormat(m_parameters.formatForVerify), renderSize.width, renderSize.height, 1u, resDstBufferAlloc.getHostPtr());
			tcu::ConstPixelBufferAccess	refPixels	(mapVkFormat(m_parameters.formatForVerify), renderSize.width, renderSize.height, 1u, refDstBufferAlloc.getHostPtr());

			string	comment	= string("Image Comparison (level=") + de::toString(level) + string(", layer=") + de::toString(layer) + string(")");

			if (isWriteToCompressedOperation())
				tcu::fuzzyCompare(m_context.getTestContext().getLog(), "ImageComparison", comment.c_str(), refPixels, resPixels, 0.001f, tcu::COMPARE_LOG_EVERYTHING);
			else
				tcu::fuzzyCompare(m_context.getTestContext().getLog(), "ImageComparison", comment.c_str(), resPixels, refPixels, 0.001f, tcu::COMPARE_LOG_EVERYTHING);

			return false;
		}
	}

	return true;
}


class GraphicsTextureTestInstance : public GraphicsAttachmentsTestInstance
{
public:
						GraphicsTextureTestInstance		(Context& context, const TestParameters& parameters);

protected:
	virtual bool		isWriteToCompressedOperation	();
	virtual void		transcodeRead					();
	virtual void		transcodeWrite					();
};

GraphicsTextureTestInstance::GraphicsTextureTestInstance (Context& context, const TestParameters& parameters)
	: GraphicsAttachmentsTestInstance(context, parameters)
{
}

bool GraphicsTextureTestInstance::isWriteToCompressedOperation ()
{
	return (m_parameters.operation == OPERATION_TEXTURE_WRITE);
}

void GraphicsTextureTestInstance::transcodeRead ()
{
	const DeviceInterface&				vk						= m_context.getDeviceInterface();
	const VkDevice						device					= m_context.getDevice();
	const deUint32						queueFamilyIndex		= m_context.getUniversalQueueFamilyIndex();
	const VkQueue						queue					= m_context.getUniversalQueue();
	Allocator&							allocator				= m_context.getDefaultAllocator();

	const VkImageCreateFlags*			imgCreateFlagsOverride	= DE_NULL;

	const VkImageCreateInfo				srcImageCreateInfo		= makeCreateImageInfo(m_srcFormat, m_parameters.imageType, m_srcImageResolutions[0], m_srcImageUsageFlags, imgCreateFlagsOverride, getLevelCount(), getLayerCount());
	MovePtr<Image>						srcImage				(new Image(vk, device, allocator, srcImageCreateInfo, MemoryRequirement::Any));

	const Unique<VkShaderModule>		vertShaderModule		(createShaderModule(vk, device, m_context.getBinaryCollection().get("vert"), 0));
	const Unique<VkShaderModule>		fragShaderModule		(createShaderModule(vk, device, m_context.getBinaryCollection().get("frag"), 0));

	const Unique<VkRenderPass>			renderPass				(makeRenderPass(vk, device));

	const Move<VkDescriptorSetLayout>	descriptorSetLayout		(DescriptorSetLayoutBuilder()
																	.addSingleBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
																	.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT)
																	.build(vk, device));
	const Move<VkDescriptorPool>		descriptorPool			(DescriptorPoolBuilder()
																	.addType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
																	.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
																	.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));
	const Move<VkDescriptorSet>			descriptorSet			(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));

	const VkExtent2D					renderSizeDummy			(makeExtent2D(1u, 1u));
	const Unique<VkPipelineLayout>		pipelineLayout			(makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkPipeline>			pipeline				(makeGraphicsPipeline(vk, device, *pipelineLayout, *renderPass, *vertShaderModule, *fragShaderModule, renderSizeDummy, 0u, true));

	const Unique<VkCommandPool>			cmdPool					(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>		cmdBuffer				(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	for (deUint32 levelNdx = 0; levelNdx < getLevelCount(); ++levelNdx)
	{
		const UVec3&				uncompressedImageRes	= m_uncompressedImageResVec[levelNdx];
		const UVec3&				srcImageResolution		= m_srcImageResolutions[levelNdx];
		const UVec3&				dstImageResolution		= m_dstImageResolutions[levelNdx];
		const size_t				srcImageSizeInBytes		= m_srcData[levelNdx][0]->size();
		const size_t				dstImageSizeInBytes		= m_dstData[levelNdx][0]->size();
		const UVec3					srcImageResBlocked		= getCompressedImageResolutionBlockCeil(m_parameters.formatCompressed, srcImageResolution);

		const VkImageCreateInfo		dstImageCreateInfo		= makeCreateImageInfo(m_dstFormat, m_parameters.imageType, dstImageResolution, m_dstImageUsageFlags, imgCreateFlagsOverride, SINGLE_LEVEL, SINGLE_LAYER);

		const VkBufferCreateInfo	srcImageBufferInfo		= makeBufferCreateInfo(srcImageSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		const MovePtr<Buffer>		srcImageBuffer			= MovePtr<Buffer>(new Buffer(vk, device, allocator, srcImageBufferInfo, MemoryRequirement::HostVisible));

		const VkBufferCreateInfo	dstImageBufferInfo		= makeBufferCreateInfo(dstImageSizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		MovePtr<Buffer>				dstImageBuffer			= MovePtr<Buffer>(new Buffer(vk, device, allocator, dstImageBufferInfo, MemoryRequirement::HostVisible));

		const VkExtent2D			renderSize				(makeExtent2D(uncompressedImageRes.x(), uncompressedImageRes.y()));
		const VkViewport			viewport				= makeViewport(0.0f, 0.0f, static_cast<float>(renderSize.width), static_cast<float>(renderSize.height), 0.0f, 1.0f);
		const VkRect2D				scissor					= makeScissor(renderSize.width, renderSize.height);

		for (deUint32 layerNdx = 0; layerNdx < getLayerCount(); ++layerNdx)
		{
			const VkImageSubresourceRange	srcSubresourceRange		= makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, levelNdx, SINGLE_LEVEL, layerNdx, SINGLE_LAYER);
			const VkImageSubresourceRange	dstSubresourceRange		= makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, SINGLE_LEVEL, 0u, SINGLE_LAYER);

			Move<VkImageView>				srcImageView			(makeImageView(vk, device, srcImage->get(), mapImageViewType(m_parameters.imageType), m_parameters.formatUncompressed, srcSubresourceRange, m_srcImageViewUsageKHR));

			de::MovePtr<Image>				dstImage				(new Image(vk, device, allocator, dstImageCreateInfo, MemoryRequirement::Any));
			Move<VkImageView>				dstImageView			(makeImageView(vk, device, dstImage->get(), mapImageViewType(m_parameters.imageType), m_parameters.formatUncompressed, dstSubresourceRange, m_dstImageViewUsageKHR));

			const VkSamplerCreateInfo		srcSamplerInfo			(makeSamplerCreateInfo());
			const Move<VkSampler>			srcSampler				= vk::createSampler(vk, device, &srcSamplerInfo);
			const VkDescriptorImageInfo		descriptorSrcImage		(makeDescriptorImageInfo(*srcSampler, *srcImageView, VK_IMAGE_LAYOUT_GENERAL));
			const VkDescriptorImageInfo		descriptorDstImage		(makeDescriptorImageInfo(DE_NULL, *dstImageView, VK_IMAGE_LAYOUT_GENERAL));

			const VkBufferImageCopy			srcCopyRegion			= makeBufferImageCopy(srcImageResolution.x(), srcImageResolution.y(), levelNdx, layerNdx, srcImageResBlocked.x(), srcImageResBlocked.y());
			const VkBufferMemoryBarrier		srcCopyBufferBarrierPre	= makeBufferMemoryBarrier(VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, srcImageBuffer->get(), 0ull, srcImageSizeInBytes);
			const VkImageMemoryBarrier		srcCopyImageBarrierPre	= makeImageMemoryBarrier(0u, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, srcImage->get(), srcSubresourceRange);
			const VkImageMemoryBarrier		srcCopyImageBarrierPost	= makeImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, srcImage->get(), srcSubresourceRange);
			const VkBufferImageCopy			dstCopyRegion			= makeBufferImageCopy(dstImageResolution.x(), dstImageResolution.y());
			const VkImageMemoryBarrier		dstInitImageBarrier		= makeImageMemoryBarrier(0u, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, dstImage->get(), dstSubresourceRange);

			const VkExtent2D				framebufferSize			(makeExtent2D(dstImageResolution[0], dstImageResolution[1]));
			const Move<VkFramebuffer>		framebuffer				(makeFramebuffer(vk, device, *renderPass, 0, DE_NULL, framebufferSize, SINGLE_LAYER));

			// Upload source image data
			const Allocation& alloc = srcImageBuffer->getAllocation();
			deMemcpy(alloc.getHostPtr(), &m_srcData[levelNdx][layerNdx]->at(0), srcImageSizeInBytes);
			flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), srcImageSizeInBytes);

			beginCommandBuffer(vk, *cmdBuffer);
			vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);

			// Copy buffer to image
			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1u, &srcCopyBufferBarrierPre, 1u, &srcCopyImageBarrierPre);
			vk.cmdCopyBufferToImage(*cmdBuffer, srcImageBuffer->get(), srcImage->get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &srcCopyRegion);
			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0u, DE_NULL, 1u, &srcCopyImageBarrierPost);

			// Define destination image layout
			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0u, DE_NULL, 1u, &dstInitImageBarrier);

			beginRenderPass(vk, *cmdBuffer, *renderPass, *framebuffer, renderSize);

			DescriptorSetUpdateBuilder()
				.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &descriptorSrcImage)
				.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descriptorDstImage)
				.update(vk, device);

			vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);
			vk.cmdBindVertexBuffers(*cmdBuffer, 0u, 1u, &m_vertexBuffer->get(), &m_vertexBufferOffset);

			vk.cmdSetViewport(*cmdBuffer, 0u, 1u, &viewport);
			vk.cmdSetScissor(*cmdBuffer, 0u, 1u, &scissor);

			vk.cmdDraw(*cmdBuffer, (deUint32)m_vertexCount, 1, 0, 0);

			vk.cmdEndRenderPass(*cmdBuffer);

			const VkImageMemoryBarrier prepareForTransferBarrier = makeImageMemoryBarrier(
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
				dstImage->get(), dstSubresourceRange);

			const VkBufferMemoryBarrier copyBarrier = makeBufferMemoryBarrier(
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT,
				dstImageBuffer->get(), 0ull, dstImageSizeInBytes);

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &prepareForTransferBarrier);
			vk.cmdCopyImageToBuffer(*cmdBuffer, dstImage->get(), VK_IMAGE_LAYOUT_GENERAL, dstImageBuffer->get(), 1u, &dstCopyRegion);
			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &copyBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);

			endCommandBuffer(vk, *cmdBuffer);

			submitCommandsAndWait(vk, device, queue, *cmdBuffer);

			const Allocation& dstImageBufferAlloc = dstImageBuffer->getAllocation();
			invalidateMappedMemoryRange(vk, device, dstImageBufferAlloc.getMemory(), dstImageBufferAlloc.getOffset(), dstImageSizeInBytes);
			deMemcpy(&m_dstData[levelNdx][layerNdx]->at(0), dstImageBufferAlloc.getHostPtr(), dstImageSizeInBytes);
		}
	}

	m_compressedImage = srcImage;
}

void GraphicsTextureTestInstance::transcodeWrite ()
{
	const DeviceInterface&				vk						= m_context.getDeviceInterface();
	const VkDevice						device					= m_context.getDevice();
	const deUint32						queueFamilyIndex		= m_context.getUniversalQueueFamilyIndex();
	const VkQueue						queue					= m_context.getUniversalQueue();
	Allocator&							allocator				= m_context.getDefaultAllocator();

	const VkImageCreateFlags*			imgCreateFlagsOverride	= DE_NULL;

	const VkImageCreateInfo				dstImageCreateInfo		= makeCreateImageInfo(m_dstFormat, m_parameters.imageType, m_dstImageResolutions[0], m_dstImageUsageFlags, imgCreateFlagsOverride, getLevelCount(), getLayerCount());
	MovePtr<Image>						dstImage				(new Image(vk, device, allocator, dstImageCreateInfo, MemoryRequirement::Any));

	const Unique<VkShaderModule>		vertShaderModule		(createShaderModule(vk, device, m_context.getBinaryCollection().get("vert"), 0));
	const Unique<VkShaderModule>		fragShaderModule		(createShaderModule(vk, device, m_context.getBinaryCollection().get("frag"), 0));

	const Unique<VkRenderPass>			renderPass				(makeRenderPass(vk, device));

	const Move<VkDescriptorSetLayout>	descriptorSetLayout		(DescriptorSetLayoutBuilder()
																	.addSingleBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
																	.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT)
																	.build(vk, device));
	const Move<VkDescriptorPool>		descriptorPool			(DescriptorPoolBuilder()
																	.addType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
																	.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
																	.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));
	const Move<VkDescriptorSet>			descriptorSet			(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));

	const VkExtent2D					renderSizeDummy			(makeExtent2D(1u, 1u));
	const Unique<VkPipelineLayout>		pipelineLayout			(makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkPipeline>			pipeline				(makeGraphicsPipeline(vk, device, *pipelineLayout, *renderPass, *vertShaderModule, *fragShaderModule, renderSizeDummy, 0u, true));

	const Unique<VkCommandPool>			cmdPool					(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>		cmdBuffer				(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	for (deUint32 levelNdx = 0; levelNdx < getLevelCount(); ++levelNdx)
	{
		const UVec3&				uncompressedImageRes	= m_uncompressedImageResVec[levelNdx];
		const UVec3&				srcImageResolution		= m_srcImageResolutions[levelNdx];
		const UVec3&				dstImageResolution		= m_dstImageResolutions[levelNdx];
		const size_t				srcImageSizeInBytes		= m_srcData[levelNdx][0]->size();
		const size_t				dstImageSizeInBytes		= m_dstData[levelNdx][0]->size();
		const UVec3					dstImageResBlocked		= getCompressedImageResolutionBlockCeil(m_parameters.formatCompressed, dstImageResolution);

		const VkImageCreateInfo		srcImageCreateInfo		= makeCreateImageInfo(m_srcFormat, m_parameters.imageType, srcImageResolution, m_srcImageUsageFlags, imgCreateFlagsOverride, SINGLE_LEVEL, SINGLE_LAYER);

		const VkExtent2D			renderSize				(makeExtent2D(uncompressedImageRes.x(), uncompressedImageRes.y()));
		const VkViewport			viewport				= makeViewport(0.0f, 0.0f, static_cast<float>(renderSize.width), static_cast<float>(renderSize.height), 0.0f, 1.0f);
		const VkRect2D				scissor					= makeScissor(renderSize.width, renderSize.height);

		for (deUint32 layerNdx = 0; layerNdx < getLayerCount(); ++layerNdx)
		{
			const VkBufferCreateInfo		srcImageBufferInfo		= makeBufferCreateInfo(srcImageSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
			const MovePtr<Buffer>			srcImageBuffer			= MovePtr<Buffer>(new Buffer(vk, device, allocator, srcImageBufferInfo, MemoryRequirement::HostVisible));

			const VkBufferCreateInfo		dstImageBufferInfo		= makeBufferCreateInfo(dstImageSizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
			MovePtr<Buffer>					dstImageBuffer			= MovePtr<Buffer>(new Buffer(vk, device, allocator, dstImageBufferInfo, MemoryRequirement::HostVisible));

			const VkImageSubresourceRange	srcSubresourceRange		= makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, SINGLE_LEVEL, 0u, SINGLE_LAYER);
			const VkImageSubresourceRange	dstSubresourceRange		= makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, levelNdx, SINGLE_LEVEL, layerNdx, SINGLE_LAYER);

			Move<VkImageView>				dstImageView			(makeImageView(vk, device, dstImage->get(), mapImageViewType(m_parameters.imageType), m_parameters.formatUncompressed, dstSubresourceRange, m_dstImageViewUsageKHR));

			de::MovePtr<Image>				srcImage				(new Image(vk, device, allocator, srcImageCreateInfo, MemoryRequirement::Any));
			Move<VkImageView>				srcImageView			(makeImageView(vk, device, srcImage->get(), mapImageViewType(m_parameters.imageType), m_parameters.formatUncompressed, srcSubresourceRange, m_srcImageViewUsageKHR));

			const VkSamplerCreateInfo		srcSamplerInfo			(makeSamplerCreateInfo());
			const Move<VkSampler>			srcSampler				= vk::createSampler(vk, device, &srcSamplerInfo);
			const VkDescriptorImageInfo		descriptorSrcImage		(makeDescriptorImageInfo(*srcSampler, *srcImageView, VK_IMAGE_LAYOUT_GENERAL));
			const VkDescriptorImageInfo		descriptorDstImage		(makeDescriptorImageInfo(DE_NULL, *dstImageView, VK_IMAGE_LAYOUT_GENERAL));

			const VkBufferImageCopy			srcCopyRegion			= makeBufferImageCopy(srcImageResolution.x(), srcImageResolution.y(), 0u, 0u);
			const VkBufferMemoryBarrier		srcCopyBufferBarrierPre	= makeBufferMemoryBarrier(VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, srcImageBuffer->get(), 0ull, srcImageSizeInBytes);
			const VkImageMemoryBarrier		srcCopyImageBarrierPre	= makeImageMemoryBarrier(0u, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, srcImage->get(), srcSubresourceRange);
			const VkImageMemoryBarrier		srcCopyImageBarrierPost	= makeImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, srcImage->get(), srcSubresourceRange);
			const VkBufferImageCopy			dstCopyRegion			= makeBufferImageCopy(dstImageResolution.x(), dstImageResolution.y(), levelNdx, layerNdx, dstImageResBlocked.x(), dstImageResBlocked.y());
			const VkImageMemoryBarrier		dstInitImageBarrier		= makeImageMemoryBarrier(0u, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, dstImage->get(), dstSubresourceRange);

			const VkExtent2D				framebufferSize			(makeExtent2D(dstImageResolution[0], dstImageResolution[1]));
			const Move<VkFramebuffer>		framebuffer				(makeFramebuffer(vk, device, *renderPass, 0, DE_NULL, framebufferSize, SINGLE_LAYER));

			// Upload source image data
			const Allocation& alloc = srcImageBuffer->getAllocation();
			deMemcpy(alloc.getHostPtr(), &m_srcData[levelNdx][layerNdx]->at(0), srcImageSizeInBytes);
			flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), srcImageSizeInBytes);

			beginCommandBuffer(vk, *cmdBuffer);
			vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);

			// Copy buffer to image
			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1u, &srcCopyBufferBarrierPre, 1u, &srcCopyImageBarrierPre);
			vk.cmdCopyBufferToImage(*cmdBuffer, srcImageBuffer->get(), srcImage->get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &srcCopyRegion);
			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0u, DE_NULL, 1u, &srcCopyImageBarrierPost);

			// Define destination image layout
			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0u, DE_NULL, 1u, &dstInitImageBarrier);

			beginRenderPass(vk, *cmdBuffer, *renderPass, *framebuffer, renderSize);

			DescriptorSetUpdateBuilder()
				.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &descriptorSrcImage)
				.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descriptorDstImage)
				.update(vk, device);

			vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);
			vk.cmdBindVertexBuffers(*cmdBuffer, 0u, 1u, &m_vertexBuffer->get(), &m_vertexBufferOffset);

			vk.cmdSetViewport(*cmdBuffer, 0u, 1u, &viewport);
			vk.cmdSetScissor(*cmdBuffer, 0u, 1u, &scissor);

			vk.cmdDraw(*cmdBuffer, (deUint32)m_vertexCount, 1, 0, 0);

			vk.cmdEndRenderPass(*cmdBuffer);

			const VkImageMemoryBarrier prepareForTransferBarrier = makeImageMemoryBarrier(
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
				dstImage->get(), dstSubresourceRange);

			const VkBufferMemoryBarrier copyBarrier = makeBufferMemoryBarrier(
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT,
				dstImageBuffer->get(), 0ull, dstImageSizeInBytes);

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &prepareForTransferBarrier);
			vk.cmdCopyImageToBuffer(*cmdBuffer, dstImage->get(), VK_IMAGE_LAYOUT_GENERAL, dstImageBuffer->get(), 1u, &dstCopyRegion);
			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &copyBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);

			endCommandBuffer(vk, *cmdBuffer);

			submitCommandsAndWait(vk, device, queue, *cmdBuffer);

			const Allocation& dstImageBufferAlloc = dstImageBuffer->getAllocation();
			invalidateMappedMemoryRange(vk, device, dstImageBufferAlloc.getMemory(), dstImageBufferAlloc.getOffset(), dstImageSizeInBytes);
			deMemcpy(&m_dstData[levelNdx][layerNdx]->at(0), dstImageBufferAlloc.getHostPtr(), dstImageSizeInBytes);
		}
	}

	m_compressedImage = dstImage;
}

class TexelViewCompatibleCase : public TestCase
{
public:
							TexelViewCompatibleCase		(TestContext&				testCtx,
														 const std::string&			name,
														 const std::string&			desc,
														 const TestParameters&		parameters);
	void					initPrograms				(SourceCollections&			programCollection) const;
	TestInstance*			createInstance				(Context&					context) const;
protected:
	const TestParameters	m_parameters;
};

TexelViewCompatibleCase::TexelViewCompatibleCase (TestContext& testCtx, const std::string& name, const std::string& desc, const TestParameters& parameters)
	: TestCase				(testCtx, name, desc)
	, m_parameters			(parameters)
{
}

void TexelViewCompatibleCase::initPrograms (vk::SourceCollections&	programCollection) const
{
	DE_ASSERT(m_parameters.size.x() > 0);
	DE_ASSERT(m_parameters.size.y() > 0);

	switch (m_parameters.shader)
	{
		case SHADER_TYPE_COMPUTE:
		{
			const std::string	imageTypeStr		= getShaderImageType(mapVkFormat(m_parameters.formatUncompressed), m_parameters.imageType);
			const std::string	formatQualifierStr	= getShaderImageFormatQualifier(mapVkFormat(m_parameters.formatUncompressed));
			std::ostringstream	src;
			std::ostringstream	src_decompress;

			src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
				<< "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n\n";
			src_decompress << src.str();

			switch(m_parameters.operation)
			{
				case OPERATION_IMAGE_LOAD:
				{
					src << "layout (binding = 0, "<<formatQualifierStr<<") readonly uniform "<<imageTypeStr<<" u_image0;\n"
						<< "layout (binding = 1, "<<formatQualifierStr<<") writeonly uniform "<<imageTypeStr<<" u_image1;\n\n"
						<< "void main (void)\n"
						<< "{\n"
						<< "    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);\n"
						<< "    imageStore(u_image1, pos, imageLoad(u_image0, pos));\n"
						<< "}\n";

					break;
				}

				case OPERATION_TEXEL_FETCH:
				{
					src << "layout (binding = 0) uniform "<<getGlslSamplerType(mapVkFormat(m_parameters.formatUncompressed), mapImageViewType(m_parameters.imageType))<<" u_image0;\n"
						<< "layout (binding = 1, "<<formatQualifierStr<<") writeonly uniform "<<imageTypeStr<<" u_image1;\n\n"
						<< "void main (void)\n"
						<< "{\n"
						<< "    ivec3 pos = ivec3(gl_GlobalInvocationID.xyz);\n"
						<< "    imageStore(u_image1, pos.xy, texelFetch(u_image0, pos.xy, pos.z));\n"
						<< "}\n";

					break;
				}

				case OPERATION_TEXTURE:
				{
					src << "layout (binding = 0) uniform "<<getGlslSamplerType(mapVkFormat(m_parameters.formatUncompressed), mapImageViewType(m_parameters.imageType))<<" u_image0;\n"
						<< "layout (binding = 1, "<<formatQualifierStr<<") writeonly uniform "<<imageTypeStr<<" u_image1;\n\n"
						<< "void main (void)\n"
						<< "{\n"
						<< "    const vec2 pixels_resolution = vec2(gl_NumWorkGroups.x - 1, gl_NumWorkGroups.y - 1);\n"
						<< "    const ivec2 pos = ivec2(gl_GlobalInvocationID.xy);\n"
						<< "    const vec2 coord = vec2(gl_GlobalInvocationID.xy) / vec2(pixels_resolution);\n"
						<< "    imageStore(u_image1, pos, texture(u_image0, coord));\n"
						<< "}\n";

					break;
				}

				case OPERATION_IMAGE_STORE:
				{
					src << "layout (binding = 0, "<<formatQualifierStr<<") uniform "<<imageTypeStr<<"           u_image0;\n"
						<< "layout (binding = 1, "<<formatQualifierStr<<") readonly uniform "<<imageTypeStr<<"  u_image1;\n"
						<< "layout (binding = 2, "<<formatQualifierStr<<") writeonly uniform "<<imageTypeStr<<" u_image2;\n\n"
						<< "void main (void)\n"
						<< "{\n"
						<< "    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);\n"
						<< "    imageStore(u_image0, pos, imageLoad(u_image1, pos));\n"
						<< "    imageStore(u_image2, pos, imageLoad(u_image0, pos));\n"
						<< "}\n";

					break;
				}

				default:
					DE_ASSERT(false);
			}

			src_decompress	<< "layout (binding = 0) uniform "<<getGlslSamplerType(mapVkFormat(m_parameters.formatUncompressed), mapImageViewType(m_parameters.imageType))<<" compressed_result;\n"
							<< "layout (binding = 1) uniform "<<getGlslSamplerType(mapVkFormat(m_parameters.formatUncompressed), mapImageViewType(m_parameters.imageType))<<" compressed_reference;\n"
							<< "layout (binding = 2, "<<formatQualifierStr<<") writeonly uniform "<<imageTypeStr<<" decompressed_result;\n"
							<< "layout (binding = 3, "<<formatQualifierStr<<") writeonly uniform "<<imageTypeStr<<" decompressed_reference;\n\n"
							<< "void main (void)\n"
							<< "{\n"
							<< "    const vec2 pixels_resolution = vec2(gl_NumWorkGroups.xy);\n"
							<< "    const vec2 cord = vec2(gl_GlobalInvocationID.xy) / vec2(pixels_resolution);\n"
							<< "    const ivec2 pos = ivec2(gl_GlobalInvocationID.xy); \n"
							<< "    imageStore(decompressed_result, pos, texture(compressed_result, cord));\n"
							<< "    imageStore(decompressed_reference, pos, texture(compressed_reference, cord));\n"
							<< "}\n";
			programCollection.glslSources.add("comp") << glu::ComputeSource(src.str());
			programCollection.glslSources.add("decompress") << glu::ComputeSource(src_decompress.str());

			break;
		}

		case SHADER_TYPE_FRAGMENT:
		{
			ImageType	imageTypeForFS = (m_parameters.imageType == IMAGE_TYPE_2D_ARRAY) ? IMAGE_TYPE_2D : m_parameters.imageType;

			// Vertex shader
			{
				std::ostringstream src;
				src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n\n"
					<< "layout(location = 0) in vec4 v_in_position;\n"
					<< "\n"
					<< "void main (void)\n"
					<< "{\n"
					<< "    gl_Position = v_in_position;\n"
					<< "}\n";

				programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
			}

			// Fragment shader
			{
				switch(m_parameters.operation)
				{
					case OPERATION_ATTACHMENT_READ:
					case OPERATION_ATTACHMENT_WRITE:
					{
						std::ostringstream	src;

						const std::string	dstTypeStr	= getGlslFormatType(m_parameters.formatUncompressed);
						const std::string	srcTypeStr	= getGlslInputFormatType(m_parameters.formatUncompressed);

						src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n\n"
							<< "precision highp int;\n"
							<< "precision highp float;\n"
							<< "\n"
							<< "layout (location = 0) out highp " << dstTypeStr << " o_color;\n"
							<< "layout (input_attachment_index = 0, set = 0, binding = 0) uniform highp " << srcTypeStr << " inputImage1;\n"
							<< "\n"
							<< "void main (void)\n"
							<< "{\n"
							<< "    o_color = " << dstTypeStr << "(subpassLoad(inputImage1));\n"
							<< "}\n";

						programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());

						break;
					}

					case OPERATION_TEXTURE_READ:
					case OPERATION_TEXTURE_WRITE:
					{
						std::ostringstream	src;

						const std::string	srcSamplerTypeStr		= getGlslSamplerType(mapVkFormat(m_parameters.formatUncompressed), mapImageViewType(imageTypeForFS));
						const std::string	dstImageTypeStr			= getShaderImageType(mapVkFormat(m_parameters.formatUncompressed), imageTypeForFS);
						const std::string	dstFormatQualifierStr	= getShaderImageFormatQualifier(mapVkFormat(m_parameters.formatUncompressed));

						src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n\n"
							<< "layout (binding = 0) uniform " << srcSamplerTypeStr << " u_imageIn;\n"
							<< "layout (binding = 1, " << dstFormatQualifierStr << ") writeonly uniform " << dstImageTypeStr << " u_imageOut;\n"
							<< "\n"
							<< "void main (void)\n"
							<< "{\n"
							<< "    const ivec2 out_pos = ivec2(gl_FragCoord.xy);\n"
							<< "    const ivec2 pixels_resolution = ivec2(textureSize(u_imageIn, 0)) - ivec2(1,1);\n"
							<< "    const vec2 in_pos = vec2(out_pos) / vec2(pixels_resolution);\n"
							<< "    imageStore(u_imageOut, out_pos, texture(u_imageIn, in_pos));\n"
							<< "}\n";

						programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());

						break;
					}

					default:
						DE_ASSERT(false);
				}
			}

			// Verification fragment shader
			{
				std::ostringstream	src;

				const std::string	samplerType			= getGlslSamplerType(mapVkFormat(m_parameters.formatForVerify), mapImageViewType(imageTypeForFS));
				const std::string	imageTypeStr		= getShaderImageType(mapVkFormat(m_parameters.formatForVerify), imageTypeForFS);
				const std::string	formatQualifierStr	= getShaderImageFormatQualifier(mapVkFormat(m_parameters.formatForVerify));

				src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n\n"
					<< "layout (binding = 0) uniform " << samplerType << " u_imageIn0;\n"
					<< "layout (binding = 1) uniform " << samplerType << " u_imageIn1;\n"
					<< "layout (binding = 2, " << formatQualifierStr << ") writeonly uniform " << imageTypeStr << " u_imageOut0;\n"
					<< "layout (binding = 3, " << formatQualifierStr << ") writeonly uniform " << imageTypeStr << " u_imageOut1;\n"
					<< "\n"
					<< "void main (void)\n"
					<< "{\n"
					<< "    const ivec2 out_pos = ivec2(gl_FragCoord.xy);\n"
					<< "\n"
					<< "    const ivec2 pixels_resolution0 = ivec2(textureSize(u_imageIn0, 0)) - ivec2(1,1);\n"
					<< "    const vec2 in_pos0 = vec2(out_pos) / vec2(pixels_resolution0);\n"
					<< "    imageStore(u_imageOut0, out_pos, texture(u_imageIn0, in_pos0));\n"
					<< "\n"
					<< "    const ivec2 pixels_resolution1 = ivec2(textureSize(u_imageIn1, 0)) - ivec2(1,1);\n"
					<< "    const vec2 in_pos1 = vec2(out_pos) / vec2(pixels_resolution1);\n"
					<< "    imageStore(u_imageOut1, out_pos, texture(u_imageIn1, in_pos1));\n"
					<< "}\n";

				programCollection.glslSources.add("frag_verify") << glu::FragmentSource(src.str());
			}

			break;
		}

		default:
			DE_ASSERT(false);
	}
}

TestInstance* TexelViewCompatibleCase::createInstance (Context& context) const
{
	const VkPhysicalDevice			physicalDevice			= context.getPhysicalDevice();
	const InstanceInterface&		vk						= context.getInstanceInterface();

	if (!m_parameters.useMipmaps)
	{
		DE_ASSERT(getNumLayers(m_parameters.imageType, m_parameters.size)     == 1u);
		DE_ASSERT(getLayerSize(m_parameters.imageType, m_parameters.size).z() == 1u);
	}

	DE_ASSERT(getLayerSize(m_parameters.imageType, m_parameters.size).x() >  0u);
	DE_ASSERT(getLayerSize(m_parameters.imageType, m_parameters.size).y() >  0u);

	if (std::find(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), "VK_KHR_maintenance2") == context.getDeviceExtensions().end())
		TCU_THROW(NotSupportedError, "Extension VK_KHR_maintenance2 not supported");

	{
		VkImageFormatProperties imageFormatProperties;

		if (VK_ERROR_FORMAT_NOT_SUPPORTED == vk.getPhysicalDeviceImageFormatProperties(physicalDevice, m_parameters.formatUncompressed,
												mapImageType(m_parameters.imageType), VK_IMAGE_TILING_OPTIMAL,
												m_parameters.uncompressedImageUsage, 0u, &imageFormatProperties))
			TCU_THROW(NotSupportedError, "Operation not supported with this image format");

		if (VK_ERROR_FORMAT_NOT_SUPPORTED == vk.getPhysicalDeviceImageFormatProperties(physicalDevice, m_parameters.formatCompressed,
												mapImageType(m_parameters.imageType), VK_IMAGE_TILING_OPTIMAL,
												VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
												VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT_KHR | VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT_KHR,
												&imageFormatProperties))
			TCU_THROW(NotSupportedError, "Operation not supported with this image format");
	}

	{
		const VkPhysicalDeviceFeatures	physicalDeviceFeatures	= getPhysicalDeviceFeatures (vk, physicalDevice);

		if (deInRange32(m_parameters.formatCompressed, VK_FORMAT_BC1_RGB_UNORM_BLOCK, VK_FORMAT_BC7_SRGB_BLOCK) &&
			!physicalDeviceFeatures.textureCompressionBC)
			TCU_THROW(NotSupportedError, "textureCompressionBC not supported");

		if (deInRange32(m_parameters.formatCompressed, VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, VK_FORMAT_EAC_R11G11_SNORM_BLOCK) &&
			!physicalDeviceFeatures.textureCompressionETC2)
			TCU_THROW(NotSupportedError, "textureCompressionETC2 not supported");

		if (deInRange32(m_parameters.formatCompressed, VK_FORMAT_ASTC_4x4_UNORM_BLOCK, VK_FORMAT_ASTC_12x12_SRGB_BLOCK) &&
			!physicalDeviceFeatures.textureCompressionASTC_LDR)
			TCU_THROW(NotSupportedError, "textureCompressionASTC_LDR not supported");

		if ((m_parameters.uncompressedImageUsage & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) &&
			isStorageImageExtendedFormat(m_parameters.formatUncompressed) &&
			!physicalDeviceFeatures.shaderStorageImageExtendedFormats)
			TCU_THROW(NotSupportedError, "Storage view format requires shaderStorageImageExtended");
	}

	switch (m_parameters.shader)
	{
		case SHADER_TYPE_COMPUTE:
		{
			switch (m_parameters.operation)
			{
				case OPERATION_IMAGE_LOAD:
				case OPERATION_TEXEL_FETCH:
				case OPERATION_TEXTURE:
					return new BasicComputeTestInstance(context, m_parameters);
				case OPERATION_IMAGE_STORE:
					return new ImageStoreComputeTestInstance(context, m_parameters);
				default:
					TCU_THROW(InternalError, "Impossible");
			}
		}

		case SHADER_TYPE_FRAGMENT:
		{
			switch (m_parameters.operation)
			{
				case OPERATION_ATTACHMENT_READ:
				case OPERATION_ATTACHMENT_WRITE:
					return new GraphicsAttachmentsTestInstance(context, m_parameters);

				case OPERATION_TEXTURE_READ:
				case OPERATION_TEXTURE_WRITE:
					return new GraphicsTextureTestInstance(context, m_parameters);

				default:
					TCU_THROW(InternalError, "Impossible");
			}
		}

		default:
			TCU_THROW(InternalError, "Impossible");
	}
}

} // anonymous ns

static tcu::UVec3 getUnniceResolution(const VkFormat format, const deUint32 layers)
{
	const deUint32	unniceMipmapTextureSize[]	= { 1, 1, 1, 8, 22, 48, 117, 275, 604, 208, 611, 274, 1211 };
	const deUint32	baseTextureWidth			= unniceMipmapTextureSize[getBlockWidth(format)];
	const deUint32	baseTextureHeight			= unniceMipmapTextureSize[getBlockHeight(format)];
	const deUint32	baseTextureWidthLevels		= deLog2Floor32(baseTextureWidth);
	const deUint32	baseTextureHeightLevels		= deLog2Floor32(baseTextureHeight);
	const deUint32	widthMultiplier				= (baseTextureHeightLevels > baseTextureWidthLevels) ? 1u << (baseTextureHeightLevels - baseTextureWidthLevels) : 1u;
	const deUint32	heightMultiplier			= (baseTextureWidthLevels > baseTextureHeightLevels) ? 1u << (baseTextureWidthLevels - baseTextureHeightLevels) : 1u;
	const deUint32	width						= baseTextureWidth * widthMultiplier;
	const deUint32	height						= baseTextureHeight * heightMultiplier;

	// Number of levels should be same on both axises
	DE_ASSERT(deLog2Floor32(width) == deLog2Floor32(height));

	return tcu::UVec3(width, height, layers);
}

tcu::TestCaseGroup* createImageCompressionTranscodingTests (tcu::TestContext& testCtx)
{
	struct FormatsArray
	{
		const VkFormat*	formats;
		deUint32		count;
	};

	const bool					mipmapness[]									=
	{
		false,
		true,
	};

	const std::string			pipelineName[SHADER_TYPE_LAST]					=
	{
		"compute",
		"graphic",
	};

	const std::string			mipmanpnessName[DE_LENGTH_OF_ARRAY(mipmapness)]	=
	{
		"basic",
		"extended",
	};

	const std::string			operationName[OPERATION_LAST]					=
	{
		"image_load",
		"texel_fetch",
		"texture",
		"image_store",
		"attachment_read",
		"attachment_write",
		"texture_read",
		"texture_write",
	};

	const VkImageUsageFlags		baseImageUsageFlagSet							= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	const VkImageUsageFlags		compressedImageUsageFlags[OPERATION_LAST]		=
	{
		baseImageUsageFlagSet | static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_STORAGE_BIT),											// "image_load"
		baseImageUsageFlagSet | static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT),				// "texel_fetch"
		baseImageUsageFlagSet | static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT),				// "texture"
		baseImageUsageFlagSet | static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT),				// "image_store"
		baseImageUsageFlagSet | static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),	// "attachment_read"
		baseImageUsageFlagSet | static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),	// "attachment_write"
		baseImageUsageFlagSet | static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_SAMPLED_BIT),											// "texture_read"
		baseImageUsageFlagSet | static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT),				// "texture_write"
	};

	const VkImageUsageFlags		compressedImageViewUsageFlags[OPERATION_LAST]	=
	{
		compressedImageUsageFlags[0],																									//"image_load"
		compressedImageUsageFlags[1],																									//"texel_fetch"
		compressedImageUsageFlags[2],																									//"texture"
		compressedImageUsageFlags[3],																									//"image_store"
		compressedImageUsageFlags[4],																									//"attachment_read"
		compressedImageUsageFlags[5] | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,																//"attachment_write"
		compressedImageUsageFlags[6],																									//"texture_read"
		compressedImageUsageFlags[7],																									//"texture_write"
	};

	const VkImageUsageFlags		uncompressedImageUsageFlags[OPERATION_LAST]		=
	{
		baseImageUsageFlagSet | static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_STORAGE_BIT),											//"image_load"
		baseImageUsageFlagSet | static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT),				//"texel_fetch"
		baseImageUsageFlagSet | static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT),				//"texture"
		baseImageUsageFlagSet | static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT),				//"image_store"
		baseImageUsageFlagSet | static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),	//"attachment_read"
		baseImageUsageFlagSet | static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT),									//"attachment_write"
		baseImageUsageFlagSet | static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),				//"texture_read"
		baseImageUsageFlagSet | static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_SAMPLED_BIT),											//"texture_write"
	};

	const VkFormat				compressedFormats64bit[]						=
	{
		VK_FORMAT_BC1_RGB_UNORM_BLOCK,
		VK_FORMAT_BC1_RGB_SRGB_BLOCK,
		VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
		VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
		VK_FORMAT_BC4_UNORM_BLOCK,
		VK_FORMAT_BC4_SNORM_BLOCK,
		VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
		VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
		VK_FORMAT_EAC_R11_UNORM_BLOCK,
		VK_FORMAT_EAC_R11_SNORM_BLOCK,
	};

	const VkFormat				compressedFormats128bit[]						=
	{
		VK_FORMAT_BC2_UNORM_BLOCK,
		VK_FORMAT_BC2_SRGB_BLOCK,
		VK_FORMAT_BC3_UNORM_BLOCK,
		VK_FORMAT_BC3_SRGB_BLOCK,
		VK_FORMAT_BC5_UNORM_BLOCK,
		VK_FORMAT_BC5_SNORM_BLOCK,
		VK_FORMAT_BC6H_UFLOAT_BLOCK,
		VK_FORMAT_BC6H_SFLOAT_BLOCK,
		VK_FORMAT_BC7_UNORM_BLOCK,
		VK_FORMAT_BC7_SRGB_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
		VK_FORMAT_EAC_R11G11_UNORM_BLOCK,
		VK_FORMAT_EAC_R11G11_SNORM_BLOCK,
		VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
		VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
		VK_FORMAT_ASTC_5x4_UNORM_BLOCK,
		VK_FORMAT_ASTC_5x4_SRGB_BLOCK,
		VK_FORMAT_ASTC_5x5_UNORM_BLOCK,
		VK_FORMAT_ASTC_5x5_SRGB_BLOCK,
		VK_FORMAT_ASTC_6x5_UNORM_BLOCK,
		VK_FORMAT_ASTC_6x5_SRGB_BLOCK,
		VK_FORMAT_ASTC_6x6_UNORM_BLOCK,
		VK_FORMAT_ASTC_6x6_SRGB_BLOCK,
		VK_FORMAT_ASTC_8x5_UNORM_BLOCK,
		VK_FORMAT_ASTC_8x5_SRGB_BLOCK,
		VK_FORMAT_ASTC_8x6_UNORM_BLOCK,
		VK_FORMAT_ASTC_8x6_SRGB_BLOCK,
		VK_FORMAT_ASTC_8x8_UNORM_BLOCK,
		VK_FORMAT_ASTC_8x8_SRGB_BLOCK,
		VK_FORMAT_ASTC_10x5_UNORM_BLOCK,
		VK_FORMAT_ASTC_10x5_SRGB_BLOCK,
		VK_FORMAT_ASTC_10x6_UNORM_BLOCK,
		VK_FORMAT_ASTC_10x6_SRGB_BLOCK,
		VK_FORMAT_ASTC_10x8_UNORM_BLOCK,
		VK_FORMAT_ASTC_10x8_SRGB_BLOCK,
		VK_FORMAT_ASTC_10x10_UNORM_BLOCK,
		VK_FORMAT_ASTC_10x10_SRGB_BLOCK,
		VK_FORMAT_ASTC_12x10_UNORM_BLOCK,
		VK_FORMAT_ASTC_12x10_SRGB_BLOCK,
		VK_FORMAT_ASTC_12x12_UNORM_BLOCK,
		VK_FORMAT_ASTC_12x12_SRGB_BLOCK,
	};

	const VkFormat				uncompressedFormats64bit[]						=
	{
		VK_FORMAT_R16G16B16A16_UNORM,
		VK_FORMAT_R16G16B16A16_SNORM,
		VK_FORMAT_R16G16B16A16_USCALED,
		VK_FORMAT_R16G16B16A16_SSCALED,
		VK_FORMAT_R16G16B16A16_UINT,
		VK_FORMAT_R16G16B16A16_SINT,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R32G32_UINT,
		VK_FORMAT_R32G32_SINT,
		VK_FORMAT_R32G32_SFLOAT,
		//VK_FORMAT_R64_UINT, remove from the test it couln'd not be use
		//VK_FORMAT_R64_SINT, remove from the test it couln'd not be use
		//VK_FORMAT_R64_SFLOAT, remove from the test it couln'd not be use
	};

	const VkFormat				uncompressedFormats128bit[]						=
	{
		VK_FORMAT_R32G32B32A32_UINT,
		VK_FORMAT_R32G32B32A32_SINT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		//VK_FORMAT_R64G64_UINT, remove from the test it couln'd not be use
		//VK_FORMAT_R64G64_SINT, remove from the test it couln'd not be use
		//VK_FORMAT_R64G64_SFLOAT, remove from the test it couln'd not be use
	};

	const FormatsArray			formatsCompressedSets[]							=
	{
		{
			compressedFormats64bit,
			DE_LENGTH_OF_ARRAY(compressedFormats64bit)
		},
		{
			compressedFormats128bit,
			DE_LENGTH_OF_ARRAY(compressedFormats128bit)
		},
	};

	const FormatsArray			formatsUncompressedSets[]						=
	{
		{
			uncompressedFormats64bit,
			DE_LENGTH_OF_ARRAY(uncompressedFormats64bit)
		},
		{
			uncompressedFormats128bit,
			DE_LENGTH_OF_ARRAY(uncompressedFormats128bit)
		},
	};

	DE_ASSERT(DE_LENGTH_OF_ARRAY(formatsCompressedSets) == DE_LENGTH_OF_ARRAY(formatsUncompressedSets));

	MovePtr<tcu::TestCaseGroup>	texelViewCompatibleTests							(new tcu::TestCaseGroup(testCtx, "texel_view_compatible", "Texel view compatible cases"));

	for (int shaderType = SHADER_TYPE_COMPUTE; shaderType < SHADER_TYPE_LAST; ++shaderType)
	{
		MovePtr<tcu::TestCaseGroup>	pipelineTypeGroup	(new tcu::TestCaseGroup(testCtx, pipelineName[shaderType].c_str(), ""));

		for (int mipmapTestNdx = 0; mipmapTestNdx < DE_LENGTH_OF_ARRAY(mipmapness); mipmapTestNdx++)
		{
			const bool mipmapTest = mipmapness[mipmapTestNdx];

			MovePtr<tcu::TestCaseGroup>	mipmapTypeGroup	(new tcu::TestCaseGroup(testCtx, mipmanpnessName[mipmapTestNdx].c_str(), ""));

			for (int operationNdx = OPERATION_IMAGE_LOAD; operationNdx < OPERATION_LAST; ++operationNdx)
			{
				if (shaderType != SHADER_TYPE_FRAGMENT && deInRange32(operationNdx, OPERATION_ATTACHMENT_READ, OPERATION_TEXTURE_WRITE))
					continue;

				if (shaderType != SHADER_TYPE_COMPUTE && deInRange32(operationNdx, OPERATION_IMAGE_LOAD, OPERATION_IMAGE_STORE))
					continue;

				MovePtr<tcu::TestCaseGroup>	imageOperationGroup	(new tcu::TestCaseGroup(testCtx, operationName[operationNdx].c_str(), ""));

				// Iterate through bitness groups (64 bit, 128 bit, etc)
				for (deUint32 formatBitnessGroup = 0; formatBitnessGroup < DE_LENGTH_OF_ARRAY(formatsCompressedSets); ++formatBitnessGroup)
				{
					for (deUint32 formatCompressedNdx = 0; formatCompressedNdx < formatsCompressedSets[formatBitnessGroup].count; ++formatCompressedNdx)
					{
						const VkFormat				formatCompressed			= formatsCompressedSets[formatBitnessGroup].formats[formatCompressedNdx];
						const std::string			compressedFormatGroupName	= getFormatShortString(formatCompressed);
						MovePtr<tcu::TestCaseGroup>	compressedFormatGroup		(new tcu::TestCaseGroup(testCtx, compressedFormatGroupName.c_str(), ""));

						for (deUint32 formatUncompressedNdx = 0; formatUncompressedNdx < formatsUncompressedSets[formatBitnessGroup].count; ++formatUncompressedNdx)
						{
							const VkFormat			formatUncompressed			= formatsUncompressedSets[formatBitnessGroup].formats[formatUncompressedNdx];
							const std::string		uncompressedFormatGroupName	= getFormatShortString(formatUncompressed);
							const TestParameters	parameters					=
							{
								static_cast<Operation>(operationNdx),
								static_cast<ShaderType>(shaderType),
								mipmapTest ? getUnniceResolution(formatCompressed, 3u) : UVec3(64u, 64u, 1u),
								IMAGE_TYPE_2D,
								formatCompressed,
								formatUncompressed,
								(operationNdx == OPERATION_IMAGE_STORE) ? 3u : 2u,
								compressedImageUsageFlags[operationNdx],
								compressedImageViewUsageFlags[operationNdx],
								uncompressedImageUsageFlags[operationNdx],
								mipmapTest,
								VK_FORMAT_R8G8B8A8_UNORM
							};

							compressedFormatGroup->addChild(new TexelViewCompatibleCase(testCtx, uncompressedFormatGroupName, "", parameters));
						}

						imageOperationGroup->addChild(compressedFormatGroup.release());
					}
				}

				mipmapTypeGroup->addChild(imageOperationGroup.release());
			}

			pipelineTypeGroup->addChild(mipmapTypeGroup.release());
		}

		texelViewCompatibleTests->addChild(pipelineTypeGroup.release());
	}

	return texelViewCompatibleTests.release();
}

} // image
} // vkt
