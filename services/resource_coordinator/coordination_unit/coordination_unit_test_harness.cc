// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/coordination_unit_test_harness.h"

#include "base/bind.h"
#include "base/run_loop.h"

namespace resource_coordinator {

namespace {

void OnLastServiceRefDestroyed() {
  // No-op. This is required by service_manager::ServiceContextRefFactory
  // construction but not needed for the tests.
}

}  // namespace

CoordinationUnitTestHarness::CoordinationUnitTestHarness()
    : task_env_(base::test::ScopedTaskEnvironment::MainThreadType::MOCK_TIME,
                base::test::ScopedTaskEnvironment::ExecutionMode::QUEUED),
      service_ref_factory_(base::Bind(&OnLastServiceRefDestroyed)),
      provider_(&service_ref_factory_, &coordination_unit_manager_) {}

CoordinationUnitTestHarness::~CoordinationUnitTestHarness() = default;

void CoordinationUnitTestHarness::TearDown() {
  base::RunLoop().RunUntilIdle();
}

}  // namespace resource_coordinator
