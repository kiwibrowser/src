/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (C) 2015-2016 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Jeremy Hayes <jeremy@lunarg.com>
 * Author: Tony Barbour <tony@LunarG.com>
 * Author: Mark Lobodzinski <mark@LunarG.com>
 * Author: Dustin Graves <dustin@lunarg.com>
 */

#define NOMINMAX

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "vk_loader_platform.h"
#include "vulkan/vk_layer.h"
#include "vk_layer_config.h"
#include "vk_enum_validate_helper.h"
#include "vk_struct_validate_helper.h"

#include "vk_layer_table.h"
#include "vk_layer_data.h"
#include "vk_layer_logging.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_utils.h"

#include "parameter_name.h"
#include "parameter_validation.h"

namespace parameter_validation {

struct layer_data {
    VkInstance instance;

    debug_report_data *report_data;
    std::vector<VkDebugReportCallbackEXT> logging_callback;

    // The following are for keeping track of the temporary callbacks that can
    // be used in vkCreateInstance and vkDestroyInstance:
    uint32_t num_tmp_callbacks;
    VkDebugReportCallbackCreateInfoEXT *tmp_dbg_create_infos;
    VkDebugReportCallbackEXT *tmp_callbacks;

    // TODO: Split instance/device structs
    // Device Data
    // Map for queue family index to queue count
    std::unordered_map<uint32_t, uint32_t> queueFamilyIndexMap;
    VkPhysicalDeviceLimits device_limits;
    VkPhysicalDeviceFeatures physical_device_features;
    VkPhysicalDevice physical_device;

    bool wsi_enabled;
    bool wsi_display_swapchain_enabled;

    layer_data()
        : report_data(nullptr), num_tmp_callbacks(0), tmp_dbg_create_infos(nullptr), tmp_callbacks(nullptr), device_limits{},
          physical_device_features{}, physical_device{}, wsi_enabled(false), wsi_display_swapchain_enabled(false) {};
};

static std::unordered_map<void *, struct instance_extension_enables> instance_extension_map;
static std::unordered_map<void *, layer_data *> layer_data_map;
static device_table_map pc_device_table_map;
static instance_table_map pc_instance_table_map;

// "my instance data"
debug_report_data *mid(VkInstance object) {
    dispatch_key key = get_dispatch_key(object);
    layer_data *data = get_my_data_ptr(key, layer_data_map);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "MID: map:  0x%p, object:  0x%p, key:  0x%p, data:  0x%p\n", &layer_data_map, object, key, data);
#endif
    assert(data != NULL);

    return data->report_data;
}

// "my device data"
debug_report_data *mdd(void *object) {
    dispatch_key key = get_dispatch_key(object);
    layer_data *data = get_my_data_ptr(key, layer_data_map);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "MDD: map:  0x%p, object:  0x%p, key:  0x%p, data:  0x%p\n", &layer_data_map, object, key, data);
#endif
    assert(data != NULL);
    return data->report_data;
}

