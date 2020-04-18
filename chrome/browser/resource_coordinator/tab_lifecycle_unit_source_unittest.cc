// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_lifecycle_unit_source.h"

#include <memory>

#include "base/macros.h"
#include "base/message_loop/message_loop_current.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/test/test_mock_time_task_runner.h"
#include "build/build_config.h"
#include "chrome/browser/resource_coordinator/lifecycle_unit_observer.h"
#include "chrome/browser/resource_coordinator/lifecycle_unit_source_observer.h"
#include "chrome/browser/resource_coordinator/tab_lifecycle_observer.h"
#include "chrome/browser/resource_coordinator/tab_lifecycle_unit.h"
#include "chrome/browser/resource_coordinator/time.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/test_tab_strip_model_delegate.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace resource_coordinator {

namespace {

constexpr base::TimeDelta kShortDelay = base::TimeDelta::FromSeconds(1);

class NoUnloadListenerTabStripModelDelegate : public TestTabStripModelDelegate {
 public:
  NoUnloadListenerTabStripModelDelegate() = default;
  bool RunUnloadListenerBeforeClosing(content::WebContents* contents) override {
    // The default TestTabStripModelDelegate prevents tabs from being closed.
    return false;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(NoUnloadListenerTabStripModelDelegate);
};

class MockLifecycleUnitSourceObserver : public LifecycleUnitSourceObserver {
 public:
  MockLifecycleUnitSourceObserver() = default;

  MOCK_METHOD1(OnLifecycleUnitCreated, void(LifecycleUnit*));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockLifecycleUnitSourceObserver);
};

class MockTabLifecycleObserver : public TabLifecycleObserver {
 public:
  MockTabLifecycleObserver() = default;

  MOCK_METHOD2(OnDiscardedStateChange,
               void(content::WebContents* contents, bool is_discarded));
  MOCK_METHOD2(OnAutoDiscardableStateChange,
               void(content::WebContents* contents, bool is_auto_discardable));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockTabLifecycleObserver);
};

class MockLifecycleUnitObserver : public LifecycleUnitObserver {
 public:
  MockLifecycleUnitObserver() = default;

  MOCK_METHOD2(OnLifecycleUnitStateChanged,
               void(LifecycleUnit* lifecycle_unit, LifecycleState));
  MOCK_METHOD2(OnLifecycleUnitVisibilityChanged,
               void(LifecycleUnit* lifecycle_unit,
                    content::Visibility visibility));
  MOCK_METHOD1(OnLifecycleUnitDestroyed, void(LifecycleUnit* lifecycle_unit));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockLifecycleUnitObserver);
};

bool IsFocused(LifecycleUnit* lifecycle_unit) {
  return lifecycle_unit->GetLastFocusedTime() == base::TimeTicks::Max();
}

#define EXPECT_FOR_ALL_DISCARD_REASONS(lifecycle_unit, method, value)    \
  do {                                                                   \
    EXPECT_EQ(value, lifecycle_unit->method(DiscardReason::kExternal));  \
    EXPECT_EQ(value, lifecycle_unit->method(DiscardReason::kProactive)); \
    EXPECT_EQ(value, lifecycle_unit->method(DiscardReason::kUrgent));    \
  } while (false)

class TabLifecycleUnitSourceTest : public ChromeRenderViewHostTestHarness {
 protected:
  TabLifecycleUnitSourceTest()
      : scoped_context_(
            std::make_unique<base::TestMockTimeTaskRunner::ScopedContext>(
                task_runner_)),
        scoped_set_tick_clock_for_testing_(task_runner_->GetMockTickClock()) {
    base::MessageLoopCurrent::Get()->SetTaskRunner(task_runner_);
  }

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    source_ = TabLifecycleUnitSource::GetInstance();
    source_->AddObserver(&source_observer_);
    source_->AddTabLifecycleObserver(&tab_observer_);

