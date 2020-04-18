// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/usb/mojo/type_converters.h"

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <utility>

#include "device/usb/usb_descriptors.h"
#include "device/usb/usb_device.h"

namespace mojo {

// static
device::mojom::UsbEndpointInfoPtr TypeConverter<
    device::mojom::UsbEndpointInfoPtr,
    device::UsbEndpointDescriptor>::Convert(const device::UsbEndpointDescriptor&
                                                endpoint) {
  auto info = device::mojom::UsbEndpointInfo::New();
  info->endpoint_number = endpoint.address & 0xf;
  info->direction = endpoint.direction;
  info->type = endpoint.transfer_type;
  info->packet_size = static_cast<uint32_t>(endpoint.maximum_packet_size);
  return info;
}

// static
device::mojom::UsbAlternateInterfaceInfoPtr
TypeConverter<device::mojom::UsbAlternateInterfaceInfoPtr,
              device::UsbInterfaceDescriptor>::
    Convert(const device::UsbInterfaceDescriptor& interface) {
  auto info = device::mojom::UsbAlternateInterfaceInfo::New();
  info->alternate_setting = interface.alternate_setting;
  info->class_code = interface.interface_class;
  info->subclass_code = interface.interface_subclass;
  info->protocol_code = interface.interface_protocol;

  // Filter out control endpoints for the public interface.
  info->endpoints.reserve(interface.endpoints.size());
  for (const auto& endpoint : interface.endpoints) {
    if (endpoint.transfer_type != device::UsbTransferType::CONTROL)
      info->endpoints.push_back(device::mojom::UsbEndpointInfo::From(endpoint));
  }

  return info;
}

// static
std::vector<device::mojom::UsbInterfaceInfoPtr>
TypeConverter<std::vector<device::mojom::UsbInterfaceInfoPtr>,
              std::vector<device::UsbInterfaceDescriptor>>::
    Convert(const std::vector<device::UsbInterfaceDescriptor>& interfaces) {
  std::vector<device::mojom::UsbInterfaceInfoPtr> infos;

  // Aggregate each alternate setting into an InterfaceInfo corresponding to its
  // interface number.
  std::map<uint8_t, device::mojom::UsbInterfaceInfo*> interface_map;
  for (size_t i = 0; i < interfaces.size(); ++i) {
    auto alternate =
        device::mojom::UsbAlternateInterfaceInfo::From(interfaces[i]);
    auto iter = interface_map.find(interfaces[i].interface_number);
    if (iter == interface_map.end()) {
      // This is the first time we're seeing an alternate with this interface
      // number, so add a new InterfaceInfo to the array and map the number.
      auto info = device::mojom::UsbInterfaceInfo::New();
      info->interface_number = interfaces[i].interface_number;
      iter = interface_map
                 .insert(
                     std::make_pair(interfaces[i].interface_number, info.get()))
                 .first;
      infos.push_back(std::move(info));
    }
    iter->second->alternates.push_back(std::move(alternate));
  }

  return infos;
}

// static
device::mojom::UsbConfigurationInfoPtr TypeConverter<
    device::mojom::UsbConfigurationInfoPtr,
    device::UsbConfigDescriptor>::Convert(const device::UsbConfigDescriptor&
                                              config) {
  auto info = device::mojom::UsbConfigurationInfo::New();
  info->configuration_value = config.configuration_value;
  info->interfaces =
      mojo::ConvertTo<std::vector<device::mojom::UsbInterfaceInfoPtr>>(
          config.interfaces);
  return info;
}

// static
device::mojom::UsbDeviceInfoPtr
TypeConverter<device::mojom::UsbDeviceInfoPtr, device::UsbDevice>::Convert(
    const device::UsbDevice& device) {
  auto info = device::mojom::UsbDeviceInfo::New();
  info->guid = device.guid();
  info->usb_version_major = device.usb_version() >> 8;
  info->usb_version_minor = device.usb_version() >> 4 & 0xf;
  info->usb_version_subminor = device.usb_version() & 0xf;
  info->class_code = device.device_class();
  info->subclass_code = device.device_subclass();
  info->protocol_code = device.device_protocol();
  info->vendor_id = device.vendor_id();
  info->product_id = device.product_id();
  info->device_version_major = device.device_version() >> 8;
  info->device_version_minor = device.device_version() >> 4 & 0xf;
  info->device_version_subminor = device.device_version() & 0xf;
  info->manufacturer_name = device.manufacturer_string();
  info->product_name = device.product_string();
  info->serial_number = device.serial_number();
  const device::UsbConfigDescriptor* config = device.active_configuration();
  info->active_configuration = config ? config->configuration_value : 0;
  info->configurations =
      mojo::ConvertTo<std::vector<device::mojom::UsbConfigurationInfoPtr>>(
          device.configurations());
  return info;
}

// static
device::mojom::UsbIsochronousPacketPtr
TypeConverter<device::mojom::UsbIsochronousPacketPtr,
              device::UsbDeviceHandle::IsochronousPacket>::
    Convert(const device::UsbDeviceHandle::IsochronousPacket& packet) {
  auto info = device::mojom::UsbIsochronousPacket::New();
  info->length = packet.length;
  info->transferred_length = packet.transferred_length;
  info->status =
      mojo::ConvertTo<device::mojom::UsbTransferStatus>(packet.status);
  return info;
}

}  // namespace mojo
