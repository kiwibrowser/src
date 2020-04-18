/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 * Copyright (c) 2014 The Android Open Source Project
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
 * \brief Texture test utilities.
 *//*--------------------------------------------------------------------*/

#include "vktTextureTestUtil.hpp"

#include "deFilePath.hpp"
#include "deMath.h"
#include "tcuCompressedTexture.hpp"
#include "tcuImageIO.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuTestLog.hpp"
#include "vkBuilderUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkPrograms.hpp"
#include "vkQueryUtil.hpp"
#include "vkRefUtil.hpp"
#include "vkTypeUtil.hpp"
#include <map>
#include <string>
#include <vector>

using tcu::TestLog;

using namespace vk;
using namespace glu::TextureTestUtil;

namespace vkt
{
namespace texture
{
namespace util
{

deUint32 findQueueFamilyIndexWithCaps (const InstanceInterface& vkInstance, VkPhysicalDevice physicalDevice, VkQueueFlags requiredCaps)
{
	const std::vector<VkQueueFamilyProperties>	queueProps	= getPhysicalDeviceQueueFamilyProperties(vkInstance, physicalDevice);

	for (size_t queueNdx = 0; queueNdx < queueProps.size(); queueNdx++)
	{
		if ((queueProps[queueNdx].queueFlags & requiredCaps) == requiredCaps)
			return (deUint32)queueNdx;
	}

	TCU_THROW(NotSupportedError, "No matching queue found");
}

struct ShaderParameters {
	float		bias;				//!< User-supplied bias.
	float		ref;				//!< Reference value for shadow lookups.
	tcu::Vec2	padding;			//!< Shader uniform padding.
	tcu::Vec4	colorScale;			//!< Scale for texture color values.
	tcu::Vec4	colorBias;			//!< Bias for texture color values.
};

const char* getProgramName(Program program)
{
	switch (program)
	{
		case PROGRAM_2D_FLOAT:			return "2D_FLOAT";
		case PROGRAM_2D_INT:			return "2D_INT";
		case PROGRAM_2D_UINT:			return "2D_UINT";
		case PROGRAM_2D_SHADOW:			return "2D_SHADOW";
		case PROGRAM_2D_FLOAT_BIAS:		return "2D_FLOAT_BIAS";
		case PROGRAM_2D_INT_BIAS:		return "2D_INT_BIAS";
		case PROGRAM_2D_UINT_BIAS:		return "2D_UINT_BIAS";
		case PROGRAM_2D_SHADOW_BIAS:	return "2D_SHADOW_BIAS";
		case PROGRAM_1D_FLOAT:			return "1D_FLOAT";
		case PROGRAM_1D_INT:			return "1D_INT";
		case PROGRAM_1D_UINT:			return "1D_UINT";
		case PROGRAM_1D_SHADOW:			return "1D_SHADOW";
		case PROGRAM_1D_FLOAT_BIAS:		return "1D_FLOAT_BIAS";
		case PROGRAM_1D_INT_BIAS:		return "1D_INT_BIAS";
		case PROGRAM_1D_UINT_BIAS:		return "1D_UINT_BIAS";
		case PROGRAM_1D_SHADOW_BIAS:	return "1D_SHADOW_BIAS";
		case PROGRAM_CUBE_FLOAT:		return "CUBE_FLOAT";
		case PROGRAM_CUBE_INT:			return "CUBE_INT";
		case PROGRAM_CUBE_UINT:			return "CUBE_UINT";
		case PROGRAM_CUBE_SHADOW:		return "CUBE_SHADOW";
		case PROGRAM_CUBE_FLOAT_BIAS:	return "CUBE_FLOAT_BIAS";
		case PROGRAM_CUBE_INT_BIAS:		return "CUBE_INT_BIAS";
		case PROGRAM_CUBE_UINT_BIAS:	return "CUBE_UINT_BIAS";
		case PROGRAM_CUBE_SHADOW_BIAS:	return "CUBE_SHADOW_BIAS";
		case PROGRAM_2D_ARRAY_FLOAT:	return "2D_ARRAY_FLOAT";
		case PROGRAM_2D_ARRAY_INT:		return "2D_ARRAY_INT";
		case PROGRAM_2D_ARRAY_UINT:		return "2D_ARRAY_UINT";
		case PROGRAM_2D_ARRAY_SHADOW:	return "2D_ARRAY_SHADOW";
		case PROGRAM_3D_FLOAT:			return "3D_FLOAT";
		case PROGRAM_3D_INT:			return "3D_INT";
		case PROGRAM_3D_UINT:			return "3D_UINT";
		case PROGRAM_3D_FLOAT_BIAS:		return "3D_FLOAT_BIAS";
		case PROGRAM_3D_INT_BIAS:		return "3D_INT_BIAS";
		case PROGRAM_3D_UINT_BIAS:		return "3D_UINT_BIAS";
		case PROGRAM_CUBE_ARRAY_FLOAT:	return "CUBE_ARRAY_FLOAT";
		case PROGRAM_CUBE_ARRAY_INT:	return "CUBE_ARRAY_INT";
		case PROGRAM_CUBE_ARRAY_UINT:	return "CUBE_ARRAY_UINT";
		case PROGRAM_CUBE_ARRAY_SHADOW:	return "CUBE_ARRAY_SHADOW";
		case PROGRAM_1D_ARRAY_FLOAT:	return "1D_ARRAY_FLOAT";
		case PROGRAM_1D_ARRAY_INT:		return "1D_ARRAY_INT";
		case PROGRAM_1D_ARRAY_UINT:		return "1D_ARRAY_UINT";
		case PROGRAM_1D_ARRAY_SHADOW:	return "1D_ARRAY_SHADOW";
		case PROGRAM_BUFFER_FLOAT:		return "BUFFER_FLOAT";
		case PROGRAM_BUFFER_INT:		return "BUFFER_INT";
		case PROGRAM_BUFFER_UINT:		return "BUFFER_UINT";
		default:
			DE_ASSERT(false);
	}
	return NULL;
}

VkImageViewType textureTypeToImageViewType (TextureBinding::Type type)
{
	switch (type)
	{
		case TextureBinding::TYPE_2D:			return VK_IMAGE_VIEW_TYPE_2D;
		case TextureBinding::TYPE_2D_ARRAY:		return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		case TextureBinding::TYPE_CUBE_MAP:		return VK_IMAGE_VIEW_TYPE_CUBE;
		case TextureBinding::TYPE_3D:			return VK_IMAGE_VIEW_TYPE_3D;
		default:
			DE_ASSERT(false);
	}

	return VK_IMAGE_VIEW_TYPE_2D;
}

VkImageType imageViewTypeToImageType (VkImageViewType type)
{
	switch (type)
	{
		case VK_IMAGE_VIEW_TYPE_2D:
		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
		case VK_IMAGE_VIEW_TYPE_CUBE:			return VK_IMAGE_TYPE_2D;
		case VK_IMAGE_VIEW_TYPE_3D:				return VK_IMAGE_TYPE_3D;
		default:
			DE_ASSERT(false);
	}

	return VK_IMAGE_TYPE_2D;
}

void initializePrograms(vk::SourceCollections& programCollection, glu::Precision texCoordPrecision, const std::vector<Program>& programs)
{
	static const char* vertShaderTemplate =
		"${VTX_HEADER}"
		"layout(location = 0) ${VTX_IN} highp vec4 a_position;\n"
		"layout(location = 1) ${VTX_IN} ${PRECISION} ${TEXCOORD_TYPE} a_texCoord;\n"
		"layout(location = 0) ${VTX_OUT} ${PRECISION} ${TEXCOORD_TYPE} v_texCoord;\n"
		"${VTX_OUT} gl_PerVertex { vec4 gl_Position; };\n"
		"\n"
		"void main (void)\n"
		"{\n"
		"	gl_Position = a_position;\n"
		"	v_texCoord = a_texCoord;\n"
		"}\n";

	static const char* fragShaderTemplate =
		"${FRAG_HEADER}"
		"layout(location = 0) ${FRAG_IN} ${PRECISION} ${TEXCOORD_TYPE} v_texCoord;\n"
		"layout(location = 0) out mediump vec4 ${FRAG_COLOR};\n"
		"layout (set=0, binding=0, std140) uniform Block \n"
		"{\n"
		"  ${PRECISION} float u_bias;\n"
		"  ${PRECISION} float u_ref;\n"
		"  ${PRECISION} vec4 u_colorScale;\n"
		"  ${PRECISION} vec4 u_colorBias;\n"
		"};\n\n"
		"layout (set=1, binding=0) uniform ${PRECISION} ${SAMPLER_TYPE} u_sampler;\n"
		"void main (void)\n"
		"{\n"
		"	${FRAG_COLOR} = ${LOOKUP} * u_colorScale + u_colorBias;\n"
		"}\n";

	tcu::StringTemplate					vertexSource	(vertShaderTemplate);
	tcu::StringTemplate					fragmentSource	(fragShaderTemplate);

	for (std::vector<Program>::const_iterator programIt = programs.begin(); programIt != programs.end(); ++programIt)
	{
		Program								program	= *programIt;
		std::map<std::string, std::string>	params;

		bool	isCube		= de::inRange<int>(program, PROGRAM_CUBE_FLOAT, PROGRAM_CUBE_SHADOW_BIAS);
		bool	isArray		= de::inRange<int>(program, PROGRAM_2D_ARRAY_FLOAT, PROGRAM_2D_ARRAY_SHADOW)
								|| de::inRange<int>(program, PROGRAM_1D_ARRAY_FLOAT, PROGRAM_1D_ARRAY_SHADOW);

		bool	is1D		= de::inRange<int>(program, PROGRAM_1D_FLOAT, PROGRAM_1D_SHADOW_BIAS)
								|| de::inRange<int>(program, PROGRAM_1D_ARRAY_FLOAT, PROGRAM_1D_ARRAY_SHADOW)
								|| de::inRange<int>(program, PROGRAM_BUFFER_FLOAT, PROGRAM_BUFFER_UINT);

		bool	is2D		= de::inRange<int>(program, PROGRAM_2D_FLOAT, PROGRAM_2D_SHADOW_BIAS)
								|| de::inRange<int>(program, PROGRAM_2D_ARRAY_FLOAT, PROGRAM_2D_ARRAY_SHADOW);

		bool	is3D		= de::inRange<int>(program, PROGRAM_3D_FLOAT, PROGRAM_3D_UINT_BIAS);
		bool	isCubeArray	= de::inRange<int>(program, PROGRAM_CUBE_ARRAY_FLOAT, PROGRAM_CUBE_ARRAY_SHADOW);

		const std::string	version	= glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450);

		params["FRAG_HEADER"]	= version + "\n";
		params["VTX_HEADER"]	= version + "\n";
		params["VTX_IN"]		= "in";
		params["VTX_OUT"]		= "out";
		params["FRAG_IN"]		= "in";
		params["FRAG_COLOR"]	= "dEQP_FragColor";

		params["PRECISION"]		= glu::getPrecisionName(texCoordPrecision);

		if (isCubeArray)
			params["TEXCOORD_TYPE"]	= "vec4";
		else if (isCube || (is2D && isArray) || is3D)
			params["TEXCOORD_TYPE"]	= "vec3";
		else if ((is1D && isArray) || is2D)
			params["TEXCOORD_TYPE"]	= "vec2";
		else if (is1D)
			params["TEXCOORD_TYPE"]	= "float";
		else
			DE_ASSERT(DE_FALSE);

		const char*	sampler	= DE_NULL;
		const char*	lookup	= DE_NULL;

		switch (program)
		{
			case PROGRAM_2D_FLOAT:			sampler = "sampler2D";				lookup = "texture(u_sampler, v_texCoord)";												break;
			case PROGRAM_2D_INT:			sampler = "isampler2D";				lookup = "vec4(texture(u_sampler, v_texCoord))";										break;
			case PROGRAM_2D_UINT:			sampler = "usampler2D";				lookup = "vec4(texture(u_sampler, v_texCoord))";										break;
			case PROGRAM_2D_SHADOW:			sampler = "sampler2DShadow";		lookup = "vec4(texture(u_sampler, vec3(v_texCoord, u_ref)), 0.0, 0.0, 1.0)";			break;
			case PROGRAM_2D_FLOAT_BIAS:		sampler = "sampler2D";				lookup = "texture(u_sampler, v_texCoord, u_bias)";										break;
			case PROGRAM_2D_INT_BIAS:		sampler = "isampler2D";				lookup = "vec4(texture(u_sampler, v_texCoord, u_bias))";								break;
			case PROGRAM_2D_UINT_BIAS:		sampler = "usampler2D";				lookup = "vec4(texture(u_sampler, v_texCoord, u_bias))";								break;
			case PROGRAM_2D_SHADOW_BIAS:	sampler = "sampler2DShadow";		lookup = "vec4(texture(u_sampler, vec3(v_texCoord, u_ref), u_bias), 0.0, 0.0, 1.0)";	break;
			case PROGRAM_1D_FLOAT:			sampler = "sampler1D";				lookup = "texture(u_sampler, v_texCoord)";												break;
			case PROGRAM_1D_INT:			sampler = "isampler1D";				lookup = "vec4(texture(u_sampler, v_texCoord))";										break;
			case PROGRAM_1D_UINT:			sampler = "usampler1D";				lookup = "vec4(texture(u_sampler, v_texCoord))";										break;
			case PROGRAM_1D_SHADOW:			sampler = "sampler1DShadow";		lookup = "vec4(texture(u_sampler, vec3(v_texCoord, 0.0, u_ref)), 0.0, 0.0, 1.0)";		break;
			case PROGRAM_1D_FLOAT_BIAS:		sampler = "sampler1D";				lookup = "texture(u_sampler, v_texCoord, u_bias)";										break;
			case PROGRAM_1D_INT_BIAS:		sampler = "isampler1D";				lookup = "vec4(texture(u_sampler, v_texCoord, u_bias))";								break;
			case PROGRAM_1D_UINT_BIAS:		sampler = "usampler1D";				lookup = "vec4(texture(u_sampler, v_texCoord, u_bias))";								break;
			case PROGRAM_1D_SHADOW_BIAS:	sampler = "sampler1DShadow";		lookup = "vec4(texture(u_sampler, vec3(v_texCoord, 0.0, u_ref), u_bias), 0.0, 0.0, 1.0)";	break;
			case PROGRAM_CUBE_FLOAT:		sampler = "samplerCube";			lookup = "texture(u_sampler, v_texCoord)";												break;
			case PROGRAM_CUBE_INT:			sampler = "isamplerCube";			lookup = "vec4(texture(u_sampler, v_texCoord))";										break;
			case PROGRAM_CUBE_UINT:			sampler = "usamplerCube";			lookup = "vec4(texture(u_sampler, v_texCoord))";										break;
			case PROGRAM_CUBE_SHADOW:		sampler = "samplerCubeShadow";		lookup = "vec4(texture(u_sampler, vec4(v_texCoord, u_ref)), 0.0, 0.0, 1.0)";			break;
			case PROGRAM_CUBE_FLOAT_BIAS:	sampler = "samplerCube";			lookup = "texture(u_sampler, v_texCoord, u_bias)";										break;
			case PROGRAM_CUBE_INT_BIAS:		sampler = "isamplerCube";			lookup = "vec4(texture(u_sampler, v_texCoord, u_bias))";								break;
			case PROGRAM_CUBE_UINT_BIAS:	sampler = "usamplerCube";			lookup = "vec4(texture(u_sampler, v_texCoord, u_bias))";								break;
			case PROGRAM_CUBE_SHADOW_BIAS:	sampler = "samplerCubeShadow";		lookup = "vec4(texture(u_sampler, vec4(v_texCoord, u_ref), u_bias), 0.0, 0.0, 1.0)";	break;
			case PROGRAM_2D_ARRAY_FLOAT:	sampler = "sampler2DArray";			lookup = "texture(u_sampler, v_texCoord)";												break;
			case PROGRAM_2D_ARRAY_INT:		sampler = "isampler2DArray";		lookup = "vec4(texture(u_sampler, v_texCoord))";										break;
			case PROGRAM_2D_ARRAY_UINT:		sampler = "usampler2DArray";		lookup = "vec4(texture(u_sampler, v_texCoord))";										break;
			case PROGRAM_2D_ARRAY_SHADOW:	sampler = "sampler2DArrayShadow";	lookup = "vec4(texture(u_sampler, vec4(v_texCoord, u_ref)), 0.0, 0.0, 1.0)";			break;
			case PROGRAM_3D_FLOAT:			sampler = "sampler3D";				lookup = "texture(u_sampler, v_texCoord)";												break;
			case PROGRAM_3D_INT:			sampler = "isampler3D";				lookup = "vec4(texture(u_sampler, v_texCoord))";										break;
			case PROGRAM_3D_UINT:			sampler = "usampler3D";				lookup = "vec4(texture(u_sampler, v_texCoord))";										break;
			case PROGRAM_3D_FLOAT_BIAS:		sampler = "sampler3D";				lookup = "texture(u_sampler, v_texCoord, u_bias)";										break;
			case PROGRAM_3D_INT_BIAS:		sampler = "isampler3D";				lookup = "vec4(texture(u_sampler, v_texCoord, u_bias))";								break;
			case PROGRAM_3D_UINT_BIAS:		sampler = "usampler3D";				lookup = "vec4(texture(u_sampler, v_texCoord, u_bias))";								break;
			case PROGRAM_CUBE_ARRAY_FLOAT:	sampler = "samplerCubeArray";		lookup = "texture(u_sampler, v_texCoord)";												break;
			case PROGRAM_CUBE_ARRAY_INT:	sampler = "isamplerCubeArray";		lookup = "vec4(texture(u_sampler, v_texCoord))";										break;
			case PROGRAM_CUBE_ARRAY_UINT:	sampler = "usamplerCubeArray";		lookup = "vec4(texture(u_sampler, v_texCoord))";										break;
			case PROGRAM_CUBE_ARRAY_SHADOW:	sampler = "samplerCubeArrayShadow";	lookup = "vec4(texture(u_sampler, v_texCoord, u_ref), 0.0, 0.0, 1.0)";			break;
			case PROGRAM_1D_ARRAY_FLOAT:	sampler = "sampler1DArray";			lookup = "texture(u_sampler, v_texCoord)";												break;
			case PROGRAM_1D_ARRAY_INT:		sampler = "isampler1DArray";		lookup = "vec4(texture(u_sampler, v_texCoord))";										break;
			case PROGRAM_1D_ARRAY_UINT:		sampler = "usampler1DArray";		lookup = "vec4(texture(u_sampler, v_texCoord))";										break;
			case PROGRAM_1D_ARRAY_SHADOW:	sampler = "sampler1DArrayShadow";	lookup = "vec4(texture(u_sampler, vec3(v_texCoord, u_ref)), 0.0, 0.0, 1.0)";			break;
			case PROGRAM_BUFFER_FLOAT:		sampler = "samplerBuffer";			lookup = "texelFetch(u_sampler, int(v_texCoord))";										break;
			case PROGRAM_BUFFER_INT:		sampler = "isamplerBuffer";			lookup = "vec4(texelFetch(u_sampler, int(v_texCoord)))";								break;
			case PROGRAM_BUFFER_UINT:		sampler = "usamplerBuffer";			lookup = "vec4(texelFetch(u_sampler, int(v_texCoord)))";								break;
			default:
				DE_ASSERT(false);
		}

		params["SAMPLER_TYPE"]	= sampler;
		params["LOOKUP"]		= lookup;

		programCollection.glslSources.add("vertext_" + std::string(getProgramName(program))) << glu::VertexSource(vertexSource.specialize(params));
		programCollection.glslSources.add("fragment_" + std::string(getProgramName(program))) << glu::FragmentSource(fragmentSource.specialize(params));
	}
}

TextureBinding::TextureBinding (Context& context)
	: m_context				(context)
{
}

TextureBinding::TextureBinding (Context& context, const TestTextureSp& textureData, const TextureBinding::Type type, const TextureBinding::ImageBackingMode backingMode)
	: m_context				(context)
	, m_type				(type)
	, m_backingMode			(backingMode)
	, m_textureData			(textureData)
{
	updateTextureData(m_textureData, m_type);
}

void TextureBinding::updateTextureData (const TestTextureSp& textureData, const TextureBinding::Type textureType)
{
	const DeviceInterface&						vkd						= m_context.getDeviceInterface();
	const VkDevice								vkDevice				= m_context.getDevice();
	const bool									sparse					= m_backingMode == IMAGE_BACKING_MODE_SPARSE;
	const deUint32								queueFamilyIndices[]	= {m_context.getUniversalQueueFamilyIndex(), m_context.getSparseQueueFamilyIndex()};
	Allocator&									allocator				= m_context.getDefaultAllocator();
	m_type			= textureType;
	m_textureData	= textureData;

	const bool									isCube					= m_type == TYPE_CUBE_MAP;
	VkImageCreateFlags							imageCreateFlags		= (isCube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0) | (sparse ? (VK_IMAGE_CREATE_SPARSE_BINDING_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT) : 0);
	const VkImageViewType						imageViewType			= textureTypeToImageViewType(textureType);
	const VkImageType							imageType				= imageViewTypeToImageType(imageViewType);
	const VkImageTiling							imageTiling				= VK_IMAGE_TILING_OPTIMAL;
	const VkImageUsageFlags						imageUsageFlags			= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	const VkFormat								format					= textureData->isCompressed() ? mapCompressedTextureFormat(textureData->getCompressedLevel(0, 0).getFormat()) : mapTextureFormat(textureData->getTextureFormat());
	const tcu::UVec3							textureDimension		= textureData->getTextureDimension();
	const deUint32								mipLevels				= textureData->getNumLevels();
	const deUint32								arraySize				= textureData->getArraySize();
	vk::VkImageFormatProperties					imageFormatProperties;
	const VkResult								imageFormatQueryResult	= m_context.getInstanceInterface().getPhysicalDeviceImageFormatProperties(m_context.getPhysicalDevice(), format, imageType, imageTiling, imageUsageFlags, imageCreateFlags, &imageFormatProperties);
	const VkSharingMode							sharingMode				= (sparse && m_context.getUniversalQueueFamilyIndex() != m_context.getSparseQueueFamilyIndex()) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;

	if (imageFormatQueryResult == VK_ERROR_FORMAT_NOT_SUPPORTED)
	{
		TCU_THROW(NotSupportedError, (std::string("Format not supported: ") + vk::getFormatName(format)).c_str());
	}
	else
		VK_CHECK(imageFormatQueryResult);

	if (imageFormatProperties.maxArrayLayers < arraySize)
		TCU_THROW(NotSupportedError, ("Maximum array layers number for this format is not enough for this test."));

	if (imageFormatProperties.maxMipLevels < mipLevels)
		TCU_THROW(NotSupportedError, ("Maximum mimap level number for this format is not enough for this test."));

	if (imageFormatProperties.maxExtent.width < textureDimension.x() ||
		imageFormatProperties.maxExtent.height < textureDimension.y() ||
		imageFormatProperties.maxExtent.depth < textureDimension.z())
	{
		TCU_THROW(NotSupportedError, ("Maximum image dimension for this format is not enough for this test."));
	}

	// Create image
	const VkImageCreateInfo						imageParams				=
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,							// VkStructureType			sType;
		DE_NULL,														// const void*				pNext;
		imageCreateFlags,												// VkImageCreateFlags		flags;
		imageType,														// VkImageType				imageType;
		format,															// VkFormat					format;
		{																// VkExtent3D				extent;
			(deUint32)textureDimension.x(),
			(deUint32)textureDimension.y(),
			(deUint32)textureDimension.z()
		},
		mipLevels,														// deUint32					mipLevels;
		arraySize,														// deUint32					arrayLayers;
		VK_SAMPLE_COUNT_1_BIT,											// VkSampleCountFlagBits	samples;
		imageTiling,													// VkImageTiling			tiling;
		imageUsageFlags,												// VkImageUsageFlags		usage;
		sharingMode,													// VkSharingMode			sharingMode;
		sharingMode == VK_SHARING_MODE_CONCURRENT ? 2u : 1u,			// deUint32					queueFamilyIndexCount;
		queueFamilyIndices,												// const deUint32*			pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED										// VkImageLayout			initialLayout;
	};

