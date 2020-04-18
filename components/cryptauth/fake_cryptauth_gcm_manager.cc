// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/fake_cryptauth_gcm_manager.h"

namespace cryptauth {

FakeCryptAuthGCMManager::FakeCryptAuthGCMManager(
    const std::string& registration_id)
    : registration_id_(registration_id) {}

FakeCryptAuthGCMManager::~FakeCryptAuthGCMManager() = default;

void FakeCryptAuthGCMManager::StartListening() {
  has_started_listening_ = true;
}

void FakeCryptAuthGCMManager::RegisterWithGCM() {
  registration_in_progress_ = true;
}

std::string FakeCryptAuthGCMManager::GetRegistrationId() {
  return registration_id_;
}

void FakeCryptAuthGCMManager::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void FakeCryptAuthGCMManager::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void FakeCryptAuthGCMManager::CompleteRegistration(
    const std::string& registration_id) {
  DCHECK(registration_in_progress_);
  registration_in_progress_ = false;
  registration_id_ = registration_id;
  bool success = !registration_id_.empty();
  for (auto& observer : observers_)
    observer.OnGCMRegistrationResult(success);
}

void FakeCryptAuthGCMManager::PushReenrollMessage() {
  for (auto& observer : observers_)
    observer.OnReenrollMessage();
}

void FakeCryptAuthGCMManager::PushResyncMessage() {
  for (auto& observer : observers_)
    observer.OnResyncMessage();
}

}  // namespace cryptauth
