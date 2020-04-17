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

#include "dawn_wire/WireClient.h"
#include "dawn_wire/WireServer.h"
#include "utils/TerribleCommandBuffer.h"

using namespace testing;
using namespace dawn_wire;

WireTest::WireTest() {
}

WireTest::~WireTest() {
}

void WireTest::SetUp() {
    DawnProcTable mockProcs;
    DawnDevice mockDevice;
    api.GetProcTableAndDevice(&mockProcs, &mockDevice);

    // This SetCallback call cannot be ignored because it is done as soon as we start the server
    EXPECT_CALL(api, OnDeviceSetErrorCallback(_, _, _)).Times(Exactly(1));
    SetupIgnoredCallExpectations();

    mS2cBuf = std::make_unique<utils::TerribleCommandBuffer>();
    mC2sBuf = std::make_unique<utils::TerribleCommandBuffer>(mWireServer.get());

    mWireServer.reset(new WireServer(mockDevice, mockProcs, mS2cBuf.get()));
    mC2sBuf->SetHandler(mWireServer.get());

    mWireClient.reset(new WireClient(mC2sBuf.get()));
    mS2cBuf->SetHandler(mWireClient.get());

    device = mWireClient->GetDevice();
    DawnProcTable clientProcs = mWireClient->GetProcs();
    dawnSetProcs(&clientProcs);

    apiDevice = mockDevice;
}

void WireTest::TearDown() {
    dawnSetProcs(nullptr);

    // Derived classes should call the base TearDown() first. The client must
    // be reset before any mocks are deleted.
    // Incomplete client callbacks will be called on deletion, so the mocks
    // cannot be null.
    api.IgnoreAllReleaseCalls();
    mWireClient = nullptr;
}

void WireTest::FlushClient() {
    ASSERT_TRUE(mC2sBuf->Flush());

    Mock::VerifyAndClearExpectations(&api);
    SetupIgnoredCallExpectations();
}

void WireTest::FlushServer() {
    ASSERT_TRUE(mS2cBuf->Flush());
}

dawn_wire::WireServer* WireTest::GetWireServer() {
    return mWireServer.get();
}

dawn_wire::WireClient* WireTest::GetWireClient() {
    return mWireClient.get();
}

void WireTest::DeleteServer() {
    mWireServer = nullptr;
}

void WireTest::SetupIgnoredCallExpectations() {
    EXPECT_CALL(api, DeviceTick(_)).Times(AnyNumber());
}
