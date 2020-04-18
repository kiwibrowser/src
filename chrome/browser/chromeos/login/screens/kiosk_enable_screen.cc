// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/kiosk_enable_screen.h"

#include "base/logging.h"
#include "chrome/browser/chromeos/customization/customization_document.h"
#include "chrome/browser/chromeos/login/screens/base_screen_delegate.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"

namespace chromeos {

KioskEnableScreen::KioskEnableScreen(BaseScreenDelegate* base_screen_delegate,
                                     KioskEnableScreenView* view)
    : BaseScreen(base_screen_delegate, OobeScreen::SCREEN_KIOSK_ENABLE),
      view_(view) {
  DCHECK(view_);
  if (view_)
    view_->SetDelegate(this);
}

KioskEnableScreen::~KioskEnableScreen() {
  if (view_)
    view_->SetDelegate(NULL);
}

void KioskEnableScreen::Show() {
  if (view_)
    view_->Show();
}

void KioskEnableScreen::OnExit() {
  Finish(ScreenExitCode::KIOSK_ENABLE_COMPLETED);
}

void KioskEnableScreen::OnViewDestroyed(KioskEnableScreenView* view) {
  if (view_ == view)
    view_ = NULL;
}

}  // namespace chromeos