	m_textureImage = createImage(vkd, vkDevice, &imageParams);

	if (sparse)
	{
		pipeline::uploadTestTextureSparse	(vkd,
											 vkDevice,
											 m_context.getPhysicalDevice(),
											 m_context.getInstanceInterface(),
											 imageParams,
											 m_context.getUniversalQueue(),
											 m_context.getUniversalQueueFamilyIndex(),
											 m_context.getSparseQueue(),
											 allocator,
											 m_allocations,
											 *m_textureData,
											 *m_textureImage);
	}
	else
	{
		m_textureImageMemory = allocator.allocate(getImageMemoryRequirements(vkd, vkDevice, *m_textureImage), MemoryRequirement::Any);
		VK_CHECK(vkd.bindImageMemory(vkDevice, *m_textureImage, m_textureImageMemory->getMemory(), m_textureImageMemory->getOffset()));

		pipeline::uploadTestTexture	(vkd,
									 vkDevice,
									 m_context.getUniversalQueue(),
									 m_context.getUniversalQueueFamilyIndex(),
									 allocator,
									 *m_textureData,
									 *m_textureImage);
	}

	updateTextureViewMipLevels(0, mipLevels - 1);
}

void TextureBinding::updateTextureViewMipLevels (deUint32 baseLevel, deUint32 maxLevel)
{
	const DeviceInterface&						vkd						= m_context.getDeviceInterface();
	const VkDevice								vkDevice				= m_context.getDevice();
	const vk::VkImageViewType					imageViewType			= textureTypeToImageViewType(m_type);
	const vk::VkFormat							format					= m_textureData->isCompressed() ? mapCompressedTextureFormat(m_textureData->getCompressedLevel(0, 0).getFormat()) : mapTextureFormat(m_textureData->getTextureFormat());
	const bool									isShadowTexture			= tcu::hasDepthComponent(m_textureData->getTextureFormat().order);
	const VkImageAspectFlags					aspectMask				= isShadowTexture ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	const deUint32								layerCount				= m_textureData->getArraySize();
	const vk::VkImageViewCreateInfo				viewParams				=
	{
		vk::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,	// VkStructureType			sType;
		NULL,											// const voide*				pNext;
		0u,												// VkImageViewCreateFlags	flags;
		*m_textureImage,								// VkImage					image;
		imageViewType,									// VkImageViewType			viewType;
		format,											// VkFormat					format;
		makeComponentMappingRGBA(),						// VkComponentMapping		components;
		{
			aspectMask,									// VkImageAspectFlags	aspectMask;
			baseLevel,									// deUint32				baseMipLevel;
			maxLevel-baseLevel+1,						// deUint32				levelCount;
			0,											// deUint32				baseArrayLayer;
			layerCount									// deUint32				layerCount;
		},												// VkImageSubresourceRange	subresourceRange;
	};

	m_textureImageView		= createImageView(vkd, vkDevice, &viewParams);
}

const deUint16		TextureRenderer::s_vertexIndices[6] = { 0, 1, 2, 2, 1, 3 };
const VkDeviceSize	TextureRenderer::s_vertexIndexBufferSize = sizeof(TextureRenderer::s_vertexIndices);

TextureRenderer::TextureRenderer (Context& context, VkSampleCountFlagBits sampleCount, deUint32 renderWidth, deUint32 renderHeight)
	: m_context					(context)
	, m_log						(context.getTestContext().getLog())
	, m_renderWidth				(renderWidth)
	, m_renderHeight			(renderHeight)
	, m_sampleCount				(sampleCount)
	, m_multisampling			(m_sampleCount != VK_SAMPLE_COUNT_1_BIT)
	, m_imageFormat				(VK_FORMAT_R8G8B8A8_UNORM)
	, m_textureFormat			(vk::mapVkFormat(m_imageFormat))
	, m_uniformBufferSize		(sizeof(ShaderParameters))
	, m_resultBufferSize		(renderWidth * renderHeight * m_textureFormat.getPixelSize())
	, m_viewportOffsetX			(0.0f)
	, m_viewportOffsetY			(0.0f)
	, m_viewportWidth			((float)renderWidth)
	, m_viewportHeight			((float)renderHeight)
{
	const DeviceInterface&						vkd						= m_context.getDeviceInterface();
	const VkDevice								vkDevice				= m_context.getDevice();
	const deUint32								queueFamilyIndex		= m_context.getUniversalQueueFamilyIndex();
	Allocator&									allocator				= m_context.getDefaultAllocator();

	// Command Pool
	m_commandPool = createCommandPool(vkd, vkDevice, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex);

	// Image
	{
		const VkImageUsageFlags	imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		VkImageFormatProperties	properties;

		if ((m_context.getInstanceInterface().getPhysicalDeviceImageFormatProperties(m_context.getPhysicalDevice(),
																					 m_imageFormat,
																					 VK_IMAGE_TYPE_2D,
																					 VK_IMAGE_TILING_OPTIMAL,
																					 imageUsage,
																					 0,
																					 &properties) == VK_ERROR_FORMAT_NOT_SUPPORTED))
		{
			TCU_THROW(NotSupportedError, "Format not supported");
		}

		if ((properties.sampleCounts & m_sampleCount) != m_sampleCount)
		{
			TCU_THROW(NotSupportedError, "Format not supported");
		}

		const VkImageCreateInfo					imageCreateInfo			=
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,		// VkStructureType			sType;
			DE_NULL,									// const void*				pNext;
			0u,											// VkImageCreateFlags		flags;
			VK_IMAGE_TYPE_2D,							// VkImageType				imageType;
			m_imageFormat,								// VkFormat					format;
			{ m_renderWidth, m_renderHeight, 1u },		// VkExtent3D				extent;
			1u,											// deUint32					mipLevels;
			1u,											// deUint32					arrayLayers;
			m_sampleCount,								// VkSampleCountFlagBits	samples;
			VK_IMAGE_TILING_OPTIMAL,					// VkImageTiling			tiling;
			imageUsage,									// VkImageUsageFlags		usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode			sharingMode;
			1u,											// deUint32					queueFamilyIndexCount;
			&queueFamilyIndex,							// const deUint32*			pQueueFamilyIndices;
			VK_IMAGE_LAYOUT_UNDEFINED					// VkImageLayout			initialLayout;
		};

		m_image = vk::createImage(vkd, vkDevice, &imageCreateInfo, DE_NULL);

		m_imageMemory	= allocator.allocate(getImageMemoryRequirements(vkd, vkDevice, *m_image), MemoryRequirement::Any);
		VK_CHECK(vkd.bindImageMemory(vkDevice, *m_image, m_imageMemory->getMemory(), m_imageMemory->getOffset()));
	}

