// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/touch/touch_transform_setter_mus.h"

#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "ui/events/devices/touch_device_transform.h"

namespace ash {

TouchTransformSetterMus::TouchTransformSetterMus(
    service_manager::Connector* connector) {
  if (!connector)
    return;

  connector->BindInterface(ui::mojom::kServiceName, &touch_device_server_);
}

TouchTransformSetterMus::~TouchTransformSetterMus() = default;

void TouchTransformSetterMus::ConfigureTouchDevices(
    const std::vector<ui::TouchDeviceTransform>& transforms) {
  if (!touch_device_server_)
    return;  // May be null in tests.

  touch_device_server_->ConfigureTouchDevices(transforms);
}

}  // namespace ash
