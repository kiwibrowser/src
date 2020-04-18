// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/test/fake_vr_display_impl_client.h"
#include "device/vr/test/fake_vr_service_client.h"

namespace device {

FakeVRDisplayImplClient::FakeVRDisplayImplClient(
    mojom::VRDisplayClientRequest request)
    : m_binding_(this, std::move(request)) {}

FakeVRDisplayImplClient::~FakeVRDisplayImplClient() {}

void FakeVRDisplayImplClient::SetServiceClient(
    FakeVRServiceClient* service_client) {
  service_client_ = service_client;
}

void FakeVRDisplayImplClient::OnChanged(mojom::VRDisplayInfoPtr display) {
  service_client_->SetLastDeviceId(display->index);
}

}  // namespace device
