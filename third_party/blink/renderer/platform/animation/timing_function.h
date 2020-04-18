/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Graham Dennis (graham.dennis@gmail.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_ANIMATION_TIMING_FUNCTION_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_ANIMATION_TIMING_FUNCTION_H_

#include "base/memory/scoped_refptr.h"
#include "cc/animation/timing_function.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class PLATFORM_EXPORT TimingFunction : public RefCounted<TimingFunction> {
 public:
  using Type = cc::TimingFunction::Type;

  virtual ~TimingFunction() = default;

  Type GetType() const { return type_; }

  virtual String ToString() const = 0;

  // Evaluates the timing function at the given fraction. The accuracy parameter
  // provides a hint as to the required accuracy and is not guaranteed.
  virtual double Evaluate(double fraction, double accuracy) const = 0;

  // This function returns the minimum and maximum values obtainable when
  // calling evaluate();
  virtual void Range(double* min_value, double* max_value) const = 0;

  // Create CC instance.
  virtual std::unique_ptr<cc::TimingFunction> CloneToCC() const = 0;

 protected:
  TimingFunction(Type type) : type_(type) {}

 private:
  Type type_;
};

class PLATFORM_EXPORT LinearTimingFunction final : public TimingFunction {
 public:
  static LinearTimingFunction* Shared() {
    DEFINE_STATIC_REF(LinearTimingFunction, linear,
                      (base::AdoptRef(new LinearTimingFunction())));
    return linear;
  }

  ~LinearTimingFunction() override = default;

  // TimingFunction implementation.
  String ToString() const override;
  double Evaluate(double fraction, double) const override;
  void Range(double* min_value, double* max_value) const override;
  std::unique_ptr<cc::TimingFunction> CloneToCC() const override;

 private:
  LinearTimingFunction() : TimingFunction(Type::LINEAR) {}
};

class PLATFORM_EXPORT CubicBezierTimingFunction final : public TimingFunction {
 public:
  using EaseType = cc::CubicBezierTimingFunction::EaseType;

  static scoped_refptr<CubicBezierTimingFunction> Create(double x1,
                                                         double y1,
                                                         double x2,
                                                         double y2) {
    return base::AdoptRef(new CubicBezierTimingFunction(x1, y1, x2, y2));
  }

  static CubicBezierTimingFunction* Preset(EaseType);

  ~CubicBezierTimingFunction() override = default;

  // TimingFunction implementation.
  String ToString() const override;
  double Evaluate(double fraction, double accuracy) const override;
  void Range(double* min_value, double* max_value) const override;
  std::unique_ptr<cc::TimingFunction> CloneToCC() const override;

  double X1() const {
    DCHECK_EQ(GetEaseType(), EaseType::CUSTOM);
    return x1_;
  }
  double Y1() const {
    DCHECK_EQ(GetEaseType(), EaseType::CUSTOM);
    return y1_;
  }
  double X2() const {
    DCHECK_EQ(GetEaseType(), EaseType::CUSTOM);
    return x2_;
  }
  double Y2() const {
    DCHECK_EQ(GetEaseType(), EaseType::CUSTOM);
    return y2_;
  }
  EaseType GetEaseType() const { return bezier_->ease_type(); }

 private:
  explicit CubicBezierTimingFunction(EaseType ease_type)
      : TimingFunction(Type::CUBIC_BEZIER),
        bezier_(cc::CubicBezierTimingFunction::CreatePreset(ease_type)),
        x1_(),
        y1_(),
        x2_(),
        y2_() {}

  CubicBezierTimingFunction(double x1, double y1, double x2, double y2)
      : TimingFunction(Type::CUBIC_BEZIER),
        bezier_(cc::CubicBezierTimingFunction::Create(x1, y1, x2, y2)),
        x1_(x1),
        y1_(y1),
        x2_(x2),
        y2_(y2) {}

  std::unique_ptr<cc::CubicBezierTimingFunction> bezier_;

  // TODO(loyso): Get these values from m_bezier->bezier_ (gfx::CubicBezier)
  const double x1_;
  const double y1_;
  const double x2_;
  const double y2_;
};

