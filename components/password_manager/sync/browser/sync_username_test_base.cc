// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/sync/browser/sync_username_test_base.h"

#include "base/strings/utf_string_conversions.h"
#include "components/signin/core/browser/signin_pref_names.h"

using autofill::PasswordForm;

namespace password_manager {

SyncUsernameTestBase::LocalFakeSyncService::LocalFakeSyncService()
    : syncing_passwords_(true) {}

SyncUsernameTestBase::LocalFakeSyncService::~LocalFakeSyncService() {}

syncer::ModelTypeSet
SyncUsernameTestBase::LocalFakeSyncService::GetPreferredDataTypes() const {
  if (syncing_passwords_)
    return syncer::ModelTypeSet(syncer::PASSWORDS);
  return syncer::ModelTypeSet();
}

SyncUsernameTestBase::SyncUsernameTestBase()
    : signin_client_(&prefs_),
      signin_manager_(&signin_client_, &account_tracker_) {
  SigninManagerBase::RegisterProfilePrefs(prefs_.registry());
  AccountTrackerService::RegisterPrefs(prefs_.registry());
  account_tracker_.Initialize(&signin_client_);
}

SyncUsernameTestBase::~SyncUsernameTestBase() {}

void SyncUsernameTestBase::FakeSigninAs(const std::string& email) {
  signin_manager_.SetAuthenticatedAccountInfo("12345", email);
}

void SyncUsernameTestBase::FakeSignout() {
  signin_manager_.ClearAuthenticatedAccountId();
  prefs_.SetString(prefs::kGoogleServicesAccountId, std::string());
}

// static
PasswordForm SyncUsernameTestBase::SimpleGaiaForm(const char* username) {
  PasswordForm form;
  form.signon_realm = "https://accounts.google.com";
  form.username_value = base::ASCIIToUTF16(username);
  return form;
}

// static
PasswordForm SyncUsernameTestBase::SimpleNonGaiaForm(const char* username) {
  PasswordForm form;
  form.signon_realm = "https://site.com";
  form.username_value = base::ASCIIToUTF16(username);
  return form;
}

// static
PasswordForm SyncUsernameTestBase::SimpleNonGaiaForm(const char* username,
                                                     const char* origin) {
  PasswordForm form;
  form.signon_realm = "https://site.com";
  form.username_value = base::ASCIIToUTF16(username);
  form.origin = GURL(origin);
  return form;
}

void SyncUsernameTestBase::SetSyncingPasswords(bool syncing_passwords) {
  sync_service_.set_syncing_passwords(syncing_passwords);
}

}  // namespace password_manager
