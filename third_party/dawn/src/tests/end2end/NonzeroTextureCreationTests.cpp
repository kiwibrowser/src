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

class NonzeroTextureCreationTests : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();
    }

    constexpr static uint32_t kSize = 128;
};

// Test that texture clears to 1's because toggle is enabled.
TEST_P(NonzeroTextureCreationTests, TextureCreationClearsOneBits) {
    dawn::TextureDescriptor descriptor;
    descriptor.dimension = dawn::TextureDimension::e2D;
    descriptor.size.width = kSize;
    descriptor.size.height = kSize;
    descriptor.size.depth = 1;
    descriptor.arrayLayerCount = 1;
    descriptor.sampleCount = 1;
    descriptor.format = dawn::TextureFormat::R8G8B8A8Unorm;
    descriptor.mipLevelCount = 1;
    descriptor.usage = dawn::TextureUsageBit::OutputAttachment | dawn::TextureUsageBit::TransferSrc;
    dawn::Texture texture = device.CreateTexture(&descriptor);

    RGBA8 filledWithOnes(255, 255, 255, 255);
    EXPECT_PIXEL_RGBA8_EQ(filledWithOnes, texture, 0, 0);
}

// Test that non-zero mip level clears to 1's because toggle is enabled.
TEST_P(NonzeroTextureCreationTests, MipMapClears) {
    constexpr uint32_t mipLevels = 4;

    dawn::TextureDescriptor descriptor;
    descriptor.dimension = dawn::TextureDimension::e2D;
    descriptor.size.width = kSize;
    descriptor.size.height = kSize;
    descriptor.size.depth = 1;
    descriptor.arrayLayerCount = 1;
    descriptor.sampleCount = 1;
    descriptor.format = dawn::TextureFormat::R8G8B8A8Unorm;
    descriptor.mipLevelCount = mipLevels;
    descriptor.usage = dawn::TextureUsageBit::OutputAttachment | dawn::TextureUsageBit::TransferSrc;
    dawn::Texture texture = device.CreateTexture(&descriptor);

    std::vector<RGBA8> expected;
    RGBA8 filledWithOnes(255, 255, 255, 255);
    for (uint32_t i = 0; i < kSize * kSize; ++i) {
        expected.push_back(filledWithOnes);
    }
    uint32_t mipSize = kSize >> 2;
    EXPECT_TEXTURE_RGBA8_EQ(expected.data(), texture, 0, 0, mipSize, mipSize, 2, 0);
}

// Test that non-zero array layers clears to 1's because toggle is enabled.
TEST_P(NonzeroTextureCreationTests, ArrayLayerClears) {
    constexpr uint32_t arrayLayers = 4;

    dawn::TextureDescriptor descriptor;
    descriptor.dimension = dawn::TextureDimension::e2D;
    descriptor.size.width = kSize;
    descriptor.size.height = kSize;
    descriptor.size.depth = 1;
    descriptor.arrayLayerCount = arrayLayers;
    descriptor.sampleCount = 1;
    descriptor.format = dawn::TextureFormat::R8G8B8A8Unorm;
    descriptor.mipLevelCount = 1;
    descriptor.usage = dawn::TextureUsageBit::OutputAttachment | dawn::TextureUsageBit::TransferSrc;
    dawn::Texture texture = device.CreateTexture(&descriptor);

    std::vector<RGBA8> expected;
    RGBA8 filledWithOnes(255, 255, 255, 255);
    for (uint32_t i = 0; i < kSize * kSize; ++i) {
        expected.push_back(filledWithOnes);
    }

    EXPECT_TEXTURE_RGBA8_EQ(expected.data(), texture, 0, 0, kSize, kSize, 0, 2);
}

DAWN_INSTANTIATE_TEST(NonzeroTextureCreationTests,
                      ForceWorkarounds(D3D12Backend,
                                       {"nonzero_clear_resources_on_creation_for_testing"},
                                       {"lazy_clear_resource_on_first_use"}),
                      ForceWorkarounds(OpenGLBackend,
                                       {"nonzero_clear_resources_on_creation_for_testing"},
                                       {"lazy_clear_resource_on_first_use"}),
                      ForceWorkarounds(VulkanBackend,
                                       {"nonzero_clear_resources_on_creation_for_testing"},
                                       {"lazy_clear_resource_on_first_use"}));
