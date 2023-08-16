// Copyright 2020 The Dawn Authors
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

#include "common/Math.h"
#include "utils/TextureFormatUtils.h"
#include "utils/WGPUHelpers.h"

namespace {

    class QueueWriteTextureValidationTest : public ValidationTest {
      private:
        void SetUp() override {
            ValidationTest::SetUp();
            queue = device.GetDefaultQueue();
        }

      protected:
        wgpu::Texture Create2DTexture(wgpu::Extent3D size,
                                      uint32_t mipLevelCount,
                                      wgpu::TextureFormat format,
                                      wgpu::TextureUsage usage,
                                      uint32_t sampleCount = 1) {
            wgpu::TextureDescriptor descriptor;
            descriptor.dimension = wgpu::TextureDimension::e2D;
            descriptor.size.width = size.width;
            descriptor.size.height = size.height;
            descriptor.size.depth = size.depth;
            descriptor.sampleCount = sampleCount;
            descriptor.format = format;
            descriptor.mipLevelCount = mipLevelCount;
            descriptor.usage = usage;
            wgpu::Texture tex = device.CreateTexture(&descriptor);
            return tex;
        }

        void TestWriteTexture(size_t dataSize,
                              uint32_t dataOffset,
                              uint32_t dataBytesPerRow,
                              uint32_t dataRowsPerImage,
                              wgpu::Texture texture,
                              uint32_t texLevel,
                              wgpu::Origin3D texOrigin,
                              wgpu::Extent3D size) {
            std::vector<uint8_t> data(dataSize);

            wgpu::TextureDataLayout textureDataLayout;
            textureDataLayout.offset = dataOffset;
            textureDataLayout.bytesPerRow = dataBytesPerRow;
            textureDataLayout.rowsPerImage = dataRowsPerImage;

            wgpu::TextureCopyView textureCopyView =
                utils::CreateTextureCopyView(texture, texLevel, texOrigin);

            queue.WriteTexture(&textureCopyView, data.data(), dataSize, &textureDataLayout, &size);
        }

        void TestWriteTextureExactDataSize(uint32_t bytesPerRow,
                                           uint32_t rowsPerImage,
                                           wgpu::Texture texture,
                                           wgpu::TextureFormat textureFormat,
                                           wgpu::Origin3D origin,
                                           wgpu::Extent3D extent3D) {
            // Check the minimal valid dataSize.
            uint64_t dataSize =
                utils::RequiredBytesInCopy(bytesPerRow, rowsPerImage, extent3D, textureFormat);
            TestWriteTexture(dataSize, 0, bytesPerRow, rowsPerImage, texture, 0, origin, extent3D);

            // Check dataSize was indeed minimal.
            uint64_t invalidSize = dataSize - 1;
            ASSERT_DEVICE_ERROR(TestWriteTexture(invalidSize, 0, bytesPerRow, rowsPerImage, texture,
                                                 0, origin, extent3D));
        }

        wgpu::Queue queue;
    };

