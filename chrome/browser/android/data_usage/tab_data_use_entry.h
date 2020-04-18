// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_DATA_USAGE_TAB_DATA_USE_ENTRY_H_
#define CHROME_BROWSER_ANDROID_DATA_USAGE_TAB_DATA_USE_ENTRY_H_

#include <string>
#include <vector>

#include "base/containers/circular_deque.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/time/time.h"

namespace android {

class DataUseTabModel;

// TabDataUseTrackingSession maintains the information about a single tracking
// session within a browser tab.
struct TabDataUseTrackingSession {
  TabDataUseTrackingSession(const std::string& label,
                            const base::TimeTicks& start_time)
      : label(label), start_time(start_time), page_loads(0) {}

  // Tracking label to be associated with the data usage of this session.
  std::string label;

  // Time the data use tracking session started.
  base::TimeTicks start_time;

  // Time the data use tracking session ended. |end_time| will be null if the
  // tracking session is currently active.
  base::TimeTicks end_time;

  // Number of page loads within the tracking session.
  uint32_t page_loads;
};

// TabDataUseEntry contains the history of the disjoint tracking sessions for a
// single browser tab.
class TabDataUseEntry {
 public:
  explicit TabDataUseEntry(DataUseTabModel* tab_model);

  TabDataUseEntry(const TabDataUseEntry& other);

  virtual ~TabDataUseEntry();

  TabDataUseEntry& operator=(const TabDataUseEntry& other) = default;

  // Initiates a new tracking session with the given |label|. Returns false if a
  // tracking session is already active, and true otherwise.
  bool StartTracking(const std::string& label);

  // Ends the active tracking session. Returns false if there is no active
  // tracking session, and true otherwise.
  bool EndTracking();

  // Ends the active tracking session if it is labeled with |label| and returns
  // true.
  bool EndTrackingWithLabel(const std::string& label);

  // Records that the tab has been closed, in preparation for deletion.
  void OnTabCloseEvent();

  // Gets the label of the session in history that was active at
  // |data_use_time|. |output_label| must not be null. If a session is found,
  // returns true and |output_label| is populated. Otherwise returns false and
  // |output_label| is set to empty string.
  bool GetLabel(const base::TimeTicks& data_use_time,
                std::string* output_label) const;

  // Returns true if the tracking session is currently active.
  bool IsTrackingDataUse() const;

  // Returns true if the tab has expired. A closed tab entry expires
  // |kClosedTabExpirationDurationSeconds| seconds after it was closed. An open
  // tab entry expires |kOpenTabExpirationDurationSeconds| seconds after the
  // most recent tracking session start or end event.
  bool IsExpired() const;

  // Returns the latest time a tracking session was started or ended. Returned
  // time will be null if no tracking session was ever started or ended.
  base::TimeTicks GetLatestStartOrEndTime() const;

  // Returns the tracking label for the active tracking session. Empty string is
  // returned if tracking session is not active.
  std::string GetActiveTrackingSessionLabel() const;

  bool is_custom_tab_package_match() const {
    return is_custom_tab_package_match_;
  }

  void set_custom_tab_package_match(bool is_custom_tab_package_match);

  // Notifies a page load event for the active tracking session.
  void NotifyPageLoad();

 private:
  friend class TabDataUseEntryTest;
  FRIEND_TEST_ALL_PREFIXES(TabDataUseEntryTest, MultipleTabSessionCloseEvent);
  FRIEND_TEST_ALL_PREFIXES(TabDataUseEntryTest, SingleTabSessionCloseEvent);
  FRIEND_TEST_ALL_PREFIXES(DataUseTabModelTest,
                           ExpiredInactiveTabEntryRemovaltimeHistogram);
  FRIEND_TEST_ALL_PREFIXES(DataUseTabModelTest, TabCloseEvent);

  using TabSessions = base::circular_deque<TabDataUseTrackingSession>;

  // Compacts the history of tracking sessions by removing oldest sessions to
  // keep the size of |sessions_| within |kMaxSessionsPerTab| entries.
  void CompactSessionHistory();

  // Contains the history of sessions in chronological order. Oldest sessions
  // will be at the front of the queue, and new sessions will get added to the
  // end of the queue.
  TabSessions sessions_;

  // Indicates the time the tab was closed. |tab_close_time_| will be null if
  // the tab is still open.
  base::TimeTicks tab_close_time_;

  // True if tracking was started in a custom tab due to package name match.
  bool is_custom_tab_package_match_;

  // Pointer to the DataUseTabModel that owns |this|.
  const DataUseTabModel* tab_model_;
};

}  // namespace android

#endif  // CHROME_BROWSER_ANDROID_DATA_USAGE_TAB_DATA_USE_ENTRY_H_