    tab_strip_model_ =
        std::make_unique<TabStripModel>(&tab_strip_model_delegate_, profile());
    tab_strip_model_->AddObserver(source_);
  }

  void TearDown() override {
    tab_strip_model_->CloseAllTabs();
    tab_strip_model_.reset();

    task_runner_->RunUntilIdle();
    scoped_context_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  // If |focus_tab_strip| is true, focuses the tab strip. Then, appends 2 tabs
  // to the tab strip and returns the associated LifecycleUnits via
  // |first_lifecycle_unit| and |second_lifecycle_unit|. The first tab is
  // background and the second tab is active.
  void CreateTwoTabs(bool focus_tab_strip,
                     LifecycleUnit** first_lifecycle_unit,
                     LifecycleUnit** second_lifecycle_unit) {
    if (focus_tab_strip)
      source_->SetFocusedTabStripModelForTesting(tab_strip_model_.get());

    // Add a foreground tab to the tab strip.
    task_runner_->FastForwardBy(kShortDelay);
    auto time_before_first_tab = NowTicks();
    EXPECT_CALL(source_observer_, OnLifecycleUnitCreated(testing::_))
        .WillOnce(testing::Invoke([&](LifecycleUnit* lifecycle_unit) {
          *first_lifecycle_unit = lifecycle_unit;

          if (focus_tab_strip) {
            EXPECT_TRUE(IsFocused(*first_lifecycle_unit));
          } else {
            EXPECT_EQ(time_before_first_tab,
                      (*first_lifecycle_unit)->GetLastFocusedTime());
          }
        }));
    std::unique_ptr<content::WebContents> first_web_contents =
        CreateAndNavigateWebContents();
    content::WebContents* raw_first_web_contents = first_web_contents.get();
    tab_strip_model_->AppendWebContents(std::move(first_web_contents), true);
    testing::Mock::VerifyAndClear(&source_observer_);
    EXPECT_TRUE(source_->GetTabLifecycleUnitExternal(raw_first_web_contents));

    // Add another foreground tab to the focused tab strip.
    task_runner_->FastForwardBy(kShortDelay);
    auto time_before_second_tab = NowTicks();
    EXPECT_CALL(source_observer_, OnLifecycleUnitCreated(testing::_))
        .WillOnce(testing::Invoke([&](LifecycleUnit* lifecycle_unit) {
          *second_lifecycle_unit = lifecycle_unit;

          if (focus_tab_strip) {
            EXPECT_EQ(time_before_second_tab,
                      (*first_lifecycle_unit)->GetLastFocusedTime());
            EXPECT_TRUE(IsFocused(*second_lifecycle_unit));
          } else {
            EXPECT_EQ(time_before_first_tab,
                      (*first_lifecycle_unit)->GetLastFocusedTime());
            EXPECT_EQ(time_before_second_tab,
                      (*second_lifecycle_unit)->GetLastFocusedTime());
          }
        }));
    std::unique_ptr<content::WebContents> second_web_contents =
        CreateAndNavigateWebContents();
    content::WebContents* raw_second_web_contents = second_web_contents.get();
    tab_strip_model_->AppendWebContents(std::move(second_web_contents), true);
    testing::Mock::VerifyAndClear(&source_observer_);
    EXPECT_TRUE(source_->GetTabLifecycleUnitExternal(raw_second_web_contents));

    // TabStripModel doesn't update the visibility of its WebContents by itself.
    raw_first_web_contents->WasHidden();
  }

  void TestAppendTabsToTabStrip(bool focus_tab_strip) {
    LifecycleUnit* first_lifecycle_unit = nullptr;
    LifecycleUnit* second_lifecycle_unit = nullptr;
    CreateTwoTabs(focus_tab_strip, &first_lifecycle_unit,
                  &second_lifecycle_unit);

    const base::TimeTicks first_tab_last_focused_time =
        first_lifecycle_unit->GetLastFocusedTime();
    const base::TimeTicks second_tab_last_focused_time =
        second_lifecycle_unit->GetLastFocusedTime();

    // Add a background tab to the focused tab strip.
    task_runner_->FastForwardBy(kShortDelay);
    LifecycleUnit* third_lifecycle_unit = nullptr;
    EXPECT_CALL(source_observer_, OnLifecycleUnitCreated(testing::_))
        .WillOnce(testing::Invoke([&](LifecycleUnit* lifecycle_unit) {
          third_lifecycle_unit = lifecycle_unit;

          if (focus_tab_strip) {
            EXPECT_EQ(first_tab_last_focused_time,
                      first_lifecycle_unit->GetLastFocusedTime());
            EXPECT_TRUE(IsFocused(second_lifecycle_unit));
          } else {
            EXPECT_EQ(first_tab_last_focused_time,
                      first_lifecycle_unit->GetLastFocusedTime());
            EXPECT_EQ(second_tab_last_focused_time,
                      second_lifecycle_unit->GetLastFocusedTime());
          }
          EXPECT_EQ(NowTicks(), third_lifecycle_unit->GetLastFocusedTime());
        }));
    std::unique_ptr<content::WebContents> third_web_contents =
        CreateAndNavigateWebContents();
    content::WebContents* raw_third_web_contents = third_web_contents.get();
    tab_strip_model_->AppendWebContents(std::move(third_web_contents), false);
    testing::Mock::VerifyAndClear(&source_observer_);
    EXPECT_TRUE(source_->GetTabLifecycleUnitExternal(raw_third_web_contents));

    // Expect notifications when tabs are closed.
    CloseTabsAndExpectNotifications(
        tab_strip_model_.get(),
        {first_lifecycle_unit, second_lifecycle_unit, third_lifecycle_unit});
  }

  void CloseTabsAndExpectNotifications(
      TabStripModel* tab_strip_model,
      std::vector<LifecycleUnit*> lifecycle_units) {
    std::vector<std::unique_ptr<testing::StrictMock<MockLifecycleUnitObserver>>>
        observers;
    for (LifecycleUnit* lifecycle_unit : lifecycle_units) {
      observers.emplace_back(
          std::make_unique<testing::StrictMock<MockLifecycleUnitObserver>>());
      lifecycle_unit->AddObserver(observers.back().get());
      EXPECT_CALL(*observers.back().get(),
                  OnLifecycleUnitDestroyed(lifecycle_unit));
    }
    tab_strip_model->CloseAllTabs();
  }

  void TransitionFromPendingDiscardToDiscardedIfNeeded(
      DiscardReason reason,
      LifecycleUnit* lifecycle_unit) {
    if (reason == DiscardReason::kProactive) {
      EXPECT_EQ(LifecycleState::PENDING_DISCARD, lifecycle_unit->GetState());
      task_runner_->FastForwardBy(kProactiveDiscardFreezeTimeout);
    }
    EXPECT_EQ(LifecycleState::DISCARDED, lifecycle_unit->GetState());
  }

  void DetachWebContentsTest(DiscardReason reason) {
    LifecycleUnit* first_lifecycle_unit = nullptr;
    LifecycleUnit* second_lifecycle_unit = nullptr;
    CreateTwoTabs(true /* focus_tab_strip */, &first_lifecycle_unit,
                  &second_lifecycle_unit);

    // Detach the non-active tab. Verify that it can no longer be discarded.
    EXPECT_FOR_ALL_DISCARD_REASONS(first_lifecycle_unit, CanDiscard, true);
    std::unique_ptr<content::WebContents> owned_contents =
        tab_strip_model_->DetachWebContentsAt(0);
    EXPECT_FOR_ALL_DISCARD_REASONS(first_lifecycle_unit, CanDiscard, false);

    // Create a second tab strip.
    NoUnloadListenerTabStripModelDelegate other_tab_strip_model_delegate;
    TabStripModel other_tab_strip_model(&other_tab_strip_model_delegate,
                                        profile());
    other_tab_strip_model.AddObserver(source_);

    // Make sure that the second tab strip has a foreground tab.
    EXPECT_CALL(source_observer_, OnLifecycleUnitCreated(testing::_));
    other_tab_strip_model.AppendWebContents(CreateTestWebContents(),
                                            /*foreground=*/true);

    // Insert the tab into the second tab strip without focusing it. Verify that
    // it can be discarded.
    other_tab_strip_model.AppendWebContents(std::move(owned_contents), false);
    EXPECT_FOR_ALL_DISCARD_REASONS(first_lifecycle_unit, CanDiscard, true);

    EXPECT_EQ(LifecycleState::ACTIVE, first_lifecycle_unit->GetState());
    EXPECT_CALL(tab_observer_, OnDiscardedStateChange(testing::_, true));
    first_lifecycle_unit->Discard(reason);

    testing::Mock::VerifyAndClear(&tab_observer_);
    TransitionFromPendingDiscardToDiscardedIfNeeded(reason,
                                                    first_lifecycle_unit);

    // Expect a notification when the tab is closed.
    CloseTabsAndExpectNotifications(&other_tab_strip_model,
                                    {first_lifecycle_unit});
  }

  void DiscardTest(DiscardReason reason) {
    const base::TimeTicks kDummyLastActiveTime =
        base::TimeTicks() + kShortDelay;

    LifecycleUnit* background_lifecycle_unit = nullptr;
    LifecycleUnit* foreground_lifecycle_unit = nullptr;
    CreateTwoTabs(true /* focus_tab_strip */, &background_lifecycle_unit,
                  &foreground_lifecycle_unit);
    content::WebContents* initial_web_contents =
        tab_strip_model_->GetWebContentsAt(0);
    initial_web_contents->SetLastActiveTime(kDummyLastActiveTime);

    // Discard the tab.
    EXPECT_EQ(LifecycleState::ACTIVE, background_lifecycle_unit->GetState());
    EXPECT_CALL(tab_observer_, OnDiscardedStateChange(testing::_, true));
    background_lifecycle_unit->Discard(reason);
    testing::Mock::VerifyAndClear(&tab_observer_);

    // Expect the tab to be discarded and the last active time to be preserved.
    TransitionFromPendingDiscardToDiscardedIfNeeded(reason,
                                                    background_lifecycle_unit);

    EXPECT_NE(initial_web_contents, tab_strip_model_->GetWebContentsAt(0));
    EXPECT_FALSE(tab_strip_model_->GetWebContentsAt(0)
                     ->GetController()
                     .GetPendingEntry());
    EXPECT_EQ(kDummyLastActiveTime,
              tab_strip_model_->GetWebContentsAt(0)->GetLastActiveTime());

    source_->SetFocusedTabStripModelForTesting(nullptr);
  }

  void DiscardAndActivateTest(DiscardReason reason) {
    LifecycleUnit* background_lifecycle_unit = nullptr;
    LifecycleUnit* foreground_lifecycle_unit = nullptr;
    CreateTwoTabs(true /* focus_tab_strip */, &background_lifecycle_unit,
                  &foreground_lifecycle_unit);
    content::WebContents* initial_web_contents =
        tab_strip_model_->GetWebContentsAt(0);

    // Discard the tab.
    EXPECT_EQ(LifecycleState::ACTIVE, background_lifecycle_unit->GetState());
    EXPECT_CALL(tab_observer_, OnDiscardedStateChange(testing::_, true));
    background_lifecycle_unit->Discard(reason);
    testing::Mock::VerifyAndClear(&tab_observer_);

    TransitionFromPendingDiscardToDiscardedIfNeeded(reason,
                                                    background_lifecycle_unit);

    EXPECT_NE(initial_web_contents, tab_strip_model_->GetWebContentsAt(0));
    EXPECT_FALSE(tab_strip_model_->GetWebContentsAt(0)
                     ->GetController()
                     .GetPendingEntry());

    // Focus the tab. Expect the state to be ACTIVE.
    EXPECT_CALL(tab_observer_, OnDiscardedStateChange(testing::_, false));
    tab_strip_model_->ActivateTabAt(0, true);
    testing::Mock::VerifyAndClear(&tab_observer_);
    EXPECT_EQ(LifecycleState::ACTIVE, background_lifecycle_unit->GetState());
    EXPECT_TRUE(tab_strip_model_->GetWebContentsAt(0)
                    ->GetController()
                    .GetPendingEntry());
  }

  void DiscardAndExplicitlyReloadTest(DiscardReason reason) {
    LifecycleUnit* background_lifecycle_unit = nullptr;
    LifecycleUnit* foreground_lifecycle_unit = nullptr;
    CreateTwoTabs(true /* focus_tab_strip */, &background_lifecycle_unit,
                  &foreground_lifecycle_unit);
    content::WebContents* initial_web_contents =
        tab_strip_model_->GetWebContentsAt(0);

    // Discard the tab.
    EXPECT_EQ(LifecycleState::ACTIVE, background_lifecycle_unit->GetState());
    EXPECT_CALL(tab_observer_, OnDiscardedStateChange(testing::_, true));
    background_lifecycle_unit->Discard(reason);
    testing::Mock::VerifyAndClear(&tab_observer_);

    TransitionFromPendingDiscardToDiscardedIfNeeded(reason,
                                                    background_lifecycle_unit);

    EXPECT_NE(initial_web_contents, tab_strip_model_->GetWebContentsAt(0));
    EXPECT_FALSE(tab_strip_model_->GetWebContentsAt(0)
                     ->GetController()
                     .GetPendingEntry());

    // Explicitly reload the tab. Expect the state to be ACTIVE.
    EXPECT_CALL(tab_observer_, OnDiscardedStateChange(testing::_, false));
    tab_strip_model_->GetWebContentsAt(0)->GetController().Reload(
        content::ReloadType::NORMAL, false);
    testing::Mock::VerifyAndClear(&tab_observer_);
    EXPECT_EQ(LifecycleState::ACTIVE, background_lifecycle_unit->GetState());
    EXPECT_TRUE(tab_strip_model_->GetWebContentsAt(0)
                    ->GetController()
                    .GetPendingEntry());
  }

  void CanOnlyDiscardOnceTest(DiscardReason reason) {
    LifecycleUnit* background_lifecycle_unit = nullptr;
    LifecycleUnit* foreground_lifecycle_unit = nullptr;
    CreateTwoTabs(true /* focus_tab_strip */, &background_lifecycle_unit,
                  &foreground_lifecycle_unit);
    content::WebContents* initial_web_contents =
        tab_strip_model_->GetWebContentsAt(0);

    // It should be possible to discard the background tab.
    EXPECT_FOR_ALL_DISCARD_REASONS(background_lifecycle_unit, CanDiscard, true);

    // Discard the tab.
    EXPECT_EQ(LifecycleState::ACTIVE, background_lifecycle_unit->GetState());
    EXPECT_CALL(tab_observer_, OnDiscardedStateChange(testing::_, true));
    background_lifecycle_unit->Discard(reason);

    testing::Mock::VerifyAndClear(&tab_observer_);

    TransitionFromPendingDiscardToDiscardedIfNeeded(reason,
                                                    background_lifecycle_unit);

    EXPECT_NE(initial_web_contents, tab_strip_model_->GetWebContentsAt(0));
    EXPECT_FALSE(tab_strip_model_->GetWebContentsAt(0)
                     ->GetController()
                     .GetPendingEntry());

    // Explicitly reload the tab. Expect the state to be LOADED.
    EXPECT_CALL(tab_observer_, OnDiscardedStateChange(testing::_, false));
    tab_strip_model_->GetWebContentsAt(0)->GetController().Reload(
        content::ReloadType::NORMAL, false);
    testing::Mock::VerifyAndClear(&tab_observer_);
    EXPECT_EQ(LifecycleState::ACTIVE, background_lifecycle_unit->GetState());
    EXPECT_TRUE(tab_strip_model_->GetWebContentsAt(0)
                    ->GetController()
                    .GetPendingEntry());

    // It shouldn't be possible to discard the background tab again, except for
    // an urgent discard on ChromeOS.
    EXPECT_FALSE(
        background_lifecycle_unit->CanDiscard(DiscardReason::kExternal));
    EXPECT_FALSE(
        background_lifecycle_unit->CanDiscard(DiscardReason::kProactive));
#if defined(OS_CHROMEOS)
    EXPECT_TRUE(background_lifecycle_unit->CanDiscard(DiscardReason::kUrgent));
#else
    EXPECT_FALSE(background_lifecycle_unit->CanDiscard(DiscardReason::kUrgent));
#endif
  }

  TabLifecycleUnitSource* source_ = nullptr;
  testing::StrictMock<MockLifecycleUnitSourceObserver> source_observer_;
  testing::StrictMock<MockTabLifecycleObserver> tab_observer_;
  std::unique_ptr<TabStripModel> tab_strip_model_;
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_ =
      base::MakeRefCounted<base::TestMockTimeTaskRunner>();
  std::unique_ptr<base::TestMockTimeTaskRunner::ScopedContext> scoped_context_;

 private:
  std::unique_ptr<content::WebContents> CreateAndNavigateWebContents() {
    std::unique_ptr<content::WebContents> web_contents =
        CreateTestWebContents();
    // Commit an URL to allow discarding.
    content::WebContentsTester::For(web_contents.get())
        ->NavigateAndCommit(GURL("https://www.example.com"));
    return web_contents;
  }

  NoUnloadListenerTabStripModelDelegate tab_strip_model_delegate_;
  ScopedSetTickClockForTesting scoped_set_tick_clock_for_testing_;

  DISALLOW_COPY_AND_ASSIGN(TabLifecycleUnitSourceTest);
};

}  // namespace

