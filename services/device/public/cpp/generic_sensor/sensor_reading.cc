// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/public/cpp/generic_sensor/sensor_reading.h"

namespace device {

SensorReadingRaw::SensorReadingRaw() = default;
SensorReadingRaw::~SensorReadingRaw() = default;

SensorReadingBase::SensorReadingBase() = default;
SensorReadingBase::~SensorReadingBase() = default;

SensorReadingSingle::SensorReadingSingle() = default;
SensorReadingSingle::~SensorReadingSingle() = default;

SensorReadingXYZ::SensorReadingXYZ() = default;
SensorReadingXYZ::~SensorReadingXYZ() = default;

SensorReadingQuat::SensorReadingQuat() = default;
SensorReadingQuat::~SensorReadingQuat() = default;

SensorReadingSharedBuffer::SensorReadingSharedBuffer() = default;
SensorReadingSharedBuffer::~SensorReadingSharedBuffer() = default;

SensorReading::SensorReading() {
  new (&raw) SensorReadingRaw();
}
SensorReading::SensorReading(const SensorReading& other) {
  raw = other.raw;
}
SensorReading::~SensorReading() {
  raw.~SensorReadingRaw();
}
SensorReading& SensorReading::operator=(const SensorReading& other) {
  raw = other.raw;
  return *this;
}

// static
uint64_t SensorReadingSharedBuffer::GetOffset(mojom::SensorType type) {
  return (static_cast<uint64_t>(mojom::SensorType::LAST) -
          static_cast<uint64_t>(type)) *
         sizeof(SensorReadingSharedBuffer);
}

}  // namespace device
