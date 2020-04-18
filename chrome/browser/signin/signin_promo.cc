// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/signin_promo.h"

#include <limits.h>

#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/signin/gaia_auth_extension_loader.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/google/google_brand.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/search_engines/ui_thread_search_terms_data.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/signin_error_controller_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/signin/signin_promo_util.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/google/core/browser/google_util.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/signin_manager.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "google_apis/gaia/gaia_urls.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif

using content::WebContents;

namespace {

// The maximum number of times we want to show the sign in promo at startup.
const int kSignInPromoShowAtStartupMaximum = 10;

// Forces the web based signin flow when set.
bool g_force_web_based_signin_flow = false;

// Checks we want to show the sign in promo for the given brand.
bool AllowPromoAtStartupForCurrentBrand() {
  std::string brand;
  google_brand::GetBrand(&brand);

  if (brand.empty())
    return true;

  if (google_brand::IsInternetCafeBrandCode(brand))
    return false;

  // Enable for both organic and distribution.
  return true;
}

// Returns true if a user has seen the sign in promo at startup previously.
bool HasShownPromoAtStartup(Profile* profile) {
  return profile->GetPrefs()->HasPrefPath(prefs::kSignInPromoStartupCount);
}

// Returns true if the user has previously skipped the sign in promo.
bool HasUserSkippedPromo(Profile* profile) {
  return profile->GetPrefs()->GetBoolean(prefs::kSignInPromoUserSkipped);
}

// Returns the sign in promo URL with the given arguments in the query.
// |access_point| indicates where the sign in is being initiated.
// |reason| indicates the purpose of using this URL.
// |auto_close| whether to close the sign in promo automatically when done.
// |is_constrained| whether to load the URL in a constrained window, false
// by default.
GURL GetPromoURL(signin_metrics::AccessPoint access_point,
                 signin_metrics::Reason reason,
                 bool auto_close,
                 bool is_constrained) {
  CHECK_LT(static_cast<int>(access_point),
           static_cast<int>(signin_metrics::AccessPoint::ACCESS_POINT_MAX));
  CHECK_NE(static_cast<int>(access_point),
           static_cast<int>(signin_metrics::AccessPoint::ACCESS_POINT_UNKNOWN));
  CHECK_LT(static_cast<int>(reason),
           static_cast<int>(signin_metrics::Reason::REASON_MAX));
  CHECK_NE(static_cast<int>(reason),
           static_cast<int>(signin_metrics::Reason::REASON_UNKNOWN_REASON));

  GURL url(chrome::kChromeUIChromeSigninURL);
  url = net::AppendQueryParameter(
      url, signin::kSignInPromoQueryKeyAccessPoint,
      base::IntToString(static_cast<int>(access_point)));
  url = net::AppendQueryParameter(url, signin::kSignInPromoQueryKeyReason,
                                  base::IntToString(static_cast<int>(reason)));
  if (auto_close) {
    url = net::AppendQueryParameter(url, signin::kSignInPromoQueryKeyAutoClose,
                                    "1");
  }
  if (is_constrained) {
    url = net::AppendQueryParameter(
        url, signin::kSignInPromoQueryKeyConstrained, "1");
  }

  return url;
}

GURL GetReauthURL(signin_metrics::AccessPoint access_point,
                  signin_metrics::Reason reason,
                  const std::string& email,
                  bool auto_close,
                  bool is_constrained) {
  GURL url = GetPromoURL(access_point, reason, auto_close, is_constrained);
  url = net::AppendQueryParameter(url, "email", email);
  url = net::AppendQueryParameter(url, "validateEmail", "1");
  return net::AppendQueryParameter(url, "readOnlyEmail", "1");
}

}  // namespace

