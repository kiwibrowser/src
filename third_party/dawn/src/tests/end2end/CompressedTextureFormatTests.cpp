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

#include "common/Assert.h"
#include "common/Constants.h"
#include "common/Math.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/TextureFormatUtils.h"
#include "utils/WGPUHelpers.h"

// The helper struct to configure the copies between buffers and textures.
struct CopyConfig {
    wgpu::TextureDescriptor textureDescriptor;
    wgpu::Extent3D copyExtent3D;
    wgpu::Origin3D copyOrigin3D = {0, 0, 0};
    uint32_t viewMipmapLevel = 0;
    uint32_t bufferOffset = 0;
    uint32_t bytesPerRowAlignment = kTextureBytesPerRowAlignment;
    uint32_t rowsPerImage = 0;
};

class CompressedTextureBCFormatTest : public DawnTest {
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

    // Compute the upload data for the copyConfig.
    std::vector<uint8_t> UploadData(const CopyConfig& copyConfig) {
        uint32_t copyWidthInBlock = copyConfig.copyExtent3D.width / kBCBlockWidthInTexels;
        uint32_t copyHeightInBlock = copyConfig.copyExtent3D.height / kBCBlockHeightInTexels;
        uint32_t rowPitchInBytes = 0;
        if (copyConfig.bytesPerRowAlignment != 0) {
            rowPitchInBytes = copyConfig.bytesPerRowAlignment;
        } else {
            rowPitchInBytes = copyWidthInBlock *
                              utils::GetTexelBlockSizeInBytes(copyConfig.textureDescriptor.format);
        }
        uint32_t copyRowsPerImageInBlock = copyConfig.rowsPerImage / kBCBlockHeightInTexels;
        if (copyRowsPerImageInBlock == 0) {
            copyRowsPerImageInBlock = copyHeightInBlock;
        }
        uint32_t copyBytesPerImage = rowPitchInBytes * copyRowsPerImageInBlock;
        uint32_t uploadBufferSize =
            copyConfig.bufferOffset + copyBytesPerImage * copyConfig.copyExtent3D.depth;

        // Fill data with the pre-prepared one-block compressed texture data.
        std::vector<uint8_t> data(uploadBufferSize, 0);
        std::vector<uint8_t> oneBlockCompressedTextureData =
            GetOneBlockBCFormatTextureData(copyConfig.textureDescriptor.format);
        for (uint32_t layer = 0; layer < copyConfig.copyExtent3D.depth; ++layer) {
            for (uint32_t h = 0; h < copyHeightInBlock; ++h) {
                for (uint32_t w = 0; w < copyWidthInBlock; ++w) {
                    uint32_t uploadBufferOffset = copyConfig.bufferOffset +
                                                  copyBytesPerImage * layer + rowPitchInBytes * h +
                                                  oneBlockCompressedTextureData.size() * w;
                    std::memcpy(&data[uploadBufferOffset], oneBlockCompressedTextureData.data(),
                                oneBlockCompressedTextureData.size() * sizeof(uint8_t));
                }
            }
        }

        return data;
    }

