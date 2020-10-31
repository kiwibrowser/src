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

#include "dawn/dawn_proc.h"
#include "dawn_wire/WireClient.h"
#include "dawn_wire/WireServer.h"
#include "utils/TerribleCommandBuffer.h"

using namespace testing;
using namespace dawn_wire;

WireTest::WireTest() {
}

WireTest::~WireTest() {
}

client::MemoryTransferService* WireTest::GetClientMemoryTransferService() {
    return nullptr;
}

server::MemoryTransferService* WireTest::GetServerMemoryTransferService() {
    return nullptr;
}

void WireTest::SetUp() {
    DawnProcTable mockProcs;
    WGPUDevice mockDevice;
    api.GetProcTableAndDevice(&mockProcs, &mockDevice);

    // This SetCallback call cannot be ignored because it is done as soon as we start the server
    EXPECT_CALL(api, OnDeviceSetUncapturedErrorCallback(_, _, _)).Times(Exactly(1));
    EXPECT_CALL(api, OnDeviceSetDeviceLostCallback(_, _, _)).Times(Exactly(1));
    SetupIgnoredCallExpectations();

    mS2cBuf = std::make_unique<utils::TerribleCommandBuffer>();
    mC2sBuf = std::make_unique<utils::TerribleCommandBuffer>(mWireServer.get());

    WireServerDescriptor serverDesc = {};
    serverDesc.device = mockDevice;
    serverDesc.procs = &mockProcs;
    serverDesc.serializer = mS2cBuf.get();
    serverDesc.memoryTransferService = GetServerMemoryTransferService();

    mWireServer.reset(new WireServer(serverDesc));
    mC2sBuf->SetHandler(mWireServer.get());

    WireClientDescriptor clientDesc = {};
    clientDesc.serializer = mC2sBuf.get();
    clientDesc.memoryTransferService = GetClientMemoryTransferService();

    mWireClient.reset(new WireClient(clientDesc));
    mS2cBuf->SetHandler(mWireClient.get());

    device = mWireClient->GetDevice();
    DawnProcTable clientProcs = dawn_wire::WireClient::GetProcs();
    dawnProcSetProcs(&clientProcs);

    apiDevice = mockDevice;

    // The GetDefaultQueue is done on WireClient startup so we expect it now.
    queue = wgpuDeviceGetDefaultQueue(device);
    apiQueue = api.GetNewQueue();
    EXPECT_CALL(api, DeviceGetDefaultQueue(apiDevice)).WillOnce(Return(apiQueue));
    FlushClient();
}

void WireTest::TearDown() {
    dawnProcSetProcs(nullptr);

    // Derived classes should call the base TearDown() first. The client must
    // be reset before any mocks are deleted.
    // Incomplete client callbacks will be called on deletion, so the mocks
    // cannot be null.
    api.IgnoreAllReleaseCalls();
    mWireClient = nullptr;
    mWireServer = nullptr;
}

void WireTest::FlushClient(bool success) {
    ASSERT_EQ(mC2sBuf->Flush(), success);

    Mock::VerifyAndClearExpectations(&api);
    SetupIgnoredCallExpectations();
}

void WireTest::FlushServer(bool success) {
    ASSERT_EQ(mS2cBuf->Flush(), success);
}

dawn_wire::WireServer* WireTest::GetWireServer() {
    return mWireServer.get();
}

dawn_wire::WireClient* WireTest::GetWireClient() {
    return mWireClient.get();
}

void WireTest::DeleteServer() {
    EXPECT_CALL(api, QueueRelease(apiQueue)).Times(1);
    mWireServer = nullptr;
}

void WireTest::SetupIgnoredCallExpectations() {
    EXPECT_CALL(api, DeviceTick(_)).Times(AnyNumber());
}
