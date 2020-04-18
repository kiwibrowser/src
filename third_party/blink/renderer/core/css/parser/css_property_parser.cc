// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/parser/css_property_parser.h"

#include "third_party/blink/renderer/core/css/css_inherited_value.h"
#include "third_party/blink/renderer/core/css/css_initial_value.h"
#include "third_party/blink/renderer/core/css/css_pending_substitution_value.h"
#include "third_party/blink/renderer/core/css/css_unicode_range_value.h"
#include "third_party/blink/renderer/core/css/css_unset_value.h"
#include "third_party/blink/renderer/core/css/css_variable_reference_value.h"
#include "third_party/blink/renderer/core/css/hash_tools.h"
#include "third_party/blink/renderer/core/css/parser/at_rule_descriptor_parser.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_local_context.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/parser/css_variable_parser.h"
#include "third_party/blink/renderer/core/css/properties/css_parsing_utils.h"
#include "third_party/blink/renderer/core/css/properties/css_property.h"
#include "third_party/blink/renderer/core/css/properties/shorthand.h"
#include "third_party/blink/renderer/core/style_property_shorthand.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

using namespace CSSPropertyParserHelpers;

class CSSIdentifierValue;

CSSPropertyParser::CSSPropertyParser(
    const CSSParserTokenRange& range,
    const CSSParserContext* context,
    HeapVector<CSSPropertyValue, 256>* parsed_properties)
    : range_(range), context_(context), parsed_properties_(parsed_properties) {
  range_.ConsumeWhitespace();
}

bool CSSPropertyParser::ParseValue(
    CSSPropertyID unresolved_property,
    bool important,
    const CSSParserTokenRange& range,
    const CSSParserContext* context,
    HeapVector<CSSPropertyValue, 256>& parsed_properties,
    StyleRule::RuleType rule_type) {
  int parsed_properties_size = parsed_properties.size();

  CSSPropertyParser parser(range, context, &parsed_properties);
  CSSPropertyID resolved_property = resolveCSSPropertyID(unresolved_property);
  bool parse_success;
  if (rule_type == StyleRule::kViewport) {
    parse_success =
        (RuntimeEnabledFeatures::CSSViewportEnabled() ||
         IsUASheetBehavior(context->Mode())) &&
        parser.ParseViewportDescriptor(resolved_property, important);
  } else if (rule_type == StyleRule::kFontFace) {
    parse_success = parser.ParseFontFaceDescriptor(resolved_property);
  } else {
    parse_success = parser.ParseValueStart(unresolved_property, important);
  }

  // This doesn't count UA style sheets
  if (parse_success)
    context->Count(context->Mode(), unresolved_property);

  if (!parse_success)
    parsed_properties.Shrink(parsed_properties_size);

  return parse_success;
}

const CSSValue* CSSPropertyParser::ParseSingleValue(
    CSSPropertyID property,
    const CSSParserTokenRange& range,
    const CSSParserContext* context) {
  DCHECK(context);
  CSSPropertyParser parser(range, context, nullptr);
  const CSSValue* value = ParseLonghand(property, CSSPropertyInvalid,
                                        *parser.context_, parser.range_);
  if (!value || !parser.range_.AtEnd())
    return nullptr;
  return value;
}

bool CSSPropertyParser::ParseValueStart(CSSPropertyID unresolved_property,
                                        bool important) {
  if (ConsumeCSSWideKeyword(unresolved_property, important))
    return true;

  CSSParserTokenRange original_range = range_;
  CSSPropertyID property_id = resolveCSSPropertyID(unresolved_property);
  const CSSProperty& property = CSSProperty::Get(property_id);
  // If a CSSPropertyID is only a known descriptor (@fontface, @viewport), not a
  // style property, it will not be a valid declaration.
  if (!property.IsProperty())
    return false;
  bool is_shorthand = property.IsShorthand();
  DCHECK(context_);
  if (is_shorthand) {
    // Variable references will fail to parse here and will fall out to the
    // variable ref parser below.
    if (ToShorthand(property).ParseShorthand(
            important, range_, *context_,
            CSSParserLocalContext(isPropertyAlias(unresolved_property),
                                  property_id),
            *parsed_properties_))
      return true;
  } else {
    if (const CSSValue* parsed_value = ParseLonghand(
            unresolved_property, CSSPropertyInvalid, *context_, range_)) {
      if (range_.AtEnd()) {
        AddProperty(property_id, CSSPropertyInvalid, *parsed_value, important,
                    IsImplicitProperty::kNotImplicit, *parsed_properties_);
        return true;
      }
    }
  }

  if (CSSVariableParser::ContainsValidVariableReferences(original_range)) {
    bool is_animation_tainted = false;
    CSSVariableReferenceValue* variable = CSSVariableReferenceValue::Create(
        CSSVariableData::Create(original_range, is_animation_tainted, true),
        *context_);

    if (is_shorthand) {
      const CSSPendingSubstitutionValue& pending_value =
          *CSSPendingSubstitutionValue::Create(property_id, variable);
      AddExpandedPropertyForValue(property_id, pending_value, important,
                                  *parsed_properties_);
    } else {
      AddProperty(property_id, CSSPropertyInvalid, *variable, important,
                  IsImplicitProperty::kNotImplicit, *parsed_properties_);
    }
    return true;
  }

  return false;
}

