// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_DATA_USAGE_DATA_USE_TAB_MODEL_H_
#define CHROME_BROWSER_ANDROID_DATA_USAGE_DATA_USE_TAB_MODEL_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "chrome/browser/android/data_usage/tab_data_use_entry.h"
#include "components/sessions/core/session_id.h"
#include "url/gurl.h"

namespace base {
class TickClock;
}

namespace content {
class NavigationEntry;
}

namespace android {

class DataUseMatcher;

// Models tracking and labeling of data usage within each Tab. Within each tab,
// the model tracks the data use of a sequence of navigations in a "tracking
// session" beginning with an entry event and ending with an exit event.
// Typically, these events are navigations matching a URL pattern, or various
// types of browser-initiated navigations. A single tab may have several
// disjoint "tracking sessions" depending on the sequence of entry and exit
// events that took place.
class DataUseTabModel {
 public:
  // TrackingInfo maintains the tracking information for a single tab.
  struct TrackingInfo {
    std::string label;
    std::string tag;
  };

  // TransitionType enumerates the types of possible browser navigation events
  // and transitions.
  enum TransitionType {
    // Navigation from the omnibox to the SRP.
    TRANSITION_OMNIBOX_SEARCH,

    // Navigation from external apps that use Custom Tabs.
    TRANSITION_CUSTOM_TAB,

    // Navigation by clicking a link in the page.
    TRANSITION_LINK,

    // Navigation by reloading the page or restoring tabs.
    TRANSITION_RELOAD,

    // Navigation from the omnibox when typing a URL.
    TRANSITION_OMNIBOX_NAVIGATION,

    // Navigation from a bookmark.
    TRANSITION_BOOKMARK,

    // Navigating from history.
    TRANSITION_HISTORY_ITEM,

    // Navigation back or forward.
    TRANSITION_FORWARD_BACK,

    // Navigation due to form submission.
    TRANSITION_FORM_SUBMIT,
  };

  // TabDataUseObserver provides the interface for getting notifications from
  // the DataUseTabModel. The observer must be added on the UI thread, and the
  // callbacks will be received on the UI thread.
  class TabDataUseObserver {
   public:
    virtual ~TabDataUseObserver() {}

    // Notification callbacks when tab tracking sessions are started and ended.
    virtual void NotifyTrackingStarting(SessionID tab_id) = 0;
    virtual void NotifyTrackingEnding(SessionID tab_id) = 0;

    // Notification callback that DataUseTabModel is ready to process the UI
    // navigation events.
    virtual void OnDataUseTabModelReady() = 0;
  };

  // The tags to report for data usage from a default chrome tab, and a chrome
  // custom tab.
  static const char kDefaultTag[];
  static const char kCustomTabTag[];

  // |force_fetch_matching_rules_callback| is the callback to be run to initiate
  // fetching matching rules and |on_matching_rules_fetched_callback|
  // is the callback to be run after matching rules are fetched.
  DataUseTabModel(
      const base::Closure& force_fetch_matching_rules_callback,
      const base::Callback<void(bool)>& on_matching_rules_fetched_callback);

  virtual ~DataUseTabModel();

  base::WeakPtr<DataUseTabModel> GetWeakPtr();

  // Notifies the DataUseTabModel of navigation events. |tab_id| is the source
  // tab of the generated event, |transition| indicates the type of the UI
  // event/transition,  |url| is the URL in the source tab, |package| indicates
  // the android package name of external application that initiated the event.
  // |navigation_entry| corresponds to the navigation entry of the current
  // navigation in back-forward navigation history, and is used to save the
  // current tracking label to be used for back-forward navigations.
  // |navigation_entry| can be null in some cases where it cannot be retrieved
  // such as buffered navigation events or when support for back-forward
  // navigations is not needed such as custom tab navigation.
  void OnNavigationEvent(SessionID tab_id,
                         TransitionType transition,
                         const GURL& url,
                         const std::string& package,
                         content::NavigationEntry* navigation_entry);

  // Notifies the DataUseTabModel that tab with |tab_id| is closed. Any active
  // tracking sessions for the tab are terminated, and the tab is marked as
  // closed.
  void OnTabCloseEvent(SessionID tab_id);

