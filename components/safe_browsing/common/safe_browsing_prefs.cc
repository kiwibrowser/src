// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/common/safe_browsing_prefs.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/features.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/url_canon.h"

namespace {

// Name of the Scout Transition UMA metric.
const char kScoutTransitionMetricName[] = "SafeBrowsing.Pref.Scout.Transition";

// Reasons that a state transition for Scout was performed.
// These values are written to logs.  New enum values can be added, but
// existing enums must never be renumbered or deleted and reused.
enum ScoutTransitionReason {
  // Flag forced Scout Group to true
  FORCE_SCOUT_FLAG_TRUE = 0,
  // Flag forced Scout Group to false
  FORCE_SCOUT_FLAG_FALSE = 1,
  // User in OnlyShowScout group, enters Scout Group
  ONLY_SHOW_SCOUT_OPT_IN = 2,
  // User in CanShowScout group, enters Scout Group immediately
  CAN_SHOW_SCOUT_OPT_IN_SCOUT_GROUP_ON = 3,
  // User in CanShowScout group, waiting for interstitial to enter Scout Group
  CAN_SHOW_SCOUT_OPT_IN_WAIT_FOR_INTERSTITIAL = 4,
  // User in CanShowScout group saw the first interstitial and entered the Scout
  // Group
  CAN_SHOW_SCOUT_OPT_IN_SAW_FIRST_INTERSTITIAL = 5,
  // User in Control group
  CONTROL = 6,
  // Rollback: SBER2 on on implies SBER1 can turn on
  ROLLBACK_SBER2_IMPLIES_SBER1 = 7,
  // Rollback: SBER2 off so SBER1 must be turned off
  ROLLBACK_NO_SBER2_SET_SBER1_FALSE = 8,
  // Rollback: SBER2 absent so SBER1 must be cleared
  ROLLBACK_NO_SBER2_CLEAR_SBER1 = 9,
  // New reasons must be added BEFORE MAX_REASONS
  MAX_REASONS
};

// The Extended Reporting pref that is currently active, used for UMA metrics.
// These values are written to logs.  New enum values can be added, but
// existing enums must never be renumbered or deleted and reused.
enum ActiveExtendedReportingPref {
  SBER1_PREF = 0,
  SBER2_PREF = 1,
  // New prefs must be added before MAX_SBER_PREF
  MAX_SBER_PREF
};

// A histogram for tracking a nullable boolean, which can be false, true or
// null. These values are written to logs. New enum values can be added, but
// existing enums must never be renumbered or deleted and reused.
enum NullableBoolean {
  NULLABLE_BOOLEAN_FALSE = 0,
  NULLABLE_BOOLEAN_TRUE = 1,
  NULLABLE_BOOLEAN_NULL = 2,
  MAX_NULLABLE_BOOLEAN
};

NullableBoolean GetPrefValueOrNull(const PrefService& prefs,
                                   const std::string& pref_name) {
  if (!prefs.HasPrefPath(pref_name)) {
    return NULLABLE_BOOLEAN_NULL;
  }
  return prefs.GetBoolean(pref_name) ? NULLABLE_BOOLEAN_TRUE
                                     : NULLABLE_BOOLEAN_FALSE;
}

// Update the correct UMA metric based on which pref was changed and which UI
// the change was made on.
void RecordExtendedReportingPrefChanged(
    const PrefService& prefs,
    safe_browsing::ExtendedReportingOptInLocation location) {
  bool pref_value = safe_browsing::IsExtendedReportingEnabled(prefs);

  if (safe_browsing::IsScout(prefs)) {
    switch (location) {
      case safe_browsing::SBER_OPTIN_SITE_CHROME_SETTINGS:
        UMA_HISTOGRAM_BOOLEAN(
            "SafeBrowsing.Pref.Scout.SetPref.SBER2Pref.ChromeSettings",
            pref_value);
        break;
      case safe_browsing::SBER_OPTIN_SITE_ANDROID_SETTINGS:
        UMA_HISTOGRAM_BOOLEAN(
            "SafeBrowsing.Pref.Scout.SetPref.SBER2Pref.AndroidSettings",
            pref_value);
        break;
      case safe_browsing::SBER_OPTIN_SITE_DOWNLOAD_FEEDBACK_POPUP:
        UMA_HISTOGRAM_BOOLEAN(
            "SafeBrowsing.Pref.Scout.SetPref.SBER2Pref.DownloadPopup",
            pref_value);
        break;
      case safe_browsing::SBER_OPTIN_SITE_SECURITY_INTERSTITIAL:
        UMA_HISTOGRAM_BOOLEAN(
            "SafeBrowsing.Pref.Scout.SetPref.SBER2Pref.SecurityInterstitial",
            pref_value);
        break;
      default:
        NOTREACHED();
    }
  } else {
    switch (location) {
      case safe_browsing::SBER_OPTIN_SITE_CHROME_SETTINGS:
        UMA_HISTOGRAM_BOOLEAN(
            "SafeBrowsing.Pref.Scout.SetPref.SBER1Pref.ChromeSettings",
            pref_value);
        break;
      case safe_browsing::SBER_OPTIN_SITE_ANDROID_SETTINGS:
        UMA_HISTOGRAM_BOOLEAN(
            "SafeBrowsing.Pref.Scout.SetPref.SBER1Pref.AndroidSettings",
            pref_value);
        break;
      case safe_browsing::SBER_OPTIN_SITE_DOWNLOAD_FEEDBACK_POPUP:
        UMA_HISTOGRAM_BOOLEAN(
            "SafeBrowsing.Pref.Scout.SetPref.SBER1Pref.DownloadPopup",
            pref_value);
        break;
      case safe_browsing::SBER_OPTIN_SITE_SECURITY_INTERSTITIAL:
        UMA_HISTOGRAM_BOOLEAN(
            "SafeBrowsing.Pref.Scout.SetPref.SBER1Pref.SecurityInterstitial",
            pref_value);
        break;
      default:
        NOTREACHED();
    }
  }
}

// A helper function to return a GURL containing just the scheme, host, port,
// and path from a URL. Equivalent to clearing any username, password, query,
// and ref. Return empty URL if |url| is not valid.
GURL GetSimplifiedURL(const GURL& url) {
  if (!url.is_valid() || !url.IsStandard())
    return GURL();

  url::Replacements<char> replacements;
  replacements.ClearUsername();
  replacements.ClearPassword();
  replacements.ClearQuery();
  replacements.ClearRef();

  return url.ReplaceComponents(replacements);
}

}  // namespace

