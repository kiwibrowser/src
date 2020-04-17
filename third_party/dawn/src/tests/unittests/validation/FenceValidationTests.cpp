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
    MOCK_METHOD2(Call, void(DawnFenceCompletionStatus status, void* userdata));
};

struct FenceOnCompletionExpectation {
    dawn::Fence fence;
    uint64_t value;
    DawnFenceCompletionStatus status;
};

static std::unique_ptr<MockFenceOnCompletionCallback> mockFenceOnCompletionCallback;
static void ToMockFenceOnCompletionCallback(DawnFenceCompletionStatus status, void* userdata) {
    mockFenceOnCompletionCallback->Call(status, userdata);
}

class FenceValidationTest : public ValidationTest {
  protected:
    void TestOnCompletion(dawn::Fence fence, uint64_t value, DawnFenceCompletionStatus status) {
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

    dawn::Queue queue;

  private:
    void SetUp() override {
        ValidationTest::SetUp();

        mockFenceOnCompletionCallback = std::make_unique<MockFenceOnCompletionCallback>();
        queue = device.CreateQueue();
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
        dawn::FenceDescriptor descriptor;
        descriptor.initialValue = 0;
        queue.CreateFence(&descriptor);
    }
}

TEST_F(FenceValidationTest, GetCompletedValue) {
    // Starts at initial value
    {
        dawn::FenceDescriptor descriptor;
        descriptor.initialValue = 1;
        dawn::Fence fence = queue.CreateFence(&descriptor);
        EXPECT_EQ(fence.GetCompletedValue(), 1u);
    }
}

// Test that OnCompletion handlers are called immediately for
// already completed fence values
TEST_F(FenceValidationTest, OnCompletionImmediate) {
    dawn::FenceDescriptor descriptor;
    descriptor.initialValue = 1;
    dawn::Fence fence = queue.CreateFence(&descriptor);

    EXPECT_CALL(*mockFenceOnCompletionCallback,
                Call(DAWN_FENCE_COMPLETION_STATUS_SUCCESS, this + 0))
        .Times(1);
    fence.OnCompletion(0u, ToMockFenceOnCompletionCallback, this + 0);

    EXPECT_CALL(*mockFenceOnCompletionCallback,
                Call(DAWN_FENCE_COMPLETION_STATUS_SUCCESS, this + 1))
        .Times(1);
    fence.OnCompletion(1u, ToMockFenceOnCompletionCallback, this + 1);
}

// Test setting OnCompletion handlers for values > signaled value
TEST_F(FenceValidationTest, OnCompletionLargerThanSignaled) {
    dawn::FenceDescriptor descriptor;
    descriptor.initialValue = 1;
    dawn::Fence fence = queue.CreateFence(&descriptor);

    // Cannot signal for values > signaled value
    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(DAWN_FENCE_COMPLETION_STATUS_ERROR, nullptr))
        .Times(1);
    ASSERT_DEVICE_ERROR(fence.OnCompletion(2u, ToMockFenceOnCompletionCallback, nullptr));

    // Can set handler after signaling
    queue.Signal(fence, 2);
    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(DAWN_FENCE_COMPLETION_STATUS_SUCCESS, nullptr))
        .Times(1);
    fence.OnCompletion(2u, ToMockFenceOnCompletionCallback, nullptr);

    Flush();
}

TEST_F(FenceValidationTest, GetCompletedValueInsideCallback) {
    dawn::FenceDescriptor descriptor;
    descriptor.initialValue = 1;
    dawn::Fence fence = queue.CreateFence(&descriptor);

    queue.Signal(fence, 3);
    fence.OnCompletion(2u, ToMockFenceOnCompletionCallback, nullptr);
    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(DAWN_FENCE_COMPLETION_STATUS_SUCCESS, nullptr))
        .WillOnce(Invoke([&](DawnFenceCompletionStatus status, void* userdata) {
            EXPECT_EQ(fence.GetCompletedValue(), 3u);
        }));

    Flush();
}

TEST_F(FenceValidationTest, GetCompletedValueAfterCallback) {
    dawn::FenceDescriptor descriptor;
    descriptor.initialValue = 1;
    dawn::Fence fence = queue.CreateFence(&descriptor);

    queue.Signal(fence, 2);
    fence.OnCompletion(2u, ToMockFenceOnCompletionCallback, nullptr);
    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(DAWN_FENCE_COMPLETION_STATUS_SUCCESS, nullptr))
        .Times(1);

    Flush();
    EXPECT_EQ(fence.GetCompletedValue(), 2u);
}

TEST_F(FenceValidationTest, SignalError) {
    dawn::FenceDescriptor descriptor;
    descriptor.initialValue = 1;
    dawn::Fence fence = queue.CreateFence(&descriptor);

    // value < fence signaled value
    ASSERT_DEVICE_ERROR(queue.Signal(fence, 0));

    // value == fence signaled value
    ASSERT_DEVICE_ERROR(queue.Signal(fence, 1));
}

TEST_F(FenceValidationTest, SignalSuccess) {
    dawn::FenceDescriptor descriptor;
    descriptor.initialValue = 1;
    dawn::Fence fence = queue.CreateFence(&descriptor);

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
TEST_F(FenceValidationTest, SignalWrongQueue) {
    dawn::Queue queue2 = device.CreateQueue();

    dawn::FenceDescriptor descriptor;
    descriptor.initialValue = 1;
    dawn::Fence fence = queue.CreateFence(&descriptor);

    ASSERT_DEVICE_ERROR(queue2.Signal(fence, 2));
}

// Test that signaling a fence on a wrong queue does not update fence signaled value
TEST_F(FenceValidationTest, SignalWrongQueueDoesNotUpdateValue) {
    dawn::Queue queue2 = device.CreateQueue();

    dawn::FenceDescriptor descriptor;
    descriptor.initialValue = 1;
    dawn::Fence fence = queue.CreateFence(&descriptor);

    ASSERT_DEVICE_ERROR(queue2.Signal(fence, 2));

    // Fence value should be unchanged.
    Flush();
    EXPECT_EQ(fence.GetCompletedValue(), 1u);

    // Signaling with 2 on the correct queue should succeed
    queue.Signal(fence, 2);
    Flush();
    EXPECT_EQ(fence.GetCompletedValue(), 2u);
}
