// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef DEVICE_VR_OPENVR_TYPE_CONVERTERS_H
#define DEVICE_VR_OPENVR_TYPE_CONVERTERS_H

#include "device/vr/public/mojom/vr_service.mojom.h"
#include "third_party/openvr/src/headers/openvr.h"

namespace mojo {

template <>
struct TypeConverter<device::mojom::VRPosePtr, vr::TrackedDevicePose_t> {
  static device::mojom::VRPosePtr Convert(
      const vr::TrackedDevicePose_t& hmd_pose);
};

}  // namespace mojo

#endif  // DEVICE_VR_OPENVR_TYPE_CONVERTERS_H
