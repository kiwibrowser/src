// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_runner.h"

namespace service_manager {

class ConnectTestSingletonApp : public Service {
 public:
  ConnectTestSingletonApp() {}
  ~ConnectTestSingletonApp() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ConnectTestSingletonApp);
};

}  // namespace service_manager

MojoResult ServiceMain(MojoHandle service_request_handle) {
  service_manager::ServiceRunner runner(
      new service_manager::ConnectTestSingletonApp);
  return runner.Run(service_request_handle);
}
