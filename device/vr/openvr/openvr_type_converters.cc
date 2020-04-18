// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/openvr/openvr_type_converters.h"

#include <math.h>
#include <iterator>
#include <vector>

#include "device/vr/public/mojom/vr_service.mojom.h"
#include "third_party/openvr/src/headers/openvr.h"

namespace mojo {

device::mojom::VRPosePtr
TypeConverter<device::mojom::VRPosePtr, vr::TrackedDevicePose_t>::Convert(
    const vr::TrackedDevicePose_t& hmd_pose) {
  device::mojom::VRPosePtr pose = device::mojom::VRPose::New();
  pose->orientation = std::vector<float>({0.0f, 0.0f, 0.0f, 1.0f});
  pose->position = std::vector<float>({0.0f, 0.0f, 0.0f});

  if (hmd_pose.bPoseIsValid && hmd_pose.bDeviceIsConnected) {
    const float(&m)[3][4] = hmd_pose.mDeviceToAbsoluteTracking.m;
    float w = sqrt(1 + m[0][0] + m[1][1] + m[2][2]) / 2;
    pose->orientation.value()[0] = (m[2][1] - m[1][2]) / (4 * w);
    pose->orientation.value()[1] = (m[0][2] - m[2][0]) / (4 * w);
    pose->orientation.value()[2] = (m[1][0] - m[0][1]) / (4 * w);
    pose->orientation.value()[3] = w;

    pose->position.value()[0] = m[0][3];
    pose->position.value()[1] = m[1][3];
    pose->position.value()[2] = m[2][3];

    pose->linearVelocity = std::vector<float>(std::begin(hmd_pose.vVelocity.v),
                                              std::end(hmd_pose.vVelocity.v));
    pose->angularVelocity =
        std::vector<float>(std::begin(hmd_pose.vAngularVelocity.v),
                           std::end(hmd_pose.vAngularVelocity.v));
  }

  return pose;
}

}  // namespace mojo
