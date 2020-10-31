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

#include <cstring>

class BufferMapReadTests : public DawnTest {
  protected:
    static void MapReadCallback(WGPUBufferMapAsyncStatus status,
                                const void* data,
                                uint64_t,
                                void* userdata) {
        ASSERT_EQ(WGPUBufferMapAsyncStatus_Success, status);
        ASSERT_NE(nullptr, data);

        static_cast<BufferMapReadTests*>(userdata)->mappedData = data;
    }

    const void* MapReadAsyncAndWait(const wgpu::Buffer& buffer) {
        buffer.MapReadAsync(MapReadCallback, this);

        while (mappedData == nullptr) {
            WaitABit();
        }

        return mappedData;
    }

    void UnmapBuffer(const wgpu::Buffer& buffer) {
        buffer.Unmap();
        mappedData = nullptr;
    }

  private:
    const void* mappedData = nullptr;
};

// Test that the simplest map read works.
TEST_P(BufferMapReadTests, SmallReadAtZero) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    uint32_t myData = 0x01020304;
    queue.WriteBuffer(buffer, 0, &myData, sizeof(myData));

    const void* mappedData = MapReadAsyncAndWait(buffer);
    ASSERT_EQ(myData, *reinterpret_cast<const uint32_t*>(mappedData));

    UnmapBuffer(buffer);
}

// Map, read and unmap twice. Test that both of these two iterations work.
TEST_P(BufferMapReadTests, MapTwice) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    uint32_t myData = 0x01020304;
    queue.WriteBuffer(buffer, 0, &myData, sizeof(myData));

    const void* mappedData = MapReadAsyncAndWait(buffer);
    EXPECT_EQ(myData, *reinterpret_cast<const uint32_t*>(mappedData));

    UnmapBuffer(buffer);

    myData = 0x05060708;
    queue.WriteBuffer(buffer, 0, &myData, sizeof(myData));

    const void* mappedData1 = MapReadAsyncAndWait(buffer);
    EXPECT_EQ(myData, *reinterpret_cast<const uint32_t*>(mappedData1));

    UnmapBuffer(buffer);
}

// Test mapping a large buffer.
TEST_P(BufferMapReadTests, LargeRead) {
    constexpr uint32_t kDataSize = 1000 * 1000;
    std::vector<uint32_t> myData;
    for (uint32_t i = 0; i < kDataSize; ++i) {
        myData.push_back(i);
    }

    wgpu::BufferDescriptor descriptor;
    descriptor.size = static_cast<uint32_t>(kDataSize * sizeof(uint32_t));
    descriptor.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    queue.WriteBuffer(buffer, 0, myData.data(), kDataSize * sizeof(uint32_t));

    const void* mappedData = MapReadAsyncAndWait(buffer);
    ASSERT_EQ(0, memcmp(mappedData, myData.data(), kDataSize * sizeof(uint32_t)));

    UnmapBuffer(buffer);
}

// Test mapping a zero-sized buffer.
TEST_P(BufferMapReadTests, ZeroSized) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 0;
    descriptor.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    MapReadAsyncAndWait(buffer);
    UnmapBuffer(buffer);
}

// Test the result of GetMappedRange when mapped for reading.
TEST_P(BufferMapReadTests, GetMappedRange) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    const void* mappedData = MapReadAsyncAndWait(buffer);
    ASSERT_EQ(buffer.GetConstMappedRange(), mappedData);
    ASSERT_NE(buffer.GetConstMappedRange(), nullptr);
    UnmapBuffer(buffer);
}

// Test the result of GetMappedRange when mapped for reading for a zero-sized buffer.
TEST_P(BufferMapReadTests, GetMappedRangeZeroSized) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 0;
    descriptor.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    const void* mappedData = MapReadAsyncAndWait(buffer);
    ASSERT_EQ(buffer.GetConstMappedRange(), mappedData);
    ASSERT_NE(buffer.GetConstMappedRange(), nullptr);
    UnmapBuffer(buffer);
}

DAWN_INSTANTIATE_TEST(BufferMapReadTests,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());

class BufferMapWriteTests : public DawnTest {
  protected:
    static void MapWriteCallback(WGPUBufferMapAsyncStatus status,
                                 void* data,
                                 uint64_t,
                                 void* userdata) {
        ASSERT_EQ(WGPUBufferMapAsyncStatus_Success, status);
        ASSERT_NE(nullptr, data);

        static_cast<BufferMapWriteTests*>(userdata)->mappedData = data;
    }

    void* MapWriteAsyncAndWait(const wgpu::Buffer& buffer) {
        buffer.MapWriteAsync(MapWriteCallback, this);

        while (mappedData == nullptr) {
            WaitABit();
        }

        // Ensure the prior write's status is updated.
        void* resultPointer = mappedData;
        mappedData = nullptr;

        return resultPointer;
    }

    void UnmapBuffer(const wgpu::Buffer& buffer) {
        buffer.Unmap();
        mappedData = nullptr;
    }

  private:
    void* mappedData = nullptr;
};

// Test that the simplest map write works.
TEST_P(BufferMapWriteTests, SmallWriteAtZero) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    uint32_t myData = 2934875;
    void* mappedData = MapWriteAsyncAndWait(buffer);
    memcpy(mappedData, &myData, sizeof(myData));
    UnmapBuffer(buffer);

    EXPECT_BUFFER_U32_EQ(myData, buffer, 0);
}

// Map, write and unmap twice. Test that both of these two iterations work.
TEST_P(BufferMapWriteTests, MapTwice) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    uint32_t myData = 2934875;
    void* mappedData = MapWriteAsyncAndWait(buffer);
    memcpy(mappedData, &myData, sizeof(myData));
    UnmapBuffer(buffer);

    EXPECT_BUFFER_U32_EQ(myData, buffer, 0);

    myData = 9999999;
    void* mappedData1 = MapWriteAsyncAndWait(buffer);
    memcpy(mappedData1, &myData, sizeof(myData));
    UnmapBuffer(buffer);

    EXPECT_BUFFER_U32_EQ(myData, buffer, 0);
}

// Test mapping a large buffer.
TEST_P(BufferMapWriteTests, LargeWrite) {
    constexpr uint32_t kDataSize = 1000 * 1000;
    std::vector<uint32_t> myData;
    for (uint32_t i = 0; i < kDataSize; ++i) {
        myData.push_back(i);
    }

    wgpu::BufferDescriptor descriptor;
    descriptor.size = static_cast<uint32_t>(kDataSize * sizeof(uint32_t));
    descriptor.usage = wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    void* mappedData = MapWriteAsyncAndWait(buffer);
    memcpy(mappedData, myData.data(), kDataSize * sizeof(uint32_t));
    UnmapBuffer(buffer);

    EXPECT_BUFFER_U32_RANGE_EQ(myData.data(), buffer, 0, kDataSize);
}

