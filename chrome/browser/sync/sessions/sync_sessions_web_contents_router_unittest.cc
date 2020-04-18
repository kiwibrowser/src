// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/sessions/sync_sessions_web_contents_router.h"
#include "chrome/browser/sync/sessions/sync_sessions_web_contents_router_factory.h"
#include "chrome/browser/ui/sync/tab_contents_synced_tab_delegate.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sync_sessions {

class StartSyncFlareMock {
 public:
  StartSyncFlareMock() {}
  ~StartSyncFlareMock() {}

  void StartSyncFlare(syncer::ModelType type) { was_run_ = true; }

  bool was_run() { return was_run_; }

 private:
  bool was_run_ = false;
};

class SyncSessionsWebContentsRouterTest : public testing::Test {
 protected:
  ~SyncSessionsWebContentsRouterTest() override {}

  void SetUp() override {
    router_ =
        SyncSessionsWebContentsRouterFactory::GetInstance()->GetForProfile(
            &profile_);
    test_contents_ =
        content::WebContentsTester::CreateTestWebContents(&profile_, nullptr);
  }

  SyncSessionsWebContentsRouter* router() { return router_; }
  content::WebContents* test_contents() { return test_contents_.get(); }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
  SyncSessionsWebContentsRouter* router_;
  std::unique_ptr<content::WebContents> test_contents_;
};

TEST_F(SyncSessionsWebContentsRouterTest, FlareNotRun) {
  StartSyncFlareMock mock;
  router()->InjectStartSyncFlare(
      base::Bind(&StartSyncFlareMock::StartSyncFlare, base::Unretained(&mock)));

  // There's no delegate for the tab, so the flare shouldn't run.
  router()->NotifyTabModified(test_contents(), false);
  EXPECT_FALSE(mock.was_run());

  TabContentsSyncedTabDelegate::CreateForWebContents(test_contents());

  // There's a delegate for the tab, but it's not a load completed event, so the
  // flare still shouldn't run.
  router()->NotifyTabModified(test_contents(), false);
  EXPECT_FALSE(mock.was_run());
}

// Make sure we don't crash when there's not a flare.
TEST_F(SyncSessionsWebContentsRouterTest, FlareNotSet) {
  TabContentsSyncedTabDelegate::CreateForWebContents(test_contents());
  router()->NotifyTabModified(test_contents(), false);
}

// Disabled on android due to complexity of creating a full TabAndroid object
// for a unit test. The logic being tested here isn't directly affected by
// platform-specific peculiarities.
#if !defined(OS_ANDROID)
TEST_F(SyncSessionsWebContentsRouterTest, FlareRunsForLoadCompleted) {
  TabContentsSyncedTabDelegate::CreateForWebContents(test_contents());

  StartSyncFlareMock mock;
  router()->InjectStartSyncFlare(
      base::Bind(&StartSyncFlareMock::StartSyncFlare, base::Unretained(&mock)));

  // There's a delegate for the tab, and it's a load completed event, so the
  // flare should run.
  router()->NotifyTabModified(test_contents(), true);
  EXPECT_TRUE(mock.was_run());
}
#endif  // !defined(OS_ANDROID)

}  // namespace sync_sessions