    // Test the success case for WriteTexture
    TEST_F(QueueWriteTextureValidationTest, Success) {
        const uint64_t dataSize =
            utils::RequiredBytesInCopy(256, 0, {4, 4, 1}, wgpu::TextureFormat::RGBA8Unorm);
        wgpu::Texture destination = Create2DTexture({16, 16, 4}, 5, wgpu::TextureFormat::RGBA8Unorm,
                                                    wgpu::TextureUsage::CopyDst);

        // Different copies, including some that touch the OOB condition
        {
            // Copy 4x4 block in corner of first mip.
            TestWriteTexture(dataSize, 0, 256, 0, destination, 0, {0, 0, 0}, {4, 4, 1});
            // Copy 4x4 block in opposite corner of first mip.
            TestWriteTexture(dataSize, 0, 256, 0, destination, 0, {12, 12, 0}, {4, 4, 1});
            // Copy 4x4 block in the 4x4 mip.
            TestWriteTexture(dataSize, 0, 256, 0, destination, 2, {0, 0, 0}, {4, 4, 1});
            // Copy with a data offset
            TestWriteTexture(dataSize, dataSize - 4, 256, 0, destination, 0, {0, 0, 0}, {1, 1, 1});
        }

        // Copies with a 256-byte aligned bytes per row but unaligned texture region
        {
            // Unaligned region
            TestWriteTexture(dataSize, 0, 256, 0, destination, 0, {0, 0, 0}, {3, 4, 1});
            // Unaligned region with texture offset
            TestWriteTexture(dataSize, 0, 256, 0, destination, 0, {5, 7, 0}, {2, 3, 1});
            // Unaligned region, with data offset
            TestWriteTexture(dataSize, 31 * 4, 256, 0, destination, 0, {0, 0, 0}, {3, 3, 1});
        }

        // Empty copies are valid
        {
            // An empty copy
            TestWriteTexture(dataSize, 0, 0, 0, destination, 0, {0, 0, 0}, {0, 0, 1});
            // An empty copy with depth = 0
            TestWriteTexture(dataSize, 0, 0, 0, destination, 0, {0, 0, 0}, {0, 0, 0});
            // An empty copy touching the end of the data
            TestWriteTexture(dataSize, dataSize, 0, 0, destination, 0, {0, 0, 0}, {0, 0, 1});
            // An empty copy touching the side of the texture
            TestWriteTexture(dataSize, 0, 0, 0, destination, 0, {16, 16, 0}, {0, 0, 1});
            // An empty copy with depth = 1 and bytesPerRow > 0
            TestWriteTexture(dataSize, 0, 256, 0, destination, 0, {0, 0, 0}, {0, 0, 1});
            // An empty copy with height > 0, depth = 0, bytesPerRow > 0 and rowsPerImage > 0
            TestWriteTexture(dataSize, 0, 256, 16, destination, 0, {0, 0, 0}, {0, 1, 0});
        }
    }

    // Test OOB conditions on the data
    TEST_F(QueueWriteTextureValidationTest, OutOfBoundsOnData) {
        const uint64_t dataSize =
            utils::RequiredBytesInCopy(256, 0, {4, 4, 1}, wgpu::TextureFormat::RGBA8Unorm);
        wgpu::Texture destination = Create2DTexture({16, 16, 1}, 5, wgpu::TextureFormat::RGBA8Unorm,
                                                    wgpu::TextureUsage::CopyDst);

        // OOB on the data because we copy too many pixels
        ASSERT_DEVICE_ERROR(
            TestWriteTexture(dataSize, 0, 256, 0, destination, 0, {0, 0, 0}, {4, 5, 1}));

        // OOB on the data because of the offset
        ASSERT_DEVICE_ERROR(
            TestWriteTexture(dataSize, 4, 256, 0, destination, 0, {0, 0, 0}, {4, 4, 1}));

        // OOB on the data because utils::RequiredBytesInCopy overflows
        ASSERT_DEVICE_ERROR(
            TestWriteTexture(dataSize, 0, 512, 0, destination, 0, {0, 0, 0}, {4, 3, 1}));

        // Not OOB on the data although bytes per row * height overflows
        // but utils::RequiredBytesInCopy * depth does not overflow
        {
            uint32_t sourceDataSize =
                utils::RequiredBytesInCopy(256, 0, {7, 3, 1}, wgpu::TextureFormat::RGBA8Unorm);
            ASSERT_TRUE(256 * 3 > sourceDataSize) << "bytes per row * height should overflow data";

            TestWriteTexture(sourceDataSize, 0, 256, 0, destination, 0, {0, 0, 0}, {7, 3, 1});
        }
    }