TEST_F(TabLifecycleUnitSourceTest, AppendTabsToFocusedTabStrip) {
  TestAppendTabsToTabStrip(true /* focus_tab_strip */);
}

TEST_F(TabLifecycleUnitSourceTest, AppendTabsToNonFocusedTabStrip) {
  TestAppendTabsToTabStrip(false /* focus_tab_strip */);
}

TEST_F(TabLifecycleUnitSourceTest, SwitchTabInFocusedTabStrip) {
  LifecycleUnit* first_lifecycle_unit = nullptr;
  LifecycleUnit* second_lifecycle_unit = nullptr;
  CreateTwoTabs(true /* focus_tab_strip */, &first_lifecycle_unit,
                &second_lifecycle_unit);

  // Activate the first tab.
  task_runner_->FastForwardBy(kShortDelay);
  auto time_before_activate = NowTicks();
  tab_strip_model_->ActivateTabAt(0, true);
  EXPECT_TRUE(IsFocused(first_lifecycle_unit));
  EXPECT_EQ(time_before_activate, second_lifecycle_unit->GetLastFocusedTime());

  // Expect notifications when tabs are closed.
  CloseTabsAndExpectNotifications(
      tab_strip_model_.get(), {first_lifecycle_unit, second_lifecycle_unit});
}

