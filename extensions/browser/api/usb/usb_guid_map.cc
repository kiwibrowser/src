// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/usb/usb_guid_map.h"

#include <utility>

#include "base/lazy_instance.h"
#include "base/strings/utf_string_conversions.h"
#include "device/base/device_client.h"
#include "device/usb/usb_device.h"
#include "device/usb/usb_service.h"
#include "extensions/common/api/usb.h"

namespace extensions {

namespace {

base::LazyInstance<BrowserContextKeyedAPIFactory<UsbGuidMap>>::Leaky
    g_usb_guid_map_factory = LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
UsbGuidMap* UsbGuidMap::Get(content::BrowserContext* browser_context) {
  return BrowserContextKeyedAPIFactory<UsbGuidMap>::Get(browser_context);
}

// static
BrowserContextKeyedAPIFactory<UsbGuidMap>* UsbGuidMap::GetFactoryInstance() {
  return g_usb_guid_map_factory.Pointer();
}

int UsbGuidMap::GetIdFromGuid(const std::string& guid) {
  auto iter = guid_to_id_map_.find(guid);
  if (iter == guid_to_id_map_.end()) {
    auto result = guid_to_id_map_.insert(std::make_pair(guid, next_id_++));
    DCHECK(result.second);
    iter = result.first;
    id_to_guid_map_.insert(std::make_pair(iter->second, guid));
  }
  return iter->second;
}

bool UsbGuidMap::GetGuidFromId(int id, std::string* guid) {
  auto iter = id_to_guid_map_.find(id);
  if (iter == id_to_guid_map_.end())
    return false;
  *guid = iter->second;
  return true;
}

void UsbGuidMap::GetApiDevice(scoped_refptr<const device::UsbDevice> device_in,
                              extensions::api::usb::Device* device_out) {
  device_out->device = GetIdFromGuid(device_in->guid());
  device_out->vendor_id = device_in->vendor_id();
  device_out->product_id = device_in->product_id();
  device_out->version = device_in->device_version();
  device_out->product_name = base::UTF16ToUTF8(device_in->product_string());
  device_out->manufacturer_name =
      base::UTF16ToUTF8(device_in->manufacturer_string());
  device_out->serial_number = base::UTF16ToUTF8(device_in->serial_number());
}

UsbGuidMap::UsbGuidMap(content::BrowserContext* browser_context)
    : browser_context_(browser_context), observer_(this) {
  device::UsbService* service = device::DeviceClient::Get()->GetUsbService();
  DCHECK(service);
  observer_.Add(service);
}

UsbGuidMap::~UsbGuidMap() {
}

void UsbGuidMap::OnDeviceRemovedCleanup(
    scoped_refptr<device::UsbDevice> device) {
  auto iter = guid_to_id_map_.find(device->guid());
  if (iter != guid_to_id_map_.end()) {
    int id = iter->second;
    guid_to_id_map_.erase(iter);
    id_to_guid_map_.erase(id);
  }
}

}  // namespace extensions
