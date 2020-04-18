// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <vector>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "services/ui/common/task_runner_test_base.h"
#include "services/ui/display/screen_manager_ozone_internal.h"
#include "services/ui/display/viewport_metrics.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/display/display.h"
#include "ui/display/display_switches.h"
#include "ui/display/manager/fake_display_delegate.h"
#include "ui/display/manager/fake_display_snapshot.h"
#include "ui/display/screen.h"
#include "ui/display/types/display_constants.h"
#include "ui/display/types/display_mode.h"
#include "ui/display/types/display_snapshot.h"
#include "ui/display/types/fake_display_controller.h"
#include "ui/events/devices/device_data_manager.h"
#include "ui/ozone/public/ozone_platform.h"

namespace display {

using testing::IsEmpty;
using testing::SizeIs;

namespace {

// Holds info about the display state we want to test.
struct DisplayState {
  Display display;
  ViewportMetrics metrics;
};

// Matchers that operate on DisplayState.
MATCHER_P(DisplayIdIs, display_id, "") {
  *result_listener << "has id " << arg.display.id();
  return arg.display.id() == display_id;
}

MATCHER_P(DisplayPixelSizeIs, size_string, "") {
  *result_listener << "has size "
                   << arg.metrics.bounds_in_pixels.size().ToString();
  return arg.metrics.bounds_in_pixels.size().ToString() == size_string;
}

MATCHER_P(DisplayBoundsIs, bounds_string, "") {
  *result_listener << "has size " << arg.display.bounds().ToString();
  return arg.display.bounds().ToString() == bounds_string;
}

// Test delegate to track what functions calls the delegate receives.
class TestScreenManagerDelegate : public ScreenManagerDelegate {
 public:
  TestScreenManagerDelegate() {}
  ~TestScreenManagerDelegate() override {}

  const std::vector<DisplayState>& added() const { return added_; }
  const std::vector<DisplayState>& modified() const { return modified_; }

  // Returns a string containing the function calls that ScreenManagerDelegate
  // has received in the order they occured. Each function call will be in the
  // form "<action>(<id>)" and multiple function calls will be separated by ";".
  // For example, if display 2 was added then display 1 was modified, changes()
  // would return "Added(2);Modified(1)".
  const std::string& changes() const { return changes_; }

  void Reset() {
    added_.clear();
    modified_.clear();
    changes_.clear();
  }

 private:
  void AddChange(const std::string& name, const std::string& value) {
    if (!changes_.empty())
      changes_ += ";";
    changes_ += name + "(" + value + ")";
  }

  void OnDisplayAdded(const display::Display& display,
                      const ViewportMetrics& metrics) override {
    added_.push_back({display, metrics});
    AddChange("Added", base::Int64ToString(display.id()));
  }

  void OnDisplayRemoved(int64_t id) override {
    AddChange("Removed", base::Int64ToString(id));
  }

  void OnDisplayModified(const display::Display& display,
                         const ViewportMetrics& metrics) override {
    modified_.push_back({display, metrics});
    AddChange("Modified", base::Int64ToString(display.id()));
  }

  void OnPrimaryDisplayChanged(int64_t primary_display_id) override {
    AddChange("Primary", base::Int64ToString(primary_display_id));
  }

  std::vector<DisplayState> added_;
  std::vector<DisplayState> modified_;
  std::string changes_;

  DISALLOW_COPY_AND_ASSIGN(TestScreenManagerDelegate);
};

}  // namespace

// Test fixture with helpers to act like DisplayConfigurator and send
// OnDisplayModeChanged() to ScreenManagerOzoneInternal.
class ScreenManagerOzoneInternalTest : public ui::TaskRunnerTestBase {
 public:
  ScreenManagerOzoneInternalTest() {}
  ~ScreenManagerOzoneInternalTest() override {}

  ScreenManagerOzoneInternal* screen_manager() { return screen_manager_.get(); }
  TestScreenManagerDelegate* delegate() { return &delegate_; }

  // Adds a display snapshot with specified ID and default size.
  void AddDisplay(int64_t id) {
    return AddDisplay(FakeDisplaySnapshot::Builder()
                          .SetId(id)
                          .SetNativeMode(gfx::Size(1024, 768))
                          .Build());
  }

  void AddDisplay(std::unique_ptr<DisplaySnapshot> snapshot) {
    EXPECT_TRUE(fake_display_controller_->AddDisplay(std::move(snapshot)));
    RunAllTasks();
  }

  // Removes display snapshot with specified ID.
  void RemoveDisplay(int64_t id) {
    EXPECT_TRUE(fake_display_controller_->RemoveDisplay(id));
    RunAllTasks();
  }

  static void SetUpTestCase() { ui::DeviceDataManager::CreateInstance(); }

  static void TearDownTestCase() { ui::DeviceDataManager::DeleteInstance(); }

