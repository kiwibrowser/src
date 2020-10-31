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

#include "common/Math.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

#define EXPECT_LAZY_CLEAR(N, statement)                                                       \
    do {                                                                                      \
        if (UsesWire()) {                                                                     \
            statement;                                                                        \
        } else {                                                                              \
            size_t lazyClearsBefore = dawn_native::GetLazyClearCountForTesting(device.Get()); \
            statement;                                                                        \
            size_t lazyClearsAfter = dawn_native::GetLazyClearCountForTesting(device.Get());  \
            EXPECT_EQ(N, lazyClearsAfter - lazyClearsBefore);                                 \
        }                                                                                     \
    } while (0)

class TextureZeroInitTest : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();
        DAWN_SKIP_TEST_IF(UsesWire());
    }
    wgpu::TextureDescriptor CreateTextureDescriptor(uint32_t mipLevelCount,
                                                    uint32_t arrayLayerCount,
                                                    wgpu::TextureUsage usage,
                                                    wgpu::TextureFormat format) {
        wgpu::TextureDescriptor descriptor;
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size.width = kSize;
        descriptor.size.height = kSize;
        descriptor.size.depth = arrayLayerCount;
        descriptor.sampleCount = 1;
        descriptor.format = format;
        descriptor.mipLevelCount = mipLevelCount;
        descriptor.usage = usage;
        return descriptor;
    }
    wgpu::TextureViewDescriptor CreateTextureViewDescriptor(uint32_t baseMipLevel,
                                                            uint32_t baseArrayLayer) {
        wgpu::TextureViewDescriptor descriptor;
        descriptor.format = kColorFormat;
        descriptor.baseArrayLayer = baseArrayLayer;
        descriptor.arrayLayerCount = 1;
        descriptor.baseMipLevel = baseMipLevel;
        descriptor.mipLevelCount = 1;
        descriptor.dimension = wgpu::TextureViewDimension::e2D;
        return descriptor;
    }
    wgpu::RenderPipeline CreatePipelineForTest() {
        utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
        pipelineDescriptor.vertexStage.module = CreateBasicVertexShaderForTest();
        const char* fs =
            R"(#version 450
            layout(location = 0) out vec4 fragColor;
            void main() {
               fragColor = vec4(1.0, 0.0, 0.0, 1.0);
            })";
        pipelineDescriptor.cFragmentStage.module =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, fs);

        pipelineDescriptor.cDepthStencilState.depthCompare = wgpu::CompareFunction::Equal;
        pipelineDescriptor.cDepthStencilState.stencilFront.compare = wgpu::CompareFunction::Equal;
        pipelineDescriptor.depthStencilState = &pipelineDescriptor.cDepthStencilState;

        return device.CreateRenderPipeline(&pipelineDescriptor);
    }
    wgpu::ShaderModule CreateBasicVertexShaderForTest() {
        return utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(#version 450
            const vec2 pos[6] = vec2[6](vec2(-1.0f, -1.0f),
                                    vec2(-1.0f,  1.0f),
                                    vec2( 1.0f, -1.0f),
                                    vec2( 1.0f,  1.0f),
                                    vec2(-1.0f,  1.0f),
                                    vec2( 1.0f, -1.0f)
                                    );

            void main() {
                gl_Position = vec4(pos[gl_VertexIndex], 0.0, 1.0);
            })");
    }
    wgpu::ShaderModule CreateSampledTextureFragmentShaderForTest() {
        return utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment,
                                         R"(#version 450
            layout(set = 0, binding = 0) uniform sampler sampler0;
            layout(set = 0, binding = 1) uniform texture2D texture0;
            layout(location = 0) out vec4 fragColor;
            void main() {
                fragColor = texelFetch(sampler2D(texture0, sampler0), ivec2(gl_FragCoord), 0);
            })");
    }
    constexpr static uint32_t kSize = 128;
    constexpr static uint32_t kUnalignedSize = 127;
    // All three texture formats used (RGBA8Unorm, Depth24PlusStencil8, and RGBA8Snorm) have the
    // same byte size of 4.
    constexpr static uint32_t kFormatBlockByteSize = 4;
    constexpr static wgpu::TextureFormat kColorFormat = wgpu::TextureFormat::RGBA8Unorm;
    constexpr static wgpu::TextureFormat kDepthStencilFormat =
        wgpu::TextureFormat::Depth24PlusStencil8;
    constexpr static wgpu::TextureFormat kNonrenderableColorFormat =
        wgpu::TextureFormat::RGBA8Snorm;
};

// This tests that the code path of CopyTextureToBuffer clears correctly to Zero after first usage
TEST_P(TextureZeroInitTest, CopyTextureToBufferSource) {
    wgpu::TextureDescriptor descriptor = CreateTextureDescriptor(
        1, 1, wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc, kColorFormat);
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    // Texture's first usage is in EXPECT_PIXEL_RGBA8_EQ's call to CopyTextureToBuffer
    RGBA8 filledWithZeros(0, 0, 0, 0);
    EXPECT_LAZY_CLEAR(1u, EXPECT_PIXEL_RGBA8_EQ(filledWithZeros, texture, 0, 0));

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(texture.Get(), 0, 1, 0, 1));
}

// This tests that the code path of CopyTextureToBuffer with multiple texture array layers clears
// correctly to Zero after first usage
TEST_P(TextureZeroInitTest, CopyMultipleTextureArrayLayersToBufferSource) {
    constexpr uint32_t kArrayLayers = 6u;

    const wgpu::TextureDescriptor descriptor = CreateTextureDescriptor(
        1, kArrayLayers, wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc,
        kColorFormat);
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    const uint32_t bytesPerRow = utils::GetMinimumBytesPerRow(kColorFormat, kSize);
    wgpu::BufferDescriptor bufferDescriptor;
    bufferDescriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    bufferDescriptor.size =
        utils::GetBytesInBufferTextureCopy(kColorFormat, kSize, bytesPerRow, kSize, kArrayLayers);
    wgpu::Buffer buffer = device.CreateBuffer(&bufferDescriptor);

    const wgpu::BufferCopyView bufferCopyView =
        utils::CreateBufferCopyView(buffer, 0, bytesPerRow, 0);
    const wgpu::TextureCopyView textureCopyView =
        utils::CreateTextureCopyView(texture, 0, {0, 0, 0});
    const wgpu::Extent3D copySize = {kSize, kSize, kArrayLayers};

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyTextureToBuffer(&textureCopyView, &bufferCopyView, &copySize);
    wgpu::CommandBuffer commandBuffer = encoder.Finish();

    // Expect texture to be lazy initialized.
    EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commandBuffer));

    // Expect texture subresource initialized to be true
    EXPECT_TRUE(dawn_native::IsTextureSubresourceInitialized(texture.Get(), 0, 1, 0, kArrayLayers));

    const std::vector<RGBA8> kExpectedAllZero(kSize * kSize, {0, 0, 0, 0});
    for (uint32_t layer = 0; layer < kArrayLayers; ++layer) {
        EXPECT_TEXTURE_RGBA8_EQ(kExpectedAllZero.data(), texture, 0, 0, kSize, kSize, 0, layer);
    }
}

// Test that non-zero mip level clears subresource to Zero after first use
// This goes through the BeginRenderPass's code path
TEST_P(TextureZeroInitTest, RenderingMipMapClearsToZero) {
    uint32_t baseMipLevel = 2;
    uint32_t levelCount = 4;
    uint32_t baseArrayLayer = 0;
    uint32_t layerCount = 1;

    wgpu::TextureDescriptor descriptor = CreateTextureDescriptor(
        levelCount, layerCount, wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc,
        kColorFormat);
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    wgpu::TextureViewDescriptor viewDescriptor =
        CreateTextureViewDescriptor(baseMipLevel, baseArrayLayer);
    wgpu::TextureView view = texture.CreateView(&viewDescriptor);

    utils::BasicRenderPass renderPass = utils::BasicRenderPass(kSize, kSize, texture, kColorFormat);

    // Specify loadOp Load. Clear should be used to zero-initialize.
    renderPass.renderPassInfo.cColorAttachments[0].loadOp = wgpu::LoadOp::Load;
    // Specify non-zero clear color. It should still be cleared to zero.
    renderPass.renderPassInfo.cColorAttachments[0].clearColor = {0.5f, 0.5f, 0.5f, 0.5f};
    renderPass.renderPassInfo.cColorAttachments[0].attachment = view;

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        // Texture's first usage is in BeginRenderPass's call to RecordRenderPass
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.EndPass();
    }
    wgpu::CommandBuffer commands = encoder.Finish();
    EXPECT_LAZY_CLEAR(0u, queue.Submit(1, &commands));

    uint32_t mipSize = kSize >> 2;
    std::vector<RGBA8> expected(mipSize * mipSize, {0, 0, 0, 0});

    EXPECT_TEXTURE_RGBA8_EQ(expected.data(), renderPass.color, 0, 0, mipSize, mipSize, baseMipLevel,
                            baseArrayLayer);

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(
                        renderPass.color.Get(), baseMipLevel, 1, baseArrayLayer, 1));
}

