// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_LAUNCHER_LAUNCHER_ARC_APP_UPDATER_H_
#define CHROME_BROWSER_UI_ASH_LAUNCHER_LAUNCHER_ARC_APP_UPDATER_H_

#include <string>

#include "base/macros.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/ash/launcher/launcher_app_updater.h"

class LauncherArcAppUpdater : public LauncherAppUpdater,
                              public ArcAppListPrefs::Observer {
 public:
  LauncherArcAppUpdater(Delegate* delegate,
                        content::BrowserContext* browser_context);
  ~LauncherArcAppUpdater() override;

  // ArcAppListPrefs::Observer:
  void OnAppRegistered(const std::string& app_id,
                       const ArcAppListPrefs::AppInfo& app_info) override;
  void OnAppRemoved(const std::string& id) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LauncherArcAppUpdater);
};

#endif  // CHROME_BROWSER_UI_ASH_LAUNCHER_LAUNCHER_ARC_APP_UPDATER_H_
