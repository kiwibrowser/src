// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/quick_unlock/quick_unlock_storage.h"

#include <memory>

#include "base/time/time.h"
#include "chrome/browser/chromeos/login/quick_unlock/quick_unlock_utils.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace chromeos {
namespace quick_unlock {

QuickUnlockStorage::QuickUnlockStorage(PrefService* pref_service)
    : pref_service_(pref_service) {
  fingerprint_storage_ = std::make_unique<FingerprintStorage>(pref_service);
  pin_storage_prefs_ = std::make_unique<PinStoragePrefs>(pref_service);
}

QuickUnlockStorage::~QuickUnlockStorage() {}

void QuickUnlockStorage::MarkStrongAuth() {
  last_strong_auth_ = base::TimeTicks::Now();
  fingerprint_storage()->ResetUnlockAttemptCount();
  pin_storage_prefs()->ResetUnlockAttemptCount();
}

bool QuickUnlockStorage::HasStrongAuth() const {
  if (last_strong_auth_.is_null())
    return false;

  // PIN and fingerprint share the same timeout policy.
  PasswordConfirmationFrequency strong_auth_interval =
      static_cast<PasswordConfirmationFrequency>(
          pref_service_->GetInteger(prefs::kQuickUnlockTimeout));
  base::TimeDelta strong_auth_timeout =
      PasswordConfirmationFrequencyToTimeDelta(strong_auth_interval);

  return TimeSinceLastStrongAuth() < strong_auth_timeout;
}

base::TimeDelta QuickUnlockStorage::TimeSinceLastStrongAuth() const {
  DCHECK(!last_strong_auth_.is_null());
  return base::TimeTicks::Now() - last_strong_auth_;
}

bool QuickUnlockStorage::IsFingerprintAuthenticationAvailable() const {
  return HasStrongAuth() &&
         fingerprint_storage_->IsFingerprintAuthenticationAvailable();
}

bool QuickUnlockStorage::IsPinAuthenticationAvailable() const {
  return HasStrongAuth() && pin_storage_prefs_->IsPinAuthenticationAvailable();
}

bool QuickUnlockStorage::TryAuthenticatePin(const Key& key) {
  return HasStrongAuth() && pin_storage_prefs()->TryAuthenticatePin(key);
}

std::string QuickUnlockStorage::CreateAuthToken(
    const chromeos::UserContext& user_context) {
  auth_token_ = std::make_unique<AuthToken>(user_context);
  DCHECK(auth_token_->Identifier().has_value());
  return *auth_token_->Identifier();
}

bool QuickUnlockStorage::GetAuthTokenExpired() {
  return !auth_token_ || !auth_token_->Identifier().has_value();
}

std::string QuickUnlockStorage::GetAuthToken() {
  if (GetAuthTokenExpired())
    return "";
  return *auth_token_->Identifier();
}

UserContext* QuickUnlockStorage::GetUserContext(const std::string& auth_token) {
  if (GetAuthToken() != auth_token)
    return nullptr;
  return auth_token_->user_context();
}

void QuickUnlockStorage::Shutdown() {
  fingerprint_storage_.reset();
  pin_storage_prefs_.reset();
}

}  // namespace quick_unlock
}  // namespace chromeos
