// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_USB_MOJO_TYPE_CONVERTERS_H_
#define DEVICE_USB_MOJO_TYPE_CONVERTERS_H_

#include <vector>

#include "device/usb/public/mojom/device.mojom.h"
#include "device/usb/public/mojom/device_manager.mojom.h"
#include "device/usb/usb_device_handle.h"
#include "mojo/public/cpp/bindings/type_converter.h"

// Type converters to translate between internal device/usb data types and
// public Mojo interface data types. This must be included by any source file
// that uses these conversions explicitly or implicitly.

namespace device {
struct UsbConfigDescriptor;
class UsbDevice;
struct UsbEndpointDescriptor;
struct UsbInterfaceDescriptor;
}

namespace mojo {

template <>
struct TypeConverter<device::mojom::UsbEndpointInfoPtr,
                     device::UsbEndpointDescriptor> {
  static device::mojom::UsbEndpointInfoPtr Convert(
      const device::UsbEndpointDescriptor& endpoint);
};

template <>
struct TypeConverter<device::mojom::UsbAlternateInterfaceInfoPtr,
                     device::UsbInterfaceDescriptor> {
  static device::mojom::UsbAlternateInterfaceInfoPtr Convert(
      const device::UsbInterfaceDescriptor& iface);
};

// Note that this is an explicit vector-to-array conversion, as
// UsbInterfaceDescriptor collections are flattened to contain all alternate
// settings, whereas InterfaceInfos contain their own sets of alternates with
// a different structure type.
template <>
struct TypeConverter<std::vector<device::mojom::UsbInterfaceInfoPtr>,
                     std::vector<device::UsbInterfaceDescriptor>> {
  static std::vector<device::mojom::UsbInterfaceInfoPtr> Convert(
      const std::vector<device::UsbInterfaceDescriptor>& interfaces);
};

template <>
struct TypeConverter<device::mojom::UsbConfigurationInfoPtr,
                     device::UsbConfigDescriptor> {
  static device::mojom::UsbConfigurationInfoPtr Convert(
      const device::UsbConfigDescriptor& config);
};

template <>
struct TypeConverter<device::mojom::UsbDeviceInfoPtr, device::UsbDevice> {
  static device::mojom::UsbDeviceInfoPtr Convert(
      const device::UsbDevice& device);
};

template <>
struct TypeConverter<device::mojom::UsbIsochronousPacketPtr,
                     device::UsbDeviceHandle::IsochronousPacket> {
  static device::mojom::UsbIsochronousPacketPtr Convert(
      const device::UsbDeviceHandle::IsochronousPacket& packet);
};

template <typename A, typename B>
struct TypeConverter<std::vector<A>, std::vector<B>> {
  static std::vector<A> Convert(const std::vector<B>& input) {
    std::vector<A> result;
    result.reserve(input.size());
    for (const B& item : input)
      result.push_back(mojo::ConvertTo<A>(item));
    return result;
  };
};

}  // namespace mojo

#endif  // DEVICE_DEVICES_APP_USB_TYPE_CONVERTERS_H_
