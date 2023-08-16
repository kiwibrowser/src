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

#include "common/Assert.h"
#include "common/Constants.h"
#include "common/Math.h"
#include "tests/unittests/validation/ValidationTest.h"
#include "utils/TextureFormatUtils.h"
#include "utils/WGPUHelpers.h"

class CopyCommandTest : public ValidationTest {
  protected:
    wgpu::Buffer CreateBuffer(uint64_t size, wgpu::BufferUsage usage) {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = size;
        descriptor.usage = usage;

        return device.CreateBuffer(&descriptor);
    }

    wgpu::Texture Create2DTexture(uint32_t width,
                                  uint32_t height,
                                  uint32_t mipLevelCount,
                                  uint32_t arrayLayerCount,
                                  wgpu::TextureFormat format,
                                  wgpu::TextureUsage usage,
                                  uint32_t sampleCount = 1) {
        wgpu::TextureDescriptor descriptor;
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size.width = width;
        descriptor.size.height = height;
        descriptor.size.depth = arrayLayerCount;
        descriptor.sampleCount = sampleCount;
        descriptor.format = format;
        descriptor.mipLevelCount = mipLevelCount;
        descriptor.usage = usage;
        wgpu::Texture tex = device.CreateTexture(&descriptor);
        return tex;
    }

    uint32_t BufferSizeForTextureCopy(
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        wgpu::TextureFormat format = wgpu::TextureFormat::RGBA8Unorm) {
        uint32_t bytesPerPixel = utils::GetTexelBlockSizeInBytes(format);
        uint32_t bytesPerRow = Align(width * bytesPerPixel, kTextureBytesPerRowAlignment);
        return (bytesPerRow * (height - 1) + width * bytesPerPixel) * depth;
    }

    void ValidateExpectation(wgpu::CommandEncoder encoder, utils::Expectation expectation) {
        if (expectation == utils::Expectation::Success) {
            encoder.Finish();
        } else {
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }
    }

    void TestB2TCopy(utils::Expectation expectation,
                     wgpu::Buffer srcBuffer,
                     uint64_t srcOffset,
                     uint32_t srcBytesPerRow,
                     uint32_t srcRowsPerImage,
                     wgpu::Texture destTexture,
                     uint32_t destLevel,
                     wgpu::Origin3D destOrigin,
                     wgpu::Extent3D extent3D) {
        wgpu::BufferCopyView bufferCopyView =
            utils::CreateBufferCopyView(srcBuffer, srcOffset, srcBytesPerRow, srcRowsPerImage);
        wgpu::TextureCopyView textureCopyView =
            utils::CreateTextureCopyView(destTexture, destLevel, destOrigin);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &extent3D);

        ValidateExpectation(encoder, expectation);
    }

    void TestT2BCopy(utils::Expectation expectation,
                     wgpu::Texture srcTexture,
                     uint32_t srcLevel,
                     wgpu::Origin3D srcOrigin,
                     wgpu::Buffer destBuffer,
                     uint64_t destOffset,
                     uint32_t destBytesPerRow,
                     uint32_t destRowsPerImage,
                     wgpu::Extent3D extent3D) {
        wgpu::BufferCopyView bufferCopyView =
            utils::CreateBufferCopyView(destBuffer, destOffset, destBytesPerRow, destRowsPerImage);
        wgpu::TextureCopyView textureCopyView =
            utils::CreateTextureCopyView(srcTexture, srcLevel, srcOrigin);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyTextureToBuffer(&textureCopyView, &bufferCopyView, &extent3D);

        ValidateExpectation(encoder, expectation);
    }

    void TestT2TCopy(utils::Expectation expectation,
                     wgpu::Texture srcTexture,
                     uint32_t srcLevel,
                     wgpu::Origin3D srcOrigin,
                     wgpu::Texture dstTexture,
                     uint32_t dstLevel,
                     wgpu::Origin3D dstOrigin,
                     wgpu::Extent3D extent3D) {
        wgpu::TextureCopyView srcTextureCopyView =
            utils::CreateTextureCopyView(srcTexture, srcLevel, srcOrigin);
        wgpu::TextureCopyView dstTextureCopyView =
            utils::CreateTextureCopyView(dstTexture, dstLevel, dstOrigin);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyTextureToTexture(&srcTextureCopyView, &dstTextureCopyView, &extent3D);

        ValidateExpectation(encoder, expectation);
    }

    void TestBothTBCopies(utils::Expectation expectation,
                          wgpu::Buffer buffer,
                          uint64_t bufferOffset,
                          uint32_t bufferBytesPerRow,
                          uint32_t rowsPerImage,
                          wgpu::Texture texture,
                          uint32_t level,
                          wgpu::Origin3D origin,
                          wgpu::Extent3D extent3D) {
        TestB2TCopy(expectation, buffer, bufferOffset, bufferBytesPerRow, rowsPerImage, texture,
                    level, origin, extent3D);
        TestT2BCopy(expectation, texture, level, origin, buffer, bufferOffset, bufferBytesPerRow,
                    rowsPerImage, extent3D);
    }

    void TestBothT2TCopies(utils::Expectation expectation,
                           wgpu::Texture texture1,
                           uint32_t level1,
                           wgpu::Origin3D origin1,
                           wgpu::Texture texture2,
                           uint32_t level2,
                           wgpu::Origin3D origin2,
                           wgpu::Extent3D extent3D) {
        TestT2TCopy(expectation, texture1, level1, origin1, texture2, level2, origin2, extent3D);
        TestT2TCopy(expectation, texture2, level2, origin2, texture1, level1, origin1, extent3D);
    }

    void TestBothTBCopiesExactBufferSize(uint32_t bufferBytesPerRow,
                                         uint32_t rowsPerImage,
                                         wgpu::Texture texture,
                                         wgpu::TextureFormat textureFormat,
                                         wgpu::Origin3D origin,
                                         wgpu::Extent3D extent3D) {
        // Check the minimal valid bufferSize.
        uint64_t bufferSize =
            utils::RequiredBytesInCopy(bufferBytesPerRow, rowsPerImage, extent3D, textureFormat);
        wgpu::Buffer source =
            CreateBuffer(bufferSize, wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst);
        TestBothTBCopies(utils::Expectation::Success, source, 0, bufferBytesPerRow, rowsPerImage,
                         texture, 0, origin, extent3D);

        // Check bufferSize was indeed minimal.
        uint64_t invalidSize = bufferSize - 1;
        wgpu::Buffer invalidSource =
            CreateBuffer(invalidSize, wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst);
        TestBothTBCopies(utils::Expectation::Failure, invalidSource, 0, bufferBytesPerRow,
                         rowsPerImage, texture, 0, origin, extent3D);
    }
};

class CopyCommandTest_B2B : public CopyCommandTest {};

// TODO(cwallez@chromium.org): Test that copies are forbidden inside renderpasses

// Test a successfull B2B copy
TEST_F(CopyCommandTest_B2B, Success) {
    wgpu::Buffer source = CreateBuffer(16, wgpu::BufferUsage::CopySrc);
    wgpu::Buffer destination = CreateBuffer(16, wgpu::BufferUsage::CopyDst);

    // Copy different copies, including some that touch the OOB condition
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(source, 0, destination, 0, 16);
        encoder.CopyBufferToBuffer(source, 8, destination, 0, 8);
        encoder.CopyBufferToBuffer(source, 0, destination, 8, 8);
        encoder.Finish();
    }

    // Empty copies are valid
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(source, 0, destination, 0, 0);
        encoder.CopyBufferToBuffer(source, 0, destination, 16, 0);
        encoder.CopyBufferToBuffer(source, 16, destination, 0, 0);
        encoder.Finish();
    }
}

