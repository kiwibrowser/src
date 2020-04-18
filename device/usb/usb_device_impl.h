// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_USB_USB_DEVICE_IMPL_H_
#define DEVICE_USB_USB_DEVICE_IMPL_H_

#include <stdint.h>

#include <list>
#include <memory>
#include <string>
#include <utility>

#include "base/callback.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "build/build_config.h"
#include "device/usb/scoped_libusb_device_ref.h"
#include "device/usb/usb_descriptors.h"
#include "device/usb/usb_device.h"

struct libusb_device;
struct libusb_device_descriptor;
struct libusb_device_handle;

namespace base {
class SequencedTaskRunner;
}

namespace device {

class UsbDeviceHandleImpl;
class UsbContext;

typedef struct libusb_device_handle* PlatformUsbDeviceHandle;

class UsbDeviceImpl : public UsbDevice {
 public:
  UsbDeviceImpl(scoped_refptr<UsbContext> context,
                ScopedLibusbDeviceRef platform_device,
                const libusb_device_descriptor& descriptor);

  // UsbDevice implementation:
  void Open(OpenCallback callback) override;

  // These functions are used during enumeration only. The values must not
  // change during the object's lifetime.
  void set_manufacturer_string(const base::string16& value) {
    manufacturer_string_ = value;
  }
  void set_product_string(const base::string16& value) {
    product_string_ = value;
  }
  void set_serial_number(const base::string16& value) {
    serial_number_ = value;
  }
  void set_webusb_landing_page(const GURL& url) { webusb_landing_page_ = url; }

  libusb_device* platform_device() const { return platform_device_.get(); }

 protected:
  friend class UsbServiceImpl;
  friend class UsbDeviceHandleImpl;

  ~UsbDeviceImpl() override;

  void ReadAllConfigurations();
  void RefreshActiveConfiguration();

  // Called only by UsbServiceImpl.
  void set_visited(bool visited) { visited_ = visited; }
  bool was_visited() const { return visited_; }

 private:
  void GetAllConfigurations();
  void OpenOnBlockingThread(
      OpenCallback callback,
      scoped_refptr<base::TaskRunner> task_runner,
      scoped_refptr<base::SequencedTaskRunner> blocking_task_runner);
  void Opened(PlatformUsbDeviceHandle platform_handle,
              OpenCallback callback,
              scoped_refptr<base::SequencedTaskRunner> blocking_task_runner);

  base::ThreadChecker thread_checker_;
  bool visited_ = false;

  // The libusb_context must not be released before the libusb_device.
  const scoped_refptr<UsbContext> context_;
  const ScopedLibusbDeviceRef platform_device_;

  DISALLOW_COPY_AND_ASSIGN(UsbDeviceImpl);
};

}  // namespace device

#endif  // DEVICE_USB_USB_DEVICE_IMPL_H_
