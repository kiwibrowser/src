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
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Mike Stroyan <mike@LunarG.com>
 * Author: Tobin Ehlis <tobin@lunarg.com>
 */

// Allow use of STL min and max functions in Windows
#define NOMINMAX

#include <algorithm>
#include <assert.h>
#include <cinttypes>
#include <memory>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <vector>

#include "vk_loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vk_struct_string_helper_cpp.h"
#include "vk_enum_validate_helper.h"
#include "image.h"
#include "vk_layer_config.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_table.h"
#include "vk_layer_data.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_utils.h"
#include "vk_layer_logging.h"
#include "vk_validation_error_messages.h"

using namespace std;

namespace image {

struct layer_data {
    VkInstance instance;

    debug_report_data *report_data;
    vector<VkDebugReportCallbackEXT> logging_callback;
    VkLayerDispatchTable *device_dispatch_table;
    VkLayerInstanceDispatchTable *instance_dispatch_table;
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties physicalDeviceProperties;

    unordered_map<VkImage, IMAGE_STATE> imageMap;

    layer_data()
        : report_data(nullptr), device_dispatch_table(nullptr), instance_dispatch_table(nullptr), physicalDevice(0),
          physicalDeviceProperties(){};
};

static unordered_map<void *, layer_data *> layer_data_map;
static std::mutex global_lock;

static void init_image(layer_data *my_data, const VkAllocationCallbacks *pAllocator) {
    layer_debug_actions(my_data->report_data, my_data->logging_callback, pAllocator, "lunarg_image");
}

static IMAGE_STATE const *getImageState(layer_data const *dev_data, VkImage image) {
    auto it = dev_data->imageMap.find(image);
    if (it == dev_data->imageMap.end()) {
        return nullptr;
    }
    return &it->second;
}

VKAPI_ATTR VkResult VKAPI_CALL
CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                             const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pMsgCallback) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkResult res = my_data->instance_dispatch_table->CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
    if (res == VK_SUCCESS) {
        res = layer_create_msg_callback(my_data->report_data, false, pCreateInfo, pAllocator, pMsgCallback);
    }
    return res;
}

VKAPI_ATTR void VKAPI_CALL DestroyDebugReportCallbackEXT(VkInstance instance,
                                                         VkDebugReportCallbackEXT msgCallback,
                                                         const VkAllocationCallbacks *pAllocator) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    my_data->instance_dispatch_table->DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
    layer_destroy_msg_callback(my_data->report_data, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL
DebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t object,
                      size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    my_data->instance_dispatch_table->DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix,
                                                            pMsg);
}

