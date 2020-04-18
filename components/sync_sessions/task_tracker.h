// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SESSIONS_TASK_TRACKER_H_
#define COMPONENTS_SYNC_SESSIONS_TASK_TRACKER_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <vector>

#include "base/time/clock.h"
#include "base/time/default_clock.h"
#include "base/time/time.h"
#include "components/sessions/core/session_id.h"
#include "components/sessions/core/session_types.h"
#include "components/sync_sessions/synced_tab_delegate.h"
#include "ui/base/page_transition_types.h"

namespace sync_sessions {

// Default/invalid global id. These are internal timestamp representations of
// a navigation (see base::Time::ToInternalValue()). Note that task ids are
// a subset of global ids.
static constexpr int64_t kInvalidGlobalID = -1;

// Default/invalid id for a navigation.
static constexpr int kInvalidNavID = 0;

// The maximum number of tasks we track in a tab.
static constexpr int kMaxNumTasksPerTab = 100;

// Class to generate and manage task ids for navigations of a tab. It is
// expected that there is only 1 TabTasks object for every tab, although Task
// ids can be duplicated across TabTasks (see copy constructor).
class TabTasks {
 public:
  TabTasks();
  explicit TabTasks(const TabTasks& rhs);
  virtual ~TabTasks();

  SessionID parent_tab_id() { return parent_tab_id_; }
  void set_parent_tab_id(SessionID parent_tab_id) {
    parent_tab_id_ = parent_tab_id;
  }

  // Gets root->leaf task id list for the navigation denoted by |nav_id|.
  // Returns an empty vector if |nav_id| is not found.
  std::vector<int64_t> GetTaskIdsForNavigation(int nav_id) const;

  void UpdateWithNavigation(int nav_id,
                            ui::PageTransition transition,
                            int64_t global_id);

 private:
  // Removes navigation from nav_to_task_id_map_ that is oldest according to
  // global_id.
  void RemoveOldestNavigation();

  FRIEND_TEST_ALL_PREFIXES(TaskTrackerTest, LimitMaxNumberOfTasksPerTab);
  FRIEND_TEST_ALL_PREFIXES(TaskTrackerTest,
                           CreateSubTaskFromExcludedAncestorTask);

  // The task id and and immediate parent for a navigation (see
  // |nav_to_task_id_map_|).
  struct TaskIdAndParent {
    // The task id for this navigation. Should always be present.
    int64_t task_id = kInvalidGlobalID;

    // The most recent global id for this navigation. This is used to determine
    // the age of this navigation when expiring old navigations. Should always
    // be present.
    int64_t global_id = kInvalidGlobalID;

    // If present, the nav id that this task is a continuation of. If this is
    // the first navigation in a new task, may not be present.
    int parent_nav_id = kInvalidNavID;
  };

  // Map converting navigation ids to a TaskIdAndParent. To find the root to
  // leaf chain for a navigation, start with its navigation id, and reindex into
  // the map using the parent id, until the parent id is kInvalidNavID.
  std::map<int, TaskIdAndParent> nav_to_task_id_map_;

  // Root navigation contains parent link to navigations that were copied from
  // another TabTasks instance. These navigations are not present in current
  // tab's navigation stack. Evicting this navigation would loose this link.
  // When the first naviation in this instance of TabTasks is created its id is
  // kept in root_nav_id_. This navigation is not considered for removal when
  // trimming nav_to_task_id_map_.
  int root_nav_id_ = kInvalidNavID;

  // The most recent navigation id seen for this tab.
  int most_recent_nav_id_ = kInvalidNavID;

  // The parent id for this tab (if there is one).
  SessionID parent_tab_id_ = SessionID::InvalidValue();

  DISALLOW_ASSIGN(TabTasks);
};

// Tracks tasks of current session. Tasks are a set of navigations that are
// related by explicit user actions (as determined by transition type. See
// |UpdateWithNavigation| above). Each task is composed of a tree of task ids
// that identify the navigations of that task (see |GetTaskIdsForNavigation|).
class TaskTracker {
 public:
  TaskTracker();
  virtual ~TaskTracker();

  // Returns a TabTasks pointer, which is owned by this object, for the tab of
  // given |tab_id|. |parent_tab_id|, if set, can be used to link the task ids
  // from one tab to another (e.g. when opening a navigation in a new tab, the
  // task ids from the original tab are necessary to continue tracking the
  // task chain).
  TabTasks* GetTabTasks(SessionID tab_id, SessionID parent_tab_id);

  // Cleans tracked task ids of navigations in the tab of |tab_id|.
  void CleanTabTasks(SessionID tab_id);

 private:
  FRIEND_TEST_ALL_PREFIXES(TaskTrackerTest, CleanTabTasks);

  std::map<SessionID, std::unique_ptr<TabTasks>> local_tab_tasks_map_;

  DISALLOW_COPY_AND_ASSIGN(TaskTracker);
};

}  // namespace sync_sessions

#endif  // COMPONENTS_SYNC_SESSIONS_TASK_TRACKER_H_
