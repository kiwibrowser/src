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

#include "tests/unittests/validation/ValidationTest.h"

#include <gmock/gmock.h>

#include <memory>

using namespace testing;

class MockBufferMapReadCallback {
  public:
    MOCK_METHOD(void,
                Call,
                (WGPUBufferMapAsyncStatus status,
                 const uint32_t* ptr,
                 uint64_t dataLength,
                 void* userdata));
};

static std::unique_ptr<MockBufferMapReadCallback> mockBufferMapReadCallback;
static void ToMockBufferMapReadCallback(WGPUBufferMapAsyncStatus status,
                                        const void* ptr,
                                        uint64_t dataLength,
                                        void* userdata) {
    // Assume the data is uint32_t to make writing matchers easier
    mockBufferMapReadCallback->Call(status, reinterpret_cast<const uint32_t*>(ptr), dataLength,
                                    userdata);
}

class MockBufferMapWriteCallback {
  public:
    MOCK_METHOD(
        void,
        Call,
        (WGPUBufferMapAsyncStatus status, uint32_t* ptr, uint64_t dataLength, void* userdata));
};

static std::unique_ptr<MockBufferMapWriteCallback> mockBufferMapWriteCallback;
static void ToMockBufferMapWriteCallback(WGPUBufferMapAsyncStatus status,
                                         void* ptr,
                                         uint64_t dataLength,
                                         void* userdata) {
    // Assume the data is uint32_t to make writing matchers easier
    mockBufferMapWriteCallback->Call(status, reinterpret_cast<uint32_t*>(ptr), dataLength,
                                     userdata);
}

class MockBufferMapAsyncCallback {
  public:
    MOCK_METHOD(void, Call, (WGPUBufferMapAsyncStatus status, void* userdata));
};

static std::unique_ptr<MockBufferMapAsyncCallback> mockBufferMapAsyncCallback;
static void ToMockBufferMapAsyncCallback(WGPUBufferMapAsyncStatus status, void* userdata) {
    mockBufferMapAsyncCallback->Call(status, userdata);
}

class BufferValidationTest : public ValidationTest {
  protected:
    wgpu::Buffer CreateMapReadBuffer(uint64_t size) {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = size;
        descriptor.usage = wgpu::BufferUsage::MapRead;

        return device.CreateBuffer(&descriptor);
    }

    wgpu::Buffer CreateMapWriteBuffer(uint64_t size) {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = size;
        descriptor.usage = wgpu::BufferUsage::MapWrite;

        return device.CreateBuffer(&descriptor);
    }

    wgpu::CreateBufferMappedResult CreateBufferMapped(uint64_t size, wgpu::BufferUsage usage) {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = size;
        descriptor.usage = usage;

        return device.CreateBufferMapped(&descriptor);
    }

    wgpu::Buffer BufferMappedAtCreation(uint64_t size, wgpu::BufferUsage usage) {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = size;
        descriptor.usage = usage;
        descriptor.mappedAtCreation = true;

        return device.CreateBuffer(&descriptor);
    }

    void AssertMapAsyncError(wgpu::Buffer buffer, wgpu::MapMode mode, size_t offset, size_t size) {
        EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Error, _)).Times(1);

        ASSERT_DEVICE_ERROR(
            buffer.MapAsync(mode, offset, size, ToMockBufferMapAsyncCallback, nullptr));
    }

    wgpu::Queue queue;

  private:
    void SetUp() override {
        ValidationTest::SetUp();

        mockBufferMapReadCallback = std::make_unique<MockBufferMapReadCallback>();
        mockBufferMapWriteCallback = std::make_unique<MockBufferMapWriteCallback>();
        mockBufferMapAsyncCallback = std::make_unique<MockBufferMapAsyncCallback>();
        queue = device.GetDefaultQueue();
    }

    void TearDown() override {
        // Delete mocks so that expectations are checked
        mockBufferMapReadCallback = nullptr;
        mockBufferMapWriteCallback = nullptr;
        mockBufferMapAsyncCallback = nullptr;

        ValidationTest::TearDown();
    }
};

// Test case where creation should succeed
TEST_F(BufferValidationTest, CreationSuccess) {
    // Success
    {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = 4;
        descriptor.usage = wgpu::BufferUsage::Uniform;

        device.CreateBuffer(&descriptor);
    }
}

// Test restriction on usages allowed with MapRead and MapWrite
TEST_F(BufferValidationTest, CreationMapUsageRestrictions) {
    // MapRead with CopyDst is ok
    {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = 4;
        descriptor.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;

        device.CreateBuffer(&descriptor);
    }

    // MapRead with something else is an error
    {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = 4;
        descriptor.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::Uniform;

        ASSERT_DEVICE_ERROR(device.CreateBuffer(&descriptor));
    }

    // MapWrite with CopySrc is ok
    {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = 4;
        descriptor.usage = wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc;

        device.CreateBuffer(&descriptor);
    }

    // MapWrite with something else is an error
    {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = 4;
        descriptor.usage = wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::Uniform;

        ASSERT_DEVICE_ERROR(device.CreateBuffer(&descriptor));
    }
}

// Test the success case for mapping buffer for reading
TEST_F(BufferValidationTest, MapAsync_ReadSuccess) {
    wgpu::Buffer buf = CreateMapReadBuffer(4);

    buf.MapAsync(wgpu::MapMode::Read, 0, 4, ToMockBufferMapAsyncCallback, nullptr);

    EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Success, _)).Times(1);
    WaitForAllOperations(device);

    buf.Unmap();
}

// Test the success case for mapping buffer for writing
TEST_F(BufferValidationTest, MapAsync_WriteSuccess) {
    wgpu::Buffer buf = CreateMapWriteBuffer(4);

    buf.MapAsync(wgpu::MapMode::Write, 0, 4, ToMockBufferMapAsyncCallback, nullptr);

    EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Success, _)).Times(1);
    WaitForAllOperations(device);

    buf.Unmap();
}

// Test map async with a buffer that's an error
TEST_F(BufferValidationTest, MapAsync_ErrorBuffer) {
    wgpu::BufferDescriptor desc;
    desc.size = 4;
    desc.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::MapWrite;
    wgpu::Buffer buffer;
    ASSERT_DEVICE_ERROR(buffer = device.CreateBuffer(&desc));

    AssertMapAsyncError(buffer, wgpu::MapMode::Read, 0, 4);
    AssertMapAsyncError(buffer, wgpu::MapMode::Write, 0, 4);
}

