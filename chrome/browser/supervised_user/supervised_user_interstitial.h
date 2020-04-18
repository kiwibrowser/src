// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SUPERVISED_USER_SUPERVISED_USER_INTERSTITIAL_H_
#define CHROME_BROWSER_SUPERVISED_USER_SUPERVISED_USER_INTERSTITIAL_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "chrome/browser/supervised_user/supervised_user_service_observer.h"
#include "chrome/browser/supervised_user/supervised_user_url_filter.h"
#include "components/supervised_user_error_page/supervised_user_error_page.h"
#include "content/public/browser/interstitial_page_delegate.h"
#include "url/gurl.h"

namespace content {
class InterstitialPage;
class WebContents;
}

class Profile;
class SupervisedUserService;

// Delegate for an interstitial page when a page is blocked for a supervised
// user because it is on a blacklist (in "allow everything" mode) or not on any
// whitelist (in "allow only specified sites" mode).
class SupervisedUserInterstitial : public content::InterstitialPageDelegate,
                                   public SupervisedUserServiceObserver {
 public:
  ~SupervisedUserInterstitial() override;

  // Interstitial type, used for testing.
  static const content::InterstitialPageDelegate::TypeID kTypeForTesting;

  static void Show(content::WebContents* web_contents,
                   const GURL& url,
                   supervised_user_error_page::FilteringBehaviorReason reason,
                   bool initial_page_load,
                   const base::Callback<void(bool)>& callback);

  static std::unique_ptr<SupervisedUserInterstitial> Create(
      content::WebContents* web_contents,
      const GURL& url,
      supervised_user_error_page::FilteringBehaviorReason reason,
      bool initial_page_load,
      const base::Callback<void(bool)>& callback);

  static std::string GetHTMLContents(
      Profile* profile,
      supervised_user_error_page::FilteringBehaviorReason reason);

  // InterstitialPageDelegate implementation. This method was made public while
  // both committed and non-committed interstitials are supported. Once
  // committed interstitials are the only codepath, this method will be removed
  // and replaced with separate handlers for go back and request permission.
  void CommandReceived(const std::string& command) override;

  // Permission requests need to be handled separately for committed
  // interstitials, since a callback needs to be setup so success/failure can be
  // reported back.
  void RequestPermission(base::OnceCallback<void(bool)> RequestCallback);

 private:
  SupervisedUserInterstitial(
      content::WebContents* web_contents,
      const GURL& url,
      supervised_user_error_page::FilteringBehaviorReason reason,
      bool initial_page_load,
      const base::Callback<void(bool)>& callback);

  void Init();

  // InterstitialPageDelegate implementation.
  std::string GetHTMLContents() override;
  void OnProceed() override;
  void OnDontProceed() override;
  content::InterstitialPageDelegate::TypeID GetTypeForTesting() const override;

  // SupervisedUserServiceObserver implementation.
  void OnURLFilterChanged() override;
  // TODO(treib): Also listen to OnCustodianInfoChanged and update as required.

  void OnAccessRequestAdded(bool success);

  // Returns whether we should now proceed on a previously-blocked URL.
  // Called initially before the interstitial is shown (to catch race
  // conditions), or when the URL filtering prefs change. Note that this does
  // not include the asynchronous online checks, so if those are enabled, then
  // the return value indicates only "allow" or "don't know".
  bool ShouldProceed();

  // Moves away from the page behind the interstitial when not proceeding with
  // the request.
  void MoveAwayFromCurrentPage();

  void DispatchContinueRequest(bool continue_request);

  void ProceedInternal();

  void DontProceedInternal();

  // Owns the interstitial, which owns us.
  content::WebContents* web_contents_;

  Profile* profile_;

  content::InterstitialPage* interstitial_page_;  // Owns us.

  GURL url_;
  supervised_user_error_page::FilteringBehaviorReason reason_;

  // True if the interstitial was shown while loading a page (with a pending
  // navigation), false if it was shown over an already loaded page.
  // Interstitials behave very differently in those cases.
  bool initial_page_load_;

  // True if we have already called Proceed() on the interstitial page.
  bool proceeded_;

  base::Callback<void(bool)> callback_;

  ScopedObserver<SupervisedUserService, SupervisedUserInterstitial>
      scoped_observer_;

  base::WeakPtrFactory<SupervisedUserInterstitial> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SupervisedUserInterstitial);
};

#endif  // CHROME_BROWSER_SUPERVISED_USER_SUPERVISED_USER_INTERSTITIAL_H_
