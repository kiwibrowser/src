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

// This file contains test for deprecated parts of Dawn's API while following WebGPU's evolution.
// It contains test for the "old" behavior that will be deleted once users are migrated, tests that
// a deprecation warning is emitted when the "old" behavior is used, and tests that an error is
// emitted when both the old and the new behavior are used (when applicable).

#include "tests/DawnTest.h"

#include "common/Math.h"
#include "utils/TextureFormatUtils.h"
#include "utils/WGPUHelpers.h"

class QueueTests : public DawnTest {};

// Test that GetDefaultQueue always returns the same object.
TEST_P(QueueTests, GetDefaultQueueSameObject) {
    wgpu::Queue q1 = device.GetDefaultQueue();
    wgpu::Queue q2 = device.GetDefaultQueue();
    EXPECT_EQ(q1.Get(), q2.Get());
}

DAWN_INSTANTIATE_TEST(QueueTests,
                      D3D12Backend(),
                      MetalBackend(),
                      NullBackend(),
                      OpenGLBackend(),
                      VulkanBackend());

class QueueWriteBufferTests : public DawnTest {};

// Test the simplest WriteBuffer setting one u32 at offset 0.
TEST_P(QueueWriteBufferTests, SmallDataAtZero) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    uint32_t value = 0x01020304;
    queue.WriteBuffer(buffer, 0, &value, sizeof(value));

    EXPECT_BUFFER_U32_EQ(value, buffer, 0);
}

// Test an empty WriteBuffer
TEST_P(QueueWriteBufferTests, ZeroSized) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    uint32_t initialValue = 0x42;
    queue.WriteBuffer(buffer, 0, &initialValue, sizeof(initialValue));

    queue.WriteBuffer(buffer, 0, nullptr, 0);

    // The content of the buffer isn't changed
    EXPECT_BUFFER_U32_EQ(initialValue, buffer, 0);
}

// Call WriteBuffer at offset 0 via a u32 twice. Test that data is updated accoordingly.
TEST_P(QueueWriteBufferTests, SetTwice) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    uint32_t value = 0x01020304;
    queue.WriteBuffer(buffer, 0, &value, sizeof(value));

    EXPECT_BUFFER_U32_EQ(value, buffer, 0);

    value = 0x05060708;
    queue.WriteBuffer(buffer, 0, &value, sizeof(value));

    EXPECT_BUFFER_U32_EQ(value, buffer, 0);
}

// Test that WriteBuffer offset works.
TEST_P(QueueWriteBufferTests, SmallDataAtOffset) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4000;
    descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    constexpr uint64_t kOffset = 2000;
    uint32_t value = 0x01020304;
    queue.WriteBuffer(buffer, kOffset, &value, sizeof(value));

    EXPECT_BUFFER_U32_EQ(value, buffer, kOffset);
}

// Stress test for many calls to WriteBuffer
TEST_P(QueueWriteBufferTests, ManyWriteBuffer) {
    // Note: Increasing the size of the buffer will likely cause timeout issues.
    // In D3D12, timeout detection occurs when the GPU scheduler tries but cannot preempt the task
    // executing these commands in-flight. If this takes longer than ~2s, a device reset occurs and
    // fails the test. Since GPUs may or may not complete by then, this test must be disabled OR
    // modified to be well-below the timeout limit.

    // TODO (jiawei.shao@intel.com): find out why this test fails on Intel Vulkan Linux bots.
    DAWN_SKIP_TEST_IF(IsIntel() && IsVulkan() && IsLinux());
    // TODO(https://bugs.chromium.org/p/dawn/issues/detail?id=228): Re-enable
    // once the issue with Metal on 10.14.6 is fixed.
    DAWN_SKIP_TEST_IF(IsMacOS() && IsIntel() && IsMetal());

    constexpr uint64_t kSize = 4000 * 1000;
    constexpr uint32_t kElements = 250 * 250;
    wgpu::BufferDescriptor descriptor;
    descriptor.size = kSize;
    descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    std::vector<uint32_t> expectedData;
    for (uint32_t i = 0; i < kElements; ++i) {
        queue.WriteBuffer(buffer, i * sizeof(uint32_t), &i, sizeof(i));
        expectedData.push_back(i);
    }

    EXPECT_BUFFER_U32_RANGE_EQ(expectedData.data(), buffer, 0, kElements);
}

