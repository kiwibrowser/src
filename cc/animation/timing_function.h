// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_TIMING_FUNCTION_H_
#define CC_ANIMATION_TIMING_FUNCTION_H_

#include <memory>

#include "base/macros.h"
#include "cc/animation/animation_export.h"
#include "ui/gfx/geometry/cubic_bezier.h"

namespace cc {

// See http://www.w3.org/TR/css3-transitions/.
class CC_ANIMATION_EXPORT TimingFunction {
 public:
  virtual ~TimingFunction();

  // Note that LINEAR is a nullptr TimingFunction (for now).
  enum class Type { LINEAR, CUBIC_BEZIER, STEPS, FRAMES };

  virtual Type GetType() const = 0;
  virtual float GetValue(double t) const = 0;
  virtual float Velocity(double time) const = 0;
  virtual std::unique_ptr<TimingFunction> Clone() const = 0;

 protected:
  TimingFunction();

  DISALLOW_ASSIGN(TimingFunction);
};

class CC_ANIMATION_EXPORT CubicBezierTimingFunction : public TimingFunction {
 public:
  enum class EaseType { EASE, EASE_IN, EASE_OUT, EASE_IN_OUT, CUSTOM };

  static std::unique_ptr<CubicBezierTimingFunction> CreatePreset(
      EaseType ease_type);
  static std::unique_ptr<CubicBezierTimingFunction> Create(double x1,
                                                           double y1,
                                                           double x2,
                                                           double y2);
  ~CubicBezierTimingFunction() override;

  // TimingFunction implementation.
  Type GetType() const override;
  float GetValue(double time) const override;
  float Velocity(double time) const override;
  std::unique_ptr<TimingFunction> Clone() const override;

  EaseType ease_type() const { return ease_type_; }
  const gfx::CubicBezier& bezier() const { return bezier_; }

 private:
  CubicBezierTimingFunction(EaseType ease_type,
                            double x1,
                            double y1,
                            double x2,
                            double y2);

  gfx::CubicBezier bezier_;
  EaseType ease_type_;

  DISALLOW_ASSIGN(CubicBezierTimingFunction);
};

class CC_ANIMATION_EXPORT StepsTimingFunction : public TimingFunction {
 public:
  // Web Animations specification, 3.12.4. Timing in discrete steps.
  enum class StepPosition { START, MIDDLE, END };

  static std::unique_ptr<StepsTimingFunction> Create(
      int steps,
      StepPosition step_position);
  ~StepsTimingFunction() override;

  // TimingFunction implementation.
  Type GetType() const override;
  float GetValue(double t) const override;
  std::unique_ptr<TimingFunction> Clone() const override;
  float Velocity(double time) const override;

  int steps() const { return steps_; }
  StepPosition step_position() const { return step_position_; }
  double GetPreciseValue(double t) const;

 private:
  StepsTimingFunction(int steps, StepPosition step_position);

  float GetStepsStartOffset() const;

  int steps_;
  StepPosition step_position_;

  DISALLOW_ASSIGN(StepsTimingFunction);
};

class CC_ANIMATION_EXPORT FramesTimingFunction : public TimingFunction {
 public:
  static std::unique_ptr<FramesTimingFunction> Create(int frames);
  ~FramesTimingFunction() override;

  // TimingFunction implementation.
  Type GetType() const override;
  float GetValue(double t) const override;
  std::unique_ptr<TimingFunction> Clone() const override;
  float Velocity(double time) const override;

  int frames() const { return frames_; }
  double GetPreciseValue(double t) const;

 private:
  explicit FramesTimingFunction(int frames);

  int frames_;

  DISALLOW_ASSIGN(FramesTimingFunction);
};

}  // namespace cc

#endif  // CC_ANIMATION_TIMING_FUNCTION_H_