// Test B2B copies with OOB
TEST_F(CopyCommandTest_B2B, OutOfBounds) {
    wgpu::Buffer source = CreateBuffer(16, wgpu::BufferUsage::CopySrc);
    wgpu::Buffer destination = CreateBuffer(16, wgpu::BufferUsage::CopyDst);

    // OOB on the source
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(source, 8, destination, 0, 12);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // OOB on the destination
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(source, 0, destination, 8, 12);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Test B2B copies with incorrect buffer usage
TEST_F(CopyCommandTest_B2B, BadUsage) {
    wgpu::Buffer source = CreateBuffer(16, wgpu::BufferUsage::CopySrc);
    wgpu::Buffer destination = CreateBuffer(16, wgpu::BufferUsage::CopyDst);
    wgpu::Buffer vertex = CreateBuffer(16, wgpu::BufferUsage::Vertex);

    // Source with incorrect usage
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(vertex, 0, destination, 0, 16);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Destination with incorrect usage
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(source, 0, vertex, 0, 16);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Test B2B copies with unaligned data size
TEST_F(CopyCommandTest_B2B, UnalignedSize) {
    wgpu::Buffer source = CreateBuffer(16, wgpu::BufferUsage::CopySrc);
    wgpu::Buffer destination = CreateBuffer(16, wgpu::BufferUsage::CopyDst);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyBufferToBuffer(source, 8, destination, 0, sizeof(uint8_t));
    ASSERT_DEVICE_ERROR(encoder.Finish());
}

// Test B2B copies with unaligned offset
TEST_F(CopyCommandTest_B2B, UnalignedOffset) {
    wgpu::Buffer source = CreateBuffer(16, wgpu::BufferUsage::CopySrc);
    wgpu::Buffer destination = CreateBuffer(16, wgpu::BufferUsage::CopyDst);

    // Unaligned source offset
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(source, 9, destination, 0, 4);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Unaligned destination offset
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(source, 8, destination, 1, 4);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Test B2B copies with buffers in error state cause errors.
TEST_F(CopyCommandTest_B2B, BuffersInErrorState) {
    wgpu::BufferDescriptor errorBufferDescriptor;
    errorBufferDescriptor.size = 4;
    errorBufferDescriptor.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopySrc;
    ASSERT_DEVICE_ERROR(wgpu::Buffer errorBuffer = device.CreateBuffer(&errorBufferDescriptor));

    constexpr uint64_t bufferSize = 4;
    wgpu::Buffer validBuffer = CreateBuffer(bufferSize, wgpu::BufferUsage::CopySrc);

    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(errorBuffer, 0, validBuffer, 0, 4);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(validBuffer, 0, errorBuffer, 0, 4);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Test it is not allowed to do B2B copies within same buffer.
TEST_F(CopyCommandTest_B2B, CopyWithinSameBuffer) {
    constexpr uint32_t kBufferSize = 16u;
    wgpu::Buffer buffer =
        CreateBuffer(kBufferSize, wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst);

    // srcOffset < dstOffset, and srcOffset + copySize > dstOffset (overlapping)
    {
        constexpr uint32_t kSrcOffset = 0u;
        constexpr uint32_t kDstOffset = 4u;
        constexpr uint32_t kCopySize = 8u;
        ASSERT(kDstOffset > kSrcOffset && kDstOffset < kSrcOffset + kCopySize);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(buffer, kSrcOffset, buffer, kDstOffset, kCopySize);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // srcOffset < dstOffset, and srcOffset + copySize == dstOffset (not overlapping)
    {
        constexpr uint32_t kSrcOffset = 0u;
        constexpr uint32_t kDstOffset = 8u;
        constexpr uint32_t kCopySize = kDstOffset - kSrcOffset;
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(buffer, kSrcOffset, buffer, kDstOffset, kCopySize);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // srcOffset > dstOffset, and srcOffset < dstOffset + copySize (overlapping)
    {
        constexpr uint32_t kSrcOffset = 4u;
        constexpr uint32_t kDstOffset = 0u;
        constexpr uint32_t kCopySize = 8u;
        ASSERT(kSrcOffset > kDstOffset && kSrcOffset < kDstOffset + kCopySize);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(buffer, kSrcOffset, buffer, kDstOffset, kCopySize);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // srcOffset > dstOffset, and srcOffset + copySize == dstOffset (not overlapping)
    {
        constexpr uint32_t kSrcOffset = 8u;
        constexpr uint32_t kDstOffset = 0u;
        constexpr uint32_t kCopySize = kSrcOffset - kDstOffset;
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(buffer, kSrcOffset, buffer, kDstOffset, kCopySize);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

class CopyCommandTest_B2T : public CopyCommandTest {};

// Test a successfull B2T copy
TEST_F(CopyCommandTest_B2T, Success) {
    uint64_t bufferSize = BufferSizeForTextureCopy(4, 4, 1);
    wgpu::Buffer source = CreateBuffer(bufferSize, wgpu::BufferUsage::CopySrc);
    wgpu::Texture destination =
        Create2DTexture(16, 16, 5, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopyDst);

    // Different copies, including some that touch the OOB condition
    {
        // Copy 4x4 block in corner of first mip.
        TestB2TCopy(utils::Expectation::Success, source, 0, 256, 0, destination, 0, {0, 0, 0},
                    {4, 4, 1});
        // Copy 4x4 block in opposite corner of first mip.
        TestB2TCopy(utils::Expectation::Success, source, 0, 256, 0, destination, 0, {12, 12, 0},
                    {4, 4, 1});
        // Copy 4x4 block in the 4x4 mip.
        TestB2TCopy(utils::Expectation::Success, source, 0, 256, 0, destination, 2, {0, 0, 0},
                    {4, 4, 1});
        // Copy with a buffer offset
        TestB2TCopy(utils::Expectation::Success, source, bufferSize - 4, 256, 0, destination, 0,
                    {0, 0, 0}, {1, 1, 1});
    }

    // Copies with a 256-byte aligned bytes per row but unaligned texture region
    {
        // Unaligned region
        TestB2TCopy(utils::Expectation::Success, source, 0, 256, 0, destination, 0, {0, 0, 0},
                    {3, 4, 1});
        // Unaligned region with texture offset
        TestB2TCopy(utils::Expectation::Success, source, 0, 256, 0, destination, 0, {5, 7, 0},
                    {2, 3, 1});
        // Unaligned region, with buffer offset
        TestB2TCopy(utils::Expectation::Success, source, 31 * 4, 256, 0, destination, 0, {0, 0, 0},
                    {3, 3, 1});
    }

    // Empty copies are valid
    {
        // An empty copy
        TestB2TCopy(utils::Expectation::Success, source, 0, 0, 0, destination, 0, {0, 0, 0},
                    {0, 0, 1});
        // An empty copy with depth = 0
        TestB2TCopy(utils::Expectation::Success, source, 0, 0, 0, destination, 0, {0, 0, 0},
                    {0, 0, 0});
        // An empty copy touching the end of the buffer
        TestB2TCopy(utils::Expectation::Success, source, bufferSize, 0, 0, destination, 0,
                    {0, 0, 0}, {0, 0, 1});
        // An empty copy touching the side of the texture
        TestB2TCopy(utils::Expectation::Success, source, 0, 0, 0, destination, 0, {16, 16, 0},
                    {0, 0, 1});
        // An empty copy with depth = 1 and bytesPerRow > 0
        TestB2TCopy(utils::Expectation::Success, source, 0, kTextureBytesPerRowAlignment, 0,
                    destination, 0, {0, 0, 0}, {0, 0, 1});
        // An empty copy with height > 0, depth = 0, bytesPerRow > 0 and rowsPerImage > 0
        TestB2TCopy(utils::Expectation::Success, source, 0, kTextureBytesPerRowAlignment, 16,
                    destination, 0, {0, 0, 0}, {0, 1, 0});
    }
}

// Test OOB conditions on the buffer
TEST_F(CopyCommandTest_B2T, OutOfBoundsOnBuffer) {
    uint64_t bufferSize = BufferSizeForTextureCopy(4, 4, 1);
    wgpu::Buffer source = CreateBuffer(bufferSize, wgpu::BufferUsage::CopySrc);
    wgpu::Texture destination =
        Create2DTexture(16, 16, 5, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopyDst);

    // OOB on the buffer because we copy too many pixels
    TestB2TCopy(utils::Expectation::Failure, source, 0, 256, 0, destination, 0, {0, 0, 0},
                {4, 5, 1});

    // OOB on the buffer because of the offset
    TestB2TCopy(utils::Expectation::Failure, source, 4, 256, 0, destination, 0, {0, 0, 0},
                {4, 4, 1});

    // OOB on the buffer because (bytes per row * (height - 1) + width * bytesPerPixel) * depth
    // overflows
    TestB2TCopy(utils::Expectation::Failure, source, 0, 512, 0, destination, 0, {0, 0, 0},
                {4, 3, 1});

    // Not OOB on the buffer although bytes per row * height overflows
    // but (bytes per row * (height - 1) + width * bytesPerPixel) * depth does not overflow
    {
        uint32_t sourceBufferSize = BufferSizeForTextureCopy(7, 3, 1);
        ASSERT_TRUE(256 * 3 > sourceBufferSize) << "bytes per row * height should overflow buffer";
        wgpu::Buffer sourceBuffer = CreateBuffer(sourceBufferSize, wgpu::BufferUsage::CopySrc);

        TestB2TCopy(utils::Expectation::Success, source, 0, 256, 0, destination, 0, {0, 0, 0},
                    {7, 3, 1});
    }
}

// Test OOB conditions on the texture
TEST_F(CopyCommandTest_B2T, OutOfBoundsOnTexture) {
    uint64_t bufferSize = BufferSizeForTextureCopy(4, 4, 1);
    wgpu::Buffer source = CreateBuffer(bufferSize, wgpu::BufferUsage::CopySrc);
    wgpu::Texture destination =
        Create2DTexture(16, 16, 5, 2, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopyDst);

    // OOB on the texture because x + width overflows
    TestB2TCopy(utils::Expectation::Failure, source, 0, 256, 0, destination, 0, {13, 12, 0},
                {4, 4, 1});

    // OOB on the texture because y + width overflows
    TestB2TCopy(utils::Expectation::Failure, source, 0, 256, 0, destination, 0, {12, 13, 0},
                {4, 4, 1});

    // OOB on the texture because we overflow a non-zero mip
    TestB2TCopy(utils::Expectation::Failure, source, 0, 256, 0, destination, 2, {1, 0, 0},
                {4, 4, 1});

    // OOB on the texture even on an empty copy when we copy to a non-existent mip.
    TestB2TCopy(utils::Expectation::Failure, source, 0, 0, 0, destination, 5, {0, 0, 0}, {0, 0, 1});

    // OOB on the texture because slice overflows
    TestB2TCopy(utils::Expectation::Failure, source, 0, 0, 0, destination, 0, {0, 0, 2}, {0, 0, 1});
}

// Test that we force Depth=1 on copies to 2D textures
TEST_F(CopyCommandTest_B2T, DepthConstraintFor2DTextures) {
    wgpu::Buffer source = CreateBuffer(16 * 4, wgpu::BufferUsage::CopySrc);
    wgpu::Texture destination =
        Create2DTexture(16, 16, 5, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopyDst);

    // Depth > 1 on an empty copy still errors
    TestB2TCopy(utils::Expectation::Failure, source, 0, 0, 0, destination, 0, {0, 0, 0}, {0, 0, 2});
}

// Test B2T copies with incorrect buffer usage
TEST_F(CopyCommandTest_B2T, IncorrectUsage) {
    wgpu::Buffer source = CreateBuffer(16 * 4, wgpu::BufferUsage::CopySrc);
    wgpu::Buffer vertex = CreateBuffer(16 * 4, wgpu::BufferUsage::Vertex);
    wgpu::Texture destination =
        Create2DTexture(16, 16, 5, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopyDst);
    wgpu::Texture sampled =
        Create2DTexture(16, 16, 5, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::Sampled);

    // Incorrect source usage
    TestB2TCopy(utils::Expectation::Failure, vertex, 0, 256, 0, destination, 0, {0, 0, 0},
                {4, 4, 1});

    // Incorrect destination usage
    TestB2TCopy(utils::Expectation::Failure, source, 0, 256, 0, sampled, 0, {0, 0, 0}, {4, 4, 1});
}

TEST_F(CopyCommandTest_B2T, IncorrectBytesPerRow) {
    uint64_t bufferSize = BufferSizeForTextureCopy(128, 16, 1);
    wgpu::Buffer source = CreateBuffer(bufferSize, wgpu::BufferUsage::CopySrc);
    wgpu::Texture destination = Create2DTexture(128, 16, 5, 1, wgpu::TextureFormat::RGBA8Unorm,
                                                wgpu::TextureUsage::CopyDst);

    // bytes per row is 0
    TestB2TCopy(utils::Expectation::Failure, source, 0, 0, 0, destination, 0, {0, 0, 0},
                {64, 4, 1});

    // bytes per row is not 256-byte aligned
    TestB2TCopy(utils::Expectation::Failure, source, 0, 128, 0, destination, 0, {0, 0, 0},
                {4, 4, 1});

    // bytes per row is less than width * bytesPerPixel
    TestB2TCopy(utils::Expectation::Failure, source, 0, 256, 0, destination, 0, {0, 0, 0},
                {65, 1, 1});
}

TEST_F(CopyCommandTest_B2T, ImageHeightConstraint) {
    uint64_t bufferSize = BufferSizeForTextureCopy(5, 5, 1);
    wgpu::Buffer source = CreateBuffer(bufferSize, wgpu::BufferUsage::CopySrc);
    wgpu::Texture destination =
        Create2DTexture(16, 16, 1, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopyDst);

    // Image height is zero (Valid)
    TestB2TCopy(utils::Expectation::Success, source, 0, 256, 0, destination, 0, {0, 0, 0},
                {4, 4, 1});

    // Image height is equal to copy height (Valid)
    TestB2TCopy(utils::Expectation::Success, source, 0, 256, 0, destination, 0, {0, 0, 0},
                {4, 4, 1});

    // Image height is larger than copy height (Valid)
    TestB2TCopy(utils::Expectation::Success, source, 0, 256, 5, destination, 0, {0, 0, 0},
                {4, 4, 1});

    // Image height is less than copy height (Invalid)
    TestB2TCopy(utils::Expectation::Failure, source, 0, 256, 3, destination, 0, {0, 0, 0},
                {4, 4, 1});
}

// Test B2T copies with incorrect buffer offset usage
TEST_F(CopyCommandTest_B2T, IncorrectBufferOffset) {
    uint64_t bufferSize = BufferSizeForTextureCopy(4, 4, 1);
    wgpu::Buffer source = CreateBuffer(bufferSize, wgpu::BufferUsage::CopySrc);
    wgpu::Texture destination =
        Create2DTexture(16, 16, 5, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopyDst);

    // Correct usage
    TestB2TCopy(utils::Expectation::Success, source, bufferSize - 4, 256, 0, destination, 0,
                {0, 0, 0}, {1, 1, 1});

    // Incorrect usages
    {
        TestB2TCopy(utils::Expectation::Failure, source, bufferSize - 5, 256, 0, destination, 0,
                    {0, 0, 0}, {1, 1, 1});
        TestB2TCopy(utils::Expectation::Failure, source, bufferSize - 6, 256, 0, destination, 0,
                    {0, 0, 0}, {1, 1, 1});
        TestB2TCopy(utils::Expectation::Failure, source, bufferSize - 7, 256, 0, destination, 0,
                    {0, 0, 0}, {1, 1, 1});
    }
}

// Test multisampled textures cannot be used in B2T copies.
TEST_F(CopyCommandTest_B2T, CopyToMultisampledTexture) {
    uint64_t bufferSize = BufferSizeForTextureCopy(16, 16, 1);
    wgpu::Buffer source = CreateBuffer(bufferSize, wgpu::BufferUsage::CopySrc);
    wgpu::Texture destination = Create2DTexture(2, 2, 1, 1, wgpu::TextureFormat::RGBA8Unorm,
                                                wgpu::TextureUsage::CopyDst, 4);

    TestB2TCopy(utils::Expectation::Failure, source, 0, 256, 0, destination, 0, {0, 0, 0},
                {2, 2, 1});
}

// Test B2T copies with buffer or texture in error state causes errors.
TEST_F(CopyCommandTest_B2T, BufferOrTextureInErrorState) {
    wgpu::BufferDescriptor errorBufferDescriptor;
    errorBufferDescriptor.size = 4;
    errorBufferDescriptor.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopySrc;
    ASSERT_DEVICE_ERROR(wgpu::Buffer errorBuffer = device.CreateBuffer(&errorBufferDescriptor));

    wgpu::TextureDescriptor errorTextureDescriptor;
    errorTextureDescriptor.size.depth = 0;
    ASSERT_DEVICE_ERROR(wgpu::Texture errorTexture = device.CreateTexture(&errorTextureDescriptor));

    wgpu::BufferCopyView errorBufferCopyView = utils::CreateBufferCopyView(errorBuffer, 0, 0, 0);
    wgpu::TextureCopyView errorTextureCopyView =
        utils::CreateTextureCopyView(errorTexture, 0, {0, 0, 0});

    wgpu::Extent3D extent3D = {1, 1, 1};

    {
        wgpu::Texture destination = Create2DTexture(16, 16, 1, 1, wgpu::TextureFormat::RGBA8Unorm,
                                                    wgpu::TextureUsage::CopyDst);
        wgpu::TextureCopyView textureCopyView =
            utils::CreateTextureCopyView(destination, 0, {0, 0, 0});

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToTexture(&errorBufferCopyView, &textureCopyView, &extent3D);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    {
        uint64_t bufferSize = BufferSizeForTextureCopy(4, 4, 1);
        wgpu::Buffer source = CreateBuffer(bufferSize, wgpu::BufferUsage::CopySrc);

        wgpu::BufferCopyView bufferCopyView = utils::CreateBufferCopyView(source, 0, 0, 0);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToTexture(&bufferCopyView, &errorTextureCopyView, &extent3D);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Regression tests for a bug in the computation of texture copy buffer size in Dawn.
TEST_F(CopyCommandTest_B2T, TextureCopyBufferSizeLastRowComputation) {
    constexpr uint32_t kBytesPerRow = 256;
    constexpr uint32_t kWidth = 4;
    constexpr uint32_t kHeight = 4;

    constexpr std::array<wgpu::TextureFormat, 2> kFormats = {wgpu::TextureFormat::RGBA8Unorm,
                                                             wgpu::TextureFormat::RG8Unorm};

    {
        // kBytesPerRow * (kHeight - 1) + kWidth is not large enough to be the valid buffer size in
        // this test because the buffer sizes in B2T copies are not in texels but in bytes.
        constexpr uint32_t kInvalidBufferSize = kBytesPerRow * (kHeight - 1) + kWidth;

        for (wgpu::TextureFormat format : kFormats) {
            wgpu::Buffer source = CreateBuffer(kInvalidBufferSize, wgpu::BufferUsage::CopySrc);
            wgpu::Texture destination =
                Create2DTexture(kWidth, kHeight, 1, 1, format, wgpu::TextureUsage::CopyDst);
            TestB2TCopy(utils::Expectation::Failure, source, 0, kBytesPerRow, 0, destination, 0,
                        {0, 0, 0}, {kWidth, kHeight, 1});
        }
    }

    {
        for (wgpu::TextureFormat format : kFormats) {
            uint32_t validBufferSize = BufferSizeForTextureCopy(kWidth, kHeight, 1, format);
            wgpu::Texture destination =
                Create2DTexture(kWidth, kHeight, 1, 1, format, wgpu::TextureUsage::CopyDst);

            // Verify the return value of BufferSizeForTextureCopy() is exactly the minimum valid
            // buffer size in this test.
            {
                uint32_t invalidBuffferSize = validBufferSize - 1;
                wgpu::Buffer source = CreateBuffer(invalidBuffferSize, wgpu::BufferUsage::CopySrc);
                TestB2TCopy(utils::Expectation::Failure, source, 0, kBytesPerRow, 0, destination, 0,
                            {0, 0, 0}, {kWidth, kHeight, 1});
            }

            {
                wgpu::Buffer source = CreateBuffer(validBufferSize, wgpu::BufferUsage::CopySrc);
                TestB2TCopy(utils::Expectation::Success, source, 0, kBytesPerRow, 0, destination, 0,
                            {0, 0, 0}, {kWidth, kHeight, 1});
            }
        }
    }
}

// Test copy from buffer to mip map of non square texture
TEST_F(CopyCommandTest_B2T, CopyToMipmapOfNonSquareTexture) {
    uint64_t bufferSize = BufferSizeForTextureCopy(4, 2, 1);
    wgpu::Buffer source = CreateBuffer(bufferSize, wgpu::BufferUsage::CopySrc);
    uint32_t maxMipmapLevel = 3;
    wgpu::Texture destination = Create2DTexture(
        4, 2, maxMipmapLevel, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopyDst);

    // Copy to top level mip map
    TestB2TCopy(utils::Expectation::Success, source, 0, 256, 0, destination, maxMipmapLevel - 1,
                {0, 0, 0}, {1, 1, 1});
    // Copy to high level mip map
    TestB2TCopy(utils::Expectation::Success, source, 0, 256, 0, destination, maxMipmapLevel - 2,
                {0, 0, 0}, {2, 1, 1});
    // Mip level out of range
    TestB2TCopy(utils::Expectation::Failure, source, 0, 256, 0, destination, maxMipmapLevel,
                {0, 0, 0}, {1, 1, 1});
    // Copy origin out of range
    TestB2TCopy(utils::Expectation::Failure, source, 0, 256, 0, destination, maxMipmapLevel - 2,
                {1, 0, 0}, {2, 1, 1});
    // Copy size out of range
    TestB2TCopy(utils::Expectation::Failure, source, 0, 256, 0, destination, maxMipmapLevel - 2,
                {0, 0, 0}, {2, 2, 1});
}

class CopyCommandTest_T2B : public CopyCommandTest {};

// Test a successfull T2B copy
TEST_F(CopyCommandTest_T2B, Success) {
    uint64_t bufferSize = BufferSizeForTextureCopy(4, 4, 1);
    wgpu::Texture source =
        Create2DTexture(16, 16, 5, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopySrc);
    wgpu::Buffer destination = CreateBuffer(bufferSize, wgpu::BufferUsage::CopyDst);

    // Different copies, including some that touch the OOB condition
    {
        // Copy from 4x4 block in corner of first mip.
        TestT2BCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, 0, 256, 0,
                    {4, 4, 1});
        // Copy from 4x4 block in opposite corner of first mip.
        TestT2BCopy(utils::Expectation::Success, source, 0, {12, 12, 0}, destination, 0, 256, 0,
                    {4, 4, 1});
        // Copy from 4x4 block in the 4x4 mip.
        TestT2BCopy(utils::Expectation::Success, source, 2, {0, 0, 0}, destination, 0, 256, 0,
                    {4, 4, 1});
        // Copy with a buffer offset
        TestT2BCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, bufferSize - 4,
                    256, 0, {1, 1, 1});
    }

    // Copies with a 256-byte aligned bytes per row but unaligned texture region
    {
        // Unaligned region
        TestT2BCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, 0, 256, 0,
                    {3, 4, 1});
        // Unaligned region with texture offset
        TestT2BCopy(utils::Expectation::Success, source, 0, {5, 7, 0}, destination, 0, 256, 0,
                    {2, 3, 1});
        // Unaligned region, with buffer offset
        TestT2BCopy(utils::Expectation::Success, source, 2, {0, 0, 0}, destination, 31 * 4, 256, 0,
                    {3, 3, 1});
    }

    // Empty copies are valid
    {
        // An empty copy
        TestT2BCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, 0, 0, 0,
                    {0, 0, 1});
        // An empty copy with depth = 0
        TestT2BCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, 0, 0, 0,
                    {0, 0, 0});
        // An empty copy touching the end of the buffer
        TestT2BCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, bufferSize, 0,
                    0, {0, 0, 1});
        // An empty copy touching the side of the texture
        TestT2BCopy(utils::Expectation::Success, source, 0, {16, 16, 0}, destination, 0, 0, 0,
                    {0, 0, 1});
    }
}

// Test OOB conditions on the texture
TEST_F(CopyCommandTest_T2B, OutOfBoundsOnTexture) {
    uint64_t bufferSize = BufferSizeForTextureCopy(4, 4, 1);
    wgpu::Texture source =
        Create2DTexture(16, 16, 5, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopySrc);
    wgpu::Buffer destination = CreateBuffer(bufferSize, wgpu::BufferUsage::CopyDst);

    // OOB on the texture because x + width overflows
    TestT2BCopy(utils::Expectation::Failure, source, 0, {13, 12, 0}, destination, 0, 256, 0,
                {4, 4, 1});

    // OOB on the texture because y + width overflows
    TestT2BCopy(utils::Expectation::Failure, source, 0, {12, 13, 0}, destination, 0, 256, 0,
                {4, 4, 1});

    // OOB on the texture because we overflow a non-zero mip
    TestT2BCopy(utils::Expectation::Failure, source, 2, {1, 0, 0}, destination, 0, 256, 0,
                {4, 4, 1});

    // OOB on the texture even on an empty copy when we copy from a non-existent mip.
    TestT2BCopy(utils::Expectation::Failure, source, 5, {0, 0, 0}, destination, 0, 0, 0, {0, 0, 1});
}

// Test OOB conditions on the buffer
TEST_F(CopyCommandTest_T2B, OutOfBoundsOnBuffer) {
    uint64_t bufferSize = BufferSizeForTextureCopy(4, 4, 1);
    wgpu::Texture source =
        Create2DTexture(16, 16, 5, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopySrc);
    wgpu::Buffer destination = CreateBuffer(bufferSize, wgpu::BufferUsage::CopyDst);

    // OOB on the buffer because we copy too many pixels
    TestT2BCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 0, 256, 0,
                {4, 5, 1});

    // OOB on the buffer because of the offset
    TestT2BCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 4, 256, 0,
                {4, 4, 1});

    // OOB on the buffer because (bytes per row * (height - 1) + width * bytesPerPixel) * depth
    // overflows
    TestT2BCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 0, 512, 0,
                {4, 3, 1});

    // Not OOB on the buffer although bytes per row * height overflows
    // but (bytes per row * (height - 1) + width * bytesPerPixel) * depth does not overflow
    {
        uint32_t destinationBufferSize = BufferSizeForTextureCopy(7, 3, 1);
        ASSERT_TRUE(256 * 3 > destinationBufferSize)
            << "bytes per row * height should overflow buffer";
        wgpu::Buffer destinationBuffer =
            CreateBuffer(destinationBufferSize, wgpu::BufferUsage::CopyDst);
        TestT2BCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destinationBuffer, 0, 256, 0,
                    {7, 3, 1});
    }
}

// Test that we force Depth=1 on copies from to 2D textures
TEST_F(CopyCommandTest_T2B, DepthConstraintFor2DTextures) {
    uint64_t bufferSize = BufferSizeForTextureCopy(4, 4, 1);
    wgpu::Texture source =
        Create2DTexture(16, 16, 5, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopySrc);
    wgpu::Buffer destination = CreateBuffer(bufferSize, wgpu::BufferUsage::CopyDst);

    // Depth > 1 on an empty copy still errors
    TestT2BCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 0, 0, 0, {0, 0, 2});
}

// Test T2B copies with incorrect buffer usage
TEST_F(CopyCommandTest_T2B, IncorrectUsage) {
    uint64_t bufferSize = BufferSizeForTextureCopy(4, 4, 1);
    wgpu::Texture source =
        Create2DTexture(16, 16, 5, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopySrc);
    wgpu::Texture sampled =
        Create2DTexture(16, 16, 5, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::Sampled);
    wgpu::Buffer destination = CreateBuffer(bufferSize, wgpu::BufferUsage::CopyDst);
    wgpu::Buffer vertex = CreateBuffer(bufferSize, wgpu::BufferUsage::Vertex);

    // Incorrect source usage
    TestT2BCopy(utils::Expectation::Failure, sampled, 0, {0, 0, 0}, destination, 0, 256, 0,
                {4, 4, 1});

    // Incorrect destination usage
    TestT2BCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, vertex, 0, 256, 0, {4, 4, 1});
}