    // Test OOB conditions on the texture
    TEST_F(QueueWriteTextureValidationTest, OutOfBoundsOnTexture) {
        const uint64_t dataSize =
            utils::RequiredBytesInCopy(256, 0, {4, 4, 1}, wgpu::TextureFormat::RGBA8Unorm);
        wgpu::Texture destination = Create2DTexture({16, 16, 2}, 5, wgpu::TextureFormat::RGBA8Unorm,
                                                    wgpu::TextureUsage::CopyDst);

        // OOB on the texture because x + width overflows
        ASSERT_DEVICE_ERROR(
            TestWriteTexture(dataSize, 0, 256, 0, destination, 0, {13, 12, 0}, {4, 4, 1}));

        // OOB on the texture because y + width overflows
        ASSERT_DEVICE_ERROR(
            TestWriteTexture(dataSize, 0, 256, 0, destination, 0, {12, 13, 0}, {4, 4, 1}));

        // OOB on the texture because we overflow a non-zero mip
        ASSERT_DEVICE_ERROR(
            TestWriteTexture(dataSize, 0, 256, 0, destination, 2, {1, 0, 0}, {4, 4, 1}));

        // OOB on the texture even on an empty copy when we copy to a non-existent mip.
        ASSERT_DEVICE_ERROR(
            TestWriteTexture(dataSize, 0, 0, 0, destination, 5, {0, 0, 0}, {0, 0, 1}));

        // OOB on the texture because slice overflows
        ASSERT_DEVICE_ERROR(
            TestWriteTexture(dataSize, 0, 0, 0, destination, 0, {0, 0, 2}, {0, 0, 1}));
    }

    // Test that we force Depth=1 on writes to 2D textures
    TEST_F(QueueWriteTextureValidationTest, DepthConstraintFor2DTextures) {
        const uint64_t dataSize =
            utils::RequiredBytesInCopy(0, 0, {0, 0, 2}, wgpu::TextureFormat::RGBA8Unorm);
        wgpu::Texture destination = Create2DTexture({16, 16, 1}, 5, wgpu::TextureFormat::RGBA8Unorm,
                                                    wgpu::TextureUsage::CopyDst);

        // Depth > 1 on an empty copy still errors
        ASSERT_DEVICE_ERROR(
            TestWriteTexture(dataSize, 0, 0, 0, destination, 0, {0, 0, 0}, {0, 0, 2}));
    }

    // Test WriteTexture with incorrect texture usage
    TEST_F(QueueWriteTextureValidationTest, IncorrectUsage) {
        const uint64_t dataSize =
            utils::RequiredBytesInCopy(256, 0, {4, 4, 1}, wgpu::TextureFormat::RGBA8Unorm);
        wgpu::Texture sampled = Create2DTexture({16, 16, 1}, 5, wgpu::TextureFormat::RGBA8Unorm,
                                                wgpu::TextureUsage::Sampled);

        // Incorrect destination usage
        ASSERT_DEVICE_ERROR(
            TestWriteTexture(dataSize, 0, 256, 0, sampled, 0, {0, 0, 0}, {4, 4, 1}));
    }

    // Test incorrect values of bytesPerRow and that values not divisible by 256 are allowed.
    TEST_F(QueueWriteTextureValidationTest, BytesPerRowLimitations) {
        wgpu::Texture destination = Create2DTexture({3, 7, 1}, 1, wgpu::TextureFormat::RGBA8Unorm,
                                                    wgpu::TextureUsage::CopyDst);

        // bytesPerRow = 0 is invalid
        ASSERT_DEVICE_ERROR(TestWriteTexture(128, 0, 0, 0, destination, 0, {0, 0, 0}, {3, 7, 1}));

        // bytesPerRow = 11 is invalid since a row takes 12 bytes.
        ASSERT_DEVICE_ERROR(TestWriteTexture(128, 0, 11, 0, destination, 0, {0, 0, 0}, {3, 7, 1}));

        // bytesPerRow = 12 is valid since a row takes 12 bytes.
        TestWriteTexture(128, 0, 12, 0, destination, 0, {0, 0, 0}, {3, 7, 1});

        // bytesPerRow = 13 is valid since a row takes 12 bytes.
        TestWriteTexture(128, 0, 13, 0, destination, 0, {0, 0, 0}, {3, 7, 1});
    }

