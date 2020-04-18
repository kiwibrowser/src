/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "third_party/blink/renderer/modules/device_orientation/device_motion_data.h"

#include "services/device/public/cpp/generic_sensor/motion_data.h"
#include "third_party/blink/renderer/modules/device_orientation/device_acceleration_init.h"
#include "third_party/blink/renderer/modules/device_orientation/device_motion_event_init.h"
#include "third_party/blink/renderer/modules/device_orientation/device_rotation_rate_init.h"

namespace blink {

DeviceMotionData::Acceleration* DeviceMotionData::Acceleration::Create(
    bool can_provide_x,
    double x,
    bool can_provide_y,
    double y,
    bool can_provide_z,
    double z) {
  return new DeviceMotionData::Acceleration(can_provide_x, x, can_provide_y, y,
                                            can_provide_z, z);
}

DeviceMotionData::Acceleration* DeviceMotionData::Acceleration::Create(
    const DeviceAccelerationInit& init) {
  return new DeviceMotionData::Acceleration(
      init.hasX(), init.hasX() ? init.x() : 0, init.hasY(),
      init.hasY() ? init.y() : 0, init.hasZ(), init.hasZ() ? init.z() : 0);
}

DeviceMotionData::Acceleration::Acceleration(bool can_provide_x,
                                             double x,
                                             bool can_provide_y,
                                             double y,
                                             bool can_provide_z,
                                             double z)
    : x_(x),
      y_(y),
      z_(z),
      can_provide_x_(can_provide_x),
      can_provide_y_(can_provide_y),
      can_provide_z_(can_provide_z)

{}

DeviceMotionData::RotationRate* DeviceMotionData::RotationRate::Create(
    bool can_provide_alpha,
    double alpha,
    bool can_provide_beta,
    double beta,
    bool can_provide_gamma,
    double gamma) {
  return new DeviceMotionData::RotationRate(can_provide_alpha, alpha,
                                            can_provide_beta, beta,
                                            can_provide_gamma, gamma);
}

DeviceMotionData::RotationRate* DeviceMotionData::RotationRate::Create(
    const DeviceRotationRateInit& init) {
  return new DeviceMotionData::RotationRate(
      init.hasAlpha(), init.hasAlpha() ? init.alpha() : 0, init.hasBeta(),
      init.hasBeta() ? init.beta() : 0, init.hasGamma(),
      init.hasGamma() ? init.gamma() : 0);
}

DeviceMotionData::RotationRate::RotationRate(bool can_provide_alpha,
                                             double alpha,
                                             bool can_provide_beta,
                                             double beta,
                                             bool can_provide_gamma,
                                             double gamma)
    : alpha_(alpha),
      beta_(beta),
      gamma_(gamma),
      can_provide_alpha_(can_provide_alpha),
      can_provide_beta_(can_provide_beta),
      can_provide_gamma_(can_provide_gamma) {}

DeviceMotionData* DeviceMotionData::Create() {
  return new DeviceMotionData;
}

DeviceMotionData* DeviceMotionData::Create(
    Acceleration* acceleration,
    Acceleration* acceleration_including_gravity,
    RotationRate* rotation_rate,
    double interval) {
  return new DeviceMotionData(acceleration, acceleration_including_gravity,
                              rotation_rate, interval);
}

DeviceMotionData* DeviceMotionData::Create(const DeviceMotionEventInit& init) {
  return DeviceMotionData::Create(
      init.hasAcceleration()
          ? DeviceMotionData::Acceleration::Create(init.acceleration())
          : nullptr,
      init.hasAccelerationIncludingGravity()
          ? DeviceMotionData::Acceleration::Create(
                init.accelerationIncludingGravity())
          : nullptr,
      init.hasRotationRate()
          ? DeviceMotionData::RotationRate::Create(init.rotationRate())
          : nullptr,
      init.interval());
}

DeviceMotionData* DeviceMotionData::Create(const device::MotionData& data) {
  return DeviceMotionData::Create(
      DeviceMotionData::Acceleration::Create(
          data.has_acceleration_x, data.acceleration_x, data.has_acceleration_y,
          data.acceleration_y, data.has_acceleration_z, data.acceleration_z),
      DeviceMotionData::Acceleration::Create(
          data.has_acceleration_including_gravity_x,
          data.acceleration_including_gravity_x,
          data.has_acceleration_including_gravity_y,
          data.acceleration_including_gravity_y,
          data.has_acceleration_including_gravity_z,
          data.acceleration_including_gravity_z),
      DeviceMotionData::RotationRate::Create(
          data.has_rotation_rate_alpha, data.rotation_rate_alpha,
          data.has_rotation_rate_beta, data.rotation_rate_beta,
          data.has_rotation_rate_gamma, data.rotation_rate_gamma),
      data.interval);
}

DeviceMotionData::DeviceMotionData() : interval_(0) {}

DeviceMotionData::DeviceMotionData(Acceleration* acceleration,
                                   Acceleration* acceleration_including_gravity,
                                   RotationRate* rotation_rate,
                                   double interval)
    : acceleration_(acceleration),
      acceleration_including_gravity_(acceleration_including_gravity),
      rotation_rate_(rotation_rate),
      interval_(interval) {}

void DeviceMotionData::Trace(blink::Visitor* visitor) {
  visitor->Trace(acceleration_);
  visitor->Trace(acceleration_including_gravity_);
  visitor->Trace(rotation_rate_);
}

bool DeviceMotionData::CanProvideEventData() const {
  const bool has_acceleration =
      acceleration_ &&
      (acceleration_->CanProvideX() || acceleration_->CanProvideY() ||
       acceleration_->CanProvideZ());
  const bool has_acceleration_including_gravity =
      acceleration_including_gravity_ &&
      (acceleration_including_gravity_->CanProvideX() ||
       acceleration_including_gravity_->CanProvideY() ||
       acceleration_including_gravity_->CanProvideZ());
  const bool has_rotation_rate =
      rotation_rate_ &&
      (rotation_rate_->CanProvideAlpha() || rotation_rate_->CanProvideBeta() ||
       rotation_rate_->CanProvideGamma());

  return has_acceleration || has_acceleration_including_gravity ||
         has_rotation_rate;
}

}  // namespace blink
