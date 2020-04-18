// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_HID_HID_CONNECTION_IMPL_H_
#define SERVICES_DEVICE_HID_HID_CONNECTION_IMPL_H_

#include "base/memory/ref_counted.h"
#include "services/device/hid/hid_connection.h"
#include "services/device/public/mojom/hid.mojom.h"

namespace device {

// HidConnectionImpl is reponsible for handling mojo communications from
// clients. It delegates to HidConnection the real work of creating
// connections in different platforms.
class HidConnectionImpl : public mojom::HidConnection {
 public:
  explicit HidConnectionImpl(scoped_refptr<device::HidConnection> connection);
  ~HidConnectionImpl() final;

  // mojom::HidConnection implementation:
  void Read(ReadCallback callback) override;
  void Write(uint8_t report_id,
             const std::vector<uint8_t>& buffer,
             WriteCallback callback) override;
  void GetFeatureReport(uint8_t report_id,
                        GetFeatureReportCallback callback) override;
  void SendFeatureReport(uint8_t report_id,
                         const std::vector<uint8_t>& buffer,
                         SendFeatureReportCallback callback) override;

 private:
  void OnRead(ReadCallback callback,
              bool success,
              scoped_refptr<base::RefCountedBytes> buffer,
              size_t size);
  void OnWrite(WriteCallback callback, bool success);
  void OnGetFeatureReport(GetFeatureReportCallback callback,
                          bool success,
                          scoped_refptr<base::RefCountedBytes> buffer,
                          size_t size);
  void OnSendFeatureReport(SendFeatureReportCallback callback, bool success);

  scoped_refptr<device::HidConnection> hid_connection_;
  base::WeakPtrFactory<HidConnectionImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(HidConnectionImpl);
};

}  // namespace device

#endif  // SERVICES_DEVICE_HID_HID_CONNECTION_IMPL_H_
