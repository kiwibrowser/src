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
 * \file
 * \brief Tests for mutable images
 *//*--------------------------------------------------------------------*/

#include "vktImageMutableTests.hpp"
#include "vktImageLoadStoreUtil.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktImageTexture.hpp"

#include "vkBuilderUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkImageUtil.hpp"

#include "deUniquePtr.hpp"
#include "deSharedPtr.hpp"

#include "tcuImageCompare.hpp"
#include "tcuTestLog.hpp"
#include "tcuTextureUtil.hpp"

#include <string>
#include <vector>

using namespace vk;
using namespace tcu;

using de::UniquePtr;
using de::MovePtr;
using de::SharedPtr;
using std::vector;

namespace vkt
{
namespace image
{

typedef SharedPtr<Unique<VkPipeline> >	SharedPtrVkPipeline;
typedef SharedPtr<Unique<VkImageView> >	SharedPtrVkImageView;

template<typename T>
inline SharedPtr<Unique<T> > makeSharedPtr (Move<T> move)
{
	return SharedPtr<Unique<T> >(new Unique<T>(move));
}

enum Upload {
	UPLOAD_CLEAR = 0,
	UPLOAD_COPY,
	UPLOAD_STORE,
	UPLOAD_DRAW,
	UPLOAD_LAST
};

enum Download {
	DOWNLOAD_COPY = 0,
	DOWNLOAD_LOAD,
	DOWNLOAD_TEXTURE,
	DOWNLOAD_LAST
};

std::string getUploadString (const int upload)
{
	const char* strs[] = { "clear", "copy", "store", "draw" };
	return strs[upload];
}

std::string getDownloadString (const int download)
{
	const char* strs[] = { "copy", "load", "texture" };
	return strs[download];
}

struct CaseDef
{
	ImageType		imageType;
	IVec3			size;
	deUint32		numLayers;
	VkFormat		imageFormat;
	VkFormat		viewFormat;
	enum Upload		upload;
	enum Download	download;
	bool			isFormatListTest;
};

static const deUint32 COLOR_TABLE_SIZE = 4;

// Reference color values for float color rendering. Values have been chosen
// so that when the bit patterns are reinterpreted as a 16-bit float, we do not
// run into NaN / inf / denorm values.
static const Vec4	COLOR_TABLE_FLOAT[COLOR_TABLE_SIZE]	=
{
	Vec4(0.00f, 0.40f, 0.80f, 0.10f),
	Vec4(0.50f, 0.10f, 0.90f, 0.20f),
	Vec4(0.20f, 0.60f, 1.00f, 0.30f),
	Vec4(0.30f, 0.70f, 0.00f, 0.40f),
};

// Reference color values for integer color rendering. We avoid negative
// values (even for SINT formats) to avoid the situation where sign extension
// leads to NaN / inf values when they are reinterpreted with a float
// format.
static const IVec4	COLOR_TABLE_INT[COLOR_TABLE_SIZE]	=
{
	IVec4(112,  60,	101,  41),
	IVec4( 60, 101,	 41, 112),
	IVec4( 41, 112,	 60, 101),
	IVec4(101,  41,	112,  60),
};

// Reference clear colors created from the color table values
static const VkClearValue REFERENCE_CLEAR_COLOR_FLOAT[COLOR_TABLE_SIZE]	=
{
	makeClearValueColorF32(COLOR_TABLE_FLOAT[0].x(), COLOR_TABLE_FLOAT[0].y(), COLOR_TABLE_FLOAT[0].z(), COLOR_TABLE_FLOAT[0].w()),
	makeClearValueColorF32(COLOR_TABLE_FLOAT[1].x(), COLOR_TABLE_FLOAT[1].y(), COLOR_TABLE_FLOAT[1].z(), COLOR_TABLE_FLOAT[1].w()),
	makeClearValueColorF32(COLOR_TABLE_FLOAT[2].x(), COLOR_TABLE_FLOAT[2].y(), COLOR_TABLE_FLOAT[2].z(), COLOR_TABLE_FLOAT[2].w()),
	makeClearValueColorF32(COLOR_TABLE_FLOAT[3].x(), COLOR_TABLE_FLOAT[3].y(), COLOR_TABLE_FLOAT[3].z(), COLOR_TABLE_FLOAT[3].w()),
};

static const VkClearValue REFERENCE_CLEAR_COLOR_INT[COLOR_TABLE_SIZE]	=
{
	makeClearValueColorI32(COLOR_TABLE_INT[0].x(), COLOR_TABLE_INT[0].y(), COLOR_TABLE_INT[0].z(), COLOR_TABLE_INT[0].w()),
	makeClearValueColorI32(COLOR_TABLE_INT[1].x(), COLOR_TABLE_INT[1].y(), COLOR_TABLE_INT[1].z(), COLOR_TABLE_INT[1].w()),
	makeClearValueColorI32(COLOR_TABLE_INT[2].x(), COLOR_TABLE_INT[2].y(), COLOR_TABLE_INT[2].z(), COLOR_TABLE_INT[2].w()),
	makeClearValueColorI32(COLOR_TABLE_INT[3].x(), COLOR_TABLE_INT[3].y(), COLOR_TABLE_INT[3].z(), COLOR_TABLE_INT[3].w()),
};

static const Texture s_textures[] =
{
	Texture(IMAGE_TYPE_2D,			tcu::IVec3(32, 32, 1),	1),
	Texture(IMAGE_TYPE_2D_ARRAY,	tcu::IVec3(32, 32, 1),	4),
};

VkImageType getImageType (const ImageType textureImageType)
{
	switch (textureImageType)
	{
		case IMAGE_TYPE_2D:
		case IMAGE_TYPE_2D_ARRAY:
			return VK_IMAGE_TYPE_2D;

		default:
			DE_ASSERT(0);
			return VK_IMAGE_TYPE_LAST;
	}
}

VkImageViewType getImageViewType (const ImageType textureImageType)
{
	switch (textureImageType)
	{
		case IMAGE_TYPE_2D:
			return VK_IMAGE_VIEW_TYPE_2D;
		case IMAGE_TYPE_2D_ARRAY:
			return VK_IMAGE_VIEW_TYPE_2D_ARRAY;

		default:
			DE_ASSERT(0);
			return VK_IMAGE_VIEW_TYPE_LAST;
	}
}

static const VkFormat s_formats[] =
{
	VK_FORMAT_R32G32B32A32_SFLOAT,
	VK_FORMAT_R16G16B16A16_SFLOAT,
	VK_FORMAT_R32G32_SFLOAT,
	VK_FORMAT_R16G16_SFLOAT,
	VK_FORMAT_R32_SFLOAT,

	VK_FORMAT_R32G32B32A32_UINT,
	VK_FORMAT_R16G16B16A16_UINT,
	VK_FORMAT_R8G8B8A8_UINT,
	VK_FORMAT_R32G32_UINT,
	VK_FORMAT_R16G16_UINT,
	VK_FORMAT_R32_UINT,

	VK_FORMAT_R32G32B32A32_SINT,
	VK_FORMAT_R16G16B16A16_SINT,
	VK_FORMAT_R8G8B8A8_SINT,
	VK_FORMAT_R32G32_SINT,
	VK_FORMAT_R16G16_SINT,
	VK_FORMAT_R32_SINT,

	VK_FORMAT_R8G8B8A8_UNORM,

	VK_FORMAT_R8G8B8A8_SNORM,
};

inline bool formatsAreCompatible (const VkFormat format0, const VkFormat format1)
{
	return format0 == format1 || mapVkFormat(format0).getPixelSize() == mapVkFormat(format1).getPixelSize();
}

std::string getColorFormatStr (const int numComponents, const bool isUint, const bool isSint)
{
	std::ostringstream str;
	if (numComponents == 1)
		str << (isUint ? "uint" : isSint ? "int" : "float");
	else
		str << (isUint ? "u" : isSint ? "i" : "") << "vec" << numComponents;

	return str.str();
}

std::string getShaderSamplerType (const tcu::TextureFormat& format, VkImageViewType type)
{
	std::ostringstream samplerType;

	if (tcu::getTextureChannelClass(format.type) == tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER)
		samplerType << "u";
	else if (tcu::getTextureChannelClass(format.type) == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER)
		samplerType << "i";

	switch (type)
	{
		case VK_IMAGE_VIEW_TYPE_2D:
			samplerType << "sampler2D";
			break;

		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
			samplerType << "sampler2DArray";
			break;

		default:
			DE_FATAL("Ivalid image view type");
			break;
	}

	return samplerType.str();
}

void initPrograms (SourceCollections& programCollection, const CaseDef caseDef)
{
	if (caseDef.upload == UPLOAD_DRAW)
	{
		{
			std::ostringstream src;
			src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
				<< "\n"
				<< "layout(location = 0) in  vec4 in_position;\n"
				<< "layout(location = 1) in  vec4 in_color;\n"
				<< "layout(location = 0) out vec4 out_color;\n"
				<< "\n"
				<< "out gl_PerVertex {\n"
				<< "	vec4 gl_Position;\n"
				<< "};\n"
				<< "\n"
				<< "void main(void)\n"
				<< "{\n"
				<< "	gl_Position	= in_position;\n"
				<< "	out_color	= in_color;\n"
				<< "}\n";

			programCollection.glslSources.add("uploadDrawVert") << glu::VertexSource(src.str());
		}

		{
			const int	numComponents		= getNumUsedChannels(mapVkFormat(caseDef.viewFormat).order);
			const bool	isUint				= isUintFormat(caseDef.viewFormat);
			const bool	isSint				= isIntFormat(caseDef.viewFormat);
			const std::string colorFormat	= getColorFormatStr(numComponents, isUint, isSint);

			std::ostringstream src;
			src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
				<< "\n"
				<< "layout(location = 0) in  vec4 in_color;\n"
				<< "layout(location = 0) out " << colorFormat << " out_color;\n"
				<< "\n"
				<< "void main(void)\n"
				<< "{\n"
				<< "    out_color = " << colorFormat << "("
				<< (numComponents == 1 ? "in_color.r"   :
					numComponents == 2 ? "in_color.rg"  :
					numComponents == 3 ? "in_color.rgb" : "in_color")
				<< ");\n"
				<< "}\n";

			programCollection.glslSources.add("uploadDrawFrag") << glu::FragmentSource(src.str());
		}
	}

	if (caseDef.upload == UPLOAD_STORE)
	{
		const TextureFormat	tcuFormat		= mapVkFormat(caseDef.viewFormat);
		const std::string	imageFormatStr	= getShaderImageFormatQualifier(tcuFormat);
		const std::string	imageTypeStr	= getShaderImageType(tcuFormat, caseDef.imageType);
		const std::string	colorTypeStr	= isUintFormat(caseDef.viewFormat) ? "uvec4" : isIntFormat(caseDef.viewFormat) ? "ivec4" : "vec4";
		const bool			isIntegerFormat	= isUintFormat(caseDef.viewFormat) || isIntFormat(caseDef.viewFormat);

		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout (local_size_x = 1) in;\n"
			<< "\n"
			<< "layout(binding=0, " << imageFormatStr << ") writeonly uniform " << imageTypeStr << " u_image;\n"
			<< "\n"
			<< "const " << colorTypeStr << " colorTable[] = " << colorTypeStr << "[](\n";
		for (deUint32 idx = 0; idx < COLOR_TABLE_SIZE; idx++)
		{
			if (isIntegerFormat)
				src << "     " << colorTypeStr << "(" << COLOR_TABLE_INT[idx].x() << ", " << COLOR_TABLE_INT[idx].y() << ", " << COLOR_TABLE_INT[idx].z() << ", " << COLOR_TABLE_INT[idx].w() << ")";
			else
				src << "     " << colorTypeStr << "(" << COLOR_TABLE_FLOAT[idx].x() << ", " << COLOR_TABLE_FLOAT[idx].y() << ", " << COLOR_TABLE_FLOAT[idx].z() << ", " << COLOR_TABLE_FLOAT[idx].w() << ")";
			if (idx < COLOR_TABLE_SIZE - 1)
				src << ",";
			src << "\n";
		}
		src << ");\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n";
		if (caseDef.imageType == IMAGE_TYPE_2D)
		{
			src	<< "    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);\n";
		}
		else
		{
			DE_ASSERT(caseDef.imageType == IMAGE_TYPE_2D_ARRAY);
			src	<< "    ivec3 pos = ivec3(gl_GlobalInvocationID.xyz);\n";
		}
		src << "    " << colorTypeStr << " color = colorTable[gl_GlobalInvocationID.z];\n"
			<< "    imageStore(u_image, pos, color);\n"
			<< "}\n";

		programCollection.glslSources.add("uploadStoreComp") << glu::ComputeSource(src.str());
	}

	if (caseDef.download == DOWNLOAD_LOAD)
	{
		const TextureFormat	tcuFormat		= mapVkFormat(caseDef.viewFormat);
		const std::string	imageFormatStr	= getShaderImageFormatQualifier(tcuFormat);
		const std::string	imageTypeStr	= getShaderImageType(tcuFormat, caseDef.imageType);
		const std::string	colorTypeStr	= isUintFormat(caseDef.viewFormat) ? "uvec4" : isIntFormat(caseDef.viewFormat) ? "ivec4" : "vec4";

		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout (local_size_x = 1) in;\n"
			<< "\n"
			<< "layout(binding=0, " << imageFormatStr << ") readonly uniform " << imageTypeStr << " in_image;\n"
			<< "layout(binding=1, " << imageFormatStr << ") writeonly uniform " << imageTypeStr << " out_image;\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n";
		if (caseDef.imageType == IMAGE_TYPE_2D)
		{
			src	<< "    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);\n";
		}
		else
		{
			DE_ASSERT(caseDef.imageType == IMAGE_TYPE_2D_ARRAY);
			src	<< "    ivec3 pos = ivec3(gl_GlobalInvocationID.xyz);\n";
		}
		src	<< "    imageStore(out_image, pos, imageLoad(in_image, pos));\n"
			<< "}\n";

		programCollection.glslSources.add("downloadLoadComp") << glu::ComputeSource(src.str());
	}

	if (caseDef.download == DOWNLOAD_TEXTURE)
	{
		const TextureFormat		tcuFormat		= mapVkFormat(caseDef.viewFormat);
		const VkImageViewType	viewType		= getImageViewType(caseDef.imageType);
		const std::string		samplerTypeStr	= getShaderSamplerType(tcuFormat, viewType);
		const std::string		imageFormatStr	= getShaderImageFormatQualifier(tcuFormat);
		const std::string		imageTypeStr	= getShaderImageType(tcuFormat, caseDef.imageType);
		const std::string		colorTypeStr	= isUintFormat(caseDef.viewFormat) ? "uvec4" : isIntFormat(caseDef.viewFormat) ? "ivec4" : "vec4";

		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout (local_size_x = 1) in;\n"
			<< "\n"
			<< "layout(binding=0) uniform " << samplerTypeStr << " u_tex;\n"
			<< "layout(binding=1, " << imageFormatStr << ") writeonly uniform " << imageTypeStr << " out_image;\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n";
		if (caseDef.imageType == IMAGE_TYPE_2D)
		{
			src	<< "    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);\n";
		}
		else
		{
			DE_ASSERT(caseDef.imageType == IMAGE_TYPE_2D_ARRAY);
			src	<< "    ivec3 pos = ivec3(gl_GlobalInvocationID.xyz);\n";
		}
		src	<< "    imageStore(out_image, pos, texelFetch(u_tex, pos, 0));\n"
			<< "}\n";

		programCollection.glslSources.add("downloadTextureComp") << glu::ComputeSource(src.str());
	}
}

Move<VkImage> makeImage (const DeviceInterface&		vk,
						 const VkDevice				device,
						 VkImageCreateFlags			flags,
						 VkImageType				imageType,
						 const VkFormat				format,
						 const VkFormat				viewFormat,
						 const bool					useImageFormatList,
						 const IVec3&				size,
						 const deUint32				numMipLevels,
						 const deUint32				numLayers,
						 const VkImageUsageFlags	usage)
{
	const VkFormat formatList[2] =
	{
		format,
		viewFormat
	};

	const VkImageFormatListCreateInfoKHR formatListInfo =
	{
		VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR,	// VkStructureType			sType;
		DE_NULL,												// const void*				pNext;
		2u,														// deUint32					viewFormatCount
		formatList												// const VkFormat*			pViewFormats
	};

	const VkImageCreateInfo imageParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,					// VkStructureType			sType;
		useImageFormatList ? &formatListInfo : DE_NULL,			// const void*				pNext;
		flags,													// VkImageCreateFlags		flags;
		imageType,												// VkImageType				imageType;
		format,													// VkFormat					format;
		makeExtent3D(size),										// VkExtent3D				extent;
		numMipLevels,											// deUint32					mipLevels;
		numLayers,												// deUint32					arrayLayers;
		VK_SAMPLE_COUNT_1_BIT,									// VkSampleCountFlagBits	samples;
		VK_IMAGE_TILING_OPTIMAL,								// VkImageTiling			tiling;
		usage,													// VkImageUsageFlags		usage;
		VK_SHARING_MODE_EXCLUSIVE,								// VkSharingMode			sharingMode;
		0u,														// deUint32					queueFamilyIndexCount;
		DE_NULL,												// const deUint32*			pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,								// VkImageLayout			initialLayout;
	};
	return createImage(vk, device, &imageParams);
}

inline Move<VkBuffer> makeBuffer (const DeviceInterface& vk, const VkDevice device, const VkDeviceSize bufferSize, const VkBufferUsageFlags usage)
{
	const VkBufferCreateInfo bufferCreateInfo = makeBufferCreateInfo(bufferSize, usage);
	return createBuffer(vk, device, &bufferCreateInfo);
}

inline VkImageSubresourceRange makeColorSubresourceRange (const int baseArrayLayer, const int layerCount)
{
	return makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, static_cast<deUint32>(baseArrayLayer), static_cast<deUint32>(layerCount));
}

