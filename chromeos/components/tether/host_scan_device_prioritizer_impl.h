// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_HOST_SCAN_DEVICE_PRIORITIZER_IMPL_H_
#define CHROMEOS_COMPONENTS_TETHER_HOST_SCAN_DEVICE_PRIORITIZER_IMPL_H_

#include "base/macros.h"
#include "chromeos/components/tether/host_scan_device_prioritizer.h"
#include "components/cryptauth/remote_device_ref.h"

namespace chromeos {

namespace tether {

class TetherHostResponseRecorder;

// Implementation of HostScanDevicePrioritizer.
class HostScanDevicePrioritizerImpl : public HostScanDevicePrioritizer {
 public:
  HostScanDevicePrioritizerImpl(
      TetherHostResponseRecorder* tether_host_response_recorder);
  ~HostScanDevicePrioritizerImpl() override;

  // HostScanDevicePrioritizer:
  void SortByHostScanOrder(
      cryptauth::RemoteDeviceRefList* remote_devices) const override;

 private:
  TetherHostResponseRecorder* tether_host_response_recorder_;

  DISALLOW_COPY_AND_ASSIGN(HostScanDevicePrioritizerImpl);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_HOST_SCAN_DEVICE_PRIORITIZER_IMPL_H_
