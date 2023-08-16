// Copyright 2018 The Dawn Authors
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

using namespace testing;

class MockFenceOnCompletionCallback {
  public:
    MOCK_METHOD(void, Call, (WGPUFenceCompletionStatus status, void* userdata));
};

struct FenceOnCompletionExpectation {
    wgpu::Fence fence;
    uint64_t value;
    WGPUFenceCompletionStatus status;
};

static std::unique_ptr<MockFenceOnCompletionCallback> mockFenceOnCompletionCallback;
static void ToMockFenceOnCompletionCallback(WGPUFenceCompletionStatus status, void* userdata) {
    mockFenceOnCompletionCallback->Call(status, userdata);
}

class FenceValidationTest : public ValidationTest {
  protected:
    void TestOnCompletion(wgpu::Fence fence, uint64_t value, WGPUFenceCompletionStatus status) {
        FenceOnCompletionExpectation* expectation = new FenceOnCompletionExpectation;
        expectation->fence = fence;
        expectation->value = value;
        expectation->status = status;

        EXPECT_CALL(*mockFenceOnCompletionCallback, Call(status, expectation)).Times(1);
        fence.OnCompletion(value, ToMockFenceOnCompletionCallback, expectation);
    }

    void Flush() {
        device.Tick();
    }

    wgpu::Queue queue;

  private:
    void SetUp() override {
        ValidationTest::SetUp();

        mockFenceOnCompletionCallback = std::make_unique<MockFenceOnCompletionCallback>();
        queue = device.GetDefaultQueue();
    }

    void TearDown() override {
        // Delete mocks so that expectations are checked
        mockFenceOnCompletionCallback = nullptr;

        ValidationTest::TearDown();
    }
};

// Test cases where creation should succeed
TEST_F(FenceValidationTest, CreationSuccess) {
    // Success
    {
        wgpu::FenceDescriptor descriptor;
        descriptor.initialValue = 0;
        queue.CreateFence(&descriptor);
    }
}

// Creation succeeds if no descriptor is provided
TEST_F(FenceValidationTest, DefaultDescriptor) {
    wgpu::Fence fence = queue.CreateFence();
    EXPECT_EQ(fence.GetCompletedValue(), 0u);
}

TEST_F(FenceValidationTest, GetCompletedValue) {
    // Starts at initial value
    {
        wgpu::FenceDescriptor descriptor;
        descriptor.initialValue = 1;
        wgpu::Fence fence = queue.CreateFence(&descriptor);
        EXPECT_EQ(fence.GetCompletedValue(), 1u);
    }
}

// Test that OnCompletion handlers are called immediately for
// already completed fence values
TEST_F(FenceValidationTest, OnCompletionImmediate) {
    wgpu::FenceDescriptor descriptor;
    descriptor.initialValue = 1;
    wgpu::Fence fence = queue.CreateFence(&descriptor);

    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, this + 0))
        .Times(1);
    fence.OnCompletion(0u, ToMockFenceOnCompletionCallback, this + 0);

    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, this + 1))
        .Times(1);
    fence.OnCompletion(1u, ToMockFenceOnCompletionCallback, this + 1);
}

// Test setting OnCompletion handlers for values > signaled value
TEST_F(FenceValidationTest, OnCompletionLargerThanSignaled) {
    wgpu::FenceDescriptor descriptor;
    descriptor.initialValue = 1;
    wgpu::Fence fence = queue.CreateFence(&descriptor);

    // Cannot signal for values > signaled value
    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Error, nullptr))
        .Times(1);
    ASSERT_DEVICE_ERROR(fence.OnCompletion(2u, ToMockFenceOnCompletionCallback, nullptr));

    // Can set handler after signaling
    queue.Signal(fence, 2);
    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, nullptr))
        .Times(1);
    fence.OnCompletion(2u, ToMockFenceOnCompletionCallback, nullptr);

    Flush();
}

TEST_F(FenceValidationTest, GetCompletedValueInsideCallback) {
    wgpu::FenceDescriptor descriptor;
    descriptor.initialValue = 1;
    wgpu::Fence fence = queue.CreateFence(&descriptor);

    queue.Signal(fence, 3);
    fence.OnCompletion(2u, ToMockFenceOnCompletionCallback, nullptr);
    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, nullptr))
        .WillOnce(Invoke([&](WGPUFenceCompletionStatus status, void* userdata) {
            EXPECT_EQ(fence.GetCompletedValue(), 3u);
        }));

    Flush();
}

TEST_F(FenceValidationTest, GetCompletedValueAfterCallback) {
    wgpu::FenceDescriptor descriptor;
    descriptor.initialValue = 1;
    wgpu::Fence fence = queue.CreateFence(&descriptor);

    queue.Signal(fence, 2);
    fence.OnCompletion(2u, ToMockFenceOnCompletionCallback, nullptr);
    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, nullptr))
        .Times(1);

    Flush();
    EXPECT_EQ(fence.GetCompletedValue(), 2u);
}

TEST_F(FenceValidationTest, SignalError) {
    wgpu::FenceDescriptor descriptor;
    descriptor.initialValue = 1;
    wgpu::Fence fence = queue.CreateFence(&descriptor);

    // value < fence signaled value
    ASSERT_DEVICE_ERROR(queue.Signal(fence, 0));

    // value == fence signaled value
    ASSERT_DEVICE_ERROR(queue.Signal(fence, 1));
}

TEST_F(FenceValidationTest, SignalSuccess) {
    wgpu::FenceDescriptor descriptor;
    descriptor.initialValue = 1;
    wgpu::Fence fence = queue.CreateFence(&descriptor);

    // Success
    queue.Signal(fence, 2);
    Flush();
    EXPECT_EQ(fence.GetCompletedValue(), 2u);

    // Success increasing fence value by more than 1
    queue.Signal(fence, 6);
    Flush();
    EXPECT_EQ(fence.GetCompletedValue(), 6u);
}

// Test it is invalid to signal a fence on a different queue than it was created on
// DISABLED until we have support for multiple queues
TEST_F(FenceValidationTest, DISABLED_SignalWrongQueue) {
    wgpu::Queue queue2 = device.GetDefaultQueue();

    wgpu::FenceDescriptor descriptor;
    descriptor.initialValue = 1;
    wgpu::Fence fence = queue.CreateFence(&descriptor);

    ASSERT_DEVICE_ERROR(queue2.Signal(fence, 2));
}

// Test that signaling a fence on a wrong queue does not update fence signaled value
// DISABLED until we have support for multiple queues
TEST_F(FenceValidationTest, DISABLED_SignalWrongQueueDoesNotUpdateValue) {
    wgpu::Queue queue2 = device.GetDefaultQueue();

    wgpu::FenceDescriptor descriptor;
    descriptor.initialValue = 1;
    wgpu::Fence fence = queue.CreateFence(&descriptor);

    ASSERT_DEVICE_ERROR(queue2.Signal(fence, 2));

    // Fence value should be unchanged.
    Flush();
    EXPECT_EQ(fence.GetCompletedValue(), 1u);

    // Signaling with 2 on the correct queue should succeed
    queue.Signal(fence, 2);
    Flush();
    EXPECT_EQ(fence.GetCompletedValue(), 2u);
}
