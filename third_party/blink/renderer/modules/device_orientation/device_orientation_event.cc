/*
 * Copyright 2010, The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/device_orientation/device_orientation_event.h"

#include "third_party/blink/renderer/modules/device_orientation/device_orientation_data.h"
#include "third_party/blink/renderer/modules/device_orientation/device_orientation_event_init.h"

namespace blink {

DeviceOrientationEvent::~DeviceOrientationEvent() = default;

DeviceOrientationEvent::DeviceOrientationEvent()
    : orientation_(DeviceOrientationData::Create()) {}

DeviceOrientationEvent::DeviceOrientationEvent(
    const AtomicString& event_type,
    const DeviceOrientationEventInit& initializer)
    : Event(event_type, initializer),
      orientation_(DeviceOrientationData::Create(initializer)) {}

DeviceOrientationEvent::DeviceOrientationEvent(
    const AtomicString& event_type,
    DeviceOrientationData* orientation)
    : Event(event_type, Bubbles::kNo, Cancelable::kNo),
      orientation_(orientation) {}

double DeviceOrientationEvent::alpha(bool& is_null) const {
  if (orientation_->CanProvideAlpha())
    return orientation_->Alpha();

  is_null = true;
  return 0;
}

double DeviceOrientationEvent::beta(bool& is_null) const {
  if (orientation_->CanProvideBeta())
    return orientation_->Beta();

  is_null = true;
  return 0;
}

double DeviceOrientationEvent::gamma(bool& is_null) const {
  if (orientation_->CanProvideGamma())
    return orientation_->Gamma();

  is_null = true;
  return 0;
}

bool DeviceOrientationEvent::absolute() const {
  return orientation_->Absolute();
}

const AtomicString& DeviceOrientationEvent::InterfaceName() const {
  return EventNames::DeviceOrientationEvent;
}

void DeviceOrientationEvent::Trace(blink::Visitor* visitor) {
  visitor->Trace(orientation_);
  Event::Trace(visitor);
}

}  // namespace blink