    // Copy the compressed texture data into the destination texture as is specified in copyConfig.
    void InitializeDataInCompressedTexture(wgpu::Texture bcCompressedTexture,
                                           const CopyConfig& copyConfig) {
        ASSERT(IsBCFormatSupported());

        std::vector<uint8_t> data = UploadData(copyConfig);

        // Copy texture data from a staging buffer to the destination texture.
        wgpu::Buffer stagingBuffer = utils::CreateBufferFromData(device, data.data(), data.size(),
                                                                 wgpu::BufferUsage::CopySrc);
        wgpu::BufferCopyView bufferCopyView =
            utils::CreateBufferCopyView(stagingBuffer, copyConfig.bufferOffset,
                                        copyConfig.bytesPerRowAlignment, copyConfig.rowsPerImage);

        wgpu::TextureCopyView textureCopyView = utils::CreateTextureCopyView(
            bcCompressedTexture, copyConfig.viewMipmapLevel, copyConfig.copyOrigin3D);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copyConfig.copyExtent3D);
        wgpu::CommandBuffer copy = encoder.Finish();
        queue.Submit(1, &copy);
    }

    // Create the bind group that includes a BC texture and a sampler.
    wgpu::BindGroup CreateBindGroupForTest(wgpu::BindGroupLayout bindGroupLayout,
                                           wgpu::Texture bcCompressedTexture,
                                           wgpu::TextureFormat bcFormat,
                                           uint32_t baseArrayLayer = 0,
                                           uint32_t baseMipLevel = 0) {
        ASSERT(IsBCFormatSupported());

        wgpu::SamplerDescriptor samplerDesc = utils::GetDefaultSamplerDescriptor();
        samplerDesc.minFilter = wgpu::FilterMode::Nearest;
        samplerDesc.magFilter = wgpu::FilterMode::Nearest;
        wgpu::Sampler sampler = device.CreateSampler(&samplerDesc);

        wgpu::TextureViewDescriptor textureViewDescriptor;
        textureViewDescriptor.format = bcFormat;
        textureViewDescriptor.dimension = wgpu::TextureViewDimension::e2D;
        textureViewDescriptor.baseMipLevel = baseMipLevel;
        textureViewDescriptor.baseArrayLayer = baseArrayLayer;
        textureViewDescriptor.arrayLayerCount = 1;
        textureViewDescriptor.mipLevelCount = 1;
        wgpu::TextureView bcTextureView = bcCompressedTexture.CreateView(&textureViewDescriptor);

        return utils::MakeBindGroup(device, bindGroupLayout, {{0, sampler}, {1, bcTextureView}});
    }

    // Create a render pipeline for sampling from a BC texture and rendering into the render target.
    wgpu::RenderPipeline CreateRenderPipelineForTest() {
        ASSERT(IsBCFormatSupported());

        utils::ComboRenderPipelineDescriptor renderPipelineDescriptor(device);
        wgpu::ShaderModule vsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
            #version 450
            layout(location=0) out vec2 texCoord;
            void main() {
                const vec2 pos[3] = vec2[3](
                    vec2(-3.0f,  1.0f),
                    vec2( 3.0f,  1.0f),
                    vec2( 0.0f, -2.0f)
                );
                gl_Position = vec4(pos[gl_VertexIndex], 0.0f, 1.0f);
                texCoord = vec2(gl_Position.x / 2.0f, -gl_Position.y / 2.0f) + vec2(0.5f);
            })");
        wgpu::ShaderModule fsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
            #version 450
            layout(set = 0, binding = 0) uniform sampler sampler0;
            layout(set = 0, binding = 1) uniform texture2D texture0;
            layout(location = 0) in vec2 texCoord;
            layout(location = 0) out vec4 fragColor;

            void main() {
                fragColor = texture(sampler2D(texture0, sampler0), texCoord);
            })");
        renderPipelineDescriptor.vertexStage.module = vsModule;
        renderPipelineDescriptor.cFragmentStage.module = fsModule;
        renderPipelineDescriptor.cColorStates[0].format =
            utils::BasicRenderPass::kDefaultColorFormat;

        return device.CreateRenderPipeline(&renderPipelineDescriptor);
    }

    // Run the given render pipeline and bind group and verify the pixels in the render target.
    void VerifyCompressedTexturePixelValues(wgpu::RenderPipeline renderPipeline,
                                            wgpu::BindGroup bindGroup,
                                            const wgpu::Extent3D& renderTargetSize,
                                            const wgpu::Origin3D& expectedOrigin,
                                            const wgpu::Extent3D& expectedExtent,
                                            const std::vector<RGBA8>& expected) {
        ASSERT(IsBCFormatSupported());

        utils::BasicRenderPass renderPass =
            utils::CreateBasicRenderPass(device, renderTargetSize.width, renderTargetSize.height);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
            pass.SetPipeline(renderPipeline);
            pass.SetBindGroup(0, bindGroup);
            pass.Draw(6);
            pass.EndPass();
        }

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        EXPECT_TEXTURE_RGBA8_EQ(expected.data(), renderPass.color, expectedOrigin.x,
                                expectedOrigin.y, expectedExtent.width, expectedExtent.height, 0,
                                0);
    }

    // Run the tests that copies pre-prepared BC format data into a BC texture and verifies if we
    // can render correctly with the pixel values sampled from the BC texture.
    void TestCopyRegionIntoBCFormatTextures(const CopyConfig& config) {
        ASSERT(IsBCFormatSupported());

        wgpu::Texture bcTexture = CreateTextureWithCompressedData(config);

        VerifyBCTexture(config, bcTexture);
    }

    void VerifyBCTexture(const CopyConfig& config, wgpu::Texture bcTexture) {
        wgpu::RenderPipeline renderPipeline = CreateRenderPipelineForTest();

        wgpu::Extent3D virtualSizeAtLevel = GetVirtualSizeAtLevel(config);

        // The copy region may exceed the subresource size because of the required paddings for BC
        // blocks, so we should limit the size of the expectedData to make it match the real size
        // of the render target.
        wgpu::Extent3D noPaddingExtent3D = config.copyExtent3D;
        if (config.copyOrigin3D.x + config.copyExtent3D.width > virtualSizeAtLevel.width) {
            noPaddingExtent3D.width = virtualSizeAtLevel.width - config.copyOrigin3D.x;
        }
        if (config.copyOrigin3D.y + config.copyExtent3D.height > virtualSizeAtLevel.height) {
            noPaddingExtent3D.height = virtualSizeAtLevel.height - config.copyOrigin3D.y;
        }
        noPaddingExtent3D.depth = 1u;

        std::vector<RGBA8> expectedData =
            GetExpectedData(config.textureDescriptor.format, noPaddingExtent3D);

        wgpu::Origin3D firstLayerCopyOrigin = {config.copyOrigin3D.x, config.copyOrigin3D.y, 0};
        for (uint32_t layer = config.copyOrigin3D.z;
             layer < config.copyOrigin3D.z + config.copyExtent3D.depth; ++layer) {
            wgpu::BindGroup bindGroup = CreateBindGroupForTest(
                renderPipeline.GetBindGroupLayout(0), bcTexture, config.textureDescriptor.format,
                layer, config.viewMipmapLevel);
            VerifyCompressedTexturePixelValues(renderPipeline, bindGroup, virtualSizeAtLevel,
                                               firstLayerCopyOrigin, noPaddingExtent3D,
                                               expectedData);
        }
    }

    // Create a texture and initialize it with the pre-prepared compressed texture data.
    wgpu::Texture CreateTextureWithCompressedData(CopyConfig config) {
        wgpu::Texture bcTexture = device.CreateTexture(&config.textureDescriptor);
        InitializeDataInCompressedTexture(bcTexture, config);
        return bcTexture;
    }

    // Record a texture-to-texture copy command into command encoder without finishing the encoding.
    void RecordTextureToTextureCopy(wgpu::CommandEncoder encoder,
                                    wgpu::Texture srcTexture,
                                    wgpu::Texture dstTexture,
                                    CopyConfig srcConfig,
                                    CopyConfig dstConfig) {
        wgpu::TextureCopyView textureCopyViewSrc = utils::CreateTextureCopyView(
            srcTexture, srcConfig.viewMipmapLevel, srcConfig.copyOrigin3D);
        wgpu::TextureCopyView textureCopyViewDst = utils::CreateTextureCopyView(
            dstTexture, dstConfig.viewMipmapLevel, dstConfig.copyOrigin3D);
        encoder.CopyTextureToTexture(&textureCopyViewSrc, &textureCopyViewDst,
                                     &dstConfig.copyExtent3D);
    }

    wgpu::Texture CreateTextureFromTexture(wgpu::Texture srcTexture,
                                           CopyConfig srcConfig,
                                           CopyConfig dstConfig) {
        wgpu::Texture dstTexture = device.CreateTexture(&dstConfig.textureDescriptor);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        RecordTextureToTextureCopy(encoder, srcTexture, dstTexture, srcConfig, dstConfig);
        wgpu::CommandBuffer copy = encoder.Finish();
        queue.Submit(1, &copy);

        return dstTexture;
    }

    // Return the pre-prepared one-block BC texture data.
    static std::vector<uint8_t> GetOneBlockBCFormatTextureData(wgpu::TextureFormat bcFormat) {
        switch (bcFormat) {
            // The expected data represents 4x4 pixel images with the left side dark red and the
            // right side dark green. We specify the same compressed data in both sRGB and non-sRGB
            // tests, but the rendering result should be different because for sRGB formats, the
            // red, green, and blue components are converted from an sRGB color space to a linear
            // color space as part of filtering.
            case wgpu::TextureFormat::BC1RGBAUnorm:
            case wgpu::TextureFormat::BC1RGBAUnormSrgb:
                return {0x0, 0xC0, 0x60, 0x6, 0x50, 0x50, 0x50, 0x50};
            case wgpu::TextureFormat::BC7RGBAUnorm:
            case wgpu::TextureFormat::BC7RGBAUnormSrgb:
                return {0x50, 0x18, 0xfc, 0xf, 0x0,  0x30, 0xe3, 0xe1,
                        0xe1, 0xe1, 0xc1, 0xf, 0xfc, 0xc0, 0xf,  0xfc};

            // The expected data represents 4x4 pixel images with the left side dark red and the
            // right side dark green. The pixels in the left side of the block all have an alpha
            // value equal to 0x88. We specify the same compressed data in both sRGB and non-sRGB
            // tests, but the rendering result should be different because for sRGB formats, the
            // red, green, and blue components are converted from an sRGB color space to a linear
            // color space as part of filtering, and any alpha component is left unchanged.
            case wgpu::TextureFormat::BC2RGBAUnorm:
            case wgpu::TextureFormat::BC2RGBAUnormSrgb:
                return {0x88, 0xFF, 0x88, 0xFF, 0x88, 0xFF, 0x88, 0xFF,
                        0x0,  0xC0, 0x60, 0x6,  0x50, 0x50, 0x50, 0x50};
            case wgpu::TextureFormat::BC3RGBAUnorm:
            case wgpu::TextureFormat::BC3RGBAUnormSrgb:
                return {0x88, 0xFF, 0x40, 0x2, 0x24, 0x40, 0x2,  0x24,
                        0x0,  0xC0, 0x60, 0x6, 0x50, 0x50, 0x50, 0x50};

            // The expected data represents 4x4 pixel images with the left side red and the
            // right side black.
            case wgpu::TextureFormat::BC4RSnorm:
                return {0x7F, 0x0, 0x40, 0x2, 0x24, 0x40, 0x2, 0x24};
            case wgpu::TextureFormat::BC4RUnorm:
                return {0xFF, 0x0, 0x40, 0x2, 0x24, 0x40, 0x2, 0x24};

            // The expected data represents 4x4 pixel images with the left side red and the right
            // side green and was encoded with DirectXTex from Microsoft.
            case wgpu::TextureFormat::BC5RGSnorm:
                return {0x7f, 0x81, 0x40, 0x2,  0x24, 0x40, 0x2,  0x24,
                        0x7f, 0x81, 0x9,  0x90, 0x0,  0x9,  0x90, 0x0};
            case wgpu::TextureFormat::BC5RGUnorm:
                return {0xff, 0x0, 0x40, 0x2,  0x24, 0x40, 0x2,  0x24,
                        0xff, 0x0, 0x9,  0x90, 0x0,  0x9,  0x90, 0x0};
            case wgpu::TextureFormat::BC6HRGBSfloat:
                return {0xe3, 0x1f, 0x0, 0x0,  0x0, 0xe0, 0x1f, 0x0,
                        0x0,  0xff, 0x0, 0xff, 0x0, 0xff, 0x0,  0xff};
            case wgpu::TextureFormat::BC6HRGBUfloat:
                return {0xe3, 0x3d, 0x0, 0x0,  0x0, 0xe0, 0x3d, 0x0,
                        0x0,  0xff, 0x0, 0xff, 0x0, 0xff, 0x0,  0xff};

            default:
                UNREACHABLE();
                return {};
        }
    }

    // Return the texture data that is decoded from the result of GetOneBlockBCFormatTextureData in
    // RGBA8 formats.
    static std::vector<RGBA8> GetExpectedData(wgpu::TextureFormat bcFormat,
                                              const wgpu::Extent3D& testRegion) {
        constexpr RGBA8 kDarkRed(198, 0, 0, 255);
        constexpr RGBA8 kDarkGreen(0, 207, 0, 255);
        constexpr RGBA8 kDarkRedSRGB(144, 0, 0, 255);
        constexpr RGBA8 kDarkGreenSRGB(0, 159, 0, 255);

        constexpr uint8_t kLeftAlpha = 0x88;
        constexpr uint8_t kRightAlpha = 0xFF;

        switch (bcFormat) {
            case wgpu::TextureFormat::BC1RGBAUnorm:
            case wgpu::TextureFormat::BC7RGBAUnorm:
                return FillExpectedData(testRegion, kDarkRed, kDarkGreen);

            case wgpu::TextureFormat::BC2RGBAUnorm:
            case wgpu::TextureFormat::BC3RGBAUnorm: {
                constexpr RGBA8 kLeftColor = RGBA8(kDarkRed.r, 0, 0, kLeftAlpha);
                constexpr RGBA8 kRightColor = RGBA8(0, kDarkGreen.g, 0, kRightAlpha);
                return FillExpectedData(testRegion, kLeftColor, kRightColor);
            }

            case wgpu::TextureFormat::BC1RGBAUnormSrgb:
            case wgpu::TextureFormat::BC7RGBAUnormSrgb:
                return FillExpectedData(testRegion, kDarkRedSRGB, kDarkGreenSRGB);

            case wgpu::TextureFormat::BC2RGBAUnormSrgb:
            case wgpu::TextureFormat::BC3RGBAUnormSrgb: {
                constexpr RGBA8 kLeftColor = RGBA8(kDarkRedSRGB.r, 0, 0, kLeftAlpha);
                constexpr RGBA8 kRightColor = RGBA8(0, kDarkGreenSRGB.g, 0, kRightAlpha);
                return FillExpectedData(testRegion, kLeftColor, kRightColor);
            }

            case wgpu::TextureFormat::BC4RSnorm:
            case wgpu::TextureFormat::BC4RUnorm:
                return FillExpectedData(testRegion, RGBA8::kRed, RGBA8::kBlack);

            case wgpu::TextureFormat::BC5RGSnorm:
            case wgpu::TextureFormat::BC5RGUnorm:
            case wgpu::TextureFormat::BC6HRGBSfloat:
            case wgpu::TextureFormat::BC6HRGBUfloat:
                return FillExpectedData(testRegion, RGBA8::kRed, RGBA8::kGreen);

            default:
                UNREACHABLE();
                return {};
        }
    }

    static std::vector<RGBA8> FillExpectedData(const wgpu::Extent3D& testRegion,
                                               RGBA8 leftColorInBlock,
                                               RGBA8 rightColorInBlock) {
        ASSERT(testRegion.depth == 1);

        std::vector<RGBA8> expectedData(testRegion.width * testRegion.height, leftColorInBlock);
        for (uint32_t y = 0; y < testRegion.height; ++y) {
            for (uint32_t x = 0; x < testRegion.width; ++x) {
                if (x % kBCBlockWidthInTexels >= kBCBlockWidthInTexels / 2) {
                    expectedData[testRegion.width * y + x] = rightColorInBlock;
                }
            }
        }
        return expectedData;
    }

    static wgpu::Extent3D GetVirtualSizeAtLevel(const CopyConfig& config) {
        return {config.textureDescriptor.size.width >> config.viewMipmapLevel,
                config.textureDescriptor.size.height >> config.viewMipmapLevel, 1};
    }

    static wgpu::Extent3D GetPhysicalSizeAtLevel(const CopyConfig& config) {
        wgpu::Extent3D sizeAtLevel = GetVirtualSizeAtLevel(config);
        sizeAtLevel.width = (sizeAtLevel.width + kBCBlockWidthInTexels - 1) /
                            kBCBlockWidthInTexels * kBCBlockWidthInTexels;
        sizeAtLevel.height = (sizeAtLevel.height + kBCBlockHeightInTexels - 1) /
                             kBCBlockHeightInTexels * kBCBlockHeightInTexels;
        return sizeAtLevel;
    }

    const std::array<wgpu::TextureFormat, 14> kBCFormats = {
        wgpu::TextureFormat::BC1RGBAUnorm,  wgpu::TextureFormat::BC1RGBAUnormSrgb,
        wgpu::TextureFormat::BC2RGBAUnorm,  wgpu::TextureFormat::BC2RGBAUnormSrgb,
        wgpu::TextureFormat::BC3RGBAUnorm,  wgpu::TextureFormat::BC3RGBAUnormSrgb,
        wgpu::TextureFormat::BC4RSnorm,     wgpu::TextureFormat::BC4RUnorm,
        wgpu::TextureFormat::BC5RGSnorm,    wgpu::TextureFormat::BC5RGUnorm,
        wgpu::TextureFormat::BC6HRGBSfloat, wgpu::TextureFormat::BC6HRGBUfloat,
        wgpu::TextureFormat::BC7RGBAUnorm,  wgpu::TextureFormat::BC7RGBAUnormSrgb};

    // Tthe block width and height in texels are 4 for all BC formats.
    static constexpr uint32_t kBCBlockWidthInTexels = 4;
    static constexpr uint32_t kBCBlockHeightInTexels = 4;

    static constexpr wgpu::TextureUsage kDefaultBCFormatTextureUsage =
        wgpu::TextureUsage::Sampled | wgpu::TextureUsage::CopyDst;

    bool mIsBCFormatSupported = false;
};

