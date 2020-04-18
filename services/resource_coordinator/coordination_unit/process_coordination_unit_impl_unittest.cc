// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/process_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_test_harness.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace resource_coordinator {

namespace {

class ProcessCoordinationUnitImplTest : public CoordinationUnitTestHarness {};

}  // namespace

TEST_F(ProcessCoordinationUnitImplTest, MeasureCPUUsage) {
  auto process_cu = CreateCoordinationUnit<ProcessCoordinationUnitImpl>();
  process_cu->SetCPUUsage(1);
  int64_t cpu_usage;
  EXPECT_TRUE(
      process_cu->GetProperty(mojom::PropertyType::kCPUUsage, &cpu_usage));
  EXPECT_EQ(1, cpu_usage / 1000.0);
}

}  // namespace resource_coordinator
