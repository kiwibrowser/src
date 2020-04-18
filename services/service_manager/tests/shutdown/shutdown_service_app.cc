// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_runner.h"
#include "services/service_manager/tests/shutdown/shutdown_unittest.mojom.h"

namespace service_manager {
namespace {

service_manager::ServiceRunner* g_app = nullptr;

class ShutdownServiceApp : public Service, public mojom::ShutdownTestService {
 public:
  ShutdownServiceApp() {
    registry_.AddInterface<mojom::ShutdownTestService>(
        base::Bind(&ShutdownServiceApp::Create, base::Unretained(this)));
  }
  ~ShutdownServiceApp() override {}

 private:
  // service_manager::Service:
  void OnBindInterface(const BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  // mojom::ShutdownTestService:
  void SetClient(mojom::ShutdownTestClientPtr client) override {}
  void ShutDown() override { g_app->Quit(); }

  void Create(mojom::ShutdownTestServiceRequest request) {
    bindings_.AddBinding(this, std::move(request));
  }

  BinderRegistry registry_;
  mojo::BindingSet<mojom::ShutdownTestService> bindings_;

  DISALLOW_COPY_AND_ASSIGN(ShutdownServiceApp);
};

}  // namespace
}  // namespace service_manager

MojoResult ServiceMain(MojoHandle service_request_handle) {
  service_manager::ServiceRunner runner(
      new service_manager::ShutdownServiceApp);
  service_manager::g_app = &runner;
  return runner.Run(service_request_handle);
}