VKAPI_ATTR VkResult VKAPI_CALL
CreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) {
    VkLayerInstanceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkCreateInstance fpCreateInstance = (PFN_vkCreateInstance)fpGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (fpCreateInstance == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS)
        return result;

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
    my_data->instance = *pInstance;
    my_data->instance_dispatch_table = new VkLayerInstanceDispatchTable;
    layer_init_instance_dispatch_table(*pInstance, my_data->instance_dispatch_table, fpGetInstanceProcAddr);

    my_data->report_data = debug_report_create_instance(my_data->instance_dispatch_table, *pInstance,
                                                        pCreateInfo->enabledExtensionCount, pCreateInfo->ppEnabledExtensionNames);

    init_image(my_data, pAllocator);

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    // Grab the key before the instance is destroyed.
    dispatch_key key = get_dispatch_key(instance);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    pTable->DestroyInstance(instance, pAllocator);

    // Clean up logging callback, if any
    while (my_data->logging_callback.size() > 0) {
        VkDebugReportCallbackEXT callback = my_data->logging_callback.back();
        layer_destroy_msg_callback(my_data->report_data, callback, pAllocator);
        my_data->logging_callback.pop_back();
    }

    layer_debug_report_destroy_instance(my_data->report_data);
    delete my_data->instance_dispatch_table;
    layer_data_map.erase(key);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDevice(VkPhysicalDevice physicalDevice,
                                            const VkDeviceCreateInfo *pCreateInfo,
                                            const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    VkLayerDeviceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    PFN_vkCreateDevice fpCreateDevice = (PFN_vkCreateDevice)fpGetInstanceProcAddr(my_instance_data->instance, "vkCreateDevice");
    if (fpCreateDevice == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);

    // Setup device dispatch table
    my_device_data->device_dispatch_table = new VkLayerDispatchTable;
    layer_init_device_dispatch_table(*pDevice, my_device_data->device_dispatch_table, fpGetDeviceProcAddr);

    my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
    my_device_data->physicalDevice = physicalDevice;

    my_instance_data->instance_dispatch_table->GetPhysicalDeviceProperties(physicalDevice,
                                                                           &(my_device_data->physicalDeviceProperties));

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    my_data->device_dispatch_table->DestroyDevice(device, pAllocator);
    delete my_data->device_dispatch_table;
    layer_data_map.erase(key);
}

static const VkExtensionProperties instance_extensions[] = {{VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_SPEC_VERSION}};

static const VkLayerProperties global_layer = {
    "VK_LAYER_LUNARG_image", VK_LAYER_API_VERSION, 1, "LunarG Validation Layer",
};

// Start of the Image layer proper

static inline uint32_t validate_VkImageLayoutKHR(VkImageLayout input_value) {
    return ((validate_VkImageLayout(input_value) == 1) || (input_value == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
}

VKAPI_ATTR VkResult VKAPI_CALL CreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo,
                                           const VkAllocationCallbacks *pAllocator, VkImage *pImage) {
    bool skip_call = false;
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    VkImageFormatProperties ImageFormatProperties;

    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkPhysicalDevice physicalDevice = device_data->physicalDevice;
    layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);

    if (pCreateInfo->format != VK_FORMAT_UNDEFINED) {
        VkFormatProperties properties;
        phy_dev_data->instance_dispatch_table->GetPhysicalDeviceFormatProperties(device_data->physicalDevice, pCreateInfo->format,
                                                                                 &properties);
        if ((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0)) {
            std::stringstream ss;
            ss << "vkCreateImage format parameter (" << string_VkFormat(pCreateInfo->format) << ") is an unsupported format";
            // TODO: Verify against Valid Use section of spec. Generally if something yield an undefined result, it's invalid
            skip_call |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT,
                                 0, __LINE__, IMAGE_FORMAT_UNSUPPORTED, "IMAGE", "%s", ss.str().c_str());
        }

        // Validate that format supports usage as color attachment
        if (pCreateInfo->usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
            if ((pCreateInfo->tiling == VK_IMAGE_TILING_OPTIMAL) &&
                ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) == 0)) {
                std::stringstream ss;
                ss << "vkCreateImage: VkFormat for TILING_OPTIMAL image (" << string_VkFormat(pCreateInfo->format)
                   << ") does not support requested Image usage type VK_IMAGE_USAGE_COLOR_ATTACHMENT";
                skip_call |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                            __LINE__, IMAGE_INVALID_FORMAT, "IMAGE", "%s", ss.str().c_str());
            }
            if ((pCreateInfo->tiling == VK_IMAGE_TILING_LINEAR) &&
                ((properties.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) == 0)) {
                std::stringstream ss;
                ss << "vkCreateImage: VkFormat for TILING_LINEAR image (" << string_VkFormat(pCreateInfo->format)
                   << ") does not support requested Image usage type VK_IMAGE_USAGE_COLOR_ATTACHMENT";
                skip_call |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                            __LINE__, IMAGE_INVALID_FORMAT, "IMAGE", "%s", ss.str().c_str());
            }
        }
        // Validate that format supports usage as depth/stencil attachment
        if (pCreateInfo->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            if ((pCreateInfo->tiling == VK_IMAGE_TILING_OPTIMAL) &&
                ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == 0)) {
                std::stringstream ss;
                ss << "vkCreateImage: VkFormat for TILING_OPTIMAL image (" << string_VkFormat(pCreateInfo->format)
                   << ") does not support requested Image usage type VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT";
                skip_call |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                            __LINE__, IMAGE_INVALID_FORMAT, "IMAGE", "%s", ss.str().c_str());
            }
            if ((pCreateInfo->tiling == VK_IMAGE_TILING_LINEAR) &&
                ((properties.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == 0)) {
                std::stringstream ss;
                ss << "vkCreateImage: VkFormat for TILING_LINEAR image (" << string_VkFormat(pCreateInfo->format)
                   << ") does not support requested Image usage type VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT";
                skip_call |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                            __LINE__, IMAGE_INVALID_FORMAT, "IMAGE", "%s", ss.str().c_str());
            }
        }
    } else {
        skip_call |=
            log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    IMAGE_INVALID_FORMAT, "IMAGE", "vkCreateImage: VkFormat for image must not be VK_FORMAT_UNDEFINED");
    }

    // Internal call to get format info.  Still goes through layers, could potentially go directly to ICD.
    phy_dev_data->instance_dispatch_table->GetPhysicalDeviceImageFormatProperties(
        physicalDevice, pCreateInfo->format, pCreateInfo->imageType, pCreateInfo->tiling, pCreateInfo->usage, pCreateInfo->flags,
        &ImageFormatProperties);

    VkDeviceSize imageGranularity = device_data->physicalDeviceProperties.limits.bufferImageGranularity;
    imageGranularity = imageGranularity == 1 ? 0 : imageGranularity;

    // Make sure all required dimension are non-zero at least.
    bool failedMinSize = false;
    switch (pCreateInfo->imageType) {
    case VK_IMAGE_TYPE_3D:
        if (pCreateInfo->extent.depth == 0) {
            failedMinSize = true;
        }
    // Intentional fall-through
    case VK_IMAGE_TYPE_2D:
        if (pCreateInfo->extent.height == 0) {
            failedMinSize = true;
        }
    // Intentional fall-through
    case VK_IMAGE_TYPE_1D:
        if (pCreateInfo->extent.width == 0) {
            failedMinSize = true;
        }
        break;
    default:
        break;
    }
    if (failedMinSize) {
        skip_call |=
            log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                    0, __LINE__, IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
                    "CreateImage extents is 0 for at least one required dimension for image of type %d: "
                    "Width = %d Height = %d Depth = %d.",
                    pCreateInfo->imageType, pCreateInfo->extent.width, pCreateInfo->extent.height, pCreateInfo->extent.depth);
    }

    if ((pCreateInfo->extent.depth > ImageFormatProperties.maxExtent.depth) ||
        (pCreateInfo->extent.width > ImageFormatProperties.maxExtent.width) ||
        (pCreateInfo->extent.height > ImageFormatProperties.maxExtent.height)) {
        skip_call |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            0, __LINE__, IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
                            "CreateImage extents exceed allowable limits for format: "
                            "Width = %d Height = %d Depth = %d:  Limits for Width = %d Height = %d Depth = %d for format %s.",
                            pCreateInfo->extent.width, pCreateInfo->extent.height, pCreateInfo->extent.depth,
                            ImageFormatProperties.maxExtent.width, ImageFormatProperties.maxExtent.height,
                            ImageFormatProperties.maxExtent.depth, string_VkFormat(pCreateInfo->format));
    }

    uint64_t totalSize = ((uint64_t)pCreateInfo->extent.width * (uint64_t)pCreateInfo->extent.height *
                              (uint64_t)pCreateInfo->extent.depth * (uint64_t)pCreateInfo->arrayLayers *
                              (uint64_t)pCreateInfo->samples * (uint64_t)vk_format_get_size(pCreateInfo->format) +
                          (uint64_t)imageGranularity) &
                         ~(uint64_t)imageGranularity;

    if (totalSize > ImageFormatProperties.maxResourceSize) {
        skip_call |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            0, __LINE__, IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
                            "CreateImage resource size exceeds allowable maximum "
                            "Image resource size = 0x%" PRIxLEAST64 ", maximum resource size = 0x%" PRIxLEAST64 " ",
                            totalSize, ImageFormatProperties.maxResourceSize);
    }

    if (pCreateInfo->mipLevels > ImageFormatProperties.maxMipLevels) {
        skip_call |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            0, __LINE__, IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
                            "CreateImage mipLevels=%d exceeds allowable maximum supported by format of %d", pCreateInfo->mipLevels,
                            ImageFormatProperties.maxMipLevels);
    }

    if (pCreateInfo->arrayLayers > ImageFormatProperties.maxArrayLayers) {
        skip_call |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            0, __LINE__, IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
                            "CreateImage arrayLayers=%d exceeds allowable maximum supported by format of %d",
                            pCreateInfo->arrayLayers, ImageFormatProperties.maxArrayLayers);
    }

    if ((pCreateInfo->samples & ImageFormatProperties.sampleCounts) == 0) {
        skip_call |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            0, __LINE__, IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
                            "CreateImage samples %s is not supported by format 0x%.8X",
                            string_VkSampleCountFlagBits(pCreateInfo->samples), ImageFormatProperties.sampleCounts);
    }

    if (pCreateInfo->initialLayout != VK_IMAGE_LAYOUT_UNDEFINED && pCreateInfo->initialLayout != VK_IMAGE_LAYOUT_PREINITIALIZED) {
        skip_call |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            0, __LINE__, IMAGE_INVALID_LAYOUT, "Image",
                            "vkCreateImage parameter, pCreateInfo->initialLayout, must be VK_IMAGE_LAYOUT_UNDEFINED or "
                            "VK_IMAGE_LAYOUT_PREINITIALIZED");
    }

    if (!skip_call) {
        result = device_data->device_dispatch_table->CreateImage(device, pCreateInfo, pAllocator, pImage);
    }
    if (result == VK_SUCCESS) {
        std::lock_guard<std::mutex> lock(global_lock);
        device_data->imageMap[*pImage] = IMAGE_STATE(pCreateInfo);
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks *pAllocator) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    device_data->imageMap.erase(image);
    lock.unlock();
    device_data->device_dispatch_table->DestroyImage(device, image, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo,
                                                const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;
    bool depth_format_present = false;

    for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
        if (pCreateInfo->pAttachments[i].format == VK_FORMAT_UNDEFINED) {
            std::stringstream ss;
            ss << "vkCreateRenderPass: pCreateInfo->pAttachments[" << i << "].format is VK_FORMAT_UNDEFINED";
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 IMAGE_RENDERPASS_INVALID_ATTACHMENT, "IMAGE", "%s", ss.str().c_str());
        }

        if (!validate_VkImageLayoutKHR(pCreateInfo->pAttachments[i].initialLayout) ||
            !validate_VkImageLayoutKHR(pCreateInfo->pAttachments[i].finalLayout)) {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkImageLayout in pCreateInfo->pAttachments[" << i << "], is unrecognized";
            // TODO: Verify against Valid Use section of spec. Generally if something yield an undefined result, it's invalid
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 IMAGE_RENDERPASS_INVALID_ATTACHMENT, "IMAGE", "%s", ss.str().c_str());
        }

        if (!validate_VkAttachmentLoadOp(pCreateInfo->pAttachments[i].loadOp)) {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkAttachmentLoadOp in pCreateInfo->pAttachments[" << i << "], is unrecognized";
            // TODO: Verify against Valid Use section of spec. Generally if something yield an undefined result, it's invalid
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 IMAGE_RENDERPASS_INVALID_ATTACHMENT, "IMAGE", "%s", ss.str().c_str());
        }

        if (!validate_VkAttachmentStoreOp(pCreateInfo->pAttachments[i].storeOp)) {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkAttachmentStoreOp in pCreateInfo->pAttachments[" << i << "], is unrecognized";
            // TODO: Verify against Valid Use section of spec. Generally if something yield an undefined result, it's invalid
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 IMAGE_RENDERPASS_INVALID_ATTACHMENT, "IMAGE", "%s", ss.str().c_str());
        }

        // Any depth buffers specified as attachments?
        depth_format_present |= vk_format_is_depth_or_stencil(pCreateInfo->pAttachments[i].format);
    }

    if (!depth_format_present) {
        // No depth attachment is present, validate that subpasses set depthStencilAttachment to VK_ATTACHMENT_UNUSED;
        for (uint32_t i = 0; i < pCreateInfo->subpassCount; i++) {
            if (pCreateInfo->pSubpasses[i].pDepthStencilAttachment &&
                pCreateInfo->pSubpasses[i].pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
                std::stringstream ss;
                ss << "vkCreateRenderPass has no depth/stencil attachment, yet subpass[" << i
                   << "] has VkSubpassDescription::depthStencilAttachment value that is not VK_ATTACHMENT_UNUSED";
                skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                     __LINE__, IMAGE_RENDERPASS_INVALID_DS_ATTACHMENT, "IMAGE", "%s", ss.str().c_str());
            }
        }
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    VkResult result = my_data->device_dispatch_table->CreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo,
                                               const VkAllocationCallbacks *pAllocator, VkImageView *pView) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    auto imageEntry = getImageState(device_data, pCreateInfo->image);
    if (imageEntry) {
        if (pCreateInfo->subresourceRange.baseMipLevel >= imageEntry->mipLevels) {
            std::stringstream ss;
            ss << "vkCreateImageView called with baseMipLevel " << pCreateInfo->subresourceRange.baseMipLevel << " for image "
               << pCreateInfo->image << " that only has " << imageEntry->mipLevels << " mip levels.";
            skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                IMAGE_VIEW_CREATE_ERROR, "IMAGE", "%s", ss.str().c_str());
        }
        if (pCreateInfo->subresourceRange.baseArrayLayer >= imageEntry->arraySize) {
            std::stringstream ss;
            ss << "vkCreateImageView called with baseArrayLayer " << pCreateInfo->subresourceRange.baseArrayLayer << " for image "
               << pCreateInfo->image << " that only has " << imageEntry->arraySize << " array layers.";
            skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                IMAGE_VIEW_CREATE_ERROR, "IMAGE", "%s", ss.str().c_str());
        }
        if (!pCreateInfo->subresourceRange.levelCount) {
            std::stringstream ss;
            ss << "vkCreateImageView called with 0 in pCreateInfo->subresourceRange.levelCount.";
            skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                IMAGE_VIEW_CREATE_ERROR, "IMAGE", "%s", ss.str().c_str());
        }
        if (!pCreateInfo->subresourceRange.layerCount) {
            std::stringstream ss;
            ss << "vkCreateImageView called with 0 in pCreateInfo->subresourceRange.layerCount.";
            skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                IMAGE_VIEW_CREATE_ERROR, "IMAGE", "%s", ss.str().c_str());
        }

        VkImageCreateFlags imageFlags = imageEntry->flags;
        VkFormat imageFormat = imageEntry->format;
        VkFormat ivciFormat = pCreateInfo->format;
        VkImageAspectFlags aspectMask = pCreateInfo->subresourceRange.aspectMask;

        // Validate VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT state
        if (imageFlags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT) {
            // Format MUST be compatible (in the same format compatibility class) as the format the image was created with
            if (vk_format_get_compatibility_class(imageFormat) != vk_format_get_compatibility_class(ivciFormat)) {
                std::stringstream ss;
                ss << "vkCreateImageView(): ImageView format " << string_VkFormat(ivciFormat)
                   << " is not in the same format compatibility class as image (" << (uint64_t)pCreateInfo->image << ")  format "
                   << string_VkFormat(imageFormat) << ".  Images created with the VK_IMAGE_CREATE_MUTABLE_FORMAT BIT "
                   << "can support ImageViews with differing formats but they must be in the same compatibility class.";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                    __LINE__, IMAGE_VIEW_CREATE_ERROR, "IMAGE", "%s", ss.str().c_str());
            }
        } else {
            // Format MUST be IDENTICAL to the format the image was created with
            if (imageFormat != ivciFormat) {
                std::stringstream ss;
                ss << "vkCreateImageView() format " << string_VkFormat(ivciFormat) << " differs from image "
                   << (uint64_t)pCreateInfo->image << " format " << string_VkFormat(imageFormat)
                   << ".  Formats MUST be IDENTICAL unless VK_IMAGE_CREATE_MUTABLE_FORMAT BIT was set on image creation.";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                    __LINE__, IMAGE_VIEW_CREATE_ERROR, "IMAGE", "%s", ss.str().c_str());
            }
        }

        // Validate correct image aspect bits for desired formats and format consistency
        if (vk_format_is_color(imageFormat)) {
            if ((aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) != VK_IMAGE_ASPECT_COLOR_BIT) {
                std::stringstream ss;
                ss << "vkCreateImageView: Color image formats must have the VK_IMAGE_ASPECT_COLOR_BIT set";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            (uint64_t)pCreateInfo->image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
            if ((aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) != aspectMask) {
                std::stringstream ss;
                ss << "vkCreateImageView: Color image formats must have ONLY the VK_IMAGE_ASPECT_COLOR_BIT set";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            (uint64_t)pCreateInfo->image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
            if (!vk_format_is_color(ivciFormat)) {
                std::stringstream ss;
                ss << "vkCreateImageView: The image view's format can differ from the parent image's format, but both must be "
                   << "color formats.  ImageFormat is " << string_VkFormat(imageFormat) << " ImageViewFormat is "
                   << string_VkFormat(ivciFormat);
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                    (uint64_t)pCreateInfo->image, __LINE__, IMAGE_INVALID_FORMAT, "IMAGE", "%s", ss.str().c_str());
            }
            // TODO:  Uncompressed formats are compatible if they occupy they same number of bits per pixel.
            //        Compressed formats are compatible if the only difference between them is the numerical type of
            //        the uncompressed pixels (e.g. signed vs. unsigned, or sRGB vs. UNORM encoding).
        } else if (vk_format_is_depth_and_stencil(imageFormat)) {
            if ((aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) == 0) {
                std::stringstream ss;
                ss << "vkCreateImageView: Depth/stencil image formats must have at least one of VK_IMAGE_ASPECT_DEPTH_BIT and "
                      "VK_IMAGE_ASPECT_STENCIL_BIT set";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            (uint64_t)pCreateInfo->image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
            if ((aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != aspectMask) {
                std::stringstream ss;
                ss << "vkCreateImageView: Combination depth/stencil image formats can have only the VK_IMAGE_ASPECT_DEPTH_BIT and "
                      "VK_IMAGE_ASPECT_STENCIL_BIT set";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            (uint64_t)pCreateInfo->image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
        } else if (vk_format_is_depth_only(imageFormat)) {
            if ((aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) != VK_IMAGE_ASPECT_DEPTH_BIT) {
                std::stringstream ss;
                ss << "vkCreateImageView: Depth-only image formats must have the VK_IMAGE_ASPECT_DEPTH_BIT set";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            (uint64_t)pCreateInfo->image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
            if ((aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) != aspectMask) {
                std::stringstream ss;
                ss << "vkCreateImageView: Depth-only image formats can have only the VK_IMAGE_ASPECT_DEPTH_BIT set";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            (uint64_t)pCreateInfo->image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
        } else if (vk_format_is_stencil_only(imageFormat)) {
            if ((aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != VK_IMAGE_ASPECT_STENCIL_BIT) {
                std::stringstream ss;
                ss << "vkCreateImageView: Stencil-only image formats must have the VK_IMAGE_ASPECT_STENCIL_BIT set";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            (uint64_t)pCreateInfo->image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
            if ((aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != aspectMask) {
                std::stringstream ss;
                ss << "vkCreateImageView: Stencil-only image formats can have only the VK_IMAGE_ASPECT_STENCIL_BIT set";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            (uint64_t)pCreateInfo->image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
        }
    }

    if (skipCall) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    VkResult result = device_data->device_dispatch_table->CreateImageView(device, pCreateInfo, pAllocator, pView);
    return result;
}

VKAPI_ATTR void VKAPI_CALL CmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image,
                                              VkImageLayout imageLayout, const VkClearColorValue *pColor,
                                              uint32_t rangeCount, const VkImageSubresourceRange *pRanges) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);

    if (imageLayout != VK_IMAGE_LAYOUT_GENERAL && imageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_LAYOUT, "IMAGE",
                            "vkCmdClearColorImage parameter, imageLayout, must be VK_IMAGE_LAYOUT_GENERAL or "
                            "VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL");
    }

    // For each range, image aspect must be color only
    for (uint32_t i = 0; i < rangeCount; i++) {
        if (pRanges[i].aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) {
            char const str[] =
                "vkCmdClearColorImage aspectMasks for all subresource ranges must be set to VK_IMAGE_ASPECT_COLOR_BIT";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", str);
        }
    }

    auto image_state = getImageState(device_data, image);
    if (image_state) {
        if (vk_format_is_depth_or_stencil(image_state->format)) {
            char const str[] = "vkCmdClearColorImage called with depth/stencil image.";
            skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                reinterpret_cast<uint64_t &>(image), __LINE__, IMAGE_INVALID_FORMAT, "IMAGE", str);
        } else if (vk_format_is_compressed(image_state->format)) {
            char const str[] = "vkCmdClearColorImage called with compressed image.";
            skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                reinterpret_cast<uint64_t &>(image), __LINE__, IMAGE_INVALID_FORMAT, "IMAGE", str);
        }

        if (!(image_state->usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
            char const str[] = "vkCmdClearColorImage called with image created without VK_IMAGE_USAGE_TRANSFER_DST_BIT.";
            skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                reinterpret_cast<uint64_t &>(image), __LINE__, IMAGE_INVALID_USAGE, "IMAGE", str);
        }
    }

    if (!skipCall) {
        device_data->device_dispatch_table->CmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
    }
}

VKAPI_ATTR void VKAPI_CALL
CmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                          const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount,
                          const VkImageSubresourceRange *pRanges) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    // For each range, Image aspect must be depth or stencil or both
    for (uint32_t i = 0; i < rangeCount; i++) {
        if (((pRanges[i].aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) != VK_IMAGE_ASPECT_DEPTH_BIT) &&
            ((pRanges[i].aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != VK_IMAGE_ASPECT_STENCIL_BIT)) {
            char const str[] = "vkCmdClearDepthStencilImage aspectMasks for all subresource ranges must be "
                               "set to VK_IMAGE_ASPECT_DEPTH_BIT and/or VK_IMAGE_ASPECT_STENCIL_BIT";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", str);
        }
    }

    auto image_state = getImageState(device_data, image);
    if (image_state && !vk_format_is_depth_or_stencil(image_state->format)) {
        char const str[] = "vkCmdClearDepthStencilImage called without a depth/stencil image.";
        skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            reinterpret_cast<uint64_t &>(image), __LINE__, IMAGE_INVALID_FORMAT, "IMAGE", str);
    }

    if (!skipCall) {
        device_data->device_dispatch_table->CmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount,
                                                                      pRanges);
    }
}

