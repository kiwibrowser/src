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

#include "tests/unittests/wire/WireTest.h"

using namespace testing;
using namespace dawn_wire;

namespace {

    // Mock classes to add expectations on the wire calling callbacks
    class MockBufferMapReadCallback {
      public:
        MOCK_METHOD4(Call,
                     void(DawnBufferMapAsyncStatus status,
                          const uint32_t* ptr,
                          uint64_t dataLength,
                          void* userdata));
    };

    std::unique_ptr<StrictMock<MockBufferMapReadCallback>> mockBufferMapReadCallback;
    void ToMockBufferMapReadCallback(DawnBufferMapAsyncStatus status,
                                     const void* ptr,
                                     uint64_t dataLength,
                                     void* userdata) {
        // Assume the data is uint32_t to make writing matchers easier
        mockBufferMapReadCallback->Call(status, static_cast<const uint32_t*>(ptr), dataLength,
                                        userdata);
    }

    class MockBufferMapWriteCallback {
      public:
        MOCK_METHOD4(Call,
                     void(DawnBufferMapAsyncStatus status,
                          uint32_t* ptr,
                          uint64_t dataLength,
                          void* userdata));
    };

    std::unique_ptr<StrictMock<MockBufferMapWriteCallback>> mockBufferMapWriteCallback;
    uint32_t* lastMapWritePointer = nullptr;
    void ToMockBufferMapWriteCallback(DawnBufferMapAsyncStatus status,
                                      void* ptr,
                                      uint64_t dataLength,
                                      void* userdata) {
        // Assume the data is uint32_t to make writing matchers easier
        lastMapWritePointer = static_cast<uint32_t*>(ptr);
        mockBufferMapWriteCallback->Call(status, lastMapWritePointer, dataLength, userdata);
    }

}  // anonymous namespace

class WireBufferMappingTests : public WireTest {
  public:
    WireBufferMappingTests() {
    }
    ~WireBufferMappingTests() override = default;

    void SetUp() override {
        WireTest::SetUp();

        mockBufferMapReadCallback = std::make_unique<StrictMock<MockBufferMapReadCallback>>();
        mockBufferMapWriteCallback = std::make_unique<StrictMock<MockBufferMapWriteCallback>>();

        DawnBufferDescriptor descriptor;
        descriptor.nextInChain = nullptr;

        apiBuffer = api.GetNewBuffer();
        buffer = dawnDeviceCreateBuffer(device, &descriptor);

        EXPECT_CALL(api, DeviceCreateBuffer(apiDevice, _))
            .WillOnce(Return(apiBuffer))
            .RetiresOnSaturation();
        FlushClient();
    }

    void TearDown() override {
        WireTest::TearDown();

        // Delete mocks so that expectations are checked
        mockBufferMapReadCallback = nullptr;
        mockBufferMapWriteCallback = nullptr;
    }

    void FlushServer() {
        WireTest::FlushServer();

        Mock::VerifyAndClearExpectations(&mockBufferMapReadCallback);
        Mock::VerifyAndClearExpectations(&mockBufferMapWriteCallback);
    }

  protected:
    // A successfully created buffer
    DawnBuffer buffer;
    DawnBuffer apiBuffer;
};

// MapRead-specific tests

// Check mapping for reading a succesfully created buffer
TEST_F(WireBufferMappingTests, MappingForReadSuccessBuffer) {
    dawnBufferMapReadAsync(buffer, ToMockBufferMapReadCallback, nullptr);

    uint32_t bufferContent = 31337;
    EXPECT_CALL(api, OnBufferMapReadAsyncCallback(apiBuffer, _, _))
        .WillOnce(InvokeWithoutArgs([&]() {
            api.CallMapReadCallback(apiBuffer, DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS, &bufferContent,
                                    sizeof(uint32_t));
        }));

    FlushClient();

    EXPECT_CALL(*mockBufferMapReadCallback, Call(DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS,
                                                 Pointee(Eq(bufferContent)), sizeof(uint32_t), _))
        .Times(1);

    FlushServer();

    dawnBufferUnmap(buffer);
    EXPECT_CALL(api, BufferUnmap(apiBuffer)).Times(1);

    FlushClient();
}

