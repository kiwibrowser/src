// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_DEVICE_BLUETOOTH_LE_LE_SCAN_MANAGER_H_
#define CHROMECAST_DEVICE_BLUETOOTH_LE_LE_SCAN_MANAGER_H_

#include <list>
#include <map>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "chromecast/device/bluetooth/le/le_scan_result.h"
#include "chromecast/device/bluetooth/le/scan_filter.h"

namespace chromecast {
namespace bluetooth {

class LeScanManager {
 public:
  class Observer {
   public:
    // Called when the scan has been enabled or disabled.
    virtual void OnScanEnableChanged(bool enabled) {}

    // Called when a new scan result is ready.
    virtual void OnNewScanResult(LeScanResult result) {}

    virtual ~Observer() = default;
  };

  virtual void AddObserver(Observer* o) = 0;
  virtual void RemoveObserver(Observer* o) = 0;

  // Enable or disable BLE scnaning. Can be called on any thread. |cb| is
  // called on the thread that calls this method. |success| is false iff the
  // operation failed.
  using SetScanEnableCallback = base::OnceCallback<void(bool success)>;
  virtual void SetScanEnable(bool enable, SetScanEnableCallback cb) = 0;

  // Asynchronously get the most recent scan results. Can be called on any
  // thread. |cb| is called on the calling thread with the results. If
  // |scan_filter| is passed, only scan results matching the given |scan_filter|
  // will be returned.
  using GetScanResultsCallback =
      base::OnceCallback<void(std::vector<LeScanResult>)>;
  virtual void GetScanResults(
      GetScanResultsCallback cb,
      base::Optional<ScanFilter> scan_filter = base::nullopt) = 0;

  virtual void ClearScanResults() = 0;

 protected:
  LeScanManager() = default;
  virtual ~LeScanManager() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(LeScanManager);
};

}  // namespace bluetooth
}  // namespace chromecast

#endif  // CHROMECAST_DEVICE_BLUETOOTH_LE_LE_SCAN_MANAGER_H_
