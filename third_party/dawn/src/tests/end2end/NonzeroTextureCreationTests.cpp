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

#include "common/Constants.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

class NonzeroTextureCreationTests : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();
    }

    constexpr static uint32_t kSize = 128;
};

// Test that texture clears 0xFF because toggle is enabled.
TEST_P(NonzeroTextureCreationTests, TextureCreationClears) {
    wgpu::TextureDescriptor descriptor;
    descriptor.dimension = wgpu::TextureDimension::e2D;
    descriptor.size.width = kSize;
    descriptor.size.height = kSize;
    descriptor.size.depth = 1;
    descriptor.sampleCount = 1;
    descriptor.format = wgpu::TextureFormat::RGBA8Unorm;
    descriptor.mipLevelCount = 1;
    descriptor.usage = wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc;
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    RGBA8 filled(255, 255, 255, 255);
    EXPECT_PIXEL_RGBA8_EQ(filled, texture, 0, 0);
}

// Test that a depth texture clears 0xFF because toggle is enabled.
TEST_P(NonzeroTextureCreationTests, Depth32TextureCreationDepthClears) {
    // Copies from depth textures not supported on the OpenGL backend right now.
    DAWN_SKIP_TEST_IF(IsOpenGL());

    // Closing the pending command list crashes flakily on D3D12 NVIDIA only.
    DAWN_SKIP_TEST_IF(IsD3D12() && IsNvidia());

    wgpu::TextureDescriptor descriptor;
    descriptor.dimension = wgpu::TextureDimension::e2D;
    descriptor.size.width = kSize;
    descriptor.size.height = kSize;
    descriptor.size.depth = 1;
    descriptor.sampleCount = 1;
    descriptor.mipLevelCount = 1;
    descriptor.usage = wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc;
    descriptor.format = wgpu::TextureFormat::Depth32Float;

    // We can only really test Depth32Float here because Depth24Plus(Stencil8)? may be in an unknown
    // format.
    // TODO(crbug.com/dawn/145): Test other formats via sampling.
    wgpu::Texture texture = device.CreateTexture(&descriptor);
    EXPECT_PIXEL_FLOAT_EQ(1.f, texture, 0, 0);
}

// Test that non-zero mip level clears 0xFF because toggle is enabled.
TEST_P(NonzeroTextureCreationTests, MipMapClears) {
    constexpr uint32_t mipLevels = 4;

    wgpu::TextureDescriptor descriptor;
    descriptor.dimension = wgpu::TextureDimension::e2D;
    descriptor.size.width = kSize;
    descriptor.size.height = kSize;
    descriptor.size.depth = 1;
    descriptor.sampleCount = 1;
    descriptor.format = wgpu::TextureFormat::RGBA8Unorm;
    descriptor.mipLevelCount = mipLevels;
    descriptor.usage = wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc;
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    std::vector<RGBA8> expected;
    RGBA8 filled(255, 255, 255, 255);
    for (uint32_t i = 0; i < kSize * kSize; ++i) {
        expected.push_back(filled);
    }
    uint32_t mipSize = kSize >> 2;
    EXPECT_TEXTURE_RGBA8_EQ(expected.data(), texture, 0, 0, mipSize, mipSize, 2, 0);
}

// Test that non-zero array layers clears 0xFF because toggle is enabled.
TEST_P(NonzeroTextureCreationTests, ArrayLayerClears) {
    constexpr uint32_t arrayLayers = 4;

    wgpu::TextureDescriptor descriptor;
    descriptor.dimension = wgpu::TextureDimension::e2D;
    descriptor.size.width = kSize;
    descriptor.size.height = kSize;
    descriptor.size.depth = arrayLayers;
    descriptor.sampleCount = 1;
    descriptor.format = wgpu::TextureFormat::RGBA8Unorm;
    descriptor.mipLevelCount = 1;
    descriptor.usage = wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc;
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    std::vector<RGBA8> expected;
    RGBA8 filled(255, 255, 255, 255);
    for (uint32_t i = 0; i < kSize * kSize; ++i) {
        expected.push_back(filled);
    }

    EXPECT_TEXTURE_RGBA8_EQ(expected.data(), texture, 0, 0, kSize, kSize, 0, 2);
}