// Check that things work correctly when a validation error happens when mapping the buffer for
// reading
TEST_F(WireBufferMappingTests, ErrorWhileMappingForRead) {
    dawnBufferMapReadAsync(buffer, ToMockBufferMapReadCallback, nullptr);

    EXPECT_CALL(api, OnBufferMapReadAsyncCallback(apiBuffer, _, _))
        .WillOnce(InvokeWithoutArgs([&]() {
            api.CallMapReadCallback(apiBuffer, DAWN_BUFFER_MAP_ASYNC_STATUS_ERROR, nullptr, 0);
        }));

    FlushClient();

    EXPECT_CALL(*mockBufferMapReadCallback, Call(DAWN_BUFFER_MAP_ASYNC_STATUS_ERROR, nullptr, 0, _))
        .Times(1);

    FlushServer();
}

// Check that the map read callback is called with UNKNOWN when the buffer is destroyed before the
// request is finished
TEST_F(WireBufferMappingTests, DestroyBeforeReadRequestEnd) {
    dawnBufferMapReadAsync(buffer, ToMockBufferMapReadCallback, nullptr);

    // Return success
    EXPECT_CALL(api, OnBufferMapReadAsyncCallback(apiBuffer, _, _))
        .WillOnce(InvokeWithoutArgs([&]() {
            api.CallMapReadCallback(apiBuffer, DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS, nullptr, 0);
        }));

    // Destroy before the client gets the success, so the callback is called with unknown.
    EXPECT_CALL(*mockBufferMapReadCallback,
                Call(DAWN_BUFFER_MAP_ASYNC_STATUS_UNKNOWN, nullptr, 0, _))
        .Times(1);
    dawnBufferRelease(buffer);
    EXPECT_CALL(api, BufferRelease(apiBuffer));

    FlushClient();
    FlushServer();
}

// Check the map read callback is called with UNKNOWN when the map request would have worked, but
// Unmap was called
TEST_F(WireBufferMappingTests, UnmapCalledTooEarlyForRead) {
    dawnBufferMapReadAsync(buffer, ToMockBufferMapReadCallback, nullptr);

    uint32_t bufferContent = 31337;
    EXPECT_CALL(api, OnBufferMapReadAsyncCallback(apiBuffer, _, _))
        .WillOnce(InvokeWithoutArgs([&]() {
            api.CallMapReadCallback(apiBuffer, DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS, &bufferContent,
                                    sizeof(uint32_t));
        }));

    FlushClient();

    // Oh no! We are calling Unmap too early!
    EXPECT_CALL(*mockBufferMapReadCallback,
                Call(DAWN_BUFFER_MAP_ASYNC_STATUS_UNKNOWN, nullptr, 0, _))
        .Times(1);
    dawnBufferUnmap(buffer);

    // The callback shouldn't get called, even when the request succeeded on the server side
    FlushServer();
}

// Check that an error map read callback gets nullptr while a buffer is already mapped
TEST_F(WireBufferMappingTests, MappingForReadingErrorWhileAlreadyMappedGetsNullptr) {
    // Successful map
    dawnBufferMapReadAsync(buffer, ToMockBufferMapReadCallback, nullptr);

    uint32_t bufferContent = 31337;
    EXPECT_CALL(api, OnBufferMapReadAsyncCallback(apiBuffer, _, _))
        .WillOnce(InvokeWithoutArgs([&]() {
            api.CallMapReadCallback(apiBuffer, DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS, &bufferContent,
                                    sizeof(uint32_t));
        }))
        .RetiresOnSaturation();

    FlushClient();

    EXPECT_CALL(*mockBufferMapReadCallback, Call(DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS,
                                                 Pointee(Eq(bufferContent)), sizeof(uint32_t), _))
        .Times(1);

    FlushServer();

    // Map failure while the buffer is already mapped
    dawnBufferMapReadAsync(buffer, ToMockBufferMapReadCallback, nullptr);
    EXPECT_CALL(api, OnBufferMapReadAsyncCallback(apiBuffer, _, _))
        .WillOnce(InvokeWithoutArgs([&]() {
            api.CallMapReadCallback(apiBuffer, DAWN_BUFFER_MAP_ASYNC_STATUS_ERROR, nullptr, 0);
        }));

    FlushClient();

    EXPECT_CALL(*mockBufferMapReadCallback, Call(DAWN_BUFFER_MAP_ASYNC_STATUS_ERROR, nullptr, 0, _))
        .Times(1);

    FlushServer();
}

