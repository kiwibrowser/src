/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
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
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Mike Stroyan <mike@LunarG.com>
 * Author: Tobin Ehlis <tobin@lunarg.com>
 */

#ifndef IMAGE_H
#define IMAGE_H
#include "vulkan/vulkan.h"
#include "vk_layer_config.h"
#include "vk_layer_logging.h"

// Image ERROR codes
enum IMAGE_ERROR {
    IMAGE_NONE,                             // Used for INFO & other non-error messages
    IMAGE_FORMAT_UNSUPPORTED,               // Request to create Image or RenderPass with a format that is not supported
    IMAGE_RENDERPASS_INVALID_ATTACHMENT,    // Invalid image layouts and/or load/storeOps for an attachment when creating RenderPass
    IMAGE_RENDERPASS_INVALID_DS_ATTACHMENT, // If no depth/stencil attachment for a RenderPass, verify that subpass DS attachment
                                            // is set to UNUSED
    IMAGE_INVALID_IMAGE_ASPECT,             // Image aspect mask bits are invalid for this API call
    IMAGE_MISMATCHED_IMAGE_ASPECT,          // Image aspect masks for source and dest images do not match
    IMAGE_VIEW_CREATE_ERROR,                // Error occurred trying to create Image View
    IMAGE_MISMATCHED_IMAGE_TYPE,            // Image types for source and dest images do not match
    IMAGE_MISMATCHED_IMAGE_FORMAT,          // Image formats for source and dest images do not match
    IMAGE_INVALID_RESOLVE_SAMPLES,          // Image resolve source samples less than two or dest samples greater than one
    IMAGE_INVALID_FORMAT,                   // Operation specifies an invalid format, or there is a format mismatch
    IMAGE_INVALID_FILTER,                   // Operation specifies an invalid filter setting
    IMAGE_INVALID_IMAGE_RESOURCE,           // Image resource/subresource called with invalid setting
    IMAGE_INVALID_FORMAT_LIMITS_VIOLATION,  // Device limits for this format have been exceeded
    IMAGE_INVALID_LAYOUT,                   // Operation specifies an invalid layout
    IMAGE_INVALID_EXTENTS,                  // Operation specifies invalid image extents
    IMAGE_INVALID_USAGE,                    // Image was created without necessary usage for operation
};

struct IMAGE_STATE {
    uint32_t mipLevels;
    uint32_t arraySize;
    VkFormat format;
    VkSampleCountFlagBits samples;
    VkImageType imageType;
    VkExtent3D extent;
    VkImageCreateFlags flags;
    VkImageUsageFlags usage;
    IMAGE_STATE()
        : mipLevels(0), arraySize(0), format(VK_FORMAT_UNDEFINED), samples(VK_SAMPLE_COUNT_1_BIT),
          imageType(VK_IMAGE_TYPE_RANGE_SIZE), extent{}, flags(0), usage(0){};
    IMAGE_STATE(const VkImageCreateInfo *pCreateInfo)
        : mipLevels(pCreateInfo->mipLevels), arraySize(pCreateInfo->arrayLayers), format(pCreateInfo->format),
          samples(pCreateInfo->samples), imageType(pCreateInfo->imageType), extent(pCreateInfo->extent), flags(pCreateInfo->flags),
          usage(pCreateInfo->usage){};
};

#endif // IMAGE_H
