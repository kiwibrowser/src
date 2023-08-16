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

using namespace testing;

class MockFenceOnCompletionCallback {
  public:
    MOCK_METHOD(void, Call, (WGPUFenceCompletionStatus status, void* userdata));
};

static std::unique_ptr<MockFenceOnCompletionCallback> mockFenceOnCompletionCallback;
static void ToMockFenceOnCompletionCallback(WGPUFenceCompletionStatus status, void* userdata) {
    mockFenceOnCompletionCallback->Call(status, userdata);
}

class MockPopErrorScopeCallback {
  public:
    MOCK_METHOD(void, Call, (WGPUErrorType type, const char* message, void* userdata));
};

static std::unique_ptr<MockPopErrorScopeCallback> mockPopErrorScopeCallback;
static void ToMockPopErrorScopeCallback(WGPUErrorType type, const char* message, void* userdata) {
    mockPopErrorScopeCallback->Call(type, message, userdata);
}

class FenceTests : public DawnTest {
  private:
    struct CallbackInfo {
        FenceTests* test;
        uint64_t value;
        WGPUFenceCompletionStatus status;
        int32_t callIndex = -1;  // If this is -1, the callback was not called

        void Update(WGPUFenceCompletionStatus status) {
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
        mockPopErrorScopeCallback = std::make_unique<MockPopErrorScopeCallback>();
    }

    void TearDown() override {
        mockFenceOnCompletionCallback = nullptr;
        mockPopErrorScopeCallback = nullptr;
        DawnTest::TearDown();
    }

    void WaitForCompletedValue(wgpu::Fence fence, uint64_t completedValue) {
        while (fence.GetCompletedValue() < completedValue) {
            WaitABit();
        }
    }
};

// Test that signaling a fence updates the completed value
TEST_P(FenceTests, SimpleSignal) {
    wgpu::FenceDescriptor descriptor;
    descriptor.initialValue = 1u;
    wgpu::Fence fence = queue.CreateFence(&descriptor);

    // Completed value starts at initial value
    EXPECT_EQ(fence.GetCompletedValue(), 1u);

    queue.Signal(fence, 2);
    WaitForCompletedValue(fence, 2);

    // Completed value updates to signaled value
    EXPECT_EQ(fence.GetCompletedValue(), 2u);
}

// Test callbacks are called in increasing order of fence completion value
TEST_P(FenceTests, OnCompletionOrdering) {
    wgpu::Fence fence = queue.CreateFence();

    queue.Signal(fence, 4);

    {
        testing::InSequence s;

        EXPECT_CALL(*mockFenceOnCompletionCallback,
                    Call(WGPUFenceCompletionStatus_Success, this + 0))
            .Times(1);

        EXPECT_CALL(*mockFenceOnCompletionCallback,
                    Call(WGPUFenceCompletionStatus_Success, this + 1))
            .Times(1);

        EXPECT_CALL(*mockFenceOnCompletionCallback,
                    Call(WGPUFenceCompletionStatus_Success, this + 2))
            .Times(1);

        EXPECT_CALL(*mockFenceOnCompletionCallback,
                    Call(WGPUFenceCompletionStatus_Success, this + 3))
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
    wgpu::Fence fence = queue.CreateFence();

    queue.Signal(fence, 2);
    queue.Signal(fence, 4);

    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, nullptr))
        .Times(1);
    fence.OnCompletion(3u, ToMockFenceOnCompletionCallback, nullptr);

    WaitForCompletedValue(fence, 4);
}

// Test callbacks still occur if Queue::Signal and fence::OnCompletion happens multiple times
TEST_P(FenceTests, SignalOnCompletionWait) {
    wgpu::Fence fence = queue.CreateFence();

    queue.Signal(fence, 2);
    queue.Signal(fence, 6);

    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, this + 2))
        .Times(1);
    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, this + 6))
        .Times(1);

    fence.OnCompletion(1u, ToMockFenceOnCompletionCallback, this + 2);
    fence.OnCompletion(5u, ToMockFenceOnCompletionCallback, this + 6);

    WaitForCompletedValue(fence, 6);
}

// Test callbacks still occur if Queue::Signal and fence::OnCompletion happens multiple times
TEST_P(FenceTests, SignalOnCompletionWaitStaggered) {
    wgpu::Fence fence = queue.CreateFence();

    queue.Signal(fence, 2);

    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, this + 2))
        .Times(1);
    fence.OnCompletion(1u, ToMockFenceOnCompletionCallback, this + 2);

    queue.Signal(fence, 4);

    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, this + 4))
        .Times(1);
    fence.OnCompletion(3u, ToMockFenceOnCompletionCallback, this + 4);

    WaitForCompletedValue(fence, 4);
}

// Test all callbacks are called if they are added for the same fence value
TEST_P(FenceTests, OnCompletionMultipleCallbacks) {
    wgpu::Fence fence = queue.CreateFence();

    queue.Signal(fence, 4);

    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, this + 0))
        .Times(1);

    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, this + 1))
        .Times(1);

    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, this + 2))
        .Times(1);

    EXPECT_CALL(*mockFenceOnCompletionCallback, Call(WGPUFenceCompletionStatus_Success, this + 3))
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
    wgpu::Fence fence = queue.CreateFence();

    // The fence in this block will be deleted when it goes out of scope
    {
        wgpu::Fence testFence = queue.CreateFence();

        queue.Signal(testFence, 4);

        EXPECT_CALL(*mockFenceOnCompletionCallback,
                    Call(WGPUFenceCompletionStatus_Unknown, this + 0))
            .Times(1);

        EXPECT_CALL(*mockFenceOnCompletionCallback,
                    Call(WGPUFenceCompletionStatus_Unknown, this + 1))
            .Times(1);

        EXPECT_CALL(*mockFenceOnCompletionCallback,
                    Call(WGPUFenceCompletionStatus_Unknown, this + 2))
            .Times(1);

        EXPECT_CALL(*mockFenceOnCompletionCallback,
                    Call(WGPUFenceCompletionStatus_Unknown, this + 3))
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

// Regression test that validation errors that are tracked client-side are captured
// in error scopes.
TEST_P(FenceTests, ClientValidationErrorInErrorScope) {
    wgpu::Fence fence = queue.CreateFence();

    queue.Signal(fence, 4);

    device.PushErrorScope(wgpu::ErrorFilter::Validation);
    queue.Signal(fence, 2);

    EXPECT_CALL(*mockPopErrorScopeCallback, Call(WGPUErrorType_Validation, _, this)).Times(1);
    device.PopErrorScope(ToMockPopErrorScopeCallback, this);

    WaitForCompletedValue(fence, 4);
}

DAWN_INSTANTIATE_TEST(FenceTests, D3D12Backend(), MetalBackend(), OpenGLBackend(), VulkanBackend());
