// Copyright 2017 The Dawn Authors
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

#include <array>
#include "common/Constants.h"
#include "common/Math.h"
#include "utils/TextureFormatUtils.h"
#include "utils/WGPUHelpers.h"

class CopyTests : public DawnTest {
  protected:
    static constexpr wgpu::TextureFormat kTextureFormat = wgpu::TextureFormat::RGBA8Unorm;

    struct TextureSpec {
        wgpu::Origin3D copyOrigin;
        wgpu::Extent3D textureSize;
        uint32_t level;
    };

    struct BufferSpec {
        uint64_t size;
        uint64_t offset;
        uint32_t bytesPerRow;
        uint32_t rowsPerImage;
    };

    static std::vector<RGBA8> GetExpectedTextureData(const utils::TextureDataCopyLayout& layout) {
        std::vector<RGBA8> textureData(layout.texelBlockCount);
        for (uint32_t layer = 0; layer < layout.mipSize.depth; ++layer) {
            const uint32_t texelIndexOffsetPerSlice = layout.texelBlocksPerImage * layer;
            for (uint32_t y = 0; y < layout.mipSize.height; ++y) {
                for (uint32_t x = 0; x < layout.mipSize.width; ++x) {
                    uint32_t i = x + y * layout.texelBlocksPerRow;
                    textureData[texelIndexOffsetPerSlice + i] =
                        RGBA8(static_cast<uint8_t>((x + layer * x) % 256),
                              static_cast<uint8_t>((y + layer * y) % 256),
                              static_cast<uint8_t>(x / 256), static_cast<uint8_t>(y / 256));
                }
            }
        }

        return textureData;
    }

    static BufferSpec MinimumBufferSpec(uint32_t width,
                                        uint32_t rowsPerImage,
                                        uint32_t arrayLayer = 1,
                                        bool testZeroRowsPerImage = true) {
        const uint32_t bytesPerRow = utils::GetMinimumBytesPerRow(kTextureFormat, width);
        const uint32_t totalBufferSize = utils::GetBytesInBufferTextureCopy(
            kTextureFormat, width, bytesPerRow, rowsPerImage, arrayLayer);
        uint32_t appliedRowsPerImage = testZeroRowsPerImage ? 0 : rowsPerImage;
        return {totalBufferSize, 0, bytesPerRow, appliedRowsPerImage};
    }

    static void PackTextureData(const RGBA8* srcData,
                                uint32_t width,
                                uint32_t height,
                                uint32_t srcTexelsPerRow,
                                RGBA8* dstData,
                                uint32_t dstTexelsPerRow) {
        for (unsigned int y = 0; y < height; ++y) {
            for (unsigned int x = 0; x < width; ++x) {
                unsigned int src = x + y * srcTexelsPerRow;
                unsigned int dst = x + y * dstTexelsPerRow;
                dstData[dst] = srcData[src];
            }
        }
    }
};

class CopyTests_T2B : public CopyTests {
  protected:
    void DoTest(const TextureSpec& textureSpec,
                const BufferSpec& bufferSpec,
                const wgpu::Extent3D& copySize) {
        // Create a texture that is `width` x `height` with (`level` + 1) mip levels.
        wgpu::TextureDescriptor descriptor;
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size = textureSpec.textureSize;
        descriptor.sampleCount = 1;
        descriptor.format = kTextureFormat;
        descriptor.mipLevelCount = textureSpec.level + 1;
        descriptor.usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc;
        wgpu::Texture texture = device.CreateTexture(&descriptor);

        const utils::TextureDataCopyLayout copyLayout =
            utils::GetTextureDataCopyLayoutForTexture2DAtLevel(
                kTextureFormat, textureSpec.textureSize, textureSpec.level,
                bufferSpec.rowsPerImage);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

        // Initialize the source texture
        std::vector<RGBA8> textureArrayData = GetExpectedTextureData(copyLayout);
        {
            wgpu::Buffer uploadBuffer = utils::CreateBufferFromData(
                device, textureArrayData.data(), copyLayout.byteLength, wgpu::BufferUsage::CopySrc);
            wgpu::BufferCopyView bufferCopyView = utils::CreateBufferCopyView(
                uploadBuffer, 0, copyLayout.bytesPerRow, bufferSpec.rowsPerImage);
            wgpu::TextureCopyView textureCopyView =
                utils::CreateTextureCopyView(texture, textureSpec.level, {0, 0, 0});
            encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copyLayout.mipSize);
        }

        // Create a buffer of `size` and populate it with empty data (0,0,0,0) Note:
        // Prepopulating the buffer with empty data ensures that there is not random data in the
        // expectation and helps ensure that the padding due to the bytes per row is not modified
        // by the copy.
        // TODO(jiawei.shao@intel.com): remove the initialization of the buffer after we support
        // buffer lazy-initialization.
        const uint32_t bytesPerTexel = utils::GetTexelBlockSizeInBytes(kTextureFormat);
        const std::vector<RGBA8> emptyData(bufferSpec.size / bytesPerTexel, RGBA8());
        wgpu::Buffer buffer =
            utils::CreateBufferFromData(device, emptyData.data(), bufferSpec.size,
                                        wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst);

        {
            wgpu::TextureCopyView textureCopyView =
                utils::CreateTextureCopyView(texture, textureSpec.level, textureSpec.copyOrigin);
            wgpu::BufferCopyView bufferCopyView = utils::CreateBufferCopyView(
                buffer, bufferSpec.offset, bufferSpec.bytesPerRow, bufferSpec.rowsPerImage);
            encoder.CopyTextureToBuffer(&textureCopyView, &bufferCopyView, &copySize);
        }

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        uint64_t bufferOffset = bufferSpec.offset;
        const uint32_t texelCountInCopyRegion =
            bufferSpec.bytesPerRow / bytesPerTexel * (copySize.height - 1) + copySize.width;
        const uint32_t maxArrayLayer = textureSpec.copyOrigin.z + copySize.depth;
        std::vector<RGBA8> expected(texelCountInCopyRegion);
        for (uint32_t slice = textureSpec.copyOrigin.z; slice < maxArrayLayer; ++slice) {
            // Pack the data used to create the upload buffer in the specified copy region to have
            // the same format as the expected buffer data.
            std::fill(expected.begin(), expected.end(), RGBA8());
            const uint32_t texelIndexOffset = copyLayout.texelBlocksPerImage * slice;
            const uint32_t expectedTexelArrayDataStartIndex =
                texelIndexOffset + (textureSpec.copyOrigin.x +
                                    textureSpec.copyOrigin.y * copyLayout.texelBlocksPerRow);

            PackTextureData(&textureArrayData[expectedTexelArrayDataStartIndex], copySize.width,
                            copySize.height, copyLayout.texelBlocksPerRow, expected.data(),
                            bufferSpec.bytesPerRow / bytesPerTexel);

            EXPECT_BUFFER_U32_RANGE_EQ(reinterpret_cast<const uint32_t*>(expected.data()), buffer,
                                       bufferOffset, static_cast<uint32_t>(expected.size()))
                << "Texture to Buffer copy failed copying region [(" << textureSpec.copyOrigin.x
                << ", " << textureSpec.copyOrigin.y << "), ("
                << textureSpec.copyOrigin.x + copySize.width << ", "
                << textureSpec.copyOrigin.y + copySize.height << ")) from "
                << textureSpec.textureSize.width << " x " << textureSpec.textureSize.height
                << " texture at mip level " << textureSpec.level << " layer " << slice << " to "
                << bufferSpec.size << "-byte buffer with offset " << bufferOffset
                << " and bytes per row " << bufferSpec.bytesPerRow << std::endl;

            bufferOffset += copyLayout.bytesPerImage;
        }
    }
};

