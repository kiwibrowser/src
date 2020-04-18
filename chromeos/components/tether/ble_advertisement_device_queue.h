// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_BLE_ADVERTISEMENT_DEVICE_QUEUE_H_
#define CHROMEOS_COMPONENTS_TETHER_BLE_ADVERTISEMENT_DEVICE_QUEUE_H_

#include <map>
#include <string>
#include <vector>

#include "base/macros.h"
#include "chromeos/components/tether/connection_priority.h"

namespace chromeos {

namespace tether {

// Queue of devices to which to advertise. Because only
// |kMaxConcurrentAdvertisements| devices can be advertised to concurrently,
// this queue maintains the order of devices to ensure that each device gets its
// fair share of time spent contacting it.
class BleAdvertisementDeviceQueue {
 public:
  BleAdvertisementDeviceQueue();
  virtual ~BleAdvertisementDeviceQueue();

  struct PrioritizedDeviceId {
    PrioritizedDeviceId(const std::string& device_id,
                        const ConnectionPriority& connection_priority);
    ~PrioritizedDeviceId();

    std::string device_id;
    ConnectionPriority connection_priority;
  };

  // Updates the queue with the given |prioritized_ids|. Devices which are
  // already in the queue and are not in |prioritized_ids| are removed from the
  // queue, and all devices which are not in the queue but are in
  // |prioritized_ids| are added to the end of the queue. Note devices that are
  // already in the queue will not change order as a result of this function
  // being called to ensure that the queue remains in order. Returns whether the
  // device list has changed due to the function call.
  bool SetPrioritizedDeviceIds(
      const std::vector<PrioritizedDeviceId>& prioritized_ids);

  // Moves the given device to the end of the queue. If the device was not in
  // the queue to begin with, do nothing.
  void MoveDeviceToEnd(const std::string& device_id);

  // Returns a list of devices to which to advertise. The devices returned are
  // the first |kMaxConcurrentAdvertisements| devices in the front of the queue,
  // or fewer if the number of devices in the queue is less than that value.
  std::vector<std::string> GetDeviceIdsToWhichToAdvertise() const;

  size_t GetSize() const;

 private:
  // Inserts each of |prioritized_ids| into |priority_to_device_ids_map_|.
  // Elements of |prioritized_ids| which already exist in the map remain.
  // Returns whether any device IDs were added (i.e., if all elements of
  // |prioritized_ids| were already present in the map, false is returned).
  bool InsertPrioritizedDeviceIdsIfNecessary(
      const std::vector<PrioritizedDeviceId>& prioritized_ids);

  // Removes entries from |priority_to_device_ids_map_| which do not appear in
  // |prioritized_ids|. Returns whether any device IDs were removed (i.e., if
  // all of the elements in the map are present in |prioritized_ids|, false is
  // returned).
  bool RemoveMapEntriesIfNecessary(
      const std::vector<PrioritizedDeviceId>& prioritized_ids);

  void AddDevicesToVectorForPriority(
      ConnectionPriority connection_priority,
      std::vector<std::string>* device_ids_out) const;

  std::map<ConnectionPriority, std::vector<std::string>>
      priority_to_device_ids_map_;

  DISALLOW_COPY_AND_ASSIGN(BleAdvertisementDeviceQueue);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_BLE_ADVERTISEMENT_DEVICE_QUEUE_H_