static void init_parameter_validation(layer_data *my_data, const VkAllocationCallbacks *pAllocator) {

    layer_debug_actions(my_data->report_data, my_data->logging_callback, pAllocator, "lunarg_parameter_validation");
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDebugReportCallbackEXT(VkInstance instance,
                                                            const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                                                            const VkAllocationCallbacks *pAllocator,
                                                            VkDebugReportCallbackEXT *pMsgCallback) {
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(pc_instance_table_map, instance);
    VkResult result = pTable->CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);

    if (result == VK_SUCCESS) {
        layer_data *data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
        result = layer_create_msg_callback(data->report_data, false, pCreateInfo, pAllocator, pMsgCallback);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT msgCallback,
                                                         const VkAllocationCallbacks *pAllocator) {
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(pc_instance_table_map, instance);
    pTable->DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);

    layer_data *data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    layer_destroy_msg_callback(data->report_data, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL DebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags,
                                                 VkDebugReportObjectTypeEXT objType, uint64_t object, size_t location,
                                                 int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(pc_instance_table_map, instance);
    pTable->DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

static const VkExtensionProperties instance_extensions[] = {{VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_SPEC_VERSION}};

static const VkLayerProperties global_layer = {
    "VK_LAYER_LUNARG_parameter_validation", VK_LAYER_API_VERSION, 1, "LunarG Validation Layer",
};

static bool ValidateEnumerator(VkFormatFeatureFlagBits const &enumerator) {
    VkFormatFeatureFlagBits allFlags = (VkFormatFeatureFlagBits)(
        VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT |
        VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT | VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT |
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT | VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT |
        VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT |
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkFormatFeatureFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT) {
        strings.push_back("VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT");
    }
    if (enumerator & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) {
        strings.push_back("VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT");
    }
    if (enumerator & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT) {
        strings.push_back("VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT");
    }
    if (enumerator & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT) {
        strings.push_back("VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT");
    }
    if (enumerator & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
        strings.push_back("VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT");
    }
    if (enumerator & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT) {
        strings.push_back("VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT");
    }
    if (enumerator & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT) {
        strings.push_back("VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT");
    }
    if (enumerator & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT) {
        strings.push_back("VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT");
    }
    if (enumerator & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) {
        strings.push_back("VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT");
    }
    if (enumerator & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        strings.push_back("VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT");
    }
    if (enumerator & VK_FORMAT_FEATURE_BLIT_SRC_BIT) {
        strings.push_back("VK_FORMAT_FEATURE_BLIT_SRC_BIT");
    }
    if (enumerator & VK_FORMAT_FEATURE_BLIT_DST_BIT) {
        strings.push_back("VK_FORMAT_FEATURE_BLIT_DST_BIT");
    }
    if (enumerator & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) {
        strings.push_back("VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkImageUsageFlagBits const &enumerator) {
    VkImageUsageFlagBits allFlags = (VkImageUsageFlagBits)(
        VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkImageUsageFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
        strings.push_back("VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT");
    }
    if (enumerator & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        strings.push_back("VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT");
    }
    if (enumerator & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        strings.push_back("VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT");
    }
    if (enumerator & VK_IMAGE_USAGE_STORAGE_BIT) {
        strings.push_back("VK_IMAGE_USAGE_STORAGE_BIT");
    }
    if (enumerator & VK_IMAGE_USAGE_SAMPLED_BIT) {
        strings.push_back("VK_IMAGE_USAGE_SAMPLED_BIT");
    }
    if (enumerator & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        strings.push_back("VK_IMAGE_USAGE_TRANSFER_DST_BIT");
    }
    if (enumerator & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) {
        strings.push_back("VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT");
    }
    if (enumerator & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        strings.push_back("VK_IMAGE_USAGE_TRANSFER_SRC_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkQueueFlagBits const &enumerator) {
    VkQueueFlagBits allFlags =
        (VkQueueFlagBits)(VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_SPARSE_BINDING_BIT | VK_QUEUE_GRAPHICS_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkQueueFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_QUEUE_TRANSFER_BIT) {
        strings.push_back("VK_QUEUE_TRANSFER_BIT");
    }
    if (enumerator & VK_QUEUE_COMPUTE_BIT) {
        strings.push_back("VK_QUEUE_COMPUTE_BIT");
    }
    if (enumerator & VK_QUEUE_SPARSE_BINDING_BIT) {
        strings.push_back("VK_QUEUE_SPARSE_BINDING_BIT");
    }
    if (enumerator & VK_QUEUE_GRAPHICS_BIT) {
        strings.push_back("VK_QUEUE_GRAPHICS_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkMemoryPropertyFlagBits const &enumerator) {
    VkMemoryPropertyFlagBits allFlags = (VkMemoryPropertyFlagBits)(
        VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkMemoryPropertyFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
        strings.push_back("VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT");
    }
    if (enumerator & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
        strings.push_back("VK_MEMORY_PROPERTY_HOST_COHERENT_BIT");
    }
    if (enumerator & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        strings.push_back("VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT");
    }
    if (enumerator & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
        strings.push_back("VK_MEMORY_PROPERTY_HOST_CACHED_BIT");
    }
    if (enumerator & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
        strings.push_back("VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkMemoryHeapFlagBits const &enumerator) {
    VkMemoryHeapFlagBits allFlags = (VkMemoryHeapFlagBits)(VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkMemoryHeapFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
        strings.push_back("VK_MEMORY_HEAP_DEVICE_LOCAL_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkSparseImageFormatFlagBits const &enumerator) {
    VkSparseImageFormatFlagBits allFlags =
        (VkSparseImageFormatFlagBits)(VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT |
                                      VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT | VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkSparseImageFormatFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT) {
        strings.push_back("VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT");
    }
    if (enumerator & VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT) {
        strings.push_back("VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT");
    }
    if (enumerator & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) {
        strings.push_back("VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkFenceCreateFlagBits const &enumerator) {
    VkFenceCreateFlagBits allFlags = (VkFenceCreateFlagBits)(VK_FENCE_CREATE_SIGNALED_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkFenceCreateFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_FENCE_CREATE_SIGNALED_BIT) {
        strings.push_back("VK_FENCE_CREATE_SIGNALED_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkQueryPipelineStatisticFlagBits const &enumerator) {
    VkQueryPipelineStatisticFlagBits allFlags = (VkQueryPipelineStatisticFlagBits)(
        VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT | VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkQueryPipelineStatisticFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT) {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT");
    }
    if (enumerator & VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT) {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT");
    }
    if (enumerator & VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT) {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT");
    }
    if (enumerator & VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT) {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT");
    }
    if (enumerator & VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT) {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT");
    }
    if (enumerator & VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT) {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT");
    }
    if (enumerator & VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT) {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT");
    }
    if (enumerator & VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT) {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT");
    }
    if (enumerator & VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT) {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT");
    }
    if (enumerator & VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT) {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT");
    }
    if (enumerator & VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT) {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkQueryResultFlagBits const &enumerator) {
    VkQueryResultFlagBits allFlags = (VkQueryResultFlagBits)(VK_QUERY_RESULT_PARTIAL_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT |
                                                             VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_64_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkQueryResultFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_QUERY_RESULT_PARTIAL_BIT) {
        strings.push_back("VK_QUERY_RESULT_PARTIAL_BIT");
    }
    if (enumerator & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) {
        strings.push_back("VK_QUERY_RESULT_WITH_AVAILABILITY_BIT");
    }
    if (enumerator & VK_QUERY_RESULT_WAIT_BIT) {
        strings.push_back("VK_QUERY_RESULT_WAIT_BIT");
    }
    if (enumerator & VK_QUERY_RESULT_64_BIT) {
        strings.push_back("VK_QUERY_RESULT_64_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkBufferUsageFlagBits const &enumerator) {
    VkBufferUsageFlagBits allFlags = (VkBufferUsageFlagBits)(
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
        VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkBufferUsageFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {
        strings.push_back("VK_BUFFER_USAGE_VERTEX_BUFFER_BIT");
    }
    if (enumerator & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {
        strings.push_back("VK_BUFFER_USAGE_INDEX_BUFFER_BIT");
    }
    if (enumerator & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT) {
        strings.push_back("VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT");
    }
    if (enumerator & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT) {
        strings.push_back("VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT");
    }
    if (enumerator & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
        strings.push_back("VK_BUFFER_USAGE_STORAGE_BUFFER_BIT");
    }
    if (enumerator & VK_BUFFER_USAGE_TRANSFER_DST_BIT) {
        strings.push_back("VK_BUFFER_USAGE_TRANSFER_DST_BIT");
    }
    if (enumerator & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT) {
        strings.push_back("VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT");
    }
    if (enumerator & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) {
        strings.push_back("VK_BUFFER_USAGE_TRANSFER_SRC_BIT");
    }
    if (enumerator & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
        strings.push_back("VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkBufferCreateFlagBits const &enumerator) {
    VkBufferCreateFlagBits allFlags = (VkBufferCreateFlagBits)(
        VK_BUFFER_CREATE_SPARSE_ALIASED_BIT | VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT | VK_BUFFER_CREATE_SPARSE_BINDING_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkBufferCreateFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_BUFFER_CREATE_SPARSE_ALIASED_BIT) {
        strings.push_back("VK_BUFFER_CREATE_SPARSE_ALIASED_BIT");
    }
    if (enumerator & VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT) {
        strings.push_back("VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT");
    }
    if (enumerator & VK_BUFFER_CREATE_SPARSE_BINDING_BIT) {
        strings.push_back("VK_BUFFER_CREATE_SPARSE_BINDING_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkImageCreateFlagBits const &enumerator) {
    VkImageCreateFlagBits allFlags = (VkImageCreateFlagBits)(
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT | VK_IMAGE_CREATE_SPARSE_ALIASED_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT |
        VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_SPARSE_BINDING_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkImageCreateFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) {
        strings.push_back("VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT");
    }
    if (enumerator & VK_IMAGE_CREATE_SPARSE_ALIASED_BIT) {
        strings.push_back("VK_IMAGE_CREATE_SPARSE_ALIASED_BIT");
    }
    if (enumerator & VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT) {
        strings.push_back("VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT");
    }
    if (enumerator & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT) {
        strings.push_back("VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT");
    }
    if (enumerator & VK_IMAGE_CREATE_SPARSE_BINDING_BIT) {
        strings.push_back("VK_IMAGE_CREATE_SPARSE_BINDING_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkColorComponentFlagBits const &enumerator) {
    VkColorComponentFlagBits allFlags = (VkColorComponentFlagBits)(VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_B_BIT |
                                                                   VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_R_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkColorComponentFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_COLOR_COMPONENT_A_BIT) {
        strings.push_back("VK_COLOR_COMPONENT_A_BIT");
    }
    if (enumerator & VK_COLOR_COMPONENT_B_BIT) {
        strings.push_back("VK_COLOR_COMPONENT_B_BIT");
    }
    if (enumerator & VK_COLOR_COMPONENT_G_BIT) {
        strings.push_back("VK_COLOR_COMPONENT_G_BIT");
    }
    if (enumerator & VK_COLOR_COMPONENT_R_BIT) {
        strings.push_back("VK_COLOR_COMPONENT_R_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkPipelineCreateFlagBits const &enumerator) {
    VkPipelineCreateFlagBits allFlags = (VkPipelineCreateFlagBits)(
        VK_PIPELINE_CREATE_DERIVATIVE_BIT | VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT | VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkPipelineCreateFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_PIPELINE_CREATE_DERIVATIVE_BIT) {
        strings.push_back("VK_PIPELINE_CREATE_DERIVATIVE_BIT");
    }
    if (enumerator & VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT) {
        strings.push_back("VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT");
    }
    if (enumerator & VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT) {
        strings.push_back("VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkShaderStageFlagBits const &enumerator) {
    VkShaderStageFlagBits allFlags = (VkShaderStageFlagBits)(
        VK_SHADER_STAGE_ALL | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_COMPUTE_BIT |
        VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_VERTEX_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkShaderStageFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_SHADER_STAGE_ALL) {
        strings.push_back("VK_SHADER_STAGE_ALL");
    }
    if (enumerator & VK_SHADER_STAGE_FRAGMENT_BIT) {
        strings.push_back("VK_SHADER_STAGE_FRAGMENT_BIT");
    }
    if (enumerator & VK_SHADER_STAGE_GEOMETRY_BIT) {
        strings.push_back("VK_SHADER_STAGE_GEOMETRY_BIT");
    }
    if (enumerator & VK_SHADER_STAGE_COMPUTE_BIT) {
        strings.push_back("VK_SHADER_STAGE_COMPUTE_BIT");
    }
    if (enumerator & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) {
        strings.push_back("VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT");
    }
    if (enumerator & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) {
        strings.push_back("VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT");
    }
    if (enumerator & VK_SHADER_STAGE_VERTEX_BIT) {
        strings.push_back("VK_SHADER_STAGE_VERTEX_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkPipelineStageFlagBits const &enumerator) {
    VkPipelineStageFlagBits allFlags = (VkPipelineStageFlagBits)(
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_HOST_BIT |
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
        VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkPipelineStageFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
        strings.push_back("VK_PIPELINE_STAGE_ALL_COMMANDS_BIT");
    }
    if (enumerator & VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT) {
        strings.push_back("VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT");
    }
    if (enumerator & VK_PIPELINE_STAGE_HOST_BIT) {
        strings.push_back("VK_PIPELINE_STAGE_HOST_BIT");
    }
    if (enumerator & VK_PIPELINE_STAGE_TRANSFER_BIT) {
        strings.push_back("VK_PIPELINE_STAGE_TRANSFER_BIT");
    }
    if (enumerator & VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT) {
        strings.push_back("VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT");
    }
    if (enumerator & VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT) {
        strings.push_back("VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT");
    }
    if (enumerator & VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {
        strings.push_back("VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT");
    }
    if (enumerator & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT) {
        strings.push_back("VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT");
    }
    if (enumerator & VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT) {
        strings.push_back("VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT");
    }
    if (enumerator & VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT) {
        strings.push_back("VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT");
    }
    if (enumerator & VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT) {
        strings.push_back("VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT");
    }
    if (enumerator & VK_PIPELINE_STAGE_VERTEX_SHADER_BIT) {
        strings.push_back("VK_PIPELINE_STAGE_VERTEX_SHADER_BIT");
    }
    if (enumerator & VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT) {
        strings.push_back("VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT");
    }
    if (enumerator & VK_PIPELINE_STAGE_VERTEX_INPUT_BIT) {
        strings.push_back("VK_PIPELINE_STAGE_VERTEX_INPUT_BIT");
    }
    if (enumerator & VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT) {
        strings.push_back("VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT");
    }
    if (enumerator & VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT) {
        strings.push_back("VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT");
    }
    if (enumerator & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT) {
        strings.push_back("VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkAccessFlagBits const &enumerator) {
    VkAccessFlagBits allFlags = (VkAccessFlagBits)(
        VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
        VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT |
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT |
        VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT);

    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkAccessFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_ACCESS_INDIRECT_COMMAND_READ_BIT) {
        strings.push_back("VK_ACCESS_INDIRECT_COMMAND_READ_BIT");
    }
    if (enumerator & VK_ACCESS_INDEX_READ_BIT) {
        strings.push_back("VK_ACCESS_INDEX_READ_BIT");
    }
    if (enumerator & VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT) {
        strings.push_back("VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT");
    }
    if (enumerator & VK_ACCESS_UNIFORM_READ_BIT) {
        strings.push_back("VK_ACCESS_UNIFORM_READ_BIT");
    }
    if (enumerator & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) {
        strings.push_back("VK_ACCESS_INPUT_ATTACHMENT_READ_BIT");
    }
    if (enumerator & VK_ACCESS_SHADER_READ_BIT) {
        strings.push_back("VK_ACCESS_SHADER_READ_BIT");
    }
    if (enumerator & VK_ACCESS_SHADER_WRITE_BIT) {
        strings.push_back("VK_ACCESS_SHADER_WRITE_BIT");
    }
    if (enumerator & VK_ACCESS_COLOR_ATTACHMENT_READ_BIT) {
        strings.push_back("VK_ACCESS_COLOR_ATTACHMENT_READ_BIT");
    }
    if (enumerator & VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT) {
        strings.push_back("VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT");
    }
    if (enumerator & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT) {
        strings.push_back("VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT");
    }
    if (enumerator & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT) {
        strings.push_back("VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT");
    }
    if (enumerator & VK_ACCESS_TRANSFER_READ_BIT) {
        strings.push_back("VK_ACCESS_TRANSFER_READ_BIT");
    }
    if (enumerator & VK_ACCESS_TRANSFER_WRITE_BIT) {
        strings.push_back("VK_ACCESS_TRANSFER_WRITE_BIT");
    }
    if (enumerator & VK_ACCESS_HOST_READ_BIT) {
        strings.push_back("VK_ACCESS_HOST_READ_BIT");
    }
    if (enumerator & VK_ACCESS_HOST_WRITE_BIT) {
        strings.push_back("VK_ACCESS_HOST_WRITE_BIT");
    }
    if (enumerator & VK_ACCESS_MEMORY_READ_BIT) {
        strings.push_back("VK_ACCESS_MEMORY_READ_BIT");
    }
    if (enumerator & VK_ACCESS_MEMORY_WRITE_BIT) {
        strings.push_back("VK_ACCESS_MEMORY_WRITE_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkCommandPoolCreateFlagBits const &enumerator) {
    VkCommandPoolCreateFlagBits allFlags =
        (VkCommandPoolCreateFlagBits)(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkCommandPoolCreateFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) {
        strings.push_back("VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT");
    }
    if (enumerator & VK_COMMAND_POOL_CREATE_TRANSIENT_BIT) {
        strings.push_back("VK_COMMAND_POOL_CREATE_TRANSIENT_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkCommandPoolResetFlagBits const &enumerator) {
    VkCommandPoolResetFlagBits allFlags = (VkCommandPoolResetFlagBits)(VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkCommandPoolResetFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT) {
        strings.push_back("VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkCommandBufferUsageFlags const &enumerator) {
    VkCommandBufferUsageFlags allFlags =
        (VkCommandBufferUsageFlags)(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT |
                                    VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkCommandBufferUsageFlags const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT) {
        strings.push_back("VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT");
    }
    if (enumerator & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) {
        strings.push_back("VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT");
    }
    if (enumerator & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT) {
        strings.push_back("VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkCommandBufferResetFlagBits const &enumerator) {
    VkCommandBufferResetFlagBits allFlags = (VkCommandBufferResetFlagBits)(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkCommandBufferResetFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT) {
        strings.push_back("VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkImageAspectFlagBits const &enumerator) {
    VkImageAspectFlagBits allFlags = (VkImageAspectFlagBits)(VK_IMAGE_ASPECT_METADATA_BIT | VK_IMAGE_ASPECT_STENCIL_BIT |
                                                             VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_COLOR_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkImageAspectFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_IMAGE_ASPECT_METADATA_BIT) {
        strings.push_back("VK_IMAGE_ASPECT_METADATA_BIT");
    }
    if (enumerator & VK_IMAGE_ASPECT_STENCIL_BIT) {
        strings.push_back("VK_IMAGE_ASPECT_STENCIL_BIT");
    }
    if (enumerator & VK_IMAGE_ASPECT_DEPTH_BIT) {
        strings.push_back("VK_IMAGE_ASPECT_DEPTH_BIT");
    }
    if (enumerator & VK_IMAGE_ASPECT_COLOR_BIT) {
        strings.push_back("VK_IMAGE_ASPECT_COLOR_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static bool ValidateEnumerator(VkQueryControlFlagBits const &enumerator) {
    VkQueryControlFlagBits allFlags = (VkQueryControlFlagBits)(VK_QUERY_CONTROL_PRECISE_BIT);
    if (enumerator & (~allFlags)) {
        return false;
    }

    return true;
}

static std::string EnumeratorString(VkQueryControlFlagBits const &enumerator) {
    if (!ValidateEnumerator(enumerator)) {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if (enumerator & VK_QUERY_CONTROL_PRECISE_BIT) {
        strings.push_back("VK_QUERY_CONTROL_PRECISE_BIT");
    }

    std::string enumeratorString;
    for (auto const &string : strings) {
        enumeratorString += string;

        if (string != strings.back()) {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static const int MaxParamCheckerStringLength = 256;

static bool validate_string(debug_report_data *report_data, const char *apiName, const ParameterName &stringName,
                            const char *validateString) {
    assert(apiName != nullptr);
    assert(validateString != nullptr);

    bool skip_call = false;

    VkStringErrorFlags result = vk_string_validate(MaxParamCheckerStringLength, validateString);

    if (result == VK_STRING_ERROR_NONE) {
        return skip_call;
    } else if (result & VK_STRING_ERROR_LENGTH) {

        skip_call = log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            INVALID_USAGE, LayerName, "%s: string %s exceeds max length %d", apiName, stringName.get_name().c_str(),
                            MaxParamCheckerStringLength);
    } else if (result & VK_STRING_ERROR_BAD_DATA) {
        skip_call = log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            INVALID_USAGE, LayerName, "%s: string %s contains invalid characters or is badly formed", apiName,
                            stringName.get_name().c_str());
    }
    return skip_call;
}

static bool validate_queue_family_index(layer_data *device_data, const char *function_name, const char *parameter_name,
                                        uint32_t index) {
    assert(device_data != nullptr);
    debug_report_data *report_data = device_data->report_data;
    bool skip_call = false;

    if (index == VK_QUEUE_FAMILY_IGNORED) {
        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, LayerName,
                             "%s: %s cannot be VK_QUEUE_FAMILY_IGNORED.", function_name, parameter_name);
    } else {
        const auto &queue_data = device_data->queueFamilyIndexMap.find(index);
        if (queue_data == device_data->queueFamilyIndexMap.end()) {
            skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1,
                                 LayerName, "%s: %s (%d) must be one of the indices specified when the device was created, via "
                                            "the VkDeviceQueueCreateInfo structure.",
                                 function_name, parameter_name, index);
            return false;
        }
    }

    return skip_call;
}

static bool validate_queue_family_indices(layer_data *device_data, const char *function_name, const char *parameter_name,
                                          const uint32_t count, const uint32_t *indices) {
    assert(device_data != nullptr);
    debug_report_data *report_data = device_data->report_data;
    bool skip_call = false;

    if (indices != nullptr) {
        for (uint32_t i = 0; i < count; i++) {
            if (indices[i] == VK_QUEUE_FAMILY_IGNORED) {
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1,
                                     LayerName, "%s: %s[%d] cannot be VK_QUEUE_FAMILY_IGNORED.", function_name, parameter_name, i);
            } else {
                const auto &queue_data = device_data->queueFamilyIndexMap.find(indices[i]);
                if (queue_data == device_data->queueFamilyIndexMap.end()) {
                    skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1,
                                         LayerName, "%s: %s[%d] (%d) must be one of the indices specified when the device was "
                                                    "created, via the VkDeviceQueueCreateInfo structure.",
                                         function_name, parameter_name, i, indices[i]);
                    return false;
                }
            }
        }
    }

    return skip_call;
}

static void CheckInstanceRegisterExtensions(const VkInstanceCreateInfo *pCreateInfo, VkInstance instance);

VKAPI_ATTR VkResult VKAPI_CALL CreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                              VkInstance *pInstance) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;

    VkLayerInstanceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);
    assert(chain_info != nullptr);
    assert(chain_info->u.pLayerInfo != nullptr);

    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkCreateInstance fpCreateInstance = (PFN_vkCreateInstance)fpGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (fpCreateInstance == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);

    if (result == VK_SUCCESS) {
        layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
        assert(my_instance_data != nullptr);

        VkLayerInstanceDispatchTable *pTable = initInstanceTable(*pInstance, fpGetInstanceProcAddr, pc_instance_table_map);

        my_instance_data->instance = *pInstance;
        my_instance_data->report_data = debug_report_create_instance(pTable, *pInstance, pCreateInfo->enabledExtensionCount,
                                                                     pCreateInfo->ppEnabledExtensionNames);

        // Look for one or more debug report create info structures
        // and setup a callback(s) for each one found.
        if (!layer_copy_tmp_callbacks(pCreateInfo->pNext, &my_instance_data->num_tmp_callbacks,
                                      &my_instance_data->tmp_dbg_create_infos, &my_instance_data->tmp_callbacks)) {
            if (my_instance_data->num_tmp_callbacks > 0) {
                // Setup the temporary callback(s) here to catch early issues:
                if (layer_enable_tmp_callbacks(my_instance_data->report_data, my_instance_data->num_tmp_callbacks,
                                               my_instance_data->tmp_dbg_create_infos, my_instance_data->tmp_callbacks)) {
                    // Failure of setting up one or more of the callback.
                    // Therefore, clean up and don't use those callbacks:
                    layer_free_tmp_callbacks(my_instance_data->tmp_dbg_create_infos, my_instance_data->tmp_callbacks);
                    my_instance_data->num_tmp_callbacks = 0;
                }
            }
        }

        init_parameter_validation(my_instance_data, pAllocator);
        CheckInstanceRegisterExtensions(pCreateInfo, *pInstance);

        // Ordinarily we'd check these before calling down the chain, but none of the layer
        // support is in place until now, if we survive we can report the issue now.
        parameter_validation_vkCreateInstance(my_instance_data->report_data, pCreateInfo, pAllocator, pInstance);

        if (pCreateInfo->pApplicationInfo) {
            if (pCreateInfo->pApplicationInfo->pApplicationName) {
                validate_string(my_instance_data->report_data, "vkCreateInstance",
                                "pCreateInfo->VkApplicationInfo->pApplicationName",
                                pCreateInfo->pApplicationInfo->pApplicationName);
            }

            if (pCreateInfo->pApplicationInfo->pEngineName) {
                validate_string(my_instance_data->report_data, "vkCreateInstance", "pCreateInfo->VkApplicationInfo->pEngineName",
                                pCreateInfo->pApplicationInfo->pEngineName);
            }
        }

        // Disable the tmp callbacks:
        if (my_instance_data->num_tmp_callbacks > 0) {
            layer_disable_tmp_callbacks(my_instance_data->report_data, my_instance_data->num_tmp_callbacks,
                                        my_instance_data->tmp_callbacks);
        }
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    // Grab the key before the instance is destroyed.
    dispatch_key key = get_dispatch_key(instance);
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    assert(my_data != NULL);

    // Enable the temporary callback(s) here to catch vkDestroyInstance issues:
    bool callback_setup = false;
    if (my_data->num_tmp_callbacks > 0) {
        if (!layer_enable_tmp_callbacks(my_data->report_data, my_data->num_tmp_callbacks, my_data->tmp_dbg_create_infos,
                                        my_data->tmp_callbacks)) {
            callback_setup = true;
        }
    }

    skip_call |= parameter_validation_vkDestroyInstance(my_data->report_data, pAllocator);

    // Disable and cleanup the temporary callback(s):
    if (callback_setup) {
        layer_disable_tmp_callbacks(my_data->report_data, my_data->num_tmp_callbacks, my_data->tmp_callbacks);
    }
    if (my_data->num_tmp_callbacks > 0) {
        layer_free_tmp_callbacks(my_data->tmp_dbg_create_infos, my_data->tmp_callbacks);
        my_data->num_tmp_callbacks = 0;
    }

    if (!skip_call) {
        VkLayerInstanceDispatchTable *pTable = get_dispatch_table(pc_instance_table_map, instance);
        pTable->DestroyInstance(instance, pAllocator);

        // Clean up logging callback, if any
        while (my_data->logging_callback.size() > 0) {
            VkDebugReportCallbackEXT callback = my_data->logging_callback.back();
            layer_destroy_msg_callback(my_data->report_data, callback, pAllocator);
            my_data->logging_callback.pop_back();
        }

        layer_debug_report_destroy_instance(mid(instance));
        layer_data_map.erase(pTable);

        pc_instance_table_map.erase(key);
        layer_data_map.erase(key);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL EnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount,
                                                        VkPhysicalDevice *pPhysicalDevices) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkEnumeratePhysicalDevices(my_data->report_data, pPhysicalDeviceCount, pPhysicalDevices);

    if (!skip_call) {
        result = get_dispatch_table(pc_instance_table_map, instance)
                     ->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);

        validate_result(my_data->report_data, "vkEnumeratePhysicalDevices", result);
        if ((result == VK_SUCCESS) && (NULL != pPhysicalDevices)) {
            for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++) {
                layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(pPhysicalDevices[i]), layer_data_map);
                // Save the supported features for each physical device
                VkLayerInstanceDispatchTable *disp_table = get_dispatch_table(pc_instance_table_map, pPhysicalDevices[i]);
                disp_table->GetPhysicalDeviceFeatures(pPhysicalDevices[i], &(phy_dev_data->physical_device_features));
            }
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL GetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures *pFeatures) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetPhysicalDeviceFeatures(my_data->report_data, pFeatures);

    if (!skip_call) {
        get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceFeatures(physicalDevice, pFeatures);
    }
}

VKAPI_ATTR void VKAPI_CALL GetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format,
                                                             VkFormatProperties *pFormatProperties) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetPhysicalDeviceFormatProperties(my_data->report_data, format, pFormatProperties);

    if (!skip_call) {
        get_dispatch_table(pc_instance_table_map, physicalDevice)
            ->GetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format,
                                                                      VkImageType type, VkImageTiling tiling,
                                                                      VkImageUsageFlags usage, VkImageCreateFlags flags,
                                                                      VkImageFormatProperties *pImageFormatProperties) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetPhysicalDeviceImageFormatProperties(my_data->report_data, format, type, tiling, usage,
                                                                               flags, pImageFormatProperties);

    if (!skip_call) {
        result = get_dispatch_table(pc_instance_table_map, physicalDevice)
                     ->GetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags,
                                                              pImageFormatProperties);

        validate_result(my_data->report_data, "vkGetPhysicalDeviceImageFormatProperties", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL GetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetPhysicalDeviceProperties(my_data->report_data, pProperties);

    if (!skip_call) {
        get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceProperties(physicalDevice, pProperties);
    }
}

VKAPI_ATTR void VKAPI_CALL GetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice,
                                                                  uint32_t *pQueueFamilyPropertyCount,
                                                                  VkQueueFamilyProperties *pQueueFamilyProperties) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetPhysicalDeviceQueueFamilyProperties(my_data->report_data, pQueueFamilyPropertyCount,
                                                                               pQueueFamilyProperties);

    if (!skip_call) {
        get_dispatch_table(pc_instance_table_map, physicalDevice)
            ->GetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
    }
}

VKAPI_ATTR void VKAPI_CALL GetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice,
                                                             VkPhysicalDeviceMemoryProperties *pMemoryProperties) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetPhysicalDeviceMemoryProperties(my_data->report_data, pMemoryProperties);

    if (!skip_call) {
        get_dispatch_table(pc_instance_table_map, physicalDevice)
            ->GetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
    }
}

void validateDeviceCreateInfo(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo,
                              const std::vector<VkQueueFamilyProperties> properties) {
    std::unordered_set<uint32_t> set;

    if ((pCreateInfo != nullptr) && (pCreateInfo->pQueueCreateInfos != nullptr)) {
        for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; ++i) {
            if (set.count(pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex)) {
                log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        INVALID_USAGE, LayerName,
                        "VkDeviceCreateInfo parameter, uint32_t pQueueCreateInfos[%d]->queueFamilyIndex, is not unique within this "
                        "structure.",
                        i);
            } else {
                set.insert(pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex);
            }

            if (pCreateInfo->pQueueCreateInfos[i].pQueuePriorities != nullptr) {
                for (uint32_t j = 0; j < pCreateInfo->pQueueCreateInfos[i].queueCount; ++j) {
                    if ((pCreateInfo->pQueueCreateInfos[i].pQueuePriorities[j] < 0.f) ||
                        (pCreateInfo->pQueueCreateInfos[i].pQueuePriorities[j] > 1.f)) {
                        log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                __LINE__, INVALID_USAGE, LayerName,
                                "VkDeviceCreateInfo parameter, uint32_t pQueueCreateInfos[%d]->pQueuePriorities[%d], must be "
                                "between 0 and 1. Actual value is %f",
                                i, j, pCreateInfo->pQueueCreateInfos[i].pQueuePriorities[j]);
                    }
                }
            }

            if (pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex >= properties.size()) {
                log_msg(
                    mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    INVALID_USAGE, LayerName,
                    "VkDeviceCreateInfo parameter, uint32_t pQueueCreateInfos[%d]->queueFamilyIndex cannot be more than the number "
                    "of queue families.",
                    i);
            } else if (pCreateInfo->pQueueCreateInfos[i].queueCount >
                       properties[pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex].queueCount) {
                log_msg(
                    mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    INVALID_USAGE, LayerName,
                    "VkDeviceCreateInfo parameter, uint32_t pQueueCreateInfos[%d]->queueCount cannot be more than the number of "
                    "queues for the given family index.",
                    i);
            }
        }
    }
}

static void CheckInstanceRegisterExtensions(const VkInstanceCreateInfo *pCreateInfo, VkInstance instance) {
    VkLayerInstanceDispatchTable *dispatch_table = get_dispatch_table(pc_instance_table_map, instance);

    instance_extension_map[dispatch_table] = {};

    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0) {
            instance_extension_map[dispatch_table].wsi_enabled = true;
        }
#ifdef VK_USE_PLATFORM_XLIB_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XLIB_SURFACE_EXTENSION_NAME) == 0) {
            instance_extension_map[dispatch_table].xlib_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XCB_SURFACE_EXTENSION_NAME) == 0) {
            instance_extension_map[dispatch_table].xcb_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME) == 0) {
            instance_extension_map[dispatch_table].wayland_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_MIR_SURFACE_EXTENSION_NAME) == 0) {
            instance_extension_map[dispatch_table].mir_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_ANDROID_SURFACE_EXTENSION_NAME) == 0) {
            instance_extension_map[dispatch_table].android_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0) {
            instance_extension_map[dispatch_table].win32_enabled = true;
        }
#endif
    }
}

static void CheckDeviceRegisterExtensions(const VkDeviceCreateInfo *pCreateInfo, VkDevice device) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    device_data->wsi_enabled = false;
    device_data->wsi_display_swapchain_enabled = false;

    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            device_data->wsi_enabled = true;
        }
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) == 0) {
            device_data->wsi_display_swapchain_enabled = true;
        }
    }
}

void storeCreateDeviceData(VkDevice device, const VkDeviceCreateInfo *pCreateInfo) {
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    if ((pCreateInfo != nullptr) && (pCreateInfo->pQueueCreateInfos != nullptr)) {
        for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; ++i) {
            my_device_data->queueFamilyIndexMap.insert(
                std::make_pair(pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex, pCreateInfo->pQueueCreateInfos[i].queueCount));
        }
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo,
                                            const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    /*
     * NOTE: We do not validate physicalDevice or any dispatchable
     * object as the first parameter. We couldn't get here if it was wrong!
     */

    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_instance_data != nullptr);

    skip_call |= parameter_validation_vkCreateDevice(my_instance_data->report_data, pCreateInfo, pAllocator, pDevice);

    if (pCreateInfo != NULL) {
        if ((pCreateInfo->enabledLayerCount > 0) && (pCreateInfo->ppEnabledLayerNames != NULL)) {
            for (size_t i = 0; i < pCreateInfo->enabledLayerCount; i++) {
                skip_call |= validate_string(my_instance_data->report_data, "vkCreateDevice", "pCreateInfo->ppEnabledLayerNames",
                                             pCreateInfo->ppEnabledLayerNames[i]);
            }
        }

        if ((pCreateInfo->enabledExtensionCount > 0) && (pCreateInfo->ppEnabledExtensionNames != NULL)) {
            for (size_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
                skip_call |= validate_string(my_instance_data->report_data, "vkCreateDevice",
                                             "pCreateInfo->ppEnabledExtensionNames", pCreateInfo->ppEnabledExtensionNames[i]);
            }
        }
    }

    if (!skip_call) {
        VkLayerDeviceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);
        assert(chain_info != nullptr);
        assert(chain_info->u.pLayerInfo != nullptr);

        PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
        PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
        PFN_vkCreateDevice fpCreateDevice = (PFN_vkCreateDevice)fpGetInstanceProcAddr(my_instance_data->instance, "vkCreateDevice");
        if (fpCreateDevice == NULL) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        // Advance the link info for the next element on the chain
        chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

        result = fpCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);

        validate_result(my_instance_data->report_data, "vkCreateDevice", result);

        if (result == VK_SUCCESS) {
            layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
            assert(my_device_data != nullptr);

            my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
            initDeviceTable(*pDevice, fpGetDeviceProcAddr, pc_device_table_map);

            CheckDeviceRegisterExtensions(pCreateInfo, *pDevice);

            uint32_t count;
            VkLayerInstanceDispatchTable *instance_dispatch_table = get_dispatch_table(pc_instance_table_map, physicalDevice);
            instance_dispatch_table->GetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
            std::vector<VkQueueFamilyProperties> properties(count);
            instance_dispatch_table->GetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, &properties[0]);

            validateDeviceCreateInfo(physicalDevice, pCreateInfo, properties);
            storeCreateDeviceData(*pDevice, pCreateInfo);

            // Query and save physical device limits for this device
            VkPhysicalDeviceProperties device_properties = {};
            instance_dispatch_table->GetPhysicalDeviceProperties(physicalDevice, &device_properties);
            memcpy(&my_device_data->device_limits, &device_properties.limits, sizeof(VkPhysicalDeviceLimits));
            my_device_data->physical_device = physicalDevice;

            // Save app-enabled features in this device's layer_data structure
            if (pCreateInfo->pEnabledFeatures) {
                my_device_data->physical_device_features = *pCreateInfo->pEnabledFeatures;
            } else {
                memset(&my_device_data->physical_device_features, 0, sizeof(VkPhysicalDeviceFeatures));
            }
        }
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    dispatch_key key = get_dispatch_key(device);
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroyDevice(my_data->report_data, pAllocator);

    if (!skip_call) {
        layer_debug_report_destroy_device(device);

#if DISPATCH_MAP_DEBUG
        fprintf(stderr, "Device:  0x%p, key:  0x%p\n", device, key);
#endif

        get_dispatch_table(pc_device_table_map, device)->DestroyDevice(device, pAllocator);
        pc_device_table_map.erase(key);
        layer_data_map.erase(key);
    }
}

bool PreGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex) {
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_device_data != nullptr);

    validate_queue_family_index(my_device_data, "vkGetDeviceQueue", "queueFamilyIndex", queueFamilyIndex);

    const auto &queue_data = my_device_data->queueFamilyIndexMap.find(queueFamilyIndex);
    if (queue_data->second <= queueIndex) {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, INVALID_USAGE,
                LayerName,
                "VkGetDeviceQueue parameter, uint32_t queueIndex %d, must be less than the number of queues given when the device "
                "was created.",
                queueIndex);
        return false;
    }
    return true;
}

VKAPI_ATTR void VKAPI_CALL GetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue *pQueue) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetDeviceQueue(my_data->report_data, queueFamilyIndex, queueIndex, pQueue);

    if (!skip_call) {
        PreGetDeviceQueue(device, queueFamilyIndex, queueIndex);

        get_dispatch_table(pc_device_table_map, device)->GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL QueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkQueueSubmit(my_data->report_data, submitCount, pSubmits, fence);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, queue)->QueueSubmit(queue, submitCount, pSubmits, fence);

        validate_result(my_data->report_data, "vkQueueSubmit", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL QueueWaitIdle(VkQueue queue) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    assert(my_data != NULL);

    VkResult result = get_dispatch_table(pc_device_table_map, queue)->QueueWaitIdle(queue);

    validate_result(my_data->report_data, "vkQueueWaitIdle", result);

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL DeviceWaitIdle(VkDevice device) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->DeviceWaitIdle(device);

    validate_result(my_data->report_data, "vkDeviceWaitIdle", result);

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL AllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo,
                                              const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkAllocateMemory(my_data->report_data, pAllocateInfo, pAllocator, pMemory);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->AllocateMemory(device, pAllocateInfo, pAllocator, pMemory);

        validate_result(my_data->report_data, "vkAllocateMemory", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL FreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkFreeMemory(my_data->report_data, memory, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->FreeMemory(device, memory, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL MapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size,
                                         VkMemoryMapFlags flags, void **ppData) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkMapMemory(my_data->report_data, memory, offset, size, flags, ppData);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->MapMemory(device, memory, offset, size, flags, ppData);

        validate_result(my_data->report_data, "vkMapMemory", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL UnmapMemory(VkDevice device, VkDeviceMemory memory) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkUnmapMemory(my_data->report_data, memory);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->UnmapMemory(device, memory);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL FlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount,
                                                       const VkMappedMemoryRange *pMemoryRanges) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkFlushMappedMemoryRanges(my_data->report_data, memoryRangeCount, pMemoryRanges);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->FlushMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);

        validate_result(my_data->report_data, "vkFlushMappedMemoryRanges", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL InvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount,
                                                            const VkMappedMemoryRange *pMemoryRanges) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkInvalidateMappedMemoryRanges(my_data->report_data, memoryRangeCount, pMemoryRanges);

    if (!skip_call) {
        result =
            get_dispatch_table(pc_device_table_map, device)->InvalidateMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);

        validate_result(my_data->report_data, "vkInvalidateMappedMemoryRanges", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL GetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory,
                                                     VkDeviceSize *pCommittedMemoryInBytes) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetDeviceMemoryCommitment(my_data->report_data, memory, pCommittedMemoryInBytes);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->GetDeviceMemoryCommitment(device, memory, pCommittedMemoryInBytes);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL BindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory,
                                                VkDeviceSize memoryOffset) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkBindBufferMemory(my_data->report_data, buffer, memory, memoryOffset);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->BindBufferMemory(device, buffer, memory, memoryOffset);

        validate_result(my_data->report_data, "vkBindBufferMemory", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL BindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkBindImageMemory(my_data->report_data, image, memory, memoryOffset);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->BindImageMemory(device, image, memory, memoryOffset);

        validate_result(my_data->report_data, "vkBindImageMemory", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL GetBufferMemoryRequirements(VkDevice device, VkBuffer buffer,
                                                       VkMemoryRequirements *pMemoryRequirements) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetBufferMemoryRequirements(my_data->report_data, buffer, pMemoryRequirements);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->GetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
    }
}

VKAPI_ATTR void VKAPI_CALL GetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements *pMemoryRequirements) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetImageMemoryRequirements(my_data->report_data, image, pMemoryRequirements);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->GetImageMemoryRequirements(device, image, pMemoryRequirements);
    }
}

bool PostGetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t *pNumRequirements,
                                          VkSparseImageMemoryRequirements *pSparseMemoryRequirements) {
    if (pSparseMemoryRequirements != nullptr) {
        if ((pSparseMemoryRequirements->formatProperties.aspectMask &
             (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT |
              VK_IMAGE_ASPECT_METADATA_BIT)) == 0) {
            log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    UNRECOGNIZED_VALUE, LayerName,
                    "vkGetImageSparseMemoryRequirements parameter, VkImageAspect "
                    "pSparseMemoryRequirements->formatProperties.aspectMask, is an unrecognized enumerator");
            return false;
        }
    }

    return true;
}

VKAPI_ATTR void VKAPI_CALL GetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t *pSparseMemoryRequirementCount,
                                                            VkSparseImageMemoryRequirements *pSparseMemoryRequirements) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetImageSparseMemoryRequirements(my_data->report_data, image, pSparseMemoryRequirementCount,
                                                                         pSparseMemoryRequirements);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)
            ->GetImageSparseMemoryRequirements(device, image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);

        PostGetImageSparseMemoryRequirements(device, image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
    }
}

bool PostGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
                                                      VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling,
                                                      uint32_t *pNumProperties, VkSparseImageFormatProperties *pProperties) {
    if (pProperties != nullptr) {
        if ((pProperties->aspectMask & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT |
                                        VK_IMAGE_ASPECT_METADATA_BIT)) == 0) {
            log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, 1,
                    LayerName,
                    "vkGetPhysicalDeviceSparseImageFormatProperties parameter, VkImageAspect pProperties->aspectMask, is an "
                    "unrecognized enumerator");
            return false;
        }
    }

    return true;
}

VKAPI_ATTR void VKAPI_CALL GetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format,
                                                                        VkImageType type, VkSampleCountFlagBits samples,
                                                                        VkImageUsageFlags usage, VkImageTiling tiling,
                                                                        uint32_t *pPropertyCount,
                                                                        VkSparseImageFormatProperties *pProperties) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetPhysicalDeviceSparseImageFormatProperties(my_data->report_data, format, type, samples,
                                                                                     usage, tiling, pPropertyCount, pProperties);

    if (!skip_call) {
        get_dispatch_table(pc_instance_table_map, physicalDevice)
            ->GetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pPropertyCount,
                                                           pProperties);

        PostGetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pPropertyCount,
                                                         pProperties);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL QueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo *pBindInfo,
                                               VkFence fence) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkQueueBindSparse(my_data->report_data, bindInfoCount, pBindInfo, fence);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, queue)->QueueBindSparse(queue, bindInfoCount, pBindInfo, fence);

        validate_result(my_data->report_data, "vkQueueBindSparse", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo,
                                           const VkAllocationCallbacks *pAllocator, VkFence *pFence) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCreateFence(my_data->report_data, pCreateInfo, pAllocator, pFence);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->CreateFence(device, pCreateInfo, pAllocator, pFence);

        validate_result(my_data->report_data, "vkCreateFence", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroyFence(my_data->report_data, fence, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->DestroyFence(device, fence, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL ResetFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkResetFences(my_data->report_data, fenceCount, pFences);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->ResetFences(device, fenceCount, pFences);

        validate_result(my_data->report_data, "vkResetFences", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL GetFenceStatus(VkDevice device, VkFence fence) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetFenceStatus(my_data->report_data, fence);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->GetFenceStatus(device, fence);

        validate_result(my_data->report_data, "vkGetFenceStatus", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL WaitForFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences, VkBool32 waitAll,
                                             uint64_t timeout) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkWaitForFences(my_data->report_data, fenceCount, pFences, waitAll, timeout);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->WaitForFences(device, fenceCount, pFences, waitAll, timeout);

        validate_result(my_data->report_data, "vkWaitForFences", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo *pCreateInfo,
                                               const VkAllocationCallbacks *pAllocator, VkSemaphore *pSemaphore) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCreateSemaphore(my_data->report_data, pCreateInfo, pAllocator, pSemaphore);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->CreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);

        validate_result(my_data->report_data, "vkCreateSemaphore", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroySemaphore(my_data->report_data, semaphore, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->DestroySemaphore(device, semaphore, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateEvent(VkDevice device, const VkEventCreateInfo *pCreateInfo,
                                           const VkAllocationCallbacks *pAllocator, VkEvent *pEvent) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCreateEvent(my_data->report_data, pCreateInfo, pAllocator, pEvent);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->CreateEvent(device, pCreateInfo, pAllocator, pEvent);

        validate_result(my_data->report_data, "vkCreateEvent", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroyEvent(my_data->report_data, event, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->DestroyEvent(device, event, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL GetEventStatus(VkDevice device, VkEvent event) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetEventStatus(my_data->report_data, event);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->GetEventStatus(device, event);

        validate_result(my_data->report_data, "vkGetEventStatus", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL SetEvent(VkDevice device, VkEvent event) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkSetEvent(my_data->report_data, event);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->SetEvent(device, event);

        validate_result(my_data->report_data, "vkSetEvent", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL ResetEvent(VkDevice device, VkEvent event) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkResetEvent(my_data->report_data, event);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->ResetEvent(device, event);

        validate_result(my_data->report_data, "vkResetEvent", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo *pCreateInfo,
                                               const VkAllocationCallbacks *pAllocator, VkQueryPool *pQueryPool) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(device_data != nullptr);
    debug_report_data *report_data = device_data->report_data;

    skip_call |= parameter_validation_vkCreateQueryPool(device_data->report_data, pCreateInfo, pAllocator, pQueryPool);

    // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
    if (pCreateInfo != nullptr) {
        // If queryType is VK_QUERY_TYPE_PIPELINE_STATISTICS, pipelineStatistics must be a valid combination of
        // VkQueryPipelineStatisticFlagBits values
        if ((pCreateInfo->queryType == VK_QUERY_TYPE_PIPELINE_STATISTICS) && (pCreateInfo->pipelineStatistics != 0) &&
            ((pCreateInfo->pipelineStatistics & (~AllVkQueryPipelineStatisticFlagBits)) != 0)) {
            skip_call |=
                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        UNRECOGNIZED_VALUE, LayerName, "vkCreateQueryPool: if pCreateInfo->queryType is "
                                                       "VK_QUERY_TYPE_PIPELINE_STATISTICS, pCreateInfo->pipelineStatistics must be "
                                                       "a valid combination of VkQueryPipelineStatisticFlagBits values");
        }
    }

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->CreateQueryPool(device, pCreateInfo, pAllocator, pQueryPool);

        validate_result(report_data, "vkCreateQueryPool", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroyQueryPool(my_data->report_data, queryPool, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->DestroyQueryPool(device, queryPool, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL GetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount,
                                                   size_t dataSize, void *pData, VkDeviceSize stride, VkQueryResultFlags flags) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetQueryPoolResults(my_data->report_data, queryPool, firstQuery, queryCount, dataSize,
                                                            pData, stride, flags);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)
                     ->GetQueryPoolResults(device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);

        validate_result(my_data->report_data, "vkGetQueryPoolResults", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateBuffer(VkDevice device, const VkBufferCreateInfo *pCreateInfo,
                                            const VkAllocationCallbacks *pAllocator, VkBuffer *pBuffer) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(device_data != nullptr);
    debug_report_data *report_data = device_data->report_data;

    // TODO: Add check for VALIDATION_ERROR_00660
    // TODO: Add check for VALIDATION_ERROR_00661
    // TODO: Add check for VALIDATION_ERROR_00662
    // TODO: Add check for VALIDATION_ERROR_00670
    // TODO: Add check for VALIDATION_ERROR_00671
    // TODO: Add check for VALIDATION_ERROR_00672
    // TODO: Add check for VALIDATION_ERROR_00673
    // TODO: Add check for VALIDATION_ERROR_00674
    // TODO: Add check for VALIDATION_ERROR_00675
    // TODO: Note that the above errors need to be generated from the next function, which is codegened.
    // TODO: Add check for VALIDATION_ERROR_00663
    skip_call |= parameter_validation_vkCreateBuffer(report_data, pCreateInfo, pAllocator, pBuffer);

    if (pCreateInfo != nullptr) {
        // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
        if (pCreateInfo->sharingMode == VK_SHARING_MODE_CONCURRENT) {
            // If sharingMode is VK_SHARING_MODE_CONCURRENT, queueFamilyIndexCount must be greater than 1
            if (pCreateInfo->queueFamilyIndexCount <= 1) {
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                     __LINE__, VALIDATION_ERROR_00665, LayerName,
                                     "vkCreateBuffer: if pCreateInfo->sharingMode is VK_SHARING_MODE_CONCURRENT, "
                                     "pCreateInfo->queueFamilyIndexCount must be greater than 1. %s",
                                     validation_error_map[VALIDATION_ERROR_00665]);
            }

            // If sharingMode is VK_SHARING_MODE_CONCURRENT, pQueueFamilyIndices must be a pointer to an array of
            // queueFamilyIndexCount uint32_t values
            if (pCreateInfo->pQueueFamilyIndices == nullptr) {
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                     __LINE__, VALIDATION_ERROR_00664, LayerName,
                                     "vkCreateBuffer: if pCreateInfo->sharingMode is VK_SHARING_MODE_CONCURRENT, "
                                     "pCreateInfo->pQueueFamilyIndices must be a pointer to an array of "
                                     "pCreateInfo->queueFamilyIndexCount uint32_t values. %s",
                                     validation_error_map[VALIDATION_ERROR_00664]);
            }

            // Ensure that the queue family indices were specified at device creation
            skip_call |= validate_queue_family_indices(device_data, "vkCreateBuffer", "pCreateInfo->pQueueFamilyIndices",
                                                       pCreateInfo->queueFamilyIndexCount, pCreateInfo->pQueueFamilyIndices);
        }
    }

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->CreateBuffer(device, pCreateInfo, pAllocator, pBuffer);

        validate_result(report_data, "vkCreateBuffer", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroyBuffer(my_data->report_data, buffer, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->DestroyBuffer(device, buffer, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateBufferView(VkDevice device, const VkBufferViewCreateInfo *pCreateInfo,
                                                const VkAllocationCallbacks *pAllocator, VkBufferView *pView) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCreateBufferView(my_data->report_data, pCreateInfo, pAllocator, pView);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->CreateBufferView(device, pCreateInfo, pAllocator, pView);

        validate_result(my_data->report_data, "vkCreateBufferView", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroyBufferView(my_data->report_data, bufferView, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->DestroyBufferView(device, bufferView, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo,
                                           const VkAllocationCallbacks *pAllocator, VkImage *pImage) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(device_data != nullptr);
    debug_report_data *report_data = device_data->report_data;

    skip_call |= parameter_validation_vkCreateImage(report_data, pCreateInfo, pAllocator, pImage);

    if (pCreateInfo != nullptr) {
        // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
        if (pCreateInfo->sharingMode == VK_SHARING_MODE_CONCURRENT) {
            // If sharingMode is VK_SHARING_MODE_CONCURRENT, queueFamilyIndexCount must be greater than 1
            if (pCreateInfo->queueFamilyIndexCount <= 1) {
                skip_call |=
                    log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            INVALID_USAGE, LayerName, "vkCreateImage: if pCreateInfo->sharingMode is VK_SHARING_MODE_CONCURRENT, "
                                                      "pCreateInfo->queueFamilyIndexCount must be greater than 1");
            }

            // If sharingMode is VK_SHARING_MODE_CONCURRENT, pQueueFamilyIndices must be a pointer to an array of
            // queueFamilyIndexCount uint32_t values
            if (pCreateInfo->pQueueFamilyIndices == nullptr) {
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                     __LINE__, REQUIRED_PARAMETER, LayerName,
                                     "vkCreateImage: if pCreateInfo->sharingMode is VK_SHARING_MODE_CONCURRENT, "
                                     "pCreateInfo->pQueueFamilyIndices must be a pointer to an array of "
                                     "pCreateInfo->queueFamilyIndexCount uint32_t values");
            }

            skip_call |= validate_queue_family_indices(device_data, "vkCreateImage", "pCreateInfo->pQueueFamilyIndices",
                                                       pCreateInfo->queueFamilyIndexCount, pCreateInfo->pQueueFamilyIndices);
        }

        // width, height, and depth members of extent must be greater than 0
        skip_call |= ValidateGreaterThan(report_data, "vkCreateImage", "pCreateInfo->extent.width", pCreateInfo->extent.width, 0u);
        skip_call |=
            ValidateGreaterThan(report_data, "vkCreateImage", "pCreateInfo->extent.height", pCreateInfo->extent.height, 0u);
        skip_call |= ValidateGreaterThan(report_data, "vkCreateImage", "pCreateInfo->extent.depth", pCreateInfo->extent.depth, 0u);

        // mipLevels must be greater than 0
        skip_call |= ValidateGreaterThan(report_data, "vkCreateImage", "pCreateInfo->mipLevels", pCreateInfo->mipLevels, 0u);

        // arrayLayers must be greater than 0
        skip_call |= ValidateGreaterThan(report_data, "vkCreateImage", "pCreateInfo->arrayLayers", pCreateInfo->arrayLayers, 0u);

        // If imageType is VK_IMAGE_TYPE_1D, both extent.height and extent.depth must be 1
        if ((pCreateInfo->imageType == VK_IMAGE_TYPE_1D) && (pCreateInfo->extent.height != 1) && (pCreateInfo->extent.depth != 1)) {
            skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1,
                                 LayerName, "vkCreateImage: if pCreateInfo->imageType is VK_IMAGE_TYPE_1D, both "
                                            "pCreateInfo->extent.height and pCreateInfo->extent.depth must be 1");
        }

        if (pCreateInfo->imageType == VK_IMAGE_TYPE_2D) {
            // If imageType is VK_IMAGE_TYPE_2D and flags contains VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, extent.width and
            // extent.height must be equal
            if ((pCreateInfo->flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) &&
                (pCreateInfo->extent.width != pCreateInfo->extent.height)) {
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1,
                                     LayerName, "vkCreateImage: if pCreateInfo->imageType is VK_IMAGE_TYPE_2D and "
                                                "pCreateInfo->flags contains VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, "
                                                "pCreateInfo->extent.width and pCreateInfo->extent.height must be equal");
            }

            if (pCreateInfo->extent.depth != 1) {
                skip_call |=
                    log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, LayerName,
                            "vkCreateImage: if pCreateInfo->imageType is VK_IMAGE_TYPE_2D, pCreateInfo->extent.depth must be 1");
            }
        }

        // mipLevels must be less than or equal to floor(log2(max(extent.width,extent.height,extent.depth)))+1
        uint32_t maxDim = std::max(std::max(pCreateInfo->extent.width, pCreateInfo->extent.height), pCreateInfo->extent.depth);
        if (pCreateInfo->mipLevels > (floor(log2(maxDim)) + 1)) {
            skip_call |=
                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, LayerName,
                        "vkCreateImage: pCreateInfo->mipLevels must be less than or equal to "
                        "floor(log2(max(pCreateInfo->extent.width, pCreateInfo->extent.height, pCreateInfo->extent.depth)))+1");
        }

        // If flags contains VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT or VK_IMAGE_CREATE_SPARSE_ALIASED_BIT, it must also contain
        // VK_IMAGE_CREATE_SPARSE_BINDING_BIT
        if (((pCreateInfo->flags & (VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT | VK_IMAGE_CREATE_SPARSE_ALIASED_BIT)) != 0) &&
            ((pCreateInfo->flags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT) != VK_IMAGE_CREATE_SPARSE_BINDING_BIT)) {
            skip_call |=
                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, LayerName,
                        "vkCreateImage: pCreateInfo->flags contains VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT or "
                        "VK_IMAGE_CREATE_SPARSE_ALIASED_BIT, it must also contain VK_IMAGE_CREATE_SPARSE_BINDING_BIT");
        }
    }

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->CreateImage(device, pCreateInfo, pAllocator, pImage);

        validate_result(report_data, "vkCreateImage", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroyImage(my_data->report_data, image, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->DestroyImage(device, image, pAllocator);
    }
}

bool PreGetImageSubresourceLayout(VkDevice device, const VkImageSubresource *pSubresource) {
    if (pSubresource != nullptr) {
        if ((pSubresource->aspectMask & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT |
                                         VK_IMAGE_ASPECT_METADATA_BIT)) == 0) {
            log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    UNRECOGNIZED_VALUE, LayerName,
                    "vkGetImageSubresourceLayout parameter, VkImageAspect pSubresource->aspectMask, is an unrecognized enumerator");
            return false;
        }
    }

    return true;
}

VKAPI_ATTR void VKAPI_CALL GetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource *pSubresource,
                                                     VkSubresourceLayout *pLayout) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetImageSubresourceLayout(my_data->report_data, image, pSubresource, pLayout);

    if (!skip_call) {
        PreGetImageSubresourceLayout(device, pSubresource);

        get_dispatch_table(pc_device_table_map, device)->GetImageSubresourceLayout(device, image, pSubresource, pLayout);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo,
                                               const VkAllocationCallbacks *pAllocator, VkImageView *pView) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);
    debug_report_data *report_data = my_data->report_data;

    skip_call |= parameter_validation_vkCreateImageView(report_data, pCreateInfo, pAllocator, pView);

    if (pCreateInfo != nullptr) {
        if ((pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_1D) || (pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_2D)) {
            if ((pCreateInfo->subresourceRange.layerCount != 1) &&
                (pCreateInfo->subresourceRange.layerCount != VK_REMAINING_ARRAY_LAYERS)) {
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1,
                                     LayerName, "vkCreateImageView: if pCreateInfo->viewType is VK_IMAGE_TYPE_%dD, "
                                                "pCreateInfo->subresourceRange.layerCount must be 1",
                                     ((pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_1D) ? 1 : 2));
            }
        } else if ((pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY) ||
                   (pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY)) {
            if ((pCreateInfo->subresourceRange.layerCount < 1) &&
                (pCreateInfo->subresourceRange.layerCount != VK_REMAINING_ARRAY_LAYERS)) {
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1,
                                     LayerName, "vkCreateImageView: if pCreateInfo->viewType is VK_IMAGE_TYPE_%dD_ARRAY, "
                                                "pCreateInfo->subresourceRange.layerCount must be >= 1",
                                     ((pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY) ? 1 : 2));
            }
        } else if (pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_CUBE) {
            if ((pCreateInfo->subresourceRange.layerCount != 6) &&
                (pCreateInfo->subresourceRange.layerCount != VK_REMAINING_ARRAY_LAYERS)) {
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1,
                                     LayerName, "vkCreateImageView: if pCreateInfo->viewType is VK_IMAGE_TYPE_CUBE, "
                                                "pCreateInfo->subresourceRange.layerCount must be 6");
            }
        } else if (pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
            if (((pCreateInfo->subresourceRange.layerCount == 0) || ((pCreateInfo->subresourceRange.layerCount % 6) != 0)) &&
                (pCreateInfo->subresourceRange.layerCount != VK_REMAINING_ARRAY_LAYERS)) {
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1,
                                     LayerName, "vkCreateImageView: if pCreateInfo->viewType is VK_IMAGE_TYPE_CUBE_ARRAY, "
                                                "pCreateInfo->subresourceRange.layerCount must be a multiple of 6");
            }
        } else if (pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_3D) {
            if (pCreateInfo->subresourceRange.baseArrayLayer != 0) {
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1,
                                     LayerName, "vkCreateImageView: if pCreateInfo->viewType is VK_IMAGE_TYPE_3D, "
                                                "pCreateInfo->subresourceRange.baseArrayLayer must be 0");
            }

            if ((pCreateInfo->subresourceRange.layerCount != 1) &&
                (pCreateInfo->subresourceRange.layerCount != VK_REMAINING_ARRAY_LAYERS)) {
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1,
                                     LayerName, "vkCreateImageView: if pCreateInfo->viewType is VK_IMAGE_TYPE_3D, "
                                                "pCreateInfo->subresourceRange.layerCount must be 1");
            }
        }
    }

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->CreateImageView(device, pCreateInfo, pAllocator, pView);

        validate_result(my_data->report_data, "vkCreateImageView", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroyImageView(my_data->report_data, imageView, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->DestroyImageView(device, imageView, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo,
                                                  const VkAllocationCallbacks *pAllocator, VkShaderModule *pShaderModule) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCreateShaderModule(my_data->report_data, pCreateInfo, pAllocator, pShaderModule);

    if (!skip_call) {
        result =
            get_dispatch_table(pc_device_table_map, device)->CreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);

        validate_result(my_data->report_data, "vkCreateShaderModule", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyShaderModule(VkDevice device, VkShaderModule shaderModule,
                                               const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroyShaderModule(my_data->report_data, shaderModule, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->DestroyShaderModule(device, shaderModule, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator, VkPipelineCache *pPipelineCache) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCreatePipelineCache(my_data->report_data, pCreateInfo, pAllocator, pPipelineCache);

    if (!skip_call) {
        result =
            get_dispatch_table(pc_device_table_map, device)->CreatePipelineCache(device, pCreateInfo, pAllocator, pPipelineCache);

        validate_result(my_data->report_data, "vkCreatePipelineCache", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache,
                                                const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroyPipelineCache(my_data->report_data, pipelineCache, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->DestroyPipelineCache(device, pipelineCache, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL GetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t *pDataSize,
                                                    void *pData) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetPipelineCacheData(my_data->report_data, pipelineCache, pDataSize, pData);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->GetPipelineCacheData(device, pipelineCache, pDataSize, pData);

        validate_result(my_data->report_data, "vkGetPipelineCacheData", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL MergePipelineCaches(VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount,
                                                   const VkPipelineCache *pSrcCaches) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkMergePipelineCaches(my_data->report_data, dstCache, srcCacheCount, pSrcCaches);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->MergePipelineCaches(device, dstCache, srcCacheCount, pSrcCaches);

        validate_result(my_data->report_data, "vkMergePipelineCaches", result);
    }

    return result;
}

bool PreCreateGraphicsPipelines(VkDevice device, const VkGraphicsPipelineCreateInfo *pCreateInfos) {
    layer_data *data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    // TODO: Handle count
    if (pCreateInfos != nullptr) {
        if (pCreateInfos->flags | VK_PIPELINE_CREATE_DERIVATIVE_BIT) {
            if (pCreateInfos->basePipelineIndex != -1) {
                if (pCreateInfos->basePipelineHandle != VK_NULL_HANDLE) {
                    log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            INVALID_USAGE, LayerName,
                            "vkCreateGraphicsPipelines parameter, pCreateInfos->basePipelineHandle, must be VK_NULL_HANDLE if "
                            "pCreateInfos->flags "
                            "contains the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag and pCreateInfos->basePipelineIndex is not -1");
                    return false;
                }
            }

            if (pCreateInfos->basePipelineHandle != VK_NULL_HANDLE) {
                if (pCreateInfos->basePipelineIndex != -1) {
                    log_msg(
                        mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        INVALID_USAGE, LayerName,
                        "vkCreateGraphicsPipelines parameter, pCreateInfos->basePipelineIndex, must be -1 if pCreateInfos->flags "
                        "contains the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag and pCreateInfos->basePipelineHandle is not "
                        "VK_NULL_HANDLE");
                    return false;
                }
            }
        }

        if (pCreateInfos->pRasterizationState != nullptr) {
            if (pCreateInfos->pRasterizationState->cullMode & ~VK_CULL_MODE_FRONT_AND_BACK) {
                log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        UNRECOGNIZED_VALUE, LayerName,
                        "vkCreateGraphicsPipelines parameter, VkCullMode pCreateInfos->pRasterizationState->cullMode, is an "
                        "unrecognized enumerator");
                return false;
            }

            if ((pCreateInfos->pRasterizationState->polygonMode != VK_POLYGON_MODE_FILL) &&
                (data->physical_device_features.fillModeNonSolid == false)) {
                log_msg(
                    mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    DEVICE_FEATURE, LayerName,
                    "vkCreateGraphicsPipelines parameter, VkPolygonMode pCreateInfos->pRasterizationState->polygonMode cannot be "
                    "VK_POLYGON_MODE_POINT or VK_POLYGON_MODE_LINE if VkPhysicalDeviceFeatures->fillModeNonSolid is false.");
                return false;
            }
        }

        size_t i = 0;
        for (size_t j = 0; j < pCreateInfos[i].stageCount; j++) {
            validate_string(data->report_data, "vkCreateGraphicsPipelines",
                            ParameterName("pCreateInfos[%i].pStages[%i].pName", ParameterName::IndexVector{i, j}),
                            pCreateInfos[i].pStages[j].pName);
        }
    }

    return true;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                                       const VkGraphicsPipelineCreateInfo *pCreateInfos,
                                                       const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(device_data != nullptr);
    debug_report_data *report_data = device_data->report_data;

    skip_call |= parameter_validation_vkCreateGraphicsPipelines(report_data, pipelineCache, createInfoCount, pCreateInfos,
                                                                pAllocator, pPipelines);

    if (pCreateInfos != nullptr) {
        for (uint32_t i = 0; i < createInfoCount; ++i) {
            // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
            if (pCreateInfos[i].pTessellationState == nullptr) {
                if (pCreateInfos[i].pStages != nullptr) {
                    // If pStages includes a tessellation control shader stage and a tessellation evaluation shader stage,
                    // pTessellationState must not be NULL
                    bool has_control = false;
                    bool has_eval = false;

                    for (uint32_t stage_index = 0; stage_index < pCreateInfos[i].stageCount; ++stage_index) {
                        if (pCreateInfos[i].pStages[stage_index].stage == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) {
                            has_control = true;
                        } else if (pCreateInfos[i].pStages[stage_index].stage == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) {
                            has_eval = true;
                        }
                    }

                    if (has_control && has_eval) {
                        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                             __LINE__, REQUIRED_PARAMETER, LayerName,
                                             "vkCreateGraphicsPipelines: if pCreateInfos[%d].pStages includes a tessellation "
                                             "control shader stage and a tessellation evaluation shader stage, "
                                             "pCreateInfos[%d].pTessellationState must not be NULL",
                                             i, i);
                    }
                }
            } else {
                skip_call |= validate_struct_pnext(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pTessellationState->pNext", ParameterName::IndexVector{i}), NULL,
                    pCreateInfos[i].pTessellationState->pNext, 0, NULL, GeneratedHeaderVersion);

                skip_call |= validate_reserved_flags(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pTessellationState->flags", ParameterName::IndexVector{i}),
                    pCreateInfos[i].pTessellationState->flags);

                if (pCreateInfos[i].pTessellationState->sType != VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO) {
                    skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                         __LINE__, INVALID_STRUCT_STYPE, LayerName,
                                         "vkCreateGraphicsPipelines: parameter pCreateInfos[%d].pTessellationState->sType must be "
                                         "VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO",
                                         i);
                }
            }

            if (pCreateInfos[i].pViewportState == nullptr) {
                // If the rasterizerDiscardEnable member of pRasterizationState is VK_FALSE, pViewportState must be a pointer to a
                // valid VkPipelineViewportStateCreateInfo structure
                if ((pCreateInfos[i].pRasterizationState != nullptr) &&
                    (pCreateInfos[i].pRasterizationState->rasterizerDiscardEnable == VK_FALSE)) {
                    skip_call |= log_msg(
                        report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        REQUIRED_PARAMETER, LayerName,
                        "vkCreateGraphicsPipelines: if pCreateInfos[%d].pRasterizationState->rasterizerDiscardEnable is VK_FALSE, "
                        "pCreateInfos[%d].pViewportState must be a pointer to a valid VkPipelineViewportStateCreateInfo structure",
                        i, i);
                }
            } else {
                skip_call |=
                    validate_struct_pnext(report_data, "vkCreateGraphicsPipelines",
                                          ParameterName("pCreateInfos[%i].pViewportState->pNext", ParameterName::IndexVector{i}),
                                          NULL, pCreateInfos[i].pViewportState->pNext, 0, NULL, GeneratedHeaderVersion);

                skip_call |=
                    validate_reserved_flags(report_data, "vkCreateGraphicsPipelines",
                                            ParameterName("pCreateInfos[%i].pViewportState->flags", ParameterName::IndexVector{i}),
                                            pCreateInfos[i].pViewportState->flags);

                if (pCreateInfos[i].pViewportState->sType != VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO) {
                    skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                         __LINE__, INVALID_STRUCT_STYPE, LayerName,
                                         "vkCreateGraphicsPipelines: parameter pCreateInfos[%d].pViewportState->sType must be "
                                         "VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO",
                                         i);
                }

                if (pCreateInfos[i].pDynamicState != nullptr) {
                    bool has_dynamic_viewport = false;
                    bool has_dynamic_scissor = false;

                    for (uint32_t state_index = 0; state_index < pCreateInfos[i].pDynamicState->dynamicStateCount; ++state_index) {
                        if (pCreateInfos[i].pDynamicState->pDynamicStates[state_index] == VK_DYNAMIC_STATE_VIEWPORT) {
                            has_dynamic_viewport = true;
                        } else if (pCreateInfos[i].pDynamicState->pDynamicStates[state_index] == VK_DYNAMIC_STATE_SCISSOR) {
                            has_dynamic_scissor = true;
                        }
                    }

                    // viewportCount must be greater than 0
                    // TODO: viewportCount must be 1 when multiple_viewport feature is not enabled
                    if (pCreateInfos[i].pViewportState->viewportCount == 0) {
                        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                             __LINE__, REQUIRED_PARAMETER, LayerName,
                                             "vkCreateGraphicsPipelines: if pCreateInfos[%d].pDynamicState->pDynamicStates does "
                                             "not contain VK_DYNAMIC_STATE_VIEWPORT, pCreateInfos[%d].pViewportState->viewportCount "
                                             "must be greater than 0",
                                             i, i);
                    }

                    // If no element of the pDynamicStates member of pDynamicState is VK_DYNAMIC_STATE_VIEWPORT, the pViewports
                    // member of pViewportState must be a pointer to an array of pViewportState->viewportCount VkViewport structures
                    if (!has_dynamic_viewport && (pCreateInfos[i].pViewportState->pViewports == nullptr)) {
                        skip_call |=
                            log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                    __LINE__, REQUIRED_PARAMETER, LayerName,
                                    "vkCreateGraphicsPipelines: if pCreateInfos[%d].pDynamicState->pDynamicStates does not contain "
                                    "VK_DYNAMIC_STATE_VIEWPORT, pCreateInfos[%d].pViewportState->pViewports must not be NULL",
                                    i, i);
                    }

                    // scissorCount must be greater than 0
                    // TODO: scissorCount must be 1 when multiple_viewport feature is not enabled
                    if (pCreateInfos[i].pViewportState->scissorCount == 0) {
                        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                             __LINE__, REQUIRED_PARAMETER, LayerName,
                                             "vkCreateGraphicsPipelines: if pCreateInfos[%d].pDynamicState->pDynamicStates does "
                                             "not contain VK_DYNAMIC_STATE_SCISSOR, pCreateInfos[%d].pViewportState->scissorCount "
                                             "must be greater than 0",
                                             i, i);
                    }

                    // If no element of the pDynamicStates member of pDynamicState is VK_DYNAMIC_STATE_SCISSOR, the pScissors member
                    // of pViewportState must be a pointer to an array of pViewportState->scissorCount VkRect2D structures
                    if (!has_dynamic_scissor && (pCreateInfos[i].pViewportState->pScissors == nullptr)) {
                        skip_call |=
                            log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                    __LINE__, REQUIRED_PARAMETER, LayerName,
                                    "vkCreateGraphicsPipelines: if pCreateInfos[%d].pDynamicState->pDynamicStates does not contain "
                                    "VK_DYNAMIC_STATE_SCISSOR, pCreateInfos[%d].pViewportState->pScissors must not be NULL",
                                    i, i);
                    }
                }
            }

            if (pCreateInfos[i].pMultisampleState == nullptr) {
                // If the rasterizerDiscardEnable member of pRasterizationState is VK_FALSE, pMultisampleState must be a pointer to
                // a valid VkPipelineMultisampleStateCreateInfo structure
                if ((pCreateInfos[i].pRasterizationState != nullptr) &&
                    pCreateInfos[i].pRasterizationState->rasterizerDiscardEnable == VK_FALSE) {
                    skip_call |=
                        log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                                REQUIRED_PARAMETER, LayerName, "vkCreateGraphicsPipelines: if "
                                                               "pCreateInfos[%d].pRasterizationState->rasterizerDiscardEnable is "
                                                               "VK_FALSE, pCreateInfos[%d].pMultisampleState must not be NULL",
                                i, i);
                }
            } else {
                skip_call |=
                    validate_struct_pnext(report_data, "vkCreateGraphicsPipelines",
                                          ParameterName("pCreateInfos[%i].pMultisampleState->pNext", ParameterName::IndexVector{i}),
                                          NULL, pCreateInfos[i].pMultisampleState->pNext, 0, NULL, GeneratedHeaderVersion);

                skip_call |= validate_reserved_flags(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pMultisampleState->flags", ParameterName::IndexVector{i}),
                    pCreateInfos[i].pMultisampleState->flags);

                skip_call |= validate_bool32(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pMultisampleState->sampleShadingEnable", ParameterName::IndexVector{i}),
                    pCreateInfos[i].pMultisampleState->sampleShadingEnable);

                skip_call |= validate_array(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pMultisampleState->rasterizationSamples", ParameterName::IndexVector{i}),
                    ParameterName("pCreateInfos[%i].pMultisampleState->pSampleMask", ParameterName::IndexVector{i}),
                    pCreateInfos[i].pMultisampleState->rasterizationSamples, pCreateInfos[i].pMultisampleState->pSampleMask, true,
                    false);

                skip_call |= validate_bool32(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pMultisampleState->alphaToCoverageEnable", ParameterName::IndexVector{i}),
                    pCreateInfos[i].pMultisampleState->alphaToCoverageEnable);

                skip_call |= validate_bool32(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pMultisampleState->alphaToOneEnable", ParameterName::IndexVector{i}),
                    pCreateInfos[i].pMultisampleState->alphaToOneEnable);

                if (pCreateInfos[i].pMultisampleState->sType != VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO) {
                    skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                         __LINE__, INVALID_STRUCT_STYPE, LayerName,
                                         "vkCreateGraphicsPipelines: parameter pCreateInfos[%d].pMultisampleState->sType must be "
                                         "VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO",
                                         i);
                }
            }

            // TODO: Conditional NULL check based on rasterizerDiscardEnable and subpass
            if (pCreateInfos[i].pDepthStencilState != nullptr) {
                skip_call |= validate_struct_pnext(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pDepthStencilState->pNext", ParameterName::IndexVector{i}), NULL,
                    pCreateInfos[i].pDepthStencilState->pNext, 0, NULL, GeneratedHeaderVersion);

                skip_call |= validate_reserved_flags(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pDepthStencilState->flags", ParameterName::IndexVector{i}),
                    pCreateInfos[i].pDepthStencilState->flags);

                skip_call |= validate_bool32(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pDepthStencilState->depthTestEnable", ParameterName::IndexVector{i}),
                    pCreateInfos[i].pDepthStencilState->depthTestEnable);

                skip_call |= validate_bool32(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pDepthStencilState->depthWriteEnable", ParameterName::IndexVector{i}),
                    pCreateInfos[i].pDepthStencilState->depthWriteEnable);

                skip_call |= validate_ranged_enum(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pDepthStencilState->depthCompareOp", ParameterName::IndexVector{i}),
                    "VkCompareOp", VK_COMPARE_OP_BEGIN_RANGE, VK_COMPARE_OP_END_RANGE,
                    pCreateInfos[i].pDepthStencilState->depthCompareOp);

                skip_call |= validate_bool32(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pDepthStencilState->depthBoundsTestEnable", ParameterName::IndexVector{i}),
                    pCreateInfos[i].pDepthStencilState->depthBoundsTestEnable);

                skip_call |= validate_bool32(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pDepthStencilState->stencilTestEnable", ParameterName::IndexVector{i}),
                    pCreateInfos[i].pDepthStencilState->stencilTestEnable);

                skip_call |= validate_ranged_enum(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pDepthStencilState->front.failOp", ParameterName::IndexVector{i}),
                    "VkStencilOp", VK_STENCIL_OP_BEGIN_RANGE, VK_STENCIL_OP_END_RANGE,
                    pCreateInfos[i].pDepthStencilState->front.failOp);

                skip_call |= validate_ranged_enum(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pDepthStencilState->front.passOp", ParameterName::IndexVector{i}),
                    "VkStencilOp", VK_STENCIL_OP_BEGIN_RANGE, VK_STENCIL_OP_END_RANGE,
                    pCreateInfos[i].pDepthStencilState->front.passOp);

                skip_call |= validate_ranged_enum(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pDepthStencilState->front.depthFailOp", ParameterName::IndexVector{i}),
                    "VkStencilOp", VK_STENCIL_OP_BEGIN_RANGE, VK_STENCIL_OP_END_RANGE,
                    pCreateInfos[i].pDepthStencilState->front.depthFailOp);

                skip_call |= validate_ranged_enum(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pDepthStencilState->front.compareOp", ParameterName::IndexVector{i}),
                    "VkCompareOp", VK_COMPARE_OP_BEGIN_RANGE, VK_COMPARE_OP_END_RANGE,
                    pCreateInfos[i].pDepthStencilState->front.compareOp);

                skip_call |= validate_ranged_enum(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pDepthStencilState->back.failOp", ParameterName::IndexVector{i}), "VkStencilOp",
                    VK_STENCIL_OP_BEGIN_RANGE, VK_STENCIL_OP_END_RANGE, pCreateInfos[i].pDepthStencilState->back.failOp);

                skip_call |= validate_ranged_enum(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pDepthStencilState->back.passOp", ParameterName::IndexVector{i}), "VkStencilOp",
                    VK_STENCIL_OP_BEGIN_RANGE, VK_STENCIL_OP_END_RANGE, pCreateInfos[i].pDepthStencilState->back.passOp);

                skip_call |= validate_ranged_enum(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pDepthStencilState->back.depthFailOp", ParameterName::IndexVector{i}),
                    "VkStencilOp", VK_STENCIL_OP_BEGIN_RANGE, VK_STENCIL_OP_END_RANGE,
                    pCreateInfos[i].pDepthStencilState->back.depthFailOp);

                skip_call |= validate_ranged_enum(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pDepthStencilState->back.compareOp", ParameterName::IndexVector{i}),
                    "VkCompareOp", VK_COMPARE_OP_BEGIN_RANGE, VK_COMPARE_OP_END_RANGE,
                    pCreateInfos[i].pDepthStencilState->back.compareOp);

                if (pCreateInfos[i].pDepthStencilState->sType != VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO) {
                    skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                         __LINE__, INVALID_STRUCT_STYPE, LayerName,
                                         "vkCreateGraphicsPipelines: parameter pCreateInfos[%d].pDepthStencilState->sType must be "
                                         "VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO",
                                         i);
                }
            }

            // TODO: Conditional NULL check based on rasterizerDiscardEnable and subpass
            if (pCreateInfos[i].pColorBlendState != nullptr) {
                skip_call |=
                    validate_struct_pnext(report_data, "vkCreateGraphicsPipelines",
                                          ParameterName("pCreateInfos[%i].pColorBlendState->pNext", ParameterName::IndexVector{i}),
                                          NULL, pCreateInfos[i].pColorBlendState->pNext, 0, NULL, GeneratedHeaderVersion);

                skip_call |= validate_reserved_flags(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pColorBlendState->flags", ParameterName::IndexVector{i}),
                    pCreateInfos[i].pColorBlendState->flags);

                skip_call |= validate_bool32(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pColorBlendState->logicOpEnable", ParameterName::IndexVector{i}),
                    pCreateInfos[i].pColorBlendState->logicOpEnable);

                skip_call |= validate_array(
                    report_data, "vkCreateGraphicsPipelines",
                    ParameterName("pCreateInfos[%i].pColorBlendState->attachmentCount", ParameterName::IndexVector{i}),
                    ParameterName("pCreateInfos[%i].pColorBlendState->pAttachments", ParameterName::IndexVector{i}),
                    pCreateInfos[i].pColorBlendState->attachmentCount, pCreateInfos[i].pColorBlendState->pAttachments, false, true);

                if (pCreateInfos[i].pColorBlendState->pAttachments != NULL) {
                    for (uint32_t attachmentIndex = 0; attachmentIndex < pCreateInfos[i].pColorBlendState->attachmentCount;
                         ++attachmentIndex) {
                        skip_call |=
                            validate_bool32(report_data, "vkCreateGraphicsPipelines",
                                            ParameterName("pCreateInfos[%i].pColorBlendState->pAttachments[%i].blendEnable",
                                                          ParameterName::IndexVector{i, attachmentIndex}),
                                            pCreateInfos[i].pColorBlendState->pAttachments[attachmentIndex].blendEnable);

                        skip_call |= validate_ranged_enum(
                            report_data, "vkCreateGraphicsPipelines",
                            ParameterName("pCreateInfos[%i].pColorBlendState->pAttachments[%i].srcColorBlendFactor",
                                          ParameterName::IndexVector{i, attachmentIndex}),
                            "VkBlendFactor", VK_BLEND_FACTOR_BEGIN_RANGE, VK_BLEND_FACTOR_END_RANGE,
                            pCreateInfos[i].pColorBlendState->pAttachments[attachmentIndex].srcColorBlendFactor);

                        skip_call |= validate_ranged_enum(
                            report_data, "vkCreateGraphicsPipelines",
                            ParameterName("pCreateInfos[%i].pColorBlendState->pAttachments[%i].dstColorBlendFactor",
                                          ParameterName::IndexVector{i, attachmentIndex}),
                            "VkBlendFactor", VK_BLEND_FACTOR_BEGIN_RANGE, VK_BLEND_FACTOR_END_RANGE,
                            pCreateInfos[i].pColorBlendState->pAttachments[attachmentIndex].dstColorBlendFactor);

                        skip_call |=
                            validate_ranged_enum(report_data, "vkCreateGraphicsPipelines",
                                                 ParameterName("pCreateInfos[%i].pColorBlendState->pAttachments[%i].colorBlendOp",
                                                               ParameterName::IndexVector{i, attachmentIndex}),
                                                 "VkBlendOp", VK_BLEND_OP_BEGIN_RANGE, VK_BLEND_OP_END_RANGE,
                                                 pCreateInfos[i].pColorBlendState->pAttachments[attachmentIndex].colorBlendOp);

                        skip_call |= validate_ranged_enum(
                            report_data, "vkCreateGraphicsPipelines",
                            ParameterName("pCreateInfos[%i].pColorBlendState->pAttachments[%i].srcAlphaBlendFactor",
                                          ParameterName::IndexVector{i, attachmentIndex}),
                            "VkBlendFactor", VK_BLEND_FACTOR_BEGIN_RANGE, VK_BLEND_FACTOR_END_RANGE,
                            pCreateInfos[i].pColorBlendState->pAttachments[attachmentIndex].srcAlphaBlendFactor);

                        skip_call |= validate_ranged_enum(
                            report_data, "vkCreateGraphicsPipelines",
                            ParameterName("pCreateInfos[%i].pColorBlendState->pAttachments[%i].dstAlphaBlendFactor",
                                          ParameterName::IndexVector{i, attachmentIndex}),
                            "VkBlendFactor", VK_BLEND_FACTOR_BEGIN_RANGE, VK_BLEND_FACTOR_END_RANGE,
                            pCreateInfos[i].pColorBlendState->pAttachments[attachmentIndex].dstAlphaBlendFactor);

                        skip_call |=
                            validate_ranged_enum(report_data, "vkCreateGraphicsPipelines",
                                                 ParameterName("pCreateInfos[%i].pColorBlendState->pAttachments[%i].alphaBlendOp",
                                                               ParameterName::IndexVector{i, attachmentIndex}),
                                                 "VkBlendOp", VK_BLEND_OP_BEGIN_RANGE, VK_BLEND_OP_END_RANGE,
                                                 pCreateInfos[i].pColorBlendState->pAttachments[attachmentIndex].alphaBlendOp);

                        skip_call |=
                            validate_flags(report_data, "vkCreateGraphicsPipelines",
                                           ParameterName("pCreateInfos[%i].pColorBlendState->pAttachments[%i].colorWriteMask",
                                                         ParameterName::IndexVector{i, attachmentIndex}),
                                           "VkColorComponentFlagBits", AllVkColorComponentFlagBits,
                                           pCreateInfos[i].pColorBlendState->pAttachments[attachmentIndex].colorWriteMask, false);
                    }
                }

                if (pCreateInfos[i].pColorBlendState->sType != VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO) {
                    skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                         __LINE__, INVALID_STRUCT_STYPE, LayerName,
                                         "vkCreateGraphicsPipelines: parameter pCreateInfos[%d].pColorBlendState->sType must be "
                                         "VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO",
                                         i);
                }

                // If logicOpEnable is VK_TRUE, logicOp must be a valid VkLogicOp value
                if (pCreateInfos[i].pColorBlendState->logicOpEnable == VK_TRUE) {
                    skip_call |= validate_ranged_enum(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pColorBlendState->logicOp", ParameterName::IndexVector{i}), "VkLogicOp",
                        VK_LOGIC_OP_BEGIN_RANGE, VK_LOGIC_OP_END_RANGE, pCreateInfos[i].pColorBlendState->logicOp);
                }
            }
        }
    }

    if (!skip_call) {
        PreCreateGraphicsPipelines(device, pCreateInfos);

        result = get_dispatch_table(pc_device_table_map, device)
                     ->CreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);

        validate_result(report_data, "vkCreateGraphicsPipelines", result);
    }

    return result;
}

