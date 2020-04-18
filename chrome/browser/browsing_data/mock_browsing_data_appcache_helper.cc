// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/mock_browsing_data_appcache_helper.h"

#include "base/callback.h"
#include "testing/gtest/include/gtest/gtest.h"

MockBrowsingDataAppCacheHelper::MockBrowsingDataAppCacheHelper(
    content::BrowserContext* browser_context)
    : BrowsingDataAppCacheHelper(browser_context),
      response_(new content::AppCacheInfoCollection) {
}

MockBrowsingDataAppCacheHelper::~MockBrowsingDataAppCacheHelper() {
}

void MockBrowsingDataAppCacheHelper::StartFetching(
    const FetchCallback& completion_callback) {
  ASSERT_FALSE(completion_callback.is_null());
  ASSERT_TRUE(completion_callback_.is_null());
  completion_callback_ = completion_callback;
}

void MockBrowsingDataAppCacheHelper::DeleteAppCacheGroup(
    const GURL& manifest_url) {
}

void MockBrowsingDataAppCacheHelper::AddAppCacheSamples() {
  const GURL kOriginURL("http://hello/");
  const url::Origin kOrigin(url::Origin::Create(kOriginURL));
  content::AppCacheInfo mock_manifest_1;
  content::AppCacheInfo mock_manifest_2;
  content::AppCacheInfo mock_manifest_3;
  mock_manifest_1.manifest_url = kOriginURL.Resolve("manifest1");
  mock_manifest_1.size = 1;
  mock_manifest_2.manifest_url = kOriginURL.Resolve("manifest2");
  mock_manifest_2.size = 2;
  mock_manifest_3.manifest_url = kOriginURL.Resolve("manifest3");
  mock_manifest_3.size = 3;
  content::AppCacheInfoVector info_vector;
  info_vector.push_back(mock_manifest_1);
  info_vector.push_back(mock_manifest_2);
  info_vector.push_back(mock_manifest_3);
  response_->infos_by_origin[kOrigin] = info_vector;
}

void MockBrowsingDataAppCacheHelper::Notify() {
  completion_callback_.Run(response_);
}
