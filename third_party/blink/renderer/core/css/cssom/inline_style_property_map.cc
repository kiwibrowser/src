// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/cssom/inline_style_property_map.h"

#include "third_party/blink/renderer/core/css/css_custom_property_declaration.h"
#include "third_party/blink/renderer/core/css/css_property_value_set.h"
#include "third_party/blink/renderer/core/css/css_variable_reference_value.h"
#include "third_party/blink/renderer/core/css/style_property_serializer.h"

namespace blink {

unsigned int InlineStylePropertyMap::size() {
  const CSSPropertyValueSet* inline_style = owner_element_->InlineStyle();
  return inline_style ? inline_style->PropertyCount() : 0;
}

const CSSValue* InlineStylePropertyMap::GetProperty(CSSPropertyID property_id) {
  const CSSPropertyValueSet* inline_style = owner_element_->InlineStyle();
  return inline_style ? inline_style->GetPropertyCSSValue(property_id)
                      : nullptr;
}

const CSSValue* InlineStylePropertyMap::GetCustomProperty(
    AtomicString property_name) {
  const CSSPropertyValueSet* inline_style = owner_element_->InlineStyle();
  return inline_style ? inline_style->GetPropertyCSSValue(property_name)
                      : nullptr;
}

void InlineStylePropertyMap::SetProperty(CSSPropertyID property_id,
                                         const CSSValue& value) {
  owner_element_->SetInlineStyleProperty(property_id, value);
}

bool InlineStylePropertyMap::SetShorthandProperty(
    CSSPropertyID property_id,
    const String& value,
    SecureContextMode secure_context_mode) {
  DCHECK(CSSProperty::Get(property_id).IsShorthand());
  const auto result = owner_element_->EnsureMutableInlineStyle().SetProperty(
      property_id, value, false /* important */, secure_context_mode);
  return result.did_parse;
}

void InlineStylePropertyMap::SetCustomProperty(
    const AtomicString& property_name,
    const CSSValue& value) {
  DCHECK(value.IsVariableReferenceValue());
  CSSVariableData* variable_data =
      ToCSSVariableReferenceValue(value).VariableDataValue();
  owner_element_->SetInlineStyleProperty(
      CSSPropertyVariable,
      *CSSCustomPropertyDeclaration::Create(property_name, variable_data));
}

void InlineStylePropertyMap::RemoveProperty(CSSPropertyID property_id) {
  owner_element_->RemoveInlineStyleProperty(property_id);
}

void InlineStylePropertyMap::RemoveCustomProperty(
    const AtomicString& property_name) {
  owner_element_->RemoveInlineStyleProperty(property_name);
}

void InlineStylePropertyMap::RemoveAllProperties() {
  owner_element_->RemoveAllInlineStyleProperties();
}

void InlineStylePropertyMap::ForEachProperty(
    const IterationCallback& callback) {
  CSSPropertyValueSet& inline_style_set =
      owner_element_->EnsureMutableInlineStyle();
  for (unsigned i = 0; i < inline_style_set.PropertyCount(); i++) {
    const auto& property_reference = inline_style_set.PropertyAt(i);
    if (property_reference.Id() == CSSPropertyVariable) {
      const auto& decl =
          ToCSSCustomPropertyDeclaration(property_reference.Value());
      callback(decl.GetName(), property_reference.Value());
    } else {
      callback(property_reference.Property().GetPropertyNameAtomicString(),
               property_reference.Value());
    }
  }
}

String InlineStylePropertyMap::SerializationForShorthand(
    const CSSProperty& property) {
  DCHECK(property.IsShorthand());
  if (const CSSPropertyValueSet* inline_style = owner_element_->InlineStyle()) {
    return StylePropertySerializer(*inline_style)
        .GetPropertyValue(property.PropertyID());
  }

  NOTREACHED();
  return "";
}

}  // namespace blink
