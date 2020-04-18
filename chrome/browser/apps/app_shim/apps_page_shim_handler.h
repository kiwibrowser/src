// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_APPS_APP_SHIM_APPS_PAGE_SHIM_HANDLER_H_
#define CHROME_BROWSER_APPS_APP_SHIM_APPS_PAGE_SHIM_HANDLER_H_

#include <vector>

#include "base/macros.h"
#include "chrome/browser/apps/app_shim/app_shim_handler_mac.h"

// Stub handler for the old app_list shim that opens chrome://apps when a copy
// of the App Launcher .app bundle created with Chrome < m52 tries to connect.
class AppsPageShimHandler : public apps::AppShimHandler {
 public:
  AppsPageShimHandler() {}

  // AppShimHandler:
  void OnShimLaunch(apps::AppShimHandler::Host* host,
                    apps::AppShimLaunchType launch_type,
                    const std::vector<base::FilePath>& files) override;
  void OnShimClose(apps::AppShimHandler::Host* host) override;
  void OnShimFocus(apps::AppShimHandler::Host* host,
                   apps::AppShimFocusType focus_type,
                   const std::vector<base::FilePath>& files) override;
  void OnShimSetHidden(apps::AppShimHandler::Host* host, bool hidden) override;
  void OnShimQuit(apps::AppShimHandler::Host* host) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AppsPageShimHandler);
};

#endif  // CHROME_BROWSER_APPS_APP_SHIM_APPS_PAGE_SHIM_HANDLER_H_