// Test that the MapReadCallback isn't fired twice when unmap() is called inside the callback
TEST_F(WireBufferMappingTests, UnmapInsideMapReadCallback) {
    dawnBufferMapReadAsync(buffer, ToMockBufferMapReadCallback, nullptr);

    uint32_t bufferContent = 31337;
    EXPECT_CALL(api, OnBufferMapReadAsyncCallback(apiBuffer, _, _))
        .WillOnce(InvokeWithoutArgs([&]() {
            api.CallMapReadCallback(apiBuffer, DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS, &bufferContent,
                                    sizeof(uint32_t));
        }));

    FlushClient();

    EXPECT_CALL(*mockBufferMapReadCallback, Call(DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS,
                                                 Pointee(Eq(bufferContent)), sizeof(uint32_t), _))
        .WillOnce(InvokeWithoutArgs([&]() { dawnBufferUnmap(buffer); }));

    FlushServer();

    EXPECT_CALL(api, BufferUnmap(apiBuffer)).Times(1);

    FlushClient();
}

// Test that the MapReadCallback isn't fired twice the buffer external refcount reaches 0 in the
// callback
TEST_F(WireBufferMappingTests, DestroyInsideMapReadCallback) {
    dawnBufferMapReadAsync(buffer, ToMockBufferMapReadCallback, nullptr);

    uint32_t bufferContent = 31337;
    EXPECT_CALL(api, OnBufferMapReadAsyncCallback(apiBuffer, _, _))
        .WillOnce(InvokeWithoutArgs([&]() {
            api.CallMapReadCallback(apiBuffer, DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS, &bufferContent,
                                    sizeof(uint32_t));
        }));

    FlushClient();

    EXPECT_CALL(*mockBufferMapReadCallback, Call(DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS,
                                                 Pointee(Eq(bufferContent)), sizeof(uint32_t), _))
        .WillOnce(InvokeWithoutArgs([&]() { dawnBufferRelease(buffer); }));

    FlushServer();

    EXPECT_CALL(api, BufferRelease(apiBuffer));

    FlushClient();
}

// MapWrite-specific tests

// Check mapping for writing a succesfully created buffer
TEST_F(WireBufferMappingTests, MappingForWriteSuccessBuffer) {
    dawnBufferMapWriteAsync(buffer, ToMockBufferMapWriteCallback, nullptr);

    uint32_t serverBufferContent = 31337;
    uint32_t updatedContent = 4242;
    uint32_t zero = 0;

    EXPECT_CALL(api, OnBufferMapWriteAsyncCallback(apiBuffer, _, _))
        .WillOnce(InvokeWithoutArgs([&]() {
            api.CallMapWriteCallback(apiBuffer, DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS,
                                     &serverBufferContent, sizeof(uint32_t));
        }));

    FlushClient();

    // The map write callback always gets a buffer full of zeroes.
    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS, Pointee(Eq(zero)), sizeof(uint32_t), _))
        .Times(1);

    FlushServer();

    // Write something to the mapped pointer
    *lastMapWritePointer = updatedContent;

    dawnBufferUnmap(buffer);
    EXPECT_CALL(api, BufferUnmap(apiBuffer)).Times(1);

    FlushClient();

    // After the buffer is unmapped, the content of the buffer is updated on the server
    ASSERT_EQ(serverBufferContent, updatedContent);
}

