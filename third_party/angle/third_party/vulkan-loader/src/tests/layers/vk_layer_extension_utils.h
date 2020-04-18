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
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 *
 */

#include "vulkan/vk_layer.h"

#ifndef LAYER_EXTENSION_UTILS_H
#define LAYER_EXTENSION_UTILS_H

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

/*
 * This file contains static functions for the generated layers
 */
extern "C" {

VK_LAYER_EXPORT VkResult util_GetExtensionProperties(const uint32_t count, const VkExtensionProperties *layer_extensions,
                                                     uint32_t *pCount, VkExtensionProperties *pProperties);

VK_LAYER_EXPORT VkResult util_GetLayerProperties(const uint32_t count, const VkLayerProperties *layer_properties, uint32_t *pCount,
                                                 VkLayerProperties *pProperties);

}  // extern "C"
#endif  // LAYER_EXTENSION_UTILS_H
