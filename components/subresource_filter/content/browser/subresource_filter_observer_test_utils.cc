// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/browser/subresource_filter_observer_test_utils.h"

#include "base/logging.h"
#include "content/public/browser/navigation_handle.h"

namespace subresource_filter {

TestSubresourceFilterObserver::TestSubresourceFilterObserver(
    content::WebContents* web_contents)
    : scoped_observer_(this) {
  auto* manager =
      SubresourceFilterObserverManager::FromWebContents(web_contents);
  DCHECK(manager);
  scoped_observer_.Add(manager);
  Observe(web_contents);
}

TestSubresourceFilterObserver::~TestSubresourceFilterObserver() {}

void TestSubresourceFilterObserver::OnSubresourceFilterGoingAway() {
  scoped_observer_.RemoveAll();
}

void TestSubresourceFilterObserver::OnSafeBrowsingCheckComplete(
    content::NavigationHandle* navigation_handle,
    safe_browsing::SBThreatType threat_type,
    const safe_browsing::ThreatMetadata& threat_metadata) {
  DCHECK(navigation_handle->IsInMainFrame());
  safe_browsing_checks_[navigation_handle->GetURL()] =
      std::make_pair(threat_type, threat_metadata);
}

void TestSubresourceFilterObserver::OnPageActivationComputed(
    content::NavigationHandle* navigation_handle,
    ActivationDecision activation_decision,
    const ActivationState& activation_state) {
  DCHECK(navigation_handle->IsInMainFrame());
  page_activations_[navigation_handle->GetURL()] = activation_decision;
  pending_activations_[navigation_handle] = activation_decision;
}

void TestSubresourceFilterObserver::OnSubframeNavigationEvaluated(
    content::NavigationHandle* navigation_handle,
    LoadPolicy load_policy,
    bool is_ad_subframe) {
  subframe_load_evaluations_[navigation_handle->GetURL()] = load_policy;
  ad_subframe_evaluations_[navigation_handle->GetURL()] = is_ad_subframe;
}

void TestSubresourceFilterObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  auto it = pending_activations_.find(navigation_handle);
  bool did_compute = it != pending_activations_.end();
  if (!navigation_handle->IsInMainFrame() ||
      !navigation_handle->HasCommitted() || navigation_handle->IsErrorPage()) {
    if (did_compute)
      pending_activations_.erase(it);
    return;
  }

  if (did_compute) {
    last_committed_activation_ = it->second;
    pending_activations_.erase(it);
  } else {
    last_committed_activation_ = base::Optional<ActivationDecision>();
  }
}

base::Optional<ActivationDecision>
TestSubresourceFilterObserver::GetPageActivation(const GURL& url) const {
  auto it = page_activations_.find(url);
  if (it != page_activations_.end())
    return it->second;
  return base::Optional<ActivationDecision>();
}

base::Optional<bool> TestSubresourceFilterObserver::GetIsAdSubframe(
    const GURL& url) const {
  auto it = ad_subframe_evaluations_.find(url);
  if (it != ad_subframe_evaluations_.end())
    return it->second;
  return base::Optional<bool>();
}

base::Optional<LoadPolicy> TestSubresourceFilterObserver::GetSubframeLoadPolicy(
    const GURL& url) const {
  auto it = subframe_load_evaluations_.find(url);
  if (it != subframe_load_evaluations_.end())
    return it->second;
  return base::Optional<LoadPolicy>();
}

base::Optional<ActivationDecision>
TestSubresourceFilterObserver::GetPageActivationForLastCommittedLoad() const {
  return last_committed_activation_;
}

base::Optional<TestSubresourceFilterObserver::SafeBrowsingCheck>
TestSubresourceFilterObserver::GetSafeBrowsingResult(const GURL& url) const {
  auto it = safe_browsing_checks_.find(url);
  if (it != safe_browsing_checks_.end())
    return it->second;
  return base::Optional<SafeBrowsingCheck>();
}

}  // namespace subresource_filter
