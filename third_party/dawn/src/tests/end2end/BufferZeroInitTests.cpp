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

#include "tests/DawnTest.h"

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

namespace {

    struct BufferZeroInitInCopyT2BSpec {
        wgpu::Extent3D textureSize;
        uint64_t bufferOffset;
        uint64_t extraBytes;
        uint32_t bytesPerRow;
        uint32_t rowsPerImage;
        uint32_t lazyClearCount;
    };

}  // anonymous namespace

class BufferZeroInitTest : public DawnTest {
  public:
    wgpu::Buffer CreateBuffer(uint64_t size,
                              wgpu::BufferUsage usage,
                              bool mappedAtCreation = false) {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = size;
        descriptor.usage = usage;
        descriptor.mappedAtCreation = mappedAtCreation;
        return device.CreateBuffer(&descriptor);
    }

    void MapAsyncAndWait(wgpu::Buffer buffer,
                         wgpu::MapMode mapMode,
                         uint64_t offset,
                         uint64_t size) {
        ASSERT(mapMode == wgpu::MapMode::Read || mapMode == wgpu::MapMode::Write);

        bool done = false;
        buffer.MapAsync(
            mapMode, offset, size,
            [](WGPUBufferMapAsyncStatus status, void* userdata) {
                ASSERT_EQ(WGPUBufferMapAsyncStatus_Success, status);
                *static_cast<bool*>(userdata) = true;
            },
            &done);

        while (!done) {
            WaitABit();
        }
    }

    wgpu::Texture CreateAndInitializeTexture(const wgpu::Extent3D& size,
                                             wgpu::TextureFormat format,
                                             wgpu::Color color = {0.f, 0.f, 0.f, 0.f}) {
        wgpu::TextureDescriptor descriptor;
        descriptor.size = size;
        descriptor.format = format;
        descriptor.usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc |
                           wgpu::TextureUsage::OutputAttachment;
        wgpu::Texture texture = device.CreateTexture(&descriptor);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

        for (uint32_t arrayLayer = 0; arrayLayer < size.depth; ++arrayLayer) {
            wgpu::TextureViewDescriptor viewDescriptor;
            viewDescriptor.format = format;
            viewDescriptor.dimension = wgpu::TextureViewDimension::e2D;
            viewDescriptor.baseArrayLayer = arrayLayer;
            viewDescriptor.arrayLayerCount = 1u;

            utils::ComboRenderPassDescriptor renderPassDescriptor(
                {texture.CreateView(&viewDescriptor)});
            renderPassDescriptor.cColorAttachments[0].clearColor = color;
            wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDescriptor);
            renderPass.EndPass();
        }

        wgpu::CommandBuffer commandBuffer = encoder.Finish();
        queue.Submit(1, &commandBuffer);

        return texture;
    }

    void TestBufferZeroInitInCopyTextureToBuffer(const BufferZeroInitInCopyT2BSpec& spec) {
        constexpr wgpu::TextureFormat kTextureFormat = wgpu::TextureFormat::R32Float;
        ASSERT(utils::GetTexelBlockSizeInBytes(kTextureFormat) * spec.textureSize.width %
                   kTextureBytesPerRowAlignment ==
               0);

        constexpr wgpu::Color kClearColor = {0.5f, 0.5f, 0.5f, 0.5f};
        wgpu::Texture texture =
            CreateAndInitializeTexture(spec.textureSize, kTextureFormat, kClearColor);

        const wgpu::TextureCopyView textureCopyView =
            utils::CreateTextureCopyView(texture, 0, {0, 0, 0});

        const uint64_t bufferSize = spec.bufferOffset + spec.extraBytes +
                                    utils::RequiredBytesInCopy(spec.bytesPerRow, spec.rowsPerImage,
                                                               spec.textureSize, kTextureFormat);
        wgpu::BufferDescriptor bufferDescriptor;
        bufferDescriptor.size = bufferSize;
        bufferDescriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
        wgpu::Buffer buffer = device.CreateBuffer(&bufferDescriptor);
        const wgpu::BufferCopyView bufferCopyView = utils::CreateBufferCopyView(
            buffer, spec.bufferOffset, spec.bytesPerRow, spec.rowsPerImage);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyTextureToBuffer(&textureCopyView, &bufferCopyView, &spec.textureSize);
        wgpu::CommandBuffer commandBuffer = encoder.Finish();
        EXPECT_LAZY_CLEAR(spec.lazyClearCount, queue.Submit(1, &commandBuffer));

        const uint64_t expectedValueCount = bufferSize / sizeof(float);
        std::vector<float> expectedValues(expectedValueCount, 0.f);

        for (uint32_t slice = 0; slice < spec.textureSize.depth; ++slice) {
            const uint64_t baseOffsetBytesPerSlice =
                spec.bufferOffset + spec.bytesPerRow * spec.rowsPerImage * slice;
            for (uint32_t y = 0; y < spec.textureSize.height; ++y) {
                const uint64_t baseOffsetBytesPerRow =
                    baseOffsetBytesPerSlice + spec.bytesPerRow * y;
                const uint64_t baseOffsetFloatCountPerRow = baseOffsetBytesPerRow / sizeof(float);
                for (uint32_t x = 0; x < spec.textureSize.width; ++x) {
                    expectedValues[baseOffsetFloatCountPerRow + x] = 0.5f;
                }
            }
        }

        EXPECT_BUFFER_FLOAT_RANGE_EQ(expectedValues.data(), buffer, 0, expectedValues.size());
    }
};

