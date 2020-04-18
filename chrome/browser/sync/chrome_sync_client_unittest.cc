// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/chrome_sync_client.h"

#include <memory>
#include <string>

#include "chrome/common/url_constants.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync/driver/sync_api_component_factory.h"
#include "components/sync_sessions/sync_sessions_client.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace browser_sync {

namespace {

const char kValidUrl[] = "http://www.example.com";
const char kInvalidUrl[] = "invalid.url";

class ChromeSyncClientTest : public testing::Test {
 public:
  ChromeSyncClientTest()
      : profile_(new TestingProfile()),
        sync_client_(new ChromeSyncClient(profile_.get())) {}
  ~ChromeSyncClientTest() override {}

  ChromeSyncClient* sync_client() { return sync_client_.get(); }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<ChromeSyncClient> sync_client_;
};

TEST_F(ChromeSyncClientTest, ShouldSyncURL) {
  EXPECT_TRUE(
      sync_client()->GetSyncSessionsClient()->ShouldSyncURL(GURL(kValidUrl)));
  EXPECT_TRUE(sync_client()->GetSyncSessionsClient()->ShouldSyncURL(
      GURL("other://anything")));
  EXPECT_TRUE(sync_client()->GetSyncSessionsClient()->ShouldSyncURL(
      GURL("chrome-other://anything")));

  EXPECT_FALSE(
      sync_client()->GetSyncSessionsClient()->ShouldSyncURL(GURL(kInvalidUrl)));
  EXPECT_FALSE(sync_client()->GetSyncSessionsClient()->ShouldSyncURL(
      GURL("file://anything")));
  EXPECT_FALSE(sync_client()->GetSyncSessionsClient()->ShouldSyncURL(
      GURL("chrome://anything")));
  EXPECT_FALSE(sync_client()->GetSyncSessionsClient()->ShouldSyncURL(
      GURL("chrome-native://anything")));

  EXPECT_TRUE(sync_client()->GetSyncSessionsClient()->ShouldSyncURL(
      GURL(chrome::kChromeUIHistoryURL)));
}

}  // namespace

}  // namespace browser_sync
