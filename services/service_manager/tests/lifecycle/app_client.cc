// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/tests/lifecycle/app_client.h"

#include "base/macros.h"
#include "base/run_loop.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace service_manager {
namespace test {

AppClient::AppClient() {
  registry_.AddInterface<mojom::LifecycleControl>(
      base::Bind(&AppClient::Create, base::Unretained(this)));
}

AppClient::~AppClient() {}

void AppClient::OnBindInterface(const BindSourceInfo& source_info,
                                const std::string& interface_name,
                                mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

bool AppClient::OnServiceManagerConnectionLost() {
  base::RunLoop::QuitCurrentWhenIdleDeprecated();
  return true;
}

void AppClient::Create(mojom::LifecycleControlRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void AppClient::Ping(PingCallback callback) {
  std::move(callback).Run();
}

void AppClient::GracefulQuit() {
  context()->CreateQuitClosure().Run();
}

void AppClient::Crash() {
  // Rather than actually crash, which causes a bunch of console spray and
  // maybe UI clutter on some platforms, just exit without shutting anything
  // down properly.
  exit(1);
}

void AppClient::CloseServiceManagerConnection() {
  context()->DisconnectFromServiceManager();
  bindings_.set_connection_error_handler(
      base::Bind(&AppClient::BindingLost, base::Unretained(this)));
}

void AppClient::BindingLost() {
  if (bindings_.empty())
    OnServiceManagerConnectionLost();
}

}  // namespace test
}  // namespace service_manager
