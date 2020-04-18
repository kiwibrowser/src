/*
** Copyright (c) 2015-2016 The Khronos Group Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/*
** This header is generated from the Khronos Vulkan XML API Registry.
**
*/

#ifndef PARAMETER_VALIDATION_H
#define PARAMETER_VALIDATION_H 1

#include <string>

#include "vulkan/vulkan.h"
#include "vk_layer_extension_utils.h"
#include "parameter_validation_utils.h"

#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(x) (void)(x)
#endif // UNUSED_PARAMETER

namespace parameter_validation {

const uint32_t GeneratedHeaderVersion = 31;

const VkAccessFlags AllVkAccessFlagBits = VK_ACCESS_INDIRECT_COMMAND_READ_BIT|VK_ACCESS_INDEX_READ_BIT|VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT|VK_ACCESS_UNIFORM_READ_BIT|VK_ACCESS_INPUT_ATTACHMENT_READ_BIT|VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT|VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_TRANSFER_WRITE_BIT|VK_ACCESS_HOST_READ_BIT|VK_ACCESS_HOST_WRITE_BIT|VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;
const VkAttachmentDescriptionFlags AllVkAttachmentDescriptionFlagBits = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
const VkBufferCreateFlags AllVkBufferCreateFlagBits = VK_BUFFER_CREATE_SPARSE_BINDING_BIT|VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT|VK_BUFFER_CREATE_SPARSE_ALIASED_BIT;
const VkBufferUsageFlags AllVkBufferUsageFlagBits = VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT|VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
const VkColorComponentFlags AllVkColorComponentFlagBits = VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
const VkCommandBufferResetFlags AllVkCommandBufferResetFlagBits = VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT;
const VkCommandBufferUsageFlags AllVkCommandBufferUsageFlagBits = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT|VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT|VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
const VkCommandPoolCreateFlags AllVkCommandPoolCreateFlagBits = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT|VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
const VkCommandPoolResetFlags AllVkCommandPoolResetFlagBits = VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT;
const VkCullModeFlags AllVkCullModeFlagBits = VK_CULL_MODE_NONE|VK_CULL_MODE_FRONT_BIT|VK_CULL_MODE_BACK_BIT|VK_CULL_MODE_FRONT_AND_BACK;
const VkDependencyFlags AllVkDependencyFlagBits = VK_DEPENDENCY_BY_REGION_BIT;
const VkDescriptorPoolCreateFlags AllVkDescriptorPoolCreateFlagBits = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
const VkFenceCreateFlags AllVkFenceCreateFlagBits = VK_FENCE_CREATE_SIGNALED_BIT;
const VkFormatFeatureFlags AllVkFormatFeatureFlagBits = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT|VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT|VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT|VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT|VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT|VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT|VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT|VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT|VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_FORMAT_FEATURE_BLIT_SRC_BIT|VK_FORMAT_FEATURE_BLIT_DST_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG;
const VkImageAspectFlags AllVkImageAspectFlagBits = VK_IMAGE_ASPECT_COLOR_BIT|VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT|VK_IMAGE_ASPECT_METADATA_BIT;
const VkImageCreateFlags AllVkImageCreateFlagBits = VK_IMAGE_CREATE_SPARSE_BINDING_BIT|VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT|VK_IMAGE_CREATE_SPARSE_ALIASED_BIT|VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT|VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
const VkImageUsageFlags AllVkImageUsageFlagBits = VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT|VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
const VkMemoryHeapFlags AllVkMemoryHeapFlagBits = VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;
const VkMemoryPropertyFlags AllVkMemoryPropertyFlagBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_CACHED_BIT|VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
const VkPipelineCreateFlags AllVkPipelineCreateFlagBits = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT|VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT|VK_PIPELINE_CREATE_DERIVATIVE_BIT;
const VkPipelineStageFlags AllVkPipelineStageFlagBits = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT|VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT|VK_PIPELINE_STAGE_VERTEX_INPUT_BIT|VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT|VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT|VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT|VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT|VK_PIPELINE_STAGE_HOST_BIT|VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT|VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
const VkQueryControlFlags AllVkQueryControlFlagBits = VK_QUERY_CONTROL_PRECISE_BIT;
const VkQueryPipelineStatisticFlags AllVkQueryPipelineStatisticFlagBits = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT|VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT|VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT|VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT|VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT|VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT|VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT|VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT|VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT|VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT|VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
const VkQueryResultFlags AllVkQueryResultFlagBits = VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WAIT_BIT|VK_QUERY_RESULT_WITH_AVAILABILITY_BIT|VK_QUERY_RESULT_PARTIAL_BIT;
const VkQueueFlags AllVkQueueFlagBits = VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT|VK_QUEUE_TRANSFER_BIT|VK_QUEUE_SPARSE_BINDING_BIT;
const VkSampleCountFlags AllVkSampleCountFlagBits = VK_SAMPLE_COUNT_1_BIT|VK_SAMPLE_COUNT_2_BIT|VK_SAMPLE_COUNT_4_BIT|VK_SAMPLE_COUNT_8_BIT|VK_SAMPLE_COUNT_16_BIT|VK_SAMPLE_COUNT_32_BIT|VK_SAMPLE_COUNT_64_BIT;
const VkShaderStageFlags AllVkShaderStageFlagBits = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT|VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT|VK_SHADER_STAGE_GEOMETRY_BIT|VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_COMPUTE_BIT|VK_SHADER_STAGE_ALL_GRAPHICS|VK_SHADER_STAGE_ALL;
const VkSparseImageFormatFlags AllVkSparseImageFormatFlagBits = VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT|VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT|VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT;
const VkSparseMemoryBindFlags AllVkSparseMemoryBindFlagBits = VK_SPARSE_MEMORY_BIND_METADATA_BIT;
const VkStencilFaceFlags AllVkStencilFaceFlagBits = VK_STENCIL_FACE_FRONT_BIT|VK_STENCIL_FACE_BACK_BIT|VK_STENCIL_FRONT_AND_BACK;

static bool parameter_validation_vkCreateInstance(
    debug_report_data*                          report_data,
    const VkInstanceCreateInfo*                 pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkInstance*                                 pInstance)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateInstance", "pCreateInfo", "VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_reserved_flags(report_data, "vkCreateInstance", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_struct_type(report_data, "vkCreateInstance", "pCreateInfo->pApplicationInfo", "VK_STRUCTURE_TYPE_APPLICATION_INFO", pCreateInfo->pApplicationInfo, VK_STRUCTURE_TYPE_APPLICATION_INFO, false);

        if (pCreateInfo->pApplicationInfo != NULL)
        {
            skipCall |= validate_struct_pnext(report_data, "vkCreateInstance", "pCreateInfo->pApplicationInfo->pNext", NULL, pCreateInfo->pApplicationInfo->pNext, 0, NULL, GeneratedHeaderVersion);
        }

        skipCall |= validate_string_array(report_data, "vkCreateInstance", "pCreateInfo->enabledLayerCount", "pCreateInfo->ppEnabledLayerNames", pCreateInfo->enabledLayerCount, pCreateInfo->ppEnabledLayerNames, false, true);

        skipCall |= validate_string_array(report_data, "vkCreateInstance", "pCreateInfo->enabledExtensionCount", "pCreateInfo->ppEnabledExtensionNames", pCreateInfo->enabledExtensionCount, pCreateInfo->ppEnabledExtensionNames, false, true);
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateInstance", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateInstance", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateInstance", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateInstance", "pInstance", pInstance);

    return skipCall;
}

static bool parameter_validation_vkDestroyInstance(
    debug_report_data*                          report_data,
    const VkAllocationCallbacks*                pAllocator)
{
    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroyInstance", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyInstance", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyInstance", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkEnumeratePhysicalDevices(
    debug_report_data*                          report_data,
    uint32_t*                                   pPhysicalDeviceCount,
    VkPhysicalDevice*                           pPhysicalDevices)
{
    bool skipCall = false;

    skipCall |= validate_array(report_data, "vkEnumeratePhysicalDevices", "pPhysicalDeviceCount", "pPhysicalDevices", pPhysicalDeviceCount, pPhysicalDevices, true, false, false);

    return skipCall;
}

static bool parameter_validation_vkGetPhysicalDeviceFeatures(
    debug_report_data*                          report_data,
    VkPhysicalDeviceFeatures*                   pFeatures)
{
    bool skipCall = false;

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceFeatures", "pFeatures", pFeatures);

    return skipCall;
}

static bool parameter_validation_vkGetPhysicalDeviceFormatProperties(
    debug_report_data*                          report_data,
    VkFormat                                    format,
    VkFormatProperties*                         pFormatProperties)
{
    bool skipCall = false;

    skipCall |= validate_ranged_enum(report_data, "vkGetPhysicalDeviceFormatProperties", "format", "VkFormat", VK_FORMAT_BEGIN_RANGE, VK_FORMAT_END_RANGE, format);

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceFormatProperties", "pFormatProperties", pFormatProperties);

    return skipCall;
}

static bool parameter_validation_vkGetPhysicalDeviceImageFormatProperties(
    debug_report_data*                          report_data,
    VkFormat                                    format,
    VkImageType                                 type,
    VkImageTiling                               tiling,
    VkImageUsageFlags                           usage,
    VkImageCreateFlags                          flags,
    VkImageFormatProperties*                    pImageFormatProperties)
{
    bool skipCall = false;

    skipCall |= validate_ranged_enum(report_data, "vkGetPhysicalDeviceImageFormatProperties", "format", "VkFormat", VK_FORMAT_BEGIN_RANGE, VK_FORMAT_END_RANGE, format);

    skipCall |= validate_ranged_enum(report_data, "vkGetPhysicalDeviceImageFormatProperties", "type", "VkImageType", VK_IMAGE_TYPE_BEGIN_RANGE, VK_IMAGE_TYPE_END_RANGE, type);

    skipCall |= validate_ranged_enum(report_data, "vkGetPhysicalDeviceImageFormatProperties", "tiling", "VkImageTiling", VK_IMAGE_TILING_BEGIN_RANGE, VK_IMAGE_TILING_END_RANGE, tiling);

    skipCall |= validate_flags(report_data, "vkGetPhysicalDeviceImageFormatProperties", "usage", "VkImageUsageFlagBits", AllVkImageUsageFlagBits, usage, true);

    skipCall |= validate_flags(report_data, "vkGetPhysicalDeviceImageFormatProperties", "flags", "VkImageCreateFlagBits", AllVkImageCreateFlagBits, flags, false);

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceImageFormatProperties", "pImageFormatProperties", pImageFormatProperties);

    return skipCall;
}

static bool parameter_validation_vkGetPhysicalDeviceProperties(
    debug_report_data*                          report_data,
    VkPhysicalDeviceProperties*                 pProperties)
{
    bool skipCall = false;

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceProperties", "pProperties", pProperties);

    return skipCall;
}

static bool parameter_validation_vkGetPhysicalDeviceQueueFamilyProperties(
    debug_report_data*                          report_data,
    uint32_t*                                   pQueueFamilyPropertyCount,
    VkQueueFamilyProperties*                    pQueueFamilyProperties)
{
    bool skipCall = false;

    skipCall |= validate_array(report_data, "vkGetPhysicalDeviceQueueFamilyProperties", "pQueueFamilyPropertyCount", "pQueueFamilyProperties", pQueueFamilyPropertyCount, pQueueFamilyProperties, true, false, false);

    return skipCall;
}

static bool parameter_validation_vkGetPhysicalDeviceMemoryProperties(
    debug_report_data*                          report_data,
    VkPhysicalDeviceMemoryProperties*           pMemoryProperties)
{
    bool skipCall = false;

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceMemoryProperties", "pMemoryProperties", pMemoryProperties);

    return skipCall;
}

static bool parameter_validation_vkCreateDevice(
    debug_report_data*                          report_data,
    const VkDeviceCreateInfo*                   pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDevice*                                   pDevice)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateDevice", "pCreateInfo", "VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_reserved_flags(report_data, "vkCreateDevice", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_struct_type_array(report_data, "vkCreateDevice", "pCreateInfo->queueCreateInfoCount", "pCreateInfo->pQueueCreateInfos", "VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO", pCreateInfo->queueCreateInfoCount, pCreateInfo->pQueueCreateInfos, VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, true, true);

        if (pCreateInfo->pQueueCreateInfos != NULL)
        {
            for (uint32_t queueCreateInfoIndex = 0; queueCreateInfoIndex < pCreateInfo->queueCreateInfoCount; ++queueCreateInfoIndex)
            {
                skipCall |= validate_struct_pnext(report_data, "vkCreateDevice", ParameterName("pCreateInfo->pQueueCreateInfos[%i].pNext", ParameterName::IndexVector{ queueCreateInfoIndex }), NULL, pCreateInfo->pQueueCreateInfos[queueCreateInfoIndex].pNext, 0, NULL, GeneratedHeaderVersion);

                skipCall |= validate_reserved_flags(report_data, "vkCreateDevice", ParameterName("pCreateInfo->pQueueCreateInfos[%i].flags", ParameterName::IndexVector{ queueCreateInfoIndex }), pCreateInfo->pQueueCreateInfos[queueCreateInfoIndex].flags);

                skipCall |= validate_array(report_data, "vkCreateDevice", ParameterName("pCreateInfo->pQueueCreateInfos[%i].queueCount", ParameterName::IndexVector{ queueCreateInfoIndex }), ParameterName("pCreateInfo->pQueueCreateInfos[%i].pQueuePriorities", ParameterName::IndexVector{ queueCreateInfoIndex }), pCreateInfo->pQueueCreateInfos[queueCreateInfoIndex].queueCount, pCreateInfo->pQueueCreateInfos[queueCreateInfoIndex].pQueuePriorities, true, true);
            }
        }

        skipCall |= validate_string_array(report_data, "vkCreateDevice", "pCreateInfo->enabledLayerCount", "pCreateInfo->ppEnabledLayerNames", pCreateInfo->enabledLayerCount, pCreateInfo->ppEnabledLayerNames, false, true);

        skipCall |= validate_string_array(report_data, "vkCreateDevice", "pCreateInfo->enabledExtensionCount", "pCreateInfo->ppEnabledExtensionNames", pCreateInfo->enabledExtensionCount, pCreateInfo->ppEnabledExtensionNames, false, true);

        if (pCreateInfo->pEnabledFeatures != NULL)
        {
            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->robustBufferAccess", pCreateInfo->pEnabledFeatures->robustBufferAccess);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->fullDrawIndexUint32", pCreateInfo->pEnabledFeatures->fullDrawIndexUint32);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->imageCubeArray", pCreateInfo->pEnabledFeatures->imageCubeArray);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->independentBlend", pCreateInfo->pEnabledFeatures->independentBlend);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->geometryShader", pCreateInfo->pEnabledFeatures->geometryShader);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->tessellationShader", pCreateInfo->pEnabledFeatures->tessellationShader);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->sampleRateShading", pCreateInfo->pEnabledFeatures->sampleRateShading);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->dualSrcBlend", pCreateInfo->pEnabledFeatures->dualSrcBlend);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->logicOp", pCreateInfo->pEnabledFeatures->logicOp);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->multiDrawIndirect", pCreateInfo->pEnabledFeatures->multiDrawIndirect);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->drawIndirectFirstInstance", pCreateInfo->pEnabledFeatures->drawIndirectFirstInstance);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->depthClamp", pCreateInfo->pEnabledFeatures->depthClamp);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->depthBiasClamp", pCreateInfo->pEnabledFeatures->depthBiasClamp);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->fillModeNonSolid", pCreateInfo->pEnabledFeatures->fillModeNonSolid);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->depthBounds", pCreateInfo->pEnabledFeatures->depthBounds);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->wideLines", pCreateInfo->pEnabledFeatures->wideLines);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->largePoints", pCreateInfo->pEnabledFeatures->largePoints);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->alphaToOne", pCreateInfo->pEnabledFeatures->alphaToOne);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->multiViewport", pCreateInfo->pEnabledFeatures->multiViewport);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->samplerAnisotropy", pCreateInfo->pEnabledFeatures->samplerAnisotropy);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->textureCompressionETC2", pCreateInfo->pEnabledFeatures->textureCompressionETC2);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->textureCompressionASTC_LDR", pCreateInfo->pEnabledFeatures->textureCompressionASTC_LDR);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->textureCompressionBC", pCreateInfo->pEnabledFeatures->textureCompressionBC);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->occlusionQueryPrecise", pCreateInfo->pEnabledFeatures->occlusionQueryPrecise);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->pipelineStatisticsQuery", pCreateInfo->pEnabledFeatures->pipelineStatisticsQuery);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->vertexPipelineStoresAndAtomics", pCreateInfo->pEnabledFeatures->vertexPipelineStoresAndAtomics);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->fragmentStoresAndAtomics", pCreateInfo->pEnabledFeatures->fragmentStoresAndAtomics);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->shaderTessellationAndGeometryPointSize", pCreateInfo->pEnabledFeatures->shaderTessellationAndGeometryPointSize);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->shaderImageGatherExtended", pCreateInfo->pEnabledFeatures->shaderImageGatherExtended);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->shaderStorageImageExtendedFormats", pCreateInfo->pEnabledFeatures->shaderStorageImageExtendedFormats);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->shaderStorageImageMultisample", pCreateInfo->pEnabledFeatures->shaderStorageImageMultisample);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->shaderStorageImageReadWithoutFormat", pCreateInfo->pEnabledFeatures->shaderStorageImageReadWithoutFormat);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->shaderStorageImageWriteWithoutFormat", pCreateInfo->pEnabledFeatures->shaderStorageImageWriteWithoutFormat);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->shaderUniformBufferArrayDynamicIndexing", pCreateInfo->pEnabledFeatures->shaderUniformBufferArrayDynamicIndexing);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->shaderSampledImageArrayDynamicIndexing", pCreateInfo->pEnabledFeatures->shaderSampledImageArrayDynamicIndexing);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->shaderStorageBufferArrayDynamicIndexing", pCreateInfo->pEnabledFeatures->shaderStorageBufferArrayDynamicIndexing);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->shaderStorageImageArrayDynamicIndexing", pCreateInfo->pEnabledFeatures->shaderStorageImageArrayDynamicIndexing);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->shaderClipDistance", pCreateInfo->pEnabledFeatures->shaderClipDistance);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->shaderCullDistance", pCreateInfo->pEnabledFeatures->shaderCullDistance);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->shaderFloat64", pCreateInfo->pEnabledFeatures->shaderFloat64);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->shaderInt64", pCreateInfo->pEnabledFeatures->shaderInt64);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->shaderInt16", pCreateInfo->pEnabledFeatures->shaderInt16);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->shaderResourceResidency", pCreateInfo->pEnabledFeatures->shaderResourceResidency);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->shaderResourceMinLod", pCreateInfo->pEnabledFeatures->shaderResourceMinLod);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->sparseBinding", pCreateInfo->pEnabledFeatures->sparseBinding);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->sparseResidencyBuffer", pCreateInfo->pEnabledFeatures->sparseResidencyBuffer);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->sparseResidencyImage2D", pCreateInfo->pEnabledFeatures->sparseResidencyImage2D);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->sparseResidencyImage3D", pCreateInfo->pEnabledFeatures->sparseResidencyImage3D);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->sparseResidency2Samples", pCreateInfo->pEnabledFeatures->sparseResidency2Samples);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->sparseResidency4Samples", pCreateInfo->pEnabledFeatures->sparseResidency4Samples);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->sparseResidency8Samples", pCreateInfo->pEnabledFeatures->sparseResidency8Samples);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->sparseResidency16Samples", pCreateInfo->pEnabledFeatures->sparseResidency16Samples);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->sparseResidencyAliased", pCreateInfo->pEnabledFeatures->sparseResidencyAliased);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->variableMultisampleRate", pCreateInfo->pEnabledFeatures->variableMultisampleRate);

            skipCall |= validate_bool32(report_data, "vkCreateDevice", "pCreateInfo->pEnabledFeatures->inheritedQueries", pCreateInfo->pEnabledFeatures->inheritedQueries);
        }
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateDevice", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateDevice", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateDevice", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateDevice", "pDevice", pDevice);

    return skipCall;
}

static bool parameter_validation_vkDestroyDevice(
    debug_report_data*                          report_data,
    const VkAllocationCallbacks*                pAllocator)
{
    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroyDevice", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyDevice", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyDevice", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkEnumerateInstanceExtensionProperties(
    debug_report_data*                          report_data,
    uint32_t*                                   pPropertyCount,
    VkExtensionProperties*                      pProperties)
{
    bool skipCall = false;

    skipCall |= validate_array(report_data, "vkEnumerateInstanceExtensionProperties", "pPropertyCount", "pProperties", pPropertyCount, pProperties, true, false, false);

    return skipCall;
}

static bool parameter_validation_vkEnumerateDeviceExtensionProperties(
    debug_report_data*                          report_data,
    const char*                                 pLayerName,
    uint32_t*                                   pPropertyCount,
    VkExtensionProperties*                      pProperties)
{
    UNUSED_PARAMETER(pLayerName);

    bool skipCall = false;

    skipCall |= validate_array(report_data, "vkEnumerateDeviceExtensionProperties", "pPropertyCount", "pProperties", pPropertyCount, pProperties, true, false, false);

    return skipCall;
}

