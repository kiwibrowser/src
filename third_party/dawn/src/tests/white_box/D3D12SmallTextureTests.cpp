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

#include "dawn_native/d3d12/TextureD3D12.h"

using namespace dawn_native::d3d12;

class D3D12SmallTextureTests : public DawnTest {
  protected:
    std::vector<const char*> GetRequiredExtensions() override {
        mIsBCFormatSupported = SupportsExtensions({"texture_compression_bc"});
        if (!mIsBCFormatSupported) {
            return {};
        }

        return {"texture_compression_bc"};
    }

    bool IsBCFormatSupported() const {
        return mIsBCFormatSupported;
    }

  private:
    bool mIsBCFormatSupported = false;
};

// Verify that creating a small compressed textures will be 4KB aligned.
TEST_P(D3D12SmallTextureTests, AlignSmallCompressedTexture) {
    DAWN_SKIP_TEST_IF(UsesWire());
    DAWN_SKIP_TEST_IF(!IsBCFormatSupported());

    // TODO(http://crbug.com/dawn/282): Investigate GPU/driver rejections of small alignment.
    DAWN_SKIP_TEST_IF(IsIntel() || IsNvidia() || IsWARP());

    wgpu::TextureDescriptor descriptor;
    descriptor.dimension = wgpu::TextureDimension::e2D;
    descriptor.size.width = 8;
    descriptor.size.height = 8;
    descriptor.size.depth = 1;
    descriptor.arrayLayerCount = 1;
    descriptor.sampleCount = 1;
    descriptor.format = wgpu::TextureFormat::BC1RGBAUnorm;
    descriptor.mipLevelCount = 1;
    descriptor.usage = wgpu::TextureUsage::Sampled;

    // Create a smaller one that allows use of the smaller alignment.
    wgpu::Texture texture = device.CreateTexture(&descriptor);
    Texture* d3dTexture = reinterpret_cast<Texture*>(texture.Get());

    EXPECT_EQ(d3dTexture->GetD3D12Resource()->GetDesc().Alignment,
              static_cast<uint64_t>(D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT));

    // Create a larger one (>64KB) that forbids use the smaller alignment.
    descriptor.size.width = 4096;
    descriptor.size.height = 4096;

    texture = device.CreateTexture(&descriptor);
    d3dTexture = reinterpret_cast<Texture*>(texture.Get());

    EXPECT_EQ(d3dTexture->GetD3D12Resource()->GetDesc().Alignment,
              static_cast<uint64_t>(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT));
}

DAWN_INSTANTIATE_TEST(D3D12SmallTextureTests, D3D12Backend());
