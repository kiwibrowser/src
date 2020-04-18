// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/resolver/font_style_resolver.h"

#include "third_party/blink/renderer/core/css/css_to_length_conversion_data.h"
#include "third_party/blink/renderer/core/css/resolver/font_builder.h"
#include "third_party/blink/renderer/core/css/resolver/style_builder_converter.h"
#include "third_party/blink/renderer/platform/fonts/font.h"
#include "third_party/blink/renderer/platform/fonts/font_description.h"

namespace blink {

FontDescription FontStyleResolver::ComputeFont(
    const CSSPropertyValueSet& property_set,
    FontSelector* font_selector) {
  FontBuilder builder(nullptr);

  FontDescription fontDescription;
  Font font(fontDescription);
  font.Update(font_selector);
  CSSToLengthConversionData::FontSizes fontSizes(16, 16, &font);
  CSSToLengthConversionData::ViewportSize viewportSize(0, 0);
  CSSToLengthConversionData conversionData(nullptr, fontSizes, viewportSize, 1);

  // CSSPropertyFontSize
  if (property_set.HasProperty(CSSPropertyFontSize)) {
    builder.SetSize(StyleBuilderConverterBase::ConvertFontSize(
        *property_set.GetPropertyCSSValue(CSSPropertyFontSize), conversionData,
        FontDescription::Size(0, 0.0f, false)));
  }

  // CSSPropertyFontFamily
  if (property_set.HasProperty(CSSPropertyFontFamily)) {
    builder.SetFamilyDescription(StyleBuilderConverterBase::ConvertFontFamily(
        *property_set.GetPropertyCSSValue(CSSPropertyFontFamily), &builder,
        nullptr));
  }

  // CSSPropertyFontStretch
  if (property_set.HasProperty(CSSPropertyFontStretch)) {
    builder.SetStretch(StyleBuilderConverterBase::ConvertFontStretch(
        *property_set.GetPropertyCSSValue(CSSPropertyFontStretch)));
  }

  // CSSPropertyFontStyle
  if (property_set.HasProperty(CSSPropertyFontStyle)) {
    builder.SetStyle(StyleBuilderConverterBase::ConvertFontStyle(
        *property_set.GetPropertyCSSValue(CSSPropertyFontStyle)));
  }

  // CSSPropertyFontVariantCaps
  if (property_set.HasProperty(CSSPropertyFontVariantCaps)) {
    builder.SetVariantCaps(StyleBuilderConverterBase::ConvertFontVariantCaps(
        *property_set.GetPropertyCSSValue(CSSPropertyFontVariantCaps)));
  }

  // CSSPropertyFontWeight
  if (property_set.HasProperty(CSSPropertyFontWeight)) {
    builder.SetWeight(StyleBuilderConverterBase::ConvertFontWeight(
        *property_set.GetPropertyCSSValue(CSSPropertyFontWeight),
        FontBuilder::InitialWeight()));
  }

  builder.UpdateFontDescription(fontDescription);

  return fontDescription;
}

}  // namespace blink