    // Test that if rowsPerImage is greater than 0, it must be at least copy height.
    TEST_F(QueueWriteTextureValidationTest, ImageHeightConstraint) {
        uint64_t dataSize =
            utils::RequiredBytesInCopy(256, 0, {4, 4, 1}, wgpu::TextureFormat::RGBA8Unorm);
        wgpu::Texture destination = Create2DTexture({16, 16, 1}, 1, wgpu::TextureFormat::RGBA8Unorm,
                                                    wgpu::TextureUsage::CopyDst);

        // Image height is zero (Valid)
        TestWriteTexture(dataSize, 0, 256, 0, destination, 0, {0, 0, 0}, {4, 4, 1});

        // Image height is equal to copy height (Valid)
        TestWriteTexture(dataSize, 0, 256, 4, destination, 0, {0, 0, 0}, {4, 4, 1});

        // Image height is larger than copy height (Valid)
        TestWriteTexture(dataSize, 0, 256, 5, destination, 0, {0, 0, 0}, {4, 4, 1});

        // Image height is less than copy height (Invalid)
        ASSERT_DEVICE_ERROR(
            TestWriteTexture(dataSize, 0, 256, 3, destination, 0, {0, 0, 0}, {4, 4, 1}));
    }

    // Test WriteTexture with incorrect data offset usage
    TEST_F(QueueWriteTextureValidationTest, IncorrectDataOffset) {
        uint64_t dataSize =
            utils::RequiredBytesInCopy(256, 0, {4, 4, 1}, wgpu::TextureFormat::RGBA8Unorm);
        wgpu::Texture destination = Create2DTexture({16, 16, 1}, 5, wgpu::TextureFormat::RGBA8Unorm,
                                                    wgpu::TextureUsage::CopyDst);

        // Correct usage
        TestWriteTexture(dataSize, dataSize - 4, 256, 0, destination, 0, {0, 0, 0}, {1, 1, 1});

        ASSERT_DEVICE_ERROR(
            TestWriteTexture(dataSize, dataSize - 6, 256, 0, destination, 0, {0, 0, 0}, {1, 1, 1}));
    }

    // Test multisampled textures can be used in WriteTexture.
    TEST_F(QueueWriteTextureValidationTest, WriteToMultisampledTexture) {
        uint64_t dataSize =
            utils::RequiredBytesInCopy(256, 0, {2, 2, 1}, wgpu::TextureFormat::RGBA8Unorm);
        wgpu::Texture destination = Create2DTexture({2, 2, 1}, 1, wgpu::TextureFormat::RGBA8Unorm,
                                                    wgpu::TextureUsage::CopyDst, 4);

        ASSERT_DEVICE_ERROR(
            TestWriteTexture(dataSize, 0, 256, 0, destination, 0, {0, 0, 0}, {2, 2, 1}));
    }

    // Test WriteTexture with texture in error state causes errors.
    TEST_F(QueueWriteTextureValidationTest, TextureInErrorState) {
        wgpu::TextureDescriptor errorTextureDescriptor;
        errorTextureDescriptor.size.depth = 0;
        ASSERT_DEVICE_ERROR(wgpu::Texture errorTexture =
                                device.CreateTexture(&errorTextureDescriptor));
        wgpu::TextureCopyView errorTextureCopyView =
            utils::CreateTextureCopyView(errorTexture, 0, {0, 0, 0});

        wgpu::Extent3D extent3D = {1, 1, 1};

        {
            std::vector<uint8_t> data(4);
            wgpu::TextureDataLayout textureDataLayout;
            textureDataLayout.offset = 0;
            textureDataLayout.bytesPerRow = 0;
            textureDataLayout.rowsPerImage = 0;

            ASSERT_DEVICE_ERROR(queue.WriteTexture(&errorTextureCopyView, data.data(), 4,
                                                   &textureDataLayout, &extent3D));
        }
    }