// Test that non-zero array layers clears subresource to Zero after first use.
// This goes through the BeginRenderPass's code path
TEST_P(TextureZeroInitTest, RenderingArrayLayerClearsToZero) {
    uint32_t baseMipLevel = 0;
    uint32_t levelCount = 1;
    uint32_t baseArrayLayer = 2;
    uint32_t layerCount = 4;

    wgpu::TextureDescriptor descriptor = CreateTextureDescriptor(
        levelCount, layerCount, wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc,
        kColorFormat);
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    wgpu::TextureViewDescriptor viewDescriptor =
        CreateTextureViewDescriptor(baseMipLevel, baseArrayLayer);
    wgpu::TextureView view = texture.CreateView(&viewDescriptor);

    utils::BasicRenderPass renderPass = utils::BasicRenderPass(kSize, kSize, texture, kColorFormat);

    // Specify loadOp Load. Clear should be used to zero-initialize.
    renderPass.renderPassInfo.cColorAttachments[0].loadOp = wgpu::LoadOp::Load;
    // Specify non-zero clear color. It should still be cleared to zero.
    renderPass.renderPassInfo.cColorAttachments[0].clearColor = {0.5f, 0.5f, 0.5f, 0.5f};
    renderPass.renderPassInfo.cColorAttachments[0].attachment = view;

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.EndPass();
    }
    wgpu::CommandBuffer commands = encoder.Finish();
    EXPECT_LAZY_CLEAR(0u, queue.Submit(1, &commands));

    std::vector<RGBA8> expected(kSize * kSize, {0, 0, 0, 0});

    EXPECT_TEXTURE_RGBA8_EQ(expected.data(), renderPass.color, 0, 0, kSize, kSize, baseMipLevel,
                            baseArrayLayer);

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(
                        renderPass.color.Get(), baseMipLevel, 1, baseArrayLayer, 1));
}

// This tests CopyBufferToTexture fully overwrites copy so lazy init is not needed.
TEST_P(TextureZeroInitTest, CopyBufferToTexture) {
    wgpu::TextureDescriptor descriptor = CreateTextureDescriptor(
        4, 1,
        wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::Sampled | wgpu::TextureUsage::CopySrc,
        kColorFormat);
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    std::vector<uint8_t> data(kFormatBlockByteSize * kSize * kSize, 100);
    wgpu::Buffer stagingBuffer = utils::CreateBufferFromData(
        device, data.data(), static_cast<uint32_t>(data.size()), wgpu::BufferUsage::CopySrc);

    wgpu::BufferCopyView bufferCopyView =
        utils::CreateBufferCopyView(stagingBuffer, 0, kSize * sizeof(uint32_t), 0);
    wgpu::TextureCopyView textureCopyView = utils::CreateTextureCopyView(texture, 0, {0, 0, 0});
    wgpu::Extent3D copySize = {kSize, kSize, 1};

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copySize);
    wgpu::CommandBuffer commands = encoder.Finish();
    EXPECT_LAZY_CLEAR(0u, queue.Submit(1, &commands));

    std::vector<RGBA8> expected(kSize * kSize, {100, 100, 100, 100});

    EXPECT_TEXTURE_RGBA8_EQ(expected.data(), texture, 0, 0, kSize, kSize, 0, 0);

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(texture.Get(), 0, 1, 0, 1));
}

// Test for a copy only to a subset of the subresource, lazy init is necessary to clear the other
// half.
TEST_P(TextureZeroInitTest, CopyBufferToTextureHalf) {
    wgpu::TextureDescriptor descriptor = CreateTextureDescriptor(
        4, 1,
        wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::Sampled | wgpu::TextureUsage::CopySrc,
        kColorFormat);
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    std::vector<uint8_t> data(kFormatBlockByteSize * kSize * kSize, 100);
    wgpu::Buffer stagingBuffer = utils::CreateBufferFromData(
        device, data.data(), static_cast<uint32_t>(data.size()), wgpu::BufferUsage::CopySrc);

    wgpu::BufferCopyView bufferCopyView =
        utils::CreateBufferCopyView(stagingBuffer, 0, kSize * sizeof(uint16_t), 0);
    wgpu::TextureCopyView textureCopyView = utils::CreateTextureCopyView(texture, 0, {0, 0, 0});
    wgpu::Extent3D copySize = {kSize / 2, kSize, 1};

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copySize);
    wgpu::CommandBuffer commands = encoder.Finish();
    EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commands));

    std::vector<RGBA8> expected100((kSize / 2) * kSize, {100, 100, 100, 100});
    std::vector<RGBA8> expectedZeros((kSize / 2) * kSize, {0, 0, 0, 0});
    // first half filled with 100, by the buffer data
    EXPECT_TEXTURE_RGBA8_EQ(expected100.data(), texture, 0, 0, kSize / 2, kSize, 0, 0);
    // second half should be cleared
    EXPECT_TEXTURE_RGBA8_EQ(expectedZeros.data(), texture, kSize / 2, 0, kSize / 2, kSize, 0, 0);

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(texture.Get(), 0, 1, 0, 1));
}

// This tests CopyBufferToTexture fully overwrites a range of subresources, so lazy initialization
// is needed for neither the subresources involved in the copy nor the other subresources.
TEST_P(TextureZeroInitTest, CopyBufferToTextureMultipleArrayLayers) {
    wgpu::TextureDescriptor descriptor = CreateTextureDescriptor(
        1, 6, wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc, kColorFormat);
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    constexpr uint32_t kBaseArrayLayer = 2u;
    constexpr uint32_t kCopyLayerCount = 3u;
    std::vector<uint8_t> data(kFormatBlockByteSize * kSize * kSize * kCopyLayerCount, 100);
    wgpu::Buffer stagingBuffer = utils::CreateBufferFromData(
        device, data.data(), static_cast<uint32_t>(data.size()), wgpu::BufferUsage::CopySrc);

    const wgpu::BufferCopyView bufferCopyView =
        utils::CreateBufferCopyView(stagingBuffer, 0, kSize * kFormatBlockByteSize, 0);
    const wgpu::TextureCopyView textureCopyView =
        utils::CreateTextureCopyView(texture, 0, {0, 0, kBaseArrayLayer});
    const wgpu::Extent3D copySize = {kSize, kSize, kCopyLayerCount};

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copySize);
    wgpu::CommandBuffer commands = encoder.Finish();

    // The copy overwrites the whole subresources so we don't need to do lazy initialization on
    // them.
    EXPECT_LAZY_CLEAR(0u, queue.Submit(1, &commands));

    // Expect texture subresource initialized to be true
    EXPECT_TRUE(dawn_native::IsTextureSubresourceInitialized(texture.Get(), 0, 1, kBaseArrayLayer,
                                                             kCopyLayerCount));

    const std::vector<RGBA8> expected100(kSize * kSize, {100, 100, 100, 100});
    for (uint32_t layer = kBaseArrayLayer; layer < kBaseArrayLayer + kCopyLayerCount; ++layer) {
        EXPECT_TEXTURE_RGBA8_EQ(expected100.data(), texture, 0, 0, kSize, kSize, 0, layer);
    }
}

// This tests CopyTextureToTexture fully overwrites copy so lazy init is not needed.
TEST_P(TextureZeroInitTest, CopyTextureToTexture) {
    wgpu::TextureDescriptor srcDescriptor = CreateTextureDescriptor(
        1, 1, wgpu::TextureUsage::Sampled | wgpu::TextureUsage::CopySrc, kColorFormat);
    wgpu::Texture srcTexture = device.CreateTexture(&srcDescriptor);

    wgpu::TextureCopyView srcTextureCopyView =
        utils::CreateTextureCopyView(srcTexture, 0, {0, 0, 0});

    wgpu::TextureDescriptor dstDescriptor =
        CreateTextureDescriptor(1, 1,
                                wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopyDst |
                                    wgpu::TextureUsage::CopySrc,
                                kColorFormat);
    wgpu::Texture dstTexture = device.CreateTexture(&dstDescriptor);

    wgpu::TextureCopyView dstTextureCopyView =
        utils::CreateTextureCopyView(dstTexture, 0, {0, 0, 0});

    wgpu::Extent3D copySize = {kSize, kSize, 1};

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyTextureToTexture(&srcTextureCopyView, &dstTextureCopyView, &copySize);
    wgpu::CommandBuffer commands = encoder.Finish();
    EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commands));

    std::vector<RGBA8> expected(kSize * kSize, {0, 0, 0, 0});

    EXPECT_TEXTURE_RGBA8_EQ(expected.data(), srcTexture, 0, 0, kSize, kSize, 0, 0);
    EXPECT_TEXTURE_RGBA8_EQ(expected.data(), dstTexture, 0, 0, kSize, kSize, 0, 0);

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(srcTexture.Get(), 0, 1, 0, 1));
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(dstTexture.Get(), 0, 1, 0, 1));
}