TEST_F(CopyCommandTest_T2B, IncorrectBytesPerRow) {
    uint64_t bufferSize = BufferSizeForTextureCopy(128, 16, 1);
    wgpu::Texture source = Create2DTexture(128, 16, 5, 1, wgpu::TextureFormat::RGBA8Unorm,
                                           wgpu::TextureUsage::CopyDst);
    wgpu::Buffer destination = CreateBuffer(bufferSize, wgpu::BufferUsage::CopySrc);

    // bytes per row is 0
    TestT2BCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 0, 256, 0,
                {64, 4, 1});

    // bytes per row is not 256-byte aligned
    TestT2BCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 0, 257, 0,
                {4, 4, 1});

    // bytes per row is less than width * bytesPerPixel
    TestT2BCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 0, 256, 0,
                {65, 1, 1});
}

TEST_F(CopyCommandTest_T2B, ImageHeightConstraint) {
    uint64_t bufferSize = BufferSizeForTextureCopy(5, 5, 1);
    wgpu::Texture source =
        Create2DTexture(16, 16, 1, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopySrc);
    wgpu::Buffer destination = CreateBuffer(bufferSize, wgpu::BufferUsage::CopyDst);

    // Image height is zero (Valid)
    TestT2BCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, 0, 256, 0,
                {4, 4, 1});

    // Image height is equal to copy height (Valid)
    TestT2BCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, 0, 256, 4,
                {4, 4, 1});

    // Image height exceeds copy height (Valid)
    TestT2BCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, 0, 256, 5,
                {4, 4, 1});

    // Image height is less than copy height (Invalid)
    TestT2BCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 0, 256, 3,
                {4, 4, 1});
}

