// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SENSOR_GYROSCOPE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SENSOR_GYROSCOPE_H_

#include "third_party/blink/renderer/modules/sensor/sensor.h"
#include "third_party/blink/renderer/modules/sensor/spatial_sensor_options.h"

namespace blink {

class Gyroscope final : public Sensor {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static Gyroscope* Create(ExecutionContext*,
                           const SpatialSensorOptions&,
                           ExceptionState&);
  static Gyroscope* Create(ExecutionContext*, ExceptionState&);

  double x(bool& is_null) const;
  double y(bool& is_null) const;
  double z(bool& is_null) const;

  void Trace(blink::Visitor*) override;

 private:
  Gyroscope(ExecutionContext*, const SpatialSensorOptions&, ExceptionState&);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SENSOR_GYROSCOPE_H_