namespace prefs {
const char kSafeBrowsingEnabled[] = "safebrowsing.enabled";
const char kSafeBrowsingExtendedReportingEnabled[] =
    "safebrowsing.extended_reporting_enabled";
const char kSafeBrowsingExtendedReportingOptInAllowed[] =
    "safebrowsing.extended_reporting_opt_in_allowed";
const char kSafeBrowsingIncidentsSent[] = "safebrowsing.incidents_sent";
const char kSafeBrowsingProceedAnywayDisabled[] =
    "safebrowsing.proceed_anyway_disabled";
const char kSafeBrowsingSawInterstitialExtendedReporting[] =
    "safebrowsing.saw_interstitial_sber1";
const char kSafeBrowsingSawInterstitialScoutReporting[] =
    "safebrowsing.saw_interstitial_sber2";
const char kSafeBrowsingScoutGroupSelected[] =
    "safebrowsing.scout_group_selected";
const char kSafeBrowsingScoutReportingEnabled[] =
    "safebrowsing.scout_reporting_enabled";
const char kSafeBrowsingTriggerEventTimestamps[] =
    "safebrowsing.trigger_event_timestamps";
const char kSafeBrowsingUnhandledSyncPasswordReuses[] =
    "safebrowsing.unhandled_sync_password_reuses";
const char kSafeBrowsingWhitelistDomains[] =
    "safebrowsing.safe_browsing_whitelist_domains";
const char kPasswordProtectionChangePasswordURL[] =
    "safebrowsing.password_protection_change_password_url";
const char kPasswordProtectionLoginURLs[] =
    "safebrowsing.password_protection_login_urls";
const char kPasswordProtectionWarningTrigger[] =
    "safebrowsing.password_protection_warning_trigger";
}  // namespace prefs