// Test T2B copies with incorrect buffer offset usage
TEST_F(CopyCommandTest_T2B, IncorrectBufferOffset) {
    uint64_t bufferSize = BufferSizeForTextureCopy(128, 16, 1);
    wgpu::Texture source = Create2DTexture(128, 16, 5, 1, wgpu::TextureFormat::RGBA8Unorm,
                                           wgpu::TextureUsage::CopySrc);
    wgpu::Buffer destination = CreateBuffer(bufferSize, wgpu::BufferUsage::CopyDst);

    // Correct usage
    TestT2BCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, bufferSize - 4, 256,
                0, {1, 1, 1});

    // Incorrect usages
    TestT2BCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, bufferSize - 5, 256,
                0, {1, 1, 1});
    TestT2BCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, bufferSize - 6, 256,
                0, {1, 1, 1});
    TestT2BCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, bufferSize - 7, 256,
                0, {1, 1, 1});
}

// Test multisampled textures cannot be used in T2B copies.
TEST_F(CopyCommandTest_T2B, CopyFromMultisampledTexture) {
    wgpu::Texture source = Create2DTexture(2, 2, 1, 1, wgpu::TextureFormat::RGBA8Unorm,
                                           wgpu::TextureUsage::CopySrc, 4);
    uint64_t bufferSize = BufferSizeForTextureCopy(16, 16, 1);
    wgpu::Buffer destination = CreateBuffer(bufferSize, wgpu::BufferUsage::CopyDst);

    TestT2BCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 0, 256, 0,
                {2, 2, 1});
}

