// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/css_color_interpolation_type.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/core/animation/color_property_functions.h"
#include "third_party/blink/renderer/core/css/css_color_value.h"
#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver_state.h"
#include "third_party/blink/renderer/core/layout/layout_theme.h"

namespace blink {

using namespace cssvalue;

enum InterpolableColorIndex : unsigned {
  kRed,
  kGreen,
  kBlue,
  kAlpha,
  kCurrentcolor,
  kWebkitActivelink,
  kWebkitLink,
  kQuirkInherit,
  kInterpolableColorIndexCount,
};

static std::unique_ptr<InterpolableValue> CreateInterpolableColorForIndex(
    InterpolableColorIndex index) {
  DCHECK_LT(index, kInterpolableColorIndexCount);
  std::unique_ptr<InterpolableList> list =
      InterpolableList::Create(kInterpolableColorIndexCount);
  for (unsigned i = 0; i < kInterpolableColorIndexCount; i++)
    list->Set(i, InterpolableNumber::Create(i == index));
  return std::move(list);
}

std::unique_ptr<InterpolableValue>
CSSColorInterpolationType::CreateInterpolableColor(const Color& color) {
  std::unique_ptr<InterpolableList> list =
      InterpolableList::Create(kInterpolableColorIndexCount);
  list->Set(kRed, InterpolableNumber::Create(color.Red() * color.Alpha()));
  list->Set(kGreen, InterpolableNumber::Create(color.Green() * color.Alpha()));
  list->Set(kBlue, InterpolableNumber::Create(color.Blue() * color.Alpha()));
  list->Set(kAlpha, InterpolableNumber::Create(color.Alpha()));
  list->Set(kCurrentcolor, InterpolableNumber::Create(0));
  list->Set(kWebkitActivelink, InterpolableNumber::Create(0));
  list->Set(kWebkitLink, InterpolableNumber::Create(0));
  list->Set(kQuirkInherit, InterpolableNumber::Create(0));
  return std::move(list);
}

std::unique_ptr<InterpolableValue>
CSSColorInterpolationType::CreateInterpolableColor(CSSValueID keyword) {
  switch (keyword) {
    case CSSValueCurrentcolor:
      return CreateInterpolableColorForIndex(kCurrentcolor);
    case CSSValueWebkitActivelink:
      return CreateInterpolableColorForIndex(kWebkitActivelink);
    case CSSValueWebkitLink:
      return CreateInterpolableColorForIndex(kWebkitLink);
    case CSSValueInternalQuirkInherit:
      return CreateInterpolableColorForIndex(kQuirkInherit);
    case CSSValueWebkitFocusRingColor:
      return CreateInterpolableColor(LayoutTheme::GetTheme().FocusRingColor());
    default:
      DCHECK(StyleColor::IsColorKeyword(keyword));
      return CreateInterpolableColor(StyleColor::ColorFromKeyword(keyword));
  }
}

std::unique_ptr<InterpolableValue>
CSSColorInterpolationType::CreateInterpolableColor(const StyleColor& color) {
  if (color.IsCurrentColor())
    return CreateInterpolableColorForIndex(kCurrentcolor);
  return CreateInterpolableColor(color.GetColor());
}

std::unique_ptr<InterpolableValue>
CSSColorInterpolationType::MaybeCreateInterpolableColor(const CSSValue& value) {
  if (value.IsColorValue())
    return CreateInterpolableColor(ToCSSColorValue(value).Value());
  if (!value.IsIdentifierValue())
    return nullptr;
  const CSSIdentifierValue& identifier_value = ToCSSIdentifierValue(value);
  if (!StyleColor::IsColorKeyword(identifier_value.GetValueID()))
    return nullptr;
  return CreateInterpolableColor(identifier_value.GetValueID());
}

static void AddPremultipliedColor(double& red,
                                  double& green,
                                  double& blue,
                                  double& alpha,
                                  double fraction,
                                  const Color& color) {
  double color_alpha = color.Alpha();
  red += fraction * color.Red() * color_alpha;
  green += fraction * color.Green() * color_alpha;
  blue += fraction * color.Blue() * color_alpha;
  alpha += fraction * color_alpha;
}

Color CSSColorInterpolationType::ResolveInterpolableColor(
    const InterpolableValue& interpolable_color,
    const StyleResolverState& state,
    bool is_visited,
    bool is_text_decoration) {
  const InterpolableList& list = ToInterpolableList(interpolable_color);
  DCHECK_EQ(list.length(), kInterpolableColorIndexCount);

  double red = ToInterpolableNumber(list.Get(kRed))->Value();
  double green = ToInterpolableNumber(list.Get(kGreen))->Value();
  double blue = ToInterpolableNumber(list.Get(kBlue))->Value();
  double alpha = ToInterpolableNumber(list.Get(kAlpha))->Value();

  if (double currentcolor_fraction =
          ToInterpolableNumber(list.Get(kCurrentcolor))->Value()) {
    auto current_color_getter = is_visited
                                    ? ColorPropertyFunctions::GetVisitedColor
                                    : ColorPropertyFunctions::GetUnvisitedColor;
    StyleColor current_style_color = StyleColor::CurrentColor();
    if (is_text_decoration) {
      current_style_color =
          current_color_getter(CSSProperty::Get(CSSPropertyWebkitTextFillColor),
                               *state.Style())
              .Access();
    }
    if (current_style_color.IsCurrentColor()) {
      current_style_color =
          current_color_getter(CSSProperty::Get(CSSPropertyColor),
                               *state.Style())
              .Access();
    }
    AddPremultipliedColor(red, green, blue, alpha, currentcolor_fraction,
                          current_style_color.GetColor());
  }
  const TextLinkColors& colors = state.GetDocument().GetTextLinkColors();
  if (double webkit_activelink_fraction =
          ToInterpolableNumber(list.Get(kWebkitActivelink))->Value())
    AddPremultipliedColor(red, green, blue, alpha, webkit_activelink_fraction,
                          colors.ActiveLinkColor());
  if (double webkit_link_fraction =
          ToInterpolableNumber(list.Get(kWebkitLink))->Value())
    AddPremultipliedColor(
        red, green, blue, alpha, webkit_link_fraction,
        is_visited ? colors.VisitedLinkColor() : colors.LinkColor());
  if (double quirk_inherit_fraction =
          ToInterpolableNumber(list.Get(kQuirkInherit))->Value())
    AddPremultipliedColor(red, green, blue, alpha, quirk_inherit_fraction,
                          colors.TextColor());

  alpha = clampTo<double>(alpha, 0, 255);
  if (alpha == 0)
    return Color::kTransparent;

  return MakeRGBA(round(red / alpha), round(green / alpha), round(blue / alpha),
                  round(alpha));
}

class InheritedColorChecker
    : public CSSInterpolationType::CSSConversionChecker {
 public:
  static std::unique_ptr<InheritedColorChecker> Create(
      const CSSProperty& property,
      const OptionalStyleColor& color) {
    return base::WrapUnique(new InheritedColorChecker(property, color));
  }

 private:
  InheritedColorChecker(const CSSProperty& property,
                        const OptionalStyleColor& color)
      : property_(property), color_(color) {}

  bool IsValid(const StyleResolverState& state,
               const InterpolationValue& underlying) const final {
    return color_ == ColorPropertyFunctions::GetUnvisitedColor(
                         property_, *state.ParentStyle());
  }

  const CSSProperty& property_;
  const OptionalStyleColor color_;
};

InterpolationValue CSSColorInterpolationType::MaybeConvertNeutral(
    const InterpolationValue&,
    ConversionCheckers&) const {
  return ConvertStyleColorPair(StyleColor(Color::kTransparent),
                               StyleColor(Color::kTransparent));
}

InterpolationValue CSSColorInterpolationType::MaybeConvertInitial(
    const StyleResolverState&,
    ConversionCheckers& conversion_checkers) const {
  OptionalStyleColor initial_color =
      ColorPropertyFunctions::GetInitialColor(CssProperty());
  if (initial_color.IsNull())
    return nullptr;
  return ConvertStyleColorPair(initial_color.Access(), initial_color.Access());
}

InterpolationValue CSSColorInterpolationType::MaybeConvertInherit(
    const StyleResolverState& state,
    ConversionCheckers& conversion_checkers) const {
  if (!state.ParentStyle())
    return nullptr;
  // Visited color can never explicitly inherit from parent visited color so
  // only use the unvisited color.
  OptionalStyleColor inherited_color =
      ColorPropertyFunctions::GetUnvisitedColor(CssProperty(),
                                                *state.ParentStyle());
  conversion_checkers.push_back(
      InheritedColorChecker::Create(CssProperty(), inherited_color));
  return ConvertStyleColorPair(inherited_color, inherited_color);
}

enum InterpolableColorPairIndex : unsigned {
  kUnvisited,
  kVisited,
  kInterpolableColorPairIndexCount,
};

InterpolationValue CSSColorInterpolationType::MaybeConvertValue(
    const CSSValue& value,
    const StyleResolverState* state,
    ConversionCheckers& conversion_checkers) const {
  if (CssProperty().PropertyID() == CSSPropertyColor &&
      value.IsIdentifierValue() &&
      ToCSSIdentifierValue(value).GetValueID() == CSSValueCurrentcolor) {
    DCHECK(state);
    return MaybeConvertInherit(*state, conversion_checkers);
  }

  std::unique_ptr<InterpolableValue> interpolable_color =
      MaybeCreateInterpolableColor(value);
  if (!interpolable_color)
    return nullptr;
  std::unique_ptr<InterpolableList> color_pair =
      InterpolableList::Create(kInterpolableColorPairIndexCount);
  color_pair->Set(kUnvisited, interpolable_color->Clone());
  color_pair->Set(kVisited, std::move(interpolable_color));
  return InterpolationValue(std::move(color_pair));
}

InterpolationValue CSSColorInterpolationType::ConvertStyleColorPair(
    const OptionalStyleColor& unvisited_color,
    const OptionalStyleColor& visited_color) const {
  if (unvisited_color.IsNull() || visited_color.IsNull()) {
    return nullptr;
  }
  std::unique_ptr<InterpolableList> color_pair =
      InterpolableList::Create(kInterpolableColorPairIndexCount);
  color_pair->Set(kUnvisited,
                  CreateInterpolableColor(unvisited_color.Access()));
  color_pair->Set(kVisited, CreateInterpolableColor(visited_color.Access()));
  return InterpolationValue(std::move(color_pair));
}

InterpolationValue
CSSColorInterpolationType::MaybeConvertStandardPropertyUnderlyingValue(
    const ComputedStyle& style) const {
  return ConvertStyleColorPair(
      ColorPropertyFunctions::GetUnvisitedColor(CssProperty(), style),
      ColorPropertyFunctions::GetVisitedColor(CssProperty(), style));
}

void CSSColorInterpolationType::ApplyStandardPropertyValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue*,
    StyleResolverState& state) const {
  const InterpolableList& color_pair = ToInterpolableList(interpolable_value);
  DCHECK_EQ(color_pair.length(), kInterpolableColorPairIndexCount);
  ColorPropertyFunctions::SetUnvisitedColor(
      CssProperty(), *state.Style(),
      ResolveInterpolableColor(
          *color_pair.Get(kUnvisited), state, false,
          CssProperty().PropertyID() == CSSPropertyTextDecorationColor));
  ColorPropertyFunctions::SetVisitedColor(
      CssProperty(), *state.Style(),
      ResolveInterpolableColor(
          *color_pair.Get(kVisited), state, true,
          CssProperty().PropertyID() == CSSPropertyTextDecorationColor));
}

const CSSValue* CSSColorInterpolationType::CreateCSSValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue*,
    const StyleResolverState& state) const {
  const InterpolableList& color_pair = ToInterpolableList(interpolable_value);
  Color color = ResolveInterpolableColor(*color_pair.Get(kUnvisited), state);
  return CSSColorValue::Create(color.Rgb());
}

}  // namespace blink
