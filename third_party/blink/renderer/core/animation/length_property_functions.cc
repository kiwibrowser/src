// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/length_property_functions.h"

#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

ValueRange LengthPropertyFunctions::GetValueRange(const CSSProperty& property) {
  switch (property.PropertyID()) {
    case CSSPropertyBorderBottomWidth:
    case CSSPropertyBorderLeftWidth:
    case CSSPropertyBorderRightWidth:
    case CSSPropertyBorderTopWidth:
    case CSSPropertyFlexBasis:
    case CSSPropertyHeight:
    case CSSPropertyLineHeight:
    case CSSPropertyMaxHeight:
    case CSSPropertyMaxWidth:
    case CSSPropertyMinHeight:
    case CSSPropertyMinWidth:
    case CSSPropertyOutlineWidth:
    case CSSPropertyPaddingBottom:
    case CSSPropertyPaddingLeft:
    case CSSPropertyPaddingRight:
    case CSSPropertyPaddingTop:
    case CSSPropertyPerspective:
    case CSSPropertyR:
    case CSSPropertyRx:
    case CSSPropertyRy:
    case CSSPropertyShapeMargin:
    case CSSPropertyStrokeWidth:
    case CSSPropertyWebkitBorderHorizontalSpacing:
    case CSSPropertyWebkitBorderVerticalSpacing:
    case CSSPropertyColumnGap:
    case CSSPropertyRowGap:
    case CSSPropertyColumnWidth:
    case CSSPropertyWidth:
      return kValueRangeNonNegative;
    default:
      return kValueRangeAll;
  }
}

bool LengthPropertyFunctions::IsZoomedLength(const CSSProperty& property) {
  return property.PropertyID() != CSSPropertyStrokeWidth;
}

bool LengthPropertyFunctions::GetPixelsForKeyword(const CSSProperty& property,
                                                  CSSValueID value_id,
                                                  double& result) {
  switch (property.PropertyID()) {
    case CSSPropertyBaselineShift:
      if (value_id == CSSValueBaseline) {
        result = 0;
        return true;
      }
      return false;
    case CSSPropertyBorderBottomWidth:
    case CSSPropertyBorderLeftWidth:
    case CSSPropertyBorderRightWidth:
    case CSSPropertyBorderTopWidth:
    case CSSPropertyColumnRuleWidth:
    case CSSPropertyOutlineWidth:
      if (value_id == CSSValueThin) {
        result = 1;
        return true;
      }
      if (value_id == CSSValueMedium) {
        result = 3;
        return true;
      }
      if (value_id == CSSValueThick) {
        result = 5;
        return true;
      }
      return false;
    case CSSPropertyLetterSpacing:
    case CSSPropertyWordSpacing:
      if (value_id == CSSValueNormal) {
        result = 0;
        return true;
      }
      return false;
    default:
      return false;
  }
}

bool LengthPropertyFunctions::GetInitialLength(const CSSProperty& property,
                                               Length& result) {
  switch (property.PropertyID()) {
    // The computed value of "initial" for the following properties is 0px if
    // the associated *-style property resolves to "none" or "hidden".
    // - border-width:
    //   https://drafts.csswg.org/css-backgrounds-3/#the-border-width
    // - outline-width: https://drafts.csswg.org/css-ui-3/#outline-width
    // - column-rule-width: https://drafts.csswg.org/css-multicol-1/#crw
    // We ignore this value adjustment for animations and use the wrong value
    // for hidden widths to avoid having to restart our animations based on the
    // computed *-style values. This is acceptable since animations running on
    // hidden widths are unobservable to the user, even via getComputedStyle().
    case CSSPropertyBorderBottomWidth:
    case CSSPropertyBorderLeftWidth:
    case CSSPropertyBorderRightWidth:
    case CSSPropertyBorderTopWidth:
      result = Length(ComputedStyleInitialValues::InitialBorderWidth(), kFixed);
      return true;
    case CSSPropertyOutlineWidth:
      result =
          Length(ComputedStyleInitialValues::InitialOutlineWidth(), kFixed);
      return true;
    case CSSPropertyColumnRuleWidth:
      result =
          Length(ComputedStyleInitialValues::InitialColumnRuleWidth(), kFixed);
      return true;

    default:
      return GetLength(property, ComputedStyle::InitialStyle(), result);
  }
}

