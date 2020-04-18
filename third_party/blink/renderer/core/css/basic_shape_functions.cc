/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/css/basic_shape_functions.h"

#include "third_party/blink/renderer/core/css/css_basic_shape_values.h"
#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/css/css_primitive_value_mappings.h"
#include "third_party/blink/renderer/core/css/css_ray_value.h"
#include "third_party/blink/renderer/core/css/css_value_pair.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver_state.h"
#include "third_party/blink/renderer/core/style/basic_shapes.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/core/style/style_ray.h"

namespace blink {

static StyleRay::RaySize KeywordToRaySize(CSSValueID id) {
  switch (id) {
    case CSSValueClosestSide:
      return StyleRay::RaySize::kClosestSide;
    case CSSValueClosestCorner:
      return StyleRay::RaySize::kClosestCorner;
    case CSSValueFarthestSide:
      return StyleRay::RaySize::kFarthestSide;
    case CSSValueFarthestCorner:
      return StyleRay::RaySize::kFarthestCorner;
    case CSSValueSides:
      return StyleRay::RaySize::kSides;
    default:
      NOTREACHED();
      return StyleRay::RaySize::kClosestSide;
  }
}

static CSSValueID RaySizeToKeyword(StyleRay::RaySize size) {
  switch (size) {
    case StyleRay::RaySize::kClosestSide:
      return CSSValueClosestSide;
    case StyleRay::RaySize::kClosestCorner:
      return CSSValueClosestCorner;
    case StyleRay::RaySize::kFarthestSide:
      return CSSValueFarthestSide;
    case StyleRay::RaySize::kFarthestCorner:
      return CSSValueFarthestCorner;
    case StyleRay::RaySize::kSides:
      return CSSValueSides;
  }
  NOTREACHED();
  return CSSValueInvalid;
}

static CSSValue* ValueForCenterCoordinate(
    const ComputedStyle& style,
    const BasicShapeCenterCoordinate& center,
    EBoxOrient orientation) {
  if (center.GetDirection() == BasicShapeCenterCoordinate::kTopLeft)
    return CSSValue::Create(center.length(), style.EffectiveZoom());

  CSSValueID keyword =
      orientation == EBoxOrient::kHorizontal ? CSSValueRight : CSSValueBottom;

  return CSSValuePair::Create(
      CSSIdentifierValue::Create(keyword),
      CSSValue::Create(center.length(), style.EffectiveZoom()),
      CSSValuePair::kDropIdenticalValues);
}

static CSSValuePair* ValueForLengthSize(const LengthSize& length_size,
                                        const ComputedStyle& style) {
  return CSSValuePair::Create(
      CSSValue::Create(length_size.Width(), style.EffectiveZoom()),
      CSSValue::Create(length_size.Height(), style.EffectiveZoom()),
      CSSValuePair::kKeepIdenticalValues);
}

static CSSValue* BasicShapeRadiusToCSSValue(const ComputedStyle& style,
                                            const BasicShapeRadius& radius) {
  switch (radius.GetType()) {
    case BasicShapeRadius::kValue:
      return CSSValue::Create(radius.Value(), style.EffectiveZoom());
    case BasicShapeRadius::kClosestSide:
      return CSSIdentifierValue::Create(CSSValueClosestSide);
    case BasicShapeRadius::kFarthestSide:
      return CSSIdentifierValue::Create(CSSValueFarthestSide);
  }

  NOTREACHED();
  return nullptr;
}

CSSValue* ValueForBasicShape(const ComputedStyle& style,
                             const BasicShape* basic_shape) {
  switch (basic_shape->GetType()) {
    case BasicShape::kStyleRayType: {
      const StyleRay& ray = ToStyleRay(*basic_shape);
      return cssvalue::CSSRayValue::Create(
          *CSSPrimitiveValue::Create(ray.Angle(),
                                     CSSPrimitiveValue::UnitType::kDegrees),
          *CSSIdentifierValue::Create(RaySizeToKeyword(ray.Size())),
          (ray.Contain() ? CSSIdentifierValue::Create(CSSValueContain)
                         : nullptr));
    }

    case BasicShape::kStylePathType:
      return ToStylePath(basic_shape)->ComputedCSSValue();

    case BasicShape::kBasicShapeCircleType: {
      const BasicShapeCircle* circle = ToBasicShapeCircle(basic_shape);
      cssvalue::CSSBasicShapeCircleValue* circle_value =
          cssvalue::CSSBasicShapeCircleValue::Create();

      circle_value->SetCenterX(ValueForCenterCoordinate(
          style, circle->CenterX(), EBoxOrient::kHorizontal));
      circle_value->SetCenterY(ValueForCenterCoordinate(
          style, circle->CenterY(), EBoxOrient::kVertical));
      circle_value->SetRadius(
          BasicShapeRadiusToCSSValue(style, circle->Radius()));
      return circle_value;
    }
    case BasicShape::kBasicShapeEllipseType: {
      const BasicShapeEllipse* ellipse = ToBasicShapeEllipse(basic_shape);
      cssvalue::CSSBasicShapeEllipseValue* ellipse_value =
          cssvalue::CSSBasicShapeEllipseValue::Create();

      ellipse_value->SetCenterX(ValueForCenterCoordinate(
          style, ellipse->CenterX(), EBoxOrient::kHorizontal));
      ellipse_value->SetCenterY(ValueForCenterCoordinate(
          style, ellipse->CenterY(), EBoxOrient::kVertical));
      ellipse_value->SetRadiusX(
          BasicShapeRadiusToCSSValue(style, ellipse->RadiusX()));
      ellipse_value->SetRadiusY(
          BasicShapeRadiusToCSSValue(style, ellipse->RadiusY()));
      return ellipse_value;
    }
    case BasicShape::kBasicShapePolygonType: {
      const BasicShapePolygon* polygon = ToBasicShapePolygon(basic_shape);
      cssvalue::CSSBasicShapePolygonValue* polygon_value =
          cssvalue::CSSBasicShapePolygonValue::Create();

      polygon_value->SetWindRule(polygon->GetWindRule());
      const Vector<Length>& values = polygon->Values();
      for (unsigned i = 0; i < values.size(); i += 2) {
        polygon_value->AppendPoint(
            CSSPrimitiveValue::Create(values.at(i), style.EffectiveZoom()),
            CSSPrimitiveValue::Create(values.at(i + 1), style.EffectiveZoom()));
      }
      return polygon_value;
    }
    case BasicShape::kBasicShapeInsetType: {
      const BasicShapeInset* inset = ToBasicShapeInset(basic_shape);
      cssvalue::CSSBasicShapeInsetValue* inset_value =
          cssvalue::CSSBasicShapeInsetValue::Create();

      inset_value->SetTop(
          CSSPrimitiveValue::Create(inset->Top(), style.EffectiveZoom()));
      inset_value->SetRight(
          CSSPrimitiveValue::Create(inset->Right(), style.EffectiveZoom()));
      inset_value->SetBottom(
          CSSPrimitiveValue::Create(inset->Bottom(), style.EffectiveZoom()));
      inset_value->SetLeft(
          CSSPrimitiveValue::Create(inset->Left(), style.EffectiveZoom()));

      inset_value->SetTopLeftRadius(
          ValueForLengthSize(inset->TopLeftRadius(), style));
      inset_value->SetTopRightRadius(
          ValueForLengthSize(inset->TopRightRadius(), style));
      inset_value->SetBottomRightRadius(
          ValueForLengthSize(inset->BottomRightRadius(), style));
      inset_value->SetBottomLeftRadius(
          ValueForLengthSize(inset->BottomLeftRadius(), style));

      return inset_value;
    }
    default:
      return nullptr;
  }
}

static Length ConvertToLength(const StyleResolverState& state,
                              const CSSPrimitiveValue* value) {
  if (!value)
    return Length(0, kFixed);
  return value->ConvertToLength(state.CssToLengthConversionData());
}

static LengthSize ConvertToLengthSize(const StyleResolverState& state,
                                      const CSSValuePair* value) {
  if (!value)
    return LengthSize(Length(0, kFixed), Length(0, kFixed));

  return LengthSize(
      ConvertToLength(state, &ToCSSPrimitiveValue(value->First())),
      ConvertToLength(state, &ToCSSPrimitiveValue(value->Second())));
}

static BasicShapeCenterCoordinate ConvertToCenterCoordinate(
    const StyleResolverState& state,
    CSSValue* value) {
  BasicShapeCenterCoordinate::Direction direction;
  Length offset = Length(0, kFixed);

  CSSValueID keyword = CSSValueTop;
  if (!value) {
    keyword = CSSValueCenter;
  } else if (value->IsIdentifierValue()) {
    keyword = ToCSSIdentifierValue(value)->GetValueID();
  } else if (value->IsValuePair()) {
    keyword = ToCSSIdentifierValue(ToCSSValuePair(value)->First()).GetValueID();
    offset = ConvertToLength(
        state, &ToCSSPrimitiveValue(ToCSSValuePair(value)->Second()));
  } else {
    offset = ConvertToLength(state, ToCSSPrimitiveValue(value));
  }

  switch (keyword) {
    case CSSValueTop:
    case CSSValueLeft:
      direction = BasicShapeCenterCoordinate::kTopLeft;
      break;
    case CSSValueRight:
    case CSSValueBottom:
      direction = BasicShapeCenterCoordinate::kBottomRight;
      break;
    case CSSValueCenter:
      direction = BasicShapeCenterCoordinate::kTopLeft;
      offset = Length(50, kPercent);
      break;
    default:
      NOTREACHED();
      direction = BasicShapeCenterCoordinate::kTopLeft;
      break;
  }

  return BasicShapeCenterCoordinate(direction, offset);
}

static BasicShapeRadius CssValueToBasicShapeRadius(
    const StyleResolverState& state,
    const CSSValue* radius) {
  if (!radius)
    return BasicShapeRadius(BasicShapeRadius::kClosestSide);

  if (radius->IsIdentifierValue()) {
    switch (ToCSSIdentifierValue(radius)->GetValueID()) {
      case CSSValueClosestSide:
        return BasicShapeRadius(BasicShapeRadius::kClosestSide);
      case CSSValueFarthestSide:
        return BasicShapeRadius(BasicShapeRadius::kFarthestSide);
      default:
        NOTREACHED();
        break;
    }
  }

  return BasicShapeRadius(ConvertToLength(state, ToCSSPrimitiveValue(radius)));
}

scoped_refptr<BasicShape> BasicShapeForValue(
    const StyleResolverState& state,
    const CSSValue& basic_shape_value) {
  scoped_refptr<BasicShape> basic_shape;

  if (basic_shape_value.IsBasicShapeCircleValue()) {
    const cssvalue::CSSBasicShapeCircleValue& circle_value =
        cssvalue::ToCSSBasicShapeCircleValue(basic_shape_value);
    scoped_refptr<BasicShapeCircle> circle = BasicShapeCircle::Create();

    circle->SetCenterX(
        ConvertToCenterCoordinate(state, circle_value.CenterX()));
    circle->SetCenterY(
        ConvertToCenterCoordinate(state, circle_value.CenterY()));
    circle->SetRadius(CssValueToBasicShapeRadius(state, circle_value.Radius()));

    basic_shape = std::move(circle);
  } else if (basic_shape_value.IsBasicShapeEllipseValue()) {
    const cssvalue::CSSBasicShapeEllipseValue& ellipse_value =
        cssvalue::ToCSSBasicShapeEllipseValue(basic_shape_value);
    scoped_refptr<BasicShapeEllipse> ellipse = BasicShapeEllipse::Create();

    ellipse->SetCenterX(
        ConvertToCenterCoordinate(state, ellipse_value.CenterX()));
    ellipse->SetCenterY(
        ConvertToCenterCoordinate(state, ellipse_value.CenterY()));
    ellipse->SetRadiusX(
        CssValueToBasicShapeRadius(state, ellipse_value.RadiusX()));
    ellipse->SetRadiusY(
        CssValueToBasicShapeRadius(state, ellipse_value.RadiusY()));

    basic_shape = std::move(ellipse);
  } else if (basic_shape_value.IsBasicShapePolygonValue()) {
    const cssvalue::CSSBasicShapePolygonValue& polygon_value =
        cssvalue::ToCSSBasicShapePolygonValue(basic_shape_value);
    scoped_refptr<BasicShapePolygon> polygon = BasicShapePolygon::Create();

    polygon->SetWindRule(polygon_value.GetWindRule());
    const HeapVector<Member<CSSPrimitiveValue>>& values =
        polygon_value.Values();
    for (unsigned i = 0; i < values.size(); i += 2)
      polygon->AppendPoint(ConvertToLength(state, values.at(i).Get()),
                           ConvertToLength(state, values.at(i + 1).Get()));

    basic_shape = std::move(polygon);
  } else if (basic_shape_value.IsBasicShapeInsetValue()) {
    const cssvalue::CSSBasicShapeInsetValue& rect_value =
        cssvalue::ToCSSBasicShapeInsetValue(basic_shape_value);
    scoped_refptr<BasicShapeInset> rect = BasicShapeInset::Create();

    rect->SetTop(ConvertToLength(state, rect_value.Top()));
    rect->SetRight(ConvertToLength(state, rect_value.Right()));
    rect->SetBottom(ConvertToLength(state, rect_value.Bottom()));
    rect->SetLeft(ConvertToLength(state, rect_value.Left()));

    rect->SetTopLeftRadius(
        ConvertToLengthSize(state, rect_value.TopLeftRadius()));
    rect->SetTopRightRadius(
        ConvertToLengthSize(state, rect_value.TopRightRadius()));
    rect->SetBottomRightRadius(
        ConvertToLengthSize(state, rect_value.BottomRightRadius()));
    rect->SetBottomLeftRadius(
        ConvertToLengthSize(state, rect_value.BottomLeftRadius()));

    basic_shape = std::move(rect);
  } else if (basic_shape_value.IsRayValue()) {
    const cssvalue::CSSRayValue& ray_value =
        cssvalue::ToCSSRayValue(basic_shape_value);
    float angle = ray_value.Angle().ComputeDegrees();
    StyleRay::RaySize size = KeywordToRaySize(ray_value.Size().GetValueID());
    bool contain = !!ray_value.Contain();
    basic_shape = StyleRay::Create(angle, size, contain);
  } else {
    NOTREACHED();
  }

  return basic_shape;
}

FloatPoint FloatPointForCenterCoordinate(
    const BasicShapeCenterCoordinate& center_x,
    const BasicShapeCenterCoordinate& center_y,
    FloatSize box_size) {
  float x = FloatValueForLength(center_x.ComputedLength(), box_size.Width());
  float y = FloatValueForLength(center_y.ComputedLength(), box_size.Height());
  return FloatPoint(x, y);
}

}  // namespace blink
