///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015-2016 The Khronos Group Inc.
// Copyright (c) 2015-2016 Valve Corporation
// Copyright (c) 2015-2016 LunarG, Inc.
// Copyright (c) 2015-2016 Google, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
///////////////////////////////////////////////////////////////////////////////

#ifndef VKJSON_H_
#define VKJSON_H_

#include <vulkan/vulkan.h>
#include <string.h>

#include <map>
#include <string>
#include <vector>

#ifdef WIN32
#undef min
#undef max
#endif

struct VkJsonLayer {
  VkLayerProperties properties;
  std::vector<VkExtensionProperties> extensions;
};

struct VkJsonDevice {
  VkJsonDevice() {
          memset(&properties, 0, sizeof(VkPhysicalDeviceProperties));
          memset(&features, 0, sizeof(VkPhysicalDeviceFeatures));
          memset(&memory, 0, sizeof(VkPhysicalDeviceMemoryProperties));
  }
  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
  VkPhysicalDeviceMemoryProperties memory;
  std::vector<VkQueueFamilyProperties> queues;
  std::vector<VkExtensionProperties> extensions;
  std::vector<VkLayerProperties> layers;
  std::map<VkFormat, VkFormatProperties> formats;
};

struct VkJsonInstance {
  std::vector<VkJsonLayer> layers;
  std::vector<VkExtensionProperties> extensions;
  std::vector<VkJsonDevice> devices;
};

VkJsonInstance VkJsonGetInstance();
std::string VkJsonInstanceToJson(const VkJsonInstance& instance);
bool VkJsonInstanceFromJson(const std::string& json,
                            VkJsonInstance* instance,
                            std::string* errors);

VkJsonDevice VkJsonGetDevice(VkPhysicalDevice device);
std::string VkJsonDeviceToJson(const VkJsonDevice& device);
bool VkJsonDeviceFromJson(const std::string& json,
                          VkJsonDevice* device,
                          std::string* errors);

std::string VkJsonImageFormatPropertiesToJson(
    const VkImageFormatProperties& properties);
bool VkJsonImageFormatPropertiesFromJson(const std::string& json,
                                         VkImageFormatProperties* properties,
                                         std::string* errors);

// Backward-compatibility aliases
typedef VkJsonDevice VkJsonAllProperties;
inline VkJsonAllProperties VkJsonGetAllProperties(
    VkPhysicalDevice physicalDevice) {
  return VkJsonGetDevice(physicalDevice);
}
inline std::string VkJsonAllPropertiesToJson(
    const VkJsonAllProperties& properties) {
  return VkJsonDeviceToJson(properties);
}
inline bool VkJsonAllPropertiesFromJson(const std::string& json,
                                        VkJsonAllProperties* properties,
                                        std::string* errors) {
  return VkJsonDeviceFromJson(json, properties, errors);
}

#endif  // VKJSON_H_
