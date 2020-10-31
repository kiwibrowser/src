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

#include "common/Assert.h"
#include "common/Constants.h"
#include "common/Math.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

#include <array>

constexpr static unsigned int kRTSize = 64;
constexpr wgpu::TextureFormat kDefaultFormat = wgpu::TextureFormat::RGBA8Unorm;
constexpr uint32_t kBytesPerTexel = 4;

namespace {
    wgpu::Texture Create2DTexture(wgpu::Device device,
                                  uint32_t width,
                                  uint32_t height,
                                  uint32_t arrayLayerCount,
                                  uint32_t mipLevelCount,
                                  wgpu::TextureUsage usage) {
        wgpu::TextureDescriptor descriptor;
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size.width = width;
        descriptor.size.height = height;
        descriptor.size.depth = arrayLayerCount;
        descriptor.sampleCount = 1;
        descriptor.format = kDefaultFormat;
        descriptor.mipLevelCount = mipLevelCount;
        descriptor.usage = usage;
        return device.CreateTexture(&descriptor);
    }

    wgpu::ShaderModule CreateDefaultVertexShaderModule(wgpu::Device device) {
        return utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
            #version 450
            layout (location = 0) out vec2 o_texCoord;
            void main() {
                const vec2 pos[6] = vec2[6](vec2(-2.f, -2.f),
                                            vec2(-2.f,  2.f),
                                            vec2( 2.f, -2.f),
                                            vec2(-2.f,  2.f),
                                            vec2( 2.f, -2.f),
                                            vec2( 2.f,  2.f));
                const vec2 texCoord[6] = vec2[6](vec2(0.f, 0.f),
                                                 vec2(0.f, 1.f),
                                                 vec2(1.f, 0.f),
                                                 vec2(0.f, 1.f),
                                                 vec2(1.f, 0.f),
                                                 vec2(1.f, 1.f));
                gl_Position = vec4(pos[gl_VertexIndex], 0.f, 1.f);
                o_texCoord = texCoord[gl_VertexIndex];
            }
        )");
    }
}  // anonymous namespace

class TextureViewSamplingTest : public DawnTest {
  protected:
    // Generates an arbitrary pixel value per-layer-per-level, used for the "actual" uploaded
    // textures and the "expected" results.
    static int GenerateTestPixelValue(uint32_t layer, uint32_t level) {
        return static_cast<int>(level * 10) + static_cast<int>(layer + 1);
    }

    void SetUp() override {
        DawnTest::SetUp();

        mRenderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

        wgpu::FilterMode kFilterMode = wgpu::FilterMode::Nearest;
        wgpu::AddressMode kAddressMode = wgpu::AddressMode::ClampToEdge;

        wgpu::SamplerDescriptor samplerDescriptor = {};
        samplerDescriptor.minFilter = kFilterMode;
        samplerDescriptor.magFilter = kFilterMode;
        samplerDescriptor.mipmapFilter = kFilterMode;
        samplerDescriptor.addressModeU = kAddressMode;
        samplerDescriptor.addressModeV = kAddressMode;
        samplerDescriptor.addressModeW = kAddressMode;
        mSampler = device.CreateSampler(&samplerDescriptor);

        mVSModule = CreateDefaultVertexShaderModule(device);
    }

