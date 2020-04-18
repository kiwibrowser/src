// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/public/cpp/test/test_service_decorator.h"

#include "base/memory/ptr_util.h"

namespace service_manager {

// static
std::unique_ptr<TestServiceDecorator>
TestServiceDecorator::CreateServiceWithUniqueOverride(
    std::unique_ptr<Service> service,
    const std::string& interface_name,
    const BinderRegistryWithArgs<const BindSourceInfo&>::Binder& binder,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner) {
  return base::WrapUnique(new TestServiceDecorator(
      std::move(service),
      {std::make_tuple(interface_name, binder, task_runner)}));
}

// static
std::unique_ptr<TestServiceDecorator>
TestServiceDecorator::CreateServiceWithInterfaceOverrides(
    std::unique_ptr<Service> decorated_service,
    std::vector<InterfaceOverride> overrides) {
  return base::WrapUnique(new TestServiceDecorator(std::move(decorated_service),
                                                   std::move(overrides)));
}

TestServiceDecorator::TestServiceDecorator(
    std::unique_ptr<Service> decorated_service,
    std::vector<InterfaceOverride> overrides)
    : decorated_service_(std::move(decorated_service)) {
  // It would be nice to accept the registry as a constructor argument, but it's
  // not moveable at this time.
  for (const auto& override : overrides) {
    overriding_registry_.AddInterface(
        std::get<0>(override), std::get<1>(override), std::get<2>(override));
  }
}

TestServiceDecorator::~TestServiceDecorator() {}

void TestServiceDecorator::OnStart() {
  decorated_service_->OnStart();
}

bool TestServiceDecorator::OnServiceManagerConnectionLost() {
  return decorated_service_->OnServiceManagerConnectionLost();
}

void TestServiceDecorator::OnBindInterface(
    const BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  if (overriding_registry_.TryBindInterface(interface_name, &interface_pipe,
                                            source_info)) {
    return;
  }
  decorated_service_->OnBindInterface(source_info, interface_name,
                                      std::move(interface_pipe));
}

void TestServiceDecorator::SetContext(ServiceContext* context) {
  decorated_service_->SetContext(context);
}

}  // namespace service_manager