// Test mapping a zero-sized buffer.
TEST_P(BufferMapWriteTests, ZeroSized) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 0;
    descriptor.usage = wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    MapWriteAsyncAndWait(buffer);
    UnmapBuffer(buffer);
}

// Stress test mapping many buffers.
TEST_P(BufferMapWriteTests, ManyWrites) {
    constexpr uint32_t kDataSize = 1000;
    std::vector<uint32_t> myData;
    for (uint32_t i = 0; i < kDataSize; ++i) {
        myData.push_back(i);
    }

    std::vector<wgpu::Buffer> buffers;

    constexpr uint32_t kBuffers = 100;
    for (uint32_t i = 0; i < kBuffers; ++i) {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = static_cast<uint32_t>(kDataSize * sizeof(uint32_t));
        descriptor.usage = wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc;
        wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

        void* mappedData = MapWriteAsyncAndWait(buffer);
        memcpy(mappedData, myData.data(), kDataSize * sizeof(uint32_t));
        UnmapBuffer(buffer);

        buffers.push_back(buffer);  // Destroy buffers upon return.
    }

    for (uint32_t i = 0; i < kBuffers; ++i) {
        EXPECT_BUFFER_U32_RANGE_EQ(myData.data(), buffers[i], 0, kDataSize);
    }
}

// Test the result of GetMappedRange when mapped for writing.
TEST_P(BufferMapWriteTests, GetMappedRange) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    void* mappedData = MapWriteAsyncAndWait(buffer);
    ASSERT_EQ(buffer.GetMappedRange(), mappedData);
    ASSERT_EQ(buffer.GetMappedRange(), buffer.GetConstMappedRange());
    ASSERT_NE(buffer.GetMappedRange(), nullptr);
    UnmapBuffer(buffer);
}

// Test the result of GetMappedRange when mapped for writing for a zero-sized buffer.
TEST_P(BufferMapWriteTests, GetMappedRangeZeroSized) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 0;
    descriptor.usage = wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    void* mappedData = MapWriteAsyncAndWait(buffer);
    ASSERT_EQ(buffer.GetMappedRange(), mappedData);
    ASSERT_EQ(buffer.GetMappedRange(), buffer.GetConstMappedRange());
    ASSERT_NE(buffer.GetMappedRange(), nullptr);
    UnmapBuffer(buffer);
}

DAWN_INSTANTIATE_TEST(BufferMapWriteTests,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());

class BufferMappingTests : public DawnTest {
  protected:
    void MapAsyncAndWait(const wgpu::Buffer& buffer,
                         wgpu::MapMode mode,
                         size_t offset,
                         size_t size) {
        bool done = false;
        buffer.MapAsync(
            mode, offset, size,
            [](WGPUBufferMapAsyncStatus status, void* userdata) {
                ASSERT_EQ(WGPUBufferMapAsyncStatus_Success, status);
                *static_cast<bool*>(userdata) = true;
            },
            &done);

        while (!done) {
            WaitABit();
        }
    }

    wgpu::Buffer CreateMapReadBuffer(uint64_t size) {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = size;
        descriptor.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
        return device.CreateBuffer(&descriptor);
    }

    wgpu::Buffer CreateMapWriteBuffer(uint64_t size) {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = size;
        descriptor.usage = wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc;
        return device.CreateBuffer(&descriptor);
    }
};

void CheckMapping(const void* actual, const void* expected, size_t size) {
    EXPECT_NE(actual, nullptr);
    if (actual != nullptr) {
        EXPECT_EQ(0, memcmp(actual, expected, size));
    }
}

// Test that the simplest map read works
TEST_P(BufferMappingTests, MapRead_Basic) {
    wgpu::Buffer buffer = CreateMapReadBuffer(4);

    uint32_t myData = 0x01020304;
    constexpr size_t kSize = sizeof(myData);
    queue.WriteBuffer(buffer, 0, &myData, kSize);

    MapAsyncAndWait(buffer, wgpu::MapMode::Read, 0, 4);
    CheckMapping(buffer.GetConstMappedRange(), &myData, kSize);
    CheckMapping(buffer.GetConstMappedRange(0, kSize), &myData, kSize);
    buffer.Unmap();
}

// Test map-reading a zero-sized buffer.
TEST_P(BufferMappingTests, MapRead_ZeroSized) {
    wgpu::Buffer buffer = CreateMapReadBuffer(0);

    MapAsyncAndWait(buffer, wgpu::MapMode::Read, 0, 0);
    ASSERT_NE(buffer.GetConstMappedRange(), nullptr);
    buffer.Unmap();
}

// Test map-reading with a non-zero offset
TEST_P(BufferMappingTests, MapRead_NonZeroOffset) {
    wgpu::Buffer buffer = CreateMapReadBuffer(8);

    uint32_t myData[2] = {0x01020304, 0x05060708};
    queue.WriteBuffer(buffer, 0, &myData, sizeof(myData));

    MapAsyncAndWait(buffer, wgpu::MapMode::Read, 4, 4);
    ASSERT_EQ(myData[1], *static_cast<const uint32_t*>(buffer.GetConstMappedRange(4)));
    buffer.Unmap();
}

// Map read and unmap twice. Test that both of these two iterations work.
TEST_P(BufferMappingTests, MapRead_Twice) {
    wgpu::Buffer buffer = CreateMapReadBuffer(4);

    uint32_t myData = 0x01020304;
    queue.WriteBuffer(buffer, 0, &myData, sizeof(myData));

    MapAsyncAndWait(buffer, wgpu::MapMode::Read, 0, 4);
    ASSERT_EQ(myData, *static_cast<const uint32_t*>(buffer.GetConstMappedRange()));
    buffer.Unmap();

    myData = 0x05060708;
    queue.WriteBuffer(buffer, 0, &myData, sizeof(myData));

    MapAsyncAndWait(buffer, wgpu::MapMode::Read, 0, 4);
    ASSERT_EQ(myData, *static_cast<const uint32_t*>(buffer.GetConstMappedRange()));
    buffer.Unmap();
}