namespace signin {

bool ShouldShowPromoAtStartup(Profile* profile, bool is_new_profile) {
  DCHECK(profile);

  // Don't show if the profile is an incognito.
  if (profile->IsOffTheRecord())
    return false;

  if (!ShouldShowPromo(profile))
    return false;

  if (!is_new_profile) {
    if (!HasShownPromoAtStartup(profile))
      return false;
  }

#if defined(OS_WIN)
  // Do not show the promo on first run on Win10 and newer.
  if (is_new_profile && base::win::GetVersion() >= base::win::VERSION_WIN10)
    return false;
#endif

  if (HasUserSkippedPromo(profile))
    return false;

  // For Chinese users skip the sign in promo.
  if (g_browser_process->GetApplicationLocale() == "zh-CN")
    return false;

  PrefService* prefs = profile->GetPrefs();
  int show_count = prefs->GetInteger(prefs::kSignInPromoStartupCount);
  if (show_count >= kSignInPromoShowAtStartupMaximum)
    return false;

  // This pref can be set in the master preferences file to allow or disallow
  // showing the sign in promo at startup.
  if (prefs->HasPrefPath(prefs::kSignInPromoShowOnFirstRunAllowed))
    return prefs->GetBoolean(prefs::kSignInPromoShowOnFirstRunAllowed);

  // For now don't show the promo for some brands.
  if (!AllowPromoAtStartupForCurrentBrand())
    return false;

  // Default to show the promo for Google Chrome builds.
#if defined(GOOGLE_CHROME_BUILD)
  return true;
#else
  return false;
#endif
}

void DidShowPromoAtStartup(Profile* profile) {
  int show_count = profile->GetPrefs()->GetInteger(
      prefs::kSignInPromoStartupCount);
  show_count++;
  profile->GetPrefs()->SetInteger(prefs::kSignInPromoStartupCount, show_count);
}

void SetUserSkippedPromo(Profile* profile) {
  profile->GetPrefs()->SetBoolean(prefs::kSignInPromoUserSkipped, true);
}

GURL GetLandingURL(signin_metrics::AccessPoint access_point) {
  GURL url(extensions::kGaiaAuthExtensionOrigin);
  GURL::Replacements replacements;
  replacements.SetPathStr(kSigninPromoLandingURLSuccessPage);
  url = url.ReplaceComponents(replacements);

  url = net::AppendQueryParameter(
      url, kSignInPromoQueryKeyAccessPoint,
      base::IntToString(static_cast<int>(access_point)));

  // TODO(gogerald): right now, gaia server needs to distinguish the source from
  // signin_metrics::SOURCE_START_PAGE, signin_metrics::SOURCE_SETTINGS and
  // the others to show advanced sync settings, remove them after
  // switching to Minute Maid sign in flow.
  signin_metrics::Source source = signin_metrics::SOURCE_OTHERS;
  if (access_point == signin_metrics::AccessPoint::ACCESS_POINT_START_PAGE) {
    source = signin_metrics::SOURCE_START_PAGE;
  } else if (access_point ==
             signin_metrics::AccessPoint::ACCESS_POINT_SETTINGS) {
    source = signin_metrics::SOURCE_SETTINGS;
  }
  url = net::AppendQueryParameter(url, signin::kSignInPromoQueryKeySource,
                                  base::IntToString(static_cast<int>(source)));
  return GURL(url);
}

GURL GetPromoURLForTab(signin_metrics::AccessPoint access_point,
                       signin_metrics::Reason reason,
                       bool auto_close) {
  if (base::FeatureList::IsEnabled(
          features::kRemoveUsageOfDeprecatedGaiaSigninEndpoint)) {
    // The full-tab sign-in endpoint is deprecated. Use the constrained page for
    // the full-tab URL as well.
    return GetPromoURL(access_point, reason, auto_close,
                       true /* is_constrained */);
  }

  return GetPromoURL(access_point, reason, auto_close,
                     false /* is_constrained */);
}

GURL GetPromoURLForDialog(signin_metrics::AccessPoint access_point,
                          signin_metrics::Reason reason,
                          bool auto_close) {
  return GetPromoURL(access_point, reason, auto_close,
                     true /* is_constrained */);
}

GURL GetReauthURLForDialog(signin_metrics::AccessPoint access_point,
                           signin_metrics::Reason reason,
                           Profile* profile,
                           const std::string& account_id) {
  AccountInfo info = AccountTrackerServiceFactory::GetForProfile(profile)
                         ->GetAccountInfo(account_id);
  return GetReauthURL(access_point, reason, info.email, true /* auto_close */,
                      true /* is_constrained */);
}

GURL GetReauthURLForTab(signin_metrics::AccessPoint access_point,
                        signin_metrics::Reason reason,
                        Profile* profile,
                        const std::string& account_id) {
  AccountInfo info =
      AccountTrackerServiceFactory::GetForProfile(profile)->GetAccountInfo(
          account_id);

  if (base::FeatureList::IsEnabled(
          features::kRemoveUsageOfDeprecatedGaiaSigninEndpoint)) {
    // The full-tab sign-in endpoint is deprecated. Use the constrained page for
    // the full-tab URL as well.
    return GetReauthURL(access_point, reason, info.email, true /* auto_close */,
                        true /* is_constrained */);
  }

  return GetReauthURL(access_point, reason, info.email, true /* auto_close */,
                      false /* is_constrained */);
}

GURL GetReauthURLWithEmailForDialog(signin_metrics::AccessPoint access_point,
                                    signin_metrics::Reason reason,
                                    const std::string& email) {
  return GetReauthURL(access_point, reason, email, true /* auto_close */,
                      true /* is_constrained */);
}

GURL GetSigninURLForDice(Profile* profile, const std::string& email) {
  DCHECK(signin::IsDicePrepareMigrationEnabled());
  GURL url = GaiaUrls::GetInstance()->signin_chrome_sync_dice();
  if (!email.empty())
    url = net::AppendQueryParameter(url, "email_hint", email);
  // Pass www.gooogle.com as the continue URL as otherwise Gaia navigates to
  // myaccount which may be very confusing for the user.
  return net::AppendQueryParameter(
      url, "continue", UIThreadSearchTermsData(profile).GoogleBaseURLValue());
}

GURL GetSigninPartitionURL() {
  return GURL("chrome-guest://chrome-signin/?");
}

GURL GetSigninURLFromBubbleViewMode(Profile* profile,
                                    profiles::BubbleViewMode mode,
                                    signin_metrics::AccessPoint access_point) {
  switch (mode) {
    case profiles::BUBBLE_VIEW_MODE_GAIA_SIGNIN:
      return GetPromoURLForDialog(
          access_point, signin_metrics::Reason::REASON_SIGNIN_PRIMARY_ACCOUNT,
          false /* auto_close */);
      break;
    case profiles::BUBBLE_VIEW_MODE_GAIA_ADD_ACCOUNT:
      return GetPromoURLForDialog(
          access_point, signin_metrics::Reason::REASON_ADD_SECONDARY_ACCOUNT,
          false /* auto_close */);
      break;
    case profiles::BUBBLE_VIEW_MODE_GAIA_REAUTH: {
      const SigninErrorController* error_controller =
          SigninErrorControllerFactory::GetForProfile(profile);
      CHECK(error_controller);
      DCHECK(error_controller->HasError());
      return GetReauthURLForDialog(
          access_point, signin_metrics::Reason::REASON_REAUTHENTICATION,
          profile, error_controller->error_account_id());
      break;
    }
    default:
      NOTREACHED() << "Called with invalid mode=" << mode;
      return GURL();
  }
}

signin_metrics::AccessPoint GetAccessPointForPromoURL(const GURL& url) {
  std::string value;
  if (!net::GetValueForKeyInQuery(url, kSignInPromoQueryKeyAccessPoint,
                                  &value)) {
    return signin_metrics::AccessPoint::ACCESS_POINT_UNKNOWN;
  }

  int access_point = -1;
  base::StringToInt(value, &access_point);
  if (access_point <
          static_cast<int>(
              signin_metrics::AccessPoint::ACCESS_POINT_START_PAGE) ||
      access_point >=
          static_cast<int>(signin_metrics::AccessPoint::ACCESS_POINT_MAX)) {
    return signin_metrics::AccessPoint::ACCESS_POINT_UNKNOWN;
  }

  return static_cast<signin_metrics::AccessPoint>(access_point);
}

signin_metrics::Reason GetSigninReasonForPromoURL(const GURL& url) {
  std::string value;
  if (!net::GetValueForKeyInQuery(url, kSignInPromoQueryKeyReason, &value))
    return signin_metrics::Reason::REASON_UNKNOWN_REASON;

  int reason = -1;
  base::StringToInt(value, &reason);
  if (reason < static_cast<int>(
                   signin_metrics::Reason::REASON_SIGNIN_PRIMARY_ACCOUNT) ||
      reason >= static_cast<int>(signin_metrics::Reason::REASON_MAX)) {
    return signin_metrics::Reason::REASON_UNKNOWN_REASON;
  }

  return static_cast<signin_metrics::Reason>(reason);
}

bool IsAutoCloseEnabledInURL(const GURL& url) {
  std::string value;
  if (net::GetValueForKeyInQuery(url, kSignInPromoQueryKeyAutoClose, &value)) {
    int enabled = 0;
    if (base::StringToInt(value, &enabled) && enabled == 1)
      return true;
  }
  return false;
}

bool ShouldShowAccountManagement(const GURL& url) {
  std::string value;
  if (net::GetValueForKeyInQuery(
          url, kSignInPromoQueryKeyShowAccountManagement, &value)) {
    int enabled = 0;
    if (base::StringToInt(value, &enabled) && enabled == 1)
      return IsAccountConsistencyMirrorEnabled();
  }
  return false;
}

void ForceWebBasedSigninFlowForTesting(bool force) {
  g_force_web_based_signin_flow = force;
}

void RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(prefs::kSignInPromoStartupCount, 0);
  registry->RegisterBooleanPref(prefs::kSignInPromoUserSkipped, false);
  registry->RegisterBooleanPref(prefs::kSignInPromoShowOnFirstRunAllowed, true);
  registry->RegisterBooleanPref(prefs::kSignInPromoShowNTPBubble, false);
  registry->RegisterIntegerPref(prefs::kDiceSigninUserMenuPromoCount, 0);
}

}  // namespace signin
