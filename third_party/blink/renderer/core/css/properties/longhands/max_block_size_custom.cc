// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/max_block_size.h"

#include "third_party/blink/renderer/core/css/properties/css_parsing_utils.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* MaxBlockSize::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  return CSSParsingUtils::ConsumeMaxWidthOrHeight(range, context);
}

}  // namespace CSSLonghand
}  // namespace blink
