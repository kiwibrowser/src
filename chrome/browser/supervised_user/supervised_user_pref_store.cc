// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/supervised_user/supervised_user_pref_store.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/values.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/net/safe_search_util.h"
#include "chrome/browser/prefs/incognito_mode_prefs.h"
#include "chrome/browser/supervised_user/supervised_user_constants.h"
#include "chrome/browser/supervised_user/supervised_user_settings_service.h"
#include "chrome/browser/supervised_user/supervised_user_url_filter.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/ntp_snippets/pref_names.h"
#include "components/prefs/pref_value_map.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "content/public/browser/notification_source.h"

namespace {

struct SupervisedUserSettingsPrefMappingEntry {
  const char* settings_name;
  const char* pref_name;
};

SupervisedUserSettingsPrefMappingEntry kSupervisedUserSettingsPrefMapping[] = {
#if defined(OS_CHROMEOS)
    {
        supervised_users::kAccountConsistencyMirrorRequired,
        prefs::kAccountConsistencyMirrorRequired,
    },
#endif
    {
        supervised_users::kApprovedExtensions,
        prefs::kSupervisedUserApprovedExtensions,
    },
    {
        supervised_users::kContentPackDefaultFilteringBehavior,
        prefs::kDefaultSupervisedUserFilteringBehavior,
    },
    {
        supervised_users::kContentPackManualBehaviorHosts,
        prefs::kSupervisedUserManualHosts,
    },
    {
        supervised_users::kContentPackManualBehaviorURLs,
        prefs::kSupervisedUserManualURLs,
    },
    {
        supervised_users::kForceSafeSearch, prefs::kForceGoogleSafeSearch,
    },
    {
        supervised_users::kSafeSitesEnabled, prefs::kSupervisedUserSafeSites,
    },
    {
        supervised_users::kSigninAllowed, prefs::kSigninAllowed,
    },
    {
        supervised_users::kUserName, prefs::kProfileName,
    },
};

}  // namespace

SupervisedUserPrefStore::SupervisedUserPrefStore(
    SupervisedUserSettingsService* supervised_user_settings_service) {
  user_settings_subscription_ = supervised_user_settings_service->Subscribe(
      base::Bind(&SupervisedUserPrefStore::OnNewSettingsAvailable,
                 base::Unretained(this)));

  // Should only be nullptr in unit tests
  // TODO(peconn): Remove this once SupervisedUserPrefStore is (partially at
  // least) a KeyedService. The user_settings_subscription_ must be reset or
  // destroyed before the SupervisedUserSettingsService is.
  if (supervised_user_settings_service->GetProfile()) {
    unsubscriber_registrar_.Add(this, chrome::NOTIFICATION_PROFILE_DESTROYED,
        content::Source<Profile>(
          supervised_user_settings_service->GetProfile()));
  }
}

bool SupervisedUserPrefStore::GetValue(const std::string& key,
                                       const base::Value** value) const {
  return prefs_->GetValue(key, value);
}

std::unique_ptr<base::DictionaryValue> SupervisedUserPrefStore::GetValues()
    const {
  return prefs_->AsDictionaryValue();
}

void SupervisedUserPrefStore::AddObserver(PrefStore::Observer* observer) {
  observers_.AddObserver(observer);
}

void SupervisedUserPrefStore::RemoveObserver(PrefStore::Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool SupervisedUserPrefStore::HasObservers() const {
  return observers_.might_have_observers();
}

bool SupervisedUserPrefStore::IsInitializationComplete() const {
  return !!prefs_;
}

SupervisedUserPrefStore::~SupervisedUserPrefStore() {
}

void SupervisedUserPrefStore::OnNewSettingsAvailable(
    const base::DictionaryValue* settings) {
  std::unique_ptr<PrefValueMap> old_prefs = std::move(prefs_);
  prefs_.reset(new PrefValueMap);
  if (settings) {
    // Set hardcoded prefs and defaults.
#if defined(OS_CHROMEOS)
    prefs_->SetBoolean(prefs::kAccountConsistencyMirrorRequired, false);
#endif
    prefs_->SetInteger(prefs::kDefaultSupervisedUserFilteringBehavior,
                       SupervisedUserURLFilter::ALLOW);
    prefs_->SetBoolean(prefs::kForceGoogleSafeSearch, true);
    prefs_->SetInteger(prefs::kForceYouTubeRestrict,
                       safe_search_util::YOUTUBE_RESTRICT_MODERATE);
    prefs_->SetBoolean(prefs::kHideWebStoreIcon, true);
    prefs_->SetBoolean(prefs::kSigninAllowed, false);
    prefs_->SetBoolean(ntp_snippets::prefs::kEnableSnippets, false);

    // Copy supervised user settings to prefs.
    for (const auto& entry : kSupervisedUserSettingsPrefMapping) {
      const base::Value* value = NULL;
      if (settings->GetWithoutPathExpansion(entry.settings_name, &value))
        prefs_->SetValue(entry.pref_name, value->CreateDeepCopy());
    }

    // Manually set preferences that aren't direct copies of the settings value.
    {
      bool record_history = true;
      settings->GetBoolean(supervised_users::kRecordHistory, &record_history);
      prefs_->SetBoolean(prefs::kAllowDeletingBrowserHistory, !record_history);
      prefs_->SetInteger(prefs::kIncognitoModeAvailability,
                         record_history ? IncognitoModePrefs::DISABLED
                                        : IncognitoModePrefs::ENABLED);

      bool record_history_includes_session_sync = true;
      settings->GetBoolean(supervised_users::kRecordHistoryIncludesSessionSync,
                           &record_history_includes_session_sync);
      prefs_->SetBoolean(
          prefs::kForceSessionSync,
          record_history && record_history_includes_session_sync);
    }

    {
      // Note that |prefs::kForceGoogleSafeSearch| is set automatically as part
      // of |kSupervisedUserSettingsPrefMapping|, but this can't be done for
      // |prefs::kForceYouTubeRestrict| because it is an int, not a bool.
      bool force_safe_search = true;
      settings->GetBoolean(supervised_users::kForceSafeSearch,
                           &force_safe_search);
      prefs_->SetInteger(
          prefs::kForceYouTubeRestrict,
          force_safe_search ? safe_search_util::YOUTUBE_RESTRICT_MODERATE
                            : safe_search_util::YOUTUBE_RESTRICT_OFF);
    }
  }

  if (!old_prefs) {
    for (Observer& observer : observers_)
      observer.OnInitializationCompleted(true);
    return;
  }

  std::vector<std::string> changed_prefs;
  prefs_->GetDifferingKeys(old_prefs.get(), &changed_prefs);

  // Send out change notifications.
  for (const std::string& pref : changed_prefs) {
    for (Observer& observer : observers_)
      observer.OnPrefValueChanged(pref);
  }
}

// Callback to unsubscribe from the supervised user settings service.
void SupervisedUserPrefStore::Observe(
    int type,
    const content::NotificationSource& src,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_PROFILE_DESTROYED, type);
  user_settings_subscription_.reset();
}
