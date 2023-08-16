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

#include "dawn_wire/client/ClientMemoryTransferService_mock.h"
#include "dawn_wire/server/ServerMemoryTransferService_mock.h"

using namespace testing;
using namespace dawn_wire;

namespace {

    // Mock classes to add expectations on the wire calling callbacks
    class MockBufferMapReadCallback {
      public:
        MOCK_METHOD(void,
                    Call,
                    (WGPUBufferMapAsyncStatus status,
                     const uint32_t* ptr,
                     uint64_t dataLength,
                     void* userdata));
    };

    std::unique_ptr<StrictMock<MockBufferMapReadCallback>> mockBufferMapReadCallback;
    void ToMockBufferMapReadCallback(WGPUBufferMapAsyncStatus status,
                                     const void* ptr,
                                     uint64_t dataLength,
                                     void* userdata) {
        // Assume the data is uint32_t to make writing matchers easier
        mockBufferMapReadCallback->Call(status, static_cast<const uint32_t*>(ptr), dataLength,
                                        userdata);
    }

    class MockBufferMapWriteCallback {
      public:
        MOCK_METHOD(
            void,
            Call,
            (WGPUBufferMapAsyncStatus status, uint32_t* ptr, uint64_t dataLength, void* userdata));
    };

    std::unique_ptr<StrictMock<MockBufferMapWriteCallback>> mockBufferMapWriteCallback;
    void ToMockBufferMapWriteCallback(WGPUBufferMapAsyncStatus status,
                                      void* ptr,
                                      uint64_t dataLength,
                                      void* userdata) {
        // Assume the data is uint32_t to make writing matchers easier
        mockBufferMapWriteCallback->Call(status, static_cast<uint32_t*>(ptr), dataLength, userdata);
    }

    class MockBufferCreateMappedCallback {
      public:
        MOCK_METHOD(void,
                    Call,
                    (WGPUBufferMapAsyncStatus status,
                     WGPUBuffer buffer,
                     uint32_t* ptr,
                     uint64_t dataLength,
                     void* userdata));
    };

}  // anonymous namespace

// WireMemoryTransferServiceTests test the MemoryTransferService with buffer mapping.
// They test the basic success and error cases for buffer mapping, and they test
// mocked failures of each fallible MemoryTransferService method that an embedder
// could implement.
// The test harness defines multiple helpers for expecting operations on Read/Write handles
// and for mocking failures. The helpers are designed such that for a given run of a test,
// a Serialization expection has a corresponding Deserialization expectation for which the
// serialized data must match.
// There are tests which check for Success for every mapping operation which mock an entire mapping
// operation from map to unmap, and add all MemoryTransferService expectations.
// Tests which check for errors perform the same mapping operations but insert mocked failures for
// various mapping or MemoryTransferService operations.
class WireMemoryTransferServiceTests : public WireTest {
  public:
    WireMemoryTransferServiceTests() {
    }
    ~WireMemoryTransferServiceTests() override = default;

    client::MemoryTransferService* GetClientMemoryTransferService() override {
        return &clientMemoryTransferService;
    }

    server::MemoryTransferService* GetServerMemoryTransferService() override {
        return &serverMemoryTransferService;
    }

    void SetUp() override {
        WireTest::SetUp();

        mockBufferMapReadCallback = std::make_unique<StrictMock<MockBufferMapReadCallback>>();
        mockBufferMapWriteCallback = std::make_unique<StrictMock<MockBufferMapWriteCallback>>();

        // TODO(enga): Make this thread-safe.
        mBufferContent++;
        mMappedBufferContent = 0;
        mUpdatedBufferContent++;
        mSerializeCreateInfo++;
        mSerializeInitialDataInfo++;
        mSerializeFlushInfo++;
    }

    void TearDown() override {
        WireTest::TearDown();

        // Delete mocks so that expectations are checked
        mockBufferMapReadCallback = nullptr;
        mockBufferMapWriteCallback = nullptr;
    }

    void FlushClient(bool success = true) {
        WireTest::FlushClient(success);
        Mock::VerifyAndClearExpectations(&serverMemoryTransferService);
    }

    void FlushServer(bool success = true) {
        WireTest::FlushServer(success);

        Mock::VerifyAndClearExpectations(&mockBufferMapReadCallback);
        Mock::VerifyAndClearExpectations(&mockBufferMapWriteCallback);
        Mock::VerifyAndClearExpectations(&clientMemoryTransferService);
    }

  protected:
    using ClientReadHandle = client::MockMemoryTransferService::MockReadHandle;
    using ServerReadHandle = server::MockMemoryTransferService::MockReadHandle;
    using ClientWriteHandle = client::MockMemoryTransferService::MockWriteHandle;
    using ServerWriteHandle = server::MockMemoryTransferService::MockWriteHandle;

    std::pair<WGPUBuffer, WGPUBuffer> CreateBuffer() {
        WGPUBufferDescriptor descriptor = {};
        descriptor.size = kBufferSize;

        WGPUBuffer apiBuffer = api.GetNewBuffer();
        WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &descriptor);

        EXPECT_CALL(api, DeviceCreateBuffer(apiDevice, _))
            .WillOnce(Return(apiBuffer))
            .RetiresOnSaturation();

