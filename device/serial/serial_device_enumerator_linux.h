// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_SERIAL_SERIAL_DEVICE_ENUMERATOR_LINUX_H_
#define DEVICE_SERIAL_SERIAL_DEVICE_ENUMERATOR_LINUX_H_

#include "base/macros.h"
#include "device/serial/serial_device_enumerator.h"
#include "device/udev_linux/scoped_udev.h"

namespace device {

// Discovers and enumerates serial devices available to the host.
class SerialDeviceEnumeratorLinux : public SerialDeviceEnumerator {
 public:
  SerialDeviceEnumeratorLinux();
  ~SerialDeviceEnumeratorLinux() override;

  // Implementation for SerialDeviceEnumerator.
  std::vector<mojom::SerialDeviceInfoPtr> GetDevices() override;

 private:
  ScopedUdevPtr udev_;

  DISALLOW_COPY_AND_ASSIGN(SerialDeviceEnumeratorLinux);
};

}  // namespace device

#endif  // DEVICE_SERIAL_SERIAL_DEVICE_ENUMERATOR_LINUX_H_