// Check that things work correctly when a validation error happens when mapping the buffer for
// writing
TEST_F(WireBufferMappingTests, ErrorWhileMappingForWrite) {
    dawnBufferMapWriteAsync(buffer, ToMockBufferMapWriteCallback, nullptr);

    EXPECT_CALL(api, OnBufferMapWriteAsyncCallback(apiBuffer, _, _))
        .WillOnce(InvokeWithoutArgs([&]() {
            api.CallMapWriteCallback(apiBuffer, DAWN_BUFFER_MAP_ASYNC_STATUS_ERROR, nullptr, 0);
        }));

    FlushClient();

    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(DAWN_BUFFER_MAP_ASYNC_STATUS_ERROR, nullptr, 0, _))
        .Times(1);

    FlushServer();
}

// Check that the map write callback is called with UNKNOWN when the buffer is destroyed before the
// request is finished
TEST_F(WireBufferMappingTests, DestroyBeforeWriteRequestEnd) {
    dawnBufferMapWriteAsync(buffer, ToMockBufferMapWriteCallback, nullptr);

    // Return success
    EXPECT_CALL(api, OnBufferMapWriteAsyncCallback(apiBuffer, _, _))
        .WillOnce(InvokeWithoutArgs([&]() {
            api.CallMapWriteCallback(apiBuffer, DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS, nullptr, 0);
        }));

    // Destroy before the client gets the success, so the callback is called with unknown.
    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(DAWN_BUFFER_MAP_ASYNC_STATUS_UNKNOWN, nullptr, 0, _))
        .Times(1);
    dawnBufferRelease(buffer);
    EXPECT_CALL(api, BufferRelease(apiBuffer));

    FlushClient();
    FlushServer();
}

// Check the map read callback is called with UNKNOWN when the map request would have worked, but
// Unmap was called
TEST_F(WireBufferMappingTests, UnmapCalledTooEarlyForWrite) {
    dawnBufferMapWriteAsync(buffer, ToMockBufferMapWriteCallback, nullptr);

    uint32_t bufferContent = 31337;
    EXPECT_CALL(api, OnBufferMapWriteAsyncCallback(apiBuffer, _, _))
        .WillOnce(InvokeWithoutArgs([&]() {
            api.CallMapWriteCallback(apiBuffer, DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS,
                                     &bufferContent, sizeof(uint32_t));
        }));

    FlushClient();

    // Oh no! We are calling Unmap too early!
    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(DAWN_BUFFER_MAP_ASYNC_STATUS_UNKNOWN, nullptr, 0, _))
        .Times(1);
    dawnBufferUnmap(buffer);

    // The callback shouldn't get called, even when the request succeeded on the server side
    FlushServer();
}

// Check that an error map read callback gets nullptr while a buffer is already mapped
TEST_F(WireBufferMappingTests, MappingForWritingErrorWhileAlreadyMappedGetsNullptr) {
    // Successful map
    dawnBufferMapWriteAsync(buffer, ToMockBufferMapWriteCallback, nullptr);

    uint32_t bufferContent = 31337;
    uint32_t zero = 0;
    EXPECT_CALL(api, OnBufferMapWriteAsyncCallback(apiBuffer, _, _))
        .WillOnce(InvokeWithoutArgs([&]() {
            api.CallMapWriteCallback(apiBuffer, DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS,
                                     &bufferContent, sizeof(uint32_t));
        }))
        .RetiresOnSaturation();

    FlushClient();

    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS, Pointee(Eq(zero)), sizeof(uint32_t), _))
        .Times(1);

    FlushServer();

    // Map failure while the buffer is already mapped
    dawnBufferMapWriteAsync(buffer, ToMockBufferMapWriteCallback, nullptr);
    EXPECT_CALL(api, OnBufferMapWriteAsyncCallback(apiBuffer, _, _))
        .WillOnce(InvokeWithoutArgs([&]() {
            api.CallMapWriteCallback(apiBuffer, DAWN_BUFFER_MAP_ASYNC_STATUS_ERROR, nullptr, 0);
        }));

    FlushClient();

    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(DAWN_BUFFER_MAP_ASYNC_STATUS_ERROR, nullptr, 0, _))
        .Times(1);

    FlushServer();
}

