// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BROWSER_WATCHER_SYSTEM_SESSION_ANALYZER_WIN_H_
#define COMPONENTS_BROWSER_WATCHER_SYSTEM_SESSION_ANALYZER_WIN_H_

#include <windows.h>
#include <winevt.h>

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/time/time.h"

namespace metrics {

// Analyzes system session events for unclean sessions. Initialization is
// expensive and therefore done lazily, as the analyzer is instantiated before
// knowing whether it will be used.
class SystemSessionAnalyzer {
 public:
  enum Status {
    FAILED = 0,
    CLEAN = 1,
    UNCLEAN = 2,
    OUTSIDE_RANGE = 3,
  };

  // Minimal information about a log event.
  struct EventInfo {
    uint16_t event_id;
    base::Time event_time;
  };

  // Creates a SystemSessionAnalyzer that will analyze system sessions based on
  // events pertaining to as many as |max_session_cnt| of the most recent system
  // sessions.
  explicit SystemSessionAnalyzer(uint32_t max_session_cnt);
  virtual ~SystemSessionAnalyzer();

  // Returns an analysis status for the system session that contains
  // |timestamp|.
  virtual Status IsSessionUnclean(base::Time timestamp);

 protected:
  // Queries for the next |requested_events|. On success, returns true and
  // |event_infos| contains up to |requested_events| events ordered from newest
  // to oldest.
  // Returns false otherwise. Virtual for unit testing.
  virtual bool FetchEvents(size_t requested_events,
                           std::vector<EventInfo>* event_infos);

 private:
  struct EvtHandleCloser {
    using pointer = EVT_HANDLE;
    void operator()(EVT_HANDLE handle) const {
      if (handle)
        ::EvtClose(handle);
    }
  };
  using EvtHandle = std::unique_ptr<EVT_HANDLE, EvtHandleCloser>;

  FRIEND_TEST_ALL_PREFIXES(SystemSessionAnalyzerTest, FetchEvents);

  bool EnsureInitialized();
  bool EnsureHandlesOpened();
  bool Initialize();
  // Validates that |end| and |start| have sane event IDs and event times.
  // Updates |coverage_start_| and adds the session to unclean_sessions_
  // as appropriate.
  bool ProcessSession(const EventInfo& end, const EventInfo& start);

  EvtHandle CreateRenderContext();

  // The maximal number of sessions to query events for.
  uint32_t max_session_cnt_;
  uint32_t sessions_queried_;

  bool initialized_ = false;
  bool init_success_ = false;

  // A handle to the query, valid after a successful initialize.
  EvtHandle query_handle_;
  // A handle to the event render context, valid after a successful initialize.
  EvtHandle render_context_;

  // Information about unclean sessions: start time to session duration.
  std::map<base::Time, base::TimeDelta> unclean_sessions_;

  // Timestamp of the oldest event.
  base::Time coverage_start_;

  DISALLOW_COPY_AND_ASSIGN(SystemSessionAnalyzer);
};

}  // namespace metrics

#endif  // COMPONENTS_BROWSER_WATCHER_SYSTEM_SESSION_ANALYZER_WIN_H_
