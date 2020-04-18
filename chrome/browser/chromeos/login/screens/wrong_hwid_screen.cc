// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/wrong_hwid_screen.h"

#include "chrome/browser/chromeos/login/screens/base_screen_delegate.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"

namespace chromeos {

WrongHWIDScreen::WrongHWIDScreen(BaseScreenDelegate* base_screen_delegate,
                                 WrongHWIDScreenView* view)
    : BaseScreen(base_screen_delegate, OobeScreen::SCREEN_WRONG_HWID),
      view_(view) {
  DCHECK(view_);
  if (view_)
    view_->SetDelegate(this);
}

WrongHWIDScreen::~WrongHWIDScreen() {
  if (view_)
    view_->SetDelegate(nullptr);
}

void WrongHWIDScreen::Show() {
  if (view_)
    view_->Show();
}

void WrongHWIDScreen::Hide() {
  if (view_)
    view_->Hide();
}

void WrongHWIDScreen::OnExit() {
  Finish(ScreenExitCode::WRONG_HWID_WARNING_SKIPPED);
}

void WrongHWIDScreen::OnViewDestroyed(WrongHWIDScreenView* view) {
  if (view_ == view)
    view_ = nullptr;
}

}  // namespace chromeos
