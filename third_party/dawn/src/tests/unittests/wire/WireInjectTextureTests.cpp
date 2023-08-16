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

using namespace testing;
using namespace dawn_wire;

class WireInjectTextureTests : public WireTest {
  public:
    WireInjectTextureTests() {
    }
    ~WireInjectTextureTests() override = default;
};

// Test that reserving and injecting a texture makes calls on the client object forward to the
// server object correctly.
TEST_F(WireInjectTextureTests, CallAfterReserveInject) {
    ReservedTexture reservation = GetWireClient()->ReserveTexture(device);

    WGPUTexture apiTexture = api.GetNewTexture();
    EXPECT_CALL(api, TextureReference(apiTexture));
    ASSERT_TRUE(GetWireServer()->InjectTexture(apiTexture, reservation.id, reservation.generation));

    wgpuTextureCreateView(reservation.texture, nullptr);
    WGPUTextureView apiDummyView = api.GetNewTextureView();
    EXPECT_CALL(api, TextureCreateView(apiTexture, nullptr)).WillOnce(Return(apiDummyView));
    FlushClient();
}

// Test that reserve correctly returns different IDs each time.
TEST_F(WireInjectTextureTests, ReserveDifferentIDs) {
    ReservedTexture reservation1 = GetWireClient()->ReserveTexture(device);
    ReservedTexture reservation2 = GetWireClient()->ReserveTexture(device);

    ASSERT_NE(reservation1.id, reservation2.id);
    ASSERT_NE(reservation1.texture, reservation2.texture);
}

// Test that injecting the same id without a destroy first fails.
TEST_F(WireInjectTextureTests, InjectExistingID) {
    ReservedTexture reservation = GetWireClient()->ReserveTexture(device);

    WGPUTexture apiTexture = api.GetNewTexture();
    EXPECT_CALL(api, TextureReference(apiTexture));
    ASSERT_TRUE(GetWireServer()->InjectTexture(apiTexture, reservation.id, reservation.generation));

    // ID already in use, call fails.
    ASSERT_FALSE(
        GetWireServer()->InjectTexture(apiTexture, reservation.id, reservation.generation));
}

// Test that the server only borrows the texture and does a single reference-release
TEST_F(WireInjectTextureTests, InjectedTextureLifetime) {
    ReservedTexture reservation = GetWireClient()->ReserveTexture(device);

    // Injecting the texture adds a reference
    WGPUTexture apiTexture = api.GetNewTexture();
    EXPECT_CALL(api, TextureReference(apiTexture));
    ASSERT_TRUE(GetWireServer()->InjectTexture(apiTexture, reservation.id, reservation.generation));

    // Releasing the texture removes a single reference.
    wgpuTextureRelease(reservation.texture);
    EXPECT_CALL(api, TextureRelease(apiTexture));
    FlushClient();

    // Deleting the server doesn't release a second reference.
    DeleteServer();
    Mock::VerifyAndClearExpectations(&api);
}
