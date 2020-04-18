// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/run_loop.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_runner.h"
#include "services/service_manager/tests/shutdown/shutdown_unittest.mojom.h"

namespace service_manager {

class ShutdownClientApp : public Service,
                          public mojom::ShutdownTestClientController,
                          public mojom::ShutdownTestClient {
 public:
  ShutdownClientApp() {
    registry_.AddInterface<mojom::ShutdownTestClientController>(
        base::Bind(&ShutdownClientApp::Create, base::Unretained(this)));
  }
  ~ShutdownClientApp() override {}

 private:
  // service_manager::Service:
  void OnBindInterface(const BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  void Create(mojom::ShutdownTestClientControllerRequest request) {
    bindings_.AddBinding(this, std::move(request));
  }

  // mojom::ShutdownTestClientController:
  void ConnectAndWait(ConnectAndWaitCallback callback) override {
    mojom::ShutdownTestServicePtr service;
    context()->connector()->BindInterface("shutdown_service", &service);

    mojo::Binding<mojom::ShutdownTestClient> client_binding(this);

    mojom::ShutdownTestClientPtr client_ptr;
    client_binding.Bind(mojo::MakeRequest(&client_ptr));

    service->SetClient(std::move(client_ptr));

    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    client_binding.set_connection_error_handler(run_loop.QuitClosure());
    run_loop.Run();

    std::move(callback).Run();
  }

  BinderRegistry registry_;
  mojo::BindingSet<mojom::ShutdownTestClientController> bindings_;

  DISALLOW_COPY_AND_ASSIGN(ShutdownClientApp);
};

}  // namespace service_manager

MojoResult ServiceMain(MojoHandle service_request_handle) {
  service_manager::ServiceRunner runner(new service_manager::ShutdownClientApp);
  return runner.Run(service_request_handle);
}
