// Copyright 2019 The Dawn Authors
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

#include "tests/unittests/validation/ValidationTest.h"

#include "common/Constants.h"

#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/DawnHelpers.h"

namespace {

class RenderPassValidationTest : public ValidationTest {};

// Test that it is invalid to draw in a render pass with missing bind groups
TEST_F(RenderPassValidationTest, MissingBindGroup) {
    dawn::ShaderModule vsModule =
        utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, R"(
#version 450
layout (set = 0, binding = 0) uniform vertexUniformBuffer {
    mat2 transform;
};
void main() {
    const vec2 pos[3] = vec2[3](vec2(-1.f, -1.f), vec2(1.f, -1.f), vec2(-1.f, 1.f));
    gl_Position = vec4(transform * pos[gl_VertexIndex], 0.f, 1.f);
})");

    dawn::ShaderModule fsModule =
        utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
#version 450
layout (set = 1, binding = 0) uniform fragmentUniformBuffer {
    vec4 color;
};
layout(location = 0) out vec4 fragColor;
void main() {
    fragColor = color;
})");

    dawn::BindGroupLayout bgls[] = {
        utils::MakeBindGroupLayout(
            device, {{0, dawn::ShaderStageBit::Vertex, dawn::BindingType::UniformBuffer}}),
        utils::MakeBindGroupLayout(
            device, {{0, dawn::ShaderStageBit::Fragment, dawn::BindingType::UniformBuffer}})};

    dawn::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindGroupLayoutCount = 2;
    pipelineLayoutDesc.bindGroupLayouts = bgls;

    dawn::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&pipelineLayoutDesc);

    utils::ComboRenderPipelineDescriptor descriptor(device);
    descriptor.layout = pipelineLayout;
    descriptor.cVertexStage.module = vsModule;
    descriptor.cFragmentStage.module = fsModule;

    dawn::RenderPipeline pipeline = device.CreateRenderPipeline(&descriptor);

    float data[4];
    dawn::Buffer buffer = utils::CreateBufferFromData(device, data, 4 * sizeof(float),
                                                        dawn::BufferUsageBit::Uniform);

    dawn::BindGroup bg1 =
        utils::MakeBindGroup(device, bgls[0], {{0, buffer, 0, 4 * sizeof(float)}});
    dawn::BindGroup bg2 =
        utils::MakeBindGroup(device, bgls[1], {{0, buffer, 0, 4 * sizeof(float)}});

    DummyRenderPass renderPass(device);
    {
        dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        dawn::RenderPassEncoder pass = commandEncoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bg1, 0, nullptr);
        pass.SetBindGroup(1, bg2, 0, nullptr);
        pass.Draw(3, 0, 0, 0);
        pass.EndPass();
        commandEncoder.Finish();
    }
    {
        dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        dawn::RenderPassEncoder pass = commandEncoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(pipeline);
        pass.Draw(3, 0, 0, 0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(commandEncoder.Finish());
    }
    {
        dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        dawn::RenderPassEncoder pass = commandEncoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(1, bg2, 0, nullptr);
        pass.Draw(3, 0, 0, 0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(commandEncoder.Finish());
    }
    {
        dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        dawn::RenderPassEncoder pass = commandEncoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bg1, 0, nullptr);
        pass.Draw(3, 0, 0, 0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(commandEncoder.Finish());
    }
}

}  // anonymous namespace
