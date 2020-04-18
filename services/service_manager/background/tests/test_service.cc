// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/background/tests/test.mojom.h"
#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_runner.h"

namespace service_manager {

// A service that exports a simple interface for testing. Used to test the
// parent background service manager.
class TestClient : public Service,
                   public mojom::TestService {
 public:
  TestClient() {
    registry_.AddInterface(base::Bind(&TestClient::BindTestServiceRequest,
                                      base::Unretained(this)));
  }
  ~TestClient() override {}

 private:
  // Service:
  void OnBindInterface(const BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  // mojom::TestService
  void Test(TestCallback callback) override { std::move(callback).Run(); }

  void BindTestServiceRequest(mojom::TestServiceRequest request) {
    bindings_.AddBinding(this, std::move(request));
  }

  void Quit() override { context()->CreateQuitClosure().Run(); }

  BinderRegistry registry_;
  mojo::BindingSet<mojom::TestService> bindings_;

  DISALLOW_COPY_AND_ASSIGN(TestClient);
};

}  // namespace service_manager

MojoResult ServiceMain(MojoHandle service_request_handle) {
  service_manager::ServiceRunner runner(new service_manager::TestClient);
  return runner.Run(service_request_handle);
}