bool PreCreateComputePipelines(VkDevice device, const VkComputePipelineCreateInfo *pCreateInfos) {
    layer_data *data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    if (pCreateInfos != nullptr) {
        // TODO: Handle count!
        uint32_t i = 0;
        validate_string(data->report_data, "vkCreateComputePipelines",
                        ParameterName("pCreateInfos[%i].stage.pName", ParameterName::IndexVector{i}), pCreateInfos[i].stage.pName);
    }

    return true;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                                      const VkComputePipelineCreateInfo *pCreateInfos,
                                                      const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCreateComputePipelines(my_data->report_data, pipelineCache, createInfoCount, pCreateInfos,
                                                               pAllocator, pPipelines);

    if (!skip_call) {
        PreCreateComputePipelines(device, pCreateInfos);

        result = get_dispatch_table(pc_device_table_map, device)
                     ->CreateComputePipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);

        validate_result(my_data->report_data, "vkCreateComputePipelines", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroyPipeline(my_data->report_data, pipeline, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->DestroyPipeline(device, pipeline, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo,
                                                    const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pPipelineLayout) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCreatePipelineLayout(my_data->report_data, pCreateInfo, pAllocator, pPipelineLayout);

    if (!skip_call) {
        result =
            get_dispatch_table(pc_device_table_map, device)->CreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);

        validate_result(my_data->report_data, "vkCreatePipelineLayout", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout,
                                                 const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroyPipelineLayout(my_data->report_data, pipelineLayout, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->DestroyPipelineLayout(device, pipelineLayout, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo,
                                             const VkAllocationCallbacks *pAllocator, VkSampler *pSampler) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(device_data != NULL);
    debug_report_data *report_data = device_data->report_data;

    skip_call |= parameter_validation_vkCreateSampler(report_data, pCreateInfo, pAllocator, pSampler);

    // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
    if (pCreateInfo != nullptr) {
        // If compareEnable is VK_TRUE, compareOp must be a valid VkCompareOp value
        if (pCreateInfo->compareEnable == VK_TRUE) {
            skip_call |= validate_ranged_enum(report_data, "vkCreateSampler", "pCreateInfo->compareOp", "VkCompareOp",
                                              VK_COMPARE_OP_BEGIN_RANGE, VK_COMPARE_OP_END_RANGE, pCreateInfo->compareOp);
        }

        // If any of addressModeU, addressModeV or addressModeW are VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, borderColor must be a
        // valid VkBorderColor value
        if ((pCreateInfo->addressModeU == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER) ||
            (pCreateInfo->addressModeV == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER) ||
            (pCreateInfo->addressModeW == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)) {
            skip_call |= validate_ranged_enum(report_data, "vkCreateSampler", "pCreateInfo->borderColor", "VkBorderColor",
                                              VK_BORDER_COLOR_BEGIN_RANGE, VK_BORDER_COLOR_END_RANGE, pCreateInfo->borderColor);
        }
    }

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->CreateSampler(device, pCreateInfo, pAllocator, pSampler);

        validate_result(report_data, "vkCreateSampler", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroySampler(my_data->report_data, sampler, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->DestroySampler(device, sampler, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                                                         const VkAllocationCallbacks *pAllocator,
                                                         VkDescriptorSetLayout *pSetLayout) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(device_data != nullptr);
    debug_report_data *report_data = device_data->report_data;

    skip_call |= parameter_validation_vkCreateDescriptorSetLayout(report_data, pCreateInfo, pAllocator, pSetLayout);

    // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
    if ((pCreateInfo != nullptr) && (pCreateInfo->pBindings != nullptr)) {
        for (uint32_t i = 0; i < pCreateInfo->bindingCount; ++i) {
            if (pCreateInfo->pBindings[i].descriptorCount != 0) {
                // If descriptorType is VK_DESCRIPTOR_TYPE_SAMPLER or VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, and descriptorCount
                // is not 0 and pImmutableSamplers is not NULL, pImmutableSamplers must be a pointer to an array of descriptorCount
                // valid VkSampler handles
                if (((pCreateInfo->pBindings[i].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER) ||
                     (pCreateInfo->pBindings[i].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)) &&
                    (pCreateInfo->pBindings[i].pImmutableSamplers != nullptr)) {
                    for (uint32_t descriptor_index = 0; descriptor_index < pCreateInfo->pBindings[i].descriptorCount;
                         ++descriptor_index) {
                        if (pCreateInfo->pBindings[i].pImmutableSamplers[descriptor_index] == VK_NULL_HANDLE) {
                            skip_call |=
                                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                        __LINE__, REQUIRED_PARAMETER, LayerName, "vkCreateDescriptorSetLayout: required parameter "
                                                                                 "pCreateInfo->pBindings[%d].pImmutableSamplers[%d]"
                                                                                 " specified as VK_NULL_HANDLE",
                                        i, descriptor_index);
                        }
                    }
                }

                // If descriptorCount is not 0, stageFlags must be a valid combination of VkShaderStageFlagBits values
                if ((pCreateInfo->pBindings[i].stageFlags != 0) &&
                    ((pCreateInfo->pBindings[i].stageFlags & (~AllVkShaderStageFlagBits)) != 0)) {
                    skip_call |=
                        log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                                UNRECOGNIZED_VALUE, LayerName,
                                "vkCreateDescriptorSetLayout: if pCreateInfo->pBindings[%d].descriptorCount is not 0, "
                                "pCreateInfo->pBindings[%d].stageFlags must be a valid combination of VkShaderStageFlagBits values",
                                i, i);
                }
            }
        }
    }

    if (!skip_call) {
        result =
            get_dispatch_table(pc_device_table_map, device)->CreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);

        validate_result(report_data, "vkCreateDescriptorSetLayout", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout,
                                                      const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroyDescriptorSetLayout(my_data->report_data, descriptorSetLayout, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->DestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *pCreateInfo,
                                                    const VkAllocationCallbacks *pAllocator, VkDescriptorPool *pDescriptorPool) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCreateDescriptorPool(my_data->report_data, pCreateInfo, pAllocator, pDescriptorPool);

    /* TODOVV: How do we validate maxSets? Probably belongs in the limits layer? */

    if (!skip_call) {
        result =
            get_dispatch_table(pc_device_table_map, device)->CreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);

        validate_result(my_data->report_data, "vkCreateDescriptorPool", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                                                 const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroyDescriptorPool(my_data->report_data, descriptorPool, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->DestroyDescriptorPool(device, descriptorPool, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL ResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                                                   VkDescriptorPoolResetFlags flags) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkResetDescriptorPool(my_data->report_data, descriptorPool, flags);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->ResetDescriptorPool(device, descriptorPool, flags);

        validate_result(my_data->report_data, "vkResetDescriptorPool", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL AllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo,
                                                      VkDescriptorSet *pDescriptorSets) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkAllocateDescriptorSets(my_data->report_data, pAllocateInfo, pDescriptorSets);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->AllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);

        validate_result(my_data->report_data, "vkAllocateDescriptorSets", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL FreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount,
                                                  const VkDescriptorSet *pDescriptorSets) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(device_data != nullptr);
    debug_report_data *report_data = device_data->report_data;

    skip_call |= parameter_validation_vkFreeDescriptorSets(report_data, descriptorPool, descriptorSetCount, pDescriptorSets);

    // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
    // This is an array of handles, where the elements are allowed to be VK_NULL_HANDLE, and does not require any validation beyond
    // validate_array()
    skip_call |= validate_array(report_data, "vkFreeDescriptorSets", "descriptorSetCount", "pDescriptorSets", descriptorSetCount,
                                pDescriptorSets, true, true);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)
                     ->FreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);

        validate_result(report_data, "vkFreeDescriptorSets", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL UpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount,
                                                const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount,
                                                const VkCopyDescriptorSet *pDescriptorCopies) {
    bool skip_call = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(device_data != NULL);
    debug_report_data *report_data = device_data->report_data;

    skip_call |= parameter_validation_vkUpdateDescriptorSets(report_data, descriptorWriteCount, pDescriptorWrites,
                                                             descriptorCopyCount, pDescriptorCopies);

    // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
    if (pDescriptorWrites != NULL) {
        for (uint32_t i = 0; i < descriptorWriteCount; ++i) {
            // descriptorCount must be greater than 0
            if (pDescriptorWrites[i].descriptorCount == 0) {
                skip_call |=
                    log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            REQUIRED_PARAMETER, LayerName,
                            "vkUpdateDescriptorSets: parameter pDescriptorWrites[%d].descriptorCount must be greater than 0", i);
            }

            if ((pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER) ||
                (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) ||
                (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) ||
                (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) ||
                (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)) {
                // If descriptorType is VK_DESCRIPTOR_TYPE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE or VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                // pImageInfo must be a pointer to an array of descriptorCount valid VkDescriptorImageInfo structures
                if (pDescriptorWrites[i].pImageInfo == nullptr) {
                    skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                         __LINE__, REQUIRED_PARAMETER, LayerName,
                                         "vkUpdateDescriptorSets: if pDescriptorWrites[%d].descriptorType is "
                                         "VK_DESCRIPTOR_TYPE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, "
                                         "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE or "
                                         "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, pDescriptorWrites[%d].pImageInfo must not be NULL",
                                         i, i);
                } else if (pDescriptorWrites[i].descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER) {
                    // If descriptorType is VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE or VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, the imageView and imageLayout
                    // members of any given element of pImageInfo must be a valid VkImageView and VkImageLayout, respectively
                    for (uint32_t descriptor_index = 0; descriptor_index < pDescriptorWrites[i].descriptorCount;
                         ++descriptor_index) {
                        skip_call |= validate_required_handle(report_data, "vkUpdateDescriptorSets",
                                                              ParameterName("pDescriptorWrites[%i].pImageInfo[%i].imageView",
                                                                            ParameterName::IndexVector{i, descriptor_index}),
                                                              pDescriptorWrites[i].pImageInfo[descriptor_index].imageView);
                        skip_call |= validate_ranged_enum(report_data, "vkUpdateDescriptorSets",
                                                          ParameterName("pDescriptorWrites[%i].pImageInfo[%i].imageLayout",
                                                                        ParameterName::IndexVector{i, descriptor_index}),
                                                          "VkImageLayout", VK_IMAGE_LAYOUT_BEGIN_RANGE, VK_IMAGE_LAYOUT_END_RANGE,
                                                          pDescriptorWrites[i].pImageInfo[descriptor_index].imageLayout);
                    }
                }
            } else if ((pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ||
                       (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) ||
                       (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ||
                       (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) {
                // If descriptorType is VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC or VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, pBufferInfo must be a
                // pointer to an array of descriptorCount valid VkDescriptorBufferInfo structures
                if (pDescriptorWrites[i].pBufferInfo == nullptr) {
                    skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                         __LINE__, REQUIRED_PARAMETER, LayerName,
                                         "vkUpdateDescriptorSets: if pDescriptorWrites[%d].descriptorType is "
                                         "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, "
                                         "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC or VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, "
                                         "pDescriptorWrites[%d].pBufferInfo must not be NULL",
                                         i, i);
                } else {
                    for (uint32_t descriptorIndex = 0; descriptorIndex < pDescriptorWrites[i].descriptorCount; ++descriptorIndex) {
                        skip_call |= validate_required_handle(report_data, "vkUpdateDescriptorSets",
                                                              ParameterName("pDescriptorWrites[%i].pBufferInfo[%i].buffer",
                                                                            ParameterName::IndexVector{i, descriptorIndex}),
                                                              pDescriptorWrites[i].pBufferInfo[descriptorIndex].buffer);
                    }
                }
            } else if ((pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) ||
                       (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)) {
                // If descriptorType is VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER or VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                // pTexelBufferView must be a pointer to an array of descriptorCount valid VkBufferView handles
                if (pDescriptorWrites[i].pTexelBufferView == nullptr) {
                    skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                         __LINE__, REQUIRED_PARAMETER, LayerName,
                                         "vkUpdateDescriptorSets: if pDescriptorWrites[%d].descriptorType is "
                                         "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER or VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, "
                                         "pDescriptorWrites[%d].pTexelBufferView must not be NULL",
                                         i, i);
                } else {
                    for (uint32_t descriptor_index = 0; descriptor_index < pDescriptorWrites[i].descriptorCount;
                         ++descriptor_index) {
                        skip_call |= validate_required_handle(report_data, "vkUpdateDescriptorSets",
                                                              ParameterName("pDescriptorWrites[%i].pTexelBufferView[%i]",
                                                                            ParameterName::IndexVector{i, descriptor_index}),
                                                              pDescriptorWrites[i].pTexelBufferView[descriptor_index]);
                    }
                }
            }

            if ((pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ||
                (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)) {
                VkDeviceSize uniformAlignment = device_data->device_limits.minUniformBufferOffsetAlignment;
                for (uint32_t j = 0; j < pDescriptorWrites[i].descriptorCount; j++) {
                    if (pDescriptorWrites[i].pBufferInfo != NULL) {
                        if (vk_safe_modulo(pDescriptorWrites[i].pBufferInfo[j].offset, uniformAlignment) != 0) {
                            skip_call |=
                                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVICE_LIMIT, LayerName,
                                        "vkUpdateDescriptorSets(): pDescriptorWrites[%d].pBufferInfo[%d].offset (0x%" PRIxLEAST64
                                        ") must be a multiple of device limit minUniformBufferOffsetAlignment 0x%" PRIxLEAST64,
                                        i, j, pDescriptorWrites[i].pBufferInfo[j].offset, uniformAlignment);
                        }
                    }
                }
            } else if ((pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) ||
                       (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) {
                VkDeviceSize storageAlignment = device_data->device_limits.minStorageBufferOffsetAlignment;
                for (uint32_t j = 0; j < pDescriptorWrites[i].descriptorCount; j++) {
                    if (pDescriptorWrites[i].pBufferInfo != NULL) {
                        if (vk_safe_modulo(pDescriptorWrites[i].pBufferInfo[j].offset, storageAlignment) != 0) {
                            skip_call |=
                                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVICE_LIMIT, LayerName,
                                        "vkUpdateDescriptorSets(): pDescriptorWrites[%d].pBufferInfo[%d].offset (0x%" PRIxLEAST64
                                        ") must be a multiple of device limit minStorageBufferOffsetAlignment 0x%" PRIxLEAST64,
                                        i, j, pDescriptorWrites[i].pBufferInfo[j].offset, storageAlignment);
                        }
                    }
                }
            }
        }
    }

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)
            ->UpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo *pCreateInfo,
                                                 const VkAllocationCallbacks *pAllocator, VkFramebuffer *pFramebuffer) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCreateFramebuffer(my_data->report_data, pCreateInfo, pAllocator, pFramebuffer);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->CreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);

        validate_result(my_data->report_data, "vkCreateFramebuffer", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroyFramebuffer(my_data->report_data, framebuffer, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->DestroyFramebuffer(device, framebuffer, pAllocator);
    }
}

bool PreCreateRenderPass(layer_data *dev_data, const VkRenderPassCreateInfo *pCreateInfo) {
    bool skip_call = false;
    uint32_t max_color_attachments = dev_data->device_limits.maxColorAttachments;

    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        if (pCreateInfo->pSubpasses[i].colorAttachmentCount > max_color_attachments) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                 __LINE__, DEVICE_LIMIT, "DL", "Cannot create a render pass with %d color attachments. Max is %d.",
                                 pCreateInfo->pSubpasses[i].colorAttachmentCount, max_color_attachments);
        }
    }
    return skip_call;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo,
                                                const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCreateRenderPass(my_data->report_data, pCreateInfo, pAllocator, pRenderPass);
    skip_call |= PreCreateRenderPass(my_data, pCreateInfo);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->CreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);

        validate_result(my_data->report_data, "vkCreateRenderPass", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroyRenderPass(my_data->report_data, renderPass, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->DestroyRenderPass(device, renderPass, pAllocator);
    }
}

