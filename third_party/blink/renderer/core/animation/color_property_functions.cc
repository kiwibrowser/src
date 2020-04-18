// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/color_property_functions.h"

#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

OptionalStyleColor ColorPropertyFunctions::GetInitialColor(
    const CSSProperty& property) {
  return GetUnvisitedColor(property, ComputedStyle::InitialStyle());
}

OptionalStyleColor ColorPropertyFunctions::GetUnvisitedColor(
    const CSSProperty& property,
    const ComputedStyle& style) {
  switch (property.PropertyID()) {
    case CSSPropertyBackgroundColor:
      return style.BackgroundColor();
    case CSSPropertyBorderLeftColor:
      return style.BorderLeftColor();
    case CSSPropertyBorderRightColor:
      return style.BorderRightColor();
    case CSSPropertyBorderTopColor:
      return style.BorderTopColor();
    case CSSPropertyBorderBottomColor:
      return style.BorderBottomColor();
    case CSSPropertyCaretColor:
      if (style.CaretColor().IsAutoColor())
        return nullptr;
      return style.CaretColor().ToStyleColor();
    case CSSPropertyColor:
      return style.GetColor();
    case CSSPropertyOutlineColor:
      return style.OutlineColor();
    case CSSPropertyColumnRuleColor:
      return style.ColumnRuleColor();
    case CSSPropertyWebkitTextEmphasisColor:
      return style.TextEmphasisColor();
    case CSSPropertyWebkitTextFillColor:
      return style.TextFillColor();
    case CSSPropertyWebkitTextStrokeColor:
      return style.TextStrokeColor();
    case CSSPropertyFloodColor:
      return style.FloodColor();
    case CSSPropertyLightingColor:
      return style.LightingColor();
    case CSSPropertyStopColor:
      return style.StopColor();
    case CSSPropertyWebkitTapHighlightColor:
      return style.TapHighlightColor();
    case CSSPropertyTextDecorationColor:
      return style.TextDecorationColor();
    default:
      NOTREACHED();
      return nullptr;
  }
}

OptionalStyleColor ColorPropertyFunctions::GetVisitedColor(
    const CSSProperty& property,
    const ComputedStyle& style) {
  switch (property.PropertyID()) {
    case CSSPropertyBackgroundColor:
      return style.VisitedLinkBackgroundColor();
    case CSSPropertyBorderLeftColor:
      return style.VisitedLinkBorderLeftColor();
    case CSSPropertyBorderRightColor:
      return style.VisitedLinkBorderRightColor();
    case CSSPropertyBorderTopColor:
      return style.VisitedLinkBorderTopColor();
    case CSSPropertyBorderBottomColor:
      return style.VisitedLinkBorderBottomColor();
    case CSSPropertyCaretColor:
      // TODO(rego): "auto" value for caret-color should not interpolate
      // (http://crbug.com/676295).
      if (style.VisitedLinkCaretColor().IsAutoColor())
        return StyleColor::CurrentColor();
      return style.VisitedLinkCaretColor().ToStyleColor();
    case CSSPropertyColor:
      return style.VisitedLinkColor();
    case CSSPropertyOutlineColor:
      return style.VisitedLinkOutlineColor();
    case CSSPropertyColumnRuleColor:
      return style.VisitedLinkColumnRuleColor();
    case CSSPropertyWebkitTextEmphasisColor:
      return style.VisitedLinkTextEmphasisColor();
    case CSSPropertyWebkitTextFillColor:
      return style.VisitedLinkTextFillColor();
    case CSSPropertyWebkitTextStrokeColor:
      return style.VisitedLinkTextStrokeColor();
    case CSSPropertyFloodColor:
      return style.FloodColor();
    case CSSPropertyLightingColor:
      return style.LightingColor();
    case CSSPropertyStopColor:
      return style.StopColor();
    case CSSPropertyWebkitTapHighlightColor:
      return style.TapHighlightColor();
    case CSSPropertyTextDecorationColor:
      return style.VisitedLinkTextDecorationColor();
    default:
      NOTREACHED();
      return nullptr;
  }
}

void ColorPropertyFunctions::SetUnvisitedColor(const CSSProperty& property,
                                               ComputedStyle& style,
                                               const Color& color) {
  switch (property.PropertyID()) {
    case CSSPropertyBackgroundColor:
      style.SetBackgroundColor(color);
      return;
    case CSSPropertyBorderBottomColor:
      style.SetBorderBottomColor(color);
      return;
    case CSSPropertyBorderLeftColor:
      style.SetBorderLeftColor(color);
      return;
    case CSSPropertyBorderRightColor:
      style.SetBorderRightColor(color);
      return;
    case CSSPropertyBorderTopColor:
      style.SetBorderTopColor(color);
      return;
    case CSSPropertyCaretColor:
      return style.SetCaretColor(color);
    case CSSPropertyColor:
      style.SetColor(color);
      return;
    case CSSPropertyFloodColor:
      style.SetFloodColor(color);
      return;
    case CSSPropertyLightingColor:
      style.SetLightingColor(color);
      return;
    case CSSPropertyOutlineColor:
      style.SetOutlineColor(color);
      return;
    case CSSPropertyStopColor:
      style.SetStopColor(color);
      return;
    case CSSPropertyTextDecorationColor:
      style.SetTextDecorationColor(color);
      return;
    case CSSPropertyColumnRuleColor:
      style.SetColumnRuleColor(color);
      return;
    case CSSPropertyWebkitTextStrokeColor:
      style.SetTextStrokeColor(color);
      return;
    default:
      NOTREACHED();
      return;
  }
}

void ColorPropertyFunctions::SetVisitedColor(const CSSProperty& property,
                                             ComputedStyle& style,
                                             const Color& color) {
  switch (property.PropertyID()) {
    case CSSPropertyBackgroundColor:
      style.SetVisitedLinkBackgroundColor(color);
      return;
    case CSSPropertyBorderBottomColor:
      style.SetVisitedLinkBorderBottomColor(color);
      return;
    case CSSPropertyBorderLeftColor:
      style.SetVisitedLinkBorderLeftColor(color);
      return;
    case CSSPropertyBorderRightColor:
      style.SetVisitedLinkBorderRightColor(color);
      return;
    case CSSPropertyBorderTopColor:
      style.SetVisitedLinkBorderTopColor(color);
      return;
    case CSSPropertyCaretColor:
      return style.SetVisitedLinkCaretColor(color);
    case CSSPropertyColor:
      style.SetVisitedLinkColor(color);
      return;
    case CSSPropertyFloodColor:
      style.SetFloodColor(color);
      return;
    case CSSPropertyLightingColor:
      style.SetLightingColor(color);
      return;
    case CSSPropertyOutlineColor:
      style.SetVisitedLinkOutlineColor(color);
      return;
    case CSSPropertyStopColor:
      style.SetStopColor(color);
      return;
    case CSSPropertyTextDecorationColor:
      style.SetVisitedLinkTextDecorationColor(color);
      return;
    case CSSPropertyColumnRuleColor:
      style.SetVisitedLinkColumnRuleColor(color);
      return;
    case CSSPropertyWebkitTextStrokeColor:
      style.SetVisitedLinkTextStrokeColor(color);
      return;
    default:
      NOTREACHED();
      return;
  }
}

}  // namespace blink
