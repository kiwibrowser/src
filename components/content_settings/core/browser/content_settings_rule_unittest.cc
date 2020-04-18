// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <list>
#include <utility>
#include <vector>

#include "components/content_settings/core/browser/content_settings_rule.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content_settings {

namespace {

class ListIterator : public RuleIterator {
 public:
  explicit ListIterator(const std::list<Rule>& rules)
      : rules_(rules) {}

  ~ListIterator() override {}

  bool HasNext() const override { return !rules_.empty(); }

  Rule Next() override {
    EXPECT_FALSE(rules_.empty());
    // |front()| returns a reference but we're going to discard the object
    // referred to; force copying here.
    Rule rule = rules_.front();
    rules_.pop_front();
    return rule;
  }

 private:
  std::list<Rule> rules_;
};

}  // namespace

TEST(RuleTest, ConcatenationIterator) {
  std::list<Rule> rules1;
  rules1.push_back(Rule(ContentSettingsPattern::FromString("a"),
                        ContentSettingsPattern::Wildcard(),
                        new base::Value(0)));
  rules1.push_back(Rule(ContentSettingsPattern::FromString("b"),
                        ContentSettingsPattern::Wildcard(),
                        new base::Value(0)));
  std::list<Rule> rules2;
  rules2.push_back(Rule(ContentSettingsPattern::FromString("c"),
                        ContentSettingsPattern::Wildcard(),
                        new base::Value(0)));
  rules2.push_back(Rule(ContentSettingsPattern::FromString("d"),
                        ContentSettingsPattern::Wildcard(),
                        new base::Value(0)));

  std::vector<std::unique_ptr<RuleIterator>> iterators;
  iterators.push_back(std::unique_ptr<RuleIterator>(new ListIterator(rules1)));
  iterators.push_back(std::unique_ptr<RuleIterator>(new ListIterator(rules2)));
  ConcatenationIterator concatenation_iterator(std::move(iterators), nullptr);

  Rule rule;
  ASSERT_TRUE(concatenation_iterator.HasNext());
  rule = concatenation_iterator.Next();
  EXPECT_EQ(rule.primary_pattern, ContentSettingsPattern::FromString("a"));

  ASSERT_TRUE(concatenation_iterator.HasNext());
  rule = concatenation_iterator.Next();
  EXPECT_EQ(rule.primary_pattern, ContentSettingsPattern::FromString("b"));

  ASSERT_TRUE(concatenation_iterator.HasNext());
  rule = concatenation_iterator.Next();
  EXPECT_EQ(rule.primary_pattern, ContentSettingsPattern::FromString("c"));

  ASSERT_TRUE(concatenation_iterator.HasNext());
  rule = concatenation_iterator.Next();
  EXPECT_EQ(rule.primary_pattern, ContentSettingsPattern::FromString("d"));

  EXPECT_FALSE(concatenation_iterator.HasNext());
}

}  // namespace content_settings
