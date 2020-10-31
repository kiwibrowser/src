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

    class MockFenceOnCompletionCallback {
      public:
        MOCK_METHOD(void, Call, (WGPUFenceCompletionStatus status, void* userdata));
    };

    std::unique_ptr<StrictMock<MockFenceOnCompletionCallback>> mockFenceOnCompletionCallback;
    void ToMockFenceOnCompletionCallback(WGPUFenceCompletionStatus status, void* userdata) {
        mockFenceOnCompletionCallback->Call(status, userdata);
    }

}  // anonymous namespace

class WireFenceTests : public WireTest {
  public:
    WireFenceTests() {
    }
    ~WireFenceTests() override = default;

    void SetUp() override {
        WireTest::SetUp();

        mockFenceOnCompletionCallback =
            std::make_unique<StrictMock<MockFenceOnCompletionCallback>>();

        {
            WGPUFenceDescriptor descriptor = {};
            descriptor.initialValue = 1;

            apiFence = api.GetNewFence();
            fence = wgpuQueueCreateFence(queue, &descriptor);

            EXPECT_CALL(api, QueueCreateFence(apiQueue, _)).WillOnce(Return(apiFence));
            FlushClient();
        }
    }

    void TearDown() override {
        WireTest::TearDown();

        mockFenceOnCompletionCallback = nullptr;
    }

    void FlushServer() {
        WireTest::FlushServer();

        Mock::VerifyAndClearExpectations(&mockFenceOnCompletionCallback);
    }

  protected:
    void DoQueueSignal(uint64_t signalValue) {
        wgpuQueueSignal(queue, fence, signalValue);
        EXPECT_CALL(api, QueueSignal(apiQueue, apiFence, signalValue)).Times(1);

        // This callback is generated to update the completedValue of the fence
        // on the client
        EXPECT_CALL(api, OnFenceOnCompletionCallback(apiFence, signalValue, _, _))
            .WillOnce(InvokeWithoutArgs([&]() {
                api.CallFenceOnCompletionCallback(apiFence, WGPUFenceCompletionStatus_Success);
            }));
    }

    // A successfully created fence
    WGPUFence fence;
    WGPUFence apiFence;
};

// Check that signaling a fence succeeds
TEST_F(WireFenceTests, QueueSignalSuccess) {
    DoQueueSignal(2u);
    DoQueueSignal(3u);
    FlushClient();
    FlushServer();
}

// Errors should be generated when signaling a value less
// than or equal to the current signaled value
TEST_F(WireFenceTests, QueueSignalValidationError) {
    wgpuQueueSignal(queue, fence, 0u);  // Error
    EXPECT_CALL(api, DeviceInjectError(apiDevice, WGPUErrorType_Validation, ValidStringMessage()))
        .Times(1);
    FlushClient();

    wgpuQueueSignal(queue, fence, 1u);  // Error
    EXPECT_CALL(api, DeviceInjectError(apiDevice, WGPUErrorType_Validation, ValidStringMessage()))
        .Times(1);
    FlushClient();

    DoQueueSignal(4u);  // Success
    FlushClient();

    wgpuQueueSignal(queue, fence, 3u);  // Error
    EXPECT_CALL(api, DeviceInjectError(apiDevice, WGPUErrorType_Validation, ValidStringMessage()))
        .Times(1);
    FlushClient();
}

// Check that callbacks are immediately called if the fence is already finished
TEST_F(WireFenceTests, OnCompletionImmediate) {
    // Can call on value < (initial) signaled value happens immediately
    {
        EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, _))
            .Times(1);
        wgpuFenceOnCompletion(fence, 0, ToMockFenceOnCompletionCallback, nullptr);
    }

    // Can call on value == (initial) signaled value happens immediately
    {
        EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, _))
            .Times(1);
        wgpuFenceOnCompletion(fence, 1, ToMockFenceOnCompletionCallback, nullptr);
    }
}

// Check that all passed client completion callbacks are called
TEST_F(WireFenceTests, OnCompletionMultiple) {
    DoQueueSignal(3u);
    DoQueueSignal(6u);

    // Add callbacks in a non-monotonic order. They should still be called
    // in order of increasing fence value.
    // Add multiple callbacks for the same value.
    wgpuFenceOnCompletion(fence, 6, ToMockFenceOnCompletionCallback, this + 0);
    wgpuFenceOnCompletion(fence, 2, ToMockFenceOnCompletionCallback, this + 1);
    wgpuFenceOnCompletion(fence, 3, ToMockFenceOnCompletionCallback, this + 2);
    wgpuFenceOnCompletion(fence, 2, ToMockFenceOnCompletionCallback, this + 3);

    Sequence s1, s2;
    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, this + 1))
        .Times(1)
        .InSequence(s1);
    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, this + 3))
        .Times(1)
        .InSequence(s2);
    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, this + 2))
        .Times(1)
        .InSequence(s1, s2);
    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, this + 0))
        .Times(1)
        .InSequence(s1, s2);

    FlushClient();
    FlushServer();
}