// Test copying into the whole BC texture with 2x2 blocks and sampling from it.
TEST_P(CompressedTextureBCFormatTest, Basic) {
    // TODO(jiawei.shao@intel.com): find out why this test is flaky on Windows Intel Vulkan
    // bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsWindows());

    // TODO(jiawei.shao@intel.com): find out why this test fails on Windows Intel OpenGL drivers.
    DAWN_SKIP_TEST_IF(IsIntel() && IsOpenGL() && IsWindows());

    DAWN_SKIP_TEST_IF(!IsBCFormatSupported());

    CopyConfig config;
    config.textureDescriptor.usage = kDefaultBCFormatTextureUsage;
    config.textureDescriptor.size = {8, 8, 1};
    config.copyExtent3D = config.textureDescriptor.size;

    for (wgpu::TextureFormat format : kBCFormats) {
        config.textureDescriptor.format = format;
        TestCopyRegionIntoBCFormatTextures(config);
    }
}

// Test copying into a sub-region of a texture with BC formats works correctly.
TEST_P(CompressedTextureBCFormatTest, CopyIntoSubRegion) {
    // TODO(jiawei.shao@intel.com): find out why this test is flaky on Windows Intel Vulkan
    // bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsWindows());

    DAWN_SKIP_TEST_IF(!IsBCFormatSupported());

    CopyConfig config;
    config.textureDescriptor.usage = kDefaultBCFormatTextureUsage;
    config.textureDescriptor.size = {8, 8, 1};

    const wgpu::Origin3D kOrigin = {4, 4, 0};
    const wgpu::Extent3D kExtent3D = {4, 4, 1};
    config.copyOrigin3D = kOrigin;
    config.copyExtent3D = kExtent3D;

    for (wgpu::TextureFormat format : kBCFormats) {
        config.textureDescriptor.format = format;
        TestCopyRegionIntoBCFormatTextures(config);
    }
}

