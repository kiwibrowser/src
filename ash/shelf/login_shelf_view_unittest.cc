// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/login_shelf_view.h"

#include <memory>
#include <vector>

#include "ash/focus_cycler.h"
#include "ash/lock_screen_action/lock_screen_action_background_controller.h"
#include "ash/lock_screen_action/test_lock_screen_action_background_controller.h"
#include "ash/login/mock_login_screen_client.h"
#include "ash/login/ui/login_test_base.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/session/test_session_controller_client.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell.h"
#include "ash/shutdown_controller.h"
#include "ash/system/tray/system_tray.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/ash_test_helper.h"
#include "ash/tray_action/test_tray_action_client.h"
#include "ash/tray_action/tray_action.h"
#include "ash/wm/lock_state_controller.h"
#include "base/command_line.h"
#include "chromeos/chromeos_switches.h"
#include "ui/events/event_utils.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/controls/button/label_button.h"

using session_manager::SessionState;

namespace ash {
namespace {

// Returns true if |view| or any child of it has focus.
bool HasFocusInAnyChildView(views::View* view) {
  if (view->HasFocus())
    return true;
  for (int i = 0; i < view->child_count(); ++i) {
    if (HasFocusInAnyChildView(view->child_at(i)))
      return true;
  }
  return false;
}

void ExpectFocused(views::View* view) {
  EXPECT_TRUE(view->GetWidget()->IsActive());
  EXPECT_TRUE(HasFocusInAnyChildView(view));
}

void ExpectNotFocused(views::View* view) {
  EXPECT_FALSE(view->GetWidget()->IsActive());
  EXPECT_FALSE(HasFocusInAnyChildView(view));
}

class LoginShelfViewTest : public LoginTestBase {
 public:
  LoginShelfViewTest() = default;
  ~LoginShelfViewTest() override = default;

  void SetUp() override {
    action_background_controller_factory_ =
        base::Bind(&LoginShelfViewTest::CreateActionBackgroundController,
                   base::Unretained(this));
    LockScreenActionBackgroundController::SetFactoryCallbackForTesting(
        &action_background_controller_factory_);

    LoginTestBase::SetUp();
    login_shelf_view_ = GetPrimaryShelf()->GetLoginShelfViewForTesting();
    Shell::Get()->tray_action()->SetClient(
        tray_action_client_.CreateInterfacePtrAndBind(),
        mojom::TrayActionState::kNotAvailable);
    // Set initial states.
    NotifySessionStateChanged(SessionState::OOBE);
    NotifyShutdownPolicyChanged(false);
  }

  void TearDown() override {
    LockScreenActionBackgroundController::SetFactoryCallbackForTesting(nullptr);
    action_background_controller_ = nullptr;
    LoginTestBase::TearDown();
  }

 protected:
  void NotifySessionStateChanged(SessionState state) {
    GetSessionControllerClient()->SetSessionState(state);
  }

  void NotifyShutdownPolicyChanged(bool reboot_on_shutdown) {
    Shell::Get()->shutdown_controller()->SetRebootOnShutdownForTesting(
        reboot_on_shutdown);
  }

  void NotifyLockScreenNoteStateChanged(mojom::TrayActionState state) {
    Shell::Get()->tray_action()->UpdateLockScreenNoteState(state);
  }

  // Simulates a click event on the button.
  void Click(LoginShelfView::ButtonId id) {
    const ui::MouseEvent event(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                               ui::EventTimeForNow(), 0, 0);
    login_shelf_view_->ButtonPressed(
        static_cast<views::Button*>(login_shelf_view_->GetViewByID(id)), event);
    RunAllPendingInMessageLoop();
  }

  // Checks if the shelf is only showing the buttons in the list. The IDs in
  // the specified list must be unique.
  bool ShowsShelfButtons(std::vector<LoginShelfView::ButtonId> ids) {
    for (LoginShelfView::ButtonId id : ids) {
      if (!login_shelf_view_->GetViewByID(id)->visible())
        return false;
    }
    size_t visible_button_count = 0;
    for (int i = 0; i < login_shelf_view_->child_count(); ++i) {
      if (login_shelf_view_->child_at(i)->visible())
        visible_button_count++;
    }
    return visible_button_count == ids.size();
  }

  TestTrayActionClient tray_action_client_;

  LoginShelfView* login_shelf_view_;  // Unowned.

  TestLockScreenActionBackgroundController* action_background_controller() {
    return action_background_controller_;
  }

 private:
  std::unique_ptr<LockScreenActionBackgroundController>
  CreateActionBackgroundController() {
    auto result = std::make_unique<TestLockScreenActionBackgroundController>();
    EXPECT_FALSE(action_background_controller_);
    action_background_controller_ = result.get();
    return result;
  }

  LockScreenActionBackgroundController::FactoryCallback
      action_background_controller_factory_;

  // LockScreenActionBackgroundController created by
  // |CreateActionBackgroundController|.
  TestLockScreenActionBackgroundController* action_background_controller_ =
      nullptr;