// Returns true if [x, xoffset] and [y, yoffset] overlap
static bool RangesIntersect(int32_t start, uint32_t start_offset, int32_t end, uint32_t end_offset) {
    bool result = false;
    uint32_t intersection_min = std::max(static_cast<uint32_t>(start), static_cast<uint32_t>(end));
    uint32_t intersection_max = std::min(static_cast<uint32_t>(start) + start_offset, static_cast<uint32_t>(end) + end_offset);

    if (intersection_max > intersection_min) {
        result = true;
    }
    return result;
}

// Returns true if two VkImageCopy structures overlap
static bool RegionIntersects(const VkImageCopy *src, const VkImageCopy *dst, VkImageType type) {
    bool result = true;
    if ((src->srcSubresource.mipLevel == dst->dstSubresource.mipLevel) &&
        (RangesIntersect(src->srcSubresource.baseArrayLayer, src->srcSubresource.layerCount, dst->dstSubresource.baseArrayLayer,
                         dst->dstSubresource.layerCount))) {

        switch (type) {
        case VK_IMAGE_TYPE_3D:
            result &= RangesIntersect(src->srcOffset.z, src->extent.depth, dst->dstOffset.z, dst->extent.depth);
        // Intentionally fall through to 2D case
        case VK_IMAGE_TYPE_2D:
            result &= RangesIntersect(src->srcOffset.y, src->extent.height, dst->dstOffset.y, dst->extent.height);
        // Intentionally fall through to 1D case
        case VK_IMAGE_TYPE_1D:
            result &= RangesIntersect(src->srcOffset.x, src->extent.width, dst->dstOffset.x, dst->extent.width);
            break;
        default:
            // Unrecognized or new IMAGE_TYPE enums will be caught in parameter_validation
            assert(false);
        }
    }
    return result;
}