        return std::make_pair(apiBuffer, buffer);
    }

    std::pair<WGPUCreateBufferMappedResult, WGPUCreateBufferMappedResult> CreateBufferMapped() {
        WGPUBufferDescriptor descriptor = {};
        descriptor.size = sizeof(mBufferContent);

        WGPUBuffer apiBuffer = api.GetNewBuffer();

        WGPUCreateBufferMappedResult apiResult;
        apiResult.buffer = apiBuffer;
        apiResult.data = reinterpret_cast<uint8_t*>(&mMappedBufferContent);
        apiResult.dataLength = sizeof(mMappedBufferContent);

        WGPUCreateBufferMappedResult result = wgpuDeviceCreateBufferMapped(device, &descriptor);

        EXPECT_CALL(api, DeviceCreateBufferMapped(apiDevice, _))
            .WillOnce(Return(apiResult))
            .RetiresOnSaturation();

        return std::make_pair(apiResult, result);
    }

    ClientReadHandle* ExpectReadHandleCreation() {
        // Create the handle first so we can use it in later expectations.
        ClientReadHandle* handle = clientMemoryTransferService.NewReadHandle();

        EXPECT_CALL(clientMemoryTransferService, OnCreateReadHandle(sizeof(mBufferContent)))
            .WillOnce(InvokeWithoutArgs([=]() { return handle; }));

        return handle;
    }

    void MockReadHandleCreationFailure() {
        EXPECT_CALL(clientMemoryTransferService, OnCreateReadHandle(sizeof(mBufferContent)))
            .WillOnce(InvokeWithoutArgs([=]() { return nullptr; }));
    }

    void ExpectReadHandleSerialization(ClientReadHandle* handle) {
        EXPECT_CALL(clientMemoryTransferService, OnReadHandleSerializeCreateSize(handle))
            .WillOnce(InvokeWithoutArgs([&]() { return sizeof(mSerializeCreateInfo); }));
        EXPECT_CALL(clientMemoryTransferService, OnReadHandleSerializeCreate(handle, _))
            .WillOnce(WithArg<1>([&](void* serializePointer) {
                memcpy(serializePointer, &mSerializeCreateInfo, sizeof(mSerializeCreateInfo));
                return sizeof(mSerializeCreateInfo);
            }));
    }

    ServerReadHandle* ExpectServerReadHandleDeserialize() {
        // Create the handle first so we can use it in later expectations.
        ServerReadHandle* handle = serverMemoryTransferService.NewReadHandle();

        EXPECT_CALL(serverMemoryTransferService,
                    OnDeserializeReadHandle(Pointee(Eq(mSerializeCreateInfo)),
                                            sizeof(mSerializeCreateInfo), _))
            .WillOnce(WithArg<2>([=](server::MemoryTransferService::ReadHandle** readHandle) {
                *readHandle = handle;
                return true;
            }));

        return handle;
    }

    void MockServerReadHandleDeserializeFailure() {
        EXPECT_CALL(serverMemoryTransferService,
                    OnDeserializeReadHandle(Pointee(Eq(mSerializeCreateInfo)),
                                            sizeof(mSerializeCreateInfo), _))
            .WillOnce(InvokeWithoutArgs([&]() { return false; }));
    }

    void ExpectServerReadHandleInitialize(ServerReadHandle* handle) {
        EXPECT_CALL(serverMemoryTransferService, OnReadHandleSerializeInitialDataSize(handle, _, _))
            .WillOnce(InvokeWithoutArgs([&]() { return sizeof(mSerializeInitialDataInfo); }));
        EXPECT_CALL(serverMemoryTransferService, OnReadHandleSerializeInitialData(handle, _, _, _))
            .WillOnce(WithArg<3>([&](void* serializePointer) {
                memcpy(serializePointer, &mSerializeInitialDataInfo,
                       sizeof(mSerializeInitialDataInfo));
                return sizeof(mSerializeInitialDataInfo);
            }));
    }

    void ExpectClientReadHandleDeserializeInitialize(ClientReadHandle* handle,
                                                     uint32_t* mappedData) {
        EXPECT_CALL(clientMemoryTransferService, OnReadHandleDeserializeInitialData(
                                                     handle, Pointee(Eq(mSerializeInitialDataInfo)),
                                                     sizeof(mSerializeInitialDataInfo), _, _))
            .WillOnce(WithArgs<3, 4>([=](const void** data, size_t* dataLength) {
                *data = mappedData;
                *dataLength = sizeof(*mappedData);
                return true;
            }));
    }

    void MockClientReadHandleDeserializeInitializeFailure(ClientReadHandle* handle) {
        EXPECT_CALL(clientMemoryTransferService, OnReadHandleDeserializeInitialData(
                                                     handle, Pointee(Eq(mSerializeInitialDataInfo)),
                                                     sizeof(mSerializeInitialDataInfo), _, _))
            .WillOnce(InvokeWithoutArgs([&]() { return false; }));
    }

    ClientWriteHandle* ExpectWriteHandleCreation() {
        // Create the handle first so we can use it in later expectations.
        ClientWriteHandle* handle = clientMemoryTransferService.NewWriteHandle();

        EXPECT_CALL(clientMemoryTransferService, OnCreateWriteHandle(sizeof(mBufferContent)))
            .WillOnce(InvokeWithoutArgs([=]() { return handle; }));

        return handle;
    }

    void MockWriteHandleCreationFailure() {
        EXPECT_CALL(clientMemoryTransferService, OnCreateWriteHandle(sizeof(mBufferContent)))
            .WillOnce(InvokeWithoutArgs([=]() { return nullptr; }));
    }

    void ExpectWriteHandleSerialization(ClientWriteHandle* handle) {
        EXPECT_CALL(clientMemoryTransferService, OnWriteHandleSerializeCreateSize(handle))
            .WillOnce(InvokeWithoutArgs([&]() { return sizeof(mSerializeCreateInfo); }));
        EXPECT_CALL(clientMemoryTransferService, OnWriteHandleSerializeCreate(handle, _))
            .WillOnce(WithArg<1>([&](void* serializePointer) {
                memcpy(serializePointer, &mSerializeCreateInfo, sizeof(mSerializeCreateInfo));
                return sizeof(mSerializeCreateInfo);
            }));
    }

    ServerWriteHandle* ExpectServerWriteHandleDeserialization() {
        // Create the handle first so it can be used in later expectations.
        ServerWriteHandle* handle = serverMemoryTransferService.NewWriteHandle();

        EXPECT_CALL(serverMemoryTransferService,
                    OnDeserializeWriteHandle(Pointee(Eq(mSerializeCreateInfo)),
                                             sizeof(mSerializeCreateInfo), _))
            .WillOnce(WithArg<2>([=](server::MemoryTransferService::WriteHandle** writeHandle) {
                *writeHandle = handle;
                return true;
            }));

        return handle;
    }

    void MockServerWriteHandleDeserializeFailure() {
        EXPECT_CALL(serverMemoryTransferService,
                    OnDeserializeWriteHandle(Pointee(Eq(mSerializeCreateInfo)),
                                             sizeof(mSerializeCreateInfo), _))
            .WillOnce(InvokeWithoutArgs([&]() { return false; }));
    }

    void ExpectClientWriteHandleOpen(ClientWriteHandle* handle, uint32_t* mappedData) {
        EXPECT_CALL(clientMemoryTransferService, OnWriteHandleOpen(handle))
            .WillOnce(InvokeWithoutArgs(
                [=]() { return std::make_pair(mappedData, sizeof(*mappedData)); }));
    }

    void MockClientWriteHandleOpenFailure(ClientWriteHandle* handle) {
        EXPECT_CALL(clientMemoryTransferService, OnWriteHandleOpen(handle))
            .WillOnce(InvokeWithoutArgs([&]() { return std::make_pair(nullptr, 0); }));
    }

    void ExpectClientWriteHandleSerializeFlush(ClientWriteHandle* handle) {
        EXPECT_CALL(clientMemoryTransferService, OnWriteHandleSerializeFlushSize(handle))
            .WillOnce(InvokeWithoutArgs([&]() { return sizeof(mSerializeFlushInfo); }));
        EXPECT_CALL(clientMemoryTransferService, OnWriteHandleSerializeFlush(handle, _))
            .WillOnce(WithArg<1>([&](void* serializePointer) {
                memcpy(serializePointer, &mSerializeFlushInfo, sizeof(mSerializeFlushInfo));
                return sizeof(mSerializeFlushInfo);
            }));
    }

    void ExpectServerWriteHandleDeserializeFlush(ServerWriteHandle* handle, uint32_t expectedData) {
        EXPECT_CALL(serverMemoryTransferService,
                    OnWriteHandleDeserializeFlush(handle, Pointee(Eq(mSerializeFlushInfo)),
                                                  sizeof(mSerializeFlushInfo)))
            .WillOnce(InvokeWithoutArgs([=]() {
                // The handle data should be updated.
                EXPECT_EQ(*handle->GetData(), expectedData);
                return true;
            }));
    }

    void MockServerWriteHandleDeserializeFlushFailure(ServerWriteHandle* handle) {
        EXPECT_CALL(serverMemoryTransferService,
                    OnWriteHandleDeserializeFlush(handle, Pointee(Eq(mSerializeFlushInfo)),
                                                  sizeof(mSerializeFlushInfo)))
            .WillOnce(InvokeWithoutArgs([&]() { return false; }));
    }

    // Arbitrary values used within tests to check if serialized data is correctly passed
    // between the client and server. The static data changes between runs of the tests and
    // test expectations will check that serialized values are passed to the respective
    // deserialization function.
    static uint32_t mSerializeCreateInfo;
    static uint32_t mSerializeInitialDataInfo;
    static uint32_t mSerializeFlushInfo;

    // Represents the buffer contents for the test.
    static uint32_t mBufferContent;

    static constexpr size_t kBufferSize = sizeof(mBufferContent);

    // The client's zero-initialized buffer for writing.
    uint32_t mMappedBufferContent = 0;

    // |mMappedBufferContent| should be set equal to |mUpdatedBufferContent| when the client
    // performs a write. Test expectations should check that |mBufferContent ==
    // mUpdatedBufferContent| after all writes are flushed.
    static uint32_t mUpdatedBufferContent;

    testing::StrictMock<dawn_wire::server::MockMemoryTransferService> serverMemoryTransferService;
    testing::StrictMock<dawn_wire::client::MockMemoryTransferService> clientMemoryTransferService;
};