// Test that calling writeBuffer to overwrite the entire buffer doesn't need to lazily initialize
// the destination buffer.
TEST_P(BufferZeroInitTest, WriteBufferToEntireBuffer) {
    constexpr uint32_t kBufferSize = 8u;
    constexpr wgpu::BufferUsage kBufferUsage =
        wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = CreateBuffer(kBufferSize, kBufferUsage);

    constexpr std::array<uint32_t, kBufferSize / sizeof(uint32_t)> kExpectedData = {
        {0x02020202u, 0x02020202u}};
    EXPECT_LAZY_CLEAR(0u, queue.WriteBuffer(buffer, 0, kExpectedData.data(), kBufferSize));

    EXPECT_BUFFER_U32_RANGE_EQ(kExpectedData.data(), buffer, 0, kBufferSize / sizeof(uint32_t));
}

// Test that calling writeBuffer to overwrite a part of buffer needs to lazily initialize the
// destination buffer.
TEST_P(BufferZeroInitTest, WriteBufferToSubBuffer) {
    constexpr uint32_t kBufferSize = 8u;
    constexpr wgpu::BufferUsage kBufferUsage =
        wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;

    constexpr uint32_t kCopyValue = 0x02020202u;

    // offset == 0
    {
        wgpu::Buffer buffer = CreateBuffer(kBufferSize, kBufferUsage);

        constexpr uint32_t kCopyOffset = 0u;
        EXPECT_LAZY_CLEAR(1u,
                          queue.WriteBuffer(buffer, kCopyOffset, &kCopyValue, sizeof(kCopyValue)));

        EXPECT_BUFFER_U32_EQ(kCopyValue, buffer, kCopyOffset);
        EXPECT_BUFFER_U32_EQ(0, buffer, kBufferSize - sizeof(kCopyValue));
    }

    // offset > 0
    {
        wgpu::Buffer buffer = CreateBuffer(kBufferSize, kBufferUsage);

        constexpr uint32_t kCopyOffset = 4u;
        EXPECT_LAZY_CLEAR(1u,
                          queue.WriteBuffer(buffer, kCopyOffset, &kCopyValue, sizeof(kCopyValue)));

        EXPECT_BUFFER_U32_EQ(0, buffer, 0);
        EXPECT_BUFFER_U32_EQ(kCopyValue, buffer, kCopyOffset);
    }
}

