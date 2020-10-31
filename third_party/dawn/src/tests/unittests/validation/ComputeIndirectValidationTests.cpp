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

#include <initializer_list>
#include <limits>
#include "tests/unittests/validation/ValidationTest.h"
#include "utils/WGPUHelpers.h"

class ComputeIndirectValidationTest : public ValidationTest {
  protected:
    void SetUp() override {
        ValidationTest::SetUp();

        wgpu::ShaderModule computeModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, R"(
                #version 450
                layout(local_size_x = 1) in;
                void main() {
                })");

        // Set up compute pipeline
        wgpu::PipelineLayout pl = utils::MakeBasicPipelineLayout(device, nullptr);

        wgpu::ComputePipelineDescriptor csDesc;
        csDesc.layout = pl;
        csDesc.computeStage.module = computeModule;
        csDesc.computeStage.entryPoint = "main";
        pipeline = device.CreateComputePipeline(&csDesc);
    }

    void ValidateExpectation(wgpu::CommandEncoder encoder, utils::Expectation expectation) {
        if (expectation == utils::Expectation::Success) {
            encoder.Finish();
        } else {
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }
    }

    void TestIndirectOffset(utils::Expectation expectation,
                            std::initializer_list<uint32_t> bufferList,
                            uint64_t indirectOffset) {
        wgpu::Buffer indirectBuffer =
            utils::CreateBufferFromData<uint32_t>(device, wgpu::BufferUsage::Indirect, bufferList);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetPipeline(pipeline);
        pass.DispatchIndirect(indirectBuffer, indirectOffset);
        pass.EndPass();

        ValidateExpectation(encoder, expectation);
    }

    wgpu::ComputePipeline pipeline;
};

// Verify out of bounds indirect dispatch calls are caught early
TEST_F(ComputeIndirectValidationTest, IndirectOffsetBounds) {
    // In bounds
    TestIndirectOffset(utils::Expectation::Success, {1, 2, 3}, 0);
    // In bounds, bigger buffer
    TestIndirectOffset(utils::Expectation::Success, {1, 2, 3, 4, 5, 6}, 0);
    // In bounds, bigger buffer, positive offset
    TestIndirectOffset(utils::Expectation::Success, {1, 2, 3, 4, 5, 6}, 3 * sizeof(uint32_t));

    // Out of bounds, buffer too small
    TestIndirectOffset(utils::Expectation::Failure, {1, 2}, 0);
    // Out of bounds, index too big
    TestIndirectOffset(utils::Expectation::Failure, {1, 2, 3}, 1 * sizeof(uint32_t));
    // Out of bounds, index past buffer
    TestIndirectOffset(utils::Expectation::Failure, {1, 2, 3}, 4 * sizeof(uint32_t));
    // Out of bounds, index + size of command overflows
    uint64_t offset = std::numeric_limits<uint64_t>::max();
    TestIndirectOffset(utils::Expectation::Failure, {1, 2, 3, 4, 5, 6}, offset);
}
