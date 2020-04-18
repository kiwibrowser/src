// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_runner.h"
#include "services/service_manager/tests/connect/connect_test.mojom.h"

using service_manager::test::mojom::ConnectTestService;
using service_manager::test::mojom::ConnectTestServiceRequest;

namespace {

class Target : public service_manager::Service,
               public ConnectTestService {
 public:
  Target() {
    registry_.AddInterface<ConnectTestService>(
        base::Bind(&Target::Create, base::Unretained(this)));
  }
  ~Target() override {}

 private:
  // service_manager::Service:
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  void Create(ConnectTestServiceRequest request) {
    bindings_.AddBinding(this, std::move(request));
  }

  // ConnectTestService:
  void GetTitle(GetTitleCallback callback) override {
    std::move(callback).Run("connect_test_exe");
  }

  void GetInstance(GetInstanceCallback callback) override {
    std::move(callback).Run(context()->identity().instance());
  }

  service_manager::BinderRegistry registry_;
  mojo::BindingSet<ConnectTestService> bindings_;

  DISALLOW_COPY_AND_ASSIGN(Target);
};

}  // namespac

MojoResult ServiceMain(MojoHandle service_request_handle) {
  service_manager::ServiceRunner runner(new Target);
  return runner.Run(service_request_handle);
}