	// Image View
	{
		const VkImageViewCreateInfo				imageViewCreateInfo		=
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,	// VkStructureType				sType;
			DE_NULL,									// const void*					pNext;
			0u,											// VkImageViewCreateFlags		flags;
			*m_image,									// VkImage						image;
			VK_IMAGE_VIEW_TYPE_2D,						// VkImageViewType				viewType;
			m_imageFormat,								// VkFormat						format;
			makeComponentMappingRGBA(),					// VkComponentMapping			components;
			{
				VK_IMAGE_ASPECT_COLOR_BIT,					// VkImageAspectFlags			aspectMask;
				0u,											// deUint32						baseMipLevel;
				1u,											// deUint32						mipLevels;
				0u,											// deUint32						baseArrayLayer;
				1u,											// deUint32						arraySize;
			},											// VkImageSubresourceRange		subresourceRange;
		};

		m_imageView = vk::createImageView(vkd, vkDevice, &imageViewCreateInfo, DE_NULL);
	}

	if (m_multisampling)
	{
		{
			// Resolved Image
			const VkImageUsageFlags	imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			VkImageFormatProperties	properties;

			if ((m_context.getInstanceInterface().getPhysicalDeviceImageFormatProperties(m_context.getPhysicalDevice(),
																						 m_imageFormat,
																						 VK_IMAGE_TYPE_2D,
																						 VK_IMAGE_TILING_OPTIMAL,
																						 imageUsage,
																						 0,
																						 &properties) == VK_ERROR_FORMAT_NOT_SUPPORTED))
			{
				TCU_THROW(NotSupportedError, "Format not supported");
			}

			const VkImageCreateInfo					imageCreateInfo			=
			{
				VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,		// VkStructureType			sType;
				DE_NULL,									// const void*				pNext;
				0u,											// VkImageCreateFlags		flags;
				VK_IMAGE_TYPE_2D,							// VkImageType				imageType;
				m_imageFormat,								// VkFormat					format;
				{ m_renderWidth, m_renderHeight, 1u },		// VkExtent3D				extent;
				1u,											// deUint32					mipLevels;
				1u,											// deUint32					arrayLayers;
				VK_SAMPLE_COUNT_1_BIT,						// VkSampleCountFlagBits	samples;
				VK_IMAGE_TILING_OPTIMAL,					// VkImageTiling			tiling;
				imageUsage,									// VkImageUsageFlags		usage;
				VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode			sharingMode;
				1u,											// deUint32					queueFamilyIndexCount;
				&queueFamilyIndex,							// const deUint32*			pQueueFamilyIndices;
				VK_IMAGE_LAYOUT_UNDEFINED					// VkImageLayout			initialLayout;
			};

			m_resolvedImage			= vk::createImage(vkd, vkDevice, &imageCreateInfo, DE_NULL);
			m_resolvedImageMemory	= allocator.allocate(getImageMemoryRequirements(vkd, vkDevice, *m_resolvedImage), MemoryRequirement::Any);
			VK_CHECK(vkd.bindImageMemory(vkDevice, *m_resolvedImage, m_resolvedImageMemory->getMemory(), m_resolvedImageMemory->getOffset()));
		}

		// Resolved Image View
		{
			const VkImageViewCreateInfo				imageViewCreateInfo		=
			{
				VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,	// VkStructureType				sType;
				DE_NULL,									// const void*					pNext;
				0u,											// VkImageViewCreateFlags		flags;
				*m_resolvedImage,							// VkImage						image;
				VK_IMAGE_VIEW_TYPE_2D,						// VkImageViewType				viewType;
				m_imageFormat,								// VkFormat						format;
				makeComponentMappingRGBA(),					// VkComponentMapping			components;
				{
					VK_IMAGE_ASPECT_COLOR_BIT,					// VkImageAspectFlags			aspectMask;
					0u,											// deUint32						baseMipLevel;
					1u,											// deUint32						mipLevels;
					0u,											// deUint32						baseArrayLayer;
					1u,											// deUint32						arraySize;
				},											// VkImageSubresourceRange		subresourceRange;
			};

			m_resolvedImageView = vk::createImageView(vkd, vkDevice, &imageViewCreateInfo, DE_NULL);
		}
	}

	// Render Pass
	{
		const VkImageLayout						imageLayout				= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		const VkAttachmentDescription			attachmentDesc[]		=
		{
			{
				0u,													// VkAttachmentDescriptionFlags		flags;
				m_imageFormat,										// VkFormat							format;
				m_sampleCount,										// VkSampleCountFlagBits			samples;
				VK_ATTACHMENT_LOAD_OP_LOAD,												// VkAttachmentLoadOp				loadOp;
				VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp				storeOp;
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				stencilLoadOp;
				VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp;
				imageLayout,										// VkImageLayout					initialLayout;
				imageLayout,										// VkImageLayout					finalLayout;
			},
			{
				0u,													// VkAttachmentDescriptionFlags		flags;
				m_imageFormat,										// VkFormat							format;
				VK_SAMPLE_COUNT_1_BIT,								// VkSampleCountFlagBits			samples;
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				loadOp;
				VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp				storeOp;
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				stencilLoadOp;
				VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp;
				imageLayout,										// VkImageLayout					initialLayout;
				imageLayout,										// VkImageLayout					finalLayout;
			}
		};

		const VkAttachmentReference				attachmentRef			=
		{
			0u,													// deUint32							attachment;
			imageLayout,										// VkImageLayout					layout;
		};

		const VkAttachmentReference				resolveAttachmentRef	=
		{
			1u,													// deUint32							attachment;
			imageLayout,										// VkImageLayout					layout;
		};

		const VkSubpassDescription				subpassDesc				=
		{
			0u,													// VkSubpassDescriptionFlags		flags;
			VK_PIPELINE_BIND_POINT_GRAPHICS,					// VkPipelineBindPoint				pipelineBindPoint;
			0u,													// deUint32							inputAttachmentCount;
			DE_NULL,											// const VkAttachmentReference*		pInputAttachments;
			1u,													// deUint32							colorAttachmentCount;
			&attachmentRef,										// const VkAttachmentReference*		pColorAttachments;
			m_multisampling ? &resolveAttachmentRef : DE_NULL,	// const VkAttachmentReference*		pResolveAttachments;
			DE_NULL,											// const VkAttachmentReference*		pDepthStencilAttachment;
			0u,													// deUint32							preserveAttachmentCount;
			DE_NULL,											// const VkAttachmentReference*		pPreserveAttachments;
		};

		const VkRenderPassCreateInfo			renderPassCreateInfo	=
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,			// VkStructureType					sType;
			DE_NULL,											// const void*						pNext;
			0u,													// VkRenderPassCreateFlags			flags;
			m_multisampling ? 2u : 1u,							// deUint32							attachmentCount;
			attachmentDesc,										// const VkAttachmentDescription*	pAttachments;
			1u,													// deUint32							subpassCount;
			&subpassDesc,										// const VkSubpassDescription*		pSubpasses;
			0u,													// deUint32							dependencyCount;
			DE_NULL,											// const VkSubpassDependency*		pDependencies;
		};

		m_renderPass =  createRenderPass(vkd, vkDevice, &renderPassCreateInfo, DE_NULL);
	}

	// Vertex index buffer
	{
		const VkBufferCreateInfo			indexBufferParams		=
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			s_vertexIndexBufferSize,					// VkDeviceSize			size;
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,			// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			1u,											// deUint32				queueFamilyCount;
			&queueFamilyIndex							// const deUint32*		pQueueFamilyIndices;
		};

		m_vertexIndexBuffer			= createBuffer(vkd, vkDevice, &indexBufferParams);
		m_vertexIndexBufferMemory	= allocator.allocate(getBufferMemoryRequirements(vkd, vkDevice, *m_vertexIndexBuffer), MemoryRequirement::HostVisible);

		VK_CHECK(vkd.bindBufferMemory(vkDevice, *m_vertexIndexBuffer, m_vertexIndexBufferMemory->getMemory(), m_vertexIndexBufferMemory->getOffset()));

		// Load vertices into vertex buffer
		deMemcpy(m_vertexIndexBufferMemory->getHostPtr(), s_vertexIndices, s_vertexIndexBufferSize);
		flushMappedMemoryRange(vkd, vkDevice, m_vertexIndexBufferMemory->getMemory(), m_vertexIndexBufferMemory->getOffset(), VK_WHOLE_SIZE);
	}

	// FrameBuffer
	{
		const VkImageView						attachments[]			=
		{
			*m_imageView,
			*m_resolvedImageView,
		};

		const VkFramebufferCreateInfo			framebufferCreateInfo	=
		{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,	// VkStructureType			sType;
			DE_NULL,									// const void*				pNext;
			0u,											// VkFramebufferCreateFlags	flags;
			*m_renderPass,								// VkRenderPass				renderPass;
			m_multisampling ? 2u : 1u,					// deUint32					attachmentCount;
			attachments,								// const VkImageView*		pAttachments;
			m_renderWidth,								// deUint32					width;
			m_renderHeight,								// deUint32					height;
			1u,											// deUint32					layers;
		};

		m_frameBuffer = createFramebuffer(vkd, vkDevice, &framebufferCreateInfo, DE_NULL);
	}

	// Uniform Buffer
	{
		const VkBufferCreateInfo				bufferCreateInfo		=
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			m_uniformBufferSize,						// VkDeviceSize			size;
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,			// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			1u,											// deUint32				queueFamilyIndexCount;
			&queueFamilyIndex							// const deUint32*		pQueueFamilyIndices;
		};

		m_uniformBuffer			= createBuffer(vkd, vkDevice, &bufferCreateInfo);
		m_uniformBufferMemory	= allocator.allocate(getBufferMemoryRequirements(vkd, vkDevice, *m_uniformBuffer), MemoryRequirement::HostVisible);

		VK_CHECK(vkd.bindBufferMemory(vkDevice, *m_uniformBuffer, m_uniformBufferMemory->getMemory(), m_uniformBufferMemory->getOffset()));
	}

	// DescriptorPool
	{
		DescriptorPoolBuilder					descriptorPoolBuilder;

		descriptorPoolBuilder.addType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		descriptorPoolBuilder.addType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_descriptorPool = descriptorPoolBuilder.build(vkd, vkDevice, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 2u);
	}

	// Fence
	m_fence = createFence(vkd, vkDevice);

	// Result Buffer
	{
		const VkBufferCreateInfo				bufferCreateInfo		=
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			m_resultBufferSize,							// VkDeviceSize			size;
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,			// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			1u,											// deUint32				queueFamilyIndexCount;
			&queueFamilyIndex							// const deUint32*		pQueueFamilyIndices;
		};

		m_resultBuffer			= createBuffer(vkd, vkDevice, &bufferCreateInfo);
		m_resultBufferMemory	= allocator.allocate(getBufferMemoryRequirements(vkd, vkDevice, *m_resultBuffer), MemoryRequirement::HostVisible);

		VK_CHECK(vkd.bindBufferMemory(vkDevice, *m_resultBuffer, m_resultBufferMemory->getMemory(), m_resultBufferMemory->getOffset()));
	}

	clearImage(*m_image);
	if(m_multisampling)
		clearImage(*m_resolvedImage);
}