// Test that the code path of CopyBufferToBuffer clears the source buffer correctly when it is the
// first use of the source buffer.
TEST_P(BufferZeroInitTest, CopyBufferToBufferSource) {
    constexpr uint64_t kBufferSize = 16u;
    constexpr wgpu::BufferUsage kBufferUsage =
        wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    wgpu::BufferDescriptor bufferDescriptor;
    bufferDescriptor.size = kBufferSize;
    bufferDescriptor.usage = kBufferUsage;

    constexpr std::array<uint8_t, kBufferSize> kInitialData = {
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}};

    wgpu::Buffer dstBuffer =
        utils::CreateBufferFromData(device, kInitialData.data(), kBufferSize, kBufferUsage);

    constexpr std::array<uint32_t, kBufferSize / sizeof(uint32_t)> kExpectedData = {{0, 0, 0, 0}};

    // Full copy from the source buffer
    {
        wgpu::Buffer srcBuffer = device.CreateBuffer(&bufferDescriptor);
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(srcBuffer, 0, dstBuffer, 0, kBufferSize);
        wgpu::CommandBuffer commandBuffer = encoder.Finish();

        EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commandBuffer));
        EXPECT_BUFFER_U32_RANGE_EQ(kExpectedData.data(), srcBuffer, 0,
                                   kBufferSize / sizeof(uint32_t));
    }

    // Partial copy from the source buffer
    // srcOffset == 0
    {
        constexpr uint64_t kSrcOffset = 0;
        constexpr uint64_t kCopySize = kBufferSize / 2;

        wgpu::Buffer srcBuffer = device.CreateBuffer(&bufferDescriptor);
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(srcBuffer, kSrcOffset, dstBuffer, 0, kCopySize);
        wgpu::CommandBuffer commandBuffer = encoder.Finish();

        EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commandBuffer));
        EXPECT_BUFFER_U32_RANGE_EQ(kExpectedData.data(), srcBuffer, 0,
                                   kBufferSize / sizeof(uint32_t));
    }

    // srcOffset > 0 and srcOffset + copySize == srcBufferSize
    {
        constexpr uint64_t kSrcOffset = kBufferSize / 2;
        constexpr uint64_t kCopySize = kBufferSize - kSrcOffset;

        wgpu::Buffer srcBuffer = device.CreateBuffer(&bufferDescriptor);
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(srcBuffer, kSrcOffset, dstBuffer, 0, kCopySize);
        wgpu::CommandBuffer commandBuffer = encoder.Finish();

        EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commandBuffer));
        EXPECT_BUFFER_U32_RANGE_EQ(kExpectedData.data(), srcBuffer, 0,
                                   kBufferSize / sizeof(uint32_t));
    }

    // srcOffset > 0 and srcOffset + copySize < srcBufferSize
    {
        constexpr uint64_t kSrcOffset = kBufferSize / 4;
        constexpr uint64_t kCopySize = kBufferSize / 2;

        wgpu::Buffer srcBuffer = device.CreateBuffer(&bufferDescriptor);
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(srcBuffer, kSrcOffset, dstBuffer, 0, kCopySize);
        wgpu::CommandBuffer commandBuffer = encoder.Finish();

        EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commandBuffer));
        EXPECT_BUFFER_U32_RANGE_EQ(kExpectedData.data(), srcBuffer, 0,
                                   kBufferSize / sizeof(uint32_t));
    }
}

// Test that the code path of CopyBufferToBuffer clears the destination buffer correctly when it is
// the first use of the destination buffer.
TEST_P(BufferZeroInitTest, CopyBufferToBufferDestination) {
    constexpr uint64_t kBufferSize = 16u;
    constexpr wgpu::BufferUsage kBufferUsage =
        wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    wgpu::BufferDescriptor bufferDescriptor;
    bufferDescriptor.size = kBufferSize;
    bufferDescriptor.usage = kBufferUsage;

    const std::array<uint8_t, kBufferSize> kInitialData = {
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}};
    wgpu::Buffer srcBuffer =
        utils::CreateBufferFromData(device, kInitialData.data(), kBufferSize, kBufferUsage);

    // Full copy from the source buffer doesn't need lazy initialization at all.
    {
        wgpu::Buffer dstBuffer = device.CreateBuffer(&bufferDescriptor);
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(srcBuffer, 0, dstBuffer, 0, kBufferSize);
        wgpu::CommandBuffer commandBuffer = encoder.Finish();

        EXPECT_LAZY_CLEAR(0u, queue.Submit(1, &commandBuffer));

        EXPECT_BUFFER_U32_RANGE_EQ(reinterpret_cast<const uint32_t*>(kInitialData.data()),
                                   dstBuffer, 0, kBufferSize / sizeof(uint32_t));
    }

    // Partial copy from the source buffer needs lazy initialization.
    // offset == 0
    {
        constexpr uint32_t kDstOffset = 0;
        constexpr uint32_t kCopySize = kBufferSize / 2;

        wgpu::Buffer dstBuffer = device.CreateBuffer(&bufferDescriptor);
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(srcBuffer, 0, dstBuffer, kDstOffset, kCopySize);
        wgpu::CommandBuffer commandBuffer = encoder.Finish();

        EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commandBuffer));

        std::array<uint8_t, kBufferSize> expectedData;
        expectedData.fill(0);
        for (uint32_t index = kDstOffset; index < kDstOffset + kCopySize; ++index) {
            expectedData[index] = kInitialData[index - kDstOffset];
        }

        EXPECT_BUFFER_U32_RANGE_EQ(reinterpret_cast<uint32_t*>(expectedData.data()), dstBuffer, 0,
                                   kBufferSize / sizeof(uint32_t));
    }

    // offset > 0 and dstOffset + CopySize == kBufferSize
    {
        constexpr uint32_t kDstOffset = kBufferSize / 2;
        constexpr uint32_t kCopySize = kBufferSize - kDstOffset;

        wgpu::Buffer dstBuffer = device.CreateBuffer(&bufferDescriptor);
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(srcBuffer, 0, dstBuffer, kDstOffset, kCopySize);
        wgpu::CommandBuffer commandBuffer = encoder.Finish();

        EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commandBuffer));

        std::array<uint8_t, kBufferSize> expectedData;
        expectedData.fill(0);
        for (uint32_t index = kDstOffset; index < kDstOffset + kCopySize; ++index) {
            expectedData[index] = kInitialData[index - kDstOffset];
        }

        EXPECT_BUFFER_U32_RANGE_EQ(reinterpret_cast<uint32_t*>(expectedData.data()), dstBuffer, 0,
                                   kBufferSize / sizeof(uint32_t));
    }

    // offset > 0 and dstOffset + CopySize < kBufferSize
    {
        constexpr uint32_t kDstOffset = kBufferSize / 4;
        constexpr uint32_t kCopySize = kBufferSize / 2;

        wgpu::Buffer dstBuffer = device.CreateBuffer(&bufferDescriptor);
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(srcBuffer, 0, dstBuffer, kDstOffset, kCopySize);
        wgpu::CommandBuffer commandBuffer = encoder.Finish();

        EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commandBuffer));

        std::array<uint8_t, kBufferSize> expectedData;
        expectedData.fill(0);
        for (uint32_t index = kDstOffset; index < kDstOffset + kCopySize; ++index) {
            expectedData[index] = kInitialData[index - kDstOffset];
        }

        EXPECT_BUFFER_U32_RANGE_EQ(reinterpret_cast<uint32_t*>(expectedData.data()), dstBuffer, 0,
                                   kBufferSize / sizeof(uint32_t));
    }
}