// Test T2B copies with buffer or texture in error state cause errors.
TEST_F(CopyCommandTest_T2B, BufferOrTextureInErrorState) {
    wgpu::BufferDescriptor errorBufferDescriptor;
    errorBufferDescriptor.size = 4;
    errorBufferDescriptor.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopySrc;
    ASSERT_DEVICE_ERROR(wgpu::Buffer errorBuffer = device.CreateBuffer(&errorBufferDescriptor));

    wgpu::TextureDescriptor errorTextureDescriptor;
    errorTextureDescriptor.size.depth = 0;
    ASSERT_DEVICE_ERROR(wgpu::Texture errorTexture = device.CreateTexture(&errorTextureDescriptor));

    wgpu::BufferCopyView errorBufferCopyView = utils::CreateBufferCopyView(errorBuffer, 0, 0, 0);
    wgpu::TextureCopyView errorTextureCopyView =
        utils::CreateTextureCopyView(errorTexture, 0, {0, 0, 0});

    wgpu::Extent3D extent3D = {1, 1, 1};

    {
        uint64_t bufferSize = BufferSizeForTextureCopy(4, 4, 1);
        wgpu::Buffer source = CreateBuffer(bufferSize, wgpu::BufferUsage::CopySrc);

        wgpu::BufferCopyView bufferCopyView = utils::CreateBufferCopyView(source, 0, 0, 0);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyTextureToBuffer(&errorTextureCopyView, &bufferCopyView, &extent3D);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    {
        wgpu::Texture destination = Create2DTexture(16, 16, 1, 1, wgpu::TextureFormat::RGBA8Unorm,
                                                    wgpu::TextureUsage::CopyDst);
        wgpu::TextureCopyView textureCopyView =
            utils::CreateTextureCopyView(destination, 0, {0, 0, 0});

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyTextureToBuffer(&textureCopyView, &errorBufferCopyView, &extent3D);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Regression tests for a bug in the computation of texture copy buffer size in Dawn.
TEST_F(CopyCommandTest_T2B, TextureCopyBufferSizeLastRowComputation) {
    constexpr uint32_t kBytesPerRow = 256;
    constexpr uint32_t kWidth = 4;
    constexpr uint32_t kHeight = 4;

    constexpr std::array<wgpu::TextureFormat, 2> kFormats = {wgpu::TextureFormat::RGBA8Unorm,
                                                             wgpu::TextureFormat::RG8Unorm};

    {
        // kBytesPerRow * (kHeight - 1) + kWidth is not large enough to be the valid buffer size in
        // this test because the buffer sizes in T2B copies are not in texels but in bytes.
        constexpr uint32_t kInvalidBufferSize = kBytesPerRow * (kHeight - 1) + kWidth;

        for (wgpu::TextureFormat format : kFormats) {
            wgpu::Texture source =
                Create2DTexture(kWidth, kHeight, 1, 1, format, wgpu::TextureUsage::CopyDst);

            wgpu::Buffer destination = CreateBuffer(kInvalidBufferSize, wgpu::BufferUsage::CopySrc);
            TestT2BCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 0,
                        kBytesPerRow, 0, {kWidth, kHeight, 1});
        }
    }

    {
        for (wgpu::TextureFormat format : kFormats) {
            uint32_t validBufferSize = BufferSizeForTextureCopy(kWidth, kHeight, 1, format);
            wgpu::Texture source =
                Create2DTexture(kWidth, kHeight, 1, 1, format, wgpu::TextureUsage::CopySrc);

            // Verify the return value of BufferSizeForTextureCopy() is exactly the minimum valid
            // buffer size in this test.
            {
                uint32_t invalidBufferSize = validBufferSize - 1;
                wgpu::Buffer destination =
                    CreateBuffer(invalidBufferSize, wgpu::BufferUsage::CopyDst);
                TestT2BCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 0,
                            kBytesPerRow, 0, {kWidth, kHeight, 1});
            }

            {
                wgpu::Buffer destination =
                    CreateBuffer(validBufferSize, wgpu::BufferUsage::CopyDst);
                TestT2BCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, 0,
                            kBytesPerRow, 0, {kWidth, kHeight, 1});
            }
        }
    }
}

// Test copy from mip map of non square texture to buffer
TEST_F(CopyCommandTest_T2B, CopyFromMipmapOfNonSquareTexture) {
    uint32_t maxMipmapLevel = 3;
    wgpu::Texture source = Create2DTexture(4, 2, maxMipmapLevel, 1, wgpu::TextureFormat::RGBA8Unorm,
                                           wgpu::TextureUsage::CopySrc);
    uint64_t bufferSize = BufferSizeForTextureCopy(4, 2, 1);
    wgpu::Buffer destination = CreateBuffer(bufferSize, wgpu::BufferUsage::CopyDst);

    // Copy from top level mip map
    TestT2BCopy(utils::Expectation::Success, source, maxMipmapLevel - 1, {0, 0, 0}, destination, 0,
                256, 0, {1, 1, 1});
    // Copy from high level mip map
    TestT2BCopy(utils::Expectation::Success, source, maxMipmapLevel - 2, {0, 0, 0}, destination, 0,
                256, 0, {2, 1, 1});
    // Mip level out of range
    TestT2BCopy(utils::Expectation::Failure, source, maxMipmapLevel, {0, 0, 0}, destination, 0, 256,
                0, {2, 1, 1});
    // Copy origin out of range
    TestT2BCopy(utils::Expectation::Failure, source, maxMipmapLevel - 2, {2, 0, 0}, destination, 0,
                256, 0, {2, 1, 1});
    // Copy size out of range
    TestT2BCopy(utils::Expectation::Failure, source, maxMipmapLevel - 2, {1, 0, 0}, destination, 0,
                256, 0, {2, 1, 1});
}

class CopyCommandTest_T2T : public CopyCommandTest {};

TEST_F(CopyCommandTest_T2T, Success) {
    wgpu::Texture source =
        Create2DTexture(16, 16, 5, 4, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopySrc);
    wgpu::Texture destination =
        Create2DTexture(16, 16, 5, 4, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopyDst);

    // Different copies, including some that touch the OOB condition
    {
        // Copy a region along top left boundary
        TestT2TCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, 0, {0, 0, 0},
                    {4, 4, 1});

        // Copy entire texture
        TestT2TCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, 0, {0, 0, 0},
                    {16, 16, 1});

        // Copy a region along bottom right boundary
        TestT2TCopy(utils::Expectation::Success, source, 0, {8, 8, 0}, destination, 0, {8, 8, 0},
                    {8, 8, 1});

        // Copy region into mip
        TestT2TCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, 2, {0, 0, 0},
                    {4, 4, 1});

        // Copy mip into region
        TestT2TCopy(utils::Expectation::Success, source, 2, {0, 0, 0}, destination, 0, {0, 0, 0},
                    {4, 4, 1});

        // Copy between slices
        TestT2TCopy(utils::Expectation::Success, source, 0, {0, 0, 1}, destination, 0, {0, 0, 1},
                    {16, 16, 1});

        // Copy multiple slices (srcTextureCopyView.arrayLayer + copySize.depth ==
        // srcTextureCopyView.texture.arrayLayerCount)
        TestT2TCopy(utils::Expectation::Success, source, 0, {0, 0, 2}, destination, 0, {0, 0, 0},
                    {16, 16, 2});

        // Copy multiple slices (dstTextureCopyView.arrayLayer + copySize.depth ==
        // dstTextureCopyView.texture.arrayLayerCount)
        TestT2TCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, 0, {0, 0, 2},
                    {16, 16, 2});
    }

    // Empty copies are valid
    {
        // An empty copy
        TestT2TCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, 0, {0, 0, 0},
                    {0, 0, 1});

        // An empty copy with depth = 0
        TestT2TCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, 0, {0, 0, 0},
                    {0, 0, 0});

        // An empty copy touching the side of the source texture
        TestT2TCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, 0, {16, 16, 0},
                    {0, 0, 1});

        // An empty copy touching the side of the destination texture
        TestT2TCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, 0, {16, 16, 0},
                    {0, 0, 1});
    }
}

