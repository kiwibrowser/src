// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_CUSTOM_PROPERTY_DECLARATION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_CUSTOM_PROPERTY_DECLARATION_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/css/css_value.h"
#include "third_party/blink/renderer/core/css/css_variable_data.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

class CSSCustomPropertyDeclaration : public CSSValue {
 public:
  static CSSCustomPropertyDeclaration* Create(
      const AtomicString& name,
      scoped_refptr<CSSVariableData> value) {
    return new CSSCustomPropertyDeclaration(name, std::move(value));
  }

  static CSSCustomPropertyDeclaration* Create(const AtomicString& name,
                                              CSSValueID id) {
    return new CSSCustomPropertyDeclaration(name, id);
  }

  const AtomicString& GetName() const { return name_; }
  CSSVariableData* Value() const { return value_.get(); }

  bool IsInherit(bool is_inherited_property) const {
    return value_id_ == CSSValueInherit ||
           (is_inherited_property && value_id_ == CSSValueUnset);
  }
  bool IsInitial(bool is_inherited_property) const {
    return value_id_ == CSSValueInitial ||
           (!is_inherited_property && value_id_ == CSSValueUnset);
  }

  String CustomCSSText() const;

  bool Equals(const CSSCustomPropertyDeclaration& other) const {
    return this == &other;
  }

  void TraceAfterDispatch(blink::Visitor*);

 private:
  CSSCustomPropertyDeclaration(const AtomicString& name, CSSValueID id)
      : CSSValue(kCustomPropertyDeclarationClass),
        name_(name),
        value_(nullptr),
        value_id_(id) {
    DCHECK(id == CSSValueInherit || id == CSSValueInitial ||
           id == CSSValueUnset);
  }

  CSSCustomPropertyDeclaration(const AtomicString& name,
                               scoped_refptr<CSSVariableData> value)
      : CSSValue(kCustomPropertyDeclarationClass),
        name_(name),
        value_(std::move(value)),
        value_id_(CSSValueInvalid) {}

  const AtomicString name_;
  scoped_refptr<CSSVariableData> value_;
  CSSValueID value_id_;
};

DEFINE_CSS_VALUE_TYPE_CASTS(CSSCustomPropertyDeclaration,
                            IsCustomPropertyDeclaration());

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_CUSTOM_PROPERTY_DECLARATION_H_
