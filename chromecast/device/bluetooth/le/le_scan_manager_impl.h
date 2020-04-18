// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_DEVICE_BLUETOOTH_LE_LE_SCAN_MANAGER_IMPL_H_
#define CHROMECAST_DEVICE_BLUETOOTH_LE_LE_SCAN_MANAGER_IMPL_H_

#include <list>
#include <map>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/observer_list_threadsafe.h"
#include "base/single_thread_task_runner.h"
#include "chromecast/device/bluetooth/le/le_scan_manager.h"
#include "chromecast/device/bluetooth/le/scan_filter.h"
#include "chromecast/device/bluetooth/shlib/le_scanner.h"

namespace chromecast {
namespace bluetooth {

class LeScanManagerImpl : public LeScanManager,
                          public bluetooth_v2_shlib::LeScanner::Delegate {
 public:
  explicit LeScanManagerImpl(bluetooth_v2_shlib::LeScannerImpl* le_scanner);
  ~LeScanManagerImpl() override;

  void Initialize(scoped_refptr<base::SingleThreadTaskRunner> io_task_runner);
  void Finalize();

  // LeScanManager implementation:
  void AddObserver(Observer* o) override;
  void RemoveObserver(Observer* o) override;
  void SetScanEnable(bool enable, SetScanEnableCallback cb) override;
  void GetScanResults(
      GetScanResultsCallback cb,
      base::Optional<ScanFilter> service_uuid = base::nullopt) override;
  void ClearScanResults() override;

 private:
  // Returns a list of all BLE scan results. The results are sorted by RSSI.
  // Must be called on |io_task_runner|.
  std::vector<LeScanResult> GetScanResultsInternal(
      base::Optional<ScanFilter> service_uuid);

  // bluetooth_v2_shlib::LeScanner::Delegate implementation:
  void OnScanResult(const bluetooth_v2_shlib::LeScanner::ScanResult&
                        scan_result_shlib) override;

  bluetooth_v2_shlib::LeScannerImpl* const le_scanner_;
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  scoped_refptr<base::ObserverListThreadSafe<Observer>> observers_;
  std::map<bluetooth_v2_shlib::Addr, std::list<LeScanResult>>
      addr_to_scan_results_;

  base::WeakPtrFactory<LeScanManagerImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(LeScanManagerImpl);
};

}  // namespace bluetooth
}  // namespace chromecast

#endif  // CHROMECAST_DEVICE_BLUETOOTH_LE_LE_SCAN_MANAGER_IMPL_H_
