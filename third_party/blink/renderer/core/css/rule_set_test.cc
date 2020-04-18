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

#include "third_party/blink/renderer/core/css/rule_set.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/css/css_test_helper.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace blink {

TEST(RuleSetTest, findBestRuleSetAndAdd_CustomPseudoElements) {
  CSSTestHelper helper;

  helper.AddCSSRules("summary::-webkit-details-marker { }");
  RuleSet& rule_set = helper.GetRuleSet();
  AtomicString str("-webkit-details-marker");
  const TerminatedArray<RuleData>* rules =
      rule_set.ShadowPseudoElementRules(str);
  ASSERT_EQ(1u, rules->size());
  ASSERT_EQ(str, rules->at(0).Selector().Value());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_Id) {
  CSSTestHelper helper;

  helper.AddCSSRules("#id { }");
  RuleSet& rule_set = helper.GetRuleSet();
  AtomicString str("id");
  const TerminatedArray<RuleData>* rules = rule_set.IdRules(str);
  ASSERT_EQ(1u, rules->size());
  ASSERT_EQ(str, rules->at(0).Selector().Value());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_NthChild) {
  CSSTestHelper helper;

  helper.AddCSSRules("div:nth-child(2) { }");
  RuleSet& rule_set = helper.GetRuleSet();
  AtomicString str("div");
  const TerminatedArray<RuleData>* rules = rule_set.TagRules(str);
  ASSERT_EQ(1u, rules->size());
  ASSERT_EQ(str, rules->at(0).Selector().TagQName().LocalName());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_ClassThenId) {
  CSSTestHelper helper;

  helper.AddCSSRules(".class#id { }");
  RuleSet& rule_set = helper.GetRuleSet();
  AtomicString str("id");
  // id is prefered over class even if class preceeds it in the selector.
  const TerminatedArray<RuleData>* rules = rule_set.IdRules(str);
  ASSERT_EQ(1u, rules->size());
  AtomicString class_str("class");
  ASSERT_EQ(class_str, rules->at(0).Selector().Value());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_IdThenClass) {
  CSSTestHelper helper;

  helper.AddCSSRules("#id.class { }");
  RuleSet& rule_set = helper.GetRuleSet();
  AtomicString str("id");
  const TerminatedArray<RuleData>* rules = rule_set.IdRules(str);
  ASSERT_EQ(1u, rules->size());
  ASSERT_EQ(str, rules->at(0).Selector().Value());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_AttrThenId) {
  CSSTestHelper helper;

  helper.AddCSSRules("[attr]#id { }");
  RuleSet& rule_set = helper.GetRuleSet();
  AtomicString str("id");
  const TerminatedArray<RuleData>* rules = rule_set.IdRules(str);
  ASSERT_EQ(1u, rules->size());
  AtomicString attr_str("attr");
  ASSERT_EQ(attr_str, rules->at(0).Selector().Attribute().LocalName());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_TagThenAttrThenId) {
  CSSTestHelper helper;

  helper.AddCSSRules("div[attr]#id { }");
  RuleSet& rule_set = helper.GetRuleSet();
  AtomicString str("id");
  const TerminatedArray<RuleData>* rules = rule_set.IdRules(str);
  ASSERT_EQ(1u, rules->size());
  AtomicString tag_str("div");
  ASSERT_EQ(tag_str, rules->at(0).Selector().TagQName().LocalName());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_DivWithContent) {
  CSSTestHelper helper;

  helper.AddCSSRules("div::content { }");
  RuleSet& rule_set = helper.GetRuleSet();
  AtomicString str("div");
  const TerminatedArray<RuleData>* rules = rule_set.TagRules(str);
  ASSERT_EQ(1u, rules->size());
  AtomicString value_str("content");
  ASSERT_EQ(value_str, rules->at(0).Selector().TagHistory()->Value());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_Host) {
  CSSTestHelper helper;

  helper.AddCSSRules(":host { }");
  RuleSet& rule_set = helper.GetRuleSet();
  const HeapVector<RuleData>* rules = rule_set.ShadowHostRules();
  ASSERT_EQ(1u, rules->size());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_HostWithId) {
  CSSTestHelper helper;

  helper.AddCSSRules(":host(#x) { }");
  RuleSet& rule_set = helper.GetRuleSet();
  const HeapVector<RuleData>* rules = rule_set.ShadowHostRules();
  ASSERT_EQ(1u, rules->size());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_HostContext) {
  CSSTestHelper helper;

  helper.AddCSSRules(":host-context(*) { }");
  RuleSet& rule_set = helper.GetRuleSet();
  const HeapVector<RuleData>* rules = rule_set.ShadowHostRules();
  ASSERT_EQ(1u, rules->size());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_HostContextWithId) {
  CSSTestHelper helper;

  helper.AddCSSRules(":host-context(#x) { }");
  RuleSet& rule_set = helper.GetRuleSet();
  const HeapVector<RuleData>* rules = rule_set.ShadowHostRules();
  ASSERT_EQ(1u, rules->size());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_HostAndHostContextNotInRightmost) {
  CSSTestHelper helper;

  helper.AddCSSRules(":host-context(#x) .y, :host(.a) > #b  { }");
  RuleSet& rule_set = helper.GetRuleSet();
  const HeapVector<RuleData>* shadow_rules = rule_set.ShadowHostRules();
  const TerminatedArray<RuleData>* id_rules = rule_set.IdRules("b");
  const TerminatedArray<RuleData>* class_rules = rule_set.ClassRules("y");
  ASSERT_EQ(0u, shadow_rules->size());
  ASSERT_EQ(1u, id_rules->size());
  ASSERT_EQ(1u, class_rules->size());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_HostAndClass) {
  CSSTestHelper helper;

  helper.AddCSSRules(".foo:host { }");
  RuleSet& rule_set = helper.GetRuleSet();
  const HeapVector<RuleData>* rules = rule_set.ShadowHostRules();
  ASSERT_EQ(0u, rules->size());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_HostContextAndClass) {
  CSSTestHelper helper;

  helper.AddCSSRules(".foo:host-context(*) { }");
  RuleSet& rule_set = helper.GetRuleSet();
  const HeapVector<RuleData>* rules = rule_set.ShadowHostRules();
  ASSERT_EQ(0u, rules->size());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_Focus) {
  CSSTestHelper helper;

  helper.AddCSSRules(":focus { }");
  helper.AddCSSRules("[attr]:focus { }");
  RuleSet& rule_set = helper.GetRuleSet();
  const HeapVector<RuleData>* rules = rule_set.FocusPseudoClassRules();
  ASSERT_EQ(2u, rules->size());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_LinkVisited) {
  CSSTestHelper helper;

  helper.AddCSSRules(":link { }");
  helper.AddCSSRules("[attr]:link { }");
  helper.AddCSSRules(":visited { }");
  helper.AddCSSRules("[attr]:visited { }");
  helper.AddCSSRules(":-webkit-any-link { }");
  helper.AddCSSRules("[attr]:-webkit-any-link { }");
  RuleSet& rule_set = helper.GetRuleSet();
  const HeapVector<RuleData>* rules = rule_set.LinkPseudoClassRules();
  ASSERT_EQ(6u, rules->size());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_Cue) {
  CSSTestHelper helper;

  helper.AddCSSRules("::cue(b) { }");
  helper.AddCSSRules("video::cue(u) { }");
  RuleSet& rule_set = helper.GetRuleSet();
  const HeapVector<RuleData>* rules = rule_set.CuePseudoRules();
  ASSERT_EQ(2u, rules->size());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_PlaceholderPseudo) {
  CSSTestHelper helper;

  helper.AddCSSRules("::placeholder { }");
  helper.AddCSSRules("input::placeholder { }");
  RuleSet& rule_set = helper.GetRuleSet();
  auto* rules = rule_set.ShadowPseudoElementRules("-webkit-input-placeholder");
  ASSERT_EQ(2u, rules->size());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_PseudoMatches) {
  CSSTestHelper helper;

  helper.AddCSSRules(".a :matches(.b+.c, .d>:matches(.e, .f)) { }");
  RuleSet& rule_set = helper.GetRuleSet();
  {
    AtomicString str("c");
    const TerminatedArray<RuleData>* rules = rule_set.ClassRules(str);
    ASSERT_EQ(1u, rules->size());
    ASSERT_EQ(str, rules->at(0).Selector().Value());
  }
  {
    AtomicString str("e");
    const TerminatedArray<RuleData>* rules = rule_set.ClassRules(str);
    ASSERT_EQ(1u, rules->size());
    ASSERT_EQ(str, rules->at(0).Selector().Value());
  }
  {
    AtomicString str("f");
    const TerminatedArray<RuleData>* rules = rule_set.ClassRules(str);
    ASSERT_EQ(1u, rules->size());
    ASSERT_EQ(str, rules->at(0).Selector().Value());
  }
}

