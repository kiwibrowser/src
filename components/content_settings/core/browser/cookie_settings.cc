// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/content_settings/core/browser/cookie_settings.h"

#include "base/bind.h"
#include "base/logging.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "extensions/buildflags/buildflags.h"
#include "net/base/net_errors.h"
#include "net/base/static_cookie_policy.h"
#include "net/cookies/cookie_util.h"
#include "url/gurl.h"

namespace {

bool IsValidSetting(ContentSetting setting) {
  return (setting == CONTENT_SETTING_ALLOW ||
          setting == CONTENT_SETTING_SESSION_ONLY ||
          setting == CONTENT_SETTING_BLOCK);
}

bool IsAllowed(ContentSetting setting) {
  DCHECK(IsValidSetting(setting));
  return (setting == CONTENT_SETTING_ALLOW ||
          setting == CONTENT_SETTING_SESSION_ONLY);
}

}  // namespace

namespace content_settings {

CookieSettings::CookieSettings(
    HostContentSettingsMap* host_content_settings_map,
    PrefService* prefs,
    const char* extension_scheme)
    : host_content_settings_map_(host_content_settings_map),
      extension_scheme_(extension_scheme),
      block_third_party_cookies_(
          prefs->GetBoolean(prefs::kBlockThirdPartyCookies)) {
  pref_change_registrar_.Init(prefs);
  pref_change_registrar_.Add(
      prefs::kBlockThirdPartyCookies,
      base::Bind(&CookieSettings::OnBlockThirdPartyCookiesChanged,
                 base::Unretained(this)));
}

ContentSetting CookieSettings::GetDefaultCookieSetting(
    std::string* provider_id) const {
  return host_content_settings_map_->GetDefaultContentSetting(
      CONTENT_SETTINGS_TYPE_COOKIES, provider_id);
}

bool CookieSettings::IsCookieAccessAllowed(const GURL& url,
                                           const GURL& first_party_url) const {
  ContentSetting setting;
  GetCookieSetting(url, first_party_url, nullptr, &setting);
  return IsAllowed(setting);
}

bool CookieSettings::IsCookieSessionOnly(const GURL& origin) const {
  ContentSetting setting;
  GetCookieSetting(origin, origin, nullptr, &setting);
  DCHECK(IsValidSetting(setting));
  return (setting == CONTENT_SETTING_SESSION_ONLY);
}

bool CookieSettings::ShouldDeleteCookieOnExit(
    const ContentSettingsForOneType& cookie_settings,
    const std::string& domain,
    bool is_https) const {
  GURL origin = net::cookie_util::CookieOriginToURL(domain, is_https);
  ContentSetting setting;
  GetCookieSetting(origin, origin, nullptr, &setting);
  DCHECK(IsValidSetting(setting));
  if (setting == CONTENT_SETTING_ALLOW)
    return false;
  // Non-secure cookies are readable by secure sites. We need to check for
  // https pattern if http is not allowed. The section below is independent
  // of the scheme so we can just retry from here.
  if (!is_https)
    return ShouldDeleteCookieOnExit(cookie_settings, domain, true);
  // Check if there is a more precise rule that "domain matches" this cookie.
  bool matches_session_only_rule = false;
  for (const auto& entry : cookie_settings) {
    const std::string& host = entry.primary_pattern.GetHost();
    if (net::cookie_util::IsDomainMatch(domain, host)) {
      if (entry.GetContentSetting() == CONTENT_SETTING_ALLOW) {
        return false;
      } else if (entry.GetContentSetting() == CONTENT_SETTING_SESSION_ONLY) {
        matches_session_only_rule = true;
      }
    }
  }
  return setting == CONTENT_SETTING_SESSION_ONLY || matches_session_only_rule;
}

void CookieSettings::GetCookieSettings(
    ContentSettingsForOneType* settings) const {
  host_content_settings_map_->GetSettingsForOneType(
      CONTENT_SETTINGS_TYPE_COOKIES, std::string(), settings);
}

void CookieSettings::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(
      prefs::kBlockThirdPartyCookies, false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
}

void CookieSettings::SetDefaultCookieSetting(ContentSetting setting) {
  DCHECK(IsValidSetting(setting));
  host_content_settings_map_->SetDefaultContentSetting(
      CONTENT_SETTINGS_TYPE_COOKIES, setting);
}

void CookieSettings::SetCookieSetting(const GURL& primary_url,
                                      ContentSetting setting) {
  DCHECK(IsValidSetting(setting));
  host_content_settings_map_->SetContentSettingDefaultScope(
      primary_url, GURL(), CONTENT_SETTINGS_TYPE_COOKIES, std::string(),
      setting);
}

void CookieSettings::ResetCookieSetting(const GURL& primary_url) {
  host_content_settings_map_->SetNarrowestContentSetting(
      primary_url, GURL(), CONTENT_SETTINGS_TYPE_COOKIES,
      CONTENT_SETTING_DEFAULT);
}

bool CookieSettings::IsStorageDurable(const GURL& origin) const {
  // TODO(dgrogan): Don't use host_content_settings_map_ directly.
  // https://crbug.com/539538
  ContentSetting setting = host_content_settings_map_->GetContentSetting(
      origin /*primary*/, origin /*secondary*/,
      CONTENT_SETTINGS_TYPE_DURABLE_STORAGE,
      std::string() /*resource_identifier*/);
  return setting == CONTENT_SETTING_ALLOW;
}

void CookieSettings::ShutdownOnUIThread() {
  DCHECK(thread_checker_.CalledOnValidThread());
  pref_change_registrar_.RemoveAll();
}

void CookieSettings::GetCookieSetting(const GURL& url,
                                      const GURL& first_party_url,
                                      content_settings::SettingSource* source,
                                      ContentSetting* cookie_setting) const {
  DCHECK(cookie_setting);
  // Auto-allow in extensions or for WebUI embedded in a secure origin.
  if (first_party_url.SchemeIs(kChromeUIScheme) &&
      url.SchemeIsCryptographic()) {
    *cookie_setting = CONTENT_SETTING_ALLOW;
    return;
  }

#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (url.SchemeIs(extension_scheme_) &&
      first_party_url.SchemeIs(extension_scheme_)) {
    *cookie_setting = CONTENT_SETTING_ALLOW;
    return;
  }
#endif