// Test that the code path of readable buffer mapping clears the buffer correctly when it is the
// first use of the buffer.
TEST_P(BufferZeroInitTest, MapReadAsync) {
    constexpr uint32_t kBufferSize = 16u;
    constexpr wgpu::BufferUsage kBufferUsage =
        wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;

    constexpr wgpu::MapMode kMapMode = wgpu::MapMode::Read;

    // Map the whole buffer
    {
        wgpu::Buffer buffer = CreateBuffer(kBufferSize, kBufferUsage);
        EXPECT_LAZY_CLEAR(1u, MapAsyncAndWait(buffer, kMapMode, 0, kBufferSize));

        const uint32_t* mappedDataUint = static_cast<const uint32_t*>(buffer.GetConstMappedRange());
        for (uint32_t i = 0; i < kBufferSize / sizeof(uint32_t); ++i) {
            EXPECT_EQ(0u, mappedDataUint[i]);
        }
        buffer.Unmap();
    }

    // Map a range of a buffer
    {
        wgpu::Buffer buffer = CreateBuffer(kBufferSize, kBufferUsage);

        constexpr uint64_t kOffset = 4u;
        constexpr uint64_t kSize = 8u;
        EXPECT_LAZY_CLEAR(1u, MapAsyncAndWait(buffer, kMapMode, kOffset, kSize));

        const uint32_t* mappedDataUint =
            static_cast<const uint32_t*>(buffer.GetConstMappedRange(kOffset));
        for (uint32_t i = 0; i < kSize / sizeof(uint32_t); ++i) {
            EXPECT_EQ(0u, mappedDataUint[i]);
        }
        buffer.Unmap();

        EXPECT_LAZY_CLEAR(0u, MapAsyncAndWait(buffer, kMapMode, 0, kBufferSize));
        mappedDataUint = static_cast<const uint32_t*>(buffer.GetConstMappedRange());
        for (uint32_t i = 0; i < kBufferSize / sizeof(uint32_t); ++i) {
            EXPECT_EQ(0u, mappedDataUint[i]);
        }
        buffer.Unmap();
    }
}

