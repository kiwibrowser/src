// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/core/article_entry.h"

#include "components/sync/protocol/sync.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

using sync_pb::EntitySpecifics;
using sync_pb::ArticlePage;
using sync_pb::ArticleSpecifics;
using testing::AssertionResult;
using testing::AssertionSuccess;
using testing::AssertionFailure;

namespace dom_distiller {

TEST(DomDistillerArticleEntryTest, TestIsEntryValid) {
  ArticleEntry entry;
  EXPECT_FALSE(IsEntryValid(entry));
  entry.set_entry_id("entry0");
  EXPECT_TRUE(IsEntryValid(entry));
  ArticleEntryPage* page0 = entry.add_pages();
  EXPECT_FALSE(IsEntryValid(entry));
  page0->set_url("example.com/1");
  EXPECT_TRUE(IsEntryValid(entry));
}

TEST(DomDistillerArticleEntryTest, TestAreEntriesEqual) {
  ArticleEntry left;
  ArticleEntry right;
  left.set_entry_id("entry0");
  right.set_entry_id("entry1");
  EXPECT_FALSE(AreEntriesEqual(left, right));
  right = left;
  EXPECT_TRUE(AreEntriesEqual(left, right));

  left.set_title("a title");
  EXPECT_FALSE(AreEntriesEqual(left, right));
  right.set_title("a different title");
  EXPECT_FALSE(AreEntriesEqual(left, right));
  right.set_title("a title");
  EXPECT_TRUE(AreEntriesEqual(left, right));

  ArticleEntryPage left_page;
  left_page.set_url("example.com/1");
  *left.add_pages() = left_page;

  EXPECT_FALSE(AreEntriesEqual(left, right));

  ArticleEntryPage right_page;
  right_page.set_url("foo.example.com/1");
  *right.add_pages() = right_page;
  EXPECT_FALSE(AreEntriesEqual(left, right));

  right = left;
  EXPECT_TRUE(AreEntriesEqual(left, right));

  *right.add_pages() = right_page;
  EXPECT_FALSE(AreEntriesEqual(left, right));
}

}  // namespace dom_distiller