// Test copying into the non-zero layer of a 2D array texture with BC formats works correctly.
TEST_P(CompressedTextureBCFormatTest, CopyIntoNonZeroArrayLayer) {
    // TODO(jiawei.shao@intel.com): find out why this test is flaky on Windows Intel Vulkan
    // bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsWindows());

    // TODO(jiawei.shao@intel.com): find out why this test fails on Windows Intel OpenGL drivers.
    DAWN_SKIP_TEST_IF(IsIntel() && IsOpenGL() && IsWindows());

    DAWN_SKIP_TEST_IF(!IsBCFormatSupported());

    CopyConfig config;
    config.textureDescriptor.usage = kDefaultBCFormatTextureUsage;
    config.textureDescriptor.size = {8, 8, 1};
    config.copyExtent3D = config.textureDescriptor.size;

    constexpr uint32_t kArrayLayerCount = 3;
    config.textureDescriptor.size.depth = kArrayLayerCount;
    config.copyOrigin3D.z = kArrayLayerCount - 1;

    for (wgpu::TextureFormat format : kBCFormats) {
        config.textureDescriptor.format = format;
        TestCopyRegionIntoBCFormatTextures(config);
    }
}

// Test copying into a non-zero mipmap level of a texture with BC texture formats.
TEST_P(CompressedTextureBCFormatTest, CopyBufferIntoNonZeroMipmapLevel) {
    // TODO(jiawei.shao@intel.com): find out why this test is flaky on Windows Intel Vulkan
    // bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsWindows());

    // TODO(jiawei.shao@intel.com): find out why this test fails on Windows Intel OpenGL drivers.
    DAWN_SKIP_TEST_IF(IsIntel() && IsOpenGL() && IsWindows());

    DAWN_SKIP_TEST_IF(!IsBCFormatSupported());

    CopyConfig config;
    config.textureDescriptor.usage = kDefaultBCFormatTextureUsage;
    config.textureDescriptor.size = {60, 60, 1};

    constexpr uint32_t kMipmapLevelCount = 3;
    config.textureDescriptor.mipLevelCount = kMipmapLevelCount;
    config.viewMipmapLevel = kMipmapLevelCount - 1;

    // The actual size of the texture at mipmap level == 2 is not a multiple of 4, paddings are
    // required in the copies.
    const wgpu::Extent3D textureSizeLevel0 = config.textureDescriptor.size;
    const uint32_t kActualWidthAtLevel = textureSizeLevel0.width >> config.viewMipmapLevel;
    const uint32_t kActualHeightAtLevel = textureSizeLevel0.height >> config.viewMipmapLevel;
    ASSERT(kActualWidthAtLevel % kBCBlockWidthInTexels != 0);
    ASSERT(kActualHeightAtLevel % kBCBlockHeightInTexels != 0);

    const uint32_t kCopyWidthAtLevel = (kActualWidthAtLevel + kBCBlockWidthInTexels - 1) /
                                       kBCBlockWidthInTexels * kBCBlockWidthInTexels;
    const uint32_t kCopyHeightAtLevel = (kActualHeightAtLevel + kBCBlockHeightInTexels - 1) /
                                        kBCBlockHeightInTexels * kBCBlockHeightInTexels;

    config.copyExtent3D = {kCopyWidthAtLevel, kCopyHeightAtLevel, 1};

    for (wgpu::TextureFormat format : kBCFormats) {
        config.textureDescriptor.format = format;
        TestCopyRegionIntoBCFormatTextures(config);
    }
}

// Test texture-to-texture whole-size copies with BC formats.
TEST_P(CompressedTextureBCFormatTest, CopyWholeTextureSubResourceIntoNonZeroMipmapLevel) {
    // TODO(jiawei.shao@intel.com): find out why this test is flaky on Windows Intel Vulkan
    // bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsWindows());

    // TODO(jiawei.shao@intel.com): find out why this test fails on Windows Intel OpenGL drivers.
    DAWN_SKIP_TEST_IF(IsIntel() && IsOpenGL() && IsWindows());

    DAWN_SKIP_TEST_IF(!IsBCFormatSupported());

    // TODO(cwallez@chromium.org): This consistently fails on with the 12th pixel being opaque black
    // instead of opaque red on Win10 FYI Release (NVIDIA GeForce GTX 1660). See
    // https://bugs.chromium.org/p/chromium/issues/detail?id=981393
    DAWN_SKIP_TEST_IF(IsWindows() && IsVulkan() && IsNvidia());

    CopyConfig config;
    config.textureDescriptor.size = {60, 60, 1};

    constexpr uint32_t kMipmapLevelCount = 3;
    config.textureDescriptor.mipLevelCount = kMipmapLevelCount;
    config.viewMipmapLevel = kMipmapLevelCount - 1;

    // The actual size of the texture at mipmap level == 2 is not a multiple of 4, paddings are
    // required in the copies.
    const wgpu::Extent3D kVirtualSize = GetVirtualSizeAtLevel(config);
    const wgpu::Extent3D kPhysicalSize = GetPhysicalSizeAtLevel(config);
    ASSERT_NE(0u, kVirtualSize.width % kBCBlockWidthInTexels);
    ASSERT_NE(0u, kVirtualSize.height % kBCBlockHeightInTexels);

    config.copyExtent3D = kPhysicalSize;
    for (wgpu::TextureFormat format : kBCFormats) {
        // Create bcTextureSrc as the source texture and initialize it with pre-prepared BC
        // compressed data.
        config.textureDescriptor.format = format;
        // Add the usage bit for both source and destination textures so that we don't need to
        // create two copy configs.
        config.textureDescriptor.usage =
            wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::Sampled;

        wgpu::Texture bcTextureSrc = CreateTextureWithCompressedData(config);

        // Create bcTexture and copy from the content in bcTextureSrc into it.
        wgpu::Texture bcTextureDst = CreateTextureFromTexture(bcTextureSrc, config, config);

        // Verify if we can use bcTexture as sampled textures correctly.
        wgpu::RenderPipeline renderPipeline = CreateRenderPipelineForTest();
        wgpu::BindGroup bindGroup =
            CreateBindGroupForTest(renderPipeline.GetBindGroupLayout(0), bcTextureDst, format,
                                   config.copyOrigin3D.z, config.viewMipmapLevel);

        std::vector<RGBA8> expectedData = GetExpectedData(format, kVirtualSize);
        VerifyCompressedTexturePixelValues(renderPipeline, bindGroup, kVirtualSize,
                                           config.copyOrigin3D, kVirtualSize, expectedData);
    }
}

