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

void SubresourceFilterObserverManager::NotifySafeBrowsingChecksComplete(
    content::NavigationHandle* navigation_handle,
    const SubresourceFilterObserver::SafeBrowsingCheckResults& results) {
  for (auto& observer : observers_) {
    observer.OnSafeBrowsingChecksComplete(navigation_handle, results);
  }
}

void SubresourceFilterObserverManager::NotifyPageActivationComputed(
    content::NavigationHandle* navigation_handle,
    const ActivationState& activation_state) {
  for (auto& observer : observers_) {
    observer.OnPageActivationComputed(navigation_handle, activation_state);
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

void SubresourceFilterObserverManager::NotifyAdSubframeDetected(
    content::RenderFrameHost* render_frame_host) {
  for (auto& observer : observers_)
    observer.OnAdSubframeDetected(render_frame_host);
}

}  // namespace subresource_filter