TextureRenderer::~TextureRenderer (void)
{
}

void TextureRenderer::clearImage(VkImage image)
{
	const DeviceInterface&			vkd					= m_context.getDeviceInterface();
	const VkDevice					vkDevice			= m_context.getDevice();
	Move<VkCommandBuffer>			commandBuffer;
	const VkQueue					queue				= m_context.getUniversalQueue();

	const VkImageSubresourceRange	subResourcerange	=
	{
		VK_IMAGE_ASPECT_COLOR_BIT,		// VkImageAspectFlags	aspectMask;
		0,								// deUint32				baseMipLevel;
		1,								// deUint32				levelCount;
		0,								// deUint32				baseArrayLayer;
		1								// deUint32				layerCount;
	};

	commandBuffer = allocateCommandBuffer(vkd, vkDevice, *m_commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	const VkCommandBufferBeginInfo		cmdBufferBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType							sType;
		DE_NULL,										// const void*								pNext;
		0u,												// VkCmdBufferOptimizeFlags					flags;
		DE_NULL											// const VkCommandBufferInheritanceInfo*	pInheritanceInfo;
	};

	VK_CHECK(vkd.beginCommandBuffer(*commandBuffer, &cmdBufferBeginInfo));

	addImageTransitionBarrier(*commandBuffer, image,
							  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,				// VkPipelineStageFlags		srcStageMask
							  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,				// VkPipelineStageFlags		dstStageMask
							  0,												// VkAccessFlags			srcAccessMask
							  VK_ACCESS_TRANSFER_WRITE_BIT,						// VkAccessFlags			dstAccessMask
							  VK_IMAGE_LAYOUT_UNDEFINED,						// VkImageLayout			oldLayout;
							  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);			// VkImageLayout			newLayout;

	VkClearColorValue color = makeClearValueColorF32(0.0f, 0.0f, 0.0f, 1.0f).color;
	vkd.cmdClearColorImage(*commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &subResourcerange);

	addImageTransitionBarrier(*commandBuffer, image,
							  VK_PIPELINE_STAGE_TRANSFER_BIT,					// VkPipelineStageFlags		srcStageMask
							  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,				// VkPipelineStageFlags		dstStageMask
							  VK_ACCESS_TRANSFER_WRITE_BIT,						// VkAccessFlags			srcAccessMask
							  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,				// VkAccessFlags			dstAccessMask
							  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,				// VkImageLayout			oldLayout;
							  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);		// VkImageLayout			newLayout;

	VK_CHECK(vkd.endCommandBuffer(*commandBuffer));

	const VkSubmitInfo					submitInfo				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,			// VkStructureType				sType;
		DE_NULL,								// const void*					pNext;
		0u,										// deUint32						waitSemaphoreCount;
		DE_NULL,								// const VkSemaphore*			pWaitSemaphores;
		DE_NULL,								// const VkPipelineStageFlags*	pWaitDstStageMask;
		1u,										// deUint32						commandBufferCount;
		&commandBuffer.get(),					// const VkCommandBuffer*		pCommandBuffers;
		0u,										// deUint32						signalSemaphoreCount;
		DE_NULL,								// const VkSemaphore*			pSignalSemaphores;
	};

	VK_CHECK(vkd.resetFences(vkDevice, 1, &m_fence.get()));
	VK_CHECK(vkd.queueSubmit(queue, 1, &submitInfo, *m_fence));
	VK_CHECK(vkd.waitForFences(vkDevice, 1, &m_fence.get(), true, ~(0ull) /* infinity */));
}

void TextureRenderer::add2DTexture (const TestTexture2DSp& texture, TextureBinding::ImageBackingMode backingMode)
{
	m_textureBindings.push_back(TextureBindingSp(new TextureBinding(m_context, texture, TextureBinding::TYPE_2D, backingMode)));
}

void TextureRenderer::addCubeTexture (const TestTextureCubeSp& texture, TextureBinding::ImageBackingMode backingMode)
{
	m_textureBindings.push_back(TextureBindingSp(new TextureBinding(m_context, texture, TextureBinding::TYPE_CUBE_MAP, backingMode)));
}

