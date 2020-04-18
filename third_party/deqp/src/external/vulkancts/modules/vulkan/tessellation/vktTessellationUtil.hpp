#ifndef _VKTTESSELLATIONUTIL_HPP
#define _VKTTESSELLATIONUTIL_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2014 The Android Open Source Project
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \brief Tessellation Utilities
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "vkMemUtil.hpp"
#include "vkRef.hpp"
#include "vkPrograms.hpp"
#include "vkRefUtil.hpp"
#include "vkQueryUtil.hpp"

#include "tcuVector.hpp"
#include "tcuMaybe.hpp"

#include "deStringUtil.hpp"

#include <algorithm>  // sort
#include <iterator>   // distance

namespace vkt
{
namespace tessellation
{

class Buffer
{
public:
										Buffer			(const vk::DeviceInterface&		vk,
														 const vk::VkDevice				device,
														 vk::Allocator&					allocator,
														 const vk::VkBufferCreateInfo&	bufferCreateInfo,
														 const vk::MemoryRequirement	memoryRequirement)

											: m_buffer		(createBuffer(vk, device, &bufferCreateInfo))
											, m_allocation	(allocator.allocate(getBufferMemoryRequirements(vk, device, *m_buffer), memoryRequirement))
										{
											VK_CHECK(vk.bindBufferMemory(device, *m_buffer, m_allocation->getMemory(), m_allocation->getOffset()));
										}

	const vk::VkBuffer&					get				(void) const { return *m_buffer; }
	const vk::VkBuffer&					operator*		(void) const { return get(); }
	vk::Allocation&						getAllocation	(void) const { return *m_allocation; }

private:
	const vk::Unique<vk::VkBuffer>		m_buffer;
	const de::UniquePtr<vk::Allocation>	m_allocation;

	// "deleted"
										Buffer			(const Buffer&);
	Buffer&								operator=		(const Buffer&);
};

class Image
{
public:
										Image			(const vk::DeviceInterface&		vk,
														 const vk::VkDevice				device,
														 vk::Allocator&					allocator,
														 const vk::VkImageCreateInfo&	imageCreateInfo,
														 const vk::MemoryRequirement	memoryRequirement)

											: m_image		(createImage(vk, device, &imageCreateInfo))
											, m_allocation	(allocator.allocate(getImageMemoryRequirements(vk, device, *m_image), memoryRequirement))
										{
											VK_CHECK(vk.bindImageMemory(device, *m_image, m_allocation->getMemory(), m_allocation->getOffset()));
										}

	const vk::VkImage&					get				(void) const { return *m_image; }
	const vk::VkImage&					operator*		(void) const { return get(); }
	vk::Allocation&						getAllocation	(void) const { return *m_allocation; }

private:
	const vk::Unique<vk::VkImage>		m_image;
	const de::UniquePtr<vk::Allocation>	m_allocation;

