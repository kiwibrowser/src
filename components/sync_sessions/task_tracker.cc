// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_sessions/task_tracker.h"

#include <utility>

#include "base/numerics/safe_conversions.h"

namespace sync_sessions {

namespace {

// The criteria for whether a navigation will continue a task chain or start a
// new one.
bool DoesTransitionContinueTask(ui::PageTransition transition) {
  if (ui::PageTransitionCoreTypeIs(transition, ui::PAGE_TRANSITION_LINK) ||
      ui::PageTransitionCoreTypeIs(transition,
                                   ui::PAGE_TRANSITION_AUTO_SUBFRAME) ||
      ui::PageTransitionCoreTypeIs(transition,
                                   ui::PAGE_TRANSITION_MANUAL_SUBFRAME) ||
      ui::PageTransitionCoreTypeIs(transition,
                                   ui::PAGE_TRANSITION_FORM_SUBMIT) ||
      transition & ui::PAGE_TRANSITION_IS_REDIRECT_MASK) {
    return true;
  }
  return false;
}

}  // namespace

TabTasks::TabTasks() {}

TabTasks::TabTasks(const TabTasks& rhs)
    : nav_to_task_id_map_(rhs.nav_to_task_id_map_),
      most_recent_nav_id_(rhs.most_recent_nav_id_) {}

TabTasks::~TabTasks() {}

std::vector<int64_t> TabTasks::GetTaskIdsForNavigation(int nav_id) const {
  std::vector<int64_t> task_id_chain;
  int next_id = nav_id;
  while (next_id != kInvalidNavID) {
    auto next_nav_iter = nav_to_task_id_map_.find(next_id);
    if (next_nav_iter == nav_to_task_id_map_.end())
      break;
    const TaskIdAndParent& next_nav = next_nav_iter->second;
    DCHECK_NE(kInvalidGlobalID, next_nav.task_id);
    task_id_chain.push_back(next_nav.task_id);
    next_id = next_nav.parent_nav_id;
    DCHECK_LE(task_id_chain.size(), static_cast<size_t>(kMaxNumTasksPerTab));
  }

  // Reverse the order so the root is the first item (oldest -> newest).
  std::reverse(task_id_chain.begin(), task_id_chain.end());

  DVLOG(1) << "Returning " << task_id_chain.size() << " task ids for nav "
           << nav_id;
  return task_id_chain;
}

void TabTasks::UpdateWithNavigation(int nav_id,
                                    ui::PageTransition transition,
                                    int64_t global_id) {
  DCHECK_NE(kInvalidNavID, nav_id);
  DCHECK_NE(kInvalidGlobalID, global_id);

  if (nav_to_task_id_map_.count(nav_id) == 0) {
    if (root_nav_id_ == kInvalidNavID)
      root_nav_id_ = nav_id;
    DVLOG(1) << "Setting current task id to " << global_id;
    nav_to_task_id_map_[nav_id].task_id = global_id;

    if (DoesTransitionContinueTask(transition))
      nav_to_task_id_map_[nav_id].parent_nav_id = most_recent_nav_id_;
  }
  DVLOG(1) << "Setting most recent nav id to " << nav_id;
  most_recent_nav_id_ = nav_id;
  nav_to_task_id_map_[nav_id].global_id = global_id;

  // Go through and drop the oldest navigations until kMaxNumTasksPerTab
  // navigations remain.
  // TODO(zea): we go through max of 100 iterations here on each new navigation.
  // May be worth attempting to optimize this further if it becomes an issue.
  if (nav_to_task_id_map_.size() > kMaxNumTasksPerTab) {
    RemoveOldestNavigation();
    DCHECK_EQ(static_cast<uint64_t>(kMaxNumTasksPerTab),
              nav_to_task_id_map_.size());
  }
}

void TabTasks::RemoveOldestNavigation() {
  int64_t oldest_nav_time = kInvalidGlobalID;
  int oldest_nav_id = kInvalidNavID;
  for (const auto& iter : nav_to_task_id_map_) {
    int nav_id = iter.first;
    // Root navigation contains parent link to navigations copied from other
    // TaskTracker that are not present in navigation stack of current tab.
    // It should not be considered for removal.
    if (nav_id == root_nav_id_)
      continue;
    if (oldest_nav_id == kInvalidNavID ||
        oldest_nav_time > iter.second.global_id) {
      oldest_nav_id = nav_id;
      oldest_nav_time = iter.second.global_id;
    }
  }
  nav_to_task_id_map_.erase(oldest_nav_id);
}

TaskTracker::TaskTracker() {}

TaskTracker::~TaskTracker() {}

TabTasks* TaskTracker::GetTabTasks(SessionID tab_id, SessionID parent_tab_id) {
  DVLOG(1) << "Getting tab tasks for " << tab_id << " with parent "
           << parent_tab_id;
  // If an existing TabTasks exists, attempt to reuse it. The caveat is that if
  // a parent tab id is provided, it must match the parent tab id associated
  // with the existing TabTasks. Otherwise a new TabTasks should be created from
  // the specified parent. This is to handle the case where at the time the
  // initial GetTabTasks is called, the parent id is not yet known, but it
  // becomes known at a later time.
  if (local_tab_tasks_map_.count(tab_id) > 0 &&
      (!parent_tab_id.is_valid() ||
       local_tab_tasks_map_[tab_id]->parent_tab_id() == parent_tab_id)) {
    return local_tab_tasks_map_[tab_id].get();
  }

  DVLOG(1) << "Creating tab tasks for " << tab_id;
  if (local_tab_tasks_map_.count(parent_tab_id) > 0) {
    // If the parent id is set, it means this tab forked from another tab.
    // In that case, the task for the current navigation might be part of a
    // larger task encompassing the parent tab. Perform a deep copy of the
    // parent's TabTasks object in order to simplify tracking this relationship.
    local_tab_tasks_map_[tab_id] =
        std::make_unique<TabTasks>(*local_tab_tasks_map_[parent_tab_id]);
    local_tab_tasks_map_[tab_id]->set_parent_tab_id(parent_tab_id);
  } else {
    local_tab_tasks_map_[tab_id] = std::make_unique<TabTasks>();
  }
  return local_tab_tasks_map_[tab_id].get();
}

void TaskTracker::CleanTabTasks(SessionID tab_id) {
  auto iter = local_tab_tasks_map_.find(tab_id);
  if (iter != local_tab_tasks_map_.end())
    local_tab_tasks_map_.erase(iter);
}

}  // namespace sync_sessions
