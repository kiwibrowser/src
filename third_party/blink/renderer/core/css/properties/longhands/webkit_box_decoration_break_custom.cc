// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/webkit_box_decoration_break.h"

#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* WebkitBoxDecorationBreak::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  if (style.BoxDecorationBreak() == EBoxDecorationBreak::kSlice)
    return CSSIdentifierValue::Create(CSSValueSlice);
  return CSSIdentifierValue::Create(CSSValueClone);
}

}  // namespace CSSLonghand
}  // namespace blink