TEST_F(TabLifecycleUnitSourceTest, CloseTabInFocusedTabStrip) {
  LifecycleUnit* first_lifecycle_unit = nullptr;
  LifecycleUnit* second_lifecycle_unit = nullptr;
  CreateTwoTabs(true /* focus_tab_strip */, &first_lifecycle_unit,
                &second_lifecycle_unit);

  // Close the second tab. The first tab should be focused.
  task_runner_->FastForwardBy(kShortDelay);
  testing::StrictMock<MockLifecycleUnitObserver> second_observer;
  second_lifecycle_unit->AddObserver(&second_observer);
  EXPECT_CALL(second_observer, OnLifecycleUnitDestroyed(second_lifecycle_unit));
  tab_strip_model_->CloseWebContentsAt(1, 0);
  testing::Mock::VerifyAndClear(&source_observer_);
  EXPECT_TRUE(IsFocused(first_lifecycle_unit));

  // Expect notifications when tabs are closed.
  CloseTabsAndExpectNotifications(tab_strip_model_.get(),
                                  {first_lifecycle_unit});
}

TEST_F(TabLifecycleUnitSourceTest, ReplaceWebContents) {
  LifecycleUnit* first_lifecycle_unit = nullptr;
  LifecycleUnit* second_lifecycle_unit = nullptr;
  CreateTwoTabs(true /* focus_tab_strip */, &first_lifecycle_unit,
                &second_lifecycle_unit);

  // Replace the WebContents in the active tab with a second WebContents. Expect
  // GetTabLifecycleUnitExternal() to return the TabLifecycleUnitExternal when
  // called with the second WebContents as argument.
  content::WebContents* original_web_contents =
      tab_strip_model_->GetWebContentsAt(1);
  TabLifecycleUnitExternal* tab_lifecycle_unit_external =
      source_->GetTabLifecycleUnitExternal(original_web_contents);
  std::unique_ptr<content::WebContents> new_web_contents =
      CreateTestWebContents();
  content::WebContents* raw_new_web_contents = new_web_contents.get();
  std::unique_ptr<content::WebContents> original_web_contents_deleter =
      tab_strip_model_->ReplaceWebContentsAt(1, std::move(new_web_contents));
  EXPECT_EQ(original_web_contents, original_web_contents_deleter.get());
  EXPECT_FALSE(source_->GetTabLifecycleUnitExternal(original_web_contents));
  EXPECT_EQ(tab_lifecycle_unit_external,
            source_->GetTabLifecycleUnitExternal(raw_new_web_contents));

  original_web_contents_deleter.reset();

  // Expect notifications when tabs are closed.
  CloseTabsAndExpectNotifications(
      tab_strip_model_.get(), {first_lifecycle_unit, second_lifecycle_unit});
}