uint32_t WireMemoryTransferServiceTests::mBufferContent = 1337;
uint32_t WireMemoryTransferServiceTests::mUpdatedBufferContent = 2349;
uint32_t WireMemoryTransferServiceTests::mSerializeCreateInfo = 4242;
uint32_t WireMemoryTransferServiceTests::mSerializeInitialDataInfo = 1394;
uint32_t WireMemoryTransferServiceTests::mSerializeFlushInfo = 1235;

// Test successful MapRead.
TEST_F(WireMemoryTransferServiceTests, BufferMapReadSuccess) {
    WGPUBuffer buffer;
    WGPUBuffer apiBuffer;
    std::tie(apiBuffer, buffer) = CreateBuffer();
    FlushClient();

    // The client should create and serialize a ReadHandle on mapReadAsync.
    ClientReadHandle* clientHandle = ExpectReadHandleCreation();
    ExpectReadHandleSerialization(clientHandle);

    wgpuBufferMapReadAsync(buffer, ToMockBufferMapReadCallback, nullptr);

    // The server should deserialize the MapRead handle from the client and then serialize
    // an initialization message.
    ServerReadHandle* serverHandle = ExpectServerReadHandleDeserialize();
    ExpectServerReadHandleInitialize(serverHandle);

    // Mock a successful callback
    EXPECT_CALL(api, OnBufferMapAsyncCallback(apiBuffer, _, _)).WillOnce(InvokeWithoutArgs([&]() {
        api.CallMapAsyncCallback(apiBuffer, WGPUBufferMapAsyncStatus_Success);
    }));
    EXPECT_CALL(api, BufferGetConstMappedRange(apiBuffer, 0, kBufferSize))
        .WillOnce(Return(&mBufferContent));

    FlushClient();

    // The client receives a successful callback.
    EXPECT_CALL(*mockBufferMapReadCallback,
                Call(WGPUBufferMapAsyncStatus_Success, &mBufferContent, sizeof(mBufferContent), _))
        .Times(1);

    // The client should receive the handle initialization message from the server.
    ExpectClientReadHandleDeserializeInitialize(clientHandle, &mBufferContent);

    FlushServer();

    // The handle is destroyed once the buffer is unmapped.
    EXPECT_CALL(clientMemoryTransferService, OnReadHandleDestroy(clientHandle)).Times(1);
    wgpuBufferUnmap(buffer);

    EXPECT_CALL(serverMemoryTransferService, OnReadHandleDestroy(serverHandle)).Times(1);
    EXPECT_CALL(api, BufferUnmap(apiBuffer)).Times(1);

    FlushClient();
}

