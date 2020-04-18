// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/lock_screen.h"

#include <memory>
#include <utility>

#include "ash/login/ui/lock_contents_view.h"
#include "ash/login/ui/lock_debug_view.h"
#include "ash/login/ui/lock_window.h"
#include "ash/login/ui/login_data_dispatcher.h"
#include "ash/login/ui/login_detachable_base_model.h"
#include "ash/public/cpp/login_constants.h"
#include "ash/public/interfaces/session_controller.mojom.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/tray_action/tray_action.h"
#include "ash/wallpaper/wallpaper_widget_controller.h"
#include "base/command_line.h"
#include "chromeos/chromeos_switches.h"
#include "ui/compositor/layer.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace ash {
namespace {

ui::Layer* GetWallpaperLayerForWindow(aura::Window* window) {
  return RootWindowController::ForWindow(window)
      ->wallpaper_widget_controller()
      ->GetWidget()
      ->GetLayer();
}

// Global lock screen instance. There can only ever be on lock screen at a
// time.
LockScreen* instance_ = nullptr;

}  // namespace

LockScreen::TestApi::TestApi(LockScreen* lock_screen)
    : lock_screen_(lock_screen) {}

LockScreen::TestApi::~TestApi() = default;

LockContentsView* LockScreen::TestApi::contents_view() const {
  return lock_screen_->contents_view_;
}

LockScreen::LockScreen(ScreenType type)
    : type_(type), tray_action_observer_(this), session_observer_(this) {
  tray_action_observer_.Add(ash::Shell::Get()->tray_action());
}

LockScreen::~LockScreen() = default;

// static
LockScreen* LockScreen::Get() {
  CHECK(instance_);
  return instance_;
}

// static
void LockScreen::Show(ScreenType type) {
  CHECK(!instance_);
  instance_ = new LockScreen(type);

  instance_->window_ = new LockWindow(Shell::GetAshConfig());
  instance_->window_->SetBounds(
      display::Screen::GetScreen()->GetPrimaryDisplay().bounds());

  auto data_dispatcher = std::make_unique<LoginDataDispatcher>();
  auto initial_note_action_state =
      Shell::Get()->tray_action()->GetLockScreenNoteState();
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          chromeos::switches::kShowLoginDevOverlay)) {
    auto* debug_view =
        new LockDebugView(initial_note_action_state, data_dispatcher.get());
    instance_->contents_view_ = debug_view->lock();
    instance_->window_->SetContentsView(debug_view);
  } else {
    auto detachable_base_model = LoginDetachableBaseModel::Create(
        Shell::Get()->detachable_base_handler(), data_dispatcher.get());
    instance_->contents_view_ =
        new LockContentsView(initial_note_action_state, data_dispatcher.get(),
                             std::move(detachable_base_model));
    instance_->window_->SetContentsView(instance_->contents_view_);
  }

  instance_->window_->set_data_dispatcher(std::move(data_dispatcher));
  instance_->window_->Show();
}

// static
bool LockScreen::IsShown() {
  return !!instance_;
}

void LockScreen::Destroy() {
  LoginScreenController::AuthenticationStage authentication_stage =
      ash::Shell::Get()->login_screen_controller()->authentication_stage();
  base::debug::Alias(&authentication_stage);
  if (ash::Shell::Get()->login_screen_controller()->authentication_stage() !=
      authentication_stage) {
    LOG(FATAL) << "Unexpected authentication stage "
               << static_cast<int>(authentication_stage);
  }
  CHECK_EQ(instance_, this);

  // Restore the initial wallpaper bluriness if they were changed.
  for (auto it = initial_blur_.begin(); it != initial_blur_.end(); ++it)
    it->first->SetLayerBlur(it->second);
  window_->Close();
  delete instance_;
  instance_ = nullptr;
}

void LockScreen::ToggleBlurForDebug() {
  // Save the initial wallpaper bluriness upon the first time this is called.
  if (instance_->initial_blur_.empty()) {
    for (aura::Window* window : Shell::GetAllRootWindows()) {
      ui::Layer* layer = GetWallpaperLayerForWindow(window);
      instance_->initial_blur_[layer] = layer->layer_blur();
    }
  }
  for (aura::Window* window : Shell::GetAllRootWindows()) {
    ui::Layer* layer = GetWallpaperLayerForWindow(window);
    if (layer->layer_blur() > 0.0f) {
      layer->SetLayerBlur(0.0f);
    } else {
      layer->SetLayerBlur(login_constants::kBlurSigma);
    }
  }
}

LoginDataDispatcher* LockScreen::data_dispatcher() {
  return window_->data_dispatcher();
}

void LockScreen::OnLockScreenNoteStateChanged(mojom::TrayActionState state) {
  if (data_dispatcher())
    data_dispatcher()->SetLockScreenNoteState(state);
}

void LockScreen::OnSessionStateChanged(session_manager::SessionState state) {
  if (type_ == ScreenType::kLogin &&
      state == session_manager::SessionState::ACTIVE) {
    Destroy();
  }
}

void LockScreen::OnLockStateChanged(bool locked) {
  if (type_ != ScreenType::kLock)
    return;

  if (!locked)
    Destroy();
  Shell::Get()->metrics()->login_metrics_recorder()->Reset();
}

}  // namespace ash
