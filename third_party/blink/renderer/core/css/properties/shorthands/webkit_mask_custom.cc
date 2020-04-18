// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/shorthands/webkit_mask.h"

#include "third_party/blink/renderer/core/css/properties/css_parsing_utils.h"

namespace blink {
namespace CSSShorthand {

bool WebkitMask::ParseShorthand(
    bool important,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext& local_context,
    HeapVector<CSSPropertyValue, 256>& properties) const {
  return CSSParsingUtils::ParseBackgroundOrMask(important, range, context,
                                                local_context, properties);
}

}  // namespace CSSShorthand
}  // namespace blink
