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

constexpr uint32_t kRTSize = 16;
constexpr dawn::TextureFormat kFormat = dawn::TextureFormat::R8G8B8A8Unorm;

class RenderPassTest : public DawnTest {
protected:
    void SetUp() override {
        DawnTest::SetUp();

        // Shaders to draw a bottom-left triangle in blue.
        dawn::ShaderModule vsModule =
            utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, R"(
                #version 450
                void main() {
                    const vec2 pos[3] = vec2[3](
                        vec2(-1.f, -1.f), vec2(1.f, 1.f), vec2(-1.f, 1.f));
                    gl_Position = vec4(pos[gl_VertexIndex], 0.f, 1.f);
                 })");

        dawn::ShaderModule fsModule =
            utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
                #version 450
                layout(location = 0) out vec4 fragColor;
                void main() {
                    fragColor = vec4(0.0, 0.0, 1.0, 1.0);
                })");

        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.cVertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.primitiveTopology = dawn::PrimitiveTopology::TriangleStrip;
        descriptor.cColorStates[0]->format = kFormat;

        pipeline = device.CreateRenderPipeline(&descriptor);
    }

    dawn::Texture CreateDefault2DTexture() {
        dawn::TextureDescriptor descriptor;
        descriptor.dimension = dawn::TextureDimension::e2D;
        descriptor.size.width = kRTSize;
        descriptor.size.height = kRTSize;
        descriptor.size.depth = 1;
        descriptor.arrayLayerCount = 1;
        descriptor.sampleCount = 1;
        descriptor.format = kFormat;
        descriptor.mipLevelCount = 1;
        descriptor.usage =
            dawn::TextureUsageBit::OutputAttachment | dawn::TextureUsageBit::TransferSrc;
        return device.CreateTexture(&descriptor);
    }

    dawn::RenderPipeline pipeline;
};

// Test using two different render passes in one commandBuffer works correctly.
TEST_P(RenderPassTest, TwoRenderPassesInOneCommandBuffer) {
    if (IsOpenGL() || IsMetal()) {
      // crbug.com/950768
      // This test is consistently failing on OpenGL and flaky on Metal.
      return;
    }
    constexpr RGBA8 kRed(255, 0, 0, 255);
    constexpr RGBA8 kGreen(0, 255, 0, 255);

    constexpr RGBA8 kBlue(0, 0, 255, 255);

    dawn::Texture renderTarget1 = CreateDefault2DTexture();
    dawn::Texture renderTarget2 = CreateDefault2DTexture();
    dawn::CommandEncoder encoder = device.CreateCommandEncoder();

    {
        // In the first render pass we clear renderTarget1 to red and draw a blue triangle in the
        // bottom left of renderTarget1.
        utils::ComboRenderPassDescriptor renderPass({renderTarget1.CreateDefaultView()});
        renderPass.cColorAttachmentsInfoPtr[0]->clearColor = {1.0f, 0.0f, 0.0f, 1.0f};

        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(pipeline);
        pass.Draw(3, 1, 0, 0);
        pass.EndPass();
    }

    {
        // In the second render pass we clear renderTarget2 to green and draw a blue triangle in the
        // bottom left of renderTarget2.
        utils::ComboRenderPassDescriptor renderPass({renderTarget2.CreateDefaultView()});
        renderPass.cColorAttachmentsInfoPtr[0]->clearColor = {0.0f, 1.0f, 0.0f, 1.0f};

        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(pipeline);
        pass.Draw(3, 1, 0, 0);
        pass.EndPass();
    }

    dawn::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(kBlue, renderTarget1, 1, kRTSize - 1);
    EXPECT_PIXEL_RGBA8_EQ(kRed, renderTarget1, kRTSize - 1, 1);

    EXPECT_PIXEL_RGBA8_EQ(kBlue, renderTarget2, 1, kRTSize - 1);
    EXPECT_PIXEL_RGBA8_EQ(kGreen, renderTarget2, kRTSize - 1, 1);
}

DAWN_INSTANTIATE_TEST(RenderPassTest, D3D12Backend, MetalBackend, OpenGLBackend, VulkanBackend);
