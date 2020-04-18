// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/arc_app_result.h"

#include <utility>

#include "ash/public/cpp/app_list/app_list_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/app_list_controller_delegate.h"
#include "chrome/browser/ui/app_list/arc/arc_app_context_menu.h"
#include "chrome/browser/ui/app_list/arc/arc_app_icon_loader.h"
#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "chrome/browser/ui/app_list/search/search_util.h"

namespace {
const char kArcAppPrefix[] = "arc://";
}

namespace app_list {

ArcAppResult::ArcAppResult(Profile* profile,
                           const std::string& app_id,
                           AppListControllerDelegate* controller,
                           bool is_recommendation)
    : AppResult(profile, app_id, controller, is_recommendation) {
  std::string id = kArcAppPrefix;
  id += app_id;
  set_id(id);
  icon_loader_.reset(new ArcAppIconLoader(
      profile, GetPreferredIconDimension(display_type()), this));
  icon_loader_->FetchImage(app_id);
}

ArcAppResult::~ArcAppResult() {
}

void ArcAppResult::OnAppImageUpdated(const std::string& app_id,
                                     const gfx::ImageSkia& image) {
  SetIcon(image);
}

void ArcAppResult::ExecuteLaunchCommand(int event_flags) {
  Open(event_flags);
}

void ArcAppResult::Open(int event_flags) {
  // Record the search metric if the result is not a suggested app.
  if (display_type() != ash::SearchResultDisplayType::kRecommendation)
    RecordHistogram(APP_SEARCH_RESULT);

  if (!arc::LaunchApp(profile(), app_id(), event_flags,
                      controller()->GetAppListDisplayId())) {
    return;
  }

  // Manually close app_list view because focus is not changed on ARC app start,
  // and current view remains active. Do not close app list for home launcher.
  if (!controller()->IsHomeLauncherEnabledInTabletMode())
    controller()->DismissView();
}

void ArcAppResult::GetContextMenuModel(GetMenuModelCallback callback) {
  context_menu_ = std::make_unique<ArcAppContextMenu>(this, profile(), app_id(),
                                                      controller());
  context_menu_->GetMenuModel(std::move(callback));
}

AppContextMenu* ArcAppResult::GetAppContextMenu() {
  return context_menu_.get();
}

}  // namespace app_list