static bool parameter_validation_vkGetDeviceQueue(
    debug_report_data*                          report_data,
    uint32_t                                    queueFamilyIndex,
    uint32_t                                    queueIndex,
    VkQueue*                                    pQueue)
{
    UNUSED_PARAMETER(queueFamilyIndex);
    UNUSED_PARAMETER(queueIndex);

    bool skipCall = false;

    skipCall |= validate_required_pointer(report_data, "vkGetDeviceQueue", "pQueue", pQueue);

    return skipCall;
}

static bool parameter_validation_vkQueueSubmit(
    debug_report_data*                          report_data,
    uint32_t                                    submitCount,
    const VkSubmitInfo*                         pSubmits,
    VkFence                                     fence)
{
    UNUSED_PARAMETER(fence);

    bool skipCall = false;

    skipCall |= validate_struct_type_array(report_data, "vkQueueSubmit", "submitCount", "pSubmits", "VK_STRUCTURE_TYPE_SUBMIT_INFO", submitCount, pSubmits, VK_STRUCTURE_TYPE_SUBMIT_INFO, false, true);

    if (pSubmits != NULL)
    {
        for (uint32_t submitIndex = 0; submitIndex < submitCount; ++submitIndex)
        {
            skipCall |= validate_struct_pnext(report_data, "vkQueueSubmit", ParameterName("pSubmits[%i].pNext", ParameterName::IndexVector{ submitIndex }), NULL, pSubmits[submitIndex].pNext, 0, NULL, GeneratedHeaderVersion);

            skipCall |= validate_array(report_data, "vkQueueSubmit", ParameterName("pSubmits[%i].waitSemaphoreCount", ParameterName::IndexVector{ submitIndex }), ParameterName("pSubmits[%i].pWaitSemaphores", ParameterName::IndexVector{ submitIndex }), pSubmits[submitIndex].waitSemaphoreCount, pSubmits[submitIndex].pWaitSemaphores, false, true);

            skipCall |= validate_flags_array(report_data, "vkQueueSubmit", ParameterName("pSubmits[%i].waitSemaphoreCount", ParameterName::IndexVector{ submitIndex }), ParameterName("pSubmits[%i].pWaitDstStageMask", ParameterName::IndexVector{ submitIndex }), "VkPipelineStageFlagBits", AllVkPipelineStageFlagBits, pSubmits[submitIndex].waitSemaphoreCount, pSubmits[submitIndex].pWaitDstStageMask, false, true);

            skipCall |= validate_array(report_data, "vkQueueSubmit", ParameterName("pSubmits[%i].commandBufferCount", ParameterName::IndexVector{ submitIndex }), ParameterName("pSubmits[%i].pCommandBuffers", ParameterName::IndexVector{ submitIndex }), pSubmits[submitIndex].commandBufferCount, pSubmits[submitIndex].pCommandBuffers, false, true);

            skipCall |= validate_array(report_data, "vkQueueSubmit", ParameterName("pSubmits[%i].signalSemaphoreCount", ParameterName::IndexVector{ submitIndex }), ParameterName("pSubmits[%i].pSignalSemaphores", ParameterName::IndexVector{ submitIndex }), pSubmits[submitIndex].signalSemaphoreCount, pSubmits[submitIndex].pSignalSemaphores, false, true);
        }
    }

    return skipCall;
}

static bool parameter_validation_vkAllocateMemory(
    debug_report_data*                          report_data,
    const VkMemoryAllocateInfo*                 pAllocateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDeviceMemory*                             pMemory)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkAllocateMemory", "pAllocateInfo", "VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO", pAllocateInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, true);

    if (pAllocateInfo != NULL)
    {
        const VkStructureType allowedStructs[] = {VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV};

        skipCall |= validate_struct_pnext(report_data, "vkAllocateMemory", "pAllocateInfo->pNext", "VkDedicatedAllocationMemoryAllocateInfoNV", pAllocateInfo->pNext, ARRAY_SIZE(allowedStructs), allowedStructs, GeneratedHeaderVersion);
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkAllocateMemory", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkAllocateMemory", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkAllocateMemory", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkAllocateMemory", "pMemory", pMemory);

    return skipCall;
}

static bool parameter_validation_vkFreeMemory(
    debug_report_data*                          report_data,
    VkDeviceMemory                              memory,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(memory);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkFreeMemory", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkFreeMemory", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkFreeMemory", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkMapMemory(
    debug_report_data*                          report_data,
    VkDeviceMemory                              memory,
    VkDeviceSize                                offset,
    VkDeviceSize                                size,
    VkMemoryMapFlags                            flags,
    void**                                      ppData)
{
    UNUSED_PARAMETER(offset);
    UNUSED_PARAMETER(size);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkMapMemory", "memory", memory);

    skipCall |= validate_reserved_flags(report_data, "vkMapMemory", "flags", flags);

    skipCall |= validate_required_pointer(report_data, "vkMapMemory", "ppData", ppData);

    return skipCall;
}

static bool parameter_validation_vkUnmapMemory(
    debug_report_data*                          report_data,
    VkDeviceMemory                              memory)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkUnmapMemory", "memory", memory);

    return skipCall;
}

static bool parameter_validation_vkFlushMappedMemoryRanges(
    debug_report_data*                          report_data,
    uint32_t                                    memoryRangeCount,
    const VkMappedMemoryRange*                  pMemoryRanges)
{
    bool skipCall = false;

    skipCall |= validate_struct_type_array(report_data, "vkFlushMappedMemoryRanges", "memoryRangeCount", "pMemoryRanges", "VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE", memoryRangeCount, pMemoryRanges, VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, true, true);

    if (pMemoryRanges != NULL)
    {
        for (uint32_t memoryRangeIndex = 0; memoryRangeIndex < memoryRangeCount; ++memoryRangeIndex)
        {
            skipCall |= validate_struct_pnext(report_data, "vkFlushMappedMemoryRanges", ParameterName("pMemoryRanges[%i].pNext", ParameterName::IndexVector{ memoryRangeIndex }), NULL, pMemoryRanges[memoryRangeIndex].pNext, 0, NULL, GeneratedHeaderVersion);

            skipCall |= validate_required_handle(report_data, "vkFlushMappedMemoryRanges", ParameterName("pMemoryRanges[%i].memory", ParameterName::IndexVector{ memoryRangeIndex }), pMemoryRanges[memoryRangeIndex].memory);
        }
    }

    return skipCall;
}

static bool parameter_validation_vkInvalidateMappedMemoryRanges(
    debug_report_data*                          report_data,
    uint32_t                                    memoryRangeCount,
    const VkMappedMemoryRange*                  pMemoryRanges)
{
    bool skipCall = false;

    skipCall |= validate_struct_type_array(report_data, "vkInvalidateMappedMemoryRanges", "memoryRangeCount", "pMemoryRanges", "VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE", memoryRangeCount, pMemoryRanges, VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, true, true);

    if (pMemoryRanges != NULL)
    {
        for (uint32_t memoryRangeIndex = 0; memoryRangeIndex < memoryRangeCount; ++memoryRangeIndex)
        {
            skipCall |= validate_struct_pnext(report_data, "vkInvalidateMappedMemoryRanges", ParameterName("pMemoryRanges[%i].pNext", ParameterName::IndexVector{ memoryRangeIndex }), NULL, pMemoryRanges[memoryRangeIndex].pNext, 0, NULL, GeneratedHeaderVersion);

            skipCall |= validate_required_handle(report_data, "vkInvalidateMappedMemoryRanges", ParameterName("pMemoryRanges[%i].memory", ParameterName::IndexVector{ memoryRangeIndex }), pMemoryRanges[memoryRangeIndex].memory);
        }
    }

    return skipCall;
}

static bool parameter_validation_vkGetDeviceMemoryCommitment(
    debug_report_data*                          report_data,
    VkDeviceMemory                              memory,
    VkDeviceSize*                               pCommittedMemoryInBytes)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkGetDeviceMemoryCommitment", "memory", memory);

    skipCall |= validate_required_pointer(report_data, "vkGetDeviceMemoryCommitment", "pCommittedMemoryInBytes", pCommittedMemoryInBytes);

    return skipCall;
}

static bool parameter_validation_vkBindBufferMemory(
    debug_report_data*                          report_data,
    VkBuffer                                    buffer,
    VkDeviceMemory                              memory,
    VkDeviceSize                                memoryOffset)
{
    UNUSED_PARAMETER(memoryOffset);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkBindBufferMemory", "buffer", buffer);

    skipCall |= validate_required_handle(report_data, "vkBindBufferMemory", "memory", memory);

    return skipCall;
}

static bool parameter_validation_vkBindImageMemory(
    debug_report_data*                          report_data,
    VkImage                                     image,
    VkDeviceMemory                              memory,
    VkDeviceSize                                memoryOffset)
{
    UNUSED_PARAMETER(memoryOffset);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkBindImageMemory", "image", image);

    skipCall |= validate_required_handle(report_data, "vkBindImageMemory", "memory", memory);

    return skipCall;
}

static bool parameter_validation_vkGetBufferMemoryRequirements(
    debug_report_data*                          report_data,
    VkBuffer                                    buffer,
    VkMemoryRequirements*                       pMemoryRequirements)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkGetBufferMemoryRequirements", "buffer", buffer);

    skipCall |= validate_required_pointer(report_data, "vkGetBufferMemoryRequirements", "pMemoryRequirements", pMemoryRequirements);

    return skipCall;
}

static bool parameter_validation_vkGetImageMemoryRequirements(
    debug_report_data*                          report_data,
    VkImage                                     image,
    VkMemoryRequirements*                       pMemoryRequirements)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkGetImageMemoryRequirements", "image", image);

    skipCall |= validate_required_pointer(report_data, "vkGetImageMemoryRequirements", "pMemoryRequirements", pMemoryRequirements);

    return skipCall;
}

static bool parameter_validation_vkGetImageSparseMemoryRequirements(
    debug_report_data*                          report_data,
    VkImage                                     image,
    uint32_t*                                   pSparseMemoryRequirementCount,
    VkSparseImageMemoryRequirements*            pSparseMemoryRequirements)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkGetImageSparseMemoryRequirements", "image", image);

    skipCall |= validate_array(report_data, "vkGetImageSparseMemoryRequirements", "pSparseMemoryRequirementCount", "pSparseMemoryRequirements", pSparseMemoryRequirementCount, pSparseMemoryRequirements, true, false, false);

    return skipCall;
}

static bool parameter_validation_vkGetPhysicalDeviceSparseImageFormatProperties(
    debug_report_data*                          report_data,
    VkFormat                                    format,
    VkImageType                                 type,
    VkSampleCountFlagBits                       samples,
    VkImageUsageFlags                           usage,
    VkImageTiling                               tiling,
    uint32_t*                                   pPropertyCount,
    VkSparseImageFormatProperties*              pProperties)
{
    UNUSED_PARAMETER(samples);

    bool skipCall = false;

    skipCall |= validate_ranged_enum(report_data, "vkGetPhysicalDeviceSparseImageFormatProperties", "format", "VkFormat", VK_FORMAT_BEGIN_RANGE, VK_FORMAT_END_RANGE, format);

    skipCall |= validate_ranged_enum(report_data, "vkGetPhysicalDeviceSparseImageFormatProperties", "type", "VkImageType", VK_IMAGE_TYPE_BEGIN_RANGE, VK_IMAGE_TYPE_END_RANGE, type);

    skipCall |= validate_flags(report_data, "vkGetPhysicalDeviceSparseImageFormatProperties", "usage", "VkImageUsageFlagBits", AllVkImageUsageFlagBits, usage, true);

    skipCall |= validate_ranged_enum(report_data, "vkGetPhysicalDeviceSparseImageFormatProperties", "tiling", "VkImageTiling", VK_IMAGE_TILING_BEGIN_RANGE, VK_IMAGE_TILING_END_RANGE, tiling);

    skipCall |= validate_array(report_data, "vkGetPhysicalDeviceSparseImageFormatProperties", "pPropertyCount", "pProperties", pPropertyCount, pProperties, true, false, false);

    return skipCall;
}

static bool parameter_validation_vkQueueBindSparse(
    debug_report_data*                          report_data,
    uint32_t                                    bindInfoCount,
    const VkBindSparseInfo*                     pBindInfo,
    VkFence                                     fence)
{
    UNUSED_PARAMETER(fence);

    bool skipCall = false;

    skipCall |= validate_struct_type_array(report_data, "vkQueueBindSparse", "bindInfoCount", "pBindInfo", "VK_STRUCTURE_TYPE_BIND_SPARSE_INFO", bindInfoCount, pBindInfo, VK_STRUCTURE_TYPE_BIND_SPARSE_INFO, false, true);

    if (pBindInfo != NULL)
    {
        for (uint32_t bindInfoIndex = 0; bindInfoIndex < bindInfoCount; ++bindInfoIndex)
        {
            skipCall |= validate_struct_pnext(report_data, "vkQueueBindSparse", ParameterName("pBindInfo[%i].pNext", ParameterName::IndexVector{ bindInfoIndex }), NULL, pBindInfo[bindInfoIndex].pNext, 0, NULL, GeneratedHeaderVersion);

            skipCall |= validate_array(report_data, "vkQueueBindSparse", ParameterName("pBindInfo[%i].waitSemaphoreCount", ParameterName::IndexVector{ bindInfoIndex }), ParameterName("pBindInfo[%i].pWaitSemaphores", ParameterName::IndexVector{ bindInfoIndex }), pBindInfo[bindInfoIndex].waitSemaphoreCount, pBindInfo[bindInfoIndex].pWaitSemaphores, false, true);

            skipCall |= validate_array(report_data, "vkQueueBindSparse", ParameterName("pBindInfo[%i].bufferBindCount", ParameterName::IndexVector{ bindInfoIndex }), ParameterName("pBindInfo[%i].pBufferBinds", ParameterName::IndexVector{ bindInfoIndex }), pBindInfo[bindInfoIndex].bufferBindCount, pBindInfo[bindInfoIndex].pBufferBinds, false, true);

            if (pBindInfo[bindInfoIndex].pBufferBinds != NULL)
            {
                for (uint32_t bufferBindIndex = 0; bufferBindIndex < pBindInfo[bindInfoIndex].bufferBindCount; ++bufferBindIndex)
                {
                    skipCall |= validate_required_handle(report_data, "vkQueueBindSparse", ParameterName("pBindInfo[%i].pBufferBinds[%i].buffer", ParameterName::IndexVector{ bindInfoIndex, bufferBindIndex }), pBindInfo[bindInfoIndex].pBufferBinds[bufferBindIndex].buffer);

                    skipCall |= validate_array(report_data, "vkQueueBindSparse", ParameterName("pBindInfo[%i].pBufferBinds[%i].bindCount", ParameterName::IndexVector{ bindInfoIndex, bufferBindIndex }), ParameterName("pBindInfo[%i].pBufferBinds[%i].pBinds", ParameterName::IndexVector{ bindInfoIndex, bufferBindIndex }), pBindInfo[bindInfoIndex].pBufferBinds[bufferBindIndex].bindCount, pBindInfo[bindInfoIndex].pBufferBinds[bufferBindIndex].pBinds, true, true);

                    if (pBindInfo[bindInfoIndex].pBufferBinds[bufferBindIndex].pBinds != NULL)
                    {
                        for (uint32_t bindIndex = 0; bindIndex < pBindInfo[bindInfoIndex].pBufferBinds[bufferBindIndex].bindCount; ++bindIndex)
                        {
                            skipCall |= validate_flags(report_data, "vkQueueBindSparse", ParameterName("pBindInfo[%i].pBufferBinds[%i].pBinds[%i].flags", ParameterName::IndexVector{ bindInfoIndex, bufferBindIndex, bindIndex }), "VkSparseMemoryBindFlagBits", AllVkSparseMemoryBindFlagBits, pBindInfo[bindInfoIndex].pBufferBinds[bufferBindIndex].pBinds[bindIndex].flags, false);
                        }
                    }
                }
            }

            skipCall |= validate_array(report_data, "vkQueueBindSparse", ParameterName("pBindInfo[%i].imageOpaqueBindCount", ParameterName::IndexVector{ bindInfoIndex }), ParameterName("pBindInfo[%i].pImageOpaqueBinds", ParameterName::IndexVector{ bindInfoIndex }), pBindInfo[bindInfoIndex].imageOpaqueBindCount, pBindInfo[bindInfoIndex].pImageOpaqueBinds, false, true);

            if (pBindInfo[bindInfoIndex].pImageOpaqueBinds != NULL)
            {
                for (uint32_t imageOpaqueBindIndex = 0; imageOpaqueBindIndex < pBindInfo[bindInfoIndex].imageOpaqueBindCount; ++imageOpaqueBindIndex)
                {
                    skipCall |= validate_required_handle(report_data, "vkQueueBindSparse", ParameterName("pBindInfo[%i].pImageOpaqueBinds[%i].image", ParameterName::IndexVector{ bindInfoIndex, imageOpaqueBindIndex }), pBindInfo[bindInfoIndex].pImageOpaqueBinds[imageOpaqueBindIndex].image);

                    skipCall |= validate_array(report_data, "vkQueueBindSparse", ParameterName("pBindInfo[%i].pImageOpaqueBinds[%i].bindCount", ParameterName::IndexVector{ bindInfoIndex, imageOpaqueBindIndex }), ParameterName("pBindInfo[%i].pImageOpaqueBinds[%i].pBinds", ParameterName::IndexVector{ bindInfoIndex, imageOpaqueBindIndex }), pBindInfo[bindInfoIndex].pImageOpaqueBinds[imageOpaqueBindIndex].bindCount, pBindInfo[bindInfoIndex].pImageOpaqueBinds[imageOpaqueBindIndex].pBinds, true, true);

                    if (pBindInfo[bindInfoIndex].pImageOpaqueBinds[imageOpaqueBindIndex].pBinds != NULL)
                    {
                        for (uint32_t bindIndex = 0; bindIndex < pBindInfo[bindInfoIndex].pImageOpaqueBinds[imageOpaqueBindIndex].bindCount; ++bindIndex)
                        {
                            skipCall |= validate_flags(report_data, "vkQueueBindSparse", ParameterName("pBindInfo[%i].pImageOpaqueBinds[%i].pBinds[%i].flags", ParameterName::IndexVector{ bindInfoIndex, imageOpaqueBindIndex, bindIndex }), "VkSparseMemoryBindFlagBits", AllVkSparseMemoryBindFlagBits, pBindInfo[bindInfoIndex].pImageOpaqueBinds[imageOpaqueBindIndex].pBinds[bindIndex].flags, false);
                        }
                    }
                }
            }

            skipCall |= validate_array(report_data, "vkQueueBindSparse", ParameterName("pBindInfo[%i].imageBindCount", ParameterName::IndexVector{ bindInfoIndex }), ParameterName("pBindInfo[%i].pImageBinds", ParameterName::IndexVector{ bindInfoIndex }), pBindInfo[bindInfoIndex].imageBindCount, pBindInfo[bindInfoIndex].pImageBinds, false, true);

            if (pBindInfo[bindInfoIndex].pImageBinds != NULL)
            {
                for (uint32_t imageBindIndex = 0; imageBindIndex < pBindInfo[bindInfoIndex].imageBindCount; ++imageBindIndex)
                {
                    skipCall |= validate_required_handle(report_data, "vkQueueBindSparse", ParameterName("pBindInfo[%i].pImageBinds[%i].image", ParameterName::IndexVector{ bindInfoIndex, imageBindIndex }), pBindInfo[bindInfoIndex].pImageBinds[imageBindIndex].image);

                    skipCall |= validate_array(report_data, "vkQueueBindSparse", ParameterName("pBindInfo[%i].pImageBinds[%i].bindCount", ParameterName::IndexVector{ bindInfoIndex, imageBindIndex }), ParameterName("pBindInfo[%i].pImageBinds[%i].pBinds", ParameterName::IndexVector{ bindInfoIndex, imageBindIndex }), pBindInfo[bindInfoIndex].pImageBinds[imageBindIndex].bindCount, pBindInfo[bindInfoIndex].pImageBinds[imageBindIndex].pBinds, true, true);

                    if (pBindInfo[bindInfoIndex].pImageBinds[imageBindIndex].pBinds != NULL)
                    {
                        for (uint32_t bindIndex = 0; bindIndex < pBindInfo[bindInfoIndex].pImageBinds[imageBindIndex].bindCount; ++bindIndex)
                        {
                            skipCall |= validate_flags(report_data, "vkQueueBindSparse", ParameterName("pBindInfo[%i].pImageBinds[%i].pBinds[%i].subresource.aspectMask", ParameterName::IndexVector{ bindInfoIndex, imageBindIndex, bindIndex }), "VkImageAspectFlagBits", AllVkImageAspectFlagBits, pBindInfo[bindInfoIndex].pImageBinds[imageBindIndex].pBinds[bindIndex].subresource.aspectMask, true);

                            skipCall |= validate_flags(report_data, "vkQueueBindSparse", ParameterName("pBindInfo[%i].pImageBinds[%i].pBinds[%i].flags", ParameterName::IndexVector{ bindInfoIndex, imageBindIndex, bindIndex }), "VkSparseMemoryBindFlagBits", AllVkSparseMemoryBindFlagBits, pBindInfo[bindInfoIndex].pImageBinds[imageBindIndex].pBinds[bindIndex].flags, false);
                        }
                    }
                }
            }

            skipCall |= validate_array(report_data, "vkQueueBindSparse", ParameterName("pBindInfo[%i].signalSemaphoreCount", ParameterName::IndexVector{ bindInfoIndex }), ParameterName("pBindInfo[%i].pSignalSemaphores", ParameterName::IndexVector{ bindInfoIndex }), pBindInfo[bindInfoIndex].signalSemaphoreCount, pBindInfo[bindInfoIndex].pSignalSemaphores, false, true);
        }
    }

    return skipCall;
}

