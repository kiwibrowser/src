// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/arc/arc_app_model_builder.h"

#include <vector>

#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/arc/arc_app_item.h"

ArcAppModelBuilder::ArcAppModelBuilder(AppListControllerDelegate* controller)
    : AppListModelBuilder(controller, ArcAppItem::kItemType) {
}

ArcAppModelBuilder::~ArcAppModelBuilder() {
  prefs_->RemoveObserver(this);
}

void ArcAppModelBuilder::BuildModel() {
  prefs_ = ArcAppListPrefs::Get(profile());
  DCHECK(prefs_);

  std::vector<std::string> app_ids = prefs_->GetAppIds();
  for (auto& app_id : app_ids) {
    std::unique_ptr<ArcAppListPrefs::AppInfo> app_info = prefs_->GetApp(app_id);
    if (!app_info)
      continue;

    if (app_info->showInLauncher)
      InsertApp(CreateApp(app_id, *app_info));
  }

  prefs_->AddObserver(this);
}

ArcAppItem* ArcAppModelBuilder::GetArcAppItem(const std::string& app_id) {
  return static_cast<ArcAppItem*>(GetAppItem(app_id));
}

std::unique_ptr<ArcAppItem> ArcAppModelBuilder::CreateApp(
    const std::string& app_id,
    const ArcAppListPrefs::AppInfo& app_info) {
  return std::make_unique<ArcAppItem>(
      profile(), model_updater(), GetSyncItem(app_id), app_id, app_info.name);
}

void ArcAppModelBuilder::OnAppRegistered(
    const std::string& app_id,
    const ArcAppListPrefs::AppInfo& app_info) {
  if (app_info.showInLauncher)
    InsertApp(CreateApp(app_id, app_info));
}

void ArcAppModelBuilder::OnAppRemoved(const std::string& app_id) {
  // Don't sync app removal in case it was caused by disabling Google Play
  // Store.
  const bool unsynced_change = !arc::IsArcPlayStoreEnabledForProfile(profile());
  RemoveApp(app_id, unsynced_change);
}

void ArcAppModelBuilder::OnAppIconUpdated(const std::string& app_id,
                                          ui::ScaleFactor scale_factor) {
  ArcAppItem* app_item = GetArcAppItem(app_id);
  if (!app_item) {
    VLOG(2) << "Could not update the icon of ARC app(" << app_id
            << ") because it was not found.";
    return;
  }

  // Initiate async icon reloading.
  app_item->arc_app_icon()->LoadForScaleFactor(scale_factor);
}

void ArcAppModelBuilder::OnAppNameUpdated(const std::string& app_id,
                                          const std::string& name) {
  ArcAppItem* app_item = GetArcAppItem(app_id);
  if (!app_item) {
    VLOG(2) << "Could not update the name of ARC app(" << app_id
            << ") because it was not found.";
    return;
  }

  app_item->SetName(name);
}