bool LengthPropertyFunctions::GetLength(const CSSProperty& property,
                                        const ComputedStyle& style,
                                        Length& result) {
  switch (property.PropertyID()) {
    case CSSPropertyBottom:
      result = style.Bottom();
      return true;
    case CSSPropertyCx:
      result = style.SvgStyle().Cx();
      return true;
    case CSSPropertyCy:
      result = style.SvgStyle().Cy();
      return true;
    case CSSPropertyFlexBasis:
      result = style.FlexBasis();
      return true;
    case CSSPropertyHeight:
      result = style.Height();
      return true;
    case CSSPropertyLeft:
      result = style.Left();
      return true;
    case CSSPropertyMarginBottom:
      result = style.MarginBottom();
      return true;
    case CSSPropertyMarginLeft:
      result = style.MarginLeft();
      return true;
    case CSSPropertyMarginRight:
      result = style.MarginRight();
      return true;
    case CSSPropertyMarginTop:
      result = style.MarginTop();
      return true;
    case CSSPropertyMaxHeight:
      result = style.MaxHeight();
      return true;
    case CSSPropertyMaxWidth:
      result = style.MaxWidth();
      return true;
    case CSSPropertyMinHeight:
      result = style.MinHeight();
      return true;
    case CSSPropertyMinWidth:
      result = style.MinWidth();
      return true;
    case CSSPropertyOffsetDistance:
      result = style.OffsetDistance();
      return true;
    case CSSPropertyPaddingBottom:
      result = style.PaddingBottom();
      return true;
    case CSSPropertyPaddingLeft:
      result = style.PaddingLeft();
      return true;
    case CSSPropertyPaddingRight:
      result = style.PaddingRight();
      return true;
    case CSSPropertyPaddingTop:
      result = style.PaddingTop();
      return true;
    case CSSPropertyR:
      result = style.SvgStyle().R();
      return true;
    case CSSPropertyRight:
      result = style.Right();
      return true;
    case CSSPropertyRx:
      result = style.SvgStyle().Rx();
      return true;
    case CSSPropertyRy:
      result = style.SvgStyle().Ry();
      return true;
    case CSSPropertyShapeMargin:
      result = style.ShapeMargin();
      return true;
    case CSSPropertyStrokeDashoffset:
      result = style.StrokeDashOffset();
      return true;
    case CSSPropertyTextIndent:
      result = style.TextIndent();
      return true;
    case CSSPropertyTop:
      result = style.Top();
      return true;
    case CSSPropertyWebkitPerspectiveOriginX:
      result = style.PerspectiveOriginX();
      return true;
    case CSSPropertyWebkitPerspectiveOriginY:
      result = style.PerspectiveOriginY();
      return true;
    case CSSPropertyWebkitTransformOriginX:
      result = style.TransformOriginX();
      return true;
    case CSSPropertyWebkitTransformOriginY:
      result = style.TransformOriginY();
      return true;
    case CSSPropertyWidth:
      result = style.Width();
      return true;
    case CSSPropertyX:
      result = style.SvgStyle().X();
      return true;
    case CSSPropertyY:
      result = style.SvgStyle().Y();
      return true;

    case CSSPropertyBorderBottomWidth:
      result = Length(style.BorderBottomWidth(), kFixed);
      return true;
    case CSSPropertyBorderLeftWidth:
      result = Length(style.BorderLeftWidth(), kFixed);
      return true;
    case CSSPropertyBorderRightWidth:
      result = Length(style.BorderRightWidth(), kFixed);
      return true;
    case CSSPropertyBorderTopWidth:
      result = Length(style.BorderTopWidth(), kFixed);
      return true;
    case CSSPropertyLetterSpacing:
      result = Length(style.LetterSpacing(), kFixed);
      return true;
    case CSSPropertyOutlineOffset:
      result = Length(style.OutlineOffset(), kFixed);
      return true;
    case CSSPropertyOutlineWidth:
      result = Length(style.OutlineWidth(), kFixed);
      return true;
    case CSSPropertyWebkitBorderHorizontalSpacing:
      result = Length(style.HorizontalBorderSpacing(), kFixed);
      return true;
    case CSSPropertyWebkitBorderVerticalSpacing:
      result = Length(style.VerticalBorderSpacing(), kFixed);
      return true;
    case CSSPropertyRowGap:
      if (style.RowGap().IsNormal())
        return false;
      result = style.RowGap().GetLength();
      return true;
    case CSSPropertyColumnGap:
      if (style.ColumnGap().IsNormal())
        return false;
      result = style.ColumnGap().GetLength();
      return true;
    case CSSPropertyColumnRuleWidth:
      result = Length(style.ColumnRuleWidth(), kFixed);
      return true;
    case CSSPropertyWebkitTransformOriginZ:
      result = Length(style.TransformOriginZ(), kFixed);
      return true;
    case CSSPropertyWordSpacing:
      result = Length(style.WordSpacing(), kFixed);
      return true;

    case CSSPropertyBaselineShift:
      if (style.BaselineShift() != BS_LENGTH)
        return false;
      result = style.BaselineShiftValue();
      return true;
    case CSSPropertyLineHeight:
      // Percent Lengths are used to represent numbers on line-height.
      if (style.SpecifiedLineHeight().IsPercentOrCalc())
        return false;
      result = style.SpecifiedLineHeight();
      return true;
    case CSSPropertyPerspective:
      if (!style.HasPerspective())
        return false;
      result = Length(style.Perspective(), kFixed);
      return true;
    case CSSPropertyStrokeWidth:
      DCHECK(!IsZoomedLength(CSSProperty::Get(CSSPropertyStrokeWidth)));
      result = style.StrokeWidth().length();
      return true;
    case CSSPropertyVerticalAlign:
      if (style.VerticalAlign() != EVerticalAlign::kLength)
        return false;
      result = style.GetVerticalAlignLength();
      return true;
    case CSSPropertyColumnWidth:
      if (style.HasAutoColumnWidth())
        return false;
      result = Length(style.ColumnWidth(), kFixed);
      return true;
    default:
      return false;
  }
}

