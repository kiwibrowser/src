// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/parser/css_parser_local_context.h"

namespace blink {

CSSParserLocalContext::CSSParserLocalContext()
    : use_alias_parsing_(false), current_shorthand_(CSSPropertyInvalid) {}

CSSParserLocalContext::CSSParserLocalContext(bool use_alias_parsing,
                                             CSSPropertyID current_shorthand)
    : use_alias_parsing_(use_alias_parsing),
      current_shorthand_(current_shorthand) {}

bool CSSParserLocalContext::UseAliasParsing() const {
  return use_alias_parsing_;
}

CSSPropertyID CSSParserLocalContext::CurrentShorthand() const {
  return current_shorthand_;
}

}  // namespace blink
