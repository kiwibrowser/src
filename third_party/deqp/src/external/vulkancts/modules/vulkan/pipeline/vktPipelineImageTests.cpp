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
 * \brief Image Tests
 *//*--------------------------------------------------------------------*/

#include "vktPipelineImageTests.hpp"
#include "vktPipelineImageSamplingInstance.hpp"
#include "vktPipelineImageUtil.hpp"
#include "vktPipelineVertexUtil.hpp"
#include "vktTestCase.hpp"
#include "vkImageUtil.hpp"
#include "vkPrograms.hpp"
#include "tcuTextureUtil.hpp"
#include "deStringUtil.hpp"

#include <sstream>
#include <vector>

namespace vkt
{
namespace pipeline
{

using namespace vk;
using de::MovePtr;

namespace
{

class ImageTest : public vkt::TestCase
{
public:
							ImageTest				(tcu::TestContext&				testContext,
													 const char*					name,
													 const char*					description,

													 AllocationKind		allocationKind,
													 VkDescriptorType				samplingType,
													 VkImageViewType				imageViewType,
													 VkFormat						imageFormat,
													 const tcu::IVec3&				imageSize,
													 int							imageCount,
													 int							arraySize);

	virtual void			initPrograms			(SourceCollections& sourceCollections) const;
	virtual TestInstance*	createInstance			(Context& context) const;
	static std::string		getGlslSamplerType		(const tcu::TextureFormat& format, VkImageViewType type);
	static std::string		getGlslTextureType		(const tcu::TextureFormat& format, VkImageViewType type);
	static std::string		getGlslSamplerDecl		(int imageCount);
	static std::string		getGlslTextureDecl		(int imageCount);
	static std::string		getGlslFragColorDecl	(int imageCount);
	static std::string		getGlslSampler			(const tcu::TextureFormat& format,
													 VkImageViewType type,
													 VkDescriptorType samplingType,
													 int imageCount);

private:
	AllocationKind		m_allocationKind;
	VkDescriptorType	m_samplingType;
	VkImageViewType		m_imageViewType;
	VkFormat			m_imageFormat;
	tcu::IVec3			m_imageSize;
	int					m_imageCount;
	int					m_arraySize;
};

ImageTest::ImageTest (tcu::TestContext&	testContext,
					  const char*		name,
					  const char*		description,
					  AllocationKind	allocationKind,
					  VkDescriptorType	samplingType,
					  VkImageViewType	imageViewType,
					  VkFormat			imageFormat,
					  const tcu::IVec3&	imageSize,
					  int				imageCount,
					  int				arraySize)

