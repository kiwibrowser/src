/*
 * Copyright (c) 2014, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Opera Software ASA nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/css/css_test_helper.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/css/css_rule_list.h"
#include "third_party/blink/renderer/core/css/css_style_sheet.h"
#include "third_party/blink/renderer/core/css/rule_set.h"
#include "third_party/blink/renderer/core/css/style_sheet_contents.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/platform/wtf/text/text_encoding.h"

namespace blink {

CSSTestHelper::~CSSTestHelper() = default;

CSSTestHelper::CSSTestHelper() {
  document_ = Document::CreateForTest();
  TextPosition position;
  style_sheet_ = CSSStyleSheet::CreateInline(*document_, NullURL(), position,
                                             UTF8Encoding());
}

CSSRuleList* CSSTestHelper::CssRules() {
  DummyExceptionStateForTesting exception_state;
  CSSRuleList* result = style_sheet_->cssRules(exception_state);
  EXPECT_FALSE(exception_state.HadException());
  return result;
}

RuleSet& CSSTestHelper::GetRuleSet() {
  RuleSet& rule_set = style_sheet_->Contents()->EnsureRuleSet(
      MediaQueryEvaluator(), kRuleHasNoSpecialState);
  rule_set.CompactRulesIfNeeded();
  return rule_set;
}

void CSSTestHelper::AddCSSRules(const char* css_text, bool is_empty_sheet) {
  TextPosition position;
  unsigned sheet_length = style_sheet_->length();
  style_sheet_->Contents()->ParseStringAtPosition(css_text, position);
  if (!is_empty_sheet)
    ASSERT_GT(style_sheet_->length(), sheet_length);
  else
    ASSERT_EQ(style_sheet_->length(), sheet_length);
}

}  // namespace blink
