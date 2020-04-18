// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_load_tracker.h"

#include "base/stl_util.h"
#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"

namespace resource_coordinator {

TabLoadTracker::~TabLoadTracker() = default;

// static
TabLoadTracker* TabLoadTracker::Get() {
  static base::NoDestructor<TabLoadTracker> tab_load_tracker;
  return tab_load_tracker.get();
}

TabLoadTracker::LoadingState TabLoadTracker::GetLoadingState(
    content::WebContents* web_contents) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto it = tabs_.find(web_contents);
  DCHECK(it != tabs_.end());
  return it->second.loading_state;
}

size_t TabLoadTracker::GetTabCount() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return tabs_.size();
}

size_t TabLoadTracker::GetTabCount(LoadingState loading_state) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return state_counts_[loading_state];
}

size_t TabLoadTracker::GetUnloadedTabCount() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return state_counts_[UNLOADED];
}

size_t TabLoadTracker::GetLoadingTabCount() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return state_counts_[LOADING];
}

size_t TabLoadTracker::GetLoadedTabCount() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return state_counts_[LOADED];
}

void TabLoadTracker::AddObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.AddObserver(observer);
}

void TabLoadTracker::RemoveObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

void TabLoadTracker::TransitionStateForTesting(
    content::WebContents* web_contents,
    LoadingState loading_state) {
  auto it = tabs_.find(web_contents);
  DCHECK(it != tabs_.end());
  TransitionState(it, loading_state, false);
}

TabLoadTracker::TabLoadTracker() = default;

void TabLoadTracker::StartTracking(content::WebContents* web_contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!base::ContainsKey(tabs_, web_contents));

  LoadingState loading_state = DetermineLoadingState(web_contents);

  // Insert the tab, making sure it's state is consistent with the valid states
  // documented in TransitionState.
  WebContentsData data;
  data.loading_state = loading_state;
  if (data.loading_state == LOADING)
    data.did_start_loading_seen = true;
  tabs_.insert(std::make_pair(web_contents, data));
  ++state_counts_[data.loading_state];

  for (Observer& observer : observers_)
    observer.OnStartTracking(web_contents, loading_state);
}

void TabLoadTracker::StopTracking(content::WebContents* web_contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto it = tabs_.find(web_contents);
  DCHECK(it != tabs_.end());

  auto loading_state = it->second.loading_state;
  DCHECK_NE(0u, state_counts_[it->second.loading_state]);
  --state_counts_[it->second.loading_state];
  tabs_.erase(it);

  for (Observer& observer : observers_)
    observer.OnStopTracking(web_contents, loading_state);
}

void TabLoadTracker::DidStartLoading(content::WebContents* web_contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!web_contents->IsLoadingToDifferentDocument())
    return;
  auto it = tabs_.find(web_contents);
  DCHECK(it != tabs_.end());
  if (it->second.loading_state == LOADING) {
    DCHECK(it->second.did_start_loading_seen);
    return;
  }
  it->second.did_start_loading_seen = true;
}

void TabLoadTracker::DidReceiveResponse(content::WebContents* web_contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto it = tabs_.find(web_contents);
  DCHECK(it != tabs_.end());
  if (it->second.loading_state == LOADING) {
    DCHECK(it->second.did_start_loading_seen);
    return;
  }
  // A transition to loading requires both DidStartLoading (navigation
  // committed) and DidReceiveResponse (data has been trasmitted over the
  // network) events to occur. This is because NavigationThrottles can block
  // actual network requests, but not the rest of the state machinery.
  if (!it->second.did_start_loading_seen)
    return;
  TransitionState(it, LOADING, true);
}

void TabLoadTracker::DidStopLoading(content::WebContents* web_contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (resource_coordinator::IsPageAlmostIdleSignalEnabled())
    return;
  MaybeTransitionToLoaded(web_contents);
}

void TabLoadTracker::DidFailLoad(content::WebContents* web_contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  MaybeTransitionToLoaded(web_contents);
}

void TabLoadTracker::OnPageAlmostIdle(content::WebContents* web_contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(resource_coordinator::IsPageAlmostIdleSignalEnabled());
  // PageAlmostIdle signals can be arbitrarily delayed as they are asynchronous.
  // As such, they can arrive after the web contents in question no longer
  // exists.
  if (!base::ContainsKey(tabs_, web_contents))
    return;
  MaybeTransitionToLoaded(web_contents);
}

TabLoadTracker::LoadingState TabLoadTracker::DetermineLoadingState(
    content::WebContents* web_contents) {
  // Determine if the WebContents is actively loading, using our definition of
  // loading. Start from the assumption that it is UNLOADED.
  LoadingState loading_state = UNLOADED;
  if (web_contents->IsLoadingToDifferentDocument() &&
      !web_contents->IsWaitingForResponse()) {
    loading_state = LOADING;
  } else {
    // Determine if the WebContents is already loaded. A loaded WebContents has
    // a committed navigation entry, is not in an initial navigation, and
    // doesn't require a reload. This can occur during prerendering, when an
    // already rendered WebContents is swapped in at the moment of a navigation.
    content::NavigationController& controller = web_contents->GetController();
    if (controller.GetLastCommittedEntry() != nullptr &&
        !controller.IsInitialNavigation() && !controller.NeedsReload()) {
      loading_state = LOADED;
    }
  }

  return loading_state;
}

void TabLoadTracker::MaybeTransitionToLoaded(
    content::WebContents* web_contents) {
  auto it = tabs_.find(web_contents);
  DCHECK(it != tabs_.end());
  if (it->second.loading_state != LOADING)
    return;
  TransitionState(it, LOADED, true);
}

void TabLoadTracker::TransitionState(TabMap::iterator it,
                                     LoadingState loading_state,
                                     bool validate_transition) {
#if DCHECK_IS_ON()
  if (validate_transition) {
    // Validate the transition.
    switch (loading_state) {
      case LOADING: {
        DCHECK_NE(LOADING, it->second.loading_state);
        DCHECK(it->second.did_start_loading_seen);
        break;
      }

      case LOADED: {
        DCHECK_EQ(LOADING, it->second.loading_state);
        DCHECK(it->second.did_start_loading_seen);
        break;
      }

      case UNLOADED:  // It never makes sense to transition to UNLOADED.
      case LOADING_STATE_MAX:
        NOTREACHED();
    }
  }
#endif

  --state_counts_[it->second.loading_state];
  it->second.loading_state = loading_state;
  ++state_counts_[loading_state];

  // If the destination state is LOADED, then also clear the
  // |did_start_loading_seen| state.
  if (loading_state == LOADED)
    it->second.did_start_loading_seen = false;

  for (Observer& observer : observers_)
    observer.OnLoadingStateChange(it->first, loading_state);
}

TabLoadTracker::Observer::Observer() {}

TabLoadTracker::Observer::~Observer() {}

}  // namespace resource_coordinator