bool LengthPropertyFunctions::SetLength(const CSSProperty& property,
                                        ComputedStyle& style,
                                        const Length& value) {
  switch (property.PropertyID()) {
    // Setters that take a Length value.
    case CSSPropertyBaselineShift:
      style.SetBaselineShiftValue(value);
      return true;
    case CSSPropertyBottom:
      style.SetBottom(value);
      return true;
    case CSSPropertyCx:
      style.SetCx(value);
      return true;
    case CSSPropertyCy:
      style.SetCy(value);
      return true;
    case CSSPropertyFlexBasis:
      style.SetFlexBasis(value);
      return true;
    case CSSPropertyHeight:
      style.SetHeight(value);
      return true;
    case CSSPropertyLeft:
      style.SetLeft(value);
      return true;
    case CSSPropertyMarginBottom:
      style.SetMarginBottom(value);
      return true;
    case CSSPropertyMarginLeft:
      style.SetMarginLeft(value);
      return true;
    case CSSPropertyMarginRight:
      style.SetMarginRight(value);
      return true;
    case CSSPropertyMarginTop:
      style.SetMarginTop(value);
      return true;
    case CSSPropertyMaxHeight:
      style.SetMaxHeight(value);
      return true;
    case CSSPropertyMaxWidth:
      style.SetMaxWidth(value);
      return true;
    case CSSPropertyMinHeight:
      style.SetMinHeight(value);
      return true;
    case CSSPropertyMinWidth:
      style.SetMinWidth(value);
      return true;
    case CSSPropertyOffsetDistance:
      style.SetOffsetDistance(value);
      return true;
    case CSSPropertyPaddingBottom:
      style.SetPaddingBottom(value);
      return true;
    case CSSPropertyPaddingLeft:
      style.SetPaddingLeft(value);
      return true;
    case CSSPropertyPaddingRight:
      style.SetPaddingRight(value);
      return true;
    case CSSPropertyPaddingTop:
      style.SetPaddingTop(value);
      return true;
    case CSSPropertyR:
      style.SetR(value);
      return true;
    case CSSPropertyRx:
      style.SetRx(value);
      return true;
    case CSSPropertyRy:
      style.SetRy(value);
      return true;
    case CSSPropertyRight:
      style.SetRight(value);
      return true;
    case CSSPropertyShapeMargin:
      style.SetShapeMargin(value);
      return true;
    case CSSPropertyStrokeDashoffset:
      style.SetStrokeDashOffset(value);
      return true;
    case CSSPropertyTop:
      style.SetTop(value);
      return true;
    case CSSPropertyWidth:
      style.SetWidth(value);
      return true;
    case CSSPropertyWebkitPerspectiveOriginX:
      style.SetPerspectiveOriginX(value);
      return true;
    case CSSPropertyWebkitPerspectiveOriginY:
      style.SetPerspectiveOriginY(value);
      return true;
    case CSSPropertyWebkitTransformOriginX:
      style.SetTransformOriginX(value);
      return true;
    case CSSPropertyWebkitTransformOriginY:
      style.SetTransformOriginY(value);
      return true;
    case CSSPropertyX:
      style.SetX(value);
      return true;
    case CSSPropertyY:
      style.SetY(value);
      return true;

    case CSSPropertyLineHeight:
      // Percent Lengths are used to represent numbers on line-height.
      if (value.IsPercentOrCalc())
        return false;
      style.SetLineHeight(value);
      return true;

    // TODO(alancutter): Support setters that take a numeric value (need to
    // resolve percentages).
    case CSSPropertyBorderBottomWidth:
    case CSSPropertyBorderLeftWidth:
    case CSSPropertyBorderRightWidth:
    case CSSPropertyBorderTopWidth:
    case CSSPropertyLetterSpacing:
    case CSSPropertyOutlineOffset:
    case CSSPropertyOutlineWidth:
    case CSSPropertyPerspective:
    case CSSPropertyStrokeWidth:
    case CSSPropertyVerticalAlign:
    case CSSPropertyWebkitBorderHorizontalSpacing:
    case CSSPropertyWebkitBorderVerticalSpacing:
    case CSSPropertyColumnGap:
    case CSSPropertyRowGap:
    case CSSPropertyColumnRuleWidth:
    case CSSPropertyColumnWidth:
    case CSSPropertyWebkitTransformOriginZ:
    case CSSPropertyWordSpacing:
      return false;

    default:
      return false;
  }
}

}  // namespace blink
