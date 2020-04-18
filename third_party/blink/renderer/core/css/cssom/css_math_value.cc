// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/cssom/css_math_value.h"

#include "third_party/blink/renderer/core/css/css_calculation_value.h"

namespace blink {

const CSSValue* CSSMathValue::ToCSSValue() const {
  CSSCalcExpressionNode* node = ToCalcExpressionNode();
  if (!node)
    return nullptr;
  return CSSPrimitiveValue::Create(CSSCalcValue::Create(node));
}

}  // namespace blink