void TextureRenderer::add2DArrayTexture (const TestTexture2DArraySp& texture, TextureBinding::ImageBackingMode backingMode)
{
	m_textureBindings.push_back(TextureBindingSp(new TextureBinding(m_context, texture, TextureBinding::TYPE_2D_ARRAY, backingMode)));
}

void TextureRenderer::add3DTexture (const TestTexture3DSp& texture, TextureBinding::ImageBackingMode backingMode)
{
	m_textureBindings.push_back(TextureBindingSp(new TextureBinding(m_context, texture, TextureBinding::TYPE_3D, backingMode)));
}

const pipeline::TestTexture2D& TextureRenderer::get2DTexture (int textureIndex) const
{
	DE_ASSERT(m_textureBindings.size() > (size_t)textureIndex);
	DE_ASSERT(m_textureBindings[textureIndex]->getType() == TextureBinding::TYPE_2D);

	return dynamic_cast<const pipeline::TestTexture2D&>(m_textureBindings[textureIndex]->getTestTexture());
}

const pipeline::TestTextureCube& TextureRenderer::getCubeTexture (int textureIndex) const
{
	DE_ASSERT(m_textureBindings.size() > (size_t)textureIndex);
	DE_ASSERT(m_textureBindings[textureIndex]->getType() == TextureBinding::TYPE_CUBE_MAP);

	return dynamic_cast<const pipeline::TestTextureCube&>(m_textureBindings[textureIndex]->getTestTexture());
}

const pipeline::TestTexture2DArray& TextureRenderer::get2DArrayTexture (int textureIndex) const
{
	DE_ASSERT(m_textureBindings.size() > (size_t)textureIndex);
	DE_ASSERT(m_textureBindings[textureIndex]->getType() == TextureBinding::TYPE_2D_ARRAY);

	return dynamic_cast<const pipeline::TestTexture2DArray&>(m_textureBindings[textureIndex]->getTestTexture());
}

const pipeline::TestTexture3D& TextureRenderer::get3DTexture (int textureIndex) const
{
	DE_ASSERT(m_textureBindings.size() > (size_t)textureIndex);
	DE_ASSERT(m_textureBindings[textureIndex]->getType() == TextureBinding::TYPE_3D);

	return dynamic_cast<const pipeline::TestTexture3D&>(m_textureBindings[textureIndex]->getTestTexture());
}

void TextureRenderer::setViewport (float viewportX, float viewportY, float viewportW, float viewportH)
{
	m_viewportHeight = viewportH;
	m_viewportWidth = viewportW;
	m_viewportOffsetX = viewportX;
	m_viewportOffsetY = viewportY;
}

TextureBinding* TextureRenderer::getTextureBinding (int textureIndex) const
{
	DE_ASSERT(m_textureBindings.size() > (size_t)textureIndex);
	return m_textureBindings[textureIndex].get();
}

deUint32 TextureRenderer::getRenderWidth (void) const
{
	return m_renderWidth;
}

deUint32 TextureRenderer::getRenderHeight (void) const
{
	return m_renderHeight;
}

Move<VkDescriptorSet> TextureRenderer::makeDescriptorSet (const VkDescriptorPool descriptorPool, const VkDescriptorSetLayout setLayout) const
{
	const DeviceInterface&						vkd						= m_context.getDeviceInterface();
	const VkDevice								vkDevice				= m_context.getDevice();

	const VkDescriptorSetAllocateInfo			allocateParams			=
	{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,		// VkStructureType					sType
			DE_NULL,											// const void*						pNext
			descriptorPool,										// VkDescriptorPool					descriptorPool
			1u,													// deUint32							descriptorSetCount
			&setLayout,											// const VkDescriptorSetLayout*		pSetLayouts
	};
	return allocateDescriptorSet(vkd, vkDevice, &allocateParams);
}

void TextureRenderer::addImageTransitionBarrier(VkCommandBuffer commandBuffer, VkImage image, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout) const
{
	const DeviceInterface&			vkd					= m_context.getDeviceInterface();

	const VkImageSubresourceRange	subResourcerange	=
	{
		VK_IMAGE_ASPECT_COLOR_BIT,		// VkImageAspectFlags	aspectMask;
		0,								// deUint32				baseMipLevel;
		1,								// deUint32				levelCount;
		0,								// deUint32				baseArrayLayer;
		1								// deUint32				layerCount;
	};

	const VkImageMemoryBarrier		imageBarrier		=
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		// VkStructureType			sType;
		DE_NULL,									// const void*				pNext;
		srcAccessMask,								// VkAccessFlags			srcAccessMask;
		dstAccessMask,								// VkAccessFlags			dstAccessMask;
		oldLayout,									// VkImageLayout			oldLayout;
		newLayout,									// VkImageLayout			newLayout;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32					srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32					destQueueFamilyIndex;
		image,										// VkImage					image;
		subResourcerange							// VkImageSubresourceRange	subresourceRange;
	};

	vkd.cmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, DE_NULL, 0, DE_NULL, 1, &imageBarrier);
}


void TextureRenderer::renderQuad (tcu::Surface& result, int texUnit, const float* texCoord, TextureType texType)
{
	renderQuad(result, texUnit, texCoord, ReferenceParams(texType));
}

void TextureRenderer::renderQuad (tcu::Surface& result, int texUnit, const float* texCoord, const ReferenceParams& params)
{
	const float	maxAnisotropy = 1.0f;
	float		positions[]	=
	{
		-1.0,	-1.0f,	0.0f,	1.0f,
		-1.0f,	+1.0f,	0.0f,	1.0f,
		+1.0f,	-1.0f,	0.0f,	1.0f,
		+1.0f,	+1.0f,	0.0f,	1.0f
	};
	renderQuad(result, positions, texUnit, texCoord, params, maxAnisotropy);
}