	// "deleted"
										Image			(const Image&);
	Image&								operator=		(const Image&);
};

class GraphicsPipelineBuilder
{
public:
								GraphicsPipelineBuilder	(void) : m_renderSize				(0, 0)
															   , m_shaderStageFlags			(0u)
															   , m_cullModeFlags			(vk::VK_CULL_MODE_NONE)
															   , m_frontFace				(vk::VK_FRONT_FACE_COUNTER_CLOCKWISE)
															   , m_patchControlPoints		(1u)
															   , m_blendEnable				(false)
															   , m_primitiveTopology		(vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
															   , m_tessellationDomainOrigin	(tcu::nothing<vk::VkTessellationDomainOriginKHR>()) {}

	GraphicsPipelineBuilder&	setRenderSize					(const tcu::IVec2& size) { m_renderSize = size; return *this; }
	GraphicsPipelineBuilder&	setShader						(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkShaderStageFlagBits stage, const vk::ProgramBinary& binary, const vk::VkSpecializationInfo* specInfo);
	GraphicsPipelineBuilder&	setPatchControlPoints			(const deUint32 controlPoints) { m_patchControlPoints = controlPoints; return *this; }
	GraphicsPipelineBuilder&	setCullModeFlags				(const vk::VkCullModeFlags cullModeFlags) { m_cullModeFlags = cullModeFlags; return *this; }
	GraphicsPipelineBuilder&	setFrontFace					(const vk::VkFrontFace frontFace) { m_frontFace = frontFace; return *this; }
	GraphicsPipelineBuilder&	setBlend						(const bool enable) { m_blendEnable = enable; return *this; }

	//! Applies only to pipelines without tessellation shaders.
	GraphicsPipelineBuilder&	setPrimitiveTopology			(const vk::VkPrimitiveTopology topology) { m_primitiveTopology = topology; return *this; }

	GraphicsPipelineBuilder&	addVertexBinding				(const vk::VkVertexInputBindingDescription vertexBinding) { m_vertexInputBindings.push_back(vertexBinding); return *this; }
	GraphicsPipelineBuilder&	addVertexAttribute				(const vk::VkVertexInputAttributeDescription vertexAttribute) { m_vertexInputAttributes.push_back(vertexAttribute); return *this; }

	//! Basic vertex input configuration (uses biding 0, location 0, etc.)
	GraphicsPipelineBuilder&	setVertexInputSingleAttribute	(const vk::VkFormat vertexFormat, const deUint32 stride);

	//! If tessellation domain origin is set, pipeline requires VK_KHR_maintenance2
	GraphicsPipelineBuilder&	setTessellationDomainOrigin		(const vk::VkTessellationDomainOriginKHR domainOrigin) { return setTessellationDomainOrigin(tcu::just(domainOrigin)); }
	GraphicsPipelineBuilder&	setTessellationDomainOrigin		(const tcu::Maybe<vk::VkTessellationDomainOriginKHR>& domainOrigin) { m_tessellationDomainOrigin = domainOrigin; return *this; }

	vk::Move<vk::VkPipeline>	build							(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkPipelineLayout pipelineLayout, const vk::VkRenderPass renderPass);

private:
	tcu::IVec2											m_renderSize;
	vk::Move<vk::VkShaderModule>						m_vertexShaderModule;
	vk::Move<vk::VkShaderModule>						m_fragmentShaderModule;
	vk::Move<vk::VkShaderModule>						m_geometryShaderModule;
	vk::Move<vk::VkShaderModule>						m_tessControlShaderModule;
	vk::Move<vk::VkShaderModule>						m_tessEvaluationShaderModule;
	std::vector<vk::VkPipelineShaderStageCreateInfo>	m_shaderStages;
	std::vector<vk::VkVertexInputBindingDescription>	m_vertexInputBindings;
	std::vector<vk::VkVertexInputAttributeDescription>	m_vertexInputAttributes;
	vk::VkShaderStageFlags								m_shaderStageFlags;
	vk::VkCullModeFlags									m_cullModeFlags;
	vk::VkFrontFace										m_frontFace;
	deUint32											m_patchControlPoints;
	bool												m_blendEnable;
	vk::VkPrimitiveTopology								m_primitiveTopology;
	tcu::Maybe<vk::VkTessellationDomainOriginKHR>		m_tessellationDomainOrigin;

	GraphicsPipelineBuilder (const GraphicsPipelineBuilder&); // "deleted"
	GraphicsPipelineBuilder& operator= (const GraphicsPipelineBuilder&);
};

struct TessLevels
{
	float inner[2];
	float outer[4];
};

enum TessPrimitiveType
{
	TESSPRIMITIVETYPE_TRIANGLES = 0,
	TESSPRIMITIVETYPE_QUADS,
	TESSPRIMITIVETYPE_ISOLINES,

	TESSPRIMITIVETYPE_LAST,
};

enum SpacingMode
{
	SPACINGMODE_EQUAL = 0,
	SPACINGMODE_FRACTIONAL_ODD,
	SPACINGMODE_FRACTIONAL_EVEN,

	SPACINGMODE_LAST,
};

enum Winding
{
	WINDING_CCW = 0,
	WINDING_CW,

	WINDING_LAST,
};

enum ShaderLanguage
{
	SHADER_LANGUAGE_GLSL = 0,
	SHADER_LANGUAGE_HLSL = 1,

