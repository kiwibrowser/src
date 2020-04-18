// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/session/logout_confirmation_controller.h"

#include <memory>
#include <utility>

#include "ash/login_status.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/shell_observer.h"
#include "ash/system/session/logout_confirmation_dialog.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/time/default_tick_clock.h"
#include "base/time/tick_clock.h"
#include "ui/aura/window.h"
#include "ui/aura/window_observer.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace {
const int kLogoutConfirmationDelayInSeconds = 20;

// Shell window containers monitored for when the last window closes.
const int kLastWindowClosedContainerIds[] = {
    kShellWindowId_DefaultContainer, kShellWindowId_AlwaysOnTopContainer};

void SignOut() {
  Shell::Get()->session_controller()->RequestSignOut();
}

}  // namespace

// Monitors window containers to detect when the last browser or app window is
// closing and shows a logout confirmation dialog.
class LogoutConfirmationController::LastWindowClosedObserver
    : public ShellObserver,
      public aura::WindowObserver {
 public:
  LastWindowClosedObserver() {
    DCHECK_EQ(Shell::Get()->session_controller()->login_status(),
              LoginStatus::PUBLIC);
    Shell::Get()->AddShellObserver(this);

    // Observe all displays.
    for (aura::Window* root : Shell::GetAllRootWindows())
      ObserveForLastWindowClosed(root);
  }

  ~LastWindowClosedObserver() override {
    // Stop observing all displays.
    for (aura::Window* root : Shell::GetAllRootWindows()) {
      for (int id : kLastWindowClosedContainerIds)
        root->GetChildById(id)->RemoveObserver(this);
    }
    Shell::Get()->RemoveShellObserver(this);
  }

 private:
  // Observes containers in the |root| window for the last browser and/or app
  // window being closed. The observers are removed automatically.
  void ObserveForLastWindowClosed(aura::Window* root) {
    for (int id : kLastWindowClosedContainerIds)
      root->GetChildById(id)->AddObserver(this);
  }

  // Shows the logout confirmation dialog if the last window is closing in the
  // containers we are tracking. Called before closing instead of after closed
  // because aura::WindowObserver only provides notifications to parent windows
  // before a child is removed, not after. Note that removing window deep inside
  // tracked container also causes OnWindowHierarchyChanging calls so we check
  // here that removing window is the last window with parent of a tracked
  // container.
  void ShowDialogIfLastWindowClosing(const aura::Window* closing_window) {
    // Enumerate all root windows.
    for (aura::Window* root : Shell::GetAllRootWindows()) {
      // For each root window enumerate tracked containers.
      for (int id : kLastWindowClosedContainerIds) {
        // In each container try to find child window that is not equal to
        // |closing_window| which would indicate that we have other top-level
        // window and logout time does not apply.
        for (const aura::Window* window : root->GetChildById(id)->children()) {
          if (window != closing_window)
            return;
        }
      }
    }

    // No more windows except currently removing. Show logout time.
    Shell::Get()->logout_confirmation_controller()->ConfirmLogout(
        base::TimeTicks::Now() +
        base::TimeDelta::FromSeconds(kLogoutConfirmationDelayInSeconds));
  }

  // ShellObserver:
  void OnRootWindowAdded(aura::Window* root) override {
    ObserveForLastWindowClosed(root);
  }

  // aura::WindowObserver:
  void OnWindowHierarchyChanging(const HierarchyChangeParams& params) override {
    if (!params.new_parent && params.old_parent) {
      // A window is being removed (and not moved to another container).
      ShowDialogIfLastWindowClosing(params.target);
    }
  }

  void OnWindowDestroying(aura::Window* window) override {
    // Stop observing the container window when it closes.
    window->RemoveObserver(this);
  }

  DISALLOW_COPY_AND_ASSIGN(LastWindowClosedObserver);
};

LogoutConfirmationController::LogoutConfirmationController()
    : clock_(base::DefaultTickClock::GetInstance()),
      logout_closure_(base::Bind(&SignOut)),
      logout_timer_(false, false) {
  if (Shell::HasInstance())  // Null in testing::Test.
    Shell::Get()->session_controller()->AddObserver(this);
}

LogoutConfirmationController::~LogoutConfirmationController() {
  if (dialog_)
    dialog_->ControllerGone();

  if (Shell::HasInstance())  // Null in testing::Test.
    Shell::Get()->session_controller()->RemoveObserver(this);
}

void LogoutConfirmationController::ConfirmLogout(base::TimeTicks logout_time) {
  if (!logout_time_.is_null() && logout_time >= logout_time_) {
    // If a confirmation dialog is already being shown and its countdown expires
    // no later than the |logout_time| requested now, keep the current dialog
    // open.
    return;
  }
  logout_time_ = logout_time;

  if (!dialog_) {
    // Show confirmation dialog unless this is a unit test without a Shell.
    if (Shell::HasInstance())
      dialog_ = new LogoutConfirmationDialog(this, logout_time_);
  } else {
    dialog_->Update(logout_time_);
  }

  logout_timer_.Start(FROM_HERE, logout_time_ - clock_->NowTicks(),
                      logout_closure_);
}

void LogoutConfirmationController::OnLoginStatusChanged(
    LoginStatus login_status) {
  if (login_status == LoginStatus::PUBLIC)
    last_window_closed_observer_ = std::make_unique<LastWindowClosedObserver>();
  else
    last_window_closed_observer_.reset();
}

void LogoutConfirmationController::OnLockStateChanged(bool locked) {
  if (!locked || logout_time_.is_null())
    return;

  // If the screen is locked while a confirmation dialog is being shown, close
  // the dialog.
  logout_time_ = base::TimeTicks();
  if (dialog_)
    dialog_->GetWidget()->Close();
  logout_timer_.Stop();
}

void LogoutConfirmationController::OnLogoutConfirmed() {
  logout_timer_.Stop();
  logout_closure_.Run();
}

void LogoutConfirmationController::OnDialogClosed() {
  logout_time_ = base::TimeTicks();
  dialog_ = NULL;
  logout_timer_.Stop();
}

void LogoutConfirmationController::SetClockForTesting(
    const base::TickClock* clock) {
  clock_ = clock;
}

void LogoutConfirmationController::SetLogoutClosureForTesting(
    const base::Closure& logout_closure) {
  logout_closure_ = logout_closure;
}

}  // namespace ash