    // Regression tests for a bug in the computation of texture data size in Dawn.
    TEST_F(QueueWriteTextureValidationTest, TextureWriteDataSizeLastRowComputation) {
        constexpr uint32_t kBytesPerRow = 256;
        constexpr uint32_t kWidth = 4;
        constexpr uint32_t kHeight = 4;

        constexpr std::array<wgpu::TextureFormat, 2> kFormats = {wgpu::TextureFormat::RGBA8Unorm,
                                                                 wgpu::TextureFormat::RG8Unorm};

        {
            // kBytesPerRow * (kHeight - 1) + kWidth is not large enough to be the valid data size
            // in this test because the data sizes in WriteTexture are not in texels but in bytes.
            constexpr uint32_t kInvalidDataSize = kBytesPerRow * (kHeight - 1) + kWidth;

            for (wgpu::TextureFormat format : kFormats) {
                wgpu::Texture destination =
                    Create2DTexture({kWidth, kHeight, 1}, 1, format, wgpu::TextureUsage::CopyDst);
                ASSERT_DEVICE_ERROR(TestWriteTexture(kInvalidDataSize, 0, kBytesPerRow, 0,
                                                     destination, 0, {0, 0, 0},
                                                     {kWidth, kHeight, 1}));
            }
        }

        {
            for (wgpu::TextureFormat format : kFormats) {
                uint32_t validDataSize =
                    utils::RequiredBytesInCopy(kBytesPerRow, 0, {kWidth, kHeight, 1}, format);
                wgpu::Texture destination =
                    Create2DTexture({kWidth, kHeight, 1}, 1, format, wgpu::TextureUsage::CopyDst);

                // Verify the return value of RequiredBytesInCopy() is exactly the minimum valid
                // data size in this test.
                {
                    uint32_t invalidDataSize = validDataSize - 1;
                    ASSERT_DEVICE_ERROR(TestWriteTexture(invalidDataSize, 0, kBytesPerRow, 0,
                                                         destination, 0, {0, 0, 0},
                                                         {kWidth, kHeight, 1}));
                }

                {
                    TestWriteTexture(validDataSize, 0, kBytesPerRow, 0, destination, 0, {0, 0, 0},
                                     {kWidth, kHeight, 1});
                }
            }
        }
    }

    // Test write from data to mip map of non square texture
    TEST_F(QueueWriteTextureValidationTest, WriteToMipmapOfNonSquareTexture) {
        uint64_t dataSize =
            utils::RequiredBytesInCopy(256, 0, {4, 2, 1}, wgpu::TextureFormat::RGBA8Unorm);
        uint32_t maxMipmapLevel = 3;
        wgpu::Texture destination =
            Create2DTexture({4, 2, 1}, maxMipmapLevel, wgpu::TextureFormat::RGBA8Unorm,
                            wgpu::TextureUsage::CopyDst);

        // Copy to top level mip map
        TestWriteTexture(dataSize, 0, 256, 0, destination, maxMipmapLevel - 1, {0, 0, 0},
                         {1, 1, 1});
        // Copy to high level mip map
        TestWriteTexture(dataSize, 0, 256, 0, destination, maxMipmapLevel - 2, {0, 0, 0},
                         {2, 1, 1});
        // Mip level out of range
        ASSERT_DEVICE_ERROR(TestWriteTexture(dataSize, 0, 256, 0, destination, maxMipmapLevel,
                                             {0, 0, 0}, {1, 1, 1}));
        // Copy origin out of range
        ASSERT_DEVICE_ERROR(TestWriteTexture(dataSize, 0, 256, 0, destination, maxMipmapLevel - 2,
                                             {1, 0, 0}, {2, 1, 1}));
        // Copy size out of range
        ASSERT_DEVICE_ERROR(TestWriteTexture(dataSize, 0, 256, 0, destination, maxMipmapLevel - 2,
                                             {0, 0, 0}, {2, 2, 1}));
    }

    // Test writes to multiple array layers of an uncompressed texture
    TEST_F(QueueWriteTextureValidationTest, WriteToMultipleArrayLayers) {
        wgpu::Texture destination = QueueWriteTextureValidationTest::Create2DTexture(
            {4, 2, 5}, 1, wgpu::TextureFormat::RGBA8Unorm,
            wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc);

        // Write to all array layers
        TestWriteTextureExactDataSize(256, 2, destination, wgpu::TextureFormat::RGBA8Unorm,
                                      {0, 0, 0}, {4, 2, 5});

        // Write to the highest array layer
        TestWriteTextureExactDataSize(256, 2, destination, wgpu::TextureFormat::RGBA8Unorm,
                                      {0, 0, 4}, {4, 2, 1});

        // Write to array layers in the middle
        TestWriteTextureExactDataSize(256, 2, destination, wgpu::TextureFormat::RGBA8Unorm,
                                      {0, 0, 1}, {4, 2, 3});

        // Copy with a non-packed rowsPerImage
        TestWriteTextureExactDataSize(256, 3, destination, wgpu::TextureFormat::RGBA8Unorm,
                                      {0, 0, 0}, {4, 2, 5});

        // Copy with bytesPerRow = 500
        TestWriteTextureExactDataSize(500, 2, destination, wgpu::TextureFormat::RGBA8Unorm,
                                      {0, 0, 1}, {4, 2, 3});
    }