	SHADER_LANGUAGE_LAST,
};

enum FeatureFlagBits
{
	FEATURE_TESSELLATION_SHADER							= 1u << 0,
	FEATURE_GEOMETRY_SHADER								= 1u << 1,
	FEATURE_SHADER_FLOAT_64								= 1u << 2,
	FEATURE_VERTEX_PIPELINE_STORES_AND_ATOMICS			= 1u << 3,
	FEATURE_FRAGMENT_STORES_AND_ATOMICS					= 1u << 4,
	FEATURE_SHADER_TESSELLATION_AND_GEOMETRY_POINT_SIZE	= 1u << 5,
};
typedef deUint32 FeatureFlags;

vk::VkBufferCreateInfo			makeBufferCreateInfo						(const vk::VkDeviceSize bufferSize, const vk::VkBufferUsageFlags usage);
vk::VkImageCreateInfo			makeImageCreateInfo							(const tcu::IVec2& size, const vk::VkFormat format, const vk::VkImageUsageFlags usage, const deUint32 numArrayLayers);
vk::Move<vk::VkCommandPool>		makeCommandPool								(const vk::DeviceInterface& vk, const vk::VkDevice device, const deUint32 queueFamilyIndex);
vk::Move<vk::VkDescriptorSet>	makeDescriptorSet							(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkDescriptorPool descriptorPool, const vk::VkDescriptorSetLayout setLayout);
vk::Move<vk::VkPipelineLayout>	makePipelineLayout							(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkDescriptorSetLayout descriptorSetLayout);
vk::Move<vk::VkPipelineLayout>	makePipelineLayoutWithoutDescriptors		(const vk::DeviceInterface& vk, const vk::VkDevice device);
vk::Move<vk::VkPipeline>		makeComputePipeline							(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkPipelineLayout pipelineLayout, const vk::VkShaderModule shaderModule, const vk::VkSpecializationInfo* specInfo);
vk::Move<vk::VkRenderPass>		makeRenderPass								(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkFormat colorFormat);
vk::Move<vk::VkRenderPass>		makeRenderPassWithoutAttachments			(const vk::DeviceInterface& vk, const vk::VkDevice device);
vk::Move<vk::VkFramebuffer>		makeFramebuffer								(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkRenderPass renderPass, const vk::VkImageView colorAttachment, const deUint32 width, const deUint32 height, const deUint32 layers);
vk::Move<vk::VkFramebuffer>		makeFramebufferWithoutAttachments			(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkRenderPass renderPass);
vk::Move<vk::VkImageView>		makeImageView								(const vk::DeviceInterface& vk, const vk::VkDevice vkDevice, const vk::VkImage image, const vk::VkImageViewType viewType, const vk::VkFormat format, const vk::VkImageSubresourceRange subresourceRange);
vk::VkBufferImageCopy			makeBufferImageCopy							(const vk::VkExtent3D extent, const vk::VkImageSubresourceLayers subresourceLayers);
vk::VkBufferMemoryBarrier		makeBufferMemoryBarrier						(const vk::VkAccessFlags srcAccessMask, const vk::VkAccessFlags dstAccessMask, const vk::VkBuffer buffer, const vk::VkDeviceSize offset, const vk::VkDeviceSize bufferSizeBytes);
vk::VkImageMemoryBarrier		makeImageMemoryBarrier						(const vk::VkAccessFlags srcAccessMask, const vk::VkAccessFlags dstAccessMask, const vk::VkImageLayout oldLayout, const vk::VkImageLayout newLayout, const vk::VkImage image, const vk::VkImageSubresourceRange subresourceRange);

void							beginCommandBuffer							(const vk::DeviceInterface& vk, const vk::VkCommandBuffer commandBuffer);
void							endCommandBuffer							(const vk::DeviceInterface& vk, const vk::VkCommandBuffer commandBuffer);
void							submitCommandsAndWait						(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkQueue queue, const vk::VkCommandBuffer commandBuffer);
void							beginRenderPass								(const vk::DeviceInterface& vk, const vk::VkCommandBuffer commandBuffer, const vk::VkRenderPass renderPass, const vk::VkFramebuffer framebuffer, const vk::VkRect2D& renderArea, const tcu::Vec4& clearColor);
void							beginRenderPassWithRasterizationDisabled	(const vk::DeviceInterface& vk, const vk::VkCommandBuffer commandBuffer, const vk::VkRenderPass renderPass, const vk::VkFramebuffer framebuffer);
void							endRenderPass								(const vk::DeviceInterface& vk, const vk::VkCommandBuffer commandBuffer);
void							requireFeatures								(const vk::InstanceInterface& vki, const vk::VkPhysicalDevice physDevice, const FeatureFlags flags);

float							getClampedTessLevel							(const SpacingMode mode, const float tessLevel);
int								getRoundedTessLevel							(const SpacingMode mode, const float clampedTessLevel);
int								getClampedRoundedTessLevel					(const SpacingMode mode, const float tessLevel);
void							getClampedRoundedTriangleTessLevels			(const SpacingMode mode, const float* innerSrc, const float* outerSrc, int* innerDst, int* outerDst);
void							getClampedRoundedQuadTessLevels				(const SpacingMode mode, const float* innerSrc, const float* outerSrc, int* innerDst, int* outerDst);
void							getClampedRoundedIsolineTessLevels			(const SpacingMode mode, const float* outerSrc, int* outerDst);
int								numOuterTessellationLevels					(const TessPrimitiveType primitiveType);
std::string						getTessellationLevelsString					(const TessLevels& tessLevels, const TessPrimitiveType primitiveType);
std::string						getTessellationLevelsString					(const float* inner, const float* outer);
bool							isPatchDiscarded							(const TessPrimitiveType primitiveType, const float* outerLevels);
std::vector<tcu::Vec3>			generateReferenceTriangleTessCoords			(const SpacingMode spacingMode, const int inner, const int outer0, const int outer1, const int outer2);
std::vector<tcu::Vec3>			generateReferenceQuadTessCoords				(const SpacingMode spacingMode, const int inner0, const int inner1, const int outer0, const int outer1, const int outer2, const int outer3);
std::vector<tcu::Vec3>			generateReferenceIsolineTessCoords			(const int outer0, const int outer1);
int								referenceVertexCount						(const TessPrimitiveType primitiveType, const SpacingMode spacingMode, const bool usePointMode, const float* innerLevels, const float* outerLevels);
int								referencePrimitiveCount						(const TessPrimitiveType primitiveType, const SpacingMode spacingMode, const bool usePointMode, const float* innerLevels, const float* outerLevels);
int								numVerticesPerPrimitive						(const TessPrimitiveType primitiveType, const bool usePointMode);

static inline const char* getTessPrimitiveTypeShaderName (const TessPrimitiveType type)
{
	switch (type)
	{
		case TESSPRIMITIVETYPE_TRIANGLES:	return "triangles";
		case TESSPRIMITIVETYPE_QUADS:		return "quads";
		case TESSPRIMITIVETYPE_ISOLINES:	return "isolines";
		default:
			DE_FATAL("Unexpected primitive type.");
			return DE_NULL;
	}
}

static inline const char* getDomainName (const TessPrimitiveType type)
{
	switch (type)
	{
		case TESSPRIMITIVETYPE_TRIANGLES:	return "tri";
		case TESSPRIMITIVETYPE_QUADS:		return "quad";
		case TESSPRIMITIVETYPE_ISOLINES:	return "isoline";
		default:
			DE_FATAL("Unexpected primitive type.");
			return DE_NULL;
	}
}

static inline const char* getOutputTopologyName (const TessPrimitiveType type, const Winding winding, const bool usePointMode)
{
	if (usePointMode)
		return "point";
	else if (type == TESSPRIMITIVETYPE_TRIANGLES || type == TESSPRIMITIVETYPE_QUADS)
		return (winding == WINDING_CCW ? "triangle_ccw" : "triangle_cw");
	else if (type == TESSPRIMITIVETYPE_ISOLINES)
		return "line";

	DE_FATAL("Unexpected primitive type.");
	return DE_NULL;
}

static inline const char* getSpacingModeShaderName (SpacingMode mode)
{
	switch (mode)
	{
		case SPACINGMODE_EQUAL:				return "equal_spacing";
		case SPACINGMODE_FRACTIONAL_ODD:	return "fractional_odd_spacing";
		case SPACINGMODE_FRACTIONAL_EVEN:	return "fractional_even_spacing";
		default:
			DE_FATAL("Unexpected spacing mode.");
			return DE_NULL;
	}
}

static inline const char* getPartitioningShaderName (SpacingMode mode)
{
	switch (mode)
	{
		case SPACINGMODE_EQUAL:				return "integer";
		case SPACINGMODE_FRACTIONAL_ODD:	return "fractional_odd";
		case SPACINGMODE_FRACTIONAL_EVEN:	return "fractional_even";
		default:
			DE_FATAL("Unexpected spacing mode.");
			return DE_NULL;
	}
}

static inline const char* getWindingShaderName (const Winding winding)
{
	switch (winding)
	{
		case WINDING_CCW:	return "ccw";
		case WINDING_CW:	return "cw";
		default:
			DE_FATAL("Unexpected winding type.");
			return DE_NULL;
	}
}

static inline const char* getShaderLanguageName (const ShaderLanguage language)
{
	switch (language)
	{
		case SHADER_LANGUAGE_GLSL:	return "glsl";
		case SHADER_LANGUAGE_HLSL:	return "hlsl";
		default:
			DE_FATAL("Unexpected shader language.");
			return DE_NULL;
	}
}

static inline const char* getGeometryShaderInputPrimitiveTypeShaderName (const TessPrimitiveType type, const bool usePointMode)
{
	if (usePointMode)
		return "points";

	switch (type)
	{
		case TESSPRIMITIVETYPE_TRIANGLES:
		case TESSPRIMITIVETYPE_QUADS:
			return "triangles";

		case TESSPRIMITIVETYPE_ISOLINES:
			return "lines";

		default:
			DE_FATAL("Unexpected primitive type.");
			return DE_NULL;
	}
}

static inline const char* getGeometryShaderOutputPrimitiveTypeShaderName (const TessPrimitiveType type, const bool usePointMode)
{
	if (usePointMode)
		return "points";

	switch (type)
	{
		case TESSPRIMITIVETYPE_TRIANGLES:
		case TESSPRIMITIVETYPE_QUADS:
			return "triangle_strip";

		case TESSPRIMITIVETYPE_ISOLINES:
			return "line_strip";

		default:
			DE_FATAL("Unexpected primitive type.");
			return DE_NULL;
	}
}

template<typename T>
inline std::size_t sizeInBytes (const std::vector<T>& vec)
{
	return vec.size() * sizeof(vec[0]);
}

template <typename T>
static std::vector<T> sorted (const std::vector<T>& unsorted)
{
	std::vector<T> result = unsorted;
	std::sort(result.begin(), result.end());
	return result;
}

template <typename T, typename P>
static std::vector<T> sorted (const std::vector<T>& unsorted, P pred)
{
	std::vector<T> result = unsorted;
	std::sort(result.begin(), result.end(), pred);
	return result;
}

template <typename IterT>
std::string elemsStr (const IterT& begin, const IterT& end, int wrapLengthParam = 0, int numIndentationSpaces = 0)
{
	const int			bigInt			= ~0u/2;
	const std::string	baseIndentation	= std::string(numIndentationSpaces, ' ');
	const std::string	deepIndentation	= baseIndentation + std::string(4, ' ');
	const int			wrapLength		= wrapLengthParam > 0 ? wrapLengthParam : bigInt;
	const int			length			= static_cast<int>(std::distance(begin, end));
	std::string			result;

	if (length > wrapLength)
		result += "(amount: " + de::toString(length) + ") ";
	result += std::string() + "{" + (length > wrapLength ? "\n"+deepIndentation : " ");

	{
		int index = 0;
		for (IterT it = begin; it != end; ++it)
		{
			if (it != begin)
				result += std::string() + ", " + (index % wrapLength == 0 ? "\n"+deepIndentation : "");
			result += de::toString(*it);
			index++;
		}

		result += length > wrapLength ? "\n"+baseIndentation : " ";
	}

	result += "}";
	return result;
}

template <typename ContainerT>
std::string containerStr (const ContainerT& c, int wrapLengthParam = 0, int numIndentationSpaces = 0)
{
	return elemsStr(c.begin(), c.end(), wrapLengthParam, numIndentationSpaces);
}

//! Copy 'count' objects of type T from 'memory' into a vector.
//! 'offset' is the offset of first object in memory, and 'stride' is the distance between consecutive objects.
template<typename T>
std::vector<T> readInterleavedData (const int count, const void* memory, const int offset, const int stride)
{
	std::vector<T> results(count);
	const deUint8* pData = static_cast<const deUint8*>(memory) + offset;

	for (int i = 0; i < count; ++i)
	{
		deMemcpy(&results[i], pData, sizeof(T));
		pData += stride;
	}

	return results;
}

} // tessellation
} // vkt

#endif // _VKTTESSELLATIONUTIL_HPP