// Returns true if offset and extent exceed image extents
static bool ExceedsBounds(const VkOffset3D *offset, const VkExtent3D *extent, const IMAGE_STATE *image) {
    bool result = false;
    // Extents/depths cannot be negative but checks left in for clarity
    switch (image->imageType) {
    case VK_IMAGE_TYPE_3D: // Validate z and depth
        if ((offset->z + extent->depth > image->extent.depth) || (offset->z < 0) ||
            ((offset->z + static_cast<int32_t>(extent->depth)) < 0)) {
            result = true;
        }
        // Intentionally fall through to 2D case to check height
    case VK_IMAGE_TYPE_2D: // Validate y and height
        if ((offset->y + extent->height > image->extent.height) || (offset->y < 0) ||
            ((offset->y + static_cast<int32_t>(extent->height)) < 0)) {
            result = true;
        }
        // Intentionally fall through to 1D case to check width
    case VK_IMAGE_TYPE_1D: // Validate x and width
        if ((offset->x + extent->width > image->extent.width) || (offset->x < 0) ||
            ((offset->x + static_cast<int32_t>(extent->width)) < 0)) {
            result = true;
        }
        break;
    default:
        assert(false);
    }
    return result;
}

bool PreCallValidateCmdCopyImage(VkCommandBuffer command_buffer, VkImage src_image, VkImage dst_image, uint32_t region_count,
                                 const VkImageCopy *regions) {
    bool skip = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(command_buffer), layer_data_map);
    auto src_image_entry = getImageState(device_data, src_image);
    auto dst_image_entry = getImageState(device_data, dst_image);

    // TODO: This does not cover swapchain-created images. This should fall out when this layer is moved
    // into the core_validation layer
    if (src_image_entry && dst_image_entry) {

        for (uint32_t i = 0; i < region_count; i++) {

            if (regions[i].srcSubresource.layerCount == 0) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: number of layers in pRegions[" << i << "] srcSubresource is zero";
                skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(command_buffer),
                                __LINE__, IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }

            if (regions[i].dstSubresource.layerCount == 0) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: number of layers in pRegions[" << i << "] dstSubresource is zero";
                skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(command_buffer),
                                __LINE__, IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }

            // For each region the layerCount member of srcSubresource and dstSubresource must match
            if (regions[i].srcSubresource.layerCount != regions[i].dstSubresource.layerCount) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: number of layers in source and destination subresources for pRegions[" << i
                   << "] do not match";
                skip |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01198, "IMAGE", "%s. %s",
                            ss.str().c_str(), validation_error_map[VALIDATION_ERROR_01198]);
            }

            // For each region, the aspectMask member of srcSubresource and dstSubresource must match
            if (regions[i].srcSubresource.aspectMask != regions[i].dstSubresource.aspectMask) {
                char const str[] = "vkCmdCopyImage: Src and dest aspectMasks for each region must match";
                skip |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", str);
            }

            // AspectMask must not contain VK_IMAGE_ASPECT_METADATA_BIT
            if ((regions[i].srcSubresource.aspectMask & VK_IMAGE_ASPECT_METADATA_BIT) ||
                (regions[i].dstSubresource.aspectMask & VK_IMAGE_ASPECT_METADATA_BIT)) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: pRegions[" << i << "] may not specify aspectMask containing VK_IMAGE_ASPECT_METADATA_BIT";
                skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(command_buffer),
                                __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }

            // For each region, if aspectMask contains VK_IMAGE_ASPECT_COLOR_BIT, it must not contain either of
            // VK_IMAGE_ASPECT_DEPTH_BIT or VK_IMAGE_ASPECT_STENCIL_BIT
            if ((regions[i].srcSubresource.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) &&
                (regions[i].srcSubresource.aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))) {
                char const str[] = "vkCmdCopyImage aspectMask cannot specify both COLOR and DEPTH/STENCIL aspects";
                skip |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", str);
            }

            // If either of the calling command's src_image or dst_image parameters are of VkImageType VK_IMAGE_TYPE_3D,
            // the baseArrayLayer and layerCount members of both srcSubresource and dstSubresource must be 0 and 1, respectively
            if (((src_image_entry->imageType == VK_IMAGE_TYPE_3D) || (dst_image_entry->imageType == VK_IMAGE_TYPE_3D)) &&
                ((regions[i].srcSubresource.baseArrayLayer != 0) || (regions[i].srcSubresource.layerCount != 1) ||
                 (regions[i].dstSubresource.baseArrayLayer != 0) || (regions[i].dstSubresource.layerCount != 1))) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: src or dstImage type was IMAGE_TYPE_3D, but in subRegion[" << i
                   << "] baseArrayLayer was not zero or layerCount was not 1.";
                skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(command_buffer),
                                __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
            }

            // MipLevel must be less than the mipLevels specified in VkImageCreateInfo when the image was created
            if (regions[i].srcSubresource.mipLevel >= src_image_entry->mipLevels) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: pRegions[" << i
                   << "] specifies a src mipLevel greater than the number specified when the srcImage was created.";
                skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(command_buffer),
                                __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
            }
            if (regions[i].dstSubresource.mipLevel >= dst_image_entry->mipLevels) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: pRegions[" << i
                   << "] specifies a dst mipLevel greater than the number specified when the dstImage was created.";
                skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(command_buffer),
                                __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
            }

            // (baseArrayLayer + layerCount) must be less than or equal to the arrayLayers specified in VkImageCreateInfo when the
            // image was created
            if ((regions[i].srcSubresource.baseArrayLayer + regions[i].srcSubresource.layerCount) > src_image_entry->arraySize) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: srcImage arrayLayers was " << src_image_entry->arraySize << " but subRegion[" << i
                   << "] baseArrayLayer + layerCount is "
                   << (regions[i].srcSubresource.baseArrayLayer + regions[i].srcSubresource.layerCount);
                skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(command_buffer),
                                __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
            }
            if ((regions[i].dstSubresource.baseArrayLayer + regions[i].dstSubresource.layerCount) > dst_image_entry->arraySize) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: dstImage arrayLayers was " << dst_image_entry->arraySize << " but subRegion[" << i
                   << "] baseArrayLayer + layerCount is "
                   << (regions[i].dstSubresource.baseArrayLayer + regions[i].dstSubresource.layerCount);
                skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(command_buffer),
                                __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
            }

            // The source region specified by a given element of regions must be a region that is contained within srcImage
            if (ExceedsBounds(&regions[i].srcOffset, &regions[i].extent, src_image_entry)) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: srcSubResource in pRegions[" << i << "] exceeds extents srcImage was created with";
                skip |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01175, "IMAGE", "%s. %s",
                            ss.str().c_str(), validation_error_map[VALIDATION_ERROR_01175]);
            }

            // The destination region specified by a given element of regions must be a region that is contained within dst_image
            if (ExceedsBounds(&regions[i].dstOffset, &regions[i].extent, dst_image_entry)) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: dstSubResource in pRegions[" << i << "] exceeds extents dstImage was created with";
                skip |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01176, "IMAGE", "%s. %s",
                            ss.str().c_str(), validation_error_map[VALIDATION_ERROR_01176]);
            }

            // The union of all source regions, and the union of all destination regions, specified by the elements of regions,
            // must not overlap in memory
            if (src_image == dst_image) {
                for (uint32_t j = 0; j < region_count; j++) {
                    if (RegionIntersects(&regions[i], &regions[j], src_image_entry->imageType)) {
                        std::stringstream ss;
                        ss << "vkCmdCopyImage: pRegions[" << i << "] src overlaps with pRegions[" << j << "].";
                        skip |=
                            log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(command_buffer),
                                    __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
                    }
                }
            }
        }

        // The formats of src_image and dst_image must be compatible. Formats are considered compatible if their texel size in bytes
        // is the same between both formats. For example, VK_FORMAT_R8G8B8A8_UNORM is compatible with VK_FORMAT_R32_UINT because
        // because both texels are 4 bytes in size. Depth/stencil formats must match exactly.
        if (vk_format_is_depth_or_stencil(src_image_entry->format) || vk_format_is_depth_or_stencil(dst_image_entry->format)) {
            if (src_image_entry->format != dst_image_entry->format) {
                char const str[] = "vkCmdCopyImage called with unmatched source and dest image depth/stencil formats.";
                skip |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, IMAGE_MISMATCHED_IMAGE_FORMAT, "IMAGE", str);
            }
        } else {
            size_t srcSize = vk_format_get_size(src_image_entry->format);
            size_t destSize = vk_format_get_size(dst_image_entry->format);
            if (srcSize != destSize) {
                char const str[] = "vkCmdCopyImage called with unmatched source and dest image format sizes.";
                skip |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01184, "IMAGE", "%s. %s", str,
                            validation_error_map[VALIDATION_ERROR_01184]);
            }
        }
    }
    return skip;
}

