// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/test/integration/sync_integration_test_util.h"

#include "base/strings/stringprintf.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/sync/test/integration/themes_helper.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "components/browser_sync/profile_sync_service.h"
#include "content/public/test/test_utils.h"

void SetCustomTheme(Profile* profile, int theme_index) {
  themes_helper::UseCustomTheme(profile, theme_index);
  content::WindowedNotificationObserver theme_change_observer(
      chrome::NOTIFICATION_BROWSER_THEME_CHANGED,
      content::Source<ThemeService>(
          ThemeServiceFactory::GetForProfile(profile)));
  theme_change_observer.Wait();
}

ServerCountMatchStatusChecker::ServerCountMatchStatusChecker(
    syncer::ModelType type,
    size_t count)
    : type_(type), count_(count) {}

bool ServerCountMatchStatusChecker::IsExitConditionSatisfied() {
  return count_ == fake_server()->GetSyncEntitiesByModelType(type_).size();
}

std::string ServerCountMatchStatusChecker::GetDebugMessage() const {
  return base::StringPrintf(
      "Waiting for fake server entity count %zu to match expected count %zu "
      "for type %d",
      (size_t)fake_server()->GetSyncEntitiesByModelType(type_).size(), count_,
      type_);
}

PassphraseRequiredChecker::PassphraseRequiredChecker(
    browser_sync::ProfileSyncService* service)
    : SingleClientStatusChangeChecker(service) {}

bool PassphraseRequiredChecker::IsExitConditionSatisfied() {
  return service()->IsPassphraseRequired();
}

std::string PassphraseRequiredChecker::GetDebugMessage() const {
  return "Passhrase Required";
}

PassphraseAcceptedChecker::PassphraseAcceptedChecker(
    browser_sync::ProfileSyncService* service)
    : SingleClientStatusChangeChecker(service) {}

bool PassphraseAcceptedChecker::IsExitConditionSatisfied() {
  return !service()->IsPassphraseRequired() &&
         service()->IsUsingSecondaryPassphrase();
}

std::string PassphraseAcceptedChecker::GetDebugMessage() const {
  return "Passhrase Accepted";
}
