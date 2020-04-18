// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/mock_browsing_data_service_worker_helper.h"

#include <vector>

#include "base/callback.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "testing/gtest/include/gtest/gtest.h"

MockBrowsingDataServiceWorkerHelper::MockBrowsingDataServiceWorkerHelper(
    Profile* profile)
    : BrowsingDataServiceWorkerHelper(
        content::BrowserContext::GetDefaultStoragePartition(profile)->
            GetServiceWorkerContext()) {
}

MockBrowsingDataServiceWorkerHelper::~MockBrowsingDataServiceWorkerHelper() {
}

void MockBrowsingDataServiceWorkerHelper::StartFetching(
    const FetchCallback& callback) {
  ASSERT_FALSE(callback.is_null());
  ASSERT_TRUE(callback_.is_null());
  callback_ = callback;
}

void MockBrowsingDataServiceWorkerHelper::DeleteServiceWorkers(
    const GURL& origin) {
  ASSERT_FALSE(callback_.is_null());
  ASSERT_TRUE(base::ContainsKey(origins_, origin));
  origins_[origin] = false;
}

void MockBrowsingDataServiceWorkerHelper::AddServiceWorkerSamples() {
  const GURL kOrigin1("https://swhost1:1/");
  std::vector<GURL> scopes1;
  scopes1.push_back(GURL("https://swhost1:1/app1/*"));
  scopes1.push_back(GURL("https://swhost1:1/app2/*"));
  const GURL kOrigin2("https://swhost2:2/");
  std::vector<GURL> scopes2;
  scopes2.push_back(GURL("https://swhost2:2/*"));

  content::ServiceWorkerUsageInfo info1(kOrigin1, scopes1);
  info1.total_size_bytes = 1;
  response_.push_back(info1);
  origins_[kOrigin1] = true;

  content::ServiceWorkerUsageInfo info2(kOrigin2, scopes2);
  info2.total_size_bytes = 2;
  response_.push_back(info2);
  origins_[kOrigin2] = true;
}

void MockBrowsingDataServiceWorkerHelper::Notify() {
  callback_.Run(response_);
}

void MockBrowsingDataServiceWorkerHelper::Reset() {
  for (auto& pair : origins_)
    pair.second = true;
}

bool MockBrowsingDataServiceWorkerHelper::AllDeleted() {
  for (const auto& pair : origins_) {
    if (pair.second)
      return false;
  }
  return true;
}