    class WriteTextureTest_CompressedTextureFormats : public QueueWriteTextureValidationTest {
      public:
        WriteTextureTest_CompressedTextureFormats() : QueueWriteTextureValidationTest() {
            device = CreateDeviceFromAdapter(adapter, {"texture_compression_bc"});
        }

      protected:
        wgpu::Texture Create2DTexture(wgpu::TextureFormat format,
                                      uint32_t mipmapLevels = 1,
                                      uint32_t width = kWidth,
                                      uint32_t height = kHeight) {
            constexpr wgpu::TextureUsage kUsage = wgpu::TextureUsage::CopyDst;
            constexpr uint32_t kArrayLayers = 1;
            return QueueWriteTextureValidationTest::Create2DTexture(
                {width, height, kArrayLayers}, mipmapLevels, format, kUsage, 1);
        }

        void TestWriteTexture(size_t dataSize,
                              uint32_t dataOffset,
                              uint32_t dataBytesPerRow,
                              uint32_t dataRowsPerImage,
                              wgpu::Texture texture,
                              uint32_t textLevel,
                              wgpu::Origin3D textOrigin,
                              wgpu::Extent3D size) {
            QueueWriteTextureValidationTest::TestWriteTexture(dataSize, dataOffset, dataBytesPerRow,
                                                              dataRowsPerImage, texture, textLevel,
                                                              textOrigin, size);
        }

        static constexpr uint32_t kWidth = 16;
        static constexpr uint32_t kHeight = 16;
    };

    // Tests to verify that data offset must be a multiple of the compressed texture blocks in bytes
    TEST_F(WriteTextureTest_CompressedTextureFormats, DataOffset) {
        for (wgpu::TextureFormat bcFormat : utils::kBCFormats) {
            wgpu::Texture texture = Create2DTexture(bcFormat);

            // Valid usages of data offset.
            {
                uint32_t validDataOffset = utils::GetTexelBlockSizeInBytes(bcFormat);
                QueueWriteTextureValidationTest::TestWriteTexture(512, validDataOffset, 256, 4,
                                                                  texture, 0, {0, 0, 0}, {4, 4, 1});
            }

            // Failures on invalid data offset.
            {
                uint32_t kInvalidDataOffset = utils::GetTexelBlockSizeInBytes(bcFormat) / 2;
                ASSERT_DEVICE_ERROR(TestWriteTexture(512, kInvalidDataOffset, 256, 4, texture, 0,
                                                     {0, 0, 0}, {4, 4, 1}));
            }
        }
    }

