// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_USB_USB_API_H_
#define EXTENSIONS_BROWSER_API_USB_USB_API_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "device/usb/public/mojom/device_manager.mojom.h"
#include "device/usb/usb_device.h"
#include "device/usb/usb_device_handle.h"
#include "extensions/browser/api/api_resource_manager.h"
#include "extensions/browser/extension_function.h"
#include "extensions/common/api/usb.h"

namespace base {
class RefCountedBytes;
}

namespace extensions {

class DevicePermissionEntry;
class DevicePermissionsPrompt;
class DevicePermissionsManager;

class UsbPermissionCheckingFunction : public UIThreadExtensionFunction {
 protected:
  UsbPermissionCheckingFunction();
  ~UsbPermissionCheckingFunction() override;

  bool HasDevicePermission(scoped_refptr<device::UsbDevice> device);
  void RecordDeviceLastUsed();

 private:
  DevicePermissionsManager* device_permissions_manager_;
  scoped_refptr<DevicePermissionEntry> permission_entry_;
};

class UsbConnectionFunction : public UIThreadExtensionFunction {
 protected:
  UsbConnectionFunction();
  ~UsbConnectionFunction() override;

  scoped_refptr<device::UsbDeviceHandle> GetDeviceHandle(
      const extensions::api::usb::ConnectionHandle& handle);
  void ReleaseDeviceHandle(
      const extensions::api::usb::ConnectionHandle& handle);
};

class UsbTransferFunction : public UsbConnectionFunction {
 protected:
  UsbTransferFunction();
  ~UsbTransferFunction() override;

  void OnCompleted(device::UsbTransferStatus status,
                   scoped_refptr<base::RefCountedBytes> data,
                   size_t length);
};

class UsbFindDevicesFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("usb.findDevices", USB_FINDDEVICES)

  UsbFindDevicesFunction();

 private:
  ~UsbFindDevicesFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  void OnGetDevicesComplete(
      const std::vector<scoped_refptr<device::UsbDevice>>& devices);
  void OnDeviceOpened(scoped_refptr<device::UsbDeviceHandle> device_handle);
  void OpenComplete();

  uint16_t vendor_id_;
  uint16_t product_id_;
  std::unique_ptr<base::ListValue> result_;
  base::Closure barrier_;

  DISALLOW_COPY_AND_ASSIGN(UsbFindDevicesFunction);
};

class UsbGetDevicesFunction : public UsbPermissionCheckingFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("usb.getDevices", USB_GETDEVICES)

  UsbGetDevicesFunction();

 private:
  ~UsbGetDevicesFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  void OnGetDevicesComplete(
      const std::vector<scoped_refptr<device::UsbDevice>>& devices);

  std::vector<device::mojom::UsbDeviceFilterPtr> filters_;

  DISALLOW_COPY_AND_ASSIGN(UsbGetDevicesFunction);
};

class UsbGetUserSelectedDevicesFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("usb.getUserSelectedDevices",
                             USB_GETUSERSELECTEDDEVICES)

  UsbGetUserSelectedDevicesFunction();

 private:
  ~UsbGetUserSelectedDevicesFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  void OnDevicesChosen(
      const std::vector<scoped_refptr<device::UsbDevice>>& devices);

  std::unique_ptr<DevicePermissionsPrompt> prompt_;

  DISALLOW_COPY_AND_ASSIGN(UsbGetUserSelectedDevicesFunction);
};

class UsbGetConfigurationsFunction : public UsbPermissionCheckingFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("usb.getConfigurations", USB_GETCONFIGURATIONS);

  UsbGetConfigurationsFunction();

 private:
  ~UsbGetConfigurationsFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  DISALLOW_COPY_AND_ASSIGN(UsbGetConfigurationsFunction);
};

class UsbRequestAccessFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("usb.requestAccess", USB_REQUESTACCESS)

  UsbRequestAccessFunction();

 private:
  ~UsbRequestAccessFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  DISALLOW_COPY_AND_ASSIGN(UsbRequestAccessFunction);
};

class UsbOpenDeviceFunction : public UsbPermissionCheckingFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("usb.openDevice", USB_OPENDEVICE)

  UsbOpenDeviceFunction();

 private:
  ~UsbOpenDeviceFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  void OnDeviceOpened(scoped_refptr<device::UsbDeviceHandle> device_handle);

  DISALLOW_COPY_AND_ASSIGN(UsbOpenDeviceFunction);
};

