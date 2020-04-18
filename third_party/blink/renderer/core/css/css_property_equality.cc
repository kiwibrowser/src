// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/css_property_equality.h"

#include "third_party/blink/renderer/core/animation/property_handle.h"
#include "third_party/blink/renderer/core/css/css_value.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/core/style/data_equivalency.h"
#include "third_party/blink/renderer/core/style/shadow_list.h"

// TODO(ikilpatrick): generate this file.

namespace blink {

namespace {

template <CSSPropertyID property>
bool FillLayersEqual(const FillLayer& a_layers, const FillLayer& b_layers) {
  const FillLayer* a_layer = &a_layers;
  const FillLayer* b_layer = &b_layers;
  while (a_layer && b_layer) {
    switch (property) {
      case CSSPropertyBackgroundPositionX:
      case CSSPropertyWebkitMaskPositionX:
        if (a_layer->PositionX() != b_layer->PositionX())
          return false;
        if (a_layer->BackgroundXOrigin() != b_layer->BackgroundXOrigin())
          return false;
        break;
      case CSSPropertyBackgroundPositionY:
      case CSSPropertyWebkitMaskPositionY:
        if (a_layer->PositionY() != b_layer->PositionY())
          return false;
        if (a_layer->BackgroundYOrigin() != b_layer->BackgroundYOrigin())
          return false;
        break;
      case CSSPropertyBackgroundSize:
      case CSSPropertyWebkitMaskSize:
        if (!(a_layer->SizeLength() == b_layer->SizeLength()))
          return false;
        break;
      case CSSPropertyBackgroundImage:
        if (!DataEquivalent(a_layer->GetImage(), b_layer->GetImage()))
          return false;
        break;
      default:
        NOTREACHED();
        return true;
    }

    a_layer = a_layer->Next();
    b_layer = b_layer->Next();
  }

  // FIXME: Shouldn't this be return !aLayer && !bLayer; ?
  return true;
}

}  // namespace

bool CSSPropertyEquality::PropertiesEqual(const PropertyHandle& property,
                                          const ComputedStyle& a,
                                          const ComputedStyle& b) {
  if (property.IsCSSCustomProperty()) {
    const AtomicString& name = property.CustomPropertyName();
    return DataEquivalent(a.GetRegisteredVariable(name),
                          b.GetRegisteredVariable(name));
  }
  switch (property.GetCSSProperty().PropertyID()) {
    case CSSPropertyBackgroundColor:
      return a.BackgroundColor() == b.BackgroundColor() &&
             a.VisitedLinkBackgroundColor() == b.VisitedLinkBackgroundColor();
    case CSSPropertyBackgroundImage:
      return FillLayersEqual<CSSPropertyBackgroundImage>(a.BackgroundLayers(),
                                                         b.BackgroundLayers());
    case CSSPropertyBackgroundPositionX:
      return FillLayersEqual<CSSPropertyBackgroundPositionX>(
          a.BackgroundLayers(), b.BackgroundLayers());
    case CSSPropertyBackgroundPositionY:
      return FillLayersEqual<CSSPropertyBackgroundPositionY>(
          a.BackgroundLayers(), b.BackgroundLayers());
    case CSSPropertyBackgroundSize:
      return FillLayersEqual<CSSPropertyBackgroundSize>(a.BackgroundLayers(),
                                                        b.BackgroundLayers());
    case CSSPropertyBaselineShift:
      return a.BaselineShiftValue() == b.BaselineShiftValue();
    case CSSPropertyBorderBottomColor:
      return a.BorderBottomColor() == b.BorderBottomColor() &&
             a.VisitedLinkBorderBottomColor() ==
                 b.VisitedLinkBorderBottomColor();
    case CSSPropertyBorderBottomLeftRadius:
      return a.BorderBottomLeftRadius() == b.BorderBottomLeftRadius();
    case CSSPropertyBorderBottomRightRadius:
      return a.BorderBottomRightRadius() == b.BorderBottomRightRadius();
    case CSSPropertyBorderBottomWidth:
      return a.BorderBottomWidth() == b.BorderBottomWidth();
    case CSSPropertyBorderImageOutset:
      return a.BorderImageOutset() == b.BorderImageOutset();
    case CSSPropertyBorderImageSlice:
      return a.BorderImageSlices() == b.BorderImageSlices();
    case CSSPropertyBorderImageSource:
      return DataEquivalent(a.BorderImageSource(), b.BorderImageSource());
    case CSSPropertyBorderImageWidth:
      return a.BorderImageWidth() == b.BorderImageWidth();
    case CSSPropertyBorderLeftColor:
      return a.BorderLeftColor() == b.BorderLeftColor() &&
             a.VisitedLinkBorderLeftColor() == b.VisitedLinkBorderLeftColor();
    case CSSPropertyBorderLeftWidth:
      return a.BorderLeftWidth() == b.BorderLeftWidth();
    case CSSPropertyBorderRightColor:
      return a.BorderRightColor() == b.BorderRightColor() &&
             a.VisitedLinkBorderRightColor() == b.VisitedLinkBorderRightColor();
    case CSSPropertyBorderRightWidth:
      return a.BorderRightWidth() == b.BorderRightWidth();
    case CSSPropertyBorderTopColor:
      return a.BorderTopColor() == b.BorderTopColor() &&
             a.VisitedLinkBorderTopColor() == b.VisitedLinkBorderTopColor();
    case CSSPropertyBorderTopLeftRadius:
      return a.BorderTopLeftRadius() == b.BorderTopLeftRadius();
    case CSSPropertyBorderTopRightRadius:
      return a.BorderTopRightRadius() == b.BorderTopRightRadius();
    case CSSPropertyBorderTopWidth:
      return a.BorderTopWidth() == b.BorderTopWidth();
    case CSSPropertyBottom:
      return a.Bottom() == b.Bottom();
    case CSSPropertyBoxShadow:
      return DataEquivalent(a.BoxShadow(), b.BoxShadow());
    case CSSPropertyCaretColor:
      return a.CaretColor() == b.CaretColor() &&
             a.VisitedLinkCaretColor() == b.VisitedLinkCaretColor();
    case CSSPropertyClip:
      return a.Clip() == b.Clip();
    case CSSPropertyColor:
      return a.GetColor() == b.GetColor() &&
             a.VisitedLinkColor() == b.VisitedLinkColor();
    case CSSPropertyFill: {
      const SVGComputedStyle& a_svg = a.SvgStyle();
      const SVGComputedStyle& b_svg = b.SvgStyle();
      return a_svg.FillPaint().EqualTypeOrColor(b_svg.FillPaint()) &&
             a_svg.VisitedLinkFillPaint().EqualTypeOrColor(
                 b_svg.VisitedLinkFillPaint());
    }
    case CSSPropertyFillOpacity:
      return a.FillOpacity() == b.FillOpacity();
    case CSSPropertyFlexBasis:
      return a.FlexBasis() == b.FlexBasis();
    case CSSPropertyFlexGrow:
      return a.FlexGrow() == b.FlexGrow();
    case CSSPropertyFlexShrink:
      return a.FlexShrink() == b.FlexShrink();
    case CSSPropertyFloodColor:
      return a.FloodColor() == b.FloodColor();
    case CSSPropertyFloodOpacity:
      return a.FloodOpacity() == b.FloodOpacity();
    case CSSPropertyFontSize:
      // CSSPropertyFontSize: Must pass a specified size to setFontSize if Text
      // Autosizing is enabled, but a computed size if text zoom is enabled (if
      // neither is enabled it's irrelevant as they're probably the same).
      // FIXME: Should we introduce an option to pass the computed font size
      // here, allowing consumers to enable text zoom rather than Text
      // Autosizing? See http://crbug.com/227545.
      return a.SpecifiedFontSize() == b.SpecifiedFontSize();
    case CSSPropertyFontSizeAdjust:
      return a.FontSizeAdjust() == b.FontSizeAdjust();
    case CSSPropertyFontStretch:
      return a.GetFontStretch() == b.GetFontStretch();
    case CSSPropertyFontVariationSettings:
      return DataEquivalent(a.GetFontDescription().VariationSettings(),
                            b.GetFontDescription().VariationSettings());
    case CSSPropertyFontWeight:
      return a.GetFontWeight() == b.GetFontWeight();
    case CSSPropertyHeight:
      return a.Height() == b.Height();
    case CSSPropertyLeft:
      return a.Left() == b.Left();
    case CSSPropertyLetterSpacing:
      return a.LetterSpacing() == b.LetterSpacing();
    case CSSPropertyLightingColor:
      return a.LightingColor() == b.LightingColor();
    case CSSPropertyLineHeight:
      return a.SpecifiedLineHeight() == b.SpecifiedLineHeight();
    case CSSPropertyListStyleImage:
      return DataEquivalent(a.ListStyleImage(), b.ListStyleImage());
    case CSSPropertyMarginBottom:
      return a.MarginBottom() == b.MarginBottom();
    case CSSPropertyMarginLeft:
      return a.MarginLeft() == b.MarginLeft();
    case CSSPropertyMarginRight:
      return a.MarginRight() == b.MarginRight();
    case CSSPropertyMarginTop:
      return a.MarginTop() == b.MarginTop();
    case CSSPropertyMaxHeight:
      return a.MaxHeight() == b.MaxHeight();
    case CSSPropertyMaxWidth:
      return a.MaxWidth() == b.MaxWidth();
    case CSSPropertyMinHeight:
      return a.MinHeight() == b.MinHeight();
    case CSSPropertyMinWidth:
      return a.MinWidth() == b.MinWidth();
    case CSSPropertyObjectPosition:
      return a.ObjectPosition() == b.ObjectPosition();
    case CSSPropertyOffsetAnchor:
      return a.OffsetAnchor() == b.OffsetAnchor();
    case CSSPropertyOffsetDistance:
      return a.OffsetDistance() == b.OffsetDistance();
    case CSSPropertyOffsetPath:
      return DataEquivalent(a.OffsetPath(), b.OffsetPath());
    case CSSPropertyOffsetPosition:
      return a.OffsetPosition() == b.OffsetPosition();
    case CSSPropertyOffsetRotate:
      return a.OffsetRotate() == b.OffsetRotate();
    case CSSPropertyOpacity:
      return a.Opacity() == b.Opacity();
    case CSSPropertyOrder:
      return a.Order() == b.Order();
    case CSSPropertyOrphans:
      return a.Orphans() == b.Orphans();
    case CSSPropertyOutlineColor:
      return a.OutlineColor() == b.OutlineColor() &&
             a.VisitedLinkOutlineColor() == b.VisitedLinkOutlineColor();
    case CSSPropertyOutlineOffset:
      return a.OutlineOffset() == b.OutlineOffset();
    case CSSPropertyOutlineWidth:
      return a.OutlineWidth() == b.OutlineWidth();
    case CSSPropertyPaddingBottom:
      return a.PaddingBottom() == b.PaddingBottom();
    case CSSPropertyPaddingLeft:
      return a.PaddingLeft() == b.PaddingLeft();
    case CSSPropertyPaddingRight:
      return a.PaddingRight() == b.PaddingRight();
    case CSSPropertyPaddingTop:
      return a.PaddingTop() == b.PaddingTop();
    case CSSPropertyRight:
      return a.Right() == b.Right();
    case CSSPropertyShapeImageThreshold:
      return a.ShapeImageThreshold() == b.ShapeImageThreshold();
    case CSSPropertyShapeMargin:
      return a.ShapeMargin() == b.ShapeMargin();
    case CSSPropertyShapeOutside:
      return DataEquivalent(a.ShapeOutside(), b.ShapeOutside());
    case CSSPropertyStopColor:
      return a.StopColor() == b.StopColor();
    case CSSPropertyStopOpacity:
      return a.StopOpacity() == b.StopOpacity();
    case CSSPropertyStroke: {
      const SVGComputedStyle& a_svg = a.SvgStyle();
      const SVGComputedStyle& b_svg = b.SvgStyle();
      return a_svg.StrokePaint().EqualTypeOrColor(b_svg.StrokePaint()) &&
             a_svg.VisitedLinkStrokePaint().EqualTypeOrColor(
                 b_svg.VisitedLinkStrokePaint());
    }
    case CSSPropertyStrokeDasharray:
      return a.StrokeDashArray() == b.StrokeDashArray();
    case CSSPropertyStrokeDashoffset:
      return a.StrokeDashOffset() == b.StrokeDashOffset();
    case CSSPropertyStrokeMiterlimit:
      return a.StrokeMiterLimit() == b.StrokeMiterLimit();
    case CSSPropertyStrokeOpacity:
      return a.StrokeOpacity() == b.StrokeOpacity();
    case CSSPropertyStrokeWidth:
      return a.StrokeWidth() == b.StrokeWidth();
    case CSSPropertyTextDecorationColor:
      return a.TextDecorationColor() == b.TextDecorationColor() &&
             a.VisitedLinkTextDecorationColor() ==
                 b.VisitedLinkTextDecorationColor();
    case CSSPropertyTextDecorationSkipInk:
      return a.TextDecorationSkipInk() == b.TextDecorationSkipInk();
    case CSSPropertyTextIndent:
      return a.TextIndent() == b.TextIndent();
    case CSSPropertyTextShadow:
      return DataEquivalent(a.TextShadow(), b.TextShadow());
    case CSSPropertyTextSizeAdjust:
      return a.GetTextSizeAdjust() == b.GetTextSizeAdjust();
    case CSSPropertyTop:
      return a.Top() == b.Top();
    case CSSPropertyVerticalAlign:
      return a.VerticalAlign() == b.VerticalAlign() &&
             (a.VerticalAlign() != EVerticalAlign::kLength ||
              a.GetVerticalAlignLength() == b.GetVerticalAlignLength());
    case CSSPropertyVisibility:
      return a.Visibility() == b.Visibility();
    case CSSPropertyWebkitBorderHorizontalSpacing:
      return a.HorizontalBorderSpacing() == b.HorizontalBorderSpacing();
    case CSSPropertyWebkitBorderVerticalSpacing:
      return a.VerticalBorderSpacing() == b.VerticalBorderSpacing();
    case CSSPropertyClipPath:
      return DataEquivalent(a.ClipPath(), b.ClipPath());
    case CSSPropertyColumnCount:
      return a.ColumnCount() == b.ColumnCount();
    case CSSPropertyColumnGap:
      return a.ColumnGap() == b.ColumnGap();
    case CSSPropertyRowGap:
      return a.RowGap() == b.RowGap();
    case CSSPropertyColumnRuleColor:
      return a.ColumnRuleColor() == b.ColumnRuleColor() &&
             a.VisitedLinkColumnRuleColor() == b.VisitedLinkColumnRuleColor();
    case CSSPropertyColumnRuleWidth:
      return a.ColumnRuleWidth() == b.ColumnRuleWidth();
    case CSSPropertyColumnWidth:
      return a.ColumnWidth() == b.ColumnWidth();
    case CSSPropertyFilter:
      return a.Filter() == b.Filter();
    case CSSPropertyBackdropFilter:
      return a.BackdropFilter() == b.BackdropFilter();
    case CSSPropertyWebkitMaskBoxImageOutset:
      return a.MaskBoxImageOutset() == b.MaskBoxImageOutset();
    case CSSPropertyWebkitMaskBoxImageSlice:
      return a.MaskBoxImageSlices() == b.MaskBoxImageSlices();
    case CSSPropertyWebkitMaskBoxImageSource:
      return DataEquivalent(a.MaskBoxImageSource(), b.MaskBoxImageSource());
    case CSSPropertyWebkitMaskBoxImageWidth:
      return a.MaskBoxImageWidth() == b.MaskBoxImageWidth();
    case CSSPropertyWebkitMaskImage:
      return DataEquivalent(a.MaskImage(), b.MaskImage());
    case CSSPropertyWebkitMaskPositionX:
      return FillLayersEqual<CSSPropertyWebkitMaskPositionX>(a.MaskLayers(),
                                                             b.MaskLayers());
    case CSSPropertyWebkitMaskPositionY:
      return FillLayersEqual<CSSPropertyWebkitMaskPositionY>(a.MaskLayers(),
                                                             b.MaskLayers());
    case CSSPropertyWebkitMaskSize:
      return FillLayersEqual<CSSPropertyWebkitMaskSize>(a.MaskLayers(),
                                                        b.MaskLayers());
    case CSSPropertyPerspective:
      return a.Perspective() == b.Perspective();
    case CSSPropertyPerspectiveOrigin:
      return a.PerspectiveOriginX() == b.PerspectiveOriginX() &&
             a.PerspectiveOriginY() == b.PerspectiveOriginY();
    case CSSPropertyWebkitTextStrokeColor:
      return a.TextStrokeColor() == b.TextStrokeColor() &&
             a.VisitedLinkTextStrokeColor() == b.VisitedLinkTextStrokeColor();
    case CSSPropertyTransform:
      return a.Transform() == b.Transform();
    case CSSPropertyTranslate:
      return DataEquivalent<TransformOperation>(a.Translate(), b.Translate());
    case CSSPropertyRotate:
      return DataEquivalent<TransformOperation>(a.Rotate(), b.Rotate());
    case CSSPropertyScale:
      return DataEquivalent<TransformOperation>(a.Scale(), b.Scale());
    case CSSPropertyTransformOrigin:
      return a.TransformOriginX() == b.TransformOriginX() &&
             a.TransformOriginY() == b.TransformOriginY() &&
             a.TransformOriginZ() == b.TransformOriginZ();
    case CSSPropertyWebkitPerspectiveOriginX:
      return a.PerspectiveOriginX() == b.PerspectiveOriginX();
    case CSSPropertyWebkitPerspectiveOriginY:
      return a.PerspectiveOriginY() == b.PerspectiveOriginY();
    case CSSPropertyWebkitTransformOriginX:
      return a.TransformOriginX() == b.TransformOriginX();
    case CSSPropertyWebkitTransformOriginY:
      return a.TransformOriginY() == b.TransformOriginY();
    case CSSPropertyWebkitTransformOriginZ:
      return a.TransformOriginZ() == b.TransformOriginZ();
    case CSSPropertyWidows:
      return a.Widows() == b.Widows();
    case CSSPropertyWidth:
      return a.Width() == b.Width();
    case CSSPropertyWordSpacing:
      return a.WordSpacing() == b.WordSpacing();
    case CSSPropertyD:
      return DataEquivalent(a.SvgStyle().D(), b.SvgStyle().D());
    case CSSPropertyCx:
      return a.SvgStyle().Cx() == b.SvgStyle().Cx();
    case CSSPropertyCy:
      return a.SvgStyle().Cy() == b.SvgStyle().Cy();
    case CSSPropertyX:
      return a.SvgStyle().X() == b.SvgStyle().X();
    case CSSPropertyY:
      return a.SvgStyle().Y() == b.SvgStyle().Y();
    case CSSPropertyR:
      return a.SvgStyle().R() == b.SvgStyle().R();
    case CSSPropertyRx:
      return a.SvgStyle().Rx() == b.SvgStyle().Rx();
    case CSSPropertyRy:
      return a.SvgStyle().Ry() == b.SvgStyle().Ry();
    case CSSPropertyZIndex:
      return a.HasAutoZIndex() == b.HasAutoZIndex() &&
             (a.HasAutoZIndex() || a.ZIndex() == b.ZIndex());
    default:
      NOTREACHED();
      return true;
  }
}

}  // namespace blink
