// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/usb/usb_chooser_context.h"

#include <utility>
#include <vector>

#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "device/base/device_client.h"
#include "device/usb/public/mojom/device.mojom.h"
#include "device/usb/usb_device.h"

using device::UsbDevice;

namespace {

const char kDeviceNameKey[] = "name";
const char kGuidKey[] = "ephemeral-guid";
const char kProductIdKey[] = "product-id";
const char kSerialNumberKey[] = "serial-number";
const char kVendorIdKey[] = "vendor-id";

// Reasons a permission may be closed. These are used in histograms so do not
// remove/reorder entries. Only add at the end just before
// WEBUSB_PERMISSION_REVOKED_MAX. Also remember to update the enum listing in
// tools/metrics/histograms/histograms.xml.
enum WebUsbPermissionRevoked {
  // Permission to access a USB device was revoked by the user.
  WEBUSB_PERMISSION_REVOKED = 0,
  // Permission to access an ephemeral USB device was revoked by the user.
  WEBUSB_PERMISSION_REVOKED_EPHEMERAL,
  // Maximum value for the enum.
  WEBUSB_PERMISSION_REVOKED_MAX
};

void RecordPermissionRevocation(WebUsbPermissionRevoked kind) {
  UMA_HISTOGRAM_ENUMERATION("WebUsb.PermissionRevoked", kind,
                            WEBUSB_PERMISSION_REVOKED_MAX);
}

bool CanStorePersistentEntry(const scoped_refptr<const UsbDevice>& device) {
  return !device->serial_number().empty();
}

}  // namespace

UsbChooserContext::UsbChooserContext(Profile* profile)
    : ChooserContextBase(profile,
                         CONTENT_SETTINGS_TYPE_USB_GUARD,
                         CONTENT_SETTINGS_TYPE_USB_CHOOSER_DATA),
      is_incognito_(profile->IsOffTheRecord()),
      observer_(this),
      weak_factory_(this) {
  usb_service_ = device::DeviceClient::Get()->GetUsbService();
  if (usb_service_)
    observer_.Add(usb_service_);
}

UsbChooserContext::~UsbChooserContext() {}

std::vector<std::unique_ptr<base::DictionaryValue>>
UsbChooserContext::GetGrantedObjects(const GURL& requesting_origin,
                                     const GURL& embedding_origin) {
  std::vector<std::unique_ptr<base::DictionaryValue>> objects =
      ChooserContextBase::GetGrantedObjects(requesting_origin,
                                            embedding_origin);

  if (CanRequestObjectPermission(requesting_origin, embedding_origin)) {
    auto it = ephemeral_devices_.find(
        std::make_pair(requesting_origin, embedding_origin));
    if (it != ephemeral_devices_.end()) {
      for (const std::string& guid : it->second) {
        scoped_refptr<UsbDevice> device = usb_service_->GetDevice(guid);
        DCHECK(device);
        std::unique_ptr<base::DictionaryValue> object(
            new base::DictionaryValue());
        object->SetString(kDeviceNameKey, device->product_string());
        object->SetString(kGuidKey, device->guid());
        objects.push_back(std::move(object));
      }
    }
  }

  return objects;
}

std::vector<std::unique_ptr<ChooserContextBase::Object>>
UsbChooserContext::GetAllGrantedObjects() {
  std::vector<std::unique_ptr<ChooserContextBase::Object>> objects =
      ChooserContextBase::GetAllGrantedObjects();

  for (const auto& map_entry : ephemeral_devices_) {
    const GURL& requesting_origin = map_entry.first.first;
    const GURL& embedding_origin = map_entry.first.second;

    if (!CanRequestObjectPermission(requesting_origin, embedding_origin))
      continue;

    for (const std::string& guid : map_entry.second) {
      scoped_refptr<UsbDevice> device = usb_service_->GetDevice(guid);
      DCHECK(device);
      base::DictionaryValue object;
      object.SetString(kDeviceNameKey, device->product_string());
      object.SetString(kGuidKey, device->guid());
      objects.push_back(std::make_unique<ChooserContextBase::Object>(
          requesting_origin, embedding_origin, &object, "preference",
          is_incognito_));
    }
  }

  return objects;
}

