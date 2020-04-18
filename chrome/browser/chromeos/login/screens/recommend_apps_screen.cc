// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/recommend_apps_screen.h"

namespace chromeos {
namespace {

constexpr const char kUserActionSkip[] = "recommendAppsSkip";

}  // namespace

RecommendAppsScreen::RecommendAppsScreen(
    BaseScreenDelegate* base_screen_delegate,
    RecommendAppsScreenView* view)
    : BaseScreen(base_screen_delegate, OobeScreen::SCREEN_RECOMMEND_APPS),
      view_(view) {
  DCHECK(view_);
  view_->Bind(this);
}

RecommendAppsScreen::~RecommendAppsScreen() {
  view_->Bind(NULL);
}

void RecommendAppsScreen::Show() {
  view_->Show();
}

void RecommendAppsScreen::Hide() {
  view_->Hide();
}

void RecommendAppsScreen::OnUserAction(const std::string& action_id) {
  if (action_id == kUserActionSkip) {
    Finish(ScreenExitCode::RECOMMEND_APPS_SKIPPED);
    return;
  }
  BaseScreen::OnUserAction(action_id);
}

}  // namespace chromeos