class CopyTests_B2T : public CopyTests {
  protected:
    static void FillBufferData(RGBA8* data, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            data[i] = RGBA8(static_cast<uint8_t>(i % 256), static_cast<uint8_t>((i / 256) % 256),
                            static_cast<uint8_t>((i / 256 / 256) % 256), 255);
        }
    }

    void DoTest(const TextureSpec& textureSpec,
                const BufferSpec& bufferSpec,
                const wgpu::Extent3D& copySize) {
        // Create a buffer of size `size` and populate it with data
        const uint32_t bytesPerTexel = utils::GetTexelBlockSizeInBytes(kTextureFormat);
        std::vector<RGBA8> bufferData(bufferSpec.size / bytesPerTexel);
        FillBufferData(bufferData.data(), bufferData.size());
        wgpu::Buffer buffer =
            utils::CreateBufferFromData(device, bufferData.data(), bufferSpec.size,
                                        wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst);

        // Create a texture that is `width` x `height` with (`level` + 1) mip levels.
        wgpu::TextureDescriptor descriptor;
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size = textureSpec.textureSize;
        descriptor.sampleCount = 1;
        descriptor.format = kTextureFormat;
        descriptor.mipLevelCount = textureSpec.level + 1;
        descriptor.usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc;
        wgpu::Texture texture = device.CreateTexture(&descriptor);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

        const utils::TextureDataCopyLayout copyLayout =
            utils::GetTextureDataCopyLayoutForTexture2DAtLevel(
                kTextureFormat, textureSpec.textureSize, textureSpec.level,
                bufferSpec.rowsPerImage);

        const uint32_t maxArrayLayer = textureSpec.copyOrigin.z + copySize.depth;

        wgpu::BufferCopyView bufferCopyView = utils::CreateBufferCopyView(
            buffer, bufferSpec.offset, bufferSpec.bytesPerRow, bufferSpec.rowsPerImage);
        wgpu::TextureCopyView textureCopyView =
            utils::CreateTextureCopyView(texture, textureSpec.level, textureSpec.copyOrigin);
        encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copySize);

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        uint64_t bufferOffset = bufferSpec.offset;
        const uint32_t texelCountLastLayer =
            copyLayout.texelBlocksPerRow * (copyLayout.mipSize.height - 1) +
            copyLayout.mipSize.width;
        for (uint32_t slice = textureSpec.copyOrigin.z; slice < maxArrayLayer; ++slice) {
            // Pack the data used to create the buffer in the specified copy region to have the same
            // format as the expected texture data.
            std::vector<RGBA8> expected(texelCountLastLayer);
            PackTextureData(&bufferData[bufferOffset / bytesPerTexel], copySize.width,
                            copySize.height, bufferSpec.bytesPerRow / bytesPerTexel,
                            expected.data(), copySize.width);

            EXPECT_TEXTURE_RGBA8_EQ(expected.data(), texture, textureSpec.copyOrigin.x,
                                    textureSpec.copyOrigin.y, copySize.width, copySize.height,
                                    textureSpec.level, slice)
                << "Buffer to Texture copy failed copying " << bufferSpec.size
                << "-byte buffer with offset " << bufferSpec.offset << " and bytes per row "
                << bufferSpec.bytesPerRow << " to [(" << textureSpec.copyOrigin.x << ", "
                << textureSpec.copyOrigin.y << "), (" << textureSpec.copyOrigin.x + copySize.width
                << ", " << textureSpec.copyOrigin.y + copySize.height << ")) region of "
                << textureSpec.textureSize.width << " x " << textureSpec.textureSize.height
                << " texture at mip level " << textureSpec.level << " layer " << slice << std::endl;
            bufferOffset += copyLayout.bytesPerImage;
        }
    }
};

class CopyTests_T2T : public CopyTests {
  protected:
    void DoTest(const TextureSpec& srcSpec,
                const TextureSpec& dstSpec,
                const wgpu::Extent3D& copySize,
                bool copyWithinSameTexture = false) {
        wgpu::TextureDescriptor srcDescriptor;
        srcDescriptor.dimension = wgpu::TextureDimension::e2D;
        srcDescriptor.size = srcSpec.textureSize;
        srcDescriptor.sampleCount = 1;
        srcDescriptor.format = kTextureFormat;
        srcDescriptor.mipLevelCount = srcSpec.level + 1;
        srcDescriptor.usage = wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst;
        wgpu::Texture srcTexture = device.CreateTexture(&srcDescriptor);

        wgpu::Texture dstTexture;
        if (copyWithinSameTexture) {
            dstTexture = srcTexture;
        } else {
            wgpu::TextureDescriptor dstDescriptor;
            dstDescriptor.dimension = wgpu::TextureDimension::e2D;
            dstDescriptor.size = dstSpec.textureSize;
            dstDescriptor.sampleCount = 1;
            dstDescriptor.format = kTextureFormat;
            dstDescriptor.mipLevelCount = dstSpec.level + 1;
            dstDescriptor.usage = wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst;
            dstTexture = device.CreateTexture(&dstDescriptor);
        }

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

        // Create an upload buffer and use it to populate the current slice of the texture in
        // `level` mip level
        const utils::TextureDataCopyLayout copyLayout =
            utils::GetTextureDataCopyLayoutForTexture2DAtLevel(
                kTextureFormat,
                {srcSpec.textureSize.width, srcSpec.textureSize.height, copySize.depth},
                srcSpec.level, 0);

        const std::vector<RGBA8> textureArrayCopyData = GetExpectedTextureData(copyLayout);

        wgpu::Buffer uploadBuffer = utils::CreateBufferFromData(
            device, textureArrayCopyData.data(), copyLayout.byteLength, wgpu::BufferUsage::CopySrc);
        wgpu::BufferCopyView bufferCopyView =
            utils::CreateBufferCopyView(uploadBuffer, 0, copyLayout.bytesPerRow, 0);
        wgpu::TextureCopyView textureCopyView =
            utils::CreateTextureCopyView(srcTexture, srcSpec.level, {0, 0, srcSpec.copyOrigin.z});
        encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copyLayout.mipSize);