// Test map-reading a large buffer.
TEST_P(BufferMappingTests, MapRead_Large) {
    constexpr uint32_t kDataSize = 1000 * 1000;
    constexpr size_t kByteSize = kDataSize * sizeof(uint32_t);
    wgpu::Buffer buffer = CreateMapReadBuffer(kByteSize);

    std::vector<uint32_t> myData;
    for (uint32_t i = 0; i < kDataSize; ++i) {
        myData.push_back(i);
    }
    queue.WriteBuffer(buffer, 0, myData.data(), kByteSize);

    MapAsyncAndWait(buffer, wgpu::MapMode::Read, 0, kByteSize);
    EXPECT_EQ(nullptr, buffer.GetConstMappedRange(0, kByteSize + 4));
    EXPECT_EQ(0, memcmp(buffer.GetConstMappedRange(), myData.data(), kByteSize));
    EXPECT_EQ(0, memcmp(buffer.GetConstMappedRange(8), myData.data() + 2, kByteSize - 8));
    EXPECT_EQ(
        0, memcmp(buffer.GetConstMappedRange(8, kByteSize - 8), myData.data() + 2, kByteSize - 8));
    buffer.Unmap();

    MapAsyncAndWait(buffer, wgpu::MapMode::Read, 12, kByteSize - 12);
    EXPECT_EQ(nullptr, buffer.GetConstMappedRange(16, kByteSize - 12));
    EXPECT_EQ(nullptr, buffer.GetConstMappedRange());
    EXPECT_EQ(nullptr, buffer.GetConstMappedRange(8));
    EXPECT_EQ(0, memcmp(buffer.GetConstMappedRange(12), myData.data() + 3, kByteSize - 12));
    EXPECT_EQ(0, memcmp(buffer.GetConstMappedRange(20), myData.data() + 5, kByteSize - 20));
    EXPECT_EQ(0, memcmp(buffer.GetConstMappedRange(20, kByteSize - 24), myData.data() + 5,
                        kByteSize - 44));
    buffer.Unmap();
}

// Test that GetConstMappedRange works inside map-read callback
TEST_P(BufferMappingTests, MapRead_InCallback) {
    constexpr size_t kBufferSize = 8;
    wgpu::Buffer buffer = CreateMapReadBuffer(kBufferSize);

    uint32_t myData[2] = {0x01020304, 0x05060708};
    constexpr size_t kSize = sizeof(myData);
    queue.WriteBuffer(buffer, 0, &myData, kSize);

    struct UserData {
        bool done;
        wgpu::Buffer buffer;
        void* expected;
    };
    UserData user{false, buffer, &myData};

    buffer.MapAsync(
        wgpu::MapMode::Read, 0, kBufferSize,
        [](WGPUBufferMapAsyncStatus status, void* userdata) {
            UserData* user = static_cast<UserData*>(userdata);

            EXPECT_EQ(WGPUBufferMapAsyncStatus_Success, status);
            if (status == WGPUBufferMapAsyncStatus_Success) {
                CheckMapping(user->buffer.GetConstMappedRange(), user->expected, kSize);
                CheckMapping(user->buffer.GetConstMappedRange(0, kSize), user->expected, kSize);

                CheckMapping(user->buffer.GetConstMappedRange(4, 4),
                             static_cast<const uint32_t*>(user->expected) + 1, sizeof(uint32_t));

                user->buffer.Unmap();
            }
            user->done = true;
        },
        &user);

    while (!user.done) {
        WaitABit();
    }
}

// Test that the simplest map write works.
TEST_P(BufferMappingTests, MapWrite_Basic) {
    wgpu::Buffer buffer = CreateMapWriteBuffer(4);

    uint32_t myData = 2934875;
    MapAsyncAndWait(buffer, wgpu::MapMode::Write, 0, 4);
    ASSERT_NE(nullptr, buffer.GetMappedRange());
    ASSERT_NE(nullptr, buffer.GetConstMappedRange());
    memcpy(buffer.GetMappedRange(), &myData, sizeof(myData));
    buffer.Unmap();

    EXPECT_BUFFER_U32_EQ(myData, buffer, 0);
}

// Test that the simplest map write works with a range.
TEST_P(BufferMappingTests, MapWrite_BasicRange) {
    wgpu::Buffer buffer = CreateMapWriteBuffer(4);

    uint32_t myData = 2934875;
    MapAsyncAndWait(buffer, wgpu::MapMode::Write, 0, 4);
    ASSERT_NE(nullptr, buffer.GetMappedRange(0, 4));
    ASSERT_NE(nullptr, buffer.GetConstMappedRange(0, 4));
    memcpy(buffer.GetMappedRange(), &myData, sizeof(myData));
    buffer.Unmap();

    EXPECT_BUFFER_U32_EQ(myData, buffer, 0);
}

// Test map-writing a zero-sized buffer.
TEST_P(BufferMappingTests, MapWrite_ZeroSized) {
    wgpu::Buffer buffer = CreateMapWriteBuffer(0);

    MapAsyncAndWait(buffer, wgpu::MapMode::Write, 0, 0);
    ASSERT_NE(buffer.GetConstMappedRange(), nullptr);
    ASSERT_NE(buffer.GetMappedRange(), nullptr);
    buffer.Unmap();
}

// Test map-writing with a non-zero offset.
TEST_P(BufferMappingTests, MapWrite_NonZeroOffset) {
    wgpu::Buffer buffer = CreateMapWriteBuffer(8);

    uint32_t myData = 2934875;
    MapAsyncAndWait(buffer, wgpu::MapMode::Write, 4, 4);
    memcpy(buffer.GetMappedRange(4), &myData, sizeof(myData));
    buffer.Unmap();

    EXPECT_BUFFER_U32_EQ(myData, buffer, 4);
}

// Map, write and unmap twice. Test that both of these two iterations work.
TEST_P(BufferMappingTests, MapWrite_Twice) {
    wgpu::Buffer buffer = CreateMapWriteBuffer(4);

    uint32_t myData = 2934875;
    MapAsyncAndWait(buffer, wgpu::MapMode::Write, 0, 4);
    memcpy(buffer.GetMappedRange(), &myData, sizeof(myData));
    buffer.Unmap();

    EXPECT_BUFFER_U32_EQ(myData, buffer, 0);

    myData = 9999999;
    MapAsyncAndWait(buffer, wgpu::MapMode::Write, 0, 4);
    memcpy(buffer.GetMappedRange(), &myData, sizeof(myData));
    buffer.Unmap();

    EXPECT_BUFFER_U32_EQ(myData, buffer, 0);
}

// Test mapping a large buffer.
TEST_P(BufferMappingTests, MapWrite_Large) {
    constexpr uint32_t kDataSize = 1000 * 1000;
    constexpr size_t kByteSize = kDataSize * sizeof(uint32_t);
    wgpu::Buffer buffer = CreateMapWriteBuffer(kDataSize * sizeof(uint32_t));

    std::vector<uint32_t> myData;
    for (uint32_t i = 0; i < kDataSize; ++i) {
        myData.push_back(i);
    }

    MapAsyncAndWait(buffer, wgpu::MapMode::Write, 12, kByteSize - 20);
    EXPECT_EQ(nullptr, buffer.GetMappedRange());
    EXPECT_EQ(nullptr, buffer.GetMappedRange(0));
    EXPECT_EQ(nullptr, buffer.GetMappedRange(8));
    EXPECT_EQ(nullptr, buffer.GetMappedRange(16, kByteSize - 8));
    EXPECT_EQ(nullptr, buffer.GetMappedRange(16, kByteSize - 20));
    memcpy(buffer.GetMappedRange(12), myData.data(), kByteSize - 20);
    buffer.Unmap();
    EXPECT_BUFFER_U32_RANGE_EQ(myData.data(), buffer, 12, kDataSize - 5);
}

