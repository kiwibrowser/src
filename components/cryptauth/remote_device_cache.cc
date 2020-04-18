// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/remote_device_cache.h"

#include "base/stl_util.h"

namespace cryptauth {

RemoteDeviceCache::RemoteDeviceCache() = default;

RemoteDeviceCache::~RemoteDeviceCache() = default;

void RemoteDeviceCache::SetRemoteDevices(
    const RemoteDeviceList& remote_devices) {
  for (const auto& remote_device : remote_devices) {
    if (!base::ContainsKey(remote_device_map_, remote_device.GetDeviceId())) {
      remote_device_map_[remote_device.GetDeviceId()] =
          std::make_shared<RemoteDevice>(remote_device);
      continue;
    }

    // Keep the same |shared_ptr| obect, and simply update the RemoteDevice it
    // references. This transparently updates the RemoteDeviceRefs used by
    // clients.
    *remote_device_map_[remote_device.GetDeviceId()] = remote_device;
  }

  // Intentionally leave behind devices in the map which weren't in
  // |remote_devices|, to prevent clients from segfaulting by accessing "stale"
  // devices.
}

RemoteDeviceRefList RemoteDeviceCache::GetRemoteDevices() const {
  RemoteDeviceRefList remote_devices;
  for (const auto& it : remote_device_map_)
    remote_devices.push_back(RemoteDeviceRef(it.second));

  return remote_devices;
}

base::Optional<RemoteDeviceRef> RemoteDeviceCache::GetRemoteDevice(
    const std::string& device_id) const {
  if (!base::ContainsKey(remote_device_map_, device_id))
    return base::nullopt;

  return RemoteDeviceRef(remote_device_map_.at(device_id));
}

}  // namespace cryptauth