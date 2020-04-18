// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/test_service.h"

#include <utility>

#include "base/logging.h"
#include "base/run_loop.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

const char kTestServiceUrl[] = "system:content_test_service";

TestService::TestService() : service_binding_(this) {
  registry_.AddInterface<mojom::TestService>(
      base::Bind(&TestService::Create, base::Unretained(this)));
}

TestService::~TestService() {
}

void TestService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  requestor_name_ = source_info.identity.name();
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void TestService::Create(mojom::TestServiceRequest request) {
  DCHECK(!service_binding_.is_bound());
  service_binding_.Bind(std::move(request));
}

void TestService::DoSomething(DoSomethingCallback callback) {
  std::move(callback).Run();
  base::RunLoop::QuitCurrentWhenIdleDeprecated();
}

void TestService::DoTerminateProcess(DoTerminateProcessCallback callback) {
  NOTREACHED();
}

void TestService::CreateFolder(CreateFolderCallback callback) {
  NOTREACHED();
}

void TestService::GetRequestorName(GetRequestorNameCallback callback) {
  std::move(callback).Run(requestor_name_);
}

void TestService::CreateSharedBuffer(const std::string& message,
                                     CreateSharedBufferCallback callback) {
  NOTREACHED();
}

}  // namespace content
