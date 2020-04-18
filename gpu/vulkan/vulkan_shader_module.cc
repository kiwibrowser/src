// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/vulkan/vulkan_shader_module.h"

#include <memory>
#include <shaderc/shaderc.h>
#include <sstream>

#include "base/logging.h"
#include "gpu/vulkan/vulkan_device_queue.h"

namespace {

class ShaderCCompiler {
 public:
  class CompilationResult {
   public:
    explicit CompilationResult(shaderc_compilation_result_t compilation_result)
        : compilation_result_(compilation_result) {}

    ~CompilationResult() { shaderc_result_release(compilation_result_); }

    bool IsValid() const {
      return shaderc_compilation_status_success ==
             shaderc_result_get_compilation_status(compilation_result_);
    }

    std::string GetErrors() const {
      return shaderc_result_get_error_message(compilation_result_);
    }

    std::string GetResult() const {
      return std::string(shaderc_result_get_bytes(compilation_result_),
                         shaderc_result_get_length(compilation_result_));
    }

   private:
    shaderc_compilation_result_t compilation_result_;
  };

  ShaderCCompiler()
      : compiler_(shaderc_compiler_initialize()),
        compiler_options_(shaderc_compile_options_initialize()) {}

  ~ShaderCCompiler() { shaderc_compiler_release(compiler_); }

  void AddMacroDef(const std::string& name, const std::string& value) {
    shaderc_compile_options_add_macro_definition(compiler_options_,
                                                 name.c_str(), name.length(),
                                                 value.c_str(), value.length());
  }

  std::unique_ptr<ShaderCCompiler::CompilationResult> CompileShaderModule(
      gpu::VulkanShaderModule::ShaderType shader_type,
      const std::string& name,
      const std::string& entry_point,
      const std::string& source) {
    return std::make_unique<ShaderCCompiler::CompilationResult>(
        shaderc_compile_into_spv(
            compiler_, source.c_str(), source.length(),
            (shader_type == gpu::VulkanShaderModule::ShaderType::VERTEX
                 ? shaderc_glsl_vertex_shader
                 : shaderc_glsl_fragment_shader),
            name.c_str(), entry_point.c_str(), compiler_options_));
  }

 private:
  shaderc_compiler_t compiler_;
  shaderc_compile_options_t compiler_options_;
};

}  // namespace

namespace gpu {

VulkanShaderModule::VulkanShaderModule(VulkanDeviceQueue* device_queue)
    : device_queue_(device_queue) {
  DCHECK(device_queue_);
}

VulkanShaderModule::~VulkanShaderModule() {
  DCHECK_EQ(static_cast<VkShaderModule>(VK_NULL_HANDLE), handle_);
}

bool VulkanShaderModule::InitializeGLSL(ShaderType type,
                                        std::string name,
                                        std::string entry_point,
                                        std::string source) {
  ShaderCCompiler shaderc_compiler;
  std::unique_ptr<ShaderCCompiler::CompilationResult> compilation_result(
      shaderc_compiler.CompileShaderModule(type, name, entry_point, source));

  if (!compilation_result->IsValid()) {
    error_messages_ = compilation_result->GetErrors();
    return false;
  }

  return InitializeSPIRV(type, std::move(name), std::move(entry_point),
                         compilation_result->GetResult());
}

bool VulkanShaderModule::InitializeSPIRV(ShaderType type,
                                         std::string name,
                                         std::string entry_point,
                                         std::string source) {
  DCHECK_EQ(static_cast<VkShaderModule>(VK_NULL_HANDLE), handle_);
  shader_type_ = type;
  name_ = std::move(name);
  entry_point_ = std::move(entry_point);

  // Make sure source is a multiple of 4.
  const int padding = 4 - (source.length() % 4);
  if (padding < 4) {
    for (int i = 0; i < padding; ++i) {
      source += ' ';
    }
  }

  VkShaderModuleCreateInfo shader_module_create_info = {};
  shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shader_module_create_info.pCode =
      reinterpret_cast<const uint32_t*>(source.c_str());
  shader_module_create_info.codeSize = source.length();

  VkShaderModule shader_module = VK_NULL_HANDLE;
  VkResult result =
      vkCreateShaderModule(device_queue_->GetVulkanDevice(),
                           &shader_module_create_info, nullptr, &shader_module);
  if (VK_SUCCESS != result) {
    std::stringstream ss;
    ss << "vkCreateShaderModule() failed: " << result;
    error_messages_ = ss.str();
    DLOG(ERROR) << error_messages_;
    return false;
  }

  handle_ = shader_module;
  return true;
}

void VulkanShaderModule::Destroy() {
  if (handle_ != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device_queue_->GetVulkanDevice(), handle_, nullptr);
    handle_ = VK_NULL_HANDLE;
  }

  entry_point_.clear();
  error_messages_.clear();
}

}  // namespace gpu