    void initTexture(uint32_t arrayLayerCount, uint32_t mipLevelCount) {
        ASSERT(arrayLayerCount > 0 && mipLevelCount > 0);

        const uint32_t textureWidthLevel0 = 1 << mipLevelCount;
        const uint32_t textureHeightLevel0 = 1 << mipLevelCount;
        constexpr wgpu::TextureUsage kUsage =
            wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::Sampled;
        mTexture = Create2DTexture(device, textureWidthLevel0, textureHeightLevel0, arrayLayerCount,
                                   mipLevelCount, kUsage);

        mDefaultTextureViewDescriptor.dimension = wgpu::TextureViewDimension::e2DArray;
        mDefaultTextureViewDescriptor.format = kDefaultFormat;
        mDefaultTextureViewDescriptor.baseMipLevel = 0;
        mDefaultTextureViewDescriptor.mipLevelCount = mipLevelCount;
        mDefaultTextureViewDescriptor.baseArrayLayer = 0;
        mDefaultTextureViewDescriptor.arrayLayerCount = arrayLayerCount;

        // Create a texture with pixel = (0, 0, 0, level * 10 + layer + 1) at level `level` and
        // layer `layer`.
        static_assert((kTextureBytesPerRowAlignment % sizeof(RGBA8)) == 0,
                      "Texture bytes per row alignment must be a multiple of sizeof(RGBA8).");
        constexpr uint32_t kPixelsPerRowPitch = kTextureBytesPerRowAlignment / sizeof(RGBA8);
        ASSERT_LE(textureWidthLevel0, kPixelsPerRowPitch);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        for (uint32_t layer = 0; layer < arrayLayerCount; ++layer) {
            for (uint32_t level = 0; level < mipLevelCount; ++level) {
                const uint32_t texWidth = textureWidthLevel0 >> level;
                const uint32_t texHeight = textureHeightLevel0 >> level;

                const int pixelValue = GenerateTestPixelValue(layer, level);

                constexpr uint32_t kPaddedTexWidth = kPixelsPerRowPitch;
                std::vector<RGBA8> data(kPaddedTexWidth * texHeight, RGBA8(0, 0, 0, pixelValue));
                wgpu::Buffer stagingBuffer = utils::CreateBufferFromData(
                    device, data.data(), data.size() * sizeof(RGBA8), wgpu::BufferUsage::CopySrc);
                wgpu::BufferCopyView bufferCopyView =
                    utils::CreateBufferCopyView(stagingBuffer, 0, kTextureBytesPerRowAlignment, 0);
                wgpu::TextureCopyView textureCopyView =
                    utils::CreateTextureCopyView(mTexture, level, {0, 0, layer});
                wgpu::Extent3D copySize = {texWidth, texHeight, 1};
                encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copySize);
            }
        }
        wgpu::CommandBuffer copy = encoder.Finish();
        queue.Submit(1, &copy);
    }

    void Verify(const wgpu::TextureView& textureView, const char* fragmentShader, int expected) {
        wgpu::ShaderModule fsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, fragmentShader);

        utils::ComboRenderPipelineDescriptor textureDescriptor(device);
        textureDescriptor.vertexStage.module = mVSModule;
        textureDescriptor.cFragmentStage.module = fsModule;
        textureDescriptor.cColorStates[0].format = mRenderPass.colorFormat;

        wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&textureDescriptor);

        wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                                         {{0, mSampler}, {1, textureView}});

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&mRenderPass.renderPassInfo);
            pass.SetPipeline(pipeline);
            pass.SetBindGroup(0, bindGroup);
            pass.Draw(6);
            pass.EndPass();
        }

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        RGBA8 expectedPixel(0, 0, 0, expected);
        EXPECT_PIXEL_RGBA8_EQ(expectedPixel, mRenderPass.color, 0, 0);
        EXPECT_PIXEL_RGBA8_EQ(expectedPixel, mRenderPass.color, mRenderPass.width - 1,
                              mRenderPass.height - 1);
        // TODO(jiawei.shao@intel.com): add tests for 3D textures once Dawn supports 3D textures
    }

    void Texture2DViewTest(uint32_t textureArrayLayers,
                           uint32_t textureMipLevels,
                           uint32_t textureViewBaseLayer,
                           uint32_t textureViewBaseMipLevel) {
        ASSERT(textureViewBaseLayer < textureArrayLayers);
        ASSERT(textureViewBaseMipLevel < textureMipLevels);

        initTexture(textureArrayLayers, textureMipLevels);

        wgpu::TextureViewDescriptor descriptor = mDefaultTextureViewDescriptor;
        descriptor.dimension = wgpu::TextureViewDimension::e2D;
        descriptor.baseArrayLayer = textureViewBaseLayer;
        descriptor.arrayLayerCount = 1;
        descriptor.baseMipLevel = textureViewBaseMipLevel;
        descriptor.mipLevelCount = 1;
        wgpu::TextureView textureView = mTexture.CreateView(&descriptor);

        const char* fragmentShader = R"(
            #version 450
            layout(set = 0, binding = 0) uniform sampler sampler0;
            layout(set = 0, binding = 1) uniform texture2D texture0;
            layout(location = 0) in vec2 texCoord;
            layout(location = 0) out vec4 fragColor;

            void main() {
                fragColor =
                    texture(sampler2D(texture0, sampler0), texCoord);
            }
        )";

        const int expected = GenerateTestPixelValue(textureViewBaseLayer, textureViewBaseMipLevel);
        Verify(textureView, fragmentShader, expected);
    }

    void Texture2DArrayViewTest(uint32_t textureArrayLayers,
                                uint32_t textureMipLevels,
                                uint32_t textureViewBaseLayer,
                                uint32_t textureViewBaseMipLevel) {
        ASSERT(textureViewBaseLayer < textureArrayLayers);
        ASSERT(textureViewBaseMipLevel < textureMipLevels);

        // We always set the layer count of the texture view to be 3 to match the fragment shader in
        // this test.
        constexpr uint32_t kTextureViewLayerCount = 3;
        ASSERT(textureArrayLayers >= textureViewBaseLayer + kTextureViewLayerCount);

        initTexture(textureArrayLayers, textureMipLevels);

        wgpu::TextureViewDescriptor descriptor = mDefaultTextureViewDescriptor;
        descriptor.dimension = wgpu::TextureViewDimension::e2DArray;
        descriptor.baseArrayLayer = textureViewBaseLayer;
        descriptor.arrayLayerCount = kTextureViewLayerCount;
        descriptor.baseMipLevel = textureViewBaseMipLevel;
        descriptor.mipLevelCount = 1;
        wgpu::TextureView textureView = mTexture.CreateView(&descriptor);

        const char* fragmentShader = R"(
            #version 450
            layout(set = 0, binding = 0) uniform sampler sampler0;
            layout(set = 0, binding = 1) uniform texture2DArray texture0;
            layout(location = 0) in vec2 texCoord;
            layout(location = 0) out vec4 fragColor;

            void main() {
                fragColor =
                    texture(sampler2DArray(texture0, sampler0), vec3(texCoord, 0)) +
                    texture(sampler2DArray(texture0, sampler0), vec3(texCoord, 1)) +
                    texture(sampler2DArray(texture0, sampler0), vec3(texCoord, 2));
            }
        )";

        int expected = 0;
        for (int i = 0; i < static_cast<int>(kTextureViewLayerCount); ++i) {
            expected += GenerateTestPixelValue(textureViewBaseLayer + i, textureViewBaseMipLevel);
        }
        Verify(textureView, fragmentShader, expected);
    }

    std::string CreateFragmentShaderForCubeMapFace(uint32_t layer, bool isCubeMapArray) {
        // Reference: https://en.wikipedia.org/wiki/Cube_mapping
        const std::array<std::string, 6> kCoordsToCubeMapFace = {{
            " 1.f,   tc,  -sc",  // Positive X
            "-1.f,   tc,   sc",  // Negative X
            "  sc,  1.f,  -tc",  // Positive Y
            "  sc, -1.f,   tc",  // Negative Y
            "  sc,   tc,  1.f",  // Positive Z
            " -sc,   tc, -1.f",  // Negative Z
        }};

        const std::string textureType = isCubeMapArray ? "textureCubeArray" : "textureCube";
        const std::string samplerType = isCubeMapArray ? "samplerCubeArray" : "samplerCube";
        const uint32_t cubeMapArrayIndex = layer / 6;
        const std::string coordToCubeMapFace = kCoordsToCubeMapFace[layer % 6];

        std::ostringstream stream;
        stream << R"(
            #version 450
            layout(set = 0, binding = 0) uniform sampler sampler0;
            layout(set = 0, binding = 1) uniform )"
               << textureType << R"( texture0;
            layout(location = 0) in vec2 texCoord;
            layout(location = 0) out vec4 fragColor;
            void main() {
                float sc = 2.f * texCoord.x - 1.f;
                float tc = 2.f * texCoord.y - 1.f;
                fragColor = texture()"
               << samplerType << "(texture0, sampler0), ";

        if (isCubeMapArray) {
            stream << "vec4(" << coordToCubeMapFace << ", " << cubeMapArrayIndex;
        } else {
            stream << "vec3(" << coordToCubeMapFace;
        }

        stream << R"());
            })";

        return stream.str();
    }

    void TextureCubeMapTest(uint32_t textureArrayLayers,
                            uint32_t textureViewBaseLayer,
                            uint32_t textureViewLayerCount,
                            bool isCubeMapArray) {
        constexpr uint32_t kMipLevels = 1u;
        initTexture(textureArrayLayers, kMipLevels);

        ASSERT_TRUE((textureViewLayerCount == 6) ||
                    (isCubeMapArray && textureViewLayerCount % 6 == 0));
        wgpu::TextureViewDimension dimension = (isCubeMapArray)
                                                   ? wgpu::TextureViewDimension::CubeArray
                                                   : wgpu::TextureViewDimension::Cube;

        wgpu::TextureViewDescriptor descriptor = mDefaultTextureViewDescriptor;
        descriptor.dimension = dimension;
        descriptor.baseArrayLayer = textureViewBaseLayer;
        descriptor.arrayLayerCount = textureViewLayerCount;

        wgpu::TextureView cubeMapTextureView = mTexture.CreateView(&descriptor);

        // Check the data in the every face of the cube map (array) texture view.
        for (uint32_t layer = 0; layer < textureViewLayerCount; ++layer) {
            const std::string& fragmentShader =
                CreateFragmentShaderForCubeMapFace(layer, isCubeMapArray);

            int expected = GenerateTestPixelValue(textureViewBaseLayer + layer, 0);
            Verify(cubeMapTextureView, fragmentShader.c_str(), expected);
        }
    }

    wgpu::Sampler mSampler;
    wgpu::Texture mTexture;
    wgpu::TextureViewDescriptor mDefaultTextureViewDescriptor;
    wgpu::ShaderModule mVSModule;
    utils::BasicRenderPass mRenderPass;
};