        // Perform the texture to texture copy
        wgpu::TextureCopyView srcTextureCopyView =
            utils::CreateTextureCopyView(srcTexture, srcSpec.level, srcSpec.copyOrigin);
        wgpu::TextureCopyView dstTextureCopyView =
            utils::CreateTextureCopyView(dstTexture, dstSpec.level, dstSpec.copyOrigin);
        encoder.CopyTextureToTexture(&srcTextureCopyView, &dstTextureCopyView, &copySize);

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        const uint32_t texelCountInCopyRegion =
            copyLayout.texelBlocksPerRow * (copySize.height - 1) + copySize.width;
        std::vector<RGBA8> expected(texelCountInCopyRegion);
        for (uint32_t slice = 0; slice < copySize.depth; ++slice) {
            std::fill(expected.begin(), expected.end(), RGBA8());
            const uint32_t texelIndexOffset = copyLayout.texelBlocksPerImage * slice;
            const uint32_t expectedTexelArrayDataStartIndex =
                texelIndexOffset +
                (srcSpec.copyOrigin.x + srcSpec.copyOrigin.y * copyLayout.texelBlocksPerRow);
            PackTextureData(&textureArrayCopyData[expectedTexelArrayDataStartIndex], copySize.width,
                            copySize.height, copyLayout.texelBlocksPerRow, expected.data(),
                            copySize.width);

            EXPECT_TEXTURE_RGBA8_EQ(expected.data(), dstTexture, dstSpec.copyOrigin.x,
                                    dstSpec.copyOrigin.y, copySize.width, copySize.height,
                                    dstSpec.level, dstSpec.copyOrigin.z + slice)
                << "Texture to Texture copy failed copying region [(" << srcSpec.copyOrigin.x
                << ", " << srcSpec.copyOrigin.y << "), (" << srcSpec.copyOrigin.x + copySize.width
                << ", " << srcSpec.copyOrigin.y + copySize.height << ")) from "
                << srcSpec.textureSize.width << " x " << srcSpec.textureSize.height
                << " texture at mip level " << srcSpec.level << " layer "
                << srcSpec.copyOrigin.z + slice << " to [(" << dstSpec.copyOrigin.x << ", "
                << dstSpec.copyOrigin.y << "), (" << dstSpec.copyOrigin.x + copySize.width << ", "
                << dstSpec.copyOrigin.y + copySize.height << ")) region of "
                << dstSpec.textureSize.width << " x " << dstSpec.textureSize.height
                << " texture at mip level " << dstSpec.level << " layer "
                << dstSpec.copyOrigin.z + slice << std::endl;
        }
    }
};

class CopyTests_B2B : public DawnTest {
  protected:
    // This is the same signature as CopyBufferToBuffer except that the buffers are replaced by
    // only their size.
    void DoTest(uint64_t sourceSize,
                uint64_t sourceOffset,
                uint64_t destinationSize,
                uint64_t destinationOffset,
                uint64_t copySize) {
        ASSERT(sourceSize % 4 == 0);
        ASSERT(destinationSize % 4 == 0);

        // Create our two test buffers, destination filled with zeros, source filled with non-zeroes
        std::vector<uint32_t> zeroes(static_cast<size_t>(destinationSize / sizeof(uint32_t)));
        wgpu::Buffer destination =
            utils::CreateBufferFromData(device, zeroes.data(), zeroes.size() * sizeof(uint32_t),
                                        wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc);

        std::vector<uint32_t> sourceData(static_cast<size_t>(sourceSize / sizeof(uint32_t)));
        for (size_t i = 0; i < sourceData.size(); i++) {
            sourceData[i] = i + 1;
        }
        wgpu::Buffer source = utils::CreateBufferFromData(device, sourceData.data(),
                                                          sourceData.size() * sizeof(uint32_t),
                                                          wgpu::BufferUsage::CopySrc);

        // Submit the copy
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(source, sourceOffset, destination, destinationOffset, copySize);
        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        // Check destination is exactly the expected content.
        EXPECT_BUFFER_U32_RANGE_EQ(zeroes.data(), destination, 0,
                                   destinationOffset / sizeof(uint32_t));
        EXPECT_BUFFER_U32_RANGE_EQ(sourceData.data() + sourceOffset / sizeof(uint32_t), destination,
                                   destinationOffset, copySize / sizeof(uint32_t));
        uint64_t copyEnd = destinationOffset + copySize;
        EXPECT_BUFFER_U32_RANGE_EQ(zeroes.data(), destination, copyEnd,
                                   (destinationSize - copyEnd) / sizeof(uint32_t));
    }
};

// Test that copying an entire texture with 256-byte aligned dimensions works
TEST_P(CopyTests_T2B, FullTextureAligned) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;

    TextureSpec textureSpec;
    textureSpec.textureSize = {kWidth, kHeight, 1};
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.level = 0;

    DoTest(textureSpec, MinimumBufferSpec(kWidth, kHeight), {kWidth, kHeight, 1});
}

// Test that copying an entire texture without 256-byte aligned dimensions works
TEST_P(CopyTests_T2B, FullTextureUnaligned) {
    constexpr uint32_t kWidth = 259;
    constexpr uint32_t kHeight = 127;

    TextureSpec textureSpec;
    textureSpec.textureSize = {kWidth, kHeight, 1};
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.level = 0;

    DoTest(textureSpec, MinimumBufferSpec(kWidth, kHeight), {kWidth, kHeight, 1});
}

