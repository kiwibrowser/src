// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/chrome_connected_header_helper.h"

#include <vector>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "components/google/core/browser/google_util.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

namespace signin {

namespace {

const char kContinueUrlAttrName[] = "continue_url";
const char kEmailAttrName[] = "email";
const char kEnableAccountConsistencyAttrName[] = "enable_account_consistency";
const char kGaiaIdAttrName[] = "id";
const char kIsSameTabAttrName[] = "is_same_tab";
const char kIsSamlAttrName[] = "is_saml";
const char kProfileModeAttrName[] = "mode";
const char kServiceTypeAttrName[] = "action";

// Determines the service type that has been passed from Gaia in the header.
GAIAServiceType GetGAIAServiceTypeFromHeader(const std::string& header_value) {
  if (header_value == "SIGNOUT")
    return GAIA_SERVICE_TYPE_SIGNOUT;
  else if (header_value == "INCOGNITO")
    return GAIA_SERVICE_TYPE_INCOGNITO;
  else if (header_value == "ADDSESSION")
    return GAIA_SERVICE_TYPE_ADDSESSION;
  else if (header_value == "REAUTH")
    return GAIA_SERVICE_TYPE_REAUTH;
  else if (header_value == "SIGNUP")
    return GAIA_SERVICE_TYPE_SIGNUP;
  else if (header_value == "DEFAULT")
    return GAIA_SERVICE_TYPE_DEFAULT;
  else
    return GAIA_SERVICE_TYPE_NONE;
}

}  // namespace

ChromeConnectedHeaderHelper::ChromeConnectedHeaderHelper(
    AccountConsistencyMethod account_consistency)
    : account_consistency_(account_consistency) {}

// static
std::string ChromeConnectedHeaderHelper::BuildRequestCookieIfPossible(
    const GURL& url,
    const std::string& account_id,
    AccountConsistencyMethod account_consistency,
    const content_settings::CookieSettings* cookie_settings,
    int profile_mode_mask) {
  ChromeConnectedHeaderHelper chrome_connected_helper(account_consistency);
  if (!chrome_connected_helper.ShouldBuildRequestHeader(url, cookie_settings))
    return "";
  return chrome_connected_helper.BuildRequestHeader(
      false /* is_header_request */, url, account_id, profile_mode_mask);
}

// static
ManageAccountsParams ChromeConnectedHeaderHelper::BuildManageAccountsParams(
    const std::string& header_value) {
  DCHECK(!header_value.empty());
  ManageAccountsParams params;
  ResponseHeaderDictionary header_dictionary =
      ParseAccountConsistencyResponseHeader(header_value);
  ResponseHeaderDictionary::const_iterator it = header_dictionary.begin();
  for (; it != header_dictionary.end(); ++it) {
    const std::string key_name(it->first);
    const std::string value(it->second);
    if (key_name == kServiceTypeAttrName) {
      params.service_type = GetGAIAServiceTypeFromHeader(value);
    } else if (key_name == kEmailAttrName) {
      params.email = value;
    } else if (key_name == kIsSamlAttrName) {
      params.is_saml = value == "true";
    } else if (key_name == kContinueUrlAttrName) {
      params.continue_url = value;
    } else if (key_name == kIsSameTabAttrName) {
      params.is_same_tab = value == "true";
    } else {
      DLOG(WARNING) << "Unexpected Gaia header attribute '" << key_name << "'.";
    }
  }
  return params;
}

bool ChromeConnectedHeaderHelper::IsUrlEligibleToIncludeGaiaId(
    const GURL& url,
    bool is_header_request) {
  if (is_header_request) {
    // Gaia ID is only necessary for Drive. Don't set it otherwise.
    return IsDriveOrigin(url.GetOrigin());
  }

  // Cookie requests don't have the granularity to only include the Gaia ID for
  // Drive origin. Set it on all google.com instead.
  if (!url.SchemeIsCryptographic())
    return false;

  const std::string kGoogleDomain = "google.com";
  std::string domain = net::registry_controlled_domains::GetDomainAndRegistry(
      url, net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES);
  return domain == kGoogleDomain;
}

bool ChromeConnectedHeaderHelper::IsDriveOrigin(const GURL& url) {
  if (!url.SchemeIsCryptographic())
    return false;

  const GURL kGoogleDriveURL("https://drive.google.com");
  const GURL kGoogleDocsURL("https://docs.google.com");
  return url == kGoogleDriveURL || url == kGoogleDocsURL;
}

bool ChromeConnectedHeaderHelper::IsUrlEligibleForRequestHeader(
    const GURL& url) {
  // Only set the header for Drive and Gaia always, and other Google properties
  // if account consistency is enabled. Vasquette, which is integrated with most
  // Google properties, needs the header to redirect certain user actions to
  // Chrome native UI. Drive and Gaia need the header to tell if the current
  // user is connected.

  // Consider the account ID sensitive and limit it to secure domains.
  if (!url.SchemeIsCryptographic())
    return false;

  GURL origin(url.GetOrigin());
  bool is_google_url =
      google_util::IsGoogleDomainUrl(
          url, google_util::ALLOW_SUBDOMAIN,
          google_util::DISALLOW_NON_STANDARD_PORTS) ||
      google_util::IsYoutubeDomainUrl(url, google_util::ALLOW_SUBDOMAIN,
                                      google_util::DISALLOW_NON_STANDARD_PORTS);
  bool is_mirror_enabled =
      account_consistency_ == AccountConsistencyMethod::kMirror;
  return (is_mirror_enabled && is_google_url) || IsDriveOrigin(origin) ||
         gaia::IsGaiaSignonRealm(origin);
}

std::string ChromeConnectedHeaderHelper::BuildRequestHeader(
    bool is_header_request,
    const GURL& url,
    const std::string& account_id,
    int profile_mode_mask) {
// If we are not on Chrome OS, an empty |account_id| corresponds to the user not
// signed in to Chrome. Do NOT enforce account consistency otherwise users will
// not be able to use Google services at all. Therefore, send an empty header.
// On Chrome OS, an empty |account_id| corresponds to Public Sessions, Guest
// Sessions and Active Directory logins. Guest Sessions have already been
// filtered upstream and we want to enforce account consistency in Public
// Sessions and Active Directory logins.
#if !defined(OS_CHROMEOS)
  if (account_id.empty())
    return std::string();
#endif

  std::vector<std::string> parts;
  if (!account_id.empty() &&
      IsUrlEligibleToIncludeGaiaId(url, is_header_request)) {
    // Only set the Gaia ID on domains that actually require it.
    parts.push_back(
        base::StringPrintf("%s=%s", kGaiaIdAttrName, account_id.c_str()));
  }
  parts.push_back(
      base::StringPrintf("%s=%s", kProfileModeAttrName,
                         base::IntToString(profile_mode_mask).c_str()));
  bool is_mirror_enabled =
      account_consistency_ == AccountConsistencyMethod::kMirror;
  parts.push_back(base::StringPrintf("%s=%s", kEnableAccountConsistencyAttrName,
                                     is_mirror_enabled ? "true" : "false"));

  return base::JoinString(parts, is_header_request ? "," : ":");
}

}  // namespace signin
