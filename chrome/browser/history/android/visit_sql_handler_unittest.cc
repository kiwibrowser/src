// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/chrome_constants.h"
#include "components/history/core/browser/android/urls_sql_handler.h"
#include "components/history/core/browser/android/visit_sql_handler.h"
#include "components/history/core/browser/history_constants.h"
#include "components/history/core/browser/history_database.h"
#include "components/history/core/test/test_history_database.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::Time;
using base::TimeDelta;

namespace history {

class VisitSQLHandlerTest : public testing::Test {
 public:
  VisitSQLHandlerTest()
      : urls_sql_handler_(&history_db_),
        visit_sql_handler_(&history_db_, &history_db_) {}
  ~VisitSQLHandlerTest() override {}

 protected:
  void SetUp() override {
    // Get a temporary directory for the test DB files.
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::FilePath history_db_name =
        temp_dir_.GetPath().AppendASCII(kHistoryFilename);
    ASSERT_EQ(sql::INIT_OK, history_db_.Init(history_db_name));
  }

  void TearDown() override {}

  TestHistoryDatabase history_db_;
  base::ScopedTempDir temp_dir_;
  UrlsSQLHandler urls_sql_handler_;
  VisitSQLHandler visit_sql_handler_;

 private:
  DISALLOW_COPY_AND_ASSIGN(VisitSQLHandlerTest);
};

// Insert a url with only url set to verify no visit was inserted in visit
// table.
TEST_F(VisitSQLHandlerTest, InsertURL) {
  HistoryAndBookmarkRow row;
  row.set_raw_url("http://google.com");
  row.set_url(GURL("http://google.com"));

  ASSERT_TRUE(urls_sql_handler_.Insert(&row));
  ASSERT_TRUE(visit_sql_handler_.Insert(&row));

  URLRow url_row;
  ASSERT_TRUE(history_db_.GetURLRow(row.url_id(), &url_row));

  // Noting should be inserted to visit table.
  VisitVector visits;
  ASSERT_TRUE(history_db_.GetVisitsForURL(row.url_id(), &visits));
  EXPECT_EQ(0u, visits.size());
}

// Insert a url with last visit time set to verify a visit was inserted.
TEST_F(VisitSQLHandlerTest, InsertURLWithLastVisitTime) {
  HistoryAndBookmarkRow row;
  row.set_raw_url("http://google.com");
  row.set_url(GURL("http://google.com"));
  row.set_last_visit_time(Time::Now());

  ASSERT_TRUE(urls_sql_handler_.Insert(&row));
  ASSERT_TRUE(visit_sql_handler_.Insert(&row));

  URLRow url_row;
  ASSERT_TRUE(history_db_.GetURLRow(row.url_id(), &url_row));

  VisitVector visits;
  ASSERT_TRUE(history_db_.GetVisitsForURL(row.url_id(), &visits));
  EXPECT_EQ(1u, visits.size());
  EXPECT_EQ(row.last_visit_time(), visits[0].visit_time);
}

// Insert a urls with created time to verify the a visit was inserted.
TEST_F(VisitSQLHandlerTest, InsertURLWithCreatedTime) {
  HistoryAndBookmarkRow row;
  row.set_raw_url("http://google.com");
  row.set_url(GURL("http://google.com"));
  row.set_title(base::UTF8ToUTF16("Google"));
  row.set_created(Time::Now());

  ASSERT_TRUE(urls_sql_handler_.Insert(&row));
  ASSERT_TRUE(visit_sql_handler_.Insert(&row));

  URLRow url_row;
  ASSERT_TRUE(history_db_.GetURLRow(row.url_id(), &url_row));

  VisitVector visits;
  ASSERT_TRUE(history_db_.GetVisitsForURL(row.url_id(), &visits));
  EXPECT_EQ(1u, visits.size());
  EXPECT_EQ(row.created(), visits[0].visit_time);
}

// Insert a URL with visit count as 1 to verify a visit was inserted.
TEST_F(VisitSQLHandlerTest, InsertURLWithVisitCount) {
  HistoryAndBookmarkRow row;
  row.set_raw_url("http://google.com");
  row.set_url(GURL("http://google.com"));
  row.set_visit_count(1);

  ASSERT_TRUE(urls_sql_handler_.Insert(&row));
  ASSERT_TRUE(visit_sql_handler_.Insert(&row));

  URLRow url_row;
  ASSERT_TRUE(history_db_.GetURLRow(row.url_id(), &url_row));

  VisitVector visits;
  ASSERT_TRUE(history_db_.GetVisitsForURL(row.url_id(), &visits));
  EXPECT_EQ(1u, visits.size());
  EXPECT_NE(Time(), visits[0].visit_time);
}

// Insert a URL with all values set to verify the visit rows
// were inserted correctly.
TEST_F(VisitSQLHandlerTest, Insert) {
  HistoryAndBookmarkRow row;
  row.set_raw_url("http://google.com");
  row.set_url(GURL("http://google.com"));
  row.set_visit_count(10);
  row.set_last_visit_time(Time::Now());
  row.set_created(Time::Now() - TimeDelta::FromDays(1));

  ASSERT_TRUE(urls_sql_handler_.Insert(&row));
  ASSERT_TRUE(visit_sql_handler_.Insert(&row));

  URLRow url_row;
  ASSERT_TRUE(history_db_.GetURLRow(row.url_id(), &url_row));

  VisitVector visits;
  ASSERT_TRUE(history_db_.GetVisitsForURL(row.url_id(), &visits));
  // 10 row were inserted.
  EXPECT_EQ(10u, visits.size());
  // The earlies one has created time set.
  EXPECT_EQ(row.created(), visits[0].visit_time);
  // The latest one has last visit time set.
  EXPECT_EQ(row.last_visit_time(), visits[9].visit_time);
}

// Test the case that both visit time and visit count updated.
TEST_F(VisitSQLHandlerTest, UpdateVisitTimeAndVisitCount) {
  HistoryAndBookmarkRow row;
  row.set_raw_url("http://google.com");
  row.set_url(GURL("http://google.com"));
  row.set_title(base::UTF8ToUTF16("Google"));
  row.set_visit_count(10);
  row.set_last_visit_time(Time::Now() - TimeDelta::FromDays(10));

  ASSERT_TRUE(urls_sql_handler_.Insert(&row));
  ASSERT_TRUE(visit_sql_handler_.Insert(&row));

  URLRow url_row;
  ASSERT_TRUE(history_db_.GetURLRow(row.url_id(), &url_row));

  HistoryAndBookmarkRow update_row;
  update_row.set_last_visit_time(Time::Now());
  update_row.set_visit_count(1);

  TableIDRow id;
  id.url_id = url_row.id();
  TableIDRows ids;
  ids.push_back(id);
  ASSERT_TRUE(urls_sql_handler_.Update(update_row, ids));
  ASSERT_TRUE(visit_sql_handler_.Update(update_row, ids));

  VisitVector visits;
  ASSERT_TRUE(history_db_.GetVisitsForURL(row.url_id(), &visits));
  EXPECT_EQ(1u, visits.size());
  EXPECT_EQ(update_row.last_visit_time(), visits[0].visit_time);
}

// Update visit count to zero to verify the visit rows of this url
// were removed.
TEST_F(VisitSQLHandlerTest, UpdateVisitCountZero) {
  HistoryAndBookmarkRow row;
  row.set_raw_url("http://google.com");
  row.set_url(GURL("http://google.com"));
  row.set_visit_count(10);
  row.set_last_visit_time(Time::Now() - TimeDelta::FromDays(10));

  ASSERT_TRUE(urls_sql_handler_.Insert(&row));
  ASSERT_TRUE(visit_sql_handler_.Insert(&row));

  URLRow url_row;
  ASSERT_TRUE(history_db_.GetURLRow(row.url_id(), &url_row));

  HistoryAndBookmarkRow update_row;
  update_row.set_visit_count(0);
  TableIDRow id;
  id.url_id = url_row.id();
  TableIDRows ids;
  ids.push_back(id);
  ASSERT_TRUE(urls_sql_handler_.Update(update_row, ids));
  ASSERT_TRUE(visit_sql_handler_.Update(update_row, ids));

  ASSERT_TRUE(history_db_.GetURLRow(row.url_id(), &url_row));
  EXPECT_EQ(0, url_row.visit_count());
  // Last visit is reset.
  EXPECT_EQ(Time(), url_row.last_visit());
  VisitVector visits;
  ASSERT_TRUE(history_db_.GetVisitsForURL(row.url_id(), &visits));
  EXPECT_EQ(0u, visits.size());
}

// Update both last visit time and created time to verify
// that visits row are updated correctly.
TEST_F(VisitSQLHandlerTest, UpdateBothTime) {
  HistoryAndBookmarkRow row;
  row.set_raw_url("http://google.com");
  row.set_url(GURL("http://google.com"));
  row.set_visit_count(10);
  row.set_last_visit_time(Time::Now() - TimeDelta::FromDays(10));

  ASSERT_TRUE(urls_sql_handler_.Insert(&row));
  ASSERT_TRUE(visit_sql_handler_.Insert(&row));
  URLRow url_row;
  ASSERT_TRUE(history_db_.GetURLRow(row.url_id(), &url_row));

  HistoryAndBookmarkRow update_row;
  update_row.set_last_visit_time(Time::Now() - TimeDelta::FromDays(9));
  update_row.set_created(Time::Now() - TimeDelta::FromDays(10));
  TableIDRow id;
  id.url_id = url_row.id();
  TableIDRows ids;
  ids.push_back(id);
  ASSERT_TRUE(urls_sql_handler_.Update(update_row, ids));
  ASSERT_TRUE(visit_sql_handler_.Update(update_row, ids));

  VisitVector visits;
  ASSERT_TRUE(history_db_.GetVisitsForURL(row.url_id(), &visits));
  // Though both time are updated, visit count was increase by 1 because of
  // last visit time's change.
  EXPECT_EQ(11u, visits.size());
  EXPECT_EQ(update_row.created(), visits[0].visit_time);
  EXPECT_EQ(update_row.last_visit_time(), visits[10].visit_time);
}

// Update the visit count to verify the new visits are inserted.
TEST_F(VisitSQLHandlerTest, UpdateVisitCountIncreased) {
  HistoryAndBookmarkRow row;
  row.set_raw_url("http://google.com");
  row.set_url(GURL("http://google.com"));
  row.set_visit_count(10);
  row.set_last_visit_time(Time::Now() - TimeDelta::FromDays(10));

  ASSERT_TRUE(urls_sql_handler_.Insert(&row));
  URLRow url_row;
  ASSERT_TRUE(history_db_.GetURLRow(row.url_id(), &url_row));
  EXPECT_EQ(row.url(), url_row.url());
  EXPECT_EQ(10, url_row.visit_count());
  EXPECT_EQ(row.last_visit_time(), url_row.last_visit());

  HistoryAndBookmarkRow update_row;
  update_row.set_visit_count(11);
  TableIDRow id;
  id.url_id = url_row.id();
  TableIDRows ids;
  ids.push_back(id);
  ASSERT_TRUE(urls_sql_handler_.Update(update_row, ids));
  ASSERT_TRUE(history_db_.GetURLRow(row.url_id(), &url_row));
  EXPECT_EQ(row.url(), url_row.url());
  EXPECT_EQ(11, url_row.visit_count());
  EXPECT_LT(row.last_visit_time(), url_row.last_visit());
}

TEST_F(VisitSQLHandlerTest, Delete) {
  HistoryAndBookmarkRow row;
  row.set_raw_url("http://google.com");
  row.set_url(GURL("http://google.com"));
  row.set_visit_count(10);
  row.set_last_visit_time(Time::Now() - TimeDelta::FromDays(10));

  ASSERT_TRUE(urls_sql_handler_.Insert(&row));
  URLRow url_row;
  ASSERT_TRUE(history_db_.GetURLRow(row.url_id(), &url_row));
  EXPECT_EQ(row.url(), url_row.url());
  EXPECT_EQ(10, url_row.visit_count());
  EXPECT_EQ(row.last_visit_time(), url_row.last_visit());

  TableIDRow id;
  id.url_id = url_row.id();
  TableIDRows ids;
  ids.push_back(id);
  ASSERT_TRUE(urls_sql_handler_.Delete(ids));
  EXPECT_FALSE(history_db_.GetURLRow(row.url_id(), &url_row));
}

}  // namespace history
