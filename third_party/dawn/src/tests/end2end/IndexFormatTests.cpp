// Copyright 2017 The Dawn Authors
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

#include "common/Assert.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/DawnHelpers.h"

constexpr uint32_t kRTSize = 400;

class IndexFormatTest : public DawnTest {
    protected:
        void SetUp() override {
            DawnTest::SetUp();

            renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);
        }

        utils::BasicRenderPass renderPass;

        dawn::RenderPipeline MakeTestPipeline(dawn::IndexFormat format) {

            dawn::ShaderModule vsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, R"(
                #version 450
                layout(location = 0) in vec4 pos;
                void main() {
                    gl_Position = pos;
                })"
            );

            dawn::ShaderModule fsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
                #version 450
                layout(location = 0) out vec4 fragColor;
                void main() {
                    fragColor = vec4(0.0, 1.0, 0.0, 1.0);
                })"
            );

            utils::ComboRenderPipelineDescriptor descriptor(device);
            descriptor.cVertexStage.module = vsModule;
            descriptor.cFragmentStage.module = fsModule;
            descriptor.primitiveTopology = dawn::PrimitiveTopology::TriangleStrip;
            descriptor.cVertexInput.indexFormat = format;
            descriptor.cVertexInput.bufferCount = 1;
            descriptor.cVertexInput.cBuffers[0].stride = 4 * sizeof(float);
            descriptor.cVertexInput.cBuffers[0].attributeCount = 1;
            descriptor.cVertexInput.cAttributes[0].format = dawn::VertexFormat::Float4;
            descriptor.cColorStates[0]->format = renderPass.colorFormat;

            return device.CreateRenderPipeline(&descriptor);
        }
};

// Test that the Uint32 index format is correctly interpreted
TEST_P(IndexFormatTest, Uint32) {
    dawn::RenderPipeline pipeline = MakeTestPipeline(dawn::IndexFormat::Uint32);

    dawn::Buffer vertexBuffer = utils::CreateBufferFromData<float>(device, dawn::BufferUsageBit::Vertex, {
        -1.0f,  1.0f, 0.0f, 1.0f, // Note Vertices[0] = Vertices[1]
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f
    });
    // If this is interpreted as Uint16, then it would be 0, 1, 0, ... and would draw nothing.
    dawn::Buffer indexBuffer = utils::CreateBufferFromData<uint32_t>(device, dawn::BufferUsageBit::Index, {
        1, 2, 3
    });

    uint64_t zeroOffset = 0;
    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetPipeline(pipeline);
        pass.SetVertexBuffers(0, 1, &vertexBuffer, &zeroOffset);
        pass.SetIndexBuffer(indexBuffer, 0);
        pass.DrawIndexed(3, 1, 0, 0, 0);
        pass.EndPass();
    }

    dawn::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), renderPass.color, 100, 300);
}

// Test that the Uint16 index format is correctly interpreted
TEST_P(IndexFormatTest, Uint16) {
    dawn::RenderPipeline pipeline = MakeTestPipeline(dawn::IndexFormat::Uint16);

    dawn::Buffer vertexBuffer = utils::CreateBufferFromData<float>(device, dawn::BufferUsageBit::Vertex, {
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f
    });
    // If this is interpreted as uint32, it will have index 1 and 2 be both 0 and render nothing
    dawn::Buffer indexBuffer = utils::CreateBufferFromData<uint16_t>(device, dawn::BufferUsageBit::Index, {
        1, 2, 0, 0, 0, 0
    });

    uint64_t zeroOffset = 0;
    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetPipeline(pipeline);
        pass.SetVertexBuffers(0, 1, &vertexBuffer, &zeroOffset);
        pass.SetIndexBuffer(indexBuffer, 0);
        pass.DrawIndexed(3, 1, 0, 0, 0);
        pass.EndPass();
    }

    dawn::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), renderPass.color, 100, 300);
}

// Test for primitive restart use vertices like in the drawing and draw the following
// indices: 0 1 2 PRIM_RESTART 3 4 2. Then A and B should be written but not C.
//      |--------------|
//      |      0       |
//      |      |\      |
//      |      |B \    |
//      |      2---1   |
//      |     /| C     |
//      |   / A|       |
//      |  4---3       |
//      |--------------|