// Test that the map offset isn't updated when the call is an error.
TEST_P(BufferMappingTests, OffsetNotUpdatedOnError) {
    uint32_t data[3] = {0xCA7, 0xB0A7, 0xBA7};
    wgpu::Buffer buffer = CreateMapReadBuffer(sizeof(data));
    queue.WriteBuffer(buffer, 0, data, sizeof(data));

    // Map the buffer but do not wait on the result yet.
    bool done = false;
    buffer.MapAsync(
        wgpu::MapMode::Read, 4, 4,
        [](WGPUBufferMapAsyncStatus status, void* userdata) {
            ASSERT_EQ(WGPUBufferMapAsyncStatus_Success, status);
            *static_cast<bool*>(userdata) = true;
        },
        &done);

    // Call MapAsync another time, it is an error because the buffer is already being mapped so
    // mMapOffset is not updated.
    ASSERT_DEVICE_ERROR(buffer.MapAsync(wgpu::MapMode::Read, 8, 4, nullptr, nullptr));

    while (!done) {
        WaitABit();
    }

    // mMapOffset has not been updated so it should still be 4, which is data[1]
    ASSERT_EQ(0, memcmp(buffer.GetConstMappedRange(4), &data[1], sizeof(uint32_t)));
}

// Test that Get(Const)MappedRange work inside map-write callback.
TEST_P(BufferMappingTests, MapWrite_InCallbackDefault) {
    wgpu::Buffer buffer = CreateMapWriteBuffer(4);

    constexpr uint32_t myData = 2934875;
    constexpr size_t kSize = sizeof(myData);

    struct UserData {
        bool done;
        wgpu::Buffer buffer;
    };
    UserData user{false, buffer};

    buffer.MapAsync(
        wgpu::MapMode::Write, 0, kSize,
        [](WGPUBufferMapAsyncStatus status, void* userdata) {
            UserData* user = static_cast<UserData*>(userdata);

            EXPECT_EQ(WGPUBufferMapAsyncStatus_Success, status);
            if (status == WGPUBufferMapAsyncStatus_Success) {
                EXPECT_NE(nullptr, user->buffer.GetConstMappedRange());
                void* ptr = user->buffer.GetMappedRange();
                EXPECT_NE(nullptr, ptr);
                if (ptr != nullptr) {
                    uint32_t data = myData;
                    memcpy(ptr, &data, kSize);
                }

                user->buffer.Unmap();
            }
            user->done = true;
        },
        &user);

    while (!user.done) {
        WaitABit();
    }

    EXPECT_BUFFER_U32_EQ(myData, buffer, 0);
}

// Test that Get(Const)MappedRange with range work inside map-write callback.
TEST_P(BufferMappingTests, MapWrite_InCallbackRange) {
    wgpu::Buffer buffer = CreateMapWriteBuffer(4);

    constexpr uint32_t myData = 2934875;
    constexpr size_t kSize = sizeof(myData);

    struct UserData {
        bool done;
        wgpu::Buffer buffer;
    };
    UserData user{false, buffer};

    buffer.MapAsync(
        wgpu::MapMode::Write, 0, kSize,
        [](WGPUBufferMapAsyncStatus status, void* userdata) {
            UserData* user = static_cast<UserData*>(userdata);

            EXPECT_EQ(WGPUBufferMapAsyncStatus_Success, status);
            if (status == WGPUBufferMapAsyncStatus_Success) {
                EXPECT_NE(nullptr, user->buffer.GetConstMappedRange(0, kSize));
                void* ptr = user->buffer.GetMappedRange(0, kSize);
                EXPECT_NE(nullptr, ptr);
                if (ptr != nullptr) {
                    uint32_t data = myData;
                    memcpy(ptr, &data, kSize);
                }

                user->buffer.Unmap();
            }
            user->done = true;
        },
        &user);

    while (!user.done) {
        WaitABit();
    }

    EXPECT_BUFFER_U32_EQ(myData, buffer, 0);
}

DAWN_INSTANTIATE_TEST(BufferMappingTests,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());

class CreateBufferMappedTests : public DawnTest {
  protected:
    static void MapReadCallback(WGPUBufferMapAsyncStatus status,
                                const void* data,
                                uint64_t,
                                void* userdata) {
        ASSERT_EQ(WGPUBufferMapAsyncStatus_Success, status);
        ASSERT_NE(nullptr, data);

        static_cast<CreateBufferMappedTests*>(userdata)->mappedData = data;
    }

    const void* MapReadAsyncAndWait(const wgpu::Buffer& buffer) {
        buffer.MapReadAsync(MapReadCallback, this);

        while (mappedData == nullptr) {
            WaitABit();
        }

        return mappedData;
    }

    void UnmapBuffer(const wgpu::Buffer& buffer) {
        buffer.Unmap();
        mappedData = nullptr;
    }

    void CheckResultStartsZeroed(const wgpu::CreateBufferMappedResult& result, uint64_t size) {
        ASSERT_EQ(result.dataLength, size);
        for (uint64_t i = 0; i < result.dataLength; ++i) {
            uint8_t value = *(reinterpret_cast<uint8_t*>(result.data) + i);
            ASSERT_EQ(value, 0u);
        }
    }

    wgpu::CreateBufferMappedResult CreateBufferMapped(wgpu::BufferUsage usage, uint64_t size) {
        wgpu::BufferDescriptor descriptor = {};
        descriptor.size = size;
        descriptor.usage = usage;

        wgpu::CreateBufferMappedResult result = device.CreateBufferMapped(&descriptor);
        CheckResultStartsZeroed(result, size);
        return result;
    }

    wgpu::CreateBufferMappedResult CreateBufferMappedWithData(wgpu::BufferUsage usage,
                                                              const std::vector<uint32_t>& data) {
        size_t byteLength = data.size() * sizeof(uint32_t);
        wgpu::CreateBufferMappedResult result = CreateBufferMapped(usage, byteLength);
        memcpy(result.data, data.data(), byteLength);

        return result;
    }

  private:
    const void* mappedData = nullptr;
};

// Test that the simplest CreateBufferMapped works for MapWrite buffers.
TEST_P(CreateBufferMappedTests, MapWriteUsageSmall) {
    uint32_t myData = 230502;
    wgpu::CreateBufferMappedResult result = CreateBufferMappedWithData(
        wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc, {myData});
    UnmapBuffer(result.buffer);
    EXPECT_BUFFER_U32_EQ(myData, result.buffer, 0);
}

