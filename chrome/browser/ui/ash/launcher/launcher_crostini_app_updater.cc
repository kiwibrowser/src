// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/launcher_crostini_app_updater.h"

#include "chrome/browser/chromeos/crostini/crostini_registry_service_factory.h"
#include "chrome/browser/profiles/profile.h"

LauncherCrostiniAppUpdater::LauncherCrostiniAppUpdater(
    Delegate* delegate,
    content::BrowserContext* browser_context)
    : LauncherAppUpdater(delegate, browser_context) {
  crostini::CrostiniRegistryService* registry_service =
      crostini::CrostiniRegistryServiceFactory::GetForProfile(
          Profile::FromBrowserContext(browser_context));
  registry_service->AddObserver(this);
}

LauncherCrostiniAppUpdater::~LauncherCrostiniAppUpdater() {
  crostini::CrostiniRegistryService* registry_service =
      crostini::CrostiniRegistryServiceFactory::GetForProfile(
          Profile::FromBrowserContext(browser_context()));
  registry_service->RemoveObserver(this);
}

void LauncherCrostiniAppUpdater::OnRegistryUpdated(
    crostini::CrostiniRegistryService* registry_service,
    const std::vector<std::string>& updated_apps,
    const std::vector<std::string>& removed_apps,
    const std::vector<std::string>& inserted_apps) {
  for (const std::string& app_id : updated_apps)
    delegate()->OnAppUpdated(browser_context(), app_id);
  for (const std::string& app_id : removed_apps) {
    delegate()->OnAppUninstalledPrepared(browser_context(), app_id);
    delegate()->OnAppUninstalled(browser_context(), app_id);
  }
  for (const std::string& app_id : inserted_apps)
    delegate()->OnAppInstalled(browser_context(), app_id);
}
