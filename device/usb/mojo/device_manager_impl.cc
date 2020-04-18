// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/usb/mojo/device_manager_impl.h"

#include <stddef.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "device/base/device_client.h"
#include "device/usb/mojo/device_impl.h"
#include "device/usb/mojo/permission_provider.h"
#include "device/usb/mojo/type_converters.h"
#include "device/usb/public/cpp/filter_utils.h"
#include "device/usb/public/mojom/device.mojom.h"
#include "device/usb/usb_device.h"
#include "device/usb/usb_service.h"

namespace device {
namespace usb {

// static
void DeviceManagerImpl::Create(
    base::WeakPtr<PermissionProvider> permission_provider,
    mojom::UsbDeviceManagerRequest request) {
  DCHECK(DeviceClient::Get());
  UsbService* service = DeviceClient::Get()->GetUsbService();
  if (!service)
    return;

  auto* device_manager_impl =
      new DeviceManagerImpl(std::move(permission_provider), service);
  device_manager_impl->binding_ = mojo::MakeStrongBinding(
      base::WrapUnique(device_manager_impl), std::move(request));
}

DeviceManagerImpl::DeviceManagerImpl(
    base::WeakPtr<PermissionProvider> permission_provider,
    UsbService* usb_service)
    : permission_provider_(permission_provider),
      usb_service_(usb_service),
      observer_(this),
      weak_factory_(this) {
  // This object owns itself and will be destroyed if the message pipe it is
  // bound to is closed, the message loop is destructed, or the UsbService is
  // shut down.
  observer_.Add(usb_service_);
}

DeviceManagerImpl::~DeviceManagerImpl() = default;

void DeviceManagerImpl::GetDevices(mojom::UsbEnumerationOptionsPtr options,
                                   GetDevicesCallback callback) {
  usb_service_->GetDevices(
      base::Bind(&DeviceManagerImpl::OnGetDevices, weak_factory_.GetWeakPtr(),
                 base::Passed(&options), base::Passed(&callback)));
}

void DeviceManagerImpl::GetDevice(const std::string& guid,
                                  mojom::UsbDeviceRequest device_request) {
  scoped_refptr<UsbDevice> device = usb_service_->GetDevice(guid);
  if (!device)
    return;

  if (permission_provider_ &&
      permission_provider_->HasDevicePermission(device)) {
    DeviceImpl::Create(std::move(device), permission_provider_,
                       std::move(device_request));
  }
}

void DeviceManagerImpl::SetClient(mojom::UsbDeviceManagerClientPtr client) {
  client_ = std::move(client);
}

void DeviceManagerImpl::OnGetDevices(
    mojom::UsbEnumerationOptionsPtr options,
    GetDevicesCallback callback,
    const std::vector<scoped_refptr<UsbDevice>>& devices) {
  std::vector<mojom::UsbDeviceFilterPtr> filters;
  if (options)
    filters.swap(options->filters);

  std::vector<mojom::UsbDeviceInfoPtr> device_infos;
  for (const auto& device : devices) {
    if (UsbDeviceFilterMatchesAny(filters, *device)) {
      if (permission_provider_ &&
          permission_provider_->HasDevicePermission(device)) {
        device_infos.push_back(mojom::UsbDeviceInfo::From(*device));
      }
    }
  }

  std::move(callback).Run(std::move(device_infos));
}

void DeviceManagerImpl::OnDeviceAdded(scoped_refptr<UsbDevice> device) {
  if (client_ && permission_provider_ &&
      permission_provider_->HasDevicePermission(device))
    client_->OnDeviceAdded(mojom::UsbDeviceInfo::From(*device));
}

void DeviceManagerImpl::OnDeviceRemoved(scoped_refptr<UsbDevice> device) {
  if (client_ && permission_provider_ &&
      permission_provider_->HasDevicePermission(device))
    client_->OnDeviceRemoved(mojom::UsbDeviceInfo::From(*device));
}

void DeviceManagerImpl::WillDestroyUsbService() {
  binding_->Close();
}

}  // namespace usb
}  // namespace device
