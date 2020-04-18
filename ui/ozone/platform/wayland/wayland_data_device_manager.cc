// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/wayland_data_device_manager.h"

#include "ui/ozone/platform/wayland/wayland_connection.h"

namespace ui {

WaylandDataDeviceManager::WaylandDataDeviceManager(
    wl_data_device_manager* device_manager)
    : device_manager_(device_manager) {}

WaylandDataDeviceManager::~WaylandDataDeviceManager() = default;

wl_data_device* WaylandDataDeviceManager::GetDevice() {
  DCHECK(connection_->seat());
  return wl_data_device_manager_get_data_device(device_manager_,
                                                connection_->seat());
}

wl_data_source* WaylandDataDeviceManager::CreateSource() {
  return wl_data_device_manager_create_data_source(device_manager_);
}

}  // namespace ui