    // Tests to verify that bytesPerRow must not be less than (width / blockWidth) *
    // blockSizeInBytes and that it doesn't have to be a multiple of the compressed
    // texture block width.
    TEST_F(WriteTextureTest_CompressedTextureFormats, BytesPerRow) {
        constexpr uint32_t kTestWidth = 160;
        constexpr uint32_t kTestHeight = 160;

        // Failures on the BytesPerRow that is not large enough.
        {
            constexpr uint32_t kSmallBytesPerRow = 256;
            for (wgpu::TextureFormat bcFormat : utils::kBCFormats) {
                wgpu::Texture texture = Create2DTexture(bcFormat, 1, kTestWidth, kTestHeight);
                ASSERT_DEVICE_ERROR(TestWriteTexture(1024, 0, kSmallBytesPerRow, 4, texture, 0,
                                                     {0, 0, 0}, {kTestWidth, 4, 1}));
            }
        }

        // Test it is valid to use a BytesPerRow that is not a multiple of 256.
        {
            for (wgpu::TextureFormat bcFormat : utils::kBCFormats) {
                wgpu::Texture texture = Create2DTexture(bcFormat, 1, kTestWidth, kTestHeight);
                uint32_t ValidBytesPerRow =
                    kTestWidth / 4 * utils::GetTexelBlockSizeInBytes(bcFormat);
                ASSERT_NE(0u, ValidBytesPerRow % 256);
                TestWriteTexture(1024, 0, ValidBytesPerRow, 4, texture, 0, {0, 0, 0},
                                 {kTestWidth, 4, 1});
            }
        }

        for (wgpu::TextureFormat bcFormat : utils::kBCFormats) {
            wgpu::Texture texture = Create2DTexture(bcFormat);

            // Valid usage of bytesPerRow in WriteTexture with compressed texture formats.
            {
                constexpr uint32_t kValidBytesPerRow = 20;
                TestWriteTexture(512, 0, kValidBytesPerRow, 0, texture, 0, {0, 0, 0}, {4, 4, 1});
            }

            // Valid bytesPerRow.
            // Note that image width is not a multiple of blockWidth.
            {
                constexpr uint32_t kValidBytesPerRow = 17;
                TestWriteTexture(512, 0, kValidBytesPerRow, 0, texture, 0, {0, 0, 0}, {4, 4, 1});
            }
        }
    }

    // Tests to verify that rowsPerImage must be a multiple of the compressed texture block height
    TEST_F(WriteTextureTest_CompressedTextureFormats, RowsPerImage) {
        for (wgpu::TextureFormat bcFormat : utils::kBCFormats) {
            wgpu::Texture texture = Create2DTexture(bcFormat);

            // Valid usages of rowsPerImage in WriteTexture with compressed texture formats.
            {
                constexpr uint32_t kValidRowsPerImage = 8;
                TestWriteTexture(512, 0, 256, kValidRowsPerImage, texture, 0, {0, 0, 0}, {4, 4, 1});
            }

            // 4 is the exact limit for rowsPerImage here.
            {
                constexpr uint32_t kValidRowsPerImage = 4;
                TestWriteTexture(512, 0, 256, kValidRowsPerImage, texture, 0, {0, 0, 0}, {4, 4, 1});
            }

            // Failure on invalid rowsPerImage.
            {
                constexpr uint32_t kInvalidRowsPerImage = 2;
                ASSERT_DEVICE_ERROR(TestWriteTexture(512, 0, 256, kInvalidRowsPerImage, texture, 0,
                                                     {0, 0, 0}, {4, 4, 1}));
            }
        }
    }

    // Tests to verify that ImageOffset.x must be a multiple of the compressed texture block width
    // and ImageOffset.y must be a multiple of the compressed texture block height
    TEST_F(WriteTextureTest_CompressedTextureFormats, ImageOffset) {
        for (wgpu::TextureFormat bcFormat : utils::kBCFormats) {
            wgpu::Texture texture = Create2DTexture(bcFormat);
            wgpu::Texture texture2 = Create2DTexture(bcFormat);

            constexpr wgpu::Origin3D kSmallestValidOrigin3D = {4, 4, 0};

            // Valid usages of ImageOffset in WriteTexture with compressed texture formats.
            { TestWriteTexture(512, 0, 256, 4, texture, 0, kSmallestValidOrigin3D, {4, 4, 1}); }

            // Failures on invalid ImageOffset.x.
            {
                constexpr wgpu::Origin3D kInvalidOrigin3D = {kSmallestValidOrigin3D.x - 1,
                                                             kSmallestValidOrigin3D.y, 0};
                ASSERT_DEVICE_ERROR(
                    TestWriteTexture(512, 0, 256, 4, texture, 0, kInvalidOrigin3D, {4, 4, 1}));
            }

            // Failures on invalid ImageOffset.y.
            {
                constexpr wgpu::Origin3D kInvalidOrigin3D = {kSmallestValidOrigin3D.x,
                                                             kSmallestValidOrigin3D.y - 1, 0};
                ASSERT_DEVICE_ERROR(
                    TestWriteTexture(512, 0, 256, 4, texture, 0, kInvalidOrigin3D, {4, 4, 1}));
            }
        }
    }

