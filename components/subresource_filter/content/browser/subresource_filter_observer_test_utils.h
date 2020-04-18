// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_SUBRESOURCE_FILTER_OBSERVER_TEST_UTILS_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_SUBRESOURCE_FILTER_OBSERVER_TEST_UTILS_H_

#include <map>
#include <utility>

#include "base/macros.h"
#include "base/optional.h"
#include "base/scoped_observer.h"
#include "components/safe_browsing/db/util.h"
#include "components/safe_browsing/db/v4_protocol_manager_util.h"
#include "components/subresource_filter/content/browser/subresource_filter_observer.h"
#include "components/subresource_filter/content/browser/subresource_filter_observer_manager.h"
#include "components/subresource_filter/core/common/activation_decision.h"
#include "components/subresource_filter/core/common/load_policy.h"
#include "content/public/browser/web_contents_observer.h"
#include "url/gurl.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace subresource_filter {

// This class can be used to observe subresource filtering events associated
// with a particular web contents. Particular events can be expected by using
// the Get* methods.
class TestSubresourceFilterObserver : public SubresourceFilterObserver,
                                      public content::WebContentsObserver {
 public:
  TestSubresourceFilterObserver(content::WebContents* web_contents);
  ~TestSubresourceFilterObserver() override;

  // SubresourceFilterObserver:
  void OnSubresourceFilterGoingAway() override;
  void OnSafeBrowsingCheckComplete(
      content::NavigationHandle* navigation_handle,
      safe_browsing::SBThreatType threat_type,
      const safe_browsing::ThreatMetadata& threat_metadata) override;
  void OnPageActivationComputed(
      content::NavigationHandle* navigation_handle,
      ActivationDecision activation_decision,
      const ActivationState& activation_state) override;
  void OnSubframeNavigationEvaluated(
      content::NavigationHandle* navigation_handle,
      LoadPolicy load_policy,
      bool is_ad_subframe) override;

  // content::WebContentsObserver
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  base::Optional<ActivationDecision> GetPageActivation(const GURL& url) const;
  base::Optional<LoadPolicy> GetSubframeLoadPolicy(const GURL& url) const;
  base::Optional<bool> GetIsAdSubframe(const GURL& url) const;
  base::Optional<ActivationDecision> GetPageActivationForLastCommittedLoad()
      const;

  using SafeBrowsingCheck =
      std::pair<safe_browsing::SBThreatType, safe_browsing::ThreatMetadata>;
  base::Optional<SafeBrowsingCheck> GetSafeBrowsingResult(
      const GURL& url) const;

 private:
  std::map<GURL, LoadPolicy> subframe_load_evaluations_;
  std::map<GURL, bool> ad_subframe_evaluations_;
  std::map<GURL, ActivationDecision> page_activations_;
  std::map<GURL, SafeBrowsingCheck> safe_browsing_checks_;

  std::map<content::NavigationHandle*, ActivationDecision> pending_activations_;
  base::Optional<ActivationDecision> last_committed_activation_;

  ScopedObserver<SubresourceFilterObserverManager, SubresourceFilterObserver>
      scoped_observer_;
  DISALLOW_COPY_AND_ASSIGN(TestSubresourceFilterObserver);
};

}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_SUBRESOURCE_FILTER_OBSERVER_TEST_UTILS_H_