// Test that the MapWriteCallback isn't fired twice when unmap() is called inside the callback
TEST_F(WireBufferMappingTests, UnmapInsideMapWriteCallback) {
    dawnBufferMapWriteAsync(buffer, ToMockBufferMapWriteCallback, nullptr);

    uint32_t bufferContent = 31337;
    uint32_t zero = 0;
    EXPECT_CALL(api, OnBufferMapWriteAsyncCallback(apiBuffer, _, _))
        .WillOnce(InvokeWithoutArgs([&]() {
            api.CallMapWriteCallback(apiBuffer, DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS,
                                     &bufferContent, sizeof(uint32_t));
        }));

    FlushClient();

    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS, Pointee(Eq(zero)), sizeof(uint32_t), _))
        .WillOnce(InvokeWithoutArgs([&]() { dawnBufferUnmap(buffer); }));

    FlushServer();

    EXPECT_CALL(api, BufferUnmap(apiBuffer)).Times(1);

    FlushClient();
}

// Test that the MapWriteCallback isn't fired twice the buffer external refcount reaches 0 in the
// callback
TEST_F(WireBufferMappingTests, DestroyInsideMapWriteCallback) {
    dawnBufferMapWriteAsync(buffer, ToMockBufferMapWriteCallback, nullptr);

    uint32_t bufferContent = 31337;
    uint32_t zero = 0;
    EXPECT_CALL(api, OnBufferMapWriteAsyncCallback(apiBuffer, _, _))
        .WillOnce(InvokeWithoutArgs([&]() {
            api.CallMapWriteCallback(apiBuffer, DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS,
                                     &bufferContent, sizeof(uint32_t));
        }));

    FlushClient();

    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS, Pointee(Eq(zero)), sizeof(uint32_t), _))
        .WillOnce(InvokeWithoutArgs([&]() { dawnBufferRelease(buffer); }));

    FlushServer();

    EXPECT_CALL(api, BufferRelease(apiBuffer));

    FlushClient();
}

// Test successful CreateBufferMapped
TEST_F(WireBufferMappingTests, CreateBufferMappedSuccess) {
    DawnBufferDescriptor descriptor;
    descriptor.nextInChain = nullptr;
    descriptor.size = 4;

    DawnBuffer apiBuffer = api.GetNewBuffer();
    DawnCreateBufferMappedResult apiResult;
    uint32_t apiBufferData = 1234;
    apiResult.buffer = apiBuffer;
    apiResult.data = reinterpret_cast<uint8_t*>(&apiBufferData);
    apiResult.dataLength = 4;

    DawnCreateBufferMappedResult result = dawnDeviceCreateBufferMapped(device, &descriptor);

    EXPECT_CALL(api, DeviceCreateBufferMapped(apiDevice, _))
        .WillOnce(Return(apiResult))
        .RetiresOnSaturation();

    FlushClient();

    dawnBufferUnmap(result.buffer);
    EXPECT_CALL(api, BufferUnmap(apiBuffer)).Times(1);

    FlushClient();
}