    // Tests to verify that ImageExtent.x must be a multiple of the compressed texture block width
    // and ImageExtent.y must be a multiple of the compressed texture block height
    TEST_F(WriteTextureTest_CompressedTextureFormats, ImageExtent) {
        constexpr uint32_t kMipmapLevels = 3;
        constexpr uint32_t kTestWidth = 60;
        constexpr uint32_t kTestHeight = 60;

        for (wgpu::TextureFormat bcFormat : utils::kBCFormats) {
            wgpu::Texture texture =
                Create2DTexture(bcFormat, kMipmapLevels, kTestWidth, kTestHeight);
            wgpu::Texture texture2 =
                Create2DTexture(bcFormat, kMipmapLevels, kTestWidth, kTestHeight);

            constexpr wgpu::Extent3D kSmallestValidExtent3D = {4, 4, 1};

            // Valid usages of ImageExtent in WriteTexture with compressed texture formats.
            { TestWriteTexture(512, 0, 256, 8, texture, 0, {0, 0, 0}, kSmallestValidExtent3D); }

            // Valid usages of ImageExtent in WriteTexture with compressed texture formats
            // and non-zero mipmap levels.
            {
                constexpr uint32_t kTestMipmapLevel = 2;
                constexpr wgpu::Origin3D kTestOrigin = {
                    (kTestWidth >> kTestMipmapLevel) - kSmallestValidExtent3D.width + 1,
                    (kTestHeight >> kTestMipmapLevel) - kSmallestValidExtent3D.height + 1, 0};

                TestWriteTexture(512, 0, 256, 4, texture, kTestMipmapLevel, kTestOrigin,
                                 kSmallestValidExtent3D);
            }

            // Failures on invalid ImageExtent.x.
            {
                constexpr wgpu::Extent3D kInValidExtent3D = {kSmallestValidExtent3D.width - 1,
                                                             kSmallestValidExtent3D.height, 1};
                ASSERT_DEVICE_ERROR(
                    TestWriteTexture(512, 0, 256, 4, texture, 0, {0, 0, 0}, kInValidExtent3D));
            }

            // Failures on invalid ImageExtent.y.
            {
                constexpr wgpu::Extent3D kInValidExtent3D = {kSmallestValidExtent3D.width,
                                                             kSmallestValidExtent3D.height - 1, 1};
                ASSERT_DEVICE_ERROR(
                    TestWriteTexture(512, 0, 256, 4, texture, 0, {0, 0, 0}, kInValidExtent3D));
            }
        }
    }

    // Test writes to multiple array layers of a compressed texture
    TEST_F(WriteTextureTest_CompressedTextureFormats, WriteToMultipleArrayLayers) {
        for (wgpu::TextureFormat bcFormat : utils::kBCFormats) {
            wgpu::Texture texture = QueueWriteTextureValidationTest::Create2DTexture(
                {12, 16, 20}, 1, bcFormat,
                wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc);

            // Write to all array layers
            TestWriteTextureExactDataSize(256, 16, texture, bcFormat, {0, 0, 0}, {12, 16, 20});

            // Write to the highest array layer
            TestWriteTextureExactDataSize(256, 16, texture, bcFormat, {0, 0, 19}, {12, 16, 1});

            // Write to array layers in the middle
            TestWriteTextureExactDataSize(256, 16, texture, bcFormat, {0, 0, 1}, {12, 16, 18});

            // Write touching the texture corners with a non-packed rowsPerImage
            TestWriteTextureExactDataSize(256, 24, texture, bcFormat, {4, 4, 4}, {8, 12, 16});

            // rowsPerImage needs to be a multiple of blockHeight
            ASSERT_DEVICE_ERROR(
                TestWriteTexture(8192, 0, 256, 6, texture, 0, {0, 0, 0}, {4, 4, 1}));

            // rowsPerImage must be a multiple of blockHeight even with an empty write
            ASSERT_DEVICE_ERROR(TestWriteTexture(0, 0, 256, 2, texture, 0, {0, 0, 0}, {0, 0, 0}));
        }
    }

}  // anonymous namespace
