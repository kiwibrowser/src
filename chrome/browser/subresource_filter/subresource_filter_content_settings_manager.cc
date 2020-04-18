// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/subresource_filter/subresource_filter_content_settings_manager.h"

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/time/default_clock.h"
#include "base/values.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/subresource_filter/chrome_subresource_filter_client.h"
#include "components/content_settings/core/browser/content_settings_details.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/subresource_filter/core/browser/subresource_filter_features.h"
#include "url/gurl.h"

namespace {

// Key into the website setting dict for the smart UI.
const char kInfobarLastShownTimeKey[] = "InfobarLastShownTime";

bool ShouldUseSmartUI() {
#if defined(OS_ANDROID)
  return base::FeatureList::IsEnabled(
      subresource_filter::kSafeBrowsingSubresourceFilterExperimentalUI);
#endif
  return false;
}

}  // namespace

constexpr base::TimeDelta
    SubresourceFilterContentSettingsManager::kDelayBeforeShowingInfobarAgain;

SubresourceFilterContentSettingsManager::
    SubresourceFilterContentSettingsManager(Profile* profile)
    : history_observer_(this),
      settings_map_(HostContentSettingsMapFactory::GetForProfile(profile)),
      clock_(std::make_unique<base::DefaultClock>(base::DefaultClock())),
      should_use_smart_ui_(ShouldUseSmartUI()) {
  DCHECK(profile);
  DCHECK(settings_map_);
  settings_map_->AddObserver(this);

  if (auto* history_service = HistoryServiceFactory::GetForProfile(
          profile, ServiceAccessType::EXPLICIT_ACCESS)) {
    history_observer_.Add(history_service);
  }

  cached_global_setting_for_metrics_ = settings_map_->GetDefaultContentSetting(
      CONTENT_SETTINGS_TYPE_ADS, nullptr);
}

SubresourceFilterContentSettingsManager::
    ~SubresourceFilterContentSettingsManager() {
  settings_map_->RemoveObserver(this);
  settings_map_ = nullptr;
  history_observer_.RemoveAll();
}

ContentSetting SubresourceFilterContentSettingsManager::GetSitePermission(
    const GURL& url) const {
  return settings_map_->GetContentSetting(
      url, GURL(), ContentSettingsType::CONTENT_SETTINGS_TYPE_ADS,
      std::string());
}

void SubresourceFilterContentSettingsManager::WhitelistSite(const GURL& url) {
  DCHECK(base::FeatureList::IsEnabled(
      subresource_filter::kSafeBrowsingSubresourceFilterExperimentalUI));
  base::AutoReset<bool> resetter(&ignore_settings_changes_, true);
  settings_map_->SetContentSettingDefaultScope(
      url, GURL(), ContentSettingsType::CONTENT_SETTINGS_TYPE_ADS,
      std::string(), CONTENT_SETTING_ALLOW);
  ChromeSubresourceFilterClient::LogAction(kActionContentSettingsAllowedFromUI);
}

void SubresourceFilterContentSettingsManager::OnDidShowUI(const GURL& url) {
  auto dict = std::make_unique<base::DictionaryValue>();
  double now = clock_->Now().ToDoubleT();
  dict->SetDouble(kInfobarLastShownTimeKey, now);
  SetSiteMetadata(url, std::move(dict));
}

bool SubresourceFilterContentSettingsManager::ShouldShowUIForSite(
    const GURL& url) const {
  if (!should_use_smart_ui())
    return true;

  std::unique_ptr<base::DictionaryValue> dict = GetSiteMetadata(url);
  if (!dict)
    return true;

  double last_shown_time_double = 0;
  if (dict->GetDouble(kInfobarLastShownTimeKey, &last_shown_time_double)) {
    base::Time last_shown = base::Time::FromDoubleT(last_shown_time_double);
    if (clock_->Now() - last_shown < kDelayBeforeShowingInfobarAgain)
      return false;
  }
  return true;
}

void SubresourceFilterContentSettingsManager::
    ResetSiteMetadataBasedOnActivation(const GURL& url, bool is_activated) {
  if (!is_activated) {
    SetSiteMetadata(url, nullptr);
  } else if (!GetSiteMetadata(url)) {
    SetSiteMetadata(url, std::make_unique<base::DictionaryValue>());
  }
}

