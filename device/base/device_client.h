// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BASE_DEVICE_CLIENT_H_
#define DEVICE_BASE_DEVICE_CLIENT_H_

#include "base/macros.h"
#include "device/base/device_base_export.h"

namespace device {

class UsbService;

// Interface used by consumers of //device APIs to get pointers to the service
// singletons appropriate for a given embedding application. For an example see
// //chrome/browser/chrome_device_client.h.
class DEVICE_BASE_EXPORT DeviceClient {
 public:
  // Construction sets the single instance.
  DeviceClient();

  // Destruction clears the single instance.
  virtual ~DeviceClient();

  // Returns the single instance of |this|.
  static DeviceClient* Get();

  // Returns the UsbService instance for this embedder.
  virtual UsbService* GetUsbService();

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceClient);
};

}  // namespace device

#endif  // DEVICE_BASE_DEVICE_CLIENT_H_