// Test drawing a rect with a 2D array texture.
TEST_P(TextureViewSamplingTest, Default2DArrayTexture) {
    // TODO(cwallez@chromium.org) understand what the issue is
    DAWN_SKIP_TEST_IF(IsVulkan() && IsNvidia());

    constexpr uint32_t kLayers = 3;
    constexpr uint32_t kMipLevels = 1;
    initTexture(kLayers, kMipLevels);

    wgpu::TextureView textureView = mTexture.CreateView();

    const char* fragmentShader = R"(
            #version 450
            layout(set = 0, binding = 0) uniform sampler sampler0;
            layout(set = 0, binding = 1) uniform texture2DArray texture0;
            layout(location = 0) in vec2 texCoord;
            layout(location = 0) out vec4 fragColor;

            void main() {
                fragColor =
                    texture(sampler2DArray(texture0, sampler0), vec3(texCoord, 0)) +
                    texture(sampler2DArray(texture0, sampler0), vec3(texCoord, 1)) +
                    texture(sampler2DArray(texture0, sampler0), vec3(texCoord, 2));
            }
        )";

    const int expected =
        GenerateTestPixelValue(0, 0) + GenerateTestPixelValue(1, 0) + GenerateTestPixelValue(2, 0);
    Verify(textureView, fragmentShader, expected);
}

