// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/test/echo/echo_service.h"

#include "services/service_manager/public/cpp/service_context.h"

namespace echo {

std::unique_ptr<service_manager::Service> CreateEchoService() {
  return std::make_unique<EchoService>();
}

EchoService::EchoService() {
  registry_.AddInterface<mojom::Echo>(
      base::Bind(&EchoService::BindEchoRequest, base::Unretained(this)));
}

EchoService::~EchoService() {}

void EchoService::OnStart() {}

void EchoService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void EchoService::BindEchoRequest(mojom::EchoRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void EchoService::EchoString(const std::string& input,
                             EchoStringCallback callback) {
  std::move(callback).Run(input);
}

void EchoService::Quit() {
  context()->CreateQuitClosure().Run();
}

}  // namespace echo
