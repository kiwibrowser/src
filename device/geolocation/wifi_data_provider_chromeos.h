// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GEOLOCATION_WIFI_DATA_PROVIDER_CHROMEOS_H_
#define DEVICE_GEOLOCATION_WIFI_DATA_PROVIDER_CHROMEOS_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "device/geolocation/wifi_data_provider.h"
#include "device/geolocation/wifi_polling_policy.h"

namespace device {

class DEVICE_GEOLOCATION_EXPORT WifiDataProviderChromeOs
    : public WifiDataProvider {
 public:
  WifiDataProviderChromeOs();

  // WifiDataProvider
  void StartDataProvider() override;
  void StopDataProvider() override;
  bool GetData(WifiData* data) override;

 private:
  friend class GeolocationChromeOsWifiDataProviderTest;
  ~WifiDataProviderChromeOs() override;

  // NetworkHandler thread
  void DoWifiScanTaskOnNetworkHandlerThread();

  // Client thread
  void DidWifiScanTaskNoResults();
  void DidWifiScanTask(const WifiData& new_data);

  // Will schedule a scan; i.e. enqueue DoWifiScanTask deferred task.
  void ScheduleNextScan(int interval);

  // Will schedule starting of the scanning process.
  void ScheduleStart();

  // Will schedule stopping of the scanning process.
  void ScheduleStop();

  // Get access point data from chromeos.
  bool GetAccessPointData(WifiData::AccessPointDataSet* data);

  // Controls the polling update interval. (client thread)
  std::unique_ptr<WifiPollingPolicy> polling_policy_;

  // The latest wifi data. (client thread)
  WifiData wifi_data_;

  // Whether we have strated the data provider. (client thread)
  bool started_;

  // Whether we've successfully completed a scan for WiFi data. (client thread)
  bool is_first_scan_complete_;

  DISALLOW_COPY_AND_ASSIGN(WifiDataProviderChromeOs);
};

}  // namespace device

#endif  // DEVICE_GEOLOCATION_WIFI_DATA_PROVIDER_CHROMEOS_H_
