// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/passwords/ios_chrome_password_manager_driver.h"

#include "base/strings/string16.h"
#include "components/autofill/core/common/password_form.h"
#include "components/autofill/core/common/password_form_fill_data.h"
#include "components/password_manager/core/browser/password_manager.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using password_manager::PasswordAutofillManager;
using password_manager::PasswordGenerationManager;
using password_manager::PasswordManager;

IOSChromePasswordManagerDriver::IOSChromePasswordManagerDriver(
    id<PasswordManagerDriverDelegate> delegate)
    : delegate_(delegate) {}

IOSChromePasswordManagerDriver::~IOSChromePasswordManagerDriver() = default;

void IOSChromePasswordManagerDriver::FillPasswordForm(
    const autofill::PasswordFormFillData& form_data) {
  [delegate_ fillPasswordForm:form_data completionHandler:nil];
}

void IOSChromePasswordManagerDriver::InformNoSavedCredentials() {
  [delegate_ onNoSavedCredentials];
}

void IOSChromePasswordManagerDriver::FormsEligibleForGenerationFound(
    const std::vector<autofill::PasswordFormGenerationData>& forms) {
  // TODO(crbug.com/569579): Implement.
}

void IOSChromePasswordManagerDriver::GeneratedPasswordAccepted(
    const base::string16& password) {
  NOTIMPLEMENTED();
}

void IOSChromePasswordManagerDriver::FillSuggestion(
    const base::string16& username,
    const base::string16& password) {
  NOTIMPLEMENTED();
}

void IOSChromePasswordManagerDriver::PreviewSuggestion(
    const base::string16& username,
    const base::string16& password) {
  NOTIMPLEMENTED();
}

// TODO(crbug.com/568713): This method should be given a non-trivial
// implementation before launch of the fill-on-account password manager
// experiment.
void IOSChromePasswordManagerDriver::ShowInitialPasswordAccountSuggestions(
    const autofill::PasswordFormFillData& form_data) {}

void IOSChromePasswordManagerDriver::ClearPreviewedForm() {
  NOTIMPLEMENTED();
}

PasswordGenerationManager*
IOSChromePasswordManagerDriver::GetPasswordGenerationManager() {
  return nullptr;
}

PasswordManager* IOSChromePasswordManagerDriver::GetPasswordManager() {
  return [delegate_ passwordManager];
}

void IOSChromePasswordManagerDriver::AllowPasswordGenerationForForm(
    const autofill::PasswordForm& form) {
}

PasswordAutofillManager*
IOSChromePasswordManagerDriver::GetPasswordAutofillManager() {
  // TODO(crbug.com/341877): Use PasswordAutofillManager to implement password
  // autofill on iOS.
  return nullptr;
}

void IOSChromePasswordManagerDriver::ForceSavePassword() {
  NOTIMPLEMENTED();
}

autofill::AutofillDriver* IOSChromePasswordManagerDriver::GetAutofillDriver() {
  NOTIMPLEMENTED();
  return nullptr;
}

bool IOSChromePasswordManagerDriver::IsMainFrame() const {
  // On IOS only processing of password forms in main frame is implemented.
  return true;
}

void IOSChromePasswordManagerDriver::MatchingBlacklistedFormFound() {
  NOTIMPLEMENTED();
}

void IOSChromePasswordManagerDriver::UserSelectedManualGenerationOption() {
  NOTIMPLEMENTED();
}