void TextureRenderer::renderQuad (tcu::Surface&									result,
								  const float*									positions,
								  int											texUnit,
								  const float*									texCoord,
								  const glu::TextureTestUtil::ReferenceParams&	params,
								  const float									maxAnisotropy)
{
	const DeviceInterface&		vkd						= m_context.getDeviceInterface();
	const VkDevice				vkDevice				= m_context.getDevice();
	const VkQueue				queue					= m_context.getUniversalQueue();
	const deUint32				queueFamilyIndex		= m_context.getUniversalQueueFamilyIndex();
	Allocator&					allocator				= m_context.getDefaultAllocator();

	tcu::Vec4					wCoord					= params.flags & RenderParams::PROJECTED ? params.w : tcu::Vec4(1.0f);
	bool						useBias					= !!(params.flags & RenderParams::USE_BIAS);
	bool						logUniforms				= !!(params.flags & RenderParams::LOG_UNIFORMS);

	// Render quad with texture.
	float						position[]				=
	{
		positions[0]*wCoord.x(),	positions[1]*wCoord.x(),	positions[2],	positions[3]*wCoord.x(),
		positions[4]*wCoord.y(),	positions[5]*wCoord.y(),	positions[6],	positions[7]*wCoord.y(),
		positions[8]*wCoord.z(),	positions[9]*wCoord.z(),	positions[10],	positions[11]*wCoord.z(),
		positions[12]*wCoord.w(),	positions[13]*wCoord.w(),	positions[14],	positions[15]*wCoord.w()
	};

	Program						progSpec				= PROGRAM_LAST;
	int							numComps				= 0;

	if (params.texType == TEXTURETYPE_2D)
	{
		numComps = 2;

		switch (params.samplerType)
		{
			case SAMPLERTYPE_FLOAT:		progSpec = useBias ? PROGRAM_2D_FLOAT_BIAS	: PROGRAM_2D_FLOAT;		break;
			case SAMPLERTYPE_INT:		progSpec = useBias ? PROGRAM_2D_INT_BIAS	: PROGRAM_2D_INT;		break;
			case SAMPLERTYPE_UINT:		progSpec = useBias ? PROGRAM_2D_UINT_BIAS	: PROGRAM_2D_UINT;		break;
			case SAMPLERTYPE_SHADOW:	progSpec = useBias ? PROGRAM_2D_SHADOW_BIAS	: PROGRAM_2D_SHADOW;	break;
			default:					DE_ASSERT(false);
		}
	}
	else if (params.texType == TEXTURETYPE_1D)
	{
		numComps = 1;

		switch (params.samplerType)
		{
			case SAMPLERTYPE_FLOAT:		progSpec = useBias ? PROGRAM_1D_FLOAT_BIAS	: PROGRAM_1D_FLOAT;		break;
			case SAMPLERTYPE_INT:		progSpec = useBias ? PROGRAM_1D_INT_BIAS	: PROGRAM_1D_INT;		break;
			case SAMPLERTYPE_UINT:		progSpec = useBias ? PROGRAM_1D_UINT_BIAS	: PROGRAM_1D_UINT;		break;
			case SAMPLERTYPE_SHADOW:	progSpec = useBias ? PROGRAM_1D_SHADOW_BIAS	: PROGRAM_1D_SHADOW;	break;
			default:					DE_ASSERT(false);
		}
	}
	else if (params.texType == TEXTURETYPE_CUBE)
	{
		numComps = 3;

		switch (params.samplerType)
		{
			case SAMPLERTYPE_FLOAT:		progSpec = useBias ? PROGRAM_CUBE_FLOAT_BIAS	: PROGRAM_CUBE_FLOAT;	break;
			case SAMPLERTYPE_INT:		progSpec = useBias ? PROGRAM_CUBE_INT_BIAS		: PROGRAM_CUBE_INT;		break;
			case SAMPLERTYPE_UINT:		progSpec = useBias ? PROGRAM_CUBE_UINT_BIAS		: PROGRAM_CUBE_UINT;	break;
			case SAMPLERTYPE_SHADOW:	progSpec = useBias ? PROGRAM_CUBE_SHADOW_BIAS	: PROGRAM_CUBE_SHADOW;	break;
			default:					DE_ASSERT(false);
		}
	}
	else if (params.texType == TEXTURETYPE_3D)
	{
		numComps = 3;

		switch (params.samplerType)
		{
			case SAMPLERTYPE_FLOAT:		progSpec = useBias ? PROGRAM_3D_FLOAT_BIAS	: PROGRAM_3D_FLOAT;		break;
			case SAMPLERTYPE_INT:		progSpec = useBias ? PROGRAM_3D_INT_BIAS	: PROGRAM_3D_INT;		break;
			case SAMPLERTYPE_UINT:		progSpec = useBias ? PROGRAM_3D_UINT_BIAS	: PROGRAM_3D_UINT;		break;
			default:					DE_ASSERT(false);
		}
	}
	else if (params.texType == TEXTURETYPE_2D_ARRAY)
	{
		DE_ASSERT(!useBias); // \todo [2012-02-17 pyry] Support bias.

		numComps = 3;

		switch (params.samplerType)
		{
			case SAMPLERTYPE_FLOAT:		progSpec = PROGRAM_2D_ARRAY_FLOAT;	break;
			case SAMPLERTYPE_INT:		progSpec = PROGRAM_2D_ARRAY_INT;	break;
			case SAMPLERTYPE_UINT:		progSpec = PROGRAM_2D_ARRAY_UINT;	break;
			case SAMPLERTYPE_SHADOW:	progSpec = PROGRAM_2D_ARRAY_SHADOW;	break;
			default:					DE_ASSERT(false);
		}
	}
	else if (params.texType == TEXTURETYPE_CUBE_ARRAY)
	{
		DE_ASSERT(!useBias);

		numComps = 4;

		switch (params.samplerType)
		{
			case SAMPLERTYPE_FLOAT:		progSpec = PROGRAM_CUBE_ARRAY_FLOAT;	break;
			case SAMPLERTYPE_INT:		progSpec = PROGRAM_CUBE_ARRAY_INT;		break;
			case SAMPLERTYPE_UINT:		progSpec = PROGRAM_CUBE_ARRAY_UINT;		break;
			case SAMPLERTYPE_SHADOW:	progSpec = PROGRAM_CUBE_ARRAY_SHADOW;	break;
			default:					DE_ASSERT(false);
		}
	}
	else if (params.texType == TEXTURETYPE_1D_ARRAY)
	{
		DE_ASSERT(!useBias); // \todo [2012-02-17 pyry] Support bias.

		numComps = 2;

		switch (params.samplerType)
		{
			case SAMPLERTYPE_FLOAT:		progSpec = PROGRAM_1D_ARRAY_FLOAT;	break;
			case SAMPLERTYPE_INT:		progSpec = PROGRAM_1D_ARRAY_INT;	break;
			case SAMPLERTYPE_UINT:		progSpec = PROGRAM_1D_ARRAY_UINT;	break;
			case SAMPLERTYPE_SHADOW:	progSpec = PROGRAM_1D_ARRAY_SHADOW;	break;
			default:					DE_ASSERT(false);
		}
	}
	else if (params.texType == TEXTURETYPE_BUFFER)
	{
		numComps = 1;

		switch (params.samplerType)
		{
			case SAMPLERTYPE_FETCH_FLOAT:	progSpec = PROGRAM_BUFFER_FLOAT;	break;
			case SAMPLERTYPE_FETCH_INT:		progSpec = PROGRAM_BUFFER_INT;		break;
			case SAMPLERTYPE_FETCH_UINT:	progSpec = PROGRAM_BUFFER_UINT;		break;
			default:						DE_ASSERT(false);
		}
	}
	else
		DE_ASSERT(DE_FALSE);

	Unique<VkShaderModule>					vertexShaderModule			(createShaderModule(vkd, vkDevice, m_context.getBinaryCollection().get("vertext_" + std::string(getProgramName(progSpec))), 0));
	Unique<VkShaderModule>					fragmentShaderModule		(createShaderModule(vkd, vkDevice, m_context.getBinaryCollection().get("fragment_" + std::string(getProgramName(progSpec))), 0));

	Move<VkSampler>							sampler;
	Move<VkDescriptorSet>					descriptorSet[2];
	Move<VkDescriptorSetLayout>				descriptorSetLayout[2];
	Move<VkPipelineLayout>					pipelineLayout;

	Move<VkCommandBuffer>					commandBuffer;
	Move<VkPipeline>						graphicsPipeline;
	Move<VkBuffer>							vertexBuffer;
	de::MovePtr<Allocation>					vertexBufferMemory;

	const VkDeviceSize						vertexBufferOffset			= 0;
	const deUint32							vertexPositionStrideSize	= deUint32(sizeof(tcu::Vec4));
	const deUint32							vertexTextureStrideSize		= deUint32(numComps * sizeof(float));
	const deUint32							positionDataSize			= vertexPositionStrideSize * 4u;
	const deUint32							textureCoordDataSize		= vertexTextureStrideSize * 4u;

	const VkPhysicalDeviceProperties		properties					= m_context.getDeviceProperties();

	if (positionDataSize > properties.limits.maxVertexInputAttributeOffset)
	{
		std::stringstream message;
		message << "Larger vertex input attribute offset is needed (" << positionDataSize << ") than the available maximum (" << properties.limits.maxVertexInputAttributeOffset << ").";
		TCU_THROW(NotSupportedError, message.str().c_str());
	}

	// Create Graphics Pipeline
	{
		const VkPipelineShaderStageCreateInfo	shaderStageParams[2]	=
		{
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType					sType;
				DE_NULL,													// const void*						pNext;
				0,															// VkPipelineShaderStageCreateFlags flags;
				VK_SHADER_STAGE_VERTEX_BIT,									// VkShaderStage					stage;
				*vertexShaderModule,										// VkShaderModule					shader;
				"main",														// const char*						pName;
				DE_NULL														// const VkSpecializationInfo*		pSpecializationInfo;
			},
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType					sType;
				DE_NULL,													// const void*						pNext;
				0,															// VkPipelineShaderStageCreateFlags flags;
				VK_SHADER_STAGE_FRAGMENT_BIT,								// VkShaderStage					stage;
				*fragmentShaderModule,										// VkShaderModule					shader;
				"main",														// const char*						pName;
				DE_NULL														// const VkSpecializationInfo*		pSpecializationInfo;
			}
		};

		const VkVertexInputBindingDescription	vertexInputBindingDescription[2]	=
		{
			{
				0u,								// deUint32					binding;
				vertexPositionStrideSize,		// deUint32					strideInBytes;
				VK_VERTEX_INPUT_RATE_VERTEX		// VkVertexInputStepRate	stepRate;
			},
			{
				1u,								// deUint32					binding;
				vertexTextureStrideSize,		// deUint32					strideInBytes;
				VK_VERTEX_INPUT_RATE_VERTEX		// VkVertexInputStepRate	stepRate;
			}
		};

		VkFormat								textureCoordinateFormat			= VK_FORMAT_R32G32B32A32_SFLOAT;

		switch (numComps) {
			case 1: textureCoordinateFormat = VK_FORMAT_R32_SFLOAT;				break;
			case 2: textureCoordinateFormat = VK_FORMAT_R32G32_SFLOAT;			break;
			case 3: textureCoordinateFormat = VK_FORMAT_R32G32B32_SFLOAT;		break;
			case 4: textureCoordinateFormat = VK_FORMAT_R32G32B32A32_SFLOAT;	break;
			default:
				DE_ASSERT(false);
		}

		const VkVertexInputAttributeDescription	vertexInputAttributeDescriptions[2] =
		{
			{
				0u,									// deUint32	location;
				0u,									// deUint32	binding;
				VK_FORMAT_R32G32B32A32_SFLOAT,		// VkFormat	format;
				0u									// deUint32	offsetInBytes;
			},
			{
				1u,									// deUint32	location;
				1u,									// deUint32	binding;
				textureCoordinateFormat,			// VkFormat	format;
				positionDataSize					// deUint32	offsetInBytes;
			}
		};

		const VkPipelineVertexInputStateCreateInfo	vertexInputStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,		// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0,																// VkPipelineVertexInputStateCreateFlags	flags;
			2u,																// deUint32									bindingCount;
			vertexInputBindingDescription,									// const VkVertexInputBindingDescription*	pVertexBindingDescriptions;
			2u,																// deUint32									attributeCount;
			vertexInputAttributeDescriptions								// const VkVertexInputAttributeDescription*	pVertexAttributeDescriptions;
		};

		const VkPipelineInputAssemblyStateCreateInfo	inputAssemblyStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0,																// VkPipelineInputAssemblyStateCreateFlags	flags;
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,							// VkPrimitiveTopology						topology;
			VK_FALSE														// VkBool32									primitiveRestartEnable;
		};

		const VkViewport						viewport =
		{
			m_viewportOffsetX,			// float	originX;
			m_viewportOffsetY,			// float	originY;
			m_viewportWidth,			// float	width;
			m_viewportHeight,			// float	height;
			0.0f,						// float	minDepth;
			1.0f						// float	maxDepth;
		};

		const VkRect2D							scissor =
		{
			{ 0, 0 },														// VkOffset2D  offset;
			{ m_renderWidth, m_renderHeight }								// VkExtent2D  extent;
		};

		const VkPipelineViewportStateCreateInfo	viewportStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,			// VkStructureType						sType;
			DE_NULL,														// const void*							pNext;
			0,																// VkPipelineViewportStateCreateFlags	flags;
			1u,																// deUint32								viewportCount;
			&viewport,														// const VkViewport*					pViewports;
			1u,																// deUint32								scissorCount;
			&scissor														// const VkRect2D*						pScissors;
		};

		const VkPipelineMultisampleStateCreateInfo multisampleStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,		// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0u,																// VkPipelineMultisampleStateCreateFlags	flags;
			m_sampleCount,													// VkSampleCountFlagBits					rasterizationSamples;
			VK_FALSE,														// VkBool32									sampleShadingEnable;
			0.0f,															// float									minSampleShading;
			DE_NULL,														// const VkSampleMask*						pSampleMask;
			VK_FALSE,														// VkBool32									alphaToCoverageEnable;
			VK_FALSE														// VkBool32									alphaToOneEnable;
		};

		const VkPipelineRasterizationStateCreateInfo	rasterizationStateCreateInfo	=
		{
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,		// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0,																// VkPipelineRasterizationStateCreateFlags	flags;
			VK_FALSE,														// VkBool32									depthClipEnable;
			VK_FALSE,														// VkBool32									rasterizerDiscardEnable;
			VK_POLYGON_MODE_FILL,											// VkFillMode								fillMode;
			VK_CULL_MODE_NONE,												// VkCullMode								cullMode;
			VK_FRONT_FACE_COUNTER_CLOCKWISE,								// VkFrontFace								frontFace;
			VK_FALSE,														// VkBool32									depthBiasEnable;
			0.0f,															// float									depthBias;
			0.0f,															// float									depthBiasClamp;
			0.0f,															// float									slopeScaledDepthBias;
			1.0f,															// float									lineWidth;
		};

		const VkPipelineColorBlendAttachmentState	colorBlendAttachmentState	=
		{
			VK_FALSE,													// VkBool32			blendEnable;
			VK_BLEND_FACTOR_ONE,										// VkBlend			srcBlendColor;
			VK_BLEND_FACTOR_ZERO,										// VkBlend			destBlendColor;
			VK_BLEND_OP_ADD,											// VkBlendOp		blendOpColor;
			VK_BLEND_FACTOR_ONE,										// VkBlend			srcBlendAlpha;
			VK_BLEND_FACTOR_ZERO,										// VkBlend			destBlendAlpha;
			VK_BLEND_OP_ADD,											// VkBlendOp		blendOpAlpha;
			(VK_COLOR_COMPONENT_R_BIT |
			 VK_COLOR_COMPONENT_G_BIT |
			 VK_COLOR_COMPONENT_B_BIT |
			 VK_COLOR_COMPONENT_A_BIT)									// VkChannelFlags	channelWriteMask;
		};

		const VkPipelineColorBlendStateCreateInfo	colorBlendStateParams		=
		{
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,	// VkStructureType								sType;
			DE_NULL,													// const void*									pNext;
			0,															// VkPipelineColorBlendStateCreateFlags			flags;
			VK_FALSE,													// VkBool32										logicOpEnable;
			VK_LOGIC_OP_COPY,											// VkLogicOp									logicOp;
			1u,															// deUint32										attachmentCount;
			&colorBlendAttachmentState,									// const VkPipelineColorBlendAttachmentState*	pAttachments;
			{ 0.0f, 0.0f, 0.0f, 0.0f },									// float										blendConst[4];
		};

		VkSamplerCreateInfo					samplerCreateInfo			= mapSampler(params.sampler, m_textureBindings[texUnit]->getTestTexture().getTextureFormat(), params.minLod, params.maxLod);

		if (maxAnisotropy > 1.0f)
		{
			samplerCreateInfo.anisotropyEnable = VK_TRUE;
			samplerCreateInfo.maxAnisotropy = maxAnisotropy;
		}

		if (samplerCreateInfo.magFilter == VK_FILTER_LINEAR || samplerCreateInfo.minFilter == VK_FILTER_LINEAR || samplerCreateInfo.mipmapMode == VK_SAMPLER_MIPMAP_MODE_LINEAR)
		{
			const pipeline::TestTexture&	testTexture			= m_textureBindings[texUnit]->getTestTexture();
			const VkFormat					textureFormat		= testTexture.isCompressed() ? mapCompressedTextureFormat(testTexture.getCompressedLevel(0, 0).getFormat())
																							 : mapTextureFormat          (testTexture.getTextureFormat());
			const VkFormatProperties		formatProperties	= getPhysicalDeviceFormatProperties(m_context.getInstanceInterface(), m_context.getPhysicalDevice(), textureFormat);

			if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
				TCU_THROW(NotSupportedError, "Linear filtering for this image format is not supported");
		}

		sampler = createSampler(vkd, vkDevice, &samplerCreateInfo);

		descriptorSetLayout[0] = DescriptorSetLayoutBuilder()
											.addSingleBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
												.build(vkd, vkDevice);

		descriptorSetLayout[1] = DescriptorSetLayoutBuilder()
											.addSingleSamplerBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, &sampler.get())
											.build(vkd, vkDevice);


		descriptorSet[0] = makeDescriptorSet(*m_descriptorPool, *descriptorSetLayout[0]);
		descriptorSet[1] = makeDescriptorSet(*m_descriptorPool, *descriptorSetLayout[1]);

		{
			const VkDescriptorBufferInfo			descriptorBufferInfo	=
			{
				*m_uniformBuffer,							// VkBuffer		buffer;
				0u,											// VkDeviceSize	offset;
				VK_WHOLE_SIZE								// VkDeviceSize	range;
			};

			DescriptorSetUpdateBuilder()
				.writeSingle(*descriptorSet[0], DescriptorSetUpdateBuilder::Location::binding(0), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &descriptorBufferInfo)
				.update(vkd, vkDevice);
		}

		{
			VkDescriptorImageInfo					descriptorImageInfo		=
			{
				*sampler,										// VkSampler		sampler;
				m_textureBindings[texUnit]->getImageView(),		// VkImageView		imageView;
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL		// VkImageLayout	imageLayout;
			};

			DescriptorSetUpdateBuilder()
				.writeSingle(*descriptorSet[1], DescriptorSetUpdateBuilder::Location::binding(0), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &descriptorImageInfo)
				.update(vkd, vkDevice);
		}

		// Pipeline Layout
		{
			VkDescriptorSetLayout					descriptorSetLayouts[2]		=
			{
				*descriptorSetLayout[0],
				*descriptorSetLayout[1]
			};

			const VkPipelineLayoutCreateInfo		pipelineLayoutCreateInfo	=
			{
				VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,		// VkStructureType				sType;
				DE_NULL,											// const void*					pNext;
				0u,													// VkPipelineLayoutCreateFlags	flags;
				2u,													// deUint32						descriptorSetCount;
				descriptorSetLayouts,								// const VkDescriptorSetLayout*	pSetLayouts;
				0u,													// deUint32						pushConstantRangeCount;
				DE_NULL												// const VkPushConstantRange*	pPushConstantRanges;
			};

			pipelineLayout = createPipelineLayout(vkd, vkDevice, &pipelineLayoutCreateInfo);
		}

		const VkGraphicsPipelineCreateInfo graphicsPipelineParams =
		{
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	// VkStructureType									sType;
			DE_NULL,											// const void*										pNext;
			0u,													// VkPipelineCreateFlags							flags;
			2u,													// deUint32											stageCount;
			shaderStageParams,									// const VkPipelineShaderStageCreateInfo*			pStages;
			&vertexInputStateParams,							// const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
			&inputAssemblyStateParams,							// const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
			DE_NULL,											// const VkPipelineTessellationStateCreateInfo*		pTessellationState;
			&viewportStateParams,								// const VkPipelineViewportStateCreateInfo*			pViewportState;
			&rasterizationStateCreateInfo,						// const VkPipelineRasterStateCreateInfo*			pRasterizationState;
			&multisampleStateParams,							// const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
			DE_NULL,											// const VkPipelineDepthStencilStateCreateInfo*		pDepthStencilState;
			&colorBlendStateParams,								// const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
			DE_NULL,											// const VkPipelineDynamicStateCreateInfo*			pDynamicState;
			*pipelineLayout,									// VkPipelineLayout									layout;
			*m_renderPass,										// VkRenderPass										renderPass;
			0u,													// deUint32											subpass;
			0u,													// VkPipeline										basePipelineHandle;
			0u													// deInt32											basePipelineIndex;
		};

		graphicsPipeline		= createGraphicsPipeline(vkd, vkDevice, DE_NULL, &graphicsPipelineParams);
	}

	// Create Vertex Buffer
	{
		VkDeviceSize bufferSize = positionDataSize + textureCoordDataSize;

		// Pad the buffer size to a stride multiple for the last element so that it isn't out of bounds
		bufferSize += vertexTextureStrideSize - ((bufferSize - vertexBufferOffset) % vertexTextureStrideSize);

		const VkBufferCreateInfo			vertexBufferParams		=
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			bufferSize,									// VkDeviceSize			size;
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,			// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			1u,											// deUint32				queueFamilyCount;
			&queueFamilyIndex							// const deUint32*		pQueueFamilyIndices;
		};

		vertexBuffer		= createBuffer(vkd, vkDevice, &vertexBufferParams);
		vertexBufferMemory	= allocator.allocate(getBufferMemoryRequirements(vkd, vkDevice, *vertexBuffer), MemoryRequirement::HostVisible);

		VK_CHECK(vkd.bindBufferMemory(vkDevice, *vertexBuffer, vertexBufferMemory->getMemory(), vertexBufferMemory->getOffset()));

		// Load vertices into vertex buffer
		deMemcpy(vertexBufferMemory->getHostPtr(), position, positionDataSize);
		deMemcpy(reinterpret_cast<deUint8*>(vertexBufferMemory->getHostPtr()) +  positionDataSize, texCoord, textureCoordDataSize);
		flushMappedMemoryRange(vkd, vkDevice, vertexBufferMemory->getMemory(), vertexBufferMemory->getOffset(), VK_WHOLE_SIZE);
	}

	// Create Command Buffer
	commandBuffer = allocateCommandBuffer(vkd, vkDevice, *m_commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	// Begin Command Buffer
	{
		const VkCommandBufferBeginInfo		cmdBufferBeginInfo		=
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType							sType;
			DE_NULL,										// const void*								pNext;
			0u,												// VkCmdBufferOptimizeFlags					flags;
			DE_NULL											// const VkCommandBufferInheritanceInfo*	pInheritanceInfo;
		};

		VK_CHECK(vkd.beginCommandBuffer(*commandBuffer, &cmdBufferBeginInfo));
	}

	// Begin Render Pass
	{
		const VkRenderPassBeginInfo			renderPassBeginInfo		=
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,				// VkStructureType		sType;
			DE_NULL,												// const void*			pNext;
			*m_renderPass,											// VkRenderPass			renderPass;
			*m_frameBuffer,											// VkFramebuffer		framebuffer;
			{
				{ 0, 0 },
				{ m_renderWidth, m_renderHeight }
			},														// VkRect2D				renderArea;
			0u,														// deUint32				clearValueCount;
			DE_NULL													// const VkClearValue*	pClearValues;
		};

		vkd.cmdBeginRenderPass(*commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	vkd.cmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *graphicsPipeline);
	vkd.cmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0u, 1, &descriptorSet[0].get(), 0u, DE_NULL);
	vkd.cmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 1u, 1, &descriptorSet[1].get(), 0u, DE_NULL);
	vkd.cmdBindVertexBuffers(*commandBuffer, 0, 1, &vertexBuffer.get(), &vertexBufferOffset);
	vkd.cmdBindVertexBuffers(*commandBuffer, 1, 1, &vertexBuffer.get(), &vertexBufferOffset);
	vkd.cmdBindIndexBuffer(*commandBuffer, *m_vertexIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
	vkd.cmdDrawIndexed(*commandBuffer, 6, 1, 0, 0, 0);
	vkd.cmdEndRenderPass(*commandBuffer);

	// Copy Image
	{
		const VkBufferMemoryBarrier			bufferBarrier			=
		{
			VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			VK_ACCESS_TRANSFER_WRITE_BIT,				// VkAccessFlags		srcAccessMask;
			VK_ACCESS_HOST_READ_BIT,					// VkAccessFlags		dstAccessMask;
			VK_QUEUE_FAMILY_IGNORED,					// deUint32				srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,					// deUint32				destQueueFamilyIndex;
			*m_resultBuffer,							// VkBuffer				buffer;
			0u,											// VkDeviceSize			offset;
			m_resultBufferSize							// VkDeviceSize			size;
		};

		const VkBufferImageCopy				copyRegion				=
		{
			0u,											// VkDeviceSize				bufferOffset;
			m_renderWidth,								// deUint32					bufferRowLength;
			m_renderHeight,								// deUint32					bufferImageHeight;
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0u,
				0u,
				1u
			},											// VkImageSubresourceCopy	imageSubresource;
			{ 0, 0, 0 },								// VkOffset3D				imageOffset;
			{ m_renderWidth, m_renderHeight, 1u }		// VkExtent3D				imageExtent;
		};

		addImageTransitionBarrier(*commandBuffer,
								  m_multisampling ? *m_resolvedImage : *m_image,
								  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,		// VkPipelineStageFlags		srcStageMask
								  VK_PIPELINE_STAGE_TRANSFER_BIT,						// VkPipelineStageFlags		dstStageMask
								  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,					// VkAccessFlags			srcAccessMask
								  VK_ACCESS_TRANSFER_READ_BIT,							// VkAccessFlags			dstAccessMask
								  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,				// VkImageLayout			oldLayout;
								  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);				// VkImageLayout			newLayout;

		if (m_multisampling)
			vkd.cmdCopyImageToBuffer(*commandBuffer, *m_resolvedImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *m_resultBuffer, 1, &copyRegion);
		else
			vkd.cmdCopyImageToBuffer(*commandBuffer, *m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *m_resultBuffer, 1, &copyRegion);

		vkd.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &bufferBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);

		addImageTransitionBarrier(*commandBuffer,
								  m_multisampling ? *m_resolvedImage : *m_image,
								  VK_PIPELINE_STAGE_TRANSFER_BIT,					// VkPipelineStageFlags		srcStageMask
								  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,	// VkPipelineStageFlags		dstStageMask
								  VK_ACCESS_TRANSFER_READ_BIT,						// VkAccessFlags			srcAccessMask
								  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,				// VkAccessFlags			dstAccessMask
								  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,				// VkImageLayout			oldLayout;
								  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);		// VkImageLayout			newLayout;
	}

	VK_CHECK(vkd.endCommandBuffer(*commandBuffer));

	// Upload uniform buffer data
	{
		const ShaderParameters	shaderParameters	=
		{
			params.bias,			// float		bias;				//!< User-supplied bias.
			params.ref,				// float		ref;				//!< Reference value for shadow lookups.
			tcu::Vec2(),			// tcu::Vec2	padding;			//!< Shader uniform padding.
			params.colorScale,		// tcu::Vec4	colorScale;			//!< Scale for texture color values.
			params.colorBias		// tcu::Vec4	colorBias;			//!< Bias for texture color values.
		};
		deMemcpy(m_uniformBufferMemory->getHostPtr(), &shaderParameters, sizeof(shaderParameters));
		flushMappedMemoryRange(vkd, vkDevice, m_uniformBufferMemory->getMemory(), m_uniformBufferMemory->getOffset(), VK_WHOLE_SIZE);

		if (logUniforms)
			m_log << TestLog::Message << "u_sampler = " << texUnit << TestLog::EndMessage;

		if (useBias)
		{
			if (logUniforms)
				m_log << TestLog::Message << "u_bias = " << shaderParameters.bias << TestLog::EndMessage;
		}

		if (params.samplerType == SAMPLERTYPE_SHADOW)
		{
			if (logUniforms)
				m_log << TestLog::Message << "u_ref = " << shaderParameters.ref << TestLog::EndMessage;
		}

		if (logUniforms)
		{
			m_log << TestLog::Message << "u_colorScale = " << shaderParameters.colorScale << TestLog::EndMessage;
			m_log << TestLog::Message << "u_colorBias = " << shaderParameters.colorBias << TestLog::EndMessage;
		}
	}

	// Submit
	{
		const VkSubmitInfo					submitInfo				=
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,			// VkStructureType				sType;
			DE_NULL,								// const void*					pNext;
			0u,										// deUint32						waitSemaphoreCount;
			DE_NULL,								// const VkSemaphore*			pWaitSemaphores;
			DE_NULL,								// const VkPipelineStageFlags*	pWaitDstStageMask;
			1u,										// deUint32						commandBufferCount;
			&commandBuffer.get(),					// const VkCommandBuffer*		pCommandBuffers;
			0u,										// deUint32						signalSemaphoreCount;
			DE_NULL,								// const VkSemaphore*			pSignalSemaphores;
		};

		VK_CHECK(vkd.resetFences(vkDevice, 1, &m_fence.get()));
		VK_CHECK(vkd.queueSubmit(queue, 1, &submitInfo, *m_fence));
		VK_CHECK(vkd.waitForFences(vkDevice, 1, &m_fence.get(), true, ~(0ull) /* infinity */));
	}

	invalidateMappedMemoryRange(vkd, vkDevice, m_resultBufferMemory->getMemory(), m_resultBufferMemory->getOffset(), VK_WHOLE_SIZE);

	tcu::copy(result.getAccess(), tcu::ConstPixelBufferAccess(m_textureFormat, tcu::IVec3(m_renderWidth, m_renderHeight, 1u), m_resultBufferMemory->getHostPtr()));
}