namespace safe_browsing {

const base::Feature kCanShowScoutOptIn{"CanShowScoutOptIn",
                                       base::FEATURE_ENABLED_BY_DEFAULT};

std::string ChooseOptInTextPreference(
    const PrefService& prefs,
    const std::string& extended_reporting_pref,
    const std::string& scout_pref) {
  return IsScout(prefs) ? scout_pref : extended_reporting_pref;
}

int ChooseOptInTextResource(const PrefService& prefs,
                            int extended_reporting_resource,
                            int scout_resource) {
  return IsScout(prefs) ? scout_resource : extended_reporting_resource;
}

bool ExtendedReportingPrefExists(const PrefService& prefs) {
  return prefs.HasPrefPath(GetExtendedReportingPrefName(prefs));
}

ExtendedReportingLevel GetExtendedReportingLevel(const PrefService& prefs) {
  if (!IsExtendedReportingEnabled(prefs)) {
    return SBER_LEVEL_OFF;
  } else {
    return IsScout(prefs) ? SBER_LEVEL_SCOUT : SBER_LEVEL_LEGACY;
  }
}

const char* GetExtendedReportingPrefName(const PrefService& prefs) {
  // The Scout pref is active if the experiment features is on, and
  // ScoutGroupSelected is on as well.
  if (base::FeatureList::IsEnabled(kCanShowScoutOptIn) &&
      prefs.GetBoolean(prefs::kSafeBrowsingScoutGroupSelected)) {
    return prefs::kSafeBrowsingScoutReportingEnabled;
  }

  // ..otherwise, either no experiment is on (ie: the Control group) or
  // ScoutGroupSelected is off. So we use the SBER pref instead.
  return prefs::kSafeBrowsingExtendedReportingEnabled;
}

void InitializeSafeBrowsingPrefs(PrefService* prefs) {
  // Handle the two possible experiment states.
  if (base::FeatureList::IsEnabled(kCanShowScoutOptIn)) {
    // CanShowScoutOptIn will only turn on ScoutGroupSelected pref if the legacy
    // SBER pref is false. Otherwise the legacy SBER pref will stay on and
    // continue to be used until the next security incident, at which point
    // the Scout pref will become the active one.
    if (!prefs->GetBoolean(prefs::kSafeBrowsingExtendedReportingEnabled)) {
      prefs->SetBoolean(prefs::kSafeBrowsingScoutGroupSelected, true);
      UMA_HISTOGRAM_ENUMERATION(kScoutTransitionMetricName,
                                CAN_SHOW_SCOUT_OPT_IN_SCOUT_GROUP_ON,
                                MAX_REASONS);
    } else {
      UMA_HISTOGRAM_ENUMERATION(kScoutTransitionMetricName,
                                CAN_SHOW_SCOUT_OPT_IN_WAIT_FOR_INTERSTITIAL,
                                MAX_REASONS);
    }
  } else {
    // Experiment feature is off, so this is the Control group. We must
    // handle the possibility that the user was previously in an experiment
    // group (above) that was reverted. We want to restore the user to a
    // reasonable state based on the ScoutGroup and ScoutReporting preferences.
    UMA_HISTOGRAM_ENUMERATION(kScoutTransitionMetricName, CONTROL, MAX_REASONS);
    bool transitioned = false;
    if (prefs->GetBoolean(prefs::kSafeBrowsingScoutReportingEnabled)) {
      // User opted-in to Scout which is broader than legacy Extended Reporting.
      // Opt them in to Extended Reporting.
      prefs->SetBoolean(prefs::kSafeBrowsingExtendedReportingEnabled, true);
      UMA_HISTOGRAM_ENUMERATION(kScoutTransitionMetricName,
                                ROLLBACK_SBER2_IMPLIES_SBER1, MAX_REASONS);
      transitioned = true;
    } else if (prefs->GetBoolean(prefs::kSafeBrowsingScoutGroupSelected)) {
      // User was in the Scout Group (ie: seeing the Scout opt-in text) but did
      // NOT opt-in to Scout. Assume this was a conscious choice and remove
      // their legacy Extended Reporting opt-in as well. The user will have a
      // chance to evaluate their choice next time they see the opt-in text.

      // We make the Extended Reporting pref mimic the state of the Scout
      // Reporting pref. So we either Clear it or set it to False.
      if (prefs->HasPrefPath(prefs::kSafeBrowsingScoutReportingEnabled)) {
        // Scout Reporting pref was explicitly set to false, so set the SBER
        // pref to false.
        prefs->SetBoolean(prefs::kSafeBrowsingExtendedReportingEnabled, false);
        UMA_HISTOGRAM_ENUMERATION(kScoutTransitionMetricName,
                                  ROLLBACK_NO_SBER2_SET_SBER1_FALSE,
                                  MAX_REASONS);
      } else {
        // Scout Reporting pref is unset, so clear the SBER pref.
        prefs->ClearPref(prefs::kSafeBrowsingExtendedReportingEnabled);
        UMA_HISTOGRAM_ENUMERATION(kScoutTransitionMetricName,
                                  ROLLBACK_NO_SBER2_CLEAR_SBER1, MAX_REASONS);
      }
      transitioned = true;
    }

    // Also clear both the Scout settings to start over from a clean state and
    // avoid the above logic from triggering on next restart.
    prefs->ClearPref(prefs::kSafeBrowsingScoutGroupSelected);
    prefs->ClearPref(prefs::kSafeBrowsingScoutReportingEnabled);

    // Also forget that the user has seen any interstitials if they're
    // reverting back to a clean state.
    if (transitioned) {
      prefs->ClearPref(prefs::kSafeBrowsingSawInterstitialExtendedReporting);
      prefs->ClearPref(prefs::kSafeBrowsingSawInterstitialScoutReporting);
    }
  }
}

bool IsExtendedReportingOptInAllowed(const PrefService& prefs) {
  return prefs.GetBoolean(prefs::kSafeBrowsingExtendedReportingOptInAllowed);
}

bool IsExtendedReportingEnabled(const PrefService& prefs) {
  return prefs.GetBoolean(GetExtendedReportingPrefName(prefs));
}

bool IsExtendedReportingPolicyManaged(const PrefService& prefs) {
  return prefs.IsManagedPreference(GetExtendedReportingPrefName(prefs));
}

bool IsScout(const PrefService& prefs) {
  return GetExtendedReportingPrefName(prefs) ==
         prefs::kSafeBrowsingScoutReportingEnabled;
}

void RecordExtendedReportingMetrics(const PrefService& prefs) {
  // This metric tracks the extended browsing opt-in based on whichever setting
  // the user is currently seeing. It tells us whether extended reporting is
  // happening for this user.
  UMA_HISTOGRAM_BOOLEAN("SafeBrowsing.Pref.Extended",
                        IsExtendedReportingEnabled(prefs));

  // Track whether this user has ever seen a security interstitial.
  UMA_HISTOGRAM_BOOLEAN(
      "SafeBrowsing.Pref.SawInterstitial.SBER1Pref",
      prefs.GetBoolean(prefs::kSafeBrowsingSawInterstitialExtendedReporting));
  UMA_HISTOGRAM_BOOLEAN(
      "SafeBrowsing.Pref.SawInterstitial.SBER2Pref",
      prefs.GetBoolean(prefs::kSafeBrowsingSawInterstitialScoutReporting));

  // These metrics track the Scout transition.
  if (prefs.GetBoolean(prefs::kSafeBrowsingScoutGroupSelected)) {
    // Users in the Scout group: currently seeing the Scout opt-in.
    UMA_HISTOGRAM_ENUMERATION(
        "SafeBrowsing.Pref.Scout.ScoutGroup.SBER1Pref",
        GetPrefValueOrNull(prefs, prefs::kSafeBrowsingExtendedReportingEnabled),
        MAX_NULLABLE_BOOLEAN);
    UMA_HISTOGRAM_ENUMERATION(
        "SafeBrowsing.Pref.Scout.ScoutGroup.SBER2Pref",
        GetPrefValueOrNull(prefs, prefs::kSafeBrowsingScoutReportingEnabled),
        MAX_NULLABLE_BOOLEAN);
  } else {
    // Users not in the Scout group: currently seeing the SBER opt-in.
    UMA_HISTOGRAM_ENUMERATION(
        "SafeBrowsing.Pref.Scout.NoScoutGroup.SBER1Pref",
        GetPrefValueOrNull(prefs, prefs::kSafeBrowsingExtendedReportingEnabled),
        MAX_NULLABLE_BOOLEAN);
    // The following metric is a corner case. User was previously in the
    // Scout group and was able to opt-in to the Scout pref, but was since
    // removed from the Scout group (eg: by rolling back a Scout experiment).
    UMA_HISTOGRAM_ENUMERATION(
        "SafeBrowsing.Pref.Scout.NoScoutGroup.SBER2Pref",
        GetPrefValueOrNull(prefs, prefs::kSafeBrowsingScoutReportingEnabled),
        MAX_NULLABLE_BOOLEAN);
  }
}

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kSafeBrowsingExtendedReportingEnabled,
                                false);
  registry->RegisterBooleanPref(prefs::kSafeBrowsingScoutReportingEnabled,
                                false);
  registry->RegisterBooleanPref(prefs::kSafeBrowsingScoutGroupSelected, false);
  registry->RegisterBooleanPref(
      prefs::kSafeBrowsingSawInterstitialExtendedReporting, false);
  registry->RegisterBooleanPref(
      prefs::kSafeBrowsingSawInterstitialScoutReporting, false);
  registry->RegisterBooleanPref(
      prefs::kSafeBrowsingExtendedReportingOptInAllowed, true);
  registry->RegisterBooleanPref(
      prefs::kSafeBrowsingEnabled, true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(prefs::kSafeBrowsingProceedAnywayDisabled,
                                false);
  registry->RegisterDictionaryPref(prefs::kSafeBrowsingIncidentsSent);
  registry->RegisterDictionaryPref(
      prefs::kSafeBrowsingUnhandledSyncPasswordReuses);
  registry->RegisterListPref(prefs::kSafeBrowsingWhitelistDomains);
  registry->RegisterStringPref(prefs::kPasswordProtectionChangePasswordURL, "");
  registry->RegisterListPref(prefs::kPasswordProtectionLoginURLs);
  registry->RegisterIntegerPref(prefs::kPasswordProtectionWarningTrigger,
                                PASSWORD_PROTECTION_OFF);
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(prefs::kSafeBrowsingTriggerEventTimestamps);
}