void UsbChooserContext::RevokeObjectPermission(
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    const base::DictionaryValue& object) {
  std::string guid;
  if (object.GetString(kGuidKey, &guid)) {
    auto it = ephemeral_devices_.find(
        std::make_pair(requesting_origin, embedding_origin));
    if (it != ephemeral_devices_.end()) {
      it->second.erase(guid);
      if (it->second.empty())
        ephemeral_devices_.erase(it);
    }
    RecordPermissionRevocation(WEBUSB_PERMISSION_REVOKED_EPHEMERAL);
  } else {
    ChooserContextBase::RevokeObjectPermission(requesting_origin,
                                               embedding_origin, object);
    RecordPermissionRevocation(WEBUSB_PERMISSION_REVOKED);
  }
}

void UsbChooserContext::GrantDevicePermission(const GURL& requesting_origin,
                                              const GURL& embedding_origin,
                                              const std::string& guid) {
  scoped_refptr<UsbDevice> device = usb_service_->GetDevice(guid);
  if (!device)
    return;

  if (CanStorePersistentEntry(device)) {
    std::unique_ptr<base::DictionaryValue> device_dict(
        new base::DictionaryValue());
    device_dict->SetString(kDeviceNameKey, device->product_string());
    device_dict->SetInteger(kVendorIdKey, device->vendor_id());
    device_dict->SetInteger(kProductIdKey, device->product_id());
    device_dict->SetString(kSerialNumberKey, device->serial_number());
    GrantObjectPermission(requesting_origin, embedding_origin,
                          std::move(device_dict));
  } else {
    ephemeral_devices_[std::make_pair(requesting_origin, embedding_origin)]
        .insert(guid);
  }
}

bool UsbChooserContext::HasDevicePermission(
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    scoped_refptr<const device::UsbDevice> device) {
  if (!CanRequestObjectPermission(requesting_origin, embedding_origin))
    return false;

  auto it = ephemeral_devices_.find(
      std::make_pair(requesting_origin, embedding_origin));
  if (it != ephemeral_devices_.end() &&
      base::ContainsKey(it->second, device->guid())) {
    return true;
  }

  std::vector<std::unique_ptr<base::DictionaryValue>> device_list =
      GetGrantedObjects(requesting_origin, embedding_origin);
  for (const std::unique_ptr<base::DictionaryValue>& device_dict :
       device_list) {
    int vendor_id;
    int product_id;
    base::string16 serial_number;
    if (device_dict->GetInteger(kVendorIdKey, &vendor_id) &&
        device->vendor_id() == vendor_id &&
        device_dict->GetInteger(kProductIdKey, &product_id) &&
        device->product_id() == product_id &&
        device_dict->GetString(kSerialNumberKey, &serial_number) &&
        device->serial_number() == serial_number) {
      return true;
    }
  }

  return false;
}

base::WeakPtr<UsbChooserContext> UsbChooserContext::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

bool UsbChooserContext::IsValidObject(const base::DictionaryValue& object) {
  return object.size() == 4 && object.HasKey(kDeviceNameKey) &&
         object.HasKey(kVendorIdKey) && object.HasKey(kProductIdKey) &&
         object.HasKey(kSerialNumberKey);
}

std::string UsbChooserContext::GetObjectName(
    const base::DictionaryValue& object) {
  DCHECK(IsValidObject(object));
  std::string name;
  bool found = object.GetString(kDeviceNameKey, &name);
  DCHECK(found);
  return name;
}

void UsbChooserContext::OnDeviceRemovedCleanup(
    scoped_refptr<UsbDevice> device) {
  for (auto& map_entry : ephemeral_devices_)
    map_entry.second.erase(device->guid());
}
