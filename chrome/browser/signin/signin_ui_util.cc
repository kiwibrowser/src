// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/signin_ui_util.h"

#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/gaia_cookie_manager_service_factory.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_error_controller_factory.h"
#include "chrome/browser/signin/signin_global_error.h"
#include "chrome/browser/signin/signin_global_error_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/common/pref_names.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/gaia_cookie_manager_service.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/user_manager/user_manager.h"
#include "third_party/re2/src/re2/re2.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/text_elider.h"

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
#include "chrome/browser/signin/account_consistency_mode_manager.h"
#include "chrome/browser/ui/webui/signin/dice_turn_sync_on_helper.h"
#endif

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
namespace {
void CreateDiceTurnSyncOnHelper(
    Profile* profile,
    Browser* browser,
    signin_metrics::AccessPoint signin_access_point,
    signin_metrics::PromoAction signin_promo_action,
    signin_metrics::Reason signin_reason,
    const std::string& account_id,
    DiceTurnSyncOnHelper::SigninAbortedMode signin_aborted_mode) {
  // DiceTurnSyncOnHelper is suicidal (it will delete itself once it finishes
  // enabling sync).
  new DiceTurnSyncOnHelper(profile, browser, signin_access_point,
                           signin_promo_action, signin_reason, account_id,
                           signin_aborted_mode);
}
}  // namespace
#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)

