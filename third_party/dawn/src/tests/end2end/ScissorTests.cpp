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

#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

class ScissorTest : public DawnTest {
  protected:
    wgpu::RenderPipeline CreateQuadPipeline(wgpu::TextureFormat format) {
        wgpu::ShaderModule vsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
            #version 450
            const vec2 pos[6] = vec2[6](
                vec2(-1.0f, -1.0f), vec2(-1.0f, 1.0f), vec2(1.0f, -1.0f),
                vec2(1.0f, 1.0f), vec2(-1.0f, 1.0f), vec2(1.0f, -1.0f)
            );
            void main() {
                gl_Position = vec4(pos[gl_VertexIndex], 0.5, 1.0);
            })");

        wgpu::ShaderModule fsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
            #version 450
            layout(location = 0) out vec4 fragColor;
            void main() {
                fragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
            })");

        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.cColorStates[0].format = format;

        return device.CreateRenderPipeline(&descriptor);
    }
};

// Test that by default the scissor test is disabled and the whole attachment can be drawn to.
TEST_P(ScissorTest, DefaultsToWholeRenderTarget) {
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, 100, 100);
    wgpu::RenderPipeline pipeline = CreateQuadPipeline(renderPass.colorFormat);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetPipeline(pipeline);
        pass.Draw(6);
        pass.EndPass();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 0, 0);
    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 0, 99);
    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 99, 0);
    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 99, 99);
}

// Test setting the scissor to something larger than the attachments.
TEST_P(ScissorTest, LargerThanAttachment) {
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, 100, 100);
    wgpu::RenderPipeline pipeline = CreateQuadPipeline(renderPass.colorFormat);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetPipeline(pipeline);
        pass.SetScissorRect(0, 0, 200, 200);
        pass.Draw(6);
        pass.EndPass();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 0, 0);
    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 0, 99);
    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 99, 0);
    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 99, 99);
}

// Test setting a partial scissor (not empty, not full attachment)
TEST_P(ScissorTest, PartialRect) {
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, 100, 100);
    wgpu::RenderPipeline pipeline = CreateQuadPipeline(renderPass.colorFormat);

    constexpr uint32_t kX = 3;
    constexpr uint32_t kY = 7;
    constexpr uint32_t kW = 5;
    constexpr uint32_t kH = 13;

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetPipeline(pipeline);
        pass.SetScissorRect(kX, kY, kW, kH);
        pass.Draw(6);
        pass.EndPass();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Test the two opposite corners of the scissor box. With one pixel inside and on outside
    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kZero, renderPass.color, kX - 1, kY - 1);
    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, kX, kY);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kZero, renderPass.color, kX + kW, kY + kH);
    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, kX + kW - 1, kY + kH - 1);
}

// Test that the scissor setting doesn't get inherited between renderpasses
TEST_P(ScissorTest, NoInheritanceBetweenRenderPass) {
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, 100, 100);
    wgpu::RenderPipeline pipeline = CreateQuadPipeline(renderPass.colorFormat);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    // RenderPass 1 set the scissor
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetScissorRect(1, 1, 1, 1);
        pass.EndPass();
    }
    // RenderPass 2 draw a full quad, it shouldn't be scissored
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetPipeline(pipeline);
        pass.Draw(6);
        pass.EndPass();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 0, 0);
    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 0, 99);
    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 99, 0);
    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 99, 99);
}

DAWN_INSTANTIATE_TEST(ScissorTest,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());