// Test map async with an invalid offset and size alignment.
TEST_F(BufferValidationTest, MapAsync_OffsetSizeAlignment) {
    // Control case, both aligned to 4 is ok.
    {
        wgpu::Buffer buffer = CreateMapReadBuffer(8);
        buffer.MapAsync(wgpu::MapMode::Read, 4, 4, nullptr, nullptr);
    }
    {
        wgpu::Buffer buffer = CreateMapWriteBuffer(8);
        buffer.MapAsync(wgpu::MapMode::Write, 4, 4, nullptr, nullptr);
    }

    // Error case, offset aligned to 2 is an error.
    {
        wgpu::Buffer buffer = CreateMapReadBuffer(8);
        AssertMapAsyncError(buffer, wgpu::MapMode::Read, 2, 4);
    }
    {
        wgpu::Buffer buffer = CreateMapWriteBuffer(8);
        AssertMapAsyncError(buffer, wgpu::MapMode::Write, 2, 4);
    }

    // Error case, size aligned to 2 is an error.
    {
        wgpu::Buffer buffer = CreateMapReadBuffer(8);
        AssertMapAsyncError(buffer, wgpu::MapMode::Read, 0, 6);
    }
    {
        wgpu::Buffer buffer = CreateMapWriteBuffer(8);
        AssertMapAsyncError(buffer, wgpu::MapMode::Write, 0, 6);
    }
}

// Test map async with an invalid offset and size OOB checks
TEST_F(BufferValidationTest, MapAsync_OffsetSizeOOB) {
    // Valid case: full buffer is ok.
    {
        wgpu::Buffer buffer = CreateMapReadBuffer(8);
        buffer.MapAsync(wgpu::MapMode::Read, 0, 8, nullptr, nullptr);
    }

    // Valid case: range in the middle of the buffer is ok.
    {
        wgpu::Buffer buffer = CreateMapReadBuffer(12);
        buffer.MapAsync(wgpu::MapMode::Read, 4, 4, nullptr, nullptr);
    }

    // Valid case: empty range at the end of the buffer is ok.
    {
        wgpu::Buffer buffer = CreateMapReadBuffer(8);
        buffer.MapAsync(wgpu::MapMode::Read, 8, 0, nullptr, nullptr);
    }

    // Error case, offset is larger than the buffer size (even if size is 0).
    {
        wgpu::Buffer buffer = CreateMapReadBuffer(8);
        AssertMapAsyncError(buffer, wgpu::MapMode::Read, 12, 0);
    }

    // Error case, offset + size is larger than the buffer
    {
        wgpu::Buffer buffer = CreateMapReadBuffer(12);
        AssertMapAsyncError(buffer, wgpu::MapMode::Read, 8, 8);
    }

    // Error case, offset + size is larger than the buffer, overflow case.
    {
        wgpu::Buffer buffer = CreateMapReadBuffer(12);
        AssertMapAsyncError(buffer, wgpu::MapMode::Read, 4,
                            std::numeric_limits<size_t>::max() & ~size_t(3));
    }
}

// Test map async with a buffer that has the wrong usage
TEST_F(BufferValidationTest, MapAsync_WrongUsage) {
    {
        wgpu::BufferDescriptor desc;
        desc.usage = wgpu::BufferUsage::Vertex;
        desc.size = 4;
        wgpu::Buffer buffer = device.CreateBuffer(&desc);

        AssertMapAsyncError(buffer, wgpu::MapMode::Read, 0, 4);
        AssertMapAsyncError(buffer, wgpu::MapMode::Write, 0, 4);
    }
    {
        wgpu::Buffer buffer = CreateMapReadBuffer(4);
        AssertMapAsyncError(buffer, wgpu::MapMode::Write, 0, 4);
    }
    {
        wgpu::Buffer buffer = CreateMapWriteBuffer(4);
        AssertMapAsyncError(buffer, wgpu::MapMode::Read, 0, 4);
    }
}

// Test map async with a wrong mode
TEST_F(BufferValidationTest, MapAsync_WrongMode) {
    {
        wgpu::Buffer buffer = CreateMapReadBuffer(4);
        AssertMapAsyncError(buffer, wgpu::MapMode::None, 0, 4);
    }
    {
        wgpu::Buffer buffer = CreateMapReadBuffer(4);
        AssertMapAsyncError(buffer, wgpu::MapMode::Read | wgpu::MapMode::Write, 0, 4);
    }
}

// Test map async with a buffer that's already mapped
TEST_F(BufferValidationTest, MapAsync_AlreadyMapped) {
    {
        wgpu::Buffer buffer = CreateMapReadBuffer(4);
        buffer.MapAsync(wgpu::MapMode::Read, 0, 4, nullptr, nullptr);
        AssertMapAsyncError(buffer, wgpu::MapMode::Read, 0, 4);
    }
    {
        wgpu::Buffer buffer = BufferMappedAtCreation(4, wgpu::BufferUsage::MapRead);
        AssertMapAsyncError(buffer, wgpu::MapMode::Read, 0, 4);
    }
    {
        wgpu::Buffer buffer = CreateMapWriteBuffer(4);
        buffer.MapAsync(wgpu::MapMode::Write, 0, 4, nullptr, nullptr);
        AssertMapAsyncError(buffer, wgpu::MapMode::Write, 0, 4);
    }
    {
        wgpu::Buffer buffer = BufferMappedAtCreation(4, wgpu::BufferUsage::MapWrite);
        AssertMapAsyncError(buffer, wgpu::MapMode::Write, 0, 4);
    }
}

// Test map async with a buffer that's destroyed
TEST_F(BufferValidationTest, MapAsync_Destroy) {
    {
        wgpu::Buffer buffer = CreateMapReadBuffer(4);
        buffer.Destroy();
        AssertMapAsyncError(buffer, wgpu::MapMode::Read, 0, 4);
    }
    {
        wgpu::Buffer buffer = CreateMapWriteBuffer(4);
        buffer.Destroy();
        AssertMapAsyncError(buffer, wgpu::MapMode::Write, 0, 4);
    }
}

// Test map async but unmapping before the result is ready.
TEST_F(BufferValidationTest, MapAsync_UnmapBeforeResult) {
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);
        buf.MapAsync(wgpu::MapMode::Read, 0, 4, ToMockBufferMapAsyncCallback, nullptr);

        EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Unknown, _))
            .Times(1);
        buf.Unmap();

        // The callback shouldn't be called again.
        WaitForAllOperations(device);
    }
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);
        buf.MapAsync(wgpu::MapMode::Write, 0, 4, ToMockBufferMapAsyncCallback, nullptr);

        EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Unknown, _))
            .Times(1);
        buf.Unmap();

        // The callback shouldn't be called again.
        WaitForAllOperations(device);
    }
}

// When a MapAsync is cancelled with Unmap it might still be in flight, test doing a new request
// works as expected and we don't get the cancelled request's data.
TEST_F(BufferValidationTest, MapAsync_UnmapBeforeResultAndMapAgain) {
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);
        buf.MapAsync(wgpu::MapMode::Read, 0, 4, ToMockBufferMapAsyncCallback, this + 0);

        EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Unknown, this + 0))
            .Times(1);
        buf.Unmap();

        buf.MapAsync(wgpu::MapMode::Read, 0, 4, ToMockBufferMapAsyncCallback, this + 1);
        EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Success, this + 1))
            .Times(1);
        WaitForAllOperations(device);
    }
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);
        buf.MapAsync(wgpu::MapMode::Write, 0, 4, ToMockBufferMapAsyncCallback, this + 0);

        EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Unknown, this + 0))
            .Times(1);
        buf.Unmap();

        buf.MapAsync(wgpu::MapMode::Write, 0, 4, ToMockBufferMapAsyncCallback, this + 1);
        EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Success, this + 1))
            .Times(1);
        WaitForAllOperations(device);
    }
}

