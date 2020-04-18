// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_FAKE_REMOTE_DEVICE_PROVIDER_H_
#define COMPONENTS_CRYPTAUTH_FAKE_REMOTE_DEVICE_PROVIDER_H_

#include "components/cryptauth/remote_device_provider.h"

namespace cryptauth {

// Test double for RemoteDeviceProvider.
class FakeRemoteDeviceProvider : public RemoteDeviceProvider {
 public:
  FakeRemoteDeviceProvider();
  ~FakeRemoteDeviceProvider() override;

  void set_synced_remote_devices(
      const RemoteDeviceList& synced_remote_devices) {
    synced_remote_devices_ = synced_remote_devices;
  }

  void NotifyObserversDeviceListChanged();

  // RemoteDeviceProvider:
  const RemoteDeviceList& GetSyncedDevices() const override;

 private:
  RemoteDeviceList synced_remote_devices_;

  DISALLOW_COPY_AND_ASSIGN(FakeRemoteDeviceProvider);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_FAKE_REMOTE_DEVICE_PROVIDER_H_