void SetExtendedReportingPrefAndMetric(
    PrefService* prefs,
    bool value,
    ExtendedReportingOptInLocation location) {
  prefs->SetBoolean(GetExtendedReportingPrefName(*prefs), value);
  RecordExtendedReportingPrefChanged(*prefs, location);
}

void SetExtendedReportingPref(PrefService* prefs, bool value) {
  prefs->SetBoolean(GetExtendedReportingPrefName(*prefs), value);
}

void UpdateMetricsAfterSecurityInterstitial(const PrefService& prefs,
                                            bool on_show_pref_existed,
                                            bool on_show_pref_value) {
  const ActiveExtendedReportingPref active_pref =
      IsScout(prefs) ? SBER2_PREF : SBER1_PREF;
  const bool cur_pref_value = IsExtendedReportingEnabled(prefs);

  if (!on_show_pref_existed) {
    if (!ExtendedReportingPrefExists(prefs)) {
      // User seeing pref for the first time, didn't touch the checkbox (left it
      // unchecked).
      UMA_HISTOGRAM_ENUMERATION(
          "SafeBrowsing.Pref.Scout.Decision.First_LeftUnchecked", active_pref,
          MAX_SBER_PREF);
      return;
    }

    // Pref currently exists so user did something to the checkbox
    if (cur_pref_value) {
      // User turned the pref on.
      UMA_HISTOGRAM_ENUMERATION(
          "SafeBrowsing.Pref.Scout.Decision.First_Enabled", active_pref,
          MAX_SBER_PREF);
      return;
    }

    // Otherwise, user turned the pref off, but because it didn't exist when
    // the interstitial was first shown, they must have turned it on and then
    // off before the interstitial was closed.
    UMA_HISTOGRAM_ENUMERATION("SafeBrowsing.Pref.Scout.Decision.First_Disabled",
                              active_pref, MAX_SBER_PREF);
    return;
  }

  // At this point, the pref existed when the interstitial was shown so this is
  // a repeat appearance of the opt-in. Existence can't be removed during an
  // interstitial so no need to check whether the pref currently exists.
  if (on_show_pref_value && cur_pref_value) {
    // User left the pref on.
    UMA_HISTOGRAM_ENUMERATION(
        "SafeBrowsing.Pref.Scout.Decision.Repeat_LeftEnabled", active_pref,
        MAX_SBER_PREF);
    return;
  } else if (on_show_pref_value && !cur_pref_value) {
    // User turned the pref off.
    UMA_HISTOGRAM_ENUMERATION(
        "SafeBrowsing.Pref.Scout.Decision.Repeat_Disabled", active_pref,
        MAX_SBER_PREF);
    return;
  } else if (!on_show_pref_value && cur_pref_value) {
    // User turned the pref on.
    UMA_HISTOGRAM_ENUMERATION("SafeBrowsing.Pref.Scout.Decision.Repeat_Enabled",
                              active_pref, MAX_SBER_PREF);
    return;
  } else {
    // Both on_show and cur values are false - user left the pref off.
    UMA_HISTOGRAM_ENUMERATION(
        "SafeBrowsing.Pref.Scout.Decision.Repeat_LeftDisabled", active_pref,
        MAX_SBER_PREF);
    return;
  }
}