// Test using WriteBuffer for lots of data
TEST_P(QueueWriteBufferTests, LargeWriteBuffer) {
    constexpr uint64_t kSize = 4000 * 1000;
    constexpr uint32_t kElements = 1000 * 1000;
    wgpu::BufferDescriptor descriptor;
    descriptor.size = kSize;
    descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    std::vector<uint32_t> expectedData;
    for (uint32_t i = 0; i < kElements; ++i) {
        expectedData.push_back(i);
    }

    queue.WriteBuffer(buffer, 0, expectedData.data(), kElements * sizeof(uint32_t));

    EXPECT_BUFFER_U32_RANGE_EQ(expectedData.data(), buffer, 0, kElements);
}

// Test using WriteBuffer for super large data block
TEST_P(QueueWriteBufferTests, SuperLargeWriteBuffer) {
    constexpr uint64_t kSize = 12000 * 1000;
    constexpr uint64_t kElements = 3000 * 1000;
    wgpu::BufferDescriptor descriptor;
    descriptor.size = kSize;
    descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    std::vector<uint32_t> expectedData;
    for (uint32_t i = 0; i < kElements; ++i) {
        expectedData.push_back(i);
    }

    queue.WriteBuffer(buffer, 0, expectedData.data(), kElements * sizeof(uint32_t));

    EXPECT_BUFFER_U32_RANGE_EQ(expectedData.data(), buffer, 0, kElements);
}

DAWN_INSTANTIATE_TEST(QueueWriteBufferTests,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());

class QueueWriteTextureTests : public DawnTest {
  protected:
    static constexpr wgpu::TextureFormat kTextureFormat = wgpu::TextureFormat::RGBA8Unorm;

    struct TextureSpec {
        wgpu::Origin3D copyOrigin;
        wgpu::Extent3D textureSize;
        uint32_t level;
    };

    struct DataSpec {
        uint64_t size;
        uint64_t offset;
        uint32_t bytesPerRow;
        uint32_t rowsPerImage;
    };

    static DataSpec MinimumDataSpec(wgpu::Extent3D writeSize,
                                    uint32_t bytesPerRow = 0,
                                    uint32_t rowsPerImage = 0) {
        if (bytesPerRow == 0) {
            bytesPerRow = writeSize.width * utils::GetTexelBlockSizeInBytes(kTextureFormat);
        }
        if (rowsPerImage == 0) {
            rowsPerImage = writeSize.height;
        }
        uint32_t totalDataSize =
            utils::RequiredBytesInCopy(bytesPerRow, rowsPerImage, writeSize, kTextureFormat);
        return {totalDataSize, 0, bytesPerRow, rowsPerImage};
    }

    static void PackTextureData(const uint8_t* srcData,
                                uint32_t width,
                                uint32_t height,
                                uint32_t srcBytesPerRow,
                                RGBA8* dstData,
                                uint32_t dstTexelPerRow,
                                uint32_t texelBlockSize) {
        for (uint64_t y = 0; y < height; ++y) {
            for (uint64_t x = 0; x < width; ++x) {
                uint64_t src = x * texelBlockSize + y * srcBytesPerRow;
                uint64_t dst = x + y * dstTexelPerRow;

                dstData[dst] = {srcData[src], srcData[src + 1], srcData[src + 2], srcData[src + 3]};
            }
        }
    }

