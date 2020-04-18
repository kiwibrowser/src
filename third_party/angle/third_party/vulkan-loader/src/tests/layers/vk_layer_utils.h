/* Copyright (c) 2015-2017 The Khronos Group Inc.
 * Copyright (c) 2015-2017 Valve Corporation
 * Copyright (c) 2015-2017 LunarG, Inc.
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
 * Author: Dave Houlton <daveh@lunarg.com>
 */

#pragma once
#include <stdbool.h>
#include <string>
#include <vector>
#include "vk_format_utils.h"
#include "vk_layer_logging.h"

#ifndef WIN32
#include <strings.h>  // For ffs()
#else
#include <intrin.h>  // For __lzcnt()
#endif

#ifdef __cplusplus
// Traits objects to allow string_join to operate on collections of const char *
template <typename String>
struct StringJoinSizeTrait {
    static size_t size(const String &str) { return str.size(); }
};

template <>
struct StringJoinSizeTrait<const char *> {
    static size_t size(const char *str) {
        if (!str) return 0;
        return strlen(str);
    }
};
// Similar to perl/python join
//    * String must support size, reserve, append, and be default constructable
//    * StringCollection must support size, const forward iteration, and store
//      strings compatible with String::append
//    * Accessor trait can be set if default accessors (compatible with string
//      and const char *) don't support size(StringCollection::value_type &)
//
// Return type based on sep type
template <typename String = std::string, typename StringCollection = std::vector<String>,
          typename Accessor = StringJoinSizeTrait<typename StringCollection::value_type>>
static inline String string_join(const String &sep, const StringCollection &strings) {
    String joined;
    const size_t count = strings.size();
    if (!count) return joined;

    // Prereserved storage, s.t. we will execute in linear time (avoids reallocation copies)
    size_t reserve = (count - 1) * sep.size();
    for (const auto &str : strings) {
        reserve += Accessor::size(str);  // abstracted to allow const char * type in StringCollection
    }
    joined.reserve(reserve + 1);

    // Seps only occur *between* strings entries, so first is special
    auto current = strings.cbegin();
    joined.append(*current);
    ++current;
    for (; current != strings.cend(); ++current) {
        joined.append(sep);
        joined.append(*current);
    }
    return joined;
}

// Requires StringCollection::value_type has a const char * constructor and is compatible the string_join::String above
template <typename StringCollection = std::vector<std::string>, typename SepString = std::string>
static inline SepString string_join(const char *sep, const StringCollection &strings) {
    return string_join<SepString, StringCollection>(SepString(sep), strings);
}

extern "C" {
#endif

#define VK_LAYER_API_VERSION VK_MAKE_VERSION(1, 0, VK_HEADER_VERSION)

typedef enum VkStringErrorFlagBits {
    VK_STRING_ERROR_NONE = 0x00000000,
    VK_STRING_ERROR_LENGTH = 0x00000001,
    VK_STRING_ERROR_BAD_DATA = 0x00000002,
} VkStringErrorFlagBits;
typedef VkFlags VkStringErrorFlags;

VK_LAYER_EXPORT void layer_debug_report_actions(debug_report_data *report_data,
                                                std::vector<VkDebugReportCallbackEXT> &logging_callback,
                                                const VkAllocationCallbacks *pAllocator, const char *layer_identifier);

VK_LAYER_EXPORT void layer_debug_messenger_actions(debug_report_data *report_data,
                                                   std::vector<VkDebugUtilsMessengerEXT> &logging_messenger,
                                                   const VkAllocationCallbacks *pAllocator, const char *layer_identifier);

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
