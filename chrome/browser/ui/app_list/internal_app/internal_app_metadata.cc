// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/internal_app/internal_app_metadata.h"

#include "ash/public/cpp/app_list/internal_app_id_constants.h"
#include "ash/public/cpp/resources/grit/ash_public_unscaled_resources.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/ash/ksv/keyboard_shortcut_viewer_util.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image_skia_operations.h"

namespace app_list {

const std::vector<InternalApp>& GetInternalAppList() {
  static const base::NoDestructor<std::vector<InternalApp>> internal_app_list(
      {{kInternalAppIdKeyboardShortcutViewer,
        IDS_INTERNAL_APP_KEYBOARD_SHORTCUT_VIEWER, IDR_SHORTCUT_VIEWER_LOGO_192,
        /*recommendable=*/false,
        /*show_in_launcher=*/false,
        IDS_LAUNCHER_SEARCHABLE_KEYBOARD_SHORTCUT_VIEWER},

       {kInternalAppIdSettings, IDS_INTERNAL_APP_SETTINGS,
        IDR_SETTINGS_LOGO_192,
        /*recommendable=*/true,
        /*show_in_launcher=*/true,
        /*searchable_string_resource_id=*/0}});
  return *internal_app_list;
}

const InternalApp* FindInternalApp(const std::string& app_id) {
  for (const auto& app : GetInternalAppList()) {
    if (app_id == app.app_id)
      return &app;
  }
  return nullptr;
}

bool IsInternalApp(const std::string& app_id) {
  return !!FindInternalApp(app_id);
}

base::string16 GetInternalAppNameById(const std::string& app_id) {
  const auto* app = FindInternalApp(app_id);
  return app ? l10n_util::GetStringUTF16(app->name_string_resource_id)
             : base::string16();
}

int GetIconResourceIdByAppId(const std::string& app_id) {
  const auto* app = FindInternalApp(app_id);
  return app ? app->icon_resource_id : 0;
}

void OpenInternalApp(const std::string& app_id, Profile* profile) {
  if (app_id == kInternalAppIdKeyboardShortcutViewer) {
    keyboard_shortcut_viewer_util::ShowKeyboardShortcutViewer();
  } else if (app_id == kInternalAppIdSettings) {
    chrome::ShowSettingsSubPageForProfile(profile, std::string());
  }
}

gfx::ImageSkia GetIconForResourceId(int resource_id, int resource_size_in_dip) {
  if (resource_id == 0)
    return gfx::ImageSkia();

  gfx::ImageSkia* source =
      ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(resource_id);
  return gfx::ImageSkiaOperations::CreateResizedImage(
      *source, skia::ImageOperations::RESIZE_BEST,
      gfx::Size(resource_size_in_dip, resource_size_in_dip));
}

size_t GetNumberOfInternalAppsShowInLauncherForTest(std::string* apps_name) {
  size_t num_of_internal_apps_show_in_launcher = 0u;
  std::vector<std::string> internal_apps_name;
  for (const auto& app : GetInternalAppList()) {
    if (app.show_in_launcher) {
      ++num_of_internal_apps_show_in_launcher;
      if (apps_name) {
        internal_apps_name.emplace_back(
            l10n_util::GetStringUTF8(app.name_string_resource_id));
      }
    }
  }
  if (apps_name)
    *apps_name = base::JoinString(internal_apps_name, ",");
  return num_of_internal_apps_show_in_launcher;
}

}  // namespace app_list