TEST_F(TabLifecycleUnitSourceTest, DetachWebContents_Urgent) {
  DetachWebContentsTest(DiscardReason::kUrgent);
}

TEST_F(TabLifecycleUnitSourceTest, DetachWebContents_Proactive) {
  DetachWebContentsTest(DiscardReason::kProactive);
}

TEST_F(TabLifecycleUnitSourceTest, DetachWebContents_External) {
  DetachWebContentsTest(DiscardReason::kExternal);
}

// Regression test for https://crbug.com/818454. Previously, TabLifecycleUnits
// were destroyed from TabStripModelObserver::TabClosingAt(). If a tab was
// detached (TabStripModel::DetachWebContentsAt) and its WebContents destroyed,
// the TabLifecycleUnit was never destroyed. This was solved by giving ownership
// of a TabLifecycleUnit to a WebContentsUserData.
TEST_F(TabLifecycleUnitSourceTest, DetachAndDeleteWebContents) {
  LifecycleUnit* first_lifecycle_unit = nullptr;
  LifecycleUnit* second_lifecycle_unit = nullptr;
  CreateTwoTabs(true /* focus_tab_strip */, &first_lifecycle_unit,
                &second_lifecycle_unit);

  testing::StrictMock<MockLifecycleUnitObserver> observer;
  first_lifecycle_unit->AddObserver(&observer);

  // Detach and destroy the non-active tab. Verify that the LifecycleUnit is
  // destroyed.
  std::unique_ptr<content::WebContents> web_contents =
      tab_strip_model_->DetachWebContentsAt(0);
  EXPECT_CALL(observer, OnLifecycleUnitDestroyed(first_lifecycle_unit));
  web_contents.reset();
  testing::Mock::VerifyAndClear(&observer);
}

