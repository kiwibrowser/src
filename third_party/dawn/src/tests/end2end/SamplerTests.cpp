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

#include <cmath>

#include "tests/DawnTest.h"

#include "common/Assert.h"
#include "common/Constants.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/DawnHelpers.h"

constexpr static unsigned int kRTSize = 64;

namespace {
    struct AddressModeTestCase {
        dawn::AddressMode mMode;
        uint8_t mExpected2;
        uint8_t mExpected3;
    };
    AddressModeTestCase addressModes[] = {
        { dawn::AddressMode::Repeat,           0, 255, },
        { dawn::AddressMode::MirroredRepeat, 255,   0, },
        { dawn::AddressMode::ClampToEdge,    255, 255, },
    };
}

class SamplerTest : public DawnTest {
protected:
    void SetUp() override {
        DawnTest::SetUp();
        mRenderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

        mBindGroupLayout = utils::MakeBindGroupLayout(
            device, {
                        {0, dawn::ShaderStageBit::Fragment, dawn::BindingType::Sampler},
                        {1, dawn::ShaderStageBit::Fragment, dawn::BindingType::SampledTexture},
                    });

        auto pipelineLayout = utils::MakeBasicPipelineLayout(device, &mBindGroupLayout);

        auto vsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, R"(
            #version 450
            void main() {
                const vec2 pos[6] = vec2[6](vec2(-2.f, -2.f),
                                            vec2(-2.f,  2.f),
                                            vec2( 2.f, -2.f),
                                            vec2(-2.f,  2.f),
                                            vec2( 2.f, -2.f),
                                            vec2( 2.f,  2.f));
                gl_Position = vec4(pos[gl_VertexIndex], 0.f, 1.f);
            }
        )");
        auto fsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
            #version 450
            layout(set = 0, binding = 0) uniform sampler sampler0;
            layout(set = 0, binding = 1) uniform texture2D texture0;
            layout(location = 0) out vec4 fragColor;

            void main() {
                fragColor = texture(sampler2D(texture0, sampler0), gl_FragCoord.xy / 2.0);
            }
        )");

        utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
        pipelineDescriptor.layout = pipelineLayout;
        pipelineDescriptor.cVertexStage.module = vsModule;
        pipelineDescriptor.cFragmentStage.module = fsModule;
        pipelineDescriptor.cColorStates[0]->format = mRenderPass.colorFormat;

        mPipeline = device.CreateRenderPipeline(&pipelineDescriptor);

        dawn::TextureDescriptor descriptor;
        descriptor.dimension = dawn::TextureDimension::e2D;
        descriptor.size.width = 2;
        descriptor.size.height = 2;
        descriptor.size.depth = 1;
        descriptor.arrayLayerCount = 1;
        descriptor.sampleCount = 1;
        descriptor.format = dawn::TextureFormat::R8G8B8A8Unorm;
        descriptor.mipLevelCount = 1;
        descriptor.usage = dawn::TextureUsageBit::TransferDst | dawn::TextureUsageBit::Sampled;
        dawn::Texture texture = device.CreateTexture(&descriptor);

        // Create a 2x2 checkerboard texture, with black in the top left and bottom right corners.
        const uint32_t rowPixels = kTextureRowPitchAlignment / sizeof(RGBA8);
        RGBA8 data[rowPixels * 2];
        RGBA8 black(0, 0, 0, 255);
        RGBA8 white(255, 255, 255, 255);
        data[0] = data[rowPixels + 1] = black;
        data[1] = data[rowPixels] = white;

        dawn::Buffer stagingBuffer = utils::CreateBufferFromData(device, data, sizeof(data), dawn::BufferUsageBit::TransferSrc);
        dawn::BufferCopyView bufferCopyView = utils::CreateBufferCopyView(stagingBuffer, 0, 256, 0);
        dawn::TextureCopyView textureCopyView =
            utils::CreateTextureCopyView(texture, 0, 0, {0, 0, 0});
        dawn::Extent3D copySize = {2, 2, 1};

        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copySize);

        dawn::CommandBuffer copy = encoder.Finish();
        queue.Submit(1, &copy);

        mTextureView = texture.CreateDefaultView();
    }

    void TestAddressModes(AddressModeTestCase u, AddressModeTestCase v, AddressModeTestCase w) {
        dawn::Sampler sampler;
        {
            dawn::SamplerDescriptor descriptor;
            descriptor.minFilter = dawn::FilterMode::Nearest;
            descriptor.magFilter = dawn::FilterMode::Nearest;
            descriptor.mipmapFilter = dawn::FilterMode::Nearest;
            descriptor.addressModeU = u.mMode;
            descriptor.addressModeV = v.mMode;
            descriptor.addressModeW = w.mMode;
            descriptor.lodMinClamp = kLodMin;
            descriptor.lodMaxClamp = kLodMax;
            descriptor.compareFunction = dawn::CompareFunction::Never;
            sampler = device.CreateSampler(&descriptor);
        }

        dawn::BindGroup bindGroup = utils::MakeBindGroup(device, mBindGroupLayout, {
            {0, sampler},
            {1, mTextureView}
        });

        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&mRenderPass.renderPassInfo);
            pass.SetPipeline(mPipeline);
            pass.SetBindGroup(0, bindGroup, 0, nullptr);
            pass.Draw(6, 1, 0, 0);
            pass.EndPass();
        }

        dawn::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        RGBA8 expectedU2(u.mExpected2, u.mExpected2, u.mExpected2, 255);
        RGBA8 expectedU3(u.mExpected3, u.mExpected3, u.mExpected3, 255);
        RGBA8 expectedV2(v.mExpected2, v.mExpected2, v.mExpected2, 255);
        RGBA8 expectedV3(v.mExpected3, v.mExpected3, v.mExpected3, 255);
        RGBA8 black(0, 0, 0, 255);
        RGBA8 white(255, 255, 255, 255);
        EXPECT_PIXEL_RGBA8_EQ(black, mRenderPass.color, 0, 0);
        EXPECT_PIXEL_RGBA8_EQ(white, mRenderPass.color, 0, 1);
        EXPECT_PIXEL_RGBA8_EQ(white, mRenderPass.color, 1, 0);
        EXPECT_PIXEL_RGBA8_EQ(black, mRenderPass.color, 1, 1);
        EXPECT_PIXEL_RGBA8_EQ(expectedU2, mRenderPass.color, 2, 0);
        EXPECT_PIXEL_RGBA8_EQ(expectedU3, mRenderPass.color, 3, 0);
        EXPECT_PIXEL_RGBA8_EQ(expectedV2, mRenderPass.color, 0, 2);
        EXPECT_PIXEL_RGBA8_EQ(expectedV3, mRenderPass.color, 0, 3);
        // TODO: add tests for W address mode, once Dawn supports 3D textures
    }

    utils::BasicRenderPass mRenderPass;
    dawn::BindGroupLayout mBindGroupLayout;
    dawn::RenderPipeline mPipeline;
    dawn::TextureView mTextureView;
};

// Test drawing a rect with a checkerboard texture with different address modes.
TEST_P(SamplerTest, AddressMode) {
    for (auto u : addressModes) {
        for (auto v : addressModes) {
            for (auto w : addressModes) {
                TestAddressModes(u, v, w);
            }
        }
    }
}

DAWN_INSTANTIATE_TEST(SamplerTest, D3D12Backend, MetalBackend, OpenGLBackend, VulkanBackend);
