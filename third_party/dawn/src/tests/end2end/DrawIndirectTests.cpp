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

constexpr uint32_t kRTSize = 4;

class DrawIndirectTest : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();

        renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

        dawn::ShaderModule vsModule =
            utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, R"(
                #version 450
                layout(location = 0) in vec4 pos;
                void main() {
                    gl_Position = pos;
                })");

        dawn::ShaderModule fsModule =
            utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
                #version 450
                layout(location = 0) out vec4 fragColor;
                void main() {
                    fragColor = vec4(0.0, 1.0, 0.0, 1.0);
                })");

        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.cVertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.primitiveTopology = dawn::PrimitiveTopology::TriangleStrip;
        descriptor.cVertexInput.bufferCount = 1;
        descriptor.cVertexInput.cBuffers[0].stride = 4 * sizeof(float);
        descriptor.cVertexInput.cBuffers[0].attributeCount = 1;
        descriptor.cVertexInput.cAttributes[0].format = dawn::VertexFormat::Float4;
        descriptor.cColorStates[0]->format = renderPass.colorFormat;

        pipeline = device.CreateRenderPipeline(&descriptor);

        vertexBuffer = utils::CreateBufferFromData<float>(
            device, dawn::BufferUsageBit::Vertex,
            {// The bottom left triangle
             -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f,

             // The top right triangle
             -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f});
    }

    utils::BasicRenderPass renderPass;
    dawn::RenderPipeline pipeline;
    dawn::Buffer vertexBuffer;

    void Test(std::initializer_list<uint32_t> bufferList,
              uint64_t indirectOffset,
              RGBA8 bottomLeftExpected,
              RGBA8 topRightExpected) {
        dawn::Buffer indirectBuffer = utils::CreateBufferFromData<uint32_t>(
            device, dawn::BufferUsageBit::Indirect, bufferList);

        uint64_t zeroOffset = 0;
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
            pass.SetPipeline(pipeline);
            pass.SetVertexBuffers(0, 1, &vertexBuffer, &zeroOffset);
            pass.DrawIndirect(indirectBuffer, indirectOffset);
            pass.EndPass();
        }

        dawn::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        EXPECT_PIXEL_RGBA8_EQ(bottomLeftExpected, renderPass.color, 1, 3);
        EXPECT_PIXEL_RGBA8_EQ(topRightExpected, renderPass.color, 3, 1);
    }
};

// The basic triangle draw.
TEST_P(DrawIndirectTest, Uint32) {
    RGBA8 filled(0, 255, 0, 255);
    RGBA8 notFilled(0, 0, 0, 0);

    // Test a draw with no indices.
    Test({0, 0, 0, 0}, 0, notFilled, notFilled);

    // Test a draw with only the first 3 indices (bottom left triangle)
    Test({3, 1, 0, 0}, 0, filled, notFilled);

    // Test a draw with only the last 3 indices (top right triangle)
    Test({3, 1, 3, 0}, 0, notFilled, filled);

    // Test a draw with all 6 indices (both triangles).
    Test({6, 1, 0, 0}, 0, filled, filled);
}

TEST_P(DrawIndirectTest, IndirectOffset) {
    RGBA8 filled(0, 255, 0, 255);
    RGBA8 notFilled(0, 0, 0, 0);

    // Test an offset draw call, with indirect buffer containing 2 calls:
    // 1) only the first 3 indices (bottom left triangle)
    // 2) only the last 3 indices (top right triangle)

    // Test #1 (no offset)
    Test({3, 1, 0, 0, 3, 1, 3, 0}, 0, filled, notFilled);

    // Offset to draw #2
    Test({3, 1, 0, 0, 3, 1, 3, 0}, 4 * sizeof(uint32_t), notFilled, filled);
}

DAWN_INSTANTIATE_TEST(DrawIndirectTest, D3D12Backend, MetalBackend, OpenGLBackend, VulkanBackend);
