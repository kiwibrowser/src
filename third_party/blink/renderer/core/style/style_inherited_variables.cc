// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/style/style_inherited_variables.h"

#include "third_party/blink/renderer/core/style/data_equivalency.h"

namespace blink {

bool StyleInheritedVariables::operator==(
    const StyleInheritedVariables& other) const {
  // It's technically possible for divergent roots to be value-equal,
  // but unlikely. This equality operator is used for optimization purposes
  // so it's OK to be occasionally wrong.
  // TODO(shanestephens): Rename this to something that indicates it may not
  // always return equality.
  if (root_ != other.root_)
    return false;

  if (data_.size() != other.data_.size())
    return false;

  for (const auto& iter : data_) {
    scoped_refptr<CSSVariableData> other_data = other.data_.at(iter.key);
    if (!DataEquivalent(iter.value, other_data))
      return false;
  }

  return true;
}

StyleInheritedVariables::StyleInheritedVariables(
    StyleInheritedVariables& other) {
  if (!other.root_) {
    root_ = &other;
  } else {
    data_ = other.data_;
    registered_data_ = other.registered_data_;
    root_ = other.root_;
  }
}

CSSVariableData* StyleInheritedVariables::GetVariable(
    const AtomicString& name) const {
  auto result = data_.find(name);
  if (result == data_.end() && root_)
    return root_->GetVariable(name);
  if (result == data_.end())
    return nullptr;
  return result->value.get();
}

void StyleInheritedVariables::SetRegisteredVariable(
    const AtomicString& name,
    const CSSValue* parsed_value) {
  registered_data_.Set(name, const_cast<CSSValue*>(parsed_value));
}

const CSSValue* StyleInheritedVariables::RegisteredVariable(
    const AtomicString& name) const {
  auto result = registered_data_.find(name);
  if (result != registered_data_.end())
    return result->value.Get();
  if (root_)
    return root_->RegisteredVariable(name);
  return nullptr;
}

void StyleInheritedVariables::RemoveVariable(const AtomicString& name) {
  data_.Set(name, nullptr);
  auto iterator = registered_data_.find(name);
  if (iterator != registered_data_.end())
    iterator->value = nullptr;
}

std::unique_ptr<HashMap<AtomicString, scoped_refptr<CSSVariableData>>>
StyleInheritedVariables::GetVariables() const {
  std::unique_ptr<HashMap<AtomicString, scoped_refptr<CSSVariableData>>> result;
  if (root_) {
    result.reset(new HashMap<AtomicString, scoped_refptr<CSSVariableData>>(
        root_->data_));
    for (auto it = data_.begin(); it != data_.end(); ++it)
      result->Set(it->key, it->value);
  } else {
    result.reset(
        new HashMap<AtomicString, scoped_refptr<CSSVariableData>>(data_));
  }
  return result;
}

}  // namespace blink