template <typename CharacterType>
static CSSPropertyID UnresolvedCSSPropertyID(const CharacterType* property_name,
                                             unsigned length) {
  if (length == 0)
    return CSSPropertyInvalid;
  if (length >= 2 && property_name[0] == '-' && property_name[1] == '-')
    return CSSPropertyVariable;
  if (length > maxCSSPropertyNameLength)
    return CSSPropertyInvalid;

  char buffer[maxCSSPropertyNameLength + 1];  // 1 for null character

  for (unsigned i = 0; i != length; ++i) {
    CharacterType c = property_name[i];
    if (c == 0 || c >= 0x7F)
      return CSSPropertyInvalid;  // illegal character
    buffer[i] = ToASCIILower(c);
  }
  buffer[length] = '\0';

  const char* name = buffer;
  const Property* hash_table_entry = FindProperty(name, length);
  if (!hash_table_entry)
    return CSSPropertyInvalid;
  CSSPropertyID property = static_cast<CSSPropertyID>(hash_table_entry->id);
  if (!CSSProperty::Get(resolveCSSPropertyID(property)).IsEnabled())
    return CSSPropertyInvalid;
  return property;
}

CSSPropertyID unresolvedCSSPropertyID(const String& string) {
  unsigned length = string.length();
  return string.Is8Bit()
             ? UnresolvedCSSPropertyID(string.Characters8(), length)
             : UnresolvedCSSPropertyID(string.Characters16(), length);
}

CSSPropertyID UnresolvedCSSPropertyID(StringView string) {
  unsigned length = string.length();
  return string.Is8Bit()
             ? UnresolvedCSSPropertyID(string.Characters8(), length)
             : UnresolvedCSSPropertyID(string.Characters16(), length);
}

template <typename CharacterType>
static CSSValueID CssValueKeywordID(const CharacterType* value_keyword,
                                    unsigned length) {
  char buffer[maxCSSValueKeywordLength + 1];  // 1 for null character

  for (unsigned i = 0; i != length; ++i) {
    CharacterType c = value_keyword[i];
    if (c == 0 || c >= 0x7F)
      return CSSValueInvalid;  // illegal character
    buffer[i] = WTF::ToASCIILower(c);
  }
  buffer[length] = '\0';

  const Value* hash_table_entry = FindValue(buffer, length);
  return hash_table_entry ? static_cast<CSSValueID>(hash_table_entry->id)
                          : CSSValueInvalid;
}

CSSValueID CssValueKeywordID(StringView string) {
  unsigned length = string.length();
  if (!length)
    return CSSValueInvalid;
  if (length > maxCSSValueKeywordLength)
    return CSSValueInvalid;

  return string.Is8Bit() ? CssValueKeywordID(string.Characters8(), length)
                         : CssValueKeywordID(string.Characters16(), length);
}

bool CSSPropertyParser::ConsumeCSSWideKeyword(CSSPropertyID unresolved_property,
                                              bool important) {
  CSSParserTokenRange range_copy = range_;
  CSSValueID id = range_copy.ConsumeIncludingWhitespace().Id();
  if (!range_copy.AtEnd())
    return false;

  CSSValue* value = nullptr;
  if (id == CSSValueInitial)
    value = CSSInitialValue::Create();
  else if (id == CSSValueInherit)
    value = CSSInheritedValue::Create();
  else if (id == CSSValueUnset)
    value = cssvalue::CSSUnsetValue::Create();
  else
    return false;

  CSSPropertyID property = resolveCSSPropertyID(unresolved_property);
  const StylePropertyShorthand& shorthand = shorthandForProperty(property);
  if (!shorthand.length()) {
    if (!CSSProperty::Get(property).IsProperty())
      return false;
    AddProperty(property, CSSPropertyInvalid, *value, important,
                IsImplicitProperty::kNotImplicit, *parsed_properties_);
  } else {
    AddExpandedPropertyForValue(property, *value, important,
                                *parsed_properties_);
  }
  range_ = range_copy;
  return true;
}

