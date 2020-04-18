// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/ble_advertisement_device_queue.h"

#include <algorithm>

#include "base/logging.h"
#include "base/stl_util.h"
#include "chromeos/components/tether/ble_constants.h"

namespace chromeos {

namespace tether {

BleAdvertisementDeviceQueue::PrioritizedDeviceId::PrioritizedDeviceId(
    const std::string& device_id,
    const ConnectionPriority& connection_priority)
    : device_id(device_id), connection_priority(connection_priority) {}

BleAdvertisementDeviceQueue::PrioritizedDeviceId::~PrioritizedDeviceId() =
    default;

BleAdvertisementDeviceQueue::BleAdvertisementDeviceQueue() = default;

BleAdvertisementDeviceQueue::~BleAdvertisementDeviceQueue() = default;

bool BleAdvertisementDeviceQueue::SetPrioritizedDeviceIds(
    const std::vector<PrioritizedDeviceId>& prioritized_ids) {
  bool devices_inserted =
      InsertPrioritizedDeviceIdsIfNecessary(prioritized_ids);
  bool devices_removed = RemoveMapEntriesIfNecessary(prioritized_ids);
  return devices_inserted || devices_removed;
}

bool BleAdvertisementDeviceQueue::InsertPrioritizedDeviceIdsIfNecessary(
    const std::vector<PrioritizedDeviceId>& prioritized_ids) {
  bool updated = false;

  // For each device provided, check to see if the device is already part of the
  // queue. If it is not, add it to the end of the vector associated with the
  // device's priority.
  for (const auto& priotizied_id : prioritized_ids) {
    std::vector<std::string>& device_ids_for_priority =
        priority_to_device_ids_map_[priotizied_id.connection_priority];
    if (!base::ContainsValue(device_ids_for_priority,
                             priotizied_id.device_id)) {
      device_ids_for_priority.push_back(priotizied_id.device_id);
      updated = true;
    }
  }

  return updated;
}

bool BleAdvertisementDeviceQueue::RemoveMapEntriesIfNecessary(
    const std::vector<PrioritizedDeviceId>& prioritized_ids) {
  bool updated = false;

  // Iterate through each priority's device IDs to see if any of the entries
  // were not provided as part of the |prioritized_ids| parameter. If any such
  // entries exist, remove them from the map.
  for (auto& map_entry : priority_to_device_ids_map_) {
    ConnectionPriority priority = map_entry.first;
    std::vector<std::string>& device_ids_for_priority = map_entry.second;

    auto device_ids_it = device_ids_for_priority.begin();
    while (device_ids_it != device_ids_for_priority.end()) {
      const std::string device_id = *device_ids_it;

      // If the device ID in the map also exists in |prioritized_ids|, skip it.
      if (std::find_if(prioritized_ids.begin(), prioritized_ids.end(),
                       [priority, &device_id](auto prioritized_id) {
                         return prioritized_id.device_id == device_id &&
                                prioritized_id.connection_priority == priority;
                       }) != prioritized_ids.end()) {
        ++device_ids_it;
        continue;
      }

      // The device ID in the map does not exist in |prioritized_ids|. Remove
      // it.
      device_ids_it = device_ids_for_priority.erase(device_ids_it);
      updated = true;
    }
  }

  return updated;
}

void BleAdvertisementDeviceQueue::MoveDeviceToEnd(
    const std::string& device_id) {
  DCHECK(!device_id.empty());

  for (auto& map_entry : priority_to_device_ids_map_) {
    std::vector<std::string>& device_ids_for_priority = map_entry.second;

    auto device_id_it = std::find(device_ids_for_priority.begin(),
                                  device_ids_for_priority.end(), device_id);
    if (device_id_it == device_ids_for_priority.end())
      continue;

    // Move the element to the end of |device_ids_for_priority|.
    std::rotate(device_id_it, device_id_it + 1, device_ids_for_priority.end());
  }
}

std::vector<std::string>
BleAdvertisementDeviceQueue::GetDeviceIdsToWhichToAdvertise() const {
  std::vector<std::string> device_ids;
  AddDevicesToVectorForPriority(ConnectionPriority::CONNECTION_PRIORITY_HIGH,
                                &device_ids);
  AddDevicesToVectorForPriority(ConnectionPriority::CONNECTION_PRIORITY_MEDIUM,
                                &device_ids);
  AddDevicesToVectorForPriority(ConnectionPriority::CONNECTION_PRIORITY_LOW,
                                &device_ids);
  DCHECK(device_ids.size() <= kMaxConcurrentAdvertisements);
  return device_ids;
}

size_t BleAdvertisementDeviceQueue::GetSize() const {
  size_t count = 0;
  for (const auto& map_entry : priority_to_device_ids_map_)
    count += map_entry.second.size();
  return count;
}

void BleAdvertisementDeviceQueue::AddDevicesToVectorForPriority(
    ConnectionPriority connection_priority,
    std::vector<std::string>* device_ids_out) const {
  if (priority_to_device_ids_map_.find(connection_priority) ==
      priority_to_device_ids_map_.end()) {
    // Nothing to do if there is no entry for this priority.
    return;
  }

  const std::vector<std::string>& device_ids_for_priority =
      priority_to_device_ids_map_.at(connection_priority);
  size_t i = 0;
  while (i < device_ids_for_priority.size() &&
         device_ids_out->size() < kMaxConcurrentAdvertisements) {
    device_ids_out->push_back(device_ids_for_priority[i]);
    ++i;
  }
}

}  // namespace tether

}  // namespace chromeos