// Tab discarding is tested here rather than in TabLifecycleUnitTest because
// collaboration from the TabLifecycleUnitSource is required to replace the
// WebContents in the TabLifecycleUnit.

TEST_F(TabLifecycleUnitSourceTest, Discard_Proactive) {
  DiscardTest(DiscardReason::kProactive);
}

TEST_F(TabLifecycleUnitSourceTest, Discard_Urgent) {
  DiscardTest(DiscardReason::kUrgent);
}

TEST_F(TabLifecycleUnitSourceTest, Discard_External) {
  DiscardTest(DiscardReason::kExternal);
}

TEST_F(TabLifecycleUnitSourceTest, DiscardAndActivate_Urgent) {
  DiscardAndActivateTest(DiscardReason::kUrgent);
}

TEST_F(TabLifecycleUnitSourceTest, DiscardAndActivate_Proactive) {
  DiscardAndActivateTest(DiscardReason::kProactive);
}

TEST_F(TabLifecycleUnitSourceTest, DiscardAndActivate_External) {
  DiscardAndActivateTest(DiscardReason::kExternal);
}

TEST_F(TabLifecycleUnitSourceTest, DiscardAndExplicitlyReload_Urgent) {
  DiscardAndExplicitlyReloadTest(DiscardReason::kUrgent);
}