VKAPI_ATTR void VKAPI_CALL GetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D *pGranularity) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetRenderAreaGranularity(my_data->report_data, renderPass, pGranularity);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->GetRenderAreaGranularity(device, renderPass, pGranularity);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo,
                                                 const VkAllocationCallbacks *pAllocator, VkCommandPool *pCommandPool) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |=
        validate_queue_family_index(my_data, "vkCreateCommandPool", "pCreateInfo->queueFamilyIndex", pCreateInfo->queueFamilyIndex);

    skip_call |= parameter_validation_vkCreateCommandPool(my_data->report_data, pCreateInfo, pAllocator, pCommandPool);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->CreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);

        validate_result(my_data->report_data, "vkCreateCommandPool", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDestroyCommandPool(my_data->report_data, commandPool, pAllocator);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)->DestroyCommandPool(device, commandPool, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL ResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkResetCommandPool(my_data->report_data, commandPool, flags);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->ResetCommandPool(device, commandPool, flags);

        validate_result(my_data->report_data, "vkResetCommandPool", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL AllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo,
                                                      VkCommandBuffer *pCommandBuffers) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkAllocateCommandBuffers(my_data->report_data, pAllocateInfo, pCommandBuffers);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->AllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);

        validate_result(my_data->report_data, "vkAllocateCommandBuffers", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL FreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount,
                                              const VkCommandBuffer *pCommandBuffers) {
    bool skip_call = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(device_data != nullptr);
    debug_report_data *report_data = device_data->report_data;

    skip_call |= parameter_validation_vkFreeCommandBuffers(report_data, commandPool, commandBufferCount, pCommandBuffers);

    // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
    // This is an array of handles, where the elements are allowed to be VK_NULL_HANDLE, and does not require any validation beyond
    // validate_array()
    skip_call |= validate_array(report_data, "vkFreeCommandBuffers", "commandBufferCount", "pCommandBuffers", commandBufferCount,
                                pCommandBuffers, true, true);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, device)
            ->FreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
    }
}