// This Tests the CopyTextureToTexture's copy only to a subset of the subresource, lazy init is
// necessary to clear the other half.
TEST_P(TextureZeroInitTest, CopyTextureToTextureHalf) {
    wgpu::TextureDescriptor srcDescriptor = CreateTextureDescriptor(
        1, 1,
        wgpu::TextureUsage::Sampled | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst,
        kColorFormat);
    wgpu::Texture srcTexture = device.CreateTexture(&srcDescriptor);

    // fill srcTexture with 100
    {
        std::vector<uint8_t> data(kFormatBlockByteSize * kSize * kSize, 100);
        wgpu::Buffer stagingBuffer = utils::CreateBufferFromData(
            device, data.data(), static_cast<uint32_t>(data.size()), wgpu::BufferUsage::CopySrc);
        wgpu::BufferCopyView bufferCopyView =
            utils::CreateBufferCopyView(stagingBuffer, 0, kSize * kFormatBlockByteSize, 0);
        wgpu::TextureCopyView textureCopyView =
            utils::CreateTextureCopyView(srcTexture, 0, {0, 0, 0});
        wgpu::Extent3D copySize = {kSize, kSize, 1};
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copySize);
        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);
    }

    wgpu::TextureCopyView srcTextureCopyView =
        utils::CreateTextureCopyView(srcTexture, 0, {0, 0, 0});

    wgpu::TextureDescriptor dstDescriptor =
        CreateTextureDescriptor(1, 1,
                                wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopyDst |
                                    wgpu::TextureUsage::CopySrc,
                                kColorFormat);
    wgpu::Texture dstTexture = device.CreateTexture(&dstDescriptor);

    wgpu::TextureCopyView dstTextureCopyView =
        utils::CreateTextureCopyView(dstTexture, 0, {0, 0, 0});
    wgpu::Extent3D copySize = {kSize / 2, kSize, 1};

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyTextureToTexture(&srcTextureCopyView, &dstTextureCopyView, &copySize);
    wgpu::CommandBuffer commands = encoder.Finish();
    EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commands));

    std::vector<RGBA8> expectedWithZeros((kSize / 2) * kSize, {0, 0, 0, 0});
    std::vector<RGBA8> expectedWith100(kSize * kSize, {100, 100, 100, 100});

    EXPECT_TEXTURE_RGBA8_EQ(expectedWith100.data(), srcTexture, 0, 0, kSize, kSize, 0, 0);
    EXPECT_TEXTURE_RGBA8_EQ(expectedWith100.data(), dstTexture, 0, 0, kSize / 2, kSize, 0, 0);
    EXPECT_TEXTURE_RGBA8_EQ(expectedWithZeros.data(), dstTexture, kSize / 2, 0, kSize / 2, kSize, 0,
                            0);

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(srcTexture.Get(), 0, 1, 0, 1));
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(dstTexture.Get(), 0, 1, 0, 1));
}

// This tests the texture with depth attachment and load op load will init depth stencil texture to
// 0s.
TEST_P(TextureZeroInitTest, RenderingLoadingDepth) {
    wgpu::TextureDescriptor srcDescriptor =
        CreateTextureDescriptor(1, 1,
                                wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst |
                                    wgpu::TextureUsage::OutputAttachment,
                                kColorFormat);
    wgpu::Texture srcTexture = device.CreateTexture(&srcDescriptor);

    wgpu::TextureDescriptor depthStencilDescriptor = CreateTextureDescriptor(
        1, 1, wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc,
        kDepthStencilFormat);
    wgpu::Texture depthStencilTexture = device.CreateTexture(&depthStencilDescriptor);

    utils::ComboRenderPassDescriptor renderPassDescriptor({srcTexture.CreateView()},
                                                          depthStencilTexture.CreateView());
    renderPassDescriptor.cDepthStencilAttachmentInfo.depthLoadOp = wgpu::LoadOp::Load;
    // Set clearDepth to non-zero. It should still be cleared to 0 by the loadOp.
    renderPassDescriptor.cDepthStencilAttachmentInfo.clearDepth = 0.5f;
    renderPassDescriptor.cDepthStencilAttachmentInfo.stencilLoadOp = wgpu::LoadOp::Clear;
    renderPassDescriptor.cDepthStencilAttachmentInfo.clearStencil = 0;
    renderPassDescriptor.cDepthStencilAttachmentInfo.depthStoreOp = wgpu::StoreOp::Store;
    renderPassDescriptor.cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Store;

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    auto pass = encoder.BeginRenderPass(&renderPassDescriptor);
    pass.SetPipeline(CreatePipelineForTest());
    pass.Draw(6);
    pass.EndPass();
    wgpu::CommandBuffer commandBuffer = encoder.Finish();
    // Expect 0 lazy clears, depth stencil texture will clear using loadop
    EXPECT_LAZY_CLEAR(0u, queue.Submit(1, &commandBuffer));

    // Expect the texture to be red because depth test passed.
    std::vector<RGBA8> expected(kSize * kSize, {255, 0, 0, 255});
    EXPECT_TEXTURE_RGBA8_EQ(expected.data(), srcTexture, 0, 0, kSize, kSize, 0, 0);

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(srcTexture.Get(), 0, 1, 0, 1));
}

// This tests the texture with stencil attachment and load op load will init depth stencil texture
// to 0s.
TEST_P(TextureZeroInitTest, RenderingLoadingStencil) {
    wgpu::TextureDescriptor srcDescriptor =
        CreateTextureDescriptor(1, 1,
                                wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst |
                                    wgpu::TextureUsage::OutputAttachment,
                                kColorFormat);
    wgpu::Texture srcTexture = device.CreateTexture(&srcDescriptor);

    wgpu::TextureDescriptor depthStencilDescriptor = CreateTextureDescriptor(
        1, 1, wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc,
        kDepthStencilFormat);
    wgpu::Texture depthStencilTexture = device.CreateTexture(&depthStencilDescriptor);

    utils::ComboRenderPassDescriptor renderPassDescriptor({srcTexture.CreateView()},
                                                          depthStencilTexture.CreateView());
    renderPassDescriptor.cDepthStencilAttachmentInfo.depthLoadOp = wgpu::LoadOp::Clear;
    renderPassDescriptor.cDepthStencilAttachmentInfo.clearDepth = 0.0f;
    renderPassDescriptor.cDepthStencilAttachmentInfo.stencilLoadOp = wgpu::LoadOp::Load;
    // Set clearStencil to non-zero. It should still be cleared to 0 by the loadOp.
    renderPassDescriptor.cDepthStencilAttachmentInfo.clearStencil = 2;
    renderPassDescriptor.cDepthStencilAttachmentInfo.depthStoreOp = wgpu::StoreOp::Store;
    renderPassDescriptor.cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Store;

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    auto pass = encoder.BeginRenderPass(&renderPassDescriptor);
    pass.SetPipeline(CreatePipelineForTest());
    pass.Draw(6);
    pass.EndPass();
    wgpu::CommandBuffer commandBuffer = encoder.Finish();
    // Expect 0 lazy clears, depth stencil texture will clear using loadop
    EXPECT_LAZY_CLEAR(0u, queue.Submit(1, &commandBuffer));

    // Expect the texture to be red because stencil test passed.
    std::vector<RGBA8> expected(kSize * kSize, {255, 0, 0, 255});
    EXPECT_TEXTURE_RGBA8_EQ(expected.data(), srcTexture, 0, 0, kSize, kSize, 0, 0);

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(srcTexture.Get(), 0, 1, 0, 1));
}

// This tests the texture with depth stencil attachment and load op load will init depth stencil
// texture to 0s.
TEST_P(TextureZeroInitTest, RenderingLoadingDepthStencil) {
    wgpu::TextureDescriptor srcDescriptor =
        CreateTextureDescriptor(1, 1,
                                wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst |
                                    wgpu::TextureUsage::OutputAttachment,
                                kColorFormat);
    wgpu::Texture srcTexture = device.CreateTexture(&srcDescriptor);

    wgpu::TextureDescriptor depthStencilDescriptor = CreateTextureDescriptor(
        1, 1, wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc,
        kDepthStencilFormat);
    wgpu::Texture depthStencilTexture = device.CreateTexture(&depthStencilDescriptor);

    utils::ComboRenderPassDescriptor renderPassDescriptor({srcTexture.CreateView()},
                                                          depthStencilTexture.CreateView());
    renderPassDescriptor.cDepthStencilAttachmentInfo.depthLoadOp = wgpu::LoadOp::Load;
    renderPassDescriptor.cDepthStencilAttachmentInfo.stencilLoadOp = wgpu::LoadOp::Load;
    renderPassDescriptor.cDepthStencilAttachmentInfo.depthStoreOp = wgpu::StoreOp::Store;
    renderPassDescriptor.cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Store;

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    auto pass = encoder.BeginRenderPass(&renderPassDescriptor);
    pass.SetPipeline(CreatePipelineForTest());
    pass.Draw(6);
    pass.EndPass();
    wgpu::CommandBuffer commandBuffer = encoder.Finish();
    // Expect 0 lazy clears, depth stencil texture will clear using loadop
    EXPECT_LAZY_CLEAR(0u, queue.Submit(1, &commandBuffer));

    // Expect the texture to be red because both depth and stencil tests passed.
    std::vector<RGBA8> expected(kSize * kSize, {255, 0, 0, 255});
    EXPECT_TEXTURE_RGBA8_EQ(expected.data(), srcTexture, 0, 0, kSize, kSize, 0, 0);

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(srcTexture.Get(), 0, 1, 0, 1));
}

