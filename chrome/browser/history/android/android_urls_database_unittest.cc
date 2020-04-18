// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/history/core/browser/android/android_urls_database.h"

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/testing_profile.h"
#include "components/history/core/browser/history_constants.h"
#include "components/history/core/browser/history_database.h"
#include "components/history/core/test/history_unittest_base.h"
#include "components/history/core/test/test_history_database.h"

namespace history {

class AndroidURLsMigrationTest : public HistoryUnitTestBase {
 public:
  AndroidURLsMigrationTest() {}
  ~AndroidURLsMigrationTest() override {}

 protected:
  void SetUp() override {
    profile_.reset(new TestingProfile);

    base::FilePath data_path;
    ASSERT_TRUE(base::PathService::Get(chrome::DIR_TEST_DATA, &data_path));
    data_path = data_path.AppendASCII("History");

    history_db_name_ = profile_->GetPath().Append(kHistoryFilename);
    // Set up history as they would be before migration.
    ASSERT_NO_FATAL_FAILURE(
        ExecuteSQLScript(data_path.AppendASCII("history.21.sql"),
                         history_db_name_));
  }

 protected:
  base::FilePath history_db_name_;
  std::unique_ptr<TestingProfile> profile_;
};

// Disabled as this does not correctly set up all the tables so that migration
// fails. See http://crbug.com/175460 .
TEST_F(AndroidURLsMigrationTest, DISABLED_MigrateToVersion22) {
  TestHistoryDatabase db;
  ASSERT_EQ(sql::INIT_OK, db.Init(history_db_name_));
  // Migration has done.
  // The column of previous table shouldn't exist.
  EXPECT_FALSE(db.GetDB().DoesColumnExist("android_urls", "bookmark"));
  sql::Statement statement(db.GetDB().GetUniqueStatement(
      "SELECT id, url_id, raw_url FROM android_urls ORDER BY id ASC"));
  ASSERT_TRUE(statement.Step());
  EXPECT_EQ(1, statement.ColumnInt64(0));
  EXPECT_EQ("http://google.com/", statement.ColumnString(2));
  EXPECT_EQ(1, statement.ColumnInt64(1));

  ASSERT_TRUE(statement.Step());
  EXPECT_EQ(4, statement.ColumnInt64(0));
  EXPECT_EQ("www.google.com/", statement.ColumnString(2));
  EXPECT_EQ(3, statement.ColumnInt64(1));

  EXPECT_FALSE(statement.Step());
}

}  // namespace history