static bool parameter_validation_vkCreateFence(
    debug_report_data*                          report_data,
    const VkFenceCreateInfo*                    pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkFence*                                    pFence)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateFence", "pCreateInfo", "VK_STRUCTURE_TYPE_FENCE_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateFence", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_flags(report_data, "vkCreateFence", "pCreateInfo->flags", "VkFenceCreateFlagBits", AllVkFenceCreateFlagBits, pCreateInfo->flags, false);
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateFence", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateFence", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateFence", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateFence", "pFence", pFence);

    return skipCall;
}

static bool parameter_validation_vkDestroyFence(
    debug_report_data*                          report_data,
    VkFence                                     fence,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(fence);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroyFence", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyFence", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyFence", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkResetFences(
    debug_report_data*                          report_data,
    uint32_t                                    fenceCount,
    const VkFence*                              pFences)
{
    bool skipCall = false;

    skipCall |= validate_handle_array(report_data, "vkResetFences", "fenceCount", "pFences", fenceCount, pFences, true, true);

    return skipCall;
}

static bool parameter_validation_vkGetFenceStatus(
    debug_report_data*                          report_data,
    VkFence                                     fence)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkGetFenceStatus", "fence", fence);

    return skipCall;
}

static bool parameter_validation_vkWaitForFences(
    debug_report_data*                          report_data,
    uint32_t                                    fenceCount,
    const VkFence*                              pFences,
    VkBool32                                    waitAll,
    uint64_t                                    timeout)
{
    UNUSED_PARAMETER(timeout);

    bool skipCall = false;

    skipCall |= validate_handle_array(report_data, "vkWaitForFences", "fenceCount", "pFences", fenceCount, pFences, true, true);

    skipCall |= validate_bool32(report_data, "vkWaitForFences", "waitAll", waitAll);

    return skipCall;
}

static bool parameter_validation_vkCreateSemaphore(
    debug_report_data*                          report_data,
    const VkSemaphoreCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSemaphore*                                pSemaphore)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateSemaphore", "pCreateInfo", "VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateSemaphore", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateSemaphore", "pCreateInfo->flags", pCreateInfo->flags);
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateSemaphore", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateSemaphore", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateSemaphore", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateSemaphore", "pSemaphore", pSemaphore);

    return skipCall;
}

static bool parameter_validation_vkDestroySemaphore(
    debug_report_data*                          report_data,
    VkSemaphore                                 semaphore,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(semaphore);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroySemaphore", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroySemaphore", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroySemaphore", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkCreateEvent(
    debug_report_data*                          report_data,
    const VkEventCreateInfo*                    pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkEvent*                                    pEvent)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateEvent", "pCreateInfo", "VK_STRUCTURE_TYPE_EVENT_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_EVENT_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateEvent", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateEvent", "pCreateInfo->flags", pCreateInfo->flags);
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateEvent", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateEvent", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateEvent", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateEvent", "pEvent", pEvent);

    return skipCall;
}

static bool parameter_validation_vkDestroyEvent(
    debug_report_data*                          report_data,
    VkEvent                                     event,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(event);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroyEvent", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyEvent", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyEvent", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkGetEventStatus(
    debug_report_data*                          report_data,
    VkEvent                                     event)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkGetEventStatus", "event", event);

    return skipCall;
}

static bool parameter_validation_vkSetEvent(
    debug_report_data*                          report_data,
    VkEvent                                     event)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkSetEvent", "event", event);

    return skipCall;
}

static bool parameter_validation_vkResetEvent(
    debug_report_data*                          report_data,
    VkEvent                                     event)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkResetEvent", "event", event);

    return skipCall;
}

static bool parameter_validation_vkCreateQueryPool(
    debug_report_data*                          report_data,
    const VkQueryPoolCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkQueryPool*                                pQueryPool)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateQueryPool", "pCreateInfo", "VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateQueryPool", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateQueryPool", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_ranged_enum(report_data, "vkCreateQueryPool", "pCreateInfo->queryType", "VkQueryType", VK_QUERY_TYPE_BEGIN_RANGE, VK_QUERY_TYPE_END_RANGE, pCreateInfo->queryType);
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateQueryPool", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateQueryPool", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateQueryPool", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateQueryPool", "pQueryPool", pQueryPool);

    return skipCall;
}

static bool parameter_validation_vkDestroyQueryPool(
    debug_report_data*                          report_data,
    VkQueryPool                                 queryPool,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(queryPool);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroyQueryPool", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyQueryPool", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyQueryPool", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkGetQueryPoolResults(
    debug_report_data*                          report_data,
    VkQueryPool                                 queryPool,
    uint32_t                                    firstQuery,
    uint32_t                                    queryCount,
    size_t                                      dataSize,
    void*                                       pData,
    VkDeviceSize                                stride,
    VkQueryResultFlags                          flags)
{
    UNUSED_PARAMETER(firstQuery);
    UNUSED_PARAMETER(queryCount);
    UNUSED_PARAMETER(stride);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkGetQueryPoolResults", "queryPool", queryPool);

    skipCall |= validate_array(report_data, "vkGetQueryPoolResults", "dataSize", "pData", dataSize, pData, true, true);

    skipCall |= validate_flags(report_data, "vkGetQueryPoolResults", "flags", "VkQueryResultFlagBits", AllVkQueryResultFlagBits, flags, false);

    return skipCall;
}

static bool parameter_validation_vkCreateBuffer(
    debug_report_data*                          report_data,
    const VkBufferCreateInfo*                   pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkBuffer*                                   pBuffer)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateBuffer", "pCreateInfo", "VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        const VkStructureType allowedStructs[] = {VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV};

        skipCall |= validate_struct_pnext(report_data, "vkCreateBuffer", "pCreateInfo->pNext", "VkDedicatedAllocationBufferCreateInfoNV", pCreateInfo->pNext, ARRAY_SIZE(allowedStructs), allowedStructs, GeneratedHeaderVersion);

        skipCall |= validate_flags(report_data, "vkCreateBuffer", "pCreateInfo->flags", "VkBufferCreateFlagBits", AllVkBufferCreateFlagBits, pCreateInfo->flags, false);

        skipCall |= validate_flags(report_data, "vkCreateBuffer", "pCreateInfo->usage", "VkBufferUsageFlagBits", AllVkBufferUsageFlagBits, pCreateInfo->usage, true);

        skipCall |= validate_ranged_enum(report_data, "vkCreateBuffer", "pCreateInfo->sharingMode", "VkSharingMode", VK_SHARING_MODE_BEGIN_RANGE, VK_SHARING_MODE_END_RANGE, pCreateInfo->sharingMode);
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateBuffer", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateBuffer", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateBuffer", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateBuffer", "pBuffer", pBuffer);

    return skipCall;
}

static bool parameter_validation_vkDestroyBuffer(
    debug_report_data*                          report_data,
    VkBuffer                                    buffer,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(buffer);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroyBuffer", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyBuffer", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyBuffer", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkCreateBufferView(
    debug_report_data*                          report_data,
    const VkBufferViewCreateInfo*               pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkBufferView*                               pView)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateBufferView", "pCreateInfo", "VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateBufferView", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateBufferView", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_required_handle(report_data, "vkCreateBufferView", "pCreateInfo->buffer", pCreateInfo->buffer);

        skipCall |= validate_ranged_enum(report_data, "vkCreateBufferView", "pCreateInfo->format", "VkFormat", VK_FORMAT_BEGIN_RANGE, VK_FORMAT_END_RANGE, pCreateInfo->format);
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateBufferView", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateBufferView", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateBufferView", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateBufferView", "pView", pView);

    return skipCall;
}

static bool parameter_validation_vkDestroyBufferView(
    debug_report_data*                          report_data,
    VkBufferView                                bufferView,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(bufferView);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroyBufferView", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyBufferView", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyBufferView", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkCreateImage(
    debug_report_data*                          report_data,
    const VkImageCreateInfo*                    pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkImage*                                    pImage)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateImage", "pCreateInfo", "VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        const VkStructureType allowedStructs[] = {VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV};

        skipCall |= validate_struct_pnext(report_data, "vkCreateImage", "pCreateInfo->pNext", "VkDedicatedAllocationImageCreateInfoNV", pCreateInfo->pNext, ARRAY_SIZE(allowedStructs), allowedStructs, GeneratedHeaderVersion);

        skipCall |= validate_flags(report_data, "vkCreateImage", "pCreateInfo->flags", "VkImageCreateFlagBits", AllVkImageCreateFlagBits, pCreateInfo->flags, false);

        skipCall |= validate_ranged_enum(report_data, "vkCreateImage", "pCreateInfo->imageType", "VkImageType", VK_IMAGE_TYPE_BEGIN_RANGE, VK_IMAGE_TYPE_END_RANGE, pCreateInfo->imageType);

        skipCall |= validate_ranged_enum(report_data, "vkCreateImage", "pCreateInfo->format", "VkFormat", VK_FORMAT_BEGIN_RANGE, VK_FORMAT_END_RANGE, pCreateInfo->format);

        skipCall |= validate_ranged_enum(report_data, "vkCreateImage", "pCreateInfo->tiling", "VkImageTiling", VK_IMAGE_TILING_BEGIN_RANGE, VK_IMAGE_TILING_END_RANGE, pCreateInfo->tiling);

        skipCall |= validate_flags(report_data, "vkCreateImage", "pCreateInfo->usage", "VkImageUsageFlagBits", AllVkImageUsageFlagBits, pCreateInfo->usage, true);

        skipCall |= validate_ranged_enum(report_data, "vkCreateImage", "pCreateInfo->sharingMode", "VkSharingMode", VK_SHARING_MODE_BEGIN_RANGE, VK_SHARING_MODE_END_RANGE, pCreateInfo->sharingMode);

        skipCall |= validate_ranged_enum(report_data, "vkCreateImage", "pCreateInfo->initialLayout", "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, pCreateInfo->initialLayout);
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateImage", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateImage", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateImage", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateImage", "pImage", pImage);

    return skipCall;
}

static bool parameter_validation_vkDestroyImage(
    debug_report_data*                          report_data,
    VkImage                                     image,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(image);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroyImage", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyImage", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyImage", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkGetImageSubresourceLayout(
    debug_report_data*                          report_data,
    VkImage                                     image,
    const VkImageSubresource*                   pSubresource,
    VkSubresourceLayout*                        pLayout)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkGetImageSubresourceLayout", "image", image);

    skipCall |= validate_required_pointer(report_data, "vkGetImageSubresourceLayout", "pSubresource", pSubresource);

    if (pSubresource != NULL)
    {
        skipCall |= validate_flags(report_data, "vkGetImageSubresourceLayout", "pSubresource->aspectMask", "VkImageAspectFlagBits", AllVkImageAspectFlagBits, pSubresource->aspectMask, true);
    }

    skipCall |= validate_required_pointer(report_data, "vkGetImageSubresourceLayout", "pLayout", pLayout);

    return skipCall;
}

static bool parameter_validation_vkCreateImageView(
    debug_report_data*                          report_data,
    const VkImageViewCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkImageView*                                pView)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateImageView", "pCreateInfo", "VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateImageView", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateImageView", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_required_handle(report_data, "vkCreateImageView", "pCreateInfo->image", pCreateInfo->image);

        skipCall |= validate_ranged_enum(report_data, "vkCreateImageView", "pCreateInfo->viewType", "VkImageViewType", VK_IMAGE_VIEW_TYPE_BEGIN_RANGE, VK_IMAGE_VIEW_TYPE_END_RANGE, pCreateInfo->viewType);

        skipCall |= validate_ranged_enum(report_data, "vkCreateImageView", "pCreateInfo->format", "VkFormat", VK_FORMAT_BEGIN_RANGE, VK_FORMAT_END_RANGE, pCreateInfo->format);

        skipCall |= validate_ranged_enum(report_data, "vkCreateImageView", "pCreateInfo->components.r", "VkComponentSwizzle", VK_COMPONENT_SWIZZLE_BEGIN_RANGE, VK_COMPONENT_SWIZZLE_END_RANGE, pCreateInfo->components.r);

        skipCall |= validate_ranged_enum(report_data, "vkCreateImageView", "pCreateInfo->components.g", "VkComponentSwizzle", VK_COMPONENT_SWIZZLE_BEGIN_RANGE, VK_COMPONENT_SWIZZLE_END_RANGE, pCreateInfo->components.g);

        skipCall |= validate_ranged_enum(report_data, "vkCreateImageView", "pCreateInfo->components.b", "VkComponentSwizzle", VK_COMPONENT_SWIZZLE_BEGIN_RANGE, VK_COMPONENT_SWIZZLE_END_RANGE, pCreateInfo->components.b);

        skipCall |= validate_ranged_enum(report_data, "vkCreateImageView", "pCreateInfo->components.a", "VkComponentSwizzle", VK_COMPONENT_SWIZZLE_BEGIN_RANGE, VK_COMPONENT_SWIZZLE_END_RANGE, pCreateInfo->components.a);

        skipCall |= validate_flags(report_data, "vkCreateImageView", "pCreateInfo->subresourceRange.aspectMask", "VkImageAspectFlagBits", AllVkImageAspectFlagBits, pCreateInfo->subresourceRange.aspectMask, true);
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateImageView", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateImageView", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateImageView", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateImageView", "pView", pView);

    return skipCall;
}

static bool parameter_validation_vkDestroyImageView(
    debug_report_data*                          report_data,
    VkImageView                                 imageView,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(imageView);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroyImageView", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyImageView", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyImageView", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkCreateShaderModule(
    debug_report_data*                          report_data,
    const VkShaderModuleCreateInfo*             pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkShaderModule*                             pShaderModule)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateShaderModule", "pCreateInfo", "VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateShaderModule", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateShaderModule", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_array(report_data, "vkCreateShaderModule", "pCreateInfo->codeSize", "pCreateInfo->pCode", pCreateInfo->codeSize, pCreateInfo->pCode, true, true);
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateShaderModule", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateShaderModule", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateShaderModule", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateShaderModule", "pShaderModule", pShaderModule);

    return skipCall;
}

static bool parameter_validation_vkDestroyShaderModule(
    debug_report_data*                          report_data,
    VkShaderModule                              shaderModule,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(shaderModule);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroyShaderModule", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyShaderModule", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyShaderModule", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkCreatePipelineCache(
    debug_report_data*                          report_data,
    const VkPipelineCacheCreateInfo*            pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkPipelineCache*                            pPipelineCache)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreatePipelineCache", "pCreateInfo", "VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreatePipelineCache", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreatePipelineCache", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_array(report_data, "vkCreatePipelineCache", "pCreateInfo->initialDataSize", "pCreateInfo->pInitialData", pCreateInfo->initialDataSize, pCreateInfo->pInitialData, false, true);
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreatePipelineCache", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreatePipelineCache", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreatePipelineCache", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreatePipelineCache", "pPipelineCache", pPipelineCache);

    return skipCall;
}

static bool parameter_validation_vkDestroyPipelineCache(
    debug_report_data*                          report_data,
    VkPipelineCache                             pipelineCache,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(pipelineCache);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroyPipelineCache", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyPipelineCache", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyPipelineCache", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkGetPipelineCacheData(
    debug_report_data*                          report_data,
    VkPipelineCache                             pipelineCache,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkGetPipelineCacheData", "pipelineCache", pipelineCache);

    skipCall |= validate_array(report_data, "vkGetPipelineCacheData", "pDataSize", "pData", pDataSize, pData, true, false, false);

    return skipCall;
}

static bool parameter_validation_vkMergePipelineCaches(
    debug_report_data*                          report_data,
    VkPipelineCache                             dstCache,
    uint32_t                                    srcCacheCount,
    const VkPipelineCache*                      pSrcCaches)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkMergePipelineCaches", "dstCache", dstCache);

    skipCall |= validate_handle_array(report_data, "vkMergePipelineCaches", "srcCacheCount", "pSrcCaches", srcCacheCount, pSrcCaches, true, true);

    return skipCall;
}