    static void FillData(uint8_t* data, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            data[i] = static_cast<uint8_t>(i % 253);
        }
    }

    void DoTest(const TextureSpec& textureSpec,
                const DataSpec& dataSpec,
                const wgpu::Extent3D& copySize) {
        // Create data of size `size` and populate it
        std::vector<uint8_t> data(dataSpec.size);
        FillData(data.data(), data.size());

        // Create a texture that is `width` x `height` with (`level` + 1) mip levels.
        wgpu::TextureDescriptor descriptor = {};
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size = textureSpec.textureSize;
        descriptor.format = kTextureFormat;
        descriptor.mipLevelCount = textureSpec.level + 1;
        descriptor.usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc;
        wgpu::Texture texture = device.CreateTexture(&descriptor);

        wgpu::TextureDataLayout textureDataLayout = utils::CreateTextureDataLayout(
            dataSpec.offset, dataSpec.bytesPerRow, dataSpec.rowsPerImage);

        wgpu::TextureCopyView textureCopyView =
            utils::CreateTextureCopyView(texture, textureSpec.level, textureSpec.copyOrigin);

        queue.WriteTexture(&textureCopyView, data.data(), dataSpec.size, &textureDataLayout,
                           &copySize);

        const uint32_t bytesPerTexel = utils::GetTexelBlockSizeInBytes(kTextureFormat);
        wgpu::Extent3D mipSize = {textureSpec.textureSize.width >> textureSpec.level,
                                  textureSpec.textureSize.height >> textureSpec.level,
                                  textureSpec.textureSize.depth};
        uint32_t alignedBytesPerRow = Align(dataSpec.bytesPerRow, bytesPerTexel);
        uint32_t appliedRowsPerImage =
            dataSpec.rowsPerImage > 0 ? dataSpec.rowsPerImage : mipSize.height;
        uint32_t bytesPerImage = dataSpec.bytesPerRow * appliedRowsPerImage;

        const uint32_t maxArrayLayer = textureSpec.copyOrigin.z + copySize.depth;

        uint64_t dataOffset = dataSpec.offset;
        const uint32_t texelCountLastLayer =
            (alignedBytesPerRow / bytesPerTexel) * (mipSize.height - 1) + mipSize.width;
        for (uint32_t slice = textureSpec.copyOrigin.z; slice < maxArrayLayer; ++slice) {
            // Pack the data in the specified copy region to have the same
            // format as the expected texture data.
            std::vector<RGBA8> expected(texelCountLastLayer);
            PackTextureData(&data[dataOffset], copySize.width, copySize.height,
                            dataSpec.bytesPerRow, expected.data(), copySize.width, bytesPerTexel);

            EXPECT_TEXTURE_RGBA8_EQ(expected.data(), texture, textureSpec.copyOrigin.x,
                                    textureSpec.copyOrigin.y, copySize.width, copySize.height,
                                    textureSpec.level, slice)
                << "Write to texture failed copying " << dataSpec.size << "-byte data with offset "
                << dataSpec.offset << " and bytes per row " << dataSpec.bytesPerRow << " to [("
                << textureSpec.copyOrigin.x << ", " << textureSpec.copyOrigin.y << "), ("
                << textureSpec.copyOrigin.x + copySize.width << ", "
                << textureSpec.copyOrigin.y + copySize.height << ")) region of "
                << textureSpec.textureSize.width << " x " << textureSpec.textureSize.height
                << " texture at mip level " << textureSpec.level << " layer " << slice << std::endl;

            dataOffset += bytesPerImage;
        }
    }
};

// Test writing the whole texture for varying texture sizes.
TEST_P(QueueWriteTextureTests, VaryingTextureSize) {
    DAWN_SKIP_TEST_IF(IsSwiftshader());

    for (unsigned int w : {127, 128}) {
        for (unsigned int h : {63, 64}) {
            for (unsigned int d : {1, 3, 4}) {
                TextureSpec textureSpec;
                textureSpec.textureSize = {w, h, d};
                textureSpec.copyOrigin = {0, 0, 0};
                textureSpec.level = 0;

                DoTest(textureSpec, MinimumDataSpec({w, h, d}), {w, h, d});
            }
        }
    }
}