  // Notifies the DataUseTabModel that tracking label |label| is removed. Any
  // active tracking sessions with the label are ended, without notifying any of
  // the TabDataUseObserver.
  virtual void OnTrackingLabelRemoved(const std::string& label);

  // Gets the tracking information for the tab with id |tab_id| at time
  // |timestamp|. |output_info| must not be null. If a tab tracking session is
  // found that was active at |timestamp|, returns true and
  // |output_tracking_info| is populated with its information. Otherwise,
  // returns false.
  virtual bool GetTrackingInfoForTabAtTime(
      SessionID tab_id,
      base::TimeTicks timestamp,
      TrackingInfo* output_tracking_info) const;

  // Returns true if the navigation event would end the tracking session for
  // |tab_id|. |transition| is the type of the UI event/transition. |url| is the
  // URL in the tab. |navigation_entry| which can be null corresponds to the
  // navigation entry of the current navigation in back-forward navigation
  // history.
  bool WouldNavigationEventEndTracking(
      SessionID tab_id,
      TransitionType transition,
      const GURL& url,
      const content::NavigationEntry* navigation_entry) const;

  // Adds observers to the observer list. Must be called on UI thread.
  // |observer| is notified on the UI thread.
  void AddObserver(TabDataUseObserver* observer);
  void RemoveObserver(TabDataUseObserver* observer);

  // Called by ExternalDataUseObserver to register multiple case-insensitive
  // regular expressions.
  void RegisterURLRegexes(const std::vector<std::string>& app_package_name,
                          const std::vector<std::string>& domain_path_regex,
                          const std::vector<std::string>& label);

  // Notifies the DataUseTabModel that the external control app is installed or
  // uninstalled. |is_control_app_installed| is true if app is installed.
  void OnControlAppInstallStateChange(bool is_control_app_installed);

  // Returns the maximum number of tracking sessions to maintain per tab.
  size_t max_sessions_per_tab() const { return max_sessions_per_tab_; }

  // Returns the expiration duration for a closed tab entry and an open tab
  // entry respectively.
  const base::TimeDelta& closed_tab_expiration_duration() const {
    return closed_tab_expiration_duration_;
  }
  const base::TimeDelta& open_tab_expiration_duration() const {
    return open_tab_expiration_duration_;
  }

  // Returns the current time.
  base::TimeTicks NowTicks() const;

  // Returns true if the |tab_id| is a custom tab and started tracking due to
  // package name match.
  bool IsCustomTabPackageMatch(SessionID tab_id) const;

  // Returns true if DataUseTabModel is ready to process UI navigation events.
  bool is_ready_for_navigation_event() const {
    return is_ready_for_navigation_event_;
  }

 protected:
  // Notifies the observers that a data usage tracking session started for
  // |tab_id|. Protected for testing.
  void NotifyObserversOfTrackingStarting(SessionID tab_id);

  // Notifies the observers that an active data usage tracking session ended for
  // |tab_id|. Protected for testing.
  void NotifyObserversOfTrackingEnding(SessionID tab_id);

  // Notifies the observers that DataUseTabModel is ready to process navigation
  // events.
  void NotifyObserversOfDataUseTabModelReady();

 private:
  friend class DataUseTabModelTest;
  friend class ExternalDataUseObserverTest;
  friend class ExternalDataUseReporterTest;
  friend class TabDataUseEntryTest;
  friend class TestDataUseTabModel;
  FRIEND_TEST_ALL_PREFIXES(DataUseTabModelTest,
                           CompactTabEntriesWithinMaxLimit);
  FRIEND_TEST_ALL_PREFIXES(DataUseTabModelTest,
                           ExpiredInactiveTabEntryRemovaltimeHistogram);
  FRIEND_TEST_ALL_PREFIXES(DataUseTabModelTest,
                           MatchingRuleClearedOnControlAppUninstall);
  FRIEND_TEST_ALL_PREFIXES(DataUseTabModelTest,
                           MultipleObserverMultipleStartEndEvents);
  FRIEND_TEST_ALL_PREFIXES(DataUseTabModelTest, ObserverStartEndEvents);
  FRIEND_TEST_ALL_PREFIXES(DataUseTabModelTest,
                           ProcessBufferedNavigationEventsAfterRuleFetch);
  FRIEND_TEST_ALL_PREFIXES(DataUseTabModelTest, TabCloseEvent);
  FRIEND_TEST_ALL_PREFIXES(DataUseTabModelTest, TabCloseEventEndsTracking);
  FRIEND_TEST_ALL_PREFIXES(DataUseTabModelTest,
                           UnexpiredTabEntryRemovaltimeHistogram);
  FRIEND_TEST_ALL_PREFIXES(ExternalDataUseObserverTest,
                           MatchingRuleFetchOnControlAppInstall);

