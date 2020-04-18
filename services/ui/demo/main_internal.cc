// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/service_runner.h"
#include "services/ui/demo/mus_demo_internal.h"

MojoResult ServiceMain(MojoHandle service_request_handle) {
  service_manager::ServiceRunner runner(new ui::demo::MusDemoInternal);
  return runner.Run(service_request_handle);
}