// Test map async but destroying before the result is ready.
TEST_F(BufferValidationTest, MapAsync_DestroyBeforeResult) {
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);
        buf.MapAsync(wgpu::MapMode::Read, 0, 4, ToMockBufferMapAsyncCallback, nullptr);

        EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Unknown, _))
            .Times(1);
        buf.Destroy();

        // The callback shouldn't be called again.
        WaitForAllOperations(device);
    }
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);
        buf.MapAsync(wgpu::MapMode::Write, 0, 4, ToMockBufferMapAsyncCallback, nullptr);

        EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Unknown, _))
            .Times(1);
        buf.Destroy();

        // The callback shouldn't be called again.
        WaitForAllOperations(device);
    }
}

// Test that the MapCallback isn't fired twice when unmap() is called inside the callback
TEST_F(BufferValidationTest, MapAsync_UnmapCalledInCallback) {
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);
        buf.MapAsync(wgpu::MapMode::Read, 0, 4, ToMockBufferMapAsyncCallback, nullptr);

        EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Success, _))
            .WillOnce(InvokeWithoutArgs([&]() { buf.Unmap(); }));

        WaitForAllOperations(device);
    }
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);
        buf.MapAsync(wgpu::MapMode::Write, 0, 4, ToMockBufferMapAsyncCallback, nullptr);

        EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Success, _))
            .WillOnce(InvokeWithoutArgs([&]() { buf.Unmap(); }));

        WaitForAllOperations(device);
    }
}

// Test that the MapCallback isn't fired twice when destroy() is called inside the callback
TEST_F(BufferValidationTest, MapAsync_DestroyCalledInCallback) {
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);
        buf.MapAsync(wgpu::MapMode::Read, 0, 4, ToMockBufferMapAsyncCallback, nullptr);

        EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Success, _))
            .WillOnce(InvokeWithoutArgs([&]() { buf.Destroy(); }));

        WaitForAllOperations(device);
    }
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);
        buf.MapAsync(wgpu::MapMode::Write, 0, 4, ToMockBufferMapAsyncCallback, nullptr);

        EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Success, _))
            .WillOnce(InvokeWithoutArgs([&]() { buf.Destroy(); }));

        WaitForAllOperations(device);
    }
}

// Test the success case for mapping buffer for reading
TEST_F(BufferValidationTest, MapReadAsyncSuccess) {
    wgpu::Buffer buf = CreateMapReadBuffer(4);

    buf.MapReadAsync(ToMockBufferMapReadCallback, nullptr);

    EXPECT_CALL(*mockBufferMapReadCallback,
                Call(WGPUBufferMapAsyncStatus_Success, Ne(nullptr), 4u, _))
        .Times(1);
    WaitForAllOperations(device);

    buf.Unmap();
}

// Test the success case for mapping buffer for writing
TEST_F(BufferValidationTest, MapWriteAsyncSuccess) {
    wgpu::Buffer buf = CreateMapWriteBuffer(4);

    buf.MapWriteAsync(ToMockBufferMapWriteCallback, nullptr);

    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(WGPUBufferMapAsyncStatus_Success, Ne(nullptr), 4u, _))
        .Times(1);
    WaitForAllOperations(device);

    buf.Unmap();
}

// Test the success case for CreateBufferMapped
TEST_F(BufferValidationTest, CreateBufferMappedSuccess) {
    wgpu::CreateBufferMappedResult result = CreateBufferMapped(4, wgpu::BufferUsage::MapWrite);
    ASSERT_NE(result.data, nullptr);
    ASSERT_EQ(result.dataLength, 4u);
    result.buffer.Unmap();
}

// Test the success case for non-mappable CreateBufferMapped
TEST_F(BufferValidationTest, NonMappableCreateBufferMappedSuccess) {
    wgpu::CreateBufferMappedResult result = CreateBufferMapped(4, wgpu::BufferUsage::CopySrc);
    ASSERT_NE(result.data, nullptr);
    ASSERT_EQ(result.dataLength, 4u);
    result.buffer.Unmap();
}

// Test the success case for mappedAtCreation
TEST_F(BufferValidationTest, MappedAtCreationSuccess) {
    BufferMappedAtCreation(4, wgpu::BufferUsage::MapWrite);
}

// Test the success case for mappedAtCreation for a non-mappable usage
TEST_F(BufferValidationTest, NonMappableMappedAtCreationSuccess) {
    BufferMappedAtCreation(4, wgpu::BufferUsage::CopySrc);
}

// Test there is an error when mappedAtCreation is set but the size isn't aligned to 4.
TEST_F(BufferValidationTest, MappedAtCreationSizeAlignment) {
    ASSERT_DEVICE_ERROR(BufferMappedAtCreation(2, wgpu::BufferUsage::MapWrite));
}

// Test map reading a buffer with wrong current usage
TEST_F(BufferValidationTest, MapReadWrongUsage) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::CopyDst;

    wgpu::Buffer buf = device.CreateBuffer(&descriptor);

    EXPECT_CALL(*mockBufferMapReadCallback, Call(WGPUBufferMapAsyncStatus_Error, nullptr, 0u, _))
        .Times(1);

    ASSERT_DEVICE_ERROR(buf.MapReadAsync(ToMockBufferMapReadCallback, nullptr));
}

// Test map writing a buffer with wrong current usage
TEST_F(BufferValidationTest, MapWriteWrongUsage) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::CopySrc;

    wgpu::Buffer buf = device.CreateBuffer(&descriptor);

    EXPECT_CALL(*mockBufferMapWriteCallback, Call(WGPUBufferMapAsyncStatus_Error, nullptr, 0u, _))
        .Times(1);

    ASSERT_DEVICE_ERROR(buf.MapWriteAsync(ToMockBufferMapWriteCallback, nullptr));
}

// Test map reading a buffer that is already mapped
TEST_F(BufferValidationTest, MapReadAlreadyMapped) {
    wgpu::Buffer buf = CreateMapReadBuffer(4);

    buf.MapReadAsync(ToMockBufferMapReadCallback, this + 0);
    EXPECT_CALL(*mockBufferMapReadCallback,
                Call(WGPUBufferMapAsyncStatus_Success, Ne(nullptr), 4u, this + 0))
        .Times(1);

    EXPECT_CALL(*mockBufferMapReadCallback,
                Call(WGPUBufferMapAsyncStatus_Error, nullptr, 0u, this + 1))
        .Times(1);
    ASSERT_DEVICE_ERROR(buf.MapReadAsync(ToMockBufferMapReadCallback, this + 1));

    WaitForAllOperations(device);
}

