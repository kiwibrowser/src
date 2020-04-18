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

#include "third_party/blink/renderer/modules/device_orientation/device_motion_event.h"

#include "third_party/blink/renderer/modules/device_orientation/device_acceleration.h"
#include "third_party/blink/renderer/modules/device_orientation/device_motion_data.h"
#include "third_party/blink/renderer/modules/device_orientation/device_motion_event_init.h"
#include "third_party/blink/renderer/modules/device_orientation/device_rotation_rate.h"

namespace blink {

DeviceMotionEvent::~DeviceMotionEvent() = default;

DeviceMotionEvent::DeviceMotionEvent()
    : device_motion_data_(DeviceMotionData::Create()) {}

DeviceMotionEvent::DeviceMotionEvent(const AtomicString& event_type,
                                     const DeviceMotionEventInit& initializer)
    : Event(event_type, initializer),
      device_motion_data_(DeviceMotionData::Create(initializer)) {}

DeviceMotionEvent::DeviceMotionEvent(const AtomicString& event_type,
                                     const DeviceMotionData* device_motion_data)
    : Event(event_type, Bubbles::kNo, Cancelable::kNo),
      device_motion_data_(device_motion_data) {}

DeviceAcceleration* DeviceMotionEvent::acceleration() {
  if (!device_motion_data_->GetAcceleration())
    return nullptr;

  if (!acceleration_)
    acceleration_ =
        DeviceAcceleration::Create(device_motion_data_->GetAcceleration());

  return acceleration_.Get();
}

DeviceAcceleration* DeviceMotionEvent::accelerationIncludingGravity() {
  if (!device_motion_data_->GetAccelerationIncludingGravity())
    return nullptr;

  if (!acceleration_including_gravity_)
    acceleration_including_gravity_ = DeviceAcceleration::Create(
        device_motion_data_->GetAccelerationIncludingGravity());

  return acceleration_including_gravity_.Get();
}

DeviceRotationRate* DeviceMotionEvent::rotationRate() {
  if (!device_motion_data_->GetRotationRate())
    return nullptr;

  if (!rotation_rate_)
    rotation_rate_ =
        DeviceRotationRate::Create(device_motion_data_->GetRotationRate());

  return rotation_rate_.Get();
}

double DeviceMotionEvent::interval() const {
  return device_motion_data_->Interval();
}

const AtomicString& DeviceMotionEvent::InterfaceName() const {
  return EventNames::DeviceMotionEvent;
}

void DeviceMotionEvent::Trace(blink::Visitor* visitor) {
  visitor->Trace(device_motion_data_);
  visitor->Trace(acceleration_);
  visitor->Trace(acceleration_including_gravity_);
  visitor->Trace(rotation_rate_);
  Event::Trace(visitor);
}

}  // namespace blink
