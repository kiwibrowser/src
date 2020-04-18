// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSSOM_CSS_MATH_VALUE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSSOM_CSS_MATH_VALUE_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/css/cssom/css_numeric_value.h"

namespace blink {

// Represents mathematical operations, acting as nodes in a tree of
// CSSNumericValues. See CSSMathValue.idl for more information about this class.
class CORE_EXPORT CSSMathValue : public CSSNumericValue {
  DEFINE_WRAPPERTYPEINFO();

 public:
  virtual String getOperator() const = 0;

  // From CSSNumericValue.
  bool IsUnitValue() const final { return false; }

  // From CSSStyleValue.
  const CSSValue* ToCSSValue() const final;

 protected:
  CSSMathValue(const CSSNumericValueType& type) : CSSNumericValue(type) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(CSSMathValue);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSSOM_CSS_MATH_VALUE_H_
