// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "chrome/browser/resource_coordinator/discard_reason.h"
#include "chrome/browser/resource_coordinator/lifecycle_state.h"
#include "chrome/browser/resource_coordinator/lifecycle_unit.h"
#include "chrome/browser/resource_coordinator/lifecycle_unit_observer.h"
#include "chrome/browser/resource_coordinator/lifecycle_unit_source_observer.h"
#include "chrome/browser/resource_coordinator/tab_lifecycle_observer.h"
#include "chrome/browser/resource_coordinator/tab_load_tracker.h"
#include "chrome/browser/resource_coordinator/tab_manager_features.h"
#include "chrome/browser/sessions/session_restore_observer.h"
#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "content/public/browser/navigation_throttle.h"
#include "ui/gfx/native_widget_types.h"

class GURL;
class TabStripModel;

namespace content {
class NavigationHandle;
class WebContents;
}

namespace resource_coordinator {

class BackgroundTabNavigationThrottle;

#if defined(OS_CHROMEOS)
class TabManagerDelegate;
#endif
class TabManagerStatsCollector;

// The TabManager periodically updates (see
// |kAdjustmentIntervalSeconds| in the source) the status of renderers
// which are then used by the algorithm embedded here for priority in being
// killed upon OOM conditions.
//
// The algorithm used favors killing tabs that are not active, not in an active
// window, not in a visible window, not pinned, and have been idle for longest,
// in that order of priority.
//
// On Chrome OS (via the delegate), the kernel (via /proc/<pid>/oom_score_adj)
// will be informed of each renderer's score, which is based on the status, so
// in case Chrome is not able to relieve the pressure quickly enough and the
// kernel is forced to kill processes, it will be able to do so using the same
// algorithm as the one used here.
//
// The TabManager also delays background tabs' navigation when needed in order
// to improve users' experience with the foreground tab.
//
// Note that the browser tests are only active for platforms that use
// TabManager (CrOS only for now) and need to be adjusted accordingly if
// support for new platforms is added.
//
// Tabs are identified by a unique ID vended by this component. These IDs are
// not reused in a session. They are stable for a given conceptual tab, and will
// follow it through discards, reloads, tab strip operations, etc.
//
// TODO(fdoray): Rename to LifecycleManager. https://crbug.com/775644
class TabManager : public LifecycleUnitObserver,
                   public LifecycleUnitSourceObserver,
                   public TabLoadTracker::Observer,
                   public TabStripModelObserver {
 public:
  // Forward declaration of resource coordinator signal observer.
  class ResourceCoordinatorSignalObserver;

  // Needs to be public for DEFINE_WEB_CONTENTS_USER_DATA_KEY.
  class WebContentsData;

  TabManager();
  ~TabManager() override;

  // Start/Stop the Tab Manager.
  void Start();
  void Stop();

  // Returns the LifecycleUnits managed by this, sorted from less to most
  // important to the user. It is unsafe to access a pointer in the returned
  // vector after a LifecycleUnit has been destroyed.
  LifecycleUnitVector GetSortedLifecycleUnits();

  // Discards a tab to free the memory occupied by its renderer. The tab still
  // exists in the tab-strip; clicking on it will reload it. If the |reason| is
  // urgent, an aggressive fast-kill will be attempted if the sudden termination
  // disablers are allowed to be ignored (e.g. On ChromeOS, we can ignore an
  // unload handler and fast-kill the tab regardless).
  void DiscardTab(DiscardReason reason);

  // Method used by the extensions API to discard tabs. If |contents| is null,
  // discards the least important tab using DiscardTab(). Otherwise discards
  // the given contents. Returns the new web_contents or null if no tab
  // was discarded.
  content::WebContents* DiscardTabByExtension(content::WebContents* contents);

  // Log memory statistics for the running processes, then discards a tab.
  // Tab discard happens sometime later, as collecting the statistics touches
  // multiple threads and takes time.
  void LogMemoryAndDiscardTab(DiscardReason reason);

  // Log memory statistics for the running processes, then call the callback.
  void LogMemory(const std::string& title, const base::Closure& callback);

  // TODO(fdoray): Remove these methods. TabManager shouldn't know about tabs.
  // https://crbug.com/775644
  void AddObserver(TabLifecycleObserver* observer);
  void RemoveObserver(TabLifecycleObserver* observer);

  // Returns true when a given renderer can be purged if the specified
  // renderer is eligible for purging.
  bool CanPurgeBackgroundedRenderer(int render_process_id) const;

  // Indicates how TabManager should load pending background tabs. The mode is
  // recorded in tracing for easier debugging. The existing explicit numbering
  // should be kept as is when new modes are added.
  enum BackgroundTabLoadingMode {
    kStaggered = 0,  // Load a background tab after another tab is done loading.
    kPaused = 1      // Pause loading background tabs unless a user selects it.
  };

  // Maybe throttle a tab's navigation based on current system status.
  content::NavigationThrottle::ThrottleCheckResult MaybeThrottleNavigation(
      BackgroundTabNavigationThrottle* throttle);

  // Notifies TabManager that one navigation has finished (committed, aborted or
  // replaced). TabManager should clean up the NavigationHandle objects bookkept
  // before.
  void OnDidFinishNavigation(content::NavigationHandle* navigation_handle);

  // Notifies TabManager that one tab WebContents has been destroyed. TabManager
  // needs to clean up data related to that tab.
  void OnWebContentsDestroyed(content::WebContents* contents);

  // Return whether tabs are being loaded during session restore.
  bool IsSessionRestoreLoadingTabs() const {
    return is_session_restore_loading_tabs_;
  }

  // Returns true if the tab was created by session restore and has not finished
  // the first navigation.
  bool IsTabInSessionRestore(content::WebContents* web_contents) const;

  // Returns true if the tab was created by session restore and initially in
  // foreground.
  bool IsTabRestoredInForeground(content::WebContents* web_contents) const;

  // Returns the number of background tabs that are loading in a background tab
  // opening session.
  size_t GetBackgroundTabLoadingCount() const;

  // Returns the number of background tabs that are pending in a background tab
  // opening session.
  size_t GetBackgroundTabPendingCount() const;

  // Returns the number of tabs open in all browser instances.
  int GetTabCount() const;

  // Returns the number of restored tabs during session restore. This is
  // non-zero only during session restore.
  int restored_tab_count() const;

 private:
  friend class TabManagerStatsCollectorTest;
  friend class TabManagerWithProactiveDiscardExperimentEnabledTest;

  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, ActivateTabResetPurgeState);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, AutoDiscardable);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, BackgroundTabLoadingMode);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, BackgroundTabLoadingSlots);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, BackgroundTabsLoadingOrdering);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, CanOnlyDiscardOnce);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, ChildProcessNotifications);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, DefaultTimeToPurgeInCorrectRange);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, EnablePageAlmostIdleSignal);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, FreezeTab);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, InvalidOrEmptyURL);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, IsInBackgroundTabOpeningSession);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, IsInternalPage);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, IsTabRestoredInForeground);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, MaybeThrottleNavigation);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, OnDelayedTabSelected);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, OnDidFinishNavigation);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, OnTabIsLoaded);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, OnWebContentsDestroyed);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, OomPressureListener);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, PauseAndResumeBackgroundTabOpening);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest,
                           ProactiveFastShutdownSharedTabProcess);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest,
                           ProactiveFastShutdownSingleTabProcess);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest,
                           ProactiveFastShutdownWithBeforeunloadHandler);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest,
                           ProactiveFastShutdownWithUnloadHandler);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, ProtectPDFPages);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, ProtectRecentlyUsedTabs);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, ProtectVideoTabs);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, PurgeBackgroundRenderer);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest,
                           SessionRestoreAfterBackgroundTabOpeningSession);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest,
                           SessionRestoreBeforeBackgroundTabOpeningSession);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, ShouldPurgeAtDefaultTime);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, TabManagerBasics);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, TabManagerWasDiscarded);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest,
                           TabManagerWasDiscardedCrossSiteSubFrame);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, TimeoutWhenLoadingBackgroundTabs);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest,
                           TrackingNumberOfLoadedLifecycleUnits);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, UrgentFastShutdownSharedTabProcess);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, UrgentFastShutdownSingleTabProcess);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest,
                           UrgentFastShutdownWithBeforeunloadHandler);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, UrgentFastShutdownWithUnloadHandler);
  FRIEND_TEST_ALL_PREFIXES(TabManagerWithExperimentDisabledTest,
                           IsInBackgroundTabOpeningSession);
  FRIEND_TEST_ALL_PREFIXES(TabManagerWithProactiveDiscardExperimentEnabledTest,
                           GetTimeInBackgroundBeforeProactiveDiscardTest);

  // The time of the first purging after a renderer is backgrounded.
  // The initial value was chosen because most of users activate backgrounded
  // tabs within 30 minutes. (c.f. Tabs.StateTransfer.Time_Inactive_Active)
  static constexpr base::TimeDelta kDefaultMinTimeToPurge =
      base::TimeDelta::FromMinutes(1);

  // The min/max time to purge ratio. The max time to purge is set to be
  // min time to purge times this value.
  const int kDefaultMinMaxTimeToPurgeRatio = 4;

  static void PurgeMemoryAndDiscardTab(DiscardReason reason);

  // Returns true if the |url| represents an internal Chrome web UI page that
  // can be easily reloaded and hence makes a good choice to discard.
  static bool IsInternalPage(const GURL& url);

  // Purges data structures in the browser that can be easily recomputed.
  void PurgeBrowserMemory();

  // Callback for when |update_timer_| fires. Takes care of executing the tasks
  // that need to be run periodically (see comment in implementation).
  void UpdateTimerCallback();

  // Returns a random time-to-purge whose min value is min_time_to_purge and max
  // value is max_time_to_purge.
  base::TimeDelta GetTimeToPurge(base::TimeDelta min_time_to_purge,
                                 base::TimeDelta max_time_to_purge) const;

  // Returns true if the tab specified by |content| is now eligible to have
  // its memory purged.
  bool ShouldPurgeNow(content::WebContents* content) const;

  // Purges renderers in backgrounded tabs if the following conditions are
  // satisfied:
  // - the renderers are not purged yet,
  // - the renderers are not playing media,
  //   (CanPurgeBackgroundedRenderer returns true)
  // - the renderers are left inactive and background for time-to-purge.
  // If renderers are purged, their internal states become 'purged'.
  // The state is reset to be 'not purged' only when they are activated
  // (=ActiveTabChanged is invoked).
  void PurgeBackgroundedTabsIfNeeded();

  // Makes a request to the WebContents at the specified index to freeze its
  // page.
  void FreezeWebContentsAt(int index, TabStripModel* model);

  // Pause or resume background tab opening according to memory pressure change
  // if there are pending background tabs.
  void PauseBackgroundTabOpeningIfNeeded();
  void ResumeBackgroundTabOpeningIfNeeded();

  // Called by the memory pressure listener when the memory pressure rises.
  void OnMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level);

  // TabStripModelObserver overrides.
  void ActiveTabChanged(content::WebContents* old_contents,
                        content::WebContents* new_contents,
                        int index,
                        int reason) override;
  void TabInsertedAt(TabStripModel* tab_strip_model,
                     content::WebContents* contents,
                     int index,
                     bool foreground) override;
  void TabReplacedAt(TabStripModel* tab_strip_model,
                     content::WebContents* old_contents,
                     content::WebContents* new_contents,
                     int index) override;

  // TabLoadTracker::Observer implementation:
  void OnStartTracking(content::WebContents* web_contents,
                       LoadingState loading_state) override;
  void OnLoadingStateChange(content::WebContents* web_contents,
                            LoadingState loading_state) override;
  void OnStopTracking(content::WebContents* web_contents,
                      LoadingState loading_state) override;

  // Returns the WebContentsData associated with |contents|. Also takes care of
  // creating one if needed.
  static WebContentsData* GetWebContentsData(content::WebContents* contents);

  // Discards the less important LifecycleUnit that supports discarding under
  // |reason|.
  content::WebContents* DiscardTabImpl(DiscardReason reason);

  void OnSessionRestoreStartedLoadingTabs();
  void OnSessionRestoreFinishedLoadingTabs();
  void OnWillRestoreTab(content::WebContents* contents);

  // Returns true if it is in BackgroundTabOpening session, which is defined as
  // the duration from the time when the browser starts to load background tabs
  // until the time when browser has finished loading those tabs. During the
  // session, the session can end when background tabs' loading are paused due
  // to memory pressure. A new session starts when background tabs' loading
  // resume when memory pressure returns to normal.
  bool IsInBackgroundTabOpeningSession() const;

  // Returns true if TabManager can start loading next tab.
  bool CanLoadNextTab() const;

  // Start |force_load_timer_| to load the next background tab if the timer
  // expires before the current tab loading is finished.
  void StartForceLoadTimer();

  // Start loading the next background tab if needed. This is called when:
  // 1. a tab has finished loading;
  // 2. or a tab has been destroyed;
  // 3. or memory pressure is relieved;
  // 4. or |force_load_timer_| fires.
  void LoadNextBackgroundTabIfNeeded();

  // Resume the tab's navigation if it is pending right now. This is called when
  // a tab is selected.
  void ResumeTabNavigationIfNeeded(content::WebContents* contents);

  // Resume navigation.
  void ResumeNavigation(BackgroundTabNavigationThrottle* throttle);

  // Remove the pending navigation for the provided WebContents. Return the
  // removed NavigationThrottle. Return nullptr if it doesn't exists.
  BackgroundTabNavigationThrottle* RemovePendingNavigationIfNeeded(
      content::WebContents* contents);

  // Returns true if |first| is considered to resume navigation before |second|.
  static bool ComparePendingNavigations(
      const BackgroundTabNavigationThrottle* first,
      const BackgroundTabNavigationThrottle* second);

  // Returns the number of tabs that are not pending load or discarded.
  int GetNumAliveTabs() const;

  // Check if the tab is loading. Use only in tests.
  bool IsTabLoadingForTest(content::WebContents* contents) const;

  // Check if the navigation is delayed. Use only in tests.
  bool IsNavigationDelayedForTest(
      const content::NavigationHandle* navigation_handle) const;

  // Set |loading_slots_|. Use only in tests.
  void SetLoadingSlotsForTest(size_t loading_slots) {
    loading_slots_ = loading_slots;
  }

  // Reset |memory_pressure_listener_| in test so that the test is not affected
  // by memory pressure.
  void ResetMemoryPressureListenerForTest() {
    memory_pressure_listener_.reset();
  }

  TabManagerStatsCollector* stats_collector() { return stats_collector_.get(); }

  // Returns true if the background tab force load timer is running.
  bool IsForceLoadTimerRunning() const;

  // Returns the threshold after which a background LifecycleUnit gets
  // discarded, given the current number of alive LifecycleUnits and experiment
  // parameters.
  base::TimeDelta GetTimeInBackgroundBeforeProactiveDiscard() const;

  // Returns the next discardable LifecycleUnit, if any, and nullptr otherwise.
  // The next discardable LifecycleUnit is the LifecycleUnit that has been in
  // the visibility state content::Visibility::HIDDEN for the longest time.
  LifecycleUnit* GetNextDiscardableLifecycleUnit() const;

  // If necessary, schedules a task to proactively discard a LifecycleUnit at
  // the right time.
  void UpdateProactiveDiscardTimerIfNecessary();

  // LifecycleUnitObserver:
  void OnLifecycleUnitVisibilityChanged(
      LifecycleUnit* lifecycle_unit,
      content::Visibility visibility) override;
  void OnLifecycleUnitDestroyed(LifecycleUnit* lifecycle_unit) override;
  void OnLifecycleUnitStateChanged(LifecycleUnit* lifecycle_unit,
                                   LifecycleState last_state) override;

  // LifecycleUnitSourceObserver:
  void OnLifecycleUnitCreated(LifecycleUnit* lifecycle_unit) override;

  // LifecycleUnits managed by this.
  LifecycleUnitSet lifecycle_units_;

  // Number of LifecycleUnits in |lifecycle_units_| that are in State::LOADED.
  // Used to determine timeout threshold for proactive discarding.
  int num_loaded_lifecycle_units_ = 0;

  // Parameters for proactive discarding. Used to determine the timeout
  // until a LifecycleUnit should be discarded based on
  // |num_loaded_lifecycle_units_|.
  ProactiveTabDiscardParams proactive_discard_params_;

  // Timer to periodically update the stats of the renderers.
  base::RepeatingTimer update_timer_;

  // Timer for proactive discarding.
  std::unique_ptr<base::OneShotTimer> proactive_discard_timer_;

  // A listener to global memory pressure events.
  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;

  // A backgrounded renderer will be purged between min_time_to_purge_ and
  // max_time_to_purge_.
  base::TimeDelta min_time_to_purge_;
  base::TimeDelta max_time_to_purge_;

