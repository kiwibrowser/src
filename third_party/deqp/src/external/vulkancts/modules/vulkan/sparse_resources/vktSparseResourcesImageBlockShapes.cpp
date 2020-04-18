/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
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
 * \file  vktSparseResourcesImageBlockShapes.cpp
 * \brief Standard block shape tests.
 *//*--------------------------------------------------------------------*/

#include "vktSparseResourcesBufferSparseBinding.hpp"
#include "vktSparseResourcesTestsUtil.hpp"
#include "vktSparseResourcesBase.hpp"
#include "vktTestCaseUtil.hpp"

#include "vkDefs.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkPlatform.hpp"
#include "vkPrograms.hpp"
#include "vkMemUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkTypeUtil.hpp"

#include "deUniquePtr.hpp"
#include "deStringUtil.hpp"

#include <string>
#include <vector>

using namespace vk;

namespace vkt
{
namespace sparse
{
namespace
{

class ImageBlockShapesCase : public TestCase
{
public:
					ImageBlockShapesCase	(tcu::TestContext&			testCtx,
											 const std::string&			name,
											 const std::string&			description,
											 const ImageType			imageType,
											 const tcu::UVec3&			imageSize,
											 const tcu::TextureFormat&	format,
											 deUint32					numSamples);

	void			initPrograms				(SourceCollections&			sourceCollections) const {DE_UNREF(sourceCollections);};
	TestInstance*	createInstance				(Context&					context) const;

private:
	const ImageType				m_imageType;
	const tcu::UVec3			m_imageSize;
	const tcu::TextureFormat	m_format;
	const deUint32				m_numSamples;
};

ImageBlockShapesCase::ImageBlockShapesCase (tcu::TestContext&			testCtx,
											const std::string&			name,
											const std::string&			description,
											const ImageType				imageType,
											const tcu::UVec3&			imageSize,
											const tcu::TextureFormat&	format,
											deUint32					numSamples)
	: TestCase				(testCtx, name, description)
	, m_imageType			(imageType)
	, m_imageSize			(imageSize)
	, m_format				(format)
	, m_numSamples			(numSamples)
{
}

class ImageBlockShapesInstance : public SparseResourcesBaseInstance
{
public:
					ImageBlockShapesInstance(Context&							context,
											 const ImageType					imageType,
											 const tcu::UVec3&					imageSize,
											 const tcu::TextureFormat&			format,
											 deUint32							numSamples);

