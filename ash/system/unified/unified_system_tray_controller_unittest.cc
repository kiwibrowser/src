// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/unified/unified_system_tray_controller.h"

#include "ash/public/cpp/config.h"
#include "ash/shell.h"
#include "ash/system/unified/unified_system_tray_model.h"
#include "ash/system/unified/unified_system_tray_view.h"
#include "ash/test/ash_test_base.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/network/network_handler.h"
#include "components/prefs/testing_pref_service.h"
#include "ui/compositor/scoped_animation_duration_scale_mode.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/views/view_observer.h"

namespace ash {

class UnifiedSystemTrayControllerTest : public AshTestBase,
                                        public views::ViewObserver {
 public:
  UnifiedSystemTrayControllerTest() = default;
  ~UnifiedSystemTrayControllerTest() override = default;

  // testing::Test:
  void SetUp() override {
    chromeos::DBusThreadManager::Initialize();
    // Initializing NetworkHandler before ash is more like production.
    chromeos::NetworkHandler::Initialize();
    AshTestBase::SetUp();
    // Mash doesn't do this yet, so don't do it in tests either.
    // http://crbug.com/718072
    if (Shell::GetAshConfig() != Config::MASH) {
      chromeos::NetworkHandler::Get()->InitializePrefServices(&profile_prefs_,
                                                              &local_state_);
    }
    // Networking stubs may have asynchronous initialization.
    base::RunLoop().RunUntilIdle();

    model_ = std::make_unique<UnifiedSystemTrayModel>();
    controller_ = std::make_unique<UnifiedSystemTrayController>(
        model(), nullptr /* system_tray */);
    view_.reset(controller_->CreateView());

    view_->AddObserver(this);
    OnViewPreferredSizeChanged(view());
  }

  void TearDown() override {
    view_->RemoveObserver(this);

    view_.reset();
    controller_.reset();
    model_.reset();

    // This roughly matches production shutdown order.
    if (Shell::GetAshConfig() != Config::MASH) {
      chromeos::NetworkHandler::Get()->ShutdownPrefServices();
    }
    AshTestBase::TearDown();
    chromeos::NetworkHandler::Shutdown();
    chromeos::DBusThreadManager::Shutdown();
  }

  // views::ViewObserver:
  void OnViewPreferredSizeChanged(views::View* observed_view) override {
    view_->SetBoundsRect(gfx::Rect(view_->GetPreferredSize()));
    view_->Layout();
  }

 protected:
  void WaitForAnimation() {
    while (controller()->animation_->is_animating())
      base::RunLoop().RunUntilIdle();
  }

  UnifiedSystemTrayModel* model() { return model_.get(); }
  UnifiedSystemTrayController* controller() { return controller_.get(); }
  UnifiedSystemTrayView* view() { return view_.get(); }

 private:
  std::unique_ptr<UnifiedSystemTrayModel> model_;
  std::unique_ptr<UnifiedSystemTrayController> controller_;
  std::unique_ptr<UnifiedSystemTrayView> view_;

  TestingPrefServiceSimple profile_prefs_;
  TestingPrefServiceSimple local_state_;

  DISALLOW_COPY_AND_ASSIGN(UnifiedSystemTrayControllerTest);
};

TEST_F(UnifiedSystemTrayControllerTest, ToggleExpanded) {
  EXPECT_TRUE(model()->expanded_on_open());
  const int expanded_height = view()->GetPreferredSize().height();

  controller()->ToggleExpanded();
  WaitForAnimation();

  const int collapsed_height = view()->GetPreferredSize().height();
  EXPECT_LT(collapsed_height, expanded_height);
  EXPECT_FALSE(model()->expanded_on_open());
}

}  // namespace ash