namespace signin_ui_util {

base::string16 GetAuthenticatedUsername(const SigninManagerBase* signin) {
  std::string user_display_name;

  if (signin->IsAuthenticated()) {
    user_display_name = signin->GetAuthenticatedAccountInfo().email;

#if defined(OS_CHROMEOS)
    if (user_manager::UserManager::IsInitialized()) {
      // On CrOS user email is sanitized and then passed to the signin manager.
      // Original email (containing dots) is stored as "display email".
      user_display_name = user_manager::UserManager::Get()->GetUserDisplayEmail(
          AccountId::FromUserEmail(user_display_name));
    }
#endif  // defined(OS_CHROMEOS)
  }

  return base::UTF8ToUTF16(user_display_name);
}

void InitializePrefsForProfile(Profile* profile) {
  if (profile->IsNewProfile()) {
    // Suppresses the upgrade tutorial for a new profile.
    profile->GetPrefs()->SetInteger(prefs::kProfileAvatarTutorialShown,
                                    kUpgradeWelcomeTutorialShowMax + 1);
  }
}

void ShowSigninErrorLearnMorePage(Profile* profile) {
  static const char kSigninErrorLearnMoreUrl[] =
      "https://support.google.com/chrome/answer/1181420?";
  NavigateParams params(profile, GURL(kSigninErrorLearnMoreUrl),
                        ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  Navigate(&params);
}

std::string GetDisplayEmail(Profile* profile, const std::string& account_id) {
  AccountTrackerService* account_tracker =
      AccountTrackerServiceFactory::GetForProfile(profile);
  std::string email = account_tracker->GetAccountInfo(account_id).email;
  if (email.empty()) {
    DCHECK_EQ(AccountTrackerService::MIGRATION_NOT_STARTED,
              account_tracker->GetMigrationState());
    return account_id;
  }
  return email;
}

void EnableSyncFromPromo(Browser* browser,
                         const AccountInfo& account,
                         signin_metrics::AccessPoint access_point,
                         bool is_default_promo_account) {
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  internal::EnableSyncFromPromo(browser, account, access_point,
                                is_default_promo_account,
                                base::BindOnce(&CreateDiceTurnSyncOnHelper));
#else
  internal::EnableSyncFromPromo(browser, account, access_point,
                                is_default_promo_account, base::DoNothing());
#endif
}

namespace internal {
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
        create_dice_turn_sync_on_helper_callback) {
  DCHECK(browser);
  DCHECK_NE(signin_metrics::AccessPoint::ACCESS_POINT_UNKNOWN, access_point);
  Profile* profile = browser->profile();
  DCHECK(!profile->IsOffTheRecord());

#if defined(OS_CHROMEOS)
  // It looks like on ChromeOS there are tests that expect that the Chrome
  // sign-in tab is presented even thought the user is signed in to Chrome
  // (e.g. BookmarkBubbleSignInDelegateTest.*). However signing in to Chrome in
  // a regular profile is not supported on ChromeOS as the primary account is
  // set when the profile is created.
  //
  // TODO(msarda): Investigate whether this flow needs to be supported on
  // ChromeOS and remove it if not.
  DCHECK(account.IsEmpty());
  chrome::ShowBrowserSignin(browser, access_point);
  return;
#endif

  if (SigninManagerFactory::GetForProfile(profile)->IsAuthenticated()) {
    DVLOG(1) << "There is already a primary account.";
    return;
  }

  if (account.IsEmpty()) {
    chrome::ShowBrowserSignin(browser, access_point);
    return;
  }

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  DCHECK(!account.account_id.empty());
  DCHECK(!account.email.empty());
  DCHECK(AccountConsistencyModeManager::IsDiceEnabledForProfile(profile));

  signin_metrics::PromoAction promo_action =
      is_default_promo_account
          ? signin_metrics::PromoAction::PROMO_ACTION_WITH_DEFAULT
          : signin_metrics::PromoAction::PROMO_ACTION_NOT_DEFAULT;

  ProfileOAuth2TokenService* token_service =
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile);
  bool needs_reauth_before_enable_sync =
      !token_service->RefreshTokenIsAvailable(account.account_id) ||
      token_service->RefreshTokenHasError(account.account_id);
  if (needs_reauth_before_enable_sync) {
    browser->signin_view_controller()->ShowDiceSigninTab(
        profiles::BUBBLE_VIEW_MODE_GAIA_SIGNIN, browser, access_point,
        promo_action, account.email);
    return;
  }

  signin_metrics::LogSigninAccessPointStarted(access_point, promo_action);
  signin_metrics::RecordSigninUserActionForAccessPoint(access_point,
                                                       promo_action);
  std::move(create_dice_turn_sync_on_helper_callback)
      .Run(profile, browser, access_point, promo_action,
           signin_metrics::Reason::REASON_SIGNIN_PRIMARY_ACCOUNT,
           account.account_id,
           DiceTurnSyncOnHelper::SigninAbortedMode::KEEP_ACCOUNT);
#else
  NOTREACHED();
#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)
}
}  // namespace internal

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
// TODO(tangltom): Add a unit test for this function.
std::vector<AccountInfo> GetAccountsForDicePromos(Profile* profile) {
  // Fetch account ids for accounts that have a token.
  ProfileOAuth2TokenService* token_service =
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile);
  std::vector<std::string> account_ids = token_service->GetAccounts();

  // Compute the default account.
  SigninManager* signin_manager = SigninManagerFactory::GetForProfile(profile);
  std::string default_account_id;
  if (signin_manager->IsAuthenticated()) {
    default_account_id = signin_manager->GetAuthenticatedAccountId();
  } else {
    // Fetch accounts in the Gaia cookies.
    GaiaCookieManagerService* cookie_manager_service =
        GaiaCookieManagerServiceFactory::GetForProfile(profile);
    std::vector<gaia::ListedAccount> cookie_accounts;
    bool gaia_accounts_stale = !cookie_manager_service->ListAccounts(
        &cookie_accounts, nullptr, "ProfileChooserView");
    UMA_HISTOGRAM_BOOLEAN("Profile.DiceUI.GaiaAccountsStale",
                          gaia_accounts_stale);
    if (!cookie_accounts.empty())
      default_account_id = cookie_accounts[0].id;
  }

  // Fetch account information for each id and make sure that the first account
  // in the list matches the first account in the Gaia cookies (if available).
  AccountTrackerService* account_tracker_service =
      AccountTrackerServiceFactory::GetForProfile(profile);
  std::vector<AccountInfo> accounts;
  for (const std::string& account_id : account_ids) {
    DCHECK(!account_id.empty());
    AccountInfo account_info =
        account_tracker_service->GetAccountInfo(account_id);
    if (account_info.IsEmpty() ||
        !signin_manager->IsAllowedUsername(account_info.email)) {
      continue;
    }
    if (account_id == default_account_id)
      accounts.insert(accounts.begin(), account_info);
    else
      accounts.push_back(account_info);
  }
  return accounts;
}

#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)

std::string GetAllowedDomain(std::string signin_pattern) {
  std::vector<std::string> splitted_signin_pattern = base::SplitString(
      signin_pattern, "@", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  // There are more than one '@'s in the pattern.
  if (splitted_signin_pattern.size() != 2)
    return std::string();

  std::string domain = splitted_signin_pattern[1];

  // Trims tailing '$' if existed.
  if (domain.size() > 0 && domain.back() == '$')
    domain.pop_back();

  // Trims tailing '\E' if existed.
  if (domain.size() > 1 &&
      base::EndsWith(domain, "\\E", base::CompareCase::SENSITIVE))
    domain.erase(domain.size() - 2);

  // Check if there is any special character in the domain. Note that
  // jsmith@[192.168.2.1] is not supported.
  if (!re2::RE2::FullMatch(domain, "[a-zA-Z0-9\\-.]+"))
    return std::string();

  return domain;
}

}  // namespace signin_ui_util