// Test unsuccessful MapRead.
TEST_F(WireMemoryTransferServiceTests, BufferMapReadError) {
    WGPUBuffer buffer;
    WGPUBuffer apiBuffer;
    std::tie(apiBuffer, buffer) = CreateBuffer();
    FlushClient();

    // The client should create and serialize a ReadHandle on mapReadAsync.
    ClientReadHandle* clientHandle = ExpectReadHandleCreation();
    ExpectReadHandleSerialization(clientHandle);

    wgpuBufferMapReadAsync(buffer, ToMockBufferMapReadCallback, nullptr);

    // The server should deserialize the ReadHandle from the client.
    ServerReadHandle* serverHandle = ExpectServerReadHandleDeserialize();

    // Mock a failed callback.
    EXPECT_CALL(api, OnBufferMapAsyncCallback(apiBuffer, _, _)).WillOnce(InvokeWithoutArgs([&]() {
        api.CallMapAsyncCallback(apiBuffer, WGPUBufferMapAsyncStatus_Error);
    }));

    // Since the mapping failed, the handle is immediately destroyed.
    EXPECT_CALL(serverMemoryTransferService, OnReadHandleDestroy(serverHandle)).Times(1);

    FlushClient();

    // The client receives an error callback.
    EXPECT_CALL(*mockBufferMapReadCallback, Call(WGPUBufferMapAsyncStatus_Error, nullptr, 0, _))
        .Times(1);

    // The client receives the map failure and destroys the handle.
    EXPECT_CALL(clientMemoryTransferService, OnReadHandleDestroy(clientHandle)).Times(1);

    FlushServer();

    wgpuBufferUnmap(buffer);

    EXPECT_CALL(api, BufferUnmap(apiBuffer)).Times(1);

    FlushClient();
}

// Test MapRead ReadHandle creation failure.
TEST_F(WireMemoryTransferServiceTests, BufferMapReadHandleCreationFailure) {
    WGPUBuffer buffer;
    WGPUBuffer apiBuffer;
    std::tie(apiBuffer, buffer) = CreateBuffer();
    FlushClient();

    // Mock a ReadHandle creation failure
    MockReadHandleCreationFailure();

    // Failed creation of a ReadHandle is a mapping failure and the client synchronously receives
    // an error callback.
    EXPECT_CALL(*mockBufferMapReadCallback, Call(WGPUBufferMapAsyncStatus_Error, nullptr, 0, _))
        .Times(1);

    wgpuBufferMapReadAsync(buffer, ToMockBufferMapReadCallback, nullptr);
}

// Test MapRead DeserializeReadHandle failure.
TEST_F(WireMemoryTransferServiceTests, BufferMapReadDeserializeReadHandleFailure) {
    WGPUBuffer buffer;
    WGPUBuffer apiBuffer;
    std::tie(apiBuffer, buffer) = CreateBuffer();
    FlushClient();

    // The client should create and serialize a ReadHandle on mapReadAsync.
    ClientReadHandle* clientHandle = ExpectReadHandleCreation();
    ExpectReadHandleSerialization(clientHandle);

    wgpuBufferMapReadAsync(buffer, ToMockBufferMapReadCallback, nullptr);

    // Mock a Deserialization failure.
    MockServerReadHandleDeserializeFailure();

    FlushClient(false);

    // The server received a fatal failure and the client callback was never returned.
    // It is called when the wire is destructed.
    EXPECT_CALL(*mockBufferMapReadCallback, Call(WGPUBufferMapAsyncStatus_Unknown, nullptr, 0, _))
        .Times(1);

    EXPECT_CALL(clientMemoryTransferService, OnReadHandleDestroy(clientHandle)).Times(1);
}

// Test MapRead DeserializeInitialData failure.
TEST_F(WireMemoryTransferServiceTests, BufferMapReadDeserializeInitialDataFailure) {
    WGPUBuffer buffer;
    WGPUBuffer apiBuffer;
    std::tie(apiBuffer, buffer) = CreateBuffer();
    FlushClient();

    // The client should create and serialize a ReadHandle on mapReadAsync.
    ClientReadHandle* clientHandle = ExpectReadHandleCreation();
    ExpectReadHandleSerialization(clientHandle);

    wgpuBufferMapReadAsync(buffer, ToMockBufferMapReadCallback, nullptr);

    // The server should deserialize the MapRead handle from the client and then serialize
    // an initialization message.
    ServerReadHandle* serverHandle = ExpectServerReadHandleDeserialize();
    ExpectServerReadHandleInitialize(serverHandle);

    // Mock a successful callback
    EXPECT_CALL(api, OnBufferMapAsyncCallback(apiBuffer, _, _)).WillOnce(InvokeWithoutArgs([&]() {
        api.CallMapAsyncCallback(apiBuffer, WGPUBufferMapAsyncStatus_Success);
    }));
    EXPECT_CALL(api, BufferGetConstMappedRange(apiBuffer, 0, kBufferSize))
        .WillOnce(Return(&mBufferContent));

    FlushClient();

    // The client should receive the handle initialization message from the server.
    // Mock a deserialization failure.
    MockClientReadHandleDeserializeInitializeFailure(clientHandle);

    // Failed deserialization is a fatal failure and the client synchronously receives a
    // DEVICE_LOST callback.
    EXPECT_CALL(*mockBufferMapReadCallback,
                Call(WGPUBufferMapAsyncStatus_DeviceLost, nullptr, 0, _))
        .Times(1);

    // The handle will be destroyed since deserializing failed.
    EXPECT_CALL(clientMemoryTransferService, OnReadHandleDestroy(clientHandle)).Times(1);

    FlushServer(false);

    EXPECT_CALL(serverMemoryTransferService, OnReadHandleDestroy(serverHandle)).Times(1);
}

