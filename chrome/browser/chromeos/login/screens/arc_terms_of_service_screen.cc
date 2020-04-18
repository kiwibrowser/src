// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/arc_terms_of_service_screen.h"

#include "chrome/browser/chromeos/login/screens/arc_terms_of_service_screen_view.h"
#include "chrome/browser/chromeos/login/screens/base_screen_delegate.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/metrics/metrics_reporting_state.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace chromeos {

ArcTermsOfServiceScreen::ArcTermsOfServiceScreen(
    BaseScreenDelegate* base_screen_delegate,
    ArcTermsOfServiceScreenView* view)
    : BaseScreen(base_screen_delegate, OobeScreen::SCREEN_ARC_TERMS_OF_SERVICE),
      view_(view) {
  DCHECK(view_);
  if (view_)
    view_->AddObserver(this);
}

ArcTermsOfServiceScreen::~ArcTermsOfServiceScreen() {
  if (view_)
    view_->RemoveObserver(this);
}

void ArcTermsOfServiceScreen::Show() {
  if (!view_)
    return;

  // Show the screen.
  view_->Show();
}

void ArcTermsOfServiceScreen::Hide() {
  if (view_)
    view_->Hide();
}

void ArcTermsOfServiceScreen::OnSkip() {
  Finish(ScreenExitCode::ARC_TERMS_OF_SERVICE_SKIPPED);
}

void ArcTermsOfServiceScreen::OnAccept() {
  Finish(ScreenExitCode::ARC_TERMS_OF_SERVICE_ACCEPTED);
}

void ArcTermsOfServiceScreen::OnViewDestroyed(
    ArcTermsOfServiceScreenView* view) {
  DCHECK_EQ(view, view_);
  view_->RemoveObserver(this);
  view_ = nullptr;
}

}  // namespace chromeos
