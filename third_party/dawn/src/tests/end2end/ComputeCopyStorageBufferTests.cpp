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

#include "tests/DawnTest.h"

#include "utils/DawnHelpers.h"

#include <array>

class ComputeCopyStorageBufferTests : public DawnTest {
  public:
    static constexpr int kInstances = 4;
    static constexpr int kUintsPerInstance = 4;
    static constexpr int kNumUints = kInstances * kUintsPerInstance;

    void BasicTest(const char* shader);
};

void ComputeCopyStorageBufferTests::BasicTest(const char* shader) {
    auto bgl = utils::MakeBindGroupLayout(
        device, {
                    {0, dawn::ShaderStageBit::Compute, dawn::BindingType::StorageBuffer},
                    {1, dawn::ShaderStageBit::Compute, dawn::BindingType::StorageBuffer},
                });

    // Set up shader and pipeline
    auto module = utils::CreateShaderModule(device, dawn::ShaderStage::Compute, shader);
    auto pl = utils::MakeBasicPipelineLayout(device, &bgl);

    dawn::ComputePipelineDescriptor csDesc;
    csDesc.layout = pl;

    dawn::PipelineStageDescriptor computeStage;
    computeStage.module = module;
    computeStage.entryPoint = "main";
    csDesc.computeStage = &computeStage;

    dawn::ComputePipeline pipeline = device.CreateComputePipeline(&csDesc);

    // Set up src storage buffer
    dawn::BufferDescriptor srcDesc;
    srcDesc.size = kNumUints * sizeof(uint32_t);
    srcDesc.usage = dawn::BufferUsageBit::Storage | dawn::BufferUsageBit::TransferSrc |
                    dawn::BufferUsageBit::TransferDst;
    dawn::Buffer src = device.CreateBuffer(&srcDesc);

    std::array<uint32_t, kNumUints> expected;
    for (uint32_t i = 0; i < kNumUints; ++i) {
        expected[i] = (i + 1u) * 0x11111111u;
    }
    src.SetSubData(0, sizeof(expected), expected.data());
    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), src, 0, kNumUints);

    // Set up dst storage buffer
    dawn::BufferDescriptor dstDesc;
    dstDesc.size = kNumUints * sizeof(uint32_t);
    dstDesc.usage = dawn::BufferUsageBit::Storage | dawn::BufferUsageBit::TransferSrc |
                    dawn::BufferUsageBit::TransferDst;
    dawn::Buffer dst = device.CreateBuffer(&dstDesc);

    std::array<uint32_t, kNumUints> zero{};
    dst.SetSubData(0, sizeof(zero), zero.data());

    // Set up bind group and issue dispatch
    dawn::BindGroup bindGroup = utils::MakeBindGroup(device, bgl, {
        {0, src, 0, kNumUints * sizeof(uint32_t)},
        {1, dst, 0, kNumUints * sizeof(uint32_t)},
    });

    dawn::CommandBuffer commands;
    {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        dawn::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bindGroup, 0, nullptr);
        pass.Dispatch(kInstances, 1, 1);
        pass.EndPass();

        commands = encoder.Finish();
    }

    queue.Submit(1, &commands);

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), dst, 0, kNumUints);
}

// Test that a trivial compute-shader memcpy implementation works.
TEST_P(ComputeCopyStorageBufferTests, SizedArrayOfBasic) {
    // TODO(cwallez@chromium.org): Fails on D3D12, could be a spirv-cross issue?
    DAWN_SKIP_TEST_IF(IsD3D12());

    BasicTest(R"(
        #version 450
        #define kInstances 4
        layout(std140, set = 0, binding = 0) buffer Src { uvec4 s[kInstances]; } src;
        layout(std140, set = 0, binding = 1) buffer Dst { uvec4 s[kInstances]; } dst;
        void main() {
            uint index = gl_GlobalInvocationID.x;
            if (index >= kInstances) { return; }
            dst.s[index] = src.s[index];
        })");
}

// Test that a slightly-less-trivial compute-shader memcpy implementation works.
TEST_P(ComputeCopyStorageBufferTests, SizedArrayOfStruct) {
    // TODO(kainino@chromium.org): Fails on D3D12. Probably due to a limitation in SPIRV-Cross?
    DAWN_SKIP_TEST_IF(IsD3D12());

    BasicTest(R"(
        #version 450
        #define kInstances 4
        struct S {
            uvec2 a, b;  // kUintsPerInstance = 4
        };
        layout(std140, set = 0, binding = 0) buffer Src { S s[kInstances]; } src;
        layout(std140, set = 0, binding = 1) buffer Dst { S s[kInstances]; } dst;
        void main() {
            uint index = gl_GlobalInvocationID.x;
            if (index >= kInstances) { return; }
            dst.s[index] = src.s[index];
        })");
}

// Test that a trivial compute-shader memcpy implementation works.
TEST_P(ComputeCopyStorageBufferTests, UnsizedArrayOfBasic) {
    // TODO(cwallez@chromium.org): Fails on D3D12, could be a spirv-cross issue?
    DAWN_SKIP_TEST_IF(IsD3D12());

    BasicTest(R"(
        #version 450
        #define kInstances 4
        layout(std140, set = 0, binding = 0) buffer Src { uvec4 s[]; } src;
        layout(std140, set = 0, binding = 1) buffer Dst { uvec4 s[]; } dst;
        void main() {
            uint index = gl_GlobalInvocationID.x;
            if (index >= kInstances) { return; }
            dst.s[index] = src.s[index];
        })");
}

// Test binding a sized array of SSBO descriptors.
//
// This is disabled because WebGPU doesn't currently have binding arrays (equivalent to
// VkDescriptorSetLayoutBinding::descriptorCount). https://github.com/gpuweb/gpuweb/pull/61
TEST_P(ComputeCopyStorageBufferTests, DISABLED_SizedDescriptorArray) {
    BasicTest(R"(
        #version 450
        #define kInstances 4
        struct S {
            uvec2 a, b;  // kUintsPerInstance = 4
        };
        layout(std140, set = 0, binding = 0) buffer Src { S s; } src[kInstances];
        layout(std140, set = 0, binding = 1) buffer Dst { S s; } dst[kInstances];
        void main() {
            uint index = gl_GlobalInvocationID.x;
            if (index >= kInstances) { return; }
            dst[index].s = src[index].s;
        })");
}

// Test binding an unsized array of SSBO descriptors.
//
// TODO(kainino@chromium.org): This test may be somewhat wrong. I'm not sure whether this is
// supposed to be possible on the various native APIs.
// Linking on OpenGL fails with "OpenGL requires constant indexes for unsized array access(dst)".
TEST_P(ComputeCopyStorageBufferTests, DISABLED_UnsizedDescriptorArray) {
    BasicTest(R"(
        #version 450
        #extension GL_EXT_nonuniform_qualifier : require
        #define kInstances 4
        struct S {
            uvec2 a, b;  // kUintsPerInstance = 4
        };
        layout(std140, set = 0, binding = 0) buffer Src { S s; } src[];
        layout(std140, set = 0, binding = 1) buffer Dst { S s; } dst[];
        void main() {
            uint index = gl_GlobalInvocationID.x;
            if (index >= kInstances) { return; }
            dst[index].s = src[index].s;
        })");
}

DAWN_INSTANTIATE_TEST(ComputeCopyStorageBufferTests,
                     D3D12Backend,
                     MetalBackend,
                     OpenGLBackend,
                     VulkanBackend);
