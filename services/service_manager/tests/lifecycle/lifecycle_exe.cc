// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/service_runner.h"
#include "services/service_manager/tests/lifecycle/app_client.h"

MojoResult ServiceMain(MojoHandle service_request_handle) {
  service_manager::ServiceRunner runner(new service_manager::test::AppClient);
  return runner.Run(service_request_handle);
}