static bool parameter_validation_vkCreateGraphicsPipelines(
    debug_report_data*                          report_data,
    VkPipelineCache                             pipelineCache,
    uint32_t                                    createInfoCount,
    const VkGraphicsPipelineCreateInfo*         pCreateInfos,
    const VkAllocationCallbacks*                pAllocator,
    VkPipeline*                                 pPipelines)
{
    UNUSED_PARAMETER(pipelineCache);

    bool skipCall = false;

    skipCall |= validate_struct_type_array(report_data, "vkCreateGraphicsPipelines", "createInfoCount", "pCreateInfos", "VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO", createInfoCount, pCreateInfos, VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, true, true);

    if (pCreateInfos != NULL)
    {
        for (uint32_t createInfoIndex = 0; createInfoIndex < createInfoCount; ++createInfoIndex)
        {
            skipCall |= validate_struct_pnext(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pNext", ParameterName::IndexVector{ createInfoIndex }), NULL, pCreateInfos[createInfoIndex].pNext, 0, NULL, GeneratedHeaderVersion);

            skipCall |= validate_flags(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].flags", ParameterName::IndexVector{ createInfoIndex }), "VkPipelineCreateFlagBits", AllVkPipelineCreateFlagBits, pCreateInfos[createInfoIndex].flags, false);

            skipCall |= validate_struct_type_array(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].stageCount", ParameterName::IndexVector{ createInfoIndex }), ParameterName("pCreateInfos[%i].pStages", ParameterName::IndexVector{ createInfoIndex }), "VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO", pCreateInfos[createInfoIndex].stageCount, pCreateInfos[createInfoIndex].pStages, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, true, true);

            if (pCreateInfos[createInfoIndex].pStages != NULL)
            {
                for (uint32_t stageIndex = 0; stageIndex < pCreateInfos[createInfoIndex].stageCount; ++stageIndex)
                {
                    skipCall |= validate_struct_pnext(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pStages[%i].pNext", ParameterName::IndexVector{ createInfoIndex, stageIndex }), NULL, pCreateInfos[createInfoIndex].pStages[stageIndex].pNext, 0, NULL, GeneratedHeaderVersion);

                    skipCall |= validate_reserved_flags(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pStages[%i].flags", ParameterName::IndexVector{ createInfoIndex, stageIndex }), pCreateInfos[createInfoIndex].pStages[stageIndex].flags);

                    skipCall |= validate_required_handle(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pStages[%i].module", ParameterName::IndexVector{ createInfoIndex, stageIndex }), pCreateInfos[createInfoIndex].pStages[stageIndex].module);

                    skipCall |= validate_required_pointer(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pStages[%i].pName", ParameterName::IndexVector{ createInfoIndex, stageIndex }), pCreateInfos[createInfoIndex].pStages[stageIndex].pName);

                    if (pCreateInfos[createInfoIndex].pStages[stageIndex].pSpecializationInfo != NULL)
                    {
                        skipCall |= validate_array(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pStages[%i].pSpecializationInfo->mapEntryCount", ParameterName::IndexVector{ createInfoIndex, stageIndex }), ParameterName("pCreateInfos[%i].pStages[%i].pSpecializationInfo->pMapEntries", ParameterName::IndexVector{ createInfoIndex, stageIndex }), pCreateInfos[createInfoIndex].pStages[stageIndex].pSpecializationInfo->mapEntryCount, pCreateInfos[createInfoIndex].pStages[stageIndex].pSpecializationInfo->pMapEntries, false, true);

                        skipCall |= validate_array(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pStages[%i].pSpecializationInfo->dataSize", ParameterName::IndexVector{ createInfoIndex, stageIndex }), ParameterName("pCreateInfos[%i].pStages[%i].pSpecializationInfo->pData", ParameterName::IndexVector{ createInfoIndex, stageIndex }), pCreateInfos[createInfoIndex].pStages[stageIndex].pSpecializationInfo->dataSize, pCreateInfos[createInfoIndex].pStages[stageIndex].pSpecializationInfo->pData, false, true);
                    }
                }
            }

            skipCall |= validate_struct_type(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pVertexInputState", ParameterName::IndexVector{ createInfoIndex }), "VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO", pCreateInfos[createInfoIndex].pVertexInputState, VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, true);

            if (pCreateInfos[createInfoIndex].pVertexInputState != NULL)
            {
                skipCall |= validate_struct_pnext(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pVertexInputState->pNext", ParameterName::IndexVector{ createInfoIndex }), NULL, pCreateInfos[createInfoIndex].pVertexInputState->pNext, 0, NULL, GeneratedHeaderVersion);

                skipCall |= validate_reserved_flags(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pVertexInputState->flags", ParameterName::IndexVector{ createInfoIndex }), pCreateInfos[createInfoIndex].pVertexInputState->flags);

                skipCall |= validate_array(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pVertexInputState->vertexBindingDescriptionCount", ParameterName::IndexVector{ createInfoIndex }), ParameterName("pCreateInfos[%i].pVertexInputState->pVertexBindingDescriptions", ParameterName::IndexVector{ createInfoIndex }), pCreateInfos[createInfoIndex].pVertexInputState->vertexBindingDescriptionCount, pCreateInfos[createInfoIndex].pVertexInputState->pVertexBindingDescriptions, false, true);

                if (pCreateInfos[createInfoIndex].pVertexInputState->pVertexBindingDescriptions != NULL)
                {
                    for (uint32_t vertexBindingDescriptionIndex = 0; vertexBindingDescriptionIndex < pCreateInfos[createInfoIndex].pVertexInputState->vertexBindingDescriptionCount; ++vertexBindingDescriptionIndex)
                    {
                        skipCall |= validate_ranged_enum(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pVertexInputState->pVertexBindingDescriptions[%i].inputRate", ParameterName::IndexVector{ createInfoIndex, vertexBindingDescriptionIndex }), "VkVertexInputRate", VK_VERTEX_INPUT_RATE_BEGIN_RANGE, VK_VERTEX_INPUT_RATE_END_RANGE, pCreateInfos[createInfoIndex].pVertexInputState->pVertexBindingDescriptions[vertexBindingDescriptionIndex].inputRate);
                    }
                }

                skipCall |= validate_array(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pVertexInputState->vertexAttributeDescriptionCount", ParameterName::IndexVector{ createInfoIndex }), ParameterName("pCreateInfos[%i].pVertexInputState->pVertexAttributeDescriptions", ParameterName::IndexVector{ createInfoIndex }), pCreateInfos[createInfoIndex].pVertexInputState->vertexAttributeDescriptionCount, pCreateInfos[createInfoIndex].pVertexInputState->pVertexAttributeDescriptions, false, true);

                if (pCreateInfos[createInfoIndex].pVertexInputState->pVertexAttributeDescriptions != NULL)
                {
                    for (uint32_t vertexAttributeDescriptionIndex = 0; vertexAttributeDescriptionIndex < pCreateInfos[createInfoIndex].pVertexInputState->vertexAttributeDescriptionCount; ++vertexAttributeDescriptionIndex)
                    {
                        skipCall |= validate_ranged_enum(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pVertexInputState->pVertexAttributeDescriptions[%i].format", ParameterName::IndexVector{ createInfoIndex, vertexAttributeDescriptionIndex }), "VkFormat", VK_FORMAT_BEGIN_RANGE, VK_FORMAT_END_RANGE, pCreateInfos[createInfoIndex].pVertexInputState->pVertexAttributeDescriptions[vertexAttributeDescriptionIndex].format);
                    }
                }
            }

            skipCall |= validate_struct_type(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pInputAssemblyState", ParameterName::IndexVector{ createInfoIndex }), "VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO", pCreateInfos[createInfoIndex].pInputAssemblyState, VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, true);

            if (pCreateInfos[createInfoIndex].pInputAssemblyState != NULL)
            {
                skipCall |= validate_struct_pnext(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pInputAssemblyState->pNext", ParameterName::IndexVector{ createInfoIndex }), NULL, pCreateInfos[createInfoIndex].pInputAssemblyState->pNext, 0, NULL, GeneratedHeaderVersion);

                skipCall |= validate_reserved_flags(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pInputAssemblyState->flags", ParameterName::IndexVector{ createInfoIndex }), pCreateInfos[createInfoIndex].pInputAssemblyState->flags);

                skipCall |= validate_ranged_enum(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pInputAssemblyState->topology", ParameterName::IndexVector{ createInfoIndex }), "VkPrimitiveTopology", VK_PRIMITIVE_TOPOLOGY_BEGIN_RANGE, VK_PRIMITIVE_TOPOLOGY_END_RANGE, pCreateInfos[createInfoIndex].pInputAssemblyState->topology);

                skipCall |= validate_bool32(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pInputAssemblyState->primitiveRestartEnable", ParameterName::IndexVector{ createInfoIndex }), pCreateInfos[createInfoIndex].pInputAssemblyState->primitiveRestartEnable);
            }

            skipCall |= validate_struct_type(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pRasterizationState", ParameterName::IndexVector{ createInfoIndex }), "VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO", pCreateInfos[createInfoIndex].pRasterizationState, VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, true);

            if (pCreateInfos[createInfoIndex].pRasterizationState != NULL)
            {
                const VkStructureType allowedStructs[] = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD};

                skipCall |= validate_struct_pnext(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pRasterizationState->pNext", ParameterName::IndexVector{ createInfoIndex }), "VkPipelineRasterizationStateRasterizationOrderAMD", pCreateInfos[createInfoIndex].pRasterizationState->pNext, ARRAY_SIZE(allowedStructs), allowedStructs, GeneratedHeaderVersion);

                skipCall |= validate_reserved_flags(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pRasterizationState->flags", ParameterName::IndexVector{ createInfoIndex }), pCreateInfos[createInfoIndex].pRasterizationState->flags);

                skipCall |= validate_bool32(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pRasterizationState->depthClampEnable", ParameterName::IndexVector{ createInfoIndex }), pCreateInfos[createInfoIndex].pRasterizationState->depthClampEnable);

                skipCall |= validate_bool32(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pRasterizationState->rasterizerDiscardEnable", ParameterName::IndexVector{ createInfoIndex }), pCreateInfos[createInfoIndex].pRasterizationState->rasterizerDiscardEnable);

                skipCall |= validate_ranged_enum(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pRasterizationState->polygonMode", ParameterName::IndexVector{ createInfoIndex }), "VkPolygonMode", VK_POLYGON_MODE_BEGIN_RANGE, VK_POLYGON_MODE_END_RANGE, pCreateInfos[createInfoIndex].pRasterizationState->polygonMode);

                skipCall |= validate_flags(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pRasterizationState->cullMode", ParameterName::IndexVector{ createInfoIndex }), "VkCullModeFlagBits", AllVkCullModeFlagBits, pCreateInfos[createInfoIndex].pRasterizationState->cullMode, false);

                skipCall |= validate_ranged_enum(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pRasterizationState->frontFace", ParameterName::IndexVector{ createInfoIndex }), "VkFrontFace", VK_FRONT_FACE_BEGIN_RANGE, VK_FRONT_FACE_END_RANGE, pCreateInfos[createInfoIndex].pRasterizationState->frontFace);

                skipCall |= validate_bool32(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pRasterizationState->depthBiasEnable", ParameterName::IndexVector{ createInfoIndex }), pCreateInfos[createInfoIndex].pRasterizationState->depthBiasEnable);
            }

            skipCall |= validate_struct_type(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pDynamicState", ParameterName::IndexVector{ createInfoIndex }), "VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO", pCreateInfos[createInfoIndex].pDynamicState, VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, false);

            if (pCreateInfos[createInfoIndex].pDynamicState != NULL)
            {
                skipCall |= validate_struct_pnext(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pDynamicState->pNext", ParameterName::IndexVector{ createInfoIndex }), NULL, pCreateInfos[createInfoIndex].pDynamicState->pNext, 0, NULL, GeneratedHeaderVersion);

                skipCall |= validate_reserved_flags(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pDynamicState->flags", ParameterName::IndexVector{ createInfoIndex }), pCreateInfos[createInfoIndex].pDynamicState->flags);

                skipCall |= validate_ranged_enum_array(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].pDynamicState->dynamicStateCount", ParameterName::IndexVector{ createInfoIndex }), ParameterName("pCreateInfos[%i].pDynamicState->pDynamicStates", ParameterName::IndexVector{ createInfoIndex }), "VkDynamicState", VK_DYNAMIC_STATE_BEGIN_RANGE, VK_DYNAMIC_STATE_END_RANGE, pCreateInfos[createInfoIndex].pDynamicState->dynamicStateCount, pCreateInfos[createInfoIndex].pDynamicState->pDynamicStates, true, true);
            }

            skipCall |= validate_required_handle(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].layout", ParameterName::IndexVector{ createInfoIndex }), pCreateInfos[createInfoIndex].layout);

            skipCall |= validate_required_handle(report_data, "vkCreateGraphicsPipelines", ParameterName("pCreateInfos[%i].renderPass", ParameterName::IndexVector{ createInfoIndex }), pCreateInfos[createInfoIndex].renderPass);
        }
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateGraphicsPipelines", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateGraphicsPipelines", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateGraphicsPipelines", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_array(report_data, "vkCreateGraphicsPipelines", "createInfoCount", "pPipelines", createInfoCount, pPipelines, true, true);

    return skipCall;
}

static bool parameter_validation_vkCreateComputePipelines(
    debug_report_data*                          report_data,
    VkPipelineCache                             pipelineCache,
    uint32_t                                    createInfoCount,
    const VkComputePipelineCreateInfo*          pCreateInfos,
    const VkAllocationCallbacks*                pAllocator,
    VkPipeline*                                 pPipelines)
{
    UNUSED_PARAMETER(pipelineCache);

    bool skipCall = false;

    skipCall |= validate_struct_type_array(report_data, "vkCreateComputePipelines", "createInfoCount", "pCreateInfos", "VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO", createInfoCount, pCreateInfos, VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, true, true);

    if (pCreateInfos != NULL)
    {
        for (uint32_t createInfoIndex = 0; createInfoIndex < createInfoCount; ++createInfoIndex)
        {
            skipCall |= validate_struct_pnext(report_data, "vkCreateComputePipelines", ParameterName("pCreateInfos[%i].pNext", ParameterName::IndexVector{ createInfoIndex }), NULL, pCreateInfos[createInfoIndex].pNext, 0, NULL, GeneratedHeaderVersion);

            skipCall |= validate_flags(report_data, "vkCreateComputePipelines", ParameterName("pCreateInfos[%i].flags", ParameterName::IndexVector{ createInfoIndex }), "VkPipelineCreateFlagBits", AllVkPipelineCreateFlagBits, pCreateInfos[createInfoIndex].flags, false);

            skipCall |= validate_struct_type(report_data, "vkCreateComputePipelines", ParameterName("pCreateInfos[%i].stage", ParameterName::IndexVector{ createInfoIndex }), "VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO", &(pCreateInfos[createInfoIndex].stage), VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, false);

            skipCall |= validate_struct_pnext(report_data, "vkCreateComputePipelines", ParameterName("pCreateInfos[%i].stage.pNext", ParameterName::IndexVector{ createInfoIndex }), NULL, pCreateInfos[createInfoIndex].stage.pNext, 0, NULL, GeneratedHeaderVersion);

            skipCall |= validate_reserved_flags(report_data, "vkCreateComputePipelines", ParameterName("pCreateInfos[%i].stage.flags", ParameterName::IndexVector{ createInfoIndex }), pCreateInfos[createInfoIndex].stage.flags);

            skipCall |= validate_required_handle(report_data, "vkCreateComputePipelines", ParameterName("pCreateInfos[%i].stage.module", ParameterName::IndexVector{ createInfoIndex }), pCreateInfos[createInfoIndex].stage.module);

            skipCall |= validate_required_pointer(report_data, "vkCreateComputePipelines", ParameterName("pCreateInfos[%i].stage.pName", ParameterName::IndexVector{ createInfoIndex }), pCreateInfos[createInfoIndex].stage.pName);

            if (pCreateInfos[createInfoIndex].stage.pSpecializationInfo != NULL)
            {
                skipCall |= validate_array(report_data, "vkCreateComputePipelines", ParameterName("pCreateInfos[%i].stage.pSpecializationInfo->mapEntryCount", ParameterName::IndexVector{ createInfoIndex }), ParameterName("pCreateInfos[%i].stage.pSpecializationInfo->pMapEntries", ParameterName::IndexVector{ createInfoIndex }), pCreateInfos[createInfoIndex].stage.pSpecializationInfo->mapEntryCount, pCreateInfos[createInfoIndex].stage.pSpecializationInfo->pMapEntries, false, true);

                skipCall |= validate_array(report_data, "vkCreateComputePipelines", ParameterName("pCreateInfos[%i].stage.pSpecializationInfo->dataSize", ParameterName::IndexVector{ createInfoIndex }), ParameterName("pCreateInfos[%i].stage.pSpecializationInfo->pData", ParameterName::IndexVector{ createInfoIndex }), pCreateInfos[createInfoIndex].stage.pSpecializationInfo->dataSize, pCreateInfos[createInfoIndex].stage.pSpecializationInfo->pData, false, true);
            }

            skipCall |= validate_required_handle(report_data, "vkCreateComputePipelines", ParameterName("pCreateInfos[%i].layout", ParameterName::IndexVector{ createInfoIndex }), pCreateInfos[createInfoIndex].layout);
        }
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateComputePipelines", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateComputePipelines", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateComputePipelines", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_array(report_data, "vkCreateComputePipelines", "createInfoCount", "pPipelines", createInfoCount, pPipelines, true, true);

    return skipCall;
}

static bool parameter_validation_vkDestroyPipeline(
    debug_report_data*                          report_data,
    VkPipeline                                  pipeline,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(pipeline);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroyPipeline", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyPipeline", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyPipeline", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkCreatePipelineLayout(
    debug_report_data*                          report_data,
    const VkPipelineLayoutCreateInfo*           pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkPipelineLayout*                           pPipelineLayout)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreatePipelineLayout", "pCreateInfo", "VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreatePipelineLayout", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreatePipelineLayout", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_array(report_data, "vkCreatePipelineLayout", "pCreateInfo->setLayoutCount", "pCreateInfo->pSetLayouts", pCreateInfo->setLayoutCount, pCreateInfo->pSetLayouts, false, true);

        skipCall |= validate_array(report_data, "vkCreatePipelineLayout", "pCreateInfo->pushConstantRangeCount", "pCreateInfo->pPushConstantRanges", pCreateInfo->pushConstantRangeCount, pCreateInfo->pPushConstantRanges, false, true);

        if (pCreateInfo->pPushConstantRanges != NULL)
        {
            for (uint32_t pushConstantRangeIndex = 0; pushConstantRangeIndex < pCreateInfo->pushConstantRangeCount; ++pushConstantRangeIndex)
            {
                skipCall |= validate_flags(report_data, "vkCreatePipelineLayout", ParameterName("pCreateInfo->pPushConstantRanges[%i].stageFlags", ParameterName::IndexVector{ pushConstantRangeIndex }), "VkShaderStageFlagBits", AllVkShaderStageFlagBits, pCreateInfo->pPushConstantRanges[pushConstantRangeIndex].stageFlags, true);
            }
        }
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreatePipelineLayout", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreatePipelineLayout", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreatePipelineLayout", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreatePipelineLayout", "pPipelineLayout", pPipelineLayout);

    return skipCall;
}

static bool parameter_validation_vkDestroyPipelineLayout(
    debug_report_data*                          report_data,
    VkPipelineLayout                            pipelineLayout,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(pipelineLayout);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroyPipelineLayout", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyPipelineLayout", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyPipelineLayout", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkCreateSampler(
    debug_report_data*                          report_data,
    const VkSamplerCreateInfo*                  pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSampler*                                  pSampler)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateSampler", "pCreateInfo", "VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateSampler", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateSampler", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_ranged_enum(report_data, "vkCreateSampler", "pCreateInfo->magFilter", "VkFilter", VK_FILTER_BEGIN_RANGE, VK_FILTER_END_RANGE, pCreateInfo->magFilter);

        skipCall |= validate_ranged_enum(report_data, "vkCreateSampler", "pCreateInfo->minFilter", "VkFilter", VK_FILTER_BEGIN_RANGE, VK_FILTER_END_RANGE, pCreateInfo->minFilter);

        skipCall |= validate_ranged_enum(report_data, "vkCreateSampler", "pCreateInfo->mipmapMode", "VkSamplerMipmapMode", VK_SAMPLER_MIPMAP_MODE_BEGIN_RANGE, VK_SAMPLER_MIPMAP_MODE_END_RANGE, pCreateInfo->mipmapMode);

        skipCall |= validate_ranged_enum(report_data, "vkCreateSampler", "pCreateInfo->addressModeU", "VkSamplerAddressMode", VK_SAMPLER_ADDRESS_MODE_BEGIN_RANGE, VK_SAMPLER_ADDRESS_MODE_END_RANGE, pCreateInfo->addressModeU);

        skipCall |= validate_ranged_enum(report_data, "vkCreateSampler", "pCreateInfo->addressModeV", "VkSamplerAddressMode", VK_SAMPLER_ADDRESS_MODE_BEGIN_RANGE, VK_SAMPLER_ADDRESS_MODE_END_RANGE, pCreateInfo->addressModeV);

        skipCall |= validate_ranged_enum(report_data, "vkCreateSampler", "pCreateInfo->addressModeW", "VkSamplerAddressMode", VK_SAMPLER_ADDRESS_MODE_BEGIN_RANGE, VK_SAMPLER_ADDRESS_MODE_END_RANGE, pCreateInfo->addressModeW);

        skipCall |= validate_bool32(report_data, "vkCreateSampler", "pCreateInfo->anisotropyEnable", pCreateInfo->anisotropyEnable);

        skipCall |= validate_bool32(report_data, "vkCreateSampler", "pCreateInfo->compareEnable", pCreateInfo->compareEnable);

        skipCall |= validate_bool32(report_data, "vkCreateSampler", "pCreateInfo->unnormalizedCoordinates", pCreateInfo->unnormalizedCoordinates);
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateSampler", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateSampler", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateSampler", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateSampler", "pSampler", pSampler);

    return skipCall;
}

static bool parameter_validation_vkDestroySampler(
    debug_report_data*                          report_data,
    VkSampler                                   sampler,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(sampler);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroySampler", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroySampler", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroySampler", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkCreateDescriptorSetLayout(
    debug_report_data*                          report_data,
    const VkDescriptorSetLayoutCreateInfo*      pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDescriptorSetLayout*                      pSetLayout)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateDescriptorSetLayout", "pCreateInfo", "VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateDescriptorSetLayout", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateDescriptorSetLayout", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_array(report_data, "vkCreateDescriptorSetLayout", "pCreateInfo->bindingCount", "pCreateInfo->pBindings", pCreateInfo->bindingCount, pCreateInfo->pBindings, false, true);

        if (pCreateInfo->pBindings != NULL)
        {
            for (uint32_t bindingIndex = 0; bindingIndex < pCreateInfo->bindingCount; ++bindingIndex)
            {
                skipCall |= validate_ranged_enum(report_data, "vkCreateDescriptorSetLayout", ParameterName("pCreateInfo->pBindings[%i].descriptorType", ParameterName::IndexVector{ bindingIndex }), "VkDescriptorType", VK_DESCRIPTOR_TYPE_BEGIN_RANGE, VK_DESCRIPTOR_TYPE_END_RANGE, pCreateInfo->pBindings[bindingIndex].descriptorType);
            }
        }
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateDescriptorSetLayout", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateDescriptorSetLayout", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateDescriptorSetLayout", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateDescriptorSetLayout", "pSetLayout", pSetLayout);

    return skipCall;
}

static bool parameter_validation_vkDestroyDescriptorSetLayout(
    debug_report_data*                          report_data,
    VkDescriptorSetLayout                       descriptorSetLayout,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(descriptorSetLayout);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroyDescriptorSetLayout", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyDescriptorSetLayout", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyDescriptorSetLayout", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkCreateDescriptorPool(
    debug_report_data*                          report_data,
    const VkDescriptorPoolCreateInfo*           pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDescriptorPool*                           pDescriptorPool)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateDescriptorPool", "pCreateInfo", "VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateDescriptorPool", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_flags(report_data, "vkCreateDescriptorPool", "pCreateInfo->flags", "VkDescriptorPoolCreateFlagBits", AllVkDescriptorPoolCreateFlagBits, pCreateInfo->flags, false);

        skipCall |= validate_array(report_data, "vkCreateDescriptorPool", "pCreateInfo->poolSizeCount", "pCreateInfo->pPoolSizes", pCreateInfo->poolSizeCount, pCreateInfo->pPoolSizes, true, true);

        if (pCreateInfo->pPoolSizes != NULL)
        {
            for (uint32_t poolSizeIndex = 0; poolSizeIndex < pCreateInfo->poolSizeCount; ++poolSizeIndex)
            {
                skipCall |= validate_ranged_enum(report_data, "vkCreateDescriptorPool", ParameterName("pCreateInfo->pPoolSizes[%i].type", ParameterName::IndexVector{ poolSizeIndex }), "VkDescriptorType", VK_DESCRIPTOR_TYPE_BEGIN_RANGE, VK_DESCRIPTOR_TYPE_END_RANGE, pCreateInfo->pPoolSizes[poolSizeIndex].type);
            }
        }
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateDescriptorPool", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateDescriptorPool", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateDescriptorPool", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateDescriptorPool", "pDescriptorPool", pDescriptorPool);

    return skipCall;
}

static bool parameter_validation_vkDestroyDescriptorPool(
    debug_report_data*                          report_data,
    VkDescriptorPool                            descriptorPool,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(descriptorPool);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroyDescriptorPool", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyDescriptorPool", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyDescriptorPool", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkResetDescriptorPool(
    debug_report_data*                          report_data,
    VkDescriptorPool                            descriptorPool,
    VkDescriptorPoolResetFlags                  flags)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkResetDescriptorPool", "descriptorPool", descriptorPool);

    skipCall |= validate_reserved_flags(report_data, "vkResetDescriptorPool", "flags", flags);

    return skipCall;
}

static bool parameter_validation_vkAllocateDescriptorSets(
    debug_report_data*                          report_data,
    const VkDescriptorSetAllocateInfo*          pAllocateInfo,
    VkDescriptorSet*                            pDescriptorSets)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkAllocateDescriptorSets", "pAllocateInfo", "VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO", pAllocateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, true);

    if (pAllocateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkAllocateDescriptorSets", "pAllocateInfo->pNext", NULL, pAllocateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_required_handle(report_data, "vkAllocateDescriptorSets", "pAllocateInfo->descriptorPool", pAllocateInfo->descriptorPool);

        skipCall |= validate_handle_array(report_data, "vkAllocateDescriptorSets", "pAllocateInfo->descriptorSetCount", "pAllocateInfo->pSetLayouts", pAllocateInfo->descriptorSetCount, pAllocateInfo->pSetLayouts, true, true);
    }

    if (pAllocateInfo != NULL) {
        skipCall |= validate_array(report_data, "vkAllocateDescriptorSets", "pAllocateInfo->descriptorSetCount", "pDescriptorSets", pAllocateInfo->descriptorSetCount, pDescriptorSets, true, true);
    }

    return skipCall;
}

static bool parameter_validation_vkFreeDescriptorSets(
    debug_report_data*                          report_data,
    VkDescriptorPool                            descriptorPool,
    uint32_t                                    descriptorSetCount,
    const VkDescriptorSet*                      pDescriptorSets)
{
    UNUSED_PARAMETER(pDescriptorSets);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkFreeDescriptorSets", "descriptorPool", descriptorPool);

    return skipCall;
}

static bool parameter_validation_vkUpdateDescriptorSets(
    debug_report_data*                          report_data,
    uint32_t                                    descriptorWriteCount,
    const VkWriteDescriptorSet*                 pDescriptorWrites,
    uint32_t                                    descriptorCopyCount,
    const VkCopyDescriptorSet*                  pDescriptorCopies)
{
    bool skipCall = false;

    skipCall |= validate_struct_type_array(report_data, "vkUpdateDescriptorSets", "descriptorWriteCount", "pDescriptorWrites", "VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET", descriptorWriteCount, pDescriptorWrites, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, false, true);

    if (pDescriptorWrites != NULL)
    {
        for (uint32_t descriptorWriteIndex = 0; descriptorWriteIndex < descriptorWriteCount; ++descriptorWriteIndex)
        {
            skipCall |= validate_struct_pnext(report_data, "vkUpdateDescriptorSets", ParameterName("pDescriptorWrites[%i].pNext", ParameterName::IndexVector{ descriptorWriteIndex }), NULL, pDescriptorWrites[descriptorWriteIndex].pNext, 0, NULL, GeneratedHeaderVersion);

            skipCall |= validate_required_handle(report_data, "vkUpdateDescriptorSets", ParameterName("pDescriptorWrites[%i].dstSet", ParameterName::IndexVector{ descriptorWriteIndex }), pDescriptorWrites[descriptorWriteIndex].dstSet);

            skipCall |= validate_ranged_enum(report_data, "vkUpdateDescriptorSets", ParameterName("pDescriptorWrites[%i].descriptorType", ParameterName::IndexVector{ descriptorWriteIndex }), "VkDescriptorType", VK_DESCRIPTOR_TYPE_BEGIN_RANGE, VK_DESCRIPTOR_TYPE_END_RANGE, pDescriptorWrites[descriptorWriteIndex].descriptorType);
        }
    }

    skipCall |= validate_struct_type_array(report_data, "vkUpdateDescriptorSets", "descriptorCopyCount", "pDescriptorCopies", "VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET", descriptorCopyCount, pDescriptorCopies, VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET, false, true);

    if (pDescriptorCopies != NULL)
    {
        for (uint32_t descriptorCopyIndex = 0; descriptorCopyIndex < descriptorCopyCount; ++descriptorCopyIndex)
        {
            skipCall |= validate_struct_pnext(report_data, "vkUpdateDescriptorSets", ParameterName("pDescriptorCopies[%i].pNext", ParameterName::IndexVector{ descriptorCopyIndex }), NULL, pDescriptorCopies[descriptorCopyIndex].pNext, 0, NULL, GeneratedHeaderVersion);

            skipCall |= validate_required_handle(report_data, "vkUpdateDescriptorSets", ParameterName("pDescriptorCopies[%i].srcSet", ParameterName::IndexVector{ descriptorCopyIndex }), pDescriptorCopies[descriptorCopyIndex].srcSet);

            skipCall |= validate_required_handle(report_data, "vkUpdateDescriptorSets", ParameterName("pDescriptorCopies[%i].dstSet", ParameterName::IndexVector{ descriptorCopyIndex }), pDescriptorCopies[descriptorCopyIndex].dstSet);
        }
    }

    return skipCall;
}

static bool parameter_validation_vkCreateFramebuffer(
    debug_report_data*                          report_data,
    const VkFramebufferCreateInfo*              pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkFramebuffer*                              pFramebuffer)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateFramebuffer", "pCreateInfo", "VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateFramebuffer", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateFramebuffer", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_required_handle(report_data, "vkCreateFramebuffer", "pCreateInfo->renderPass", pCreateInfo->renderPass);

        skipCall |= validate_array(report_data, "vkCreateFramebuffer", "pCreateInfo->attachmentCount", "pCreateInfo->pAttachments", pCreateInfo->attachmentCount, pCreateInfo->pAttachments, false, true);
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateFramebuffer", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateFramebuffer", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateFramebuffer", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateFramebuffer", "pFramebuffer", pFramebuffer);

    return skipCall;
}

static bool parameter_validation_vkDestroyFramebuffer(
    debug_report_data*                          report_data,
    VkFramebuffer                               framebuffer,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(framebuffer);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroyFramebuffer", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyFramebuffer", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyFramebuffer", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkCreateRenderPass(
    debug_report_data*                          report_data,
    const VkRenderPassCreateInfo*               pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkRenderPass*                               pRenderPass)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateRenderPass", "pCreateInfo", "VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateRenderPass", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateRenderPass", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_array(report_data, "vkCreateRenderPass", "pCreateInfo->attachmentCount", "pCreateInfo->pAttachments", pCreateInfo->attachmentCount, pCreateInfo->pAttachments, false, true);

        if (pCreateInfo->pAttachments != NULL)
        {
            for (uint32_t attachmentIndex = 0; attachmentIndex < pCreateInfo->attachmentCount; ++attachmentIndex)
            {
                skipCall |= validate_flags(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pAttachments[%i].flags", ParameterName::IndexVector{ attachmentIndex }), "VkAttachmentDescriptionFlagBits", AllVkAttachmentDescriptionFlagBits, pCreateInfo->pAttachments[attachmentIndex].flags, false);

                skipCall |= validate_ranged_enum(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pAttachments[%i].format", ParameterName::IndexVector{ attachmentIndex }), "VkFormat", VK_FORMAT_BEGIN_RANGE, VK_FORMAT_END_RANGE, pCreateInfo->pAttachments[attachmentIndex].format);

                skipCall |= validate_ranged_enum(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pAttachments[%i].loadOp", ParameterName::IndexVector{ attachmentIndex }), "VkAttachmentLoadOp", VK_ATTACHMENT_LOAD_OP_BEGIN_RANGE, VK_ATTACHMENT_LOAD_OP_END_RANGE, pCreateInfo->pAttachments[attachmentIndex].loadOp);

                skipCall |= validate_ranged_enum(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pAttachments[%i].storeOp", ParameterName::IndexVector{ attachmentIndex }), "VkAttachmentStoreOp", VK_ATTACHMENT_STORE_OP_BEGIN_RANGE, VK_ATTACHMENT_STORE_OP_END_RANGE, pCreateInfo->pAttachments[attachmentIndex].storeOp);

                skipCall |= validate_ranged_enum(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pAttachments[%i].stencilLoadOp", ParameterName::IndexVector{ attachmentIndex }), "VkAttachmentLoadOp", VK_ATTACHMENT_LOAD_OP_BEGIN_RANGE, VK_ATTACHMENT_LOAD_OP_END_RANGE, pCreateInfo->pAttachments[attachmentIndex].stencilLoadOp);

                skipCall |= validate_ranged_enum(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pAttachments[%i].stencilStoreOp", ParameterName::IndexVector{ attachmentIndex }), "VkAttachmentStoreOp", VK_ATTACHMENT_STORE_OP_BEGIN_RANGE, VK_ATTACHMENT_STORE_OP_END_RANGE, pCreateInfo->pAttachments[attachmentIndex].stencilStoreOp);

                skipCall |= validate_ranged_enum(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pAttachments[%i].initialLayout", ParameterName::IndexVector{ attachmentIndex }), "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, pCreateInfo->pAttachments[attachmentIndex].initialLayout);

                skipCall |= validate_ranged_enum(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pAttachments[%i].finalLayout", ParameterName::IndexVector{ attachmentIndex }), "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, pCreateInfo->pAttachments[attachmentIndex].finalLayout);
            }
        }

        skipCall |= validate_array(report_data, "vkCreateRenderPass", "pCreateInfo->subpassCount", "pCreateInfo->pSubpasses", pCreateInfo->subpassCount, pCreateInfo->pSubpasses, true, true);

        if (pCreateInfo->pSubpasses != NULL)
        {
            for (uint32_t subpassIndex = 0; subpassIndex < pCreateInfo->subpassCount; ++subpassIndex)
            {
                skipCall |= validate_reserved_flags(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pSubpasses[%i].flags", ParameterName::IndexVector{ subpassIndex }), pCreateInfo->pSubpasses[subpassIndex].flags);

                skipCall |= validate_ranged_enum(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pSubpasses[%i].pipelineBindPoint", ParameterName::IndexVector{ subpassIndex }), "VkPipelineBindPoint", VK_PIPELINE_BIND_POINT_BEGIN_RANGE, VK_PIPELINE_BIND_POINT_END_RANGE, pCreateInfo->pSubpasses[subpassIndex].pipelineBindPoint);

                skipCall |= validate_array(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pSubpasses[%i].inputAttachmentCount", ParameterName::IndexVector{ subpassIndex }), ParameterName("pCreateInfo->pSubpasses[%i].pInputAttachments", ParameterName::IndexVector{ subpassIndex }), pCreateInfo->pSubpasses[subpassIndex].inputAttachmentCount, pCreateInfo->pSubpasses[subpassIndex].pInputAttachments, false, true);

                if (pCreateInfo->pSubpasses[subpassIndex].pInputAttachments != NULL)
                {
                    for (uint32_t inputAttachmentIndex = 0; inputAttachmentIndex < pCreateInfo->pSubpasses[subpassIndex].inputAttachmentCount; ++inputAttachmentIndex)
                    {
                        skipCall |= validate_ranged_enum(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pSubpasses[%i].pInputAttachments[%i].layout", ParameterName::IndexVector{ subpassIndex, inputAttachmentIndex }), "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, pCreateInfo->pSubpasses[subpassIndex].pInputAttachments[inputAttachmentIndex].layout);
                    }
                }

                skipCall |= validate_array(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pSubpasses[%i].colorAttachmentCount", ParameterName::IndexVector{ subpassIndex }), ParameterName("pCreateInfo->pSubpasses[%i].pColorAttachments", ParameterName::IndexVector{ subpassIndex }), pCreateInfo->pSubpasses[subpassIndex].colorAttachmentCount, pCreateInfo->pSubpasses[subpassIndex].pColorAttachments, false, true);

                if (pCreateInfo->pSubpasses[subpassIndex].pColorAttachments != NULL)
                {
                    for (uint32_t colorAttachmentIndex = 0; colorAttachmentIndex < pCreateInfo->pSubpasses[subpassIndex].colorAttachmentCount; ++colorAttachmentIndex)
                    {
                        skipCall |= validate_ranged_enum(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pSubpasses[%i].pColorAttachments[%i].layout", ParameterName::IndexVector{ subpassIndex, colorAttachmentIndex }), "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, pCreateInfo->pSubpasses[subpassIndex].pColorAttachments[colorAttachmentIndex].layout);
                    }
                }

                if (pCreateInfo->pSubpasses[subpassIndex].pResolveAttachments != NULL)
                {
                    for (uint32_t colorAttachmentIndex = 0; colorAttachmentIndex < pCreateInfo->pSubpasses[subpassIndex].colorAttachmentCount; ++colorAttachmentIndex)
                    {
                        skipCall |= validate_ranged_enum(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pSubpasses[%i].pResolveAttachments[%i].layout", ParameterName::IndexVector{ subpassIndex, colorAttachmentIndex }), "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, pCreateInfo->pSubpasses[subpassIndex].pResolveAttachments[colorAttachmentIndex].layout);
                    }
                }

                if (pCreateInfo->pSubpasses[subpassIndex].pDepthStencilAttachment != NULL)
                {
                    skipCall |= validate_ranged_enum(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pSubpasses[%i].pDepthStencilAttachment->layout", ParameterName::IndexVector{ subpassIndex }), "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, pCreateInfo->pSubpasses[subpassIndex].pDepthStencilAttachment->layout);
                }

                skipCall |= validate_array(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pSubpasses[%i].preserveAttachmentCount", ParameterName::IndexVector{ subpassIndex }), ParameterName("pCreateInfo->pSubpasses[%i].pPreserveAttachments", ParameterName::IndexVector{ subpassIndex }), pCreateInfo->pSubpasses[subpassIndex].preserveAttachmentCount, pCreateInfo->pSubpasses[subpassIndex].pPreserveAttachments, false, true);
            }
        }

        skipCall |= validate_array(report_data, "vkCreateRenderPass", "pCreateInfo->dependencyCount", "pCreateInfo->pDependencies", pCreateInfo->dependencyCount, pCreateInfo->pDependencies, false, true);

        if (pCreateInfo->pDependencies != NULL)
        {
            for (uint32_t dependencyIndex = 0; dependencyIndex < pCreateInfo->dependencyCount; ++dependencyIndex)
            {
                skipCall |= validate_flags(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pDependencies[%i].srcStageMask", ParameterName::IndexVector{ dependencyIndex }), "VkPipelineStageFlagBits", AllVkPipelineStageFlagBits, pCreateInfo->pDependencies[dependencyIndex].srcStageMask, true);

                skipCall |= validate_flags(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pDependencies[%i].dstStageMask", ParameterName::IndexVector{ dependencyIndex }), "VkPipelineStageFlagBits", AllVkPipelineStageFlagBits, pCreateInfo->pDependencies[dependencyIndex].dstStageMask, true);

                skipCall |= validate_flags(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pDependencies[%i].srcAccessMask", ParameterName::IndexVector{ dependencyIndex }), "VkAccessFlagBits", AllVkAccessFlagBits, pCreateInfo->pDependencies[dependencyIndex].srcAccessMask, false);

                skipCall |= validate_flags(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pDependencies[%i].dstAccessMask", ParameterName::IndexVector{ dependencyIndex }), "VkAccessFlagBits", AllVkAccessFlagBits, pCreateInfo->pDependencies[dependencyIndex].dstAccessMask, false);

                skipCall |= validate_flags(report_data, "vkCreateRenderPass", ParameterName("pCreateInfo->pDependencies[%i].dependencyFlags", ParameterName::IndexVector{ dependencyIndex }), "VkDependencyFlagBits", AllVkDependencyFlagBits, pCreateInfo->pDependencies[dependencyIndex].dependencyFlags, false);
            }
        }
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateRenderPass", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateRenderPass", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateRenderPass", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateRenderPass", "pRenderPass", pRenderPass);

    return skipCall;
}

static bool parameter_validation_vkDestroyRenderPass(
    debug_report_data*                          report_data,
    VkRenderPass                                renderPass,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(renderPass);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroyRenderPass", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyRenderPass", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyRenderPass", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkGetRenderAreaGranularity(
    debug_report_data*                          report_data,
    VkRenderPass                                renderPass,
    VkExtent2D*                                 pGranularity)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkGetRenderAreaGranularity", "renderPass", renderPass);

    skipCall |= validate_required_pointer(report_data, "vkGetRenderAreaGranularity", "pGranularity", pGranularity);

    return skipCall;
}

static bool parameter_validation_vkCreateCommandPool(
    debug_report_data*                          report_data,
    const VkCommandPoolCreateInfo*              pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkCommandPool*                              pCommandPool)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateCommandPool", "pCreateInfo", "VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateCommandPool", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_flags(report_data, "vkCreateCommandPool", "pCreateInfo->flags", "VkCommandPoolCreateFlagBits", AllVkCommandPoolCreateFlagBits, pCreateInfo->flags, false);
    }

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkCreateCommandPool", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateCommandPool", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkCreateCommandPool", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateCommandPool", "pCommandPool", pCommandPool);

    return skipCall;
}

static bool parameter_validation_vkDestroyCommandPool(
    debug_report_data*                          report_data,
    VkCommandPool                               commandPool,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(commandPool);

    bool skipCall = false;

    if (pAllocator != NULL)
    {
        skipCall |= validate_required_pointer(report_data, "vkDestroyCommandPool", "pAllocator->pfnAllocation", reinterpret_cast<const void*>(pAllocator->pfnAllocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyCommandPool", "pAllocator->pfnReallocation", reinterpret_cast<const void*>(pAllocator->pfnReallocation));

        skipCall |= validate_required_pointer(report_data, "vkDestroyCommandPool", "pAllocator->pfnFree", reinterpret_cast<const void*>(pAllocator->pfnFree));
    }

    return skipCall;
}

static bool parameter_validation_vkResetCommandPool(
    debug_report_data*                          report_data,
    VkCommandPool                               commandPool,
    VkCommandPoolResetFlags                     flags)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkResetCommandPool", "commandPool", commandPool);

    skipCall |= validate_flags(report_data, "vkResetCommandPool", "flags", "VkCommandPoolResetFlagBits", AllVkCommandPoolResetFlagBits, flags, false);

    return skipCall;
}

static bool parameter_validation_vkAllocateCommandBuffers(
    debug_report_data*                          report_data,
    const VkCommandBufferAllocateInfo*          pAllocateInfo,
    VkCommandBuffer*                            pCommandBuffers)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkAllocateCommandBuffers", "pAllocateInfo", "VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO", pAllocateInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, true);

    if (pAllocateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkAllocateCommandBuffers", "pAllocateInfo->pNext", NULL, pAllocateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_required_handle(report_data, "vkAllocateCommandBuffers", "pAllocateInfo->commandPool", pAllocateInfo->commandPool);

        skipCall |= validate_ranged_enum(report_data, "vkAllocateCommandBuffers", "pAllocateInfo->level", "VkCommandBufferLevel", VK_COMMAND_BUFFER_LEVEL_BEGIN_RANGE, VK_COMMAND_BUFFER_LEVEL_END_RANGE, pAllocateInfo->level);
    }

    if (pAllocateInfo != NULL) {
        skipCall |= validate_array(report_data, "vkAllocateCommandBuffers", "pAllocateInfo->commandBufferCount", "pCommandBuffers", pAllocateInfo->commandBufferCount, pCommandBuffers, true, true);
    }

    return skipCall;
}

static bool parameter_validation_vkFreeCommandBuffers(
    debug_report_data*                          report_data,
    VkCommandPool                               commandPool,
    uint32_t                                    commandBufferCount,
    const VkCommandBuffer*                      pCommandBuffers)
{
    UNUSED_PARAMETER(pCommandBuffers);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkFreeCommandBuffers", "commandPool", commandPool);

    return skipCall;
}

static bool parameter_validation_vkBeginCommandBuffer(
    debug_report_data*                          report_data,
    const VkCommandBufferBeginInfo*             pBeginInfo)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkBeginCommandBuffer", "pBeginInfo", "VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO", pBeginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, true);

    if (pBeginInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkBeginCommandBuffer", "pBeginInfo->pNext", NULL, pBeginInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_flags(report_data, "vkBeginCommandBuffer", "pBeginInfo->flags", "VkCommandBufferUsageFlagBits", AllVkCommandBufferUsageFlagBits, pBeginInfo->flags, false);
    }

    return skipCall;
}

static bool parameter_validation_vkResetCommandBuffer(
    debug_report_data*                          report_data,
    VkCommandBufferResetFlags                   flags)
{
    bool skipCall = false;

    skipCall |= validate_flags(report_data, "vkResetCommandBuffer", "flags", "VkCommandBufferResetFlagBits", AllVkCommandBufferResetFlagBits, flags, false);

    return skipCall;
}

static bool parameter_validation_vkCmdBindPipeline(
    debug_report_data*                          report_data,
    VkPipelineBindPoint                         pipelineBindPoint,
    VkPipeline                                  pipeline)
{
    bool skipCall = false;

    skipCall |= validate_ranged_enum(report_data, "vkCmdBindPipeline", "pipelineBindPoint", "VkPipelineBindPoint", VK_PIPELINE_BIND_POINT_BEGIN_RANGE, VK_PIPELINE_BIND_POINT_END_RANGE, pipelineBindPoint);

    skipCall |= validate_required_handle(report_data, "vkCmdBindPipeline", "pipeline", pipeline);

    return skipCall;
}

static bool parameter_validation_vkCmdSetViewport(
    debug_report_data*                          report_data,
    uint32_t                                    firstViewport,
    uint32_t                                    viewportCount,
    const VkViewport*                           pViewports)
{
    UNUSED_PARAMETER(firstViewport);

    bool skipCall = false;

    skipCall |= validate_array(report_data, "vkCmdSetViewport", "viewportCount", "pViewports", viewportCount, pViewports, true, true);

    return skipCall;
}

static bool parameter_validation_vkCmdSetScissor(
    debug_report_data*                          report_data,
    uint32_t                                    firstScissor,
    uint32_t                                    scissorCount,
    const VkRect2D*                             pScissors)
{
    UNUSED_PARAMETER(firstScissor);

    bool skipCall = false;

    skipCall |= validate_array(report_data, "vkCmdSetScissor", "scissorCount", "pScissors", scissorCount, pScissors, true, true);

    return skipCall;
}

static bool parameter_validation_vkCmdSetBlendConstants(
    debug_report_data*                          report_data,
    const float                                 blendConstants[4])
{
    bool skipCall = false;

    skipCall |= validate_required_pointer(report_data, "vkCmdSetBlendConstants", "blendConstants", blendConstants);

    return skipCall;
}

static bool parameter_validation_vkCmdSetStencilCompareMask(
    debug_report_data*                          report_data,
    VkStencilFaceFlags                          faceMask,
    uint32_t                                    compareMask)
{
    UNUSED_PARAMETER(compareMask);

    bool skipCall = false;

    skipCall |= validate_flags(report_data, "vkCmdSetStencilCompareMask", "faceMask", "VkStencilFaceFlagBits", AllVkStencilFaceFlagBits, faceMask, true);

    return skipCall;
}

static bool parameter_validation_vkCmdSetStencilWriteMask(
    debug_report_data*                          report_data,
    VkStencilFaceFlags                          faceMask,
    uint32_t                                    writeMask)
{
    UNUSED_PARAMETER(writeMask);

    bool skipCall = false;

    skipCall |= validate_flags(report_data, "vkCmdSetStencilWriteMask", "faceMask", "VkStencilFaceFlagBits", AllVkStencilFaceFlagBits, faceMask, true);

    return skipCall;
}

static bool parameter_validation_vkCmdSetStencilReference(
    debug_report_data*                          report_data,
    VkStencilFaceFlags                          faceMask,
    uint32_t                                    reference)
{
    UNUSED_PARAMETER(reference);

    bool skipCall = false;

    skipCall |= validate_flags(report_data, "vkCmdSetStencilReference", "faceMask", "VkStencilFaceFlagBits", AllVkStencilFaceFlagBits, faceMask, true);

    return skipCall;
}

static bool parameter_validation_vkCmdBindDescriptorSets(
    debug_report_data*                          report_data,
    VkPipelineBindPoint                         pipelineBindPoint,
    VkPipelineLayout                            layout,
    uint32_t                                    firstSet,
    uint32_t                                    descriptorSetCount,
    const VkDescriptorSet*                      pDescriptorSets,
    uint32_t                                    dynamicOffsetCount,
    const uint32_t*                             pDynamicOffsets)
{
    UNUSED_PARAMETER(firstSet);

    bool skipCall = false;

    skipCall |= validate_ranged_enum(report_data, "vkCmdBindDescriptorSets", "pipelineBindPoint", "VkPipelineBindPoint", VK_PIPELINE_BIND_POINT_BEGIN_RANGE, VK_PIPELINE_BIND_POINT_END_RANGE, pipelineBindPoint);

    skipCall |= validate_required_handle(report_data, "vkCmdBindDescriptorSets", "layout", layout);

    skipCall |= validate_handle_array(report_data, "vkCmdBindDescriptorSets", "descriptorSetCount", "pDescriptorSets", descriptorSetCount, pDescriptorSets, true, true);

    skipCall |= validate_array(report_data, "vkCmdBindDescriptorSets", "dynamicOffsetCount", "pDynamicOffsets", dynamicOffsetCount, pDynamicOffsets, false, true);

    return skipCall;
}

static bool parameter_validation_vkCmdBindIndexBuffer(
    debug_report_data*                          report_data,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    VkIndexType                                 indexType)
{
    UNUSED_PARAMETER(offset);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdBindIndexBuffer", "buffer", buffer);

    skipCall |= validate_ranged_enum(report_data, "vkCmdBindIndexBuffer", "indexType", "VkIndexType", VK_INDEX_TYPE_BEGIN_RANGE, VK_INDEX_TYPE_END_RANGE, indexType);

    return skipCall;
}

static bool parameter_validation_vkCmdBindVertexBuffers(
    debug_report_data*                          report_data,
    uint32_t                                    firstBinding,
    uint32_t                                    bindingCount,
    const VkBuffer*                             pBuffers,
    const VkDeviceSize*                         pOffsets)
{
    UNUSED_PARAMETER(firstBinding);

    bool skipCall = false;

    skipCall |= validate_handle_array(report_data, "vkCmdBindVertexBuffers", "bindingCount", "pBuffers", bindingCount, pBuffers, true, true);

    skipCall |= validate_array(report_data, "vkCmdBindVertexBuffers", "bindingCount", "pOffsets", bindingCount, pOffsets, true, true);

    return skipCall;
}

static bool parameter_validation_vkCmdDrawIndirect(
    debug_report_data*                          report_data,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    uint32_t                                    drawCount,
    uint32_t                                    stride)
{
    UNUSED_PARAMETER(offset);
    UNUSED_PARAMETER(drawCount);
    UNUSED_PARAMETER(stride);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdDrawIndirect", "buffer", buffer);

    return skipCall;
}

static bool parameter_validation_vkCmdDrawIndexedIndirect(
    debug_report_data*                          report_data,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    uint32_t                                    drawCount,
    uint32_t                                    stride)
{
    UNUSED_PARAMETER(offset);
    UNUSED_PARAMETER(drawCount);
    UNUSED_PARAMETER(stride);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdDrawIndexedIndirect", "buffer", buffer);

    return skipCall;
}

static bool parameter_validation_vkCmdDispatchIndirect(
    debug_report_data*                          report_data,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset)
{
    UNUSED_PARAMETER(offset);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdDispatchIndirect", "buffer", buffer);

    return skipCall;
}

static bool parameter_validation_vkCmdCopyBuffer(
    debug_report_data*                          report_data,
    VkBuffer                                    srcBuffer,
    VkBuffer                                    dstBuffer,
    uint32_t                                    regionCount,
    const VkBufferCopy*                         pRegions)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdCopyBuffer", "srcBuffer", srcBuffer);

    skipCall |= validate_required_handle(report_data, "vkCmdCopyBuffer", "dstBuffer", dstBuffer);

    skipCall |= validate_array(report_data, "vkCmdCopyBuffer", "regionCount", "pRegions", regionCount, pRegions, true, true);

    return skipCall;
}

static bool parameter_validation_vkCmdCopyImage(
    debug_report_data*                          report_data,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkImageCopy*                          pRegions)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdCopyImage", "srcImage", srcImage);

    skipCall |= validate_ranged_enum(report_data, "vkCmdCopyImage", "srcImageLayout", "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, srcImageLayout);

    skipCall |= validate_required_handle(report_data, "vkCmdCopyImage", "dstImage", dstImage);

    skipCall |= validate_ranged_enum(report_data, "vkCmdCopyImage", "dstImageLayout", "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, dstImageLayout);

    skipCall |= validate_array(report_data, "vkCmdCopyImage", "regionCount", "pRegions", regionCount, pRegions, true, true);

    if (pRegions != NULL)
    {
        for (uint32_t regionIndex = 0; regionIndex < regionCount; ++regionIndex)
        {
            skipCall |= validate_flags(report_data, "vkCmdCopyImage", ParameterName("pRegions[%i].srcSubresource.aspectMask", ParameterName::IndexVector{ regionIndex }), "VkImageAspectFlagBits", AllVkImageAspectFlagBits, pRegions[regionIndex].srcSubresource.aspectMask, true);

            skipCall |= validate_flags(report_data, "vkCmdCopyImage", ParameterName("pRegions[%i].dstSubresource.aspectMask", ParameterName::IndexVector{ regionIndex }), "VkImageAspectFlagBits", AllVkImageAspectFlagBits, pRegions[regionIndex].dstSubresource.aspectMask, true);
        }
    }

    return skipCall;
}

static bool parameter_validation_vkCmdBlitImage(
    debug_report_data*                          report_data,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkImageBlit*                          pRegions,
    VkFilter                                    filter)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdBlitImage", "srcImage", srcImage);

    skipCall |= validate_ranged_enum(report_data, "vkCmdBlitImage", "srcImageLayout", "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, srcImageLayout);

    skipCall |= validate_required_handle(report_data, "vkCmdBlitImage", "dstImage", dstImage);

    skipCall |= validate_ranged_enum(report_data, "vkCmdBlitImage", "dstImageLayout", "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, dstImageLayout);

    skipCall |= validate_array(report_data, "vkCmdBlitImage", "regionCount", "pRegions", regionCount, pRegions, true, true);

    if (pRegions != NULL)
    {
        for (uint32_t regionIndex = 0; regionIndex < regionCount; ++regionIndex)
        {
            skipCall |= validate_flags(report_data, "vkCmdBlitImage", ParameterName("pRegions[%i].srcSubresource.aspectMask", ParameterName::IndexVector{ regionIndex }), "VkImageAspectFlagBits", AllVkImageAspectFlagBits, pRegions[regionIndex].srcSubresource.aspectMask, true);

            skipCall |= validate_flags(report_data, "vkCmdBlitImage", ParameterName("pRegions[%i].dstSubresource.aspectMask", ParameterName::IndexVector{ regionIndex }), "VkImageAspectFlagBits", AllVkImageAspectFlagBits, pRegions[regionIndex].dstSubresource.aspectMask, true);
        }
    }

    skipCall |= validate_ranged_enum(report_data, "vkCmdBlitImage", "filter", "VkFilter", VK_FILTER_BEGIN_RANGE, VK_FILTER_END_RANGE, filter);

    return skipCall;
}

static bool parameter_validation_vkCmdCopyBufferToImage(
    debug_report_data*                          report_data,
    VkBuffer                                    srcBuffer,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                    pRegions)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdCopyBufferToImage", "srcBuffer", srcBuffer);

    skipCall |= validate_required_handle(report_data, "vkCmdCopyBufferToImage", "dstImage", dstImage);

    skipCall |= validate_ranged_enum(report_data, "vkCmdCopyBufferToImage", "dstImageLayout", "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, dstImageLayout);

    skipCall |= validate_array(report_data, "vkCmdCopyBufferToImage", "regionCount", "pRegions", regionCount, pRegions, true, true);

    if (pRegions != NULL)
    {
        for (uint32_t regionIndex = 0; regionIndex < regionCount; ++regionIndex)
        {
            skipCall |= validate_flags(report_data, "vkCmdCopyBufferToImage", ParameterName("pRegions[%i].imageSubresource.aspectMask", ParameterName::IndexVector{ regionIndex }), "VkImageAspectFlagBits", AllVkImageAspectFlagBits, pRegions[regionIndex].imageSubresource.aspectMask, true);
        }
    }

    return skipCall;
}

static bool parameter_validation_vkCmdCopyImageToBuffer(
    debug_report_data*                          report_data,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkBuffer                                    dstBuffer,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                    pRegions)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdCopyImageToBuffer", "srcImage", srcImage);

    skipCall |= validate_ranged_enum(report_data, "vkCmdCopyImageToBuffer", "srcImageLayout", "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, srcImageLayout);

    skipCall |= validate_required_handle(report_data, "vkCmdCopyImageToBuffer", "dstBuffer", dstBuffer);

    skipCall |= validate_array(report_data, "vkCmdCopyImageToBuffer", "regionCount", "pRegions", regionCount, pRegions, true, true);

    if (pRegions != NULL)
    {
        for (uint32_t regionIndex = 0; regionIndex < regionCount; ++regionIndex)
        {
            skipCall |= validate_flags(report_data, "vkCmdCopyImageToBuffer", ParameterName("pRegions[%i].imageSubresource.aspectMask", ParameterName::IndexVector{ regionIndex }), "VkImageAspectFlagBits", AllVkImageAspectFlagBits, pRegions[regionIndex].imageSubresource.aspectMask, true);
        }
    }

    return skipCall;
}

static bool parameter_validation_vkCmdUpdateBuffer(
    debug_report_data*                          report_data,
    VkBuffer                                    dstBuffer,
    VkDeviceSize                                dstOffset,
    VkDeviceSize                                dataSize,
    const void*                                 pData)
{
    UNUSED_PARAMETER(dstOffset);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdUpdateBuffer", "dstBuffer", dstBuffer);

    skipCall |= validate_array(report_data, "vkCmdUpdateBuffer", "dataSize", "pData", dataSize, pData, true, true);

    return skipCall;
}

static bool parameter_validation_vkCmdFillBuffer(
    debug_report_data*                          report_data,
    VkBuffer                                    dstBuffer,
    VkDeviceSize                                dstOffset,
    VkDeviceSize                                size,
    uint32_t                                    data)
{
    UNUSED_PARAMETER(dstOffset);
    UNUSED_PARAMETER(size);
    UNUSED_PARAMETER(data);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdFillBuffer", "dstBuffer", dstBuffer);

    return skipCall;
}

static bool parameter_validation_vkCmdClearColorImage(
    debug_report_data*                          report_data,
    VkImage                                     image,
    VkImageLayout                               imageLayout,
    const VkClearColorValue*                    pColor,
    uint32_t                                    rangeCount,
    const VkImageSubresourceRange*              pRanges)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdClearColorImage", "image", image);

    skipCall |= validate_ranged_enum(report_data, "vkCmdClearColorImage", "imageLayout", "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, imageLayout);

    skipCall |= validate_required_pointer(report_data, "vkCmdClearColorImage", "pColor", pColor);

    skipCall |= validate_array(report_data, "vkCmdClearColorImage", "rangeCount", "pRanges", rangeCount, pRanges, true, true);

    if (pRanges != NULL)
    {
        for (uint32_t rangeIndex = 0; rangeIndex < rangeCount; ++rangeIndex)
        {
            skipCall |= validate_flags(report_data, "vkCmdClearColorImage", ParameterName("pRanges[%i].aspectMask", ParameterName::IndexVector{ rangeIndex }), "VkImageAspectFlagBits", AllVkImageAspectFlagBits, pRanges[rangeIndex].aspectMask, true);
        }
    }

    return skipCall;
}

static bool parameter_validation_vkCmdClearDepthStencilImage(
    debug_report_data*                          report_data,
    VkImage                                     image,
    VkImageLayout                               imageLayout,
    const VkClearDepthStencilValue*             pDepthStencil,
    uint32_t                                    rangeCount,
    const VkImageSubresourceRange*              pRanges)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdClearDepthStencilImage", "image", image);

    skipCall |= validate_ranged_enum(report_data, "vkCmdClearDepthStencilImage", "imageLayout", "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, imageLayout);

    skipCall |= validate_required_pointer(report_data, "vkCmdClearDepthStencilImage", "pDepthStencil", pDepthStencil);

    skipCall |= validate_array(report_data, "vkCmdClearDepthStencilImage", "rangeCount", "pRanges", rangeCount, pRanges, true, true);

    if (pRanges != NULL)
    {
        for (uint32_t rangeIndex = 0; rangeIndex < rangeCount; ++rangeIndex)
        {
            skipCall |= validate_flags(report_data, "vkCmdClearDepthStencilImage", ParameterName("pRanges[%i].aspectMask", ParameterName::IndexVector{ rangeIndex }), "VkImageAspectFlagBits", AllVkImageAspectFlagBits, pRanges[rangeIndex].aspectMask, true);
        }
    }

    return skipCall;
}

static bool parameter_validation_vkCmdClearAttachments(
    debug_report_data*                          report_data,
    uint32_t                                    attachmentCount,
    const VkClearAttachment*                    pAttachments,
    uint32_t                                    rectCount,
    const VkClearRect*                          pRects)
{
    bool skipCall = false;

    skipCall |= validate_array(report_data, "vkCmdClearAttachments", "attachmentCount", "pAttachments", attachmentCount, pAttachments, true, true);

    if (pAttachments != NULL)
    {
        for (uint32_t attachmentIndex = 0; attachmentIndex < attachmentCount; ++attachmentIndex)
        {
            skipCall |= validate_flags(report_data, "vkCmdClearAttachments", ParameterName("pAttachments[%i].aspectMask", ParameterName::IndexVector{ attachmentIndex }), "VkImageAspectFlagBits", AllVkImageAspectFlagBits, pAttachments[attachmentIndex].aspectMask, true);
        }
    }

    skipCall |= validate_array(report_data, "vkCmdClearAttachments", "rectCount", "pRects", rectCount, pRects, true, true);

    return skipCall;
}

static bool parameter_validation_vkCmdResolveImage(
    debug_report_data*                          report_data,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkImageResolve*                       pRegions)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdResolveImage", "srcImage", srcImage);

    skipCall |= validate_ranged_enum(report_data, "vkCmdResolveImage", "srcImageLayout", "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, srcImageLayout);

    skipCall |= validate_required_handle(report_data, "vkCmdResolveImage", "dstImage", dstImage);

    skipCall |= validate_ranged_enum(report_data, "vkCmdResolveImage", "dstImageLayout", "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, dstImageLayout);

    skipCall |= validate_array(report_data, "vkCmdResolveImage", "regionCount", "pRegions", regionCount, pRegions, true, true);

    if (pRegions != NULL)
    {
        for (uint32_t regionIndex = 0; regionIndex < regionCount; ++regionIndex)
        {
            skipCall |= validate_flags(report_data, "vkCmdResolveImage", ParameterName("pRegions[%i].srcSubresource.aspectMask", ParameterName::IndexVector{ regionIndex }), "VkImageAspectFlagBits", AllVkImageAspectFlagBits, pRegions[regionIndex].srcSubresource.aspectMask, true);

            skipCall |= validate_flags(report_data, "vkCmdResolveImage", ParameterName("pRegions[%i].dstSubresource.aspectMask", ParameterName::IndexVector{ regionIndex }), "VkImageAspectFlagBits", AllVkImageAspectFlagBits, pRegions[regionIndex].dstSubresource.aspectMask, true);
        }
    }

    return skipCall;
}

static bool parameter_validation_vkCmdSetEvent(
    debug_report_data*                          report_data,
    VkEvent                                     event,
    VkPipelineStageFlags                        stageMask)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdSetEvent", "event", event);

    skipCall |= validate_flags(report_data, "vkCmdSetEvent", "stageMask", "VkPipelineStageFlagBits", AllVkPipelineStageFlagBits, stageMask, true);

    return skipCall;
}

static bool parameter_validation_vkCmdResetEvent(
    debug_report_data*                          report_data,
    VkEvent                                     event,
    VkPipelineStageFlags                        stageMask)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdResetEvent", "event", event);

    skipCall |= validate_flags(report_data, "vkCmdResetEvent", "stageMask", "VkPipelineStageFlagBits", AllVkPipelineStageFlagBits, stageMask, true);

    return skipCall;
}

static bool parameter_validation_vkCmdWaitEvents(
    debug_report_data*                          report_data,
    uint32_t                                    eventCount,
    const VkEvent*                              pEvents,
    VkPipelineStageFlags                        srcStageMask,
    VkPipelineStageFlags                        dstStageMask,
    uint32_t                                    memoryBarrierCount,
    const VkMemoryBarrier*                      pMemoryBarriers,
    uint32_t                                    bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier*                pBufferMemoryBarriers,
    uint32_t                                    imageMemoryBarrierCount,
    const VkImageMemoryBarrier*                 pImageMemoryBarriers)
{
    bool skipCall = false;

    skipCall |= validate_handle_array(report_data, "vkCmdWaitEvents", "eventCount", "pEvents", eventCount, pEvents, true, true);

    skipCall |= validate_flags(report_data, "vkCmdWaitEvents", "srcStageMask", "VkPipelineStageFlagBits", AllVkPipelineStageFlagBits, srcStageMask, true);

    skipCall |= validate_flags(report_data, "vkCmdWaitEvents", "dstStageMask", "VkPipelineStageFlagBits", AllVkPipelineStageFlagBits, dstStageMask, true);

    skipCall |= validate_struct_type_array(report_data, "vkCmdWaitEvents", "memoryBarrierCount", "pMemoryBarriers", "VK_STRUCTURE_TYPE_MEMORY_BARRIER", memoryBarrierCount, pMemoryBarriers, VK_STRUCTURE_TYPE_MEMORY_BARRIER, false, true);

    if (pMemoryBarriers != NULL)
    {
        for (uint32_t memoryBarrierIndex = 0; memoryBarrierIndex < memoryBarrierCount; ++memoryBarrierIndex)
        {
            skipCall |= validate_struct_pnext(report_data, "vkCmdWaitEvents", ParameterName("pMemoryBarriers[%i].pNext", ParameterName::IndexVector{ memoryBarrierIndex }), NULL, pMemoryBarriers[memoryBarrierIndex].pNext, 0, NULL, GeneratedHeaderVersion);

            skipCall |= validate_flags(report_data, "vkCmdWaitEvents", ParameterName("pMemoryBarriers[%i].srcAccessMask", ParameterName::IndexVector{ memoryBarrierIndex }), "VkAccessFlagBits", AllVkAccessFlagBits, pMemoryBarriers[memoryBarrierIndex].srcAccessMask, false);

            skipCall |= validate_flags(report_data, "vkCmdWaitEvents", ParameterName("pMemoryBarriers[%i].dstAccessMask", ParameterName::IndexVector{ memoryBarrierIndex }), "VkAccessFlagBits", AllVkAccessFlagBits, pMemoryBarriers[memoryBarrierIndex].dstAccessMask, false);
        }
    }

    skipCall |= validate_struct_type_array(report_data, "vkCmdWaitEvents", "bufferMemoryBarrierCount", "pBufferMemoryBarriers", "VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER", bufferMemoryBarrierCount, pBufferMemoryBarriers, VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, false, true);

    if (pBufferMemoryBarriers != NULL)
    {
        for (uint32_t bufferMemoryBarrierIndex = 0; bufferMemoryBarrierIndex < bufferMemoryBarrierCount; ++bufferMemoryBarrierIndex)
        {
            skipCall |= validate_struct_pnext(report_data, "vkCmdWaitEvents", ParameterName("pBufferMemoryBarriers[%i].pNext", ParameterName::IndexVector{ bufferMemoryBarrierIndex }), NULL, pBufferMemoryBarriers[bufferMemoryBarrierIndex].pNext, 0, NULL, GeneratedHeaderVersion);

            skipCall |= validate_flags(report_data, "vkCmdWaitEvents", ParameterName("pBufferMemoryBarriers[%i].srcAccessMask", ParameterName::IndexVector{ bufferMemoryBarrierIndex }), "VkAccessFlagBits", AllVkAccessFlagBits, pBufferMemoryBarriers[bufferMemoryBarrierIndex].srcAccessMask, false);

            skipCall |= validate_flags(report_data, "vkCmdWaitEvents", ParameterName("pBufferMemoryBarriers[%i].dstAccessMask", ParameterName::IndexVector{ bufferMemoryBarrierIndex }), "VkAccessFlagBits", AllVkAccessFlagBits, pBufferMemoryBarriers[bufferMemoryBarrierIndex].dstAccessMask, false);

            skipCall |= validate_required_handle(report_data, "vkCmdWaitEvents", ParameterName("pBufferMemoryBarriers[%i].buffer", ParameterName::IndexVector{ bufferMemoryBarrierIndex }), pBufferMemoryBarriers[bufferMemoryBarrierIndex].buffer);
        }
    }

    skipCall |= validate_struct_type_array(report_data, "vkCmdWaitEvents", "imageMemoryBarrierCount", "pImageMemoryBarriers", "VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER", imageMemoryBarrierCount, pImageMemoryBarriers, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, false, true);

    if (pImageMemoryBarriers != NULL)
    {
        for (uint32_t imageMemoryBarrierIndex = 0; imageMemoryBarrierIndex < imageMemoryBarrierCount; ++imageMemoryBarrierIndex)
        {
            skipCall |= validate_struct_pnext(report_data, "vkCmdWaitEvents", ParameterName("pImageMemoryBarriers[%i].pNext", ParameterName::IndexVector{ imageMemoryBarrierIndex }), NULL, pImageMemoryBarriers[imageMemoryBarrierIndex].pNext, 0, NULL, GeneratedHeaderVersion);

            skipCall |= validate_flags(report_data, "vkCmdWaitEvents", ParameterName("pImageMemoryBarriers[%i].srcAccessMask", ParameterName::IndexVector{ imageMemoryBarrierIndex }), "VkAccessFlagBits", AllVkAccessFlagBits, pImageMemoryBarriers[imageMemoryBarrierIndex].srcAccessMask, false);

            skipCall |= validate_flags(report_data, "vkCmdWaitEvents", ParameterName("pImageMemoryBarriers[%i].dstAccessMask", ParameterName::IndexVector{ imageMemoryBarrierIndex }), "VkAccessFlagBits", AllVkAccessFlagBits, pImageMemoryBarriers[imageMemoryBarrierIndex].dstAccessMask, false);

            skipCall |= validate_ranged_enum(report_data, "vkCmdWaitEvents", ParameterName("pImageMemoryBarriers[%i].oldLayout", ParameterName::IndexVector{ imageMemoryBarrierIndex }), "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, pImageMemoryBarriers[imageMemoryBarrierIndex].oldLayout);

            skipCall |= validate_ranged_enum(report_data, "vkCmdWaitEvents", ParameterName("pImageMemoryBarriers[%i].newLayout", ParameterName::IndexVector{ imageMemoryBarrierIndex }), "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, pImageMemoryBarriers[imageMemoryBarrierIndex].newLayout);

            skipCall |= validate_required_handle(report_data, "vkCmdWaitEvents", ParameterName("pImageMemoryBarriers[%i].image", ParameterName::IndexVector{ imageMemoryBarrierIndex }), pImageMemoryBarriers[imageMemoryBarrierIndex].image);

            skipCall |= validate_flags(report_data, "vkCmdWaitEvents", ParameterName("pImageMemoryBarriers[%i].subresourceRange.aspectMask", ParameterName::IndexVector{ imageMemoryBarrierIndex }), "VkImageAspectFlagBits", AllVkImageAspectFlagBits, pImageMemoryBarriers[imageMemoryBarrierIndex].subresourceRange.aspectMask, true);
        }
    }

    return skipCall;
}

static bool parameter_validation_vkCmdPipelineBarrier(
    debug_report_data*                          report_data,
    VkPipelineStageFlags                        srcStageMask,
    VkPipelineStageFlags                        dstStageMask,
    VkDependencyFlags                           dependencyFlags,
    uint32_t                                    memoryBarrierCount,
    const VkMemoryBarrier*                      pMemoryBarriers,
    uint32_t                                    bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier*                pBufferMemoryBarriers,
    uint32_t                                    imageMemoryBarrierCount,
    const VkImageMemoryBarrier*                 pImageMemoryBarriers)
{
    bool skipCall = false;

    skipCall |= validate_flags(report_data, "vkCmdPipelineBarrier", "srcStageMask", "VkPipelineStageFlagBits", AllVkPipelineStageFlagBits, srcStageMask, true);

    skipCall |= validate_flags(report_data, "vkCmdPipelineBarrier", "dstStageMask", "VkPipelineStageFlagBits", AllVkPipelineStageFlagBits, dstStageMask, true);

    skipCall |= validate_flags(report_data, "vkCmdPipelineBarrier", "dependencyFlags", "VkDependencyFlagBits", AllVkDependencyFlagBits, dependencyFlags, false);

    skipCall |= validate_struct_type_array(report_data, "vkCmdPipelineBarrier", "memoryBarrierCount", "pMemoryBarriers", "VK_STRUCTURE_TYPE_MEMORY_BARRIER", memoryBarrierCount, pMemoryBarriers, VK_STRUCTURE_TYPE_MEMORY_BARRIER, false, true);

    if (pMemoryBarriers != NULL)
    {
        for (uint32_t memoryBarrierIndex = 0; memoryBarrierIndex < memoryBarrierCount; ++memoryBarrierIndex)
        {
            skipCall |= validate_struct_pnext(report_data, "vkCmdPipelineBarrier", ParameterName("pMemoryBarriers[%i].pNext", ParameterName::IndexVector{ memoryBarrierIndex }), NULL, pMemoryBarriers[memoryBarrierIndex].pNext, 0, NULL, GeneratedHeaderVersion);

            skipCall |= validate_flags(report_data, "vkCmdPipelineBarrier", ParameterName("pMemoryBarriers[%i].srcAccessMask", ParameterName::IndexVector{ memoryBarrierIndex }), "VkAccessFlagBits", AllVkAccessFlagBits, pMemoryBarriers[memoryBarrierIndex].srcAccessMask, false);

            skipCall |= validate_flags(report_data, "vkCmdPipelineBarrier", ParameterName("pMemoryBarriers[%i].dstAccessMask", ParameterName::IndexVector{ memoryBarrierIndex }), "VkAccessFlagBits", AllVkAccessFlagBits, pMemoryBarriers[memoryBarrierIndex].dstAccessMask, false);
        }
    }

    skipCall |= validate_struct_type_array(report_data, "vkCmdPipelineBarrier", "bufferMemoryBarrierCount", "pBufferMemoryBarriers", "VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER", bufferMemoryBarrierCount, pBufferMemoryBarriers, VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, false, true);

    if (pBufferMemoryBarriers != NULL)
    {
        for (uint32_t bufferMemoryBarrierIndex = 0; bufferMemoryBarrierIndex < bufferMemoryBarrierCount; ++bufferMemoryBarrierIndex)
        {
            skipCall |= validate_struct_pnext(report_data, "vkCmdPipelineBarrier", ParameterName("pBufferMemoryBarriers[%i].pNext", ParameterName::IndexVector{ bufferMemoryBarrierIndex }), NULL, pBufferMemoryBarriers[bufferMemoryBarrierIndex].pNext, 0, NULL, GeneratedHeaderVersion);

            skipCall |= validate_flags(report_data, "vkCmdPipelineBarrier", ParameterName("pBufferMemoryBarriers[%i].srcAccessMask", ParameterName::IndexVector{ bufferMemoryBarrierIndex }), "VkAccessFlagBits", AllVkAccessFlagBits, pBufferMemoryBarriers[bufferMemoryBarrierIndex].srcAccessMask, false);

            skipCall |= validate_flags(report_data, "vkCmdPipelineBarrier", ParameterName("pBufferMemoryBarriers[%i].dstAccessMask", ParameterName::IndexVector{ bufferMemoryBarrierIndex }), "VkAccessFlagBits", AllVkAccessFlagBits, pBufferMemoryBarriers[bufferMemoryBarrierIndex].dstAccessMask, false);

            skipCall |= validate_required_handle(report_data, "vkCmdPipelineBarrier", ParameterName("pBufferMemoryBarriers[%i].buffer", ParameterName::IndexVector{ bufferMemoryBarrierIndex }), pBufferMemoryBarriers[bufferMemoryBarrierIndex].buffer);
        }
    }

    skipCall |= validate_struct_type_array(report_data, "vkCmdPipelineBarrier", "imageMemoryBarrierCount", "pImageMemoryBarriers", "VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER", imageMemoryBarrierCount, pImageMemoryBarriers, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, false, true);

    if (pImageMemoryBarriers != NULL)
    {
        for (uint32_t imageMemoryBarrierIndex = 0; imageMemoryBarrierIndex < imageMemoryBarrierCount; ++imageMemoryBarrierIndex)
        {
            skipCall |= validate_struct_pnext(report_data, "vkCmdPipelineBarrier", ParameterName("pImageMemoryBarriers[%i].pNext", ParameterName::IndexVector{ imageMemoryBarrierIndex }), NULL, pImageMemoryBarriers[imageMemoryBarrierIndex].pNext, 0, NULL, GeneratedHeaderVersion);

            skipCall |= validate_flags(report_data, "vkCmdPipelineBarrier", ParameterName("pImageMemoryBarriers[%i].srcAccessMask", ParameterName::IndexVector{ imageMemoryBarrierIndex }), "VkAccessFlagBits", AllVkAccessFlagBits, pImageMemoryBarriers[imageMemoryBarrierIndex].srcAccessMask, false);

            skipCall |= validate_flags(report_data, "vkCmdPipelineBarrier", ParameterName("pImageMemoryBarriers[%i].dstAccessMask", ParameterName::IndexVector{ imageMemoryBarrierIndex }), "VkAccessFlagBits", AllVkAccessFlagBits, pImageMemoryBarriers[imageMemoryBarrierIndex].dstAccessMask, false);

            skipCall |= validate_ranged_enum(report_data, "vkCmdPipelineBarrier", ParameterName("pImageMemoryBarriers[%i].oldLayout", ParameterName::IndexVector{ imageMemoryBarrierIndex }), "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, pImageMemoryBarriers[imageMemoryBarrierIndex].oldLayout);

            skipCall |= validate_ranged_enum(report_data, "vkCmdPipelineBarrier", ParameterName("pImageMemoryBarriers[%i].newLayout", ParameterName::IndexVector{ imageMemoryBarrierIndex }), "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE, pImageMemoryBarriers[imageMemoryBarrierIndex].newLayout);

            skipCall |= validate_required_handle(report_data, "vkCmdPipelineBarrier", ParameterName("pImageMemoryBarriers[%i].image", ParameterName::IndexVector{ imageMemoryBarrierIndex }), pImageMemoryBarriers[imageMemoryBarrierIndex].image);

            skipCall |= validate_flags(report_data, "vkCmdPipelineBarrier", ParameterName("pImageMemoryBarriers[%i].subresourceRange.aspectMask", ParameterName::IndexVector{ imageMemoryBarrierIndex }), "VkImageAspectFlagBits", AllVkImageAspectFlagBits, pImageMemoryBarriers[imageMemoryBarrierIndex].subresourceRange.aspectMask, true);
        }
    }

    return skipCall;
}

static bool parameter_validation_vkCmdBeginQuery(
    debug_report_data*                          report_data,
    VkQueryPool                                 queryPool,
    uint32_t                                    query,
    VkQueryControlFlags                         flags)
{
    UNUSED_PARAMETER(query);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdBeginQuery", "queryPool", queryPool);

    skipCall |= validate_flags(report_data, "vkCmdBeginQuery", "flags", "VkQueryControlFlagBits", AllVkQueryControlFlagBits, flags, false);

    return skipCall;
}

static bool parameter_validation_vkCmdEndQuery(
    debug_report_data*                          report_data,
    VkQueryPool                                 queryPool,
    uint32_t                                    query)
{
    UNUSED_PARAMETER(query);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdEndQuery", "queryPool", queryPool);

    return skipCall;
}

static bool parameter_validation_vkCmdResetQueryPool(
    debug_report_data*                          report_data,
    VkQueryPool                                 queryPool,
    uint32_t                                    firstQuery,
    uint32_t                                    queryCount)
{
    UNUSED_PARAMETER(firstQuery);
    UNUSED_PARAMETER(queryCount);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdResetQueryPool", "queryPool", queryPool);

    return skipCall;
}

static bool parameter_validation_vkCmdWriteTimestamp(
    debug_report_data*                          report_data,
    VkPipelineStageFlagBits                     pipelineStage,
    VkQueryPool                                 queryPool,
    uint32_t                                    query)
{
    UNUSED_PARAMETER(pipelineStage);
    UNUSED_PARAMETER(query);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdWriteTimestamp", "queryPool", queryPool);

    return skipCall;
}

static bool parameter_validation_vkCmdCopyQueryPoolResults(
    debug_report_data*                          report_data,
    VkQueryPool                                 queryPool,
    uint32_t                                    firstQuery,
    uint32_t                                    queryCount,
    VkBuffer                                    dstBuffer,
    VkDeviceSize                                dstOffset,
    VkDeviceSize                                stride,
    VkQueryResultFlags                          flags)
{
    UNUSED_PARAMETER(firstQuery);
    UNUSED_PARAMETER(queryCount);
    UNUSED_PARAMETER(dstOffset);
    UNUSED_PARAMETER(stride);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdCopyQueryPoolResults", "queryPool", queryPool);

    skipCall |= validate_required_handle(report_data, "vkCmdCopyQueryPoolResults", "dstBuffer", dstBuffer);

    skipCall |= validate_flags(report_data, "vkCmdCopyQueryPoolResults", "flags", "VkQueryResultFlagBits", AllVkQueryResultFlagBits, flags, false);

    return skipCall;
}

static bool parameter_validation_vkCmdPushConstants(
    debug_report_data*                          report_data,
    VkPipelineLayout                            layout,
    VkShaderStageFlags                          stageFlags,
    uint32_t                                    offset,
    uint32_t                                    size,
    const void*                                 pValues)
{
    UNUSED_PARAMETER(offset);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCmdPushConstants", "layout", layout);

    skipCall |= validate_flags(report_data, "vkCmdPushConstants", "stageFlags", "VkShaderStageFlagBits", AllVkShaderStageFlagBits, stageFlags, true);

    skipCall |= validate_array(report_data, "vkCmdPushConstants", "size", "pValues", size, pValues, true, true);

    return skipCall;
}

static bool parameter_validation_vkCmdBeginRenderPass(
    debug_report_data*                          report_data,
    const VkRenderPassBeginInfo*                pRenderPassBegin,
    VkSubpassContents                           contents)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCmdBeginRenderPass", "pRenderPassBegin", "VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO", pRenderPassBegin, VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, true);

    if (pRenderPassBegin != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCmdBeginRenderPass", "pRenderPassBegin->pNext", NULL, pRenderPassBegin->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_required_handle(report_data, "vkCmdBeginRenderPass", "pRenderPassBegin->renderPass", pRenderPassBegin->renderPass);

        skipCall |= validate_required_handle(report_data, "vkCmdBeginRenderPass", "pRenderPassBegin->framebuffer", pRenderPassBegin->framebuffer);

        skipCall |= validate_array(report_data, "vkCmdBeginRenderPass", "pRenderPassBegin->clearValueCount", "pRenderPassBegin->pClearValues", pRenderPassBegin->clearValueCount, pRenderPassBegin->pClearValues, false, true);
    }

    skipCall |= validate_ranged_enum(report_data, "vkCmdBeginRenderPass", "contents", "VkSubpassContents", VK_SUBPASS_CONTENTS_BEGIN_RANGE, VK_SUBPASS_CONTENTS_END_RANGE, contents);

    return skipCall;
}

static bool parameter_validation_vkCmdNextSubpass(
    debug_report_data*                          report_data,
    VkSubpassContents                           contents)
{
    bool skipCall = false;

    skipCall |= validate_ranged_enum(report_data, "vkCmdNextSubpass", "contents", "VkSubpassContents", VK_SUBPASS_CONTENTS_BEGIN_RANGE, VK_SUBPASS_CONTENTS_END_RANGE, contents);

    return skipCall;
}

static bool parameter_validation_vkCmdExecuteCommands(
    debug_report_data*                          report_data,
    uint32_t                                    commandBufferCount,
    const VkCommandBuffer*                      pCommandBuffers)
{
    bool skipCall = false;

    skipCall |= validate_handle_array(report_data, "vkCmdExecuteCommands", "commandBufferCount", "pCommandBuffers", commandBufferCount, pCommandBuffers, true, true);

    return skipCall;
}


const VkCompositeAlphaFlagsKHR AllVkCompositeAlphaFlagBitsKHR = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR|VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR|VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR|VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
const VkSurfaceTransformFlagsKHR AllVkSurfaceTransformFlagBitsKHR = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR|VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR|VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR|VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR|VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR|VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR|VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR|VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR|VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR;

static bool parameter_validation_vkGetPhysicalDeviceSurfaceSupportKHR(
    debug_report_data*                          report_data,
    uint32_t                                    queueFamilyIndex,
    VkSurfaceKHR                                surface,
    VkBool32*                                   pSupported)
{
    UNUSED_PARAMETER(queueFamilyIndex);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkGetPhysicalDeviceSurfaceSupportKHR", "surface", surface);

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceSurfaceSupportKHR", "pSupported", pSupported);

    return skipCall;
}

static bool parameter_validation_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    debug_report_data*                          report_data,
    VkSurfaceKHR                                surface,
    VkSurfaceCapabilitiesKHR*                   pSurfaceCapabilities)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR", "surface", surface);

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR", "pSurfaceCapabilities", pSurfaceCapabilities);

    return skipCall;
}

static bool parameter_validation_vkGetPhysicalDeviceSurfaceFormatsKHR(
    debug_report_data*                          report_data,
    VkSurfaceKHR                                surface,
    uint32_t*                                   pSurfaceFormatCount,
    VkSurfaceFormatKHR*                         pSurfaceFormats)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkGetPhysicalDeviceSurfaceFormatsKHR", "surface", surface);

    skipCall |= validate_array(report_data, "vkGetPhysicalDeviceSurfaceFormatsKHR", "pSurfaceFormatCount", "pSurfaceFormats", pSurfaceFormatCount, pSurfaceFormats, true, false, false);

    return skipCall;
}

static bool parameter_validation_vkGetPhysicalDeviceSurfacePresentModesKHR(
    debug_report_data*                          report_data,
    VkSurfaceKHR                                surface,
    uint32_t*                                   pPresentModeCount,
    VkPresentModeKHR*                           pPresentModes)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkGetPhysicalDeviceSurfacePresentModesKHR", "surface", surface);

    skipCall |= validate_array(report_data, "vkGetPhysicalDeviceSurfacePresentModesKHR", "pPresentModeCount", "pPresentModes", pPresentModeCount, pPresentModes, true, false, false);

    return skipCall;
}



static bool parameter_validation_vkCreateSwapchainKHR(
    debug_report_data*                          report_data,
    const VkSwapchainCreateInfoKHR*             pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSwapchainKHR*                             pSwapchain)
{
    UNUSED_PARAMETER(pAllocator);

    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateSwapchainKHR", "pCreateInfo", "VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR", pCreateInfo, VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateSwapchainKHR", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateSwapchainKHR", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_bool32(report_data, "vkCreateSwapchainKHR", "pCreateInfo->clipped", pCreateInfo->clipped);
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateSwapchainKHR", "pSwapchain", pSwapchain);

    return skipCall;
}

static bool parameter_validation_vkGetSwapchainImagesKHR(
    debug_report_data*                          report_data,
    VkSwapchainKHR                              swapchain,
    uint32_t*                                   pSwapchainImageCount,
    VkImage*                                    pSwapchainImages)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkGetSwapchainImagesKHR", "swapchain", swapchain);

    skipCall |= validate_array(report_data, "vkGetSwapchainImagesKHR", "pSwapchainImageCount", "pSwapchainImages", pSwapchainImageCount, pSwapchainImages, true, false, false);

    return skipCall;
}

static bool parameter_validation_vkAcquireNextImageKHR(
    debug_report_data*                          report_data,
    VkSwapchainKHR                              swapchain,
    uint64_t                                    timeout,
    VkSemaphore                                 semaphore,
    VkFence                                     fence,
    uint32_t*                                   pImageIndex)
{
    UNUSED_PARAMETER(timeout);
    UNUSED_PARAMETER(semaphore);
    UNUSED_PARAMETER(fence);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkAcquireNextImageKHR", "swapchain", swapchain);

    skipCall |= validate_required_pointer(report_data, "vkAcquireNextImageKHR", "pImageIndex", pImageIndex);

    return skipCall;
}

static bool parameter_validation_vkQueuePresentKHR(
    debug_report_data*                          report_data,
    const VkPresentInfoKHR*                     pPresentInfo)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkQueuePresentKHR", "pPresentInfo", "VK_STRUCTURE_TYPE_PRESENT_INFO_KHR", pPresentInfo, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, true);

    if (pPresentInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkQueuePresentKHR", "pPresentInfo->pNext", NULL, pPresentInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_handle_array(report_data, "vkQueuePresentKHR", "pPresentInfo->swapchainCount", "pPresentInfo->pSwapchains", pPresentInfo->swapchainCount, pPresentInfo->pSwapchains, true, true);

        skipCall |= validate_array(report_data, "vkQueuePresentKHR", "pPresentInfo->swapchainCount", "pPresentInfo->pImageIndices", pPresentInfo->swapchainCount, pPresentInfo->pImageIndices, true, true);

        skipCall |= validate_array(report_data, "vkQueuePresentKHR", "pPresentInfo->swapchainCount", "pPresentInfo->pResults", pPresentInfo->swapchainCount, pPresentInfo->pResults, true, false);
    }

    return skipCall;
}


const VkDisplayPlaneAlphaFlagsKHR AllVkDisplayPlaneAlphaFlagBitsKHR = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR|VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR|VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR|VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR;

static bool parameter_validation_vkGetPhysicalDeviceDisplayPropertiesKHR(
    debug_report_data*                          report_data,
    uint32_t*                                   pPropertyCount,
    VkDisplayPropertiesKHR*                     pProperties)
{
    bool skipCall = false;

    skipCall |= validate_array(report_data, "vkGetPhysicalDeviceDisplayPropertiesKHR", "pPropertyCount", "pProperties", pPropertyCount, pProperties, true, false, false);

    return skipCall;
}

static bool parameter_validation_vkGetPhysicalDeviceDisplayPlanePropertiesKHR(
    debug_report_data*                          report_data,
    uint32_t*                                   pPropertyCount,
    VkDisplayPlanePropertiesKHR*                pProperties)
{
    bool skipCall = false;

    skipCall |= validate_array(report_data, "vkGetPhysicalDeviceDisplayPlanePropertiesKHR", "pPropertyCount", "pProperties", pPropertyCount, pProperties, true, false, false);

    return skipCall;
}

static bool parameter_validation_vkGetDisplayPlaneSupportedDisplaysKHR(
    debug_report_data*                          report_data,
    uint32_t                                    planeIndex,
    uint32_t*                                   pDisplayCount,
    VkDisplayKHR*                               pDisplays)
{
    UNUSED_PARAMETER(planeIndex);

    bool skipCall = false;

    skipCall |= validate_array(report_data, "vkGetDisplayPlaneSupportedDisplaysKHR", "pDisplayCount", "pDisplays", pDisplayCount, pDisplays, true, false, false);

    return skipCall;
}

static bool parameter_validation_vkGetDisplayModePropertiesKHR(
    debug_report_data*                          report_data,
    VkDisplayKHR                                display,
    uint32_t*                                   pPropertyCount,
    VkDisplayModePropertiesKHR*                 pProperties)
{
    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkGetDisplayModePropertiesKHR", "display", display);

    skipCall |= validate_array(report_data, "vkGetDisplayModePropertiesKHR", "pPropertyCount", "pProperties", pPropertyCount, pProperties, true, false, false);

    return skipCall;
}

static bool parameter_validation_vkCreateDisplayModeKHR(
    debug_report_data*                          report_data,
    VkDisplayKHR                                display,
    const VkDisplayModeCreateInfoKHR*           pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDisplayModeKHR*                           pMode)
{
    UNUSED_PARAMETER(pAllocator);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkCreateDisplayModeKHR", "display", display);

    skipCall |= validate_struct_type(report_data, "vkCreateDisplayModeKHR", "pCreateInfo", "VK_STRUCTURE_TYPE_DISPLAY_MODE_CREATE_INFO_KHR", pCreateInfo, VK_STRUCTURE_TYPE_DISPLAY_MODE_CREATE_INFO_KHR, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateDisplayModeKHR", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateDisplayModeKHR", "pCreateInfo->flags", pCreateInfo->flags);
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateDisplayModeKHR", "pMode", pMode);

    return skipCall;
}

static bool parameter_validation_vkGetDisplayPlaneCapabilitiesKHR(
    debug_report_data*                          report_data,
    VkDisplayModeKHR                            mode,
    uint32_t                                    planeIndex,
    VkDisplayPlaneCapabilitiesKHR*              pCapabilities)
{
    UNUSED_PARAMETER(planeIndex);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkGetDisplayPlaneCapabilitiesKHR", "mode", mode);

    skipCall |= validate_required_pointer(report_data, "vkGetDisplayPlaneCapabilitiesKHR", "pCapabilities", pCapabilities);

    return skipCall;
}

static bool parameter_validation_vkCreateDisplayPlaneSurfaceKHR(
    debug_report_data*                          report_data,
    const VkDisplaySurfaceCreateInfoKHR*        pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    UNUSED_PARAMETER(pAllocator);

    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateDisplayPlaneSurfaceKHR", "pCreateInfo", "VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR", pCreateInfo, VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateDisplayPlaneSurfaceKHR", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateDisplayPlaneSurfaceKHR", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_required_handle(report_data, "vkCreateDisplayPlaneSurfaceKHR", "pCreateInfo->displayMode", pCreateInfo->displayMode);
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateDisplayPlaneSurfaceKHR", "pSurface", pSurface);

    return skipCall;
}



static bool parameter_validation_vkCreateSharedSwapchainsKHR(
    debug_report_data*                          report_data,
    uint32_t                                    swapchainCount,
    const VkSwapchainCreateInfoKHR*             pCreateInfos,
    const VkAllocationCallbacks*                pAllocator,
    VkSwapchainKHR*                             pSwapchains)
{
    UNUSED_PARAMETER(pAllocator);

    bool skipCall = false;

    skipCall |= validate_array(report_data, "vkCreateSharedSwapchainsKHR", "swapchainCount", "pCreateInfos", swapchainCount, pCreateInfos, true, true);

    skipCall |= validate_array(report_data, "vkCreateSharedSwapchainsKHR", "swapchainCount", "pSwapchains", swapchainCount, pSwapchains, true, true);

    return skipCall;
}


#ifdef VK_USE_PLATFORM_XLIB_KHR

static bool parameter_validation_vkCreateXlibSurfaceKHR(
    debug_report_data*                          report_data,
    const VkXlibSurfaceCreateInfoKHR*           pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    UNUSED_PARAMETER(pAllocator);

    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateXlibSurfaceKHR", "pCreateInfo", "VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR", pCreateInfo, VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateXlibSurfaceKHR", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateXlibSurfaceKHR", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_required_pointer(report_data, "vkCreateXlibSurfaceKHR", "pCreateInfo->dpy", pCreateInfo->dpy);
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateXlibSurfaceKHR", "pSurface", pSurface);

    return skipCall;
}

static bool parameter_validation_vkGetPhysicalDeviceXlibPresentationSupportKHR(
    debug_report_data*                          report_data,
    uint32_t                                    queueFamilyIndex,
    Display*                                    dpy,
    VisualID                                    visualID)
{
    UNUSED_PARAMETER(queueFamilyIndex);
    UNUSED_PARAMETER(visualID);

    bool skipCall = false;

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceXlibPresentationSupportKHR", "dpy", dpy);

    return skipCall;
}
#endif /* VK_USE_PLATFORM_XLIB_KHR */

#ifdef VK_USE_PLATFORM_XCB_KHR

static bool parameter_validation_vkCreateXcbSurfaceKHR(
    debug_report_data*                          report_data,
    const VkXcbSurfaceCreateInfoKHR*            pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    UNUSED_PARAMETER(pAllocator);

    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateXcbSurfaceKHR", "pCreateInfo", "VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR", pCreateInfo, VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateXcbSurfaceKHR", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateXcbSurfaceKHR", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_required_pointer(report_data, "vkCreateXcbSurfaceKHR", "pCreateInfo->connection", pCreateInfo->connection);
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateXcbSurfaceKHR", "pSurface", pSurface);

    return skipCall;
}

static bool parameter_validation_vkGetPhysicalDeviceXcbPresentationSupportKHR(
    debug_report_data*                          report_data,
    uint32_t                                    queueFamilyIndex,
    xcb_connection_t*                           connection,
    xcb_visualid_t                              visual_id)
{
    UNUSED_PARAMETER(queueFamilyIndex);
    UNUSED_PARAMETER(visual_id);

    bool skipCall = false;

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceXcbPresentationSupportKHR", "connection", connection);

    return skipCall;
}
#endif /* VK_USE_PLATFORM_XCB_KHR */

#ifdef VK_USE_PLATFORM_WAYLAND_KHR

static bool parameter_validation_vkCreateWaylandSurfaceKHR(
    debug_report_data*                          report_data,
    const VkWaylandSurfaceCreateInfoKHR*        pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    UNUSED_PARAMETER(pAllocator);

    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateWaylandSurfaceKHR", "pCreateInfo", "VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR", pCreateInfo, VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateWaylandSurfaceKHR", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateWaylandSurfaceKHR", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_required_pointer(report_data, "vkCreateWaylandSurfaceKHR", "pCreateInfo->display", pCreateInfo->display);

        skipCall |= validate_required_pointer(report_data, "vkCreateWaylandSurfaceKHR", "pCreateInfo->surface", pCreateInfo->surface);
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateWaylandSurfaceKHR", "pSurface", pSurface);

    return skipCall;
}

static bool parameter_validation_vkGetPhysicalDeviceWaylandPresentationSupportKHR(
    debug_report_data*                          report_data,
    uint32_t                                    queueFamilyIndex,
    struct wl_display*                          display)
{
    UNUSED_PARAMETER(queueFamilyIndex);

    bool skipCall = false;

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceWaylandPresentationSupportKHR", "display", display);

    return skipCall;
}
#endif /* VK_USE_PLATFORM_WAYLAND_KHR */

#ifdef VK_USE_PLATFORM_MIR_KHR

static bool parameter_validation_vkCreateMirSurfaceKHR(
    debug_report_data*                          report_data,
    const VkMirSurfaceCreateInfoKHR*            pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    UNUSED_PARAMETER(pAllocator);

    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateMirSurfaceKHR", "pCreateInfo", "VK_STRUCTURE_TYPE_MIR_SURFACE_CREATE_INFO_KHR", pCreateInfo, VK_STRUCTURE_TYPE_MIR_SURFACE_CREATE_INFO_KHR, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateMirSurfaceKHR", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateMirSurfaceKHR", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_required_pointer(report_data, "vkCreateMirSurfaceKHR", "pCreateInfo->connection", pCreateInfo->connection);

        skipCall |= validate_required_pointer(report_data, "vkCreateMirSurfaceKHR", "pCreateInfo->mirSurface", pCreateInfo->mirSurface);
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateMirSurfaceKHR", "pSurface", pSurface);

    return skipCall;
}

static bool parameter_validation_vkGetPhysicalDeviceMirPresentationSupportKHR(
    debug_report_data*                          report_data,
    uint32_t                                    queueFamilyIndex,
    MirConnection*                              connection)
{
    UNUSED_PARAMETER(queueFamilyIndex);

    bool skipCall = false;

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceMirPresentationSupportKHR", "connection", connection);

    return skipCall;
}
#endif /* VK_USE_PLATFORM_MIR_KHR */

#ifdef VK_USE_PLATFORM_ANDROID_KHR

static bool parameter_validation_vkCreateAndroidSurfaceKHR(
    debug_report_data*                          report_data,
    const VkAndroidSurfaceCreateInfoKHR*        pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    UNUSED_PARAMETER(pAllocator);

    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateAndroidSurfaceKHR", "pCreateInfo", "VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR", pCreateInfo, VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateAndroidSurfaceKHR", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateAndroidSurfaceKHR", "pCreateInfo->flags", pCreateInfo->flags);

        skipCall |= validate_required_pointer(report_data, "vkCreateAndroidSurfaceKHR", "pCreateInfo->window", pCreateInfo->window);
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateAndroidSurfaceKHR", "pSurface", pSurface);

    return skipCall;
}
#endif /* VK_USE_PLATFORM_ANDROID_KHR */

#ifdef VK_USE_PLATFORM_WIN32_KHR

static bool parameter_validation_vkCreateWin32SurfaceKHR(
    debug_report_data*                          report_data,
    const VkWin32SurfaceCreateInfoKHR*          pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    UNUSED_PARAMETER(pAllocator);

    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCreateWin32SurfaceKHR", "pCreateInfo", "VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR", pCreateInfo, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, true);

    if (pCreateInfo != NULL)
    {
        skipCall |= validate_struct_pnext(report_data, "vkCreateWin32SurfaceKHR", "pCreateInfo->pNext", NULL, pCreateInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skipCall |= validate_reserved_flags(report_data, "vkCreateWin32SurfaceKHR", "pCreateInfo->flags", pCreateInfo->flags);
    }

    skipCall |= validate_required_pointer(report_data, "vkCreateWin32SurfaceKHR", "pSurface", pSurface);

    return skipCall;
}
#endif /* VK_USE_PLATFORM_WIN32_KHR */




const VkDebugReportFlagsEXT AllVkDebugReportFlagBitsEXT = VK_DEBUG_REPORT_INFORMATION_BIT_EXT|VK_DEBUG_REPORT_WARNING_BIT_EXT|VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT|VK_DEBUG_REPORT_ERROR_BIT_EXT|VK_DEBUG_REPORT_DEBUG_BIT_EXT;

static bool parameter_validation_vkDestroyDebugReportCallbackEXT(
    debug_report_data*                          report_data,
    VkDebugReportCallbackEXT                    callback,
    const VkAllocationCallbacks*                pAllocator)
{
    UNUSED_PARAMETER(pAllocator);

    bool skipCall = false;

    skipCall |= validate_required_handle(report_data, "vkDestroyDebugReportCallbackEXT", "callback", callback);

    return skipCall;
}


















static bool parameter_validation_vkDebugMarkerSetObjectTagEXT(
    debug_report_data*                          report_data,
    VkDebugMarkerObjectTagInfoEXT*              pTagInfo)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkDebugMarkerSetObjectTagEXT", "pTagInfo", "VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT", pTagInfo, VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT, true);

    return skipCall;
}

static bool parameter_validation_vkDebugMarkerSetObjectNameEXT(
    debug_report_data*                          report_data,
    VkDebugMarkerObjectNameInfoEXT*             pNameInfo)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkDebugMarkerSetObjectNameEXT", "pNameInfo", "VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT", pNameInfo, VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT, true);

    return skipCall;
}

static bool parameter_validation_vkCmdDebugMarkerBeginEXT(
    debug_report_data*                          report_data,
    VkDebugMarkerMarkerInfoEXT*                 pMarkerInfo)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCmdDebugMarkerBeginEXT", "pMarkerInfo", "VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT", pMarkerInfo, VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT, true);

    return skipCall;
}

static bool parameter_validation_vkCmdDebugMarkerInsertEXT(
    debug_report_data*                          report_data,
    VkDebugMarkerMarkerInfoEXT*                 pMarkerInfo)
{
    bool skipCall = false;

    skipCall |= validate_struct_type(report_data, "vkCmdDebugMarkerInsertEXT", "pMarkerInfo", "VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT", pMarkerInfo, VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT, true);

    return skipCall;
}























const VkExternalMemoryFeatureFlagsNV AllVkExternalMemoryFeatureFlagBitsNV = VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT_NV|VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_NV|VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT_NV;
const VkExternalMemoryHandleTypeFlagsNV AllVkExternalMemoryHandleTypeFlagBitsNV = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_NV|VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_NV|VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_IMAGE_BIT_NV|VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_IMAGE_KMT_BIT_NV;

static bool parameter_validation_vkGetPhysicalDeviceExternalImageFormatPropertiesNV(
    debug_report_data*                          report_data,
    VkFormat                                    format,
    VkImageType                                 type,
    VkImageTiling                               tiling,
    VkImageUsageFlags                           usage,
    VkImageCreateFlags                          flags,
    VkExternalMemoryHandleTypeFlagsNV           externalHandleType,
    VkExternalImageFormatPropertiesNV*          pExternalImageFormatProperties)
{
    UNUSED_PARAMETER(format);
    UNUSED_PARAMETER(type);
    UNUSED_PARAMETER(tiling);
    UNUSED_PARAMETER(usage);
    UNUSED_PARAMETER(flags);

    bool skipCall = false;

    skipCall |= validate_flags(report_data, "vkGetPhysicalDeviceExternalImageFormatPropertiesNV", "externalHandleType", "VkExternalMemoryHandleTypeFlagBitsNV", AllVkExternalMemoryHandleTypeFlagBitsNV, externalHandleType, true);

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceExternalImageFormatPropertiesNV", "pExternalImageFormatProperties", pExternalImageFormatProperties);

    return skipCall;
}





#ifdef VK_USE_PLATFORM_WIN32_KHR

static bool parameter_validation_vkGetMemoryWin32HandleNV(
    debug_report_data*                          report_data,
    VkDeviceMemory                              memory,
    VkExternalMemoryHandleTypeFlagsNV           handleType,
    HANDLE*                                     pHandle)
{
    UNUSED_PARAMETER(memory);
    UNUSED_PARAMETER(handleType);

    bool skipCall = false;

    skipCall |= validate_required_pointer(report_data, "vkGetMemoryWin32HandleNV", "pHandle", pHandle);

    return skipCall;
}
#endif /* VK_USE_PLATFORM_WIN32_KHR */

#ifdef VK_USE_PLATFORM_WIN32_KHR

#endif /* VK_USE_PLATFORM_WIN32_KHR */




} // namespace parameter_validation

#endif
