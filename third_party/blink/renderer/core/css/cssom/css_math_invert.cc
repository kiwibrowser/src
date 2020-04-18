// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/cssom/css_math_invert.h"

#include "third_party/blink/renderer/core/css/cssom/css_numeric_sum_value.h"

namespace blink {

base::Optional<CSSNumericSumValue> CSSMathInvert::SumValue() const {
  auto sum = value_->SumValue();
  if (!sum || sum->terms.size() != 1)
    return base::nullopt;

  for (auto& unit_exponent : sum->terms[0].units)
    unit_exponent.value *= -1;

  sum->terms[0].value = 1.0 / sum->terms[0].value;
  return sum;
}

void CSSMathInvert::BuildCSSText(Nested nested,
                                 ParenLess paren_less,
                                 StringBuilder& result) const {
  if (paren_less == ParenLess::kNo)
    result.Append(nested == Nested::kYes ? "(" : "calc(");

  result.Append("1 / ");
  value_->BuildCSSText(Nested::kYes, ParenLess::kNo, result);

  if (paren_less == ParenLess::kNo)
    result.Append(")");
}

}  // namespace blink