std::unique_ptr<base::DictionaryValue>
SubresourceFilterContentSettingsManager::GetSiteMetadata(
    const GURL& url) const {
  return base::DictionaryValue::From(settings_map_->GetWebsiteSetting(
      url, GURL(), CONTENT_SETTINGS_TYPE_ADS_DATA, std::string(), nullptr));
}

void SubresourceFilterContentSettingsManager::SetSiteMetadata(
    const GURL& url,
    std::unique_ptr<base::DictionaryValue> dict) {
  if (!base::FeatureList::IsEnabled(
          subresource_filter::kSafeBrowsingSubresourceFilterExperimentalUI))
    return;
  settings_map_->SetWebsiteSettingDefaultScope(
      url, GURL(), ContentSettingsType::CONTENT_SETTINGS_TYPE_ADS_DATA,
      std::string(), std::move(dict));
}

void SubresourceFilterContentSettingsManager::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    std::string resource_identifier) {
  if (content_type != CONTENT_SETTINGS_TYPE_ADS || ignore_settings_changes_)
    return;

  const ContentSettingsDetails details(primary_pattern, secondary_pattern,
                                       content_type, resource_identifier);
  DCHECK(!details.update_all_types());

  if (details.update_all()) {
    ContentSetting global_setting = settings_map_->GetDefaultContentSetting(
        CONTENT_SETTINGS_TYPE_ADS, nullptr);
    // Ignore changes which retain the status quo. This also avoids logging
    // metrics for changes which somehow notify this observer multiple times in
    // a row. This shouldn't discount real user initiated changes.
    if (global_setting == cached_global_setting_for_metrics_)
      return;
    cached_global_setting_for_metrics_ = global_setting;
    if (global_setting == CONTENT_SETTING_ALLOW) {
      ChromeSubresourceFilterClient::LogAction(
          kActionContentSettingsAllowedGlobal);
    } else if (global_setting == CONTENT_SETTING_BLOCK) {
      ChromeSubresourceFilterClient::LogAction(
          kActionContentSettingsBlockedGlobal);
    } else {
      NOTREACHED();
    }
    return;
  }

  // Remove this DCHECK if extension APIs or admin policies are given the
  // ability to set secondary patterns for this setting.
  DCHECK(secondary_pattern == ContentSettingsPattern::Wildcard());

  DCHECK(primary_pattern.IsValid());

  // An invalid URL indicates that this is a wildcard pattern.
  GURL url = GURL(primary_pattern.ToString());
  if (!url.is_valid()) {
    ChromeSubresourceFilterClient::LogAction(
        kActionContentSettingsWildcardUpdate);
    return;
  }

  ContentSetting setting = settings_map_->GetContentSetting(
      url, url, ContentSettingsType::CONTENT_SETTINGS_TYPE_ADS, std::string());
  if (setting == CONTENT_SETTING_ALLOW) {
    ChromeSubresourceFilterClient::LogAction(kActionContentSettingsAllowed);
  } else if (setting == CONTENT_SETTING_BLOCK) {
    ChromeSubresourceFilterClient::LogAction(kActionContentSettingsBlocked);
  } else {
    NOTREACHED();
  }

  if (!ShouldShowUIForSite(url)) {
    ChromeSubresourceFilterClient::LogAction(
        kActionContentSettingsAllowedWhileUISuppressed);
  }
}

// When history URLs are deleted, clear the metadata for the smart UI.
void SubresourceFilterContentSettingsManager::OnURLsDeleted(
    history::HistoryService* history_service,
    const history::DeletionInfo& deletion_info) {
  if (deletion_info.IsAllHistory()) {
    settings_map_->ClearSettingsForOneType(CONTENT_SETTINGS_TYPE_ADS_DATA);
    return;
  }

  for (const auto& entry : deletion_info.deleted_urls_origin_map()) {
    const GURL& origin = entry.first;
    int remaining_urls = entry.second.first;
    if (!origin.is_empty() && remaining_urls == 0)
      SetSiteMetadata(origin, nullptr);
  }
}