	: vkt::TestCase		(testContext, name, description)
	, m_allocationKind	(allocationKind)
	, m_samplingType	(samplingType)
	, m_imageViewType	(imageViewType)
	, m_imageFormat		(imageFormat)
	, m_imageSize		(imageSize)
	, m_imageCount		(imageCount)
	, m_arraySize		(arraySize)
{
}

void ImageTest::initPrograms (SourceCollections& sourceCollections) const
{
	std::ostringstream				vertexSrc;
	std::ostringstream				fragmentSrc;
	const char*						texCoordSwizzle	= DE_NULL;
	const tcu::TextureFormat		format			= (isCompressedFormat(m_imageFormat)) ? tcu::getUncompressedFormat(mapVkCompressedFormat(m_imageFormat))
																						  : mapVkFormat(m_imageFormat);

	tcu::Vec4						lookupScale;
	tcu::Vec4						lookupBias;

	getLookupScaleBias(m_imageFormat, lookupScale, lookupBias);

	switch (m_imageViewType)
	{
		case VK_IMAGE_VIEW_TYPE_1D:
			texCoordSwizzle = "x";
			break;
		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
		case VK_IMAGE_VIEW_TYPE_2D:
			texCoordSwizzle = "xy";
			break;
		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
		case VK_IMAGE_VIEW_TYPE_3D:
		case VK_IMAGE_VIEW_TYPE_CUBE:
			texCoordSwizzle = "xyz";
			break;
		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			texCoordSwizzle = "xyzw";
			break;
		default:
			DE_ASSERT(false);
			break;
	}

	vertexSrc << "#version 440\n"
			  << "layout(location = 0) in vec4 position;\n"
			  << "layout(location = 1) in vec4 texCoords;\n"
			  << "layout(location = 0) out highp vec4 vtxTexCoords;\n"
			  << "out gl_PerVertex {\n"
			  << "	vec4 gl_Position;\n"
			  << "};\n"
			  << "void main (void)\n"
			  << "{\n"
			  << "	gl_Position = position;\n"
			  << "	vtxTexCoords = texCoords;\n"
			  << "}\n";

	fragmentSrc << "#version 440\n";
	switch (m_samplingType)
	{
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			fragmentSrc
				<< "layout(set = 0, binding = 0) uniform highp sampler texSampler;\n"
				<< "layout(set = 0, binding = 1) uniform highp " << getGlslTextureType(format, m_imageViewType) << " " << getGlslTextureDecl(m_imageCount) << ";\n";
			break;
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		default:
			fragmentSrc
				<< "layout(set = 0, binding = 0) uniform highp " << getGlslSamplerType(format, m_imageViewType) << " " << getGlslSamplerDecl(m_imageCount) << ";\n";
	}
	fragmentSrc << "layout(location = 0) in highp vec4 vtxTexCoords;\n"
				<< "layout(location = 0) out highp vec4 " << getGlslFragColorDecl(m_imageCount) << ";\n"
				<< "void main (void)\n"
				<< "{\n";
	if (m_imageCount > 1)
		fragmentSrc
				<< "	for (uint i = 0; i < " << m_imageCount << "; ++i)\n"
				<< "		fragColors[i] = (texture(" << getGlslSampler(format, m_imageViewType, m_samplingType, m_imageCount) << ", vtxTexCoords." << texCoordSwizzle << std::scientific << ") * vec4" << lookupScale << ") + vec4" << lookupBias << "; \n";
	else
		fragmentSrc
				<< "	fragColor = (texture(" << getGlslSampler(format, m_imageViewType, m_samplingType, m_imageCount) << ", vtxTexCoords." << texCoordSwizzle << std::scientific << ") * vec4" << lookupScale << ") + vec4" << lookupBias << "; \n";
	fragmentSrc << "}\n";

	sourceCollections.glslSources.add("tex_vert") << glu::VertexSource(vertexSrc.str());
	sourceCollections.glslSources.add("tex_frag") << glu::FragmentSource(fragmentSrc.str());
}

TestInstance* ImageTest::createInstance (Context& context) const
{
	tcu::UVec2 renderSize;
	const VkPhysicalDeviceFeatures&	features = context.getDeviceFeatures();

	// Using an loop to index into an array of images requires shaderSampledImageArrayDynamicIndexing
	if (m_imageCount > 1 && features.shaderSampledImageArrayDynamicIndexing == VK_FALSE)
	{
		TCU_THROW(NotSupportedError, "shaderSampledImageArrayDynamicIndexing feature is not supported");
	}

	if (m_imageViewType == VK_IMAGE_VIEW_TYPE_1D || m_imageViewType == VK_IMAGE_VIEW_TYPE_2D)
	{
		renderSize = tcu::UVec2((deUint32)m_imageSize.x(), (deUint32)m_imageSize.y());
	}
	else
	{
		// Draw a 3x2 grid of texture layers
		renderSize = tcu::UVec2((deUint32)m_imageSize.x() * 3, (deUint32)m_imageSize.y() * 2);
	}

	const std::vector<Vertex4Tex4>	vertices			= createTestQuadMosaic(m_imageViewType);
	const VkComponentMapping		componentMapping	= { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	const VkImageSubresourceRange	subresourceRange	=
	{
		VK_IMAGE_ASPECT_COLOR_BIT,
		0u,
		(deUint32)deLog2Floor32(deMax32(m_imageSize.x(), deMax32(m_imageSize.y(), m_imageSize.z()))) + 1,
		0u,
		(deUint32)m_arraySize,
	};

	const VkSamplerCreateInfo samplerParams =
	{
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,									// VkStructureType			sType;
		DE_NULL,																// const void*				pNext;
		0u,																		// VkSamplerCreateFlags		flags;
		VK_FILTER_NEAREST,														// VkFilter					magFilter;
		VK_FILTER_NEAREST,														// VkFilter					minFilter;
		VK_SAMPLER_MIPMAP_MODE_NEAREST,											// VkSamplerMipmapMode		mipmapMode;
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,									// VkSamplerAddressMode		addressModeU;
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,									// VkSamplerAddressMode		addressModeV;
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,									// VkSamplerAddressMode		addressModeW;
		0.0f,																	// float					mipLodBias;
		VK_FALSE,																// VkBool32					anisotropyEnable;
		1.0f,																	// float					maxAnisotropy;
		false,																	// VkBool32					compareEnable;
		VK_COMPARE_OP_NEVER,													// VkCompareOp				compareOp;
		0.0f,																	// float					minLod;
		(float)(subresourceRange.levelCount - 1),								// float					maxLod;
		getFormatBorderColor(BORDER_COLOR_TRANSPARENT_BLACK, m_imageFormat),	// VkBorderColor			borderColor;
		false																	// VkBool32					unnormalizedCoordinates;
	};

	return new ImageSamplingInstance(context, renderSize, m_imageViewType, m_imageFormat, m_imageSize, m_arraySize, componentMapping, subresourceRange, samplerParams, 0.0f, vertices, m_samplingType, m_imageCount, m_allocationKind);
}

std::string ImageTest::getGlslSamplerType (const tcu::TextureFormat& format, VkImageViewType type)
{
	std::ostringstream samplerType;

	if (tcu::getTextureChannelClass(format.type) == tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER)
		samplerType << "u";
	else if (tcu::getTextureChannelClass(format.type) == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER)
		samplerType << "i";

	switch (type)
	{
		case VK_IMAGE_VIEW_TYPE_1D:
			samplerType << "sampler1D";
			break;

		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
			samplerType << "sampler1DArray";
			break;

		case VK_IMAGE_VIEW_TYPE_2D:
			samplerType << "sampler2D";
			break;

		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
			samplerType << "sampler2DArray";
			break;

		case VK_IMAGE_VIEW_TYPE_3D:
			samplerType << "sampler3D";
			break;

		case VK_IMAGE_VIEW_TYPE_CUBE:
			samplerType << "samplerCube";
			break;

		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			samplerType << "samplerCubeArray";
			break;

		default:
			DE_FATAL("Unknown image view type");
			break;
	}

	return samplerType.str();
}

std::string ImageTest::getGlslTextureType (const tcu::TextureFormat& format, VkImageViewType type)
{
	std::ostringstream textureType;

	if (tcu::getTextureChannelClass(format.type) == tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER)
		textureType << "u";
	else if (tcu::getTextureChannelClass(format.type) == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER)
		textureType << "i";

	switch (type)
	{
		case VK_IMAGE_VIEW_TYPE_1D:
			textureType << "texture1D";
			break;

		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
			textureType << "texture1DArray";
			break;

		case VK_IMAGE_VIEW_TYPE_2D:
			textureType << "texture2D";
			break;

		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
			textureType << "texture2DArray";
			break;

		case VK_IMAGE_VIEW_TYPE_3D:
			textureType << "texture3D";
			break;

		case VK_IMAGE_VIEW_TYPE_CUBE:
			textureType << "textureCube";
			break;

		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			textureType << "textureCubeArray";
			break;

		default:
			DE_FATAL("Unknown image view type");
	}

	return textureType.str();
}

std::string ImageTest::getGlslSamplerDecl (int imageCount)
{
	std::ostringstream samplerArray;
	samplerArray << "texSamplers[" << imageCount << "]";

	return imageCount > 1 ? samplerArray.str() : "texSampler";
}

std::string ImageTest::getGlslTextureDecl (int imageCount)
{
	std::ostringstream textureArray;
	textureArray << "texImages[" << imageCount << "]";

	return imageCount > 1 ? textureArray.str() : "texImage";
}

std::string ImageTest::getGlslFragColorDecl (int imageCount)
{
	std::ostringstream samplerArray;
	samplerArray << "fragColors[" << imageCount << "]";

	return imageCount > 1 ? samplerArray.str() : "fragColor";
}

std::string ImageTest::getGlslSampler (const tcu::TextureFormat& format, VkImageViewType type, VkDescriptorType samplingType, int imageCount)
{
	std::string texSampler	= imageCount > 1 ? "texSamplers[i]" : "texSampler";
	std::string texImage	= imageCount > 1 ? "texImages[i]" : "texImage";

	switch (samplingType)
	{
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			return getGlslSamplerType(format, type) + "(" + texImage + ", texSampler)";
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		default:
			return texSampler;
	}
}

std::string getFormatCaseName (const VkFormat format)
{
	const std::string	fullName	= getFormatName(format);

	DE_ASSERT(de::beginsWith(fullName, "VK_FORMAT_"));

	return de::toLower(fullName.substr(10));
}

std::string getSizeName (VkImageViewType viewType, const tcu::IVec3& size, int arraySize)
{
	std::ostringstream	caseName;

	switch (viewType)
	{
		case VK_IMAGE_VIEW_TYPE_1D:
		case VK_IMAGE_VIEW_TYPE_2D:
		case VK_IMAGE_VIEW_TYPE_CUBE:
			caseName << size.x() << "x" << size.y();
			break;

		case VK_IMAGE_VIEW_TYPE_3D:
			caseName << size.x() << "x" << size.y() << "x" << size.z();
			break;

		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			caseName << size.x() << "x" << size.y() << "_array_of_" << arraySize;
			break;

		default:
			DE_ASSERT(false);
			break;
	}

	return caseName.str();
}

de::MovePtr<tcu::TestCaseGroup> createImageSizeTests (tcu::TestContext& testCtx, AllocationKind allocationKind, VkDescriptorType samplingType, VkImageViewType imageViewType, VkFormat imageFormat, int imageCount)
{
	using tcu::IVec3;

	std::vector<IVec3>					imageSizes;
	std::vector<int>					arraySizes;
	de::MovePtr<tcu::TestCaseGroup>		imageSizeTests	(new tcu::TestCaseGroup(testCtx, "size", ""));

	// Select image imageSizes
	switch (imageViewType)
	{
		case VK_IMAGE_VIEW_TYPE_1D:
		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
			// POT
			if (imageCount == 1)
			{
				imageSizes.push_back(IVec3(1, 1, 1));
				imageSizes.push_back(IVec3(2, 1, 1));
				imageSizes.push_back(IVec3(32, 1, 1));
				imageSizes.push_back(IVec3(128, 1, 1));
			}
			imageSizes.push_back(IVec3(512, 1, 1));

			// NPOT
			if (imageCount == 1)
			{
				imageSizes.push_back(IVec3(3, 1, 1));
				imageSizes.push_back(IVec3(13, 1, 1));
				imageSizes.push_back(IVec3(127, 1, 1));
			}
			imageSizes.push_back(IVec3(443, 1, 1));
			break;

		case VK_IMAGE_VIEW_TYPE_2D:
		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
			if (imageCount == 1)
			{
				// POT
				imageSizes.push_back(IVec3(1, 1, 1));
				imageSizes.push_back(IVec3(2, 2, 1));
				imageSizes.push_back(IVec3(32, 32, 1));

				// NPOT
				imageSizes.push_back(IVec3(3, 3, 1));
				imageSizes.push_back(IVec3(13, 13, 1));
			}

			// POT rectangular
			if (imageCount == 1)
				imageSizes.push_back(IVec3(8, 16, 1));
			imageSizes.push_back(IVec3(32, 16, 1));

			// NPOT rectangular
			imageSizes.push_back(IVec3(13, 23, 1));
			if (imageCount == 1)
				imageSizes.push_back(IVec3(23, 8, 1));
			break;

		case VK_IMAGE_VIEW_TYPE_3D:
			// POT cube
			if (imageCount == 1)
			{
				imageSizes.push_back(IVec3(1, 1, 1));
				imageSizes.push_back(IVec3(2, 2, 2));
			}
			imageSizes.push_back(IVec3(16, 16, 16));

			// NPOT cube
			if (imageCount == 1)
			{
				imageSizes.push_back(IVec3(3, 3, 3));
				imageSizes.push_back(IVec3(5, 5, 5));
			}
			imageSizes.push_back(IVec3(11, 11, 11));

			// POT non-cube
			if (imageCount == 1)
				imageSizes.push_back(IVec3(32, 16, 8));
			imageSizes.push_back(IVec3(8, 16, 32));

			// NPOT non-cube
			imageSizes.push_back(IVec3(17, 11, 5));
			if (imageCount == 1)
				imageSizes.push_back(IVec3(5, 11, 17));
			break;

		case VK_IMAGE_VIEW_TYPE_CUBE:
		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			// POT
			imageSizes.push_back(IVec3(32, 32, 1));

			// NPOT
			imageSizes.push_back(IVec3(13, 13, 1));
			break;

		default:
			DE_ASSERT(false);
			break;
	}

	// Select array sizes
	switch (imageViewType)
	{
		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
			if (imageCount == 1)
				arraySizes.push_back(3);
			arraySizes.push_back(6);
			break;

		case VK_IMAGE_VIEW_TYPE_CUBE:
			arraySizes.push_back(6);
			break;

		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			if (imageCount == 1)
				arraySizes.push_back(6);
			arraySizes.push_back(6 * 6);
			break;

		default:
			arraySizes.push_back(1);
			break;
	}

	for (size_t sizeNdx = 0; sizeNdx < imageSizes.size(); sizeNdx++)
	{
		for (size_t arraySizeNdx = 0; arraySizeNdx < arraySizes.size(); arraySizeNdx++)
		{
			imageSizeTests->addChild(new ImageTest(testCtx,
												   getSizeName(imageViewType, imageSizes[sizeNdx], arraySizes[arraySizeNdx]).c_str(),
												   "",
												   allocationKind,
												   samplingType,
												   imageViewType,
												   imageFormat,
												   imageSizes[sizeNdx],
												   imageCount,
												   arraySizes[arraySizeNdx]));
		}
	}

	return imageSizeTests;
}

void createImageCountTests (tcu::TestCaseGroup* parentGroup, tcu::TestContext& testCtx, AllocationKind allocationKind, VkDescriptorType samplingType, VkImageViewType imageViewType, VkFormat imageFormat)
{
	const int		coreImageCounts[]					= { 1, 4, 8 };
	const int		dedicatedAllocationImageCounts[]	= { 1 };
	const int*		imageCounts							= (allocationKind == ALLOCATION_KIND_DEDICATED)
														  ? dedicatedAllocationImageCounts
														  : coreImageCounts;
	const size_t	imageCountsLength					= (allocationKind == ALLOCATION_KIND_DEDICATED)
														  ? DE_LENGTH_OF_ARRAY(dedicatedAllocationImageCounts)
														  : DE_LENGTH_OF_ARRAY(coreImageCounts);

	for (size_t countNdx = 0; countNdx < imageCountsLength; countNdx++)
	{
		std::ostringstream	caseName;
		caseName << "count_" << imageCounts[countNdx];
		de::MovePtr<tcu::TestCaseGroup>	countGroup(new tcu::TestCaseGroup(testCtx, caseName.str().c_str(), ""));
		de::MovePtr<tcu::TestCaseGroup> sizeTests = createImageSizeTests(testCtx, allocationKind, samplingType, imageViewType, imageFormat, imageCounts[countNdx]);

		countGroup->addChild(sizeTests.release());
		parentGroup->addChild(countGroup.release());
	}
}

de::MovePtr<tcu::TestCaseGroup> createImageFormatTests (tcu::TestContext& testCtx, AllocationKind allocationKind, VkDescriptorType samplingType, VkImageViewType imageViewType)
{
	// All supported dEQP formats that are not intended for depth or stencil.
	const VkFormat coreFormats[]					=
	{
		VK_FORMAT_R4G4_UNORM_PACK8,
		VK_FORMAT_R4G4B4A4_UNORM_PACK16,
		VK_FORMAT_R5G6B5_UNORM_PACK16,
		VK_FORMAT_R5G5B5A1_UNORM_PACK16,
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_R8_SNORM,
		VK_FORMAT_R8_USCALED,
		VK_FORMAT_R8_SSCALED,
		VK_FORMAT_R8_UINT,
		VK_FORMAT_R8_SINT,
		VK_FORMAT_R8_SRGB,
		VK_FORMAT_R8G8_UNORM,
		VK_FORMAT_R8G8_SNORM,
		VK_FORMAT_R8G8_USCALED,
		VK_FORMAT_R8G8_SSCALED,
		VK_FORMAT_R8G8_UINT,
		VK_FORMAT_R8G8_SINT,
		VK_FORMAT_R8G8_SRGB,
		VK_FORMAT_R8G8B8_UNORM,
		VK_FORMAT_R8G8B8_SNORM,
		VK_FORMAT_R8G8B8_USCALED,
		VK_FORMAT_R8G8B8_SSCALED,
		VK_FORMAT_R8G8B8_UINT,
		VK_FORMAT_R8G8B8_SINT,
		VK_FORMAT_R8G8B8_SRGB,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R8G8B8A8_SNORM,
		VK_FORMAT_R8G8B8A8_USCALED,
		VK_FORMAT_R8G8B8A8_SSCALED,
		VK_FORMAT_R8G8B8A8_UINT,
		VK_FORMAT_R8G8B8A8_SINT,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_A2R10G10B10_UNORM_PACK32,
		VK_FORMAT_A2R10G10B10_UINT_PACK32,
		VK_FORMAT_A2R10G10B10_USCALED_PACK32,
		VK_FORMAT_R16_UNORM,
		VK_FORMAT_R16_SNORM,
		VK_FORMAT_R16_USCALED,
		VK_FORMAT_R16_SSCALED,
		VK_FORMAT_R16_UINT,
		VK_FORMAT_R16_SINT,
		VK_FORMAT_R16_SFLOAT,
		VK_FORMAT_R16G16_UNORM,
		VK_FORMAT_R16G16_SNORM,
		VK_FORMAT_R16G16_USCALED,
		VK_FORMAT_R16G16_SSCALED,
		VK_FORMAT_R16G16_UINT,
		VK_FORMAT_R16G16_SINT,
		VK_FORMAT_R16G16_SFLOAT,
		VK_FORMAT_R16G16B16_UNORM,
		VK_FORMAT_R16G16B16_SNORM,
		VK_FORMAT_R16G16B16_USCALED,
		VK_FORMAT_R16G16B16_SSCALED,
		VK_FORMAT_R16G16B16_UINT,
		VK_FORMAT_R16G16B16_SINT,
		VK_FORMAT_R16G16B16_SFLOAT,
		VK_FORMAT_R16G16B16A16_UNORM,
		VK_FORMAT_R16G16B16A16_SNORM,
		VK_FORMAT_R16G16B16A16_USCALED,
		VK_FORMAT_R16G16B16A16_SSCALED,
		VK_FORMAT_R16G16B16A16_UINT,
		VK_FORMAT_R16G16B16A16_SINT,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R32_SINT,
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32G32_UINT,
		VK_FORMAT_R32G32_SINT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32B32_UINT,
		VK_FORMAT_R32G32B32_SINT,
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R32G32B32A32_UINT,
		VK_FORMAT_R32G32B32A32_SINT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_FORMAT_B10G11R11_UFLOAT_PACK32,
		VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
		VK_FORMAT_B4G4R4A4_UNORM_PACK16,
		VK_FORMAT_B5G5R5A1_UNORM_PACK16,

		// Compressed formats
		VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
		VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
		VK_FORMAT_EAC_R11_UNORM_BLOCK,
		VK_FORMAT_EAC_R11_SNORM_BLOCK,
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
	// Formats to test with dedicated allocation
	const VkFormat	dedicatedAllocationFormats[]	=
	{
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R16_SFLOAT,
	};
	const VkFormat*	formats							= (allocationKind == ALLOCATION_KIND_DEDICATED)
													  ? dedicatedAllocationFormats
													  : coreFormats;
	const size_t	formatsLength					= (allocationKind == ALLOCATION_KIND_DEDICATED)
													  ? DE_LENGTH_OF_ARRAY(dedicatedAllocationFormats)
													  : DE_LENGTH_OF_ARRAY(coreFormats);

	de::MovePtr<tcu::TestCaseGroup>	imageFormatTests(new tcu::TestCaseGroup(testCtx, "format", "Tests samplable formats"));

	for (size_t formatNdx = 0; formatNdx < formatsLength; formatNdx++)
	{
		const VkFormat	format = formats[formatNdx];

		if (isCompressedFormat(format))
		{
			// Do not use compressed formats with 1D and 1D array textures.
			if (imageViewType == VK_IMAGE_VIEW_TYPE_1D || imageViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY)
				break;
		}

		de::MovePtr<tcu::TestCaseGroup>	formatGroup(new tcu::TestCaseGroup(testCtx,
			getFormatCaseName(format).c_str(),
			(std::string("Samples a texture of format ") + getFormatName(format)).c_str()));
		createImageCountTests(formatGroup.get(), testCtx, allocationKind, samplingType, imageViewType, format);

		imageFormatTests->addChild(formatGroup.release());
	}

	return imageFormatTests;
}

de::MovePtr<tcu::TestCaseGroup> createImageViewTypeTests (tcu::TestContext& testCtx, AllocationKind allocationKind, VkDescriptorType samplingType)
{
	const struct
	{
		VkImageViewType		type;
		const char*			name;
	}
	imageViewTypes[] =
	{
		{ VK_IMAGE_VIEW_TYPE_1D,			"1d" },
		{ VK_IMAGE_VIEW_TYPE_1D_ARRAY,		"1d_array" },
		{ VK_IMAGE_VIEW_TYPE_2D,			"2d" },
		{ VK_IMAGE_VIEW_TYPE_2D_ARRAY,		"2d_array" },
		{ VK_IMAGE_VIEW_TYPE_3D,			"3d" },
		{ VK_IMAGE_VIEW_TYPE_CUBE,			"cube" },
		{ VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,	"cube_array" }
	};

	de::MovePtr<tcu::TestCaseGroup> imageViewTypeTests(new tcu::TestCaseGroup(testCtx, "view_type", ""));

	for (int viewTypeNdx = 0; viewTypeNdx < DE_LENGTH_OF_ARRAY(imageViewTypes); viewTypeNdx++)
	{
		const VkImageViewType			viewType = imageViewTypes[viewTypeNdx].type;
		de::MovePtr<tcu::TestCaseGroup>	viewTypeGroup(new tcu::TestCaseGroup(testCtx, imageViewTypes[viewTypeNdx].name, (std::string("Uses a ") + imageViewTypes[viewTypeNdx].name + " view").c_str()));
		de::MovePtr<tcu::TestCaseGroup>	formatTests = createImageFormatTests(testCtx, allocationKind, samplingType, viewType);

		viewTypeGroup->addChild(formatTests.release());
		imageViewTypeTests->addChild(viewTypeGroup.release());
	}

	return imageViewTypeTests;
}

de::MovePtr<tcu::TestCaseGroup> createImageSamplingTypeTests (tcu::TestContext& testCtx, AllocationKind allocationKind)
{
	VkDescriptorType samplingTypes[] =
	{
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
	};

	de::MovePtr<tcu::TestCaseGroup> imageSamplingTypeTests(new tcu::TestCaseGroup(testCtx, "sampling_type", ""));

	for (int smpTypeNdx = 0; smpTypeNdx < DE_LENGTH_OF_ARRAY(samplingTypes); smpTypeNdx++)
	{
		const char* smpTypeName = samplingTypes[smpTypeNdx] == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ? "combined" : "separate";
		de::MovePtr<tcu::TestCaseGroup>	samplingTypeGroup(new tcu::TestCaseGroup(testCtx, smpTypeName, (std::string("Uses a ") + smpTypeName + " sampler").c_str()));
		de::MovePtr<tcu::TestCaseGroup>	viewTypeTests = createImageViewTypeTests(testCtx, allocationKind, samplingTypes[smpTypeNdx]);

		samplingTypeGroup->addChild(viewTypeTests.release());
		imageSamplingTypeTests->addChild(samplingTypeGroup.release());
	}

	return imageSamplingTypeTests;
}

de::MovePtr<tcu::TestCaseGroup> createSuballocationTests(tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	suballocationTestsGroup(new tcu::TestCaseGroup(testCtx, "suballocation", "Suballocation Image Tests"));
	de::MovePtr<tcu::TestCaseGroup>	samplingTypeTests = createImageSamplingTypeTests(testCtx, ALLOCATION_KIND_SUBALLOCATED);

	suballocationTestsGroup->addChild(samplingTypeTests.release());

	return suballocationTestsGroup;
}

de::MovePtr<tcu::TestCaseGroup> createDedicatedAllocationTests(tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	dedicatedAllocationTestsGroup(new tcu::TestCaseGroup(testCtx, "dedicated_allocation", "Image Tests For Dedicated Allocation"));
	de::MovePtr<tcu::TestCaseGroup>	samplingTypeTests = createImageSamplingTypeTests(testCtx, ALLOCATION_KIND_DEDICATED);

	dedicatedAllocationTestsGroup->addChild(samplingTypeTests.release());

	return dedicatedAllocationTestsGroup;
}
} // anonymous

tcu::TestCaseGroup* createImageTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> imageTests(new tcu::TestCaseGroup(testCtx, "image", "Image tests"));
	de::MovePtr<tcu::TestCaseGroup> imageSuballocationTests = createSuballocationTests(testCtx);
	de::MovePtr<tcu::TestCaseGroup> imageDedicatedAllocationTests = createDedicatedAllocationTests(testCtx);

	imageTests->addChild(imageSuballocationTests.release());
	imageTests->addChild(imageDedicatedAllocationTests.release());

	return imageTests.release();
}

} // pipeline
} // vkt