class UsbSetConfigurationFunction : public UsbConnectionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("usb.setConfiguration", USB_SETCONFIGURATION)

  UsbSetConfigurationFunction();

 private:
  ~UsbSetConfigurationFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  void OnComplete(bool success);

  DISALLOW_COPY_AND_ASSIGN(UsbSetConfigurationFunction);
};

class UsbGetConfigurationFunction : public UsbConnectionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("usb.getConfiguration", USB_GETCONFIGURATION)

  UsbGetConfigurationFunction();

 private:
  ~UsbGetConfigurationFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  DISALLOW_COPY_AND_ASSIGN(UsbGetConfigurationFunction);
};

class UsbListInterfacesFunction : public UsbConnectionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("usb.listInterfaces", USB_LISTINTERFACES)

  UsbListInterfacesFunction();

 private:
  ~UsbListInterfacesFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  DISALLOW_COPY_AND_ASSIGN(UsbListInterfacesFunction);
};

class UsbCloseDeviceFunction : public UsbConnectionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("usb.closeDevice", USB_CLOSEDEVICE)

  UsbCloseDeviceFunction();

 private:
  ~UsbCloseDeviceFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  DISALLOW_COPY_AND_ASSIGN(UsbCloseDeviceFunction);
};

class UsbClaimInterfaceFunction : public UsbConnectionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("usb.claimInterface", USB_CLAIMINTERFACE)

  UsbClaimInterfaceFunction();

 private:
  ~UsbClaimInterfaceFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  void OnComplete(bool success);

  DISALLOW_COPY_AND_ASSIGN(UsbClaimInterfaceFunction);
};

class UsbReleaseInterfaceFunction : public UsbConnectionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("usb.releaseInterface", USB_RELEASEINTERFACE)

  UsbReleaseInterfaceFunction();

 private:
  ~UsbReleaseInterfaceFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  void OnComplete(bool success);

  DISALLOW_COPY_AND_ASSIGN(UsbReleaseInterfaceFunction);
};

class UsbSetInterfaceAlternateSettingFunction : public UsbConnectionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("usb.setInterfaceAlternateSetting",
                             USB_SETINTERFACEALTERNATESETTING)

  UsbSetInterfaceAlternateSettingFunction();

 private:
  ~UsbSetInterfaceAlternateSettingFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  void OnComplete(bool success);

  DISALLOW_COPY_AND_ASSIGN(UsbSetInterfaceAlternateSettingFunction);
};

class UsbControlTransferFunction : public UsbTransferFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("usb.controlTransfer", USB_CONTROLTRANSFER)

  UsbControlTransferFunction();

 private:
  ~UsbControlTransferFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  DISALLOW_COPY_AND_ASSIGN(UsbControlTransferFunction);
};

class UsbBulkTransferFunction : public UsbTransferFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("usb.bulkTransfer", USB_BULKTRANSFER)

  UsbBulkTransferFunction();

 private:
  ~UsbBulkTransferFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  DISALLOW_COPY_AND_ASSIGN(UsbBulkTransferFunction);
};

class UsbInterruptTransferFunction : public UsbTransferFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("usb.interruptTransfer", USB_INTERRUPTTRANSFER)

  UsbInterruptTransferFunction();

 private:
  ~UsbInterruptTransferFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  DISALLOW_COPY_AND_ASSIGN(UsbInterruptTransferFunction);
};

class UsbIsochronousTransferFunction : public UsbConnectionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("usb.isochronousTransfer", USB_ISOCHRONOUSTRANSFER)

  UsbIsochronousTransferFunction();

 private:
  ~UsbIsochronousTransferFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  void OnCompleted(
      scoped_refptr<base::RefCountedBytes> data,
      const std::vector<device::UsbDeviceHandle::IsochronousPacket>& packets);

  DISALLOW_COPY_AND_ASSIGN(UsbIsochronousTransferFunction);
};

class UsbResetDeviceFunction : public UsbConnectionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("usb.resetDevice", USB_RESETDEVICE)

  UsbResetDeviceFunction();

 private:
  ~UsbResetDeviceFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  void OnComplete(bool success);

  std::unique_ptr<extensions::api::usb::ResetDevice::Params> parameters_;

  DISALLOW_COPY_AND_ASSIGN(UsbResetDeviceFunction);
};
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_USB_USB_API_H_
