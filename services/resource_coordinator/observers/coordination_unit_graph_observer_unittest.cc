// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/observers/coordination_unit_graph_observer.h"

#include "base/process/process_handle.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_manager.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_test_harness.h"
#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/process_coordination_unit_impl.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_id.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_types.h"
#include "services/resource_coordinator/public/mojom/coordination_unit.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace resource_coordinator {

namespace {

class CoordinationUnitGraphObserverTest : public CoordinationUnitTestHarness {};

class TestCoordinationUnitGraphObserver : public CoordinationUnitGraphObserver {
 public:
  TestCoordinationUnitGraphObserver()
      : coordination_unit_created_count_(0u),
        coordination_unit_destroyed_count_(0u),
        property_changed_count_(0u) {}

  size_t coordination_unit_created_count() {
    return coordination_unit_created_count_;
  }
  size_t coordination_unit_destroyed_count() {
    return coordination_unit_destroyed_count_;
  }
  size_t property_changed_count() { return property_changed_count_; }

  // Overridden from CoordinationUnitGraphObserver.
  bool ShouldObserve(const CoordinationUnitBase* coordination_unit) override {
    return coordination_unit->id().type == CoordinationUnitType::kFrame;
  }
  void OnCoordinationUnitCreated(
      const CoordinationUnitBase* coordination_unit) override {
    ++coordination_unit_created_count_;
  }
  void OnBeforeCoordinationUnitDestroyed(
      const CoordinationUnitBase* coordination_unit) override {
    ++coordination_unit_destroyed_count_;
  }
  void OnFramePropertyChanged(
      const FrameCoordinationUnitImpl* frame_coordination_unit,
      const mojom::PropertyType property_type,
      int64_t value) override {
    ++property_changed_count_;
  }

 private:
  size_t coordination_unit_created_count_;
  size_t coordination_unit_destroyed_count_;
  size_t property_changed_count_;
};

}  // namespace

TEST_F(CoordinationUnitGraphObserverTest, CallbacksInvoked) {
  EXPECT_TRUE(coordination_unit_manager().observers_for_testing().empty());
  coordination_unit_manager().RegisterObserver(
      std::make_unique<TestCoordinationUnitGraphObserver>());
  EXPECT_EQ(1u, coordination_unit_manager().observers_for_testing().size());

  TestCoordinationUnitGraphObserver* observer =
      static_cast<TestCoordinationUnitGraphObserver*>(
          coordination_unit_manager().observers_for_testing()[0].get());

  auto process_cu = CreateCoordinationUnit<ProcessCoordinationUnitImpl>();
  auto root_frame_cu = CreateCoordinationUnit<FrameCoordinationUnitImpl>();
  auto frame_cu = CreateCoordinationUnit<FrameCoordinationUnitImpl>();

  coordination_unit_manager().OnCoordinationUnitCreated(process_cu.get());
  coordination_unit_manager().OnCoordinationUnitCreated(root_frame_cu.get());
  coordination_unit_manager().OnCoordinationUnitCreated(frame_cu.get());
  EXPECT_EQ(2u, observer->coordination_unit_created_count());

  // The registered observer will only observe the events that happen to
  // |root_frame_coordination_unit| and |frame_coordination_unit| because
  // they are CoordinationUnitType::kFrame, so OnPropertyChanged
  // will only be called for |root_frame_coordination_unit|.
  root_frame_cu->SetPropertyForTesting(42);
  process_cu->SetPropertyForTesting(42);
  EXPECT_EQ(1u, observer->property_changed_count());

  coordination_unit_manager().OnBeforeCoordinationUnitDestroyed(
      process_cu.get());
  coordination_unit_manager().OnBeforeCoordinationUnitDestroyed(
      root_frame_cu.get());
  coordination_unit_manager().OnBeforeCoordinationUnitDestroyed(frame_cu.get());
  EXPECT_EQ(2u, observer->coordination_unit_destroyed_count());
}

}  // namespace resource_coordinator