TEST_F(CopyCommandTest_T2T, IncorrectUsage) {
    wgpu::Texture source =
        Create2DTexture(16, 16, 5, 2, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopySrc);
    wgpu::Texture destination =
        Create2DTexture(16, 16, 5, 2, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopyDst);

    // Incorrect source usage causes failure
    TestT2TCopy(utils::Expectation::Failure, destination, 0, {0, 0, 0}, destination, 0, {0, 0, 0},
                {16, 16, 1});

    // Incorrect destination usage causes failure
    TestT2TCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, source, 0, {0, 0, 0},
                {16, 16, 1});
}

TEST_F(CopyCommandTest_T2T, OutOfBounds) {
    wgpu::Texture source =
        Create2DTexture(16, 16, 5, 4, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopySrc);
    wgpu::Texture destination =
        Create2DTexture(16, 16, 5, 4, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopyDst);

    // OOB on source
    {
        // x + width overflows
        TestT2TCopy(utils::Expectation::Failure, source, 0, {1, 0, 0}, destination, 0, {0, 0, 0},
                    {16, 16, 1});

        // y + height overflows
        TestT2TCopy(utils::Expectation::Failure, source, 0, {0, 1, 0}, destination, 0, {0, 0, 0},
                    {16, 16, 1});

        // non-zero mip overflows
        TestT2TCopy(utils::Expectation::Failure, source, 1, {0, 0, 0}, destination, 0, {0, 0, 0},
                    {9, 9, 1});

        // arrayLayer + depth OOB
        TestT2TCopy(utils::Expectation::Failure, source, 0, {0, 0, 3}, destination, 0, {0, 0, 0},
                    {16, 16, 2});

        // empty copy on non-existent mip fails
        TestT2TCopy(utils::Expectation::Failure, source, 6, {0, 0, 0}, destination, 0, {0, 0, 0},
                    {0, 0, 1});

        // empty copy from non-existent slice fails
        TestT2TCopy(utils::Expectation::Failure, source, 0, {0, 0, 4}, destination, 0, {0, 0, 0},
                    {0, 0, 1});
    }

    // OOB on destination
    {
        // x + width overflows
        TestT2TCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 0, {1, 0, 0},
                    {16, 16, 1});

        // y + height overflows
        TestT2TCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 0, {0, 1, 0},
                    {16, 16, 1});

        // non-zero mip overflows
        TestT2TCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 1, {0, 0, 0},
                    {9, 9, 1});

        // arrayLayer + depth OOB
        TestT2TCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 0, {0, 0, 3},
                    {16, 16, 2});

        // empty copy on non-existent mip fails
        TestT2TCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 6, {0, 0, 0},
                    {0, 0, 1});

        // empty copy on non-existent slice fails
        TestT2TCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 0, {0, 0, 4},
                    {0, 0, 1});
    }
}

TEST_F(CopyCommandTest_T2T, 2DTextureDepthStencil) {
    wgpu::Texture source = Create2DTexture(16, 16, 1, 1, wgpu::TextureFormat::Depth24PlusStencil8,
                                           wgpu::TextureUsage::CopySrc);
    wgpu::Texture destination = Create2DTexture(
        16, 16, 1, 1, wgpu::TextureFormat::Depth24PlusStencil8, wgpu::TextureUsage::CopyDst);

    // Success when entire depth stencil subresource is copied
    TestT2TCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, 0, {0, 0, 0},
                {16, 16, 1});

    // Failure when depth stencil subresource is partially copied
    TestT2TCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 0, {0, 0, 0},
                {15, 15, 1});
}

TEST_F(CopyCommandTest_T2T, 2DTextureArrayDepthStencil) {
    {
        wgpu::Texture source = Create2DTexture(
            16, 16, 1, 3, wgpu::TextureFormat::Depth24PlusStencil8, wgpu::TextureUsage::CopySrc);
        wgpu::Texture destination = Create2DTexture(
            16, 16, 1, 1, wgpu::TextureFormat::Depth24PlusStencil8, wgpu::TextureUsage::CopyDst);

        // Success when entire depth stencil subresource (layer) is the copy source
        TestT2TCopy(utils::Expectation::Success, source, 0, {0, 0, 1}, destination, 0, {0, 0, 0},
                    {16, 16, 1});
    }

    {
        wgpu::Texture source = Create2DTexture(
            16, 16, 1, 1, wgpu::TextureFormat::Depth24PlusStencil8, wgpu::TextureUsage::CopySrc);
        wgpu::Texture destination = Create2DTexture(
            16, 16, 1, 3, wgpu::TextureFormat::Depth24PlusStencil8, wgpu::TextureUsage::CopyDst);

        // Success when entire depth stencil subresource (layer) is the copy destination
        TestT2TCopy(utils::Expectation::Success, source, 0, {0, 0, 0}, destination, 0, {0, 0, 1},
                    {16, 16, 1});
    }

    {
        wgpu::Texture source = Create2DTexture(
            16, 16, 1, 3, wgpu::TextureFormat::Depth24PlusStencil8, wgpu::TextureUsage::CopySrc);
        wgpu::Texture destination = Create2DTexture(
            16, 16, 1, 3, wgpu::TextureFormat::Depth24PlusStencil8, wgpu::TextureUsage::CopyDst);

        // Success when src and dst are an entire depth stencil subresource (layer)
        TestT2TCopy(utils::Expectation::Success, source, 0, {0, 0, 2}, destination, 0, {0, 0, 1},
                    {16, 16, 1});

        // Success when src and dst are an array of entire depth stencil subresources
        TestT2TCopy(utils::Expectation::Success, source, 0, {0, 0, 1}, destination, 0, {0, 0, 0},
                    {16, 16, 2});
    }
}

TEST_F(CopyCommandTest_T2T, FormatsMismatch) {
    wgpu::Texture source =
        Create2DTexture(16, 16, 5, 2, wgpu::TextureFormat::RGBA8Uint, wgpu::TextureUsage::CopySrc);
    wgpu::Texture destination =
        Create2DTexture(16, 16, 5, 2, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopyDst);

    // Failure when formats don't match
    TestT2TCopy(utils::Expectation::Failure, source, 0, {0, 0, 0}, destination, 0, {0, 0, 0},
                {0, 0, 1});
}

TEST_F(CopyCommandTest_T2T, MultisampledCopies) {
    wgpu::Texture sourceMultiSampled1x = Create2DTexture(
        16, 16, 1, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopySrc, 1);
    wgpu::Texture sourceMultiSampled4x = Create2DTexture(
        16, 16, 1, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopySrc, 4);
    wgpu::Texture destinationMultiSampled4x = Create2DTexture(
        16, 16, 1, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopyDst, 4);

    // Success when entire multisampled subresource is copied
    {
        TestT2TCopy(utils::Expectation::Success, sourceMultiSampled4x, 0, {0, 0, 0},
                    destinationMultiSampled4x, 0, {0, 0, 0}, {16, 16, 1});
    }

    // Failures
    {
        // An empty copy with mismatched samples fails
        TestT2TCopy(utils::Expectation::Failure, sourceMultiSampled1x, 0, {0, 0, 0},
                    destinationMultiSampled4x, 0, {0, 0, 0}, {0, 0, 1});

        // A copy fails when samples are greater than 1, and entire subresource isn't copied
        TestT2TCopy(utils::Expectation::Failure, sourceMultiSampled4x, 0, {0, 0, 0},
                    destinationMultiSampled4x, 0, {0, 0, 0}, {15, 15, 1});
    }
}

// Test copy to mip map of non square textures
TEST_F(CopyCommandTest_T2T, CopyToMipmapOfNonSquareTexture) {
    uint32_t maxMipmapLevel = 3;
    wgpu::Texture source = Create2DTexture(4, 2, maxMipmapLevel, 1, wgpu::TextureFormat::RGBA8Unorm,
                                           wgpu::TextureUsage::CopySrc);
    wgpu::Texture destination = Create2DTexture(
        4, 2, maxMipmapLevel, 1, wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureUsage::CopyDst);
    // Copy to top level mip map
    TestT2TCopy(utils::Expectation::Success, source, maxMipmapLevel - 1, {0, 0, 0}, destination,
                maxMipmapLevel - 1, {0, 0, 0}, {1, 1, 1});
    // Copy to high level mip map
    TestT2TCopy(utils::Expectation::Success, source, maxMipmapLevel - 2, {0, 0, 0}, destination,
                maxMipmapLevel - 2, {0, 0, 0}, {2, 1, 1});
    // Mip level out of range
    TestT2TCopy(utils::Expectation::Failure, source, maxMipmapLevel, {0, 0, 0}, destination,
                maxMipmapLevel, {0, 0, 0}, {2, 1, 1});
    // Copy origin out of range
    TestT2TCopy(utils::Expectation::Failure, source, maxMipmapLevel - 2, {2, 0, 0}, destination,
                maxMipmapLevel - 2, {2, 0, 0}, {2, 1, 1});
    // Copy size out of range
    TestT2TCopy(utils::Expectation::Failure, source, maxMipmapLevel - 2, {1, 0, 0}, destination,
                maxMipmapLevel - 2, {0, 0, 0}, {2, 1, 1});
}

