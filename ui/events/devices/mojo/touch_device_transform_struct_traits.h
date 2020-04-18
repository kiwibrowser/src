// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_DISPLAY_MANAGER_CHROMEOS_MOJO_TOUCH_DEVICE_TRANSFORM_STRUCT_TRAITS_H_
#define UI_DISPLAY_MANAGER_CHROMEOS_MOJO_TOUCH_DEVICE_TRANSFORM_STRUCT_TRAITS_H_

#include <stdint.h>

#include "ui/events/devices/mojo/touch_device_transform.mojom.h"
#include "ui/events/devices/touch_device_transform.h"
#include "ui/gfx/mojo/transform_struct_traits.h"

namespace mojo {

template <>
struct StructTraits<ui::mojom::TouchDeviceTransformDataView,
                    ui::TouchDeviceTransform> {
 public:
  static int64_t display_id(const ui::TouchDeviceTransform& r) {
    return r.display_id;
  }
  static int32_t device_id(const ui::TouchDeviceTransform& r) {
    return r.device_id;
  }
  static const gfx::Transform& transform(const ui::TouchDeviceTransform& r) {
    return r.transform;
  }
  static double radius_scale(const ui::TouchDeviceTransform& r) {
    return r.radius_scale;
  }

  static bool Read(ui::mojom::TouchDeviceTransformDataView data,
                   ui::TouchDeviceTransform* out) {
    out->display_id = data.display_id();
    out->device_id = data.device_id();
    if (!data.ReadTransform(&(out->transform)))
      return false;
    out->radius_scale = data.radius_scale();
    return true;
  }
};

}  // namespace mojo

#endif  // UI_DISPLAY_MANAGER_CHROMEOS_MOJO_TOUCH_DEVICE_TRANSFORM_STRUCT_TRAITS_H_
