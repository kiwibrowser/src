// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/projecting_observer.h"

#include <memory>
#include <vector>

#include "chromeos/dbus/fake_power_manager_client.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/display/manager/fake_display_snapshot.h"

namespace ash {

namespace {

std::unique_ptr<display::DisplaySnapshot> CreateInternalSnapshot() {
  return display::FakeDisplaySnapshot::Builder()
      .SetId(123)
      .SetNativeMode(gfx::Size(1024, 768))
      .SetType(display::DISPLAY_CONNECTION_TYPE_INTERNAL)
      .Build();
}

std::unique_ptr<display::DisplaySnapshot> CreateVGASnapshot() {
  return display::FakeDisplaySnapshot::Builder()
      .SetId(456)
      .SetNativeMode(gfx::Size(1024, 768))
      .SetType(display::DISPLAY_CONNECTION_TYPE_VGA)
      .Build();
}

display::DisplayConfigurator::DisplayStateList GetPointers(
    const std::vector<std::unique_ptr<display::DisplaySnapshot>>& displays) {
  display::DisplayConfigurator::DisplayStateList result;
  for (const auto& display : displays)
    result.push_back(display.get());
  return result;
}

}  // namespace

class ProjectingObserverTest : public testing::Test {
 public:
  ProjectingObserverTest() {
    observer_ = std::make_unique<ProjectingObserver>(nullptr);
    observer_->set_power_manager_client_for_test(&fake_power_client_);
  }

  ~ProjectingObserverTest() override = default;

 protected:
  chromeos::FakePowerManagerClient fake_power_client_;
  std::unique_ptr<ProjectingObserver> observer_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ProjectingObserverTest);
};

TEST_F(ProjectingObserverTest, CheckNoDisplay) {
  std::vector<std::unique_ptr<display::DisplaySnapshot>> displays;
  observer_->OnDisplayModeChanged(GetPointers(displays));

  EXPECT_EQ(1, fake_power_client_.num_set_is_projecting_calls());
  EXPECT_FALSE(fake_power_client_.is_projecting());
}

TEST_F(ProjectingObserverTest, CheckWithoutInternalDisplay) {
  std::vector<std::unique_ptr<display::DisplaySnapshot>> displays;
  displays.push_back(CreateVGASnapshot());
  observer_->OnDisplayModeChanged(GetPointers(displays));

  EXPECT_EQ(1, fake_power_client_.num_set_is_projecting_calls());
  EXPECT_FALSE(fake_power_client_.is_projecting());
}

TEST_F(ProjectingObserverTest, CheckWithInternalDisplay) {
  std::vector<std::unique_ptr<display::DisplaySnapshot>> displays;
  displays.push_back(CreateInternalSnapshot());
  observer_->OnDisplayModeChanged(GetPointers(displays));

  EXPECT_EQ(1, fake_power_client_.num_set_is_projecting_calls());
  EXPECT_FALSE(fake_power_client_.is_projecting());
}

TEST_F(ProjectingObserverTest, CheckWithTwoVGADisplays) {
  std::vector<std::unique_ptr<display::DisplaySnapshot>> displays;
  displays.push_back(CreateVGASnapshot());
  displays.push_back(CreateVGASnapshot());
  observer_->OnDisplayModeChanged(GetPointers(displays));

  EXPECT_EQ(1, fake_power_client_.num_set_is_projecting_calls());
  // We need at least 1 internal display to set projecting to on.
  EXPECT_FALSE(fake_power_client_.is_projecting());
}

TEST_F(ProjectingObserverTest, CheckWithInternalAndVGADisplays) {
  std::vector<std::unique_ptr<display::DisplaySnapshot>> displays;
  displays.push_back(CreateInternalSnapshot());
  displays.push_back(CreateVGASnapshot());
  observer_->OnDisplayModeChanged(GetPointers(displays));

  EXPECT_EQ(1, fake_power_client_.num_set_is_projecting_calls());
  EXPECT_TRUE(fake_power_client_.is_projecting());
}

TEST_F(ProjectingObserverTest, CheckWithVGADisplayAndOneCastingSession) {
  std::vector<std::unique_ptr<display::DisplaySnapshot>> displays;
  displays.push_back(CreateVGASnapshot());
  observer_->OnDisplayModeChanged(GetPointers(displays));

  observer_->OnCastingSessionStartedOrStopped(true);

  EXPECT_EQ(2, fake_power_client_.num_set_is_projecting_calls());
  // Need at least one internal display to set projecting state to |true|.
  EXPECT_FALSE(fake_power_client_.is_projecting());
}

TEST_F(ProjectingObserverTest, CheckWithInternalDisplayAndOneCastingSession) {
  std::vector<std::unique_ptr<display::DisplaySnapshot>> displays;
  displays.push_back(CreateInternalSnapshot());
  observer_->OnDisplayModeChanged(GetPointers(displays));

  observer_->OnCastingSessionStartedOrStopped(true);

  EXPECT_EQ(2, fake_power_client_.num_set_is_projecting_calls());
  EXPECT_TRUE(fake_power_client_.is_projecting());
}

TEST_F(ProjectingObserverTest, CheckProjectingAfterClosingACastingSession) {
  std::vector<std::unique_ptr<display::DisplaySnapshot>> displays;
  displays.push_back(CreateInternalSnapshot());
  observer_->OnDisplayModeChanged(GetPointers(displays));

  observer_->OnCastingSessionStartedOrStopped(true);
  observer_->OnCastingSessionStartedOrStopped(true);

  EXPECT_EQ(3, fake_power_client_.num_set_is_projecting_calls());
  EXPECT_TRUE(fake_power_client_.is_projecting());

  observer_->OnCastingSessionStartedOrStopped(false);

  EXPECT_EQ(4, fake_power_client_.num_set_is_projecting_calls());
  EXPECT_TRUE(fake_power_client_.is_projecting());
}

TEST_F(ProjectingObserverTest,
       CheckStopProjectingAfterClosingAllCastingSessions) {
  std::vector<std::unique_ptr<display::DisplaySnapshot>> displays;
  displays.push_back(CreateInternalSnapshot());
  observer_->OnDisplayModeChanged(GetPointers(displays));

  observer_->OnCastingSessionStartedOrStopped(true);
  observer_->OnCastingSessionStartedOrStopped(false);

  EXPECT_EQ(3, fake_power_client_.num_set_is_projecting_calls());
  EXPECT_FALSE(fake_power_client_.is_projecting());
}

TEST_F(ProjectingObserverTest,
       CheckStopProjectingAfterDisconnectingSecondOutput) {
  std::vector<std::unique_ptr<display::DisplaySnapshot>> displays;
  displays.push_back(CreateInternalSnapshot());
  displays.push_back(CreateVGASnapshot());
  observer_->OnDisplayModeChanged(GetPointers(displays));

  // Remove VGA output.
  displays.erase(displays.begin() + 1);
  observer_->OnDisplayModeChanged(GetPointers(displays));

  EXPECT_EQ(2, fake_power_client_.num_set_is_projecting_calls());
  EXPECT_FALSE(fake_power_client_.is_projecting());
}

}  // namespace ash
