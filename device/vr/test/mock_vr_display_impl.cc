// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/test/mock_vr_display_impl.h"

namespace device {

MockVRDisplayImpl::MockVRDisplayImpl(device::VRDevice* device,
                                     mojom::VRServiceClient* service_client,
                                     mojom::VRDisplayInfoPtr display_info,
                                     mojom::VRDisplayHostPtr display_host,
                                     mojom::VRDisplayClientRequest request,
                                     bool in_frame_focused)
    : VRDisplayImpl(device,
                    std::move(service_client),
                    std::move(display_info),
                    std::move(display_host),
                    std::move(request),
                    in_frame_focused) {}

MockVRDisplayImpl::~MockVRDisplayImpl() = default;

}  // namespace device
