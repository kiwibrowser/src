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

#include "utils/WGPUHelpers.h"

#include <array>

class ComputeSharedMemoryTests : public DawnTest {
  public:
    static constexpr uint32_t kInstances = 11;

    void BasicTest(const char* shader);
};

void ComputeSharedMemoryTests::BasicTest(const char* shader) {
    // Set up shader and pipeline
    auto module = utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, shader);

    wgpu::ComputePipelineDescriptor csDesc;
    csDesc.computeStage.module = module;
    csDesc.computeStage.entryPoint = "main";
    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&csDesc);

    // Set up dst storage buffer
    wgpu::BufferDescriptor dstDesc;
    dstDesc.size = sizeof(uint32_t);
    dstDesc.usage =
        wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer dst = device.CreateBuffer(&dstDesc);

    const uint32_t zero = 0;
    queue.WriteBuffer(dst, 0, &zero, sizeof(zero));

    // Set up bind group and issue dispatch
    wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                                     {
                                                         {0, dst, 0, sizeof(uint32_t)},
                                                     });

    wgpu::CommandBuffer commands;
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bindGroup);
        pass.Dispatch(1);
        pass.EndPass();

        commands = encoder.Finish();
    }

    queue.Submit(1, &commands);

    const uint32_t expected = kInstances;
    EXPECT_BUFFER_U32_RANGE_EQ(&expected, dst, 0, 1);
}

// Basic shared memory test
TEST_P(ComputeSharedMemoryTests, Basic) {
    BasicTest(R"(
        #version 450
        const uint kTileSize = 4;
        const uint kInstances = 11;

        layout(local_size_x = kTileSize, local_size_y = kTileSize, local_size_z = 1) in;
        layout(std140, set = 0, binding = 0) buffer Dst { uint x; } dst;
        shared uint tmp;

        void main() {
            uint index = gl_LocalInvocationID.y * kTileSize + gl_LocalInvocationID.x;
            if (index == 0) {
                tmp = 0;
            }
            barrier();
            for (uint i = 0; i < kInstances; ++i) {
                if (i == index) {
                    tmp++;
                }
                barrier();
            }
            if (index == 0) {
                dst.x = tmp;
            }
        })");
}

DAWN_INSTANTIATE_TEST(ComputeSharedMemoryTests,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());
