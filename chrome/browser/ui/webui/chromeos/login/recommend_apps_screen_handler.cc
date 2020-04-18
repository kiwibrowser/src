// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/recommend_apps_screen_handler.h"

#include "chrome/browser/chromeos/login/screens/recommend_apps_screen.h"
#include "chrome/grit/generated_resources.h"
#include "components/login/localized_values_builder.h"

namespace {

const char kJsScreenPath[] = "login.RecommendAppsScreen";

}  // namespace

namespace chromeos {

RecommendAppsScreenHandler::RecommendAppsScreenHandler()
    : BaseScreenHandler(kScreenId) {
  set_call_js_prefix(kJsScreenPath);
}

RecommendAppsScreenHandler::~RecommendAppsScreenHandler() {}

void RecommendAppsScreenHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {
  builder->Add("recommendAppsScreenTitle",
               IDS_LOGIN_RECOMMEND_APPS_SCREEN_TITLE);
  builder->Add("recommendAppsScreenDescription",
               IDS_LOGIN_RECOMMEND_APPS_SCREEN_DESCRIPTION);
  builder->Add("recommendAppsSkip", IDS_LOGIN_RECOMMEND_APPS_SKIP);
  builder->Add("recommendAppsInstall", IDS_LOGIN_RECOMMEND_APPS_INSTALL);
}

void RecommendAppsScreenHandler::RegisterMessages() {
  BaseScreenHandler::RegisterMessages();
}

void RecommendAppsScreenHandler::Bind(RecommendAppsScreen* screen) {
  screen_ = screen;
  BaseScreenHandler::SetBaseScreen(screen);
}

void RecommendAppsScreenHandler::Show() {
  ShowScreen(kScreenId);
}

void RecommendAppsScreenHandler::Hide() {}

void RecommendAppsScreenHandler::Initialize() {}

}  // namespace chromeos
