// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/public/c/main.h"
#include "base/message_loop/message_loop.h"
#include "services/service_manager/public/cpp/service_runner.h"
#include "services/ui/service.h"

MojoResult ServiceMain(MojoHandle service_request_handle) {
  ui::Service::InitParams params;
  params.running_standalone = true;
  ui::Service* ui_service = new ui::Service(params);
  service_manager::ServiceRunner runner(ui_service);
  runner.set_message_loop_type(base::MessageLoop::TYPE_UI);
  return runner.Run(service_request_handle);
}