Move<VkSampler> makeSampler (const DeviceInterface& vk, const VkDevice device)
{
	const VkSamplerCreateInfo samplerParams =
	{
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,		// VkStructureType			sType;
		DE_NULL,									// const void*				pNext;
		(VkSamplerCreateFlags)0,					// VkSamplerCreateFlags		flags;
		VK_FILTER_NEAREST,							// VkFilter					magFilter;
		VK_FILTER_NEAREST,							// VkFilter					minFilter;
		VK_SAMPLER_MIPMAP_MODE_NEAREST,				// VkSamplerMipmapMode		mipmapMode;
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		// VkSamplerAddressMode		addressModeU;
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		// VkSamplerAddressMode		addressModeV;
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		// VkSamplerAddressMode		addressModeW;
		0.0f,										// float					mipLodBias;
		VK_FALSE,									// VkBool32					anisotropyEnable;
		1.0f,										// float					maxAnisotropy;
		VK_FALSE,									// VkBool32					compareEnable;
		VK_COMPARE_OP_ALWAYS,						// VkCompareOp				compareOp;
		0.0f,										// float					minLod;
		0.0f,										// float					maxLod;
		VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,	// VkBorderColor			borderColor;
		VK_FALSE,									// VkBool32					unnormalizedCoordinates;
	};

	return createSampler(vk, device, &samplerParams);
}


Move<VkPipelineLayout> makePipelineLayout (const DeviceInterface&	vk,
										   const VkDevice			device)
{
	const VkPipelineLayoutCreateInfo info =
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		DE_NULL,
		(VkPipelineLayoutCreateFlags)0,
		0u,
		DE_NULL,
		0u,
		DE_NULL,
	};
	return createPipelineLayout(vk, device, &info);
}