	tcu::TestStatus	iterate						(void);

private:
	const ImageType				m_imageType;
	const tcu::UVec3			m_imageSize;
	const tcu::TextureFormat	m_format;
	const deUint32				m_numSamples;
};

ImageBlockShapesInstance::ImageBlockShapesInstance (Context&					context,
													const ImageType				imageType,
													const tcu::UVec3&			imageSize,
													const tcu::TextureFormat&	format,
													deUint32					numSamples)
	: SparseResourcesBaseInstance	(context)
	, m_imageType					(imageType)
	, m_imageSize					(imageSize)
	, m_format						(format)
	, m_numSamples					(numSamples)
{
}

tcu::TestStatus ImageBlockShapesInstance::iterate (void)
{
	const InstanceInterface&				instance = m_context.getInstanceInterface();
	const VkPhysicalDevice					physicalDevice = m_context.getPhysicalDevice();
	const VkPhysicalDeviceProperties		physicalDeviceProperties = getPhysicalDeviceProperties(instance, physicalDevice);
	VkImageCreateInfo						imageCreateInfo;
	VkSparseImageMemoryRequirements			aspectRequirements;
	VkExtent3D								imageGranularity;
	const VkPhysicalDeviceSparseProperties	sparseProperties = physicalDeviceProperties.sparseProperties;
	const deUint32							pixelSize = tcu::getPixelSize(m_format) * 8;
	VkExtent3D								expectedGranularity;

	// Check the image size does not exceed device limits
	if (!isImageSizeSupported(instance, physicalDevice, m_imageType, m_imageSize))
		TCU_THROW(NotSupportedError, "Image size not supported for device");

	// Check if device supports sparse operations for image type
	if (!checkSparseSupportForImageType(instance, physicalDevice, m_imageType))
		TCU_THROW(NotSupportedError, "Sparse residency for image type is not supported");

	imageCreateInfo.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext					= DE_NULL;
	imageCreateInfo.flags					= VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT | VK_IMAGE_CREATE_SPARSE_BINDING_BIT;
	imageCreateInfo.imageType				= mapImageType(m_imageType);
	imageCreateInfo.format					= mapTextureFormat(m_format);
	imageCreateInfo.extent					= makeExtent3D(getLayerSize(m_imageType, m_imageSize));
	imageCreateInfo.mipLevels				= 1u;
	imageCreateInfo.arrayLayers				= getNumLayers(m_imageType, m_imageSize);
	imageCreateInfo.samples					= static_cast<VkSampleCountFlagBits>(m_numSamples);
	imageCreateInfo.tiling					= VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage					= VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
											  VK_IMAGE_USAGE_STORAGE_BIT;
	imageCreateInfo.sharingMode				= VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount	= 0u;
	imageCreateInfo.pQueueFamilyIndices		= DE_NULL;

	if (m_imageType == IMAGE_TYPE_CUBE || m_imageType == IMAGE_TYPE_CUBE_ARRAY)
	{
		imageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	// Check the format supports given number of samples
	const VkImageFormatProperties formatProperties = getPhysicalDeviceImageFormatProperties(instance, physicalDevice, imageCreateInfo.format, imageCreateInfo.imageType, imageCreateInfo.tiling, imageCreateInfo.usage, imageCreateInfo.flags);

	if (!(formatProperties.sampleCounts & imageCreateInfo.samples))
		TCU_THROW(NotSupportedError, "The image format does not support the number of samples specified");

	// Check if device supports sparse operations for image format
	if (!checkSparseSupportForImageFormat(instance, physicalDevice, imageCreateInfo))
		TCU_THROW(NotSupportedError, "The image format does not support sparse operations");

	{
		QueueRequirementsVec queueRequirements;
		queueRequirements.push_back(QueueRequirements(VK_QUEUE_SPARSE_BINDING_BIT, 1u));

		createDeviceSupportingQueues(queueRequirements);
	}

	{
		const DeviceInterface&								deviceInterface				= getDeviceInterface();

		// Create sparse image
		const Unique<VkImage>								sparseImage					(createImage(deviceInterface, getDevice(), &imageCreateInfo));

		// Get sparse image sparse memory requirements
		const std::vector<VkSparseImageMemoryRequirements>	sparseMemoryRequirements	= getImageSparseMemoryRequirements(deviceInterface, getDevice(), *sparseImage);

		DE_ASSERT(sparseMemoryRequirements.size() != 0);

		const deUint32										colorAspectIndex			= getSparseAspectRequirementsIndex(sparseMemoryRequirements, VK_IMAGE_ASPECT_COLOR_BIT);

		if (colorAspectIndex == NO_MATCH_FOUND)
			TCU_THROW(NotSupportedError, "Not supported image aspect - the test supports currently only VK_IMAGE_ASPECT_COLOR_BIT");

		aspectRequirements	= sparseMemoryRequirements[colorAspectIndex];
		imageGranularity	= aspectRequirements.formatProperties.imageGranularity;
	}

	if (m_imageType == IMAGE_TYPE_3D)
	{
		if (!sparseProperties.residencyStandard3DBlockShape)
			return tcu::TestStatus::pass("Pass (residencyStandard3DBlockShape disabled)");

		switch (pixelSize)
		{
			case 8:
				expectedGranularity.width	= 64;
				expectedGranularity.height	= 32;
				expectedGranularity.depth	= 32;
				break;
			case 16:
				expectedGranularity.width	= 32;
				expectedGranularity.height	= 32;
				expectedGranularity.depth	= 32;
				break;
			case 32:
				expectedGranularity.width	= 32;
				expectedGranularity.height	= 32;
				expectedGranularity.depth	= 16;
				break;
			case 64:
				expectedGranularity.width	= 32;
				expectedGranularity.height	= 16;
				expectedGranularity.depth	= 16;
				break;
			default:
				DE_ASSERT(pixelSize == 128);
				expectedGranularity.width	= 16;
				expectedGranularity.height	= 16;
				expectedGranularity.depth	= 16;
				break;
		};
	}
	else if (m_numSamples == 2)
	{
		if (!sparseProperties.residencyStandard2DMultisampleBlockShape)
			return tcu::TestStatus::pass("Pass (residencyStandard2DMultisampleBlockShape disabled)");

		expectedGranularity.depth = 1;

		switch (pixelSize)
		{
			case 8:
				expectedGranularity.width	= 128;
				expectedGranularity.height	= 256;
				break;
			case 16:
				expectedGranularity.width	= 128;
				expectedGranularity.height	= 128;
				break;
			case 32:
				expectedGranularity.width	= 64;
				expectedGranularity.height	= 128;
				break;
			case 64:
				expectedGranularity.width	= 64;
				expectedGranularity.height	= 64;
				break;
			default:
				DE_ASSERT(pixelSize == 128);
				expectedGranularity.width	= 32;
				expectedGranularity.height	= 64;
				break;
		};
	}
	else if (m_numSamples == 4)
	{
		if (!sparseProperties.residencyStandard2DMultisampleBlockShape)
			return tcu::TestStatus::pass("Pass (residencyStandard2DMultisampleBlockShape disabled)");

		expectedGranularity.depth = 1;

		switch (pixelSize)
		{
			case 8:
				expectedGranularity.width	= 128;
				expectedGranularity.height	= 128;
				break;
			case 16:
				expectedGranularity.width	= 128;
				expectedGranularity.height	= 64;
				break;
			case 32:
				expectedGranularity.width	= 64;
				expectedGranularity.height	= 64;
				break;
			case 64:
				expectedGranularity.width	= 64;
				expectedGranularity.height	= 32;
				break;
			default:
				DE_ASSERT(pixelSize == 128);
				expectedGranularity.width	= 32;
				expectedGranularity.height	= 32;
				break;
		};
	}
	else if (m_numSamples == 8)
	{
		if (!sparseProperties.residencyStandard2DMultisampleBlockShape)
			return tcu::TestStatus::pass("Pass (residencyStandard2DMultisampleBlockShape disabled)");

		expectedGranularity.depth = 1;

		switch (pixelSize)
		{
			case 8:
				expectedGranularity.width	= 64;
				expectedGranularity.height	= 128;
				break;
			case 16:
				expectedGranularity.width	= 64;
				expectedGranularity.height	= 64;
				break;
			case 32:
				expectedGranularity.width	= 32;
				expectedGranularity.height	= 64;
				break;
			case 64:
				expectedGranularity.width	= 32;
				expectedGranularity.height	= 32;
				break;
			default:
				DE_ASSERT(pixelSize == 128);
				expectedGranularity.width	= 16;
				expectedGranularity.height	= 32;
				break;
		};
	}
	else if (m_numSamples == 16)
	{
		if (!sparseProperties.residencyStandard2DMultisampleBlockShape)
			return tcu::TestStatus::pass("Pass (residencyStandard2DMultisampleBlockShape disabled)");

		expectedGranularity.depth = 1;

		switch (pixelSize)
		{
			case 8:
				expectedGranularity.width	= 64;
				expectedGranularity.height	= 64;
				break;
			case 16:
				expectedGranularity.width	= 64;
				expectedGranularity.height	= 32;
				break;
			case 32:
				expectedGranularity.width	= 32;
				expectedGranularity.height	= 32;
				break;
			case 64:
				expectedGranularity.width	= 32;
				expectedGranularity.height	= 16;
				break;
			default:
				DE_ASSERT(pixelSize == 128);
				expectedGranularity.width	= 16;
				expectedGranularity.height	= 16;
				break;
		};
	}
	else
	{
		DE_ASSERT(m_numSamples == 1);

		if (!sparseProperties.residencyStandard2DBlockShape)
			return tcu::TestStatus::pass("Pass (residencyStandard2DBlockShape disabled)");

		expectedGranularity.depth = 1;

		switch (pixelSize)
		{
			case 8:
				expectedGranularity.width	= 256;
				expectedGranularity.height	= 256;
				break;
			case 16:
				expectedGranularity.width	= 256;
				expectedGranularity.height	= 128;
				break;
			case 32:
				expectedGranularity.width	= 128;
				expectedGranularity.height	= 128;
				break;
			case 64:
				expectedGranularity.width	= 128;
				expectedGranularity.height	= 64;
				break;
			default:
				DE_ASSERT(pixelSize == 128);
				expectedGranularity.width	= 64;
				expectedGranularity.height	= 64;
				break;
		};
	}

	if (imageGranularity.width == expectedGranularity.width
		&& imageGranularity.height == expectedGranularity.height
		&& imageGranularity.depth == expectedGranularity.depth)
	{
		return tcu::TestStatus::pass("Passed");
	}
	else
	{
		return tcu::TestStatus::fail("Non-standard block shape used");
	}
}

TestInstance* ImageBlockShapesCase::createInstance (Context& context) const
{
	return new ImageBlockShapesInstance(context, m_imageType, m_imageSize, m_format, m_numSamples);
}

} // anonymous ns

tcu::TestCaseGroup* createImageBlockShapesTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> testGroup(new tcu::TestCaseGroup(testCtx, "image_block_shapes", "Standard block shape"));