VKAPI_ATTR void VKAPI_CALL CmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage,
                                        VkImageLayout srcImageLayout, VkImage dstImage,
                                        VkImageLayout dstImageLayout, uint32_t regionCount,
                                        const VkImageCopy *pRegions) {

    bool skip = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);

    skip = PreCallValidateCmdCopyImage(commandBuffer, srcImage, dstImage, regionCount, pRegions);

    if (!skip) {
        device_data->device_dispatch_table->CmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout,
                                                         regionCount, pRegions);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount,
                                               const VkClearAttachment *pAttachments, uint32_t rectCount,
                                               const VkClearRect *pRects) {
    bool skipCall = false;
    VkImageAspectFlags aspectMask;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    for (uint32_t i = 0; i < attachmentCount; i++) {
        aspectMask = pAttachments[i].aspectMask;
        if (aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) {
            if (aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) {
                // VK_IMAGE_ASPECT_COLOR_BIT is not the only bit set for this attachment
                char const str[] =
                    "vkCmdClearAttachments aspectMask [%d] must set only VK_IMAGE_ASPECT_COLOR_BIT of a color attachment.";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", str, i);
            }
        } else {
            // Image aspect must be depth or stencil or both
            if (((aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) != VK_IMAGE_ASPECT_DEPTH_BIT) &&
                ((aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != VK_IMAGE_ASPECT_STENCIL_BIT)) {
                char const str[] = "vkCmdClearAttachments aspectMask [%d] must be set to VK_IMAGE_ASPECT_DEPTH_BIT and/or "
                                   "VK_IMAGE_ASPECT_STENCIL_BIT";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", str, i);
            }
        }
    }

    if (!skipCall) {
        device_data->device_dispatch_table->CmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
    }
}

static bool ValidateBufferToImageData(layer_data *dev_data, uint32_t regionCount, const VkBufferImageCopy *pRegions, VkImage image,
                                      const char *function) {
    bool skip = false;

    for (uint32_t i = 0; i < regionCount; i++) {

        auto image_info = getImageState(dev_data, image);
        if (image_info) {
            // BufferOffset must be a multiple of the calling command's VkImage parameter's texel size
            auto texel_size = vk_format_get_size(image_info->format);
            if (vk_safe_modulo(pRegions[i].bufferOffset, texel_size) != 0) {
                skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                reinterpret_cast<uint64_t &>(image), __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE",
                                "%s(): pRegion[%d] bufferOffset 0x%" PRIxLEAST64
                                " must be a multiple of this format's texel size (" PRINTF_SIZE_T_SPECIFIER ")",
                                function, i, pRegions[i].bufferOffset, texel_size);
            }
            //  BufferOffset must be a multiple of 4
            if (vk_safe_modulo(pRegions[i].bufferOffset, 4) != 0) {
                skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                reinterpret_cast<uint64_t &>(image), __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE",
                                "%s(): pRegion[%d] bufferOffset 0x%" PRIxLEAST64 " must be a multiple of 4", function, i,
                                pRegions[i].bufferOffset);
            }

            //  BufferRowLength must be 0, or greater than or equal to the width member of imageExtent
            if ((pRegions[i].bufferRowLength != 0) && (pRegions[i].bufferRowLength < pRegions[i].imageExtent.width)) {
                skip |= log_msg(
                    dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                    reinterpret_cast<uint64_t &>(image), __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE",
                    "%s(): pRegion[%d] bufferRowLength (%d) must be zero or greater-than-or-equal-to imageExtent.width (%d).",
                    function, i, pRegions[i].bufferRowLength, pRegions[i].imageExtent.width);
            }

            //  BufferImageHeight must be 0, or greater than or equal to the height member of imageExtent
            if ((pRegions[i].bufferImageHeight != 0) && (pRegions[i].bufferImageHeight < pRegions[i].imageExtent.height)) {
                skip |= log_msg(
                    dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                    reinterpret_cast<uint64_t &>(image), __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE",
                    "%s(): pRegion[%d] bufferImageHeight (%d) must be zero or greater-than-or-equal-to imageExtent.height (%d).",
                    function, i, pRegions[i].bufferImageHeight, pRegions[i].imageExtent.height);
            }
        }
    }

    return skip;
}

