// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/web_state/global_web_state_event_tracker.h"

#include <stddef.h>

#include "base/macros.h"
#include "base/memory/singleton.h"
#import "ios/web/public/web_state/web_state_user_data.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

GlobalWebStateEventTracker* GlobalWebStateEventTracker::GetInstance() {
  return base::Singleton<GlobalWebStateEventTracker>::get();
}

GlobalWebStateEventTracker::GlobalWebStateEventTracker()
    : scoped_observer_(this) {}

GlobalWebStateEventTracker::~GlobalWebStateEventTracker() = default;

void GlobalWebStateEventTracker::OnWebStateCreated(WebState* web_state) {
  scoped_observer_.Add(web_state);
}

void GlobalWebStateEventTracker::AddObserver(GlobalWebStateObserver* observer) {
  observer_list_.AddObserver(observer);
}

void GlobalWebStateEventTracker::RemoveObserver(
    GlobalWebStateObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

void GlobalWebStateEventTracker::NavigationItemsPruned(
    WebState* web_state,
    size_t pruned_item_count) {
  for (auto& observer : observer_list_)
    observer.NavigationItemsPruned(web_state, pruned_item_count);
}

void GlobalWebStateEventTracker::NavigationItemChanged(WebState* web_state) {
  for (auto& observer : observer_list_)
    observer.NavigationItemChanged(web_state);
}

void GlobalWebStateEventTracker::NavigationItemCommitted(
    WebState* web_state,
    const LoadCommittedDetails& load_details) {
  for (auto& observer : observer_list_)
    observer.NavigationItemCommitted(web_state, load_details);
}

void GlobalWebStateEventTracker::DidStartNavigation(
    WebState* web_state,
    NavigationContext* navigation_context) {
  for (auto& observer : observer_list_)
    observer.WebStateDidStartNavigation(web_state, navigation_context);
}

void GlobalWebStateEventTracker::DidStartLoading(WebState* web_state) {
  for (auto& observer : observer_list_)
    observer.WebStateDidStartLoading(web_state);
}

void GlobalWebStateEventTracker::DidStopLoading(WebState* web_state) {
  for (auto& observer : observer_list_)
    observer.WebStateDidStopLoading(web_state);
}

void GlobalWebStateEventTracker::PageLoaded(
    WebState* web_state,
    PageLoadCompletionStatus load_completion_status) {
  for (auto& observer : observer_list_)
    observer.PageLoaded(web_state, load_completion_status);
}

void GlobalWebStateEventTracker::RenderProcessGone(WebState* web_state) {
  for (auto& observer : observer_list_)
    observer.RenderProcessGone(web_state);
}

void GlobalWebStateEventTracker::WebStateDestroyed(WebState* web_state) {
  for (auto& observer : observer_list_)
    observer.WebStateDestroyed(web_state);
  scoped_observer_.Remove(web_state);
}

}  // namespace web
