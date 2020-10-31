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

#include "tests/unittests/validation/ValidationTest.h"

#include <gmock/gmock.h>

using namespace testing;

class MockDevicePopErrorScopeCallback {
  public:
    MOCK_METHOD(void, Call, (WGPUErrorType type, const char* message, void* userdata));
};

static std::unique_ptr<MockDevicePopErrorScopeCallback> mockDevicePopErrorScopeCallback;
static void ToMockDevicePopErrorScopeCallback(WGPUErrorType type,
                                              const char* message,
                                              void* userdata) {
    mockDevicePopErrorScopeCallback->Call(type, message, userdata);
}

class ErrorScopeValidationTest : public ValidationTest {
  private:
    void SetUp() override {
        ValidationTest::SetUp();
        mockDevicePopErrorScopeCallback = std::make_unique<MockDevicePopErrorScopeCallback>();
    }

    void TearDown() override {
        // Delete mocks so that expectations are checked
        mockDevicePopErrorScopeCallback = nullptr;
        ValidationTest::TearDown();
    }
};

// Test the simple success case.
TEST_F(ErrorScopeValidationTest, Success) {
    device.PushErrorScope(wgpu::ErrorFilter::Validation);

    EXPECT_CALL(*mockDevicePopErrorScopeCallback, Call(WGPUErrorType_NoError, _, this)).Times(1);
    device.PopErrorScope(ToMockDevicePopErrorScopeCallback, this);
}

// Test the simple case where the error scope catches an error.
TEST_F(ErrorScopeValidationTest, CatchesError) {
    device.PushErrorScope(wgpu::ErrorFilter::Validation);

    wgpu::BufferDescriptor desc = {};
    desc.usage = static_cast<wgpu::BufferUsage>(WGPUBufferUsage_Force32);
    device.CreateBuffer(&desc);

    EXPECT_CALL(*mockDevicePopErrorScopeCallback, Call(WGPUErrorType_Validation, _, this)).Times(1);
    device.PopErrorScope(ToMockDevicePopErrorScopeCallback, this);
}

// Test that errors bubble to the parent scope if not handled by the current scope.
TEST_F(ErrorScopeValidationTest, ErrorBubbles) {
    device.PushErrorScope(wgpu::ErrorFilter::Validation);
    device.PushErrorScope(wgpu::ErrorFilter::OutOfMemory);

    wgpu::BufferDescriptor desc = {};
    desc.usage = static_cast<wgpu::BufferUsage>(WGPUBufferUsage_Force32);
    device.CreateBuffer(&desc);

    // OutOfMemory does not match Validation error.
    EXPECT_CALL(*mockDevicePopErrorScopeCallback, Call(WGPUErrorType_NoError, _, this)).Times(1);
    device.PopErrorScope(ToMockDevicePopErrorScopeCallback, this);

    // Parent validation error scope captures the error.
    EXPECT_CALL(*mockDevicePopErrorScopeCallback, Call(WGPUErrorType_Validation, _, this + 1))
        .Times(1);
    device.PopErrorScope(ToMockDevicePopErrorScopeCallback, this + 1);
}

// Test that if an error scope matches an error, it does not bubble to the parent scope.
TEST_F(ErrorScopeValidationTest, HandledErrorsStopBubbling) {
    device.PushErrorScope(wgpu::ErrorFilter::OutOfMemory);
    device.PushErrorScope(wgpu::ErrorFilter::Validation);

    wgpu::BufferDescriptor desc = {};
    desc.usage = static_cast<wgpu::BufferUsage>(WGPUBufferUsage_Force32);
    device.CreateBuffer(&desc);

    // Inner scope catches the error.
    EXPECT_CALL(*mockDevicePopErrorScopeCallback, Call(WGPUErrorType_Validation, _, this)).Times(1);
    device.PopErrorScope(ToMockDevicePopErrorScopeCallback, this);

    // Parent scope does not see the error.
    EXPECT_CALL(*mockDevicePopErrorScopeCallback, Call(WGPUErrorType_NoError, _, this + 1))
        .Times(1);
    device.PopErrorScope(ToMockDevicePopErrorScopeCallback, this + 1);
}

// Test that if no error scope handles an error, it goes to the device UncapturedError callback
TEST_F(ErrorScopeValidationTest, UnhandledErrorsMatchUncapturedErrorCallback) {
    device.PushErrorScope(wgpu::ErrorFilter::OutOfMemory);

    wgpu::BufferDescriptor desc = {};
    desc.usage = static_cast<wgpu::BufferUsage>(WGPUBufferUsage_Force32);
    ASSERT_DEVICE_ERROR(device.CreateBuffer(&desc));

    EXPECT_CALL(*mockDevicePopErrorScopeCallback, Call(WGPUErrorType_NoError, _, this)).Times(1);
    device.PopErrorScope(ToMockDevicePopErrorScopeCallback, this);
}