// Test writing a pixel with an offset.
TEST_P(QueueWriteTextureTests, VaryingTextureOffset) {
    constexpr uint32_t kWidth = 259;
    constexpr uint32_t kHeight = 127;
    DataSpec pixelData = MinimumDataSpec({1, 1, 1});

    constexpr wgpu::Extent3D kCopySize = {1, 1, 1};
    constexpr wgpu::Extent3D kTextureSize = {kWidth, kHeight, 1};
    TextureSpec defaultTextureSpec;
    defaultTextureSpec.textureSize = kTextureSize;
    defaultTextureSpec.level = 0;

    for (unsigned int w : {0u, kWidth / 7, kWidth / 3, kWidth - 1}) {
        for (unsigned int h : {0u, kHeight / 7, kHeight / 3, kHeight - 1}) {
            TextureSpec textureSpec = defaultTextureSpec;
            textureSpec.copyOrigin = {w, h, 0};
            DoTest(textureSpec, pixelData, kCopySize);
        }
    }
}

// Test writing a pixel with an offset to a texture array
TEST_P(QueueWriteTextureTests, VaryingTextureArrayOffset) {
    constexpr uint32_t kWidth = 259;
    constexpr uint32_t kHeight = 127;
    constexpr uint32_t kDepth = 62;
    DataSpec pixelData = MinimumDataSpec({1, 1, 1});

    constexpr wgpu::Extent3D kCopySize = {1, 1, 1};
    constexpr wgpu::Extent3D kTextureSize = {kWidth, kHeight, kDepth};
    TextureSpec defaultTextureSpec;
    defaultTextureSpec.textureSize = kTextureSize;
    defaultTextureSpec.level = 0;

    for (unsigned int w : {0u, kWidth / 7, kWidth / 3, kWidth - 1}) {
        for (unsigned int h : {0u, kHeight / 7, kHeight / 3, kHeight - 1}) {
            for (unsigned int d : {0u, kDepth / 7, kDepth / 3, kDepth - 1}) {
                TextureSpec textureSpec = defaultTextureSpec;
                textureSpec.copyOrigin = {w, h, d};
                DoTest(textureSpec, pixelData, kCopySize);
            }
        }
    }
}

// Test writing with varying write sizes.
TEST_P(QueueWriteTextureTests, VaryingWriteSize) {
    constexpr uint32_t kWidth = 257;
    constexpr uint32_t kHeight = 127;
    for (unsigned int w : {13, 63, 128, 256}) {
        for (unsigned int h : {16, 19, 32, 63}) {
            TextureSpec textureSpec;
            textureSpec.copyOrigin = {0, 0, 0};
            textureSpec.level = 0;
            textureSpec.textureSize = {kWidth, kHeight, 1};
            DoTest(textureSpec, MinimumDataSpec({w, h, 1}), {w, h, 1});
        }
    }
}

// Test writing with varying write sizes to texture arrays.
TEST_P(QueueWriteTextureTests, VaryingArrayWriteSize) {
    constexpr uint32_t kWidth = 257;
    constexpr uint32_t kHeight = 127;
    constexpr uint32_t kDepth = 65;
    for (unsigned int w : {13, 63, 128, 256}) {
        for (unsigned int h : {16, 19, 32, 63}) {
            for (unsigned int d : {3, 6}) {
                TextureSpec textureSpec;
                textureSpec.copyOrigin = {0, 0, 0};
                textureSpec.level = 0;
                textureSpec.textureSize = {kWidth, kHeight, kDepth};
                DoTest(textureSpec, MinimumDataSpec({w, h, d}), {w, h, d});
            }
        }
    }
}

// Test writing to varying mips
TEST_P(QueueWriteTextureTests, TextureWriteToMip) {
    constexpr uint32_t kWidth = 259;
    constexpr uint32_t kHeight = 127;

    TextureSpec defaultTextureSpec;
    defaultTextureSpec.copyOrigin = {0, 0, 0};
    defaultTextureSpec.textureSize = {kWidth, kHeight, 1};

    for (unsigned int i = 1; i < 4; ++i) {
        TextureSpec textureSpec = defaultTextureSpec;
        textureSpec.level = i;
        DoTest(textureSpec, MinimumDataSpec({kWidth >> i, kHeight >> i, 1}),
               {kWidth >> i, kHeight >> i, 1});
    }
}

