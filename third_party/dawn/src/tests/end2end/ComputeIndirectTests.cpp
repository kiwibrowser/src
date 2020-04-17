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

#include "dawn/dawncpp.h"
#include "tests/DawnTest.h"

#include "utils/DawnHelpers.h"

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
    dawn::BindGroupLayout bgl = utils::MakeBindGroupLayout(
        device, {
                    {0, dawn::ShaderStageBit::Compute, dawn::BindingType::UniformBuffer},
                    {1, dawn::ShaderStageBit::Compute, dawn::BindingType::StorageBuffer},
                });

    // Set up shader and pipeline
    dawn::ShaderModule module =
        utils::CreateShaderModule(device, dawn::ShaderStage::Compute, shaderSource);
    dawn::PipelineLayout pl = utils::MakeBasicPipelineLayout(device, &bgl);

    dawn::ComputePipelineDescriptor csDesc;
    csDesc.layout = pl;

    dawn::PipelineStageDescriptor computeStage;
    computeStage.module = module;
    computeStage.entryPoint = "main";
    csDesc.computeStage = &computeStage;

    dawn::ComputePipeline pipeline = device.CreateComputePipeline(&csDesc);

    // Set up dst storage buffer to contain dispatch x, y, z
    dawn::Buffer dst = utils::CreateBufferFromData<uint32_t>(device,
                                                             dawn::BufferUsageBit::Storage |
                                                                 dawn::BufferUsageBit::TransferSrc |
                                                                 dawn::BufferUsageBit::TransferDst,
                                                             {0, 0, 0});

    std::vector<uint32_t> indirectBufferData = bufferList;

    dawn::Buffer indirectBuffer =
        utils::CreateBufferFromData<uint32_t>(device, dawn::BufferUsageBit::Indirect, bufferList);

    dawn::Buffer expectedBuffer =
        utils::CreateBufferFromData(device, &indirectBufferData[indirectOffset / sizeof(uint32_t)],
                                    3 * sizeof(uint32_t), dawn::BufferUsageBit::Uniform);

    // Set up bind group and issue dispatch
    dawn::BindGroup bindGroup =
        utils::MakeBindGroup(device, bgl,
                             {
                                 {0, expectedBuffer, 0, 3 * sizeof(uint32_t)},
                                 {1, dst, 0, 3 * sizeof(uint32_t)},
                             });

    dawn::CommandBuffer commands;
    {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        dawn::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bindGroup, 0, nullptr);
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
    // See https://bugs.chromium.org/p/dawn/issues/detail?id=159
    DAWN_SKIP_TEST_IF(IsD3D12() && IsNvidia());

    BasicTest({2, 3, 4}, 0);
}

// Test indirect with buffer offset
TEST_P(ComputeIndirectTests, IndirectOffset) {
    // See https://bugs.chromium.org/p/dawn/issues/detail?id=159
    DAWN_SKIP_TEST_IF(IsD3D12() && IsNvidia());

    BasicTest({0, 0, 0, 2, 3, 4}, 3 * sizeof(uint32_t));
}

DAWN_INSTANTIATE_TEST(ComputeIndirectTests,
                      D3D12Backend,
                      MetalBackend,
                      OpenGLBackend,
                      VulkanBackend);
