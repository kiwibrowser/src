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

#include <gmock/gmock.h>
#include "tests/DawnTest.h"

#include <array>
#include <cstring>

class MockFenceOnCompletionCallback {
  public:
    MOCK_METHOD2(Call, void(DawnFenceCompletionStatus status, void* userdata));
};

static std::unique_ptr<MockFenceOnCompletionCallback> mockFenceOnCompletionCallback;
static void ToMockFenceOnCompletionCallback(DawnFenceCompletionStatus status, void* userdata) {
    mockFenceOnCompletionCallback->Call(status, userdata);
}
class FenceTests : public DawnTest {
  private:
    struct CallbackInfo {
        FenceTests* test;
        uint64_t value;
        DawnFenceCompletionStatus status;
        int32_t callIndex = -1;  // If this is -1, the callback was not called

        void Update(DawnFenceCompletionStatus status) {
            this->callIndex = test->mCallIndex++;
            this->status = status;
        }
    };

    int32_t mCallIndex;

  protected:
    FenceTests() : mCallIndex(0) {
    }

    void SetUp() override {
        DawnTest::SetUp();
        mockFenceOnCompletionCallback = std::make_unique<MockFenceOnCompletionCallback>();
    }

    void TearDown() override {
        mockFenceOnCompletionCallback = nullptr;
        DawnTest::TearDown();
    }

    void WaitForCompletedValue(dawn::Fence fence, uint64_t completedValue) {
        while (fence.GetCompletedValue() < completedValue) {
            WaitABit();
        }
    }
};

// Test that signaling a fence updates the completed value
TEST_P(FenceTests, SimpleSignal) {
    dawn::FenceDescriptor descriptor;
    descriptor.initialValue = 1u;
    dawn::Fence fence = queue.CreateFence(&descriptor);

    // Completed value starts at initial value
    EXPECT_EQ(fence.GetCompletedValue(), 1u);

    queue.Signal(fence, 2);
    WaitForCompletedValue(fence, 2);

    // Completed value updates to signaled value
    EXPECT_EQ(fence.GetCompletedValue(), 2u);
}

// Test callbacks are called in increasing order of fence completion value
TEST_P(FenceTests, OnCompletionOrdering) {
    dawn::FenceDescriptor descriptor;
    descriptor.initialValue = 0u;
    dawn::Fence fence = queue.CreateFence(&descriptor);

    queue.Signal(fence, 4);

    {
        testing::InSequence s;

        EXPECT_CALL(*mockFenceOnCompletionCallback,
                    Call(DAWN_FENCE_COMPLETION_STATUS_SUCCESS, this + 0))
            .Times(1);

        EXPECT_CALL(*mockFenceOnCompletionCallback,
                    Call(DAWN_FENCE_COMPLETION_STATUS_SUCCESS, this + 1))
            .Times(1);

        EXPECT_CALL(*mockFenceOnCompletionCallback,
                    Call(DAWN_FENCE_COMPLETION_STATUS_SUCCESS, this + 2))
            .Times(1);

        EXPECT_CALL(*mockFenceOnCompletionCallback,
                    Call(DAWN_FENCE_COMPLETION_STATUS_SUCCESS, this + 3))
            .Times(1);
    }

    fence.OnCompletion(2u, ToMockFenceOnCompletionCallback, this + 2);
    fence.OnCompletion(0u, ToMockFenceOnCompletionCallback, this + 0);
    fence.OnCompletion(3u, ToMockFenceOnCompletionCallback, this + 3);
    fence.OnCompletion(1u, ToMockFenceOnCompletionCallback, this + 1);

    WaitForCompletedValue(fence, 4);
}