Move<VkPipeline> makeGraphicsPipeline (const DeviceInterface&		vk,
									   const VkDevice				device,
									   const VkPipelineLayout		pipelineLayout,
									   const VkRenderPass			renderPass,
									   const VkShaderModule			vertexModule,
									   const VkShaderModule			fragmentModule,
									   const IVec2&					renderSize,
									   const VkPrimitiveTopology	topology,
									   const deUint32				subpass)
{
	const VkVertexInputBindingDescription vertexInputBindingDescription =
	{
		0u,									// deUint32				binding;
		(deUint32)(2 * sizeof(Vec4)),		// deUint32				stride;
		VK_VERTEX_INPUT_RATE_VERTEX,		// VkVertexInputRate	inputRate;
	};

	const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[] =
	{
		{
			0u,								// deUint32			location;
			0u,								// deUint32			binding;
			VK_FORMAT_R32G32B32A32_SFLOAT,	// VkFormat			format;
			0u,								// deUint32			offset;
		},
		{
			1u,								// deUint32			location;
			0u,								// deUint32			binding;
			VK_FORMAT_R32G32B32A32_SFLOAT,	// VkFormat			format;
			(deUint32)sizeof(Vec4),			// deUint32			offset;
		}
	};

	const VkPipelineVertexInputStateCreateInfo vertexInputStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		(VkPipelineVertexInputStateCreateFlags)0,					// VkPipelineVertexInputStateCreateFlags	flags;
		1u,															// deUint32									vertexBindingDescriptionCount;
		&vertexInputBindingDescription,								// const VkVertexInputBindingDescription*	pVertexBindingDescriptions;
		DE_LENGTH_OF_ARRAY(vertexInputAttributeDescriptions),		// deUint32									vertexAttributeDescriptionCount;
		vertexInputAttributeDescriptions,							// const VkVertexInputAttributeDescription*	pVertexAttributeDescriptions;
	};

	const VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,														// const void*								pNext;
		(VkPipelineInputAssemblyStateCreateFlags)0,						// VkPipelineInputAssemblyStateCreateFlags	flags;
		topology,														// VkPrimitiveTopology						topology;
		VK_FALSE,														// VkBool32									primitiveRestartEnable;
	};

	const VkViewport viewport = makeViewport(
		0.0f, 0.0f,
		static_cast<float>(renderSize.x()), static_cast<float>(renderSize.y()),
		0.0f, 1.0f);

	const VkRect2D scissor =
	{
		makeOffset2D(0, 0),
		makeExtent2D(renderSize.x(), renderSize.y()),
	};

	const VkPipelineViewportStateCreateInfo pipelineViewportStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,		// VkStructureType						sType;
		DE_NULL,													// const void*							pNext;
		(VkPipelineViewportStateCreateFlags)0,						// VkPipelineViewportStateCreateFlags	flags;
		1u,															// deUint32								viewportCount;
		&viewport,													// const VkViewport*					pViewports;
		1u,															// deUint32								scissorCount;
		&scissor,													// const VkRect2D*						pScissors;
	};

	const VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		(VkPipelineRasterizationStateCreateFlags)0,					// VkPipelineRasterizationStateCreateFlags	flags;
		VK_FALSE,													// VkBool32									depthClampEnable;
		VK_FALSE,													// VkBool32									rasterizerDiscardEnable;
		VK_POLYGON_MODE_FILL,										// VkPolygonMode							polygonMode;
		VK_CULL_MODE_NONE,											// VkCullModeFlags							cullMode;
		VK_FRONT_FACE_COUNTER_CLOCKWISE,							// VkFrontFace								frontFace;
		VK_FALSE,													// VkBool32									depthBiasEnable;
		0.0f,														// float									depthBiasConstantFactor;
		0.0f,														// float									depthBiasClamp;
		0.0f,														// float									depthBiasSlopeFactor;
		1.0f,														// float									lineWidth;
	};

	const VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		(VkPipelineMultisampleStateCreateFlags)0,					// VkPipelineMultisampleStateCreateFlags	flags;
		VK_SAMPLE_COUNT_1_BIT,										// VkSampleCountFlagBits					rasterizationSamples;
		VK_FALSE,													// VkBool32									sampleShadingEnable;
		0.0f,														// float									minSampleShading;
		DE_NULL,													// const VkSampleMask*						pSampleMask;
		VK_FALSE,													// VkBool32									alphaToCoverageEnable;
		VK_FALSE													// VkBool32									alphaToOneEnable;
	};

	const VkStencilOpState stencilOpState = makeStencilOpState(
		VK_STENCIL_OP_KEEP,											// stencil fail
		VK_STENCIL_OP_ZERO,											// depth & stencil pass
		VK_STENCIL_OP_KEEP,											// depth only fail
		VK_COMPARE_OP_EQUAL,										// compare op
		~0u,														// compare mask
		~0u,														// write mask
		0u);														// reference

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		(VkPipelineDepthStencilStateCreateFlags)0,					// VkPipelineDepthStencilStateCreateFlags	flags;
		VK_FALSE,													// VkBool32									depthTestEnable;
		VK_FALSE,													// VkBool32									depthWriteEnable;
		VK_COMPARE_OP_LESS,											// VkCompareOp								depthCompareOp;
		VK_FALSE,													// VkBool32									depthBoundsTestEnable;
		VK_FALSE,													// VkBool32									stencilTestEnable;
		stencilOpState,												// VkStencilOpState							front;
		stencilOpState,												// VkStencilOpState							back;
		0.0f,														// float									minDepthBounds;
		1.0f,														// float									maxDepthBounds;
	};

	const VkColorComponentFlags colorComponentsAll = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	const VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState =
	{
		VK_FALSE,													// VkBool32					blendEnable;
		VK_BLEND_FACTOR_ONE,										// VkBlendFactor			srcColorBlendFactor;
		VK_BLEND_FACTOR_ZERO,										// VkBlendFactor			dstColorBlendFactor;
		VK_BLEND_OP_ADD,											// VkBlendOp				colorBlendOp;
		VK_BLEND_FACTOR_ONE,										// VkBlendFactor			srcAlphaBlendFactor;
		VK_BLEND_FACTOR_ZERO,										// VkBlendFactor			dstAlphaBlendFactor;
		VK_BLEND_OP_ADD,											// VkBlendOp				alphaBlendOp;
		colorComponentsAll,											// VkColorComponentFlags	colorWriteMask;
	};

	const VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,	// VkStructureType								sType;
		DE_NULL,													// const void*									pNext;
		(VkPipelineColorBlendStateCreateFlags)0,					// VkPipelineColorBlendStateCreateFlags			flags;
		VK_FALSE,													// VkBool32										logicOpEnable;
		VK_LOGIC_OP_COPY,											// VkLogicOp									logicOp;
		1u,															// deUint32										attachmentCount;
		&pipelineColorBlendAttachmentState,							// const VkPipelineColorBlendAttachmentState*	pAttachments;
		{ 0.0f, 0.0f, 0.0f, 0.0f },									// float										blendConstants[4];
	};

	const VkPipelineShaderStageCreateInfo pShaderStages[] =
	{
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType						sType;
			DE_NULL,												// const void*							pNext;
			(VkPipelineShaderStageCreateFlags)0,					// VkPipelineShaderStageCreateFlags		flags;
			VK_SHADER_STAGE_VERTEX_BIT,								// VkShaderStageFlagBits				stage;
			vertexModule,											// VkShaderModule						module;
			"main",													// const char*							pName;
			DE_NULL,												// const VkSpecializationInfo*			pSpecializationInfo;
		},
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType						sType;
			DE_NULL,												// const void*							pNext;
			(VkPipelineShaderStageCreateFlags)0,					// VkPipelineShaderStageCreateFlags		flags;
			VK_SHADER_STAGE_FRAGMENT_BIT,							// VkShaderStageFlagBits				stage;
			fragmentModule,											// VkShaderModule						module;
			"main",													// const char*							pName;
			DE_NULL,												// const VkSpecializationInfo*			pSpecializationInfo;
		}
	};

	const VkGraphicsPipelineCreateInfo	graphicsPipelineInfo =
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	// VkStructureType									sType;
		DE_NULL,											// const void*										pNext;
		0u,													// VkPipelineCreateFlags							flags;
		DE_LENGTH_OF_ARRAY(pShaderStages),					// deUint32											stageCount;
		pShaderStages,										// const VkPipelineShaderStageCreateInfo*			pStages;
		&vertexInputStateInfo,								// const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
		&pipelineInputAssemblyStateInfo,					// const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
		DE_NULL,											// const VkPipelineTessellationStateCreateInfo*		pTessellationState;
		&pipelineViewportStateInfo,							// const VkPipelineViewportStateCreateInfo*			pViewportState;
		&pipelineRasterizationStateInfo,					// const VkPipelineRasterizationStateCreateInfo*	pRasterizationState;
		&pipelineMultisampleStateInfo,						// const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
		&pipelineDepthStencilStateInfo,						// const VkPipelineDepthStencilStateCreateInfo*		pDepthStencilState;
		&pipelineColorBlendStateInfo,						// const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
		DE_NULL,											// const VkPipelineDynamicStateCreateInfo*			pDynamicState;
		pipelineLayout,										// VkPipelineLayout									layout;
		renderPass,											// VkRenderPass										renderPass;
		subpass,											// deUint32											subpass;
		DE_NULL,											// VkPipeline										basePipelineHandle;
		0,													// deInt32											basePipelineIndex;
	};

	return createGraphicsPipeline(vk, device, DE_NULL, &graphicsPipelineInfo);
}

Move<VkPipeline> makeComputePipeline (const DeviceInterface&		vk,
									  const VkDevice				device,
									  const VkPipelineLayout		pipelineLayout,
									  const VkShaderModule			shaderModule,
									  const VkSpecializationInfo*	specInfo)
{
	const VkPipelineShaderStageCreateInfo shaderStageInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType					sType;
		DE_NULL,												// const void*						pNext;
		(VkPipelineShaderStageCreateFlags)0,					// VkPipelineShaderStageCreateFlags	flags;
		VK_SHADER_STAGE_COMPUTE_BIT,							// VkShaderStageFlagBits			stage;
		shaderModule,											// VkShaderModule					module;
		"main",													// const char*						pName;
		specInfo,												// const VkSpecializationInfo*		pSpecializationInfo;
	};
	const VkComputePipelineCreateInfo pipelineInfo =
	{
		VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,			// VkStructureType					sType;
		DE_NULL,												// const void*						pNext;
		(VkPipelineCreateFlags)0,								// VkPipelineCreateFlags			flags;
		shaderStageInfo,										// VkPipelineShaderStageCreateInfo	stage;
		pipelineLayout,											// VkPipelineLayout					layout;
		DE_NULL,												// VkPipeline						basePipelineHandle;
		0,														// deInt32							basePipelineIndex;
	};
	return createComputePipeline(vk, device, DE_NULL , &pipelineInfo);
}