// Test map writing a buffer that is already mapped
TEST_F(BufferValidationTest, MapWriteAlreadyMapped) {
    wgpu::Buffer buf = CreateMapWriteBuffer(4);

    buf.MapWriteAsync(ToMockBufferMapWriteCallback, this + 0);
    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(WGPUBufferMapAsyncStatus_Success, Ne(nullptr), 4u, this + 0))
        .Times(1);

    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(WGPUBufferMapAsyncStatus_Error, nullptr, 0u, this + 1))
        .Times(1);
    ASSERT_DEVICE_ERROR(buf.MapWriteAsync(ToMockBufferMapWriteCallback, this + 1));

    WaitForAllOperations(device);
}
// TODO(cwallez@chromium.org) Test a MapWrite and already MapRead and vice-versa

// Test unmapping before having the result gives UNKNOWN - for reading
TEST_F(BufferValidationTest, MapReadUnmapBeforeResult) {
    wgpu::Buffer buf = CreateMapReadBuffer(4);

    buf.MapReadAsync(ToMockBufferMapReadCallback, nullptr);

    EXPECT_CALL(*mockBufferMapReadCallback, Call(WGPUBufferMapAsyncStatus_Unknown, nullptr, 0u, _))
        .Times(1);
    buf.Unmap();

    // The callback shouldn't be called again.
    WaitForAllOperations(device);
}

// Test unmapping before having the result gives UNKNOWN - for writing
TEST_F(BufferValidationTest, MapWriteUnmapBeforeResult) {
    wgpu::Buffer buf = CreateMapWriteBuffer(4);

    buf.MapWriteAsync(ToMockBufferMapWriteCallback, nullptr);

    EXPECT_CALL(*mockBufferMapWriteCallback, Call(WGPUBufferMapAsyncStatus_Unknown, nullptr, 0u, _))
        .Times(1);
    buf.Unmap();

    // The callback shouldn't be called again.
    WaitForAllOperations(device);
}

// Test destroying the buffer before having the result gives UNKNOWN - for reading
// TODO(cwallez@chromium.org) currently this doesn't work because the buffer doesn't know
// when its external ref count reaches 0.
TEST_F(BufferValidationTest, DISABLED_MapReadDestroyBeforeResult) {
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);

        buf.MapReadAsync(ToMockBufferMapReadCallback, nullptr);

        EXPECT_CALL(*mockBufferMapReadCallback,
                    Call(WGPUBufferMapAsyncStatus_Unknown, nullptr, 0u, _))
            .Times(1);
    }

    // The callback shouldn't be called again.
    WaitForAllOperations(device);
}

// Test destroying the buffer before having the result gives UNKNOWN - for writing
// TODO(cwallez@chromium.org) currently this doesn't work because the buffer doesn't know
// when its external ref count reaches 0.
TEST_F(BufferValidationTest, DISABLED_MapWriteDestroyBeforeResult) {
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);

        buf.MapWriteAsync(ToMockBufferMapWriteCallback, nullptr);

        EXPECT_CALL(*mockBufferMapWriteCallback,
                    Call(WGPUBufferMapAsyncStatus_Unknown, nullptr, 0u, _))
            .Times(1);
    }

    // The callback shouldn't be called again.
    WaitForAllOperations(device);
}

// When a MapRead is cancelled with Unmap it might still be in flight, test doing a new request
// works as expected and we don't get the cancelled request's data.
TEST_F(BufferValidationTest, MapReadUnmapBeforeResultThenMapAgain) {
    wgpu::Buffer buf = CreateMapReadBuffer(4);

    buf.MapReadAsync(ToMockBufferMapReadCallback, this + 0);

    EXPECT_CALL(*mockBufferMapReadCallback,
                Call(WGPUBufferMapAsyncStatus_Unknown, nullptr, 0u, this + 0))
        .Times(1);
    buf.Unmap();

    buf.MapReadAsync(ToMockBufferMapReadCallback, this + 1);

    EXPECT_CALL(*mockBufferMapReadCallback,
                Call(WGPUBufferMapAsyncStatus_Success, Ne(nullptr), 4u, this + 1))
        .Times(1);
    WaitForAllOperations(device);
}
// TODO(cwallez@chromium.org) Test a MapWrite and already MapRead and vice-versa

// When a MapWrite is cancelled with Unmap it might still be in flight, test doing a new request
// works as expected and we don't get the cancelled request's data.
TEST_F(BufferValidationTest, MapWriteUnmapBeforeResultThenMapAgain) {
    wgpu::Buffer buf = CreateMapWriteBuffer(4);

    buf.MapWriteAsync(ToMockBufferMapWriteCallback, this + 0);

    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(WGPUBufferMapAsyncStatus_Unknown, nullptr, 0u, this + 0))
        .Times(1);
    buf.Unmap();

    buf.MapWriteAsync(ToMockBufferMapWriteCallback, this + 1);

    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(WGPUBufferMapAsyncStatus_Success, Ne(nullptr), 4u, this + 1))
        .Times(1);
    WaitForAllOperations(device);
}

// Test that the MapReadCallback isn't fired twice when unmap() is called inside the callback
TEST_F(BufferValidationTest, UnmapInsideMapReadCallback) {
    wgpu::Buffer buf = CreateMapReadBuffer(4);

    buf.MapReadAsync(ToMockBufferMapReadCallback, nullptr);

    EXPECT_CALL(*mockBufferMapReadCallback,
                Call(WGPUBufferMapAsyncStatus_Success, Ne(nullptr), 4u, _))
        .WillOnce(InvokeWithoutArgs([&]() { buf.Unmap(); }));

    WaitForAllOperations(device);
}

// Test that the MapWriteCallback isn't fired twice when unmap() is called inside the callback
TEST_F(BufferValidationTest, UnmapInsideMapWriteCallback) {
    wgpu::Buffer buf = CreateMapWriteBuffer(4);

    buf.MapWriteAsync(ToMockBufferMapWriteCallback, nullptr);

    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(WGPUBufferMapAsyncStatus_Success, Ne(nullptr), 4u, _))
        .WillOnce(InvokeWithoutArgs([&]() { buf.Unmap(); }));

    WaitForAllOperations(device);
}

// Test that the MapReadCallback isn't fired twice the buffer external refcount reaches 0 in the
// callback
TEST_F(BufferValidationTest, DestroyInsideMapReadCallback) {
    wgpu::Buffer buf = CreateMapReadBuffer(4);

    buf.MapReadAsync(ToMockBufferMapReadCallback, nullptr);

    EXPECT_CALL(*mockBufferMapReadCallback,
                Call(WGPUBufferMapAsyncStatus_Success, Ne(nullptr), 4u, _))
        .WillOnce(InvokeWithoutArgs([&]() { buf = wgpu::Buffer(); }));

    WaitForAllOperations(device);
}

// Test that the MapWriteCallback isn't fired twice the buffer external refcount reaches 0 in the
// callback
TEST_F(BufferValidationTest, DestroyInsideMapWriteCallback) {
    wgpu::Buffer buf = CreateMapWriteBuffer(4);

    buf.MapWriteAsync(ToMockBufferMapWriteCallback, nullptr);

    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(WGPUBufferMapAsyncStatus_Success, Ne(nullptr), 4u, _))
        .WillOnce(InvokeWithoutArgs([&]() { buf = wgpu::Buffer(); }));

    WaitForAllOperations(device);
}

