// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_SUBRESOURCE_FILTER_OBSERVER_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_SUBRESOURCE_FILTER_OBSERVER_H_

#include "components/safe_browsing/db/v4_protocol_manager_util.h"
#include "components/subresource_filter/core/common/activation_decision.h"
#include "components/subresource_filter/core/common/load_policy.h"

namespace content {
class NavigationHandle;
}  // namespace content

namespace safe_browsing {
struct ThreatMetadata;
}  // namespace safe_browsing

namespace subresource_filter {

struct ActivationState;

// Class to receive notifications of subresource filter events for a given
// WebContents. Registered with a SubresourceFilterObserverManager.
class SubresourceFilterObserver {
 public:
  virtual ~SubresourceFilterObserver() = default;

  // Called before the observer manager is destroyed. Observers must unregister
  // themselves by this point.
  virtual void OnSubresourceFilterGoingAway() {}

  // Called when the SubresourceFilter Safe Browsing check is available for this
  // main frame navigation. Will be called at WillProcessResponse time at the
  // latest. Right now it will only include phishing and subresource filter
  // threat types.
  virtual void OnSafeBrowsingCheckComplete(
      content::NavigationHandle* navigation_handle,
      safe_browsing::SBThreatType threat_type,
      const safe_browsing::ThreatMetadata& threat_metadata) {}

  // Called at most once per navigation when page activation is computed. This
  // will be called before ReadyToCommitNavigation.
  virtual void OnPageActivationComputed(
      content::NavigationHandle* navigation_handle,
      ActivationDecision activation_decision,
      const ActivationState& activation_state) {}

  // Called before navigation commit, either at the WillStartRequest stage or
  // WillRedirectRequest stage. |is_ad_frame| is true if |load_policy| is
  // ALLOW or WOULD_DISALLOW or if ad tagging has determined that the frame is
  // an ad.
  virtual void OnSubframeNavigationEvaluated(
      content::NavigationHandle* navigation_handle,
      LoadPolicy load_policy,
      bool is_ad_subframe) {}
};

}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_SUBRESOURCE_FILTER_OBSERVER_H_
