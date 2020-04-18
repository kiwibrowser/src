/*
 * Copyright (C) 2011 Apple Inc.  All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_ANIMATION_ANIMATION_UTILITIES_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_ANIMATION_ANIMATION_UTILITIES_H_

#include "third_party/blink/renderer/platform/geometry/float_point.h"
#include "third_party/blink/renderer/platform/geometry/int_point.h"
#include "third_party/blink/renderer/platform/layout_unit.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/math_extras.h"
#include "ui/gfx/geometry/cubic_bezier.h"

#include <type_traits>

namespace blink {

inline int Blend(int from, int to, double progress) {
  return lround(from + (to - from) * progress);
}

// For unsigned types.
template <typename T>
inline T Blend(T from, T to, double progress) {
  static_assert(std::is_integral<T>::value,
                "blend can only be used with integer types");
  return clampTo<T>(round(to > from ? from + (to - from) * progress
                                    : from - (from - to) * progress));
}

inline double Blend(double from, double to, double progress) {
  return from + (to - from) * progress;
}

inline float Blend(float from, float to, double progress) {
  return static_cast<float>(from + (to - from) * progress);
}

inline LayoutUnit Blend(LayoutUnit from, LayoutUnit to, double progress) {
  return LayoutUnit(from + (to - from) * progress);
}

inline IntPoint Blend(const IntPoint& from,
                      const IntPoint& to,
                      double progress) {
  return IntPoint(Blend(from.X(), to.X(), progress),
                  Blend(from.Y(), to.Y(), progress));
}

inline FloatPoint Blend(const FloatPoint& from,
                        const FloatPoint& to,
                        double progress) {
  return FloatPoint(Blend(from.X(), to.X(), progress),
                    Blend(from.Y(), to.Y(), progress));
}

// Calculates the accuracy for evaluating a timing function for an animation
// with the specified duration.
inline double AccuracyForDuration(double duration) {
  double accuracy = 1.0 / (200.0 * duration);
  double default_epsilon = gfx::CubicBezier::GetDefaultEpsilon();
  // Avoid min()/max() from std here in the header, because that would require
  // inclusion of <algorithm>, which is slow to compile.
  return accuracy > default_epsilon ? accuracy : default_epsilon;
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_ANIMATION_ANIMATION_UTILITIES_H_