// Test BC format texture-to-texture partial copies where the physical size of the destination
// subresource is different from its virtual size.
TEST_P(CompressedTextureBCFormatTest, CopyIntoSubresourceWithPhysicalSizeNotEqualToVirtualSize) {
    DAWN_SKIP_TEST_IF(!IsBCFormatSupported());

    // TODO(jiawei.shao@intel.com): add workaround on the T2T copies where Extent3D fits in one
    // subresource and does not fit in another one on OpenGL.
    DAWN_SKIP_TEST_IF(IsOpenGL());

    // TODO(jiawei.shao@intel.com): find out why this test is flaky on Windows Intel Vulkan
    // bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsWindows());

    CopyConfig srcConfig;
    srcConfig.textureDescriptor.size = {60, 60, 1};
    srcConfig.viewMipmapLevel = srcConfig.textureDescriptor.mipLevelCount - 1;

    const wgpu::Extent3D kSrcVirtualSize = GetVirtualSizeAtLevel(srcConfig);

    CopyConfig dstConfig;
    dstConfig.textureDescriptor.size = {60, 60, 1};
    constexpr uint32_t kMipmapLevelCount = 3;
    dstConfig.textureDescriptor.mipLevelCount = kMipmapLevelCount;
    dstConfig.viewMipmapLevel = kMipmapLevelCount - 1;

    // The actual size of the texture at mipmap level == 2 is not a multiple of 4, paddings are
    // required in the copies.
    const wgpu::Extent3D kDstVirtualSize = GetVirtualSizeAtLevel(dstConfig);
    ASSERT_NE(0u, kDstVirtualSize.width % kBCBlockWidthInTexels);
    ASSERT_NE(0u, kDstVirtualSize.height % kBCBlockHeightInTexels);

    const wgpu::Extent3D kDstPhysicalSize = GetPhysicalSizeAtLevel(dstConfig);

    srcConfig.copyExtent3D = dstConfig.copyExtent3D = kDstPhysicalSize;
    ASSERT_LT(srcConfig.copyOrigin3D.x + srcConfig.copyExtent3D.width, kSrcVirtualSize.width);
    ASSERT_LT(srcConfig.copyOrigin3D.y + srcConfig.copyExtent3D.height, kSrcVirtualSize.height);

    for (wgpu::TextureFormat format : kBCFormats) {
        // Create bcTextureSrc as the source texture and initialize it with pre-prepared BC
        // compressed data.
        srcConfig.textureDescriptor.format = format;
        srcConfig.textureDescriptor.usage =
            wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst;
        wgpu::Texture bcTextureSrc = CreateTextureWithCompressedData(srcConfig);
        wgpu::TextureCopyView textureCopyViewSrc = utils::CreateTextureCopyView(
            bcTextureSrc, srcConfig.viewMipmapLevel, srcConfig.copyOrigin3D);

        // Create bcTexture and copy from the content in bcTextureSrc into it.
        dstConfig.textureDescriptor.format = format;
        dstConfig.textureDescriptor.usage = kDefaultBCFormatTextureUsage;
        wgpu::Texture bcTextureDst = CreateTextureFromTexture(bcTextureSrc, srcConfig, dstConfig);

        // Verify if we can use bcTexture as sampled textures correctly.
        wgpu::RenderPipeline renderPipeline = CreateRenderPipelineForTest();
        wgpu::BindGroup bindGroup =
            CreateBindGroupForTest(renderPipeline.GetBindGroupLayout(0), bcTextureDst, format,
                                   dstConfig.copyOrigin3D.z, dstConfig.viewMipmapLevel);

        std::vector<RGBA8> expectedData = GetExpectedData(format, kDstVirtualSize);
        VerifyCompressedTexturePixelValues(renderPipeline, bindGroup, kDstVirtualSize,
                                           dstConfig.copyOrigin3D, kDstVirtualSize, expectedData);
    }
}

// Test BC format texture-to-texture partial copies where the physical size of the source
// subresource is different from its virtual size.
TEST_P(CompressedTextureBCFormatTest, CopyFromSubresourceWithPhysicalSizeNotEqualToVirtualSize) {
    DAWN_SKIP_TEST_IF(!IsBCFormatSupported());

    // TODO(jiawei.shao@intel.com): add workaround on the T2T copies where Extent3D fits in one
    // subresource and does not fit in another one on OpenGL.
    DAWN_SKIP_TEST_IF(IsOpenGL());

    // TODO(jiawei.shao@intel.com): find out why this test is flaky on Windows Intel Vulkan
    // bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsWindows());

    CopyConfig srcConfig;
    srcConfig.textureDescriptor.size = {60, 60, 1};
    constexpr uint32_t kMipmapLevelCount = 3;
    srcConfig.textureDescriptor.mipLevelCount = kMipmapLevelCount;
    srcConfig.viewMipmapLevel = srcConfig.textureDescriptor.mipLevelCount - 1;

    // The actual size of the texture at mipmap level == 2 is not a multiple of 4, paddings are
    // required in the copies.
    const wgpu::Extent3D kSrcVirtualSize = GetVirtualSizeAtLevel(srcConfig);
    ASSERT_NE(0u, kSrcVirtualSize.width % kBCBlockWidthInTexels);
    ASSERT_NE(0u, kSrcVirtualSize.height % kBCBlockHeightInTexels);

    CopyConfig dstConfig;
    dstConfig.textureDescriptor.size = {16, 16, 1};
    dstConfig.viewMipmapLevel = dstConfig.textureDescriptor.mipLevelCount - 1;

    const wgpu::Extent3D kDstVirtualSize = GetVirtualSizeAtLevel(dstConfig);
    srcConfig.copyExtent3D = dstConfig.copyExtent3D = kDstVirtualSize;

    ASSERT_GT(srcConfig.copyOrigin3D.x + srcConfig.copyExtent3D.width, kSrcVirtualSize.width);
    ASSERT_GT(srcConfig.copyOrigin3D.y + srcConfig.copyExtent3D.height, kSrcVirtualSize.height);

    for (wgpu::TextureFormat format : kBCFormats) {
        srcConfig.textureDescriptor.format = dstConfig.textureDescriptor.format = format;
        srcConfig.textureDescriptor.usage =
            wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst;
        dstConfig.textureDescriptor.usage = kDefaultBCFormatTextureUsage;

        // Create bcTextureSrc as the source texture and initialize it with pre-prepared BC
        // compressed data.
        wgpu::Texture bcTextureSrc = CreateTextureWithCompressedData(srcConfig);

        // Create bcTexture and copy from the content in bcTextureSrc into it.
        wgpu::Texture bcTextureDst = CreateTextureFromTexture(bcTextureSrc, srcConfig, dstConfig);

        // Verify if we can use bcTexture as sampled textures correctly.
        wgpu::RenderPipeline renderPipeline = CreateRenderPipelineForTest();
        wgpu::BindGroup bindGroup =
            CreateBindGroupForTest(renderPipeline.GetBindGroupLayout(0), bcTextureDst, format,
                                   dstConfig.copyOrigin3D.z, dstConfig.viewMipmapLevel);

        std::vector<RGBA8> expectedData = GetExpectedData(format, kDstVirtualSize);
        VerifyCompressedTexturePixelValues(renderPipeline, bindGroup, kDstVirtualSize,
                                           dstConfig.copyOrigin3D, kDstVirtualSize, expectedData);
    }
}