// Test that releasing after CreateBufferMapped does not call Unmap
TEST_F(WireBufferMappingTests, ReleaseAfterCreateBufferMapped) {
    DawnBufferDescriptor descriptor;
    descriptor.nextInChain = nullptr;
    descriptor.size = 4;

    DawnBuffer apiBuffer = api.GetNewBuffer();
    DawnCreateBufferMappedResult apiResult;
    uint32_t apiBufferData = 1234;
    apiResult.buffer = apiBuffer;
    apiResult.data = reinterpret_cast<uint8_t*>(&apiBufferData);
    apiResult.dataLength = 4;

    DawnCreateBufferMappedResult result = dawnDeviceCreateBufferMapped(device, &descriptor);

    EXPECT_CALL(api, DeviceCreateBufferMapped(apiDevice, _))
        .WillOnce(Return(apiResult))
        .RetiresOnSaturation();

    FlushClient();

    dawnBufferRelease(result.buffer);
    EXPECT_CALL(api, BufferRelease(apiBuffer)).Times(1);

    FlushClient();
}

// Test that it is valid to map a buffer after CreateBufferMapped and Unmap
TEST_F(WireBufferMappingTests, CreateBufferMappedThenMapSuccess) {
    DawnBufferDescriptor descriptor;
    descriptor.nextInChain = nullptr;
    descriptor.size = 4;

    DawnBuffer apiBuffer = api.GetNewBuffer();
    DawnCreateBufferMappedResult apiResult;
    uint32_t apiBufferData = 9863;
    apiResult.buffer = apiBuffer;
    apiResult.data = reinterpret_cast<uint8_t*>(&apiBufferData);
    apiResult.dataLength = 4;

    DawnCreateBufferMappedResult result = dawnDeviceCreateBufferMapped(device, &descriptor);

    EXPECT_CALL(api, DeviceCreateBufferMapped(apiDevice, _))
        .WillOnce(Return(apiResult))
        .RetiresOnSaturation();

    FlushClient();

    dawnBufferUnmap(result.buffer);
    EXPECT_CALL(api, BufferUnmap(apiBuffer)).Times(1);

    FlushClient();

    dawnBufferMapWriteAsync(result.buffer, ToMockBufferMapWriteCallback, nullptr);

    uint32_t zero = 0;
    EXPECT_CALL(api, OnBufferMapWriteAsyncCallback(apiBuffer, _, _))
        .WillOnce(InvokeWithoutArgs([&]() {
            api.CallMapWriteCallback(apiBuffer, DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS,
                                     &apiBufferData, sizeof(uint32_t));
        }));

    FlushClient();

    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS, Pointee(Eq(zero)), sizeof(uint32_t), _))
        .Times(1);

    FlushServer();
}

// Test that it is invalid to map a buffer after CreateBufferMapped before Unmap
TEST_F(WireBufferMappingTests, CreateBufferMappedThenMapFailure) {
    DawnBufferDescriptor descriptor;
    descriptor.nextInChain = nullptr;
    descriptor.size = 4;

    DawnBuffer apiBuffer = api.GetNewBuffer();
    DawnCreateBufferMappedResult apiResult;
    uint32_t apiBufferData = 9863;
    apiResult.buffer = apiBuffer;
    apiResult.data = reinterpret_cast<uint8_t*>(&apiBufferData);
    apiResult.dataLength = 4;

    DawnCreateBufferMappedResult result = dawnDeviceCreateBufferMapped(device, &descriptor);

    EXPECT_CALL(api, DeviceCreateBufferMapped(apiDevice, _))
        .WillOnce(Return(apiResult))
        .RetiresOnSaturation();

    FlushClient();

    dawnBufferMapWriteAsync(result.buffer, ToMockBufferMapWriteCallback, nullptr);

    EXPECT_CALL(api, OnBufferMapWriteAsyncCallback(apiBuffer, _, _))
        .WillOnce(InvokeWithoutArgs([&]() {
            api.CallMapWriteCallback(apiBuffer, DAWN_BUFFER_MAP_ASYNC_STATUS_ERROR, nullptr, 0);
        }));

    FlushClient();

    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(DAWN_BUFFER_MAP_ASYNC_STATUS_ERROR, nullptr, 0, _))
        .Times(1);

    FlushServer();

    dawnBufferUnmap(result.buffer);
    EXPECT_CALL(api, BufferUnmap(apiBuffer)).Times(1);

    FlushClient();
}
