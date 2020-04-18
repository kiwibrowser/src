/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc.
 * All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007, 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved.
 * (http://www.torchmobile.com/)
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/css/resolver/filter_operation_resolver.h"

#include "third_party/blink/renderer/core/css/css_function_value.h"
#include "third_party/blink/renderer/core/css/css_primitive_value_mappings.h"
#include "third_party/blink/renderer/core/css/css_uri_value.h"
#include "third_party/blink/renderer/core/css/resolver/style_builder_converter.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver_state.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

static const float kOffScreenCanvasEmFontSize = 16.0;
static const float kOffScreenCanvasRemFontSize = 16.0;

FilterOperation::OperationType FilterOperationResolver::FilterOperationForType(
    CSSValueID type) {
  switch (type) {
    case CSSValueGrayscale:
      return FilterOperation::GRAYSCALE;
    case CSSValueSepia:
      return FilterOperation::SEPIA;
    case CSSValueSaturate:
      return FilterOperation::SATURATE;
    case CSSValueHueRotate:
      return FilterOperation::HUE_ROTATE;
    case CSSValueInvert:
      return FilterOperation::INVERT;
    case CSSValueOpacity:
      return FilterOperation::OPACITY;
    case CSSValueBrightness:
      return FilterOperation::BRIGHTNESS;
    case CSSValueContrast:
      return FilterOperation::CONTRAST;
    case CSSValueBlur:
      return FilterOperation::BLUR;
    case CSSValueDropShadow:
      return FilterOperation::DROP_SHADOW;
    default:
      NOTREACHED();
      // FIXME: We shouldn't have a type None since we never create them
      return FilterOperation::NONE;
  }
}

static void CountFilterUse(FilterOperation::OperationType operation_type,
                           const Document& document) {
  // This variable is always reassigned, but MSVC thinks it might be left
  // uninitialized.
  WebFeature feature = WebFeature::kNumberOfFeatures;
  switch (operation_type) {
    case FilterOperation::NONE:
    case FilterOperation::BOX_REFLECT:
      NOTREACHED();
      return;
    case FilterOperation::REFERENCE:
      feature = WebFeature::kCSSFilterReference;
      break;
    case FilterOperation::GRAYSCALE:
      feature = WebFeature::kCSSFilterGrayscale;
      break;
    case FilterOperation::SEPIA:
      feature = WebFeature::kCSSFilterSepia;
      break;
    case FilterOperation::SATURATE:
      feature = WebFeature::kCSSFilterSaturate;
      break;
    case FilterOperation::HUE_ROTATE:
      feature = WebFeature::kCSSFilterHueRotate;
      break;
    case FilterOperation::INVERT:
      feature = WebFeature::kCSSFilterInvert;
      break;
    case FilterOperation::OPACITY:
      feature = WebFeature::kCSSFilterOpacity;
      break;
    case FilterOperation::BRIGHTNESS:
      feature = WebFeature::kCSSFilterBrightness;
      break;
    case FilterOperation::CONTRAST:
      feature = WebFeature::kCSSFilterContrast;
      break;
    case FilterOperation::BLUR:
      feature = WebFeature::kCSSFilterBlur;
      break;
    case FilterOperation::DROP_SHADOW:
      feature = WebFeature::kCSSFilterDropShadow;
      break;
  };
  UseCounter::Count(document, feature);
}

FilterOperations FilterOperationResolver::CreateFilterOperations(
    StyleResolverState& state,
    const CSSValue& in_value) {
  FilterOperations operations;

  if (in_value.IsIdentifierValue()) {
    DCHECK_EQ(ToCSSIdentifierValue(in_value).GetValueID(), CSSValueNone);
    return operations;
  }

  const CSSToLengthConversionData& conversion_data =
      state.CssToLengthConversionData();

  for (auto& curr_value : ToCSSValueList(in_value)) {
    if (curr_value->IsURIValue()) {
      CountFilterUse(FilterOperation::REFERENCE, state.GetDocument());

      const CSSURIValue& url_value = ToCSSURIValue(*curr_value);
      SVGResource* resource =
          state.GetElementStyleResources().GetSVGResourceFromValue(
              state.GetTreeScope(), url_value,
              ElementStyleResources::kAllowExternalResource);
      operations.Operations().push_back(
          ReferenceFilterOperation::Create(url_value.Value(), resource));
      continue;
    }

    const CSSFunctionValue* filter_value = ToCSSFunctionValue(curr_value.Get());
    FilterOperation::OperationType operation_type =
        FilterOperationForType(filter_value->FunctionType());
    CountFilterUse(operation_type, state.GetDocument());
    DCHECK_LE(filter_value->length(), 1u);

    const CSSPrimitiveValue* first_value =
        filter_value->length() && filter_value->Item(0).IsPrimitiveValue()
            ? &ToCSSPrimitiveValue(filter_value->Item(0))
            : nullptr;
    double first_number =
        StyleBuilderConverter::ConvertValueToNumber(filter_value, first_value);

    switch (filter_value->FunctionType()) {
      case CSSValueGrayscale:
      case CSSValueSepia:
      case CSSValueSaturate:
      case CSSValueHueRotate: {
        operations.Operations().push_back(
            BasicColorMatrixFilterOperation::Create(first_number,
                                                    operation_type));
        break;
      }
      case CSSValueInvert:
      case CSSValueBrightness:
      case CSSValueContrast:
      case CSSValueOpacity: {
        operations.Operations().push_back(
            BasicComponentTransferFilterOperation::Create(first_number,
                                                          operation_type));
        break;
      }
      case CSSValueBlur: {
        Length std_deviation = Length(0, kFixed);
        if (filter_value->length() >= 1) {
          std_deviation = first_value->ConvertToLength(conversion_data);
        }
        operations.Operations().push_back(
            BlurFilterOperation::Create(std_deviation));
        break;
      }
      case CSSValueDropShadow: {
        ShadowData shadow = StyleBuilderConverter::ConvertShadow(
            conversion_data, &state, filter_value->Item(0));
        // TODO(fs): Resolve 'currentcolor' when constructing the filter chain.
        if (shadow.GetColor().IsCurrentColor()) {
          shadow.OverrideColor(state.Style()->GetColor());
        }
        operations.Operations().push_back(
            DropShadowFilterOperation::Create(shadow));
        break;
      }
      default:
        NOTREACHED();
        break;
    }
  }

  return operations;
}

FilterOperations FilterOperationResolver::CreateOffscreenFilterOperations(
    const CSSValue& in_value) {
  FilterOperations operations;

  if (in_value.IsIdentifierValue()) {
    DCHECK_EQ(ToCSSIdentifierValue(in_value).GetValueID(), CSSValueNone);
    return operations;
  }

  FontDescription font_description;
  Font font(font_description);
  CSSToLengthConversionData::FontSizes font_sizes(
      kOffScreenCanvasEmFontSize, kOffScreenCanvasRemFontSize, &font);
  CSSToLengthConversionData::ViewportSize viewport_size(0, 0);
  CSSToLengthConversionData conversion_data(nullptr,  // ComputedStyle
                                            font_sizes, viewport_size,
                                            1);  // zoom

  for (auto& curr_value : ToCSSValueList(in_value)) {
    if (curr_value->IsURIValue())
      continue;

    const CSSFunctionValue* filter_value = ToCSSFunctionValue(curr_value.Get());
    FilterOperation::OperationType operation_type =
        FilterOperationForType(filter_value->FunctionType());
    // TODO(fserb): Take an ExecutionContext argument to this function,
    // so we can have workers using UseCounter as well.
    // countFilterUse(operationType, state.document());
    DCHECK_LE(filter_value->length(), 1u);

    const CSSPrimitiveValue* first_value =
        filter_value->length() && filter_value->Item(0).IsPrimitiveValue()
            ? &ToCSSPrimitiveValue(filter_value->Item(0))
            : nullptr;
    double first_number =
        StyleBuilderConverter::ConvertValueToNumber(filter_value, first_value);

    switch (filter_value->FunctionType()) {
      case CSSValueGrayscale:
      case CSSValueSepia:
      case CSSValueSaturate:
      case CSSValueHueRotate: {
        operations.Operations().push_back(
            BasicColorMatrixFilterOperation::Create(first_number,
                                                    operation_type));
        break;
      }
      case CSSValueInvert:
      case CSSValueBrightness:
      case CSSValueContrast:
      case CSSValueOpacity: {
        operations.Operations().push_back(
            BasicComponentTransferFilterOperation::Create(first_number,
                                                          operation_type));
        break;
      }
      case CSSValueBlur: {
        Length std_deviation = Length(0, kFixed);
        if (filter_value->length() >= 1) {
          std_deviation = first_value->ConvertToLength(conversion_data);
        }
        operations.Operations().push_back(
            BlurFilterOperation::Create(std_deviation));
        break;
      }
      case CSSValueDropShadow: {
        ShadowData shadow = StyleBuilderConverter::ConvertShadow(
            conversion_data, nullptr, filter_value->Item(0));
        // For offscreen canvas, the default color is always black.
        if (shadow.GetColor().IsCurrentColor()) {
          shadow.OverrideColor(Color::kBlack);
        }
        operations.Operations().push_back(
            DropShadowFilterOperation::Create(shadow));
        break;
      }
      default:
        NOTREACHED();
        break;
    }
  }
  return operations;
}

}  // namespace blink
