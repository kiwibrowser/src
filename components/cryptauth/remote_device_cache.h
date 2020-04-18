// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_CACHE_H_
#define COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_CACHE_H_

#include <memory>
#include <unordered_map>

#include "base/macros.h"
#include "base/optional.h"
#include "components/cryptauth/remote_device.h"
#include "components/cryptauth/remote_device_ref.h"

namespace cryptauth {

// A simple cache of RemoteDeviceRefs. Note that if multiple calls to
// SetRemoteDevices() are provided different sets of devices, the set of devices
// returned by GetRemoteDevices() is the union of those different sets (i.e.,
// devices are not deleted from the cache).
class RemoteDeviceCache {
 public:
  RemoteDeviceCache();
  virtual ~RemoteDeviceCache();

  void SetRemoteDevices(const RemoteDeviceList& remote_devices);

  RemoteDeviceRefList GetRemoteDevices() const;

  base::Optional<RemoteDeviceRef> GetRemoteDevice(
      const std::string& device_id) const;

 private:
  std::unordered_map<std::string, std::shared_ptr<RemoteDevice>>
      remote_device_map_;

  DISALLOW_COPY_AND_ASSIGN(RemoteDeviceCache);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_CACHE_H_
