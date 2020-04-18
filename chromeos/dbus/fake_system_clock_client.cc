// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/fake_system_clock_client.h"
#include "base/bind.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace chromeos {

FakeSystemClockClient::FakeSystemClockClient() = default;

FakeSystemClockClient::~FakeSystemClockClient() = default;

void FakeSystemClockClient::NotifyObserversSystemClockUpdated() {
  for (auto& observer : observers_)
    observer.SystemClockUpdated();
}

void FakeSystemClockClient::Init(dbus::Bus* bus) {
}

void FakeSystemClockClient::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void FakeSystemClockClient::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool FakeSystemClockClient::HasObserver(const Observer* observer) const {
  return observers_.HasObserver(observer);
}

void FakeSystemClockClient::SetTime(int64_t time_in_seconds) {}

bool FakeSystemClockClient::CanSetTime() {
  return true;
}

void FakeSystemClockClient::GetLastSyncInfo(GetLastSyncInfoCallback callback) {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), network_synchronized_));
}

void FakeSystemClockClient::WaitForServiceToBeAvailable(
    dbus::ObjectProxy::WaitForServiceToBeAvailableCallback callback) {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), true));
}

}  // namespace chromeos
