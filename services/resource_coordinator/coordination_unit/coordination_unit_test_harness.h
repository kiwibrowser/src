// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_TEST_HARNESS_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_TEST_HARNESS_H_

#include <stdint.h>
#include <string>

#include "base/test/scoped_task_environment.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_base.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_manager.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_provider_impl.h"
#include "services/service_manager/public/cpp/service_context_ref.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace resource_coordinator {

struct CoordinationUnitID;

template <class CoordinationUnitClass>
class TestCoordinationUnitWrapper {
 public:
  static TestCoordinationUnitWrapper<CoordinationUnitClass> Create() {
    CoordinationUnitID cu_id(CoordinationUnitClass::Type(), std::string());
    return TestCoordinationUnitWrapper<CoordinationUnitClass>(
        CoordinationUnitClass::Create(cu_id, nullptr));
  }

  TestCoordinationUnitWrapper(CoordinationUnitClass* impl) : impl_(impl) {
    DCHECK(impl);
  }
  ~TestCoordinationUnitWrapper() { impl_->Destruct(); }

  CoordinationUnitClass* operator->() const { return impl_; }

  TestCoordinationUnitWrapper(TestCoordinationUnitWrapper&& other)
      : impl_(other.impl_) {}

  CoordinationUnitClass* get() const { return impl_; }

 private:
  CoordinationUnitClass* impl_;

  DISALLOW_COPY_AND_ASSIGN(TestCoordinationUnitWrapper);
};

class CoordinationUnitTestHarness : public testing::Test {
 public:
  CoordinationUnitTestHarness();
  ~CoordinationUnitTestHarness() override;

  template <class CoordinationUnitClass>
  TestCoordinationUnitWrapper<CoordinationUnitClass> CreateCoordinationUnit(
      CoordinationUnitID cu_id) {
    return TestCoordinationUnitWrapper<CoordinationUnitClass>(
        CoordinationUnitClass::Create(cu_id, service_ref_factory_.CreateRef()));
  }

  template <class CoordinationUnitClass>
  TestCoordinationUnitWrapper<CoordinationUnitClass> CreateCoordinationUnit() {
    CoordinationUnitID cu_id(CoordinationUnitClass::Type(), std::string());
    return CreateCoordinationUnit<CoordinationUnitClass>(cu_id);
  }

  // testing::Test:
  void TearDown() override;

 protected:
  base::test::ScopedTaskEnvironment& task_env() {
    return task_env_;
  }
  service_manager::ServiceContextRefFactory* service_context_ref_factory() {
    return &service_ref_factory_;
  }
  CoordinationUnitManager& coordination_unit_manager() {
    return coordination_unit_manager_;
  }
  CoordinationUnitProviderImpl* provider() { return &provider_; }

 private:
  base::test::ScopedTaskEnvironment task_env_;
  service_manager::ServiceContextRefFactory service_ref_factory_;
  CoordinationUnitManager coordination_unit_manager_;
  CoordinationUnitProviderImpl provider_;
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_TEST_HARNESS_H_