// Test that the simplest CreateBufferMapped works for MapRead buffers.
TEST_P(CreateBufferMappedTests, MapReadUsageSmall) {
    uint32_t myData = 230502;
    wgpu::CreateBufferMappedResult result =
        CreateBufferMappedWithData(wgpu::BufferUsage::MapRead, {myData});
    UnmapBuffer(result.buffer);

    const void* mappedData = MapReadAsyncAndWait(result.buffer);
    ASSERT_EQ(myData, *reinterpret_cast<const uint32_t*>(mappedData));
    UnmapBuffer(result.buffer);
}

// Test that the simplest CreateBufferMapped works for non-mappable buffers.
TEST_P(CreateBufferMappedTests, NonMappableUsageSmall) {
    uint32_t myData = 4239;
    wgpu::CreateBufferMappedResult result =
        CreateBufferMappedWithData(wgpu::BufferUsage::CopySrc, {myData});
    UnmapBuffer(result.buffer);

    EXPECT_BUFFER_U32_EQ(myData, result.buffer, 0);
}

// Test CreateBufferMapped for a large MapWrite buffer
TEST_P(CreateBufferMappedTests, MapWriteUsageLarge) {
    constexpr uint64_t kDataSize = 1000 * 1000;
    std::vector<uint32_t> myData;
    for (uint32_t i = 0; i < kDataSize; ++i) {
        myData.push_back(i);
    }

    wgpu::CreateBufferMappedResult result = CreateBufferMappedWithData(
        wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc, {myData});
    UnmapBuffer(result.buffer);

    EXPECT_BUFFER_U32_RANGE_EQ(myData.data(), result.buffer, 0, kDataSize);
}

// Test CreateBufferMapped for a large MapRead buffer
TEST_P(CreateBufferMappedTests, MapReadUsageLarge) {
    constexpr uint64_t kDataSize = 1000 * 1000;
    std::vector<uint32_t> myData;
    for (uint32_t i = 0; i < kDataSize; ++i) {
        myData.push_back(i);
    }

    wgpu::CreateBufferMappedResult result =
        CreateBufferMappedWithData(wgpu::BufferUsage::MapRead, myData);
    UnmapBuffer(result.buffer);

    const void* mappedData = MapReadAsyncAndWait(result.buffer);
    ASSERT_EQ(0, memcmp(mappedData, myData.data(), kDataSize * sizeof(uint32_t)));
    UnmapBuffer(result.buffer);
}

// Test CreateBufferMapped for a large non-mappable buffer
TEST_P(CreateBufferMappedTests, NonMappableUsageLarge) {
    constexpr uint64_t kDataSize = 1000 * 1000;
    std::vector<uint32_t> myData;
    for (uint32_t i = 0; i < kDataSize; ++i) {
        myData.push_back(i);
    }

    wgpu::CreateBufferMappedResult result =
        CreateBufferMappedWithData(wgpu::BufferUsage::CopySrc, {myData});
    UnmapBuffer(result.buffer);

    EXPECT_BUFFER_U32_RANGE_EQ(myData.data(), result.buffer, 0, kDataSize);
}

// Test destroying a non-mappable buffer mapped at creation.
// This is a regression test for an issue where the D3D12 backend thought the buffer was actually
// mapped and tried to unlock the heap residency (when actually the buffer was using a staging
// buffer)
TEST_P(CreateBufferMappedTests, DestroyNonMappableWhileMappedForCreation) {
    wgpu::CreateBufferMappedResult result = CreateBufferMapped(wgpu::BufferUsage::CopySrc, 4);
    result.buffer.Destroy();
}

// Test destroying a mappable buffer mapped at creation.
TEST_P(CreateBufferMappedTests, DestroyMappableWhileMappedForCreation) {
    wgpu::CreateBufferMappedResult result = CreateBufferMapped(wgpu::BufferUsage::MapRead, 4);
    result.buffer.Destroy();
}

// Test that mapping a buffer is valid after CreateBufferMapped and Unmap
TEST_P(CreateBufferMappedTests, CreateThenMapSuccess) {
    static uint32_t myData = 230502;
    static uint32_t myData2 = 1337;
    wgpu::CreateBufferMappedResult result = CreateBufferMappedWithData(
        wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc, {myData});
    UnmapBuffer(result.buffer);

    EXPECT_BUFFER_U32_EQ(myData, result.buffer, 0);

    bool done = false;
    result.buffer.MapWriteAsync(
        [](WGPUBufferMapAsyncStatus status, void* data, uint64_t, void* userdata) {
            ASSERT_EQ(WGPUBufferMapAsyncStatus_Success, status);
            ASSERT_NE(nullptr, data);

            *static_cast<uint32_t*>(data) = myData2;
            *static_cast<bool*>(userdata) = true;
        },
        &done);

    while (!done) {
        WaitABit();
    }

    UnmapBuffer(result.buffer);
    EXPECT_BUFFER_U32_EQ(myData2, result.buffer, 0);
}

// Test that is is invalid to map a buffer twice when using CreateBufferMapped
TEST_P(CreateBufferMappedTests, CreateThenMapBeforeUnmapFailure) {
    uint32_t myData = 230502;
    wgpu::CreateBufferMappedResult result = CreateBufferMappedWithData(
        wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc, {myData});

    ASSERT_DEVICE_ERROR([&]() {
        bool done = false;
        result.buffer.MapWriteAsync(
            [](WGPUBufferMapAsyncStatus status, void* data, uint64_t, void* userdata) {
                ASSERT_EQ(WGPUBufferMapAsyncStatus_Error, status);
                ASSERT_EQ(nullptr, data);

                *static_cast<bool*>(userdata) = true;
            },
            &done);

        while (!done) {
            WaitABit();
        }
    }());

    // CreateBufferMapped is unaffected by the MapWrite error.
    UnmapBuffer(result.buffer);
    EXPECT_BUFFER_U32_EQ(myData, result.buffer, 0);
}

// Test that creating a zero-sized buffer mapped is allowed.
TEST_P(CreateBufferMappedTests, ZeroSized) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 0;
    descriptor.usage = wgpu::BufferUsage::Vertex;
    wgpu::CreateBufferMappedResult result = device.CreateBufferMapped(&descriptor);

    ASSERT_EQ(0u, result.dataLength);
    ASSERT_NE(nullptr, result.data);

    // Check that unmapping the buffer works too.
    UnmapBuffer(result.buffer);
}

// Test that creating a zero-sized mapppable buffer mapped. (it is a different code path)
TEST_P(CreateBufferMappedTests, ZeroSizedMappableBuffer) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 0;
    descriptor.usage = wgpu::BufferUsage::MapWrite;
    wgpu::CreateBufferMappedResult result = device.CreateBufferMapped(&descriptor);

    ASSERT_EQ(0u, result.dataLength);
    ASSERT_NE(nullptr, result.data);

    // Check that unmapping the buffer works too.
    UnmapBuffer(result.buffer);
}

