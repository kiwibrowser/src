// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/arc/arc_app_icon_loader.h"

#include "ash/public/cpp/app_list/app_list_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"

ArcAppIconLoader::ArcAppIconLoader(Profile* profile,
                                   int icon_size,
                                   AppIconLoaderDelegate* delegate)
    : AppIconLoader(profile, icon_size, delegate),
      arc_prefs_(ArcAppListPrefs::Get(profile)) {
  DCHECK(arc_prefs_);
  arc_prefs_->AddObserver(this);
}

ArcAppIconLoader::~ArcAppIconLoader() {
  arc_prefs_->RemoveObserver(this);
}

bool ArcAppIconLoader::CanLoadImageForApp(const std::string& app_id) {
  if (icon_map_.find(app_id) != icon_map_.end())
    return true;
  return arc::IsArcItem(profile(), app_id);
}

void ArcAppIconLoader::FetchImage(const std::string& app_id) {
  if (icon_map_.find(app_id) != icon_map_.end())
    return;  // Already loading the image.

  // Note, ARC icon is available only for 48x48 dips. In case |icon_size_|
  // differs from this size, re-scale is required.
  std::unique_ptr<ArcAppIcon> icon(
      new ArcAppIcon(profile(), app_id, app_list::kGridIconDimension, this));
  icon->image_skia().EnsureRepsForSupportedScales();
  icon_map_[app_id] = std::move(icon);
  UpdateImage(app_id);
}

void ArcAppIconLoader::ClearImage(const std::string& app_id) {
  icon_map_.erase(app_id);
}

void ArcAppIconLoader::UpdateImage(const std::string& app_id) {
  AppIDToIconMap::iterator it = icon_map_.find(app_id);
  if (it == icon_map_.end())
    return;

  delegate()->OnAppImageUpdated(app_id, it->second->image_skia());
}

void ArcAppIconLoader::OnIconUpdated(ArcAppIcon* icon) {
  UpdateImage(icon->app_id());
}

void ArcAppIconLoader::OnAppReadyChanged(const std::string& app_id,
                                         bool ready) {
  AppIDToIconMap::const_iterator it = icon_map_.find(app_id);
  if (it == icon_map_.end())
    return;

  UpdateImage(app_id);
}

void ArcAppIconLoader::OnAppIconUpdated(const std::string& app_id,
                                        ui::ScaleFactor scale_factor) {
  AppIDToIconMap::const_iterator it = icon_map_.find(app_id);
  if (it == icon_map_.end())
    return;
  it->second->LoadForScaleFactor(scale_factor);
}
