// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/internal_app_result.h"

#include <utility>

#include "ash/public/cpp/app_list/app_list_constants.h"
#include "ash/public/cpp/app_list/internal_app_id_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/app_context_menu.h"
#include "chrome/browser/ui/app_list/app_list_controller_delegate.h"
#include "chrome/browser/ui/app_list/internal_app/internal_app_metadata.h"
#include "chrome/browser/ui/app_list/search/search_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image_skia_operations.h"

namespace app_list {

InternalAppResult::InternalAppResult(Profile* profile,
                                     const std::string& app_id,
                                     AppListControllerDelegate* controller,
                                     bool is_recommendation)
    : AppResult(profile, app_id, controller, is_recommendation) {
  set_id(app_id);
  SetResultType(ResultType::kInternalApp);
  SetIcon(
      GetIconForResourceId(GetIconResourceIdByAppId(app_id), kTileIconSize));
}

InternalAppResult::~InternalAppResult() {}

void InternalAppResult::ExecuteLaunchCommand(int event_flags) {
  Open(event_flags);
}

void InternalAppResult::Open(int event_flags) {
  // Record the search metric if the result is not a suggested app.
  if (display_type() != DisplayType::kRecommendation)
    RecordHistogram(APP_SEARCH_RESULT);

  OpenInternalApp(id(), profile());
}

void InternalAppResult::GetContextMenuModel(GetMenuModelCallback callback) {
  const auto* internal_app = app_list::FindInternalApp(id());
  DCHECK(internal_app);
  if (!internal_app->show_in_launcher) {
    std::move(callback).Run(nullptr);
    return;
  }

  if (!context_menu_) {
    context_menu_ = std::make_unique<AppContextMenu>(nullptr, profile(), id(),
                                                     controller());
  }
  context_menu_->GetMenuModel(std::move(callback));
}

AppContextMenu* InternalAppResult::GetAppContextMenu() {
  return context_menu_.get();
}

}  // namespace app_list
