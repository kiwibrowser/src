// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/fake_hammerd_client.h"

namespace chromeos {

FakeHammerdClient::FakeHammerdClient() = default;

FakeHammerdClient::~FakeHammerdClient() = default;

void FakeHammerdClient::Init(dbus::Bus* bus) {}

void FakeHammerdClient::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void FakeHammerdClient::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void FakeHammerdClient::FireBaseFirmwareNeedUpdateSignal() {
  for (auto& observer : observers_)
    observer.BaseFirmwareUpdateNeeded();
}

void FakeHammerdClient::FireBaseFirmwareUpdateStartedSignal() {
  for (auto& observer : observers_)
    observer.BaseFirmwareUpdateStarted();
}

void FakeHammerdClient::FireBaseFirmwareUpdateSucceededSignal() {
  for (auto& observer : observers_)
    observer.BaseFirmwareUpdateSucceeded();
}

void FakeHammerdClient::FireBaseFirmwareUpdateFailedSignal() {
  for (auto& observer : observers_)
    observer.BaseFirmwareUpdateFailed();
}

void FakeHammerdClient::FirePairChallengeSucceededSignal(
    const std::vector<uint8_t>& base_id) {
  for (auto& observer : observers_)
    observer.PairChallengeSucceeded(base_id);
}

void FakeHammerdClient::FirePairChallengeFailedSignal() {
  for (auto& observer : observers_)
    observer.PairChallengeFailed();
}

void FakeHammerdClient::FireInvalidBaseConnectedSignal() {
  for (auto& observer : observers_)
    observer.InvalidBaseConnected();
}

}  // namespace chromeos
