// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/accelerometer/accelerometer_types.h"

#include "ui/gfx/geometry/vector3d_f.h"

namespace chromeos {
namespace {

// The maximum deviation from the acceleration expected due to gravity for which
// the device will be considered stable: 1g.
constexpr float kDeviationFromGravityThreshold = 1.0f;

// The mean acceleration due to gravity on Earth in m/s^2.
constexpr float kMeanGravity = 9.80665f;

}  // namespace

AccelerometerReading::AccelerometerReading() : present(false) {
}

AccelerometerReading::~AccelerometerReading() = default;

AccelerometerUpdate::AccelerometerUpdate() = default;

AccelerometerUpdate::~AccelerometerUpdate() = default;

gfx::Vector3dF AccelerometerUpdate::GetVector(
    AccelerometerSource source) const {
  const AccelerometerReading& reading = data_[source];
  return gfx::Vector3dF(reading.x, reading.y, reading.z);
}

bool AccelerometerUpdate::IsReadingStable(AccelerometerSource source) const {
  if (!has(source))
    return false;

  return std::abs(GetVector(source).Length() - kMeanGravity) <=
         kDeviationFromGravityThreshold;
}

}  // namespace chromeos
