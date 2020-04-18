// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/input_devices/touch_device_server.h"

#include <utility>
#include <vector>

#include "ui/display/manager/default_touch_transform_setter.h"
#include "ui/events/devices/input_device.h"
#include "ui/events/devices/touchscreen_device.h"

namespace ui {

TouchDeviceServer::TouchDeviceServer()
    : touch_transform_setter_(
          std::make_unique<display::DefaultTouchTransformSetter>()) {}

TouchDeviceServer::~TouchDeviceServer() {}

void TouchDeviceServer::AddBinding(mojom::TouchDeviceServerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void TouchDeviceServer::ConfigureTouchDevices(
    const std::vector<ui::TouchDeviceTransform>& transforms) {
  touch_transform_setter_->ConfigureTouchDevices(transforms);
}

}  // namespace ui
