// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/browser/subresource_filter_observer_manager.h"

#include "components/subresource_filter/core/common/activation_state.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(
    subresource_filter::SubresourceFilterObserverManager);

namespace subresource_filter {

SubresourceFilterObserverManager::SubresourceFilterObserverManager(
    content::WebContents* web_contents) {}

SubresourceFilterObserverManager::~SubresourceFilterObserverManager() {
  for (auto& observer : observers_)
    observer.OnSubresourceFilterGoingAway();
}

void SubresourceFilterObserverManager::AddObserver(
    SubresourceFilterObserver* observer) {
  observers_.AddObserver(observer);
}

void SubresourceFilterObserverManager::RemoveObserver(
    SubresourceFilterObserver* observer) {
  observers_.RemoveObserver(observer);
}

void SubresourceFilterObserverManager::NotifySafeBrowsingCheckComplete(
    content::NavigationHandle* navigation_handle,
    safe_browsing::SBThreatType threat_type,
    const safe_browsing::ThreatMetadata& threat_metadata) {
  for (auto& observer : observers_) {
    observer.OnSafeBrowsingCheckComplete(navigation_handle, threat_type,
                                         threat_metadata);
  }
}

void SubresourceFilterObserverManager::NotifyPageActivationComputed(
    content::NavigationHandle* navigation_handle,
    ActivationDecision activation_decision,
    const ActivationState& activation_state) {
  for (auto& observer : observers_) {
    observer.OnPageActivationComputed(navigation_handle, activation_decision,
                                      activation_state);
  }
}

void SubresourceFilterObserverManager::NotifySubframeNavigationEvaluated(
    content::NavigationHandle* navigation_handle,
    LoadPolicy load_policy,
    bool is_ad_subframe) {
  for (auto& observer : observers_)
    observer.OnSubframeNavigationEvaluated(navigation_handle, load_policy,
                                           is_ad_subframe);
}

}  // namespace subresource_filter
