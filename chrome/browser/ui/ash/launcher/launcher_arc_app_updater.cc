// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/launcher_arc_app_updater.h"

#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"

LauncherArcAppUpdater::LauncherArcAppUpdater(
    Delegate* delegate,
    content::BrowserContext* browser_context)
    : LauncherAppUpdater(delegate, browser_context) {
  ArcAppListPrefs* prefs = ArcAppListPrefs::Get(browser_context);
  DCHECK(prefs);
  prefs->AddObserver(this);
}

LauncherArcAppUpdater::~LauncherArcAppUpdater() {
  ArcAppListPrefs* prefs = ArcAppListPrefs::Get(browser_context());
  DCHECK(prefs);
  prefs->RemoveObserver(this);
}

void LauncherArcAppUpdater::OnAppRegistered(
    const std::string& app_id,
    const ArcAppListPrefs::AppInfo& app_info) {
  delegate()->OnAppInstalled(browser_context(), app_id);
}

void LauncherArcAppUpdater::OnAppRemoved(const std::string& app_id) {
  // ChromeLauncherController generally removes items from the sync model when
  // they are removed from the shelf model, but that should not happen when ARC
  // apps are disabled due to ARC itself being disabled. The app entries should
  // remain in the sync model for other synced devices that do support ARC, and
  // to restore the respective shelf items if ARC is re-enabled on this device.
  ChromeLauncherController::ScopedPinSyncDisabler scoped_pin_sync_disabler =
      ChromeLauncherController::instance()->GetScopedPinSyncDisabler();
  delegate()->OnAppUninstalledPrepared(browser_context(), app_id);
  delegate()->OnAppUninstalled(browser_context(), app_id);
}