  // First get any host-specific settings.
  SettingInfo info;
  std::unique_ptr<base::Value> value =
      host_content_settings_map_->GetWebsiteSetting(
          url, first_party_url, CONTENT_SETTINGS_TYPE_COOKIES, std::string(),
          &info);
  if (source)
    *source = info.source;

  // If no explicit exception has been made and third-party cookies are blocked
  // by default, apply CONTENT_SETTING_BLOCKED.
  bool block_third = info.primary_pattern.MatchesAllHosts() &&
                     info.secondary_pattern.MatchesAllHosts() &&
                     ShouldBlockThirdPartyCookies() &&
                     !first_party_url.SchemeIs(extension_scheme_);
  net::StaticCookiePolicy policy(
      net::StaticCookiePolicy::BLOCK_ALL_THIRD_PARTY_COOKIES);

  // We should always have a value, at least from the default provider.
  DCHECK(value);
  ContentSetting setting = ValueToContentSetting(value.get());
  bool block =
      block_third && policy.CanAccessCookies(url, first_party_url) != net::OK;
  *cookie_setting = block ? CONTENT_SETTING_BLOCK : setting;
}

CookieSettings::~CookieSettings() {
}

void CookieSettings::OnBlockThirdPartyCookiesChanged() {
  DCHECK(thread_checker_.CalledOnValidThread());

  base::AutoLock auto_lock(lock_);
  block_third_party_cookies_ = pref_change_registrar_.prefs()->GetBoolean(
      prefs::kBlockThirdPartyCookies);
}

bool CookieSettings::ShouldBlockThirdPartyCookies() const {
  base::AutoLock auto_lock(lock_);
  return block_third_party_cookies_;
}

}  // namespace content_settings
