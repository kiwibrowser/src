// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/resolver/style_rule_usage_tracker.h"

#include "third_party/blink/renderer/core/css/css_style_sheet.h"
#include "third_party/blink/renderer/core/css/style_rule.h"

namespace blink {

StyleRuleUsageTracker::RuleListByStyleSheet StyleRuleUsageTracker::TakeDelta() {
  RuleListByStyleSheet result;
  result.swap(used_rules_delta_);
  return result;
}

void StyleRuleUsageTracker::Track(const CSSStyleSheet* parent_sheet,
                                  const StyleRule* rule) {
  if (!parent_sheet)
    return;
  if (!used_rules_.insert(std::make_pair(parent_sheet, rule)).is_new_entry)
    return;
  auto it = used_rules_delta_.find(parent_sheet);
  if (it != used_rules_delta_.end()) {
    it->value.push_back(rule);
  } else {
    used_rules_delta_
        .insert(parent_sheet, HeapVector<Member<const StyleRule>>())
        .stored_value->value.push_back(rule);
  }
}

void StyleRuleUsageTracker::Trace(blink::Visitor* visitor) {
  visitor->Trace(used_rules_);
  visitor->Trace(used_rules_delta_);
}

}  // namespace blink
