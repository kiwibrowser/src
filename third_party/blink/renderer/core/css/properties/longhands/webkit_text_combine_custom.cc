// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/webkit_text_combine.h"

#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* WebkitTextCombine::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  if (style.TextCombine() == ETextCombine::kAll)
    return CSSIdentifierValue::Create(CSSValueHorizontal);
  return CSSIdentifierValue::Create(style.TextCombine());
}

}  // namespace CSSLonghand
}  // namespace blink
