// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_SIGNIN_UI_UTIL_H_
#define CHROME_BROWSER_SIGNIN_SIGNIN_UI_UTIL_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/strings/string16.h"
#include "build/buildflag.h"
#include "chrome/browser/ui/webui/signin/dice_turn_sync_on_helper.h"
#include "components/signin/core/browser/account_info.h"
#include "components/signin/core/browser/signin_buildflags.h"
#include "components/signin/core/browser/signin_metrics.h"

class Profile;
class Browser;
class SigninManagerBase;

// Utility functions to gather status information from the various signed in
// services and construct messages suitable for showing in UI.
namespace signin_ui_util {

// The maximum number of times to show the welcome tutorial for an upgrade user.
const int kUpgradeWelcomeTutorialShowMax = 1;

// Returns the username of the authenticated user or an empty string if there is
// no authenticated user.
base::string16 GetAuthenticatedUsername(const SigninManagerBase* signin);

// Initializes signin-related preferences.
void InitializePrefsForProfile(Profile* profile);

// Shows a learn more page for signin errors.
void ShowSigninErrorLearnMorePage(Profile* profile);

// This function is used to enable sync for a given account:
// * This function does nothing if the user is already signed in to Chrome.
// * If |account| is empty, then it presents the Chrome sign-in page.
// * If token service has an invalid refreh token for account |account|,
//   then it presents the Chrome sign-in page with |account.emil| prefilled.
// * If token service has a valid refresh token for |account|, then it
//   enables sync for |account|.
// |is_default_promo_account| is true if |account| corresponds to the default
// account in the promo. It is ignored if |account| is empty.
void EnableSyncFromPromo(Browser* browser,
                         const AccountInfo& account,
                         signin_metrics::AccessPoint access_point,
                         bool is_default_promo_account);

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
// Returns the display email string for the given account.  If the profile
// has not been migrated to use gaia ids, then its possible for the display
// to not ne known yet.  In this case, use |account_id|, which is assumed to
// be an email address.
std::string GetDisplayEmail(Profile* profile, const std::string& account_id);

// Returns the list of all accounts that have a token. The default account in
// the Gaia cookies will be the first account in the list.
std::vector<AccountInfo> GetAccountsForDicePromos(Profile* profile);

#endif

// Returns the domain of the policy value of RestrictSigninToPattern. Returns
// an empty string if the policy is not set or can not be parsed. The parser
// only supports the policy value that matches [^@]+@[a-zA-Z0-9\-.]+(\\E)?\$?$.
// Also, the parser does not validate the policy value.
std::string GetAllowedDomain(std::string signin_pattern);

namespace internal {
// Same as |EnableSyncFromPromo| but with a callback that creates a
// DiceTurnSyncOnHelper so that it can be unit tested.
void EnableSyncFromPromo(
    Browser* browser,
    const AccountInfo& account,
    signin_metrics::AccessPoint access_point,
    bool is_default_promo_account,
    base::OnceCallback<
        void(Profile* profile,
             Browser* browser,
             signin_metrics::AccessPoint signin_access_point,
             signin_metrics::PromoAction signin_promo_action,
             signin_metrics::Reason signin_reason,
             const std::string& account_id,
             DiceTurnSyncOnHelper::SigninAbortedMode signin_aborted_mode)>
        create_dice_turn_sync_on_helper_callback);
}  // namespace internal

}  // namespace signin_ui_util

#endif  // CHROME_BROWSER_SIGNIN_SIGNIN_UI_UTIL_H_