// This tests the color attachments clear to 0s
TEST_P(TextureZeroInitTest, ColorAttachmentsClear) {
    wgpu::TextureDescriptor descriptor = CreateTextureDescriptor(
        1, 1, wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc, kColorFormat);
    wgpu::Texture texture = device.CreateTexture(&descriptor);
    utils::BasicRenderPass renderPass = utils::BasicRenderPass(kSize, kSize, texture, kColorFormat);
    renderPass.renderPassInfo.cColorAttachments[0].loadOp = wgpu::LoadOp::Load;

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
    pass.EndPass();

    wgpu::CommandBuffer commands = encoder.Finish();
    EXPECT_LAZY_CLEAR(0u, queue.Submit(1, &commands));

    std::vector<RGBA8> expected(kSize * kSize, {0, 0, 0, 0});
    EXPECT_TEXTURE_RGBA8_EQ(expected.data(), renderPass.color, 0, 0, kSize, kSize, 0, 0);

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true,
              dawn_native::IsTextureSubresourceInitialized(renderPass.color.Get(), 0, 1, 0, 1));
}

// This tests the clearing of sampled textures in render pass
TEST_P(TextureZeroInitTest, RenderPassSampledTextureClear) {
    // Create needed resources
    wgpu::TextureDescriptor descriptor =
        CreateTextureDescriptor(1, 1, wgpu::TextureUsage::Sampled, kColorFormat);
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    wgpu::TextureDescriptor renderTextureDescriptor = CreateTextureDescriptor(
        1, 1, wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::OutputAttachment, kColorFormat);
    wgpu::Texture renderTexture = device.CreateTexture(&renderTextureDescriptor);

    wgpu::SamplerDescriptor samplerDesc = utils::GetDefaultSamplerDescriptor();
    wgpu::Sampler sampler = device.CreateSampler(&samplerDesc);

    // Create render pipeline
    utils::ComboRenderPipelineDescriptor renderPipelineDescriptor(device);
    renderPipelineDescriptor.cColorStates[0].format = kColorFormat;
    renderPipelineDescriptor.vertexStage.module = CreateBasicVertexShaderForTest();
    renderPipelineDescriptor.cFragmentStage.module = CreateSampledTextureFragmentShaderForTest();
    wgpu::RenderPipeline renderPipeline = device.CreateRenderPipeline(&renderPipelineDescriptor);

    // Create bindgroup
    wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, renderPipeline.GetBindGroupLayout(0),
                                                     {{0, sampler}, {1, texture.CreateView()}});

    // Encode pass and submit
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    utils::ComboRenderPassDescriptor renderPassDesc({renderTexture.CreateView()});
    renderPassDesc.cColorAttachments[0].clearColor = {1.0, 1.0, 1.0, 1.0};
    renderPassDesc.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
    pass.SetPipeline(renderPipeline);
    pass.SetBindGroup(0, bindGroup);
    pass.Draw(6);
    pass.EndPass();
    wgpu::CommandBuffer commands = encoder.Finish();
    // Expect 1 lazy clear for sampled texture
    EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commands));

    // Expect the rendered texture to be cleared
    std::vector<RGBA8> expectedWithZeros(kSize * kSize, {0, 0, 0, 0});
    EXPECT_TEXTURE_RGBA8_EQ(expectedWithZeros.data(), renderTexture, 0, 0, kSize, kSize, 0, 0);

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(renderTexture.Get(), 0, 1, 0, 1));
}

// This tests the clearing of sampled textures during compute pass
TEST_P(TextureZeroInitTest, ComputePassSampledTextureClear) {
    // Create needed resources
    wgpu::TextureDescriptor descriptor =
        CreateTextureDescriptor(1, 1, wgpu::TextureUsage::Sampled, kColorFormat);
    descriptor.size.width = 1;
    descriptor.size.height = 1;
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    uint32_t bufferSize = kFormatBlockByteSize * sizeof(uint32_t);
    wgpu::BufferDescriptor bufferDescriptor;
    bufferDescriptor.size = bufferSize;
    bufferDescriptor.usage =
        wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer bufferTex = device.CreateBuffer(&bufferDescriptor);
    // Add data to buffer to ensure it is initialized
    uint32_t data = 100;
    queue.WriteBuffer(bufferTex, 0, &data, sizeof(data));

    wgpu::SamplerDescriptor samplerDesc = utils::GetDefaultSamplerDescriptor();
    wgpu::Sampler sampler = device.CreateSampler(&samplerDesc);

    // Create compute pipeline
    wgpu::ComputePipelineDescriptor computePipelineDescriptor;
    wgpu::ProgrammableStageDescriptor computeStage;
    const char* cs =
        R"(#version 450
        layout(binding = 0) uniform texture2D sampleTex;
        layout(std430, binding = 1) buffer BufferTex {
           vec4 result;
        } bufferTex;
        layout(binding = 2) uniform sampler sampler0;
        void main() {
           bufferTex.result =
                 texelFetch(sampler2D(sampleTex, sampler0), ivec2(0,0), 0);
        })";
    computePipelineDescriptor.computeStage.module =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, cs);
    computePipelineDescriptor.computeStage.entryPoint = "main";
    wgpu::ComputePipeline computePipeline =
        device.CreateComputePipeline(&computePipelineDescriptor);

    // Create bindgroup
    wgpu::BindGroup bindGroup = utils::MakeBindGroup(
        device, computePipeline.GetBindGroupLayout(0),
        {{0, texture.CreateView()}, {1, bufferTex, 0, bufferSize}, {2, sampler}});

    // Encode the pass and submit
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
    pass.SetPipeline(computePipeline);
    pass.SetBindGroup(0, bindGroup);
    pass.Dispatch(1);
    pass.EndPass();
    wgpu::CommandBuffer commands = encoder.Finish();
    EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commands));

    // Expect the buffer to be zeroed out by the compute pass
    std::vector<uint32_t> expectedWithZeros(bufferSize, 0);
    EXPECT_BUFFER_U32_RANGE_EQ(expectedWithZeros.data(), bufferTex, 0, kFormatBlockByteSize);

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(texture.Get(), 0, 1, 0, 1));
}

// This tests that the code path of CopyTextureToBuffer clears correctly for non-renderable textures
TEST_P(TextureZeroInitTest, NonRenderableTextureClear) {
    wgpu::TextureDescriptor descriptor =
        CreateTextureDescriptor(1, 1, wgpu::TextureUsage::CopySrc, kNonrenderableColorFormat);
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    // Set buffer with dirty data so we know it is cleared by the lazy cleared texture copy
    uint32_t bytesPerRow = Align(kSize * kFormatBlockByteSize, kTextureBytesPerRowAlignment);
    uint32_t bufferSize = bytesPerRow * kSize;
    std::vector<uint8_t> data(bufferSize, 100);
    wgpu::Buffer bufferDst = utils::CreateBufferFromData(
        device, data.data(), static_cast<uint32_t>(data.size()), wgpu::BufferUsage::CopySrc);

    wgpu::BufferCopyView bufferCopyView = utils::CreateBufferCopyView(bufferDst, 0, bytesPerRow, 0);
    wgpu::TextureCopyView textureCopyView = utils::CreateTextureCopyView(texture, 0, {0, 0, 0});
    wgpu::Extent3D copySize = {kSize, kSize, 1};

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyTextureToBuffer(&textureCopyView, &bufferCopyView, &copySize);
    wgpu::CommandBuffer commands = encoder.Finish();
    EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commands));

    std::vector<uint32_t> expectedWithZeros(bufferSize, 0);
    EXPECT_BUFFER_U32_RANGE_EQ(expectedWithZeros.data(), bufferDst, 0, kSize);

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(texture.Get(), 0, 1, 0, 1));
}