// Test that the code path of writable buffer mapping clears the buffer correctly when it is the
// first use of the buffer.
TEST_P(BufferZeroInitTest, MapWriteAsync) {
    constexpr uint32_t kBufferSize = 16u;
    constexpr wgpu::BufferUsage kBufferUsage =
        wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc;

    constexpr wgpu::MapMode kMapMode = wgpu::MapMode::Write;

    constexpr std::array<uint32_t, kBufferSize / sizeof(uint32_t)> kExpectedData = {{0, 0, 0, 0}};

    // Map the whole buffer
    {
        wgpu::Buffer buffer = CreateBuffer(kBufferSize, kBufferUsage);
        EXPECT_LAZY_CLEAR(1u, MapAsyncAndWait(buffer, kMapMode, 0, kBufferSize));
        buffer.Unmap();

        EXPECT_BUFFER_U32_RANGE_EQ(reinterpret_cast<const uint32_t*>(kExpectedData.data()), buffer,
                                   0, kExpectedData.size());
    }

    // Map a range of a buffer
    {
        wgpu::Buffer buffer = CreateBuffer(kBufferSize, kBufferUsage);

        constexpr uint64_t kOffset = 4u;
        constexpr uint64_t kSize = 8u;
        EXPECT_LAZY_CLEAR(1u, MapAsyncAndWait(buffer, kMapMode, kOffset, kSize));
        buffer.Unmap();

        EXPECT_BUFFER_U32_RANGE_EQ(reinterpret_cast<const uint32_t*>(kExpectedData.data()), buffer,
                                   0, kExpectedData.size());
    }
}

// Test that the code path of creating a buffer with BufferDescriptor.mappedAtCreation == true
// clears the buffer correctly at the creation of the buffer.
TEST_P(BufferZeroInitTest, MapAtCreation) {
    constexpr uint32_t kBufferSize = 16u;
    constexpr wgpu::BufferUsage kBufferUsage =
        wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc;

    wgpu::Buffer buffer;
    EXPECT_LAZY_CLEAR(1u, buffer = CreateBuffer(kBufferSize, kBufferUsage, true));
    buffer.Unmap();

    constexpr std::array<uint32_t, kBufferSize / sizeof(uint32_t)> kExpectedData = {{0, 0, 0, 0}};
    EXPECT_BUFFER_U32_RANGE_EQ(reinterpret_cast<const uint32_t*>(kExpectedData.data()), buffer, 0,
                               kExpectedData.size());
}

// Test that the code path of CopyBufferToTexture clears the source buffer correctly when it is the
// first use of the buffer.
TEST_P(BufferZeroInitTest, CopyBufferToTexture) {
    constexpr wgpu::Extent3D kTextureSize = {16u, 16u, 1u};

    constexpr wgpu::TextureFormat kTextureFormat = wgpu::TextureFormat::R32Uint;

    wgpu::Texture texture = CreateAndInitializeTexture(kTextureSize, kTextureFormat);
    const wgpu::TextureCopyView textureCopyView =
        utils::CreateTextureCopyView(texture, 0, {0, 0, 0});

    const uint32_t requiredBufferSizeForCopy = utils::GetBytesInBufferTextureCopy(
        kTextureFormat, kTextureSize.width, kTextureBytesPerRowAlignment, kTextureSize.width,
        kTextureSize.depth);

    constexpr wgpu::BufferUsage kBufferUsage =
        wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;

    // bufferOffset == 0
    {
        constexpr uint64_t kOffset = 0;
        const uint32_t totalBufferSize = requiredBufferSizeForCopy + kOffset;
        wgpu::BufferDescriptor bufferDescriptor;
        bufferDescriptor.size = totalBufferSize;
        bufferDescriptor.usage = kBufferUsage;

        wgpu::Buffer buffer = device.CreateBuffer(&bufferDescriptor);
        const wgpu::BufferCopyView bufferCopyView = utils::CreateBufferCopyView(
            buffer, kOffset, kTextureBytesPerRowAlignment, kTextureSize.height);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &kTextureSize);
        wgpu::CommandBuffer commandBuffer = encoder.Finish();
        EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commandBuffer));

        std::vector<uint32_t> expectedValues(totalBufferSize / sizeof(uint32_t), 0);
        EXPECT_BUFFER_U32_RANGE_EQ(expectedValues.data(), buffer, 0,
                                   totalBufferSize / sizeof(uint32_t));
    }

    // bufferOffset > 0
    {
        constexpr uint64_t kOffset = 8u;
        const uint32_t totalBufferSize = requiredBufferSizeForCopy + kOffset;
        wgpu::BufferDescriptor bufferDescriptor;
        bufferDescriptor.size = totalBufferSize;
        bufferDescriptor.usage = kBufferUsage;

        wgpu::Buffer buffer = device.CreateBuffer(&bufferDescriptor);
        const wgpu::BufferCopyView bufferCopyView = utils::CreateBufferCopyView(
            buffer, kOffset, kTextureBytesPerRowAlignment, kTextureSize.height);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &kTextureSize);
        wgpu::CommandBuffer commandBuffer = encoder.Finish();
        EXPECT_LAZY_CLEAR(1u, queue.Submit(1, &commandBuffer));

        std::vector<uint32_t> expectedValues(totalBufferSize / sizeof(uint32_t), 0);
        EXPECT_BUFFER_U32_RANGE_EQ(expectedValues.data(), buffer, 0,
                                   totalBufferSize / sizeof(uint32_t));
    }
}

