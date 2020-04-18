// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/collapsed_border_value.h"

#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

CollapsedBorderValue::CollapsedBorderValue(const BorderValue& border,
                                           const Color& color,
                                           EBorderPrecedence precedence)
    : color_(color),
      style_(static_cast<unsigned>(border.Style())),
      precedence_(precedence) {
  if (!ComputedStyle::BorderStyleIsVisible(border.Style())) {
    width_ = 0;
  } else {
    if (border.Width() > 0.0f && border.Width() <= 1.0f)
      width_ = 1;
    else
      width_ = border.Width();
  }
  DCHECK(precedence != kBorderPrecedenceOff);
}

CollapsedBorderValue::CollapsedBorderValue(EBorderStyle style,
                                           const float width,
                                           const Color& color,
                                           EBorderPrecedence precedence)
    : color_(color),
      style_(static_cast<unsigned>(style)),
      precedence_(precedence) {
  if (!ComputedStyle::BorderStyleIsVisible(style)) {
    width_ = 0;
  } else {
    if (width > 0.0f && width <= 1.0f)
      width_ = 1;
    else
      width_ = width;
  }
  DCHECK(precedence != kBorderPrecedenceOff);
}

}  // namespace blink
