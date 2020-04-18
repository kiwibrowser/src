// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/core/dom_distiller_model.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace dom_distiller {

TEST(DomDistillerModelTest, TestGetByEntryId) {
  ArticleEntry entry1;
  entry1.set_entry_id("id1");
  entry1.set_title("title1");
  ArticleEntry entry2;
  entry2.set_entry_id("id2");
  entry2.set_title("title1");

  std::vector<ArticleEntry> initial_model;
  initial_model.push_back(entry1);
  initial_model.push_back(entry2);

  DomDistillerModel model(initial_model);

  ArticleEntry found_entry;
  EXPECT_TRUE(model.GetEntryById(entry1.entry_id(), &found_entry));
  ASSERT_TRUE(IsEntryValid(found_entry));
  EXPECT_TRUE(AreEntriesEqual(entry1, found_entry));

  EXPECT_TRUE(model.GetEntryById(entry2.entry_id(), &found_entry));
  ASSERT_TRUE(IsEntryValid(found_entry));
  EXPECT_TRUE(AreEntriesEqual(entry2, found_entry));

  EXPECT_FALSE(model.GetEntryById("some_other_id", nullptr));
}

TEST(DomDistillerModelTest, TestGetByUrl) {
  ArticleEntry entry1;
  entry1.set_entry_id("id1");
  entry1.set_title("title1");
  ArticleEntryPage* page1 = entry1.add_pages();
  page1->set_url("http://example.com/1");
  ArticleEntryPage* page2 = entry1.add_pages();
  page2->set_url("http://example.com/2");

  ArticleEntry entry2;
  entry2.set_entry_id("id2");
  entry2.set_title("title1");
  ArticleEntryPage* page3 = entry2.add_pages();
  page3->set_url("http://example.com/a1");

  std::vector<ArticleEntry> initial_model;
  initial_model.push_back(entry1);
  initial_model.push_back(entry2);

  DomDistillerModel model(initial_model);

  ArticleEntry found_entry;
  EXPECT_TRUE(model.GetEntryByUrl(GURL(page1->url()), &found_entry));
  ASSERT_TRUE(IsEntryValid(found_entry));
  EXPECT_TRUE(AreEntriesEqual(entry1, found_entry));

  EXPECT_TRUE(model.GetEntryByUrl(GURL(page2->url()), &found_entry));
  ASSERT_TRUE(IsEntryValid(found_entry));
  EXPECT_TRUE(AreEntriesEqual(entry1, found_entry));

  EXPECT_TRUE(model.GetEntryByUrl(GURL(page3->url()), &found_entry));
  ASSERT_TRUE(IsEntryValid(found_entry));
  EXPECT_TRUE(AreEntriesEqual(entry2, found_entry));

  EXPECT_FALSE(model.GetEntryByUrl(GURL("http://example.com/foo"), nullptr));
}

// This test ensures that the model handles the case where an URL maps to
// multiple entries. In that case, the model is allowed to have an inconsistent
// url-to-entry mapping, but it should not fail in other ways (i.e. id-to-entry
// should be correct, shouldn't crash).
TEST(DomDistillerModelTest, TestUrlToMultipleEntries) {
  ArticleEntry entry1;
  entry1.set_entry_id("id1");
  entry1.set_title("title1");
  ArticleEntryPage* page1 = entry1.add_pages();
  page1->set_url("http://example.com/1");
  ArticleEntryPage* page2 = entry1.add_pages();
  page2->set_url("http://example.com/2");

  ArticleEntry entry2;
  entry2.set_entry_id("id2");
  entry2.set_title("title1");
  ArticleEntryPage* page3 = entry2.add_pages();
  page3->set_url("http://example.com/1");

  std::vector<ArticleEntry> initial_model;
  initial_model.push_back(entry1);
  initial_model.push_back(entry2);

  DomDistillerModel model(initial_model);

  EXPECT_TRUE(model.GetEntryByUrl(GURL(page1->url()), nullptr));
  EXPECT_TRUE(model.GetEntryByUrl(GURL(page2->url()), nullptr));
  EXPECT_TRUE(model.GetEntryByUrl(GURL(page3->url()), nullptr));

  ArticleEntry found_entry;
  EXPECT_TRUE(model.GetEntryById(entry1.entry_id(), &found_entry));
  ASSERT_TRUE(IsEntryValid(found_entry));
  EXPECT_TRUE(AreEntriesEqual(entry1, found_entry));

  EXPECT_TRUE(model.GetEntryById(entry2.entry_id(), &found_entry));
  ASSERT_TRUE(IsEntryValid(found_entry));
  EXPECT_TRUE(AreEntriesEqual(entry2, found_entry));

  syncer::SyncChangeList changes_to_apply;
  syncer::SyncChangeList changes_applied;
  syncer::SyncChangeList changes_missing;

  entry2.mutable_pages(0)->set_url("http://example.com/foo1");
  changes_to_apply.push_back(syncer::SyncChange(
      FROM_HERE, syncer::SyncChange::ACTION_UPDATE, CreateLocalData(entry2)));
  model.ApplyChangesToModel(
      changes_to_apply, &changes_applied, &changes_missing);

  EXPECT_TRUE(model.GetEntryById(entry1.entry_id(), &found_entry));
  ASSERT_TRUE(IsEntryValid(found_entry));
  EXPECT_TRUE(AreEntriesEqual(entry1, found_entry));

  EXPECT_TRUE(model.GetEntryById(entry2.entry_id(), &found_entry));
  ASSERT_TRUE(IsEntryValid(found_entry));
  EXPECT_TRUE(AreEntriesEqual(entry2, found_entry));
}

}  // namespace dom_distiller