// Test that creating a zero-sized error buffer mapped. (it is a different code path)
TEST_P(CreateBufferMappedTests, ZeroSizedErrorBuffer) {
    DAWN_SKIP_TEST_IF(IsDawnValidationSkipped());

    wgpu::BufferDescriptor descriptor;
    descriptor.size = 0;
    descriptor.usage = wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::Storage;
    wgpu::CreateBufferMappedResult result;
    ASSERT_DEVICE_ERROR(result = device.CreateBufferMapped(&descriptor));

    ASSERT_EQ(0u, result.dataLength);
    ASSERT_NE(nullptr, result.data);
}

// Test the result of GetMappedRange when mapped at creation.
TEST_P(CreateBufferMappedTests, GetMappedRange) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::CopyDst;
    wgpu::CreateBufferMappedResult result;
    result = device.CreateBufferMapped(&descriptor);

    ASSERT_EQ(result.buffer.GetMappedRange(), result.data);
    ASSERT_EQ(result.buffer.GetMappedRange(), result.buffer.GetConstMappedRange());
    ASSERT_NE(result.buffer.GetMappedRange(), nullptr);
    result.buffer.Unmap();
}

// Test the result of GetMappedRange when mapped at creation for a zero-sized buffer.
TEST_P(CreateBufferMappedTests, GetMappedRangeZeroSized) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 0;
    descriptor.usage = wgpu::BufferUsage::CopyDst;
    wgpu::CreateBufferMappedResult result;
    result = device.CreateBufferMapped(&descriptor);

    ASSERT_EQ(result.buffer.GetMappedRange(), result.data);
    ASSERT_EQ(result.buffer.GetMappedRange(), result.buffer.GetConstMappedRange());
    ASSERT_NE(result.buffer.GetMappedRange(), nullptr);
    result.buffer.Unmap();
}

DAWN_INSTANTIATE_TEST(CreateBufferMappedTests,
                      D3D12Backend(),
                      D3D12Backend({}, {"use_d3d12_resource_heap_tier2"}),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());

class BufferMappedAtCreationTests : public DawnTest {
  protected:
    static void MapReadCallback(WGPUBufferMapAsyncStatus status,
                                const void* data,
                                uint64_t,
                                void* userdata) {
        ASSERT_EQ(WGPUBufferMapAsyncStatus_Success, status);
        ASSERT_NE(nullptr, data);

        static_cast<BufferMappedAtCreationTests*>(userdata)->mappedData = data;
    }

    const void* MapReadAsyncAndWait(const wgpu::Buffer& buffer) {
        buffer.MapReadAsync(MapReadCallback, this);

        while (mappedData == nullptr) {
            WaitABit();
        }

        return mappedData;
    }

    void UnmapBuffer(const wgpu::Buffer& buffer) {
        buffer.Unmap();
        mappedData = nullptr;
    }

    wgpu::Buffer BufferMappedAtCreation(wgpu::BufferUsage usage, uint64_t size) {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = size;
        descriptor.usage = usage;
        descriptor.mappedAtCreation = true;
        return device.CreateBuffer(&descriptor);
    }

    wgpu::Buffer BufferMappedAtCreationWithData(wgpu::BufferUsage usage,
                                                const std::vector<uint32_t>& data) {
        size_t byteLength = data.size() * sizeof(uint32_t);
        wgpu::Buffer buffer = BufferMappedAtCreation(usage, byteLength);
        memcpy(buffer.GetMappedRange(), data.data(), byteLength);
        return buffer;
    }

  private:
    const void* mappedData = nullptr;
};

// Test that the simplest mappedAtCreation works for MapWrite buffers.
TEST_P(BufferMappedAtCreationTests, MapWriteUsageSmall) {
    uint32_t myData = 230502;
    wgpu::Buffer buffer = BufferMappedAtCreationWithData(
        wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc, {myData});
    UnmapBuffer(buffer);
    EXPECT_BUFFER_U32_EQ(myData, buffer, 0);
}

// Test that the simplest mappedAtCreation works for MapRead buffers.
TEST_P(BufferMappedAtCreationTests, MapReadUsageSmall) {
    uint32_t myData = 230502;
    wgpu::Buffer buffer = BufferMappedAtCreationWithData(wgpu::BufferUsage::MapRead, {myData});
    UnmapBuffer(buffer);

    const void* mappedData = MapReadAsyncAndWait(buffer);
    ASSERT_EQ(myData, *reinterpret_cast<const uint32_t*>(mappedData));
    UnmapBuffer(buffer);
}

// Test that the simplest mappedAtCreation works for non-mappable buffers.
TEST_P(BufferMappedAtCreationTests, NonMappableUsageSmall) {
    uint32_t myData = 4239;
    wgpu::Buffer buffer = BufferMappedAtCreationWithData(wgpu::BufferUsage::CopySrc, {myData});
    UnmapBuffer(buffer);

    EXPECT_BUFFER_U32_EQ(myData, buffer, 0);
}

// Test mappedAtCreation for a large MapWrite buffer
TEST_P(BufferMappedAtCreationTests, MapWriteUsageLarge) {
    constexpr uint64_t kDataSize = 1000 * 1000;
    std::vector<uint32_t> myData;
    for (uint32_t i = 0; i < kDataSize; ++i) {
        myData.push_back(i);
    }

    wgpu::Buffer buffer = BufferMappedAtCreationWithData(
        wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc, {myData});
    UnmapBuffer(buffer);

    EXPECT_BUFFER_U32_RANGE_EQ(myData.data(), buffer, 0, kDataSize);
}

// Test mappedAtCreation for a large MapRead buffer
TEST_P(BufferMappedAtCreationTests, MapReadUsageLarge) {
    constexpr uint64_t kDataSize = 1000 * 1000;
    std::vector<uint32_t> myData;
    for (uint32_t i = 0; i < kDataSize; ++i) {
        myData.push_back(i);
    }

    wgpu::Buffer buffer = BufferMappedAtCreationWithData(wgpu::BufferUsage::MapRead, myData);
    UnmapBuffer(buffer);

    const void* mappedData = MapReadAsyncAndWait(buffer);
    ASSERT_EQ(0, memcmp(mappedData, myData.data(), kDataSize * sizeof(uint32_t)));
    UnmapBuffer(buffer);
}

// Test mappedAtCreation for a large non-mappable buffer
TEST_P(BufferMappedAtCreationTests, NonMappableUsageLarge) {
    constexpr uint64_t kDataSize = 1000 * 1000;
    std::vector<uint32_t> myData;
    for (uint32_t i = 0; i < kDataSize; ++i) {
        myData.push_back(i);
    }

    wgpu::Buffer buffer = BufferMappedAtCreationWithData(wgpu::BufferUsage::CopySrc, {myData});
    UnmapBuffer(buffer);

    EXPECT_BUFFER_U32_RANGE_EQ(myData.data(), buffer, 0, kDataSize);
}

