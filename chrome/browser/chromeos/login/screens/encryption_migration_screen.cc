// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/encryption_migration_screen.h"

#include <utility>

#include "base/logging.h"
#include "chrome/browser/chromeos/login/screens/base_screen_delegate.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"

namespace chromeos {

EncryptionMigrationScreen::EncryptionMigrationScreen(
    BaseScreenDelegate* base_screen_delegate,
    EncryptionMigrationScreenView* view)
    : BaseScreen(base_screen_delegate, OobeScreen::SCREEN_ENCRYPTION_MIGRATION),
      view_(view) {
  DCHECK(view_);
  if (view_)
    view_->SetDelegate(this);
}

EncryptionMigrationScreen::~EncryptionMigrationScreen() {
  if (view_)
    view_->SetDelegate(nullptr);
}

void EncryptionMigrationScreen::Show() {
  if (view_)
    view_->Show();
}

void EncryptionMigrationScreen::Hide() {
  if (view_)
    view_->Hide();
}

void EncryptionMigrationScreen::OnExit() {
  Finish(ScreenExitCode::ENCRYPTION_MIGRATION_FINISHED);
}

void EncryptionMigrationScreen::OnViewDestroyed(
    EncryptionMigrationScreenView* view) {
  if (view_ == view)
    view_ = nullptr;
}

void EncryptionMigrationScreen::SetUserContext(
    const UserContext& user_context) {
  DCHECK(view_);
  view_->SetUserContext(user_context);
}

void EncryptionMigrationScreen::SetMode(EncryptionMigrationMode mode) {
  DCHECK(view_);
  view_->SetMode(mode);
}

void EncryptionMigrationScreen::SetContinueLoginCallback(
    ContinueLoginCallback callback) {
  DCHECK(view_);
  view_->SetContinueLoginCallback(std::move(callback));
}

void EncryptionMigrationScreen::SetRestartLoginCallback(
    RestartLoginCallback callback) {
  DCHECK(view_);
  view_->SetRestartLoginCallback(std::move(callback));
}

void EncryptionMigrationScreen::SetupInitialView() {
  DCHECK(view_);
  view_->SetupInitialView();
}

}  // namespace chromeos