/*--------------------------------------------------------------------*//*!
 * \brief Map Vulkan sampler parameters to tcu::Sampler.
 *
 * If no mapping is found, throws tcu::InternalError.
 *
 * \param wrapU			U-component wrap mode
 * \param wrapV			V-component wrap mode
 * \param wrapW			W-component wrap mode
 * \param minFilterMode	Minification filter mode
 * \param magFilterMode	Magnification filter mode
 * \return Sampler description.
 *//*--------------------------------------------------------------------*/
tcu::Sampler createSampler (tcu::Sampler::WrapMode wrapU, tcu::Sampler::WrapMode wrapV, tcu::Sampler::WrapMode wrapW, tcu::Sampler::FilterMode minFilterMode, tcu::Sampler::FilterMode magFilterMode)
{
	return tcu::Sampler(wrapU, wrapV, wrapW,
						minFilterMode, magFilterMode,
						0.0f /* lod threshold */,
						true /* normalized coords */,
						tcu::Sampler::COMPAREMODE_NONE /* no compare */,
						0 /* compare channel */,
						tcu::Vec4(0.0f) /* border color, not used */);
}

/*--------------------------------------------------------------------*//*!
 * \brief Map Vulkan sampler parameters to tcu::Sampler.
 *
 * If no mapping is found, throws tcu::InternalError.
 *
 * \param wrapU			U-component wrap mode
 * \param wrapV			V-component wrap mode
 * \param minFilterMode	Minification filter mode
 * \param minFilterMode	Magnification filter mode
 * \return Sampler description.
 *//*--------------------------------------------------------------------*/