// Test sampling from a 2D texture view created on a 2D array texture.
TEST_P(TextureViewSamplingTest, Texture2DViewOn2DArrayTexture) {
    Texture2DViewTest(6, 1, 4, 0);
}

// Test sampling from a 2D array texture view created on a 2D array texture.
TEST_P(TextureViewSamplingTest, Texture2DArrayViewOn2DArrayTexture) {
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());
    Texture2DArrayViewTest(6, 1, 2, 0);
}

// Test sampling from a 2D texture view created on a mipmap level of a 2D texture.
TEST_P(TextureViewSamplingTest, Texture2DViewOnOneLevelOf2DTexture) {
    Texture2DViewTest(1, 6, 0, 4);
}

// Test sampling from a 2D texture view created on a mipmap level of a 2D array texture layer.
TEST_P(TextureViewSamplingTest, Texture2DViewOnOneLevelOf2DArrayTexture) {
    Texture2DViewTest(6, 6, 3, 4);
}

// Test sampling from a 2D array texture view created on a mipmap level of a 2D array texture.
TEST_P(TextureViewSamplingTest, Texture2DArrayViewOnOneLevelOf2DArrayTexture) {
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());
    Texture2DArrayViewTest(6, 6, 2, 4);
}

// Test sampling from a cube map texture view that covers a whole 2D array texture.
TEST_P(TextureViewSamplingTest, TextureCubeMapOnWholeTexture) {
    constexpr uint32_t kTotalLayers = 6;
    TextureCubeMapTest(kTotalLayers, 0, kTotalLayers, false);
}

