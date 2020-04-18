// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_LAUNCHER_LAUNCHER_CROSTINI_APP_UPDATER_H_
#define CHROME_BROWSER_UI_ASH_LAUNCHER_LAUNCHER_CROSTINI_APP_UPDATER_H_

#include <string>

#include "base/macros.h"
#include "chrome/browser/chromeos/crostini/crostini_registry_service.h"
#include "chrome/browser/ui/ash/launcher/launcher_app_updater.h"

class LauncherCrostiniAppUpdater
    : public LauncherAppUpdater,
      public crostini::CrostiniRegistryService::Observer {
 public:
  LauncherCrostiniAppUpdater(Delegate* delegate,
                             content::BrowserContext* browser_context);
  ~LauncherCrostiniAppUpdater() override;

  // crostini::CrostiniRegistryService::Observer:
  void OnRegistryUpdated(
      crostini::CrostiniRegistryService* registry_service,
      const std::vector<std::string>& updated_apps,
      const std::vector<std::string>& removed_apps,
      const std::vector<std::string>& inserted_apps) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LauncherCrostiniAppUpdater);
};

#endif  // CHROME_BROWSER_UI_ASH_LAUNCHER_LAUNCHER_CROSTINI_APP_UPDATER_H_
