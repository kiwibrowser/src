// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sessions/tab_loader.h"

#include <vector>

#include "base/memory/memory_coordinator_client_registry.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/memory_coordinator_delegate.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "content/public/test/memory_coordinator_test_utils.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_web_contents_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

class TabLoaderTest : public testing::Test {
 protected:
  using RestoredTab = SessionRestoreDelegate::RestoredTab;

  TabLoaderTest() = default;

  // testing::Test:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(features::kMemoryCoordinator);
    test_web_contents_factory_.reset(new content::TestWebContentsFactory);
  }

  void TearDown() override {
    // Clean up the |shared_tab_loader_|, this would reduce the ref counting and
    // thus it essentially deletes the |shared_tab_loader_|.
    if (TabLoader::shared_tab_loader_)
      TabLoader::shared_tab_loader_->this_retainer_ = nullptr;
    ASSERT_TRUE(TabLoader::shared_tab_loader_ == nullptr);
    test_web_contents_factory_.reset();
  }

  content::WebContents* CreateRestoredWebContents() {
    content::WebContents* test_contents =
        test_web_contents_factory_->CreateWebContents(&testing_profile_);
    std::vector<std::unique_ptr<content::NavigationEntry>> entries;
    entries.push_back(content::NavigationEntry::Create());
    test_contents->GetController().Restore(
        0, content::RestoreType::LAST_SESSION_EXITED_CLEANLY, &entries);
    return test_contents;
  }

  std::unique_ptr<content::TestWebContentsFactory> test_web_contents_factory_;
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile testing_profile_;
  base::test::ScopedFeatureList scoped_feature_list_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TabLoaderTest);
};

// TODO(hajimehoshi): Enable this test on macos when MemoryMonitorMac is
// implemented.
#if defined(OS_MACOSX)
#define MAYBE_OnMemoryStateChange DISABLED_OnMemoryStateChange
#else
#define MAYBE_OnMemoryStateChange OnMemoryStateChange
#endif
TEST_F(TabLoaderTest, MAYBE_OnMemoryStateChange) {
  content::SetUpMemoryCoordinatorProxyForTesting();

  std::vector<RestoredTab> restored_tabs;
  content::WebContents* contents = CreateRestoredWebContents();
  restored_tabs.push_back(RestoredTab(contents, false, false, false));

  TabLoader::RestoreTabs(restored_tabs, base::TimeTicks());
  EXPECT_TRUE(TabLoader::shared_tab_loader_->loading_enabled_);
  base::MemoryCoordinatorClientRegistry::GetInstance()->Notify(
      base::MemoryState::THROTTLED);
  // ObserverListThreadsafe is used to notify the state to clients, so running
  // the loop is necessary here.
  base::RunLoop loop;
  loop.RunUntilIdle();
  EXPECT_FALSE(TabLoader::shared_tab_loader_->loading_enabled_);
}

TEST_F(TabLoaderTest, UsePageAlmostIdleSignal) {
  std::vector<RestoredTab> restored_tabs;
  content::WebContents* web_contents1 = CreateRestoredWebContents();
  restored_tabs.push_back(RestoredTab(web_contents1, true, false, false));
  content::WebContents* web_contents2 = CreateRestoredWebContents();
  restored_tabs.push_back(RestoredTab(web_contents2, false, false, false));

  TabLoader::RestoreTabs(restored_tabs, base::TimeTicks());
  EXPECT_TRUE(TabLoader::shared_tab_loader_->loading_enabled_);
  EXPECT_EQ(1u, TabLoader::shared_tab_loader_->tabs_loading_.size());
  EXPECT_EQ(1u, TabLoader::shared_tab_loader_->tabs_to_load_.size());

  // By calling this, TabLoader now considers |web_contents1| is loaded, and
  // starts to load |web_contents2|.
  TabLoader::shared_tab_loader_->OnPageAlmostIdle(web_contents1);
  EXPECT_TRUE(TabLoader::shared_tab_loader_->loading_enabled_);
  EXPECT_EQ(1u, TabLoader::shared_tab_loader_->tabs_loading_.size());
  EXPECT_EQ(0u, TabLoader::shared_tab_loader_->tabs_to_load_.size());

  // |web_contents3| is not managed by TabLoader, thus it's ignored, and
  // shouldn't cause loading state change or crash.
  content::WebContents* web_contents3 = CreateRestoredWebContents();
  TabLoader::shared_tab_loader_->OnPageAlmostIdle(web_contents3);
  EXPECT_TRUE(TabLoader::shared_tab_loader_->loading_enabled_);
  EXPECT_EQ(1u, TabLoader::shared_tab_loader_->tabs_loading_.size());
  EXPECT_EQ(0u, TabLoader::shared_tab_loader_->tabs_to_load_.size());

  // Load rest of the tabs.
  TabLoader::shared_tab_loader_->OnPageAlmostIdle(web_contents2);
  EXPECT_TRUE(TabLoader::shared_tab_loader_->loading_enabled_);
  EXPECT_EQ(0u, TabLoader::shared_tab_loader_->tabs_loading_.size());
  EXPECT_EQ(0u, TabLoader::shared_tab_loader_->tabs_to_load_.size());
}