// Test destroying a non-mappable buffer mapped at creation.
// This is a regression test for an issue where the D3D12 backend thought the buffer was actually
// mapped and tried to unlock the heap residency (when actually the buffer was using a staging
// buffer)
TEST_P(BufferMappedAtCreationTests, DestroyNonMappableWhileMappedForCreation) {
    wgpu::Buffer buffer = BufferMappedAtCreation(wgpu::BufferUsage::CopySrc, 4);
    buffer.Destroy();
}

// Test destroying a mappable buffer mapped at creation.
TEST_P(BufferMappedAtCreationTests, DestroyMappableWhileMappedForCreation) {
    wgpu::Buffer buffer = BufferMappedAtCreation(wgpu::BufferUsage::MapRead, 4);
    buffer.Destroy();
}

// Test that mapping a buffer is valid after mappedAtCreation and Unmap
TEST_P(BufferMappedAtCreationTests, CreateThenMapSuccess) {
    static uint32_t myData = 230502;
    static uint32_t myData2 = 1337;
    wgpu::Buffer buffer = BufferMappedAtCreationWithData(
        wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc, {myData});
    UnmapBuffer(buffer);

    EXPECT_BUFFER_U32_EQ(myData, buffer, 0);

    bool done = false;
    buffer.MapWriteAsync(
        [](WGPUBufferMapAsyncStatus status, void* data, uint64_t, void* userdata) {
            ASSERT_EQ(WGPUBufferMapAsyncStatus_Success, status);
            ASSERT_NE(nullptr, data);

            *static_cast<uint32_t*>(data) = myData2;
            *static_cast<bool*>(userdata) = true;
        },
        &done);

    while (!done) {
        WaitABit();
    }

    UnmapBuffer(buffer);
    EXPECT_BUFFER_U32_EQ(myData2, buffer, 0);
}

// Test that is is invalid to map a buffer twice when using mappedAtCreation
TEST_P(BufferMappedAtCreationTests, CreateThenMapBeforeUnmapFailure) {
    uint32_t myData = 230502;
    wgpu::Buffer buffer = BufferMappedAtCreationWithData(
        wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc, {myData});

    ASSERT_DEVICE_ERROR([&]() {
        bool done = false;
        buffer.MapWriteAsync(
            [](WGPUBufferMapAsyncStatus status, void* data, uint64_t, void* userdata) {
                ASSERT_EQ(WGPUBufferMapAsyncStatus_Error, status);
                ASSERT_EQ(nullptr, data);

                *static_cast<bool*>(userdata) = true;
            },
            &done);

        while (!done) {
            WaitABit();
        }
    }());

    // mappedAtCreation is unaffected by the MapWrite error.
    UnmapBuffer(buffer);
    EXPECT_BUFFER_U32_EQ(myData, buffer, 0);
}

// Test that creating a zero-sized buffer mapped is allowed.
TEST_P(BufferMappedAtCreationTests, ZeroSized) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 0;
    descriptor.usage = wgpu::BufferUsage::Vertex;
    descriptor.mappedAtCreation = true;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    ASSERT_NE(nullptr, buffer.GetMappedRange());

    // Check that unmapping the buffer works too.
    UnmapBuffer(buffer);
}

// Test that creating a zero-sized mapppable buffer mapped. (it is a different code path)
TEST_P(BufferMappedAtCreationTests, ZeroSizedMappableBuffer) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 0;
    descriptor.usage = wgpu::BufferUsage::MapWrite;
    descriptor.mappedAtCreation = true;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    ASSERT_NE(nullptr, buffer.GetMappedRange());

    // Check that unmapping the buffer works too.
    UnmapBuffer(buffer);
}

// Test that creating a zero-sized error buffer mapped. (it is a different code path)
TEST_P(BufferMappedAtCreationTests, ZeroSizedErrorBuffer) {
    DAWN_SKIP_TEST_IF(IsDawnValidationSkipped());

    wgpu::BufferDescriptor descriptor;
    descriptor.size = 0;
    descriptor.usage = wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::Storage;
    descriptor.mappedAtCreation = true;
    wgpu::Buffer buffer;
    ASSERT_DEVICE_ERROR(buffer = device.CreateBuffer(&descriptor));

    ASSERT_NE(nullptr, buffer.GetMappedRange());
}

// Test the result of GetMappedRange when mapped at creation.
TEST_P(BufferMappedAtCreationTests, GetMappedRange) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::CopyDst;
    descriptor.mappedAtCreation = true;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    ASSERT_EQ(buffer.GetMappedRange(), buffer.GetConstMappedRange());
    ASSERT_NE(buffer.GetMappedRange(), nullptr);
    buffer.Unmap();
}

// Test the result of GetMappedRange when mapped at creation for a zero-sized buffer.
TEST_P(BufferMappedAtCreationTests, GetMappedRangeZeroSized) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 0;
    descriptor.usage = wgpu::BufferUsage::CopyDst;
    descriptor.mappedAtCreation = true;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    ASSERT_EQ(buffer.GetMappedRange(), buffer.GetConstMappedRange());
    ASSERT_NE(buffer.GetMappedRange(), nullptr);
    buffer.Unmap();
}

DAWN_INSTANTIATE_TEST(BufferMappedAtCreationTests,
                      D3D12Backend(),
                      D3D12Backend({}, {"use_d3d12_resource_heap_tier2"}),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());

class BufferTests : public DawnTest {};

// Test that creating a zero-buffer is allowed.
TEST_P(BufferTests, ZeroSizedBuffer) {
    wgpu::BufferDescriptor desc;
    desc.size = 0;
    desc.usage = wgpu::BufferUsage::CopyDst;
    device.CreateBuffer(&desc);
}

// Test that creating a very large buffers fails gracefully.
TEST_P(BufferTests, CreateBufferOOM) {
    // TODO(http://crbug.com/dawn/27): Missing support.
    DAWN_SKIP_TEST_IF(IsOpenGL());
    DAWN_SKIP_TEST_IF(IsAsan());

    wgpu::BufferDescriptor descriptor;
    descriptor.usage = wgpu::BufferUsage::CopyDst;

    descriptor.size = std::numeric_limits<uint64_t>::max();
    ASSERT_DEVICE_ERROR(device.CreateBuffer(&descriptor));

    // UINT64_MAX may be special cased. Test a smaller, but really large buffer also fails
    descriptor.size = 1ull << 50;
    ASSERT_DEVICE_ERROR(device.CreateBuffer(&descriptor));
}

