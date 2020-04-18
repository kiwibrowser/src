// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SENSOR_RELATIVE_ORIENTATION_SENSOR_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SENSOR_RELATIVE_ORIENTATION_SENSOR_H_

#include "third_party/blink/renderer/modules/sensor/orientation_sensor.h"
#include "third_party/blink/renderer/modules/sensor/spatial_sensor_options.h"

namespace blink {

class RelativeOrientationSensor final : public OrientationSensor {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static RelativeOrientationSensor* Create(ExecutionContext*,
                                           const SpatialSensorOptions&,
                                           ExceptionState&);
  static RelativeOrientationSensor* Create(ExecutionContext*, ExceptionState&);

  void Trace(blink::Visitor*) override;

 private:
  RelativeOrientationSensor(ExecutionContext*,
                            const SpatialSensorOptions&,
                            ExceptionState&);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SENSOR_RELATIVE_ORIENTATION_SENSOR_H_