#if defined(OS_CHROMEOS)
  std::unique_ptr<TabManagerDelegate> delegate_;
#endif

  // Responsible for automatically registering this class as an observer of all
  // TabStripModels. Automatically tracks browsers as they come and go.
  BrowserTabStripTracker browser_tab_strip_tracker_;

  bool is_session_restore_loading_tabs_;
  size_t restored_tab_count_;

  class TabManagerSessionRestoreObserver;
  std::unique_ptr<TabManagerSessionRestoreObserver> session_restore_observer_;

  // The mode that TabManager is using to load pending background tabs.
  BackgroundTabLoadingMode background_tab_loading_mode_;

  // When the timer fires, it forces loading the next background tab if needed.
  std::unique_ptr<base::OneShotTimer> force_load_timer_;

  // The list of navigations that are delayed.
  std::vector<BackgroundTabNavigationThrottle*> pending_navigations_;

  // The tabs that are currently loading. We will consider loading the next
  // background tab when these tabs have finished loading or a background tab
  // is brought to foreground.
  std::set<content::WebContents*> loading_contents_;

  // The number of loading slots that TabManager can use to load background tabs
  // in parallel.
  size_t loading_slots_;

  // |resource_coordinator_signal_observer_| is owned by TabManager and is used
  // to receive various signals from ResourceCoordinator.
  std::unique_ptr<ResourceCoordinatorSignalObserver>
      resource_coordinator_signal_observer_;

  // Records UMAs for tab and system-related events and properties during
  // session restore.
  std::unique_ptr<TabManagerStatsCollector> stats_collector_;

  // Weak pointer factory used for posting delayed tasks.
  base::WeakPtrFactory<TabManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TabManager);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_H_