// This tests that the code path of CopyTextureToBuffer clears correctly for non-renderable textures
TEST_P(TextureZeroInitTest, NonRenderableTextureClearUnalignedSize) {
    wgpu::TextureDescriptor descriptor =
        CreateTextureDescriptor(1, 1, wgpu::TextureUsage::CopySrc, kNonrenderableColorFormat);
    descriptor.size.width = kUnalignedSize;
    descriptor.size.height = kUnalignedSize;
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    // Set buffer with dirty data so we know it is cleared by the lazy cleared texture copy
    uint32_t bytesPerRow =
        Align(kUnalignedSize * kFormatBlockByteSize, kTextureBytesPerRowAlignment);
    uint32_t bufferSize = bytesPerRow * kUnalignedSize;
    std::vector<uint8_t> data(bufferSize, 100);
    wgpu::Buffer bufferDst = utils::CreateBufferFromData(
        device, data.data(), static_cast<uint32_t>(data.size()), wgpu::BufferUsage::CopySrc);
    wgpu::BufferCopyView bufferCopyView = utils::CreateBufferCopyView(bufferDst, 0, bytesPerRow, 0);
    wgpu::TextureCopyView textureCopyView = utils::CreateTextureCopyView(texture, 0, {0, 0, 0});
    wgpu::Extent3D copySize = {kUnalignedSize, kUnalignedSize, 1};

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyTextureToBuffer(&textureCopyView, &bufferCopyView, &copySize);
    wgpu::CommandBuffer commands = encoder.Finish();
    EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commands));

    std::vector<uint32_t> expectedWithZeros(bufferSize, 0);
    EXPECT_BUFFER_U32_RANGE_EQ(expectedWithZeros.data(), bufferDst, 0, kUnalignedSize);

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(texture.Get(), 0, 1, 0, 1));
}

// This tests that the code path of CopyTextureToBuffer clears correctly for non-renderable textures
// with more than 1 array layers
TEST_P(TextureZeroInitTest, NonRenderableTextureClearWithMultiArrayLayers) {
    wgpu::TextureDescriptor descriptor =
        CreateTextureDescriptor(1, 2, wgpu::TextureUsage::CopySrc, kNonrenderableColorFormat);
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    // Set buffer with dirty data so we know it is cleared by the lazy cleared texture copy
    uint32_t bufferSize = kFormatBlockByteSize * kSize * kSize;
    std::vector<uint8_t> data(bufferSize, 100);
    wgpu::Buffer bufferDst = utils::CreateBufferFromData(
        device, data.data(), static_cast<uint32_t>(data.size()), wgpu::BufferUsage::CopySrc);

    wgpu::BufferCopyView bufferCopyView =
        utils::CreateBufferCopyView(bufferDst, 0, kSize * kFormatBlockByteSize, 0);
    wgpu::TextureCopyView textureCopyView = utils::CreateTextureCopyView(texture, 0, {0, 0, 1});
    wgpu::Extent3D copySize = {kSize, kSize, 1};

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyTextureToBuffer(&textureCopyView, &bufferCopyView, &copySize);
    wgpu::CommandBuffer commands = encoder.Finish();
    EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commands));

    std::vector<uint32_t> expectedWithZeros(bufferSize, 0);
    EXPECT_BUFFER_U32_RANGE_EQ(expectedWithZeros.data(), bufferDst, 0, 8);

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(texture.Get(), 0, 1, 1, 1));
}

// This tests that storeOp clear resets resource state to uninitialized.
// Start with a sample texture that is initialized with data.
// Then expect the render texture to not store the data from sample texture
// because it will be lazy cleared by the EXPECT_TEXTURE_RGBA8_EQ call.
TEST_P(TextureZeroInitTest, RenderPassStoreOpClear) {
    // Create needed resources
    wgpu::TextureDescriptor descriptor = CreateTextureDescriptor(
        1, 1, wgpu::TextureUsage::Sampled | wgpu::TextureUsage::CopyDst, kColorFormat);
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    wgpu::TextureDescriptor renderTextureDescriptor = CreateTextureDescriptor(
        1, 1, wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::OutputAttachment, kColorFormat);
    wgpu::Texture renderTexture = device.CreateTexture(&renderTextureDescriptor);

    wgpu::SamplerDescriptor samplerDesc = utils::GetDefaultSamplerDescriptor();
    wgpu::Sampler sampler = device.CreateSampler(&samplerDesc);

    // Fill the sample texture with data
    std::vector<uint8_t> data(kFormatBlockByteSize * kSize * kSize, 1);
    wgpu::Buffer stagingBuffer = utils::CreateBufferFromData(
        device, data.data(), static_cast<uint32_t>(data.size()), wgpu::BufferUsage::CopySrc);
    wgpu::BufferCopyView bufferCopyView =
        utils::CreateBufferCopyView(stagingBuffer, 0, kSize * kFormatBlockByteSize, 0);
    wgpu::TextureCopyView textureCopyView = utils::CreateTextureCopyView(texture, 0, {0, 0, 0});
    wgpu::Extent3D copySize = {kSize, kSize, 1};
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copySize);
    wgpu::CommandBuffer commands = encoder.Finish();
    // Expect 0 lazy clears because the texture will be completely copied to
    EXPECT_LAZY_CLEAR(0u, queue.Submit(1, &commands));

    // Create render pipeline
    utils::ComboRenderPipelineDescriptor renderPipelineDescriptor(device);
    renderPipelineDescriptor.vertexStage.module = CreateBasicVertexShaderForTest();
    renderPipelineDescriptor.cFragmentStage.module = CreateSampledTextureFragmentShaderForTest();
    renderPipelineDescriptor.cColorStates[0].format = kColorFormat;
    wgpu::RenderPipeline renderPipeline = device.CreateRenderPipeline(&renderPipelineDescriptor);

    // Create bindgroup
    wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, renderPipeline.GetBindGroupLayout(0),
                                                     {{0, sampler}, {1, texture.CreateView()}});

    // Encode pass and submit
    encoder = device.CreateCommandEncoder();
    utils::ComboRenderPassDescriptor renderPassDesc({renderTexture.CreateView()});
    renderPassDesc.cColorAttachments[0].clearColor = {0.0, 0.0, 0.0, 0.0};
    renderPassDesc.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
    renderPassDesc.cColorAttachments[0].storeOp = wgpu::StoreOp::Clear;
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
    pass.SetPipeline(renderPipeline);
    pass.SetBindGroup(0, bindGroup);
    pass.Draw(6);
    pass.EndPass();
    commands = encoder.Finish();
    // Expect 0 lazy clears, sample texture is initialized by copyBufferToTexture and render texture
    // is cleared by loadop
    EXPECT_LAZY_CLEAR(0u, queue.Submit(1, &commands));

    // Expect the rendered texture to be cleared
    std::vector<RGBA8> expectedWithZeros(kSize * kSize, {0, 0, 0, 0});
    EXPECT_LAZY_CLEAR(1u, EXPECT_TEXTURE_RGBA8_EQ(expectedWithZeros.data(), renderTexture, 0, 0,
                                                  kSize, kSize, 0, 0));

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(texture.Get(), 0, 1, 0, 1));
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(renderTexture.Get(), 0, 1, 0, 1));
}

