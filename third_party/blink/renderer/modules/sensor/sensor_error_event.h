// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SENSOR_SENSOR_ERROR_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SENSOR_SENSOR_ERROR_EVENT_H_

#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/modules/event_modules.h"
#include "third_party/blink/renderer/modules/sensor/sensor_error_event_init.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class SensorErrorEvent : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static SensorErrorEvent* Create(const AtomicString& event_type,
                                  DOMException* error) {
    return new SensorErrorEvent(event_type, error);
  }

  static SensorErrorEvent* Create(const AtomicString& event_type,
                                  const SensorErrorEventInit& initializer) {
    return new SensorErrorEvent(event_type, initializer);
  }

  ~SensorErrorEvent() override;

  void Trace(blink::Visitor*) override;

  const AtomicString& InterfaceName() const override;

  DOMException* error() { return error_; }

 private:
  SensorErrorEvent(const AtomicString& event_type, DOMException* error);
  SensorErrorEvent(const AtomicString& event_type,
                   const SensorErrorEventInit& initializer);

  Member<DOMException> error_;
};

DEFINE_TYPE_CASTS(SensorErrorEvent,
                  Event,
                  event,
                  event->InterfaceName() == EventNames::SensorErrorEvent,
                  event.InterfaceName() == EventNames::SensorErrorEvent);

}  // namepsace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SENSOR_SENSOR_ERROR_EVENT_H_