// Test that nonrenderable texture formats clear 0x01 because toggle is enabled
TEST_P(NonzeroTextureCreationTests, NonrenderableTextureFormat) {
    wgpu::TextureDescriptor descriptor;
    descriptor.dimension = wgpu::TextureDimension::e2D;
    descriptor.size.width = kSize;
    descriptor.size.height = kSize;
    descriptor.size.depth = 1;
    descriptor.sampleCount = 1;
    descriptor.format = wgpu::TextureFormat::RGBA8Snorm;
    descriptor.mipLevelCount = 1;
    descriptor.usage = wgpu::TextureUsage::CopySrc;
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    // Set buffer with dirty data so we know it is cleared by the lazy cleared texture copy
    uint32_t bufferSize = 4 * kSize * kSize;
    std::vector<uint8_t> data(bufferSize, 100);
    wgpu::Buffer bufferDst = utils::CreateBufferFromData(
        device, data.data(), static_cast<uint32_t>(data.size()), wgpu::BufferUsage::CopySrc);

    wgpu::BufferCopyView bufferCopyView = utils::CreateBufferCopyView(bufferDst, 0, kSize * 4, 0);
    wgpu::TextureCopyView textureCopyView = utils::CreateTextureCopyView(texture, 0, {0, 0, 0});
    wgpu::Extent3D copySize = {kSize, kSize, 1};

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyTextureToBuffer(&textureCopyView, &bufferCopyView, &copySize);
    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    std::vector<uint32_t> expected(bufferSize, 0x01010101);
    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), bufferDst, 0, 8);
}

// Test that textures with more than 1 array layers and nonrenderable texture formats clear to 0x01
// because toggle is enabled
TEST_P(NonzeroTextureCreationTests, NonRenderableTextureClearWithMultiArrayLayers) {
    wgpu::TextureDescriptor descriptor;
    descriptor.dimension = wgpu::TextureDimension::e2D;
    descriptor.size.width = kSize;
    descriptor.size.height = kSize;
    descriptor.size.depth = 2;
    descriptor.sampleCount = 1;
    descriptor.format = wgpu::TextureFormat::RGBA8Snorm;
    descriptor.mipLevelCount = 1;
    descriptor.usage = wgpu::TextureUsage::CopySrc;
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    // Set buffer with dirty data so we know it is cleared by the lazy cleared texture copy
    uint32_t bufferSize = 4 * kSize * kSize;
    std::vector<uint8_t> data(bufferSize, 100);
    wgpu::Buffer bufferDst = utils::CreateBufferFromData(
        device, data.data(), static_cast<uint32_t>(data.size()), wgpu::BufferUsage::CopySrc);

    wgpu::BufferCopyView bufferCopyView = utils::CreateBufferCopyView(bufferDst, 0, kSize * 4, 0);
    wgpu::TextureCopyView textureCopyView = utils::CreateTextureCopyView(texture, 0, {0, 0, 1});
    wgpu::Extent3D copySize = {kSize, kSize, 1};

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyTextureToBuffer(&textureCopyView, &bufferCopyView, &copySize);
    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    std::vector<uint32_t> expected(bufferSize, 0x01010101);
    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), bufferDst, 0, 8);
}

