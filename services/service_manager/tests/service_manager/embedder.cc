// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_runner.h"
#include "services/service_manager/public/mojom/service_factory.mojom.h"
#include "services/service_manager/public/mojom/service_manager.mojom.h"

namespace {

class RegularService : public service_manager::Service {
 public:
  RegularService() = default;
  ~RegularService() override = default;

 private:
  // service_manager::Service:
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {}

  DISALLOW_COPY_AND_ASSIGN(RegularService);
};

class AllUsersService : public service_manager::Service {
 public:
  AllUsersService() = default;
  ~AllUsersService() override = default;

 private:
  // service_manager::Service:
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {}

  DISALLOW_COPY_AND_ASSIGN(AllUsersService);
};

class SingletonService : public service_manager::Service {
 public:
  explicit SingletonService() = default;
  ~SingletonService() override = default;

 private:
  // service_manager::Service:
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {}

  DISALLOW_COPY_AND_ASSIGN(SingletonService);
};

class Embedder : public service_manager::Service,
                 public service_manager::mojom::ServiceFactory {
 public:
  Embedder() {
    registry_.AddInterface<service_manager::mojom::ServiceFactory>(
        base::Bind(&Embedder::Create, base::Unretained(this)));
  }
  ~Embedder() override {}

 private:
  // service_manager::Service:
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  bool OnServiceManagerConnectionLost() override {
    base::RunLoop::QuitCurrentWhenIdleDeprecated();
    return true;
  }

  void Create(service_manager::mojom::ServiceFactoryRequest request) {
    service_factory_bindings_.AddBinding(this, std::move(request));
  }

  // mojom::ServiceFactory:
  void CreateService(
      service_manager::mojom::ServiceRequest request,
      const std::string& name,
      service_manager::mojom::PIDReceiverPtr pid_receiver) override {
    if (name == "service_manager_unittest_all_users") {
      context_.reset(new service_manager::ServiceContext(
          std::make_unique<AllUsersService>(), std::move(request)));
    } else if (name == "service_manager_unittest_singleton") {
      context_.reset(new service_manager::ServiceContext(
          std::make_unique<SingletonService>(), std::move(request)));
    } else if (name == "service_manager_unittest_regular") {
      context_.reset(new service_manager::ServiceContext(
          std::make_unique<RegularService>(), std::move(request)));
    } else {
      LOG(ERROR) << "Failed to create unknow service " << name;
    }
  }

  std::unique_ptr<service_manager::ServiceContext> context_;
  service_manager::BinderRegistry registry_;
  mojo::BindingSet<service_manager::mojom::ServiceFactory>
      service_factory_bindings_;

  DISALLOW_COPY_AND_ASSIGN(Embedder);
};

}  // namespace

MojoResult ServiceMain(MojoHandle service_request_handle) {
  service_manager::ServiceRunner runner(new Embedder);
  return runner.Run(service_request_handle);
}
