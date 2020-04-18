// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/coordination_unit_base.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_provider_impl.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_test_harness.h"
#include "services/resource_coordinator/coordination_unit/mock_coordination_unit_graphs.h"
#include "services/resource_coordinator/coordination_unit/page_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/process_coordination_unit_impl.h"
#include "services/resource_coordinator/public/mojom/coordination_unit.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace resource_coordinator {

namespace {

class CoordinationUnitBaseTest : public CoordinationUnitTestHarness {};

using CoordinationUnitBaseDeathTest = CoordinationUnitBaseTest;

}  // namespace

TEST_F(CoordinationUnitBaseTest, GetSetProperty) {
  auto coordination_unit = CreateCoordinationUnit<PageCoordinationUnitImpl>();

  // An empty value should be returned if property is not found
  int64_t test_value;
  EXPECT_FALSE(
      coordination_unit->GetProperty(mojom::PropertyType::kTest, &test_value));

  // Perform a valid storage property set
  coordination_unit->SetPropertyForTesting(41);
  EXPECT_EQ(1u, coordination_unit->properties_for_testing().size());
  EXPECT_TRUE(
      coordination_unit->GetProperty(mojom::PropertyType::kTest, &test_value));
  EXPECT_EQ(41, test_value);
}

TEST_F(CoordinationUnitBaseTest,
       GetAssociatedCoordinationUnitsForSinglePageInSingleProcess) {
  MockSinglePageInSingleProcessCoordinationUnitGraph cu_graph;

  auto pages_associated_with_process =
      cu_graph.process->GetAssociatedPageCoordinationUnits();
  EXPECT_EQ(1u, pages_associated_with_process.size());
  EXPECT_EQ(1u, pages_associated_with_process.count(cu_graph.page.get()));

  auto processes_associated_with_page =
      cu_graph.page->GetAssociatedProcessCoordinationUnits();
  EXPECT_EQ(1u, processes_associated_with_page.size());
  EXPECT_EQ(1u, processes_associated_with_page.count(cu_graph.process.get()));
}

TEST_F(CoordinationUnitBaseTest,
       GetAssociatedCoordinationUnitsForMultiplePagesInSingleProcess) {
  MockMultiplePagesInSingleProcessCoordinationUnitGraph cu_graph;

  auto pages_associated_with_process =
      cu_graph.process->GetAssociatedPageCoordinationUnits();
  EXPECT_EQ(2u, pages_associated_with_process.size());
  EXPECT_EQ(1u, pages_associated_with_process.count(cu_graph.page.get()));
  EXPECT_EQ(1u, pages_associated_with_process.count(cu_graph.other_page.get()));

  auto processes_associated_with_page =
      cu_graph.page->GetAssociatedProcessCoordinationUnits();
  EXPECT_EQ(1u, processes_associated_with_page.size());
  EXPECT_EQ(1u, processes_associated_with_page.count(cu_graph.process.get()));

  auto processes_associated_with_other_page =
      cu_graph.other_page->GetAssociatedProcessCoordinationUnits();
  EXPECT_EQ(1u, processes_associated_with_other_page.size());
  EXPECT_EQ(1u, processes_associated_with_page.count(cu_graph.process.get()));
}

TEST_F(CoordinationUnitBaseTest,
       GetAssociatedCoordinationUnitsForSinglePageWithMultipleProcesses) {
  MockSinglePageWithMultipleProcessesCoordinationUnitGraph cu_graph;

  auto pages_associated_with_process =
      cu_graph.process->GetAssociatedPageCoordinationUnits();
  EXPECT_EQ(1u, pages_associated_with_process.size());
  EXPECT_EQ(1u, pages_associated_with_process.count(cu_graph.page.get()));

  auto pages_associated_with_other_process =
      cu_graph.other_process->GetAssociatedPageCoordinationUnits();
  EXPECT_EQ(1u, pages_associated_with_other_process.size());
  EXPECT_EQ(1u, pages_associated_with_other_process.count(cu_graph.page.get()));

  auto processes_associated_with_page =
      cu_graph.page->GetAssociatedProcessCoordinationUnits();
  EXPECT_EQ(2u, processes_associated_with_page.size());
  EXPECT_EQ(1u, processes_associated_with_page.count(cu_graph.process.get()));
  EXPECT_EQ(1u,
            processes_associated_with_page.count(cu_graph.other_process.get()));
}

TEST_F(CoordinationUnitBaseTest,
       GetAssociatedCoordinationUnitsForMultiplePagesWithMultipleProcesses) {
  MockMultiplePagesWithMultipleProcessesCoordinationUnitGraph cu_graph;

  auto pages_associated_with_process =
      cu_graph.process->GetAssociatedPageCoordinationUnits();
  EXPECT_EQ(2u, pages_associated_with_process.size());
  EXPECT_EQ(1u, pages_associated_with_process.count(cu_graph.page.get()));
  EXPECT_EQ(1u, pages_associated_with_process.count(cu_graph.other_page.get()));

  auto pages_associated_with_other_process =
      cu_graph.other_process->GetAssociatedPageCoordinationUnits();
  EXPECT_EQ(1u, pages_associated_with_other_process.size());
  EXPECT_EQ(
      1u, pages_associated_with_other_process.count(cu_graph.other_page.get()));

  auto processes_associated_with_page =
      cu_graph.page->GetAssociatedProcessCoordinationUnits();
  EXPECT_EQ(1u, processes_associated_with_page.size());
  EXPECT_EQ(1u, processes_associated_with_page.count(cu_graph.process.get()));

  auto processes_associated_with_other_page =
      cu_graph.other_page->GetAssociatedProcessCoordinationUnits();
  EXPECT_EQ(2u, processes_associated_with_other_page.size());
  EXPECT_EQ(1u,
            processes_associated_with_other_page.count(cu_graph.process.get()));
  EXPECT_EQ(1u, processes_associated_with_other_page.count(
                    cu_graph.other_process.get()));
}

}  // namespace resource_coordinator