// Test that a very large CreateBufferMapped fails gracefully.
TEST_P(BufferTests, CreateBufferMappedOOM) {
    // TODO(http://crbug.com/dawn/27): Missing support.
    DAWN_SKIP_TEST_IF(IsOpenGL());
    DAWN_SKIP_TEST_IF(IsAsan());

    // Test non-mappable buffer
    {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = 4;
        descriptor.usage = wgpu::BufferUsage::CopyDst;

        // Control: test a small buffer works.
        device.CreateBufferMapped(&descriptor);

        // Test an enormous buffer fails
        descriptor.size = std::numeric_limits<uint64_t>::max();
        ASSERT_DEVICE_ERROR(device.CreateBufferMapped(&descriptor));

        // UINT64_MAX may be special cased. Test a smaller, but really large buffer also fails
        descriptor.size = 1ull << 50;
        ASSERT_DEVICE_ERROR(device.CreateBufferMapped(&descriptor));
    }

    // Test mappable buffer
    {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = 4;
        descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;

        // Control: test a small buffer works.
        device.CreateBufferMapped(&descriptor);

        // Test an enormous buffer fails
        descriptor.size = std::numeric_limits<uint64_t>::max();
        ASSERT_DEVICE_ERROR(device.CreateBufferMapped(&descriptor));

        // UINT64_MAX may be special cased. Test a smaller, but really large buffer also fails
        descriptor.size = 1ull << 50;
        ASSERT_DEVICE_ERROR(device.CreateBufferMapped(&descriptor));
    }
}

// Test that a very large buffer mappedAtCreation fails gracefully.
TEST_P(BufferTests, BufferMappedAtCreationOOM) {
    // TODO(http://crbug.com/dawn/27): Missing support.
    DAWN_SKIP_TEST_IF(IsOpenGL());
    DAWN_SKIP_TEST_IF(IsAsan());

    // Test non-mappable buffer
    {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = 4;
        descriptor.usage = wgpu::BufferUsage::CopyDst;
        descriptor.mappedAtCreation = true;

        // Control: test a small buffer works.
        device.CreateBuffer(&descriptor);

        // Test an enormous buffer fails
        descriptor.size = std::numeric_limits<uint64_t>::max();
        ASSERT_DEVICE_ERROR(device.CreateBuffer(&descriptor));

        // UINT64_MAX may be special cased. Test a smaller, but really large buffer also fails
        descriptor.size = 1ull << 50;
        ASSERT_DEVICE_ERROR(device.CreateBuffer(&descriptor));
    }

    // Test mappable buffer
    {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = 4;
        descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;
        descriptor.mappedAtCreation = true;

        // Control: test a small buffer works.
        device.CreateBuffer(&descriptor);

        // Test an enormous buffer fails
        descriptor.size = std::numeric_limits<uint64_t>::max();
        ASSERT_DEVICE_ERROR(device.CreateBuffer(&descriptor));

        // UINT64_MAX may be special cased. Test a smaller, but really large buffer also fails
        descriptor.size = 1ull << 50;
        ASSERT_DEVICE_ERROR(device.CreateBuffer(&descriptor));
    }
}

// Test that mapping an OOM buffer for reading fails gracefully
TEST_P(BufferTests, CreateBufferOOMMapReadAsync) {
    // TODO(http://crbug.com/dawn/27): Missing support.
    DAWN_SKIP_TEST_IF(IsOpenGL());
    DAWN_SKIP_TEST_IF(IsAsan());

    auto RunTest = [this](const wgpu::BufferDescriptor& descriptor) {
        wgpu::Buffer buffer;
        ASSERT_DEVICE_ERROR(buffer = device.CreateBuffer(&descriptor));

        bool done = false;
        ASSERT_DEVICE_ERROR(buffer.MapReadAsync(
            [](WGPUBufferMapAsyncStatus status, const void* ptr, uint64_t dataLength,
               void* userdata) {
                EXPECT_EQ(status, WGPUBufferMapAsyncStatus_Error);
                EXPECT_EQ(ptr, nullptr);
                EXPECT_EQ(dataLength, 0u);
                *static_cast<bool*>(userdata) = true;
            },
            &done));

        while (!done) {
            WaitABit();
        }
    };

    // Test an enormous buffer
    wgpu::BufferDescriptor descriptor;
    descriptor.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;

    descriptor.size = std::numeric_limits<uint64_t>::max();
    RunTest(descriptor);

    // UINT64_MAX may be special cased. Test a smaller, but really large buffer also fails
    descriptor.size = 1ull << 50;
    RunTest(descriptor);
}

// Test that mapping an OOM buffer for reading fails gracefully
TEST_P(BufferTests, CreateBufferOOMMapWriteAsync) {
    // TODO(http://crbug.com/dawn/27): Missing support.
    DAWN_SKIP_TEST_IF(IsOpenGL());
    DAWN_SKIP_TEST_IF(IsAsan());

    auto RunTest = [this](const wgpu::BufferDescriptor& descriptor) {
        wgpu::Buffer buffer;
        ASSERT_DEVICE_ERROR(buffer = device.CreateBuffer(&descriptor));

        bool done = false;
        ASSERT_DEVICE_ERROR(buffer.MapWriteAsync(
            [](WGPUBufferMapAsyncStatus status, void* ptr, uint64_t dataLength, void* userdata) {
                EXPECT_EQ(status, WGPUBufferMapAsyncStatus_Error);
                EXPECT_EQ(ptr, nullptr);
                EXPECT_EQ(dataLength, 0u);
                *static_cast<bool*>(userdata) = true;
            },
            &done));

        while (!done) {
            WaitABit();
        }
    };

    wgpu::BufferDescriptor descriptor;
    descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;

    // Test an enormous buffer
    descriptor.size = std::numeric_limits<uint64_t>::max();
    RunTest(descriptor);

    // UINT64_MAX may be special cased. Test a smaller, but really large buffer also fails
    descriptor.size = 1ull << 50;
    RunTest(descriptor);
}

// Test that mapping an OOM buffer fails gracefully
TEST_P(BufferTests, CreateBufferOOMMapAsync) {
    // TODO(http://crbug.com/dawn/27): Missing support.
    DAWN_SKIP_TEST_IF(IsOpenGL());
    DAWN_SKIP_TEST_IF(IsAsan());

    auto RunTest = [this](const wgpu::BufferDescriptor& descriptor) {
        wgpu::Buffer buffer;
        ASSERT_DEVICE_ERROR(buffer = device.CreateBuffer(&descriptor));

        bool done = false;
        ASSERT_DEVICE_ERROR(buffer.MapAsync(
            wgpu::MapMode::Write, 0, 4,
            [](WGPUBufferMapAsyncStatus status, void* userdata) {
                EXPECT_EQ(status, WGPUBufferMapAsyncStatus_Error);
                *static_cast<bool*>(userdata) = true;
            },
            &done));

        while (!done) {
            WaitABit();
        }
    };

    wgpu::BufferDescriptor descriptor;
    descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;

    // Test an enormous buffer
    descriptor.size = std::numeric_limits<uint64_t>::max();
    RunTest(descriptor);

    // UINT64_MAX may be special cased. Test a smaller, but really large buffer also fails
    descriptor.size = 1ull << 50;
    RunTest(descriptor);
}

DAWN_INSTANTIATE_TEST(BufferTests,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());
