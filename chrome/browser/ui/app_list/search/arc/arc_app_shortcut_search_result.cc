// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/arc/arc_app_shortcut_search_result.h"

#include <string>
#include <utility>

#include "ash/public/cpp/app_list/app_list_constants.h"
#include "ash/public/cpp/app_list/app_list_types.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/arc/icon_decode_request.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/app_list_controller_delegate.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "ui/gfx/image/image_skia_operations.h"

namespace app_list {

namespace {
constexpr char kAppShortcutSearchPrefix[] = "appshortcutsearch://";
}  // namespace

ArcAppShortcutSearchResult::ArcAppShortcutSearchResult(
    arc::mojom::AppShortcutItemPtr data,
    Profile* profile,
    AppListControllerDelegate* list_controller)
    : data_(std::move(data)),
      profile_(profile),
      list_controller_(list_controller) {
  SetTitle(base::UTF8ToUTF16(data_->short_label));
  set_id(kAppShortcutSearchPrefix + GetAppId() + "/" + data_->shortcut_id);
  SetDisplayType(ash::SearchResultDisplayType::kTile);

  icon_decode_request_ = std::make_unique<arc::IconDecodeRequest>(
      base::BindOnce(&ArcAppShortcutSearchResult::SetIcon,
                     base::Unretained(this)),
      kGridIconDimension);
  icon_decode_request_->StartWithOptions(data_->icon_png);

  badge_icon_loader_ =
      std::make_unique<ArcAppIconLoader>(profile_, kGridIconDimension, this);
  badge_icon_loader_->FetchImage(GetAppId());
}

ArcAppShortcutSearchResult::~ArcAppShortcutSearchResult() = default;

void ArcAppShortcutSearchResult::Open(int event_flags) {
  arc::LaunchAppShortcutItem(profile_, GetAppId(), data_->shortcut_id,
                             list_controller_->GetAppListDisplayId());
}

void ArcAppShortcutSearchResult::OnAppImageUpdated(
    const std::string& app_id,
    const gfx::ImageSkia& image) {
  SetBadgeIcon(gfx::ImageSkiaOperations::CreateResizedImage(
      image, skia::ImageOperations::RESIZE_BEST,
      gfx::Size(kAppBadgeIconSize, kAppBadgeIconSize)));
}

std::string ArcAppShortcutSearchResult::GetAppId() const {
  if (!data_->package_name)
    return std::string();
  const ArcAppListPrefs* arc_prefs = ArcAppListPrefs::Get(profile_);
  DCHECK(arc_prefs);
  return arc_prefs->GetAppIdByPackageName(data_->package_name.value());
}

}  // namespace app_list