class PLATFORM_EXPORT StepsTimingFunction final : public TimingFunction {
 public:
  using StepPosition = cc::StepsTimingFunction::StepPosition;

  static scoped_refptr<StepsTimingFunction> Create(int steps,
                                                   StepPosition step_position) {
    return base::AdoptRef(new StepsTimingFunction(steps, step_position));
  }

  static StepsTimingFunction* Preset(StepPosition position) {
    DEFINE_STATIC_REF(StepsTimingFunction, start,
                      Create(1, StepPosition::START));
    DEFINE_STATIC_REF(StepsTimingFunction, middle,
                      Create(1, StepPosition::MIDDLE));
    DEFINE_STATIC_REF(StepsTimingFunction, end, Create(1, StepPosition::END));
    switch (position) {
      case StepPosition::START:
        return start;
      case StepPosition::MIDDLE:
        return middle;
      case StepPosition::END:
        return end;
      default:
        NOTREACHED();
        return end;
    }
  }

  ~StepsTimingFunction() override = default;

  // TimingFunction implementation.
  String ToString() const override;
  double Evaluate(double fraction, double) const override;
  void Range(double* min_value, double* max_value) const override;
  std::unique_ptr<cc::TimingFunction> CloneToCC() const override;

  int NumberOfSteps() const { return steps_->steps(); }
  StepPosition GetStepPosition() const { return steps_->step_position(); }

 private:
  StepsTimingFunction(int steps, StepPosition step_position)
      : TimingFunction(Type::STEPS),
        steps_(cc::StepsTimingFunction::Create(steps, step_position)) {}

  std::unique_ptr<cc::StepsTimingFunction> steps_;
};

class PLATFORM_EXPORT FramesTimingFunction final : public TimingFunction {
 public:
  static scoped_refptr<FramesTimingFunction> Create(int frames) {
    return base::AdoptRef(new FramesTimingFunction(frames));
  }

  ~FramesTimingFunction() override = default;

  // TimingFunction implementation.
  String ToString() const override;
  double Evaluate(double fraction, double) const override;
  void Range(double* min_value, double* max_value) const override;
  std::unique_ptr<cc::TimingFunction> CloneToCC() const override;

  int NumberOfFrames() const { return frames_->frames(); }

 private:
  FramesTimingFunction(int frames)
      : TimingFunction(Type::FRAMES),
        frames_(cc::FramesTimingFunction::Create(frames)) {
    DCHECK(RuntimeEnabledFeatures::FramesTimingFunctionEnabled());
  }

  std::unique_ptr<cc::FramesTimingFunction> frames_;
};

PLATFORM_EXPORT scoped_refptr<TimingFunction>
CreateCompositorTimingFunctionFromCC(const cc::TimingFunction*);

PLATFORM_EXPORT bool operator==(const LinearTimingFunction&,
                                const TimingFunction&);
PLATFORM_EXPORT bool operator==(const CubicBezierTimingFunction&,
                                const TimingFunction&);
PLATFORM_EXPORT bool operator==(const StepsTimingFunction&,
                                const TimingFunction&);
PLATFORM_EXPORT bool operator==(const FramesTimingFunction&,
                                const TimingFunction&);

PLATFORM_EXPORT bool operator==(const TimingFunction&, const TimingFunction&);
PLATFORM_EXPORT bool operator!=(const TimingFunction&, const TimingFunction&);

#define DEFINE_TIMING_FUNCTION_TYPE_CASTS(typeName, enumName)           \
  DEFINE_TYPE_CASTS(typeName##TimingFunction, TimingFunction, value,    \
                    value->GetType() == TimingFunction::Type::enumName, \
                    value.GetType() == TimingFunction::Type::enumName)

DEFINE_TIMING_FUNCTION_TYPE_CASTS(Linear, LINEAR);
DEFINE_TIMING_FUNCTION_TYPE_CASTS(CubicBezier, CUBIC_BEZIER);
DEFINE_TIMING_FUNCTION_TYPE_CASTS(Steps, STEPS);
DEFINE_TIMING_FUNCTION_TYPE_CASTS(Frames, FRAMES);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_ANIMATION_TIMING_FUNCTION_H_
