// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/underlying_value_owner.h"

#include <memory>

namespace blink {

struct NullValueWrapper {
  NullValueWrapper() : value(nullptr) {}
  const InterpolationValue value;
};

const InterpolationValue& UnderlyingValueOwner::Value() const {
  DEFINE_STATIC_LOCAL(NullValueWrapper, null_value_wrapper, ());
  return *this ? *value_ : null_value_wrapper.value;
}

void UnderlyingValueOwner::Set(std::nullptr_t) {
  type_ = nullptr;
  value_owner_.Clear();
  value_ = nullptr;
}

void UnderlyingValueOwner::Set(const InterpolationType& type,
                               const InterpolationValue& value) {
  DCHECK(value);
  type_ = &type;
  // By clearing m_valueOwner we will perform a copy before attempting to mutate
  // m_value, thus upholding the const contract for this instance of
  // interpolationValue.
  value_owner_.Clear();
  value_ = &value;
}

void UnderlyingValueOwner::Set(const InterpolationType& type,
                               InterpolationValue&& value) {
  DCHECK(value);
  type_ = &type;
  value_owner_ = std::move(value);
  value_ = &value_owner_;
}

void UnderlyingValueOwner::Set(std::unique_ptr<TypedInterpolationValue> value) {
  if (value)
    Set(value->GetType(), std::move(value->MutableValue()));
  else
    Set(nullptr);
}

void UnderlyingValueOwner::Set(const TypedInterpolationValue* value) {
  if (value)
    Set(value->GetType(), value->Value());
  else
    Set(nullptr);
}

InterpolationValue& UnderlyingValueOwner::MutableValue() {
  DCHECK(type_ && value_);
  if (!value_owner_) {
    value_owner_ = value_->Clone();
    value_ = &value_owner_;
  }
  return value_owner_;
}

}  // namespace blink
