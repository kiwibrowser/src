// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_REF_H_
#define COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_REF_H_

#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/remote_device.h"
#include "components/cryptauth/software_feature_state.h"

namespace chromeos {
class EasyUnlockServiceRegular;
namespace tether {
class TetherHostFetcherImpl;
class TetherHostFetcherImplTest;
}  // namespace tether
}  // namespace chromeos

namespace proximity_auth {
class BluetoothLowEnergySetupConnectionFinder;
class ProximityAuthWebUIHandler;
}  // namespace proximity_auth

namespace cryptauth {

class RemoteDeviceCache;

// Contains metadata specific to a device associated with a user's account.
// Because this metadata contains large and expensive data types, and that data
// can become stale if a Device Sync occurs during a client application's
// lifecycle, RemoteDeviceRef is implemented using a pointer to a struct
// containing this metadata; if multiple clients want to reference the same
// device, multiple RemoteDeviceRefs can be created cheaply without duplicating
// the underlying data. Should be passed by value.
class RemoteDeviceRef {
 public:
  // Generates the device ID for a device given its public key.
  static std::string GenerateDeviceId(const std::string& public_key);

  // Derives the public key that was used to generate the given device ID;
  // returns empty string if |device_id| is not a valid device ID.
  static std::string DerivePublicKey(const std::string& device_id);

  // Static method for truncated device ID for logs.
  static std::string TruncateDeviceIdForLogs(const std::string& full_id);

  RemoteDeviceRef(const RemoteDeviceRef& other);
  ~RemoteDeviceRef();

  const std::string& user_id() const { return remote_device_->user_id; }
  const std::string& name() const { return remote_device_->name; }
  const std::string& public_key() const { return remote_device_->public_key; }
  const std::string& persistent_symmetric_key() const {
    return remote_device_->persistent_symmetric_key;
  }
  bool unlock_key() const { return remote_device_->unlock_key; }
  bool supports_mobile_hotspot() const {
    return remote_device_->supports_mobile_hotspot;
  }
  int64_t last_update_time_millis() const {
    return remote_device_->last_update_time_millis;
  }
  const std::vector<BeaconSeed>& beacon_seeds() const {
    return remote_device_->beacon_seeds;
  }
  bool are_beacon_seeds_loaded() const {
    return remote_device_->are_beacon_seeds_loaded;
  }

  std::string GetDeviceId() const;
  SoftwareFeatureState GetSoftwareFeatureState(
      const SoftwareFeature& software_feature) const;

  // Returns a shortened device ID for the purpose of concise logging (device
  // IDs are often so long that logs are difficult to read). Note that this
  // ID is not guaranteed to be unique, so it should only be used for log.
  std::string GetTruncatedDeviceIdForLogs() const;

  bool operator==(const RemoteDeviceRef& other) const;

  // This function is necessary in order to use |RemoteDeviceRef| as a key of a
  // std::map.
  bool operator<(const RemoteDeviceRef& other) const;

 private:
  friend class RemoteDeviceCache;
  friend class RemoteDeviceRefBuilder;
  friend class RemoteDeviceRefTest;
  FRIEND_TEST_ALL_PREFIXES(RemoteDeviceRefTest, TestFields);
  FRIEND_TEST_ALL_PREFIXES(RemoteDeviceRefTest, TestCopyAndAssign);

  // TODO(crbug.com/752273): Remove these once clients have migrated to Device
  // Sync service.
  friend class chromeos::EasyUnlockServiceRegular;
  friend class chromeos::tether::TetherHostFetcherImpl;
  friend class chromeos::tether::TetherHostFetcherImplTest;
  friend class proximity_auth::ProximityAuthWebUIHandler;
  friend class proximity_auth::BluetoothLowEnergySetupConnectionFinder;

  explicit RemoteDeviceRef(std::shared_ptr<RemoteDevice> remote_device);

  std::shared_ptr<const RemoteDevice> remote_device_;
};

typedef std::vector<RemoteDeviceRef> RemoteDeviceRefList;

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_REF_H_