// Test recording two BC format texture-to-texture partial copies where the physical size of the
// source subresource is different from its virtual size into one command buffer.
TEST_P(CompressedTextureBCFormatTest, MultipleCopiesWithPhysicalSizeNotEqualToVirtualSize) {
    DAWN_SKIP_TEST_IF(!IsBCFormatSupported());

    // TODO(jiawei.shao@intel.com): add workaround on the T2T copies where Extent3D fits in one
    // subresource and does not fit in another one on OpenGL.
    DAWN_SKIP_TEST_IF(IsOpenGL());

    // TODO(jiawei.shao@intel.com): find out why this test is flaky on Windows Intel Vulkan
    // bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsWindows());

    constexpr uint32_t kTotalCopyCount = 2;
    std::array<CopyConfig, kTotalCopyCount> srcConfigs;
    std::array<CopyConfig, kTotalCopyCount> dstConfigs;

    constexpr uint32_t kSrcMipmapLevelCount0 = 3;
    srcConfigs[0].textureDescriptor.size = {60, 60, 1};
    srcConfigs[0].textureDescriptor.mipLevelCount = kSrcMipmapLevelCount0;
    srcConfigs[0].viewMipmapLevel = srcConfigs[0].textureDescriptor.mipLevelCount - 1;
    dstConfigs[0].textureDescriptor.size = {16, 16, 1};
    dstConfigs[0].viewMipmapLevel = dstConfigs[0].textureDescriptor.mipLevelCount - 1;
    srcConfigs[0].copyExtent3D = dstConfigs[0].copyExtent3D = GetVirtualSizeAtLevel(dstConfigs[0]);
    const wgpu::Extent3D kSrcVirtualSize0 = GetVirtualSizeAtLevel(srcConfigs[0]);
    ASSERT_NE(0u, kSrcVirtualSize0.width % kBCBlockWidthInTexels);
    ASSERT_NE(0u, kSrcVirtualSize0.height % kBCBlockHeightInTexels);

    constexpr uint32_t kDstMipmapLevelCount1 = 4;
    srcConfigs[1].textureDescriptor.size = {8, 8, 1};
    srcConfigs[1].viewMipmapLevel = srcConfigs[1].textureDescriptor.mipLevelCount - 1;
    dstConfigs[1].textureDescriptor.size = {56, 56, 1};
    dstConfigs[1].textureDescriptor.mipLevelCount = kDstMipmapLevelCount1;
    dstConfigs[1].viewMipmapLevel = dstConfigs[1].textureDescriptor.mipLevelCount - 1;
    srcConfigs[1].copyExtent3D = dstConfigs[1].copyExtent3D = GetVirtualSizeAtLevel(srcConfigs[1]);

    std::array<wgpu::Extent3D, kTotalCopyCount> dstVirtualSizes;
    for (uint32_t i = 0; i < kTotalCopyCount; ++i) {
        dstVirtualSizes[i] = GetVirtualSizeAtLevel(dstConfigs[i]);
    }
    ASSERT_NE(0u, dstVirtualSizes[1].width % kBCBlockWidthInTexels);
    ASSERT_NE(0u, dstVirtualSizes[1].height % kBCBlockHeightInTexels);

    for (wgpu::TextureFormat format : kBCFormats) {
        std::array<wgpu::Texture, kTotalCopyCount> bcSrcTextures;
        std::array<wgpu::Texture, kTotalCopyCount> bcDstTextures;

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        for (uint32_t i = 0; i < kTotalCopyCount; ++i) {
            srcConfigs[i].textureDescriptor.format = dstConfigs[i].textureDescriptor.format =
                format;
            srcConfigs[i].textureDescriptor.usage =
                wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst;
            dstConfigs[i].textureDescriptor.usage = kDefaultBCFormatTextureUsage;

            // Create bcSrcTextures as the source textures and initialize them with pre-prepared BC
            // compressed data.
            bcSrcTextures[i] = CreateTextureWithCompressedData(srcConfigs[i]);
            bcDstTextures[i] = device.CreateTexture(&dstConfigs[i].textureDescriptor);

            RecordTextureToTextureCopy(encoder, bcSrcTextures[i], bcDstTextures[i], srcConfigs[i],
                                       dstConfigs[i]);
        }

        wgpu::CommandBuffer commandBuffer = encoder.Finish();
        queue.Submit(1, &commandBuffer);

        wgpu::RenderPipeline renderPipeline = CreateRenderPipelineForTest();

        for (uint32_t i = 0; i < kTotalCopyCount; ++i) {
            // Verify if we can use bcDstTextures as sampled textures correctly.
            wgpu::BindGroup bindGroup0 = CreateBindGroupForTest(
                renderPipeline.GetBindGroupLayout(0), bcDstTextures[i], format,
                dstConfigs[i].copyOrigin3D.z, dstConfigs[i].viewMipmapLevel);

            std::vector<RGBA8> expectedData = GetExpectedData(format, dstVirtualSizes[i]);
            VerifyCompressedTexturePixelValues(renderPipeline, bindGroup0, dstVirtualSizes[i],
                                               dstConfigs[i].copyOrigin3D, dstVirtualSizes[i],
                                               expectedData);
        }
    }
}

// Test the special case of the B2T copies on the D3D12 backend that the buffer offset and texture
// extent exactly fit the RowPitch.
TEST_P(CompressedTextureBCFormatTest, BufferOffsetAndExtentFitRowPitch) {
    // TODO(jiawei.shao@intel.com): find out why this test is flaky on Windows Intel Vulkan
    // bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsWindows());

    // TODO(jiawei.shao@intel.com): find out why this test fails on Windows Intel OpenGL drivers.
    DAWN_SKIP_TEST_IF(IsIntel() && IsOpenGL() && IsWindows());

    DAWN_SKIP_TEST_IF(!IsBCFormatSupported());

    CopyConfig config;
    config.textureDescriptor.usage = kDefaultBCFormatTextureUsage;
    config.textureDescriptor.size = {8, 8, 1};
    config.copyExtent3D = config.textureDescriptor.size;

    const uint32_t blockCountPerRow = config.textureDescriptor.size.width / kBCBlockWidthInTexels;

    for (wgpu::TextureFormat format : kBCFormats) {
        config.textureDescriptor.format = format;

        const uint32_t blockSizeInBytes = utils::GetTexelBlockSizeInBytes(format);
        const uint32_t blockCountPerRowPitch = config.bytesPerRowAlignment / blockSizeInBytes;

        config.bufferOffset = (blockCountPerRowPitch - blockCountPerRow) * blockSizeInBytes;

        TestCopyRegionIntoBCFormatTextures(config);
    }
}

