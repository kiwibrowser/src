// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/webkit_app_region.h"

#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* WebkitAppRegion::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  if (style.DraggableRegionMode() == EDraggableRegionMode::kNone)
    return CSSIdentifierValue::Create(CSSValueNone);
  return CSSIdentifierValue::Create(style.DraggableRegionMode() ==
                                            EDraggableRegionMode::kDrag
                                        ? CSSValueDrag
                                        : CSSValueNoDrag);
}

}  // namespace CSSLonghand
}  // namespace blink