  using TabEntryMap = std::map<SessionID, TabDataUseEntry>;

  // Gets the current label of a tab, and the new label if a navigation event
  // occurs in the tab. |tab_id| is the source tab of the generated event,
  // |transition| indicates the type of the UI event/transition,  |url| is the
  // URL in the source tab, |package| indicates the android package name of
  // external application that initiated the event. |navigation_entry| which can
  // be null is the navigation entry of the current navigation in back-forward
  // history. |current_label|, |new_label| and |is_package_match| should not be
  // null, and are set with current and new labels respectively. |current_label|
  // will be set to empty string, if there is no active tracking session.
  // |new_label| will be set to empty string if there would be no active
  // tracking session if the navigation happens. |is_package_match| will be set
  // to true if a tracking session will start due to package name match.
  void GetCurrentAndNewLabelForNavigationEvent(
      SessionID tab_id,
      TransitionType transition,
      const GURL& url,
      const std::string& package,
      const content::NavigationEntry* navigation_entry,
      std::string* current_label,
      std::string* new_label,
      bool* is_package_match) const;

  // Initiates a new tracking session with the |label| for tab with id |tab_id|.
  // |is_custom_tab_package_match| is true if |tab_id| is a custom tab and
  // started tracking due to package name match.
  void StartTrackingDataUse(TransitionType transition,
                            SessionID tab_id,
                            const std::string& label,
                            bool is_custom_tab_package_match);

  // Ends the current tracking session for tab with id |tab_id|.
  void EndTrackingDataUse(TransitionType transition, SessionID tab_id);

  // Compacts the tab entry map |active_tabs_| by removing expired tab entries.
  // After removing expired tab entries, if the size of |active_tabs_| exceeds
  // |kMaxTabEntries|, oldest unexpired tab entries will be removed until its
  // size is |kMaxTabEntries|.
  void CompactTabEntries();

  // Collection of observers that receive tracking session start and end
  // notifications. Notifications are posted on UI thread.
  base::ObserverList<TabDataUseObserver> observers_;

  // Maintains the tracking sessions of multiple tabs.
  TabEntryMap active_tabs_;

  // Maximum number of tab entries to maintain session information about.
  const size_t max_tab_entries_;

  // Maximum number of tracking sessions to maintain per tab.
  const size_t max_sessions_per_tab_;

  // Expiration duration for a closed tab entry and an open tab entry
  // respectively.
  const base::TimeDelta closed_tab_expiration_duration_;
  const base::TimeDelta open_tab_expiration_duration_;

  // TickClock used for obtaining the current time.
  const base::TickClock* tick_clock_;

  // Stores the matching patterns.
  std::unique_ptr<DataUseMatcher> data_use_matcher_;

  // Callback to be run to initiate fetching the matching rules.
  const base::Closure force_fetch_matching_rules_callback_;

  // True if DataUseTabModel is ready to process UI navigation events.
  // DataUseTabModel will be considered ready when the first rule fetch is
  // complete or the control app not installed callback was received, whichever
  // is sooner.
  bool is_ready_for_navigation_event_;

  // True if the external control app is installed.
  bool is_control_app_installed_;

  base::ThreadChecker thread_checker_;

  base::WeakPtrFactory<DataUseTabModel> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DataUseTabModel);
};

}  // namespace android

#endif  // CHROME_BROWSER_ANDROID_DATA_USAGE_DATA_USE_TAB_MODEL_H_
