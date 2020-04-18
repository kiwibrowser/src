// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_NAMESPACE_RULE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_NAMESPACE_RULE_H_

#include "third_party/blink/renderer/core/css/css_rule.h"

namespace blink {

class StyleRuleNamespace;

class CSSNamespaceRule final : public CSSRule {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static CSSNamespaceRule* Create(StyleRuleNamespace* rule,
                                  CSSStyleSheet* sheet) {
    return new CSSNamespaceRule(rule, sheet);
  }

  ~CSSNamespaceRule() override;

  String cssText() const override;
  void Reattach(StyleRuleBase*) override {}

  AtomicString namespaceURI() const;
  AtomicString prefix() const;

  void Trace(blink::Visitor*) override;

 private:
  CSSNamespaceRule(StyleRuleNamespace*, CSSStyleSheet*);

  CSSRule::Type type() const override { return kNamespaceRule; }

  Member<StyleRuleNamespace> namespace_rule_;
};

DEFINE_CSS_RULE_TYPE_CASTS(CSSNamespaceRule, kNamespaceRule);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_NAMESPACE_RULE_H_