void UpdatePrefsBeforeSecurityInterstitial(PrefService* prefs) {
  // Move the user into the Scout Group if the CanShowScoutOptIn experiment is
  // enabled and they're not in the group already.
  if (base::FeatureList::IsEnabled(kCanShowScoutOptIn) &&
      !prefs->GetBoolean(prefs::kSafeBrowsingScoutGroupSelected)) {
    prefs->SetBoolean(prefs::kSafeBrowsingScoutGroupSelected, true);
    UMA_HISTOGRAM_ENUMERATION(kScoutTransitionMetricName,
                              CAN_SHOW_SCOUT_OPT_IN_SAW_FIRST_INTERSTITIAL,
                              MAX_REASONS);
  }

  // Remember that this user saw an interstitial with the current opt-in text.
  prefs->SetBoolean(IsScout(*prefs)
                        ? prefs::kSafeBrowsingSawInterstitialScoutReporting
                        : prefs::kSafeBrowsingSawInterstitialExtendedReporting,
                    true);
}

base::ListValue GetSafeBrowsingPreferencesList(PrefService* prefs) {
  base::ListValue preferences_list;

  const char* safe_browsing_preferences[] = {
      prefs::kSafeBrowsingEnabled,
      prefs::kSafeBrowsingExtendedReportingOptInAllowed,
      prefs::kSafeBrowsingExtendedReportingEnabled,
      prefs::kSafeBrowsingScoutReportingEnabled};

  // Add the status of the preferences if they are Enabled or Disabled for the
  // user.
  for (const char* preference : safe_browsing_preferences) {
    preferences_list.GetList().push_back(base::Value(preference));
    bool enabled = prefs->GetBoolean(preference);
    preferences_list.GetList().push_back(
        base::Value(enabled ? "Enabled" : "Disabled"));
  }
  return preferences_list;
}

