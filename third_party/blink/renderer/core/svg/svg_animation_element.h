/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron McCormack <cam@mcc.id.au>
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SVG_SVG_ANIMATION_ELEMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SVG_SVG_ANIMATION_ELEMENT_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/svg/animation/svg_smil_element.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "ui/gfx/geometry/cubic_bezier.h"

namespace blink {

class ExceptionState;

enum AnimationMode {
  kNoAnimation,
  kFromToAnimation,
  kFromByAnimation,
  kToAnimation,
  kByAnimation,
  kValuesAnimation,
  kPathAnimation  // Used by AnimateMotion.
};

enum CalcMode {
  kCalcModeDiscrete,
  kCalcModeLinear,
  kCalcModePaced,
  kCalcModeSpline
};

class CORE_EXPORT SVGAnimationElement : public SVGSMILElement {
  DEFINE_WRAPPERTYPEINFO();

 public:
  // SVGAnimationElement
  float getStartTime(ExceptionState&) const;
  float getCurrentTime() const;
  float getSimpleDuration(ExceptionState&) const;

  void beginElement() { beginElementAt(0); }
  void beginElementAt(float offset);
  void endElement() { endElementAt(0); }
  void endElementAt(float offset);

  DEFINE_MAPPED_ATTRIBUTE_EVENT_LISTENER(begin, beginEvent);
  DEFINE_MAPPED_ATTRIBUTE_EVENT_LISTENER(end, endEvent);
  DEFINE_MAPPED_ATTRIBUTE_EVENT_LISTENER(repeat, repeatEvent);

  virtual bool IsAdditive();
  bool IsAccumulated() const;
  AnimationMode GetAnimationMode() const { return animation_mode_; }
  CalcMode GetCalcMode() const { return calc_mode_; }

  template <typename AnimatedType>
  void AnimateDiscreteType(float percentage,
                           const AnimatedType& from_type,
                           const AnimatedType& to_type,
                           AnimatedType& animated_type) {
    if ((GetAnimationMode() == kFromToAnimation && percentage > 0.5) ||
        GetAnimationMode() == kToAnimation || percentage == 1) {
      animated_type = AnimatedType(to_type);
      return;
    }
    animated_type = AnimatedType(from_type);
  }

  void AnimateAdditiveNumber(float percentage,
                             unsigned repeat_count,
                             float from_number,
                             float to_number,
                             float to_at_end_of_duration_number,
                             float& animated_number) {
    float number;
    if (GetCalcMode() == kCalcModeDiscrete)
      number = percentage < 0.5 ? from_number : to_number;
    else
      number = (to_number - from_number) * percentage + from_number;

    if (IsAccumulated() && repeat_count)
      number += to_at_end_of_duration_number * repeat_count;

    if (IsAdditive() && GetAnimationMode() != kToAnimation)
      animated_number += number;
    else
      animated_number = number;
  }

 protected:
  SVGAnimationElement(const QualifiedName&, Document&);

  void ParseAttribute(const AttributeModificationParams&) override;
  void SvgAttributeChanged(const QualifiedName&) override;

  String ToValue() const;
  String ByValue() const;
  String FromValue() const;

  // from SVGSMILElement
  void StartedActiveInterval() override;
  void UpdateAnimation(float percent,
                       unsigned repeat,
                       SVGSMILElement* result_element) override;

  virtual void UpdateAnimationMode();
  void SetAnimationMode(AnimationMode animation_mode) {
    animation_mode_ = animation_mode;
  }
  void SetCalcMode(CalcMode calc_mode) { calc_mode_ = calc_mode; }

  // Parses a list of values as specified by SVG, stripping leading
  // and trailing whitespace, and places them in result. If the
  // format of the string is not valid, parseValues empties result
  // and returns false. See
  // http://www.w3.org/TR/SVG/animate.html#ValuesAttribute .
  static bool ParseValues(const String&, Vector<String>& result);

  void InvalidatedValuesCache();
  void AnimationAttributeChanged() override;

 private:
  bool IsValid() const final { return SVGTests::IsValid(); }

  virtual bool CalculateToAtEndOfDurationValue(
      const String& to_at_end_of_duration_string) = 0;
  virtual bool CalculateFromAndToValues(const String& from_string,
                                        const String& to_string) = 0;
  virtual bool CalculateFromAndByValues(const String& from_string,
                                        const String& by_string) = 0;
  virtual void CalculateAnimatedValue(float percent,
                                      unsigned repeat_count,
                                      SVGSMILElement* result_element) = 0;
  virtual float CalculateDistance(const String& /*fromString*/,
                                  const String& /*toString*/) {
    return -1.f;
  }

  void CurrentValuesForValuesAnimation(float percent,
                                       float& effective_percent,
                                       String& from,
                                       String& to);
  void CalculateKeyTimesForCalcModePaced();
  float CalculatePercentFromKeyPoints(float percent) const;
  void CurrentValuesFromKeyPoints(float percent,
                                  float& effective_percent,
                                  String& from,
                                  String& to) const;
  float CalculatePercentForSpline(float percent, unsigned spline_index) const;
  float CalculatePercentForFromTo(float percent) const;
  unsigned CalculateKeyTimesIndex(float percent) const;

  void SetCalcMode(const AtomicString&);

  bool animation_valid_;

  Vector<String> values_;
  // FIXME: We should probably use doubles for this, but there's no point
  // making such a change unless all SVG logic for sampling animations is
  // changed to use doubles.
  Vector<float> key_times_;
  Vector<float> key_points_;
  Vector<gfx::CubicBezier> key_splines_;
  String last_values_animation_from_;
  String last_values_animation_to_;
  CalcMode calc_mode_;
  AnimationMode animation_mode_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_SVG_SVG_ANIMATION_ELEMENT_H_
