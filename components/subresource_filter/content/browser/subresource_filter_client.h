// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_SUBRESOURCE_FILTER_CLIENT_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_SUBRESOURCE_FILTER_CLIENT_H_

#include "components/subresource_filter/content/browser/verified_ruleset_dealer.h"
#include "content/public/browser/web_contents.h"

namespace content {
class NavigationHandle;
}  // namespace content

namespace subresource_filter {

class SubresourceFilterClient {
 public:
  virtual ~SubresourceFilterClient() = default;

  // Informs the embedder to show some UI indicating that resources are being
  // blocked.
  virtual void ShowNotification() = 0;

  // Called when the component is starting to observe a new navigation.
  virtual void OnNewNavigationStarted() = 0;

  // Called when the activation decision is otherwise completely computed by the
  // subresource filter. At this point, the embedder still has a chance to
  // return false to suppress the activation. Returns whether the activation
  // should be whitelisted for this navigation.
  //
  // Precondition: The navigation must be a main frame navigation.
  virtual bool OnPageActivationComputed(
      content::NavigationHandle* navigation_handle,
      bool activated) = 0;

  virtual VerifiedRulesetDealer::Handle* GetRulesetDealer() = 0;

  // Returns whether this navigation should be forced to be activated. This is
  // currently only used for devtools.
  virtual bool ForceActivationInCurrentWebContents() = 0;
};

}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_SUBRESOURCE_FILTER_CLIENT_H_
