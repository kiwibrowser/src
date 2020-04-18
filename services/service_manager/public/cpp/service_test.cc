// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/public/cpp/service_test.h"

#include "base/run_loop.h"
#include "base/threading/thread.h"
#include "base/values.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"
#include "services/service_manager/background/background_service_manager.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace service_manager {
namespace test {

ServiceTestClient::ServiceTestClient(ServiceTest* test) : test_(test) {}

ServiceTestClient::~ServiceTestClient() {}

void ServiceTestClient::OnStart() {
  test_->OnStartCalled(context()->connector(), context()->identity().name(),
                       context()->identity().user_id());
}

void ServiceTestClient::OnBindInterface(
    const BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {}

ServiceTest::ServiceTest() : ServiceTest(std::string()) {}

ServiceTest::ServiceTest(const std::string& test_name,
                         base::test::ScopedTaskEnvironment::MainThreadType type)
    : scoped_task_environment_(type), test_name_(test_name) {}

ServiceTest::~ServiceTest() {}

void ServiceTest::InitTestName(const std::string& test_name) {
  DCHECK(test_name_.empty());
  test_name_ = test_name;
}

std::unique_ptr<Service> ServiceTest::CreateService() {
  return std::make_unique<ServiceTestClient>(this);
}

std::unique_ptr<base::Value> ServiceTest::CreateCustomTestCatalog() {
  return nullptr;
}

void ServiceTest::OnStartCalled(Connector* connector,
                                const std::string& name,
                                const std::string& user_id) {
  DCHECK_EQ(connector_, connector);
  initialize_name_ = name;
  initialize_userid_ = user_id;
  initialize_called_.Run();
}

void ServiceTest::SetUp() {
  background_service_manager_ =
      std::make_unique<service_manager::BackgroundServiceManager>(
          nullptr, CreateCustomTestCatalog());

  // Create the service manager connection. We don't proceed until we get our
  // Service's OnStart() method is called.
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  initialize_called_ = run_loop.QuitClosure();

  mojom::ServicePtr service;
  context_ = std::make_unique<ServiceContext>(CreateService(),
                                              mojo::MakeRequest(&service));
  background_service_manager_->RegisterService(
      Identity(test_name_, mojom::kRootUserID), std::move(service), nullptr);
  connector_ = context_->connector();
  run_loop.Run();
}

void ServiceTest::TearDown() {
  background_service_manager_.reset();
  context_.reset();
}

}  // namespace test
}  // namespace service_manager
