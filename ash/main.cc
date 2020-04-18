// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/public/c/main.h"
#include "ash/window_manager_service.h"
#include "services/service_manager/public/cpp/service_runner.h"

MojoResult ServiceMain(MojoHandle service_request_handle) {
  const bool show_primary_host_on_connect = true;
  ash::WindowManagerService* service =
      new ash::WindowManagerService(show_primary_host_on_connect);
  service->set_running_standalone(true);
  service_manager::ServiceRunner runner(service);
  return runner.Run(service_request_handle);
}
