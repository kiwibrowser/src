// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_APPS_APP_INFO_DIALOG_ARC_APP_INFO_LINKS_PANEL_H_
#define CHROME_BROWSER_UI_VIEWS_APPS_APP_INFO_DIALOG_ARC_APP_INFO_LINKS_PANEL_H_

#include <stdint.h>

#include <string>

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/views/apps/app_info_dialog/app_info_panel.h"
#include "ui/views/controls/link_listener.h"

class Profile;

namespace extensions {
class Extension;
}

namespace views {
class Link;
}

// Shows a link to get to managing supported links activity on ARC side.
class ArcAppInfoLinksPanel : public AppInfoPanel,
                             public views::LinkListener,
                             public ArcAppListPrefs::Observer {
 public:
  ArcAppInfoLinksPanel(Profile* profile, const extensions::Extension* app);
  ~ArcAppInfoLinksPanel() override;

 private:
  // views::LinkListener:
  void LinkClicked(views::Link* source, int event_flags) override;

  // ArcAppListPrefs::Observer:
  void OnAppReadyChanged(const std::string& app_id, bool ready) override;
  void OnAppRemoved(const std::string& app_id) override;
  void OnAppRegistered(const std::string& app_id,
                       const ArcAppListPrefs::AppInfo& app_info) override;

  void UpdateLink(bool enabled);

  ScopedObserver<ArcAppListPrefs, ArcAppListPrefs::Observer> app_list_observer_;
  views::Link* manage_link_;

  DISALLOW_COPY_AND_ASSIGN(ArcAppInfoLinksPanel);
};

#endif  // CHROME_BROWSER_UI_VIEWS_APPS_APP_INFO_DIALOG_ARC_APP_INFO_LINKS_PANEL_H_
