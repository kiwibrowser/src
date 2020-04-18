// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_PUBLIC_CPP_TEST_TEST_SERVICE_DECORATOR_H_
#define SERVICES_SERVICE_MANAGER_PUBLIC_CPP_TEST_TEST_SERVICE_DECORATOR_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

namespace service_manager {

// This class allows tests to set custom implementations for an arbitrary
// service. If there is no override for an interface, then the OnBindInterface
// call is delegated to the |decorated_service_|.
class TestServiceDecorator : public Service {
 public:
  // Tuple of interface name, binder, and task runner.
  using InterfaceOverride =
      std::tuple<std::string,
                 BinderRegistryWithArgs<const BindSourceInfo&>::Binder,
                 scoped_refptr<base::SequencedTaskRunner>>;

  static std::unique_ptr<TestServiceDecorator> CreateServiceWithUniqueOverride(
      std::unique_ptr<Service> decorated_service,
      const std::string& interface_name,
      const BinderRegistryWithArgs<const BindSourceInfo&>::Binder& binder,
      const scoped_refptr<base::SequencedTaskRunner>& task_runner = nullptr);

  static std::unique_ptr<TestServiceDecorator>
  CreateServiceWithInterfaceOverrides(
      std::unique_ptr<Service> decorated_service,
      std::vector<InterfaceOverride> overrides);

  ~TestServiceDecorator() override;

  // Service implementation:
  void OnStart() override;
  void OnBindInterface(const BindSourceInfo& source,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;
  bool OnServiceManagerConnectionLost() override;

 private:
  TestServiceDecorator(std::unique_ptr<Service> delegate_service,
                       std::vector<InterfaceOverride> overrides);

  void SetContext(ServiceContext* context) override;

  std::unique_ptr<Service> decorated_service_;
  BinderRegistryWithArgs<const BindSourceInfo&> overriding_registry_;

  DISALLOW_COPY_AND_ASSIGN(TestServiceDecorator);
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_PUBLIC_CPP_TEST_TEST_SERVICE_DECORATOR_H_
