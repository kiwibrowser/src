// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/webapks_handler.h"

#include <string>

#include "base/callback_forward.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "chrome/browser/android/color_helpers.h"
#include "chrome/browser/android/shortcut_helper.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/manifest_util.h"
#include "ui/gfx/color_utils.h"

WebApksHandler::WebApksHandler() : weak_ptr_factory_(this) {}

WebApksHandler::~WebApksHandler() {}

void WebApksHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "requestWebApksInfo",
      base::BindRepeating(&WebApksHandler::HandleRequestWebApksInfo,
                          base::Unretained(this)));
}

void WebApksHandler::HandleRequestWebApksInfo(const base::ListValue* args) {
  AllowJavascript();
  ShortcutHelper::RetrieveWebApks(base::Bind(
      &WebApksHandler::OnWebApkInfoRetrieved, weak_ptr_factory_.GetWeakPtr()));
}

void WebApksHandler::OnWebApkInfoRetrieved(
    const std::vector<WebApkInfo>& webapks_list) {
  if (!IsJavascriptAllowed())
    return;
  base::ListValue list;
  for (const auto& webapk_info : webapks_list) {
    std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());
    result->SetString("name", webapk_info.name);
    result->SetString("shortName", webapk_info.short_name);
    result->SetString("packageName", webapk_info.package_name);
    result->SetInteger("shellApkVersion", webapk_info.shell_apk_version);
    result->SetInteger("versionCode", webapk_info.version_code);
    result->SetString("uri", webapk_info.uri);
    result->SetString("scope", webapk_info.scope);
    result->SetString("manifestUrl", webapk_info.manifest_url);
    result->SetString("manifestStartUrl", webapk_info.manifest_start_url);
    result->SetString("displayMode",
                      content::WebDisplayModeToString(webapk_info.display));
    result->SetString(
        "orientation",
        content::WebScreenOrientationLockTypeToString(webapk_info.orientation));
    result->SetString("themeColor",
                      OptionalSkColorToString(webapk_info.theme_color));
    result->SetString("backgroundColor",
                      OptionalSkColorToString(webapk_info.background_color));
    result->SetDouble("lastUpdateCheckTimeMs",
                      webapk_info.last_update_check_time.ToJsTime());
    result->SetBoolean("relaxUpdates", webapk_info.relax_updates);
    list.Append(std::move(result));
  }

  CallJavascriptFunction("returnWebApksInfo", list);
}