// Check that push/popping error scopes must be balanced.
TEST_F(ErrorScopeValidationTest, PushPopBalanced) {
    // No error scopes to pop.
    { EXPECT_FALSE(device.PopErrorScope(ToMockDevicePopErrorScopeCallback, this)); }

    // Too many pops
    {
        device.PushErrorScope(wgpu::ErrorFilter::Validation);

        EXPECT_CALL(*mockDevicePopErrorScopeCallback, Call(WGPUErrorType_NoError, _, this + 1))
            .Times(1);
        device.PopErrorScope(ToMockDevicePopErrorScopeCallback, this + 1);

        EXPECT_FALSE(device.PopErrorScope(ToMockDevicePopErrorScopeCallback, this + 2));
    }
}

// Test that error scopes do not call their callbacks until after an enclosed Queue::Submit
// completes
TEST_F(ErrorScopeValidationTest, CallbackAfterQueueSubmit) {
    wgpu::Queue queue = device.GetDefaultQueue();

    device.PushErrorScope(wgpu::ErrorFilter::OutOfMemory);
    queue.Submit(0, nullptr);
    device.PopErrorScope(ToMockDevicePopErrorScopeCallback, this);

    EXPECT_CALL(*mockDevicePopErrorScopeCallback, Call(WGPUErrorType_NoError, _, this)).Times(1);

    // Side effects of Queue::Submit only are seen after Tick()
    device.Tick();
}

// Test that parent error scopes do not call their callbacks until after an enclosed Queue::Submit
// completes
TEST_F(ErrorScopeValidationTest, CallbackAfterQueueSubmitNested) {
    wgpu::Queue queue = device.GetDefaultQueue();

    device.PushErrorScope(wgpu::ErrorFilter::OutOfMemory);
    device.PushErrorScope(wgpu::ErrorFilter::OutOfMemory);
    queue.Submit(0, nullptr);
    device.PopErrorScope(ToMockDevicePopErrorScopeCallback, this);
    device.PopErrorScope(ToMockDevicePopErrorScopeCallback, this + 1);

    EXPECT_CALL(*mockDevicePopErrorScopeCallback, Call(WGPUErrorType_NoError, _, this)).Times(1);
    EXPECT_CALL(*mockDevicePopErrorScopeCallback, Call(WGPUErrorType_NoError, _, this + 1))
        .Times(1);

    // Side effects of Queue::Submit only are seen after Tick()
    device.Tick();
}

// Test a callback that returns asynchronously followed by a synchronous one
TEST_F(ErrorScopeValidationTest, AsynchronousThenSynchronous) {
    wgpu::Queue queue = device.GetDefaultQueue();

    device.PushErrorScope(wgpu::ErrorFilter::OutOfMemory);
    queue.Submit(0, nullptr);
    device.PopErrorScope(ToMockDevicePopErrorScopeCallback, this);

    EXPECT_CALL(*mockDevicePopErrorScopeCallback, Call(WGPUErrorType_NoError, _, this + 1))
        .Times(1);
    device.PushErrorScope(wgpu::ErrorFilter::OutOfMemory);
    device.PopErrorScope(ToMockDevicePopErrorScopeCallback, this + 1);

    EXPECT_CALL(*mockDevicePopErrorScopeCallback, Call(WGPUErrorType_NoError, _, this)).Times(1);

    // Side effects of Queue::Submit only are seen after Tick()
    device.Tick();
}

// Test that if the device is destroyed before the callback occurs, it is called with NoError
// because all previous operations are waited upon before the destruction returns.
TEST_F(ErrorScopeValidationTest, DeviceDestroyedBeforeCallback) {
    wgpu::Queue queue = device.GetDefaultQueue();

    device.PushErrorScope(wgpu::ErrorFilter::OutOfMemory);
    queue.Submit(0, nullptr);
    device.PopErrorScope(ToMockDevicePopErrorScopeCallback, this);

    EXPECT_CALL(*mockDevicePopErrorScopeCallback, Call(WGPUErrorType_NoError, _, this)).Times(1);
    device = nullptr;
}

// Regression test that on device shutdown, we don't get a recursion in O(pushed error scope) that
// would lead to a stack overflow
TEST_F(ErrorScopeValidationTest, ShutdownStackOverflow) {
    for (size_t i = 0; i < 1'000'000; i++) {
        device.PushErrorScope(wgpu::ErrorFilter::Validation);
    }
}