Move<VkRenderPass> makeRenderPass (const DeviceInterface&	vk,
								   const VkDevice			device,
								   const VkFormat			colorFormat,
								   const deUint32			numLayers)
{
	const VkAttachmentDescription colorAttachmentDescription =
	{
		(VkAttachmentDescriptionFlags)0,					// VkAttachmentDescriptionFlags		flags;
		colorFormat,										// VkFormat							format;
		VK_SAMPLE_COUNT_1_BIT,								// VkSampleCountFlagBits			samples;
		VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp				loadOp;
		VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp				storeOp;
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				stencilLoadOp;
		VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp;
		VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout					initialLayout;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout					finalLayout;
	};
	vector<VkAttachmentDescription> attachmentDescriptions(numLayers, colorAttachmentDescription);

	// Create a subpass for each attachment (each attachement is a layer of an arrayed image).
	vector<VkAttachmentReference>	colorAttachmentReferences	(numLayers);
	vector<VkSubpassDescription>	subpasses;

	// Ordering here must match the framebuffer attachments
	for (deUint32 i = 0; i < numLayers; ++i)
	{
		const VkAttachmentReference attachmentRef =
		{
			i,													// deUint32			attachment;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL			// VkImageLayout	layout;
		};

		colorAttachmentReferences[i]	= attachmentRef;

		const VkSubpassDescription		subpassDescription	=
		{
			(VkSubpassDescriptionFlags)0,					// VkSubpassDescriptionFlags		flags;
			VK_PIPELINE_BIND_POINT_GRAPHICS,				// VkPipelineBindPoint				pipelineBindPoint;
			0u,												// deUint32							inputAttachmentCount;
			DE_NULL,										// const VkAttachmentReference*		pInputAttachments;
			1u,												// deUint32							colorAttachmentCount;
			&colorAttachmentReferences[i],					// const VkAttachmentReference*		pColorAttachments;
			DE_NULL,										// const VkAttachmentReference*		pResolveAttachments;
			DE_NULL,										// const VkAttachmentReference*		pDepthStencilAttachment;
			0u,												// deUint32							preserveAttachmentCount;
			DE_NULL											// const deUint32*					pPreserveAttachments;
		};
		subpasses.push_back(subpassDescription);
	}

	const VkRenderPassCreateInfo renderPassInfo =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,				// VkStructureType					sType;
		DE_NULL,												// const void*						pNext;
		(VkRenderPassCreateFlags)0,								// VkRenderPassCreateFlags			flags;
		static_cast<deUint32>(attachmentDescriptions.size()),	// deUint32							attachmentCount;
		&attachmentDescriptions[0],								// const VkAttachmentDescription*	pAttachments;
		static_cast<deUint32>(subpasses.size()),				// deUint32							subpassCount;
		&subpasses[0],											// const VkSubpassDescription*		pSubpasses;
		0u,														// deUint32							dependencyCount;
		DE_NULL													// const VkSubpassDependency*		pDependencies;
	};

	return createRenderPass(vk, device, &renderPassInfo);
}

Move<VkFramebuffer> makeFramebuffer (const DeviceInterface&	vk,
									 const VkDevice			device,
									 const VkRenderPass		renderPass,
									 const deUint32			attachmentCount,
									 const VkImageView*		pAttachments,
									 const IVec2			size)
{
	const VkFramebufferCreateInfo framebufferInfo =
	{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		DE_NULL,
		(VkFramebufferCreateFlags)0,
		renderPass,
		attachmentCount,
		pAttachments,
		static_cast<deUint32>(size.x()),
		static_cast<deUint32>(size.y()),
		1u,
	};

	return createFramebuffer(vk, device, &framebufferInfo);
}

Move<VkCommandBuffer> makeCommandBuffer	(const DeviceInterface& vk, const VkDevice device, const VkCommandPool commandPool)
{
	return allocateCommandBuffer(vk, device, commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

MovePtr<Allocation> bindImage (const DeviceInterface& vk, const VkDevice device, Allocator& allocator, const VkImage image, const MemoryRequirement requirement)
{
        MovePtr<Allocation> alloc = allocator.allocate(getImageMemoryRequirements(vk, device, image), requirement);
        VK_CHECK(vk.bindImageMemory(device, image, alloc->getMemory(), alloc->getOffset()));
        return alloc;
}

MovePtr<Allocation> bindBuffer (const DeviceInterface& vk, const VkDevice device, Allocator& allocator, const VkBuffer buffer, const MemoryRequirement requirement)
{
        MovePtr<Allocation> alloc(allocator.allocate(getBufferMemoryRequirements(vk, device, buffer), requirement));
        VK_CHECK(vk.bindBufferMemory(device, buffer, alloc->getMemory(), alloc->getOffset()));
        return alloc;
}

vector<Vec4> genVertexData (const CaseDef& caseDef)
{
	vector<Vec4>	vectorData;
	const bool		isIntegerFormat	= isUintFormat(caseDef.viewFormat) || isIntFormat(caseDef.viewFormat);

	for (deUint32 z = 0; z < caseDef.numLayers; z++)
	{
		const deUint32	colorIdx	= z % COLOR_TABLE_SIZE;
		const Vec4		color		= isIntegerFormat ? COLOR_TABLE_INT[colorIdx].cast<float>() : COLOR_TABLE_FLOAT[colorIdx];

		vectorData.push_back(Vec4(-1.0f, -1.0f, 0.0f, 1.0f));
		vectorData.push_back(color);
		vectorData.push_back(Vec4(-1.0f,  1.0f, 0.0f, 1.0f));
		vectorData.push_back(color);
		vectorData.push_back(Vec4( 1.0f, -1.0f, 0.0f, 1.0f));
		vectorData.push_back(color);
		vectorData.push_back(Vec4( 1.0f,  1.0f, 0.0f, 1.0f));
		vectorData.push_back(color);
	}

	return vectorData;
}

void generateExpectedImage(const tcu::PixelBufferAccess& image, const CaseDef& caseDef)
{
	const tcu::TextureChannelClass	channelClass	= tcu::getTextureChannelClass(image.getFormat().type);
	const bool						isIntegerFormat	= channelClass == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER || channelClass == tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER;
	const IVec2						size			= caseDef.size.swizzle(0, 1);

	for (int z = 0; z < static_cast<int>(caseDef.numLayers); z++)
	{
		const deUint32	colorIdx	= z % COLOR_TABLE_SIZE;
		for (int y = 0; y < size.y(); y++)
		for (int x = 0; x < size.x(); x++)
		{
			if (isIntegerFormat)
				image.setPixel(COLOR_TABLE_INT[colorIdx], x, y, z);
			else
				image.setPixel(COLOR_TABLE_FLOAT[colorIdx], x, y, z);
		}
	}
}

VkImageUsageFlags getImageUsageForTestCase (const CaseDef& caseDef)
{
	VkImageUsageFlags flags = 0u;

	switch (caseDef.upload)
	{
	case UPLOAD_CLEAR:
		flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		break;
	case UPLOAD_DRAW:
		flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		break;
	case UPLOAD_STORE:
		flags |= VK_IMAGE_USAGE_STORAGE_BIT;
		break;
	case UPLOAD_COPY:
		flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		break;
	default:
		DE_ASSERT("Invalid upload method");
		break;
	}

	switch (caseDef.download)
	{
	case DOWNLOAD_TEXTURE:
		flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
		break;
	case DOWNLOAD_LOAD:
		flags |= VK_IMAGE_USAGE_STORAGE_BIT;
		break;
	case DOWNLOAD_COPY:
		flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		break;
	default:
		DE_ASSERT("Invalid download method");
		break;
	}

	// We can only create a view for the image if it is going to be used for any of these usages,
	// so let's make sure that we have at least one of them.
	VkImageUsageFlags viewRequiredFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (!(flags & viewRequiredFlags))
		flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	return flags;
}

// Executes a combination of upload/download methods
class UploadDownloadExecutor
{
public:
	UploadDownloadExecutor(Context &context, const CaseDef& caseSpec) :
	m_caseDef(caseSpec),
	m_haveMaintenance2(de::contains(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), "VK_KHR_maintenance2")),
	m_vk(context.getDeviceInterface()),
	m_device(context.getDevice()),
	m_queue(context.getUniversalQueue()),
	m_queueFamilyIndex(context.getUniversalQueueFamilyIndex()),
	m_allocator(context.getDefaultAllocator())
	{
	}

	void run(Context& context, VkBuffer buffer);

private:
	void uploadClear(Context& context);
	void uploadStore(Context& context);
	void uploadCopy(Context& context);
	void uploadDraw(Context& context);
	void downloadCopy(Context& context, VkBuffer buffer);
	void downloadTexture(Context& context, VkBuffer buffer);
	void downloadLoad(Context& context, VkBuffer buffer);

	void copyImageToBuffer(VkImage				image,
						   VkBuffer				buffer,
						   const IVec3			size,
						   const VkAccessFlags	srcAccessMask,
						   const VkImageLayout	oldLayout,
						   const deUint32		numLayers);

	const CaseDef&						m_caseDef;

	bool								m_haveMaintenance2;

	const DeviceInterface&				m_vk;
	const VkDevice						m_device;
	const VkQueue						m_queue;
	const deUint32						m_queueFamilyIndex;
	Allocator&							m_allocator;

	Move<VkCommandPool>					m_cmdPool;
	Move<VkCommandBuffer>				m_cmdBuffer;

	bool								m_imageIsIntegerFormat;
	bool								m_viewIsIntegerFormat;

	// Target image for upload paths
	Move<VkImage>						m_image;
	MovePtr<Allocation>					m_imageAlloc;

	// Upload copy
	struct
	{
		Move<VkBuffer>					colorBuffer;
		VkDeviceSize					colorBufferSize;
		MovePtr<Allocation>				colorBufferAlloc;
	} m_uCopy;

	// Upload draw
	struct
	{
		Move<VkBuffer>					vertexBuffer;
		MovePtr<Allocation>				vertexBufferAlloc;
		Move<VkPipelineLayout>			pipelineLayout;
		Move<VkRenderPass>				renderPass;
		Move<VkShaderModule>			vertexModule;
		Move<VkShaderModule>			fragmentModule;
		vector<SharedPtrVkImageView>	attachments;
		vector<VkImageView>				attachmentHandles;
		vector<SharedPtrVkPipeline>		pipelines;
		Move<VkFramebuffer>				framebuffer;
	} m_uDraw;

	// Upload store
	struct
	{
		Move<VkDescriptorPool>			descriptorPool;
		Move<VkPipelineLayout>			pipelineLayout;
		Move<VkDescriptorSetLayout>		descriptorSetLayout;
		Move<VkDescriptorSet>			descriptorSet;
		VkDescriptorImageInfo			imageDescriptorInfo;
		Move<VkShaderModule>			computeModule;
		Move<VkPipeline>				computePipeline;
		Move<VkImageView>				imageView;
	} m_uStore;

	// Download load
	struct
	{
		Move<VkDescriptorPool>			descriptorPool;
		Move<VkPipelineLayout>			pipelineLayout;
		Move<VkDescriptorSetLayout>		descriptorSetLayout;
		Move<VkDescriptorSet>			descriptorSet;
		Move<VkShaderModule>			computeModule;
		Move<VkPipeline>				computePipeline;
		Move<VkImageView>				inImageView;
		VkDescriptorImageInfo			inImageDescriptorInfo;
		Move<VkImage>					outImage;
		Move<VkImageView>				outImageView;
		MovePtr<Allocation>				outImageAlloc;
		VkDescriptorImageInfo			outImageDescriptorInfo;
	} m_dLoad;

	// Download texture
	struct
	{
		Move<VkDescriptorPool>			descriptorPool;
		Move<VkPipelineLayout>			pipelineLayout;
		Move<VkDescriptorSetLayout>		descriptorSetLayout;
		Move<VkDescriptorSet>			descriptorSet;
		Move<VkShaderModule>			computeModule;
		Move<VkPipeline>				computePipeline;
		Move<VkImageView>				inImageView;
		VkDescriptorImageInfo			inImageDescriptorInfo;
		Move<VkSampler>					sampler;
		Move<VkImage>					outImage;
		Move<VkImageView>				outImageView;
		MovePtr<Allocation>				outImageAlloc;
		VkDescriptorImageInfo			outImageDescriptorInfo;
	} m_dTex;

	VkImageLayout						m_imageLayoutAfterUpload;
	VkAccessFlagBits					m_imageUploadAccessMask;
};

void UploadDownloadExecutor::run(Context& context, VkBuffer buffer)
{
	m_imageIsIntegerFormat	= isUintFormat(m_caseDef.imageFormat) || isIntFormat(m_caseDef.imageFormat);
	m_viewIsIntegerFormat	= isUintFormat(m_caseDef.viewFormat) || isIntFormat(m_caseDef.viewFormat);

	m_cmdPool				= createCommandPool(m_vk, m_device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, m_queueFamilyIndex);
	m_cmdBuffer				= makeCommandBuffer(m_vk, m_device, *m_cmdPool);
	beginCommandBuffer(m_vk, *m_cmdBuffer);

	const VkImageUsageFlags		imageUsage	= getImageUsageForTestCase(m_caseDef);
	const VkImageCreateFlags	imageFlags	= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | (m_haveMaintenance2 ? VK_IMAGE_CREATE_EXTENDED_USAGE_BIT_KHR : 0);
	m_image									= makeImage(m_vk, m_device, imageFlags, getImageType(m_caseDef.imageType), m_caseDef.imageFormat, m_caseDef.viewFormat,
														m_caseDef.isFormatListTest, m_caseDef.size, 1u, m_caseDef.numLayers, imageUsage);
	m_imageAlloc							= bindImage(m_vk, m_device, m_allocator, *m_image, MemoryRequirement::Any);

	switch (m_caseDef.upload)
	{
	case UPLOAD_DRAW:
		uploadDraw(context);
		break;
	case UPLOAD_STORE:
		uploadStore(context);
		break;
	case UPLOAD_CLEAR:
		uploadClear(context);
		break;
	case UPLOAD_COPY:
		uploadCopy(context);
		break;
	default:
		DE_ASSERT("Unsupported upload method");
	}

	switch (m_caseDef.download)
	{
	case DOWNLOAD_COPY:
		downloadCopy(context, buffer);
		break;
	case DOWNLOAD_LOAD:
		downloadLoad(context, buffer);
		break;
	case DOWNLOAD_TEXTURE:
		downloadTexture(context, buffer);
		break;
	default:
		DE_ASSERT("Unsupported download method");
	}

	VK_CHECK(m_vk.endCommandBuffer(*m_cmdBuffer));
	submitCommandsAndWait(m_vk, m_device, m_queue, *m_cmdBuffer);
}

void UploadDownloadExecutor::uploadClear(Context& context)
{
	(void) context;

	VkImageLayout					requiredImageLayout	= VK_IMAGE_LAYOUT_GENERAL;

	const VkImageSubresourceRange	subresourceRange	= makeColorSubresourceRange(0, m_caseDef.numLayers);
	const VkImageMemoryBarrier		imageInitBarrier	=
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType			sType;
		DE_NULL,										// const void*				pNext;
		0u,												// VkAccessFlags			srcAccessMask;
		VK_ACCESS_TRANSFER_WRITE_BIT,					// VkAccessFlags			dstAcessMask;
		VK_IMAGE_LAYOUT_UNDEFINED,						// VkImageLayout			oldLayout;
		requiredImageLayout,							// VkImageLayout			newLayout;
		VK_QUEUE_FAMILY_IGNORED,						// deUint32					srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,						// deUint32					destQueueFamilyIndex;
		*m_image,										// VkImage					image;
		subresourceRange								// VkImageSubresourceRange	subresourceRange;
	};

	m_vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
		0u, DE_NULL, 0u, DE_NULL, 1u, &imageInitBarrier);

	for (deUint32 layer = 0; layer < m_caseDef.numLayers; layer++)
	{
		const VkImageSubresourceRange	layerSubresourceRange	= makeColorSubresourceRange(layer, 1u);
		const deUint32					colorIdx				= layer % COLOR_TABLE_SIZE;
		const VkClearColorValue			clearColor				= m_imageIsIntegerFormat ? REFERENCE_CLEAR_COLOR_INT[colorIdx].color : REFERENCE_CLEAR_COLOR_FLOAT[colorIdx].color;
		m_vk.cmdClearColorImage(*m_cmdBuffer, *m_image, requiredImageLayout, &clearColor, 1u, &layerSubresourceRange);
	}

	m_imageLayoutAfterUpload	= requiredImageLayout;
	m_imageUploadAccessMask		= VK_ACCESS_TRANSFER_WRITE_BIT;
}

