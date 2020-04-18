// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/vulkan/tests/basic_vulkan_test.h"
#include "gpu/vulkan/vulkan_shader_module.h"

// This file tests basic shader module functionality.

namespace gpu {

class ShaderModuleTest : public BasicVulkanTest {};

TEST_F(ShaderModuleTest, BasicGLSLVertexShader) {
  const VulkanShaderModule::ShaderType kShaderType =
      VulkanShaderModule::ShaderType::VERTEX;
  const std::string kShaderName = "Basic Vertex Shader";
  const std::string kShaderEntry = "main";
  const std::string kShaderSource =
      "#version 150\n"
      "void main() {\n"
      "  gl_Position = vec4(1.0, 1.0, 1.0, 1.0);\n"
      "}";

  VulkanShaderModule vertex_shader_module(GetDeviceQueue());
  EXPECT_TRUE(vertex_shader_module.InitializeGLSL(kShaderType, kShaderName,
                                                  kShaderEntry, kShaderSource));
  EXPECT_TRUE(vertex_shader_module.IsValid());
  EXPECT_EQ(kShaderType, vertex_shader_module.shader_type());
  EXPECT_EQ(kShaderName, vertex_shader_module.name());
  EXPECT_EQ(kShaderEntry, vertex_shader_module.entry_point());
  vertex_shader_module.Destroy();

  EXPECT_FALSE(vertex_shader_module.IsValid());
}

TEST_F(ShaderModuleTest, BasicGLSLFragmentShader) {
  const VulkanShaderModule::ShaderType kShaderType =
      VulkanShaderModule::ShaderType::FRAGMENT;
  const std::string kShaderName = "Basic Fragment Shader";
  const std::string kShaderEntry = "main";
  const std::string kShaderSource =
      "#version 150\n"
      "out vec4 colorOut;\n"
      "void main() {\n"
      "  colorOut = vec4(1.0, 1.0, 1.0, 1.0);\n"
      "}";

  VulkanShaderModule frag_shader_module(GetDeviceQueue());
  EXPECT_TRUE(frag_shader_module.InitializeGLSL(kShaderType, kShaderName,
                                                kShaderEntry, kShaderSource));
  EXPECT_TRUE(frag_shader_module.IsValid());
  EXPECT_EQ(kShaderType, frag_shader_module.shader_type());
  EXPECT_EQ(kShaderName, frag_shader_module.name());
  EXPECT_EQ(kShaderEntry, frag_shader_module.entry_point());
  frag_shader_module.Destroy();

  EXPECT_FALSE(frag_shader_module.IsValid());
}

TEST_F(ShaderModuleTest, BasicGLSLError) {
  const VulkanShaderModule::ShaderType kShaderType =
      VulkanShaderModule::ShaderType::VERTEX;
  const std::string kShaderName = "Basic Fragment Shader";
  const std::string kShaderEntry = "main";
  const std::string kShaderSource =
      "#version 150\n"
      "void main() {\n"
      "  typo\n"
      "}";

  VulkanShaderModule vertex_shader_module(GetDeviceQueue());
  EXPECT_FALSE(vertex_shader_module.InitializeGLSL(
      kShaderType, kShaderName, kShaderEntry, kShaderSource));
  EXPECT_FALSE(vertex_shader_module.IsValid());
  EXPECT_FALSE(vertex_shader_module.GetErrorMessages().empty());
  vertex_shader_module.Destroy();
}

}  // namespace gpu
