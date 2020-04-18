// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/content_settings/core/browser/content_settings_pref_provider.h"

#include <stddef.h>

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/default_clock.h"
#include "components/content_settings/core/browser/content_settings_pref.h"
#include "components/content_settings/core/browser/content_settings_rule.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/browser/website_settings_info.h"
#include "components/content_settings/core/browser/website_settings_registry.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "services/preferences/public/cpp/dictionary_value_update.h"
#include "services/preferences/public/cpp/scoped_pref_update.h"

namespace content_settings {

namespace {

// These settings are no longer used, and should be deleted on profile startup.
const char kObsoleteDomainToOriginMigrationStatus[] =
    "profile.content_settings.domain_to_origin_migration_status";

#if !defined(OS_IOS)
const char kObsoleteFullscreenExceptionsPref[] =
    "profile.content_settings.exceptions.fullscreen";
#if !defined(OS_ANDROID)
const char kObsoleteMouseLockExceptionsPref[] =
    "profile.content_settings.exceptions.mouselock";
#endif  // !defined(OS_ANDROID)
#endif  // !defined(OS_IOS)

}  // namespace

// ////////////////////////////////////////////////////////////////////////////
// PrefProvider:
//

// static
void PrefProvider::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(
      prefs::kContentSettingsVersion,
      ContentSettingsPattern::kContentSettingsPatternVersion);

  WebsiteSettingsRegistry* website_settings =
      WebsiteSettingsRegistry::GetInstance();
  for (const WebsiteSettingsInfo* info : *website_settings) {
    registry->RegisterDictionaryPref(info->pref_name(),
                                     info->GetPrefRegistrationFlags());
  }

  // Obsolete prefs ----------------------------------------------------------

  // These prefs have been removed, but need to be registered so they can
  // be deleted on startup.
  registry->RegisterIntegerPref(kObsoleteDomainToOriginMigrationStatus, 0);
#if !defined(OS_IOS)
  registry->RegisterDictionaryPref(
      kObsoleteFullscreenExceptionsPref,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
#if !defined(OS_ANDROID)
  registry->RegisterDictionaryPref(
      kObsoleteMouseLockExceptionsPref,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
#endif  // !defined(OS_ANDROID)
#endif  // !defined(OS_IOS)
}

PrefProvider::PrefProvider(PrefService* prefs,
                           bool incognito,
                           bool store_last_modified)
    : prefs_(prefs),
      is_incognito_(incognito),
      store_last_modified_(store_last_modified),
      clock_(base::DefaultClock::GetInstance()) {
  DCHECK(prefs_);
  // Verify preferences version.
  if (!prefs_->HasPrefPath(prefs::kContentSettingsVersion)) {
    prefs_->SetInteger(prefs::kContentSettingsVersion,
                       ContentSettingsPattern::kContentSettingsPatternVersion);
  }
  if (prefs_->GetInteger(prefs::kContentSettingsVersion) >
      ContentSettingsPattern::kContentSettingsPatternVersion) {
    return;
  }

  DiscardObsoletePreferences();

  pref_change_registrar_.Init(prefs_);

  WebsiteSettingsRegistry* website_settings =
      WebsiteSettingsRegistry::GetInstance();
  for (const WebsiteSettingsInfo* info : *website_settings) {
    content_settings_prefs_.insert(std::make_pair(
        info->type(),
        std::make_unique<ContentSettingsPref>(
            info->type(), prefs_, &pref_change_registrar_, info->pref_name(),
            is_incognito_,
            base::Bind(&PrefProvider::Notify, base::Unretained(this)))));
  }

  if (!is_incognito_) {
    size_t num_exceptions = 0;
    for (const auto& pref : content_settings_prefs_)
      num_exceptions += pref.second->GetNumExceptions();

    UMA_HISTOGRAM_COUNTS("ContentSettings.NumberOfExceptions",
                         num_exceptions);
  }
}

PrefProvider::~PrefProvider() {
  DCHECK(!prefs_);
}

std::unique_ptr<RuleIterator> PrefProvider::GetRuleIterator(
    ContentSettingsType content_type,
    const ResourceIdentifier& resource_identifier,
    bool incognito) const {
  return GetPref(content_type)->GetRuleIterator(resource_identifier, incognito);
}

bool PrefProvider::SetWebsiteSetting(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    const ResourceIdentifier& resource_identifier,
    base::Value* in_value) {
  DCHECK(CalledOnValidThread());
  DCHECK(prefs_);

  // Default settings are set using a wildcard pattern for both
  // |primary_pattern| and |secondary_pattern|. Don't store default settings in
  // the |PrefProvider|. The |PrefProvider| handles settings for specific
  // sites/origins defined by the |primary_pattern| and the |secondary_pattern|.
  // Default settings are handled by the |DefaultProvider|.
  if (primary_pattern == ContentSettingsPattern::Wildcard() &&
      secondary_pattern == ContentSettingsPattern::Wildcard() &&
      resource_identifier.empty()) {
    return false;
  }

  base::Time modified_time =
      store_last_modified_ ? clock_->Now() : base::Time();

  return GetPref(content_type)
      ->SetWebsiteSetting(primary_pattern, secondary_pattern,
                          resource_identifier, modified_time, in_value);
}

base::Time PrefProvider::GetWebsiteSettingLastModified(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    const ResourceIdentifier& resource_identifier) {
  DCHECK(CalledOnValidThread());
  DCHECK(prefs_);

  return GetPref(content_type)
      ->GetWebsiteSettingLastModified(primary_pattern, secondary_pattern,
                                      resource_identifier);
}

void PrefProvider::ClearAllContentSettingsRules(
    ContentSettingsType content_type) {
  DCHECK(CalledOnValidThread());
  DCHECK(prefs_);

  GetPref(content_type)->ClearAllContentSettingsRules();
}

void PrefProvider::ShutdownOnUIThread() {
  DCHECK(CalledOnValidThread());
  DCHECK(prefs_);
  RemoveAllObservers();
  pref_change_registrar_.RemoveAll();
  prefs_ = nullptr;
}

void PrefProvider::ClearPrefs() {
  DCHECK(CalledOnValidThread());
  DCHECK(prefs_);

  for (const auto& pref : content_settings_prefs_)
    pref.second->ClearPref();
}

ContentSettingsPref* PrefProvider::GetPref(ContentSettingsType type) const {
  auto it = content_settings_prefs_.find(type);
  DCHECK(it != content_settings_prefs_.end());
  return it->second.get();
}

void PrefProvider::Notify(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    const std::string& resource_identifier) {
  NotifyObservers(primary_pattern,
                  secondary_pattern,
                  content_type,
                  resource_identifier);
}

void PrefProvider::DiscardObsoletePreferences() {
  if (is_incognito_)
    return;

  prefs_->ClearPref(kObsoleteDomainToOriginMigrationStatus);

  // These prefs were never stored on iOS/Android so they don't need to be
  // deleted.
#if !defined(OS_IOS)
  prefs_->ClearPref(kObsoleteFullscreenExceptionsPref);
#if !defined(OS_ANDROID)
  prefs_->ClearPref(kObsoleteMouseLockExceptionsPref);
#endif  // !defined(OS_ANDROID)
#endif  // !defined(OS_IOS)
}

void PrefProvider::SetClockForTesting(base::Clock* clock) {
  clock_ = clock;
}

}  // namespace content_settings
