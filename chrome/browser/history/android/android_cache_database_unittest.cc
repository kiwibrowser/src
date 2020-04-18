// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/history/core/browser/android/android_cache_database.h"

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/test/base/testing_profile.h"
#include "components/history/core/browser/android/android_time.h"
#include "components/history/core/browser/history_database.h"
#include "components/history/core/test/test_history_database.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::Time;
using base::TimeDelta;

namespace history {

class AndroidCacheDatabaseTest : public testing::Test {
 public:
  AndroidCacheDatabaseTest() {}
  ~AndroidCacheDatabaseTest() override {}

 protected:
  void SetUp() override {
    // Get a temporary directory for the test DB files.
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::FilePath history_db_name_ =
        temp_dir_.GetPath().AppendASCII("history.db");
    android_cache_db_name_ =
        temp_dir_.GetPath().AppendASCII("TestAndroidCache.db");
    ASSERT_EQ(sql::INIT_OK, history_db_.Init(history_db_name_));
    ASSERT_EQ(sql::INIT_OK,
              history_db_.InitAndroidCacheDatabase(android_cache_db_name_));
  }

  base::ScopedTempDir temp_dir_;
  base::FilePath android_cache_db_name_;
  TestHistoryDatabase history_db_;
};

TEST(AndroidCacheDatabaseAttachTest, AttachDatabaseInTransactionNesting) {
  base::ScopedTempDir temp_dir;
  base::FilePath android_cache_db_name;
  TestHistoryDatabase history_db;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath history_db_name = temp_dir.GetPath().AppendASCII("history.db");
  android_cache_db_name = temp_dir.GetPath().AppendASCII("TestAndroidCache.db");
  ASSERT_EQ(sql::INIT_OK, history_db.Init(history_db_name));
  // Create nested transactions.
  history_db.BeginTransaction();
  history_db.BeginTransaction();
  history_db.BeginTransaction();
  int transaction_nesting = history_db.transaction_nesting();
  ASSERT_EQ(sql::INIT_OK,
            history_db.InitAndroidCacheDatabase(android_cache_db_name));
  // The count of nested transaction is still same.
  EXPECT_EQ(transaction_nesting, history_db.transaction_nesting());
}

TEST_F(AndroidCacheDatabaseTest, InitAndroidCacheDatabase) {
  // Try to run a sql against the table to verify them exist.
  AndroidCacheDatabase* cache_db =
      static_cast<AndroidCacheDatabase*>(&history_db_);
  EXPECT_TRUE(cache_db->GetDB().Execute(
      "DELETE FROM android_cache_db.bookmark_cache"));
  EXPECT_TRUE(cache_db->GetDB().Execute(
      "DELETE FROM android_cache_db.search_terms"));
}

TEST_F(AndroidCacheDatabaseTest, SearchTermsTable) {
  // Test AddSearchTerm.
  Time search_time1 = Time::Now() - TimeDelta::FromDays(1);
  base::string16 search_term1(base::UTF8ToUTF16("search term 1"));
  SearchTermID id1 = history_db_.AddSearchTerm(search_term1, search_time1);
  ASSERT_TRUE(id1);
  SearchTermRow row1;
  ASSERT_EQ(id1, history_db_.GetSearchTerm(search_term1, &row1));
  EXPECT_EQ(search_term1, row1.term);
  EXPECT_EQ(ToDatabaseTime(search_time1),
            ToDatabaseTime(row1.last_visit_time));
  EXPECT_EQ(id1, row1.id);

  // Test UpdateSearchTerm.
  SearchTermRow update_row1;
  update_row1.term = (base::UTF8ToUTF16("update search term1"));
  update_row1.last_visit_time = Time::Now();
  ASSERT_TRUE(history_db_.UpdateSearchTerm(id1, update_row1));
  EXPECT_EQ(id1, history_db_.GetSearchTerm(update_row1.term, &row1));
  EXPECT_EQ(update_row1.term, row1.term);
  EXPECT_EQ(ToDatabaseTime(update_row1.last_visit_time),
            ToDatabaseTime(row1.last_visit_time));
  EXPECT_EQ(id1, row1.id);

  Time search_time2 = Time::Now() - TimeDelta::FromHours(1);
  base::string16 search_term2(base::UTF8ToUTF16("search term 2"));
  SearchTermID id2 = history_db_.AddSearchTerm(search_term2, search_time2);
  ASSERT_TRUE(id2);
  ASSERT_TRUE(history_db_.SetKeywordSearchTermsForURL(1, 1, search_term2));
  ASSERT_TRUE(history_db_.DeleteUnusedSearchTerms());

  // The search_term1 was removed.
  EXPECT_FALSE(history_db_.GetSearchTerm(update_row1.term, NULL));
  // The search_term2 should still in the table.
  ASSERT_EQ(id2, history_db_.GetSearchTerm(search_term2, &row1));
  EXPECT_EQ(id2, row1.id);
  EXPECT_EQ(ToDatabaseTime(search_time2),
            ToDatabaseTime(row1.last_visit_time));
  EXPECT_EQ(search_term2, row1.term);
}

}  // namespace history
