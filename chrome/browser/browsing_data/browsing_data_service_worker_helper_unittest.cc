// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/browsing_data_service_worker_helper.h"

#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class CannedBrowsingDataServiceWorkerHelperTest : public testing::Test {
 public:
  content::ServiceWorkerContext* ServiceWorkerContext() {
    return content::BrowserContext::GetDefaultStoragePartition(&profile_)->
        GetServiceWorkerContext();
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
};

TEST_F(CannedBrowsingDataServiceWorkerHelperTest, Empty) {
  const GURL origin("https://host1:1/");
  std::vector<GURL> scopes;
  scopes.push_back(GURL("https://host1:1/*"));

  scoped_refptr<CannedBrowsingDataServiceWorkerHelper> helper(
      new CannedBrowsingDataServiceWorkerHelper(ServiceWorkerContext()));

  ASSERT_TRUE(helper->empty());
  helper->AddServiceWorker(origin, scopes);
  ASSERT_FALSE(helper->empty());
  helper->Reset();
  ASSERT_TRUE(helper->empty());
}

TEST_F(CannedBrowsingDataServiceWorkerHelperTest, Delete) {
  const GURL origin1("http://host1:9000");
  std::vector<GURL> scopes1;
  scopes1.push_back(GURL("http://host1:9000/*"));

  const GURL origin2("https://example.com");
  std::vector<GURL> scopes2;
  scopes2.push_back(GURL("https://example.com/app1/*"));
  scopes2.push_back(GURL("https://example.com/app2/*"));

  scoped_refptr<CannedBrowsingDataServiceWorkerHelper> helper(
      new CannedBrowsingDataServiceWorkerHelper(ServiceWorkerContext()));

  EXPECT_TRUE(helper->empty());
  helper->AddServiceWorker(origin1, scopes1);
  helper->AddServiceWorker(origin2, scopes2);
  EXPECT_EQ(2u, helper->GetServiceWorkerCount());
  helper->DeleteServiceWorkers(origin2);
  EXPECT_EQ(1u, helper->GetServiceWorkerCount());
}

TEST_F(CannedBrowsingDataServiceWorkerHelperTest, IgnoreExtensionsAndDevTools) {
  const GURL origin1("chrome-extension://abcdefghijklmnopqrstuvwxyz/");
  const GURL origin2("chrome-devtools://abcdefghijklmnopqrstuvwxyz/");
  const std::vector<GURL> scopes;

  scoped_refptr<CannedBrowsingDataServiceWorkerHelper> helper(
      new CannedBrowsingDataServiceWorkerHelper(ServiceWorkerContext()));

  ASSERT_TRUE(helper->empty());
  helper->AddServiceWorker(origin1, scopes);
  ASSERT_TRUE(helper->empty());
  helper->AddServiceWorker(origin2, scopes);
  ASSERT_TRUE(helper->empty());
}

}  // namespace