// Test that it is valid to destroy an unmapped buffer
TEST_F(BufferValidationTest, DestroyUnmappedBuffer) {
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);
        buf.Destroy();
    }
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);
        buf.Destroy();
    }
}

// Test that it is valid to destroy a mapped buffer
TEST_F(BufferValidationTest, DestroyMappedBuffer) {
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);
        buf.MapReadAsync(ToMockBufferMapReadCallback, nullptr);
        buf.Destroy();
    }
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);
        buf.MapWriteAsync(ToMockBufferMapWriteCallback, nullptr);
        buf.Destroy();
    }
}

// Test that destroying a buffer implicitly unmaps it
TEST_F(BufferValidationTest, DestroyMappedBufferCausesImplicitUnmap) {
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);
        buf.MapReadAsync(ToMockBufferMapReadCallback, this + 0);
        // Buffer is destroyed. Callback should be called with UNKNOWN status
        EXPECT_CALL(*mockBufferMapReadCallback,
                    Call(WGPUBufferMapAsyncStatus_Unknown, nullptr, 0, this + 0))
            .Times(1);
        buf.Destroy();
        WaitForAllOperations(device);
    }
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);
        buf.MapWriteAsync(ToMockBufferMapWriteCallback, this + 1);
        // Buffer is destroyed. Callback should be called with UNKNOWN status
        EXPECT_CALL(*mockBufferMapWriteCallback,
                    Call(WGPUBufferMapAsyncStatus_Unknown, nullptr, 0, this + 1))
            .Times(1);
        buf.Destroy();
        WaitForAllOperations(device);
    }
}

// Test that it is valid to Destroy a destroyed buffer
TEST_F(BufferValidationTest, DestroyDestroyedBuffer) {
    wgpu::Buffer buf = CreateMapWriteBuffer(4);
    buf.Destroy();
    buf.Destroy();
}

// Test that it is invalid to Unmap a destroyed buffer
TEST_F(BufferValidationTest, UnmapDestroyedBuffer) {
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);
        buf.Destroy();
        ASSERT_DEVICE_ERROR(buf.Unmap());
    }
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);
        buf.Destroy();
        ASSERT_DEVICE_ERROR(buf.Unmap());
    }
}

// Test that it is invalid to map a destroyed buffer
TEST_F(BufferValidationTest, MapDestroyedBuffer) {
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);
        buf.Destroy();
        ASSERT_DEVICE_ERROR(buf.MapReadAsync(ToMockBufferMapReadCallback, nullptr));
    }
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);
        buf.Destroy();
        ASSERT_DEVICE_ERROR(buf.MapWriteAsync(ToMockBufferMapWriteCallback, nullptr));
    }
}

// Test that is is invalid to Map a mapped buffer
TEST_F(BufferValidationTest, MapMappedBuffer) {
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);
        buf.MapReadAsync(ToMockBufferMapReadCallback, nullptr);
        ASSERT_DEVICE_ERROR(buf.MapReadAsync(ToMockBufferMapReadCallback, nullptr));
        WaitForAllOperations(device);
    }
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);
        buf.MapWriteAsync(ToMockBufferMapWriteCallback, nullptr);
        ASSERT_DEVICE_ERROR(buf.MapWriteAsync(ToMockBufferMapWriteCallback, nullptr));
        WaitForAllOperations(device);
    }
}

// Test that is is invalid to Map a CreateBufferMapped buffer
TEST_F(BufferValidationTest, MapCreateBufferMappedBuffer) {
    {
        wgpu::Buffer buf = CreateBufferMapped(4, wgpu::BufferUsage::MapRead).buffer;
        ASSERT_DEVICE_ERROR(buf.MapReadAsync(ToMockBufferMapReadCallback, nullptr));
        WaitForAllOperations(device);
    }
    {
        wgpu::Buffer buf = CreateBufferMapped(4, wgpu::BufferUsage::MapWrite).buffer;
        ASSERT_DEVICE_ERROR(buf.MapWriteAsync(ToMockBufferMapWriteCallback, nullptr));
        WaitForAllOperations(device);
    }
}

// Test that is is invalid to Map a buffer mapped at creation.
TEST_F(BufferValidationTest, MapBufferMappedAtCreation) {
    {
        wgpu::Buffer buf = BufferMappedAtCreation(4, wgpu::BufferUsage::MapRead);
        ASSERT_DEVICE_ERROR(buf.MapReadAsync(ToMockBufferMapReadCallback, nullptr));
        WaitForAllOperations(device);
    }
    {
        wgpu::Buffer buf = BufferMappedAtCreation(4, wgpu::BufferUsage::MapWrite);
        ASSERT_DEVICE_ERROR(buf.MapWriteAsync(ToMockBufferMapWriteCallback, nullptr));
        WaitForAllOperations(device);
    }
}

// Test that it is valid to submit a buffer in a queue with a map usage if it is unmapped
TEST_F(BufferValidationTest, SubmitBufferWithMapUsage) {
    wgpu::BufferDescriptor descriptorA;
    descriptorA.size = 4;
    descriptorA.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;

    wgpu::BufferDescriptor descriptorB;
    descriptorB.size = 4;
    descriptorB.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;

    wgpu::Buffer bufA = device.CreateBuffer(&descriptorA);
    wgpu::Buffer bufB = device.CreateBuffer(&descriptorB);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyBufferToBuffer(bufA, 0, bufB, 0, 4);
    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);
}