// Test writing with different multiples of texel block size as data offset
TEST_P(QueueWriteTextureTests, VaryingDataOffset) {
    constexpr uint32_t kWidth = 259;
    constexpr uint32_t kHeight = 127;

    TextureSpec textureSpec;
    textureSpec.copyOrigin = {0, 0, 0};
    textureSpec.textureSize = {kWidth, kHeight, 1};
    textureSpec.level = 0;

    for (unsigned int i : {1, 2, 4, 17, 64, 128, 300}) {
        DataSpec dataSpec = MinimumDataSpec({kWidth, kHeight, 1});
        uint64_t offset = i * utils::GetTexelBlockSizeInBytes(kTextureFormat);
        dataSpec.size += offset;
        dataSpec.offset += offset;
        DoTest(textureSpec, dataSpec, {kWidth, kHeight, 1});
    }
}

// Test writing with rowsPerImage greater than needed.
TEST_P(QueueWriteTextureTests, VaryingRowsPerImage) {
    constexpr uint32_t kWidth = 65;
    constexpr uint32_t kHeight = 31;
    constexpr uint32_t kDepth = 17;

    constexpr wgpu::Extent3D copySize = {kWidth - 1, kHeight - 1, kDepth - 1};

    for (unsigned int r : {1, 2, 3, 64, 200}) {
        TextureSpec textureSpec;
        textureSpec.copyOrigin = {1, 1, 1};
        textureSpec.textureSize = {kWidth, kHeight, kDepth};
        textureSpec.level = 0;

        DataSpec dataSpec = MinimumDataSpec(copySize, 0, copySize.height + r);
        DoTest(textureSpec, dataSpec, copySize);
    }
}

// Test with bytesPerRow greater than needed
TEST_P(QueueWriteTextureTests, VaryingBytesPerRow) {
    constexpr uint32_t kWidth = 257;
    constexpr uint32_t kHeight = 129;

    TextureSpec textureSpec;
    textureSpec.textureSize = {kWidth, kHeight, 1};
    textureSpec.copyOrigin = {1, 2, 0};
    textureSpec.level = 0;

    constexpr wgpu::Extent3D copyExtent = {17, 19, 1};

    for (unsigned int b : {1, 2, 3, 4}) {
        uint32_t bytesPerRow =
            copyExtent.width * utils::GetTexelBlockSizeInBytes(kTextureFormat) + b;
        DoTest(textureSpec, MinimumDataSpec(copyExtent, bytesPerRow, 0), copyExtent);
    }
}

// Test with bytesPerRow greater than needed in a write to a texture array.
TEST_P(QueueWriteTextureTests, VaryingArrayBytesPerRow) {
    constexpr uint32_t kWidth = 257;
    constexpr uint32_t kHeight = 129;
    constexpr uint32_t kLayers = 65;

    TextureSpec textureSpec;
    textureSpec.textureSize = {kWidth, kHeight, kLayers};
    textureSpec.copyOrigin = {1, 2, 3};
    textureSpec.level = 0;

    constexpr wgpu::Extent3D copyExtent = {17, 19, 21};

    // Test with bytesPerRow divisible by blockWidth
    for (unsigned int b : {1, 2, 3, 65, 300}) {
        uint32_t bytesPerRow =
            (copyExtent.width + b) * utils::GetTexelBlockSizeInBytes(kTextureFormat);
        uint32_t rowsPerImage = 23;
        DoTest(textureSpec, MinimumDataSpec(copyExtent, bytesPerRow, rowsPerImage), copyExtent);
    }

    // Test with bytesPerRow not divisible by blockWidth
    for (unsigned int b : {1, 2, 3, 19, 301}) {
        uint32_t bytesPerRow =
            copyExtent.width * utils::GetTexelBlockSizeInBytes(kTextureFormat) + b;
        uint32_t rowsPerImage = 23;
        DoTest(textureSpec, MinimumDataSpec(copyExtent, bytesPerRow, rowsPerImage), copyExtent);
    }
}

DAWN_INSTANTIATE_TEST(QueueWriteTextureTests, MetalBackend(), VulkanBackend());