// Test MapRead destroying the buffer before unmapping on the client side.
TEST_F(WireMemoryTransferServiceTests, BufferMapReadDestroyBeforeUnmap) {
    WGPUBuffer buffer;
    WGPUBuffer apiBuffer;
    std::tie(apiBuffer, buffer) = CreateBuffer();
    FlushClient();

    // The client should create and serialize a ReadHandle on mapReadAsync.
    ClientReadHandle* clientHandle = ExpectReadHandleCreation();
    ExpectReadHandleSerialization(clientHandle);

    wgpuBufferMapReadAsync(buffer, ToMockBufferMapReadCallback, nullptr);

    // The server should deserialize the MapRead handle from the client and then serialize
    // an initialization message.
    ServerReadHandle* serverHandle = ExpectServerReadHandleDeserialize();
    ExpectServerReadHandleInitialize(serverHandle);

    // Mock a successful callback
    EXPECT_CALL(api, OnBufferMapAsyncCallback(apiBuffer, _, _)).WillOnce(InvokeWithoutArgs([&]() {
        api.CallMapAsyncCallback(apiBuffer, WGPUBufferMapAsyncStatus_Success);
    }));
    EXPECT_CALL(api, BufferGetConstMappedRange(apiBuffer, 0, kBufferSize))
        .WillOnce(Return(&mBufferContent));

    FlushClient();

    // The client receives a successful callback.
    EXPECT_CALL(*mockBufferMapReadCallback,
                Call(WGPUBufferMapAsyncStatus_Success, &mBufferContent, sizeof(mBufferContent), _))
        .Times(1);

    // The client should receive the handle initialization message from the server.
    ExpectClientReadHandleDeserializeInitialize(clientHandle, &mBufferContent);

    FlushServer();

    // THIS IS THE TEST: destroy the buffer before unmapping and check it destroyed the mapping
    // immediately, both in the client and server side.
    {
        EXPECT_CALL(clientMemoryTransferService, OnReadHandleDestroy(clientHandle)).Times(1);
        wgpuBufferDestroy(buffer);

        EXPECT_CALL(serverMemoryTransferService, OnReadHandleDestroy(serverHandle)).Times(1);
        EXPECT_CALL(api, BufferDestroy(apiBuffer)).Times(1);
        FlushClient();

        // The handle is already destroyed so unmap only results in a server unmap call.
        wgpuBufferUnmap(buffer);

        EXPECT_CALL(api, BufferUnmap(apiBuffer)).Times(1);
        FlushClient();
    }
}

// Test successful MapWrite.
TEST_F(WireMemoryTransferServiceTests, BufferMapWriteSuccess) {
    WGPUBuffer buffer;
    WGPUBuffer apiBuffer;
    std::tie(apiBuffer, buffer) = CreateBuffer();
    FlushClient();

    ClientWriteHandle* clientHandle = ExpectWriteHandleCreation();
    ExpectWriteHandleSerialization(clientHandle);

    wgpuBufferMapWriteAsync(buffer, ToMockBufferMapWriteCallback, nullptr);

    // The server should then deserialize the WriteHandle from the client.
    ServerWriteHandle* serverHandle = ExpectServerWriteHandleDeserialization();

    // Mock a successful callback.
    EXPECT_CALL(api, OnBufferMapAsyncCallback(apiBuffer, _, _)).WillOnce(InvokeWithoutArgs([&]() {
        api.CallMapAsyncCallback(apiBuffer, WGPUBufferMapAsyncStatus_Success);
    }));
    EXPECT_CALL(api, BufferGetMappedRange(apiBuffer, 0, kBufferSize))
        .WillOnce(Return(&mMappedBufferContent));

    FlushClient();

    // The client receives a successful callback.
    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(WGPUBufferMapAsyncStatus_Success, &mMappedBufferContent,
                     sizeof(mMappedBufferContent), _))
        .Times(1);

    // Since the mapping succeeds, the client opens the WriteHandle.
    ExpectClientWriteHandleOpen(clientHandle, &mMappedBufferContent);

    FlushServer();

    // The client writes to the handle contents.
    mMappedBufferContent = mUpdatedBufferContent;

    // The client will then flush and destroy the handle on Unmap()
    ExpectClientWriteHandleSerializeFlush(clientHandle);
    EXPECT_CALL(clientMemoryTransferService, OnWriteHandleDestroy(clientHandle)).Times(1);

    wgpuBufferUnmap(buffer);

    // The server deserializes the Flush message.
    ExpectServerWriteHandleDeserializeFlush(serverHandle, mUpdatedBufferContent);

    // After the handle is updated it can be destroyed.
    EXPECT_CALL(serverMemoryTransferService, OnWriteHandleDestroy(serverHandle)).Times(1);
    EXPECT_CALL(api, BufferUnmap(apiBuffer)).Times(1);

    FlushClient();
}

