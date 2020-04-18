// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/number_property_functions.h"

#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

base::Optional<double> NumberPropertyFunctions::GetInitialNumber(
    const CSSProperty& property) {
  return GetNumber(property, ComputedStyle::InitialStyle());
}

base::Optional<double> NumberPropertyFunctions::GetNumber(
    const CSSProperty& property,
    const ComputedStyle& style) {
  switch (property.PropertyID()) {
    case CSSPropertyFillOpacity:
      return style.FillOpacity();
    case CSSPropertyFlexGrow:
      return style.FlexGrow();
    case CSSPropertyFlexShrink:
      return style.FlexShrink();
    case CSSPropertyFloodOpacity:
      return style.FloodOpacity();
    case CSSPropertyOpacity:
      return style.Opacity();
    case CSSPropertyOrder:
      return style.Order();
    case CSSPropertyOrphans:
      return style.Orphans();
    case CSSPropertyShapeImageThreshold:
      return style.ShapeImageThreshold();
    case CSSPropertyStopOpacity:
      return style.StopOpacity();
    case CSSPropertyStrokeMiterlimit:
      return style.StrokeMiterLimit();
    case CSSPropertyStrokeOpacity:
      return style.StrokeOpacity();
    case CSSPropertyWidows:
      return style.Widows();

    case CSSPropertyFontSizeAdjust:
      if (!style.HasFontSizeAdjust())
        return base::Optional<double>();
      return style.FontSizeAdjust();
    case CSSPropertyColumnCount:
      if (style.HasAutoColumnCount())
        return base::Optional<double>();
      return style.ColumnCount();
    case CSSPropertyZIndex:
      if (style.HasAutoZIndex())
        return base::Optional<double>();
      return style.ZIndex();

    case CSSPropertyTextSizeAdjust: {
      const TextSizeAdjust& text_size_adjust = style.GetTextSizeAdjust();
      if (text_size_adjust.IsAuto())
        return base::Optional<double>();
      return text_size_adjust.Multiplier() * 100;
    }

    case CSSPropertyLineHeight: {
      const Length& length = style.SpecifiedLineHeight();
      // Numbers are represented by percentages.
      if (length.GetType() != kPercent)
        return base::Optional<double>();
      double value = length.Value();
      // -100% represents the keyword "normal".
      if (value == -100)
        return base::Optional<double>();
      return value / 100;
    }

    default:
      return base::Optional<double>();
  }
}

double NumberPropertyFunctions::ClampNumber(const CSSProperty& property,
                                            double value) {
  switch (property.PropertyID()) {
    case CSSPropertyStrokeMiterlimit:
      return clampTo<float>(value, 1);

    case CSSPropertyFloodOpacity:
    case CSSPropertyStopOpacity:
    case CSSPropertyStrokeOpacity:
    case CSSPropertyShapeImageThreshold:
      return clampTo<float>(value, 0, 1);

    case CSSPropertyFillOpacity:
    case CSSPropertyOpacity:
      return clampTo<float>(value, 0, nextafterf(1, 0));

    case CSSPropertyFlexGrow:
    case CSSPropertyFlexShrink:
    case CSSPropertyFontSizeAdjust:
    case CSSPropertyLineHeight:
    case CSSPropertyTextSizeAdjust:
      return clampTo<float>(value, 0);

    case CSSPropertyOrphans:
    case CSSPropertyWidows:
      return clampTo<short>(round(value), 1);

    case CSSPropertyColumnCount:
      return clampTo<unsigned short>(round(value), 1);

    case CSSPropertyColumnRuleWidth:
      return clampTo<unsigned short>(round(value));

    case CSSPropertyOrder:
    case CSSPropertyZIndex:
      return clampTo<int>(round(value));

    default:
      NOTREACHED();
      return value;
  }
}

bool NumberPropertyFunctions::SetNumber(const CSSProperty& property,
                                        ComputedStyle& style,
                                        double value) {
  DCHECK_EQ(value, ClampNumber(property, value));
  switch (property.PropertyID()) {
    case CSSPropertyFillOpacity:
      style.SetFillOpacity(value);
      return true;
    case CSSPropertyFlexGrow:
      style.SetFlexGrow(value);
      return true;
    case CSSPropertyFlexShrink:
      style.SetFlexShrink(value);
      return true;
    case CSSPropertyFloodOpacity:
      style.SetFloodOpacity(value);
      return true;
    case CSSPropertyLineHeight:
      style.SetLineHeight(Length(value * 100, kPercent));
      return true;
    case CSSPropertyOpacity:
      style.SetOpacity(value);
      return true;
    case CSSPropertyOrder:
      style.SetOrder(value);
      return true;
    case CSSPropertyOrphans:
      style.SetOrphans(value);
      return true;
    case CSSPropertyShapeImageThreshold:
      style.SetShapeImageThreshold(value);
      return true;
    case CSSPropertyStopOpacity:
      style.SetStopOpacity(value);
      return true;
    case CSSPropertyStrokeMiterlimit:
      style.SetStrokeMiterLimit(value);
      return true;
    case CSSPropertyStrokeOpacity:
      style.SetStrokeOpacity(value);
      return true;
    case CSSPropertyColumnCount:
      style.SetColumnCount(value);
      return true;
    case CSSPropertyTextSizeAdjust:
      style.SetTextSizeAdjust(value / 100.);
      return true;
    case CSSPropertyWidows:
      style.SetWidows(value);
      return true;
    case CSSPropertyZIndex:
      style.SetZIndex(value);
      return true;
    default:
      return false;
  }
}

}  // namespace blink