  DISALLOW_COPY_AND_ASSIGN(LoginShelfViewTest);
};

// Checks the login shelf updates UI after session state changes.
TEST_F(LoginShelfViewTest, ShouldUpdateUiAfterSessionStateChange) {
  EXPECT_TRUE(ShowsShelfButtons({LoginShelfView::kShutdown}));

  NotifySessionStateChanged(SessionState::LOGIN_PRIMARY);
  EXPECT_TRUE(ShowsShelfButtons({LoginShelfView::kShutdown,
                                 LoginShelfView::kBrowseAsGuest,
                                 LoginShelfView::kAddUser}));

  NotifySessionStateChanged(SessionState::LOGGED_IN_NOT_ACTIVE);
  EXPECT_TRUE(ShowsShelfButtons({LoginShelfView::kShutdown}));

  NotifySessionStateChanged(SessionState::ACTIVE);
  EXPECT_TRUE(ShowsShelfButtons({}));

  NotifySessionStateChanged(SessionState::LOGIN_SECONDARY);
  EXPECT_TRUE(
      ShowsShelfButtons({LoginShelfView::kShutdown, LoginShelfView::kCancel}));

  NotifySessionStateChanged(SessionState::ACTIVE);
  EXPECT_TRUE(ShowsShelfButtons({}));

  NotifySessionStateChanged(SessionState::LOCKED);
  EXPECT_TRUE(
      ShowsShelfButtons({LoginShelfView::kShutdown, LoginShelfView::kSignOut}));
}

// Checks the login shelf updates UI after shutdown policy change when the
// screen is locked.
TEST_F(LoginShelfViewTest,
       ShouldUpdateUiAfterShutdownPolicyChangeAtLockScreen) {
  EXPECT_TRUE(ShowsShelfButtons({LoginShelfView::kShutdown}));

  NotifySessionStateChanged(SessionState::LOCKED);
  EXPECT_TRUE(
      ShowsShelfButtons({LoginShelfView::kShutdown, LoginShelfView::kSignOut}));

  NotifyShutdownPolicyChanged(true /*reboot_on_shutdown*/);
  EXPECT_TRUE(
      ShowsShelfButtons({LoginShelfView::kRestart, LoginShelfView::kSignOut}));

  NotifyShutdownPolicyChanged(false /*reboot_on_shutdown*/);
  EXPECT_TRUE(
      ShowsShelfButtons({LoginShelfView::kShutdown, LoginShelfView::kSignOut}));
}

// Checks shutdown policy change during another session state (e.g. ACTIVE)
// will be reflected when the screen becomes locked.
TEST_F(LoginShelfViewTest, ShouldUpdateUiBasedOnShutdownPolicyInActiveSession) {
  // The initial state of |reboot_on_shutdown| is false.
  EXPECT_TRUE(ShowsShelfButtons({LoginShelfView::kShutdown}));

  NotifySessionStateChanged(SessionState::ACTIVE);
  NotifyShutdownPolicyChanged(true /*reboot_on_shutdown*/);

  NotifySessionStateChanged(SessionState::LOCKED);
  EXPECT_TRUE(
      ShowsShelfButtons({LoginShelfView::kRestart, LoginShelfView::kSignOut}));
}

// Checks the login shelf updates UI after lock screen note state changes.
TEST_F(LoginShelfViewTest, ShouldUpdateUiAfterLockScreenNoteState) {
  EXPECT_TRUE(ShowsShelfButtons({LoginShelfView::kShutdown}));

  NotifySessionStateChanged(SessionState::LOCKED);
  EXPECT_TRUE(
      ShowsShelfButtons({LoginShelfView::kShutdown, LoginShelfView::kSignOut}));

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kAvailable);
  EXPECT_TRUE(
      ShowsShelfButtons({LoginShelfView::kShutdown, LoginShelfView::kSignOut}));

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kLaunching);
  // Shelf buttons should not be changed until the lock screen action background
  // show animation completes.
  ASSERT_EQ(LockScreenActionBackgroundState::kShowing,
            action_background_controller()->state());
  EXPECT_TRUE(
      ShowsShelfButtons({LoginShelfView::kShutdown, LoginShelfView::kSignOut}));

  // Complete lock screen action background animation - this should change the
  // visible buttons.
  ASSERT_TRUE(action_background_controller()->FinishShow());
  EXPECT_TRUE(ShowsShelfButtons({LoginShelfView::kCloseNote}));

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kActive);
  EXPECT_TRUE(ShowsShelfButtons({LoginShelfView::kCloseNote}));

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kAvailable);
  // When lock screen action background is animating to hidden state, the close
  // button should immediately be replaced by kShutdown and kSignout buttons.
  ASSERT_EQ(LockScreenActionBackgroundState::kHiding,
            action_background_controller()->state());
  EXPECT_TRUE(
      ShowsShelfButtons({LoginShelfView::kShutdown, LoginShelfView::kSignOut}));

  ASSERT_TRUE(action_background_controller()->FinishHide());
  EXPECT_TRUE(
      ShowsShelfButtons({LoginShelfView::kShutdown, LoginShelfView::kSignOut}));

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kNotAvailable);
  EXPECT_TRUE(
      ShowsShelfButtons({LoginShelfView::kShutdown, LoginShelfView::kSignOut}));
}

