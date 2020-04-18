// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_runner.h"
#include "services/service_manager/tests/service_manager/service_manager_unittest.mojom.h"

using service_manager::test::mojom::CreateInstanceTestPtr;

namespace {

class Target : public service_manager::Service {
 public:
  Target() {}
  ~Target() override {}

 private:
  // service_manager::Service:
  void OnStart() override {
    CreateInstanceTestPtr service;
    context()->connector()->BindInterface("service_manager_unittest",
                                          &service);
    service->SetTargetIdentity(context()->identity());
  }
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {}

  DISALLOW_COPY_AND_ASSIGN(Target);
};

}  // namespace

MojoResult ServiceMain(MojoHandle service_request_handle) {
  service_manager::ServiceRunner runner(new Target);
  return runner.Run(service_request_handle);
}