// Test that reading pixels from a 256-byte aligned texture works
TEST_P(CopyTests_T2B, PixelReadAligned) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;
    BufferSpec pixelBuffer = MinimumBufferSpec(1, 1);

    constexpr wgpu::Extent3D kCopySize = {1, 1, 1};
    constexpr wgpu::Extent3D kTextureSize = {kWidth, kHeight, 1};
    TextureSpec defaultTextureSpec;
    defaultTextureSpec.textureSize = kTextureSize;
    defaultTextureSpec.level = 0;

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {0, 0, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {kWidth - 1, 0, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {0, kHeight - 1, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {kWidth - 1, kHeight - 1, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {kWidth / 3, kHeight / 7, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {kWidth / 7, kHeight / 3, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }
}

// Test that copying pixels from a texture that is not 256-byte aligned works
TEST_P(CopyTests_T2B, PixelReadUnaligned) {
    constexpr uint32_t kWidth = 259;
    constexpr uint32_t kHeight = 127;
    BufferSpec pixelBuffer = MinimumBufferSpec(1, 1);

    constexpr wgpu::Extent3D kCopySize = {1, 1, 1};
    constexpr wgpu::Extent3D kTextureSize = {kWidth, kHeight, 1};
    TextureSpec defaultTextureSpec;
    defaultTextureSpec.textureSize = kTextureSize;
    defaultTextureSpec.level = 0;

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {0, 0, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {kWidth - 1, 0, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {0, kHeight - 1, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {kWidth - 1, kHeight - 1, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {kWidth / 3, kHeight / 7, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {kWidth / 7, kHeight / 3, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }
}

// Test that copying regions with 256-byte aligned sizes works
TEST_P(CopyTests_T2B, TextureRegionAligned) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;
    for (unsigned int w : {64, 128, 256}) {
        for (unsigned int h : {16, 32, 48}) {
            TextureSpec textureSpec;
            textureSpec.copyOrigin = {0, 0, 0};
            textureSpec.level = 0;
            textureSpec.textureSize = {kWidth, kHeight, 1};
            DoTest(textureSpec, MinimumBufferSpec(w, h), {w, h, 1});
        }
    }
}

// Test that copying regions without 256-byte aligned sizes works
TEST_P(CopyTests_T2B, TextureRegionUnaligned) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;

    TextureSpec defaultTextureSpec;
    defaultTextureSpec.copyOrigin = {0, 0, 0};
    defaultTextureSpec.level = 0;
    defaultTextureSpec.textureSize = {kWidth, kHeight, 1};

    for (unsigned int w : {13, 63, 65}) {
        for (unsigned int h : {17, 19, 63}) {
            TextureSpec textureSpec = defaultTextureSpec;
            DoTest(textureSpec, MinimumBufferSpec(w, h), {w, h, 1});
        }
    }
}

// Test that copying mips with 256-byte aligned sizes works
TEST_P(CopyTests_T2B, TextureMipAligned) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;

    TextureSpec defaultTextureSpec;
    defaultTextureSpec.copyOrigin = {0, 0, 0};
    defaultTextureSpec.textureSize = {kWidth, kHeight, 1};

    for (unsigned int i = 1; i < 4; ++i) {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.level = i;
        DoTest(textureSpec, MinimumBufferSpec(kWidth >> i, kHeight >> i),
               {kWidth >> i, kHeight >> i, 1});
    }
}

// Test that copying mips without 256-byte aligned sizes works
TEST_P(CopyTests_T2B, TextureMipUnaligned) {
    constexpr uint32_t kWidth = 259;
    constexpr uint32_t kHeight = 127;

    TextureSpec defaultTextureSpec;
    defaultTextureSpec.copyOrigin = {0, 0, 0};
    defaultTextureSpec.textureSize = {kWidth, kHeight, 1};

    for (unsigned int i = 1; i < 4; ++i) {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.level = i;
        DoTest(textureSpec, MinimumBufferSpec(kWidth >> i, kHeight >> i),
               {kWidth >> i, kHeight >> i, 1});
    }
}

// Test that copying with a 512-byte aligned buffer offset works
TEST_P(CopyTests_T2B, OffsetBufferAligned) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.textureSize = {kWidth, kHeight, 1};
    textureSpec.level = 0;

    for (unsigned int i = 0; i < 3; ++i) {
        BufferSpec bufferSpec = MinimumBufferSpec(kWidth, kHeight);
        uint64_t offset = 512 * i;
        bufferSpec.size += offset;
        bufferSpec.offset += offset;
        DoTest(textureSpec, bufferSpec, {kWidth, kHeight, 1});
    }
}

// Test that copying without a 512-byte aligned buffer offset works
TEST_P(CopyTests_T2B, OffsetBufferUnaligned) {
    constexpr uint32_t kWidth = 128;
    constexpr uint32_t kHeight = 128;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.textureSize = {kWidth, kHeight, 1};
    textureSpec.level = 0;

    const uint32_t bytesPerTexel = utils::GetTexelBlockSizeInBytes(kTextureFormat);
    for (uint32_t i = bytesPerTexel; i < 512; i += bytesPerTexel * 9) {
        BufferSpec bufferSpec = MinimumBufferSpec(kWidth, kHeight);
        bufferSpec.size += i;
        bufferSpec.offset += i;
        DoTest(textureSpec, bufferSpec, {kWidth, kHeight, 1});
    }
}

// Test that copying without a 512-byte aligned buffer offset that is greater than the bytes per row
// works
TEST_P(CopyTests_T2B, OffsetBufferUnalignedSmallRowPitch) {
    constexpr uint32_t kWidth = 32;
    constexpr uint32_t kHeight = 128;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.textureSize = {kWidth, kHeight, 1};
    textureSpec.level = 0;

    const uint32_t bytesPerTexel = utils::GetTexelBlockSizeInBytes(kTextureFormat);
    for (uint32_t i = 256 + bytesPerTexel; i < 512; i += bytesPerTexel * 9) {
        BufferSpec bufferSpec = MinimumBufferSpec(kWidth, kHeight);
        bufferSpec.size += i;
        bufferSpec.offset += i;
        DoTest(textureSpec, bufferSpec, {kWidth, kHeight, 1});
    }
}

// Test that copying with a greater bytes per row than needed on a 256-byte aligned texture works
TEST_P(CopyTests_T2B, RowPitchAligned) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.textureSize = {kWidth, kHeight, 1};
    textureSpec.level = 0;

    BufferSpec bufferSpec = MinimumBufferSpec(kWidth, kHeight);
    for (unsigned int i = 1; i < 4; ++i) {
        bufferSpec.bytesPerRow += 256;
        bufferSpec.size += 256 * kHeight;
        DoTest(textureSpec, bufferSpec, {kWidth, kHeight, 1});
    }
}

// Test that copying with a greater bytes per row than needed on a texture that is not 256-byte
// aligned works
TEST_P(CopyTests_T2B, RowPitchUnaligned) {
    constexpr uint32_t kWidth = 259;
    constexpr uint32_t kHeight = 127;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.textureSize = {kWidth, kHeight, 1};
    textureSpec.level = 0;

    BufferSpec bufferSpec = MinimumBufferSpec(kWidth, kHeight);
    for (unsigned int i = 1; i < 4; ++i) {
        bufferSpec.bytesPerRow += 256;
        bufferSpec.size += 256 * kHeight;
        DoTest(textureSpec, bufferSpec, {kWidth, kHeight, 1});
    }
}

// Test that copying whole texture 2D array layers in one texture-to-buffer-copy works.
TEST_P(CopyTests_T2B, Texture2DArrayRegion) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;
    constexpr uint32_t kLayers = 6u;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.textureSize = {kWidth, kHeight, kLayers};
    textureSpec.level = 0;

    DoTest(textureSpec, MinimumBufferSpec(kWidth, kHeight, kLayers), {kWidth, kHeight, kLayers});
}

// Test that copying a range of texture 2D array layers in one texture-to-buffer-copy works.
TEST_P(CopyTests_T2B, Texture2DArraySubRegion) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;
    constexpr uint32_t kLayers = 6u;
    constexpr uint32_t kBaseLayer = 2u;
    constexpr uint32_t kCopyLayers = 3u;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, kBaseLayer};
    textureSpec.textureSize = {kWidth, kHeight, kLayers};
    textureSpec.level = 0;

    DoTest(textureSpec, MinimumBufferSpec(kWidth, kHeight, kCopyLayers),
           {kWidth, kHeight, kCopyLayers});
}

// Test that copying texture 2D array mips with 256-byte aligned sizes works
TEST_P(CopyTests_T2B, Texture2DArrayMip) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;
    constexpr uint32_t kLayers = 6u;

    TextureSpec defaultTextureSpec;
    defaultTextureSpec.copyOrigin = {0, 0, 0};
    defaultTextureSpec.textureSize = {kWidth, kHeight, kLayers};

    for (unsigned int i = 1; i < 4; ++i) {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.level = i;

        DoTest(textureSpec, MinimumBufferSpec(kWidth >> i, kHeight >> i, kLayers),
               {kWidth >> i, kHeight >> i, kLayers});
    }
}