 private:
  // testing::Test:
  void SetUp() override {
    TaskRunnerTestBase::SetUp();

    base::CommandLine::ForCurrentProcess()->AppendSwitchNative(
        switches::kScreenConfig, "none");

    screen_manager_ = std::make_unique<ScreenManagerOzoneInternal>();

    // Create NDD for FakeDisplayController.
    std::unique_ptr<NativeDisplayDelegate> ndd =
        std::make_unique<FakeDisplayDelegate>();
    fake_display_controller_ = ndd->GetFakeDisplayController();

    // Add NDD to ScreenManager so one isn't loaded from Ozone.
    screen_manager_->native_display_delegate_ = std::move(ndd);

    AddDisplay(FakeDisplaySnapshot::Builder()
                   .SetId(1)
                   .SetNativeMode(gfx::Size(1024, 768))
                   .SetType(DISPLAY_CONNECTION_TYPE_INTERNAL)
                   .Build());

    screen_manager_->Init(&delegate_);
    RunAllTasks();

    // Double check the expected display exists and clear counters.
    ASSERT_THAT(delegate()->added(), SizeIs(1));
    ASSERT_THAT(delegate_.added()[0], DisplayIdIs(1));
    ASSERT_THAT(delegate_.added()[0], DisplayBoundsIs("0,0 1024x768"));
    ASSERT_THAT(delegate_.added()[0], DisplayPixelSizeIs("1024x768"));
    ASSERT_EQ("Added(1);Primary(1)", delegate()->changes());
    delegate_.Reset();
  }

  void TearDown() override {
    delegate_.Reset();
    screen_manager_.reset();
  }

  FakeDisplayController* fake_display_controller_ = nullptr;
  TestScreenManagerDelegate delegate_;
  std::unique_ptr<ScreenManagerOzoneInternal> screen_manager_;
};

TEST_F(ScreenManagerOzoneInternalTest, AddDisplay) {
  AddDisplay(FakeDisplaySnapshot::Builder()
                 .SetId(2)
                 .SetNativeMode(gfx::Size(1600, 900))
                 .Build());

  // Check that display 2 was added with expected bounds and pixel_size.
  EXPECT_EQ("Added(2)", delegate()->changes());
  EXPECT_THAT(delegate()->added()[0], DisplayPixelSizeIs("1600x900"));
  EXPECT_THAT(delegate()->added()[0], DisplayBoundsIs("1024,0 1600x900"));
}

TEST_F(ScreenManagerOzoneInternalTest, RemoveDisplay) {
  AddDisplay(2);
  delegate()->Reset();

  RemoveDisplay(2);

  // Check that display 2 was removed.
  EXPECT_EQ("Removed(2)", delegate()->changes());
}

TEST_F(ScreenManagerOzoneInternalTest, DISABLED_RemovePrimaryDisplay) {
  AddDisplay(2);
  delegate()->Reset();

  RemoveDisplay(1);

  // Check that display 1 was removed and display 2 becomes the primary display
  // and has it's origin change.
  EXPECT_EQ("Removed(1);Modified(2);Primary(2)", delegate()->changes());
  ASSERT_THAT(delegate()->modified(), SizeIs(1));
  EXPECT_THAT(delegate()->modified()[0], DisplayIdIs(2));
  EXPECT_THAT(delegate()->modified()[0], DisplayBoundsIs("0,0 1024x768"));
}

TEST_F(ScreenManagerOzoneInternalTest, AddRemoveMultipleDisplay) {
  AddDisplay(2);
  AddDisplay(3);
  EXPECT_EQ("Added(2);Added(3)", delegate()->changes());
  EXPECT_THAT(delegate()->added()[0], DisplayBoundsIs("1024,0 1024x768"));
  EXPECT_THAT(delegate()->added()[1], DisplayBoundsIs("2048,0 1024x768"));
  delegate()->Reset();

  // Check that display 2 was removed and display 3 origin changed.
  RemoveDisplay(2);
  EXPECT_EQ("Removed(2);Modified(3)", delegate()->changes());
  EXPECT_THAT(delegate()->modified()[0], DisplayBoundsIs("1024,0 1024x768"));
  delegate()->Reset();

  // Check that display 3 was removed.
  RemoveDisplay(3);
  EXPECT_EQ("Removed(3)", delegate()->changes());
}

TEST_F(ScreenManagerOzoneInternalTest, AddDisplay4k) {
  AddDisplay(FakeDisplaySnapshot::Builder()
                 .SetId(2)
                 .SetNativeMode(gfx::Size(4096, 2160))
                 .SetType(DISPLAY_CONNECTION_TYPE_DVI)
                 .Build());

  EXPECT_EQ("Added(2)", delegate()->changes());
  EXPECT_THAT(delegate()->added()[0], DisplayBoundsIs("1024,0 4096x2160"));
  EXPECT_THAT(delegate()->added()[0], DisplayPixelSizeIs("4096x2160"));
}

TEST_F(ScreenManagerOzoneInternalTest, SwapPrimaryDisplay) {
  AddDisplay(2);
  delegate()->Reset();

  EXPECT_EQ(1, Screen::GetScreen()->GetPrimaryDisplay().id());

  // Swapping displays will modify the bounds of both displays and change the
  // primary.
  screen_manager()->SwapPrimaryDisplay();
  EXPECT_EQ("Modified(1);Modified(2);Primary(2)", delegate()->changes());
  EXPECT_THAT(delegate()->modified()[0], DisplayBoundsIs("-1024,0 1024x768"));
  EXPECT_THAT(delegate()->modified()[1], DisplayBoundsIs("0,0 1024x768"));
  EXPECT_EQ(2, Screen::GetScreen()->GetPrimaryDisplay().id());
  delegate()->Reset();

  // Swapping again should be similar and end up back with display 1 as primary.
  screen_manager()->SwapPrimaryDisplay();
  EXPECT_EQ("Modified(1);Modified(2);Primary(1)", delegate()->changes());
  EXPECT_THAT(delegate()->modified()[0], DisplayBoundsIs("0,0 1024x768"));
  EXPECT_THAT(delegate()->modified()[1], DisplayBoundsIs("1024,0 1024x768"));
  EXPECT_EQ(1, Screen::GetScreen()->GetPrimaryDisplay().id());
}

}  // namespace display
