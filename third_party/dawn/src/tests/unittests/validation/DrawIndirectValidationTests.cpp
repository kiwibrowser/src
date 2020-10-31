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
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

class DrawIndirectValidationTest : public ValidationTest {
  protected:
    void SetUp() override {
        ValidationTest::SetUp();

        wgpu::ShaderModule vsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
            #version 450
            void main() {
                gl_Position = vec4(0.0);
            })");

        wgpu::ShaderModule fsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
            #version 450
            layout(location = 0) out vec4 fragColor;
            void main() {
                fragColor = vec4(0.0);
            })");

        // Set up render pipeline
        wgpu::PipelineLayout pipelineLayout = utils::MakeBasicPipelineLayout(device, nullptr);

        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.layout = pipelineLayout;
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;

        pipeline = device.CreateRenderPipeline(&descriptor);
    }

    void ValidateExpectation(wgpu::CommandEncoder encoder, utils::Expectation expectation) {
        if (expectation == utils::Expectation::Success) {
            encoder.Finish();
        } else {
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }
    }

    void TestIndirectOffsetDrawIndexed(utils::Expectation expectation,
                                       std::initializer_list<uint32_t> bufferList,
                                       uint64_t indirectOffset) {
        TestIndirectOffset(expectation, bufferList, indirectOffset, true);
    }

    void TestIndirectOffsetDraw(utils::Expectation expectation,
                                std::initializer_list<uint32_t> bufferList,
                                uint64_t indirectOffset) {
        TestIndirectOffset(expectation, bufferList, indirectOffset, false);
    }

    void TestIndirectOffset(utils::Expectation expectation,
                            std::initializer_list<uint32_t> bufferList,
                            uint64_t indirectOffset,
                            bool indexed) {
        wgpu::Buffer indirectBuffer =
            utils::CreateBufferFromData<uint32_t>(device, wgpu::BufferUsage::Indirect, bufferList);

        DummyRenderPass renderPass(device);
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(pipeline);
        if (indexed) {
            uint32_t zeros[100] = {};
            wgpu::Buffer indexBuffer =
                utils::CreateBufferFromData(device, zeros, sizeof(zeros), wgpu::BufferUsage::Index);
            pass.SetIndexBuffer(indexBuffer);
            pass.DrawIndexedIndirect(indirectBuffer, indirectOffset);
        } else {
            pass.DrawIndirect(indirectBuffer, indirectOffset);
        }
        pass.EndPass();

        ValidateExpectation(encoder, expectation);
    }

    wgpu::RenderPipeline pipeline;
};

// Verify out of bounds indirect draw calls are caught early
TEST_F(DrawIndirectValidationTest, DrawIndirectOffsetBounds) {
    // In bounds
    TestIndirectOffsetDraw(utils::Expectation::Success, {1, 2, 3, 4}, 0);
    // In bounds, bigger buffer
    TestIndirectOffsetDraw(utils::Expectation::Success, {1, 2, 3, 4, 5, 6, 7}, 0);
    // In bounds, bigger buffer, positive offset
    TestIndirectOffsetDraw(utils::Expectation::Success, {1, 2, 3, 4, 5, 6, 7, 8},
                           4 * sizeof(uint32_t));

    // Out of bounds, buffer too small
    TestIndirectOffsetDraw(utils::Expectation::Failure, {1, 2, 3}, 0);
    // Out of bounds, index too big
    TestIndirectOffsetDraw(utils::Expectation::Failure, {1, 2, 3, 4}, 1 * sizeof(uint32_t));
    // Out of bounds, index past buffer
    TestIndirectOffsetDraw(utils::Expectation::Failure, {1, 2, 3, 4}, 5 * sizeof(uint32_t));
    // Out of bounds, index + size of command overflows
    uint64_t offset = std::numeric_limits<uint64_t>::max();
    TestIndirectOffsetDraw(utils::Expectation::Failure, {1, 2, 3, 4, 5, 6, 7}, offset);
}

// Verify out of bounds indirect draw indexed calls are caught early
TEST_F(DrawIndirectValidationTest, DrawIndexedIndirectOffsetBounds) {
    // In bounds
    TestIndirectOffsetDrawIndexed(utils::Expectation::Success, {1, 2, 3, 4, 5}, 0);
    // In bounds, bigger buffer
    TestIndirectOffsetDrawIndexed(utils::Expectation::Success, {1, 2, 3, 4, 5, 6, 7, 8, 9}, 0);
    // In bounds, bigger buffer, positive offset
    TestIndirectOffsetDrawIndexed(utils::Expectation::Success, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
                                  5 * sizeof(uint32_t));

    // Out of bounds, buffer too small
    TestIndirectOffsetDrawIndexed(utils::Expectation::Failure, {1, 2, 3, 4}, 0);
    // Out of bounds, index too big
    TestIndirectOffsetDrawIndexed(utils::Expectation::Failure, {1, 2, 3, 4, 5},
                                  1 * sizeof(uint32_t));
    // Out of bounds, index past buffer
    TestIndirectOffsetDrawIndexed(utils::Expectation::Failure, {1, 2, 3, 4, 5},
                                  5 * sizeof(uint32_t));
    // Out of bounds, index + size of command overflows
    uint64_t offset = std::numeric_limits<uint64_t>::max();
    TestIndirectOffsetDrawIndexed(utils::Expectation::Failure, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
                                  offset);
}
