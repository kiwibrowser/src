/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_DEVICE_ORIENTATION_DEVICE_MOTION_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_DEVICE_ORIENTATION_DEVICE_MOTION_EVENT_H_

#include "third_party/blink/renderer/modules/event_modules.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class DeviceAcceleration;
class DeviceMotionData;
class DeviceMotionEventInit;
class DeviceRotationRate;

class DeviceMotionEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ~DeviceMotionEvent() override;
  static DeviceMotionEvent* Create() { return new DeviceMotionEvent; }
  static DeviceMotionEvent* Create(const AtomicString& event_type,
                                   const DeviceMotionEventInit& initializer) {
    return new DeviceMotionEvent(event_type, initializer);
  }
  static DeviceMotionEvent* Create(const AtomicString& event_type,
                                   const DeviceMotionData* device_motion_data) {
    return new DeviceMotionEvent(event_type, device_motion_data);
  }

  const DeviceMotionData* GetDeviceMotionData() const {
    return device_motion_data_.Get();
  }

  DeviceAcceleration* acceleration();
  DeviceAcceleration* accelerationIncludingGravity();
  DeviceRotationRate* rotationRate();
  double interval() const;

  const AtomicString& InterfaceName() const override;

  void Trace(blink::Visitor*) override;

 private:
  DeviceMotionEvent();
  DeviceMotionEvent(const AtomicString&, const DeviceMotionEventInit&);
  DeviceMotionEvent(const AtomicString& event_type, const DeviceMotionData*);

  Member<const DeviceMotionData> device_motion_data_;
  Member<DeviceAcceleration> acceleration_;
  Member<DeviceAcceleration> acceleration_including_gravity_;
  Member<DeviceRotationRate> rotation_rate_;
};

DEFINE_TYPE_CASTS(DeviceMotionEvent,
                  Event,
                  event,
                  event->InterfaceName() == EventNames::DeviceMotionEvent,
                  event.InterfaceName() == EventNames::DeviceMotionEvent);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_DEVICE_ORIENTATION_DEVICE_MOTION_EVENT_H_