// This tests storeOp Clear on depth and stencil textures.
// We put the depth stencil texture through 2 passes:
// 1) LoadOp::Clear and StoreOp::Clear, fail the depth and stencil test set in the render pipeline.
//      This means nothing is drawn and subresource is set as uninitialized.
// 2) LoadOp::Load and StoreOp::Clear, pass the depth and stencil test set in the render pipeline.
//      Because LoadOp is Load and the subresource is uninitialized, the texture will be cleared to
//      0's This means the depth and stencil test will pass and the red square is drawn.
TEST_P(TextureZeroInitTest, RenderingLoadingDepthStencilStoreOpClear) {
    wgpu::TextureDescriptor srcDescriptor =
        CreateTextureDescriptor(1, 1,
                                wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst |
                                    wgpu::TextureUsage::OutputAttachment,
                                kColorFormat);
    wgpu::Texture srcTexture = device.CreateTexture(&srcDescriptor);

    wgpu::TextureDescriptor depthStencilDescriptor =
        CreateTextureDescriptor(1, 1,
                                wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc |
                                    wgpu::TextureUsage::CopyDst,
                                kDepthStencilFormat);
    wgpu::Texture depthStencilTexture = device.CreateTexture(&depthStencilDescriptor);

    // Setup the renderPass for the first pass.
    // We want to fail the depth and stencil test here so that nothing gets drawn and we can
    // see that the subresource correctly gets set as unintialized in the second pass
    utils::ComboRenderPassDescriptor renderPassDescriptor({srcTexture.CreateView()},
                                                          depthStencilTexture.CreateView());
    renderPassDescriptor.cDepthStencilAttachmentInfo.depthLoadOp = wgpu::LoadOp::Clear;
    renderPassDescriptor.cDepthStencilAttachmentInfo.stencilLoadOp = wgpu::LoadOp::Clear;
    renderPassDescriptor.cDepthStencilAttachmentInfo.clearDepth = 1.0f;
    renderPassDescriptor.cDepthStencilAttachmentInfo.clearStencil = 1u;
    renderPassDescriptor.cDepthStencilAttachmentInfo.depthStoreOp = wgpu::StoreOp::Clear;
    renderPassDescriptor.cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Clear;
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDescriptor);
        pass.SetPipeline(CreatePipelineForTest());
        pass.Draw(6);
        pass.EndPass();
        wgpu::CommandBuffer commandBuffer = encoder.Finish();
        // Expect 0 lazy clears, depth stencil texture will clear using loadop
        EXPECT_LAZY_CLEAR(0u, queue.Submit(1, &commandBuffer));

        // The depth stencil test should fail and not draw because the depth stencil texture is
        // cleared to 1's by using loadOp clear and set values from descriptor.
        std::vector<RGBA8> expectedBlack(kSize * kSize, {0, 0, 0, 0});
        EXPECT_TEXTURE_RGBA8_EQ(expectedBlack.data(), srcTexture, 0, 0, kSize, kSize, 0, 0);

        // Expect texture subresource initialized to be false since storeop is clear, sets
        // subresource as uninitialized
        EXPECT_EQ(false, dawn_native::IsTextureSubresourceInitialized(depthStencilTexture.Get(), 0,
                                                                      1, 0, 1));
    }

    // Now we put the depth stencil texture back into renderpass, it should be cleared by loadop
    // because storeOp clear sets the subresource as uninitialized
    {
        renderPassDescriptor.cDepthStencilAttachmentInfo.depthLoadOp = wgpu::LoadOp::Load;
        renderPassDescriptor.cDepthStencilAttachmentInfo.stencilLoadOp = wgpu::LoadOp::Load;
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDescriptor);
        pass.SetPipeline(CreatePipelineForTest());
        pass.Draw(6);
        pass.EndPass();
        wgpu::CommandBuffer commandBuffer = encoder.Finish();
        // Expect 0 lazy clears, depth stencil texture will clear using loadop
        EXPECT_LAZY_CLEAR(0u, queue.Submit(1, &commandBuffer));

        // Now the depth stencil test should pass since depth stencil texture is cleared to 0's by
        // loadop load and uninitialized subresource, so we should have a red square
        std::vector<RGBA8> expectedRed(kSize * kSize, {255, 0, 0, 255});
        EXPECT_TEXTURE_RGBA8_EQ(expectedRed.data(), srcTexture, 0, 0, kSize, kSize, 0, 0);

        // Expect texture subresource initialized to be false since storeop is clear, sets
        // subresource as uninitialized
        EXPECT_EQ(false, dawn_native::IsTextureSubresourceInitialized(depthStencilTexture.Get(), 0,
                                                                      1, 0, 1));
    }
}

// Test that if one mip of a texture is initialized and another is uninitialized, lazy clearing the
// uninitialized mip does not clear the initialized mip.
TEST_P(TextureZeroInitTest, PreservesInitializedMip) {
    wgpu::TextureDescriptor sampleTextureDescriptor = CreateTextureDescriptor(
        2, 1,
        wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::Sampled,
        kColorFormat);
    wgpu::Texture sampleTexture = device.CreateTexture(&sampleTextureDescriptor);

    wgpu::SamplerDescriptor samplerDesc = utils::GetDefaultSamplerDescriptor();
    wgpu::Sampler sampler = device.CreateSampler(&samplerDesc);

    wgpu::TextureDescriptor renderTextureDescriptor = CreateTextureDescriptor(
        1, 1, wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::OutputAttachment, kColorFormat);
    wgpu::Texture renderTexture = device.CreateTexture(&renderTextureDescriptor);

    // Fill the sample texture's second mip with data
    uint32_t mipSize = kSize >> 1;
    std::vector<uint8_t> data(kFormatBlockByteSize * mipSize * mipSize, 2);
    wgpu::Buffer stagingBuffer = utils::CreateBufferFromData(
        device, data.data(), static_cast<uint32_t>(data.size()), wgpu::BufferUsage::CopySrc);
    wgpu::BufferCopyView bufferCopyView =
        utils::CreateBufferCopyView(stagingBuffer, 0, mipSize * kFormatBlockByteSize, 0);
    wgpu::TextureCopyView textureCopyView =
        utils::CreateTextureCopyView(sampleTexture, 1, {0, 0, 0});
    wgpu::Extent3D copySize = {mipSize, mipSize, 1};
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copySize);
    wgpu::CommandBuffer commands = encoder.Finish();
    // Expect 0 lazy clears because the texture subresource will be completely copied to
    EXPECT_LAZY_CLEAR(0u, queue.Submit(1, &commands));

    // Create render pipeline
    utils::ComboRenderPipelineDescriptor renderPipelineDescriptor(device);
    renderPipelineDescriptor.vertexStage.module = CreateBasicVertexShaderForTest();
    renderPipelineDescriptor.cFragmentStage.module = CreateSampledTextureFragmentShaderForTest();
    renderPipelineDescriptor.cColorStates[0].format = kColorFormat;
    wgpu::RenderPipeline renderPipeline = device.CreateRenderPipeline(&renderPipelineDescriptor);

    // Create bindgroup
    wgpu::BindGroup bindGroup =
        utils::MakeBindGroup(device, renderPipeline.GetBindGroupLayout(0),
                             {{0, sampler}, {1, sampleTexture.CreateView()}});

    // Encode pass and submit
    encoder = device.CreateCommandEncoder();
    utils::ComboRenderPassDescriptor renderPassDesc({renderTexture.CreateView()});
    renderPassDesc.cColorAttachments[0].clearColor = {0.0, 0.0, 0.0, 0.0};
    renderPassDesc.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
    renderPassDesc.cColorAttachments[0].storeOp = wgpu::StoreOp::Clear;
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
    pass.SetPipeline(renderPipeline);
    pass.SetBindGroup(0, bindGroup);
    pass.Draw(6);
    pass.EndPass();
    commands = encoder.Finish();
    // Expect 1 lazy clears, because not all mips of the sample texture are initialized by
    // copyBufferToTexture.
    EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commands));

    // Expect the rendered texture to be cleared since we copied from the uninitialized first
    // mip.
    std::vector<RGBA8> expectedWithZeros(kSize * kSize, {0, 0, 0, 0});
    EXPECT_LAZY_CLEAR(1u, EXPECT_TEXTURE_RGBA8_EQ(expectedWithZeros.data(), renderTexture, 0, 0,
                                                  kSize, kSize, 0, 0));

    // Expect the first mip to have been lazy cleared to 0.
    EXPECT_LAZY_CLEAR(0u, EXPECT_TEXTURE_RGBA8_EQ(expectedWithZeros.data(), sampleTexture, 0, 0,
                                                  kSize, kSize, 0, 0));

    // Expect the second mip to still be filled with 2.
    std::vector<RGBA8> expectedWithTwos(mipSize * mipSize, {2, 2, 2, 2});
    EXPECT_LAZY_CLEAR(0u, EXPECT_TEXTURE_RGBA8_EQ(expectedWithTwos.data(), sampleTexture, 0, 0,
                                                  mipSize, mipSize, 1, 0));

    // Expect the whole texture to be initialized
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(sampleTexture.Get(), 0, 2, 0, 1));
}