void UploadDownloadExecutor::uploadStore(Context& context)
{
	const vk::VkImageViewUsageCreateInfoKHR viewUsageCreateInfo = {
		VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO_KHR,	// VkStructureType		sType
		DE_NULL,											// const void*			pNext
		VK_IMAGE_USAGE_STORAGE_BIT,							// VkImageUsageFlags	usage;
	};
	m_uStore.imageView				= makeImageView(m_vk, m_device, *m_image, getImageViewType(m_caseDef.imageType), m_caseDef.viewFormat,
													makeColorSubresourceRange(0, m_caseDef.numLayers), m_haveMaintenance2 ? &viewUsageCreateInfo : DE_NULL);

	// Setup compute pipeline
	m_uStore.descriptorPool			= DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		.build(m_vk, m_device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);

	m_uStore.descriptorSetLayout	= DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(m_vk, m_device);

	m_uStore.pipelineLayout			= makePipelineLayout(m_vk, m_device, *m_uStore.descriptorSetLayout);
	m_uStore.descriptorSet			= makeDescriptorSet(m_vk, m_device, *m_uStore.descriptorPool, *m_uStore.descriptorSetLayout);
	m_uStore.imageDescriptorInfo	= makeDescriptorImageInfo(DE_NULL, *m_uStore.imageView, VK_IMAGE_LAYOUT_GENERAL);
	m_uStore.computeModule			= createShaderModule(m_vk, m_device, context.getBinaryCollection().get("uploadStoreComp"), 0);
	m_uStore.computePipeline		= makeComputePipeline(m_vk, m_device, *m_uStore.pipelineLayout, *m_uStore.computeModule, DE_NULL);

	DescriptorSetUpdateBuilder()
		.writeSingle(*m_uStore.descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &m_uStore.imageDescriptorInfo)
		.update(m_vk, m_device);

	// Transition storage image for shader access (imageStore)
	VkImageLayout requiredImageLayout	= VK_IMAGE_LAYOUT_GENERAL;
	const VkImageMemoryBarrier imageBarrier	=
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,				// VkStructureType			sType;
		DE_NULL,											// const void*				pNext;
		(VkAccessFlags)0,									// VkAccessFlags			srcAccessMask;
		(VkAccessFlags)VK_ACCESS_SHADER_WRITE_BIT,			// VkAccessFlags			dstAccessMask;
		VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout			oldLayout;
		requiredImageLayout,								// VkImageLayout			newLayout;
		VK_QUEUE_FAMILY_IGNORED,							// deUint32					srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,							// deUint32					destQueueFamilyIndex;
		*m_image,											// VkImage					image;
		makeColorSubresourceRange(0, m_caseDef.numLayers),	// VkImageSubresourceRange	subresourceRange;
	};

	m_vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0u,
		0u, DE_NULL, 0u, DE_NULL, 1u, &imageBarrier);

	// Dispatch
	m_vk.cmdBindPipeline(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *m_uStore.computePipeline);
	m_vk.cmdBindDescriptorSets(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *m_uStore.pipelineLayout, 0u, 1u, &m_uStore.descriptorSet.get(), 0u, DE_NULL);
	m_vk.cmdDispatch(*m_cmdBuffer, m_caseDef.size.x(), m_caseDef.size.y(), m_caseDef.numLayers);

	m_imageLayoutAfterUpload	= requiredImageLayout;
	m_imageUploadAccessMask		= VK_ACCESS_SHADER_WRITE_BIT;
}

