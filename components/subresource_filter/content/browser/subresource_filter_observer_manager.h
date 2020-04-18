// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_SUBRESOURCE_FILTER_OBSERVER_MANAGER_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_SUBRESOURCE_FILTER_OBSERVER_MANAGER_H_

#include "base/macros.h"
#include "base/observer_list.h"
#include "components/subresource_filter/content/browser/subresource_filter_observer.h"
#include "components/subresource_filter/core/common/activation_decision.h"
#include "components/subresource_filter/core/common/load_policy.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace subresource_filter {

struct ActivationState;

// Manages retaining the list of SubresourceFilterObservers and notifying them
// of various filtering events. Scoped to the lifetime of a WebContents.
class SubresourceFilterObserverManager
    : public content::WebContentsUserData<SubresourceFilterObserverManager> {
 public:
  explicit SubresourceFilterObserverManager(content::WebContents* web_contents);
  ~SubresourceFilterObserverManager() override;

  void AddObserver(SubresourceFilterObserver* observer);
  void RemoveObserver(SubresourceFilterObserver* observer);

  // Called when the SubresourceFilter Safe Browsing check is available for this
  // main frame navigation. Will be called at WillProcessResponse time at the
  // latest. Right now it will only include phishing and subresource filter
  // threat types.
  virtual void NotifySafeBrowsingCheckComplete(
      content::NavigationHandle* navigation_handle,
      safe_browsing::SBThreatType threat_type,
      const safe_browsing::ThreatMetadata& threat_metadata);

  // Will be called at the latest in the WillProcessResponse stage from a
  // NavigationThrottle that was registered before the throttle manager's
  // throttles created in MaybeAppendNavigationThrottles().
  void NotifyPageActivationComputed(
      content::NavigationHandle* navigation_handle,
      ActivationDecision activation_decision,
      const ActivationState& activation_state);

  // Called in WillStartRequest or WillRedirectRequest stage from a
  // SubframeNavigationFilteringThrottle.
  void NotifySubframeNavigationEvaluated(
      content::NavigationHandle* navigation_handle,
      LoadPolicy load_policy,
      bool is_ad_subframe);

 private:
  base::ObserverList<SubresourceFilterObserver> observers_;
  DISALLOW_COPY_AND_ASSIGN(SubresourceFilterObserverManager);
};

}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_SUBRESOURCE_FILTER_OBSERVER_MANAGER_H_