bool PreBeginCommandBuffer(layer_data *dev_data, VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo) {
    bool skip_call = false;
    layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(dev_data->physical_device), layer_data_map);
    const VkCommandBufferInheritanceInfo *pInfo = pBeginInfo->pInheritanceInfo;

    if (pInfo != NULL) {
        if ((phy_dev_data->physical_device_features.inheritedQueries == VK_FALSE) && (pInfo->occlusionQueryEnable != VK_FALSE)) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        reinterpret_cast<uint64_t>(commandBuffer), __LINE__, DEVICE_FEATURE, LayerName,
                        "Cannot set inherited occlusionQueryEnable in vkBeginCommandBuffer() when device does not support "
                        "inheritedQueries.");
        }

        if ((phy_dev_data->physical_device_features.inheritedQueries != VK_FALSE) && (pInfo->occlusionQueryEnable != VK_FALSE) &&
            (!validate_VkQueryControlFlagBits(VkQueryControlFlagBits(pInfo->queryFlags)))) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        reinterpret_cast<uint64_t>(commandBuffer), __LINE__, DEVICE_FEATURE, LayerName,
                        "Cannot enable in occlusion queries in vkBeginCommandBuffer() and set queryFlags to %d which is not a "
                        "valid combination of VkQueryControlFlagBits.",
                        pInfo->queryFlags);
        }
    }
    return skip_call;
}