static CSSValue* ConsumeSingleViewportDescriptor(
    CSSParserTokenRange& range,
    CSSPropertyID prop_id,
    CSSParserMode css_parser_mode) {
  CSSValueID id = range.Peek().Id();
  switch (prop_id) {
    case CSSPropertyMinWidth:
    case CSSPropertyMaxWidth:
    case CSSPropertyMinHeight:
    case CSSPropertyMaxHeight:
      if (id == CSSValueAuto || id == CSSValueInternalExtendToZoom)
        return ConsumeIdent(range);
      return ConsumeLengthOrPercent(range, css_parser_mode,
                                    kValueRangeNonNegative);
    case CSSPropertyMinZoom:
    case CSSPropertyMaxZoom:
    case CSSPropertyZoom: {
      if (id == CSSValueAuto)
        return ConsumeIdent(range);
      CSSValue* parsed_value = ConsumeNumber(range, kValueRangeNonNegative);
      if (parsed_value)
        return parsed_value;
      return ConsumePercent(range, kValueRangeNonNegative);
    }
    case CSSPropertyUserZoom:
      return ConsumeIdent<CSSValueZoom, CSSValueFixed>(range);
    case CSSPropertyOrientation:
      return ConsumeIdent<CSSValueAuto, CSSValuePortrait, CSSValueLandscape>(
          range);
    case CSSPropertyViewportFit:
      return ConsumeIdent<CSSValueAuto, CSSValueContain, CSSValueCover>(range);
    default:
      NOTREACHED();
      break;
  }

  NOTREACHED();
  return nullptr;
}

bool CSSPropertyParser::ParseViewportDescriptor(CSSPropertyID prop_id,
                                                bool important) {
  DCHECK(RuntimeEnabledFeatures::CSSViewportEnabled() ||
         IsUASheetBehavior(context_->Mode()));

  switch (prop_id) {
    case CSSPropertyWidth: {
      CSSValue* min_width = ConsumeSingleViewportDescriptor(
          range_, CSSPropertyMinWidth, context_->Mode());
      if (!min_width)
        return false;
      CSSValue* max_width = min_width;
      if (!range_.AtEnd()) {
        max_width = ConsumeSingleViewportDescriptor(range_, CSSPropertyMaxWidth,
                                                    context_->Mode());
      }
      if (!max_width || !range_.AtEnd())
        return false;
      AddProperty(CSSPropertyMinWidth, CSSPropertyInvalid, *min_width,
                  important, IsImplicitProperty::kNotImplicit,
                  *parsed_properties_);
      AddProperty(CSSPropertyMaxWidth, CSSPropertyInvalid, *max_width,
                  important, IsImplicitProperty::kNotImplicit,
                  *parsed_properties_);
      return true;
    }
    case CSSPropertyHeight: {
      CSSValue* min_height = ConsumeSingleViewportDescriptor(
          range_, CSSPropertyMinHeight, context_->Mode());
      if (!min_height)
        return false;
      CSSValue* max_height = min_height;
      if (!range_.AtEnd()) {
        max_height = ConsumeSingleViewportDescriptor(
            range_, CSSPropertyMaxHeight, context_->Mode());
      }
      if (!max_height || !range_.AtEnd())
        return false;
      AddProperty(CSSPropertyMinHeight, CSSPropertyInvalid, *min_height,
                  important, IsImplicitProperty::kNotImplicit,
                  *parsed_properties_);
      AddProperty(CSSPropertyMaxHeight, CSSPropertyInvalid, *max_height,
                  important, IsImplicitProperty::kNotImplicit,
                  *parsed_properties_);
      return true;
    }
    case CSSPropertyViewportFit:
    case CSSPropertyMinWidth:
    case CSSPropertyMaxWidth:
    case CSSPropertyMinHeight:
    case CSSPropertyMaxHeight:
    case CSSPropertyMinZoom:
    case CSSPropertyMaxZoom:
    case CSSPropertyZoom:
    case CSSPropertyUserZoom:
    case CSSPropertyOrientation: {
      CSSValue* parsed_value =
          ConsumeSingleViewportDescriptor(range_, prop_id, context_->Mode());
      if (!parsed_value || !range_.AtEnd())
        return false;
      AddProperty(prop_id, CSSPropertyInvalid, *parsed_value, important,
                  IsImplicitProperty::kNotImplicit, *parsed_properties_);
      return true;
    }
    default:
      return false;
  }
}

bool CSSPropertyParser::ParseFontFaceDescriptor(
    CSSPropertyID resolved_property) {
  // TODO(meade): This function should eventually take an AtRuleDescriptorID.
  const AtRuleDescriptorID id =
      CSSPropertyIDAsAtRuleDescriptor(resolved_property);
  DCHECK_NE(id, AtRuleDescriptorID::Invalid);
  CSSValue* parsed_value =
      AtRuleDescriptorParser::ParseFontFaceDescriptor(id, range_, *context_);
  if (!parsed_value)
    return false;

  AddProperty(resolved_property, CSSPropertyInvalid /* current_shorthand */,
              *parsed_value, false /* important */,
              IsImplicitProperty::kNotImplicit, *parsed_properties_);
  return true;
}

}  // namespace blink
