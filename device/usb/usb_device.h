// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_USB_USB_DEVICE_H_
#define DEVICE_USB_USB_DEVICE_H_

#include <stdint.h>

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/observer_list.h"
#include "base/strings/string16.h"
#include "device/usb/usb_descriptors.h"
#include "url/gurl.h"

namespace device {

class UsbDeviceHandle;

// A UsbDevice object represents a detected USB device, providing basic
// information about it. Methods other than simple property accessors must be
// called from the thread on which this object was created. For further
// manipulation of the device, a UsbDeviceHandle must be created from Open()
// method.
class UsbDevice : public base::RefCountedThreadSafe<UsbDevice> {
 public:
  using OpenCallback = base::OnceCallback<void(scoped_refptr<UsbDeviceHandle>)>;
  using ResultCallback = base::OnceCallback<void(bool success)>;

  // This observer interface should be used by objects that need only be
  // notified about the removal of a particular device as it is more efficient
  // than registering a large number of observers with UsbService::AddObserver.
  class Observer {
   public:
    virtual ~Observer();

    // This method is called when the UsbService that created this object
    // detects that the device has been disconnected from the host.
    virtual void OnDeviceRemoved(scoped_refptr<UsbDevice> device);
  };

  // A unique identifier which remains stable for the lifetime of this device
  // object (i.e., until the device is unplugged or the USB service dies.)
  const std::string& guid() const { return guid_; }

  // Accessors to basic information.
  uint16_t usb_version() const { return descriptor_.usb_version; }
  uint8_t device_class() const { return descriptor_.device_class; }
  uint8_t device_subclass() const { return descriptor_.device_subclass; }
  uint8_t device_protocol() const { return descriptor_.device_protocol; }
  uint16_t vendor_id() const { return descriptor_.vendor_id; }
  uint16_t product_id() const { return descriptor_.product_id; }
  uint16_t device_version() const { return descriptor_.device_version; }
  const base::string16& manufacturer_string() const {
    return manufacturer_string_;
  }
  const base::string16& product_string() const { return product_string_; }
  const base::string16& serial_number() const { return serial_number_; }
  const GURL& webusb_landing_page() const { return webusb_landing_page_; }
  const std::vector<UsbConfigDescriptor>& configurations() const {
    return descriptor_.configurations;
  }
  const UsbConfigDescriptor* active_configuration() const {
    return active_configuration_;
  }

  // On ChromeOS the permission_broker service must be used to open USB devices.
  // This function asks it to check whether a future Open call will be allowed.
  // On all other platforms this is a no-op and always returns true.
  virtual void CheckUsbAccess(ResultCallback callback);

  // On Android applications must request permission from the user to access a
  // USB device before it can be opened. After permission is granted the device
  // properties may contain information not previously available. On all other
  // platforms this is a no-op and always returns true.
  virtual void RequestPermission(ResultCallback callback);
  virtual bool permission_granted() const;

  // Creates a UsbDeviceHandle for further manipulation.
  virtual void Open(OpenCallback callback) = 0;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  friend class UsbService;

  UsbDevice();
  UsbDevice(const UsbDeviceDescriptor& descriptor,
            const base::string16& manufacturer_string,
            const base::string16& product_string,
            const base::string16& serial_number);
  UsbDevice(uint16_t usb_version,
            uint8_t device_class,
            uint8_t device_subclass,
            uint8_t device_protocol,
            uint16_t vendor_id,
            uint16_t product_id,
            uint16_t device_version,
            const base::string16& manufacturer_string,
            const base::string16& product_string,
            const base::string16& serial_number);
  virtual ~UsbDevice();

  void ActiveConfigurationChanged(int configuration_value);
  void NotifyDeviceRemoved();

  std::list<UsbDeviceHandle*>& handles() { return handles_; }

  // These members must be mutable by subclasses as necessary during device
  // enumeration. To preserve the thread safety of this object they must remain
  // constant afterwards.
  UsbDeviceDescriptor descriptor_;
  base::string16 manufacturer_string_;
  base::string16 product_string_;
  base::string16 serial_number_;
  GURL webusb_landing_page_;

 private:
  friend class base::RefCountedThreadSafe<UsbDevice>;
  friend class UsbDeviceHandleImpl;
  friend class UsbDeviceHandleUsbfs;
  friend class UsbDeviceHandleWin;
  friend class UsbServiceAndroid;
  friend class UsbServiceImpl;
  friend class UsbServiceLinux;
  friend class UsbServiceWin;

  void OnDisconnect();
  void HandleClosed(UsbDeviceHandle* handle);

  const std::string guid_;

  // The current device configuration descriptor. May be null if the device is
  // in an unconfigured state; if not null, it is a pointer to one of the
  // items in |descriptor_.configurations|.
  const UsbConfigDescriptor* active_configuration_ = nullptr;

  // Weak pointers to open handles. HandleClosed() will be called before each
  // is freed.
  std::list<UsbDeviceHandle*> handles_;

  base::ObserverList<Observer, true> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(UsbDevice);
};

}  // namespace device

#endif  // DEVICE_USB_USB_DEVICE_H_
