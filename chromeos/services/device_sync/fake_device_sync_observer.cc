// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/device_sync/fake_device_sync_observer.h"

namespace chromeos {

namespace device_sync {

FakeDeviceSyncObserver::FakeDeviceSyncObserver() = default;

FakeDeviceSyncObserver::~FakeDeviceSyncObserver() = default;

mojom::DeviceSyncObserverPtr FakeDeviceSyncObserver::GenerateInterfacePtr() {
  mojom::DeviceSyncObserverPtr interface_ptr;
  bindings_.AddBinding(this, mojo::MakeRequest(&interface_ptr));
  return interface_ptr;
}

void FakeDeviceSyncObserver::OnEnrollmentFinished() {
  ++num_enrollment_events_;
}

void FakeDeviceSyncObserver::OnNewDevicesSynced() {
  ++num_sync_events_;
}

}  // namespace device_sync

}  // namespace chromeos