// Test that if one layer of a texture is initialized and another is uninitialized, lazy clearing
// the uninitialized layer does not clear the initialized layer.
TEST_P(TextureZeroInitTest, PreservesInitializedArrayLayer) {
    wgpu::TextureDescriptor sampleTextureDescriptor = CreateTextureDescriptor(
        1, 2,
        wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::Sampled,
        kColorFormat);
    wgpu::Texture sampleTexture = device.CreateTexture(&sampleTextureDescriptor);

    wgpu::SamplerDescriptor samplerDesc = utils::GetDefaultSamplerDescriptor();
    wgpu::Sampler sampler = device.CreateSampler(&samplerDesc);

    wgpu::TextureDescriptor renderTextureDescriptor = CreateTextureDescriptor(
        1, 1, wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::OutputAttachment, kColorFormat);
    wgpu::Texture renderTexture = device.CreateTexture(&renderTextureDescriptor);

    // Fill the sample texture's second array layer with data
    std::vector<uint8_t> data(kFormatBlockByteSize * kSize * kSize, 2);
    wgpu::Buffer stagingBuffer = utils::CreateBufferFromData(
        device, data.data(), static_cast<uint32_t>(data.size()), wgpu::BufferUsage::CopySrc);
    wgpu::BufferCopyView bufferCopyView =
        utils::CreateBufferCopyView(stagingBuffer, 0, kSize * kFormatBlockByteSize, 0);
    wgpu::TextureCopyView textureCopyView =
        utils::CreateTextureCopyView(sampleTexture, 0, {0, 0, 1});
    wgpu::Extent3D copySize = {kSize, kSize, 1};
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copySize);
    wgpu::CommandBuffer commands = encoder.Finish();
    // Expect 0 lazy clears because the texture subresource will be completely copied to
    EXPECT_LAZY_CLEAR(0u, queue.Submit(1, &commands));

    // Create render pipeline
    utils::ComboRenderPipelineDescriptor renderPipelineDescriptor(device);
    renderPipelineDescriptor.vertexStage.module = CreateBasicVertexShaderForTest();
    renderPipelineDescriptor.cFragmentStage.module = CreateSampledTextureFragmentShaderForTest();
    renderPipelineDescriptor.cColorStates[0].format = kColorFormat;
    wgpu::RenderPipeline renderPipeline = device.CreateRenderPipeline(&renderPipelineDescriptor);

    // Only sample from the uninitialized first layer.
    wgpu::TextureViewDescriptor textureViewDescriptor;
    textureViewDescriptor.dimension = wgpu::TextureViewDimension::e2D;
    textureViewDescriptor.arrayLayerCount = 1;

    // Create bindgroup
    wgpu::BindGroup bindGroup =
        utils::MakeBindGroup(device, renderPipeline.GetBindGroupLayout(0),
                             {{0, sampler}, {1, sampleTexture.CreateView(&textureViewDescriptor)}});

    // Encode pass and submit
    encoder = device.CreateCommandEncoder();
    utils::ComboRenderPassDescriptor renderPassDesc({renderTexture.CreateView()});
    renderPassDesc.cColorAttachments[0].clearColor = {0.0, 0.0, 0.0, 0.0};
    renderPassDesc.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
    renderPassDesc.cColorAttachments[0].storeOp = wgpu::StoreOp::Clear;
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
    pass.SetPipeline(renderPipeline);
    pass.SetBindGroup(0, bindGroup);
    pass.Draw(6);
    pass.EndPass();
    commands = encoder.Finish();
    // Expect 1 lazy clears, because not all array layers of the sample texture are initialized by
    // copyBufferToTexture.
    EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commands));

    // Expect the rendered texture to be cleared since we copied from the uninitialized first
    // array layer.
    std::vector<RGBA8> expectedWithZeros(kSize * kSize, {0, 0, 0, 0});
    EXPECT_LAZY_CLEAR(1u, EXPECT_TEXTURE_RGBA8_EQ(expectedWithZeros.data(), renderTexture, 0, 0,
                                                  kSize, kSize, 0, 0));

    // Expect the first array layer to have been lazy cleared to 0.
    EXPECT_LAZY_CLEAR(0u, EXPECT_TEXTURE_RGBA8_EQ(expectedWithZeros.data(), sampleTexture, 0, 0,
                                                  kSize, kSize, 0, 0));

    // Expect the second array layer to still be filled with 2.
    std::vector<RGBA8> expectedWithTwos(kSize * kSize, {2, 2, 2, 2});
    EXPECT_LAZY_CLEAR(0u, EXPECT_TEXTURE_RGBA8_EQ(expectedWithTwos.data(), sampleTexture, 0, 0,
                                                  kSize, kSize, 0, 1));

    // Expect the whole texture to be initialized
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(sampleTexture.Get(), 0, 1, 0, 2));
}

// This is a regression test for crbug.com/dawn/451 where the lazy texture
// init path on D3D12 had a divide-by-zero exception in the copy split logic.
TEST_P(TextureZeroInitTest, CopyTextureToBufferNonRenderableUnaligned) {
    wgpu::TextureDescriptor descriptor;
    descriptor.size.width = kUnalignedSize;
    descriptor.size.height = kUnalignedSize;
    descriptor.size.depth = 1;
    descriptor.format = wgpu::TextureFormat::R8Snorm;
    descriptor.usage = wgpu::TextureUsage::CopySrc;
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    {
        uint32_t bytesPerRow = Align(kUnalignedSize, kTextureBytesPerRowAlignment);

        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.size = kUnalignedSize * bytesPerRow;
        bufferDesc.usage = wgpu::BufferUsage::CopyDst;
        wgpu::Buffer buffer = device.CreateBuffer(&bufferDesc);

        wgpu::TextureCopyView textureCopyView = utils::CreateTextureCopyView(texture, 0, {0, 0, 0});
        wgpu::BufferCopyView bufferCopyView =
            utils::CreateBufferCopyView(buffer, 0, bytesPerRow, 0);
        wgpu::Extent3D copySize = {kUnalignedSize, kUnalignedSize, 1};

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyTextureToBuffer(&textureCopyView, &bufferCopyView, &copySize);

        wgpu::CommandBuffer commands = encoder.Finish();
        EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commands));
    }

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(texture.Get(), 0, 1, 0, 1));
}

// In this test WriteTexture fully overwrites a texture
TEST_P(TextureZeroInitTest, WriteWholeTexture) {
    // TODO(dawn:483): Remove this condition after implementing WriteTexture in those backends.
    DAWN_SKIP_TEST_IF(IsOpenGL() || IsD3D12());

    wgpu::TextureDescriptor descriptor = CreateTextureDescriptor(
        1, 1, wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc, kColorFormat);
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    wgpu::TextureCopyView textureCopyView = utils::CreateTextureCopyView(texture, 0, {0, 0, 0});
    wgpu::Extent3D copySize = {kSize, kSize, 1};

    wgpu::TextureDataLayout textureDataLayout;
    textureDataLayout.offset = 0;
    textureDataLayout.bytesPerRow = kSize * kFormatBlockByteSize;
    textureDataLayout.rowsPerImage = kSize;

    std::vector<RGBA8> data(
        utils::RequiredBytesInCopy(textureDataLayout.bytesPerRow, textureDataLayout.rowsPerImage,
                                   copySize, kColorFormat) /
            sizeof(RGBA8),
        {100, 100, 100, 100});

    // The write overwrites the whole texture so we don't need to do lazy initialization.
    EXPECT_LAZY_CLEAR(0u,
                      queue.WriteTexture(&textureCopyView, data.data(), data.size() * sizeof(RGBA8),
                                         &textureDataLayout, &copySize));

    // Expect texture initialized to be true
    EXPECT_TRUE(dawn_native::IsTextureSubresourceInitialized(texture.Get(), 0, 1, 0, 1));

    EXPECT_TEXTURE_RGBA8_EQ(data.data(), texture, 0, 0, kSize, kSize, 0, 0);
}

// Test WriteTexture to a subset of the texture, lazy init is necessary to clear the other
// half.
TEST_P(TextureZeroInitTest, WriteTextureHalf) {
    // TODO(dawn:483): Remove this condition after implementing WriteTexture in those backends.
    DAWN_SKIP_TEST_IF(IsOpenGL() || IsD3D12());

    wgpu::TextureDescriptor descriptor = CreateTextureDescriptor(
        4, 1,
        wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::Sampled | wgpu::TextureUsage::CopySrc,
        kColorFormat);
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    wgpu::TextureCopyView textureCopyView = utils::CreateTextureCopyView(texture, 0, {0, 0, 0});
    wgpu::Extent3D copySize = {kSize / 2, kSize, 1};

    wgpu::TextureDataLayout textureDataLayout;
    textureDataLayout.offset = 0;
    textureDataLayout.bytesPerRow = kSize * kFormatBlockByteSize / 2;
    textureDataLayout.rowsPerImage = kSize;

    std::vector<RGBA8> data(
        utils::RequiredBytesInCopy(textureDataLayout.bytesPerRow, textureDataLayout.rowsPerImage,
                                   copySize, kColorFormat) /
            sizeof(RGBA8),
        {100, 100, 100, 100});

    EXPECT_LAZY_CLEAR(1u,
                      queue.WriteTexture(&textureCopyView, data.data(), data.size() * sizeof(RGBA8),
                                         &textureDataLayout, &copySize));

    // Expect texture initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(texture.Get(), 0, 1, 0, 1));

    std::vector<RGBA8> expectedZeros((kSize / 2) * kSize, {0, 0, 0, 0});
    // first half filled with 100, by the data
    EXPECT_TEXTURE_RGBA8_EQ(data.data(), texture, 0, 0, kSize / 2, kSize, 0, 0);
    // second half should be cleared
    EXPECT_TEXTURE_RGBA8_EQ(expectedZeros.data(), texture, kSize / 2, 0, kSize / 2, kSize, 0, 0);
}