// Test copy within the same texture
TEST_F(CopyCommandTest_T2T, CopyWithinSameTexture) {
    wgpu::Texture texture =
        Create2DTexture(32, 32, 2, 4, wgpu::TextureFormat::RGBA8Unorm,
                        wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst);

    // The base array layer of the copy source being equal to that of the copy destination is not
    // allowed.
    {
        constexpr uint32_t kBaseArrayLayer = 0;

        // copyExtent.z == 1
        {
            constexpr uint32_t kCopyArrayLayerCount = 1;
            TestT2TCopy(utils::Expectation::Failure, texture, 0, {0, 0, kBaseArrayLayer}, texture,
                        0, {2, 2, kBaseArrayLayer}, {1, 1, kCopyArrayLayerCount});
        }

        // copyExtent.z > 1
        {
            constexpr uint32_t kCopyArrayLayerCount = 2;
            TestT2TCopy(utils::Expectation::Failure, texture, 0, {0, 0, kBaseArrayLayer}, texture,
                        0, {2, 2, kBaseArrayLayer}, {1, 1, kCopyArrayLayerCount});
        }
    }

    // The array slices of the source involved in the copy have no overlap with those of the
    // destination is allowed.
    {
        constexpr uint32_t kCopyArrayLayerCount = 2;

        // srcBaseArrayLayer < dstBaseArrayLayer
        {
            constexpr uint32_t kSrcBaseArrayLayer = 0;
            constexpr uint32_t kDstBaseArrayLayer = kSrcBaseArrayLayer + kCopyArrayLayerCount;

            TestT2TCopy(utils::Expectation::Success, texture, 0, {0, 0, kSrcBaseArrayLayer},
                        texture, 0, {0, 0, kDstBaseArrayLayer}, {1, 1, kCopyArrayLayerCount});
        }

        // srcBaseArrayLayer > dstBaseArrayLayer
        {
            constexpr uint32_t kSrcBaseArrayLayer = 2;
            constexpr uint32_t kDstBaseArrayLayer = kSrcBaseArrayLayer - kCopyArrayLayerCount;
            TestT2TCopy(utils::Expectation::Success, texture, 0, {0, 0, kSrcBaseArrayLayer},
                        texture, 0, {0, 0, kDstBaseArrayLayer}, {1, 1, kCopyArrayLayerCount});
        }
    }

    // Copy between different mipmap levels is allowed.
    {
        constexpr uint32_t kSrcMipLevel = 0;
        constexpr uint32_t kDstMipLevel = 1;

        // Copy one slice
        {
            constexpr uint32_t kCopyArrayLayerCount = 1;
            TestT2TCopy(utils::Expectation::Success, texture, kSrcMipLevel, {0, 0, 0}, texture,
                        kDstMipLevel, {1, 1, 0}, {1, 1, kCopyArrayLayerCount});
        }

        // The base array layer of the copy source is equal to that of the copy destination.
        {
            constexpr uint32_t kCopyArrayLayerCount = 2;
            constexpr uint32_t kBaseArrayLayer = 0;

            TestT2TCopy(utils::Expectation::Success, texture, kSrcMipLevel, {0, 0, kBaseArrayLayer},
                        texture, kDstMipLevel, {1, 1, kBaseArrayLayer},
                        {1, 1, kCopyArrayLayerCount});
        }

        // The array slices of the source involved in the copy have overlaps with those of the
        // destination, and the copy areas have overlaps.
        {
            constexpr uint32_t kCopyArrayLayerCount = 2;

            constexpr uint32_t kSrcBaseArrayLayer = 0;
            constexpr uint32_t kDstBaseArrayLayer = 1;
            ASSERT(kSrcBaseArrayLayer + kCopyArrayLayerCount > kDstBaseArrayLayer);

            constexpr wgpu::Extent3D kCopyExtent = {1, 1, kCopyArrayLayerCount};

            TestT2TCopy(utils::Expectation::Success, texture, kSrcMipLevel,
                        {0, 0, kSrcBaseArrayLayer}, texture, kDstMipLevel,
                        {0, 0, kDstBaseArrayLayer}, kCopyExtent);
        }
    }

    // The array slices of the source involved in the copy have overlaps with those of the
    // destination is not allowed.
    {
        constexpr uint32_t kMipmapLevel = 0;
        constexpr uint32_t kMinBaseArrayLayer = 0;
        constexpr uint32_t kMaxBaseArrayLayer = 1;
        constexpr uint32_t kCopyArrayLayerCount = 3;
        ASSERT(kMinBaseArrayLayer + kCopyArrayLayerCount > kMaxBaseArrayLayer);

        constexpr wgpu::Extent3D kCopyExtent = {4, 4, kCopyArrayLayerCount};

        const wgpu::Origin3D srcOrigin = {0, 0, kMinBaseArrayLayer};
        const wgpu::Origin3D dstOrigin = {4, 4, kMaxBaseArrayLayer};
        TestT2TCopy(utils::Expectation::Failure, texture, kMipmapLevel, srcOrigin, texture,
                    kMipmapLevel, dstOrigin, kCopyExtent);
    }

    // Copy between different mipmap levels and array slices is allowed.
    TestT2TCopy(utils::Expectation::Success, texture, 0, {0, 0, 1}, texture, 1, {1, 1, 0},
                {1, 1, 1});
}

class CopyCommandTest_CompressedTextureFormats : public CopyCommandTest {
  public:
    CopyCommandTest_CompressedTextureFormats() : CopyCommandTest() {
        device = CreateDeviceFromAdapter(adapter, {"texture_compression_bc"});
    }

  protected:
    wgpu::Texture Create2DTexture(wgpu::TextureFormat format,
                                  uint32_t mipmapLevels = 1,
                                  uint32_t width = kWidth,
                                  uint32_t height = kHeight) {
        constexpr wgpu::TextureUsage kUsage =
            wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::Sampled;
        constexpr uint32_t kArrayLayers = 1;
        return CopyCommandTest::Create2DTexture(width, height, mipmapLevels, kArrayLayers, format,
                                                kUsage, 1);
    }

    static constexpr uint32_t kWidth = 16;
    static constexpr uint32_t kHeight = 16;
};

// Tests to verify that bufferOffset must be a multiple of the compressed texture blocks in bytes
// in buffer-to-texture or texture-to-buffer copies with compressed texture formats.
TEST_F(CopyCommandTest_CompressedTextureFormats, BufferOffset) {
    wgpu::Buffer buffer =
        CreateBuffer(512, wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst);

    for (wgpu::TextureFormat bcFormat : utils::kBCFormats) {
        wgpu::Texture texture = Create2DTexture(bcFormat);

        // Valid usages of BufferOffset in B2T and T2B copies with compressed texture formats.
        {
            uint32_t validBufferOffset = utils::GetTexelBlockSizeInBytes(bcFormat);
            TestBothTBCopies(utils::Expectation::Success, buffer, validBufferOffset, 256, 4,
                             texture, 0, {0, 0, 0}, {4, 4, 1});
        }

        // Failures on invalid bufferOffset.
        {
            uint32_t kInvalidBufferOffset = utils::GetTexelBlockSizeInBytes(bcFormat) / 2;
            TestBothTBCopies(utils::Expectation::Failure, buffer, kInvalidBufferOffset, 256, 4,
                             texture, 0, {0, 0, 0}, {4, 4, 1});
        }
    }
}

// Tests to verify that bytesPerRow must not be less than (width / blockWidth) * blockSizeInBytes.
// Note that in Dawn we require bytesPerRow be a multiple of 256, which ensures bytesPerRow will
// always be the multiple of compressed texture block width in bytes.
TEST_F(CopyCommandTest_CompressedTextureFormats, BytesPerRow) {
    wgpu::Buffer buffer =
        CreateBuffer(1024, wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst);

    {
        constexpr uint32_t kTestWidth = 160;
        constexpr uint32_t kTestHeight = 160;

        // Failures on the BytesPerRow that is not large enough.
        {
            constexpr uint32_t kSmallBytesPerRow = 256;
            for (wgpu::TextureFormat bcFormat : utils::kBCFormats) {
                wgpu::Texture texture = Create2DTexture(bcFormat, 1, kTestWidth, kTestHeight);
                TestBothTBCopies(utils::Expectation::Failure, buffer, 0, kSmallBytesPerRow, 4,
                                 texture, 0, {0, 0, 0}, {kTestWidth, 4, 1});
            }
        }

        // Test it is not valid to use a BytesPerRow that is not a multiple of 256.
        {
            for (wgpu::TextureFormat bcFormat : utils::kBCFormats) {
                wgpu::Texture texture = Create2DTexture(bcFormat, 1, kTestWidth, kTestHeight);
                uint32_t inValidBytesPerRow =
                    kTestWidth / 4 * utils::GetTexelBlockSizeInBytes(bcFormat);
                ASSERT_NE(0u, inValidBytesPerRow % 256);
                TestBothTBCopies(utils::Expectation::Failure, buffer, 0, inValidBytesPerRow, 4,
                                 texture, 0, {0, 0, 0}, {kTestWidth, 4, 1});
            }
        }

        // Test the smallest valid BytesPerRow should work.
        {
            for (wgpu::TextureFormat bcFormat : utils::kBCFormats) {
                wgpu::Texture texture = Create2DTexture(bcFormat, 1, kTestWidth, kTestHeight);
                uint32_t smallestValidBytesPerRow =
                    Align(kTestWidth / 4 * utils::GetTexelBlockSizeInBytes(bcFormat), 256);
                TestBothTBCopies(utils::Expectation::Success, buffer, 0, smallestValidBytesPerRow,
                                 4, texture, 0, {0, 0, 0}, {kTestWidth, 4, 1});
            }
        }
    }
}

