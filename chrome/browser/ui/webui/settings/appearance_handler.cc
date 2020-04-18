// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/settings/appearance_handler.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "content/public/browser/web_ui.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/ui/ash/wallpaper_controller_client.h"
#endif

namespace settings {

AppearanceHandler::AppearanceHandler(content::WebUI* webui)
    : profile_(Profile::FromWebUI(webui)), weak_ptr_factory_(this) {}

AppearanceHandler::~AppearanceHandler() {}

void AppearanceHandler::OnJavascriptAllowed() {}
void AppearanceHandler::OnJavascriptDisallowed() {}

void AppearanceHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "useDefaultTheme",
      base::BindRepeating(&AppearanceHandler::HandleUseDefaultTheme,
                          base::Unretained(this)));
#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
  web_ui()->RegisterMessageCallback(
      "useSystemTheme",
      base::BindRepeating(&AppearanceHandler::HandleUseSystemTheme,
                          base::Unretained(this)));
#endif
#if defined(OS_CHROMEOS)
  web_ui()->RegisterMessageCallback(
      "openWallpaperManager",
      base::BindRepeating(&AppearanceHandler::HandleOpenWallpaperManager,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "isWallpaperSettingVisible",
      base::BindRepeating(&AppearanceHandler::IsWallpaperSettingVisible,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "isWallpaperPolicyControlled",
      base::BindRepeating(&AppearanceHandler::IsWallpaperPolicyControlled,
                          base::Unretained(this)));
#endif
}

void AppearanceHandler::HandleUseDefaultTheme(const base::ListValue* args) {
  ThemeServiceFactory::GetForProfile(profile_)->UseDefaultTheme();
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
void AppearanceHandler::HandleUseSystemTheme(const base::ListValue* args) {
  if (profile_->IsSupervised())
    NOTREACHED();
  else
    ThemeServiceFactory::GetForProfile(profile_)->UseSystemTheme();
}
#endif

#if defined(OS_CHROMEOS)
void AppearanceHandler::IsWallpaperSettingVisible(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  WallpaperControllerClient::Get()->ShouldShowWallpaperSetting(
      base::Bind(&AppearanceHandler::ResolveCallback,
                 weak_ptr_factory_.GetWeakPtr(), args->GetList()[0].Clone()));
}

void AppearanceHandler::IsWallpaperPolicyControlled(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  WallpaperControllerClient::Get()->IsActiveUserWallpaperControlledByPolicy(
      base::Bind(&AppearanceHandler::ResolveCallback,
                 weak_ptr_factory_.GetWeakPtr(), args->GetList()[0].Clone()));
}

void AppearanceHandler::HandleOpenWallpaperManager(
    const base::ListValue* args) {
  WallpaperControllerClient::Get()->OpenWallpaperPickerIfAllowed();
}

void AppearanceHandler::ResolveCallback(const base::Value& callback_id,
                                        bool result) {
  AllowJavascript();
  ResolveJavascriptCallback(callback_id, base::Value(result));
}
#endif

}  // namespace settings
