/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights
 * reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_STYLE_SHADOW_DATA_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_STYLE_SHADOW_DATA_H_

#include "third_party/blink/renderer/core/css/style_color.h"
#include "third_party/blink/renderer/platform/geometry/float_point.h"
#include "third_party/blink/renderer/platform/geometry/float_rect_outsets.h"
#include "third_party/blink/renderer/platform/graphics/skia/skia_utils.h"

namespace blink {

enum ShadowStyle { kNormal, kInset };

// This class holds information about shadows for the text-shadow and box-shadow
// properties, as well as the drop-shadow(...) filter operation.
class ShadowData {
  USING_FAST_MALLOC(ShadowData);

 public:
  ShadowData(const FloatPoint& location,
             float blur,
             float spread,
             ShadowStyle style,
             StyleColor color)
      : location_(location),
        blur_(blur),
        spread_(spread),
        color_(color),
        style_(style) {}

  bool operator==(const ShadowData&) const;
  bool operator!=(const ShadowData& o) const { return !(*this == o); }

  ShadowData Blend(const ShadowData& from,
                   double progress,
                   const Color& current_color) const;
  static ShadowData NeutralValue();

  float X() const { return location_.X(); }
  float Y() const { return location_.Y(); }
  FloatPoint Location() const { return location_; }
  float Blur() const { return blur_; }
  float Spread() const { return spread_; }
  ShadowStyle Style() const { return style_; }
  StyleColor GetColor() const { return color_; }

  void OverrideColor(Color color) { color_ = StyleColor(color); }

  // Outsets needed to adjust a source rectangle to the one cast by this
  // shadow.
  FloatRectOutsets RectOutsets() const {
    // 3 * skBlurRadiusToSigma(blur()) is how Skia implements the radius of a
    // blur. See also https://crbug.com/624175.
    float blur_and_spread = ceil(3 * SkBlurRadiusToSigma(Blur())) + Spread();
    return FloatRectOutsets(
        blur_and_spread - Y() /* top */, blur_and_spread + X() /* right */,
        blur_and_spread + Y() /* bottom */, blur_and_spread - X() /* left */);
  }

 private:
  FloatPoint location_;
  float blur_;
  float spread_;
  StyleColor color_;
  ShadowStyle style_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_STYLE_SHADOW_DATA_H_
