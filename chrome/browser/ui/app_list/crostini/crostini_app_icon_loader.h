// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_CROSTINI_CROSTINI_APP_ICON_LOADER_H_
#define CHROME_BROWSER_UI_APP_LIST_CROSTINI_CROSTINI_APP_ICON_LOADER_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/chromeos/crostini/crostini_registry_service.h"
#include "chrome/browser/ui/app_icon_loader.h"
#include "chrome/browser/ui/app_list/crostini/crostini_app_icon.h"
#include "ui/gfx/image/image_skia.h"

class Profile;

// An AppIconLoader that loads icons for Crostini apps.
class CrostiniAppIconLoader
    : public AppIconLoader,
      public crostini::CrostiniRegistryService::Observer,
      public CrostiniAppIcon::Observer {
 public:
  CrostiniAppIconLoader(Profile* profile,
                        int resource_size_in_dip,
                        AppIconLoaderDelegate* delegate);
  ~CrostiniAppIconLoader() override;

  // AppIconLoader:
  bool CanLoadImageForApp(const std::string& app_id) override;
  void FetchImage(const std::string& app_id) override;
  void ClearImage(const std::string& app_id) override;
  void UpdateImage(const std::string& app_id) override;

  // CrostiniRegistryService::Observer:
  void OnAppIconUpdated(const std::string& app_id,
                        ui::ScaleFactor scale_factor) override;

  // CrostiniAppIcon::Observer:
  void OnIconUpdated(CrostiniAppIcon* icon) override;

 private:
  using AppIDToIconMap =
      std::map<std::string, std::unique_ptr<CrostiniAppIcon>>;

  // Not owned.
  crostini::CrostiniRegistryService* registry_service_;

  // Maps from Crostini app id to icon.
  AppIDToIconMap icon_map_;

  DISALLOW_COPY_AND_ASSIGN(CrostiniAppIconLoader);
};

#endif  // CHROME_BROWSER_UI_APP_LIST_CROSTINI_CROSTINI_APP_ICON_LOADER_H_