// In this test WriteTexture fully overwrites a range of subresources, so lazy initialization
// is needed for neither the subresources involved in the write nor the other subresources.
TEST_P(TextureZeroInitTest, WriteWholeTextureArray) {
    // TODO(dawn:483): Remove this condition after implementing WriteTexture in those backends.
    DAWN_SKIP_TEST_IF(IsOpenGL() || IsD3D12());

    wgpu::TextureDescriptor descriptor = CreateTextureDescriptor(
        1, 6, wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc, kColorFormat);
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    constexpr uint32_t kBaseArrayLayer = 2u;
    constexpr uint32_t kCopyLayerCount = 3u;

    wgpu::TextureCopyView textureCopyView =
        utils::CreateTextureCopyView(texture, 0, {0, 0, kBaseArrayLayer});
    wgpu::Extent3D copySize = {kSize, kSize, kCopyLayerCount};

    wgpu::TextureDataLayout textureDataLayout;
    textureDataLayout.offset = 0;
    textureDataLayout.bytesPerRow = kSize * kFormatBlockByteSize;
    textureDataLayout.rowsPerImage = kSize;

    std::vector<RGBA8> data(
        utils::RequiredBytesInCopy(textureDataLayout.bytesPerRow, textureDataLayout.rowsPerImage,
                                   copySize, kColorFormat) /
            sizeof(RGBA8),
        {100, 100, 100, 100});

    // The write overwrites the whole subresources so we don't need to do lazy initialization on
    // them.
    EXPECT_LAZY_CLEAR(0u,
                      queue.WriteTexture(&textureCopyView, data.data(), data.size() * sizeof(RGBA8),
                                         &textureDataLayout, &copySize));

    // Expect texture subresource initialized to be true
    EXPECT_TRUE(dawn_native::IsTextureSubresourceInitialized(texture.Get(), 0, 1, kBaseArrayLayer,
                                                             kCopyLayerCount));

    for (uint32_t layer = kBaseArrayLayer; layer < kBaseArrayLayer + kCopyLayerCount; ++layer) {
        EXPECT_TEXTURE_RGBA8_EQ(data.data(), texture, 0, 0, kSize, kSize, 0, layer);
    }
}

// Test WriteTexture to a subset of the subresource, lazy init is necessary to clear the other
// half.
TEST_P(TextureZeroInitTest, WriteTextureArrayHalf) {
    // TODO(dawn:483): Remove this condition after implementing WriteTexture in those backends.
    DAWN_SKIP_TEST_IF(IsOpenGL() || IsD3D12());

    wgpu::TextureDescriptor descriptor = CreateTextureDescriptor(
        4, 6,
        wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::Sampled | wgpu::TextureUsage::CopySrc,
        kColorFormat);
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    constexpr uint32_t kBaseArrayLayer = 2u;
    constexpr uint32_t kCopyLayerCount = 3u;

    wgpu::TextureCopyView textureCopyView =
        utils::CreateTextureCopyView(texture, 0, {0, 0, kBaseArrayLayer});
    wgpu::Extent3D copySize = {kSize / 2, kSize, kCopyLayerCount};

    wgpu::TextureDataLayout textureDataLayout;
    textureDataLayout.offset = 0;
    textureDataLayout.bytesPerRow = kSize * kFormatBlockByteSize / 2;
    textureDataLayout.rowsPerImage = kSize;

    std::vector<RGBA8> data(
        utils::RequiredBytesInCopy(textureDataLayout.bytesPerRow, textureDataLayout.rowsPerImage,
                                   copySize, kColorFormat) /
            sizeof(RGBA8),
        {100, 100, 100, 100});

    EXPECT_LAZY_CLEAR(1u,
                      queue.WriteTexture(&textureCopyView, data.data(), data.size() * sizeof(RGBA8),
                                         &textureDataLayout, &copySize));

    // Expect texture subresource initialized to be true
    EXPECT_EQ(true, dawn_native::IsTextureSubresourceInitialized(texture.Get(), 0, 1,
                                                                 kBaseArrayLayer, kCopyLayerCount));

    std::vector<RGBA8> expectedZeros((kSize / 2) * kSize, {0, 0, 0, 0});
    for (uint32_t layer = kBaseArrayLayer; layer < kBaseArrayLayer + kCopyLayerCount; ++layer) {
        // first half filled with 100, by the data
        EXPECT_TEXTURE_RGBA8_EQ(data.data(), texture, 0, 0, kSize / 2, kSize, 0, layer);
        // second half should be cleared
        EXPECT_TEXTURE_RGBA8_EQ(expectedZeros.data(), texture, kSize / 2, 0, kSize / 2, kSize, 0,
                                layer);
    }
}

// In this test WriteTexture fully overwrites a texture at mip level.
TEST_P(TextureZeroInitTest, WriteWholeTextureAtMipLevel) {
    // TODO(dawn:483): Remove this condition after implementing WriteTexture in those backends.
    DAWN_SKIP_TEST_IF(IsOpenGL() || IsD3D12());

    wgpu::TextureDescriptor descriptor = CreateTextureDescriptor(
        4, 1, wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc, kColorFormat);
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    constexpr uint32_t kMipLevel = 2;
    constexpr uint32_t kMipSize = kSize >> kMipLevel;

    wgpu::TextureCopyView textureCopyView =
        utils::CreateTextureCopyView(texture, kMipLevel, {0, 0, 0});
    wgpu::Extent3D copySize = {kMipSize, kMipSize, 1};

    wgpu::TextureDataLayout textureDataLayout;
    textureDataLayout.offset = 0;
    textureDataLayout.bytesPerRow = kMipSize * kFormatBlockByteSize;
    textureDataLayout.rowsPerImage = kMipSize;

    std::vector<RGBA8> data(
        utils::RequiredBytesInCopy(textureDataLayout.bytesPerRow, textureDataLayout.rowsPerImage,
                                   copySize, kColorFormat) /
            sizeof(RGBA8),
        {100, 100, 100, 100});

    // The write overwrites the whole texture so we don't need to do lazy initialization.
    EXPECT_LAZY_CLEAR(0u,
                      queue.WriteTexture(&textureCopyView, data.data(), data.size() * sizeof(RGBA8),
                                         &textureDataLayout, &copySize));

    // Expect texture initialized to be true
    EXPECT_TRUE(dawn_native::IsTextureSubresourceInitialized(texture.Get(), kMipLevel, 1, 0, 1));

    EXPECT_TEXTURE_RGBA8_EQ(data.data(), texture, 0, 0, kMipSize, kMipSize, kMipLevel, 0);
}

// Test WriteTexture to a subset of the texture at mip level, lazy init is necessary to clear the
// other half.
TEST_P(TextureZeroInitTest, WriteTextureHalfAtMipLevel) {
    // TODO(dawn:483): Remove this condition after implementing WriteTexture in those backends.
    DAWN_SKIP_TEST_IF(IsOpenGL() || IsD3D12());

    wgpu::TextureDescriptor descriptor = CreateTextureDescriptor(
        4, 1,
        wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::Sampled | wgpu::TextureUsage::CopySrc,
        kColorFormat);
    wgpu::Texture texture = device.CreateTexture(&descriptor);

    constexpr uint32_t kMipLevel = 2;
    constexpr uint32_t kMipSize = kSize >> kMipLevel;

    wgpu::TextureCopyView textureCopyView =
        utils::CreateTextureCopyView(texture, kMipLevel, {0, 0, 0});
    wgpu::Extent3D copySize = {kMipSize / 2, kMipSize, 1};

    wgpu::TextureDataLayout textureDataLayout;
    textureDataLayout.offset = 0;
    textureDataLayout.bytesPerRow = kMipSize * kFormatBlockByteSize / 2;
    textureDataLayout.rowsPerImage = kMipSize;

    std::vector<RGBA8> data(
        utils::RequiredBytesInCopy(textureDataLayout.bytesPerRow, textureDataLayout.rowsPerImage,
                                   copySize, kColorFormat) /
            sizeof(RGBA8),
        {100, 100, 100, 100});

    EXPECT_LAZY_CLEAR(1u,
                      queue.WriteTexture(&textureCopyView, data.data(), data.size() * sizeof(RGBA8),
                                         &textureDataLayout, &copySize));

    // Expect texture initialized to be true
    EXPECT_EQ(true,
              dawn_native::IsTextureSubresourceInitialized(texture.Get(), kMipLevel, 1, 0, 1));

    std::vector<RGBA8> expectedZeros((kMipSize / 2) * kMipSize, {0, 0, 0, 0});
    // first half filled with 100, by the data
    EXPECT_TEXTURE_RGBA8_EQ(data.data(), texture, 0, 0, kMipSize / 2, kMipSize, kMipLevel, 0);
    // second half should be cleared
    EXPECT_TEXTURE_RGBA8_EQ(expectedZeros.data(), texture, kMipSize / 2, 0, kMipSize / 2, kMipSize,
                            kMipLevel, 0);
}

DAWN_INSTANTIATE_TEST(TextureZeroInitTest,
                      D3D12Backend({"nonzero_clear_resources_on_creation_for_testing"}),
                      D3D12Backend({"nonzero_clear_resources_on_creation_for_testing"},
                                   {"use_d3d12_render_pass"}),
                      OpenGLBackend({"nonzero_clear_resources_on_creation_for_testing"}),
                      MetalBackend({"nonzero_clear_resources_on_creation_for_testing"}),
                      VulkanBackend({"nonzero_clear_resources_on_creation_for_testing"}));