VKAPI_ATTR VkResult VKAPI_CALL BeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(device_data != nullptr);
    debug_report_data *report_data = device_data->report_data;

    skip_call |= parameter_validation_vkBeginCommandBuffer(report_data, pBeginInfo);

    // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
    // TODO: pBeginInfo->pInheritanceInfo must not be NULL if commandBuffer is a secondary command buffer
    skip_call |= validate_struct_type(report_data, "vkBeginCommandBuffer", "pBeginInfo->pInheritanceInfo",
                                      "VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO", pBeginInfo->pInheritanceInfo,
                                      VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO, false);

    if (pBeginInfo->pInheritanceInfo != NULL) {
        skip_call |= validate_struct_pnext(report_data, "vkBeginCommandBuffer", "pBeginInfo->pInheritanceInfo->pNext", NULL,
                                           pBeginInfo->pInheritanceInfo->pNext, 0, NULL, GeneratedHeaderVersion);

        skip_call |= validate_bool32(report_data, "vkBeginCommandBuffer", "pBeginInfo->pInheritanceInfo->occlusionQueryEnable",
                                     pBeginInfo->pInheritanceInfo->occlusionQueryEnable);

        // TODO: This only needs to be validated when the inherited queries feature is enabled
        // skip_call |= validate_flags(report_data, "vkBeginCommandBuffer", "pBeginInfo->pInheritanceInfo->queryFlags",
        // "VkQueryControlFlagBits", AllVkQueryControlFlagBits, pBeginInfo->pInheritanceInfo->queryFlags, false);

        // TODO: This must be 0 if the pipeline statistics queries feature is not enabled
        skip_call |= validate_flags(report_data, "vkBeginCommandBuffer", "pBeginInfo->pInheritanceInfo->pipelineStatistics",
                                    "VkQueryPipelineStatisticFlagBits", AllVkQueryPipelineStatisticFlagBits,
                                    pBeginInfo->pInheritanceInfo->pipelineStatistics, false);
    }

    skip_call |= PreBeginCommandBuffer(device_data, commandBuffer, pBeginInfo);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, commandBuffer)->BeginCommandBuffer(commandBuffer, pBeginInfo);

        validate_result(report_data, "vkBeginCommandBuffer", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL EndCommandBuffer(VkCommandBuffer commandBuffer) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    VkResult result = get_dispatch_table(pc_device_table_map, commandBuffer)->EndCommandBuffer(commandBuffer);

    validate_result(my_data->report_data, "vkEndCommandBuffer", result);

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL ResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    bool skip_call = parameter_validation_vkResetCommandBuffer(my_data->report_data, flags);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, commandBuffer)->ResetCommandBuffer(commandBuffer, flags);

        validate_result(my_data->report_data, "vkResetCommandBuffer", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL CmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                                           VkPipeline pipeline) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdBindPipeline(my_data->report_data, pipelineBindPoint, pipeline);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount,
                                          const VkViewport *pViewports) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdSetViewport(my_data->report_data, firstViewport, viewportCount, pViewports);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount,
                                         const VkRect2D *pScissors) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdSetScissor(my_data->report_data, firstScissor, scissorCount, pScissors);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth) {
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdSetLineWidth(commandBuffer, lineWidth);
}

VKAPI_ATTR void VKAPI_CALL CmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp,
                                           float depthBiasSlopeFactor) {
    get_dispatch_table(pc_device_table_map, commandBuffer)
        ->CmdSetDepthBias(commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

VKAPI_ATTR void VKAPI_CALL CmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4]) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdSetBlendConstants(my_data->report_data, blendConstants);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdSetBlendConstants(commandBuffer, blendConstants);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds) {
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdSetDepthBounds(commandBuffer, minDepthBounds, maxDepthBounds);
}

VKAPI_ATTR void VKAPI_CALL CmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask,
                                                    uint32_t compareMask) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdSetStencilCompareMask(my_data->report_data, faceMask, compareMask);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdSetStencilCompareMask(commandBuffer, faceMask, compareMask);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdSetStencilWriteMask(my_data->report_data, faceMask, writeMask);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdSetStencilWriteMask(commandBuffer, faceMask, writeMask);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdSetStencilReference(my_data->report_data, faceMask, reference);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdSetStencilReference(commandBuffer, faceMask, reference);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                                                 VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount,
                                                 const VkDescriptorSet *pDescriptorSets, uint32_t dynamicOffsetCount,
                                                 const uint32_t *pDynamicOffsets) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |=
        parameter_validation_vkCmdBindDescriptorSets(my_data->report_data, pipelineBindPoint, layout, firstSet, descriptorSetCount,
                                                     pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets,
                                    dynamicOffsetCount, pDynamicOffsets);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
                                              VkIndexType indexType) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdBindIndexBuffer(my_data->report_data, buffer, offset, indexType);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount,
                                                const VkBuffer *pBuffers, const VkDeviceSize *pOffsets) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdBindVertexBuffers(my_data->report_data, firstBinding, bindingCount, pBuffers, pOffsets);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
    }
}

bool PreCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                uint32_t firstInstance) {
    if (vertexCount == 0) {
        // TODO: Verify against Valid Usage section. I don't see a non-zero vertexCount listed, may need to add that and make
        // this an error or leave as is.
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                REQUIRED_PARAMETER, LayerName, "vkCmdDraw parameter, uint32_t vertexCount, is 0");
        return false;
    }

    if (instanceCount == 0) {
        // TODO: Verify against Valid Usage section. I don't see a non-zero instanceCount listed, may need to add that and make
        // this an error or leave as is.
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                REQUIRED_PARAMETER, LayerName, "vkCmdDraw parameter, uint32_t instanceCount, is 0");
        return false;
    }

    return true;
}

VKAPI_ATTR void VKAPI_CALL CmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount,
                                   uint32_t firstVertex, uint32_t firstInstance) {
    PreCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);

    get_dispatch_table(pc_device_table_map, commandBuffer)
        ->CmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

VKAPI_ATTR void VKAPI_CALL CmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount,
                                          uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    get_dispatch_table(pc_device_table_map, commandBuffer)
        ->CmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

VKAPI_ATTR void VKAPI_CALL CmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count,
                                           uint32_t stride) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdDrawIndirect(my_data->report_data, buffer, offset, count, stride);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdDrawIndirect(commandBuffer, buffer, offset, count, stride);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
                                                  uint32_t count, uint32_t stride) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdDrawIndexedIndirect(my_data->report_data, buffer, offset, count, stride);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdDrawIndexedIndirect(commandBuffer, buffer, offset, count, stride);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdDispatch(VkCommandBuffer commandBuffer, uint32_t x, uint32_t y, uint32_t z) {
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdDispatch(commandBuffer, x, y, z);
}

VKAPI_ATTR void VKAPI_CALL CmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdDispatchIndirect(my_data->report_data, buffer, offset);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdDispatchIndirect(commandBuffer, buffer, offset);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer,
                                         uint32_t regionCount, const VkBufferCopy *pRegions) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdCopyBuffer(my_data->report_data, srcBuffer, dstBuffer, regionCount, pRegions);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
    }
}

bool PreCmdCopyImage(VkCommandBuffer commandBuffer, const VkImageCopy *pRegions) {
    if (pRegions != nullptr) {
        if ((pRegions->srcSubresource.aspectMask & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT |
                                                    VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0) {
            log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    UNRECOGNIZED_VALUE, LayerName,
                    "vkCmdCopyImage parameter, VkImageAspect pRegions->srcSubresource.aspectMask, is an unrecognized enumerator");
            return false;
        }
        if ((pRegions->dstSubresource.aspectMask & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT |
                                                    VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0) {
            log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    UNRECOGNIZED_VALUE, LayerName,
                    "vkCmdCopyImage parameter, VkImageAspect pRegions->dstSubresource.aspectMask, is an unrecognized enumerator");
            return false;
        }
    }

    return true;
}

VKAPI_ATTR void VKAPI_CALL CmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                        VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
                                        const VkImageCopy *pRegions) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdCopyImage(my_data->report_data, srcImage, srcImageLayout, dstImage, dstImageLayout,
                                                     regionCount, pRegions);

    if (!skip_call) {
        PreCmdCopyImage(commandBuffer, pRegions);

        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
    }
}

bool PreCmdBlitImage(VkCommandBuffer commandBuffer, const VkImageBlit *pRegions) {
    if (pRegions != nullptr) {
        if ((pRegions->srcSubresource.aspectMask & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT |
                                                    VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0) {
            log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    UNRECOGNIZED_VALUE, LayerName,
                    "vkCmdCopyImage parameter, VkImageAspect pRegions->srcSubresource.aspectMask, is an unrecognized enumerator");
            return false;
        }
        if ((pRegions->dstSubresource.aspectMask & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT |
                                                    VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0) {
            log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    UNRECOGNIZED_VALUE, LayerName,
                    "vkCmdCopyImage parameter, VkImageAspect pRegions->dstSubresource.aspectMask, is an unrecognized enumerator");
            return false;
        }
    }

    return true;
}

VKAPI_ATTR void VKAPI_CALL CmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                        VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
                                        const VkImageBlit *pRegions, VkFilter filter) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdBlitImage(my_data->report_data, srcImage, srcImageLayout, dstImage, dstImageLayout,
                                                     regionCount, pRegions, filter);

    if (!skip_call) {
        PreCmdBlitImage(commandBuffer, pRegions);

        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
    }
}

bool PreCmdCopyBufferToImage(VkCommandBuffer commandBuffer, const VkBufferImageCopy *pRegions) {
    if (pRegions != nullptr) {
        if ((pRegions->imageSubresource.aspectMask & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT |
                                                      VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0) {
            log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    UNRECOGNIZED_VALUE, LayerName,
                    "vkCmdCopyBufferToImage parameter, VkImageAspect pRegions->imageSubresource.aspectMask, is an unrecognized "
                    "enumerator");
            return false;
        }
    }

    return true;
}

VKAPI_ATTR void VKAPI_CALL CmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage,
                                                VkImageLayout dstImageLayout, uint32_t regionCount,
                                                const VkBufferImageCopy *pRegions) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdCopyBufferToImage(my_data->report_data, srcBuffer, dstImage, dstImageLayout, regionCount,
                                                             pRegions);

    if (!skip_call) {
        PreCmdCopyBufferToImage(commandBuffer, pRegions);

        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
    }
}

bool PreCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, const VkBufferImageCopy *pRegions) {
    if (pRegions != nullptr) {
        if ((pRegions->imageSubresource.aspectMask & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT |
                                                      VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0) {
            log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    UNRECOGNIZED_VALUE, LayerName,
                    "vkCmdCopyImageToBuffer parameter, VkImageAspect pRegions->imageSubresource.aspectMask, is an unrecognized "
                    "enumerator");
            return false;
        }
    }

    return true;
}

VKAPI_ATTR void VKAPI_CALL CmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                                VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy *pRegions) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdCopyImageToBuffer(my_data->report_data, srcImage, srcImageLayout, dstBuffer, regionCount,
                                                             pRegions);

    if (!skip_call) {
        PreCmdCopyImageToBuffer(commandBuffer, pRegions);

        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset,
                                           VkDeviceSize dataSize, const uint32_t *pData) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdUpdateBuffer(my_data->report_data, dstBuffer, dstOffset, dataSize, pData);

    if (dstOffset & 3) {
        skip_call |= log_msg(
            my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__, INVALID_USAGE,
            LayerName, "CmdUpdateBuffer parameter, VkDeviceSize dstOffset (0x%" PRIxLEAST64 "), is not a multiple of 4", dstOffset);
    }

    if ((dataSize <= 0) || (dataSize > 65536)) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                             INVALID_USAGE, LayerName, "CmdUpdateBuffer parameter, VkDeviceSize dataSize (0x%" PRIxLEAST64
                                                       "), must be greater than zero and less than or equal to 65536",
                             dataSize);
    } else if (dataSize & 3) {
        skip_call |= log_msg(
            my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__, INVALID_USAGE,
            LayerName, "CmdUpdateBuffer parameter, VkDeviceSize dataSize (0x%" PRIxLEAST64 "), is not a multiple of 4", dataSize);
    }

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset,
                                         VkDeviceSize size, uint32_t data) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdFillBuffer(my_data->report_data, dstBuffer, dstOffset, size, data);

    if (dstOffset & 3) {
        skip_call |= log_msg(
            my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__, INVALID_USAGE,
            LayerName, "vkCmdFillBuffer parameter, VkDeviceSize dstOffset (0x%" PRIxLEAST64 "), is not a multiple of 4", dstOffset);
    }

    if (size != VK_WHOLE_SIZE) {
        if (size <= 0) {
            skip_call |= log_msg(
                my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__, INVALID_USAGE,
                LayerName, "vkCmdFillBuffer parameter, VkDeviceSize size (0x%" PRIxLEAST64 "), must be greater than zero", size);
        } else if (size & 3) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                                 INVALID_USAGE, LayerName,
                                 "vkCmdFillBuffer parameter, VkDeviceSize size (0x%" PRIxLEAST64 "), is not a multiple of 4", size);
        }
    }

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                                              const VkClearColorValue *pColor, uint32_t rangeCount,
                                              const VkImageSubresourceRange *pRanges) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdClearColorImage(my_data->report_data, image, imageLayout, pColor, rangeCount, pRanges);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                                                     const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount,
                                                     const VkImageSubresourceRange *pRanges) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdClearDepthStencilImage(my_data->report_data, image, imageLayout, pDepthStencil,
                                                                  rangeCount, pRanges);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount,
                                               const VkClearAttachment *pAttachments, uint32_t rectCount,
                                               const VkClearRect *pRects) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdClearAttachments(my_data->report_data, attachmentCount, pAttachments, rectCount, pRects);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
    }
}