void GetSafeBrowsingWhitelistDomainsPref(
    const PrefService& prefs,
    std::vector<std::string>* out_canonicalized_domain_list) {
  if (base::FeatureList::IsEnabled(kEnterprisePasswordProtectionV1)) {
    const base::ListValue* pref_value =
        prefs.GetList(prefs::kSafeBrowsingWhitelistDomains);
    CanonicalizeDomainList(*pref_value, out_canonicalized_domain_list);
  }
}

void CanonicalizeDomainList(
    const base::ListValue& raw_domain_list,
    std::vector<std::string>* out_canonicalized_domain_list) {
  out_canonicalized_domain_list->clear();
  for (auto it = raw_domain_list.GetList().begin();
       it != raw_domain_list.GetList().end(); it++) {
    // Verify if it is valid domain string.
    url::CanonHostInfo host_info;
    std::string canonical_host =
        net::CanonicalizeHost(it->GetString(), &host_info);
    if (!canonical_host.empty())
      out_canonicalized_domain_list->push_back(canonical_host);
  }
}

bool IsURLWhitelistedByPolicy(const GURL& url,
                              StringListPrefMember* pref_member) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!pref_member)
    return false;

  std::vector<std::string> sb_whitelist_domains = pref_member->GetValue();
  return std::find_if(sb_whitelist_domains.begin(), sb_whitelist_domains.end(),
                      [&url](const std::string& domain) {
                        return url.DomainIs(domain);
                      }) != sb_whitelist_domains.end();
}

