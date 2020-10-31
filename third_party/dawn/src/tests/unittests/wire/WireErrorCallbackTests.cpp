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
    class MockDeviceErrorCallback {
      public:
        MOCK_METHOD(void, Call, (WGPUErrorType type, const char* message, void* userdata));
    };

    std::unique_ptr<StrictMock<MockDeviceErrorCallback>> mockDeviceErrorCallback;
    void ToMockDeviceErrorCallback(WGPUErrorType type, const char* message, void* userdata) {
        mockDeviceErrorCallback->Call(type, message, userdata);
    }

    class MockDevicePopErrorScopeCallback {
      public:
        MOCK_METHOD(void, Call, (WGPUErrorType type, const char* message, void* userdata));
    };

    std::unique_ptr<StrictMock<MockDevicePopErrorScopeCallback>> mockDevicePopErrorScopeCallback;
    void ToMockDevicePopErrorScopeCallback(WGPUErrorType type,
                                           const char* message,
                                           void* userdata) {
        mockDevicePopErrorScopeCallback->Call(type, message, userdata);
    }

    class MockDeviceLostCallback {
      public:
        MOCK_METHOD(void, Call, (const char* message, void* userdata));
    };

    std::unique_ptr<StrictMock<MockDeviceLostCallback>> mockDeviceLostCallback;
    void ToMockDeviceLostCallback(const char* message, void* userdata) {
        mockDeviceLostCallback->Call(message, userdata);
    }

}  // anonymous namespace

class WireErrorCallbackTests : public WireTest {
  public:
    WireErrorCallbackTests() {
    }
    ~WireErrorCallbackTests() override = default;

    void SetUp() override {
        WireTest::SetUp();

        mockDeviceErrorCallback = std::make_unique<StrictMock<MockDeviceErrorCallback>>();
        mockDevicePopErrorScopeCallback =
            std::make_unique<StrictMock<MockDevicePopErrorScopeCallback>>();
        mockDeviceLostCallback = std::make_unique<StrictMock<MockDeviceLostCallback>>();
    }

    void TearDown() override {
        WireTest::TearDown();

        mockDeviceErrorCallback = nullptr;
        mockDevicePopErrorScopeCallback = nullptr;
        mockDeviceLostCallback = nullptr;
    }

    void FlushServer() {
        WireTest::FlushServer();

        Mock::VerifyAndClearExpectations(&mockDeviceErrorCallback);
        Mock::VerifyAndClearExpectations(&mockDevicePopErrorScopeCallback);
    }
};

// Test the return wire for device error callbacks
TEST_F(WireErrorCallbackTests, DeviceErrorCallback) {
    wgpuDeviceSetUncapturedErrorCallback(device, ToMockDeviceErrorCallback, this);

    // Setting the error callback should stay on the client side and do nothing
    FlushClient();

    // Calling the callback on the server side will result in the callback being called on the
    // client side
    api.CallDeviceErrorCallback(apiDevice, WGPUErrorType_Validation, "Some error message");

    EXPECT_CALL(*mockDeviceErrorCallback,
                Call(WGPUErrorType_Validation, StrEq("Some error message"), this))
        .Times(1);

    FlushServer();
}

// Test the return wire for error scopes.
TEST_F(WireErrorCallbackTests, PushPopErrorScopeCallback) {
    wgpuDevicePushErrorScope(device, WGPUErrorFilter_Validation);
    EXPECT_CALL(api, DevicePushErrorScope(apiDevice, WGPUErrorFilter_Validation)).Times(1);

    FlushClient();

    wgpuDevicePopErrorScope(device, ToMockDevicePopErrorScopeCallback, this);

    WGPUErrorCallback callback;
    void* userdata;
    EXPECT_CALL(api, OnDevicePopErrorScopeCallback(apiDevice, _, _))
        .WillOnce(DoAll(SaveArg<1>(&callback), SaveArg<2>(&userdata), Return(true)));

    FlushClient();

    callback(WGPUErrorType_Validation, "Some error message", userdata);
    EXPECT_CALL(*mockDevicePopErrorScopeCallback,
                Call(WGPUErrorType_Validation, StrEq("Some error message"), this))
        .Times(1);

    FlushServer();
}

