// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_USB_USB_SERVICE_WIN_H_
#define DEVICE_USB_USB_SERVICE_WIN_H_

#include "device/usb/usb_service.h"

#include <list>
#include <unordered_map>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "device/base/device_monitor_win.h"
#include "device/usb/usb_device_win.h"

namespace device {

class UsbServiceWin : public DeviceMonitorWin::Observer, public UsbService {
 public:
  UsbServiceWin();
  ~UsbServiceWin() override;

 private:
  class BlockingTaskHelper;

  // device::UsbService implementation
  void GetDevices(const GetDevicesCallback& callback) override;

  // device::DeviceMonitorWin::Observer implementation
  void OnDeviceAdded(const GUID& class_guid,
                     const std::string& device_path) override;
  void OnDeviceRemoved(const GUID& class_guid,
                       const std::string& device_path) override;

  // Methods called by BlockingThreadHelper
  void HelperStarted();
  void CreateDeviceObject(const std::string& device_path,
                          const std::string& hub_path,
                          int port_number,
                          const std::string& driver_name);

  void DeviceReady(scoped_refptr<UsbDeviceWin> device, bool success);

  bool enumeration_ready() {
    return helper_started_ && first_enumeration_countdown_ == 0;
  }

  // Enumeration callbacks are queued until an enumeration completes.
  bool helper_started_ = false;
  uint32_t first_enumeration_countdown_ = 0;
  std::list<GetDevicesCallback> enumeration_callbacks_;

  std::unique_ptr<BlockingTaskHelper> helper_;
  std::unordered_map<std::string, scoped_refptr<UsbDeviceWin>> devices_by_path_;

  ScopedObserver<DeviceMonitorWin, DeviceMonitorWin::Observer> device_observer_;

  base::WeakPtrFactory<UsbServiceWin> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(UsbServiceWin);
};

}  // namespace device

#endif  // DEVICE_USB_USB_SERVICE_WIN_H_
