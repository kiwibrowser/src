// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/vulkan/vulkan_instance.h"

#include <unordered_set>
#include <vector>
#include "base/logging.h"
#include "base/macros.h"
#include "gpu/vulkan/vulkan_device_queue.h"

namespace gpu {

VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanErrorCallback(VkDebugReportFlagsEXT flags,
                    VkDebugReportObjectTypeEXT objectType,
                    uint64_t object,
                    size_t location,
                    int32_t messageCode,
                    const char* pLayerPrefix,
                    const char* pMessage,
                    void* pUserData) {
  LOG(ERROR) << pMessage;
  return VK_TRUE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanWarningCallback(VkDebugReportFlagsEXT flags,
                      VkDebugReportObjectTypeEXT objectType,
                      uint64_t object,
                      size_t location,
                      int32_t messageCode,
                      const char* pLayerPrefix,
                      const char* pMessage,
                      void* pUserData) {
  LOG(WARNING) << pMessage;
  return VK_TRUE;
}

VulkanInstance::VulkanInstance() {}

VulkanInstance::~VulkanInstance() {
  Destroy();
}

bool VulkanInstance::Initialize(
    const std::vector<const char*>& required_extensions) {
  DCHECK(!vk_instance_);

  VkResult result = VK_SUCCESS;

  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Chromium";
  app_info.apiVersion = VK_MAKE_VERSION(1, 0, 2);

  std::vector<const char*> enabled_ext_names;
  enabled_ext_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
  enabled_ext_names.insert(std::end(enabled_ext_names),
                           std::begin(required_extensions),
                           std::end(required_extensions));

  uint32_t num_instance_exts = 0;
  result = vkEnumerateInstanceExtensionProperties(nullptr, &num_instance_exts,
                                                  nullptr);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkEnumerateInstanceExtensionProperties(NULL) failed: "
                << result;
    return false;
  }

  std::vector<VkExtensionProperties> instance_exts(num_instance_exts);
  result = vkEnumerateInstanceExtensionProperties(nullptr, &num_instance_exts,
                                                  instance_exts.data());
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkEnumerateInstanceExtensionProperties() failed: "
                << result;
    return false;
  }

  for (const VkExtensionProperties& ext_property : instance_exts) {
    if (strcmp(ext_property.extensionName,
               VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0) {
      debug_report_enabled_ = true;
      enabled_ext_names.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }
  }

  std::vector<const char*> enabled_layer_names;
#if DCHECK_IS_ON()
  uint32_t num_instance_layers = 0;
  result = vkEnumerateInstanceLayerProperties(&num_instance_layers, nullptr);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkEnumerateInstanceLayerProperties(NULL) failed: "
                << result;
    return false;
  }

  std::vector<VkLayerProperties> instance_layers(num_instance_layers);
  result = vkEnumerateInstanceLayerProperties(&num_instance_layers,
                                              instance_layers.data());
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkEnumerateInstanceLayerProperties() failed: " << result;
    return false;
  }

  std::unordered_set<std::string> desired_layers({
#if !defined(USE_X11) && !defined(USE_OZONE)
    // TODO(crbug.com/843346): Make validation work in combination with
    // VK_KHR_xlib_surface or switch to VK_KHR_xcb_surface.
    "VK_LAYER_LUNARG_standard_validation",
#endif
  });

  for (const VkLayerProperties& layer_property : instance_layers) {
    if (desired_layers.find(layer_property.layerName) != desired_layers.end())
      enabled_layer_names.push_back(layer_property.layerName);
  }
#endif

  VkInstanceCreateInfo instance_create_info = {};
  instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_create_info.pApplicationInfo = &app_info;
  instance_create_info.enabledLayerCount = enabled_layer_names.size();
  instance_create_info.ppEnabledLayerNames = enabled_layer_names.data();
  instance_create_info.enabledExtensionCount = enabled_ext_names.size();
  instance_create_info.ppEnabledExtensionNames = enabled_ext_names.data();

  result = vkCreateInstance(&instance_create_info, nullptr, &vk_instance_);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkCreateInstance() failed: " << result;
    return false;
  }

#if DCHECK_IS_ON()
  // Register our error logging function.
  if (debug_report_enabled_) {
    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT =
        reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
            vkGetInstanceProcAddr(vk_instance_,
                                  "vkCreateDebugReportCallbackEXT"));
    DCHECK(vkCreateDebugReportCallbackEXT);

    VkDebugReportCallbackCreateInfoEXT cb_create_info = {};
    cb_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;

    cb_create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT;
    cb_create_info.pfnCallback = &VulkanErrorCallback;
    result = vkCreateDebugReportCallbackEXT(vk_instance_, &cb_create_info,
                                            nullptr, &error_callback_);
    if (VK_SUCCESS != result) {
      DLOG(ERROR) << "vkCreateDebugReportCallbackEXT(ERROR) failed: " << result;
      return false;
    }

    cb_create_info.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT |
                           VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    cb_create_info.pfnCallback = &VulkanWarningCallback;
    result = vkCreateDebugReportCallbackEXT(vk_instance_, &cb_create_info,
                                            nullptr, &warning_callback_);
    if (VK_SUCCESS != result) {
      DLOG(ERROR) << "vkCreateDebugReportCallbackEXT(WARN) failed: " << result;
      return false;
    }
  }
#endif

  return true;
}

void VulkanInstance::Destroy() {
#if DCHECK_IS_ON()
  if (debug_report_enabled_) {
    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT =
        reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
            vkGetInstanceProcAddr(vk_instance_,
                                  "vkDestroyDebugReportCallbackEXT"));
    DCHECK(vkDestroyDebugReportCallbackEXT);
    vkDestroyDebugReportCallbackEXT(vk_instance_, error_callback_, nullptr);
    vkDestroyDebugReportCallbackEXT(vk_instance_, warning_callback_, nullptr);
  }
#endif
  vkDestroyInstance(vk_instance_, nullptr);
  vk_instance_ = VK_NULL_HANDLE;
}

}  // namespace gpu
