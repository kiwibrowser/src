// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/desktop_promotion/desktop_promotion_sync_service.h"

#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/desktop_promotion/desktop_promotion_sync_observer.h"
#include "ios/chrome/browser/pref_names.h"

DesktopPromotionSyncService::DesktopPromotionSyncService(
    PrefService* pref_service,
    browser_sync::ProfileSyncService* sync_service)
    : observer_(pref_service, sync_service) {}

DesktopPromotionSyncService::~DesktopPromotionSyncService() = default;

// static
void DesktopPromotionSyncService::RegisterDesktopPromotionUserPrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(
      prefs::kDesktopIOSPromotionEligible, false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);
  registry->RegisterIntegerPref(
      prefs::kDesktopIOSPromotionSMSEntryPoint, 0,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);
  registry->RegisterIntegerPref(
      prefs::kDesktopIOSPromotionShownEntryPoints, 0,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);
  registry->RegisterDoublePref(
      prefs::kDesktopIOSPromotionLastImpression, 0,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);
  registry->RegisterBooleanPref(
      prefs::kDesktopIOSPromotionDone, false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);
  registry->RegisterIntegerPref(
      prefs::kDesktopIOSPromotionVariationId, 0,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);
}
