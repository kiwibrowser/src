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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_DEVICE_ORIENTATION_DEVICE_MOTION_DATA_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_DEVICE_ORIENTATION_DEVICE_MOTION_DATA_H_

#include "third_party/blink/renderer/platform/heap/handle.h"

namespace device {
class MotionData;
}

namespace blink {

class DeviceAccelerationInit;
class DeviceMotionEventInit;
class DeviceRotationRateInit;

class DeviceMotionData final : public GarbageCollected<DeviceMotionData> {
 public:
  class Acceleration final
      : public GarbageCollected<DeviceMotionData::Acceleration> {
   public:
    static Acceleration* Create(bool can_provide_x,
                                double x,
                                bool can_provide_y,
                                double y,
                                bool can_provide_z,
                                double z);
    static Acceleration* Create(const DeviceAccelerationInit&);
    void Trace(blink::Visitor* visitor) {}

    bool CanProvideX() const { return can_provide_x_; }
    bool CanProvideY() const { return can_provide_y_; }
    bool CanProvideZ() const { return can_provide_z_; }

    double X() const { return x_; }
    double Y() const { return y_; }
    double Z() const { return z_; }

   private:
    Acceleration(bool can_provide_x,
                 double x,
                 bool can_provide_y,
                 double y,
                 bool can_provide_z,
                 double z);

    double x_;
    double y_;
    double z_;

    bool can_provide_x_;
    bool can_provide_y_;
    bool can_provide_z_;
  };

  class RotationRate final
      : public GarbageCollected<DeviceMotionData::RotationRate> {
   public:
    static RotationRate* Create(bool can_provide_alpha,
                                double alpha,
                                bool can_provide_beta,
                                double beta,
                                bool can_provide_gamma,
                                double gamma);
    static RotationRate* Create(const DeviceRotationRateInit&);
    void Trace(blink::Visitor* visitor) {}

    bool CanProvideAlpha() const { return can_provide_alpha_; }
    bool CanProvideBeta() const { return can_provide_beta_; }
    bool CanProvideGamma() const { return can_provide_gamma_; }

    double Alpha() const { return alpha_; }
    double Beta() const { return beta_; }
    double Gamma() const { return gamma_; }

   private:
    RotationRate(bool can_provide_alpha,
                 double alpha,
                 bool can_provide_beta,
                 double beta,
                 bool can_provide_gamma,
                 double gamma);

    double alpha_;
    double beta_;
    double gamma_;

    bool can_provide_alpha_;
    bool can_provide_beta_;
    bool can_provide_gamma_;
  };

  static DeviceMotionData* Create();
  static DeviceMotionData* Create(Acceleration*,
                                  Acceleration* acceleration_including_gravity,
                                  RotationRate*,
                                  double interval);
  static DeviceMotionData* Create(const DeviceMotionEventInit&);
  static DeviceMotionData* Create(const device::MotionData&);
  void Trace(blink::Visitor*);

  Acceleration* GetAcceleration() const { return acceleration_.Get(); }
  Acceleration* GetAccelerationIncludingGravity() const {
    return acceleration_including_gravity_.Get();
  }
  RotationRate* GetRotationRate() const { return rotation_rate_.Get(); }

  double Interval() const { return interval_; }

  bool CanProvideEventData() const;

 private:
  DeviceMotionData();
  DeviceMotionData(Acceleration*,
                   Acceleration* acceleration_including_gravity,
                   RotationRate*,
                   double interval);

  Member<Acceleration> acceleration_;
  Member<Acceleration> acceleration_including_gravity_;
  Member<RotationRate> rotation_rate_;
  double interval_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_DEVICE_ORIENTATION_DEVICE_MOTION_DATA_H_
