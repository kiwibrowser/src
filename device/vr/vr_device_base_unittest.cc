// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/vr_device.h"

#include <memory>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "device/vr/public/mojom/vr_service.mojom.h"
#include "device/vr/test/fake_vr_device.h"
#include "device/vr/test/fake_vr_service_client.h"
#include "device/vr/test/mock_vr_display_impl.h"
#include "device/vr/vr_device_base.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

namespace {

class VRDeviceBaseForTesting : public VRDeviceBase {
 public:
  VRDeviceBaseForTesting() = default;
  ~VRDeviceBaseForTesting() override = default;

  void SetVRDisplayInfoForTest(mojom::VRDisplayInfoPtr display_info) {
    SetVRDisplayInfo(std::move(display_info));
  }

  void FireDisplayActivate() {
    OnActivate(device::mojom::VRDisplayEventReason::MOUNTED, base::DoNothing());
  }

  bool ListeningForActivate() { return listening_for_activate; }

 private:
  void OnListeningForActivate(bool listening) override {
    listening_for_activate = listening;
  }

  bool listening_for_activate = false;

  DISALLOW_COPY_AND_ASSIGN(VRDeviceBaseForTesting);
};

class StubVRDeviceEventListener : public VRDeviceEventListener {
 public:
  ~StubVRDeviceEventListener() override {}

  MOCK_METHOD1(DoOnChanged, void(mojom::VRDisplayInfo* vr_device_info));
  void OnChanged(mojom::VRDisplayInfoPtr vr_device_info) override {
    DoOnChanged(vr_device_info.get());
  }

  MOCK_METHOD2(OnActivate,
               void(mojom::VRDisplayEventReason,
                    base::OnceCallback<void(bool)>));

  MOCK_METHOD0(OnExitPresent, void());
  MOCK_METHOD0(OnBlur, void());
  MOCK_METHOD0(OnFocus, void());
  MOCK_METHOD1(OnDeactivate, void(mojom::VRDisplayEventReason));
};

}  // namespace

class VRDeviceTest : public testing::Test {
 public:
  VRDeviceTest() {}
  ~VRDeviceTest() override {}

 protected:
  void SetUp() override {
    mojom::VRServiceClientPtr proxy;
    client_ = std::make_unique<FakeVRServiceClient>(mojo::MakeRequest(&proxy));
  }

  std::unique_ptr<MockVRDisplayImpl> MakeMockDisplay(VRDeviceBase* device) {
    mojom::VRDisplayClientPtr display_client;
    return std::make_unique<testing::NiceMock<MockVRDisplayImpl>>(
        device, client(), nullptr, nullptr, mojo::MakeRequest(&display_client),
        false);
  }

  std::unique_ptr<VRDeviceBaseForTesting> MakeVRDevice() {
    std::unique_ptr<VRDeviceBaseForTesting> device =
        std::make_unique<VRDeviceBaseForTesting>();
    device->SetVRDisplayInfoForTest(MakeVRDisplayInfo(device->GetId()));
    return device;
  }

  mojom::VRDisplayInfoPtr MakeVRDisplayInfo(unsigned int device_id) {
    mojom::VRDisplayInfoPtr display_info = mojom::VRDisplayInfo::New();
    display_info->index = device_id;
    return display_info;
  }

  FakeVRServiceClient* client() { return client_.get(); }

  std::unique_ptr<FakeVRServiceClient> client_;
  base::MessageLoop message_loop_;

  DISALLOW_COPY_AND_ASSIGN(VRDeviceTest);
};

// Tests VRDevice class default behaviour when it dispatches "vrdevicechanged"
// event. The expected behaviour is all of the services related with this device
// will receive the "vrdevicechanged" event.
TEST_F(VRDeviceTest, DeviceChangedDispatched) {
  auto device = MakeVRDevice();
  StubVRDeviceEventListener listener;
  device->SetVRDeviceEventListener(&listener);
  EXPECT_CALL(listener, DoOnChanged(testing::_)).Times(1);
  device->SetVRDisplayInfoForTest(MakeVRDisplayInfo(device->GetId()));
}

TEST_F(VRDeviceTest, DisplayActivateRegsitered) {
  device::mojom::VRDisplayEventReason mounted =
      device::mojom::VRDisplayEventReason::MOUNTED;
  auto device = MakeVRDevice();
  StubVRDeviceEventListener listener;
  device->SetVRDeviceEventListener(&listener);

  EXPECT_FALSE(device->ListeningForActivate());
  device->SetListeningForActivate(true);
  EXPECT_TRUE(device->ListeningForActivate());

  EXPECT_CALL(listener, OnActivate(mounted, testing::_)).Times(1);
  device->FireDisplayActivate();
}

TEST_F(VRDeviceTest, NoMagicWindowPosesWhileBrowsing) {
  auto device = std::make_unique<FakeVRDevice>();
  device->SetPose(mojom::VRPose::New());

  device->GetMagicWindowPose(
      base::BindOnce([](mojom::VRPosePtr pose) { EXPECT_TRUE(pose); }));
  device->SetMagicWindowEnabled(false);
  device->GetMagicWindowPose(
      base::BindOnce([](mojom::VRPosePtr pose) { EXPECT_FALSE(pose); }));
}

}  // namespace device
