// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SESSIONS_TAB_LOADER_H_
#define CHROME_BROWSER_SESSIONS_TAB_LOADER_H_

#include <stddef.h>

#include <list>
#include <memory>
#include <set>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/memory_coordinator_client.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/timer/timer.h"
#include "chrome/browser/resource_coordinator/page_signal_receiver.h"
#include "chrome/browser/sessions/session_restore_delegate.h"
#include "chrome/browser/sessions/tab_loader_delegate.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace content {
class NavigationController;
class RenderWidgetHost;
}

class SessionRestoreStatsCollector;
class TabLoaderTest;

// TabLoader is responsible for loading tabs after session restore has finished
// creating all the tabs. Tabs are loaded after a previously tab finishes
// loading or a timeout is reached. If the timeout is reached before a tab
// finishes loading the timeout delay is doubled.
//
// TabLoader keeps a reference to itself when it's loading. When it has finished
// loading, it drops the reference. If another profile is restored while the
// TabLoader is loading, it will schedule its tabs to get loaded by the same
// TabLoader. When doing the scheduling, it holds a reference to the TabLoader.
//
// This is not part of SessionRestoreImpl so that synchronous destruction
// of SessionRestoreImpl doesn't have timing problems.
class TabLoader : public content::NotificationObserver,
                  public base::RefCounted<TabLoader>,
                  public TabLoaderCallback,
                  public base::MemoryCoordinatorClient,
                  public resource_coordinator::PageSignalObserver {
 public:
  using RestoredTab = SessionRestoreDelegate::RestoredTab;

  // NotificationObserver method. Removes the specified tab and loads the next
  // tab.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // resource_coordinator::PageSignalObserver implementation.
  void OnPageAlmostIdle(content::WebContents* web_contents) override;

  // TabLoaderCallback:
  void SetTabLoadingEnabled(bool enable_tab_loading) override;

  // Called to start restoring tabs.
  static void RestoreTabs(const std::vector<RestoredTab>& tabs,
                          const base::TimeTicks& restore_started);

 private:
  FRIEND_TEST_ALL_PREFIXES(TabLoaderTest, OnMemoryStateChange);
  FRIEND_TEST_ALL_PREFIXES(TabLoaderTest, UsePageAlmostIdleSignal);
  FRIEND_TEST_ALL_PREFIXES(TabRestoreTest,
                           TabsFromRestoredWindowsAreLoadedGradually);

  friend class base::RefCounted<TabLoader>;
  friend class TabLoaderTest;

  using TabsLoading = std::set<content::NavigationController*>;
  using TabsToLoad = std::list<content::NavigationController*>;

  explicit TabLoader(base::TimeTicks restore_started);
  ~TabLoader() override;

  // This is invoked once by RestoreTabs to start loading.
  void StartLoading(const std::vector<RestoredTab>& tabs);

  // Loads the next tab. If there are no more tabs to load this deletes itself,
  // otherwise |force_load_timer_| is restarted.
  void LoadNextTab();

  // Starts |force_load_timer_| to load the first non-visible tab if the timer
  // expires before a visible tab has finished loading. This uses the same
  // timer but a different timeout value than StartTimer.
  void StartFirstTimer();

  // Starts |force_load_timer_| to load the next tab if the timer expires
  // before the current tab loading is finished. This uses the same timer but a
  // different timeout value than StartFirstTimer.
  void StartTimer();

  // Removes the listeners from the specified tab and removes the tab from
  // the set of tabs to load and list of tabs we're waiting to get a load
  // from.
  void RemoveTab(content::NavigationController* controller);

  // Invoked from |force_load_timer_|. Doubles |force_load_delay_multiplier_|
  // and invokes |LoadNextTab| to load the next tab
  void ForceLoadTimerFired();

  // Returns the RenderWidgetHost associated with a tab if there is one,
  // NULL otherwise.
  static content::RenderWidgetHost* GetRenderWidgetHost(
      content::NavigationController* controller);

  // Register for necessary notifications on a tab navigation controller.
  void RegisterForNotifications(content::NavigationController* controller);

  // Called when a tab goes away or a load completes.
  void HandleTabClosedOrLoaded(content::NavigationController* controller);

  // Returns true when this is under memory pressure and required to purge
  // memory by stopping loading tabs.
  bool ShouldStopLoadingTabs() const;

  // React to memory pressure by stopping to load any more tabs.
  void OnMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level);

  // base::MemoryCoordinatorClient implementation:
  void OnMemoryStateChange(base::MemoryState state) override;

  // Stops loading tabs to purge memory by stopping to load any more tabs.
  void StopLoadingTabs();

  // Limit the number of loaded tabs.
  // Value of 0 restores default behavior. In test mode command line flags and
  // free memory size are not taken into account.
  static void SetMaxLoadedTabCountForTest(size_t value);

  std::unique_ptr<TabLoaderDelegate> delegate_;

  // Listens for system under memory pressure notifications and stops loading
  // of tabs when we start running out of memory.
  base::MemoryPressureListener memory_pressure_listener_;

  content::NotificationRegistrar registrar_;

  // The delay timer multiplier. See class description for details.
  size_t force_load_delay_multiplier_;

  // True if the tab loading is enabled.
  bool loading_enabled_;

  // The set of tabs we've initiated loading on. This does NOT include the
  // selected tabs.
  TabsLoading tabs_loading_;

  // The tabs we need to load.
  TabsToLoad tabs_to_load_;

  // The number of tabs started to load.
  // (This value never decreases.)
  size_t started_to_load_count_;

  base::OneShotTimer force_load_timer_;

  // The time the restore process started.
  base::TimeTicks restore_started_;

  // For keeping TabLoader alive while it's loading even if no
  // SessionRestoreImpls reference it.
  scoped_refptr<TabLoader> this_retainer_;

  // The SessionRestoreStatsCollector associated with this TabLoader. This is
  // explicitly referenced so that it can be notified of deferred tab loads due
  // to memory pressure.
  scoped_refptr<SessionRestoreStatsCollector> stats_collector_;

  static TabLoader* shared_tab_loader_;

  DISALLOW_COPY_AND_ASSIGN(TabLoader);
};

#endif  // CHROME_BROWSER_SESSIONS_TAB_LOADER_H_
