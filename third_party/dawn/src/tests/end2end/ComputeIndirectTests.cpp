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
#include <initializer_list>

class ComputeIndirectTests : public DawnTest {
  public:
    // Write into the output buffer if we saw the biggest dispatch
    // This is a workaround since D3D12 doesn't have gl_NumWorkGroups
    const char* shaderSource = R"(
        #version 450
        layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
        layout(std140, set = 0, binding = 0) uniform inputBuf {
            uvec3 expectedDispatch;
        };
        layout(std140, set = 0, binding = 1) buffer outputBuf {
            uvec3 workGroups;
        };

        void main() {
            if (gl_GlobalInvocationID == expectedDispatch - uvec3(1, 1, 1)) {
                workGroups = expectedDispatch;
            }
        })";

    void BasicTest(std::initializer_list<uint32_t> buffer, uint64_t indirectOffset);
};

void ComputeIndirectTests::BasicTest(std::initializer_list<uint32_t> bufferList,
                                     uint64_t indirectOffset) {
    // Set up shader and pipeline
    wgpu::ShaderModule module =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, shaderSource);

    wgpu::ComputePipelineDescriptor csDesc;
    csDesc.computeStage.module = module;
    csDesc.computeStage.entryPoint = "main";
    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&csDesc);

    // Set up dst storage buffer to contain dispatch x, y, z
    wgpu::Buffer dst = utils::CreateBufferFromData<uint32_t>(
        device,
        wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst,
        {0, 0, 0});

    std::vector<uint32_t> indirectBufferData = bufferList;

    wgpu::Buffer indirectBuffer =
        utils::CreateBufferFromData<uint32_t>(device, wgpu::BufferUsage::Indirect, bufferList);

    wgpu::Buffer expectedBuffer =
        utils::CreateBufferFromData(device, &indirectBufferData[indirectOffset / sizeof(uint32_t)],
                                    3 * sizeof(uint32_t), wgpu::BufferUsage::Uniform);

    // Set up bind group and issue dispatch
    wgpu::BindGroup bindGroup =
        utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                             {
                                 {0, expectedBuffer, 0, 3 * sizeof(uint32_t)},
                                 {1, dst, 0, 3 * sizeof(uint32_t)},
                             });

    wgpu::CommandBuffer commands;
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bindGroup);
        pass.DispatchIndirect(indirectBuffer, indirectOffset);
        pass.EndPass();

        commands = encoder.Finish();
    }

    queue.Submit(1, &commands);

    // Verify the dispatch got called with group counts in indirect buffer
    EXPECT_BUFFER_U32_RANGE_EQ(&indirectBufferData[indirectOffset / sizeof(uint32_t)], dst, 0, 3);
}

// Test basic indirect
TEST_P(ComputeIndirectTests, Basic) {
    BasicTest({2, 3, 4}, 0);
}

// Test indirect with buffer offset
TEST_P(ComputeIndirectTests, IndirectOffset) {
    BasicTest({0, 0, 0, 2, 3, 4}, 3 * sizeof(uint32_t));
}

DAWN_INSTANTIATE_TEST(ComputeIndirectTests,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());
