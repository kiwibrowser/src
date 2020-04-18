// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_TRACKER_H_
#define COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_TRACKER_H_

#include <memory>
#include <string>
#include <vector>

#include "components/sync/device_info/device_info.h"

namespace syncer {

// Interface for tracking synced DeviceInfo.
class DeviceInfoTracker {
 public:
  virtual ~DeviceInfoTracker() {}

  // Observer class for listening to device info changes.
  class Observer {
   public:
    virtual void OnDeviceInfoChange() = 0;
  };

  // Returns true when DeviceInfo datatype is enabled and syncing.
  virtual bool IsSyncing() const = 0;
  // Gets DeviceInfo the synced device with specified client ID.
  // Returns an empty unique_ptr if device with the given |client_id| hasn't
  // been synced.
  virtual std::unique_ptr<DeviceInfo> GetDeviceInfo(
      const std::string& client_id) const = 0;
  // Gets DeviceInfo for all synced devices (including the local one).
  virtual std::vector<std::unique_ptr<DeviceInfo>> GetAllDeviceInfo() const = 0;
  // Registers an observer to be called on syncing any updated DeviceInfo.
  virtual void AddObserver(Observer* observer) = 0;
  // Unregisters an observer.
  virtual void RemoveObserver(Observer* observer) = 0;
  // Returns the count of active devices.
  virtual int CountActiveDevices() const = 0;
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_TRACKER_H_