// Test the special case of the B2T copies on the D3D12 backend that the buffer offset exceeds the
// slice pitch (slicePitch = bytesPerRow * (rowsPerImage / blockHeightInTexels)). On D3D12
// backend the texelOffset.y will be greater than 0 after calcuting the texelOffset in the function
// ComputeTexelOffsets().
TEST_P(CompressedTextureBCFormatTest, BufferOffsetExceedsSlicePitch) {
    // TODO(jiawei.shao@intel.com): find out why this test is flaky on Windows Intel Vulkan
    // bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsWindows());

    // TODO(jiawei.shao@intel.com): find out why this test fails on Windows Intel OpenGL drivers.
    DAWN_SKIP_TEST_IF(IsIntel() && IsOpenGL() && IsWindows());

    DAWN_SKIP_TEST_IF(!IsBCFormatSupported());

    CopyConfig config;
    config.textureDescriptor.usage = kDefaultBCFormatTextureUsage;
    config.textureDescriptor.size = {8, 8, 1};
    config.copyExtent3D = config.textureDescriptor.size;

    const wgpu::Extent3D textureSizeLevel0 = config.textureDescriptor.size;
    const uint32_t blockCountPerRow = textureSizeLevel0.width / kBCBlockWidthInTexels;
    const uint32_t slicePitchInBytes =
        config.bytesPerRowAlignment * (textureSizeLevel0.height / kBCBlockHeightInTexels);

    for (wgpu::TextureFormat format : kBCFormats) {
        config.textureDescriptor.format = format;

        const uint32_t blockSizeInBytes = utils::GetTexelBlockSizeInBytes(format);
        const uint32_t blockCountPerRowPitch = config.bytesPerRowAlignment / blockSizeInBytes;

        config.bufferOffset = (blockCountPerRowPitch - blockCountPerRow) * blockSizeInBytes +
                              config.bytesPerRowAlignment + slicePitchInBytes;

        TestCopyRegionIntoBCFormatTextures(config);
    }
}

// Test the special case of the B2T copies on the D3D12 backend that the buffer offset and texture
// extent exceed the RowPitch. On D3D12 backend two copies are required for this case.
TEST_P(CompressedTextureBCFormatTest, CopyWithBufferOffsetAndExtentExceedRowPitch) {
    // TODO(jiawei.shao@intel.com): find out why this test is flaky on Windows Intel Vulkan
    // bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsWindows());

    // TODO(jiawei.shao@intel.com): find out why this test fails on Windows Intel OpenGL drivers.
    DAWN_SKIP_TEST_IF(IsIntel() && IsOpenGL() && IsWindows());

    DAWN_SKIP_TEST_IF(!IsBCFormatSupported());

    CopyConfig config;
    config.textureDescriptor.usage = kDefaultBCFormatTextureUsage;
    config.textureDescriptor.size = {8, 8, 1};
    config.copyExtent3D = config.textureDescriptor.size;

    const uint32_t blockCountPerRow = config.textureDescriptor.size.width / kBCBlockWidthInTexels;

    constexpr uint32_t kExceedRowBlockCount = 1;

    for (wgpu::TextureFormat format : kBCFormats) {
        config.textureDescriptor.format = format;

        const uint32_t blockSizeInBytes = utils::GetTexelBlockSizeInBytes(format);
        const uint32_t blockCountPerRowPitch = config.bytesPerRowAlignment / blockSizeInBytes;
        config.bufferOffset =
            (blockCountPerRowPitch - blockCountPerRow + kExceedRowBlockCount) * blockSizeInBytes;

        TestCopyRegionIntoBCFormatTextures(config);
    }
}

// Test the special case of the B2T copies on the D3D12 backend that the slicePitch is equal to the
// bytesPerRow. On D3D12 backend the texelOffset.z will be greater than 0 after calcuting the
// texelOffset in the function ComputeTexelOffsets().
TEST_P(CompressedTextureBCFormatTest, RowPitchEqualToSlicePitch) {
    // TODO(jiawei.shao@intel.com): find out why this test is flaky on Windows Intel Vulkan
    // bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsWindows());

    DAWN_SKIP_TEST_IF(!IsBCFormatSupported());

    CopyConfig config;
    config.textureDescriptor.usage = kDefaultBCFormatTextureUsage;
    config.textureDescriptor.size = {8, kBCBlockHeightInTexels, 1};
    config.copyExtent3D = config.textureDescriptor.size;

    const uint32_t blockCountPerRow = config.textureDescriptor.size.width / kBCBlockWidthInTexels;
    const uint32_t slicePitchInBytes = config.bytesPerRowAlignment;

    for (wgpu::TextureFormat format : kBCFormats) {
        config.textureDescriptor.format = format;

        const uint32_t blockSizeInBytes = utils::GetTexelBlockSizeInBytes(format);
        const uint32_t blockCountPerRowPitch = config.bytesPerRowAlignment / blockSizeInBytes;

        config.bufferOffset =
            (blockCountPerRowPitch - blockCountPerRow) * blockSizeInBytes + slicePitchInBytes;

        TestCopyRegionIntoBCFormatTextures(config);
    }
}

// Test the workaround in the B2T copies when (bufferSize - bufferOffset < bytesPerImage *
// copyExtent.depth) on Metal backends. As copyExtent.depth can only be 1 for BC formats, on Metal
// backend we will use two copies to implement such copy.
TEST_P(CompressedTextureBCFormatTest, LargeImageHeight) {
    // TODO(jiawei.shao@intel.com): find out why this test is flaky on Windows Intel Vulkan
    // bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsWindows());

    // TODO(jiawei.shao@intel.com): find out why this test fails on Windows Intel OpenGL drivers.
    DAWN_SKIP_TEST_IF(IsIntel() && IsOpenGL() && IsWindows());

    DAWN_SKIP_TEST_IF(!IsBCFormatSupported());

    CopyConfig config;
    config.textureDescriptor.usage = kDefaultBCFormatTextureUsage;
    config.textureDescriptor.size = {8, 8, 1};
    config.copyExtent3D = config.textureDescriptor.size;

    config.rowsPerImage = config.textureDescriptor.size.height * 2;

    for (wgpu::TextureFormat format : kBCFormats) {
        config.textureDescriptor.format = format;
        TestCopyRegionIntoBCFormatTextures(config);
    }
}

// Test the workaround in the B2T copies when (bufferSize - bufferOffset < bytesPerImage *
// copyExtent.depth) and copyExtent needs to be clamped.
TEST_P(CompressedTextureBCFormatTest, LargeImageHeightAndClampedCopyExtent) {
    // TODO(jiawei.shao@intel.com): find out why this test is flaky on Windows Intel Vulkan
    // bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsWindows());

    // TODO(jiawei.shao@intel.com): find out why this test fails on Windows Intel OpenGL drivers.
    DAWN_SKIP_TEST_IF(IsIntel() && IsOpenGL() && IsWindows());

    DAWN_SKIP_TEST_IF(!IsBCFormatSupported());

    CopyConfig config;
    config.textureDescriptor.usage = kDefaultBCFormatTextureUsage;
    config.textureDescriptor.size = {56, 56, 1};

    constexpr uint32_t kMipmapLevelCount = 3;
    config.textureDescriptor.mipLevelCount = kMipmapLevelCount;
    config.viewMipmapLevel = kMipmapLevelCount - 1;

    // The actual size of the texture at mipmap level == 2 is not a multiple of 4, paddings are
    // required in the copies.
    const wgpu::Extent3D textureSizeLevel0 = config.textureDescriptor.size;
    const uint32_t kActualWidthAtLevel = textureSizeLevel0.width >> config.viewMipmapLevel;
    const uint32_t kActualHeightAtLevel = textureSizeLevel0.height >> config.viewMipmapLevel;
    ASSERT(kActualWidthAtLevel % kBCBlockWidthInTexels != 0);
    ASSERT(kActualHeightAtLevel % kBCBlockHeightInTexels != 0);

    const uint32_t kCopyWidthAtLevel = (kActualWidthAtLevel + kBCBlockWidthInTexels - 1) /
                                       kBCBlockWidthInTexels * kBCBlockWidthInTexels;
    const uint32_t kCopyHeightAtLevel = (kActualHeightAtLevel + kBCBlockHeightInTexels - 1) /
                                        kBCBlockHeightInTexels * kBCBlockHeightInTexels;

    config.copyExtent3D = {kCopyWidthAtLevel, kCopyHeightAtLevel, 1};

    config.rowsPerImage = kCopyHeightAtLevel * 2;

    for (wgpu::TextureFormat format : kBCFormats) {
        config.textureDescriptor.format = format;
        TestCopyRegionIntoBCFormatTextures(config);
    }
}