TEST(RuleSetTest, findBestRuleSetAndAdd_PseudoIS) {
  CSSTestHelper helper;

  helper.AddCSSRules(".a :is(.b+.c, .d>:is(.e, .f)) { }");
  RuleSet& rule_set = helper.GetRuleSet();
  {
    AtomicString str("c");
    const TerminatedArray<RuleData>* rules = rule_set.ClassRules(str);
    ASSERT_EQ(1u, rules->size());
    ASSERT_EQ(str, rules->at(0).Selector().Value());
  }
  {
    AtomicString str("e");
    const TerminatedArray<RuleData>* rules = rule_set.ClassRules(str);
    ASSERT_EQ(1u, rules->size());
    ASSERT_EQ(str, rules->at(0).Selector().Value());
  }
  {
    AtomicString str("f");
    const TerminatedArray<RuleData>* rules = rule_set.ClassRules(str);
    ASSERT_EQ(1u, rules->size());
    ASSERT_EQ(str, rules->at(0).Selector().Value());
  }
}

TEST(RuleSetTest, findBestRuleSetAndAdd_PseudoMatchesTooLarge) {
  // RuleData cannot support selectors at index 8192 or beyond so the expansion
  // is limited to this size
  CSSTestHelper helper;

  helper.AddCSSRules(
      ":matches(.a#a, .b#b, .c#c, .d#d) + "
      ":matches(.e#e, .f#f, .g#g, .h#h) + "
      ":matches(.i#i, .j#j, .k#k, .l#l) + "
      ":matches(.m#m, .n#n, .o#o, .p#p) + "
      ":matches(.q#q, .r#r, .s#s, .t#t) + "
      ":matches(.u#u, .v#v, .w#w, .x#x) { }",
      true);

  RuleSet& rule_set = helper.GetRuleSet();
  ASSERT_EQ(0u, rule_set.RuleCount());
}