VKAPI_ATTR void VKAPI_CALL CmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage,
                                                VkImageLayout srcImageLayout, VkBuffer dstBuffer,
                                                uint32_t regionCount, const VkBufferImageCopy *pRegions) {
    bool skip_call = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);

    skip_call |= ValidateBufferToImageData(device_data, regionCount, pRegions, srcImage, "vkCmdCopyImageToBuffer");

    // For each region, the number of layers in the image subresource should not be zero
    // Image aspect must be ONE OF color, depth, stencil
    for (uint32_t i = 0; i < regionCount; i++) {
        if (pRegions[i].imageSubresource.layerCount == 0) {
            char const str[] = "vkCmdCopyImageToBuffer: number of layers in image subresource is zero";
            // TODO: Verify against Valid Use section of spec, if this case yields undefined results, then it's an error
            skip_call |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", str);
        }

        VkImageAspectFlags aspectMask = pRegions[i].imageSubresource.aspectMask;
        if ((aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) && (aspectMask != VK_IMAGE_ASPECT_DEPTH_BIT) &&
            (aspectMask != VK_IMAGE_ASPECT_STENCIL_BIT)) {
            char const str[] = "vkCmdCopyImageToBuffer: aspectMasks for each region must specify only COLOR or DEPTH or STENCIL";
            skip_call |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", str);
        }
    }

    if (!skip_call) {
        device_data->device_dispatch_table->CmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount,
                                                                 pRegions);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer,
                                                VkImage dstImage, VkImageLayout dstImageLayout,
                                                uint32_t regionCount, const VkBufferImageCopy *pRegions) {
    bool skip_call = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);

    skip_call |= ValidateBufferToImageData(device_data, regionCount, pRegions, dstImage, "vkCmdCopyBufferToImage");

    // For each region, the number of layers in the image subresource should not be zero
    // Image aspect must be ONE OF color, depth, stencil
    for (uint32_t i = 0; i < regionCount; i++) {
        if (pRegions[i].imageSubresource.layerCount == 0) {
            char const str[] = "vkCmdCopyBufferToImage: number of layers in image subresource is zero";
            // TODO: Verify against Valid Use section of spec, if this case yields undefined results, then it's an error
            skip_call |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", str);
        }

        VkImageAspectFlags aspectMask = pRegions[i].imageSubresource.aspectMask;
        if ((aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) && (aspectMask != VK_IMAGE_ASPECT_DEPTH_BIT) &&
            (aspectMask != VK_IMAGE_ASPECT_STENCIL_BIT)) {
            char const str[] = "vkCmdCopyBufferToImage: aspectMasks for each region must specify only COLOR or DEPTH or STENCIL";
            skip_call |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", str);
        }
    }

    if (!skip_call) {
        device_data->device_dispatch_table->CmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount,
                                                                 pRegions);
    }
}

