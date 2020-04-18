// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/wait_for_container_ready_screen.h"

#include "chrome/browser/chromeos/login/screens/base_screen_delegate.h"
#include "chrome/browser/chromeos/login/screens/wait_for_container_ready_screen_view.h"

namespace chromeos {

WaitForContainerReadyScreen::WaitForContainerReadyScreen(
    BaseScreenDelegate* base_screen_delegate,
    WaitForContainerReadyScreenView* view)
    : BaseScreen(base_screen_delegate,
                 OobeScreen::SCREEN_WAIT_FOR_CONTAINER_READY),
      view_(view) {
  DCHECK(view_);
  if (view_)
    view_->Bind(this);
}

WaitForContainerReadyScreen::~WaitForContainerReadyScreen() {
  if (view_)
    view_->Unbind();
}

void WaitForContainerReadyScreen::Show() {
  if (!view_)
    return;

  view_->Show();
}

void WaitForContainerReadyScreen::Hide() {
  if (view_)
    view_->Hide();
}

void WaitForContainerReadyScreen::OnViewDestroyed(
    WaitForContainerReadyScreenView* view) {
  if (view_ == view)
    view_ = nullptr;
}

void WaitForContainerReadyScreen::OnContainerReady() {
  Finish(ScreenExitCode::WAIT_FOR_CONTAINER_READY_FINISHED);
}

void WaitForContainerReadyScreen::OnContainerError() {
  Finish(ScreenExitCode::WAIT_FOR_CONTAINER_READY_ERROR);
}

}  // namespace chromeos
