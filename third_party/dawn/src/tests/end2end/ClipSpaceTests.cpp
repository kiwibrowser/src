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

class ClipSpaceTest : public DawnTest {
  protected:
    dawn::RenderPipeline CreatePipelineForTest() {
        utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);

        // Draw two triangles:
        // 1. The depth value of the top-left one is >= 0.5
        // 2. The depth value of the bottom-right one is <= 0.5
        const char* vs =
            R"(#version 450
        const vec3 pos[6] = vec3[6](vec3(-1.0f, -1.0f, 1.0f),
                                    vec3(-1.0f,  1.0f, 0.5f),
                                    vec3( 1.0f, -1.0f, 0.5f),
                                    vec3( 1.0f, -1.0f, 0.5f),
                                    vec3(-1.0f,  1.0f, 0.5f),
                                    vec3( 1.0f,  1.0f, 0.0f));
        void main() {
           gl_Position = vec4(pos[gl_VertexIndex], 1.0);
       })";
        pipelineDescriptor.cVertexStage.module =
            utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, vs);

        const char* fs =
            "#version 450\n"
            "layout(location = 0) out vec4 fragColor;"
            "void main() {\n"
            "   fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
            "}\n";
        pipelineDescriptor.cFragmentStage.module =
            utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, fs);

        pipelineDescriptor.cDepthStencilState.depthCompare = dawn::CompareFunction::LessEqual;
        pipelineDescriptor.depthStencilState = &pipelineDescriptor.cDepthStencilState;

        return device.CreateRenderPipeline(&pipelineDescriptor);
    }

    dawn::Texture Create2DTextureForTest(dawn::TextureFormat format) {
        dawn::TextureDescriptor textureDescriptor;
        textureDescriptor.dimension = dawn::TextureDimension::e2D;
        textureDescriptor.format = format;
        textureDescriptor.usage =
            dawn::TextureUsageBit::OutputAttachment | dawn::TextureUsageBit::TransferSrc;
        textureDescriptor.arrayLayerCount = 1;
        textureDescriptor.mipLevelCount = 1;
        textureDescriptor.sampleCount = 1;
        textureDescriptor.size = {kSize, kSize, 1};
        return device.CreateTexture(&textureDescriptor);
    }

    static constexpr uint32_t kSize = 4;
};

// Test that the clip space is correctly configured.
TEST_P(ClipSpaceTest, ClipSpace) {
    dawn::Texture colorTexture = Create2DTextureForTest(dawn::TextureFormat::R8G8B8A8Unorm);
    dawn::Texture depthStencilTexture = Create2DTextureForTest(dawn::TextureFormat::D32FloatS8Uint);

    utils::ComboRenderPassDescriptor renderPassDescriptor(
        {colorTexture.CreateDefaultView()}, depthStencilTexture.CreateDefaultView());
    renderPassDescriptor.cColorAttachmentsInfoPtr[0]->clearColor = {0.0, 1.0, 0.0, 1.0};
    renderPassDescriptor.cColorAttachmentsInfoPtr[0]->loadOp = dawn::LoadOp::Clear;

    // Clear the depth stencil attachment to 0.5f, so only the bottom-right triangle should be
    // drawn.
    renderPassDescriptor.cDepthStencilAttachmentInfo.clearDepth = 0.5f;
    renderPassDescriptor.cDepthStencilAttachmentInfo.depthLoadOp = dawn::LoadOp::Clear;

    dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    dawn::RenderPassEncoder renderPass = commandEncoder.BeginRenderPass(&renderPassDescriptor);
    renderPass.SetPipeline(CreatePipelineForTest());
    renderPass.Draw(6, 1, 0, 0);
    renderPass.EndPass();
    dawn::CommandBuffer commandBuffer = commandEncoder.Finish();
    dawn::Queue queue = device.CreateQueue();
    queue.Submit(1, &commandBuffer);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8(255, 0, 0, 255), colorTexture, kSize - 1, kSize - 1);
    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), colorTexture, 0, 0);
}

DAWN_INSTANTIATE_TEST(ClipSpaceTest, D3D12Backend, MetalBackend, OpenGLBackend, VulkanBackend);
