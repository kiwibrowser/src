// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_TEST_MOCK_VR_DISPLAY_IMPL_H
#define DEVICE_VR_TEST_MOCK_VR_DISPLAY_IMPL_H

#include "base/macros.h"
#include "device/vr/vr_display_impl.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace device {

class MockVRDisplayImpl : public VRDisplayImpl {
 public:
  MockVRDisplayImpl(device::VRDevice* device,
                    mojom::VRServiceClient* service_client,
                    mojom::VRDisplayInfoPtr display_info,
                    mojom::VRDisplayHostPtr display_host,
                    mojom::VRDisplayClientRequest request,
                    bool in_frame_focused);
  ~MockVRDisplayImpl() override;

  MOCK_METHOD1(DoOnChanged, void(mojom::VRDisplayInfo* vr_device_info));
  void OnChanged(mojom::VRDisplayInfoPtr vr_device_info) {
    DoOnChanged(vr_device_info.get());
  }

  MOCK_METHOD2(OnActivate,
               void(mojom::VRDisplayEventReason, base::Callback<void(bool)>));

  MOCK_METHOD0(ListeningForActivate, bool());
  MOCK_METHOD0(InFocusedFrame, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockVRDisplayImpl);
};

}  // namespace device

#endif  // DEVICE_VR_TEST_MOCK_VR_DISPLAY_IMPL_H
