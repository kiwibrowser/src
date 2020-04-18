// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/test/fake_vr_service_client.h"
#include "device/vr/test/fake_vr_display_impl_client.h"

namespace device {

FakeVRServiceClient::FakeVRServiceClient(mojom::VRServiceClientRequest request)
    : m_binding_(this, std::move(request)) {}

FakeVRServiceClient::~FakeVRServiceClient() {}

void FakeVRServiceClient::OnDisplayConnected(
    mojom::VRMagicWindowProviderPtr magic_window_provider,
    mojom::VRDisplayHostPtr display,
    mojom::VRDisplayClientRequest request,
    mojom::VRDisplayInfoPtr displayInfo) {
  displays_.push_back(std::move(displayInfo));
  auto* display_client = new FakeVRDisplayImplClient(std::move(request));
  display_client->SetServiceClient(this);

  display_clients_.push_back(display_client);
}

void FakeVRServiceClient::SetLastDeviceId(unsigned int id) {
  last_device_id_ = id;
}

bool FakeVRServiceClient::CheckDeviceId(unsigned int id) {
  return id == last_device_id_;
}

}  // namespace device
