// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/browsing_data_database_helper.h"

#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "storage/common/database/database_identifier.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using storage::DatabaseIdentifier;

class CannedBrowsingDataDatabaseHelperTest : public testing::Test {
  content::TestBrowserThreadBundle thread_bundle_;
};

TEST_F(CannedBrowsingDataDatabaseHelperTest, Empty) {
  TestingProfile profile;

  const GURL origin("http://host1:1/");
  const char db[] = "db1";

  scoped_refptr<CannedBrowsingDataDatabaseHelper> helper(
      new CannedBrowsingDataDatabaseHelper(&profile));

  ASSERT_TRUE(helper->empty());
  helper->AddDatabase(origin, db, std::string());
  ASSERT_FALSE(helper->empty());
  helper->Reset();
  ASSERT_TRUE(helper->empty());
}

TEST_F(CannedBrowsingDataDatabaseHelperTest, Delete) {
  TestingProfile profile;

  const GURL origin1("http://host1:9000");
  const char db1[] = "db1";

  const GURL origin2("http://example.com");
  const char db2[] = "db2";

  const GURL origin3("http://foo.example.com");
  const char db3[] = "db3";

  scoped_refptr<CannedBrowsingDataDatabaseHelper> helper(
      new CannedBrowsingDataDatabaseHelper(&profile));

  EXPECT_TRUE(helper->empty());
  helper->AddDatabase(origin1, db1, std::string());
  helper->AddDatabase(origin2, db2, std::string());
  helper->AddDatabase(origin3, db3, std::string());
  EXPECT_EQ(3u, helper->GetDatabaseCount());
  helper->DeleteDatabase(
      DatabaseIdentifier::CreateFromOrigin(origin2).ToString(), db1);
  EXPECT_EQ(3u, helper->GetDatabaseCount());
  helper->DeleteDatabase(
      DatabaseIdentifier::CreateFromOrigin(origin2).ToString(), db2);
  EXPECT_EQ(2u, helper->GetDatabaseCount());
}

TEST_F(CannedBrowsingDataDatabaseHelperTest, IgnoreExtensionsAndDevTools) {
  TestingProfile profile;

  const GURL origin1("chrome-extension://abcdefghijklmnopqrstuvwxyz/");
  const GURL origin2("chrome-devtools://abcdefghijklmnopqrstuvwxyz/");
  const char db[] = "db1";

  scoped_refptr<CannedBrowsingDataDatabaseHelper> helper(
      new CannedBrowsingDataDatabaseHelper(&profile));

  ASSERT_TRUE(helper->empty());
  helper->AddDatabase(origin1, db, std::string());
  ASSERT_TRUE(helper->empty());
  helper->AddDatabase(origin2, db, std::string());
  ASSERT_TRUE(helper->empty());
  helper->Reset();
  ASSERT_TRUE(helper->empty());
}

}  // namespace
