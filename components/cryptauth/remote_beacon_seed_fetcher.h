// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_REMOTE_BEACON_SEED_FETCHER_H_
#define COMPONENTS_CRYPTAUTH_REMOTE_BEACON_SEED_FETCHER_H_

#include <vector>

#include "base/macros.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"

namespace cryptauth {

class CryptAuthDeviceManager;

// Fetches |BeaconSeed|s corresponding to a given |RemoteDevice|. Note that an
// alternate solution would be to embed a list of |BeaconSeed|s in each
// |RemoteDevice| object; however, because |BeaconSeed| objects take up almost
// 1kB of memory apiece, that approach adds unnecessary memory overhead.
class RemoteBeaconSeedFetcher {
 public:
  RemoteBeaconSeedFetcher(const CryptAuthDeviceManager* device_manager);
  virtual ~RemoteBeaconSeedFetcher();

  virtual bool FetchSeedsForDeviceId(
      const std::string& device_id,
      std::vector<BeaconSeed>* beacon_seeds_out) const;

 private:
  // Not owned by this instance and must outlive it.
  const CryptAuthDeviceManager* device_manager_;

  DISALLOW_COPY_AND_ASSIGN(RemoteBeaconSeedFetcher);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_REMOTE_BEACON_SEED_FETCHER_H_
