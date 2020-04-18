// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_DEVICE_BLUETOOTH_LE_MOCK_LE_SCAN_MANAGER_H_
#define CHROMECAST_DEVICE_BLUETOOTH_LE_MOCK_LE_SCAN_MANAGER_H_

#include <vector>

#include "chromecast/device/bluetooth/le/le_scan_manager.h"
#include "chromecast/device/bluetooth/le/scan_filter.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace chromecast {
namespace bluetooth {

class MockLeScanManager : public LeScanManager {
 public:
  MockLeScanManager();
  ~MockLeScanManager();

  void AddObserver(Observer* o) override {
    DCHECK(o && !observer_);
    observer_ = o;
  }
  void RemoveObserver(Observer* o) override {
    DCHECK(o && o == observer_);
    observer_ = nullptr;
  }

  MOCK_METHOD1(SetScanEnable, bool(bool enable));
  void SetScanEnable(bool enable, SetScanEnableCallback cb) override {
    std::move(cb).Run(SetScanEnable(enable));
  }

  MOCK_METHOD1(
      GetScanResults,
      std::vector<LeScanResult>(base::Optional<ScanFilter> scan_filter));
  void GetScanResults(GetScanResultsCallback cb,
                      base::Optional<ScanFilter> scan_filter) override {
    std::move(cb).Run(GetScanResults(std::move(scan_filter)));
  }
  MOCK_METHOD0(ClearScanResults, void());

  Observer* observer_ = nullptr;
};

}  // namespace bluetooth
}  // namespace chromecast

#endif  // CHROMECAST_DEVICE_BLUETOOTH_LE_MOCK_LE_SCAN_MANAGER_H_