// Test that copying from a range of texture 2D array layers in one texture-to-buffer-copy when
// RowsPerImage is not equal to the height of the texture works.
TEST_P(CopyTests_T2B, Texture2DArrayRegionNonzeroRowsPerImage) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;
    constexpr uint32_t kLayers = 6u;
    constexpr uint32_t kBaseLayer = 2u;
    constexpr uint32_t kCopyLayers = 3u;

    constexpr uint32_t kRowsPerImage = kHeight * 2;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, kBaseLayer};
    textureSpec.textureSize = {kWidth, kHeight, kLayers};
    textureSpec.level = 0;

    BufferSpec bufferSpec = MinimumBufferSpec(kWidth, kRowsPerImage, kCopyLayers, false);
    bufferSpec.rowsPerImage = kRowsPerImage;
    DoTest(textureSpec, bufferSpec, {kWidth, kHeight, kCopyLayers});
}

// Test a special code path in the D3D12 backends when (BytesPerRow * RowsPerImage) is not a
// multiple of 512.
TEST_P(CopyTests_T2B, Texture2DArrayRegionWithOffsetOddRowsPerImage) {
    constexpr uint32_t kWidth = 64;
    constexpr uint32_t kHeight = 128;
    constexpr uint32_t kLayers = 8u;
    constexpr uint32_t kBaseLayer = 2u;
    constexpr uint32_t kCopyLayers = 5u;

    constexpr uint32_t kRowsPerImage = kHeight + 1;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, kBaseLayer};
    textureSpec.textureSize = {kWidth, kHeight, kLayers};
    textureSpec.level = 0;

    BufferSpec bufferSpec = MinimumBufferSpec(kWidth, kRowsPerImage, kCopyLayers, false);
    bufferSpec.offset += 128u;
    bufferSpec.size += 128u;
    bufferSpec.rowsPerImage = kRowsPerImage;
    DoTest(textureSpec, bufferSpec, {kWidth, kHeight, kCopyLayers});
}

// Test a special code path in the D3D12 backends when (BytesPerRow * RowsPerImage) is a multiple
// of 512.
TEST_P(CopyTests_T2B, Texture2DArrayRegionWithOffsetEvenRowsPerImage) {
    constexpr uint32_t kWidth = 64;
    constexpr uint32_t kHeight = 128;
    constexpr uint32_t kLayers = 8u;
    constexpr uint32_t kBaseLayer = 2u;
    constexpr uint32_t kCopyLayers = 4u;

    constexpr uint32_t kRowsPerImage = kHeight + 2;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, kBaseLayer};
    textureSpec.textureSize = {kWidth, kHeight, kLayers};
    textureSpec.level = 0;

    BufferSpec bufferSpec = MinimumBufferSpec(kWidth, kRowsPerImage, kCopyLayers, false);
    bufferSpec.offset += 128u;
    bufferSpec.size += 128u;
    bufferSpec.rowsPerImage = kRowsPerImage;
    DoTest(textureSpec, bufferSpec, {kWidth, kHeight, kCopyLayers});
}

DAWN_INSTANTIATE_TEST(CopyTests_T2B,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());

// Test that copying an entire texture with 256-byte aligned dimensions works
TEST_P(CopyTests_B2T, FullTextureAligned) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;

    TextureSpec textureSpec;
    textureSpec.textureSize = {kWidth, kHeight, 1};
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.level = 0;

    DoTest(textureSpec, MinimumBufferSpec(kWidth, kHeight), {kWidth, kHeight, 1});
}

// Test that copying an entire texture without 256-byte aligned dimensions works
TEST_P(CopyTests_B2T, FullTextureUnaligned) {
    constexpr uint32_t kWidth = 259;
    constexpr uint32_t kHeight = 127;

    TextureSpec textureSpec;
    textureSpec.textureSize = {kWidth, kHeight, 1};
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.level = 0;

    DoTest(textureSpec, MinimumBufferSpec(kWidth, kHeight), {kWidth, kHeight, 1});
}