bool IsURLWhitelistedByPolicy(const GURL& url, const PrefService& pref) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!pref.HasPrefPath(prefs::kSafeBrowsingWhitelistDomains))
    return false;
  const base::ListValue* whitelist =
      pref.GetList(prefs::kSafeBrowsingWhitelistDomains);
  for (const base::Value& value : whitelist->GetList()) {
    if (url.DomainIs(value.GetString()))
      return true;
  }
  return false;
}

void GetPasswordProtectionLoginURLsPref(const PrefService& prefs,
                                        std::vector<GURL>* out_login_url_list) {
  const base::ListValue* pref_value =
      prefs.GetList(prefs::kPasswordProtectionLoginURLs);
  out_login_url_list->clear();
  for (const base::Value& value : pref_value->GetList()) {
    GURL login_url(value.GetString());
    // Skip invalid or none-http/https login URLs.
    if (login_url.is_valid() && login_url.SchemeIsHTTPOrHTTPS())
      out_login_url_list->push_back(login_url);
  }
}

bool MatchesPasswordProtectionLoginURL(const GURL& url,
                                       const PrefService& prefs) {
  if (!base::FeatureList::IsEnabled(kEnterprisePasswordProtectionV1) ||
      !url.is_valid()) {
    return false;
  }

  std::vector<GURL> login_urls;
  GetPasswordProtectionLoginURLsPref(prefs, &login_urls);
  if (login_urls.empty())
    return false;

  GURL simple_url = GetSimplifiedURL(url);
  for (const GURL& login_url : login_urls) {
    if (GetSimplifiedURL(login_url) == simple_url) {
      return true;
    }
  }
  return false;
}

GURL GetPasswordProtectionChangePasswordURLPref(const PrefService& prefs) {
  if (!prefs.HasPrefPath(prefs::kPasswordProtectionChangePasswordURL))
    return GURL();
  GURL change_password_url_from_pref(
      prefs.GetString(prefs::kPasswordProtectionChangePasswordURL));
  // Skip invalid or non-http/https URL.
  if (change_password_url_from_pref.is_valid() &&
      change_password_url_from_pref.SchemeIsHTTPOrHTTPS()) {
    return change_password_url_from_pref;
  }

  return GURL();
}

bool MatchesPasswordProtectionChangePasswordURL(const GURL& url,
                                                const PrefService& prefs) {
  if (!base::FeatureList::IsEnabled(kEnterprisePasswordProtectionV1) ||
      !url.is_valid()) {
    return false;
  }

  GURL change_password_url = GetPasswordProtectionChangePasswordURLPref(prefs);
  if (change_password_url.is_empty())
    return false;

  return GetSimplifiedURL(change_password_url) == GetSimplifiedURL(url);
}

}  // namespace safe_browsing
