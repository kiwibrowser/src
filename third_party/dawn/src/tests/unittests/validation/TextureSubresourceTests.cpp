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

#include "utils/WGPUHelpers.h"

#include "tests/unittests/validation/ValidationTest.h"

namespace {

    class TextureSubresourceTest : public ValidationTest {
      public:
        static constexpr uint32_t kSize = 32u;
        static constexpr wgpu::TextureFormat kFormat = wgpu::TextureFormat::RGBA8Unorm;

        wgpu::Texture CreateTexture(uint32_t mipLevelCount,
                                    uint32_t arrayLayerCount,
                                    wgpu::TextureUsage usage) {
            wgpu::TextureDescriptor texDesc;
            texDesc.dimension = wgpu::TextureDimension::e2D;
            texDesc.size = {kSize, kSize, arrayLayerCount};
            texDesc.sampleCount = 1;
            texDesc.mipLevelCount = mipLevelCount;
            texDesc.usage = usage;
            texDesc.format = kFormat;
            return device.CreateTexture(&texDesc);
        }

        wgpu::TextureView CreateTextureView(wgpu::Texture texture,
                                            uint32_t baseMipLevel,
                                            uint32_t baseArrayLayer) {
            wgpu::TextureViewDescriptor viewDesc;
            viewDesc.format = kFormat;
            viewDesc.baseArrayLayer = baseArrayLayer;
            viewDesc.arrayLayerCount = 1;
            viewDesc.baseMipLevel = baseMipLevel;
            viewDesc.mipLevelCount = 1;
            viewDesc.dimension = wgpu::TextureViewDimension::e2D;
            return texture.CreateView(&viewDesc);
        }

        void TestRenderPass(const wgpu::TextureView& renderView,
                            const wgpu::TextureView& samplerView) {
            // Create bind group
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Vertex, wgpu::BindingType::SampledTexture}});

            utils::ComboRenderPassDescriptor renderPassDesc({renderView});

            // It is valid to read from and write into different subresources of the same texture
            {
                wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, bgl, {{0, samplerView}});
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
                pass.SetBindGroup(0, bindGroup);
                pass.EndPass();
                encoder.Finish();
            }

            // It is valid to has multiple read from a subresource and one single write into another
            // subresource
            {
                wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, bgl, {{0, samplerView}});

                wgpu::BindGroupLayout bgl1 = utils::MakeBindGroupLayout(
                    device,
                    {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ReadonlyStorageTexture,
                      false, 0, false, wgpu::TextureViewDimension::Undefined,
                      wgpu::TextureComponentType::Float, kFormat}});

                wgpu::BindGroup bindGroup1 = utils::MakeBindGroup(device, bgl1, {{0, samplerView}});

                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
                pass.SetBindGroup(0, bindGroup);
                pass.SetBindGroup(1, bindGroup1);
                pass.EndPass();
                encoder.Finish();
            }

            // It is invalid to read and write into the same subresources
            {
                wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, bgl, {{0, renderView}});
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
                pass.SetBindGroup(0, bindGroup);
                pass.EndPass();
                ASSERT_DEVICE_ERROR(encoder.Finish());
            }

            // It is valid to write into and then read from the same level of a texture in different
            // render passes
            {
                wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, bgl, {{0, samplerView}});

                wgpu::BindGroupLayout bgl1 = utils::MakeBindGroupLayout(
                    device,
                    {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::WriteonlyStorageTexture,
                      false, 0, false, wgpu::TextureViewDimension::Undefined,
                      wgpu::TextureComponentType::Float, kFormat}});
                wgpu::BindGroup bindGroup1 = utils::MakeBindGroup(device, bgl1, {{0, samplerView}});

                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::RenderPassEncoder pass1 = encoder.BeginRenderPass(&renderPassDesc);
                pass1.SetBindGroup(0, bindGroup1);
                pass1.EndPass();

                wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
                pass.SetBindGroup(0, bindGroup);
                pass.EndPass();

                encoder.Finish();
            }
        }
    };

    // Test different mipmap levels
    TEST_F(TextureSubresourceTest, MipmapLevelsTest) {
        // Create texture with 2 mipmap levels and 1 layer
        wgpu::Texture texture =
            CreateTexture(2, 1,
                          wgpu::TextureUsage::Sampled | wgpu::TextureUsage::OutputAttachment |
                              wgpu::TextureUsage::Storage);

        // Create two views on different mipmap levels.
        wgpu::TextureView samplerView = CreateTextureView(texture, 0, 0);
        wgpu::TextureView renderView = CreateTextureView(texture, 1, 0);
        TestRenderPass(samplerView, renderView);
    }

    // Test different array layers
    TEST_F(TextureSubresourceTest, ArrayLayersTest) {
        // Create texture with 1 mipmap level and 2 layers
        wgpu::Texture texture =
            CreateTexture(1, 2,
                          wgpu::TextureUsage::Sampled | wgpu::TextureUsage::OutputAttachment |
                              wgpu::TextureUsage::Storage);

        // Create two views on different layers.
        wgpu::TextureView samplerView = CreateTextureView(texture, 0, 0);
        wgpu::TextureView renderView = CreateTextureView(texture, 0, 1);

        TestRenderPass(samplerView, renderView);
    }

    // TODO (yunchao.he@intel.com):
    //	* Add tests for compute, in which texture subresource is traced per dispatch.
    //
    //	* Add tests for multiple encoders upon the same resource simultaneously. This situation fits
    //	some cases like VR, multi-threading, etc.
    //
    //	* Add tests for conflicts between usages in two render bundles used in the same pass.

}  // anonymous namespace
