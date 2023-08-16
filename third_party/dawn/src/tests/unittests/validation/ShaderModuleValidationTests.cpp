// Copyright 2018 The Dawn Authors
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

#include "common/Constants.h"

#include "tests/unittests/validation/ValidationTest.h"

#include "utils/WGPUHelpers.h"

#include <sstream>

class ShaderModuleValidationTest : public ValidationTest {};

// Test case with a simpler shader that should successfully be created
TEST_F(ShaderModuleValidationTest, CreationSuccess) {
    const char* shader = R"(
                   OpCapability Shader
              %1 = OpExtInstImport "GLSL.std.450"
                   OpMemoryModel Logical GLSL450
                   OpEntryPoint Fragment %main "main" %fragColor
                   OpExecutionMode %main OriginUpperLeft
                   OpSource GLSL 450
                   OpSourceExtension "GL_GOOGLE_cpp_style_line_directive"
                   OpSourceExtension "GL_GOOGLE_include_directive"
                   OpName %main "main"
                   OpName %fragColor "fragColor"
                   OpDecorate %fragColor Location 0
           %void = OpTypeVoid
              %3 = OpTypeFunction %void
          %float = OpTypeFloat 32
        %v4float = OpTypeVector %float 4
    %_ptr_Output_v4float = OpTypePointer Output %v4float
      %fragColor = OpVariable %_ptr_Output_v4float Output
        %float_1 = OpConstant %float 1
        %float_0 = OpConstant %float 0
             %12 = OpConstantComposite %v4float %float_1 %float_0 %float_0 %float_1
           %main = OpFunction %void None %3
              %5 = OpLabel
                   OpStore %fragColor %12
                   OpReturn
                   OpFunctionEnd)";

    utils::CreateShaderModuleFromASM(device, shader);
}

// Test case with a shader with OpUndef to test WebGPU-specific validation
// TODO(cwallez@chromium.org): Disabled because of
// https://bugs.chromium.org/p/dawn/issues/detail?id=57
TEST_F(ShaderModuleValidationTest, DISABLED_OpUndef) {
    const char* shader = R"(
                   OpCapability Shader
              %1 = OpExtInstImport "GLSL.std.450"
                   OpMemoryModel Logical GLSL450
                   OpEntryPoint Fragment %main "main" %fragColor
                   OpExecutionMode %main OriginUpperLeft
                   OpSource GLSL 450
                   OpSourceExtension "GL_GOOGLE_cpp_style_line_directive"
                   OpSourceExtension "GL_GOOGLE_include_directive"
                   OpName %main "main"
                   OpName %fragColor "fragColor"
                   OpDecorate %fragColor Location 0
           %void = OpTypeVoid
              %3 = OpTypeFunction %void
          %float = OpTypeFloat 32
        %v4float = OpTypeVector %float 4
    %_ptr_Output_v4float = OpTypePointer Output %v4float
      %fragColor = OpVariable %_ptr_Output_v4float Output
        %float_1 = OpConstant %float 1
        %float_0 = OpConstant %float 0
             %12 = OpConstantComposite %v4float %float_1 %float_0 %float_0 %float_1
           %main = OpFunction %void None %3
              %5 = OpLabel
              %6 = OpUndef %v4float
                   OpStore %fragColor %12
                   OpReturn
                   OpFunctionEnd)";

    // Notice "%6 = OpUndef %v4float" above
    ASSERT_DEVICE_ERROR(utils::CreateShaderModuleFromASM(device, shader));

    std::string error = GetLastDeviceErrorMessage();
    ASSERT_NE(error.find("OpUndef"), std::string::npos);
}

// Tests that if the output location exceeds kMaxColorAttachments the fragment shader will fail to
// be compiled.
TEST_F(ShaderModuleValidationTest, FragmentOutputLocationExceedsMaxColorAttachments) {
    std::ostringstream stream;
    stream << R"(#version 450
              layout(location = )"
           << kMaxColorAttachments << R"() out vec4 fragColor;
              void main() {
                  fragColor = vec4(0.0, 1.0, 0.0, 1.0);
              })";

    ASSERT_DEVICE_ERROR(utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment,
                                                  stream.str().c_str()));
}

// Test that it is invalid to create a shader module with no chained descriptor. (It must be
// WGSL or SPIRV, not empty)
TEST_F(ShaderModuleValidationTest, NoChainedDescriptor) {
    wgpu::ShaderModuleDescriptor desc = {};
    ASSERT_DEVICE_ERROR(device.CreateShaderModule(&desc));
}

// Test that it is not allowed to use combined texture and sampler.
// TODO(jiawei.shao@intel.com): support extracting  combined texture and sampler in spvc.
TEST_F(ShaderModuleValidationTest, CombinedTextureAndSampler) {
    const char* shader = R"(
        #version 450
        layout (set = 0, binding = 0) uniform sampler2D texture;
        void main() {
        })";

    ASSERT_DEVICE_ERROR(
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, shader));
}
