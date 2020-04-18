// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/sensor/ambient_light_sensor.h"

#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"

using device::mojom::blink::SensorType;

namespace blink {

// static
AmbientLightSensor* AmbientLightSensor::Create(
    ExecutionContext* execution_context,
    const SensorOptions& options,
    ExceptionState& exception_state) {
  return new AmbientLightSensor(execution_context, options, exception_state);
}

// static
AmbientLightSensor* AmbientLightSensor::Create(
    ExecutionContext* execution_context,
    ExceptionState& exception_state) {
  return Create(execution_context, SensorOptions(), exception_state);
}

AmbientLightSensor::AmbientLightSensor(ExecutionContext* execution_context,
                                       const SensorOptions& options,
                                       ExceptionState& exception_state)
    : Sensor(execution_context,
             options,
             exception_state,
             SensorType::AMBIENT_LIGHT,
             {mojom::FeaturePolicyFeature::kAmbientLightSensor}) {}

double AmbientLightSensor::illuminance(bool& is_null) const {
  INIT_IS_NULL_AND_RETURN(is_null, 0.0);
  return GetReading().als.value;
}

void AmbientLightSensor::Trace(blink::Visitor* visitor) {
  Sensor::Trace(visitor);
}

}  // namespace blink
