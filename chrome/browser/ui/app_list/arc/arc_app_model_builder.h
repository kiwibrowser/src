// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_ARC_ARC_APP_MODEL_BUILDER_H_
#define CHROME_BROWSER_UI_APP_LIST_ARC_ARC_APP_MODEL_BUILDER_H_

#include <stddef.h>

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/ui/app_list/app_list_model_builder.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"

class AppListControllerDelegate;
class ArcAppItem;

// This class populates and maintains ARC apps.
class ArcAppModelBuilder : public AppListModelBuilder,
                           public ArcAppListPrefs::Observer {
 public:
  explicit ArcAppModelBuilder(AppListControllerDelegate* controller);
  ~ArcAppModelBuilder() override;

 private:
  // AppListModelBuilder
  void BuildModel() override;

  // ArcAppListPrefs::Observer
  void OnAppRegistered(const std::string& app_id,
                       const ArcAppListPrefs::AppInfo& app_info) override;
  void OnAppRemoved(const std::string& id) override;
  void OnAppIconUpdated(const std::string& app_id,
                        ui::ScaleFactor scale_factor) override;
  void OnAppNameUpdated(const std::string& app_id,
                        const std::string& name) override;

  std::unique_ptr<ArcAppItem> CreateApp(const std::string& app_id,
                                        const ArcAppListPrefs::AppInfo& info);

  ArcAppItem* GetArcAppItem(const std::string& app_id);

  ArcAppListPrefs* prefs_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ArcAppModelBuilder);
};

#endif  // CHROME_BROWSER_UI_APP_LIST_ARC_ARC_APP_MODEL_BUILDER_H_
