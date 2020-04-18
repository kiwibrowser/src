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
 * Author: Tobin Ehlis <tobin@lunarg.com>
 */

#pragma once

#include "vulkan/vk_layer.h"
#include "vulkan/vulkan.h"
#include <unordered_map>

typedef std::unordered_map<void *, VkLayerDispatchTable *> device_table_map;
typedef std::unordered_map<void *, VkLayerInstanceDispatchTable *> instance_table_map;
VkLayerDispatchTable *initDeviceTable(VkDevice device, const PFN_vkGetDeviceProcAddr gpa, device_table_map &map);
VkLayerDispatchTable *initDeviceTable(VkDevice device, const PFN_vkGetDeviceProcAddr gpa);
VkLayerInstanceDispatchTable *initInstanceTable(VkInstance instance, const PFN_vkGetInstanceProcAddr gpa, instance_table_map &map);
VkLayerInstanceDispatchTable *initInstanceTable(VkInstance instance, const PFN_vkGetInstanceProcAddr gpa);

typedef void *dispatch_key;

static inline dispatch_key get_dispatch_key(const void *object) { return (dispatch_key) * (VkLayerDispatchTable **)object; }

VkLayerDispatchTable *device_dispatch_table(void *object);

VkLayerInstanceDispatchTable *instance_dispatch_table(void *object);

VkLayerDispatchTable *get_dispatch_table(device_table_map &map, void *object);

VkLayerInstanceDispatchTable *get_dispatch_table(instance_table_map &map, void *object);

VkLayerInstanceCreateInfo *get_chain_info(const VkInstanceCreateInfo *pCreateInfo, VkLayerFunction func);
VkLayerDeviceCreateInfo *get_chain_info(const VkDeviceCreateInfo *pCreateInfo, VkLayerFunction func);

void destroy_device_dispatch_table(dispatch_key key);
void destroy_instance_dispatch_table(dispatch_key key);
void destroy_dispatch_table(device_table_map &map, dispatch_key key);
void destroy_dispatch_table(instance_table_map &map, dispatch_key key);
