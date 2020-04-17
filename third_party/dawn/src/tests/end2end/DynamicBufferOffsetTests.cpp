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

#include "tests/DawnTest.h"

#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/DawnHelpers.h"

constexpr uint32_t kRTSize = 400;
constexpr uint32_t kBufferElementsCount = kMinDynamicBufferOffsetAlignment / sizeof(uint32_t) + 2;
constexpr uint32_t kBufferSize = kBufferElementsCount * sizeof(uint32_t);

class DynamicBufferOffsetTests : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();

        std::array<uint32_t, kBufferElementsCount> uniformData = {0};

        uniformData[0] = 1;
        uniformData[1] = 2;
        uniformData[uniformData.size() - 2] = 5;
        uniformData[uniformData.size() - 1] = 6;

        mUniformBuffer = utils::CreateBufferFromData(device, uniformData.data(), kBufferSize,
                                                     dawn::BufferUsageBit::Uniform);

        dawn::BufferDescriptor storageBufferDescriptor;
        storageBufferDescriptor.size = kBufferSize;
        storageBufferDescriptor.usage = dawn::BufferUsageBit::Storage |
                                        dawn::BufferUsageBit::TransferDst |
                                        dawn::BufferUsageBit::TransferSrc;

        mStorageBuffer = device.CreateBuffer(&storageBufferDescriptor);

        mBindGroupLayout = utils::MakeBindGroupLayout(
            device, {{0, dawn::ShaderStageBit::Compute | dawn::ShaderStageBit::Fragment,
                      dawn::BindingType::DynamicUniformBuffer},
                     {1, dawn::ShaderStageBit::Compute | dawn::ShaderStageBit::Fragment,
                      dawn::BindingType::DynamicStorageBuffer}});

        mBindGroup = utils::MakeBindGroup(
            device, mBindGroupLayout,
            {{0, mUniformBuffer, 0, kBufferSize}, {1, mStorageBuffer, 0, kBufferSize}});
    }
    // Create objects to use as resources inside test bind groups.

    dawn::BindGroup mBindGroup;
    dawn::BindGroupLayout mBindGroupLayout;
    dawn::Buffer mUniformBuffer;
    dawn::Buffer mStorageBuffer;
    dawn::Texture mColorAttachment;

    dawn::RenderPipeline CreateRenderPipeline() {
        dawn::ShaderModule vsModule =
            utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, R"(
                #version 450
                void main() {
                    const vec2 pos[3] = vec2[3](vec2(-1.0f, 0.0f), vec2(-1.0f, -1.0f), vec2(0.0f, -1.0f));
                    gl_Position = vec4(pos[gl_VertexIndex], 0.0, 1.0);
                })");

        dawn::ShaderModule fsModule =
            utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
                #version 450
                layout(std140, set = 0, binding = 0) uniform uBuffer {
                     uvec2 value;
                };
                layout(std140, set = 0, binding = 1) buffer SBuffer {
                     uvec2 result;
                } sBuffer;
                layout(location = 0) out vec4 fragColor;
                void main() {
                    sBuffer.result.xy = value.xy;
                    fragColor = vec4(value.x / 255.0f, value.y / 255.0f, 1.0f, 1.0f);
                })");

        utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
        pipelineDescriptor.cVertexStage.module = vsModule;
        pipelineDescriptor.cFragmentStage.module = fsModule;
        pipelineDescriptor.cColorStates[0]->format = dawn::TextureFormat::R8G8B8A8Unorm;
        dawn::PipelineLayout pipelineLayout =
            utils::MakeBasicPipelineLayout(device, &mBindGroupLayout);
        pipelineDescriptor.layout = pipelineLayout;

        return device.CreateRenderPipeline(&pipelineDescriptor);
    }

    dawn::ComputePipeline CreateComputePipeline() {
        dawn::ShaderModule csModule =
            utils::CreateShaderModule(device, dawn::ShaderStage::Compute, R"(
                #version 450
                layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
                layout(std140, set = 0, binding = 0) uniform UniformBuffer {
                    uvec2 value;
                };
                layout(std140, set = 0, binding = 1) buffer SBuffer {
                    uvec2 result;
                } sBuffer;

                void main() {
                    sBuffer.result.xy = value.xy;
                })");

        dawn::ComputePipelineDescriptor csDesc;
        dawn::PipelineLayout pipelineLayout =
            utils::MakeBasicPipelineLayout(device, &mBindGroupLayout);
        csDesc.layout = pipelineLayout;

        dawn::PipelineStageDescriptor computeStage;
        computeStage.module = csModule;
        computeStage.entryPoint = "main";
        csDesc.computeStage = &computeStage;

        return device.CreateComputePipeline(&csDesc);
    }
};