// Test that it is invalid to submit a mapped buffer in a queue
TEST_F(BufferValidationTest, SubmitMappedBuffer) {
    wgpu::BufferDescriptor descriptorA;
    descriptorA.size = 4;
    descriptorA.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;

    wgpu::BufferDescriptor descriptorB;
    descriptorB.size = 4;
    descriptorB.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
    {
        wgpu::Buffer bufA = device.CreateBuffer(&descriptorA);
        wgpu::Buffer bufB = device.CreateBuffer(&descriptorB);

        bufA.MapAsync(wgpu::MapMode::Write, 0, 4, nullptr, nullptr);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(bufA, 0, bufB, 0, 4);
        wgpu::CommandBuffer commands = encoder.Finish();
        ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));
        WaitForAllOperations(device);
    }
    {
        wgpu::Buffer bufA = device.CreateBuffer(&descriptorA);
        wgpu::Buffer bufB = device.CreateBuffer(&descriptorB);

        bufB.MapAsync(wgpu::MapMode::Read, 0, 4, nullptr, nullptr);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(bufA, 0, bufB, 0, 4);
        wgpu::CommandBuffer commands = encoder.Finish();
        ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));
        WaitForAllOperations(device);
    }
    {
        wgpu::Buffer bufA = device.CreateBuffer(&descriptorA);
        wgpu::Buffer bufB = device.CreateBuffer(&descriptorB);

        bufA.MapWriteAsync(ToMockBufferMapWriteCallback, nullptr);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(bufA, 0, bufB, 0, 4);
        wgpu::CommandBuffer commands = encoder.Finish();
        ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));
        WaitForAllOperations(device);
    }
    {
        wgpu::Buffer bufA = device.CreateBuffer(&descriptorA);
        wgpu::Buffer bufB = device.CreateBuffer(&descriptorB);

        bufB.MapReadAsync(ToMockBufferMapReadCallback, nullptr);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(bufA, 0, bufB, 0, 4);
        wgpu::CommandBuffer commands = encoder.Finish();
        ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));
        WaitForAllOperations(device);
    }
    {
        wgpu::Buffer bufA = device.CreateBufferMapped(&descriptorA).buffer;
        wgpu::Buffer bufB = device.CreateBuffer(&descriptorB);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(bufA, 0, bufB, 0, 4);
        wgpu::CommandBuffer commands = encoder.Finish();
        ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));
        WaitForAllOperations(device);
    }
    {
        wgpu::Buffer bufA = device.CreateBuffer(&descriptorA);
        wgpu::Buffer bufB = device.CreateBufferMapped(&descriptorB).buffer;

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(bufA, 0, bufB, 0, 4);
        wgpu::CommandBuffer commands = encoder.Finish();
        ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));
        WaitForAllOperations(device);
    }
    {
        wgpu::BufferDescriptor mappedBufferDesc = descriptorA;
        mappedBufferDesc.mappedAtCreation = true;
        wgpu::Buffer bufA = device.CreateBuffer(&mappedBufferDesc);
        wgpu::Buffer bufB = device.CreateBuffer(&descriptorB);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(bufA, 0, bufB, 0, 4);
        wgpu::CommandBuffer commands = encoder.Finish();
        ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));
        WaitForAllOperations(device);
    }
    {
        wgpu::BufferDescriptor mappedBufferDesc = descriptorB;
        mappedBufferDesc.mappedAtCreation = true;
        wgpu::Buffer bufA = device.CreateBuffer(&descriptorA);
        wgpu::Buffer bufB = device.CreateBuffer(&mappedBufferDesc);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(bufA, 0, bufB, 0, 4);
        wgpu::CommandBuffer commands = encoder.Finish();
        ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));
        WaitForAllOperations(device);
    }
}

// Test that it is invalid to submit a destroyed buffer in a queue
TEST_F(BufferValidationTest, SubmitDestroyedBuffer) {
    wgpu::BufferDescriptor descriptorA;
    descriptorA.size = 4;
    descriptorA.usage = wgpu::BufferUsage::CopySrc;

    wgpu::BufferDescriptor descriptorB;
    descriptorB.size = 4;
    descriptorB.usage = wgpu::BufferUsage::CopyDst;

    wgpu::Buffer bufA = device.CreateBuffer(&descriptorA);
    wgpu::Buffer bufB = device.CreateBuffer(&descriptorB);

    bufA.Destroy();
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyBufferToBuffer(bufA, 0, bufB, 0, 4);
    wgpu::CommandBuffer commands = encoder.Finish();
    ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));
}

// Test that a map usage is required to call Unmap
TEST_F(BufferValidationTest, UnmapWithoutMapUsage) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buf = device.CreateBuffer(&descriptor);

    ASSERT_DEVICE_ERROR(buf.Unmap());
}

// Test that it is valid to call Unmap on a buffer that is not mapped
TEST_F(BufferValidationTest, UnmapUnmappedBuffer) {
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);
        // Buffer starts unmapped. Unmap should succeed.
        buf.Unmap();
        buf.MapReadAsync(ToMockBufferMapReadCallback, nullptr);
        buf.Unmap();
        // Unmapping twice should succeed
        buf.Unmap();
    }
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);
        // Buffer starts unmapped. Unmap should succeed.
        buf.Unmap();
        buf.MapWriteAsync(ToMockBufferMapWriteCallback, nullptr);
        // Unmapping twice should succeed
        buf.Unmap();
        buf.Unmap();
    }
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);
        // Buffer starts unmapped. Unmap should succeed.
        buf.Unmap();
        buf.MapAsync(wgpu::MapMode::Read, 0, 4, nullptr, nullptr);
        buf.Unmap();
        // Unmapping twice should succeed
        buf.Unmap();
    }
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);
        // Buffer starts unmapped. Unmap should succeed.
        buf.Unmap();
        buf.MapAsync(wgpu::MapMode::Write, 0, 4, nullptr, nullptr);
        // Unmapping twice should succeed
        buf.Unmap();
        buf.Unmap();
    }
}

// Test that it is invalid to call GetMappedRange on an unmapped buffer.
TEST_F(BufferValidationTest, GetMappedRange_OnUnmappedBuffer) {
    // Unmapped at creation case.
    {
        wgpu::BufferDescriptor desc;
        desc.size = 4;
        desc.usage = wgpu::BufferUsage::CopySrc;
        wgpu::Buffer buf = device.CreateBuffer(&desc);

        ASSERT_EQ(nullptr, buf.GetMappedRange());
        ASSERT_EQ(nullptr, buf.GetConstMappedRange());
    }

    // Unmapped after CreateBufferMapped case.
    {
        wgpu::Buffer buf = CreateBufferMapped(4, wgpu::BufferUsage::CopySrc).buffer;
        buf.Unmap();

        ASSERT_EQ(nullptr, buf.GetMappedRange());
        ASSERT_EQ(nullptr, buf.GetConstMappedRange());
    }

    // Unmapped after mappedAtCreation case.
    {
        wgpu::Buffer buf = BufferMappedAtCreation(4, wgpu::BufferUsage::CopySrc);
        buf.Unmap();

        ASSERT_EQ(nullptr, buf.GetMappedRange());
        ASSERT_EQ(nullptr, buf.GetConstMappedRange());
    }

    // Unmapped after MapReadAsync case.
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);

        buf.MapReadAsync(ToMockBufferMapReadCallback, nullptr);
        EXPECT_CALL(*mockBufferMapReadCallback,
                    Call(WGPUBufferMapAsyncStatus_Success, Ne(nullptr), 4u, _))
            .Times(1);
        WaitForAllOperations(device);
        buf.Unmap();

        ASSERT_EQ(nullptr, buf.GetMappedRange());
        ASSERT_EQ(nullptr, buf.GetConstMappedRange());
    }

    // Unmapped after MapWriteAsync case.
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);
        buf.MapWriteAsync(ToMockBufferMapWriteCallback, nullptr);
        EXPECT_CALL(*mockBufferMapWriteCallback,
                    Call(WGPUBufferMapAsyncStatus_Success, Ne(nullptr), 4u, _))
            .Times(1);
        WaitForAllOperations(device);
        buf.Unmap();

        ASSERT_EQ(nullptr, buf.GetMappedRange());
        ASSERT_EQ(nullptr, buf.GetConstMappedRange());
    }
    // Unmapped after MapAsync read case.
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);

        buf.MapAsync(wgpu::MapMode::Read, 0, 4, ToMockBufferMapAsyncCallback, nullptr);
        EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Success, _))
            .Times(1);
        WaitForAllOperations(device);
        buf.Unmap();

        ASSERT_EQ(nullptr, buf.GetMappedRange());
        ASSERT_EQ(nullptr, buf.GetConstMappedRange());
    }

    // Unmapped after MapAsync write case.
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);
        buf.MapAsync(wgpu::MapMode::Write, 0, 4, ToMockBufferMapAsyncCallback, nullptr);
        EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Success, _))
            .Times(1);
        WaitForAllOperations(device);
        buf.Unmap();

        ASSERT_EQ(nullptr, buf.GetMappedRange());
        ASSERT_EQ(nullptr, buf.GetConstMappedRange());
    }
}