bool PreCmdResolveImage(VkCommandBuffer commandBuffer, const VkImageResolve *pRegions) {
    if (pRegions != nullptr) {
        if ((pRegions->srcSubresource.aspectMask & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT |
                                                    VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0) {
            log_msg(
                mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                UNRECOGNIZED_VALUE, LayerName,
                "vkCmdResolveImage parameter, VkImageAspect pRegions->srcSubresource.aspectMask, is an unrecognized enumerator");
            return false;
        }
        if ((pRegions->dstSubresource.aspectMask & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT |
                                                    VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0) {
            log_msg(
                mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                UNRECOGNIZED_VALUE, LayerName,
                "vkCmdResolveImage parameter, VkImageAspect pRegions->dstSubresource.aspectMask, is an unrecognized enumerator");
            return false;
        }
    }

    return true;
}

VKAPI_ATTR void VKAPI_CALL CmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                           VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
                                           const VkImageResolve *pRegions) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdResolveImage(my_data->report_data, srcImage, srcImageLayout, dstImage, dstImageLayout,
                                                        regionCount, pRegions);

    if (!skip_call) {
        PreCmdResolveImage(commandBuffer, pRegions);

        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdSetEvent(my_data->report_data, event, stageMask);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdSetEvent(commandBuffer, event, stageMask);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdResetEvent(my_data->report_data, event, stageMask);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdResetEvent(commandBuffer, event, stageMask);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents,
                                         VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                                         uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
                                         uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers,
                                         uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdWaitEvents(my_data->report_data, eventCount, pEvents, srcStageMask, dstStageMask,
                                                      memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount,
                                                      pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdWaitEvents(commandBuffer, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers,
                            bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
                                              VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
                                              uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
                                              uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers,
                                              uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdPipelineBarrier(my_data->report_data, srcStageMask, dstStageMask, dependencyFlags,
                                                           memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount,
                                                           pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers,
                                 bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t slot,
                                         VkQueryControlFlags flags) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdBeginQuery(my_data->report_data, queryPool, slot, flags);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdBeginQuery(commandBuffer, queryPool, slot, flags);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t slot) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdEndQuery(my_data->report_data, queryPool, slot);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdEndQuery(commandBuffer, queryPool, slot);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery,
                                             uint32_t queryCount) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdResetQueryPool(my_data->report_data, queryPool, firstQuery, queryCount);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdResetQueryPool(commandBuffer, queryPool, firstQuery, queryCount);
    }
}

bool PostCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool,
                           uint32_t slot) {

    ValidateEnumerator(pipelineStage);

    return true;
}

VKAPI_ATTR void VKAPI_CALL CmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage,
                                             VkQueryPool queryPool, uint32_t query) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdWriteTimestamp(my_data->report_data, pipelineStage, queryPool, query);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, query);

        PostCmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, query);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery,
                                                   uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset,
                                                   VkDeviceSize stride, VkQueryResultFlags flags) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdCopyQueryPoolResults(my_data->report_data, queryPool, firstQuery, queryCount, dstBuffer,
                                                                dstOffset, stride, flags);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdCopyQueryPoolResults(commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags,
                                            uint32_t offset, uint32_t size, const void *pValues) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdPushConstants(my_data->report_data, layout, stageFlags, offset, size, pValues);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdPushConstants(commandBuffer, layout, stageFlags, offset, size, pValues);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin,
                                              VkSubpassContents contents) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdBeginRenderPass(my_data->report_data, pRenderPassBegin, contents);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdNextSubpass(my_data->report_data, contents);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdNextSubpass(commandBuffer, contents);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdEndRenderPass(VkCommandBuffer commandBuffer) {
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdEndRenderPass(commandBuffer);
}

VKAPI_ATTR void VKAPI_CALL CmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount,
                                              const VkCommandBuffer *pCommandBuffers) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdExecuteCommands(my_data->report_data, commandBufferCount, pCommandBuffers);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)
            ->CmdExecuteCommands(commandBuffer, commandBufferCount, pCommandBuffers);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &global_layer, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
                                                              VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &global_layer, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,
                                                                    VkExtensionProperties *pProperties) {
    if (pLayerName && !strcmp(pLayerName, global_layer.layerName))
        return util_GetExtensionProperties(1, instance_extensions, pCount, pProperties);

    return VK_ERROR_LAYER_NOT_PRESENT;
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName,
                                                                  uint32_t *pCount, VkExtensionProperties *pProperties) {
    /* parameter_validation does not have any physical device extensions */
    if (pLayerName && !strcmp(pLayerName, global_layer.layerName))
        return util_GetExtensionProperties(0, NULL, pCount, pProperties);

    assert(physicalDevice);

    return get_dispatch_table(pc_instance_table_map, physicalDevice)
        ->EnumerateDeviceExtensionProperties(physicalDevice, NULL, pCount, pProperties);
}

// WSI Extension Functions

VKAPI_ATTR VkResult VKAPI_CALL CreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo,
                                                  const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCreateSwapchainKHR(my_data->report_data, pCreateInfo, pAllocator, pSwapchain);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);

        validate_result(my_data->report_data, "vkCreateSwapchainKHR", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL GetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount,
                                                     VkImage *pSwapchainImages) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |=
        parameter_validation_vkGetSwapchainImagesKHR(my_data->report_data, swapchain, pSwapchainImageCount, pSwapchainImages);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)
                     ->GetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);

        validate_result(my_data->report_data, "vkGetSwapchainImagesKHR", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL AcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout,
                                                   VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |=
        parameter_validation_vkAcquireNextImageKHR(my_data->report_data, swapchain, timeout, semaphore, fence, pImageIndex);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)
                     ->AcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);

        validate_result(my_data->report_data, "vkAcquireNextImageKHR", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkQueuePresentKHR(my_data->report_data, pPresentInfo);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, queue)->QueuePresentKHR(queue, pPresentInfo);

        validate_result(my_data->report_data, "vkQueuePresentKHR", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex,
                                                                  VkSurfaceKHR surface, VkBool32 *pSupported) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    skip_call |=
        parameter_validation_vkGetPhysicalDeviceSurfaceSupportKHR(my_data->report_data, queueFamilyIndex, surface, pSupported);

    if (!skip_call) {
        result = get_dispatch_table(pc_instance_table_map, physicalDevice)
                     ->GetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, pSupported);

        validate_result(my_data->report_data, "vkGetPhysicalDeviceSurfaceSupportKHR", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                                                       VkSurfaceCapabilitiesKHR *pSurfaceCapabilities) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    skip_call |=
        parameter_validation_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(my_data->report_data, surface, pSurfaceCapabilities);

    if (!skip_call) {
        result = get_dispatch_table(pc_instance_table_map, physicalDevice)
                     ->GetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, pSurfaceCapabilities);

        validate_result(my_data->report_data, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                                                  uint32_t *pSurfaceFormatCount,
                                                                  VkSurfaceFormatKHR *pSurfaceFormats) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetPhysicalDeviceSurfaceFormatsKHR(my_data->report_data, surface, pSurfaceFormatCount,
                                                                           pSurfaceFormats);

    if (!skip_call) {
        result = get_dispatch_table(pc_instance_table_map, physicalDevice)
                     ->GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);

        validate_result(my_data->report_data, "vkGetPhysicalDeviceSurfaceFormatsKHR", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                                                       uint32_t *pPresentModeCount,
                                                                       VkPresentModeKHR *pPresentModes) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetPhysicalDeviceSurfacePresentModesKHR(my_data->report_data, surface, pPresentModeCount,
                                                                                pPresentModes);

    if (!skip_call) {
        result = get_dispatch_table(pc_instance_table_map, physicalDevice)
                     ->GetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, pPresentModeCount, pPresentModes);

        validate_result(my_data->report_data, "vkGetPhysicalDeviceSurfacePresentModesKHR", result);
    }

    return result;
}

#ifdef VK_USE_PLATFORM_WIN32_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo,
                                                     const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    assert(my_data != NULL);

    bool skip_call = parameter_validation_vkCreateWin32SurfaceKHR(my_data->report_data, pCreateInfo, pAllocator, pSurface);

    if (!skip_call) {
        result =
            get_dispatch_table(pc_instance_table_map, instance)->CreateWin32SurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    }

    validate_result(my_data->report_data, "vkCreateWin32SurfaceKHR", result);

    return result;
}
#endif // VK_USE_PLATFORM_WIN32_KHR

#ifdef VK_USE_PLATFORM_XCB_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateXcbSurfaceKHR(VkInstance instance, const VkXcbSurfaceCreateInfoKHR *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    assert(my_data != NULL);

    bool skip_call = parameter_validation_vkCreateXcbSurfaceKHR(my_data->report_data, pCreateInfo, pAllocator, pSurface);

    if (!skip_call) {
        result =
            get_dispatch_table(pc_instance_table_map, instance)->CreateXcbSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    }

    validate_result(my_data->report_data, "vkCreateXcbSurfaceKHR", result);

    return result;
}

VKAPI_ATTR VkBool32 VKAPI_CALL GetPhysicalDeviceXcbPresentationSupportKHR(VkPhysicalDevice physicalDevice,
                                                                          uint32_t queueFamilyIndex, xcb_connection_t *connection,
                                                                          xcb_visualid_t visual_id) {
    VkBool32 result = false;

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    bool skip_call = parameter_validation_vkGetPhysicalDeviceXcbPresentationSupportKHR(my_data->report_data, queueFamilyIndex,
                                                                                       connection, visual_id);

    if (!skip_call) {
        result = get_dispatch_table(pc_instance_table_map, physicalDevice)
                     ->GetPhysicalDeviceXcbPresentationSupportKHR(physicalDevice, queueFamilyIndex, connection, visual_id);
    }

    return result;
}
#endif // VK_USE_PLATFORM_XCB_KHR

#ifdef VK_USE_PLATFORM_XLIB_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateXlibSurfaceKHR(VkInstance instance, const VkXlibSurfaceCreateInfoKHR *pCreateInfo,
                                                    const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    assert(my_data != NULL);

    bool skip_call = parameter_validation_vkCreateXlibSurfaceKHR(my_data->report_data, pCreateInfo, pAllocator, pSurface);

    if (!skip_call) {
        result =
            get_dispatch_table(pc_instance_table_map, instance)->CreateXlibSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    }

    validate_result(my_data->report_data, "vkCreateXlibSurfaceKHR", result);

    return result;
}

VKAPI_ATTR VkBool32 VKAPI_CALL GetPhysicalDeviceXlibPresentationSupportKHR(VkPhysicalDevice physicalDevice,
                                                                           uint32_t queueFamilyIndex, Display *dpy,
                                                                           VisualID visualID) {
    VkBool32 result = false;

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    bool skip_call =
        parameter_validation_vkGetPhysicalDeviceXlibPresentationSupportKHR(my_data->report_data, queueFamilyIndex, dpy, visualID);

    if (!skip_call) {
        result = get_dispatch_table(pc_instance_table_map, physicalDevice)
                     ->GetPhysicalDeviceXlibPresentationSupportKHR(physicalDevice, queueFamilyIndex, dpy, visualID);
    }
    return result;
}
#endif // VK_USE_PLATFORM_XLIB_KHR

#ifdef VK_USE_PLATFORM_MIR_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateMirSurfaceKHR(VkInstance instance, const VkMirSurfaceCreateInfoKHR *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    assert(my_data != NULL);

    bool skip_call = parameter_validation_vkCreateMirSurfaceKHR(my_data->report_data, pCreateInfo, pAllocator, pSurface);

    if (!skip_call) {
        result =
            get_dispatch_table(pc_instance_table_map, instance)->CreateMirSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    }

    validate_result(my_data->report_data, "vkCreateMirSurfaceKHR", result);

    return result;
}

VKAPI_ATTR VkBool32 VKAPI_CALL GetPhysicalDeviceMirPresentationSupportKHR(VkPhysicalDevice physicalDevice,
                                                                          uint32_t queueFamilyIndex, MirConnection *connection) {
    VkBool32 result = false;

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    bool skip_call =
        parameter_validation_vkGetPhysicalDeviceMirPresentationSupportKHR(my_data->report_data, queueFamilyIndex, connection);

    if (!skip_call) {
        result = get_dispatch_table(pc_instance_table_map, physicalDevice)
                     ->GetPhysicalDeviceMirPresentationSupportKHR(physicalDevice, queueFamilyIndex, connection);
    }
}
#endif // VK_USE_PLATFORM_MIR_KHR

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateWaylandSurfaceKHR(VkInstance instance, const VkWaylandSurfaceCreateInfoKHR *pCreateInfo,
                                                       const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    assert(my_data != NULL);

    bool skip_call = parameter_validation_vkCreateWaylandSurfaceKHR(my_data->report_data, pCreateInfo, pAllocator, pSurface);

    if (!skip_call) {
        result = get_dispatch_table(pc_instance_table_map, instance)
                     ->CreateWaylandSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    }

    validate_result(my_data->report_data, "vkCreateWaylandSurfaceKHR", result);

    return result;
}

VKAPI_ATTR VkBool32 VKAPI_CALL GetPhysicalDeviceWaylandPresentationSupportKHR(VkPhysicalDevice physicalDevice,
                                                                              uint32_t queueFamilyIndex,
                                                                              struct wl_display *display) {
    VkBool32 result = false;

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    bool skip_call =
        parameter_validation_vkGetPhysicalDeviceWaylandPresentationSupportKHR(my_data->report_data, queueFamilyIndex, display);

    if (!skip_call) {
        result = get_dispatch_table(pc_instance_table_map, physicalDevice)
                     ->GetPhysicalDeviceWaylandPresentationSupportKHR(physicalDevice, queueFamilyIndex, display);
    }
}
#endif // VK_USE_PLATFORM_WAYLAND_KHR

#ifdef VK_USE_PLATFORM_ANDROID_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateAndroidSurfaceKHR(VkInstance instance, const VkAndroidSurfaceCreateInfoKHR *pCreateInfo,
                                                       const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    assert(my_data != NULL);

    bool skip_call = parameter_validation_vkCreateAndroidSurfaceKHR(my_data->report_data, pCreateInfo, pAllocator, pSurface);

    if (!skip_call) {
        result = get_dispatch_table(pc_instance_table_map, instance)
                     ->CreateAndroidSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    }

    validate_result(my_data->report_data, "vkCreateAndroidSurfaceKHR", result);

    return result;
}
#endif // VK_USE_PLATFORM_ANDROID_KHR

VKAPI_ATTR VkResult VKAPI_CALL CreateSharedSwapchainsKHR(VkDevice device, uint32_t swapchainCount,
                                                         const VkSwapchainCreateInfoKHR *pCreateInfos,
                                                         const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchains) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCreateSharedSwapchainsKHR(my_data->report_data, swapchainCount, pCreateInfos, pAllocator,
                                                                  pSwapchains);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)
                     ->CreateSharedSwapchainsKHR(device, swapchainCount, pCreateInfos, pAllocator, pSwapchains);

        validate_result(my_data->report_data, "vkCreateSharedSwapchainsKHR", result);
    }

    return result;
}

// VK_EXT_debug_marker Extension
VKAPI_ATTR VkResult VKAPI_CALL DebugMarkerSetObjectTagEXT(VkDevice device, VkDebugMarkerObjectTagInfoEXT *pTagInfo) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDebugMarkerSetObjectTagEXT(my_data->report_data, pTagInfo);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->DebugMarkerSetObjectTagEXT(device, pTagInfo);

        validate_result(my_data->report_data, "vkDebugMarkerSetObjectTagEXT", result);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL DebugMarkerSetObjectNameEXT(VkDevice device, VkDebugMarkerObjectNameInfoEXT *pNameInfo) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkDebugMarkerSetObjectNameEXT(my_data->report_data, pNameInfo);

    if (!skip_call) {
        VkResult result = get_dispatch_table(pc_device_table_map, device)->DebugMarkerSetObjectNameEXT(device, pNameInfo);

        validate_result(my_data->report_data, "vkDebugMarkerSetObjectNameEXT", result);
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL CmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, VkDebugMarkerMarkerInfoEXT *pMarkerInfo) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdDebugMarkerBeginEXT(my_data->report_data, pMarkerInfo);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdDebugMarkerBeginEXT(commandBuffer, pMarkerInfo);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, VkDebugMarkerMarkerInfoEXT *pMarkerInfo) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkCmdDebugMarkerInsertEXT(my_data->report_data, pMarkerInfo);

    if (!skip_call) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdDebugMarkerInsertEXT(commandBuffer, pMarkerInfo);
    }
}

// VK_NV_external_memory_capabilities Extension
VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceExternalImageFormatPropertiesNV(
    VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage,
    VkImageCreateFlags flags, VkExternalMemoryHandleTypeFlagsNV externalHandleType,
    VkExternalImageFormatPropertiesNV *pExternalImageFormatProperties) {

    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetPhysicalDeviceExternalImageFormatPropertiesNV(
        my_data->report_data, format, type, tiling, usage, flags, externalHandleType, pExternalImageFormatProperties);

    if (!skip_call) {
        result = get_dispatch_table(pc_instance_table_map, physicalDevice)
                     ->GetPhysicalDeviceExternalImageFormatPropertiesNV(physicalDevice, format, type, tiling, usage, flags,
                                                                        externalHandleType, pExternalImageFormatProperties);

        validate_result(my_data->report_data, "vkGetPhysicalDeviceExternalImageFormatPropertiesNV", result);
    }

    return result;
}

#ifdef VK_USE_PLATFORM_WIN32_KHR
// VK_NV_external_memory_win32 Extension
VKAPI_ATTR VkResult VKAPI_CALL GetMemoryWin32HandleNV(VkDevice device, VkDeviceMemory memory,
                                                      VkExternalMemoryHandleTypeFlagsNV handleType, HANDLE *pHandle) {

    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skip_call |= parameter_validation_vkGetMemoryWin32HandleNV(my_data->report_data, memory, handleType, pHandle);

    if (!skip_call) {
        result = get_dispatch_table(pc_device_table_map, device)->GetMemoryWin32HandleNV(device, memory, handleType, pHandle);
    }

    return result;
}
#endif // VK_USE_PLATFORM_WIN32_KHR



static PFN_vkVoidFunction intercept_core_instance_command(const char *name);

static PFN_vkVoidFunction intercept_core_device_command(const char *name);

static PFN_vkVoidFunction InterceptWsiEnabledCommand(const char *name, VkDevice device);

