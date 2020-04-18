// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SENSOR_SENSOR_INSPECTOR_AGENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SENSOR_SENSOR_INSPECTOR_AGENT_H_

#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class LocalFrame;
class SensorProviderProxy;

class SensorInspectorAgent : public GarbageCollected<SensorInspectorAgent> {
 public:
  explicit SensorInspectorAgent(LocalFrame* frame);
  virtual void Trace(blink::Visitor*);

  void SetOrientationSensorOverride(double alpha, double beta, double gamma);

  void Disable();

 private:
  Member<SensorProviderProxy> provider_;

  DISALLOW_COPY_AND_ASSIGN(SensorInspectorAgent);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SENSOR_SENSOR_INSPECTOR_AGENT_H_
