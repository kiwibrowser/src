// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_PUBLIC_CPP_GENERIC_SENSOR_ORIENTATION_DATA_H_
#define SERVICES_DEVICE_PUBLIC_CPP_GENERIC_SENSOR_ORIENTATION_DATA_H_

namespace device {

#pragma pack(push, 1)

class OrientationData {
 public:
  OrientationData();
  ~OrientationData() {}

  double alpha;
  double beta;
  double gamma;

  bool has_alpha : 1;
  bool has_beta : 1;
  bool has_gamma : 1;

  bool absolute : 1;

  bool all_available_sensors_are_active : 1;
};

static_assert(sizeof(OrientationData) ==
                  (3 * sizeof(double) + 1 * sizeof(char)),
              "OrientationData has wrong size");

#pragma pack(pop)

}  // namespace device

#endif  // SERVICES_DEVICE_PUBLIC_CPP_GENERIC_SENSOR_ORIENTATION_DATA_H_