void UploadDownloadExecutor::uploadCopy(Context& context)
{
	(void) context;

	// Create a host-mappable buffer with the color data to upload
	const VkDeviceSize	pixelSize			= tcu::getPixelSize(mapVkFormat(m_caseDef.imageFormat));
	const VkDeviceSize	layerSize			= m_caseDef.size.x() * m_caseDef.size.y() * m_caseDef.size.z() * pixelSize;

	m_uCopy.colorBufferSize					= layerSize * m_caseDef.numLayers;
	m_uCopy.colorBuffer						= makeBuffer(m_vk, m_device, m_uCopy.colorBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	m_uCopy.colorBufferAlloc				= bindBuffer(m_vk, m_device, m_allocator, *m_uCopy.colorBuffer, MemoryRequirement::HostVisible);

	// Fill color buffer
	const tcu::TextureFormat	tcuFormat	= mapVkFormat(m_caseDef.imageFormat);
	VkDeviceSize				layerOffset = 0ull;
	for (deUint32 layer = 0; layer < m_caseDef.numLayers; layer++)
	{
		tcu::PixelBufferAccess	imageAccess	= tcu::PixelBufferAccess(tcuFormat, m_caseDef.size.x(), m_caseDef.size.y(), 1u, (deUint8*) m_uCopy.colorBufferAlloc->getHostPtr() + layerOffset);
		const deUint32			colorIdx	= layer % COLOR_TABLE_SIZE;
		if (m_imageIsIntegerFormat)
			tcu::clear(imageAccess, COLOR_TABLE_INT[colorIdx]);
		else
			tcu::clear(imageAccess, COLOR_TABLE_FLOAT[colorIdx]);
		layerOffset += layerSize;
	}

	flushMappedMemoryRange(m_vk, m_device, m_uCopy.colorBufferAlloc->getMemory(), m_uCopy.colorBufferAlloc->getOffset(), VK_WHOLE_SIZE);

	// Prepare buffer and image for copy
	const VkBufferMemoryBarrier	bufferInitBarrier	=
	{
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,			// VkStructureType    sType;
		DE_NULL,											// const void*        pNext;
		VK_ACCESS_HOST_WRITE_BIT,							// VkAccessFlags      srcAccessMask;
		VK_ACCESS_TRANSFER_READ_BIT,						// VkAccessFlags      dstAccessMask;
		VK_QUEUE_FAMILY_IGNORED,							// deUint32           srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,							// deUint32           dstQueueFamilyIndex;
		*m_uCopy.colorBuffer,								// VkBuffer           buffer;
		0ull,												// VkDeviceSize       offset;
		VK_WHOLE_SIZE,										// VkDeviceSize       size;
	};

	const VkImageMemoryBarrier	imageInitBarrier	=
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,				// VkStructureType			sType;
		DE_NULL,											// const void*				pNext;
		0u,													// VkAccessFlags			srcAccessMask;
		VK_ACCESS_TRANSFER_WRITE_BIT,						// VkAccessFlags			dstAccessMask;
		VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout			oldLayout;
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,				// VkImageLayout			newLayout;
		VK_QUEUE_FAMILY_IGNORED,							// deUint32					srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,							// deUint32					destQueueFamilyIndex;
		*m_image,											// VkImage					image;
		makeColorSubresourceRange(0, m_caseDef.numLayers)	// VkImageSubresourceRange	subresourceRange;
	};

	m_vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
		0u, DE_NULL, 1u, &bufferInitBarrier, 1u, &imageInitBarrier);

	// Copy buffer to image
	const VkImageSubresourceLayers	subresource	=
	{
		VK_IMAGE_ASPECT_COLOR_BIT,							// VkImageAspectFlags    aspectMask;
		0u,													// deUint32              mipLevel;
		0u,													// deUint32              baseArrayLayer;
		m_caseDef.numLayers,								// deUint32              layerCount;
	};

	const VkBufferImageCopy			region		=
	{
		0ull,												// VkDeviceSize                bufferOffset;
		0u,													// deUint32                    bufferRowLength;
		0u,													// deUint32                    bufferImageHeight;
		subresource,										// VkImageSubresourceLayers    imageSubresource;
		makeOffset3D(0, 0, 0),								// VkOffset3D                  imageOffset;
		makeExtent3D(m_caseDef.size),						// VkExtent3D                  imageExtent;
	};

	m_vk.cmdCopyBufferToImage(*m_cmdBuffer, *m_uCopy.colorBuffer, *m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &region);

	const VkImageMemoryBarrier	imagePostInitBarrier	=
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,				// VkStructureType			sType;
		DE_NULL,											// const void*				pNext;
		VK_ACCESS_TRANSFER_WRITE_BIT,						// VkAccessFlags			srcAccessMask;
		VK_ACCESS_TRANSFER_READ_BIT,						// VkAccessFlags			dstAccessMask;
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,				// VkImageLayout			oldLayout;
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,				// VkImageLayout			newLayout;
		VK_QUEUE_FAMILY_IGNORED,							// deUint32					srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,							// deUint32					destQueueFamilyIndex;
		*m_image,											// VkImage					image;
		makeColorSubresourceRange(0, m_caseDef.numLayers)	// VkImageSubresourceRange	subresourceRange;
	};

	m_vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
		0u, DE_NULL, 0u, DE_NULL, 1u, &imagePostInitBarrier);

	m_imageLayoutAfterUpload	= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	m_imageUploadAccessMask		= VK_ACCESS_TRANSFER_WRITE_BIT;
}

void UploadDownloadExecutor::uploadDraw(Context& context)
{
	// Create vertex buffer
	{
		const vector<Vec4>	vertices				= genVertexData(m_caseDef);
		const VkDeviceSize	vertexBufferSize		= vertices.size() * sizeof(Vec4);

		m_uDraw.vertexBuffer						= makeBuffer(m_vk, m_device, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		m_uDraw.vertexBufferAlloc					= bindBuffer(m_vk, m_device, m_allocator, *m_uDraw.vertexBuffer, MemoryRequirement::HostVisible);
		deMemcpy(m_uDraw.vertexBufferAlloc->getHostPtr(), &vertices[0], static_cast<std::size_t>(vertexBufferSize));
		flushMappedMemoryRange(m_vk, m_device, m_uDraw.vertexBufferAlloc->getMemory(), m_uDraw.vertexBufferAlloc->getOffset(), vertexBufferSize);
	}

	// Create attachments and pipelines for each image layer
	m_uDraw.pipelineLayout							= makePipelineLayout(m_vk, m_device);
	m_uDraw.renderPass								= makeRenderPass(m_vk, m_device, m_caseDef.viewFormat, m_caseDef.numLayers);
	m_uDraw.vertexModule							= createShaderModule(m_vk, m_device, context.getBinaryCollection().get("uploadDrawVert"), 0u);
	m_uDraw.fragmentModule							= createShaderModule(m_vk, m_device, context.getBinaryCollection().get("uploadDrawFrag"), 0u);

	for (deUint32 subpassNdx = 0; subpassNdx < m_caseDef.numLayers; ++subpassNdx)
	{
		const vk::VkImageViewUsageCreateInfoKHR viewUsageCreateInfo = {
			VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO_KHR,	// VkStructureType		sType
			DE_NULL,											// const void*			pNext
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,				// VkImageUsageFlags	usage;
		};
		Move<VkImageView>	imageView	= makeImageView(m_vk, m_device, *m_image, getImageViewType(m_caseDef.imageType), m_caseDef.viewFormat,
														makeColorSubresourceRange(subpassNdx, 1), m_haveMaintenance2 ? &viewUsageCreateInfo : DE_NULL);
		m_uDraw.attachmentHandles.push_back(*imageView);
		m_uDraw.attachments.push_back(makeSharedPtr(imageView));
		m_uDraw.pipelines.push_back(makeSharedPtr(makeGraphicsPipeline(m_vk, m_device, *m_uDraw.pipelineLayout, *m_uDraw.renderPass, *m_uDraw.vertexModule, *m_uDraw.fragmentModule,
			m_caseDef.size.swizzle(0, 1), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, subpassNdx)));
	}

	// Create framebuffer
	m_uDraw.framebuffer	= makeFramebuffer(m_vk, m_device, *m_uDraw.renderPass, static_cast<deUint32>(m_uDraw.attachmentHandles.size()), &m_uDraw.attachmentHandles[0], m_caseDef.size.swizzle(0, 1));

	// Create command buffer
	{
		{
			vector<VkClearValue>	clearValues		(m_caseDef.numLayers, m_viewIsIntegerFormat ? REFERENCE_CLEAR_COLOR_INT[0] : REFERENCE_CLEAR_COLOR_FLOAT[0]);

			const VkRect2D			renderArea	=
			{
				makeOffset2D(0, 0),
				makeExtent2D(m_caseDef.size.x(), m_caseDef.size.y()),
			};

			const VkRenderPassBeginInfo	renderPassBeginInfo	=
			{
				VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,	// VkStructureType         sType;
				DE_NULL,									// const void*             pNext;
				*m_uDraw.renderPass,							// VkRenderPass            renderPass;
				*m_uDraw.framebuffer,							// VkFramebuffer           framebuffer;
				renderArea,									// VkRect2D                renderArea;
				static_cast<deUint32>(clearValues.size()),	// deUint32                clearValueCount;
				&clearValues[0],							// const VkClearValue*     pClearValues;
			};

			m_vk.cmdBeginRenderPass(*m_cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		}

		// Render
		const VkDeviceSize	vertexDataPerDraw	= 4 * 2 * sizeof(Vec4);
		VkDeviceSize		vertexBufferOffset	= 0ull;
		for (deUint32 subpassNdx = 0; subpassNdx < m_caseDef.numLayers; ++subpassNdx)
		{
			if (subpassNdx != 0)
				m_vk.cmdNextSubpass(*m_cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);

			m_vk.cmdBindPipeline(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, **m_uDraw.pipelines[subpassNdx]);

			m_vk.cmdBindVertexBuffers(*m_cmdBuffer, 0u, 1u, &m_uDraw.vertexBuffer.get(), &vertexBufferOffset);
			m_vk.cmdDraw(*m_cmdBuffer, 4u, 1u, 0u, 0u);
			vertexBufferOffset	+= vertexDataPerDraw;
		}

		m_vk.cmdEndRenderPass(*m_cmdBuffer);
	}

	m_imageLayoutAfterUpload	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	m_imageUploadAccessMask		= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
}

void UploadDownloadExecutor::downloadCopy(Context& context, VkBuffer buffer)
{
	(void) context;

	copyImageToBuffer(*m_image, buffer, m_caseDef.size, m_imageUploadAccessMask, m_imageLayoutAfterUpload, m_caseDef.numLayers);
}

void UploadDownloadExecutor::downloadTexture(Context& context, VkBuffer buffer)
{
	// Create output image with download result
	const VkImageUsageFlags	usageFlags	= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	m_dTex.outImage						= makeImage(m_vk, m_device, 0u, VK_IMAGE_TYPE_2D, m_caseDef.viewFormat, m_caseDef.viewFormat, false, m_caseDef.size, 1u, m_caseDef.numLayers, usageFlags);
	m_dTex.outImageAlloc				= bindImage(m_vk, m_device, m_allocator, *m_dTex.outImage, MemoryRequirement::Any);
	m_dTex.outImageView					= makeImageView(m_vk, m_device, *m_dTex.outImage, getImageViewType(m_caseDef.imageType), m_caseDef.viewFormat, makeColorSubresourceRange(0, m_caseDef.numLayers));

	const vk::VkImageViewUsageCreateInfoKHR viewUsageCreateInfo = {
		VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO_KHR,	// VkStructureType		sType
		DE_NULL,											// const void*			pNext
		VK_IMAGE_USAGE_SAMPLED_BIT,							// VkImageUsageFlags	usage;
	};
	m_dTex.inImageView					= makeImageView(m_vk, m_device, *m_image, getImageViewType(m_caseDef.imageType), m_caseDef.viewFormat,
														makeColorSubresourceRange(0, m_caseDef.numLayers), m_haveMaintenance2 ? &viewUsageCreateInfo : DE_NULL);
	m_dTex.sampler						= makeSampler(m_vk, m_device);

	// Setup compute pipeline
	m_dTex.descriptorPool				= DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		.build(m_vk, m_device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);

	m_dTex.descriptorSetLayout			= DescriptorSetLayoutBuilder()
		.addSingleSamplerBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, &m_dTex.sampler.get())
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(m_vk, m_device);

	m_dTex.pipelineLayout				= makePipelineLayout(m_vk, m_device, *m_dTex.descriptorSetLayout);
	m_dTex.descriptorSet				= makeDescriptorSet(m_vk, m_device, *m_dTex.descriptorPool, *m_dTex.descriptorSetLayout);
	m_dTex.inImageDescriptorInfo		= makeDescriptorImageInfo(DE_NULL, *m_dTex.inImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	m_dTex.outImageDescriptorInfo		= makeDescriptorImageInfo(DE_NULL, *m_dTex.outImageView, VK_IMAGE_LAYOUT_GENERAL);
	m_dTex.computeModule				= createShaderModule(m_vk, m_device, context.getBinaryCollection().get("downloadTextureComp"), 0);
	m_dTex.computePipeline				= makeComputePipeline(m_vk, m_device, *m_dTex.pipelineLayout, *m_dTex.computeModule, DE_NULL);

	DescriptorSetUpdateBuilder()
		.writeSingle(*m_dTex.descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &m_dTex.inImageDescriptorInfo)
		.writeSingle(*m_dTex.descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &m_dTex.outImageDescriptorInfo)
		.update(m_vk, m_device);

	// Transition images for shader access (texture / imageStore)
	const VkImageMemoryBarrier imageBarriers[]	=
	{
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,				// VkStructureType			sType;
			DE_NULL,											// const void*				pNext;
			(VkAccessFlags)m_imageUploadAccessMask,				// VkAccessFlags			srcAccessMask;
			(VkAccessFlags)VK_ACCESS_SHADER_READ_BIT,			// VkAccessFlags			dstAccessMask;
			m_imageLayoutAfterUpload,							// VkImageLayout			oldLayout;
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,			// VkImageLayout			newLayout;
			VK_QUEUE_FAMILY_IGNORED,							// deUint32					srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,							// deUint32					destQueueFamilyIndex;
			*m_image,											// VkImage					image;
			makeColorSubresourceRange(0, m_caseDef.numLayers),	// VkImageSubresourceRange	subresourceRange;
		},
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,				// VkStructureType			sType;
			DE_NULL,											// const void*				pNext;
			(VkAccessFlags)0,									// VkAccessFlags			srcAccessMask;
			(VkAccessFlags)VK_ACCESS_SHADER_WRITE_BIT,			// VkAccessFlags			dstAccessMask;
			VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout			oldLayout;
			VK_IMAGE_LAYOUT_GENERAL,							// VkImageLayout			newLayout;
			VK_QUEUE_FAMILY_IGNORED,							// deUint32					srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,							// deUint32					destQueueFamilyIndex;
			*m_dTex.outImage,									// VkImage					image;
			makeColorSubresourceRange(0, m_caseDef.numLayers),	// VkImageSubresourceRange	subresourceRange;
		}
	};

	m_vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0u,
		0u, DE_NULL, 0u, DE_NULL, 2u, imageBarriers);

	// Dispatch
	m_vk.cmdBindPipeline(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *m_dTex.computePipeline);
	m_vk.cmdBindDescriptorSets(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *m_dTex.pipelineLayout, 0u, 1u, &m_dTex.descriptorSet.get(), 0u, DE_NULL);
	m_vk.cmdDispatch(*m_cmdBuffer, m_caseDef.size.x(), m_caseDef.size.y(), m_caseDef.numLayers);

	// Copy output image to color buffer
	copyImageToBuffer(*m_dTex.outImage, buffer, m_caseDef.size, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, m_caseDef.numLayers);
}

