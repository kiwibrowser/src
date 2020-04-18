// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SUPERVISED_USER_SUPERVISED_USER_NAVIGATION_OBSERVER_H_
#define CHROME_BROWSER_SUPERVISED_USER_SUPERVISED_USER_NAVIGATION_OBSERVER_H_

#include <vector>

#include "base/macros.h"
#include "chrome/browser/supervised_user/supervised_user_navigation_throttle.h"
#include "chrome/browser/supervised_user/supervised_user_service_observer.h"
#include "chrome/browser/supervised_user/supervised_user_url_filter.h"
#include "chrome/browser/supervised_user/supervised_users.h"
#include "chrome/common/supervised_user_commands.mojom.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "components/supervised_user_error_page/supervised_user_error_page.h"
#include "content/public/browser/web_contents_binding_set.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class SupervisedUserService;
class SupervisedUserInterstitial;

namespace content {
class NavigationHandle;
class WebContents;
}

class SupervisedUserNavigationObserver
    : public content::WebContentsUserData<SupervisedUserNavigationObserver>,
      public content::WebContentsObserver,
      public SupervisedUserServiceObserver,
      public supervised_user::mojom::SupervisedUserCommands {
 public:
  ~SupervisedUserNavigationObserver() override;

  const std::vector<std::unique_ptr<const sessions::SerializedNavigationEntry>>&
  blocked_navigations() const {
    return blocked_navigations_;
  }

  // Called when a network request to |url| is blocked.
  static void OnRequestBlocked(
      content::WebContents* web_contents,
      const GURL& url,
      supervised_user_error_page::FilteringBehaviorReason reason,
      int64_t navigation_id,
      const base::Callback<
          void(SupervisedUserNavigationThrottle::CallbackActions)>& callback);

  // WebContentsObserver implementation.
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  // SupervisedUserServiceObserver implementation.
  void OnURLFilterChanged() override;

 private:
  friend class content::WebContentsUserData<SupervisedUserNavigationObserver>;

  explicit SupervisedUserNavigationObserver(content::WebContents* web_contents);

  void OnRequestBlockedInternal(
      const GURL& url,
      supervised_user_error_page::FilteringBehaviorReason reason,
      int64_t navigation_id,
      const base::Callback<
          void(SupervisedUserNavigationThrottle::CallbackActions)>& callback);

  void URLFilterCheckCallback(
      const GURL& url,
      SupervisedUserURLFilter::FilteringBehavior behavior,
      supervised_user_error_page::FilteringBehaviorReason reason,
      bool uncertain);

  void MaybeShowInterstitial(
      const GURL& url,
      supervised_user_error_page::FilteringBehaviorReason reason,
      bool initial_page_load,
      int64_t navigation_id,
      const base::Callback<
          void(SupervisedUserNavigationThrottle::CallbackActions)>& callback);

  void OnInterstitialResult(
      const base::Callback<
          void(SupervisedUserNavigationThrottle::CallbackActions)>& callback,
      bool result);

  // supervised_user::mojom::SupervisedUserCommands implementation. Should not
  // be called when an interstitial is no longer showing. This should be
  // enforced by the mojo caller.
  void GoBack() override;
  void RequestPermission(RequestPermissionCallback callback) override;
  void Feedback() override;

  // Owned by SupervisedUserService.
  const SupervisedUserURLFilter* url_filter_;

  // Owned by SupervisedUserServiceFactory (lifetime of Profile).
  SupervisedUserService* supervised_user_service_;

  // Navigation ID of the navigation that triggered the last interstitial.
  int64_t interstitial_navigation_id_;

  bool is_showing_interstitial_ = false;

  std::vector<std::unique_ptr<const sessions::SerializedNavigationEntry>>
      blocked_navigations_;

  std::unique_ptr<SupervisedUserInterstitial> interstitial_;

  content::WebContentsFrameBindingSet<
      supervised_user::mojom::SupervisedUserCommands>
      binding_;

  base::WeakPtrFactory<SupervisedUserNavigationObserver> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SupervisedUserNavigationObserver);
};

#endif  // CHROME_BROWSER_SUPERVISED_USER_SUPERVISED_USER_NAVIGATION_OBSERVER_H_