// Test callbacks still occur if Queue::Signal happens multiple times
TEST_P(FenceTests, MultipleSignalOnCompletion) {
    dawn::FenceDescriptor descriptor;
    descriptor.initialValue = 0u;
    dawn::Fence fence = queue.CreateFence(&descriptor);

    queue.Signal(fence, 2);
    queue.Signal(fence, 4);

    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(DAWN_FENCE_COMPLETION_STATUS_SUCCESS, nullptr))
        .Times(1);
    fence.OnCompletion(3u, ToMockFenceOnCompletionCallback, nullptr);

    WaitForCompletedValue(fence, 4);
}

// Test all callbacks are called if they are added for the same fence value
TEST_P(FenceTests, OnCompletionMultipleCallbacks) {
    dawn::FenceDescriptor descriptor;
    descriptor.initialValue = 0u;
    dawn::Fence fence = queue.CreateFence(&descriptor);

    queue.Signal(fence, 4);

    EXPECT_CALL(*mockFenceOnCompletionCallback,
                Call(DAWN_FENCE_COMPLETION_STATUS_SUCCESS, this + 0))
        .Times(1);

    EXPECT_CALL(*mockFenceOnCompletionCallback,
                Call(DAWN_FENCE_COMPLETION_STATUS_SUCCESS, this + 1))
        .Times(1);

    EXPECT_CALL(*mockFenceOnCompletionCallback,
                Call(DAWN_FENCE_COMPLETION_STATUS_SUCCESS, this + 2))
        .Times(1);

    EXPECT_CALL(*mockFenceOnCompletionCallback,
                Call(DAWN_FENCE_COMPLETION_STATUS_SUCCESS, this + 3))
        .Times(1);

    fence.OnCompletion(4u, ToMockFenceOnCompletionCallback, this + 0);
    fence.OnCompletion(4u, ToMockFenceOnCompletionCallback, this + 1);
    fence.OnCompletion(4u, ToMockFenceOnCompletionCallback, this + 2);
    fence.OnCompletion(4u, ToMockFenceOnCompletionCallback, this + 3);

    WaitForCompletedValue(fence, 4u);
}

// TODO(enga): Enable when fence is removed from fence signal tracker
// Currently it holds a reference and is not destructed
TEST_P(FenceTests, DISABLED_DestroyBeforeOnCompletionEnd) {
    dawn::FenceDescriptor descriptor;
    descriptor.initialValue = 0u;
    dawn::Fence fence = queue.CreateFence(&descriptor);

    // The fence in this block will be deleted when it goes out of scope
    {
        dawn::FenceDescriptor descriptor;
        descriptor.initialValue = 0u;
        dawn::Fence testFence = queue.CreateFence(&descriptor);

        queue.Signal(testFence, 4);

        EXPECT_CALL(*mockFenceOnCompletionCallback,
                    Call(DAWN_FENCE_COMPLETION_STATUS_UNKNOWN, this + 0))
            .Times(1);

        EXPECT_CALL(*mockFenceOnCompletionCallback,
                    Call(DAWN_FENCE_COMPLETION_STATUS_UNKNOWN, this + 1))
            .Times(1);

        EXPECT_CALL(*mockFenceOnCompletionCallback,
                    Call(DAWN_FENCE_COMPLETION_STATUS_UNKNOWN, this + 2))
            .Times(1);

        EXPECT_CALL(*mockFenceOnCompletionCallback,
                    Call(DAWN_FENCE_COMPLETION_STATUS_UNKNOWN, this + 3))
            .Times(1);

        testFence.OnCompletion(1u, ToMockFenceOnCompletionCallback, this + 0);
        testFence.OnCompletion(2u, ToMockFenceOnCompletionCallback, this + 1);
        testFence.OnCompletion(2u, ToMockFenceOnCompletionCallback, this + 2);
        testFence.OnCompletion(3u, ToMockFenceOnCompletionCallback, this + 3);
    }

    // Wait for another fence to be sure all callbacks have cleared
    queue.Signal(fence, 1);
    WaitForCompletedValue(fence, 1);
}

DAWN_INSTANTIATE_TEST(FenceTests, D3D12Backend, MetalBackend, OpenGLBackend, VulkanBackend);