// Test unsuccessful MapWrite.
TEST_F(WireMemoryTransferServiceTests, BufferMapWriteError) {
    WGPUBuffer buffer;
    WGPUBuffer apiBuffer;
    std::tie(apiBuffer, buffer) = CreateBuffer();
    FlushClient();

    // The client should create and serialize a WriteHandle on mapWriteAsync.
    ClientWriteHandle* clientHandle = ExpectWriteHandleCreation();
    ExpectWriteHandleSerialization(clientHandle);

    wgpuBufferMapWriteAsync(buffer, ToMockBufferMapWriteCallback, nullptr);

    // The server should then deserialize the WriteHandle from the client.
    ServerWriteHandle* serverHandle = ExpectServerWriteHandleDeserialization();

    // Mock an error callback.
    EXPECT_CALL(api, OnBufferMapAsyncCallback(apiBuffer, _, _)).WillOnce(InvokeWithoutArgs([&]() {
        api.CallMapAsyncCallback(apiBuffer, WGPUBufferMapAsyncStatus_Error);
    }));

    // Since the mapping fails, the handle is immediately destroyed because it won't be written.
    EXPECT_CALL(serverMemoryTransferService, OnWriteHandleDestroy(serverHandle)).Times(1);

    FlushClient();

    // The client receives an error callback.
    EXPECT_CALL(*mockBufferMapWriteCallback, Call(WGPUBufferMapAsyncStatus_Error, nullptr, 0, _))
        .Times(1);

    // Client receives the map failure and destroys the handle.
    EXPECT_CALL(clientMemoryTransferService, OnWriteHandleDestroy(clientHandle)).Times(1);

    FlushServer();

    wgpuBufferUnmap(buffer);

    EXPECT_CALL(api, BufferUnmap(apiBuffer)).Times(1);

    FlushClient();
}

// Test MapRead WriteHandle creation failure.
TEST_F(WireMemoryTransferServiceTests, BufferMapWriteHandleCreationFailure) {
    WGPUBuffer buffer;
    WGPUBuffer apiBuffer;
    std::tie(apiBuffer, buffer) = CreateBuffer();
    FlushClient();

    // Mock a WriteHandle creation failure
    MockWriteHandleCreationFailure();

    // Failed creation of a WriteHandle is a mapping failure and the client synchronously receives
    // an error callback.
    EXPECT_CALL(*mockBufferMapWriteCallback, Call(WGPUBufferMapAsyncStatus_Error, nullptr, 0, _))
        .Times(1);

    wgpuBufferMapWriteAsync(buffer, ToMockBufferMapWriteCallback, nullptr);
}

// Test MapWrite DeserializeWriteHandle failure.
TEST_F(WireMemoryTransferServiceTests, BufferMapWriteDeserializeWriteHandleFailure) {
    WGPUBuffer buffer;
    WGPUBuffer apiBuffer;
    std::tie(apiBuffer, buffer) = CreateBuffer();
    FlushClient();

    // The client should create and serialize a WriteHandle on mapWriteAsync.
    ClientWriteHandle* clientHandle = ExpectWriteHandleCreation();
    ExpectWriteHandleSerialization(clientHandle);

    wgpuBufferMapWriteAsync(buffer, ToMockBufferMapWriteCallback, nullptr);

    // Mock a deserialization failure.
    MockServerWriteHandleDeserializeFailure();

    FlushClient(false);

    // The server hit a fatal failure and never returned the callback. The client callback is
    // called when the wire is destructed.
    EXPECT_CALL(*mockBufferMapWriteCallback, Call(WGPUBufferMapAsyncStatus_Unknown, nullptr, 0, _))
        .Times(1);

    EXPECT_CALL(clientMemoryTransferService, OnWriteHandleDestroy(clientHandle)).Times(1);
}

// Test MapWrite handle Open failure.
TEST_F(WireMemoryTransferServiceTests, BufferMapWriteHandleOpenFailure) {
    WGPUBuffer buffer;
    WGPUBuffer apiBuffer;
    std::tie(apiBuffer, buffer) = CreateBuffer();
    FlushClient();

    ClientWriteHandle* clientHandle = ExpectWriteHandleCreation();
    ExpectWriteHandleSerialization(clientHandle);

    wgpuBufferMapWriteAsync(buffer, ToMockBufferMapWriteCallback, nullptr);

    // The server should then deserialize the WriteHandle from the client.
    ServerWriteHandle* serverHandle = ExpectServerWriteHandleDeserialization();

    // Mock a successful callback.
    EXPECT_CALL(api, OnBufferMapAsyncCallback(apiBuffer, _, _)).WillOnce(InvokeWithoutArgs([&]() {
        api.CallMapAsyncCallback(apiBuffer, WGPUBufferMapAsyncStatus_Success);
    }));
    EXPECT_CALL(api, BufferGetMappedRange(apiBuffer, 0, kBufferSize))
        .WillOnce(Return(&mMappedBufferContent));

    FlushClient();

    // Since the mapping succeeds, the client opens the WriteHandle.
    // Mock a failure.
    MockClientWriteHandleOpenFailure(clientHandle);

    // Failing to open a handle is a fatal failure and the client receives a DEVICE_LOST callback.
    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(WGPUBufferMapAsyncStatus_DeviceLost, nullptr, 0, _))
        .Times(1);

    // Since opening the handle fails, it gets destroyed immediately.
    EXPECT_CALL(clientMemoryTransferService, OnWriteHandleDestroy(clientHandle)).Times(1);

    FlushServer(false);

    EXPECT_CALL(serverMemoryTransferService, OnWriteHandleDestroy(serverHandle)).Times(1);
}