// Test that reading pixels from a 256-byte aligned texture works
TEST_P(CopyTests_B2T, PixelReadAligned) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;
    BufferSpec pixelBuffer = MinimumBufferSpec(1, 1);

    constexpr wgpu::Extent3D kCopySize = {1, 1, 1};
    constexpr wgpu::Extent3D kTextureSize = {kWidth, kHeight, 1};
    TextureSpec defaultTextureSpec;
    defaultTextureSpec.textureSize = kTextureSize;
    defaultTextureSpec.level = 0;

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {0, 0, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {kWidth - 1, 0, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {0, kHeight - 1, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {kWidth - 1, kHeight - 1, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {kWidth / 3, kHeight / 7, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {kWidth / 7, kHeight / 3, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }
}

// Test that copying pixels from a texture that is not 256-byte aligned works
TEST_P(CopyTests_B2T, PixelReadUnaligned) {
    constexpr uint32_t kWidth = 259;
    constexpr uint32_t kHeight = 127;
    BufferSpec pixelBuffer = MinimumBufferSpec(1, 1);

    constexpr wgpu::Extent3D kCopySize = {1, 1, 1};
    constexpr wgpu::Extent3D kTextureSize = {kWidth, kHeight, 1};
    TextureSpec defaultTextureSpec;
    defaultTextureSpec.textureSize = kTextureSize;
    defaultTextureSpec.level = 0;

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {0, 0, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {kWidth - 1, 0, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {0, kHeight - 1, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {kWidth - 1, kHeight - 1, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {kWidth / 3, kHeight / 7, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }

    {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.copyOrigin = {kWidth / 7, kHeight / 3, 0};
        DoTest(textureSpec, pixelBuffer, kCopySize);
    }
}

// Test that copying regions with 256-byte aligned sizes works
TEST_P(CopyTests_B2T, TextureRegionAligned) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;
    for (unsigned int w : {64, 128, 256}) {
        for (unsigned int h : {16, 32, 48}) {
            TextureSpec textureSpec;
            textureSpec.copyOrigin = {0, 0, 0};
            textureSpec.level = 0;
            textureSpec.textureSize = {kWidth, kHeight, 1};
            DoTest(textureSpec, MinimumBufferSpec(w, h), {w, h, 1});
        }
    }
}

// Test that copying regions without 256-byte aligned sizes works
TEST_P(CopyTests_B2T, TextureRegionUnaligned) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;

    TextureSpec defaultTextureSpec;
    defaultTextureSpec.copyOrigin = {0, 0, 0};
    defaultTextureSpec.level = 0;
    defaultTextureSpec.textureSize = {kWidth, kHeight, 1};

    for (unsigned int w : {13, 63, 65}) {
        for (unsigned int h : {17, 19, 63}) {
            TextureSpec textureSpec = defaultTextureSpec;
            DoTest(textureSpec, MinimumBufferSpec(w, h), {w, h, 1});
        }
    }
}

// Test that copying mips with 256-byte aligned sizes works
TEST_P(CopyTests_B2T, TextureMipAligned) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;

    TextureSpec defaultTextureSpec;
    defaultTextureSpec.copyOrigin = {0, 0, 0};
    defaultTextureSpec.textureSize = {kWidth, kHeight, 1};

    for (unsigned int i = 1; i < 4; ++i) {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.level = i;
        DoTest(textureSpec, MinimumBufferSpec(kWidth >> i, kHeight >> i),
               {kWidth >> i, kHeight >> i, 1});
    }
}

// Test that copying mips without 256-byte aligned sizes works
TEST_P(CopyTests_B2T, TextureMipUnaligned) {
    constexpr uint32_t kWidth = 259;
    constexpr uint32_t kHeight = 127;

    TextureSpec defaultTextureSpec;
    defaultTextureSpec.copyOrigin = {0, 0, 0};
    defaultTextureSpec.textureSize = {kWidth, kHeight, 1};

    for (unsigned int i = 1; i < 4; ++i) {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.level = i;
        DoTest(textureSpec, MinimumBufferSpec(kWidth >> i, kHeight >> i),
               {kWidth >> i, kHeight >> i, 1});
    }
}

// Test that copying with a 512-byte aligned buffer offset works
TEST_P(CopyTests_B2T, OffsetBufferAligned) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.textureSize = {kWidth, kHeight, 1};
    textureSpec.level = 0;

    for (unsigned int i = 0; i < 3; ++i) {
        BufferSpec bufferSpec = MinimumBufferSpec(kWidth, kHeight);
        uint64_t offset = 512 * i;
        bufferSpec.size += offset;
        bufferSpec.offset += offset;
        DoTest(textureSpec, bufferSpec, {kWidth, kHeight, 1});
    }
}

// Test that copying without a 512-byte aligned buffer offset works
TEST_P(CopyTests_B2T, OffsetBufferUnaligned) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.textureSize = {kWidth, kHeight, 1};
    textureSpec.level = 0;

    const uint32_t bytesPerTexel = utils::GetTexelBlockSizeInBytes(kTextureFormat);
    for (uint32_t i = bytesPerTexel; i < 512; i += bytesPerTexel * 9) {
        BufferSpec bufferSpec = MinimumBufferSpec(kWidth, kHeight);
        bufferSpec.size += i;
        bufferSpec.offset += i;
        DoTest(textureSpec, bufferSpec, {kWidth, kHeight, 1});
    }
}

// Test that copying without a 512-byte aligned buffer offset that is greater than the bytes per row
// works
TEST_P(CopyTests_B2T, OffsetBufferUnalignedSmallRowPitch) {
    constexpr uint32_t kWidth = 32;
    constexpr uint32_t kHeight = 128;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.textureSize = {kWidth, kHeight, 1};
    textureSpec.level = 0;

    const uint32_t bytesPerTexel = utils::GetTexelBlockSizeInBytes(kTextureFormat);
    for (uint32_t i = 256 + bytesPerTexel; i < 512; i += bytesPerTexel * 9) {
        BufferSpec bufferSpec = MinimumBufferSpec(kWidth, kHeight);
        bufferSpec.size += i;
        bufferSpec.offset += i;
        DoTest(textureSpec, bufferSpec, {kWidth, kHeight, 1});
    }
}

// Test that copying with a greater bytes per row than needed on a 256-byte aligned texture works
TEST_P(CopyTests_B2T, RowPitchAligned) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.textureSize = {kWidth, kHeight, 1};
    textureSpec.level = 0;

    BufferSpec bufferSpec = MinimumBufferSpec(kWidth, kHeight);
    for (unsigned int i = 1; i < 4; ++i) {
        bufferSpec.bytesPerRow += 256;
        bufferSpec.size += 256 * kHeight;
        DoTest(textureSpec, bufferSpec, {kWidth, kHeight, 1});
    }
}

// Test that copying with a greater bytes per row than needed on a texture that is not 256-byte
// aligned works
TEST_P(CopyTests_B2T, RowPitchUnaligned) {
    constexpr uint32_t kWidth = 259;
    constexpr uint32_t kHeight = 127;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.textureSize = {kWidth, kHeight, 1};
    textureSpec.level = 0;

    BufferSpec bufferSpec = MinimumBufferSpec(kWidth, kHeight);
    for (unsigned int i = 1; i < 4; ++i) {
        bufferSpec.bytesPerRow += 256;
        bufferSpec.size += 256 * kHeight;
        DoTest(textureSpec, bufferSpec, {kWidth, kHeight, 1});
    }
}

// Test that copying whole texture 2D array layers in one texture-to-buffer-copy works.
TEST_P(CopyTests_B2T, Texture2DArrayRegion) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;
    constexpr uint32_t kLayers = 6u;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.textureSize = {kWidth, kHeight, kLayers};
    textureSpec.level = 0;

    DoTest(textureSpec, MinimumBufferSpec(kWidth, kHeight, kLayers), {kWidth, kHeight, kLayers});
}

// Test that copying a range of texture 2D array layers in one texture-to-buffer-copy works.
TEST_P(CopyTests_B2T, Texture2DArraySubRegion) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;
    constexpr uint32_t kLayers = 6u;
    constexpr uint32_t kBaseLayer = 2u;
    constexpr uint32_t kCopyLayers = 3u;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, kBaseLayer};
    textureSpec.textureSize = {kWidth, kHeight, kLayers};
    textureSpec.level = 0;

    DoTest(textureSpec, MinimumBufferSpec(kWidth, kHeight, kCopyLayers),
           {kWidth, kHeight, kCopyLayers});
}