// Tests to verify that rowsPerImage must be a multiple of the compressed texture block height in
// buffer-to-texture or texture-to-buffer copies with compressed texture formats.
TEST_F(CopyCommandTest_CompressedTextureFormats, ImageHeight) {
    wgpu::Buffer buffer =
        CreateBuffer(512, wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst);

    for (wgpu::TextureFormat bcFormat : utils::kBCFormats) {
        wgpu::Texture texture = Create2DTexture(bcFormat);

        // Valid usages of rowsPerImage in B2T and T2B copies with compressed texture formats.
        {
            constexpr uint32_t kValidImageHeight = 8;
            TestBothTBCopies(utils::Expectation::Success, buffer, 0, 256, kValidImageHeight,
                             texture, 0, {0, 0, 0}, {4, 4, 1});
        }

        // Failures on invalid rowsPerImage.
        {
            constexpr uint32_t kInvalidImageHeight = 3;
            TestBothTBCopies(utils::Expectation::Failure, buffer, 0, 256, kInvalidImageHeight,
                             texture, 0, {0, 0, 0}, {4, 4, 1});
        }
    }
}

// Tests to verify that ImageOffset.x must be a multiple of the compressed texture block width and
// ImageOffset.y must be a multiple of the compressed texture block height in buffer-to-texture,
// texture-to-buffer or texture-to-texture copies with compressed texture formats.
TEST_F(CopyCommandTest_CompressedTextureFormats, ImageOffset) {
    wgpu::Buffer buffer =
        CreateBuffer(512, wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst);

    for (wgpu::TextureFormat bcFormat : utils::kBCFormats) {
        wgpu::Texture texture = Create2DTexture(bcFormat);
        wgpu::Texture texture2 = Create2DTexture(bcFormat);

        constexpr wgpu::Origin3D kSmallestValidOrigin3D = {4, 4, 0};

        // Valid usages of ImageOffset in B2T, T2B and T2T copies with compressed texture formats.
        {
            TestBothTBCopies(utils::Expectation::Success, buffer, 0, 256, 4, texture, 0,
                             kSmallestValidOrigin3D, {4, 4, 1});
            TestBothT2TCopies(utils::Expectation::Success, texture, 0, {0, 0, 0}, texture2, 0,
                              kSmallestValidOrigin3D, {4, 4, 1});
        }

        // Failures on invalid ImageOffset.x.
        {
            constexpr wgpu::Origin3D kInvalidOrigin3D = {kSmallestValidOrigin3D.x - 1,
                                                         kSmallestValidOrigin3D.y, 0};
            TestBothTBCopies(utils::Expectation::Failure, buffer, 0, 256, 4, texture, 0,
                             kInvalidOrigin3D, {4, 4, 1});
            TestBothT2TCopies(utils::Expectation::Failure, texture, 0, kInvalidOrigin3D, texture2,
                              0, {0, 0, 0}, {4, 4, 1});
        }

        // Failures on invalid ImageOffset.y.
        {
            constexpr wgpu::Origin3D kInvalidOrigin3D = {kSmallestValidOrigin3D.x,
                                                         kSmallestValidOrigin3D.y - 1, 0};
            TestBothTBCopies(utils::Expectation::Failure, buffer, 0, 256, 4, texture, 0,
                             kInvalidOrigin3D, {4, 4, 1});
            TestBothT2TCopies(utils::Expectation::Failure, texture, 0, kInvalidOrigin3D, texture2,
                              0, {0, 0, 0}, {4, 4, 1});
        }
    }
}

// Tests to verify that ImageExtent.x must be a multiple of the compressed texture block width and
// ImageExtent.y must be a multiple of the compressed texture block height in buffer-to-texture,
// texture-to-buffer or texture-to-texture copies with compressed texture formats.
TEST_F(CopyCommandTest_CompressedTextureFormats, ImageExtent) {
    wgpu::Buffer buffer =
        CreateBuffer(512, wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst);

    constexpr uint32_t kMipmapLevels = 3;
    constexpr uint32_t kTestWidth = 60;
    constexpr uint32_t kTestHeight = 60;

    for (wgpu::TextureFormat bcFormat : utils::kBCFormats) {
        wgpu::Texture texture = Create2DTexture(bcFormat, kMipmapLevels, kTestWidth, kTestHeight);
        wgpu::Texture texture2 = Create2DTexture(bcFormat, kMipmapLevels, kTestWidth, kTestHeight);

        constexpr wgpu::Extent3D kSmallestValidExtent3D = {4, 4, 1};

        // Valid usages of ImageExtent in B2T, T2B and T2T copies with compressed texture formats.
        {
            TestBothTBCopies(utils::Expectation::Success, buffer, 0, 256, 8, texture, 0, {0, 0, 0},
                             kSmallestValidExtent3D);
            TestBothT2TCopies(utils::Expectation::Success, texture, 0, {0, 0, 0}, texture2, 0,
                              {0, 0, 0}, kSmallestValidExtent3D);
        }

        // Valid usages of ImageExtent in B2T, T2B and T2T copies with compressed texture formats
        // and non-zero mipmap levels.
        {
            constexpr uint32_t kTestMipmapLevel = 2;
            constexpr wgpu::Origin3D kTestOrigin = {
                (kTestWidth >> kTestMipmapLevel) - kSmallestValidExtent3D.width + 1,
                (kTestHeight >> kTestMipmapLevel) - kSmallestValidExtent3D.height + 1, 0};

            TestBothTBCopies(utils::Expectation::Success, buffer, 0, 256, 4, texture,
                             kTestMipmapLevel, kTestOrigin, kSmallestValidExtent3D);
            TestBothT2TCopies(utils::Expectation::Success, texture, kTestMipmapLevel, kTestOrigin,
                              texture2, 0, {0, 0, 0}, kSmallestValidExtent3D);
        }

        // Failures on invalid ImageExtent.x.
        {
            constexpr wgpu::Extent3D kInValidExtent3D = {kSmallestValidExtent3D.width - 1,
                                                         kSmallestValidExtent3D.height, 1};
            TestBothTBCopies(utils::Expectation::Failure, buffer, 0, 256, 4, texture, 0, {0, 0, 0},
                             kInValidExtent3D);
            TestBothT2TCopies(utils::Expectation::Failure, texture, 0, {0, 0, 0}, texture2, 0,
                              {0, 0, 0}, kInValidExtent3D);
        }

        // Failures on invalid ImageExtent.y.
        {
            constexpr wgpu::Extent3D kInValidExtent3D = {kSmallestValidExtent3D.width,
                                                         kSmallestValidExtent3D.height - 1, 1};
            TestBothTBCopies(utils::Expectation::Failure, buffer, 0, 256, 4, texture, 0, {0, 0, 0},
                             kInValidExtent3D);
            TestBothT2TCopies(utils::Expectation::Failure, texture, 0, {0, 0, 0}, texture2, 0,
                              {0, 0, 0}, kInValidExtent3D);
        }
    }
}

// Test copies between buffer and multiple array layers of an uncompressed texture
TEST_F(CopyCommandTest, CopyToMultipleArrayLayers) {
    wgpu::Texture destination =
        CopyCommandTest::Create2DTexture(4, 2, 1, 5, wgpu::TextureFormat::RGBA8Unorm,
                                         wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc);

    // Copy to all array layers
    TestBothTBCopiesExactBufferSize(256, 2, destination, wgpu::TextureFormat::RGBA8Unorm, {0, 0, 0},
                                    {4, 2, 5});

    // Copy to the highest array layer
    TestBothTBCopiesExactBufferSize(256, 2, destination, wgpu::TextureFormat::RGBA8Unorm, {0, 0, 4},
                                    {4, 2, 1});

    // Copy to array layers in the middle
    TestBothTBCopiesExactBufferSize(256, 2, destination, wgpu::TextureFormat::RGBA8Unorm, {0, 0, 1},
                                    {4, 2, 3});

    // Copy with a non-packed rowsPerImage
    TestBothTBCopiesExactBufferSize(256, 3, destination, wgpu::TextureFormat::RGBA8Unorm, {0, 0, 0},
                                    {4, 2, 5});

    // Copy with bytesPerRow = 512
    TestBothTBCopiesExactBufferSize(512, 2, destination, wgpu::TextureFormat::RGBA8Unorm, {0, 0, 1},
                                    {4, 2, 3});
}

// Test copies between buffer and multiple array layers of a compressed texture
TEST_F(CopyCommandTest_CompressedTextureFormats, CopyToMultipleArrayLayers) {
    for (wgpu::TextureFormat bcFormat : utils::kBCFormats) {
        wgpu::Texture texture = CopyCommandTest::Create2DTexture(
            12, 16, 1, 20, bcFormat, wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc);

        // Copy to all array layers
        TestBothTBCopiesExactBufferSize(256, 16, texture, bcFormat, {0, 0, 0}, {12, 16, 20});

        // Copy to the highest array layer
        TestBothTBCopiesExactBufferSize(256, 16, texture, bcFormat, {0, 0, 19}, {12, 16, 1});

        // Copy to array layers in the middle
        TestBothTBCopiesExactBufferSize(256, 16, texture, bcFormat, {0, 0, 1}, {12, 16, 18});

        // Copy touching the texture corners with a non-packed rowsPerImage
        TestBothTBCopiesExactBufferSize(256, 24, texture, bcFormat, {4, 4, 4}, {8, 12, 16});

        // rowsPerImage needs to be a multiple of blockHeight
        {
            wgpu::Buffer source =
                CreateBuffer(8192, wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst);
            TestBothTBCopies(utils::Expectation::Failure, source, 0, 256, 6, texture, 0, {0, 0, 0},
                             {4, 4, 1});
        }

        // rowsPerImage must be a multiple of blockHeight even with an empty copy
        {
            wgpu::Buffer source =
                CreateBuffer(0, wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst);
            TestBothTBCopies(utils::Expectation::Failure, source, 0, 256, 2, texture, 0, {0, 0, 0},
                             {0, 0, 0});
        }
    }
}