// Without any flushes, it is valid to wait on a value less than or equal to
// the last signaled value
TEST_F(WireFenceTests, OnCompletionSynchronousValidationSuccess) {
    wgpuQueueSignal(queue, fence, 4u);
    wgpuFenceOnCompletion(fence, 2u, ToMockFenceOnCompletionCallback, 0);
    wgpuFenceOnCompletion(fence, 3u, ToMockFenceOnCompletionCallback, 0);
    wgpuFenceOnCompletion(fence, 4u, ToMockFenceOnCompletionCallback, 0);

    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Unknown, _))
        .Times(3);
}

// Errors should be generated when waiting on a value greater
// than the last signaled value
TEST_F(WireFenceTests, OnCompletionValidationError) {
    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Error, this + 0))
        .Times(1);

    wgpuFenceOnCompletion(fence, 2u, ToMockFenceOnCompletionCallback, this + 0);

    EXPECT_CALL(api, DeviceInjectError(apiDevice, WGPUErrorType_Validation, ValidStringMessage()))
        .Times(1);
    FlushClient();
}

// Check that the fence completed value is initialized
TEST_F(WireFenceTests, GetCompletedValueInitialization) {
    EXPECT_EQ(wgpuFenceGetCompletedValue(fence), 1u);
}

// Check that the fence completed value updates after signaling the fence
TEST_F(WireFenceTests, GetCompletedValueUpdate) {
    DoQueueSignal(3u);
    FlushClient();
    FlushServer();

    EXPECT_EQ(wgpuFenceGetCompletedValue(fence), 3u);
}

// Check that the fence completed value does not update without a flush
TEST_F(WireFenceTests, GetCompletedValueNoUpdate) {
    wgpuQueueSignal(queue, fence, 3u);
    EXPECT_EQ(wgpuFenceGetCompletedValue(fence), 1u);
}

// Check that the callback is called with UNKNOWN when the fence is destroyed
// before the completed value is updated
TEST_F(WireFenceTests, DestroyBeforeOnCompletionEnd) {
    wgpuQueueSignal(queue, fence, 3u);
    wgpuFenceOnCompletion(fence, 2u, ToMockFenceOnCompletionCallback, nullptr);
    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Unknown, _))
        .Times(1);
}

// Test that signaling a fence on a wrong queue is invalid
// DISABLED until we have support for multiple queues.
TEST_F(WireFenceTests, DISABLED_SignalWrongQueue) {
    WGPUQueue queue2 = wgpuDeviceGetDefaultQueue(device);
    WGPUQueue apiQueue2 = api.GetNewQueue();
    EXPECT_CALL(api, DeviceGetDefaultQueue(apiDevice)).WillOnce(Return(apiQueue2));
    FlushClient();

    wgpuQueueSignal(queue2, fence, 2u);  // error
    EXPECT_CALL(api, DeviceInjectError(apiDevice, WGPUErrorType_Validation, ValidStringMessage()))
        .Times(1);
    FlushClient();
}

// Test that signaling a fence on a wrong queue does not update fence signaled value
// DISABLED until we have support for multiple queues.
TEST_F(WireFenceTests, DISABLED_SignalWrongQueueDoesNotUpdateValue) {
    WGPUQueue queue2 = wgpuDeviceGetDefaultQueue(device);
    WGPUQueue apiQueue2 = api.GetNewQueue();
    EXPECT_CALL(api, DeviceGetDefaultQueue(apiDevice)).WillOnce(Return(apiQueue2));
    FlushClient();

    wgpuQueueSignal(queue2, fence, 2u);  // error
    EXPECT_CALL(api, DeviceInjectError(apiDevice, WGPUErrorType_Validation, ValidStringMessage()))
        .Times(1);
    FlushClient();

    // Fence value should be unchanged.
    FlushClient();
    FlushServer();
    EXPECT_EQ(wgpuFenceGetCompletedValue(fence), 1u);

    // Signaling with 2 on the correct queue should succeed
    DoQueueSignal(2u);  // success
    FlushClient();
    FlushServer();
    EXPECT_EQ(wgpuFenceGetCompletedValue(fence), 2u);
}