// Test sampling from a cube map texture view that covers a sub part of a 2D array texture.
TEST_P(TextureViewSamplingTest, TextureCubeMapViewOnPartOfTexture) {
    TextureCubeMapTest(10, 2, 6, false);
}

// Test sampling from a cube map texture view that covers the last layer of a 2D array texture.
TEST_P(TextureViewSamplingTest, TextureCubeMapViewCoveringLastLayer) {
    constexpr uint32_t kTotalLayers = 10;
    constexpr uint32_t kBaseLayer = 4;
    TextureCubeMapTest(kTotalLayers, kBaseLayer, kTotalLayers - kBaseLayer, false);
}

// Test sampling from a cube map texture array view that covers a whole 2D array texture.
TEST_P(TextureViewSamplingTest, TextureCubeMapArrayOnWholeTexture) {
    constexpr uint32_t kTotalLayers = 12;
    TextureCubeMapTest(kTotalLayers, 0, kTotalLayers, true);
}

// Test sampling from a cube map texture array view that covers a sub part of a 2D array texture.
TEST_P(TextureViewSamplingTest, TextureCubeMapArrayViewOnPartOfTexture) {
    // Test failing on the GPU FYI Mac Pro (AMD), see
    // https://bugs.chromium.org/p/dawn/issues/detail?id=58
    DAWN_SKIP_TEST_IF(IsMacOS() && IsMetal() && IsAMD());

    TextureCubeMapTest(20, 3, 12, true);
}

// Test sampling from a cube map texture array view that covers the last layer of a 2D array
// texture.
TEST_P(TextureViewSamplingTest, TextureCubeMapArrayViewCoveringLastLayer) {
    // Test failing on the GPU FYI Mac Pro (AMD), see
    // https://bugs.chromium.org/p/dawn/issues/detail?id=58
    DAWN_SKIP_TEST_IF(IsMacOS() && IsMetal() && IsAMD());

    constexpr uint32_t kTotalLayers = 20;
    constexpr uint32_t kBaseLayer = 8;
    TextureCubeMapTest(kTotalLayers, kBaseLayer, kTotalLayers - kBaseLayer, true);
}

// Test sampling from a cube map array texture view that only has a single cube map.
TEST_P(TextureViewSamplingTest, TextureCubeMapArrayViewSingleCubeMap) {
    // Test failing on the GPU FYI Mac Pro (AMD), see
    // https://bugs.chromium.org/p/dawn/issues/detail?id=58
    DAWN_SKIP_TEST_IF(IsMacOS() && IsMetal() && IsAMD());

    TextureCubeMapTest(20, 7, 6, true);
}