// Dynamic offsets are all zero and no effect to result.
TEST_P(DynamicBufferOffsetTests, BasicRenderPipeline) {
    dawn::RenderPipeline pipeline = CreateRenderPipeline();
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    std::array<uint64_t, 2> offsets = {0, 0};
    dawn::RenderPassEncoder renderPassEncoder =
        commandEncoder.BeginRenderPass(&renderPass.renderPassInfo);
    renderPassEncoder.SetPipeline(pipeline);
    renderPassEncoder.SetBindGroup(0, mBindGroup, offsets.size(), offsets.data());
    renderPassEncoder.Draw(3, 1, 0, 0);
    renderPassEncoder.EndPass();
    dawn::CommandBuffer commands = commandEncoder.Finish();
    queue.Submit(1, &commands);

    std::vector<uint32_t> expectedData = {1, 2};
    EXPECT_PIXEL_RGBA8_EQ(RGBA8(1, 2, 255, 255), renderPass.color, 0, 0);
    EXPECT_BUFFER_U32_RANGE_EQ(expectedData.data(), mStorageBuffer, 0, expectedData.size());
}

// Have non-zero dynamic offsets.
TEST_P(DynamicBufferOffsetTests, SetDynamicOffestsRenderPipeline) {
    dawn::RenderPipeline pipeline = CreateRenderPipeline();
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    std::array<uint64_t, 2> offsets = {kMinDynamicBufferOffsetAlignment,
                                       kMinDynamicBufferOffsetAlignment};
    dawn::RenderPassEncoder renderPassEncoder =
        commandEncoder.BeginRenderPass(&renderPass.renderPassInfo);
    renderPassEncoder.SetPipeline(pipeline);
    renderPassEncoder.SetBindGroup(0, mBindGroup, offsets.size(), offsets.data());
    renderPassEncoder.Draw(3, 1, 0, 0);
    renderPassEncoder.EndPass();
    dawn::CommandBuffer commands = commandEncoder.Finish();
    queue.Submit(1, &commands);

    std::vector<uint32_t> expectedData = {5, 6};
    EXPECT_PIXEL_RGBA8_EQ(RGBA8(5, 6, 255, 255), renderPass.color, 0, 0);
    EXPECT_BUFFER_U32_RANGE_EQ(expectedData.data(), mStorageBuffer,
                               kMinDynamicBufferOffsetAlignment, expectedData.size());
}

// Dynamic offsets are all zero and no effect to result.
TEST_P(DynamicBufferOffsetTests, BasicComputePipeline) {
    dawn::ComputePipeline pipeline = CreateComputePipeline();

    std::array<uint64_t, 2> offsets = {0, 0};

    dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    dawn::ComputePassEncoder computePassEncoder = commandEncoder.BeginComputePass();
    computePassEncoder.SetPipeline(pipeline);
    computePassEncoder.SetBindGroup(0, mBindGroup, offsets.size(), offsets.data());
    computePassEncoder.Dispatch(1, 1, 1);
    computePassEncoder.EndPass();
    dawn::CommandBuffer commands = commandEncoder.Finish();
    queue.Submit(1, &commands);

    std::vector<uint32_t> expectedData = {1, 2};
    EXPECT_BUFFER_U32_RANGE_EQ(expectedData.data(), mStorageBuffer, 0, expectedData.size());
}

// Have non-zero dynamic offsets.
TEST_P(DynamicBufferOffsetTests, SetDynamicOffestsComputePipeline) {
    dawn::ComputePipeline pipeline = CreateComputePipeline();

    std::array<uint64_t, 2> offsets = {kMinDynamicBufferOffsetAlignment,
                                       kMinDynamicBufferOffsetAlignment};

    dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    dawn::ComputePassEncoder computePassEncoder = commandEncoder.BeginComputePass();
    computePassEncoder.SetPipeline(pipeline);
    computePassEncoder.SetBindGroup(0, mBindGroup, offsets.size(), offsets.data());
    computePassEncoder.Dispatch(1, 1, 1);
    computePassEncoder.EndPass();
    dawn::CommandBuffer commands = commandEncoder.Finish();
    queue.Submit(1, &commands);

    std::vector<uint32_t> expectedData = {5, 6};
    EXPECT_BUFFER_U32_RANGE_EQ(expectedData.data(), mStorageBuffer,
                               kMinDynamicBufferOffsetAlignment, expectedData.size());
}

DAWN_INSTANTIATE_TEST(DynamicBufferOffsetTests, MetalBackend, VulkanBackend);
