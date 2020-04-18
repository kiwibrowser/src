// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/browsing_data_cache_storage_helper.h"

#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class CannedBrowsingDataCacheStorageHelperTest : public testing::Test {
 public:
  content::CacheStorageContext* CacheStorageContext() {
    return content::BrowserContext::GetDefaultStoragePartition(&profile_)
        ->GetCacheStorageContext();
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
};

TEST_F(CannedBrowsingDataCacheStorageHelperTest, Empty) {
  const GURL origin("http://host1:1/");

  scoped_refptr<CannedBrowsingDataCacheStorageHelper> helper(
      new CannedBrowsingDataCacheStorageHelper(CacheStorageContext()));

  ASSERT_TRUE(helper->empty());
  helper->AddCacheStorage(origin);
  ASSERT_FALSE(helper->empty());
  helper->Reset();
  ASSERT_TRUE(helper->empty());
}

TEST_F(CannedBrowsingDataCacheStorageHelperTest, Delete) {
  const GURL origin1("http://host1:9000");
  const GURL origin2("http://example.com");

  scoped_refptr<CannedBrowsingDataCacheStorageHelper> helper(
      new CannedBrowsingDataCacheStorageHelper(CacheStorageContext()));

  EXPECT_TRUE(helper->empty());
  helper->AddCacheStorage(origin1);
  helper->AddCacheStorage(origin2);
  helper->AddCacheStorage(origin2);
  EXPECT_EQ(2u, helper->GetCacheStorageCount());
  helper->DeleteCacheStorage(origin2);
  EXPECT_EQ(1u, helper->GetCacheStorageCount());
}

TEST_F(CannedBrowsingDataCacheStorageHelperTest, IgnoreExtensionsAndDevTools) {
  const GURL origin1("chrome-extension://abcdefghijklmnopqrstuvwxyz/");
  const GURL origin2("chrome-devtools://abcdefghijklmnopqrstuvwxyz/");

  scoped_refptr<CannedBrowsingDataCacheStorageHelper> helper(
      new CannedBrowsingDataCacheStorageHelper(CacheStorageContext()));

  ASSERT_TRUE(helper->empty());
  helper->AddCacheStorage(origin1);
  ASSERT_TRUE(helper->empty());
  helper->AddCacheStorage(origin2);
  ASSERT_TRUE(helper->empty());
}

}  // namespace