TEST(RuleSetTest, findBestRuleSetAndAdd_PseudoISTooLarge) {
  // RuleData cannot support selectors at index 8192 or beyond so the expansion
  // is limited to this size
  CSSTestHelper helper;

  helper.AddCSSRules(
      ":is(.a#a, .b#b, .c#c, .d#d) + :is(.e#e, .f#f, .g#g, .h#h) + "
      ":is(.i#i, .j#j, .k#k, .l#l) + :is(.m#m, .n#n, .o#o, .p#p) + "
      ":is(.q#q, .r#r, .s#s, .t#t) + :is(.u#u, .v#v, .w#w, .x#x) { }",
      true);

  RuleSet& rule_set = helper.GetRuleSet();
  ASSERT_EQ(0u, rule_set.RuleCount());
}

TEST(RuleSetTest, SelectorIndexLimit) {
  StringBuilder builder;

  // We use 13 bits to storing the selector start index in RuleData. This is a
  // test to check that we don't regress. We WONTFIX issues asking for more
  // since 2^13 simple selectors in a style rule is already excessive.
  for (unsigned i = 0; i < 8191; i++)
    builder.Append("div,");

  builder.Append("b,span {}");

  CSSTestHelper helper;
  helper.AddCSSRules(builder.ToString().Ascii().data());
  const RuleSet& rule_set = helper.GetRuleSet();
  const HeapTerminatedArray<RuleData>* rules = rule_set.TagRules("b");
  ASSERT_EQ(1u, rules->size());
  EXPECT_EQ("b", rules->at(0).Selector().TagQName().LocalName());
  EXPECT_FALSE(rule_set.TagRules("span"));
}

}  // namespace blink