void UploadDownloadExecutor::downloadLoad(Context& context, VkBuffer buffer)
{
	// Create output image with download result
	const VkImageUsageFlags usageFlags	= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	m_dLoad.outImage					= makeImage(m_vk, m_device, 0u, VK_IMAGE_TYPE_2D, m_caseDef.viewFormat, m_caseDef.viewFormat, false, m_caseDef.size, 1u, m_caseDef.numLayers, usageFlags);
	m_dLoad.outImageAlloc				= bindImage(m_vk, m_device, m_allocator, *m_dLoad.outImage, MemoryRequirement::Any);
	m_dLoad.outImageView				= makeImageView(m_vk, m_device, *m_dLoad.outImage, getImageViewType(m_caseDef.imageType), m_caseDef.viewFormat, makeColorSubresourceRange(0, m_caseDef.numLayers));

	const vk::VkImageViewUsageCreateInfoKHR viewUsageCreateInfo = {
		VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO_KHR,	// VkStructureType		sType
		DE_NULL,											// const void*			pNext
		VK_IMAGE_USAGE_STORAGE_BIT,							// VkImageUsageFlags	usage;
	};
	m_dLoad.inImageView					= makeImageView(m_vk, m_device, *m_image, getImageViewType(m_caseDef.imageType), m_caseDef.viewFormat,
														makeColorSubresourceRange(0, m_caseDef.numLayers), m_haveMaintenance2 ? &viewUsageCreateInfo : DE_NULL);

	// Setup compute pipeline
	m_dLoad.descriptorPool				= DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2u)
		.build(m_vk, m_device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);

	m_dLoad.descriptorSetLayout			= DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(m_vk, m_device);

	m_dLoad.pipelineLayout				= makePipelineLayout(m_vk, m_device, *m_dLoad.descriptorSetLayout);
	m_dLoad.descriptorSet				= makeDescriptorSet(m_vk, m_device, *m_dLoad.descriptorPool, *m_dLoad.descriptorSetLayout);
	m_dLoad.inImageDescriptorInfo		= makeDescriptorImageInfo(DE_NULL, *m_dLoad.inImageView, VK_IMAGE_LAYOUT_GENERAL);
	m_dLoad.outImageDescriptorInfo		= makeDescriptorImageInfo(DE_NULL, *m_dLoad.outImageView, VK_IMAGE_LAYOUT_GENERAL);
	m_dLoad.computeModule				= createShaderModule(m_vk, m_device, context.getBinaryCollection().get("downloadLoadComp"), 0);
	m_dLoad.computePipeline				= makeComputePipeline(m_vk, m_device, *m_dLoad.pipelineLayout, *m_dLoad.computeModule, DE_NULL);

	DescriptorSetUpdateBuilder()
		.writeSingle(*m_dLoad.descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &m_dLoad.inImageDescriptorInfo)
		.writeSingle(*m_dLoad.descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &m_dLoad.outImageDescriptorInfo)
		.update(m_vk, m_device);

	// Transition storage images for shader access (imageLoad/Store)
	VkImageLayout requiredImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	const VkImageMemoryBarrier imageBarriers[]	=
	{
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,				// VkStructureType			sType;
			DE_NULL,											// const void*				pNext;
			(VkAccessFlags)m_imageUploadAccessMask,				// VkAccessFlags			srcAccessMask;
			(VkAccessFlags)VK_ACCESS_SHADER_READ_BIT,			// VkAccessFlags			dstAccessMask;
			m_imageLayoutAfterUpload,							// VkImageLayout			oldLayout;
			requiredImageLayout,								// VkImageLayout			newLayout;
			VK_QUEUE_FAMILY_IGNORED,							// deUint32					srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,							// deUint32					destQueueFamilyIndex;
			*m_image,											// VkImage					image;
			makeColorSubresourceRange(0, m_caseDef.numLayers),	// VkImageSubresourceRange	subresourceRange;
		},
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,				// VkStructureType			sType;
			DE_NULL,											// const void*				pNext;
			(VkAccessFlags)0,									// VkAccessFlags			srcAccessMask;
			(VkAccessFlags)VK_ACCESS_SHADER_WRITE_BIT,			// VkAccessFlags			dstAccessMask;
			VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout			oldLayout;
			requiredImageLayout,								// VkImageLayout			newLayout;
			VK_QUEUE_FAMILY_IGNORED,							// deUint32					srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,							// deUint32					destQueueFamilyIndex;
			*m_dLoad.outImage,									// VkImage					image;
			makeColorSubresourceRange(0, m_caseDef.numLayers),	// VkImageSubresourceRange	subresourceRange;
		}
	};

	m_vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0u,
		0u, DE_NULL, 0u, DE_NULL, 2u, imageBarriers);

	// Dispatch
	m_vk.cmdBindPipeline(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *m_dLoad.computePipeline);
	m_vk.cmdBindDescriptorSets(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *m_dLoad.pipelineLayout, 0u, 1u, &m_dLoad.descriptorSet.get(), 0u, DE_NULL);
	m_vk.cmdDispatch(*m_cmdBuffer, m_caseDef.size.x(), m_caseDef.size.y(), m_caseDef.numLayers);

	// Copy output image to color buffer
	copyImageToBuffer(*m_dLoad.outImage, buffer, m_caseDef.size, VK_ACCESS_SHADER_WRITE_BIT, requiredImageLayout, m_caseDef.numLayers);
}