class TextureViewRenderingTest : public DawnTest {
  protected:
    void TextureLayerAsColorAttachmentTest(wgpu::TextureViewDimension dimension,
                                           uint32_t layerCount,
                                           uint32_t levelCount,
                                           uint32_t textureViewBaseLayer,
                                           uint32_t textureViewBaseLevel) {
        ASSERT(dimension == wgpu::TextureViewDimension::e2D ||
               dimension == wgpu::TextureViewDimension::e2DArray);
        ASSERT_LT(textureViewBaseLayer, layerCount);
        ASSERT_LT(textureViewBaseLevel, levelCount);

        const uint32_t textureWidthLevel0 = 1 << levelCount;
        const uint32_t textureHeightLevel0 = 1 << levelCount;
        constexpr wgpu::TextureUsage kUsage =
            wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc;
        wgpu::Texture texture = Create2DTexture(device, textureWidthLevel0, textureHeightLevel0,
                                                layerCount, levelCount, kUsage);

        wgpu::TextureViewDescriptor descriptor;
        descriptor.format = kDefaultFormat;
        descriptor.dimension = dimension;
        descriptor.baseArrayLayer = textureViewBaseLayer;
        descriptor.arrayLayerCount = 1;
        descriptor.baseMipLevel = textureViewBaseLevel;
        descriptor.mipLevelCount = 1;
        wgpu::TextureView textureView = texture.CreateView(&descriptor);

        wgpu::ShaderModule vsModule = CreateDefaultVertexShaderModule(device);

        // Clear textureView with Red(255, 0, 0, 255) and render Green(0, 255, 0, 255) into it
        utils::ComboRenderPassDescriptor renderPassInfo({textureView});
        renderPassInfo.cColorAttachments[0].clearColor = {1.0f, 0.0f, 0.0f, 1.0f};

        const char* oneColorFragmentShader = R"(
            #version 450
            layout(location = 0) out vec4 fragColor;

            void main() {
                fragColor = vec4(0.0, 1.0, 0.0, 1.0);
            }
        )";
        wgpu::ShaderModule oneColorFsModule = utils::CreateShaderModule(
            device, utils::SingleShaderStage::Fragment, oneColorFragmentShader);

        utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
        pipelineDescriptor.vertexStage.module = vsModule;
        pipelineDescriptor.cFragmentStage.module = oneColorFsModule;
        pipelineDescriptor.cColorStates[0].format = kDefaultFormat;

        wgpu::RenderPipeline oneColorPipeline = device.CreateRenderPipeline(&pipelineDescriptor);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassInfo);
            pass.SetPipeline(oneColorPipeline);
            pass.Draw(6);
            pass.EndPass();
        }

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        // Check if the right pixels (Green) have been written into the right part of the texture.
        uint32_t textureViewWidth = textureWidthLevel0 >> textureViewBaseLevel;
        uint32_t textureViewHeight = textureHeightLevel0 >> textureViewBaseLevel;
        uint32_t bytesPerRow =
            Align(kBytesPerTexel * textureWidthLevel0, kTextureBytesPerRowAlignment);
        uint32_t expectedDataSize =
            bytesPerRow / kBytesPerTexel * (textureWidthLevel0 - 1) + textureHeightLevel0;
        constexpr RGBA8 kExpectedPixel(0, 255, 0, 255);
        std::vector<RGBA8> expected(expectedDataSize, kExpectedPixel);
        EXPECT_TEXTURE_RGBA8_EQ(expected.data(), texture, 0, 0, textureViewWidth, textureViewHeight,
                                textureViewBaseLevel, textureViewBaseLayer);
    }
};

// Test rendering into a 2D texture view created on a mipmap level of a 2D texture.
TEST_P(TextureViewRenderingTest, Texture2DViewOnALevelOf2DTextureAsColorAttachment) {
    constexpr uint32_t kLayers = 1;
    constexpr uint32_t kMipLevels = 4;
    constexpr uint32_t kBaseLayer = 0;

    // Rendering into the first level
    {
        constexpr uint32_t kBaseLevel = 0;
        TextureLayerAsColorAttachmentTest(wgpu::TextureViewDimension::e2D, kLayers, kMipLevels,
                                          kBaseLayer, kBaseLevel);
    }

    // Rendering into the last level
    {
        constexpr uint32_t kBaseLevel = kMipLevels - 1;
        TextureLayerAsColorAttachmentTest(wgpu::TextureViewDimension::e2D, kLayers, kMipLevels,
                                          kBaseLayer, kBaseLevel);
    }
}

