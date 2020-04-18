// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_HID_MOCK_HID_CONNECTION_H_
#define SERVICES_DEVICE_HID_MOCK_HID_CONNECTION_H_

#include "services/device/hid/hid_connection.h"

namespace device {

class MockHidConnection : public HidConnection {
 public:
  friend class base::RefCountedThreadSafe<MockHidConnection>;

  explicit MockHidConnection(scoped_refptr<HidDeviceInfo> device);

  // HidConnection implementation.
  void PlatformClose() override;
  void PlatformRead(ReadCallback callback) override;
  void PlatformWrite(scoped_refptr<base::RefCountedBytes> buffer,
                     WriteCallback callback) override;
  void PlatformGetFeatureReport(uint8_t report_id,
                                ReadCallback callback) override;
  void PlatformSendFeatureReport(scoped_refptr<base::RefCountedBytes> buffer,
                                 WriteCallback callback) override;

 private:
  ~MockHidConnection() override;

  DISALLOW_COPY_AND_ASSIGN(MockHidConnection);
};

}  // namespace device

#endif  // SERVICES_DEVICE_HID_MOCK_HID_CONNECTION_H_
