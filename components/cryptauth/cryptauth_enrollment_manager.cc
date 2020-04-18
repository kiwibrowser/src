// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/cryptauth_enrollment_manager.h"

#include "components/cryptauth/pref_names.h"
#include "components/prefs/pref_registry_simple.h"

namespace cryptauth {

// static
void CryptAuthEnrollmentManager::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(
      prefs::kCryptAuthEnrollmentIsRecoveringFromFailure, false);
  registry->RegisterDoublePref(
      prefs::kCryptAuthEnrollmentLastEnrollmentTimeSeconds, 0.0);
  registry->RegisterIntegerPref(prefs::kCryptAuthEnrollmentReason,
                                INVOCATION_REASON_UNKNOWN);
  registry->RegisterStringPref(prefs::kCryptAuthEnrollmentUserPublicKey,
                               std::string());
  registry->RegisterStringPref(prefs::kCryptAuthEnrollmentUserPrivateKey,
                               std::string());
}

CryptAuthEnrollmentManager::CryptAuthEnrollmentManager() = default;

CryptAuthEnrollmentManager::~CryptAuthEnrollmentManager() = default;

void CryptAuthEnrollmentManager::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void CryptAuthEnrollmentManager::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void CryptAuthEnrollmentManager::NotifyEnrollmentStarted() {
  for (auto& observer : observers_)
    observer.OnEnrollmentStarted();
}

void CryptAuthEnrollmentManager::NotifyEnrollmentFinished(bool success) {
  for (auto& observer : observers_)
    observer.OnEnrollmentFinished(success);
}

}  // namespace cryptauth