// Test that copying into a range of texture 2D array layers in one texture-to-buffer-copy when
// RowsPerImage is not equal to the height of the texture works.
TEST_P(CopyTests_B2T, Texture2DArrayRegionNonzeroRowsPerImage) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;
    constexpr uint32_t kLayers = 6u;
    constexpr uint32_t kBaseLayer = 2u;
    constexpr uint32_t kCopyLayers = 3u;

    constexpr uint32_t kRowsPerImage = kHeight * 2;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, kBaseLayer};
    textureSpec.textureSize = {kWidth, kHeight, kLayers};
    textureSpec.level = 0;

    BufferSpec bufferSpec = MinimumBufferSpec(kWidth, kRowsPerImage, kCopyLayers, false);
    bufferSpec.rowsPerImage = kRowsPerImage;
    DoTest(textureSpec, bufferSpec, {kWidth, kHeight, kCopyLayers});
}

// Test a special code path in the D3D12 backends when (BytesPerRow * RowsPerImage) is not a
// multiple of 512.
TEST_P(CopyTests_B2T, Texture2DArrayRegionWithOffsetOddRowsPerImage) {
    constexpr uint32_t kWidth = 64;
    constexpr uint32_t kHeight = 128;
    constexpr uint32_t kLayers = 8u;
    constexpr uint32_t kBaseLayer = 2u;
    constexpr uint32_t kCopyLayers = 5u;

    constexpr uint32_t kRowsPerImage = kHeight + 1;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, kBaseLayer};
    textureSpec.textureSize = {kWidth, kHeight, kLayers};
    textureSpec.level = 0;

    BufferSpec bufferSpec = MinimumBufferSpec(kWidth, kRowsPerImage, kCopyLayers, false);
    bufferSpec.offset += 128u;
    bufferSpec.size += 128u;
    bufferSpec.rowsPerImage = kRowsPerImage;
    DoTest(textureSpec, bufferSpec, {kWidth, kHeight, kCopyLayers});
}

// Test a special code path in the D3D12 backends when (BytesPerRow * RowsPerImage) is a multiple
// of 512.
TEST_P(CopyTests_B2T, Texture2DArrayRegionWithOffsetEvenRowsPerImage) {
    constexpr uint32_t kWidth = 64;
    constexpr uint32_t kHeight = 128;
    constexpr uint32_t kLayers = 8u;
    constexpr uint32_t kBaseLayer = 2u;
    constexpr uint32_t kCopyLayers = 5u;

    constexpr uint32_t kRowsPerImage = kHeight + 2;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, kBaseLayer};
    textureSpec.textureSize = {kWidth, kHeight, kLayers};
    textureSpec.level = 0;

    BufferSpec bufferSpec = MinimumBufferSpec(kWidth, kRowsPerImage, kCopyLayers, false);
    bufferSpec.offset += 128u;
    bufferSpec.size += 128u;
    bufferSpec.rowsPerImage = kRowsPerImage;
    DoTest(textureSpec, bufferSpec, {kWidth, kHeight, kCopyLayers});
}

DAWN_INSTANTIATE_TEST(CopyTests_B2T,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());

TEST_P(CopyTests_T2T, Texture) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.level = 0;
    textureSpec.textureSize = {kWidth, kHeight, 1};
    DoTest(textureSpec, textureSpec, {kWidth, kHeight, 1});
}

TEST_P(CopyTests_T2T, TextureRegion) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;

    TextureSpec defaultTextureSpec;
    defaultTextureSpec.copyOrigin = {0, 0, 0};
    defaultTextureSpec.level = 0;
    defaultTextureSpec.textureSize = {kWidth, kHeight, 1};

    for (unsigned int w : {64, 128, 256}) {
        for (unsigned int h : {16, 32, 48}) {
            TextureSpec textureSpec = defaultTextureSpec;
            DoTest(textureSpec, textureSpec, {w, h, 1});
        }
    }
}

// Test copying the whole 2D array texture.
TEST_P(CopyTests_T2T, Texture2DArray) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;
    constexpr uint32_t kLayers = 6u;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.level = 0;
    textureSpec.textureSize = {kWidth, kHeight, kLayers};

    DoTest(textureSpec, textureSpec, {kWidth, kHeight, kLayers});
}

// Test copying a subresource region of the 2D array texture.
TEST_P(CopyTests_T2T, Texture2DArrayRegion) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;
    constexpr uint32_t kLayers = 6u;

    TextureSpec defaultTextureSpec;
    defaultTextureSpec.copyOrigin = {0, 0, 0};
    defaultTextureSpec.level = 0;
    defaultTextureSpec.textureSize = {kWidth, kHeight, kLayers};

    for (unsigned int w : {64, 128, 256}) {
        for (unsigned int h : {16, 32, 48}) {
            TextureSpec textureSpec = defaultTextureSpec;
            DoTest(textureSpec, textureSpec, {w, h, kLayers});
        }
    }
}

// Test copying one slice of a 2D array texture.
TEST_P(CopyTests_T2T, Texture2DArrayCopyOneSlice) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;
    constexpr uint32_t kLayers = 6u;
    constexpr uint32_t kSrcBaseLayer = 1u;
    constexpr uint32_t kDstBaseLayer = 3u;
    constexpr uint32_t kCopyArrayLayerCount = 1u;

    TextureSpec defaultTextureSpec;
    defaultTextureSpec.textureSize = {kWidth, kHeight, kLayers};
    defaultTextureSpec.level = 0;

    TextureSpec srcTextureSpec = defaultTextureSpec;
    srcTextureSpec.copyOrigin = {0, 0, kSrcBaseLayer};

    TextureSpec dstTextureSpec = defaultTextureSpec;
    dstTextureSpec.copyOrigin = {0, 0, kDstBaseLayer};

    DoTest(srcTextureSpec, dstTextureSpec, {kWidth, kHeight, kCopyArrayLayerCount});
}