static PFN_vkVoidFunction InterceptWsiEnabledCommand(const char *name, VkInstance instance);

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(VkDevice device, const char *funcName) {
    assert(device);

    layer_data *data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    if (validate_string(data->report_data, "vkGetDeviceProcAddr", "funcName", funcName)) {
        return NULL;
    }

    PFN_vkVoidFunction proc = intercept_core_device_command(funcName);
    if (proc)
        return proc;

    proc = InterceptWsiEnabledCommand(funcName, device);
    if (proc)
        return proc;

    if (get_dispatch_table(pc_device_table_map, device)->GetDeviceProcAddr == NULL)
        return NULL;
    return get_dispatch_table(pc_device_table_map, device)->GetDeviceProcAddr(device, funcName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char *funcName) {
    PFN_vkVoidFunction proc = intercept_core_instance_command(funcName);
    if (!proc)
        proc = intercept_core_device_command(funcName);

    if (!proc)
        proc = InterceptWsiEnabledCommand(funcName, VkDevice(VK_NULL_HANDLE));

    if (proc)
        return proc;

    assert(instance);

    layer_data *data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    proc = debug_report_get_instance_proc_addr(data->report_data, funcName);
    if (!proc)
        proc = InterceptWsiEnabledCommand(funcName, instance);

    if (proc)
        return proc;

    if (get_dispatch_table(pc_instance_table_map, instance)->GetInstanceProcAddr == NULL)
        return NULL;
    return get_dispatch_table(pc_instance_table_map, instance)->GetInstanceProcAddr(instance, funcName);
}

static PFN_vkVoidFunction intercept_core_instance_command(const char *name) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } core_instance_commands[] = {
        {"vkGetInstanceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetInstanceProcAddr)},
        {"vkCreateInstance", reinterpret_cast<PFN_vkVoidFunction>(CreateInstance)},
        {"vkDestroyInstance", reinterpret_cast<PFN_vkVoidFunction>(DestroyInstance)},
        {"vkCreateDevice", reinterpret_cast<PFN_vkVoidFunction>(CreateDevice)},
        {"vkEnumeratePhysicalDevices", reinterpret_cast<PFN_vkVoidFunction>(EnumeratePhysicalDevices)},
        {"vkGetPhysicalDeviceProperties", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceProperties)},
        {"vkGetPhysicalDeviceFeatures", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceFeatures)},
        {"vkGetPhysicalDeviceFormatProperties", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceFormatProperties)},
        {"vkGetPhysicalDeviceImageFormatProperties", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceImageFormatProperties)},
        {"vkGetPhysicalDeviceSparseImageFormatProperties",
         reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceSparseImageFormatProperties)},
        {"vkGetPhysicalDeviceQueueFamilyProperties", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceQueueFamilyProperties)},
        {"vkGetPhysicalDeviceMemoryProperties", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceMemoryProperties)},
        {"vkEnumerateInstanceLayerProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceLayerProperties)},
        {"vkEnumerateDeviceLayerProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateDeviceLayerProperties)},
        {"vkEnumerateInstanceExtensionProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceExtensionProperties)},
        {"vkEnumerateDeviceExtensionProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateDeviceExtensionProperties)},
        {"vkGetPhysicalDeviceExternalImageFormatPropertiesNV", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceExternalImageFormatPropertiesNV) },
    };

    for (size_t i = 0; i < ARRAY_SIZE(core_instance_commands); i++) {
        if (!strcmp(core_instance_commands[i].name, name))
            return core_instance_commands[i].proc;
    }

    return nullptr;
}

static PFN_vkVoidFunction intercept_core_device_command(const char *name) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } core_device_commands[] = {
        {"vkGetDeviceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceProcAddr)},
        {"vkDestroyDevice", reinterpret_cast<PFN_vkVoidFunction>(DestroyDevice)},
        {"vkGetDeviceQueue", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceQueue)},
        {"vkQueueSubmit", reinterpret_cast<PFN_vkVoidFunction>(QueueSubmit)},
        {"vkQueueWaitIdle", reinterpret_cast<PFN_vkVoidFunction>(QueueWaitIdle)},
        {"vkDeviceWaitIdle", reinterpret_cast<PFN_vkVoidFunction>(DeviceWaitIdle)},
        {"vkAllocateMemory", reinterpret_cast<PFN_vkVoidFunction>(AllocateMemory)},
        {"vkFreeMemory", reinterpret_cast<PFN_vkVoidFunction>(FreeMemory)},
        {"vkMapMemory", reinterpret_cast<PFN_vkVoidFunction>(MapMemory)},
        {"vkUnmapMemory", reinterpret_cast<PFN_vkVoidFunction>(UnmapMemory)},
        {"vkFlushMappedMemoryRanges", reinterpret_cast<PFN_vkVoidFunction>(FlushMappedMemoryRanges)},
        {"vkInvalidateMappedMemoryRanges", reinterpret_cast<PFN_vkVoidFunction>(InvalidateMappedMemoryRanges)},
        {"vkGetDeviceMemoryCommitment", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceMemoryCommitment)},
        {"vkBindBufferMemory", reinterpret_cast<PFN_vkVoidFunction>(BindBufferMemory)},
        {"vkBindImageMemory", reinterpret_cast<PFN_vkVoidFunction>(BindImageMemory)},
        {"vkCreateFence", reinterpret_cast<PFN_vkVoidFunction>(CreateFence)},
        {"vkDestroyFence", reinterpret_cast<PFN_vkVoidFunction>(DestroyFence)},
        {"vkResetFences", reinterpret_cast<PFN_vkVoidFunction>(ResetFences)},
        {"vkGetFenceStatus", reinterpret_cast<PFN_vkVoidFunction>(GetFenceStatus)},
        {"vkWaitForFences", reinterpret_cast<PFN_vkVoidFunction>(WaitForFences)},
        {"vkCreateSemaphore", reinterpret_cast<PFN_vkVoidFunction>(CreateSemaphore)},
        {"vkDestroySemaphore", reinterpret_cast<PFN_vkVoidFunction>(DestroySemaphore)},
        {"vkCreateEvent", reinterpret_cast<PFN_vkVoidFunction>(CreateEvent)},
        {"vkDestroyEvent", reinterpret_cast<PFN_vkVoidFunction>(DestroyEvent)},
        {"vkGetEventStatus", reinterpret_cast<PFN_vkVoidFunction>(GetEventStatus)},
        {"vkSetEvent", reinterpret_cast<PFN_vkVoidFunction>(SetEvent)},
        {"vkResetEvent", reinterpret_cast<PFN_vkVoidFunction>(ResetEvent)},
        {"vkCreateQueryPool", reinterpret_cast<PFN_vkVoidFunction>(CreateQueryPool)},
        {"vkDestroyQueryPool", reinterpret_cast<PFN_vkVoidFunction>(DestroyQueryPool)},
        {"vkGetQueryPoolResults", reinterpret_cast<PFN_vkVoidFunction>(GetQueryPoolResults)},
        {"vkCreateBuffer", reinterpret_cast<PFN_vkVoidFunction>(CreateBuffer)},
        {"vkDestroyBuffer", reinterpret_cast<PFN_vkVoidFunction>(DestroyBuffer)},
        {"vkCreateBufferView", reinterpret_cast<PFN_vkVoidFunction>(CreateBufferView)},
        {"vkDestroyBufferView", reinterpret_cast<PFN_vkVoidFunction>(DestroyBufferView)},
        {"vkCreateImage", reinterpret_cast<PFN_vkVoidFunction>(CreateImage)},
        {"vkDestroyImage", reinterpret_cast<PFN_vkVoidFunction>(DestroyImage)},
        {"vkGetImageSubresourceLayout", reinterpret_cast<PFN_vkVoidFunction>(GetImageSubresourceLayout)},
        {"vkCreateImageView", reinterpret_cast<PFN_vkVoidFunction>(CreateImageView)},
        {"vkDestroyImageView", reinterpret_cast<PFN_vkVoidFunction>(DestroyImageView)},
        {"vkCreateShaderModule", reinterpret_cast<PFN_vkVoidFunction>(CreateShaderModule)},
        {"vkDestroyShaderModule", reinterpret_cast<PFN_vkVoidFunction>(DestroyShaderModule)},
        {"vkCreatePipelineCache", reinterpret_cast<PFN_vkVoidFunction>(CreatePipelineCache)},
        {"vkDestroyPipelineCache", reinterpret_cast<PFN_vkVoidFunction>(DestroyPipelineCache)},
        {"vkGetPipelineCacheData", reinterpret_cast<PFN_vkVoidFunction>(GetPipelineCacheData)},
        {"vkMergePipelineCaches", reinterpret_cast<PFN_vkVoidFunction>(MergePipelineCaches)},
        {"vkCreateGraphicsPipelines", reinterpret_cast<PFN_vkVoidFunction>(CreateGraphicsPipelines)},
        {"vkCreateComputePipelines", reinterpret_cast<PFN_vkVoidFunction>(CreateComputePipelines)},
        {"vkDestroyPipeline", reinterpret_cast<PFN_vkVoidFunction>(DestroyPipeline)},
        {"vkCreatePipelineLayout", reinterpret_cast<PFN_vkVoidFunction>(CreatePipelineLayout)},
        {"vkDestroyPipelineLayout", reinterpret_cast<PFN_vkVoidFunction>(DestroyPipelineLayout)},
        {"vkCreateSampler", reinterpret_cast<PFN_vkVoidFunction>(CreateSampler)},
        {"vkDestroySampler", reinterpret_cast<PFN_vkVoidFunction>(DestroySampler)},
        {"vkCreateDescriptorSetLayout", reinterpret_cast<PFN_vkVoidFunction>(CreateDescriptorSetLayout)},
        {"vkDestroyDescriptorSetLayout", reinterpret_cast<PFN_vkVoidFunction>(DestroyDescriptorSetLayout)},
        {"vkCreateDescriptorPool", reinterpret_cast<PFN_vkVoidFunction>(CreateDescriptorPool)},
        {"vkDestroyDescriptorPool", reinterpret_cast<PFN_vkVoidFunction>(DestroyDescriptorPool)},
        {"vkResetDescriptorPool", reinterpret_cast<PFN_vkVoidFunction>(ResetDescriptorPool)},
        {"vkAllocateDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(AllocateDescriptorSets)},
        {"vkFreeDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(FreeDescriptorSets)},
        {"vkUpdateDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(UpdateDescriptorSets)},
        {"vkCmdSetViewport", reinterpret_cast<PFN_vkVoidFunction>(CmdSetViewport)},
        {"vkCmdSetScissor", reinterpret_cast<PFN_vkVoidFunction>(CmdSetScissor)},
        {"vkCmdSetLineWidth", reinterpret_cast<PFN_vkVoidFunction>(CmdSetLineWidth)},
        {"vkCmdSetDepthBias", reinterpret_cast<PFN_vkVoidFunction>(CmdSetDepthBias)},
        {"vkCmdSetBlendConstants", reinterpret_cast<PFN_vkVoidFunction>(CmdSetBlendConstants)},
        {"vkCmdSetDepthBounds", reinterpret_cast<PFN_vkVoidFunction>(CmdSetDepthBounds)},
        {"vkCmdSetStencilCompareMask", reinterpret_cast<PFN_vkVoidFunction>(CmdSetStencilCompareMask)},
        {"vkCmdSetStencilWriteMask", reinterpret_cast<PFN_vkVoidFunction>(CmdSetStencilWriteMask)},
        {"vkCmdSetStencilReference", reinterpret_cast<PFN_vkVoidFunction>(CmdSetStencilReference)},
        {"vkAllocateCommandBuffers", reinterpret_cast<PFN_vkVoidFunction>(AllocateCommandBuffers)},
        {"vkFreeCommandBuffers", reinterpret_cast<PFN_vkVoidFunction>(FreeCommandBuffers)},
        {"vkBeginCommandBuffer", reinterpret_cast<PFN_vkVoidFunction>(BeginCommandBuffer)},
        {"vkEndCommandBuffer", reinterpret_cast<PFN_vkVoidFunction>(EndCommandBuffer)},
        {"vkResetCommandBuffer", reinterpret_cast<PFN_vkVoidFunction>(ResetCommandBuffer)},
        {"vkCmdBindPipeline", reinterpret_cast<PFN_vkVoidFunction>(CmdBindPipeline)},
        {"vkCmdBindDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(CmdBindDescriptorSets)},
        {"vkCmdBindVertexBuffers", reinterpret_cast<PFN_vkVoidFunction>(CmdBindVertexBuffers)},
        {"vkCmdBindIndexBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdBindIndexBuffer)},
        {"vkCmdDraw", reinterpret_cast<PFN_vkVoidFunction>(CmdDraw)},
        {"vkCmdDrawIndexed", reinterpret_cast<PFN_vkVoidFunction>(CmdDrawIndexed)},
        {"vkCmdDrawIndirect", reinterpret_cast<PFN_vkVoidFunction>(CmdDrawIndirect)},
        {"vkCmdDrawIndexedIndirect", reinterpret_cast<PFN_vkVoidFunction>(CmdDrawIndexedIndirect)},
        {"vkCmdDispatch", reinterpret_cast<PFN_vkVoidFunction>(CmdDispatch)},
        {"vkCmdDispatchIndirect", reinterpret_cast<PFN_vkVoidFunction>(CmdDispatchIndirect)},
        {"vkCmdCopyBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyBuffer)},
        {"vkCmdCopyImage", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyImage)},
        {"vkCmdBlitImage", reinterpret_cast<PFN_vkVoidFunction>(CmdBlitImage)},
        {"vkCmdCopyBufferToImage", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyBufferToImage)},
        {"vkCmdCopyImageToBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyImageToBuffer)},
        {"vkCmdUpdateBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdUpdateBuffer)},
        {"vkCmdFillBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdFillBuffer)},
        {"vkCmdClearColorImage", reinterpret_cast<PFN_vkVoidFunction>(CmdClearColorImage)},
        {"vkCmdClearDepthStencilImage", reinterpret_cast<PFN_vkVoidFunction>(CmdClearDepthStencilImage)},
        {"vkCmdClearAttachments", reinterpret_cast<PFN_vkVoidFunction>(CmdClearAttachments)},
        {"vkCmdResolveImage", reinterpret_cast<PFN_vkVoidFunction>(CmdResolveImage)},
        {"vkCmdSetEvent", reinterpret_cast<PFN_vkVoidFunction>(CmdSetEvent)},
        {"vkCmdResetEvent", reinterpret_cast<PFN_vkVoidFunction>(CmdResetEvent)},
        {"vkCmdWaitEvents", reinterpret_cast<PFN_vkVoidFunction>(CmdWaitEvents)},
        {"vkCmdPipelineBarrier", reinterpret_cast<PFN_vkVoidFunction>(CmdPipelineBarrier)},
        {"vkCmdBeginQuery", reinterpret_cast<PFN_vkVoidFunction>(CmdBeginQuery)},
        {"vkCmdEndQuery", reinterpret_cast<PFN_vkVoidFunction>(CmdEndQuery)},
        {"vkCmdResetQueryPool", reinterpret_cast<PFN_vkVoidFunction>(CmdResetQueryPool)},
        {"vkCmdWriteTimestamp", reinterpret_cast<PFN_vkVoidFunction>(CmdWriteTimestamp)},
        {"vkCmdCopyQueryPoolResults", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyQueryPoolResults)},
        {"vkCmdPushConstants", reinterpret_cast<PFN_vkVoidFunction>(CmdPushConstants)},
        {"vkCreateFramebuffer", reinterpret_cast<PFN_vkVoidFunction>(CreateFramebuffer)},
        {"vkDestroyFramebuffer", reinterpret_cast<PFN_vkVoidFunction>(DestroyFramebuffer)},
        {"vkCreateRenderPass", reinterpret_cast<PFN_vkVoidFunction>(CreateRenderPass)},
        {"vkDestroyRenderPass", reinterpret_cast<PFN_vkVoidFunction>(DestroyRenderPass)},
        {"vkGetRenderAreaGranularity", reinterpret_cast<PFN_vkVoidFunction>(GetRenderAreaGranularity)},
        {"vkCreateCommandPool", reinterpret_cast<PFN_vkVoidFunction>(CreateCommandPool)},
        {"vkDestroyCommandPool", reinterpret_cast<PFN_vkVoidFunction>(DestroyCommandPool)},
        {"vkResetCommandPool", reinterpret_cast<PFN_vkVoidFunction>(ResetCommandPool)},
        {"vkCmdBeginRenderPass", reinterpret_cast<PFN_vkVoidFunction>(CmdBeginRenderPass)},
        {"vkCmdNextSubpass", reinterpret_cast<PFN_vkVoidFunction>(CmdNextSubpass)},
        {"vkCmdExecuteCommands", reinterpret_cast<PFN_vkVoidFunction>(CmdExecuteCommands)},
        {"vkCmdEndRenderPass", reinterpret_cast<PFN_vkVoidFunction>(CmdEndRenderPass)},
        {"vkDebugMarkerSetObjectTagEXT", reinterpret_cast<PFN_vkVoidFunction>(DebugMarkerSetObjectTagEXT) },
        {"vkDebugMarkerSetObjectNameEXT", reinterpret_cast<PFN_vkVoidFunction>(DebugMarkerSetObjectNameEXT) },
        {"vkCmdDebugMarkerBeginEXT", reinterpret_cast<PFN_vkVoidFunction>(CmdDebugMarkerBeginEXT) },
        {"vkCmdDebugMarkerInsertEXT", reinterpret_cast<PFN_vkVoidFunction>(CmdDebugMarkerInsertEXT) },
#ifdef VK_USE_PLATFORM_WIN32_KHR
        {"vkGetMemoryWin32HandleNV", reinterpret_cast<PFN_vkVoidFunction>(GetMemoryWin32HandleNV) },
#endif // VK_USE_PLATFORM_WIN32_KHR
};


    for (size_t i = 0; i < ARRAY_SIZE(core_device_commands); i++) {
        if (!strcmp(core_device_commands[i].name, name))
            return core_device_commands[i].proc;
    }

    return nullptr;
}

static PFN_vkVoidFunction InterceptWsiEnabledCommand(const char *name, VkDevice device) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } wsi_device_commands[] = {
        {"vkCreateSwapchainKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateSwapchainKHR)},
        {"vkGetSwapchainImagesKHR", reinterpret_cast<PFN_vkVoidFunction>(GetSwapchainImagesKHR)},
        {"vkAcquireNextImageKHR", reinterpret_cast<PFN_vkVoidFunction>(AcquireNextImageKHR)},
        {"vkQueuePresentKHR", reinterpret_cast<PFN_vkVoidFunction>(QueuePresentKHR)},
    };

    if (device) {
        layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

        if (device_data->wsi_enabled) {
            for (size_t i = 0; i < ARRAY_SIZE(wsi_device_commands); i++) {
                if (!strcmp(wsi_device_commands[i].name, name))
                    return wsi_device_commands[i].proc;
            }
        }

        if (device_data->wsi_display_swapchain_enabled) {
            if (!strcmp("vkCreateSharedSwapchainsKHR", name)) {
                return reinterpret_cast<PFN_vkVoidFunction>(CreateSharedSwapchainsKHR);
            }
        }
    }

    return nullptr;
}

static PFN_vkVoidFunction InterceptWsiEnabledCommand(const char *name, VkInstance instance) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } wsi_instance_commands[] = {
        {"vkGetPhysicalDeviceSurfaceSupportKHR", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceSurfaceSupportKHR)},
        {"vkGetPhysicalDeviceSurfaceCapabilitiesKHR",
         reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceSurfaceCapabilitiesKHR)},
        {"vkGetPhysicalDeviceSurfaceFormatsKHR", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceSurfaceFormatsKHR)},
        {"vkGetPhysicalDeviceSurfacePresentModesKHR",
         reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceSurfacePresentModesKHR)},
    };

    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(pc_instance_table_map, instance);
    if (instance_extension_map.size() == 0 || !instance_extension_map[pTable].wsi_enabled)
        return nullptr;

    for (size_t i = 0; i < ARRAY_SIZE(wsi_instance_commands); i++) {
        if (!strcmp(wsi_instance_commands[i].name, name))
            return wsi_instance_commands[i].proc;
    }

#ifdef VK_USE_PLATFORM_WIN32_KHR
    if ((instance_extension_map[pTable].win32_enabled == true) && !strcmp("vkCreateWin32SurfaceKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(CreateWin32SurfaceKHR);
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
    if ((instance_extension_map[pTable].xcb_enabled == true) && !strcmp("vkCreateXcbSurfaceKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(CreateXcbSurfaceKHR);
    if ((instance_extension_map[pTable].xcb_enabled == true) && !strcmp("vkGetPhysicalDeviceXcbPresentationSupportKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceXcbPresentationSupportKHR);
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
    if ((instance_extension_map[pTable].xlib_enabled == true) && !strcmp("vkCreateXlibSurfaceKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(CreateXlibSurfaceKHR);
    if ((instance_extension_map[pTable].xlib_enabled == true) && !strcmp("vkGetPhysicalDeviceXlibPresentationSupportKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceXlibPresentationSupportKHR);
#endif // VK_USE_PLATFORM_XLIB_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
    if ((instance_extension_map[pTable].mir_enabled == true) && !strcmp("vkCreateMirSurfaceKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(CreateMirSurfaceKHR);
    if ((instance_extension_map[pTable].mir_enabled == true) && !strcmp("vkGetPhysicalDeviceMirPresentationSupportKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceMirPresentationSupportKHR);
#endif // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    if ((instance_extension_map[pTable].wayland_enabled == true) && !strcmp("vkCreateWaylandSurfaceKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(CreateWaylandSurfaceKHR);
    if ((instance_extension_map[pTable].wayland_enabled == true) &&
        !strcmp("vkGetPhysicalDeviceWaylandPresentationSupportKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceWaylandPresentationSupportKHR);
#endif // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    if ((instance_extension_map[pTable].android_enabled == true) && !strcmp("vkCreateAndroidSurfaceKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(CreateAndroidSurfaceKHR);
#endif // VK_USE_PLATFORM_ANDROID_KHR

    return nullptr;
}

} // namespace parameter_validation

// vk_layer_logging.h expects these to be defined

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(VkInstance instance,
                                                              const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                                                              const VkAllocationCallbacks *pAllocator,
                                                              VkDebugReportCallbackEXT *pMsgCallback) {
    return parameter_validation::CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT msgCallback,
                                                           const VkAllocationCallbacks *pAllocator) {
    parameter_validation::DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags,
                                                   VkDebugReportObjectTypeEXT objType, uint64_t object, size_t location,
                                                   int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    parameter_validation::DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

// loader-layer interface v0

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,
                                                                                      VkExtensionProperties *pProperties) {
    return parameter_validation::EnumerateInstanceExtensionProperties(pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pCount,
                                                                                  VkLayerProperties *pProperties) {
    return parameter_validation::EnumerateInstanceLayerProperties(pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
                                                                                VkLayerProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return parameter_validation::EnumerateDeviceLayerProperties(VK_NULL_HANDLE, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                                                                    const char *pLayerName, uint32_t *pCount,
                                                                                    VkExtensionProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return parameter_validation::EnumerateDeviceExtensionProperties(VK_NULL_HANDLE, pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev, const char *funcName) {
    return parameter_validation::GetDeviceProcAddr(dev, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *funcName) {
    return parameter_validation::GetInstanceProcAddr(instance, funcName);
}