void UploadDownloadExecutor::copyImageToBuffer(VkImage				sourceImage,
											   VkBuffer				buffer,
											   const IVec3			size,
											   const VkAccessFlags	srcAccessMask,
											   const VkImageLayout	oldLayout,
											   const deUint32		numLayers)
{
	// Copy result to host visible buffer for inspection
	const VkImageMemoryBarrier	imageBarrier	=
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		// VkStructureType			sType;
		DE_NULL,									// const void*				pNext;
		srcAccessMask,								// VkAccessFlags			srcAccessMask;
		VK_ACCESS_TRANSFER_READ_BIT,				// VkAccessFlags			dstAccessMask;
		oldLayout,									// VkImageLayout			oldLayout;
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,		// VkImageLayout			newLayout;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32					srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32					destQueueFamilyIndex;
		sourceImage,								// VkImage					image;
		makeColorSubresourceRange(0, numLayers)		// VkImageSubresourceRange	subresourceRange;
	};

	m_vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
		0u, DE_NULL, 0u, DE_NULL, 1u, &imageBarrier);

	const VkImageSubresourceLayers	subresource	=
	{
		VK_IMAGE_ASPECT_COLOR_BIT,					// VkImageAspectFlags    aspectMask;
		0u,											// deUint32              mipLevel;
		0u,											// deUint32              baseArrayLayer;
		numLayers,									// deUint32              layerCount;
	};

	const VkBufferImageCopy			region		=
	{
		0ull,										// VkDeviceSize                bufferOffset;
		0u,											// deUint32                    bufferRowLength;
		0u,											// deUint32                    bufferImageHeight;
		subresource,								// VkImageSubresourceLayers    imageSubresource;
		makeOffset3D(0, 0, 0),						// VkOffset3D                  imageOffset;
		makeExtent3D(size),							// VkExtent3D                  imageExtent;
	};

	m_vk.cmdCopyImageToBuffer(*m_cmdBuffer, sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1u, &region);

	const VkBufferMemoryBarrier	bufferBarrier =
	{
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	// VkStructureType    sType;
		DE_NULL,									// const void*        pNext;
		VK_ACCESS_TRANSFER_WRITE_BIT,				// VkAccessFlags      srcAccessMask;
		VK_ACCESS_HOST_READ_BIT,					// VkAccessFlags      dstAccessMask;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32           srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32           dstQueueFamilyIndex;
		buffer,										// VkBuffer           buffer;
		0ull,										// VkDeviceSize       offset;
		VK_WHOLE_SIZE,								// VkDeviceSize       size;
	};

	m_vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
		0u, DE_NULL, 1u, &bufferBarrier, 0u, DE_NULL);
}

tcu::TestStatus testMutable (Context& context, const CaseDef caseDef)
{
	const DeviceInterface&			vk			= context.getDeviceInterface();
	const InstanceInterface&		vki			= context.getInstanceInterface();
	const VkDevice					device		= context.getDevice();
	const VkPhysicalDevice			physDevice	= context.getPhysicalDevice();
	Allocator&						allocator	= context.getDefaultAllocator();

	// If this is a VK_KHR_image_format_list test, check that the extension is supported
	if (caseDef.isFormatListTest && !de::contains(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), "VK_KHR_image_format_list"))
		TCU_THROW(NotSupportedError, "VK_KHR_image_format_list not supported");

	// Check required features on the format for the required upload/download methods
	VkFormatProperties	imageFormatProps, viewFormatProps;
	vki.getPhysicalDeviceFormatProperties(physDevice, caseDef.imageFormat, &imageFormatProps);
	vki.getPhysicalDeviceFormatProperties(physDevice, caseDef.viewFormat, &viewFormatProps);

	VkFormatFeatureFlags	viewFormatFeatureFlags = 0u;
	switch (caseDef.upload)
	{
	case UPLOAD_DRAW:
		viewFormatFeatureFlags |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
		break;
	case UPLOAD_STORE:
		viewFormatFeatureFlags |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
		break;
	default:
		DE_ASSERT("Invalid upload method");
		break;
	}
	switch (caseDef.download)
	{
	case DOWNLOAD_TEXTURE:
		viewFormatFeatureFlags |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
		// For the texture case we write the samples read to a separate output image with the same view format
		// so we need to check that we can also use the view format for storage
		viewFormatFeatureFlags |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
		break;
	case DOWNLOAD_LOAD:
		viewFormatFeatureFlags |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
		break;
	default:
		DE_ASSERT("Invalid download method");
		break;
	}

	if ((viewFormatFeatureFlags & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) &&
		isStorageImageExtendedFormat(caseDef.viewFormat) &&
		!getPhysicalDeviceFeatures(vki, physDevice).shaderStorageImageExtendedFormats)
	{
		TCU_THROW(NotSupportedError, "View format requires shaderStorageImageExtended");
	}

	if ((viewFormatProps.optimalTilingFeatures & viewFormatFeatureFlags) != viewFormatFeatureFlags)
		TCU_THROW(NotSupportedError, "View format doesn't support upload/download method");

	// We don't use the base image for anything other than transfer
	// operations so there are no features to check.  However, The Vulkan
	// 1.0 spec does not allow us to create an image view with usage that
	// is not supported by the main format.  With VK_KHR_maintenance2, we
	// can do this via VK_IMAGE_CREATE_EXTENDED_USAGE_BIT_KHR.
	if ((imageFormatProps.optimalTilingFeatures & viewFormatFeatureFlags) != viewFormatFeatureFlags &&
	    !de::contains(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), "VK_KHR_maintenance2"))
	{
		TCU_THROW(NotSupportedError, "Image format doesn't support upload/download method");
	}

	// Create a color buffer for host-inspection of results
	// For the Copy download method, this is the target of the download, for other
	// download methods, pixel data will be copied to this buffer from the download
	// target
	const VkDeviceSize			colorBufferSize		= caseDef.size.x() * caseDef.size.y() * caseDef.size.z() * caseDef.numLayers * tcu::getPixelSize(mapVkFormat(caseDef.imageFormat));
	const Unique<VkBuffer>		colorBuffer			(makeBuffer(vk, device, colorBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT));
	const UniquePtr<Allocation>	colorBufferAlloc	(bindBuffer(vk, device, allocator, *colorBuffer, MemoryRequirement::HostVisible));
	deMemset(colorBufferAlloc->getHostPtr(), 0, static_cast<std::size_t>(colorBufferSize));
	flushMappedMemoryRange(vk, device, colorBufferAlloc->getMemory(), colorBufferAlloc->getOffset(), VK_WHOLE_SIZE);

	// Execute the test
	UploadDownloadExecutor executor(context, caseDef);
	executor.run(context, *colorBuffer);

	// Verify results
	{
		invalidateMappedMemoryRange(vk, device, colorBufferAlloc->getMemory(), colorBufferAlloc->getOffset(), VK_WHOLE_SIZE);

		// For verification purposes, we use the format of the upload to generate the expected image
		const VkFormat						format			= caseDef.upload == UPLOAD_CLEAR || caseDef.upload == UPLOAD_COPY ? caseDef.imageFormat : caseDef.viewFormat;
		const tcu::TextureFormat			tcuFormat		= mapVkFormat(format);
		const bool							isIntegerFormat	= isUintFormat(format) || isIntFormat(format);
		const tcu::ConstPixelBufferAccess	resultImage		(tcuFormat, caseDef.size.x(), caseDef.size.y(), caseDef.numLayers, colorBufferAlloc->getHostPtr());
		tcu::TextureLevel					textureLevel	(tcuFormat, caseDef.size.x(), caseDef.size.y(), caseDef.numLayers);
		const tcu::PixelBufferAccess		expectedImage	= textureLevel.getAccess();
		generateExpectedImage(expectedImage, caseDef);

		bool ok;
		if (isIntegerFormat)
			ok = tcu::intThresholdCompare(context.getTestContext().getLog(), "Image comparison", "", expectedImage, resultImage, tcu::UVec4(1), tcu::COMPARE_LOG_RESULT);
		else
			ok = tcu::floatThresholdCompare(context.getTestContext().getLog(), "Image comparison", "", expectedImage, resultImage, tcu::Vec4(0.01f), tcu::COMPARE_LOG_RESULT);
		return ok ? tcu::TestStatus::pass("Pass") : tcu::TestStatus::fail("Fail");
	}
}

tcu::TestCaseGroup* createImageMutableTests (TestContext& testCtx)
{
	de::MovePtr<TestCaseGroup> testGroup	(new TestCaseGroup(testCtx, "mutable", "Cases with mutable images"));
	for (int textureNdx = 0; textureNdx < DE_LENGTH_OF_ARRAY(s_textures); ++textureNdx)
	{
		const Texture&					texture					= s_textures[textureNdx];
		de::MovePtr<tcu::TestCaseGroup> groupByImageViewType	(new tcu::TestCaseGroup(testCtx, getImageTypeName(texture.type()).c_str(), ""));

		for (int imageFormatNdx = 0; imageFormatNdx < DE_LENGTH_OF_ARRAY(s_formats); ++imageFormatNdx)
		for (int viewFormatNdx = 0; viewFormatNdx < DE_LENGTH_OF_ARRAY(s_formats); ++viewFormatNdx)
		{
			if (imageFormatNdx != viewFormatNdx && formatsAreCompatible(s_formats[imageFormatNdx], s_formats[viewFormatNdx]))
			{
				for (int upload = 0; upload < UPLOAD_LAST; upload++)
				for (int download = 0; download < DOWNLOAD_LAST; download++)
				{
					CaseDef caseDef =
					{
						texture.type(),
						texture.layerSize(),
						static_cast<deUint32>(texture.numLayers()),
						s_formats[imageFormatNdx],
						s_formats[viewFormatNdx],
						static_cast<enum Upload>(upload),
						static_cast<enum Download>(download),
						false
					};

					std::string caseName = getFormatShortString(s_formats[imageFormatNdx]) + "_" + getFormatShortString(s_formats[viewFormatNdx]) +
						"_" + getUploadString(upload) + "_" + getDownloadString(download);
					addFunctionCaseWithPrograms(groupByImageViewType.get(), caseName, "", initPrograms, testMutable, caseDef);

					caseDef.isFormatListTest = true;
					caseName += "_format_list";
					addFunctionCaseWithPrograms(groupByImageViewType.get(), caseName, "", initPrograms, testMutable, caseDef);
				}
			}
		}

		testGroup->addChild(groupByImageViewType.release());
	}

	return testGroup.release();
}

} // image
} // vkt