// Test that it is invalid to call GetMappedRange on a destroyed buffer.
TEST_F(BufferValidationTest, GetMappedRange_OnDestroyedBuffer) {
    // Destroyed after creation case.
    {
        wgpu::BufferDescriptor desc;
        desc.size = 4;
        desc.usage = wgpu::BufferUsage::CopySrc;
        wgpu::Buffer buf = device.CreateBuffer(&desc);
        buf.Destroy();

        ASSERT_EQ(nullptr, buf.GetMappedRange());
        ASSERT_EQ(nullptr, buf.GetConstMappedRange());
    }

    // Destroyed after CreateBufferMapped case.
    {
        wgpu::Buffer buf = CreateBufferMapped(4, wgpu::BufferUsage::CopySrc).buffer;
        buf.Destroy();

        ASSERT_EQ(nullptr, buf.GetMappedRange());
        ASSERT_EQ(nullptr, buf.GetConstMappedRange());
    }

    // Destroyed after mappedAtCreation case.
    {
        wgpu::Buffer buf = BufferMappedAtCreation(4, wgpu::BufferUsage::CopySrc);
        buf.Destroy();

        ASSERT_EQ(nullptr, buf.GetMappedRange());
        ASSERT_EQ(nullptr, buf.GetConstMappedRange());
    }

    // Destroyed after MapReadAsync case.
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);

        buf.MapReadAsync(ToMockBufferMapReadCallback, nullptr);
        EXPECT_CALL(*mockBufferMapReadCallback,
                    Call(WGPUBufferMapAsyncStatus_Success, Ne(nullptr), 4u, _))
            .Times(1);
        WaitForAllOperations(device);
        buf.Destroy();

        ASSERT_EQ(nullptr, buf.GetMappedRange());
        ASSERT_EQ(nullptr, buf.GetConstMappedRange());
    }

    // Destroyed after MapWriteAsync case.
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);
        buf.MapWriteAsync(ToMockBufferMapWriteCallback, nullptr);
        EXPECT_CALL(*mockBufferMapWriteCallback,
                    Call(WGPUBufferMapAsyncStatus_Success, Ne(nullptr), 4u, _))
            .Times(1);
        WaitForAllOperations(device);
        buf.Destroy();

        ASSERT_EQ(nullptr, buf.GetMappedRange());
        ASSERT_EQ(nullptr, buf.GetConstMappedRange());
    }
    // Destroyed after MapAsync read case.
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);

        buf.MapAsync(wgpu::MapMode::Read, 0, 4, ToMockBufferMapAsyncCallback, nullptr);
        EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Success, _))
            .Times(1);
        WaitForAllOperations(device);
        buf.Destroy();

        ASSERT_EQ(nullptr, buf.GetMappedRange());
        ASSERT_EQ(nullptr, buf.GetConstMappedRange());
    }

    // Destroyed after MapAsync write case.
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);
        buf.MapAsync(wgpu::MapMode::Write, 0, 4, ToMockBufferMapAsyncCallback, nullptr);
        EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Success, _))
            .Times(1);
        WaitForAllOperations(device);
        buf.Destroy();

        ASSERT_EQ(nullptr, buf.GetMappedRange());
        ASSERT_EQ(nullptr, buf.GetConstMappedRange());
    }
}

// Test that it is invalid to call GetMappedRange on a buffer after MapReadAsync
TEST_F(BufferValidationTest, GetMappedRange_OnMappedForReading) {
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);

        buf.MapReadAsync(ToMockBufferMapReadCallback, nullptr);
        EXPECT_CALL(*mockBufferMapReadCallback,
                    Call(WGPUBufferMapAsyncStatus_Success, Ne(nullptr), 4u, _))
            .Times(1);
        WaitForAllOperations(device);

        ASSERT_EQ(nullptr, buf.GetMappedRange());
    }
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);

        buf.MapAsync(wgpu::MapMode::Read, 0, 4, ToMockBufferMapAsyncCallback, nullptr);
        EXPECT_CALL(*mockBufferMapAsyncCallback, Call(WGPUBufferMapAsyncStatus_Success, _))
            .Times(1);
        WaitForAllOperations(device);

        ASSERT_EQ(nullptr, buf.GetMappedRange());
    }
}

// Test valid cases to call GetMappedRange on a buffer.
TEST_F(BufferValidationTest, GetMappedRange_ValidBufferStateCases) {
    // GetMappedRange after CreateBufferMapped case.
    {
        wgpu::CreateBufferMappedResult result = CreateBufferMapped(4, wgpu::BufferUsage::CopySrc);
        ASSERT_NE(result.buffer.GetConstMappedRange(), nullptr);
        ASSERT_EQ(result.buffer.GetConstMappedRange(), result.buffer.GetMappedRange());
        ASSERT_EQ(result.buffer.GetConstMappedRange(), result.data);
    }

    // GetMappedRange after mappedAtCreation case.
    {
        wgpu::Buffer buffer = BufferMappedAtCreation(4, wgpu::BufferUsage::CopySrc);
        ASSERT_NE(buffer.GetConstMappedRange(), nullptr);
        ASSERT_EQ(buffer.GetConstMappedRange(), buffer.GetMappedRange());
    }

    // GetMappedRange after MapReadAsync case.
    {
        wgpu::Buffer buf = CreateMapReadBuffer(4);

        buf.MapReadAsync(ToMockBufferMapReadCallback, nullptr);

        const void* mappedPointer = nullptr;
        EXPECT_CALL(*mockBufferMapReadCallback,
                    Call(WGPUBufferMapAsyncStatus_Success, Ne(nullptr), 4u, _))
            .WillOnce(SaveArg<1>(&mappedPointer));

        WaitForAllOperations(device);

        ASSERT_NE(buf.GetConstMappedRange(), nullptr);
        ASSERT_EQ(buf.GetConstMappedRange(), mappedPointer);
    }

    // GetMappedRange after MapWriteAsync case.
    {
        wgpu::Buffer buf = CreateMapWriteBuffer(4);
        buf.MapWriteAsync(ToMockBufferMapWriteCallback, nullptr);

        const void* mappedPointer = nullptr;
        EXPECT_CALL(*mockBufferMapWriteCallback,
                    Call(WGPUBufferMapAsyncStatus_Success, Ne(nullptr), 4u, _))
            .WillOnce(SaveArg<1>(&mappedPointer));

        WaitForAllOperations(device);

        ASSERT_NE(buf.GetConstMappedRange(), nullptr);
        ASSERT_EQ(buf.GetConstMappedRange(), buf.GetMappedRange());
        ASSERT_EQ(buf.GetConstMappedRange(), mappedPointer);
    }
}

