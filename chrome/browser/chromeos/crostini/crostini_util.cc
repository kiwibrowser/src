// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/crostini/crostini_util.h"

#include "base/feature_list.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/string_util.h"
#include "chrome/browser/chromeos/crostini/crostini_manager.h"
#include "chrome/browser/chromeos/crostini/crostini_pref_names.h"
#include "chrome/browser/chromeos/crostini/crostini_registry_service.h"
#include "chrome/browser/chromeos/crostini/crostini_registry_service_factory.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/chromeos/virtual_machines/virtual_machines_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_features.h"
#include "components/prefs/pref_service.h"

namespace {

constexpr char kCrostiniAppLaunchHistogram[] = "Crostini.AppLaunch";
constexpr char kCrostiniAppNamePrefix[] = "_crostini_";

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class CrostiniAppLaunchAppType {
  // An app which isn't in the CrostiniAppRegistry. This shouldn't happen.
  kUnknownApp = 0,

  // The main terminal app.
  kTerminal = 1,

  // An app for which there is something in the CrostiniAppRegistry.
  kRegisteredApp = 2,

  kCount
};

void RecordAppLaunchHistogram(CrostiniAppLaunchAppType app_type) {
  base::UmaHistogramEnumeration(kCrostiniAppLaunchHistogram, app_type,
                                CrostiniAppLaunchAppType::kCount);
}

void MaybeLaunchTerminal(Profile* profile,
                         crostini::ConciergeClientResult result) {
  if (result == crostini::ConciergeClientResult::SUCCESS) {
    crostini::CrostiniManager::GetInstance()->LaunchContainerTerminal(
        profile, kCrostiniDefaultVmName, kCrostiniDefaultContainerName);
  }
}

void MaybeLaunchContainerApplication(
    Profile* profile,
    std::unique_ptr<crostini::CrostiniRegistryService::Registration>
        registration,
    crostini::ConciergeClientResult result) {
  if (result == crostini::ConciergeClientResult::SUCCESS) {
    // TODO(timloh): Do something if launching failed, as otherwise the app
    // launcher remains open and there's no feedback.
    crostini::CrostiniManager::GetInstance()->LaunchContainerApplication(
        profile, registration->vm_name, registration->container_name,
        registration->desktop_file_id, base::DoNothing());
  }
}

}  // namespace

bool IsCrostiniAllowed() {
  return virtual_machines::AreVirtualMachinesAllowedByVersionAndChannel() &&
         virtual_machines::AreVirtualMachinesAllowedByPolicy() &&
         base::FeatureList::IsEnabled(features::kCrostini);
}

bool IsCrostiniUIAllowedForProfile(Profile* profile) {
  if (!chromeos::ProfileHelper::IsPrimaryProfile(profile)) {
    return false;
  }

  return IsCrostiniAllowed() &&
         base::FeatureList::IsEnabled(features::kExperimentalCrostiniUI);
}

bool IsCrostiniEnabled(Profile* profile) {
  return profile->GetPrefs()->GetBoolean(crostini::prefs::kCrostiniEnabled);
}

void LaunchCrostiniApp(Profile* profile, const std::string& app_id) {
  auto* crostini_manager = crostini::CrostiniManager::GetInstance();
  crostini::CrostiniRegistryService* registry_service =
      crostini::CrostiniRegistryServiceFactory::GetForProfile(profile);

  if (app_id == kCrostiniTerminalId) {
    RecordAppLaunchHistogram(CrostiniAppLaunchAppType::kTerminal);

    if (!crostini_manager->IsCrosTerminaInstalled() ||
        !IsCrostiniEnabled(profile)) {
      ShowCrostiniInstallerView(profile);
    } else {
      crostini_manager->RestartCrostini(
          profile, kCrostiniDefaultVmName, kCrostiniDefaultContainerName,
          base::BindOnce(&MaybeLaunchTerminal, profile));
    }
    registry_service->AppLaunched(app_id);
    return;
  }

  std::unique_ptr<crostini::CrostiniRegistryService::Registration>
      registration = registry_service->GetRegistration(app_id);
  if (!registration) {
    RecordAppLaunchHistogram(CrostiniAppLaunchAppType::kUnknownApp);
    LOG(ERROR) << "LaunchCrostiniApp called with an unknown app_id: " << app_id;
    return;
  }

  RecordAppLaunchHistogram(CrostiniAppLaunchAppType::kRegisteredApp);
  crostini_manager->RestartCrostini(
      profile, registration->vm_name, registration->container_name,
      base::BindOnce(&MaybeLaunchContainerApplication, profile,
                     std::move(registration)));
  registry_service->AppLaunched(app_id);
}

std::string CryptohomeIdForProfile(Profile* profile) {
  std::string id = chromeos::ProfileHelper::GetUserIdHashFromProfile(profile);
  // Empty id means we're running in a test.
  return id.empty() ? "test" : id;
}

std::string ContainerUserNameForProfile(Profile* profile) {
  // Get rid of the @domain.name in the profile user name (an email address).
  std::string container_username = profile->GetProfileUserName();
  return container_username.substr(0, container_username.find('@'));
}

std::string AppNameFromCrostiniAppId(const std::string& id) {
  return kCrostiniAppNamePrefix + id;
}

base::Optional<std::string> CrostiniAppIdFromAppName(
    const std::string& app_name) {
  if (!base::StartsWith(app_name, kCrostiniAppNamePrefix,
                        base::CompareCase::SENSITIVE)) {
    return base::nullopt;
  }
  return app_name.substr(strlen(kCrostiniAppNamePrefix));
}