TEST_F(LoginShelfViewTest, ClickShutdownButton) {
  Click(LoginShelfView::kShutdown);
  EXPECT_TRUE(Shell::Get()->lock_state_controller()->ShutdownRequested());
}

TEST_F(LoginShelfViewTest, ClickRestartButton) {
  Click(LoginShelfView::kRestart);
  EXPECT_TRUE(Shell::Get()->lock_state_controller()->ShutdownRequested());
}

TEST_F(LoginShelfViewTest, ClickSignOutButton) {
  NotifySessionStateChanged(SessionState::ACTIVE);
  EXPECT_EQ(session_manager::SessionState::ACTIVE,
            Shell::Get()->session_controller()->GetSessionState());
  Click(LoginShelfView::kSignOut);
  EXPECT_EQ(session_manager::SessionState::LOGIN_PRIMARY,
            Shell::Get()->session_controller()->GetSessionState());
}

TEST_F(LoginShelfViewTest, ClickUnlockButton) {
  // The unlock button is visible only when session state is LOCKED and note
  // state is kActive or kLaunching.
  NotifySessionStateChanged(SessionState::LOCKED);

  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kActive);
  ASSERT_TRUE(action_background_controller()->FinishShow());
  EXPECT_TRUE(tray_action_client_.close_note_reasons().empty());
  Click(LoginShelfView::kCloseNote);
  EXPECT_EQ(std::vector<mojom::CloseLockScreenNoteReason>(
                {mojom::CloseLockScreenNoteReason::kUnlockButtonPressed}),
            tray_action_client_.close_note_reasons());

  tray_action_client_.ClearRecordedRequests();
  NotifyLockScreenNoteStateChanged(mojom::TrayActionState::kLaunching);
  EXPECT_TRUE(tray_action_client_.close_note_reasons().empty());
  Click(LoginShelfView::kCloseNote);
  EXPECT_EQ(std::vector<mojom::CloseLockScreenNoteReason>(
                {mojom::CloseLockScreenNoteReason::kUnlockButtonPressed}),
            tray_action_client_.close_note_reasons());
}

TEST_F(LoginShelfViewTest, ClickCancelButton) {
  std::unique_ptr<MockLoginScreenClient> client = BindMockLoginScreenClient();
  EXPECT_CALL(*client, CancelAddUser());
  Click(LoginShelfView::kCancel);
}

TEST_F(LoginShelfViewTest, ClickBrowseAsGuestButton) {
  std::unique_ptr<MockLoginScreenClient> client = BindMockLoginScreenClient();
  EXPECT_CALL(*client, LoginAsGuest());
  Click(LoginShelfView::kBrowseAsGuest);
}

TEST_F(LoginShelfViewTest, TabGoesFromShelfToStatusAreaAndBackToShelf) {
  NotifySessionStateChanged(SessionState::LOCKED);
  EXPECT_TRUE(
      ShowsShelfButtons({LoginShelfView::kShutdown, LoginShelfView::kSignOut}));

  gfx::NativeWindow window = login_shelf_view_->GetWidget()->GetNativeWindow();
  views::View* shelf =
      Shelf::ForWindow(window)->shelf_widget()->GetContentsView();
  views::View* status_area = RootWindowController::ForWindow(window)
                                 ->GetSystemTray()
                                 ->GetWidget()
                                 ->GetContentsView();

  // Give focus to the shelf. The tabbing between lock screen and shelf is
  // verified by |LockScreenSanityTest::TabGoesFromLockToShelfAndBackToLock|.
  Shelf::ForWindow(window)->shelf_widget()->set_default_last_focusable_child(
      false /*reverse*/);
  Shell::Get()->focus_cycler()->FocusWidget(
      Shelf::ForWindow(window)->shelf_widget());
  // The first shelf button has focus.
  ExpectFocused(shelf);
  ExpectNotFocused(status_area);
  EXPECT_TRUE(
      login_shelf_view_->GetViewByID(LoginShelfView::kShutdown)->HasFocus());

  // Focus from the first button to the second button.
  GetEventGenerator().PressKey(ui::KeyboardCode::VKEY_TAB, 0);
  ExpectFocused(shelf);
  ExpectNotFocused(status_area);
  EXPECT_TRUE(
      login_shelf_view_->GetViewByID(LoginShelfView::kSignOut)->HasFocus());

  // Focus from the second button to the status area.
  GetEventGenerator().PressKey(ui::KeyboardCode::VKEY_TAB, 0);
  ExpectNotFocused(shelf);
  ExpectFocused(status_area);

  // A single shift+tab brings focus back to the second shelf button.
  GetEventGenerator().PressKey(ui::KeyboardCode::VKEY_TAB, ui::EF_SHIFT_DOWN);
  ExpectFocused(shelf);
  ExpectNotFocused(status_area);
  EXPECT_TRUE(
      login_shelf_view_->GetViewByID(LoginShelfView::kSignOut)->HasFocus());
}

}  // namespace
}  // namespace ash