TEST_F(TabLifecycleUnitSourceTest, DiscardAndExplicitlyReload_Proactive) {
  DiscardAndExplicitlyReloadTest(DiscardReason::kProactive);
}

TEST_F(TabLifecycleUnitSourceTest, DiscardAndExplicitlyReload_External) {
  DiscardAndExplicitlyReloadTest(DiscardReason::kExternal);
}

TEST_F(TabLifecycleUnitSourceTest, CanOnlyDiscardOnce_Urgent) {
  CanOnlyDiscardOnceTest(DiscardReason::kUrgent);
}

TEST_F(TabLifecycleUnitSourceTest, CanOnlyDiscardOnce_Proactive) {
  CanOnlyDiscardOnceTest(DiscardReason::kProactive);
}

TEST_F(TabLifecycleUnitSourceTest, CanOnlyDiscardOnce_External) {
  CanOnlyDiscardOnceTest(DiscardReason::kExternal);
}

TEST_F(TabLifecycleUnitSourceTest, CannotFreezeADiscardedTab) {
  LifecycleUnit* background_lifecycle_unit = nullptr;
  LifecycleUnit* foreground_lifecycle_unit = nullptr;
  CreateTwoTabs(true /* focus_tab_strip */, &background_lifecycle_unit,
                &foreground_lifecycle_unit);
  content::WebContents* initial_web_contents =
      tab_strip_model_->GetWebContentsAt(0);
  task_runner_->FastForwardBy(kShortDelay);

  // It should be possible to discard the background tab.
  EXPECT_FOR_ALL_DISCARD_REASONS(background_lifecycle_unit, CanDiscard, true);

  // Discard the tab. Use DiscardReason::kUrgent to force the discard.
  EXPECT_EQ(LifecycleState::ACTIVE, background_lifecycle_unit->GetState());
  EXPECT_CALL(tab_observer_, OnDiscardedStateChange(testing::_, true));
  background_lifecycle_unit->Discard(DiscardReason::kUrgent);

  testing::Mock::VerifyAndClear(&tab_observer_);
  EXPECT_EQ(LifecycleState::DISCARDED, background_lifecycle_unit->GetState());
  EXPECT_NE(initial_web_contents, tab_strip_model_->GetWebContentsAt(0));
  EXPECT_FALSE(
      tab_strip_model_->GetWebContentsAt(0)->GetController().GetPendingEntry());

  EXPECT_FALSE(background_lifecycle_unit->Freeze());

  // Explicitly reload the tab. Expect the state to be LOADED.
  EXPECT_CALL(tab_observer_, OnDiscardedStateChange(testing::_, false));
  tab_strip_model_->GetWebContentsAt(0)->GetController().Reload(
      content::ReloadType::NORMAL, false);
  testing::Mock::VerifyAndClear(&tab_observer_);
  EXPECT_EQ(LifecycleState::ACTIVE, background_lifecycle_unit->GetState());
  EXPECT_TRUE(
      tab_strip_model_->GetWebContentsAt(0)->GetController().GetPendingEntry());

  // Should be able to freeze the reloaded tab.
  EXPECT_TRUE(background_lifecycle_unit->Freeze());
}

