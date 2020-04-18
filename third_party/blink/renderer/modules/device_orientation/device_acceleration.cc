/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/device_orientation/device_acceleration.h"

namespace blink {

DeviceAcceleration::DeviceAcceleration(
    DeviceMotionData::Acceleration* acceleration)
    : acceleration_(acceleration) {}

void DeviceAcceleration::Trace(blink::Visitor* visitor) {
  visitor->Trace(acceleration_);
  ScriptWrappable::Trace(visitor);
}

double DeviceAcceleration::x(bool& is_null) const {
  if (acceleration_->CanProvideX())
    return acceleration_->X();

  is_null = true;
  return 0;
}

double DeviceAcceleration::y(bool& is_null) const {
  if (acceleration_->CanProvideY())
    return acceleration_->Y();

  is_null = true;
  return 0;
}

double DeviceAcceleration::z(bool& is_null) const {
  if (acceleration_->CanProvideZ())
    return acceleration_->Z();

  is_null = true;
  return 0;
}

}  // namespace blink