VKAPI_ATTR void VKAPI_CALL
CmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
             VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit *pRegions, VkFilter filter) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);

    // Warn for zero-sized regions
    for (uint32_t i = 0; i < regionCount; i++) {
        if ((pRegions[i].srcOffsets[0].x == pRegions[i].srcOffsets[1].x) ||
            (pRegions[i].srcOffsets[0].y == pRegions[i].srcOffsets[1].y) ||
            (pRegions[i].srcOffsets[0].z == pRegions[i].srcOffsets[1].z)) {
            std::stringstream ss;
            ss << "vkCmdBlitImage: pRegions[" << i << "].srcOffsets specify a zero-volume area.";
            skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t>(commandBuffer),
                __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
        }
        if ((pRegions[i].dstOffsets[0].x == pRegions[i].dstOffsets[1].x) ||
            (pRegions[i].dstOffsets[0].y == pRegions[i].dstOffsets[1].y) ||
            (pRegions[i].dstOffsets[0].z == pRegions[i].dstOffsets[1].z)) {
            std::stringstream ss;
            ss << "vkCmdBlitImage: pRegions[" << i << "].dstOffsets specify a zero-volume area.";
            skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t>(commandBuffer),
                __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
        }
    }

    auto srcImageEntry = getImageState(device_data, srcImage);
    auto dstImageEntry = getImageState(device_data, dstImage);

    if (srcImageEntry && dstImageEntry) {

        VkFormat srcFormat = srcImageEntry->format;
        VkFormat dstFormat = dstImageEntry->format;

        // Validate consistency for signed and unsigned formats
        if ((vk_format_is_sint(srcFormat) && !vk_format_is_sint(dstFormat)) ||
            (vk_format_is_uint(srcFormat) && !vk_format_is_uint(dstFormat))) {
            std::stringstream ss;
            ss << "vkCmdBlitImage: If one of srcImage and dstImage images has signed/unsigned integer format, "
               << "the other one must also have signed/unsigned integer format.  "
               << "Source format is " << string_VkFormat(srcFormat) << " Destination format is " << string_VkFormat(dstFormat);
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_FORMAT, "IMAGE", "%s", ss.str().c_str());
        }

        // Validate aspect bits and formats for depth/stencil images
        if (vk_format_is_depth_or_stencil(srcFormat) || vk_format_is_depth_or_stencil(dstFormat)) {
            if (srcFormat != dstFormat) {
                std::stringstream ss;
                ss << "vkCmdBlitImage: If one of srcImage and dstImage images has a format of depth, stencil or depth "
                   << "stencil, the other one must have exactly the same format.  "
                   << "Source format is " << string_VkFormat(srcFormat) << " Destination format is " << string_VkFormat(dstFormat);
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_FORMAT, "IMAGE", "%s", ss.str().c_str());
            }

            for (uint32_t i = 0; i < regionCount; i++) {
                if (pRegions[i].srcSubresource.layerCount == 0) {
                    char const str[] = "vkCmdBlitImage: number of layers in source subresource is zero";
                    // TODO: Verify against Valid Use section of spec, if this case yields undefined results, then it's an error
                    skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                                        IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", str);
                }

                if (pRegions[i].dstSubresource.layerCount == 0) {
                    char const str[] = "vkCmdBlitImage: number of layers in destination subresource is zero";
                    // TODO: Verify against Valid Use section of spec, if this case yields undefined results, then it's an error
                    skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                                        IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", str);
                }

                if (pRegions[i].srcSubresource.layerCount != pRegions[i].dstSubresource.layerCount) {
                    char const str[] = "vkCmdBlitImage: number of layers in source and destination subresources must match";
                    skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                                        IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", str);
                }

                VkImageAspectFlags srcAspect = pRegions[i].srcSubresource.aspectMask;
                VkImageAspectFlags dstAspect = pRegions[i].dstSubresource.aspectMask;

                if (srcAspect != dstAspect) {
                    std::stringstream ss;
                    ss << "vkCmdBlitImage: Image aspects of depth/stencil images should match";
                    // TODO: Verify against Valid Use section of spec, if this case yields undefined results, then it's an error
                    skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                                        IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
                }
                if (vk_format_is_depth_and_stencil(srcFormat)) {
                    if ((srcAspect != VK_IMAGE_ASPECT_DEPTH_BIT) && (srcAspect != VK_IMAGE_ASPECT_STENCIL_BIT)) {
                        std::stringstream ss;
                        ss << "vkCmdBlitImage: Combination depth/stencil image formats must have only one of "
                              "VK_IMAGE_ASPECT_DEPTH_BIT "
                           << "and VK_IMAGE_ASPECT_STENCIL_BIT set in srcImage and dstImage";
                        skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                            VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                                            IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
                    }
                } else if (vk_format_is_stencil_only(srcFormat)) {
                    if (srcAspect != VK_IMAGE_ASPECT_STENCIL_BIT) {
                        std::stringstream ss;
                        ss << "vkCmdBlitImage: Stencil-only image formats must have only the VK_IMAGE_ASPECT_STENCIL_BIT "
                           << "set in both the srcImage and dstImage";
                        skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                            VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                                            IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
                    }
                } else if (vk_format_is_depth_only(srcFormat)) {
                    if (srcAspect != VK_IMAGE_ASPECT_DEPTH_BIT) {
                        std::stringstream ss;
                        ss << "vkCmdBlitImage: Depth-only image formats must have only the VK_IMAGE_ASPECT_DEPTH "
                           << "set in both the srcImage and dstImage";
                        skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                            VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                                            IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
                    }
                }
            }
        }

        // Validate filter
        if (vk_format_is_depth_or_stencil(srcFormat) || vk_format_is_int(srcFormat)) {
            if (filter != VK_FILTER_NEAREST) {
                std::stringstream ss;
                ss << "vkCmdBlitImage: If the format of srcImage is a depth, stencil, depth stencil or integer-based format "
                   << "then filter must be VK_FILTER_NEAREST.";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_FILTER, "IMAGE", "%s", ss.str().c_str());
            }
        }
    }

    if (!skipCall) {
        device_data->device_dispatch_table->CmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount,
                                                         pRegions, filter);
    }
}

VKAPI_ATTR void VKAPI_CALL
CmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                   VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
                   uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers,
                   uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);

    for (uint32_t i = 0; i < imageMemoryBarrierCount; ++i) {
        VkImageMemoryBarrier const *const barrier = (VkImageMemoryBarrier const *const) & pImageMemoryBarriers[i];
        if (barrier->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            if (barrier->subresourceRange.layerCount == 0) {
                std::stringstream ss;
                ss << "vkCmdPipelineBarrier called with 0 in ppMemoryBarriers[" << i << "]->subresourceRange.layerCount.";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                    __LINE__, IMAGE_INVALID_IMAGE_RESOURCE, "IMAGE", "%s", ss.str().c_str());
            }
        }
    }

    if (skipCall) {
        return;
    }

    device_data->device_dispatch_table->CmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags,
                                                           memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount,
                                                           pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

VKAPI_ATTR void VKAPI_CALL
CmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve *pRegions) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    auto srcImageEntry = getImageState(device_data, srcImage);
    auto dstImageEntry = getImageState(device_data, dstImage);

    // For each region, the number of layers in the image subresource should not be zero
    // For each region, src and dest image aspect must be color only
    for (uint32_t i = 0; i < regionCount; i++) {
        if (pRegions[i].srcSubresource.layerCount == 0) {
            char const str[] = "vkCmdResolveImage: number of layers in source subresource is zero";
            // TODO: Verify against Valid Use section of spec. Generally if something yield an undefined result, it's invalid/error
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", str);
        }

        if (pRegions[i].dstSubresource.layerCount == 0) {
            char const str[] = "vkCmdResolveImage: number of layers in destination subresource is zero";

            // TODO: Verify against Valid Use section of spec. Generally if something yield an undefined result, it's invalid/error
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", str);
        }

        if ((pRegions[i].srcSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) ||
            (pRegions[i].dstSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)) {
            char const str[] =
                "vkCmdResolveImage: src and dest aspectMasks for each region must specify only VK_IMAGE_ASPECT_COLOR_BIT";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", str);
        }
    }

    if (srcImageEntry && dstImageEntry) {
        if (srcImageEntry->format != dstImageEntry->format) {
            char const str[] = "vkCmdResolveImage called with unmatched source and dest formats.";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_MISMATCHED_IMAGE_FORMAT, "IMAGE", str);
        }
        if (srcImageEntry->imageType != dstImageEntry->imageType) {
            char const str[] = "vkCmdResolveImage called with unmatched source and dest image types.";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_MISMATCHED_IMAGE_TYPE, "IMAGE", str);
        }
        if (srcImageEntry->samples == VK_SAMPLE_COUNT_1_BIT) {
            char const str[] = "vkCmdResolveImage called with source sample count less than 2.";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_RESOLVE_SAMPLES, "IMAGE", str);
        }
        if (dstImageEntry->samples != VK_SAMPLE_COUNT_1_BIT) {
            char const str[] = "vkCmdResolveImage called with dest sample count greater than 1.";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_RESOLVE_SAMPLES, "IMAGE", str);
        }
    }

    if (!skipCall) {
        device_data->device_dispatch_table->CmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout,
                                                            regionCount, pRegions);
    }
}

