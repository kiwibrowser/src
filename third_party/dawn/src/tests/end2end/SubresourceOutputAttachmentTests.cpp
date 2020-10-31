// Copyright 2020 The Dawn Authors
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

// Test that rendering to a subresource of a texture works.
class SubresourceOutputAttachmentTest : public DawnTest {
    constexpr static uint32_t kRTSize = 2;

  protected:
    enum class Type { Color, Depth, Stencil };

    void DoSingleTest(Type type,
                      wgpu::TextureFormat format,
                      wgpu::Texture renderTarget,
                      uint32_t textureSize,
                      uint32_t baseArrayLayer,
                      uint32_t baseMipLevel) {
        wgpu::TextureViewDescriptor renderTargetViewDesc;
        renderTargetViewDesc.baseArrayLayer = baseArrayLayer;
        renderTargetViewDesc.arrayLayerCount = 1;
        renderTargetViewDesc.baseMipLevel = baseMipLevel;
        renderTargetViewDesc.mipLevelCount = 1;
        wgpu::TextureView renderTargetView = renderTarget.CreateView(&renderTargetViewDesc);

        RGBA8 expectedColor(0, 255, 0, 255);
        float expectedDepth = 0.3f;
        uint8_t expectedStencil = 7;

        utils::ComboRenderPassDescriptor renderPass = [&]() {
            switch (type) {
                case Type::Color: {
                    utils::ComboRenderPassDescriptor renderPass({renderTargetView});
                    renderPass.cColorAttachments[0].clearColor = {
                        static_cast<float>(expectedColor.r) / 255.f,
                        static_cast<float>(expectedColor.g) / 255.f,
                        static_cast<float>(expectedColor.b) / 255.f,
                        static_cast<float>(expectedColor.a) / 255.f,
                    };
                    return renderPass;
                }
                case Type::Depth: {
                    utils::ComboRenderPassDescriptor renderPass({}, renderTargetView);
                    renderPass.cDepthStencilAttachmentInfo.clearDepth = expectedDepth;
                    return renderPass;
                }
                case Type::Stencil: {
                    utils::ComboRenderPassDescriptor renderPass({}, renderTargetView);
                    renderPass.cDepthStencilAttachmentInfo.clearStencil = expectedStencil;
                    return renderPass;
                }
                default:
                    UNREACHABLE();
            }
        }();

        wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&renderPass);
        passEncoder.EndPass();
        wgpu::CommandBuffer commands = commandEncoder.Finish();
        queue.Submit(1, &commands);

        const uint32_t renderTargetSize = textureSize >> baseMipLevel;
        switch (type) {
            case Type::Color: {
                std::vector<RGBA8> expected(renderTargetSize * renderTargetSize, expectedColor);
                EXPECT_TEXTURE_RGBA8_EQ(expected.data(), renderTarget, 0, 0, renderTargetSize,
                                        renderTargetSize, baseMipLevel, baseArrayLayer);
                break;
            }
            case Type::Depth: {
                std::vector<float> expected(renderTargetSize * renderTargetSize, expectedDepth);
                EXPECT_TEXTURE_FLOAT_EQ(expected.data(), renderTarget, 0, 0, renderTargetSize,
                                        renderTargetSize, baseMipLevel, baseArrayLayer);
                break;
            }
            case Type::Stencil:
                // TODO(crbug.com/dawn/439): sample / copy of the stencil aspect.
            default:
                UNREACHABLE();
        }
    }

    void DoTest(Type type) {
        constexpr uint32_t kArrayLayerCount = 5;
        constexpr uint32_t kMipLevelCount = 4;

        wgpu::TextureFormat format;
        switch (type) {
            case Type::Color:
                format = wgpu::TextureFormat::RGBA8Unorm;
                break;
            case Type::Depth:
                format = wgpu::TextureFormat::Depth32Float;
                break;
            case Type::Stencil:
                format = wgpu::TextureFormat::Depth24PlusStencil8;
                break;
            default:
                UNREACHABLE();
        }

        constexpr uint32_t kTextureSize = kRTSize << (kMipLevelCount - 1);

        wgpu::TextureDescriptor renderTargetDesc;
        renderTargetDesc.dimension = wgpu::TextureDimension::e2D;
        renderTargetDesc.size.width = kTextureSize;
        renderTargetDesc.size.height = kTextureSize;
        renderTargetDesc.size.depth = kArrayLayerCount;
        renderTargetDesc.sampleCount = 1;
        renderTargetDesc.format = format;
        renderTargetDesc.mipLevelCount = kMipLevelCount;
        renderTargetDesc.usage = wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc;

        wgpu::Texture renderTarget = device.CreateTexture(&renderTargetDesc);

        // Test rendering into the first, middle, and last of each of array layer and mip level.
        for (uint32_t arrayLayer : {0u, kArrayLayerCount / 2, kArrayLayerCount - 1u}) {
            for (uint32_t mipLevel : {0u, kMipLevelCount / 2, kMipLevelCount - 1u}) {
                DoSingleTest(type, format, renderTarget, kTextureSize, arrayLayer, mipLevel);
            }
        }
    }
};

// Test rendering into a subresource of a color texture
TEST_P(SubresourceOutputAttachmentTest, ColorTexture) {
    DoTest(Type::Color);
}

// Test rendering into a subresource of a depth texture
TEST_P(SubresourceOutputAttachmentTest, DepthTexture) {
    DoTest(Type::Depth);
}

// Test rendering into a subresource of a stencil texture
// TODO(crbug.com/dawn/439): sample / copy of the stencil aspect.
TEST_P(SubresourceOutputAttachmentTest, DISABLED_StencilTexture) {
    DoTest(Type::Stencil);
}

DAWN_INSTANTIATE_TEST(SubresourceOutputAttachmentTest,
                      D3D12Backend(),
                      D3D12Backend({}, {"use_d3d12_render_pass"}),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());
