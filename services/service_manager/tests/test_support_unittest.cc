// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context_ref.h"
#include "services/service_manager/public/cpp/test/test_connector_factory.h"
#include "services/service_manager/tests/test.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace service_manager {

namespace {

// TestBImpl and TestCImpl are simple test interfaces whose methods invokes
// their callback when called without doing anything.
class TestBImpl : public TestB {
 public:
  explicit TestBImpl(std::unique_ptr<ServiceContextRef> service_ref)
      : service_ref_(std::move(service_ref)) {}
  ~TestBImpl() override = default;

 private:
  // TestB:
  void B(BCallback callback) override { std::move(callback).Run(); }
  void CallC(CallCCallback callback) override { std::move(callback).Run(); }

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  DISALLOW_COPY_AND_ASSIGN(TestBImpl);
};

class TestCImpl : public TestC {
 public:
  explicit TestCImpl(std::unique_ptr<ServiceContextRef> service_ref)
      : service_ref_(std::move(service_ref)) {}
  ~TestCImpl() override = default;

 private:
  // TestC:
  void C(CCallback callback) override { std::move(callback).Run(); }

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  DISALLOW_COPY_AND_ASSIGN(TestCImpl);
};

void OnTestBRequest(service_manager::ServiceContextRefFactory* ref_factory,
                    TestBRequest request) {
  mojo::MakeStrongBinding(std::make_unique<TestBImpl>(ref_factory->CreateRef()),
                          std::move(request));
}

void OnTestCRequest(service_manager::ServiceContextRefFactory* ref_factory,
                    TestCRequest request) {
  mojo::MakeStrongBinding(std::make_unique<TestCImpl>(ref_factory->CreateRef()),
                          std::move(request));
}

// This is a test service used to demonstrate usage of TestConnectorFactory.
// See documentation on TestConnectorFactory for more details about usage.
class TestServiceImplBase : public Service {
 public:
  TestServiceImplBase() = default;
  ~TestServiceImplBase() override = default;

 private:
  // Service:
  void OnStart() override {
    ref_factory_.reset(new ServiceContextRefFactory(base::DoNothing()));
    RegisterInterfaces(&registry_, ref_factory_.get());
  }

  void OnBindInterface(const BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  virtual void RegisterInterfaces(
      service_manager::BinderRegistry* registry,
      service_manager::ServiceContextRefFactory* ref_factory) = 0;

  // State needed to manage service lifecycle and lifecycle of bound clients.
  std::unique_ptr<service_manager::ServiceContextRefFactory> ref_factory_;
  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(TestServiceImplBase);
};

class TestBServiceImpl : public TestServiceImplBase {
 private:
  void RegisterInterfaces(
      service_manager::BinderRegistry* registry,
      service_manager::ServiceContextRefFactory* ref_factory) override {
    registry->AddInterface(base::Bind(&OnTestBRequest, ref_factory));
  }
};

class TestCServiceImpl : public TestServiceImplBase {
 private:
  void RegisterInterfaces(
      service_manager::BinderRegistry* registry,
      service_manager::ServiceContextRefFactory* ref_factory) override {
    registry->AddInterface(base::Bind(&OnTestCRequest, ref_factory));
  }
};

constexpr char kServiceBName[] = "ServiceB";
constexpr char kServiceCName[] = "ServiceC";

}  // namespace

TEST(ServiceManagerTestSupport, TestConnectorFactoryUniqueService) {
  base::test::ScopedTaskEnvironment task_environment;

  std::unique_ptr<TestConnectorFactory> factory =
      TestConnectorFactory::CreateForUniqueService(
          std::make_unique<TestCServiceImpl>());
  std::unique_ptr<Connector> connector = factory->CreateConnector();
  TestCPtr c;
  connector->BindInterface(TestC::Name_, &c);
  base::RunLoop loop;
  c->C(loop.QuitClosure());
  loop.Run();
}

TEST(ServiceManagerTestSupport, TestConnectorFactoryMultipleServices) {
  base::test::ScopedTaskEnvironment task_environment;

  TestConnectorFactory::NameToServiceMap services;
  services.insert(
      std::make_pair(kServiceBName, std::make_unique<TestBServiceImpl>()));
  services.insert(
      std::make_pair(kServiceCName, std::make_unique<TestCServiceImpl>()));
  std::unique_ptr<TestConnectorFactory> factory =
      TestConnectorFactory::CreateForServices(std::move(services));
  std::unique_ptr<Connector> connector = factory->CreateConnector();

  {
    TestBPtr b;
    connector->BindInterface(kServiceBName, &b);
    base::RunLoop loop;
    b->B(loop.QuitClosure());
    loop.Run();
  }

  {
    TestCPtr c;
    connector->BindInterface(kServiceCName, &c);
    base::RunLoop loop;
    c->C(loop.QuitClosure());
    loop.Run();
  }
}

}  // namespace service_manager
