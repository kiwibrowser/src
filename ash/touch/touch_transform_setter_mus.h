// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TOUCH_TOUCH_TRANSFORM_SETTER_MUS_H_
#define ASH_TOUCH_TOUCH_TRANSFORM_SETTER_MUS_H_

#include "base/macros.h"
#include "services/ui/public/interfaces/input_devices/touch_device_server.mojom.h"
#include "ui/display/manager/touch_transform_setter.h"

namespace service_manager {
class Connector;
}

namespace ash {

// display::TouchTransformSetter implementation for mus. Updates are applied
// by way of ui::mojom::TouchDeviceServer.
class TouchTransformSetterMus : public display::TouchTransformSetter {
 public:
  explicit TouchTransformSetterMus(service_manager::Connector* connector);
  ~TouchTransformSetterMus() override;

  // TouchTransformSetter:
  void ConfigureTouchDevices(
      const std::vector<ui::TouchDeviceTransform>& transforms) override;

 private:
  ui::mojom::TouchDeviceServerPtr touch_device_server_;

  DISALLOW_COPY_AND_ASSIGN(TouchTransformSetterMus);
};

}  // namespace ash

#endif  // ASH_TOUCH_TOUCH_TRANSFORM_SETTER_MUS_H_
