// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/threading/simple_thread.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_runner.h"
#include "services/service_manager/public/mojom/service_factory.mojom.h"
#include "services/service_manager/tests/connect/connect_test.mojom.h"

// Tests that multiple services can be packaged in a single service by
// implementing ServiceFactory; that these services can be specified by
// the package's manifest and are thus registered with the PackageManager.

namespace service_manager {
namespace {

void QuitLoop(base::RunLoop* loop,
              mojom::ConnectResult* out_result,
              Identity* out_resolved_identity,
              mojom::ConnectResult result,
              const Identity& resolved_identity) {
  loop->Quit();
  *out_result = result;
  *out_resolved_identity = resolved_identity;
}

}  // namespace

using GetTitleCallback = test::mojom::ConnectTestService::GetTitleCallback;

class ProvidedService : public Service,
                        public test::mojom::ConnectTestService,
                        public test::mojom::BlockedInterface,
                        public test::mojom::UserIdTest,
                        public base::SimpleThread {
 public:
  ProvidedService(const std::string& title, mojom::ServiceRequest request)
      : base::SimpleThread(title),
        title_(title),
        request_(std::move(request)) {
    Start();
  }

  ~ProvidedService() override {
    Join();
  }

 private:
  // service_manager::Service:
  void OnStart() override {
    bindings_.set_connection_error_handler(
        base::Bind(&ProvidedService::OnConnectionError,
                   base::Unretained(this)));
    registry_.AddInterface<test::mojom::ConnectTestService>(
        base::Bind(&ProvidedService::BindConnectTestServiceRequest,
                   base::Unretained(this)));
    registry_.AddInterface<test::mojom::BlockedInterface>(base::Bind(
        &ProvidedService::BindBlockedInterfaceRequest, base::Unretained(this)));
    registry_.AddInterface<test::mojom::UserIdTest>(base::Bind(
        &ProvidedService::BindUserIdTestRequest, base::Unretained(this)));
  }
  void OnBindInterface(const BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe),
                            source_info);
  }

  void BindConnectTestServiceRequest(
      test::mojom::ConnectTestServiceRequest request,
      const BindSourceInfo& source_info) {
    bindings_.AddBinding(this, std::move(request));
    test::mojom::ConnectionStatePtr state(test::mojom::ConnectionState::New());
    state->connection_remote_name = source_info.identity.name();
    state->connection_remote_userid = source_info.identity.user_id();
    state->initialize_local_name = context()->identity().name();
    state->initialize_userid = context()->identity().user_id();

    context()->connector()->BindInterface(source_info.identity, &caller_);
    caller_->ConnectionAccepted(std::move(state));
  }

  void BindBlockedInterfaceRequest(test::mojom::BlockedInterfaceRequest request,
                                   const BindSourceInfo& source_info) {
    blocked_bindings_.AddBinding(this, std::move(request));
  }

  void BindUserIdTestRequest(test::mojom::UserIdTestRequest request,
                             const BindSourceInfo& source_info) {
    user_id_test_bindings_.AddBinding(this, std::move(request));
  }

  // test::mojom::ConnectTestService:
  void GetTitle(GetTitleCallback callback) override {
    std::move(callback).Run(title_);
  }

  void GetInstance(GetInstanceCallback callback) override {
    std::move(callback).Run(context()->identity().instance());
  }

  // test::mojom::BlockedInterface:
  void GetTitleBlocked(GetTitleBlockedCallback callback) override {
    std::move(callback).Run("Called Blocked Interface!");
  }

  // test::mojom::UserIdTest:
  void ConnectToClassAppAsDifferentUser(
      const service_manager::Identity& target,
      ConnectToClassAppAsDifferentUserCallback callback) override {
    context()->connector()->StartService(target);
    mojom::ConnectResult result;
    Identity resolved_identity;
    {
      base::RunLoop loop(base::RunLoop::Type::kNestableTasksAllowed);
      Connector::TestApi test_api(context()->connector());
      test_api.SetStartServiceCallback(
          base::Bind(&QuitLoop, &loop, &result, &resolved_identity));
      loop.Run();
    }
    std::move(callback).Run(static_cast<int32_t>(result), resolved_identity);
  }

  // base::SimpleThread:
  void Run() override {
    ServiceRunner(new ForwardingService(this)).Run(
        request_.PassMessagePipe().release().value(), false);
    caller_.reset();
    bindings_.CloseAllBindings();
    blocked_bindings_.CloseAllBindings();
    user_id_test_bindings_.CloseAllBindings();
  }

  void OnConnectionError() {
    if (bindings_.empty())
      base::RunLoop::QuitCurrentWhenIdleDeprecated();
  }

  const std::string title_;
  mojom::ServiceRequest request_;
  test::mojom::ExposedInterfacePtr caller_;
  BinderRegistryWithArgs<const BindSourceInfo&> registry_;
  mojo::BindingSet<test::mojom::ConnectTestService> bindings_;
  mojo::BindingSet<test::mojom::BlockedInterface> blocked_bindings_;
  mojo::BindingSet<test::mojom::UserIdTest> user_id_test_bindings_;

  DISALLOW_COPY_AND_ASSIGN(ProvidedService);
};

