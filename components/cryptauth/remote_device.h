// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_H_
#define COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_H_

#include <map>
#include <string>
#include <vector>

#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/software_feature_state.h"

namespace cryptauth {

struct RemoteDevice {
 public:
  // Generates the device ID for a device given its public key.
  static std::string GenerateDeviceId(const std::string& public_key);

  std::string user_id;
  std::string name;
  std::string public_key;
  std::string persistent_symmetric_key;
  bool unlock_key;
  bool supports_mobile_hotspot;
  int64_t last_update_time_millis;
  std::map<SoftwareFeature, SoftwareFeatureState> software_features;

  // Note: To save space, the BeaconSeeds may not necessarily be included in
  // this object.
  bool are_beacon_seeds_loaded = false;
  std::vector<BeaconSeed> beacon_seeds;

  RemoteDevice();
  RemoteDevice(
      const std::string& user_id,
      const std::string& name,
      const std::string& public_key,
      const std::string& persistent_symmetric_key,
      bool unlock_key,
      bool supports_mobile_hotspot,
      int64_t last_update_time_millis,
      const std::map<SoftwareFeature, SoftwareFeatureState>& software_features);
  RemoteDevice(const RemoteDevice& other);
  ~RemoteDevice();

  // Loads a vector of BeaconSeeds for the RemoteDevice.
  void LoadBeaconSeeds(const std::vector<BeaconSeed>& beacon_seeds);

  std::string GetDeviceId() const;

  bool operator==(const RemoteDevice& other) const;

  // Compares devices via their public keys. Note that this function is
  // necessary in order to use |RemoteDevice| as a key of a std::map.
  bool operator<(const RemoteDevice& other) const;
};

typedef std::vector<RemoteDevice> RemoteDeviceList;

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_H_
