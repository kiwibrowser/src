// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_SERIAL_SERIAL_DEVICE_ENUMERATOR_IMPL_H_
#define SERVICES_DEVICE_SERIAL_SERIAL_DEVICE_ENUMERATOR_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "device/serial/serial_device_enumerator.h"
#include "services/device/public/mojom/serial.mojom.h"

namespace device {

// TODO(leonhsl): Merge this class with SerialDeviceEnumerator if/once
// SerialDeviceEnumerator is exposed only via the Device Service.
// crbug.com/748505
class SerialDeviceEnumeratorImpl : public mojom::SerialDeviceEnumerator {
 public:
  static void Create(mojom::SerialDeviceEnumeratorRequest request);

  SerialDeviceEnumeratorImpl();
  ~SerialDeviceEnumeratorImpl() override;

 private:
  // mojom::SerialDeviceEnumerator methods:
  void GetDevices(GetDevicesCallback callback) override;

  std::unique_ptr<device::SerialDeviceEnumerator> enumerator_;

  DISALLOW_COPY_AND_ASSIGN(SerialDeviceEnumeratorImpl);
};

}  // namespace device

#endif  // SERVICES_DEVICE_SERIAL_SERIAL_DEVICE_ENUMERATOR_IMPL_H_