// Test copying a whole 2D array texture with array layer count > 1 in one copy command works with
// BC formats.
TEST_P(CompressedTextureBCFormatTest, CopyWhole2DArrayTexture) {
    // TODO(jiawei.shao@intel.com): find out why this test is flaky on Windows Intel Vulkan
    // bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsWindows());

    // TODO(jiawei.shao@intel.com): find out why this test fails on Windows Intel OpenGL drivers.
    DAWN_SKIP_TEST_IF(IsIntel() && IsOpenGL() && IsWindows());

    DAWN_SKIP_TEST_IF(!IsBCFormatSupported());

    constexpr uint32_t kArrayLayerCount = 3;

    CopyConfig config;
    config.textureDescriptor.usage = kDefaultBCFormatTextureUsage;
    config.textureDescriptor.size = {8, 8, kArrayLayerCount};

    config.copyExtent3D = config.textureDescriptor.size;
    config.copyExtent3D.depth = kArrayLayerCount;

    for (wgpu::TextureFormat format : kBCFormats) {
        config.textureDescriptor.format = format;
        TestCopyRegionIntoBCFormatTextures(config);
    }
}

// Test copying a multiple 2D texture array layers in one copy command works with BC formats.
TEST_P(CompressedTextureBCFormatTest, CopyMultiple2DArrayLayers) {
    // TODO(jiawei.shao@intel.com): find out why this test is flaky on Windows Intel Vulkan
    // bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsWindows());

    // TODO(jiawei.shao@intel.com): find out why this test fails on Windows Intel OpenGL drivers.
    DAWN_SKIP_TEST_IF(IsIntel() && IsOpenGL() && IsWindows());

    DAWN_SKIP_TEST_IF(!IsBCFormatSupported());

    constexpr uint32_t kArrayLayerCount = 3;

    CopyConfig config;
    config.textureDescriptor.usage = kDefaultBCFormatTextureUsage;
    config.textureDescriptor.size = {8, 8, kArrayLayerCount};

    constexpr uint32_t kCopyBaseArrayLayer = 1;
    constexpr uint32_t kCopyLayerCount = 2;
    config.copyOrigin3D = {0, 0, kCopyBaseArrayLayer};
    config.copyExtent3D = config.textureDescriptor.size;
    config.copyExtent3D.depth = kCopyLayerCount;

    for (wgpu::TextureFormat format : kBCFormats) {
        config.textureDescriptor.format = format;
        TestCopyRegionIntoBCFormatTextures(config);
    }
}

// TODO(jiawei.shao@intel.com): support BC formats on OpenGL backend
DAWN_INSTANTIATE_TEST(CompressedTextureBCFormatTest,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend(),
                      VulkanBackend({"use_temporary_buffer_in_texture_to_texture_copy"}));

class CompressedTextureWriteTextureTest : public CompressedTextureBCFormatTest {
  protected:
    void SetUp() override {
        CompressedTextureBCFormatTest::SetUp();
        DAWN_SKIP_TEST_IF(!IsBCFormatSupported());
    }

    // Write the compressed texture data into the destination texture as is specified in copyConfig.
    void WriteToCompressedTexture(wgpu::Texture bcCompressedTexture, const CopyConfig& copyConfig) {
        ASSERT(IsBCFormatSupported());

        std::vector<uint8_t> data = UploadData(copyConfig);

        wgpu::TextureDataLayout textureDataLayout = utils::CreateTextureDataLayout(
            copyConfig.bufferOffset, copyConfig.bytesPerRowAlignment, copyConfig.rowsPerImage);

        wgpu::TextureCopyView textureCopyView = utils::CreateTextureCopyView(
            bcCompressedTexture, copyConfig.viewMipmapLevel, copyConfig.copyOrigin3D);

        queue.WriteTexture(&textureCopyView, data.data(), data.size(), &textureDataLayout,
                           &copyConfig.copyExtent3D);
    }

    // Run the tests that write pre-prepared BC format data into a BC texture and verifies if we
    // can render correctly with the pixel values sampled from the BC texture.
    void TestWriteRegionIntoBCFormatTextures(const CopyConfig& config) {
        ASSERT(IsBCFormatSupported());

        wgpu::Texture bcTexture = device.CreateTexture(&config.textureDescriptor);
        WriteToCompressedTexture(bcTexture, config);

        VerifyBCTexture(config, bcTexture);
    }
};

// Test WriteTexture to a 2D texture with all parameters non-default
// with BC formats.
TEST_P(CompressedTextureWriteTextureTest, Basic) {
    CopyConfig config;
    config.textureDescriptor.usage = kDefaultBCFormatTextureUsage;
    config.textureDescriptor.size = {20, 24, 1};

    config.copyOrigin3D = {4, 8, 0};
    config.copyExtent3D = {12, 16, 1};
    config.bytesPerRowAlignment = 511;
    config.rowsPerImage = 20;

    for (wgpu::TextureFormat format : kBCFormats) {
        config.textureDescriptor.format = format;
        TestWriteRegionIntoBCFormatTextures(config);
    }
}

// Test writing to multiple 2D texture array layers with BC formats.
TEST_P(CompressedTextureWriteTextureTest, WriteMultiple2DArrayLayers) {
    // TODO(dawn:483): find out why this test is flaky on Windows Intel Vulkan
    // bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsWindows());

    CopyConfig config;
    config.textureDescriptor.usage = kDefaultBCFormatTextureUsage;
    config.textureDescriptor.size = {20, 24, 9};

    config.copyOrigin3D = {4, 8, 3};
    config.copyExtent3D = {12, 16, 6};
    config.bytesPerRowAlignment = 511;
    config.rowsPerImage = 20;

    for (wgpu::TextureFormat format : kBCFormats) {
        config.textureDescriptor.format = format;
        TestWriteRegionIntoBCFormatTextures(config);
    }
}

// Test BC format write textures where the physical size of the destination
// subresource is different from its virtual size.
TEST_P(CompressedTextureWriteTextureTest,
       WriteIntoSubresourceWithPhysicalSizeNotEqualToVirtualSize) {
    // TODO(dawn:483): find out why this test is flaky on Windows Intel Vulkan
    // bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsWindows());

    // Texture virtual size at mipLevel 2 will be {15, 15, 1} while the physical
    // size will be {16, 16, 1}.
    // Setting copyExtent.width or copyExtent.height to 16 fits in
    // the texture physical size, but doesn't fit in the virtual size.
    for (unsigned int w : {12, 16}) {
        for (unsigned int h : {12, 16}) {
            for (wgpu::TextureFormat format : kBCFormats) {
                CopyConfig config;
                config.textureDescriptor.usage = kDefaultBCFormatTextureUsage;
                config.textureDescriptor.size = {60, 60, 1};
                config.textureDescriptor.mipLevelCount = 4;
                config.viewMipmapLevel = 2;

                config.copyOrigin3D = {0, 0, 0};
                config.copyExtent3D = {w, h, 1};
                config.bytesPerRowAlignment = 256;
                config.textureDescriptor.format = format;
                TestWriteRegionIntoBCFormatTextures(config);
            }
        }
    }
}

DAWN_INSTANTIATE_TEST(CompressedTextureWriteTextureTest, MetalBackend(), VulkanBackend());