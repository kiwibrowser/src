// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/browsing_data_indexed_db_helper.h"

#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class CannedBrowsingDataIndexedDBHelperTest : public testing::Test {
 public:
  content::IndexedDBContext* IndexedDBContext() {
    return content::BrowserContext::GetDefaultStoragePartition(&profile_)->
        GetIndexedDBContext();
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
};

TEST_F(CannedBrowsingDataIndexedDBHelperTest, Empty) {
  const GURL origin("http://host1:1/");
  const base::string16 description(base::ASCIIToUTF16("description"));

  scoped_refptr<CannedBrowsingDataIndexedDBHelper> helper(
      new CannedBrowsingDataIndexedDBHelper(IndexedDBContext()));

  ASSERT_TRUE(helper->empty());
  helper->AddIndexedDB(origin, description);
  ASSERT_FALSE(helper->empty());
  helper->Reset();
  ASSERT_TRUE(helper->empty());
}

TEST_F(CannedBrowsingDataIndexedDBHelperTest, Delete) {
  const GURL origin1("http://host1:9000");
  const base::string16 db1(base::ASCIIToUTF16("db1"));

  const GURL origin2("http://example.com");
  const base::string16 db2(base::ASCIIToUTF16("db2"));
  const base::string16 db3(base::ASCIIToUTF16("db3"));

  scoped_refptr<CannedBrowsingDataIndexedDBHelper> helper(
      new CannedBrowsingDataIndexedDBHelper(IndexedDBContext()));

  EXPECT_TRUE(helper->empty());
  helper->AddIndexedDB(origin1, db1);
  helper->AddIndexedDB(origin2, db2);
  helper->AddIndexedDB(origin2, db3);
  EXPECT_EQ(3u, helper->GetIndexedDBCount());
  helper->DeleteIndexedDB(origin2);
  EXPECT_EQ(1u, helper->GetIndexedDBCount());
}

TEST_F(CannedBrowsingDataIndexedDBHelperTest, IgnoreExtensionsAndDevTools) {
  const GURL origin1("chrome-extension://abcdefghijklmnopqrstuvwxyz/");
  const GURL origin2("chrome-devtools://abcdefghijklmnopqrstuvwxyz/");
  const base::string16 description(base::ASCIIToUTF16("description"));

  scoped_refptr<CannedBrowsingDataIndexedDBHelper> helper(
      new CannedBrowsingDataIndexedDBHelper(IndexedDBContext()));

  ASSERT_TRUE(helper->empty());
  helper->AddIndexedDB(origin1, description);
  ASSERT_TRUE(helper->empty());
  helper->AddIndexedDB(origin2, description);
  ASSERT_TRUE(helper->empty());
}

}  // namespace