tcu::Sampler createSampler (tcu::Sampler::WrapMode wrapU, tcu::Sampler::WrapMode wrapV, tcu::Sampler::FilterMode minFilterMode, tcu::Sampler::FilterMode magFilterMode)
{
	return createSampler(wrapU, wrapV, wrapU, minFilterMode, magFilterMode);
}

/*--------------------------------------------------------------------*//*!
 * \brief Map Vulkan sampler parameters to tcu::Sampler.
 *
 * If no mapping is found, throws tcu::InternalError.
 *
 * \param wrapU			U-component wrap mode
 * \param minFilterMode	Minification filter mode
 * \return Sampler description.
 *//*--------------------------------------------------------------------*/
tcu::Sampler createSampler (tcu::Sampler::WrapMode wrapU, tcu::Sampler::FilterMode minFilterMode, tcu::Sampler::FilterMode magFilterMode)
{
	return createSampler(wrapU, wrapU, wrapU, minFilterMode, magFilterMode);
}

TestTexture2DSp loadTexture2D (const tcu::Archive& archive, const std::vector<std::string>& filenames)
{
	DE_ASSERT(filenames.size() > 0);

	TestTexture2DSp texture;

	std::string ext = de::FilePath(filenames[0]).getFileExtension();

	if (ext == "png")
	{

		for (size_t fileIndex = 0; fileIndex < filenames.size(); ++fileIndex)
		{
			tcu::TextureLevel level;

			tcu::ImageIO::loadImage(level, archive, filenames[fileIndex].c_str());

			TCU_CHECK_INTERNAL(level.getFormat() == tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8) ||
											   level.getFormat() == tcu::TextureFormat(tcu::TextureFormat::RGB, tcu::TextureFormat::UNORM_INT8));

			if (fileIndex == 0)
				texture = TestTexture2DSp(new pipeline::TestTexture2D(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8), level.getWidth(), level.getHeight()));

			tcu::copy(texture->getLevel((int)fileIndex, 0), level.getAccess());
		}
	}
	else if (ext == "pkm")
	{

		for (size_t fileIndex = 0; fileIndex < filenames.size(); ++fileIndex)
		{
			// Compressed texture.
			tcu::CompressedTexture	level;

			tcu::ImageIO::loadPKM(level, archive, filenames[fileIndex].c_str());

			tcu::TextureFormat		uncompressedFormat		= tcu::getUncompressedFormat(level.getFormat());
			std::vector<deUint8>	uncompressedData		(uncompressedFormat.getPixelSize() * level.getWidth() * level.getHeight(), 0);
			tcu::PixelBufferAccess	decompressedBuffer		(uncompressedFormat, level.getWidth(), level.getHeight(), 1, uncompressedData.data());

			tcu::TextureFormat		commonFormat			= tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8);
			std::vector<deUint8>	commonFromatData		(commonFormat.getPixelSize() * level.getWidth() * level.getHeight(), 0);
			tcu::PixelBufferAccess	commonFormatBuffer		(commonFormat, level.getWidth(), level.getHeight(), 1, commonFromatData.data());

			if (fileIndex == 0)
				texture = TestTexture2DSp(new pipeline::TestTexture2D(commonFormat, level.getWidth(), level.getHeight()));

			level.decompress(decompressedBuffer, tcu::TexDecompressionParams(tcu::TexDecompressionParams::ASTCMODE_LDR));

			tcu::copy(commonFormatBuffer, decompressedBuffer);
			tcu::copy(texture->getLevel((int)fileIndex, 0), commonFormatBuffer);
		}
	}
	else
		TCU_FAIL("Unsupported file format");

	return texture;
}

TestTextureCubeSp loadTextureCube (const tcu::Archive& archive, const std::vector<std::string>& filenames)
{
	DE_ASSERT(filenames.size() > 0);
	DE_STATIC_ASSERT(tcu::CUBEFACE_LAST == 6);
	TCU_CHECK((int)filenames.size() % tcu::CUBEFACE_LAST == 0);

	TestTextureCubeSp texture;

	std::string ext = de::FilePath(filenames[0]).getFileExtension();

	if (ext == "png")
	{

		for (size_t fileIndex = 0; fileIndex < filenames.size(); ++fileIndex)
		{
			tcu::TextureLevel level;

			tcu::ImageIO::loadImage(level, archive, filenames[fileIndex].c_str());

			TCU_CHECK_INTERNAL(level.getFormat() == tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8) ||
											   level.getFormat() == tcu::TextureFormat(tcu::TextureFormat::RGB, tcu::TextureFormat::UNORM_INT8));

			TCU_CHECK( level.getWidth() == level.getHeight());

			if (fileIndex == 0)
				texture = TestTextureCubeSp(new pipeline::TestTextureCube(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8), level.getWidth()));

			tcu::copy(texture->getLevel((int)fileIndex / 6, (int)fileIndex % 6), level.getAccess());
		}
	}
	else if (ext == "pkm")
	{
		for (size_t fileIndex = 0; fileIndex < filenames.size(); ++fileIndex)
		{
			// Compressed texture.
			tcu::CompressedTexture	level;

			tcu::ImageIO::loadPKM(level, archive, filenames[fileIndex].c_str());

			TCU_CHECK( level.getWidth() == level.getHeight());

			tcu::TextureFormat		uncompressedFormat				= tcu::getUncompressedFormat(level.getFormat());
			std::vector<deUint8>	uncompressedData				(uncompressedFormat.getPixelSize() * level.getWidth() * level.getHeight(), 0);
			tcu::PixelBufferAccess	decompressedBuffer				(uncompressedFormat, level.getWidth(), level.getHeight(), 1, uncompressedData.data());

			tcu::TextureFormat		commonFormat					= tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8);
			std::vector<deUint8>	commonFromatData				(commonFormat.getPixelSize() * level.getWidth() * level.getHeight(), 0);
			tcu::PixelBufferAccess	commonFormatBuffer				(commonFormat, level.getWidth(), level.getHeight(), 1, commonFromatData.data());

			if (fileIndex == 0)
				texture = TestTextureCubeSp(new pipeline::TestTextureCube(commonFormat, level.getWidth()));

			level.decompress(decompressedBuffer, tcu::TexDecompressionParams(tcu::TexDecompressionParams::ASTCMODE_LDR));

			tcu::copy(commonFormatBuffer, decompressedBuffer);
			tcu::copy(texture->getLevel((int)fileIndex / 6, (int)fileIndex % 6), commonFormatBuffer);
		}
	}
	else
		TCU_FAIL("Unsupported file format");

	return texture;
}

TextureCommonTestCaseParameters::TextureCommonTestCaseParameters (void)
	: sampleCount			(VK_SAMPLE_COUNT_1_BIT)
	, texCoordPrecision		(glu::PRECISION_HIGHP)
	, minFilter				(tcu::Sampler::LINEAR)
	, magFilter				(tcu::Sampler::LINEAR)
	, wrapS					(tcu::Sampler::REPEAT_GL)
	, wrapT					(tcu::Sampler::REPEAT_GL)
	, format				(VK_FORMAT_R8G8B8A8_UNORM)
{
}

Texture2DTestCaseParameters::Texture2DTestCaseParameters (void)
	: width					(64)
	, height				(64)
{
}

TextureCubeTestCaseParameters::TextureCubeTestCaseParameters (void)
	: size					(64)
{
}

Texture2DArrayTestCaseParameters::Texture2DArrayTestCaseParameters (void)
	: numLayers				(8)
{
}

Texture3DTestCaseParameters::Texture3DTestCaseParameters (void)
	: wrapR					(tcu::Sampler::REPEAT_GL)
	, depth					(64)
{
}

} // util
} // texture
} // vkt