// Test that all subresources of a renderable texture are filled because the toggle is enabled.
TEST_P(NonzeroTextureCreationTests, AllSubresourcesFilled) {
    wgpu::TextureDescriptor baseDescriptor;
    baseDescriptor.dimension = wgpu::TextureDimension::e2D;
    baseDescriptor.size.width = kSize;
    baseDescriptor.size.height = kSize;
    baseDescriptor.size.depth = 1;
    baseDescriptor.sampleCount = 1;
    baseDescriptor.format = wgpu::TextureFormat::RGBA8Unorm;
    baseDescriptor.mipLevelCount = 1;
    baseDescriptor.usage = wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc;

    RGBA8 filled(255, 255, 255, 255);

    {
        wgpu::TextureDescriptor descriptor = baseDescriptor;
        // Some textures may be cleared with render pass load/store ops.
        // Test above the max attachment count.
        descriptor.size.depth = kMaxColorAttachments + 1;
        wgpu::Texture texture = device.CreateTexture(&descriptor);

        for (uint32_t i = 0; i < descriptor.size.depth; ++i) {
            EXPECT_TEXTURE_RGBA8_EQ(&filled, texture, 0, 0, 1, 1, 0, i);
        }
    }

    {
        wgpu::TextureDescriptor descriptor = baseDescriptor;
        descriptor.mipLevelCount = 3;
        wgpu::Texture texture = device.CreateTexture(&descriptor);

        for (uint32_t i = 0; i < descriptor.mipLevelCount; ++i) {
            EXPECT_TEXTURE_RGBA8_EQ(&filled, texture, 0, 0, 1, 1, i, 0);
        }
    }

    {
        wgpu::TextureDescriptor descriptor = baseDescriptor;
        // Some textures may be cleared with render pass load/store ops.
        // Test above the max attachment count.
        descriptor.size.depth = kMaxColorAttachments + 1;
        descriptor.mipLevelCount = 3;
        wgpu::Texture texture = device.CreateTexture(&descriptor);

        for (uint32_t i = 0; i < descriptor.size.depth; ++i) {
            for (uint32_t j = 0; j < descriptor.mipLevelCount; ++j) {
                EXPECT_TEXTURE_RGBA8_EQ(&filled, texture, 0, 0, 1, 1, j, i);
            }
        }
    }
}

// Test that all subresources of a nonrenderable texture are filled because the toggle is enabled.
TEST_P(NonzeroTextureCreationTests, NonRenderableAllSubresourcesFilled) {
    wgpu::TextureDescriptor baseDescriptor;
    baseDescriptor.dimension = wgpu::TextureDimension::e2D;
    baseDescriptor.size.width = kSize;
    baseDescriptor.size.height = kSize;
    baseDescriptor.size.depth = 1;
    baseDescriptor.sampleCount = 1;
    baseDescriptor.format = wgpu::TextureFormat::RGBA8Snorm;
    baseDescriptor.mipLevelCount = 1;
    baseDescriptor.usage = wgpu::TextureUsage::CopySrc;

    RGBA8 filled(1, 1, 1, 1);

    {
        wgpu::TextureDescriptor descriptor = baseDescriptor;
        // Some textures may be cleared with render pass load/store ops.
        // Test above the max attachment count.
        descriptor.size.depth = kMaxColorAttachments + 1;
        wgpu::Texture texture = device.CreateTexture(&descriptor);

        for (uint32_t i = 0; i < descriptor.size.depth; ++i) {
            EXPECT_TEXTURE_RGBA8_EQ(&filled, texture, 0, 0, 1, 1, 0, i);
        }
    }

    {
        wgpu::TextureDescriptor descriptor = baseDescriptor;
        descriptor.mipLevelCount = 3;
        wgpu::Texture texture = device.CreateTexture(&descriptor);

        for (uint32_t i = 0; i < descriptor.mipLevelCount; ++i) {
            EXPECT_TEXTURE_RGBA8_EQ(&filled, texture, 0, 0, 1, 1, i, 0);
        }
    }

    {
        wgpu::TextureDescriptor descriptor = baseDescriptor;
        // Some textures may be cleared with render pass load/store ops.
        // Test above the max attachment count.
        descriptor.size.depth = kMaxColorAttachments + 1;
        descriptor.mipLevelCount = 3;
        wgpu::Texture texture = device.CreateTexture(&descriptor);

        for (uint32_t i = 0; i < descriptor.size.depth; ++i) {
            for (uint32_t j = 0; j < descriptor.mipLevelCount; ++j) {
                EXPECT_TEXTURE_RGBA8_EQ(&filled, texture, 0, 0, 1, 1, j, i);
            }
        }
    }
}

DAWN_INSTANTIATE_TEST(NonzeroTextureCreationTests,
                      D3D12Backend({"nonzero_clear_resources_on_creation_for_testing"},
                                   {"lazy_clear_resource_on_first_use"}),
                      MetalBackend({"nonzero_clear_resources_on_creation_for_testing"},
                                   {"lazy_clear_resource_on_first_use"}),
                      OpenGLBackend({"nonzero_clear_resources_on_creation_for_testing"},
                                    {"lazy_clear_resource_on_first_use"}),
                      VulkanBackend({"nonzero_clear_resources_on_creation_for_testing"},
                                    {"lazy_clear_resource_on_first_use"}));
