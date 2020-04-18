// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef CHROME_BROWSER_UI_APP_LIST_CROSTINI_CROSTINI_APP_MODEL_BUILDER_H_
#define CHROME_BROWSER_UI_APP_LIST_CROSTINI_CROSTINI_APP_MODEL_BUILDER_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/crostini/crostini_registry_service.h"
#include "chrome/browser/ui/app_list/app_list_model_builder.h"

class AppListControllerDelegate;
class PrefChangeRegistrar;

// This class populates and maintains Crostini apps.
class CrostiniAppModelBuilder
    : public AppListModelBuilder,
      public crostini::CrostiniRegistryService::Observer {
 public:
  explicit CrostiniAppModelBuilder(AppListControllerDelegate* controller);
  ~CrostiniAppModelBuilder() override;

 private:
  // AppListModelBuilder:
  void BuildModel() override;

  // CrostiniRegistryService::Observer:
  void OnRegistryUpdated(
      crostini::CrostiniRegistryService* registry_service,
      const std::vector<std::string>& updated_apps,
      const std::vector<std::string>& removed_apps,
      const std::vector<std::string>& inserted_apps) override;
  void OnAppIconUpdated(const std::string& app_id,
                        ui::ScaleFactor scale_factor) override;

  void InsertCrostiniAppItem(
      const crostini::CrostiniRegistryService* registry_service,
      const std::string& app_id);

  void OnCrostiniEnabledChanged();

  // Observer Crostini installation so we can start showing The Terminal app.
  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;

  DISALLOW_COPY_AND_ASSIGN(CrostiniAppModelBuilder);
};

#endif  // CHROME_BROWSER_UI_APP_LIST_CROSTINI_CROSTINI_APP_MODEL_BUILDER_H_