// Test the return wire for error scopes when callbacks return in a various orders.
TEST_F(WireErrorCallbackTests, PopErrorScopeCallbackOrdering) {
    // Two error scopes are popped, and the first one returns first.
    {
        wgpuDevicePushErrorScope(device, WGPUErrorFilter_Validation);
        wgpuDevicePushErrorScope(device, WGPUErrorFilter_Validation);
        EXPECT_CALL(api, DevicePushErrorScope(apiDevice, WGPUErrorFilter_Validation)).Times(2);

        FlushClient();

        wgpuDevicePopErrorScope(device, ToMockDevicePopErrorScopeCallback, this);
        wgpuDevicePopErrorScope(device, ToMockDevicePopErrorScopeCallback, this + 1);

        WGPUErrorCallback callback1;
        WGPUErrorCallback callback2;
        void* userdata1;
        void* userdata2;
        EXPECT_CALL(api, OnDevicePopErrorScopeCallback(apiDevice, _, _))
            .WillOnce(DoAll(SaveArg<1>(&callback1), SaveArg<2>(&userdata1), Return(true)))
            .WillOnce(DoAll(SaveArg<1>(&callback2), SaveArg<2>(&userdata2), Return(true)));

        FlushClient();

        callback1(WGPUErrorType_Validation, "First error message", userdata1);
        EXPECT_CALL(*mockDevicePopErrorScopeCallback,
                    Call(WGPUErrorType_Validation, StrEq("First error message"), this))
            .Times(1);
        FlushServer();

        callback2(WGPUErrorType_Validation, "Second error message", userdata2);
        EXPECT_CALL(*mockDevicePopErrorScopeCallback,
                    Call(WGPUErrorType_Validation, StrEq("Second error message"), this + 1))
            .Times(1);
        FlushServer();
    }

    // Two error scopes are popped, and the second one returns first.
    {
        wgpuDevicePushErrorScope(device, WGPUErrorFilter_Validation);
        wgpuDevicePushErrorScope(device, WGPUErrorFilter_Validation);
        EXPECT_CALL(api, DevicePushErrorScope(apiDevice, WGPUErrorFilter_Validation)).Times(2);

        FlushClient();

        wgpuDevicePopErrorScope(device, ToMockDevicePopErrorScopeCallback, this);
        wgpuDevicePopErrorScope(device, ToMockDevicePopErrorScopeCallback, this + 1);

        WGPUErrorCallback callback1;
        WGPUErrorCallback callback2;
        void* userdata1;
        void* userdata2;
        EXPECT_CALL(api, OnDevicePopErrorScopeCallback(apiDevice, _, _))
            .WillOnce(DoAll(SaveArg<1>(&callback1), SaveArg<2>(&userdata1), Return(true)))
            .WillOnce(DoAll(SaveArg<1>(&callback2), SaveArg<2>(&userdata2), Return(true)));

        FlushClient();

        callback2(WGPUErrorType_Validation, "Second error message", userdata2);
        EXPECT_CALL(*mockDevicePopErrorScopeCallback,
                    Call(WGPUErrorType_Validation, StrEq("Second error message"), this + 1))
            .Times(1);
        FlushServer();

        callback1(WGPUErrorType_Validation, "First error message", userdata1);
        EXPECT_CALL(*mockDevicePopErrorScopeCallback,
                    Call(WGPUErrorType_Validation, StrEq("First error message"), this))
            .Times(1);
        FlushServer();
    }
}

// Test the return wire for error scopes in flight when the device is destroyed.
TEST_F(WireErrorCallbackTests, PopErrorScopeDeviceDestroyed) {
    wgpuDevicePushErrorScope(device, WGPUErrorFilter_Validation);
    EXPECT_CALL(api, DevicePushErrorScope(apiDevice, WGPUErrorFilter_Validation)).Times(1);

    FlushClient();

    EXPECT_TRUE(wgpuDevicePopErrorScope(device, ToMockDevicePopErrorScopeCallback, this));

    EXPECT_CALL(api, OnDevicePopErrorScopeCallback(apiDevice, _, _)).WillOnce(Return(true));
    FlushClient();

    // Incomplete callback called in Device destructor.
    EXPECT_CALL(*mockDevicePopErrorScopeCallback,
                Call(WGPUErrorType_Unknown, ValidStringMessage(), this))
        .Times(1);
}

// Test that PopErrorScope returns false if there are no error scopes.
TEST_F(WireErrorCallbackTests, PopErrorScopeEmptyStack) {
    // Empty stack
    { EXPECT_FALSE(wgpuDevicePopErrorScope(device, ToMockDevicePopErrorScopeCallback, this)); }

    // Pop too many times
    {
        wgpuDevicePushErrorScope(device, WGPUErrorFilter_Validation);
        EXPECT_CALL(api, DevicePushErrorScope(apiDevice, WGPUErrorFilter_Validation)).Times(1);

        EXPECT_TRUE(wgpuDevicePopErrorScope(device, ToMockDevicePopErrorScopeCallback, this));
        EXPECT_FALSE(wgpuDevicePopErrorScope(device, ToMockDevicePopErrorScopeCallback, this + 1));

        WGPUErrorCallback callback;
        void* userdata;
        EXPECT_CALL(api, OnDevicePopErrorScopeCallback(apiDevice, _, _))
            .WillOnce(DoAll(SaveArg<1>(&callback), SaveArg<2>(&userdata), Return(true)));

        FlushClient();

        callback(WGPUErrorType_Validation, "Some error message", userdata);
        EXPECT_CALL(*mockDevicePopErrorScopeCallback,
                    Call(WGPUErrorType_Validation, StrEq("Some error message"), this))
            .Times(1);

        FlushServer();
    }
}

// Test the return wire for device lost callback
TEST_F(WireErrorCallbackTests, DeviceLostCallback) {
    wgpuDeviceSetDeviceLostCallback(device, ToMockDeviceLostCallback, this);

    // Setting the error callback should stay on the client side and do nothing
    FlushClient();

    // Calling the callback on the server side will result in the callback being called on the
    // client side
    api.CallDeviceLostCallback(apiDevice, "Some error message");

    EXPECT_CALL(*mockDeviceLostCallback, Call(StrEq("Some error message"), this)).Times(1);

    FlushServer();
}