// Test MapWrite DeserializeFlush failure.
TEST_F(WireMemoryTransferServiceTests, BufferMapWriteDeserializeFlushFailure) {
    WGPUBuffer buffer;
    WGPUBuffer apiBuffer;
    std::tie(apiBuffer, buffer) = CreateBuffer();
    FlushClient();

    ClientWriteHandle* clientHandle = ExpectWriteHandleCreation();
    ExpectWriteHandleSerialization(clientHandle);

    wgpuBufferMapWriteAsync(buffer, ToMockBufferMapWriteCallback, nullptr);

    // The server should then deserialize the WriteHandle from the client.
    ServerWriteHandle* serverHandle = ExpectServerWriteHandleDeserialization();

    // Mock a successful callback.
    EXPECT_CALL(api, OnBufferMapAsyncCallback(apiBuffer, _, _)).WillOnce(InvokeWithoutArgs([&]() {
        api.CallMapAsyncCallback(apiBuffer, WGPUBufferMapAsyncStatus_Success);
    }));
    EXPECT_CALL(api, BufferGetMappedRange(apiBuffer, 0, kBufferSize))
        .WillOnce(Return(&mMappedBufferContent));

    FlushClient();

    // The client receives a success callback.
    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(WGPUBufferMapAsyncStatus_Success, &mMappedBufferContent,
                     sizeof(mMappedBufferContent), _))
        .Times(1);

    // Since the mapping succeeds, the client opens the WriteHandle.
    ExpectClientWriteHandleOpen(clientHandle, &mMappedBufferContent);

    FlushServer();

    // The client writes to the handle contents.
    mMappedBufferContent = mUpdatedBufferContent;

    // The client will then flush and destroy the handle on Unmap()
    ExpectClientWriteHandleSerializeFlush(clientHandle);
    EXPECT_CALL(clientMemoryTransferService, OnWriteHandleDestroy(clientHandle)).Times(1);

    wgpuBufferUnmap(buffer);

    // The server deserializes the Flush message. Mock a deserialization failure.
    MockServerWriteHandleDeserializeFlushFailure(serverHandle);

    FlushClient(false);

    EXPECT_CALL(serverMemoryTransferService, OnWriteHandleDestroy(serverHandle)).Times(1);
}

// Test MapWrite destroying the buffer before unmapping on the client side.
TEST_F(WireMemoryTransferServiceTests, BufferMapWriteDestroyBeforeUnmap) {
    WGPUBuffer buffer;
    WGPUBuffer apiBuffer;
    std::tie(apiBuffer, buffer) = CreateBuffer();
    FlushClient();

    ClientWriteHandle* clientHandle = ExpectWriteHandleCreation();
    ExpectWriteHandleSerialization(clientHandle);

    wgpuBufferMapWriteAsync(buffer, ToMockBufferMapWriteCallback, nullptr);

    // The server should then deserialize the WriteHandle from the client.
    ServerWriteHandle* serverHandle = ExpectServerWriteHandleDeserialization();

    // Mock a successful callback.
    EXPECT_CALL(api, OnBufferMapAsyncCallback(apiBuffer, _, _)).WillOnce(InvokeWithoutArgs([&]() {
        api.CallMapAsyncCallback(apiBuffer, WGPUBufferMapAsyncStatus_Success);
    }));
    EXPECT_CALL(api, BufferGetMappedRange(apiBuffer, 0, kBufferSize))
        .WillOnce(Return(&mMappedBufferContent));

    FlushClient();

    // The client receives a successful callback.
    EXPECT_CALL(*mockBufferMapWriteCallback,
                Call(WGPUBufferMapAsyncStatus_Success, &mMappedBufferContent,
                     sizeof(mMappedBufferContent), _))
        .Times(1);

    // Since the mapping succeeds, the client opens the WriteHandle.
    ExpectClientWriteHandleOpen(clientHandle, &mMappedBufferContent);

    FlushServer();

    // The client writes to the handle contents.
    mMappedBufferContent = mUpdatedBufferContent;

    // THIS IS THE TEST: destroy the buffer before unmapping and check it destroyed the mapping
    // immediately, both in the client and server side.
    {
        EXPECT_CALL(clientMemoryTransferService, OnWriteHandleDestroy(clientHandle)).Times(1);
        wgpuBufferDestroy(buffer);

        EXPECT_CALL(serverMemoryTransferService, OnWriteHandleDestroy(serverHandle)).Times(1);
        EXPECT_CALL(api, BufferDestroy(apiBuffer)).Times(1);
        FlushClient();

        // The handle is already destroyed so unmap only results in a server unmap call.
        wgpuBufferUnmap(buffer);

        EXPECT_CALL(api, BufferUnmap(apiBuffer)).Times(1);
        FlushClient();
    }
}

// Test successful CreateBufferMapped.
TEST_F(WireMemoryTransferServiceTests, CreateBufferMappedSuccess) {
    // The client should create and serialize a WriteHandle on createBufferMapped.
    ClientWriteHandle* clientHandle = ExpectWriteHandleCreation();

    // Staging data is immediately available so the handle is Opened.
    ExpectClientWriteHandleOpen(clientHandle, &mMappedBufferContent);

    ExpectWriteHandleSerialization(clientHandle);

    // The server should then deserialize the WriteHandle from the client.
    ServerWriteHandle* serverHandle = ExpectServerWriteHandleDeserialization();

    WGPUCreateBufferMappedResult result;
    WGPUCreateBufferMappedResult apiResult;
    std::tie(apiResult, result) = CreateBufferMapped();
    FlushClient();

    // Update the mapped contents.
    mMappedBufferContent = mUpdatedBufferContent;

    // When the client Unmaps the buffer, it will flush writes to the handle and destroy it.
    ExpectClientWriteHandleSerializeFlush(clientHandle);
    EXPECT_CALL(clientMemoryTransferService, OnWriteHandleDestroy(clientHandle)).Times(1);

    wgpuBufferUnmap(result.buffer);

    // The server deserializes the Flush message.
    ExpectServerWriteHandleDeserializeFlush(serverHandle, mUpdatedBufferContent);

    // After the handle is updated it can be destroyed.
    EXPECT_CALL(serverMemoryTransferService, OnWriteHandleDestroy(serverHandle)).Times(1);
    EXPECT_CALL(api, BufferUnmap(apiResult.buffer)).Times(1);

    FlushClient();
}

