// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/kiosk_autolaunch_screen.h"

#include "base/logging.h"
#include "chrome/browser/chromeos/customization/customization_document.h"
#include "chrome/browser/chromeos/login/screens/base_screen_delegate.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"

namespace chromeos {

KioskAutolaunchScreen::KioskAutolaunchScreen(
    BaseScreenDelegate* base_screen_delegate,
    KioskAutolaunchScreenView* view)
    : BaseScreen(base_screen_delegate, OobeScreen::SCREEN_KIOSK_AUTOLAUNCH),
      view_(view) {
  DCHECK(view_);
  if (view_)
    view_->SetDelegate(this);
}

KioskAutolaunchScreen::~KioskAutolaunchScreen() {
  if (view_)
    view_->SetDelegate(NULL);
}

void KioskAutolaunchScreen::Show() {
  if (view_)
    view_->Show();
}

void KioskAutolaunchScreen::OnExit(bool confirmed) {
  Finish(confirmed ? ScreenExitCode::KIOSK_AUTOLAUNCH_CONFIRMED
                   : ScreenExitCode::KIOSK_AUTOLAUNCH_CANCELED);
}

void KioskAutolaunchScreen::OnViewDestroyed(KioskAutolaunchScreenView* view) {
  if (view_ == view)
    view_ = NULL;
}

}  // namespace chromeos