// Test rendering into a 2D texture view created on a layer of a 2D array texture.
TEST_P(TextureViewRenderingTest, Texture2DViewOnALayerOf2DArrayTextureAsColorAttachment) {
    constexpr uint32_t kMipLevels = 1;
    constexpr uint32_t kBaseLevel = 0;
    constexpr uint32_t kLayers = 10;

    // Rendering into the first layer
    {
        constexpr uint32_t kBaseLayer = 0;
        TextureLayerAsColorAttachmentTest(wgpu::TextureViewDimension::e2D, kLayers, kMipLevels,
                                          kBaseLayer, kBaseLevel);
    }

    // Rendering into the last layer
    {
        constexpr uint32_t kBaseLayer = kLayers - 1;
        TextureLayerAsColorAttachmentTest(wgpu::TextureViewDimension::e2D, kLayers, kMipLevels,
                                          kBaseLayer, kBaseLevel);
    }
}

// Test rendering into a 1-layer 2D array texture view created on a mipmap level of a 2D texture.
TEST_P(TextureViewRenderingTest, Texture2DArrayViewOnALevelOf2DTextureAsColorAttachment) {
    constexpr uint32_t kLayers = 1;
    constexpr uint32_t kMipLevels = 4;
    constexpr uint32_t kBaseLayer = 0;

    // Rendering into the first level
    {
        constexpr uint32_t kBaseLevel = 0;
        TextureLayerAsColorAttachmentTest(wgpu::TextureViewDimension::e2DArray, kLayers, kMipLevels,
                                          kBaseLayer, kBaseLevel);
    }

    // Rendering into the last level
    {
        constexpr uint32_t kBaseLevel = kMipLevels - 1;
        TextureLayerAsColorAttachmentTest(wgpu::TextureViewDimension::e2DArray, kLayers, kMipLevels,
                                          kBaseLayer, kBaseLevel);
    }
}

// Test rendering into a 1-layer 2D array texture view created on a layer of a 2D array texture.
TEST_P(TextureViewRenderingTest, Texture2DArrayViewOnALayerOf2DArrayTextureAsColorAttachment) {
    constexpr uint32_t kMipLevels = 1;
    constexpr uint32_t kBaseLevel = 0;
    constexpr uint32_t kLayers = 10;

    // Rendering into the first layer
    {
        constexpr uint32_t kBaseLayer = 0;
        TextureLayerAsColorAttachmentTest(wgpu::TextureViewDimension::e2DArray, kLayers, kMipLevels,
                                          kBaseLayer, kBaseLevel);
    }

    // Rendering into the last layer
    {
        constexpr uint32_t kBaseLayer = kLayers - 1;
        TextureLayerAsColorAttachmentTest(wgpu::TextureViewDimension::e2DArray, kLayers, kMipLevels,
                                          kBaseLayer, kBaseLevel);
    }
}

DAWN_INSTANTIATE_TEST(TextureViewSamplingTest,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());

DAWN_INSTANTIATE_TEST(TextureViewRenderingTest,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());

class TextureViewTest : public DawnTest {};

// This is a regression test for crbug.com/dawn/399 where creating a texture view with only copy
// usage would cause the Vulkan validation layers to warn
TEST_P(TextureViewTest, OnlyCopySrcDst) {
    wgpu::TextureDescriptor descriptor;
    descriptor.size = {4, 4, 1};
    descriptor.usage = wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst;
    descriptor.format = wgpu::TextureFormat::RGBA8Unorm;

    wgpu::Texture texture = device.CreateTexture(&descriptor);
    wgpu::TextureView view = texture.CreateView();
}

DAWN_INSTANTIATE_TEST(TextureViewTest,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());
