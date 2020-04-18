// Copyright 2016 the chromium authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/cssom/style_property_map.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/css_value_pair.h"
#include "third_party/blink/renderer/core/css/cssom/css_style_value.h"
#include "third_party/blink/renderer/core/css/cssom/cssom_types.h"
#include "third_party/blink/renderer/core/css/cssom/style_value_factory.h"
#include "third_party/blink/renderer/core/css/parser/css_parser.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"
#include "third_party/blink/renderer/core/css/properties/css_property.h"
#include "third_party/blink/renderer/core/style_property_shorthand.h"

namespace blink {

namespace {

CSSValueList* CssValueListForPropertyID(CSSPropertyID property_id) {
  DCHECK(CSSProperty::Get(property_id).IsRepeated());
  char separator = CSSProperty::Get(property_id).RepetitionSeparator();
  switch (separator) {
    case ' ':
      return CSSValueList::CreateSpaceSeparated();
    case ',':
      return CSSValueList::CreateCommaSeparated();
    case '/':
      return CSSValueList::CreateSlashSeparated();
    default:
      NOTREACHED();
      return nullptr;
  }
}

const CSSValue* StyleValueToCSSValue(
    const CSSProperty& property,
    const CSSStyleValue& style_value,
    const ExecutionContext& execution_context) {
  const CSSPropertyID property_id = property.PropertyID();
  if (!CSSOMTypes::PropertyCanTake(property_id, style_value))
    return nullptr;

  if (style_value.GetType() == CSSStyleValue::kUnknownType) {
    return CSSParser::ParseSingleValue(
        property.PropertyID(), style_value.toString(),
        CSSParserContext::Create(execution_context));
  }

  // Handle properties that use ad-hoc structures for their CSSValues:
  // TODO(https://crbug.com/545324): Move this into a method on
  // CSSProperty when there are more of these cases.
  switch (property_id) {
    case CSSPropertyBorderBottomLeftRadius:
    case CSSPropertyBorderBottomRightRadius:
    case CSSPropertyBorderTopLeftRadius:
    case CSSPropertyBorderTopRightRadius: {
      // level 1 only accept single <length-percentages>, but border-radius-*
      // expects pairs.
      const auto* value = style_value.ToCSSValue();
      if (value->IsPrimitiveValue()) {
        return CSSValuePair::Create(value, value,
                                    CSSValuePair::kDropIdenticalValues);
      }
      break;
    }
    case CSSPropertyContain: {
      // level 1 only accepts single values, which are stored internally
      // as a single element list.
      const auto* value = style_value.ToCSSValue();
      if ((value->IsIdentifierValue() && !value->IsCSSWideKeyword()) ||
          value->IsPrimitiveValue()) {
        CSSValueList* list = CSSValueList::CreateSpaceSeparated();
        list->Append(*style_value.ToCSSValue());
        return list;
      }
      break;
    }
    case CSSPropertyFontVariantEastAsian:
    case CSSPropertyFontVariantLigatures:
    case CSSPropertyFontVariantNumeric: {
      // level 1 only accept single keywords, but font-variant-* store
      // them as a list
      if (const auto* value =
              ToCSSIdentifierValueOrNull(style_value.ToCSSValue())) {
        // 'none' and 'normal' are stored as a single value
        if (value->GetValueID() == CSSValueNone ||
            value->GetValueID() == CSSValueNormal) {
          break;
        }

        CSSValueList* list = CSSValueList::CreateSpaceSeparated();
        list->Append(*style_value.ToCSSValue());
        return list;
      }
      break;
    }
    case CSSPropertyGridAutoFlow: {
      // level 1 only accepts single keywords
      const auto* value = style_value.ToCSSValue();
      // single keywords are wrapped in a list.
      if (value->IsIdentifierValue() && !value->IsCSSWideKeyword()) {
        CSSValueList* list = CSSValueList::CreateSpaceSeparated();
        list->Append(*style_value.ToCSSValue());
        return list;
      }
      break;
    }
    case CSSPropertyOffsetRotate: {
      // level 1 only accepts single values, which are stored internally
      // as a single element list.
      const auto* value = style_value.ToCSSValue();
      if ((value->IsIdentifierValue() && !value->IsCSSWideKeyword()) ||
          value->IsPrimitiveValue()) {
        CSSValueList* list = CSSValueList::CreateSpaceSeparated();
        list->Append(*style_value.ToCSSValue());
        return list;
      }
      break;
    }
    case CSSPropertyPaintOrder: {
      // level 1 only accepts single keywords
      const auto* value = style_value.ToCSSValue();
      // only 'normal' is stored as an identifier, the other keywords are
      // wrapped in a list.
      if (value->IsIdentifierValue() && !value->IsCSSWideKeyword() &&
          ToCSSIdentifierValue(value)->GetValueID() != CSSValueNormal) {
        CSSValueList* list = CSSValueList::CreateSpaceSeparated();
        list->Append(*style_value.ToCSSValue());
        return list;
      }
      break;
    }
    case CSSPropertyTextDecorationLine: {
      // level 1 only accepts single keywords
      const auto* value = style_value.ToCSSValue();
      // only 'none' is stored as an identifier, the other keywords are
      // wrapped in a list.
      if (value->IsIdentifierValue() && !value->IsCSSWideKeyword() &&
          ToCSSIdentifierValue(value)->GetValueID() != CSSValueNone) {
        CSSValueList* list = CSSValueList::CreateSpaceSeparated();
        list->Append(*style_value.ToCSSValue());
        return list;
      }
      break;
    }
    case CSSPropertyTextIndent: {
      // level 1 only accepts single values, which are stored internally
      // as a single element list.
      const auto* value = style_value.ToCSSValue();
      if (value->IsPrimitiveValue()) {
        CSSValueList* list = CSSValueList::CreateSpaceSeparated();
        list->Append(*value);
        return list;
      }
      break;
    }
    case CSSPropertyTransitionProperty:
    case CSSPropertyTouchAction: {
      // level 1 only accepts single keywords, which are stored internally
      // as a single element list
      const auto* value = style_value.ToCSSValue();
      if (value->IsIdentifierValue() && !value->IsCSSWideKeyword()) {
        CSSValueList* list = CSSValueList::CreateSpaceSeparated();
        list->Append(*style_value.ToCSSValue());
        return list;
      }
      break;
    }
    default:
      break;
  }

  return style_value.ToCSSValueWithProperty(property_id);
}

const CSSValue* CoerceStyleValueOrString(
    const CSSProperty& property,
    const CSSStyleValueOrString& value,
    const ExecutionContext& execution_context) {
  DCHECK(!property.IsRepeated());

  if (value.IsCSSStyleValue()) {
    if (!value.GetAsCSSStyleValue())
      return nullptr;

    return StyleValueToCSSValue(property, *value.GetAsCSSStyleValue(),
                                execution_context);
  } else {
    DCHECK(value.IsString());
    const auto values = StyleValueFactory::FromString(
        property.PropertyID(), value.GetAsString(),
        CSSParserContext::Create(execution_context));
    if (values.size() != 1U)
      return nullptr;

    return StyleValueToCSSValue(property, *values[0], execution_context);
  }
}

const CSSValue* CoerceStyleValuesOrStrings(
    const CSSProperty& property,
    const HeapVector<CSSStyleValueOrString>& values,
    const ExecutionContext& execution_context) {
  DCHECK(property.IsRepeated());
  if (values.IsEmpty())
    return nullptr;

  const CSSParserContext* parser_context = nullptr;

  HeapVector<Member<const CSSValue>> css_values;
  for (const auto& value : values) {
    if (value.IsCSSStyleValue()) {
      if (!value.GetAsCSSStyleValue())
        return nullptr;

      css_values.push_back(StyleValueToCSSValue(
          property, *value.GetAsCSSStyleValue(), execution_context));
    } else {
      DCHECK(value.IsString());
      if (!parser_context)
        parser_context = CSSParserContext::Create(execution_context);

      const auto subvalues = StyleValueFactory::FromString(
          property.PropertyID(), value.GetAsString(), parser_context);
      if (subvalues.IsEmpty())
        return nullptr;

      for (const auto& subvalue : subvalues) {
        DCHECK(subvalue);
        css_values.push_back(
            StyleValueToCSSValue(property, *subvalue, execution_context));
      }
    }
  }

  CSSValueList* result = CssValueListForPropertyID(property.PropertyID());
  for (const auto& css_value : css_values) {
    if (!css_value)
      return nullptr;
    if (css_value->IsCSSWideKeyword() || css_value->IsVariableReferenceValue())
      return css_values.size() == 1U ? css_value : nullptr;

    result->Append(*css_value);
  }

  return result;
}

}  // namespace

void StylePropertyMap::set(const ExecutionContext* execution_context,
                           const String& property_name,
                           const HeapVector<CSSStyleValueOrString>& values,
                           ExceptionState& exception_state) {
  const CSSPropertyID property_id = cssPropertyID(property_name);
  if (property_id == CSSPropertyInvalid) {
    exception_state.ThrowTypeError("Invalid propertyName: " + property_name);
    return;
  }

  DCHECK(isValidCSSPropertyID(property_id));
  const CSSProperty& property = CSSProperty::Get(property_id);
  if (property.IsShorthand()) {
    if (values.size() != 1) {
      exception_state.ThrowTypeError("Invalid type for property");
      return;
    }

    String css_text;
    if (values[0].IsCSSStyleValue()) {
      CSSStyleValue* style_value = values[0].GetAsCSSStyleValue();
      if (style_value && CSSOMTypes::PropertyCanTake(property_id, *style_value))
        css_text = style_value->toString();
    } else {
      css_text = values[0].GetAsString();
    }

    if (css_text.IsEmpty() ||
        !SetShorthandProperty(property.PropertyID(), css_text,
                              execution_context->GetSecureContextMode()))
      exception_state.ThrowTypeError("Invalid type for property");

    return;
  }

  const CSSValue* result = nullptr;
  if (property.IsRepeated())
    result = CoerceStyleValuesOrStrings(property, values, *execution_context);
  else if (values.size() == 1U)
    result = CoerceStyleValueOrString(property, values[0], *execution_context);

  if (!result) {
    exception_state.ThrowTypeError("Invalid type for property");
    return;
  }

  if (property_id == CSSPropertyVariable)
    SetCustomProperty(AtomicString(property_name), *result);
  else
    SetProperty(property_id, *result);
}

void StylePropertyMap::append(const ExecutionContext* execution_context,
                              const String& property_name,
                              const HeapVector<CSSStyleValueOrString>& values,
                              ExceptionState& exception_state) {
  if (values.IsEmpty())
    return;

  const CSSPropertyID property_id = cssPropertyID(property_name);

  if (property_id == CSSPropertyInvalid) {
    exception_state.ThrowTypeError("Invalid propertyName: " + property_name);
    return;
  }

  if (property_id == CSSPropertyVariable) {
    exception_state.ThrowTypeError(
        "Appending to custom properties is not supported");
    return;
  }

  const CSSProperty& property = CSSProperty::Get(property_id);
  if (!property.IsRepeated()) {
    exception_state.ThrowTypeError("Property does not support multiple values");
    return;
  }

  CSSValueList* current_value = nullptr;
  if (const CSSValue* css_value = GetProperty(property_id)) {
    DCHECK(css_value->IsValueList());
    current_value = ToCSSValueList(css_value)->Copy();
  } else {
    current_value = CssValueListForPropertyID(property_id);
  }

  const CSSValue* result =
      CoerceStyleValuesOrStrings(property, values, *execution_context);
  if (!result || !result->IsValueList()) {
    exception_state.ThrowTypeError("Invalid type for property");
    return;
  }

  for (const auto& value : *ToCSSValueList(result)) {
    current_value->Append(*value);
  }

  SetProperty(property_id, *current_value);
}

void StylePropertyMap::remove(const String& property_name,
                              ExceptionState& exception_state) {
  CSSPropertyID property_id = cssPropertyID(property_name);
  if (property_id == CSSPropertyInvalid) {
    exception_state.ThrowTypeError("Invalid property name: " + property_name);
    return;
  }

  if (property_id == CSSPropertyVariable) {
    RemoveCustomProperty(AtomicString(property_name));
  } else {
    RemoveProperty(property_id);
  }
}

void StylePropertyMap::clear() {
  RemoveAllProperties();
}

}  // namespace blink
