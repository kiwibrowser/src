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

#include "tests/unittests/validation/ValidationTest.h"

namespace {

class TextureViewValidationTest : public ValidationTest {
};

constexpr uint32_t kWidth = 32u;
constexpr uint32_t kHeight = 32u;
constexpr uint32_t kDefaultMipLevels = 6u;

constexpr dawn::TextureFormat kDefaultTextureFormat = dawn::TextureFormat::R8G8B8A8Unorm;

dawn::Texture Create2DArrayTexture(dawn::Device& device,
                                   uint32_t arrayLayerCount,
                                   uint32_t width = kWidth,
                                   uint32_t height = kHeight,
                                   uint32_t mipLevelCount = kDefaultMipLevels,
                                   uint32_t sampleCount = 1) {
    dawn::TextureDescriptor descriptor;
    descriptor.dimension = dawn::TextureDimension::e2D;
    descriptor.size.width = width;
    descriptor.size.height = height;
    descriptor.size.depth = 1;
    descriptor.arrayLayerCount = arrayLayerCount;
    descriptor.sampleCount = sampleCount;
    descriptor.format = kDefaultTextureFormat;
    descriptor.mipLevelCount = mipLevelCount;
    descriptor.usage = dawn::TextureUsageBit::Sampled;
    return device.CreateTexture(&descriptor);
}

dawn::TextureViewDescriptor CreateDefaultViewDescriptor(dawn::TextureViewDimension dimension) {
    dawn::TextureViewDescriptor descriptor;
    descriptor.format = kDefaultTextureFormat;
    descriptor.dimension = dimension;
    descriptor.baseMipLevel = 0;
    descriptor.mipLevelCount = kDefaultMipLevels;
    descriptor.baseArrayLayer = 0;
    descriptor.arrayLayerCount = 1;
    return descriptor;
}

// Test creating texture view on a 2D non-array texture
TEST_F(TextureViewValidationTest, CreateTextureViewOnTexture2D) {
    dawn::Texture texture = Create2DArrayTexture(device, 1);

    dawn::TextureViewDescriptor base2DTextureViewDescriptor =
        CreateDefaultViewDescriptor(dawn::TextureViewDimension::e2D);

    // It is OK to create a 2D texture view on a 2D texture.
    {
        dawn::TextureViewDescriptor descriptor = base2DTextureViewDescriptor;
        descriptor.arrayLayerCount = 1;
        texture.CreateView(&descriptor);
    }

    // It is an error to specify the layer count of the texture view > 1 when texture view dimension
    // is 2D.
    {
        dawn::TextureViewDescriptor descriptor = base2DTextureViewDescriptor;
        descriptor.arrayLayerCount = 2;
        ASSERT_DEVICE_ERROR(texture.CreateView(&descriptor));
    }

    // It is OK to create a 1-layer 2D array texture view on a 2D texture.
    {
        dawn::TextureViewDescriptor descriptor = base2DTextureViewDescriptor;
        descriptor.dimension = dawn::TextureViewDimension::e2DArray;
        descriptor.arrayLayerCount = 1;
        texture.CreateView(&descriptor);
    }

    // It is an error to specify mipLevelCount == 0.
    {
        dawn::TextureViewDescriptor descriptor = base2DTextureViewDescriptor;
        descriptor.mipLevelCount = 0;
        ASSERT_DEVICE_ERROR(texture.CreateView(&descriptor));
    }

    // It is an error to make the mip level out of range.
    {
        dawn::TextureViewDescriptor descriptor = base2DTextureViewDescriptor;
        descriptor.baseMipLevel = kDefaultMipLevels - 1;
        descriptor.mipLevelCount = 2;
        ASSERT_DEVICE_ERROR(texture.CreateView(&descriptor));
    }
}

// Test creating texture view on a 2D array texture
TEST_F(TextureViewValidationTest, CreateTextureViewOnTexture2DArray) {
    constexpr uint32_t kDefaultArrayLayers = 6;

    dawn::Texture texture = Create2DArrayTexture(device, kDefaultArrayLayers);

    dawn::TextureViewDescriptor base2DArrayTextureViewDescriptor =
        CreateDefaultViewDescriptor(dawn::TextureViewDimension::e2DArray);

    // It is OK to create a 2D texture view on a 2D array texture.
    {
        dawn::TextureViewDescriptor descriptor = base2DArrayTextureViewDescriptor;
        descriptor.dimension = dawn::TextureViewDimension::e2D;
        descriptor.arrayLayerCount = 1;
        texture.CreateView(&descriptor);
    }

    // It is OK to create a 2D array texture view on a 2D array texture.
    {
        dawn::TextureViewDescriptor descriptor = base2DArrayTextureViewDescriptor;
        descriptor.arrayLayerCount = kDefaultArrayLayers;
        texture.CreateView(&descriptor);
    }

    // It is an error to specify arrayLayerCount == 0.
    {
        dawn::TextureViewDescriptor descriptor = base2DArrayTextureViewDescriptor;
        descriptor.arrayLayerCount = 0;
        ASSERT_DEVICE_ERROR(texture.CreateView(&descriptor));
    }

    // It is an error to make the array layer out of range.
    {
        dawn::TextureViewDescriptor descriptor = base2DArrayTextureViewDescriptor;
        descriptor.arrayLayerCount = kDefaultArrayLayers + 1;
        ASSERT_DEVICE_ERROR(texture.CreateView(&descriptor));
    }
}

// Test creating cube map texture view
TEST_F(TextureViewValidationTest, CreateCubeMapTextureView) {
    constexpr uint32_t kDefaultArrayLayers = 16;

    dawn::Texture texture = Create2DArrayTexture(device, kDefaultArrayLayers);

    dawn::TextureViewDescriptor base2DArrayTextureViewDescriptor =
        CreateDefaultViewDescriptor(dawn::TextureViewDimension::e2DArray);

    // It is OK to create a cube map texture view with arrayLayerCount == 6.
    {
        dawn::TextureViewDescriptor descriptor = base2DArrayTextureViewDescriptor;
        descriptor.dimension = dawn::TextureViewDimension::Cube;
        descriptor.arrayLayerCount = 6;
        texture.CreateView(&descriptor);
    }

    // It is an error to create a cube map texture view with arrayLayerCount != 6.
    {
        dawn::TextureViewDescriptor descriptor = base2DArrayTextureViewDescriptor;
        descriptor.dimension = dawn::TextureViewDimension::Cube;
        descriptor.arrayLayerCount = 3;
        ASSERT_DEVICE_ERROR(texture.CreateView(&descriptor));
    }

    // It is OK to create a cube map array texture view with arrayLayerCount % 6 == 0.
    {
        dawn::TextureViewDescriptor descriptor = base2DArrayTextureViewDescriptor;
        descriptor.dimension = dawn::TextureViewDimension::CubeArray;
        descriptor.arrayLayerCount = 12;
        texture.CreateView(&descriptor);
    }

    // It is an error to create a cube map array texture view with arrayLayerCount % 6 != 0.
    {
        dawn::TextureViewDescriptor descriptor = base2DArrayTextureViewDescriptor;
        descriptor.dimension = dawn::TextureViewDimension::CubeArray;
        descriptor.arrayLayerCount = 11;
        ASSERT_DEVICE_ERROR(texture.CreateView(&descriptor));
    }

    // It is an error to create a cube map texture view with width != height.
    {
        dawn::Texture nonSquareTexture = Create2DArrayTexture(device, 18, 32, 16, 5);

        dawn::TextureViewDescriptor descriptor = base2DArrayTextureViewDescriptor;
        descriptor.dimension = dawn::TextureViewDimension::Cube;
        descriptor.arrayLayerCount = 6;
        ASSERT_DEVICE_ERROR(nonSquareTexture.CreateView(&descriptor));
    }

    // It is an error to create a cube map array texture view with width != height.
    {
        dawn::Texture nonSquareTexture = Create2DArrayTexture(device, 18, 32, 16, 5);

        dawn::TextureViewDescriptor descriptor = base2DArrayTextureViewDescriptor;
        descriptor.dimension = dawn::TextureViewDimension::CubeArray;
        descriptor.arrayLayerCount = 12;
        ASSERT_DEVICE_ERROR(nonSquareTexture.CreateView(&descriptor));
    }
}

// Test the format compatibility rules when creating a texture view.
// TODO(jiawei.shao@intel.com): add more tests when the rules are fully implemented.
TEST_F(TextureViewValidationTest, TextureViewFormatCompatibility) {
    dawn::Texture texture = Create2DArrayTexture(device, 1);

    dawn::TextureViewDescriptor base2DTextureViewDescriptor =
        CreateDefaultViewDescriptor(dawn::TextureViewDimension::e2D);

    // It is an error to create a texture view in depth-stencil format on a RGBA texture.
    {
        dawn::TextureViewDescriptor descriptor = base2DTextureViewDescriptor;
        descriptor.format = dawn::TextureFormat::D32FloatS8Uint;
        ASSERT_DEVICE_ERROR(texture.CreateView(&descriptor));
    }
}

// Test that it's invalid to create a texture view from a destroyed texture
TEST_F(TextureViewValidationTest, DestroyCreateTextureView) {
    dawn::Texture texture = Create2DArrayTexture(device, 1);
    dawn::TextureViewDescriptor descriptor =
        CreateDefaultViewDescriptor(dawn::TextureViewDimension::e2D);
    texture.Destroy();
    ASSERT_DEVICE_ERROR(texture.CreateView(&descriptor));
}
}
