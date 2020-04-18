// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_STYLE_RULE_NAMESPACE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_STYLE_RULE_NAMESPACE_H_

#include "third_party/blink/renderer/core/css/style_rule.h"

namespace blink {

// This class is never actually stored anywhere currently, but only used for
// the parser to pass to a stylesheet
class StyleRuleNamespace final : public StyleRuleBase {
 public:
  static StyleRuleNamespace* Create(AtomicString prefix, AtomicString uri) {
    return new StyleRuleNamespace(prefix, uri);
  }

  StyleRuleNamespace* Copy() const {
    return new StyleRuleNamespace(prefix_, uri_);
  }

  AtomicString Prefix() const { return prefix_; }
  AtomicString Uri() const { return uri_; }

  void TraceAfterDispatch(blink::Visitor* visitor) {
    StyleRuleBase::TraceAfterDispatch(visitor);
  }

 private:
  StyleRuleNamespace(AtomicString prefix, AtomicString uri)
      : StyleRuleBase(kNamespace), prefix_(prefix), uri_(uri) {}

  AtomicString prefix_;
  AtomicString uri_;
};

DEFINE_STYLE_RULE_TYPE_CASTS(Namespace);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_STYLE_RULE_NAMESPACE_H_