class ConnectTestService : public Service,
                           public mojom::ServiceFactory,
                           public test::mojom::ConnectTestService {
 public:
  ConnectTestService() {}
  ~ConnectTestService() override {}

 private:
  // service_manager::Service:
  void OnStart() override {
    base::Closure error_handler =
        base::Bind(&ConnectTestService::OnConnectionError,
                   base::Unretained(this));
    bindings_.set_connection_error_handler(error_handler);
    service_factory_bindings_.set_connection_error_handler(error_handler);
    registry_.AddInterface<ServiceFactory>(
        base::Bind(&ConnectTestService::BindServiceFactoryRequest,
                   base::Unretained(this)));
    registry_.AddInterface<test::mojom::ConnectTestService>(
        base::Bind(&ConnectTestService::BindConnectTestServiceRequest,
                   base::Unretained(this)));
  }
  void OnBindInterface(const BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  bool OnServiceManagerConnectionLost() override {
    provided_services_.clear();
    return true;
  }

  void BindServiceFactoryRequest(mojom::ServiceFactoryRequest request) {
    service_factory_bindings_.AddBinding(this, std::move(request));
  }

  void BindConnectTestServiceRequest(
      test::mojom::ConnectTestServiceRequest request) {
    bindings_.AddBinding(this, std::move(request));
  }

  // mojom::ServiceFactory:
  void CreateService(mojom::ServiceRequest request,
                     const std::string& name,
                     mojom::PIDReceiverPtr pid_receiver) override {
    if (name == "connect_test_a") {
      provided_services_.emplace_back(
          std::make_unique<ProvidedService>("A", std::move(request)));
    } else if (name == "connect_test_b") {
      provided_services_.emplace_back(
          std::make_unique<ProvidedService>("B", std::move(request)));
    }
  }

  // test::mojom::ConnectTestService:
  void GetTitle(GetTitleCallback callback) override {
    std::move(callback).Run("ROOT");
  }

  void GetInstance(GetInstanceCallback callback) override {
    std::move(callback).Run(context()->identity().instance());
  }

  void OnConnectionError() {
    if (bindings_.empty() && service_factory_bindings_.empty())
      context()->CreateQuitClosure().Run();
  }

  std::vector<std::unique_ptr<Service>> delegates_;
  mojo::BindingSet<mojom::ServiceFactory> service_factory_bindings_;
  BinderRegistry registry_;
  mojo::BindingSet<test::mojom::ConnectTestService> bindings_;
  std::list<std::unique_ptr<ProvidedService>> provided_services_;

  DISALLOW_COPY_AND_ASSIGN(ConnectTestService);
};

}  // namespace service_manager

MojoResult ServiceMain(MojoHandle service_request_handle) {
  service_manager::ServiceRunner runner(
      new service_manager::ConnectTestService);
  return runner.Run(service_request_handle);
}
