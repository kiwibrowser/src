// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_USB_MOJO_DEVICE_MANAGER_IMPL_H_
#define DEVICE_USB_MOJO_DEVICE_MANAGER_IMPL_H_

#include <memory>
#include <queue>
#include <set>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "device/usb/public/mojom/device_manager.mojom.h"
#include "device/usb/usb_service.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace device {

class UsbDevice;

namespace usb {

class PermissionProvider;

// Implements the public Mojo UsbDeviceManager interface by wrapping the
// UsbService instance.
class DeviceManagerImpl : public mojom::UsbDeviceManager,
                          public UsbService::Observer {
 public:
  static void Create(base::WeakPtr<PermissionProvider> permission_provider,
                     mojom::UsbDeviceManagerRequest request);

  ~DeviceManagerImpl() override;

 private:
  DeviceManagerImpl(base::WeakPtr<PermissionProvider> permission_provider,
                    UsbService* usb_service);

  // DeviceManager implementation:
  void GetDevices(mojom::UsbEnumerationOptionsPtr options,
                  GetDevicesCallback callback) override;
  void GetDevice(const std::string& guid,
                 mojom::UsbDeviceRequest device_request) override;
  void SetClient(mojom::UsbDeviceManagerClientPtr client) override;

  // Callbacks to handle the async responses from the underlying UsbService.
  void OnGetDevices(mojom::UsbEnumerationOptionsPtr options,
                    GetDevicesCallback callback,
                    const std::vector<scoped_refptr<UsbDevice>>& devices);

  // UsbService::Observer implementation:
  void OnDeviceAdded(scoped_refptr<UsbDevice> device) override;
  void OnDeviceRemoved(scoped_refptr<UsbDevice> device) override;
  void WillDestroyUsbService() override;

  void MaybeRunDeviceChangesCallback();

  mojo::StrongBindingPtr<mojom::UsbDeviceManager> binding_;
  base::WeakPtr<PermissionProvider> permission_provider_;

  UsbService* usb_service_;
  ScopedObserver<UsbService, UsbService::Observer> observer_;
  mojom::UsbDeviceManagerClientPtr client_;

  base::WeakPtrFactory<DeviceManagerImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DeviceManagerImpl);
};

}  // namespace usb
}  // namespace device

#endif  // DEVICE_USB_MOJO_DEVICE_MANAGER_IMPL_H_