	struct ImageParameters
	{
		ImageType	imageType;
		tcu::UVec3	imageSize;
	};

	static const ImageParameters imageParametersArray[] =
	{
		{ IMAGE_TYPE_2D,		 tcu::UVec3(512u, 256u, 1u)		},
		{ IMAGE_TYPE_2D_ARRAY,	 tcu::UVec3(512u, 256u, 6u)		},
		{ IMAGE_TYPE_CUBE,		 tcu::UVec3(256u, 256u, 1u)		},
		{ IMAGE_TYPE_CUBE_ARRAY, tcu::UVec3(256u, 256u, 6u)		},
		{ IMAGE_TYPE_3D,		 tcu::UVec3(512u, 256u, 16u)	}
	};

	static const tcu::TextureFormat formats[] =
	{
		tcu::TextureFormat(tcu::TextureFormat::R,	 tcu::TextureFormat::SIGNED_INT32),
		tcu::TextureFormat(tcu::TextureFormat::R,	 tcu::TextureFormat::SIGNED_INT16),
		tcu::TextureFormat(tcu::TextureFormat::R,	 tcu::TextureFormat::SIGNED_INT8),
		tcu::TextureFormat(tcu::TextureFormat::RG,	 tcu::TextureFormat::SIGNED_INT32),
		tcu::TextureFormat(tcu::TextureFormat::RG,   tcu::TextureFormat::SIGNED_INT16),
		tcu::TextureFormat(tcu::TextureFormat::RG,   tcu::TextureFormat::SIGNED_INT8),
		tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNSIGNED_INT32),
		tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNSIGNED_INT16),
		tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNSIGNED_INT8)
	};

	static const deUint32 sampleCounts[] = { 1u, 2u, 4u, 8u, 16u };

	for (deInt32 imageTypeNdx = 0; imageTypeNdx < DE_LENGTH_OF_ARRAY(imageParametersArray); ++imageTypeNdx)
	{
		const ImageType					imageType = imageParametersArray[imageTypeNdx].imageType;
		de::MovePtr<tcu::TestCaseGroup> imageTypeGroup(new tcu::TestCaseGroup(testCtx, getImageTypeName(imageType).c_str(), ""));

		for (deInt32 formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); ++formatNdx)
		{
			const tcu::TextureFormat&		format = formats[formatNdx];
			de::MovePtr<tcu::TestCaseGroup> formatGroup(new tcu::TestCaseGroup(testCtx, getShaderImageFormatQualifier(format).c_str(), ""));

			for (deInt32 sampleCountNdx = 0; sampleCountNdx < DE_LENGTH_OF_ARRAY(sampleCounts); ++sampleCountNdx)
			{
				const tcu::UVec3	imageSize	= imageParametersArray[imageTypeNdx].imageSize;
				const deUint32		sampleCount	= sampleCounts[sampleCountNdx];
				const std::string	name		= std::string("samples_") + de::toString(sampleCount);

				formatGroup->addChild(new ImageBlockShapesCase(testCtx, name.c_str(), "", imageType, imageSize, format, sampleCount));
			}
			imageTypeGroup->addChild(formatGroup.release());
		}
		testGroup->addChild(imageTypeGroup.release());
	}

	return testGroup.release();
}

} // sparse
} // vkt
