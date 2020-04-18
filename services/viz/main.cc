// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/public/c/main.h"
#include "base/message_loop/message_loop.h"
#include "services/service_manager/public/cpp/service_runner.h"
#include "services/viz/service.h"

MojoResult ServiceMain(MojoHandle service_request_handle) {
  viz::Service* viz_service = new viz::Service();
  service_manager::ServiceRunner runner(viz_service);
  runner.set_message_loop_type(base::MessageLoop::TYPE_UI);
  return runner.Run(service_request_handle);
}
