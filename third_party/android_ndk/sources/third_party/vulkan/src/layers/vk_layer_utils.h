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
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 */

#pragma once
#include <stdbool.h>
#include <vector>
#include "vk_layer_logging.h"

#ifndef WIN32
#include <strings.h> /* for ffs() */
#else
#include <intrin.h> /* for __lzcnt() */
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define VK_LAYER_API_VERSION VK_MAKE_VERSION(1, 0, VK_HEADER_VERSION)
typedef enum VkFormatCompatibilityClass {
    VK_FORMAT_COMPATIBILITY_CLASS_NONE_BIT = 0,
    VK_FORMAT_COMPATIBILITY_CLASS_8_BIT = 1,
    VK_FORMAT_COMPATIBILITY_CLASS_16_BIT = 2,
    VK_FORMAT_COMPATIBILITY_CLASS_24_BIT = 3,
    VK_FORMAT_COMPATIBILITY_CLASS_32_BIT = 4,
    VK_FORMAT_COMPATIBILITY_CLASS_48_BIT = 5,
    VK_FORMAT_COMPATIBILITY_CLASS_64_BIT = 6,
    VK_FORMAT_COMPATIBILITY_CLASS_96_BIT = 7,
    VK_FORMAT_COMPATIBILITY_CLASS_128_BIT = 8,
    VK_FORMAT_COMPATIBILITY_CLASS_192_BIT = 9,
    VK_FORMAT_COMPATIBILITY_CLASS_256_BIT = 10,
    VK_FORMAT_COMPATIBILITY_CLASS_BC1_RGB_BIT = 11,
    VK_FORMAT_COMPATIBILITY_CLASS_BC1_RGBA_BIT = 12,
    VK_FORMAT_COMPATIBILITY_CLASS_BC2_BIT = 13,
    VK_FORMAT_COMPATIBILITY_CLASS_BC3_BIT = 14,
    VK_FORMAT_COMPATIBILITY_CLASS_BC4_BIT = 15,
    VK_FORMAT_COMPATIBILITY_CLASS_BC5_BIT = 16,
    VK_FORMAT_COMPATIBILITY_CLASS_BC6H_BIT = 17,
    VK_FORMAT_COMPATIBILITY_CLASS_BC7_BIT = 18,
    VK_FORMAT_COMPATIBILITY_CLASS_ETC2_RGB_BIT = 19,
    VK_FORMAT_COMPATIBILITY_CLASS_ETC2_RGBA_BIT = 20,
    VK_FORMAT_COMPATIBILITY_CLASS_ETC2_EAC_RGBA_BIT = 21,
    VK_FORMAT_COMPATIBILITY_CLASS_EAC_R_BIT = 22,
    VK_FORMAT_COMPATIBILITY_CLASS_EAC_RG_BIT = 23,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_4X4_BIT = 24,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_5X4_BIT = 25,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_5X5_BIT = 26,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_6X5_BIT = 27,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_6X6_BIT = 28,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_8X5_BIT = 29,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_8X6_BIT = 20,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_8X8_BIT = 31,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X5_BIT = 32,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X6_BIT = 33,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X8_BIT = 34,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X10_BIT = 35,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_12X10_BIT = 36,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_12X12_BIT = 37,
    VK_FORMAT_COMPATIBILITY_CLASS_D16_BIT = 38,
    VK_FORMAT_COMPATIBILITY_CLASS_D24_BIT = 39,
    VK_FORMAT_COMPATIBILITY_CLASS_D32_BIT = 30,
    VK_FORMAT_COMPATIBILITY_CLASS_S8_BIT = 41,
    VK_FORMAT_COMPATIBILITY_CLASS_D16S8_BIT = 42,
    VK_FORMAT_COMPATIBILITY_CLASS_D24S8_BIT = 43,
    VK_FORMAT_COMPATIBILITY_CLASS_D32S8_BIT = 44,
    VK_FORMAT_COMPATIBILITY_CLASS_MAX_ENUM = 45
} VkFormatCompatibilityClass;

typedef enum VkStringErrorFlagBits {
    VK_STRING_ERROR_NONE = 0x00000000,
    VK_STRING_ERROR_LENGTH = 0x00000001,
    VK_STRING_ERROR_BAD_DATA = 0x00000002,
} VkStringErrorFlagBits;
typedef VkFlags VkStringErrorFlags;

VK_LAYER_EXPORT void layer_debug_actions(debug_report_data *report_data, std::vector<VkDebugReportCallbackEXT> &logging_callback,
                                         const VkAllocationCallbacks *pAllocator, const char *layer_identifier);

static inline bool vk_format_is_undef(VkFormat format) { return (format == VK_FORMAT_UNDEFINED); }

bool vk_format_is_depth_or_stencil(VkFormat format);
bool vk_format_is_depth_and_stencil(VkFormat format);
bool vk_format_is_depth_only(VkFormat format);
bool vk_format_is_stencil_only(VkFormat format);

static inline bool vk_format_is_color(VkFormat format) {
    return !(vk_format_is_undef(format) || vk_format_is_depth_or_stencil(format));
}

VK_LAYER_EXPORT bool vk_format_is_norm(VkFormat format);
VK_LAYER_EXPORT bool vk_format_is_int(VkFormat format);
VK_LAYER_EXPORT bool vk_format_is_sint(VkFormat format);
VK_LAYER_EXPORT bool vk_format_is_uint(VkFormat format);
VK_LAYER_EXPORT bool vk_format_is_float(VkFormat format);
VK_LAYER_EXPORT bool vk_format_is_srgb(VkFormat format);
VK_LAYER_EXPORT bool vk_format_is_compressed(VkFormat format);
VK_LAYER_EXPORT VkExtent2D vk_format_compressed_block_size(VkFormat format);
VK_LAYER_EXPORT size_t vk_format_get_size(VkFormat format);
VK_LAYER_EXPORT unsigned int vk_format_get_channel_count(VkFormat format);
VK_LAYER_EXPORT VkFormatCompatibilityClass vk_format_get_compatibility_class(VkFormat format);
VK_LAYER_EXPORT VkDeviceSize vk_safe_modulo(VkDeviceSize dividend, VkDeviceSize divisor);
VK_LAYER_EXPORT VkStringErrorFlags vk_string_validate(const int max_length, const char *char_array);
VK_LAYER_EXPORT bool white_list(const char *item, const char *whitelist);

static inline int u_ffs(int val) {
#ifdef WIN32
    unsigned long bit_pos = 0;
    if (_BitScanForward(&bit_pos, val) != 0) {
        bit_pos += 1;
    }
    return bit_pos;
#else
    return ffs(val);
#endif
}

#ifdef __cplusplus
}
#endif
