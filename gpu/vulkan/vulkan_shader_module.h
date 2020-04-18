// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_VULKAN_VULKAN_SHADER_MODULE_H_
#define GPU_VULKAN_VULKAN_SHADER_MODULE_H_

#include <string>
#include <vulkan/vulkan.h>

#include "base/macros.h"
#include "gpu/vulkan/vulkan_export.h"

namespace gpu {

class VulkanDeviceQueue;

class VULKAN_EXPORT VulkanShaderModule {
 public:
  enum class ShaderType {
    VERTEX,
    FRAGMENT,
  };

  explicit VulkanShaderModule(VulkanDeviceQueue* device_queue);
  ~VulkanShaderModule();

  bool InitializeGLSL(ShaderType type,
                      std::string name,
                      std::string entry_point,
                      std::string source);
  bool InitializeSPIRV(ShaderType type,
                       std::string name,
                       std::string entry_point,
                       std::string source);
  void Destroy();

  bool IsValid() const { return handle_ != VK_NULL_HANDLE; }
  std::string GetErrorMessages() const { return error_messages_; }

  ShaderType shader_type() const { return shader_type_; }
  const std::string& name() const { return name_; }
  VkShaderModule handle() const { return handle_; }
  const std::string& entry_point() const { return entry_point_; }

 private:
  VulkanDeviceQueue* device_queue_ = nullptr;
  ShaderType shader_type_ = ShaderType::VERTEX;
  VkShaderModule handle_ = VK_NULL_HANDLE;
  std::string name_;
  std::string entry_point_;
  std::string error_messages_;

  DISALLOW_COPY_AND_ASSIGN(VulkanShaderModule);
};

}  // namespace gpu

#endif  // GPU_VULKAN_VULKAN_SHADER_MODULE_H_
