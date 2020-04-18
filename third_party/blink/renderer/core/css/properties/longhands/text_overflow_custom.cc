// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/text_overflow.h"

#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* TextOverflow::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  if (style.TextOverflow() != ETextOverflow::kClip)
    return CSSIdentifierValue::Create(CSSValueEllipsis);
  return CSSIdentifierValue::Create(CSSValueClip);
}

}  // namespace CSSLonghand
}  // namespace blink