// Test copying multiple contiguous slices of a 2D array texture.
TEST_P(CopyTests_T2T, Texture2DArrayCopyMultipleSlices) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;
    constexpr uint32_t kLayers = 6u;
    constexpr uint32_t kSrcBaseLayer = 0u;
    constexpr uint32_t kDstBaseLayer = 3u;
    constexpr uint32_t kCopyArrayLayerCount = 3u;

    TextureSpec defaultTextureSpec;
    defaultTextureSpec.textureSize = {kWidth, kHeight, kLayers};
    defaultTextureSpec.level = 0;

    TextureSpec srcTextureSpec = defaultTextureSpec;
    srcTextureSpec.copyOrigin = {0, 0, kSrcBaseLayer};

    TextureSpec dstTextureSpec = defaultTextureSpec;
    dstTextureSpec.copyOrigin = {0, 0, kDstBaseLayer};

    DoTest(srcTextureSpec, dstTextureSpec, {kWidth, kHeight, kCopyArrayLayerCount});
}

// Test copying one texture slice within the same texture.
TEST_P(CopyTests_T2T, CopyWithinSameTextureOneSlice) {
    constexpr uint32_t kWidth = 256u;
    constexpr uint32_t kHeight = 128u;
    constexpr uint32_t kLayers = 6u;
    constexpr uint32_t kSrcBaseLayer = 0u;
    constexpr uint32_t kDstBaseLayer = 3u;
    constexpr uint32_t kCopyArrayLayerCount = 1u;

    TextureSpec defaultTextureSpec;
    defaultTextureSpec.textureSize = {kWidth, kHeight, kLayers};
    defaultTextureSpec.level = 0;

    TextureSpec srcTextureSpec = defaultTextureSpec;
    srcTextureSpec.copyOrigin = {0, 0, kSrcBaseLayer};

    TextureSpec dstTextureSpec = defaultTextureSpec;
    dstTextureSpec.copyOrigin = {0, 0, kDstBaseLayer};

    DoTest(srcTextureSpec, dstTextureSpec, {kWidth, kHeight, kCopyArrayLayerCount}, true);
}

// Test copying multiple contiguous texture slices within the same texture with non-overlapped
// slices.
TEST_P(CopyTests_T2T, CopyWithinSameTextureNonOverlappedSlices) {
    constexpr uint32_t kWidth = 256u;
    constexpr uint32_t kHeight = 128u;
    constexpr uint32_t kLayers = 6u;
    constexpr uint32_t kSrcBaseLayer = 0u;
    constexpr uint32_t kDstBaseLayer = 3u;
    constexpr uint32_t kCopyArrayLayerCount = 3u;

    TextureSpec defaultTextureSpec;
    defaultTextureSpec.textureSize = {kWidth, kHeight, kLayers};
    defaultTextureSpec.level = 0;

    TextureSpec srcTextureSpec = defaultTextureSpec;
    srcTextureSpec.copyOrigin = {0, 0, kSrcBaseLayer};

    TextureSpec dstTextureSpec = defaultTextureSpec;
    dstTextureSpec.copyOrigin = {0, 0, kDstBaseLayer};

    DoTest(srcTextureSpec, dstTextureSpec, {kWidth, kHeight, kCopyArrayLayerCount}, true);
}

TEST_P(CopyTests_T2T, TextureMip) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;

    TextureSpec defaultTextureSpec;
    defaultTextureSpec.copyOrigin = {0, 0, 0};
    defaultTextureSpec.textureSize = {kWidth, kHeight, 1};

    for (unsigned int i = 1; i < 4; ++i) {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.level = i;

        DoTest(textureSpec, textureSpec, {kWidth >> i, kHeight >> i, 1});
    }
}

TEST_P(CopyTests_T2T, SingleMipSrcMultipleMipDst) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;

    TextureSpec defaultTextureSpec;
    defaultTextureSpec.copyOrigin = {0, 0, 0};

    for (unsigned int i = 1; i < 4; ++i) {
        TextureSpec srcTextureSpec = defaultTextureSpec;
        srcTextureSpec.textureSize = {kWidth >> i, kHeight >> i, 1};
        srcTextureSpec.level = 0;

        TextureSpec dstTextureSpec = defaultTextureSpec;
        dstTextureSpec.textureSize = {kWidth, kHeight, 1};
        dstTextureSpec.level = i;

        DoTest(srcTextureSpec, dstTextureSpec, {kWidth >> i, kHeight >> i, 1});
    }
}

TEST_P(CopyTests_T2T, MultipleMipSrcSingleMipDst) {
    constexpr uint32_t kWidth = 256;
    constexpr uint32_t kHeight = 128;

    TextureSpec defaultTextureSpec;
    defaultTextureSpec.copyOrigin = {0, 0, 0};

    for (unsigned int i = 1; i < 4; ++i) {
        TextureSpec srcTextureSpec = defaultTextureSpec;
        srcTextureSpec.textureSize = {kWidth, kHeight, 1};
        srcTextureSpec.level = i;

        TextureSpec dstTextureSpec = defaultTextureSpec;
        dstTextureSpec.textureSize = {kWidth >> i, kHeight >> i, 1};
        dstTextureSpec.level = 0;

        DoTest(srcTextureSpec, dstTextureSpec, {kWidth >> i, kHeight >> i, 1});
    }
}

DAWN_INSTANTIATE_TEST(CopyTests_T2T,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());

static constexpr uint64_t kSmallBufferSize = 4;
static constexpr uint64_t kLargeBufferSize = 1 << 16;

// Test copying full buffers
TEST_P(CopyTests_B2B, FullCopy) {
    DoTest(kSmallBufferSize, 0, kSmallBufferSize, 0, kSmallBufferSize);
    DoTest(kLargeBufferSize, 0, kLargeBufferSize, 0, kLargeBufferSize);
}

// Test copying small pieces of a buffer at different corner case offsets
TEST_P(CopyTests_B2B, SmallCopyInBigBuffer) {
    constexpr uint64_t kEndOffset = kLargeBufferSize - kSmallBufferSize;
    DoTest(kLargeBufferSize, 0, kLargeBufferSize, 0, kSmallBufferSize);
    DoTest(kLargeBufferSize, kEndOffset, kLargeBufferSize, 0, kSmallBufferSize);
    DoTest(kLargeBufferSize, 0, kLargeBufferSize, kEndOffset, kSmallBufferSize);
    DoTest(kLargeBufferSize, kEndOffset, kLargeBufferSize, kEndOffset, kSmallBufferSize);
}

// Test zero-size copies
TEST_P(CopyTests_B2B, ZeroSizedCopy) {
    DoTest(kLargeBufferSize, 0, kLargeBufferSize, 0, 0);
    DoTest(kLargeBufferSize, 0, kLargeBufferSize, kLargeBufferSize, 0);
    DoTest(kLargeBufferSize, kLargeBufferSize, kLargeBufferSize, 0, 0);
    DoTest(kLargeBufferSize, kLargeBufferSize, kLargeBufferSize, kLargeBufferSize, 0);
}

DAWN_INSTANTIATE_TEST(CopyTests_B2B,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());
