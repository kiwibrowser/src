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
#include "utils/WGPUHelpers.h"

constexpr uint32_t kRTSize = 400;

class IndexFormatTest : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();

        renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);
    }

    utils::BasicRenderPass renderPass;

    wgpu::RenderPipeline MakeTestPipeline(wgpu::IndexFormat format) {
        wgpu::ShaderModule vsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
                #version 450
                layout(location = 0) in vec4 pos;
                void main() {
                    gl_Position = pos;
                })");

        wgpu::ShaderModule fsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
                #version 450
                layout(location = 0) out vec4 fragColor;
                void main() {
                    fragColor = vec4(0.0, 1.0, 0.0, 1.0);
                })");

        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.primitiveTopology = wgpu::PrimitiveTopology::TriangleStrip;
        descriptor.cVertexState.indexFormat = format;
        descriptor.cVertexState.vertexBufferCount = 1;
        descriptor.cVertexState.cVertexBuffers[0].arrayStride = 4 * sizeof(float);
        descriptor.cVertexState.cVertexBuffers[0].attributeCount = 1;
        descriptor.cVertexState.cAttributes[0].format = wgpu::VertexFormat::Float4;
        descriptor.cColorStates[0].format = renderPass.colorFormat;

        return device.CreateRenderPipeline(&descriptor);
    }
};

// Test that the Uint32 index format is correctly interpreted
TEST_P(IndexFormatTest, Uint32) {
    wgpu::RenderPipeline pipeline = MakeTestPipeline(wgpu::IndexFormat::Uint32);

    wgpu::Buffer vertexBuffer = utils::CreateBufferFromData<float>(
        device, wgpu::BufferUsage::Vertex,
        {-1.0f, -1.0f, 0.0f, 1.0f,  // Note Vertices[0] = Vertices[1]
         -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f});
    // If this is interpreted as Uint16, then it would be 0, 1, 0, ... and would draw nothing.
    wgpu::Buffer indexBuffer =
        utils::CreateBufferFromData<uint32_t>(device, wgpu::BufferUsage::Index, {1, 2, 3});

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetPipeline(pipeline);
        pass.SetVertexBuffer(0, vertexBuffer);
        pass.SetIndexBuffer(indexBuffer);
        pass.DrawIndexed(3);
        pass.EndPass();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 100, 300);
}

// Test that the Uint16 index format is correctly interpreted
TEST_P(IndexFormatTest, Uint16) {
    wgpu::RenderPipeline pipeline = MakeTestPipeline(wgpu::IndexFormat::Uint16);

    wgpu::Buffer vertexBuffer = utils::CreateBufferFromData<float>(
        device, wgpu::BufferUsage::Vertex,
        {-1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f});
    // If this is interpreted as uint32, it will have index 1 and 2 be both 0 and render nothing
    wgpu::Buffer indexBuffer =
        utils::CreateBufferFromData<uint16_t>(device, wgpu::BufferUsage::Index, {1, 2, 0, 0, 0, 0});

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetPipeline(pipeline);
        pass.SetVertexBuffer(0, vertexBuffer);
        pass.SetIndexBuffer(indexBuffer);
        pass.DrawIndexed(3);
        pass.EndPass();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 100, 300);
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
    wgpu::RenderPipeline pipeline = MakeTestPipeline(wgpu::IndexFormat::Uint32);

    wgpu::Buffer vertexBuffer = utils::CreateBufferFromData<float>(
        device, wgpu::BufferUsage::Vertex,
        {
            0.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
            0.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f,
        });
    wgpu::Buffer indexBuffer =
        utils::CreateBufferFromData<uint32_t>(device, wgpu::BufferUsage::Index,
                                              {
                                                  0,
                                                  1,
                                                  2,
                                                  0xFFFFFFFFu,
                                                  3,
                                                  4,
                                                  2,
                                              });

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetPipeline(pipeline);
        pass.SetVertexBuffer(0, vertexBuffer);
        pass.SetIndexBuffer(indexBuffer);
        pass.DrawIndexed(7);
        pass.EndPass();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 190, 190);  // A
    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 210, 210);  // B
    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kZero, renderPass.color, 210, 190);   // C
}