// Test CreateBufferMapped WriteHandle creation failure.
TEST_F(WireMemoryTransferServiceTests, CreateBufferMappedWriteHandleCreationFailure) {
    // Mock a WriteHandle creation failure
    MockWriteHandleCreationFailure();

    WGPUBufferDescriptor descriptor = {};
    descriptor.size = sizeof(mBufferContent);

    WGPUCreateBufferMappedResult result = wgpuDeviceCreateBufferMapped(device, &descriptor);

    // TODO(enga): Check that the client generated a context lost.
    EXPECT_EQ(result.data, nullptr);
    EXPECT_EQ(result.dataLength, 0u);
}

// Test CreateBufferMapped DeserializeWriteHandle failure.
TEST_F(WireMemoryTransferServiceTests, CreateBufferMappedDeserializeWriteHandleFailure) {
    // The client should create and serialize a WriteHandle on createBufferMapped.
    ClientWriteHandle* clientHandle = ExpectWriteHandleCreation();

    // Staging data is immediately available so the handle is Opened.
    ExpectClientWriteHandleOpen(clientHandle, &mMappedBufferContent);

    ExpectWriteHandleSerialization(clientHandle);

    // The server should then deserialize the WriteHandle from the client.
    MockServerWriteHandleDeserializeFailure();

    WGPUCreateBufferMappedResult result;
    WGPUCreateBufferMappedResult apiResult;
    std::tie(apiResult, result) = CreateBufferMapped();
    FlushClient(false);

    EXPECT_CALL(clientMemoryTransferService, OnWriteHandleDestroy(clientHandle)).Times(1);
}

// Test CreateBufferMapped handle Open failure.
TEST_F(WireMemoryTransferServiceTests, CreateBufferMappedHandleOpenFailure) {
    // The client should create a WriteHandle on createBufferMapped.
    ClientWriteHandle* clientHandle = ExpectWriteHandleCreation();

    // Staging data is immediately available so the handle is Opened.
    // Mock a failure.
    MockClientWriteHandleOpenFailure(clientHandle);

    // Since synchronous opening of the handle failed, it is destroyed immediately.
    // Note: The handle is not serialized because sychronously opening it failed.
    EXPECT_CALL(clientMemoryTransferService, OnWriteHandleDestroy(clientHandle)).Times(1);

    WGPUBufferDescriptor descriptor = {};
    descriptor.size = sizeof(mBufferContent);

    WGPUCreateBufferMappedResult result = wgpuDeviceCreateBufferMapped(device, &descriptor);

    // TODO(enga): Check that the client generated a context lost.
    EXPECT_EQ(result.data, nullptr);
    EXPECT_EQ(result.dataLength, 0u);
}

// Test CreateBufferMapped DeserializeFlush failure.
TEST_F(WireMemoryTransferServiceTests, CreateBufferMappedDeserializeFlushFailure) {
    // The client should create and serialize a WriteHandle on createBufferMapped.
    ClientWriteHandle* clientHandle = ExpectWriteHandleCreation();

    // Staging data is immediately available so the handle is Opened.
    ExpectClientWriteHandleOpen(clientHandle, &mMappedBufferContent);

    ExpectWriteHandleSerialization(clientHandle);

    // The server should then deserialize the WriteHandle from the client.
    ServerWriteHandle* serverHandle = ExpectServerWriteHandleDeserialization();

    WGPUCreateBufferMappedResult result;
    WGPUCreateBufferMappedResult apiResult;
    std::tie(apiResult, result) = CreateBufferMapped();
    FlushClient();

    // Update the mapped contents.
    mMappedBufferContent = mUpdatedBufferContent;

    // When the client Unmaps the buffer, it will flush writes to the handle and destroy it.
    ExpectClientWriteHandleSerializeFlush(clientHandle);
    EXPECT_CALL(clientMemoryTransferService, OnWriteHandleDestroy(clientHandle)).Times(1);

    wgpuBufferUnmap(result.buffer);

    // The server deserializes the Flush message. Mock a deserialization failure.
    MockServerWriteHandleDeserializeFlushFailure(serverHandle);

    FlushClient(false);

    EXPECT_CALL(serverMemoryTransferService, OnWriteHandleDestroy(serverHandle)).Times(1);
}

// Test CreateBufferMapped destroying the buffer before unmapping on the client side.
TEST_F(WireMemoryTransferServiceTests, CreateBufferMappedDestroyBeforeUnmap) {
    // The client should create and serialize a WriteHandle on createBufferMapped.
    ClientWriteHandle* clientHandle = ExpectWriteHandleCreation();

    // Staging data is immediately available so the handle is Opened.
    ExpectClientWriteHandleOpen(clientHandle, &mMappedBufferContent);

    ExpectWriteHandleSerialization(clientHandle);

    // The server should then deserialize the WriteHandle from the client.
    ServerWriteHandle* serverHandle = ExpectServerWriteHandleDeserialization();

    WGPUCreateBufferMappedResult result;
    WGPUCreateBufferMappedResult apiResult;
    std::tie(apiResult, result) = CreateBufferMapped();
    FlushClient();

    // Update the mapped contents.
    mMappedBufferContent = mUpdatedBufferContent;

    // THIS IS THE TEST: destroy the buffer before unmapping and check it destroyed the mapping
    // immediately, both in the client and server side.
    {
        EXPECT_CALL(clientMemoryTransferService, OnWriteHandleDestroy(clientHandle)).Times(1);
        wgpuBufferDestroy(result.buffer);

        EXPECT_CALL(serverMemoryTransferService, OnWriteHandleDestroy(serverHandle)).Times(1);
        EXPECT_CALL(api, BufferDestroy(apiResult.buffer)).Times(1);
        FlushClient();

        // The handle is already destroyed so unmap only results in a server unmap call.
        wgpuBufferUnmap(result.buffer);

        EXPECT_CALL(api, BufferUnmap(apiResult.buffer)).Times(1);
        FlushClient();
    }
}
