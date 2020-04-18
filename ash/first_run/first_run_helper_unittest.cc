// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/first_run/first_run_helper.h"

#include "ash/first_run/desktop_cleaner.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"

namespace ash {

class FirstRunHelperTest : public AshTestBase,
                           public mojom::FirstRunHelperClient {
 public:
  FirstRunHelperTest() : cancelled_times_(0) {}

  ~FirstRunHelperTest() override = default;

  void SetUp() override {
    AshTestBase::SetUp();
    CheckContainersAreVisible();
    helper_ = Shell::Get()->first_run_helper();
    mojom::FirstRunHelperClientPtr client_ptr;
    binding_.Bind(mojo::MakeRequest(&client_ptr));
    helper_->Start(std::move(client_ptr));
  }

  void TearDown() override {
    helper_->Stop();
    helper_ = nullptr;
    CheckContainersAreVisible();
    AshTestBase::TearDown();
  }

  void CheckContainersAreVisible() const {
    aura::Window* root_window = Shell::Get()->GetPrimaryRootWindow();
    std::vector<int> containers_to_check =
        DesktopCleaner::GetContainersToHideForTest();
    for (size_t i = 0; i < containers_to_check.size(); ++i) {
      aura::Window* container =
          Shell::GetContainer(root_window, containers_to_check[i]);
      EXPECT_TRUE(container->IsVisible());
    }
  }

  void CheckContainersAreHidden() const {
    aura::Window* root_window = Shell::Get()->GetPrimaryRootWindow();
    std::vector<int> containers_to_check =
        DesktopCleaner::GetContainersToHideForTest();
    for (size_t i = 0; i < containers_to_check.size(); ++i) {
      aura::Window* container =
          Shell::GetContainer(root_window, containers_to_check[i]);
      EXPECT_TRUE(!container->IsVisible());
    }
  }

  FirstRunHelper* helper() { return helper_; }

  int cancelled_times() const { return cancelled_times_; }

 private:
  // mojom::FirstRunHelperClient:
  void OnCancelled() override { ++cancelled_times_; }

  FirstRunHelper* helper_;
  mojo::Binding<mojom::FirstRunHelperClient> binding_{this};
  int cancelled_times_;

  DISALLOW_COPY_AND_ASSIGN(FirstRunHelperTest);
};

// This test creates helper, checks that containers are hidden and then
// destructs helper.
TEST_F(FirstRunHelperTest, ContainersAreHidden) {
  CheckContainersAreHidden();
}

// Tests that screen lock cancels the tutorial.
TEST_F(FirstRunHelperTest, ScreenLock) {
  Shell::Get()->session_controller()->LockScreenAndFlushForTest();
  helper()->FlushForTesting();
  EXPECT_EQ(cancelled_times(), 1);
}

// Tests that shutdown cancels the tutorial.
TEST_F(FirstRunHelperTest, ChromeTerminating) {
  Shell::Get()->session_controller()->NotifyChromeTerminating();
  helper()->FlushForTesting();
  EXPECT_EQ(cancelled_times(), 1);
}

}  // namespace ash