// Test that the code path of CopyTextureToBuffer clears the destination buffer correctly when it is
// the first use of the buffer and the texture is a 2D non-array texture.
TEST_P(BufferZeroInitTest, Copy2DTextureToBuffer) {
    constexpr wgpu::Extent3D kTextureSize = {64u, 8u, 1u};

    // bytesPerRow == texelBlockSizeInBytes * copySize.width && bytesPerRow * copySize.height ==
    // buffer.size
    {
        TestBufferZeroInitInCopyTextureToBuffer(
            {kTextureSize, 0u, 0u, kTextureBytesPerRowAlignment, kTextureSize.height, 0u});
    }

    // bytesPerRow > texelBlockSizeInBytes * copySize.width
    {
        constexpr uint64_t kBytesPerRow = kTextureBytesPerRowAlignment * 2;
        TestBufferZeroInitInCopyTextureToBuffer(
            {kTextureSize, 0u, 0u, kBytesPerRow, kTextureSize.height, 1u});
    }

    // bufferOffset > 0
    {
        constexpr uint64_t kBufferOffset = 16u;
        TestBufferZeroInitInCopyTextureToBuffer({kTextureSize, kBufferOffset, 0u,
                                                 kTextureBytesPerRowAlignment, kTextureSize.height,
                                                 1u});
    }

    // bytesPerRow * copySize.height < buffer.size
    {
        constexpr uint64_t kExtraBufferSize = 16u;
        TestBufferZeroInitInCopyTextureToBuffer({kTextureSize, 0u, kExtraBufferSize,
                                                 kTextureBytesPerRowAlignment, kTextureSize.height,
                                                 1u});
    }
}

// Test that the code path of CopyTextureToBuffer clears the destination buffer correctly when it is
// the first use of the buffer and the texture is a 2D array texture.
TEST_P(BufferZeroInitTest, Copy2DArrayTextureToBuffer) {
    constexpr wgpu::Extent3D kTextureSize = {64u, 4u, 3u};

    // bytesPerRow == texelBlockSizeInBytes * copySize.width && rowsPerImage == copySize.height &&
    // bytesPerRow * (rowsPerImage * (copySize.depth - 1) + copySize.height) == buffer.size
    {
        TestBufferZeroInitInCopyTextureToBuffer(
            {kTextureSize, 0u, 0u, kTextureBytesPerRowAlignment, kTextureSize.height, 0u});
    }

    // rowsPerImage > copySize.height
    {
        constexpr uint64_t kRowsPerImage = kTextureSize.height + 1u;
        TestBufferZeroInitInCopyTextureToBuffer(
            {kTextureSize, 0u, 0u, kTextureBytesPerRowAlignment, kRowsPerImage, 1u});
    }

    // bytesPerRow * rowsPerImage * copySize.depth < buffer.size
    {
        constexpr uint64_t kExtraBufferSize = 16u;
        TestBufferZeroInitInCopyTextureToBuffer({kTextureSize, 0u, kExtraBufferSize,
                                                 kTextureBytesPerRowAlignment, kTextureSize.height,
                                                 1u});
    }
}

DAWN_INSTANTIATE_TEST(BufferZeroInitTest,
                      D3D12Backend({"nonzero_clear_resources_on_creation_for_testing",
                                    "lazy_clear_buffer_on_first_use"}),
                      MetalBackend({"nonzero_clear_resources_on_creation_for_testing",
                                    "lazy_clear_buffer_on_first_use"}),
                      OpenGLBackend({"nonzero_clear_resources_on_creation_for_testing",
                                     "lazy_clear_buffer_on_first_use"}),
                      VulkanBackend({"nonzero_clear_resources_on_creation_for_testing",
                                     "lazy_clear_buffer_on_first_use"}));
