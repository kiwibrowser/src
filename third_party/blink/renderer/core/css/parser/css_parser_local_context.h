// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_PARSER_CSS_PARSER_LOCAL_CONTEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_PARSER_CSS_PARSER_LOCAL_CONTEXT_H_

#include "third_party/blink/renderer/core/css_property_names.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

// A wrapper class containing all local context when parsing a property.

class CSSParserLocalContext {
  STACK_ALLOCATED();

 public:
  CSSParserLocalContext();
  CSSParserLocalContext(bool use_alias_parsing,
                        CSSPropertyID current_shorthand);
  bool UseAliasParsing() const;
  CSSPropertyID CurrentShorthand() const;

 private:
  bool use_alias_parsing_;
  CSSPropertyID current_shorthand_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_PARSER_CSS_PARSER_LOCAL_CONTEXT_H_
