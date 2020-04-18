// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_HID_HID_CONNECTION_H_
#define SERVICES_DEVICE_HID_HID_CONNECTION_H_

#include <stddef.h>
#include <stdint.h>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "services/device/hid/hid_device_info.h"

namespace base {
class RefCountedBytes;
}

namespace device {

class HidConnection : public base::RefCountedThreadSafe<HidConnection> {
 public:
  enum SpecialReportIds {
    kNullReportId = 0x00,
    kAnyReportId = 0xFF,
  };

  using ReadCallback =
      base::OnceCallback<void(bool success,
                              scoped_refptr<base::RefCountedBytes> buffer,
                              size_t size)>;

  using WriteCallback = base::OnceCallback<void(bool success)>;

  scoped_refptr<HidDeviceInfo> device_info() const { return device_info_; }
  bool has_protected_collection() const { return has_protected_collection_; }
  const base::ThreadChecker& thread_checker() const { return thread_checker_; }
  bool closed() const { return closed_; }

  // Closes the connection. This must be called before the object is freed.
  void Close();

  // The report ID (or 0 if report IDs are not supported by the device) is
  // always returned in the first byte of the buffer.
  void Read(ReadCallback callback);

  // The report ID (or 0 if report IDs are not supported by the device) is
  // always expected in the first byte of the buffer.
  void Write(scoped_refptr<base::RefCountedBytes> buffer,
             WriteCallback callback);

  // The buffer will contain whatever report data was received from the device.
  // This may include the report ID. The report ID is not stripped because a
  // device may respond with other data in place of the report ID.
  void GetFeatureReport(uint8_t report_id, ReadCallback callback);

  // The report ID (or 0 if report IDs are not supported by the device) is
  // always expected in the first byte of the buffer.
  void SendFeatureReport(scoped_refptr<base::RefCountedBytes> buffer,
                         WriteCallback callback);

 protected:
  friend class base::RefCountedThreadSafe<HidConnection>;

  explicit HidConnection(scoped_refptr<HidDeviceInfo> device_info);
  virtual ~HidConnection();

  virtual void PlatformClose() = 0;
  virtual void PlatformRead(ReadCallback callback) = 0;
  virtual void PlatformWrite(scoped_refptr<base::RefCountedBytes> buffer,
                             WriteCallback callback) = 0;
  virtual void PlatformGetFeatureReport(uint8_t report_id,
                                        ReadCallback callback) = 0;
  virtual void PlatformSendFeatureReport(
      scoped_refptr<base::RefCountedBytes> buffer,
      WriteCallback callback) = 0;

  bool IsReportIdProtected(uint8_t report_id);

 private:
  scoped_refptr<HidDeviceInfo> device_info_;
  bool has_protected_collection_;
  base::ThreadChecker thread_checker_;
  bool closed_;

  DISALLOW_COPY_AND_ASSIGN(HidConnection);
};

struct PendingHidReport {
  PendingHidReport();
  PendingHidReport(const PendingHidReport& other);
  ~PendingHidReport();

  scoped_refptr<base::RefCountedBytes> buffer;
  size_t size;
};

struct PendingHidRead {
  PendingHidRead();
  PendingHidRead(PendingHidRead&& other);
  ~PendingHidRead();

  HidConnection::ReadCallback callback;
};

}  // namespace device

#endif  // SERVICES_DEVICE_HID_HID_CONNECTION_H_
