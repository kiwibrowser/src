#ifndef _VKTPIPELINESPECCONSTANTUTIL_HPP
#define _VKTPIPELINESPECCONSTANTUTIL_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
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
 * \brief Pipeline specialization constants test utilities
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "vkRef.hpp"
#include "vkPrograms.hpp"
#include "vkMemUtil.hpp"
#include "vkRefUtil.hpp"
#include "vkQueryUtil.hpp"

namespace vkt
{
namespace pipeline
{

class GraphicsPipelineBuilder
{
public:
														GraphicsPipelineBuilder			(void) : m_renderSize		(16, 16)
																							   , m_shaderStageFlags	(0u) {}

	GraphicsPipelineBuilder&							setRenderSize					(const tcu::IVec2& size) { m_renderSize = size; return *this; }
	GraphicsPipelineBuilder&							setShader						(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkShaderStageFlagBits stage, const vk::ProgramBinary& binary, const vk::VkSpecializationInfo* specInfo);
	vk::Move<vk::VkPipeline>							build							(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkPipelineLayout pipelineLayout, const vk::VkRenderPass renderPass);

private:
	tcu::IVec2											m_renderSize;
	vk::Move<vk::VkShaderModule>						m_vertexShaderModule;
	vk::Move<vk::VkShaderModule>						m_fragmentShaderModule;
	vk::Move<vk::VkShaderModule>						m_geometryShaderModule;
	vk::Move<vk::VkShaderModule>						m_tessControlShaderModule;
	vk::Move<vk::VkShaderModule>						m_tessEvaluationShaderModule;
	std::vector<vk::VkPipelineShaderStageCreateInfo>	m_shaderStages;
	vk::VkShaderStageFlags								m_shaderStageFlags;

														GraphicsPipelineBuilder			(const GraphicsPipelineBuilder&); // "deleted"
	GraphicsPipelineBuilder&							operator=						(const GraphicsPipelineBuilder&);
};

enum FeatureFlagBits
{
	FEATURE_TESSELLATION_SHADER					= 1u << 0,
	FEATURE_GEOMETRY_SHADER						= 1u << 1,
	FEATURE_SHADER_FLOAT_64						= 1u << 2,
	FEATURE_VERTEX_PIPELINE_STORES_AND_ATOMICS	= 1u << 3,
	FEATURE_FRAGMENT_STORES_AND_ATOMICS			= 1u << 4,
};
typedef deUint32 FeatureFlags;

vk::VkImageCreateInfo			makeImageCreateInfo		(const tcu::IVec2& size, const vk::VkFormat format, const vk::VkImageUsageFlags usage);
vk::Move<vk::VkRenderPass>		makeRenderPass			(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkFormat colorFormat);
void							beginRenderPass			(const vk::DeviceInterface& vk, const vk::VkCommandBuffer commandBuffer, const vk::VkRenderPass renderPass, const vk::VkFramebuffer framebuffer, const vk::VkRect2D& renderArea, const tcu::Vec4& clearColor);
void							requireFeatures			(const vk::InstanceInterface& vki, const vk::VkPhysicalDevice physDevice, const FeatureFlags flags);

// Ugly, brute-force replacement for the initializer list

template<typename T>
std::vector<T> makeVector (const T& o1)
{
	std::vector<T> vec;
	vec.reserve(1);
	vec.push_back(o1);
	return vec;
}

template<typename T>
std::vector<T> makeVector (const T& o1, const T& o2)
{
	std::vector<T> vec;
	vec.reserve(2);
	vec.push_back(o1);
	vec.push_back(o2);
	return vec;
}

template<typename T>
std::vector<T> makeVector (const T& o1, const T& o2, const T& o3)
{
	std::vector<T> vec;
	vec.reserve(3);
	vec.push_back(o1);
	vec.push_back(o2);
	vec.push_back(o3);
	return vec;
}

template<typename T>
std::vector<T> makeVector (const T& o1, const T& o2, const T& o3, const T& o4)
{
	std::vector<T> vec;
	vec.reserve(4);
	vec.push_back(o1);
	vec.push_back(o2);
	vec.push_back(o3);
	vec.push_back(o4);
	return vec;
}

} // pipeline
} // vkt

#endif // _VKTPIPELINESPECCONSTANTUTIL_HPP