// Test use of primitive restart with an Uint32 index format
TEST_P(IndexFormatTest, Uint32PrimitiveRestart) {
    dawn::RenderPipeline pipeline = MakeTestPipeline(dawn::IndexFormat::Uint32);

    dawn::Buffer vertexBuffer = utils::CreateBufferFromData<float>(device, dawn::BufferUsageBit::Vertex, {
         0.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  0.0f, 0.0f, 1.0f,
         0.0f,  0.0f, 0.0f, 1.0f,
         0.0f, -1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
    });
    dawn::Buffer indexBuffer = utils::CreateBufferFromData<uint32_t>(device, dawn::BufferUsageBit::Index, {
        0, 1, 2, 0xFFFFFFFFu, 3, 4, 2,
    });

    uint64_t zeroOffset = 0;
    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetPipeline(pipeline);
        pass.SetVertexBuffers(0, 1, &vertexBuffer, &zeroOffset);
        pass.SetIndexBuffer(indexBuffer, 0);
        pass.DrawIndexed(7, 1, 0, 0, 0);
        pass.EndPass();
    }

    dawn::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), renderPass.color, 190, 190);  // A
    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), renderPass.color, 210, 210);  // B
    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 0, 0, 0), renderPass.color, 210, 190);      // C
}

// Test use of primitive restart with an Uint16 index format
TEST_P(IndexFormatTest, Uint16PrimitiveRestart) {
    dawn::RenderPipeline pipeline = MakeTestPipeline(dawn::IndexFormat::Uint16);

    dawn::Buffer vertexBuffer = utils::CreateBufferFromData<float>(device, dawn::BufferUsageBit::Vertex, {
         0.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  0.0f, 0.0f, 1.0f,
         0.0f,  0.0f, 0.0f, 1.0f,
         0.0f, -1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
    });
    dawn::Buffer indexBuffer = utils::CreateBufferFromData<uint16_t>(device, dawn::BufferUsageBit::Index, {
        0, 1, 2, 0xFFFFu, 3, 4, 2,
        // This value is for padding.
        0xFFFFu,
    });

    uint64_t zeroOffset = 0;
    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetPipeline(pipeline);
        pass.SetVertexBuffers(0, 1, &vertexBuffer, &zeroOffset);
        pass.SetIndexBuffer(indexBuffer, 0);
        pass.DrawIndexed(7, 1, 0, 0, 0);
        pass.EndPass();
    }

    dawn::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), renderPass.color, 190, 190);  // A
    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), renderPass.color, 210, 210);  // B
    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 0, 0, 0), renderPass.color, 210, 190);      // C
}

// Test that the index format used is the format of the last set pipeline. This is to
// prevent a case in D3D12 where the index format would be captured from the last
// pipeline on SetIndexBuffer.
TEST_P(IndexFormatTest, ChangePipelineAfterSetIndexBuffer) {
    DAWN_SKIP_TEST_IF(IsD3D12() || IsVulkan());

    dawn::RenderPipeline pipeline32 = MakeTestPipeline(dawn::IndexFormat::Uint32);
    dawn::RenderPipeline pipeline16 = MakeTestPipeline(dawn::IndexFormat::Uint16);

    dawn::Buffer vertexBuffer = utils::CreateBufferFromData<float>(device, dawn::BufferUsageBit::Vertex, {
        -1.0f,  1.0f, 0.0f, 1.0f, // Note Vertices[0] = Vertices[1]
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f
    });
    // If this is interpreted as Uint16, then it would be 0, 1, 0, ... and would draw nothing.
    dawn::Buffer indexBuffer = utils::CreateBufferFromData<uint32_t>(device, dawn::BufferUsageBit::Index, {
        1, 2, 3
    });

    uint64_t zeroOffset = 0;
    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetPipeline(pipeline16);
        pass.SetVertexBuffers(0, 1, &vertexBuffer, &zeroOffset);
        pass.SetIndexBuffer(indexBuffer, 0);
        pass.SetPipeline(pipeline32);
        pass.DrawIndexed(3, 1, 0, 0, 0);
        pass.EndPass();
    }

    dawn::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), renderPass.color, 100, 300);
}

// Test that setting the index buffer before the pipeline works, this is important
// for backends where the index format is passed inside the call to SetIndexBuffer
// because it needs to be done lazily (to query the format from the last pipeline).
// TODO(cwallez@chromium.org): This is currently disallowed by the validation but
// we want to support eventually.
TEST_P(IndexFormatTest, DISABLED_SetIndexBufferBeforeSetPipeline) {
    dawn::RenderPipeline pipeline = MakeTestPipeline(dawn::IndexFormat::Uint32);

    dawn::Buffer vertexBuffer = utils::CreateBufferFromData<float>(device, dawn::BufferUsageBit::Vertex, {
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f
    });
    dawn::Buffer indexBuffer = utils::CreateBufferFromData<uint32_t>(device, dawn::BufferUsageBit::Index, {
        0, 1, 2
    });

    uint64_t zeroOffset = 0;
    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetIndexBuffer(indexBuffer, 0);
        pass.SetPipeline(pipeline);
        pass.SetVertexBuffers(0, 1, &vertexBuffer, &zeroOffset);
        pass.DrawIndexed(3, 1, 0, 0, 0);
        pass.EndPass();
    }

    dawn::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), renderPass.color, 100, 300);
}

DAWN_INSTANTIATE_TEST(IndexFormatTest, D3D12Backend, MetalBackend, OpenGLBackend, VulkanBackend);
