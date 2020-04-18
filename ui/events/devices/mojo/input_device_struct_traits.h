// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_DEVICES_MOJO_INPUT_DEVICE_STRUCT_TRAITS_H_
#define UI_EVENTS_DEVICES_MOJO_INPUT_DEVICE_STRUCT_TRAITS_H_

#include <string>

#include "ui/events/devices/input_device.h"
#include "ui/events/devices/mojo/input_devices.mojom.h"
#include "ui/events/devices/stylus_state.h"
#include "ui/events/devices/touchscreen_device.h"
#include "ui/gfx/geometry/size.h"

namespace mojo {

template <>
struct EnumTraits<ui::mojom::InputDeviceType, ui::InputDeviceType> {
  static ui::mojom::InputDeviceType ToMojom(ui::InputDeviceType type);
  static bool FromMojom(ui::mojom::InputDeviceType type,
                        ui::InputDeviceType* output);
};

template <>
struct StructTraits<ui::mojom::InputDeviceDataView, ui::InputDevice> {
  static int32_t id(const ui::InputDevice& device) { return device.id; }

  static ui::InputDeviceType type(const ui::InputDevice& device) {
    return device.type;
  }

  static const std::string& name(const ui::InputDevice& device) {
    return device.name;
  }

  static bool enabled(const ui::InputDevice& device) { return device.enabled; }

  static std::string sys_path(const ui::InputDevice& device) {
    return device.sys_path.AsUTF8Unsafe();
  }

  static uint32_t vendor_id(const ui::InputDevice& device) {
    return device.vendor_id;
  }

  static uint32_t product_id(const ui::InputDevice& device) {
    return device.product_id;
  }

  static bool Read(ui::mojom::InputDeviceDataView data, ui::InputDevice* out);
};

template <>
struct EnumTraits<ui::mojom::StylusState, ui::StylusState> {
  static ui::mojom::StylusState ToMojom(ui::StylusState type);
  static bool FromMojom(ui::mojom::StylusState type, ui::StylusState* output);
};

template <>
struct StructTraits<ui::mojom::TouchscreenDeviceDataView,
                    ui::TouchscreenDevice> {
  static const ui::InputDevice& input_device(
      const ui::TouchscreenDevice& device) {
    return static_cast<const ui::InputDevice&>(device);
  }

  static const gfx::Size& size(const ui::TouchscreenDevice& device) {
    return device.size;
  }

  static int32_t touch_points(const ui::TouchscreenDevice& device) {
    return device.touch_points;
  }

  static bool has_stylus(const ui::TouchscreenDevice& device) {
    return device.has_stylus;
  }

  static bool Read(ui::mojom::TouchscreenDeviceDataView data,
                   ui::TouchscreenDevice* out);
};

}  // namespace mojo

#endif  // UI_EVENTS_DEVICES_MOJO_INPUT_DEVICE_STRUCT_TRAITS_H_
