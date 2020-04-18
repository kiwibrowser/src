// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/style/style_non_inherited_variables.h"

#include "third_party/blink/renderer/core/style/data_equivalency.h"

namespace blink {

bool StyleNonInheritedVariables::operator==(
    const StyleNonInheritedVariables& other) const {
  if (data_.size() != other.data_.size())
    return false;

  for (const auto& iter : data_) {
    scoped_refptr<CSSVariableData> other_data = other.data_.at(iter.key);
    if (!DataEquivalent(iter.value, other_data))
      return false;
  }

  return true;
}

CSSVariableData* StyleNonInheritedVariables::GetVariable(
    const AtomicString& name) const {
  return data_.at(name);
}

void StyleNonInheritedVariables::SetRegisteredVariable(
    const AtomicString& name,
    const CSSValue* parsed_value) {
  registered_data_.Set(name, const_cast<CSSValue*>(parsed_value));
}

void StyleNonInheritedVariables::RemoveVariable(const AtomicString& name) {
  data_.Set(name, nullptr);
  registered_data_.Set(name, nullptr);
}

StyleNonInheritedVariables::StyleNonInheritedVariables(
    StyleNonInheritedVariables& other) {
  data_ = other.data_;
  registered_data_ = other.registered_data_;
}

}  // namespace blink
