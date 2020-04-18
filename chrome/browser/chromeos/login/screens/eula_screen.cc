// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/eula_screen.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/customization/customization_document.h"
#include "chrome/browser/chromeos/login/screens/base_screen_delegate.h"
#include "chrome/browser/chromeos/login/screens/eula_view.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "chromeos/dbus/dbus_method_call_status.h"
#include "chromeos/dbus/dbus_thread_manager.h"

namespace chromeos {
namespace {

constexpr const char kUserActionAcceptButtonClicked[] = "accept-button";
constexpr const char kUserActionBackButtonClicked[] = "back-button";
constexpr const char kContextKeyUsageStatsEnabled[] = "usageStatsEnabled";

}  // namespace

EulaScreen::EulaScreen(BaseScreenDelegate* base_screen_delegate,
                       Delegate* delegate,
                       EulaView* view)
    : BaseScreen(base_screen_delegate, OobeScreen::SCREEN_OOBE_EULA),
      delegate_(delegate),
      view_(view),
      password_fetcher_(this) {
  DCHECK(view_);
  DCHECK(delegate_);
  if (view_)
    view_->Bind(this);
}

EulaScreen::~EulaScreen() {
  if (view_)
    view_->Unbind();
}

GURL EulaScreen::GetOemEulaUrl() const {
  const StartupCustomizationDocument* customization =
      StartupCustomizationDocument::GetInstance();
  if (customization->IsReady()) {
    // Previously we're using "initial locale" that device initially
    // booted with out-of-box. http://crbug.com/145142
    std::string locale = g_browser_process->GetApplicationLocale();
    std::string eula_page = customization->GetEULAPage(locale);
    if (!eula_page.empty())
      return GURL(eula_page);

    VLOG(1) << "No eula found for locale: " << locale;
  } else {
    LOG(ERROR) << "No manifest found.";
  }
  return GURL();
}

void EulaScreen::InitiatePasswordFetch() {
  if (tpm_password_.empty()) {
    password_fetcher_.Fetch();
    // Will call view after password has been fetched.
  } else if (view_) {
    view_->OnPasswordFetched(tpm_password_);
  }
}

bool EulaScreen::IsUsageStatsEnabled() const {
  return delegate_ && delegate_->GetUsageStatisticsReporting();
}

void EulaScreen::OnViewDestroyed(EulaView* view) {
  if (view_ == view)
    view_ = NULL;
}

void EulaScreen::Show() {
  // Command to own the TPM.
  DBusThreadManager::Get()->GetCryptohomeClient()->TpmCanAttemptOwnership(
      EmptyVoidDBusMethodCallback());
  if (WizardController::UsingHandsOffEnrollment())
    OnUserAction(kUserActionAcceptButtonClicked);
  else if (view_)
    view_->Show();
}

void EulaScreen::Hide() {
  if (view_)
    view_->Hide();
}

void EulaScreen::OnUserAction(const std::string& action_id) {
  if (action_id == kUserActionAcceptButtonClicked)
    Finish(ScreenExitCode::EULA_ACCEPTED);
  else if (action_id == kUserActionBackButtonClicked)
    Finish(ScreenExitCode::EULA_BACK);
  else
    BaseScreen::OnUserAction(action_id);
}

void EulaScreen::OnContextKeyUpdated(
    const ::login::ScreenContext::KeyType& key) {
  if (key == kContextKeyUsageStatsEnabled && delegate_) {
    delegate_->SetUsageStatisticsReporting(
        context_.GetBoolean(kContextKeyUsageStatsEnabled));
  }
}

void EulaScreen::OnPasswordFetched(const std::string& tpm_password) {
  tpm_password_ = tpm_password;
  if (view_)
    view_->OnPasswordFetched(tpm_password_);
}

}  // namespace chromeos