// Test valid cases to call GetMappedRange on an error buffer.
TEST_F(BufferValidationTest, GetMappedRange_OnErrorBuffer) {
    wgpu::BufferDescriptor desc;
    desc.size = 4;
    desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::MapRead;

    uint64_t kStupidLarge = uint64_t(1) << uint64_t(63);

    // GetMappedRange after CreateBufferMapped a zero-sized buffer returns a non-nullptr.
    // This is to check we don't do a malloc(0).
    {
        wgpu::CreateBufferMappedResult result;
        ASSERT_DEVICE_ERROR(result = CreateBufferMapped(
                                0, wgpu::BufferUsage::Storage | wgpu::BufferUsage::MapRead));

        ASSERT_NE(result.buffer.GetConstMappedRange(), nullptr);
        ASSERT_EQ(result.buffer.GetConstMappedRange(), result.buffer.GetMappedRange());
        ASSERT_EQ(result.buffer.GetConstMappedRange(), result.data);
    }

    // GetMappedRange after CreateBufferMapped non-OOM returns a non-nullptr.
    {
        wgpu::CreateBufferMappedResult result;
        ASSERT_DEVICE_ERROR(result = CreateBufferMapped(
                                4, wgpu::BufferUsage::Storage | wgpu::BufferUsage::MapRead));

        ASSERT_NE(result.buffer.GetConstMappedRange(), nullptr);
        ASSERT_EQ(result.buffer.GetConstMappedRange(), result.buffer.GetMappedRange());
        ASSERT_EQ(result.buffer.GetConstMappedRange(), result.data);
    }

    // GetMappedRange after CreateBufferMapped OOM case returns nullptr.
    {
        wgpu::CreateBufferMappedResult result;
        ASSERT_DEVICE_ERROR(result =
                                CreateBufferMapped(kStupidLarge, wgpu::BufferUsage::Storage |
                                                                     wgpu::BufferUsage::MapRead));

        ASSERT_EQ(result.buffer.GetConstMappedRange(), nullptr);
        ASSERT_EQ(result.buffer.GetConstMappedRange(), result.buffer.GetMappedRange());
        ASSERT_EQ(result.buffer.GetConstMappedRange(), result.data);
    }

    // GetMappedRange after mappedAtCreation a zero-sized buffer returns a non-nullptr.
    // This is to check we don't do a malloc(0).
    {
        wgpu::Buffer buffer;
        ASSERT_DEVICE_ERROR(buffer = BufferMappedAtCreation(
                                0, wgpu::BufferUsage::Storage | wgpu::BufferUsage::MapRead));

        ASSERT_NE(buffer.GetConstMappedRange(), nullptr);
        ASSERT_EQ(buffer.GetConstMappedRange(), buffer.GetMappedRange());
    }

    // GetMappedRange after mappedAtCreation non-OOM returns a non-nullptr.
    {
        wgpu::Buffer buffer;
        ASSERT_DEVICE_ERROR(buffer = BufferMappedAtCreation(
                                4, wgpu::BufferUsage::Storage | wgpu::BufferUsage::MapRead));

        ASSERT_NE(buffer.GetConstMappedRange(), nullptr);
        ASSERT_EQ(buffer.GetConstMappedRange(), buffer.GetMappedRange());
    }

    // GetMappedRange after mappedAtCreation OOM case returns nullptr.
    {
        wgpu::Buffer buffer;
        ASSERT_DEVICE_ERROR(
            buffer = BufferMappedAtCreation(
                kStupidLarge, wgpu::BufferUsage::Storage | wgpu::BufferUsage::MapRead));

        ASSERT_EQ(buffer.GetConstMappedRange(), nullptr);
        ASSERT_EQ(buffer.GetConstMappedRange(), buffer.GetMappedRange());
    }
}

// Test validation of the GetMappedRange parameters
TEST_F(BufferValidationTest, GetMappedRange_OffsetSizeOOB) {
    // Valid case: full range is ok
    {
        wgpu::Buffer buffer = CreateMapWriteBuffer(8);
        buffer.MapAsync(wgpu::MapMode::Write, 0, 8, nullptr, nullptr);
        EXPECT_NE(buffer.GetMappedRange(0, 8), nullptr);
    }

    // Valid case: full range is ok with defaulted MapAsync size
    {
        wgpu::Buffer buffer = CreateMapWriteBuffer(8);
        buffer.MapAsync(wgpu::MapMode::Write, 0, 0, nullptr, nullptr);
        EXPECT_NE(buffer.GetMappedRange(0, 8), nullptr);
    }

    // Valid case: empty range at the end is ok
    {
        wgpu::Buffer buffer = CreateMapWriteBuffer(8);
        buffer.MapAsync(wgpu::MapMode::Write, 0, 8, nullptr, nullptr);
        EXPECT_NE(buffer.GetMappedRange(8, 0), nullptr);
    }

    // Valid case: range in the middle is ok.
    {
        wgpu::Buffer buffer = CreateMapWriteBuffer(12);
        buffer.MapAsync(wgpu::MapMode::Write, 0, 12, nullptr, nullptr);
        EXPECT_NE(buffer.GetMappedRange(4, 4), nullptr);
    }

    // Error case: offset is larger than the mapped range (even with size = 0)
    {
        wgpu::Buffer buffer = CreateMapWriteBuffer(8);
        buffer.MapAsync(wgpu::MapMode::Write, 0, 8, nullptr, nullptr);
        EXPECT_EQ(buffer.GetMappedRange(9, 0), nullptr);
    }

    // Error case: offset + size is larger than the mapped range
    {
        wgpu::Buffer buffer = CreateMapWriteBuffer(12);
        buffer.MapAsync(wgpu::MapMode::Write, 0, 12, nullptr, nullptr);
        EXPECT_EQ(buffer.GetMappedRange(8, 5), nullptr);
    }

    // Error case: offset + size is larger than the mapped range, overflow case
    {
        wgpu::Buffer buffer = CreateMapWriteBuffer(12);
        buffer.MapAsync(wgpu::MapMode::Write, 0, 12, nullptr, nullptr);
        EXPECT_EQ(buffer.GetMappedRange(8, std::numeric_limits<size_t>::max()), nullptr);
    }

    // Error case: offset is before the start of the range
    {
        wgpu::Buffer buffer = CreateMapWriteBuffer(12);
        buffer.MapAsync(wgpu::MapMode::Write, 4, 4, nullptr, nullptr);
        EXPECT_EQ(buffer.GetMappedRange(3, 4), nullptr);
    }
}
