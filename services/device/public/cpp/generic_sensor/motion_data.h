// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_PUBLIC_CPP_GENERIC_SENSOR_MOTION_DATA_H_
#define SERVICES_DEVICE_PUBLIC_CPP_GENERIC_SENSOR_MOTION_DATA_H_

namespace device {

#pragma pack(push, 1)

class MotionData {
 public:
  MotionData();
  MotionData(const MotionData& other);
  ~MotionData() {}

  double acceleration_x;
  double acceleration_y;
  double acceleration_z;

  double acceleration_including_gravity_x;
  double acceleration_including_gravity_y;
  double acceleration_including_gravity_z;

  double rotation_rate_alpha;
  double rotation_rate_beta;
  double rotation_rate_gamma;

  double interval;

  bool has_acceleration_x : 1;
  bool has_acceleration_y : 1;
  bool has_acceleration_z : 1;

  bool has_acceleration_including_gravity_x : 1;
  bool has_acceleration_including_gravity_y : 1;
  bool has_acceleration_including_gravity_z : 1;

  bool has_rotation_rate_alpha : 1;
  bool has_rotation_rate_beta : 1;
  bool has_rotation_rate_gamma : 1;

  bool all_available_sensors_are_active : 1;
};

static_assert(sizeof(MotionData) == (10 * sizeof(double) + 2 * sizeof(char)),
              "MotionData has wrong size");

#pragma pack(pop)

}  // namespace device

#endif  // SERVICES_DEVICE_PUBLIC_CPP_GENERIC_SENSOR_MOTION_DATA_H_
