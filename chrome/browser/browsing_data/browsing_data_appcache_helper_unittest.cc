// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/browsing_data_appcache_helper.h"

#include <set>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/stl_util.h"
#include "base/test/scoped_task_environment.h"
#include "build/build_config.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
class TestCompletionCallback {
 public:
  TestCompletionCallback() {}

  bool have_result() const { return info_collection_.get(); }

  content::AppCacheInfoCollection* info_collection() const {
    return info_collection_.get();
  }

  void set_info_collection(
      scoped_refptr<content::AppCacheInfoCollection> info_collection) {
    info_collection_ = info_collection;
  }

 private:
  scoped_refptr<content::AppCacheInfoCollection> info_collection_;

  DISALLOW_COPY_AND_ASSIGN(TestCompletionCallback);
};

}  // namespace

class CannedBrowsingDataAppCacheHelperTest : public testing::Test {
 public:
  CannedBrowsingDataAppCacheHelperTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI),
        thread_bundle_(content::TestBrowserThreadBundle::REAL_IO_THREAD) {}

  void TearDown() override {
    // Make sure we run all pending tasks on IO thread before testing
    // profile is destructed.
    content::RunAllPendingInMessageLoop(content::BrowserThread::IO);
    scoped_task_environment_.RunUntilIdle();
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
};

TEST_F(CannedBrowsingDataAppCacheHelperTest, SetInfo) {
  GURL manifest1("http://example1.com/manifest.xml");
  GURL manifest2("http://example2.com/path1/manifest.xml");
  GURL manifest3("http://example2.com/path2/manifest.xml");

  scoped_refptr<CannedBrowsingDataAppCacheHelper> helper(
      new CannedBrowsingDataAppCacheHelper(profile_.GetOffTheRecordProfile()));
  helper->AddAppCache(manifest1);
  helper->AddAppCache(manifest2);
  helper->AddAppCache(manifest3);

  TestCompletionCallback callback;
  helper->StartFetching(base::Bind(&TestCompletionCallback::set_info_collection,
                                   base::Unretained(&callback)));
  ASSERT_TRUE(callback.have_result());

  std::map<url::Origin, content::AppCacheInfoVector>& collection =
      callback.info_collection()->infos_by_origin;

  ASSERT_EQ(2u, collection.size());
  EXPECT_TRUE(base::ContainsKey(collection, url::Origin::Create(manifest1)));
  ASSERT_EQ(1u, collection[url::Origin::Create(manifest1)].size());
  EXPECT_EQ(manifest1,
            collection[url::Origin::Create(manifest1)].at(0).manifest_url);

  EXPECT_TRUE(base::ContainsKey(collection, url::Origin::Create(manifest2)));
  EXPECT_EQ(2u, collection[url::Origin::Create(manifest2)].size());
  std::set<GURL> manifest_results;
  manifest_results.insert(
      collection[url::Origin::Create(manifest2)].at(0).manifest_url);
  manifest_results.insert(
      collection[url::Origin::Create(manifest2)].at(1).manifest_url);
  EXPECT_TRUE(base::ContainsKey(manifest_results, manifest2));
  EXPECT_TRUE(base::ContainsKey(manifest_results, manifest3));
}

TEST_F(CannedBrowsingDataAppCacheHelperTest, Unique) {
  GURL manifest("http://example.com/manifest.xml");

  scoped_refptr<CannedBrowsingDataAppCacheHelper> helper(
      new CannedBrowsingDataAppCacheHelper(profile_.GetOffTheRecordProfile()));
  helper->AddAppCache(manifest);
  helper->AddAppCache(manifest);

  TestCompletionCallback callback;
  helper->StartFetching(base::Bind(&TestCompletionCallback::set_info_collection,
                                   base::Unretained(&callback)));
  ASSERT_TRUE(callback.have_result());

  std::map<url::Origin, content::AppCacheInfoVector>& collection =
      callback.info_collection()->infos_by_origin;

  ASSERT_EQ(1u, collection.size());
  EXPECT_TRUE(base::ContainsKey(collection, url::Origin::Create(manifest)));
  ASSERT_EQ(1u, collection[url::Origin::Create(manifest)].size());
  EXPECT_EQ(manifest,
            collection[url::Origin::Create(manifest)].at(0).manifest_url);
}

TEST_F(CannedBrowsingDataAppCacheHelperTest, Empty) {
  GURL manifest("http://example.com/manifest.xml");

  scoped_refptr<CannedBrowsingDataAppCacheHelper> helper(
      new CannedBrowsingDataAppCacheHelper(profile_.GetOffTheRecordProfile()));

  ASSERT_TRUE(helper->empty());
  helper->AddAppCache(manifest);
  ASSERT_FALSE(helper->empty());
  helper->Reset();
  ASSERT_TRUE(helper->empty());
}

TEST_F(CannedBrowsingDataAppCacheHelperTest, Delete) {
  GURL manifest1("http://example.com/manifest1.xml");
  GURL manifest2("http://foo.example.com/manifest2.xml");
  GURL manifest3("http://bar.example.com/manifest3.xml");

  scoped_refptr<CannedBrowsingDataAppCacheHelper> helper(
      new CannedBrowsingDataAppCacheHelper(profile_.GetOffTheRecordProfile()));

  EXPECT_TRUE(helper->empty());
  helper->AddAppCache(manifest1);
  helper->AddAppCache(manifest2);
  helper->AddAppCache(manifest3);
  EXPECT_FALSE(helper->empty());
  EXPECT_EQ(3u, helper->GetAppCacheCount());
  helper->DeleteAppCacheGroup(manifest2);
  EXPECT_EQ(2u, helper->GetAppCacheCount());
  EXPECT_FALSE(base::ContainsKey(helper->GetOriginAppCacheInfoMap(),
                                 url::Origin::Create(manifest2)));
}

TEST_F(CannedBrowsingDataAppCacheHelperTest, IgnoreExtensionsAndDevTools) {
  GURL manifest1("chrome-extension://abcdefghijklmnopqrstuvwxyz/manifest.xml");
  GURL manifest2("chrome-devtools://abcdefghijklmnopqrstuvwxyz/manifest.xml");

  scoped_refptr<CannedBrowsingDataAppCacheHelper> helper(
      new CannedBrowsingDataAppCacheHelper(profile_.GetOffTheRecordProfile()));

  ASSERT_TRUE(helper->empty());
  helper->AddAppCache(manifest1);
  ASSERT_TRUE(helper->empty());
  helper->AddAppCache(manifest2);
  ASSERT_TRUE(helper->empty());
}
