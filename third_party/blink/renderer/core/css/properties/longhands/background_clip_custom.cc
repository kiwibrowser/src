// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/background_clip.h"

#include "third_party/blink/renderer/core/css/properties/css_parsing_utils.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* BackgroundClip::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext&,
    const CSSParserLocalContext& local_context) const {
  return CSSParsingUtils::ParseBackgroundBox(
      range, local_context, CSSParsingUtils::AllowTextValue::kAllow);
}

const CSSValue* BackgroundClip::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  CSSValueList* list = CSSValueList::CreateCommaSeparated();
  const FillLayer* curr_layer = &style.BackgroundLayers();
  for (; curr_layer; curr_layer = curr_layer->Next()) {
    EFillBox box = curr_layer->Clip();
    list->Append(*CSSIdentifierValue::Create(box));
  }
  return list;
}

}  // namespace CSSLonghand
}  // namespace blink