VKAPI_ATTR void VKAPI_CALL
GetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource *pSubresource, VkSubresourceLayout *pLayout) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkFormat format;

    auto imageEntry = getImageState(device_data, image);

    // Validate that image aspects match formats
    if (imageEntry) {
        format = imageEntry->format;
        if (vk_format_is_color(format)) {
            if (pSubresource->aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) {
                std::stringstream ss;
                ss << "vkGetImageSubresourceLayout: For color formats, the aspectMask field of VkImageSubresource must be "
                      "VK_IMAGE_ASPECT_COLOR.";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                    (uint64_t)image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
        } else if (vk_format_is_depth_or_stencil(format)) {
            if ((pSubresource->aspectMask != VK_IMAGE_ASPECT_DEPTH_BIT) &&
                (pSubresource->aspectMask != VK_IMAGE_ASPECT_STENCIL_BIT)) {
                std::stringstream ss;
                ss << "vkGetImageSubresourceLayout: For depth/stencil formats, the aspectMask selects either the depth or stencil "
                      "image aspectMask.";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                    (uint64_t)image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
        }
    }

    if (!skipCall) {
        device_data->device_dispatch_table->GetImageSubresourceLayout(device, image, pSubresource, pLayout);
    }
}

VKAPI_ATTR void VKAPI_CALL
GetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties) {
    layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    phy_dev_data->instance_dispatch_table->GetPhysicalDeviceProperties(physicalDevice, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL
EnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &global_layer, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL
EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount, VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &global_layer, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL
EnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount, VkExtensionProperties *pProperties) {
    if (pLayerName && !strcmp(pLayerName, global_layer.layerName))
        return util_GetExtensionProperties(1, instance_extensions, pCount, pProperties);

    return VK_ERROR_LAYER_NOT_PRESENT;
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                                                  const char *pLayerName, uint32_t *pCount,
                                                                  VkExtensionProperties *pProperties) {
    // Image does not have any physical device extensions
    if (pLayerName && !strcmp(pLayerName, global_layer.layerName))
        return util_GetExtensionProperties(0, NULL, pCount, pProperties);

    assert(physicalDevice);

    dispatch_key key = get_dispatch_key(physicalDevice);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    return pTable->EnumerateDeviceExtensionProperties(physicalDevice, NULL, pCount, pProperties);
}

static PFN_vkVoidFunction
intercept_core_instance_command(const char *name);
static PFN_vkVoidFunction
intercept_core_device_command(const char *name);

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(VkDevice device, const char *funcName) {
    PFN_vkVoidFunction proc = intercept_core_device_command(funcName);
    if (proc)
        return proc;

    assert(device);

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    {
        if (pTable->GetDeviceProcAddr == NULL)
            return NULL;
        return pTable->GetDeviceProcAddr(device, funcName);
    }
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char *funcName) {
    PFN_vkVoidFunction proc = intercept_core_instance_command(funcName);
    if (!proc)
        proc = intercept_core_device_command(funcName);
    if (proc)
        return proc;

    assert(instance);
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    proc = debug_report_get_instance_proc_addr(my_data->report_data, funcName);
    if (proc)
        return proc;

    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    if (pTable->GetInstanceProcAddr == NULL)
        return NULL;
    return pTable->GetInstanceProcAddr(instance, funcName);
}

static PFN_vkVoidFunction
intercept_core_instance_command(const char *name) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } core_instance_commands[] = {
        { "vkGetInstanceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetInstanceProcAddr) },
        { "vkCreateInstance", reinterpret_cast<PFN_vkVoidFunction>(CreateInstance) },
        { "vkDestroyInstance", reinterpret_cast<PFN_vkVoidFunction>(DestroyInstance) },
        { "vkCreateDevice", reinterpret_cast<PFN_vkVoidFunction>(CreateDevice) },
        { "vkEnumerateInstanceLayerProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceLayerProperties) },
        { "vkEnumerateDeviceLayerProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateDeviceLayerProperties) },
        { "vkEnumerateInstanceExtensionProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceExtensionProperties) },
        { "vkEnumerateDeviceExtensionProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateDeviceExtensionProperties) },
        { "vkGetPhysicalDeviceProperties", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceProperties) },
    };

    for (size_t i = 0; i < ARRAY_SIZE(core_instance_commands); i++) {
        if (!strcmp(core_instance_commands[i].name, name))
            return core_instance_commands[i].proc;
    }

    return nullptr;
}

static PFN_vkVoidFunction
intercept_core_device_command(const char *name) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } core_device_commands[] = {
        { "vkGetDeviceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceProcAddr) },
        { "vkDestroyDevice", reinterpret_cast<PFN_vkVoidFunction>(DestroyDevice) },
        { "vkCreateImage", reinterpret_cast<PFN_vkVoidFunction>(CreateImage) },
        { "vkDestroyImage", reinterpret_cast<PFN_vkVoidFunction>(DestroyImage) },
        { "vkCreateImageView", reinterpret_cast<PFN_vkVoidFunction>(CreateImageView) },
        { "vkCreateRenderPass", reinterpret_cast<PFN_vkVoidFunction>(CreateRenderPass) },
        { "vkCmdClearColorImage", reinterpret_cast<PFN_vkVoidFunction>(CmdClearColorImage) },
        { "vkCmdClearDepthStencilImage", reinterpret_cast<PFN_vkVoidFunction>(CmdClearDepthStencilImage) },
        { "vkCmdClearAttachments", reinterpret_cast<PFN_vkVoidFunction>(CmdClearAttachments) },
        { "vkCmdCopyImage", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyImage) },
        { "vkCmdCopyImageToBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyImageToBuffer) },
        { "vkCmdCopyBufferToImage", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyBufferToImage) },
        { "vkCmdBlitImage", reinterpret_cast<PFN_vkVoidFunction>(CmdBlitImage) },
        { "vkCmdPipelineBarrier", reinterpret_cast<PFN_vkVoidFunction>(CmdPipelineBarrier) },
        { "vkCmdResolveImage", reinterpret_cast<PFN_vkVoidFunction>(CmdResolveImage) },
        { "vkGetImageSubresourceLayout", reinterpret_cast<PFN_vkVoidFunction>(GetImageSubresourceLayout) },
    };

    for (size_t i = 0; i < ARRAY_SIZE(core_device_commands); i++) {
        if (!strcmp(core_device_commands[i].name, name))
            return core_device_commands[i].proc;
    }

    return nullptr;
}

} // namespace image

// vk_layer_logging.h expects these to be defined

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                               const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pMsgCallback) {
    return image::CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyDebugReportCallbackEXT(VkInstance instance,
                                VkDebugReportCallbackEXT msgCallback,
                                const VkAllocationCallbacks *pAllocator) {
    image::DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL
vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t object,
                        size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    image::DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

// loader-layer interface v0, just wrappers since there is only a layer

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount, VkExtensionProperties *pProperties) {
    return image::EnumerateInstanceExtensionProperties(pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties) {
    return image::EnumerateInstanceLayerProperties(pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount, VkLayerProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return image::EnumerateDeviceLayerProperties(VK_NULL_HANDLE, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                                                                    const char *pLayerName, uint32_t *pCount,
                                                                                    VkExtensionProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return image::EnumerateDeviceExtensionProperties(VK_NULL_HANDLE, pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev, const char *funcName) {
    return image::GetDeviceProcAddr(dev, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *funcName) {
    return image::GetInstanceProcAddr(instance, funcName);
}