TEST_F(TabLifecycleUnitSourceTest, TabProactiveDiscardedByFrozenCallback) {
  LifecycleUnit* background_lifecycle_unit = nullptr;
  LifecycleUnit* foreground_lifecycle_unit = nullptr;
  CreateTwoTabs(true /* focus_tab_strip */, &background_lifecycle_unit,
                &foreground_lifecycle_unit);

  EXPECT_EQ(LifecycleState::ACTIVE, background_lifecycle_unit->GetState());

  EXPECT_CALL(tab_observer_, OnDiscardedStateChange(testing::_, true));

  background_lifecycle_unit->Discard(DiscardReason::kProactive);
  EXPECT_EQ(LifecycleState::PENDING_DISCARD,
            background_lifecycle_unit->GetState());

  reinterpret_cast<TabLifecycleUnitSource::TabLifecycleUnit*>(
      background_lifecycle_unit)
      ->UpdateLifecycleState(mojom::LifecycleState::kFrozen);
  EXPECT_EQ(LifecycleState::DISCARDED, background_lifecycle_unit->GetState());
  EXPECT_CALL(tab_observer_, OnDiscardedStateChange(testing::_, false));
  tab_strip_model_->GetWebContentsAt(0)->GetController().Reload(
      content::ReloadType::NORMAL, false);
  testing::Mock::VerifyAndClear(&tab_observer_);
}

TEST_F(TabLifecycleUnitSourceTest, TabProactiveDiscardedByFrozenTimeout) {
  LifecycleUnit* background_lifecycle_unit = nullptr;
  LifecycleUnit* foreground_lifecycle_unit = nullptr;
  CreateTwoTabs(true /* focus_tab_strip */, &background_lifecycle_unit,
                &foreground_lifecycle_unit);

  EXPECT_EQ(LifecycleState::ACTIVE, background_lifecycle_unit->GetState());
  EXPECT_CALL(tab_observer_, OnDiscardedStateChange(testing::_, true));

  background_lifecycle_unit->Discard(DiscardReason::kProactive);
  EXPECT_EQ(LifecycleState::PENDING_DISCARD,
            background_lifecycle_unit->GetState());
  task_runner_->FastForwardBy(kProactiveDiscardFreezeTimeout);

  EXPECT_EQ(LifecycleState::DISCARDED, background_lifecycle_unit->GetState());

  EXPECT_CALL(tab_observer_, OnDiscardedStateChange(testing::_, false));
  tab_strip_model_->GetWebContentsAt(0)->GetController().Reload(
      content::ReloadType::NORMAL, false);
  testing::Mock::VerifyAndClear(&tab_observer_);
}

}  // namespace resource_coordinator
