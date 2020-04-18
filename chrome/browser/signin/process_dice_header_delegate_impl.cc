// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/process_dice_header_delegate_impl.h"

#include <utility>

#include "base/callback.h"
#include "base/logging.h"
#include "chrome/common/webui_url_constants.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/signin_manager.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace {

void RedirectToNtp(content::WebContents* contents) {
  VLOG(1) << "RedirectToNtp";
  contents->GetController().LoadURL(
      GURL(chrome::kChromeUINewTabURL), content::Referrer(),
      ui::PAGE_TRANSITION_AUTO_TOPLEVEL, std::string());
}

}  // namespace

ProcessDiceHeaderDelegateImpl::ProcessDiceHeaderDelegateImpl(
    content::WebContents* web_contents,
    PrefService* user_prefs,
    SigninManager* signin_manager,
    bool is_sync_signin_tab,
    EnableSyncCallback enable_sync_callback,
    ShowSigninErrorCallback show_signin_error_callback)
    : content::WebContentsObserver(web_contents),
      user_prefs_(user_prefs),
      signin_manager_(signin_manager),
      enable_sync_callback_(std::move(enable_sync_callback)),
      show_signin_error_callback_(std::move(show_signin_error_callback)),
      is_sync_signin_tab_(is_sync_signin_tab) {
  DCHECK(web_contents);
  DCHECK(user_prefs_);
  DCHECK(signin_manager_);
}

ProcessDiceHeaderDelegateImpl::~ProcessDiceHeaderDelegateImpl() = default;

bool ProcessDiceHeaderDelegateImpl::ShouldEnableSync() {
  if (!signin::IsDicePrepareMigrationEnabled()) {
    VLOG(1) << "Do not start sync after web sign-in [DICE prepare migration "
               "not enabled].";
    return false;
  }

  if (signin_manager_->IsAuthenticated()) {
    VLOG(1) << "Do not start sync after web sign-in [already authenticated].";
    return false;
  }

  if (!is_sync_signin_tab_) {
    VLOG(1)
        << "Do not start sync after web sign-in [not a Chrome sign-in tab].";
    return false;
  }

  return true;
}

void ProcessDiceHeaderDelegateImpl::EnableSync(const std::string& account_id) {
  if (!ShouldEnableSync()) {
    // No special treatment is needed if the user is not enabling sync.
    return;
  }

  content::WebContents* web_contents = this->web_contents();
  VLOG(1) << "Start sync after web sign-in.";
  std::move(enable_sync_callback_).Run(web_contents, account_id);

  if (!web_contents)
    return;

  // After signing in to Chrome, the user should be redirected to the NTP.
  RedirectToNtp(web_contents);
}

void ProcessDiceHeaderDelegateImpl::HandleTokenExchangeFailure(
    const std::string& email,
    const GoogleServiceAuthError& error) {
  DCHECK_NE(GoogleServiceAuthError::NONE, error.state());
  bool should_enable_sync = ShouldEnableSync();
  if (!should_enable_sync && !signin::IsDiceEnabledForProfile(user_prefs_))
    return;

  content::WebContents* web_contents = this->web_contents();
  if (should_enable_sync && web_contents)
    RedirectToNtp(web_contents);

  // Show the error even if the WebContents was closed, because the user may be
  // signed out of the web.
  std::move(show_signin_error_callback_)
      .Run(web_contents, error.ToString(), email);
}