// Test use of primitive restart with an Uint16 index format
TEST_P(IndexFormatTest, Uint16PrimitiveRestart) {
    wgpu::RenderPipeline pipeline = MakeTestPipeline(wgpu::IndexFormat::Uint16);

    wgpu::Buffer vertexBuffer = utils::CreateBufferFromData<float>(
        device, wgpu::BufferUsage::Vertex,
        {
            0.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
            0.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f,
        });
    wgpu::Buffer indexBuffer =
        utils::CreateBufferFromData<uint16_t>(device, wgpu::BufferUsage::Index,
                                              {
                                                  0,
                                                  1,
                                                  2,
                                                  0xFFFFu,
                                                  3,
                                                  4,
                                                  2,
                                                  // This value is for padding.
                                                  0xFFFFu,
                                              });

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetPipeline(pipeline);
        pass.SetVertexBuffer(0, vertexBuffer);
        pass.SetIndexBuffer(indexBuffer);
        pass.DrawIndexed(7);
        pass.EndPass();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 190, 190);  // A
    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 210, 210);  // B
    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kZero, renderPass.color, 210, 190);   // C
}

// Test that the index format used is the format of the last set pipeline. This is to
// prevent a case in D3D12 where the index format would be captured from the last
// pipeline on SetIndexBuffer.
TEST_P(IndexFormatTest, ChangePipelineAfterSetIndexBuffer) {
    DAWN_SKIP_TEST_IF(IsD3D12() || IsVulkan());

    wgpu::RenderPipeline pipeline32 = MakeTestPipeline(wgpu::IndexFormat::Uint32);
    wgpu::RenderPipeline pipeline16 = MakeTestPipeline(wgpu::IndexFormat::Uint16);

    wgpu::Buffer vertexBuffer = utils::CreateBufferFromData<float>(
        device, wgpu::BufferUsage::Vertex,
        {-1.0f, -1.0f, 0.0f, 1.0f,  // Note Vertices[0] = Vertices[1]
         -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f});
    // If this is interpreted as Uint16, then it would be 0, 1, 0, ... and would draw nothing.
    wgpu::Buffer indexBuffer =
        utils::CreateBufferFromData<uint32_t>(device, wgpu::BufferUsage::Index, {1, 2, 3});

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetPipeline(pipeline16);
        pass.SetVertexBuffer(0, vertexBuffer);
        pass.SetIndexBuffer(indexBuffer);
        pass.SetPipeline(pipeline32);
        pass.DrawIndexed(3);
        pass.EndPass();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 100, 300);
}

// Test that setting the index buffer before the pipeline works, this is important
// for backends where the index format is passed inside the call to SetIndexBuffer
// because it needs to be done lazily (to query the format from the last pipeline).
// TODO(cwallez@chromium.org): This is currently disallowed by the validation but
// we want to support eventually.
TEST_P(IndexFormatTest, DISABLED_SetIndexBufferBeforeSetPipeline) {
    wgpu::RenderPipeline pipeline = MakeTestPipeline(wgpu::IndexFormat::Uint32);

    wgpu::Buffer vertexBuffer = utils::CreateBufferFromData<float>(
        device, wgpu::BufferUsage::Vertex,
        {-1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f});
    wgpu::Buffer indexBuffer =
        utils::CreateBufferFromData<uint32_t>(device, wgpu::BufferUsage::Index, {0, 1, 2});

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetIndexBuffer(indexBuffer);
        pass.SetPipeline(pipeline);
        pass.SetVertexBuffer(0, vertexBuffer);
        pass.DrawIndexed(3);
        pass.EndPass();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), renderPass.color, 100, 300);
}

DAWN_INSTANTIATE_TEST(IndexFormatTest,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());
