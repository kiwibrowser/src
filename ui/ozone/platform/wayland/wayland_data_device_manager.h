// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_WAYLAND_DATA_DEVICE_MANAGER_H_
#define UI_OZONE_PLATFORM_WAYLAND_WAYLAND_DATA_DEVICE_MANAGER_H_

#include <wayland-client.h>

#include "base/logging.h"
#include "base/macros.h"

namespace ui {

class WaylandConnection;

class WaylandDataDeviceManager {
 public:
  explicit WaylandDataDeviceManager(wl_data_device_manager* device_manager);
  ~WaylandDataDeviceManager();

  wl_data_device* GetDevice();
  wl_data_source* CreateSource();

  void set_connection(WaylandConnection* connection) {
    DCHECK(connection);
    connection_ = connection;
  }

 private:
  wl_data_device_manager* device_manager_;

  WaylandConnection* connection_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(WaylandDataDeviceManager);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_WAYLAND_DATA_DEVICE_MANAGER_H_
