// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/root_window_controller.h"

#include <memory>

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/session/session_controller.h"
#include "ash/session/test_session_controller_client.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/system_modal_container_layout_manager.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "ash/wm/window_properties.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "base/command_line.h"
#include "ui/aura/client/focus_change_observer.h"
#include "ui/aura/client/focus_client.h"
#include "ui/aura/client/window_parenting_client.h"
#include "ui/aura/env.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/test/test_windows.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tracker.h"
#include "ui/base/ime/dummy_text_input_client.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/test/display_manager_test_api.h"
#include "ui/events/test/event_generator.h"
#include "ui/events/test/test_event_handler.h"
#include "ui/keyboard/keyboard_controller.h"
#include "ui/keyboard/keyboard_switches.h"
#include "ui/keyboard/keyboard_test_util.h"
#include "ui/keyboard/keyboard_ui.h"
#include "ui/keyboard/keyboard_util.h"
#include "ui/views/controls/menu/menu_controller.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

using aura::Window;
using views::Widget;

namespace ash {
namespace {

class TestDelegate : public views::WidgetDelegateView {
 public:
  explicit TestDelegate(bool system_modal) : system_modal_(system_modal) {}
  ~TestDelegate() override = default;

  // Overridden from views::WidgetDelegate:
  ui::ModalType GetModalType() const override {
    return system_modal_ ? ui::MODAL_TYPE_SYSTEM : ui::MODAL_TYPE_NONE;
  }

 private:
  bool system_modal_;

  DISALLOW_COPY_AND_ASSIGN(TestDelegate);
};

class DeleteOnBlurDelegate : public aura::test::TestWindowDelegate,
                             public aura::client::FocusChangeObserver {
 public:
  DeleteOnBlurDelegate() : window_(NULL) {}
  ~DeleteOnBlurDelegate() override = default;

  void SetWindow(aura::Window* window) {
    window_ = window;
    aura::client::SetFocusChangeObserver(window_, this);
  }

 private:
  // aura::test::TestWindowDelegate overrides:
  bool CanFocus() override { return true; }

  // aura::client::FocusChangeObserver implementation:
  void OnWindowFocused(aura::Window* gained_focus,
                       aura::Window* lost_focus) override {
    if (window_ == lost_focus)
      delete window_;
  }

  aura::Window* window_;

  DISALLOW_COPY_AND_ASSIGN(DeleteOnBlurDelegate);
};

aura::LayoutManager* GetLayoutManager(RootWindowController* controller,
                                      int id) {
  return controller->GetContainer(id)->layout_manager();
}

}  // namespace

class RootWindowControllerTest : public AshTestBase {
 public:
  views::Widget* CreateTestWidget(const gfx::Rect& bounds) {
    views::Widget* widget = views::Widget::CreateWindowWithContextAndBounds(
        NULL, CurrentContext(), bounds);
    // The initial bounds will be constrained to the screen work area or the
    // parent. See Widget::InitialBounds() & Widget::SetBoundsConstrained().
    // Explicitly setting the bounds here will allow the view to be positioned
    // such that it can extend outside the screen work area.
    widget->SetBounds(bounds);
    widget->Show();
    return widget;
  }

  views::Widget* CreateModalWidget(const gfx::Rect& bounds) {
    views::Widget* widget = views::Widget::CreateWindowWithContextAndBounds(
        new TestDelegate(true), CurrentContext(), bounds);
    // See the above comment.
    widget->SetBounds(bounds);
    widget->Show();
    return widget;
  }

  views::Widget* CreateModalWidgetWithParent(const gfx::Rect& bounds,
                                             aura::Window* parent) {
    views::Widget* widget = views::Widget::CreateWindowWithParentAndBounds(
        new TestDelegate(true), parent, bounds);
    // See the above comment.
    widget->SetBounds(bounds);
    widget->Show();
    return widget;
  }

  aura::Window* GetModalContainer(aura::Window* root_window) {
    return Shell::GetContainer(root_window,
                               kShellWindowId_SystemModalContainer);
  }
};

TEST_F(RootWindowControllerTest, MoveWindows_Basic) {
  // Windows origin should be doubled when moved to the 1st display.
  UpdateDisplay("600x600,300x300");
  aura::Window::Windows root_windows = Shell::GetAllRootWindows();

  views::Widget* normal = CreateTestWidget(gfx::Rect(650, 10, 100, 100));
  EXPECT_EQ(root_windows[1], normal->GetNativeView()->GetRootWindow());
  EXPECT_EQ("650,10 100x100", normal->GetWindowBoundsInScreen().ToString());
  EXPECT_EQ("50,10 100x100",
            normal->GetNativeView()->GetBoundsInRootWindow().ToString());

  views::Widget* maximized = CreateTestWidget(gfx::Rect(700, 10, 100, 100));
  maximized->Maximize();
  EXPECT_EQ(root_windows[1], maximized->GetNativeView()->GetRootWindow());
  EXPECT_EQ(gfx::Rect(600, 0, 300, 252).ToString(),
            maximized->GetWindowBoundsInScreen().ToString());
  EXPECT_EQ(gfx::Rect(0, 0, 300, 252).ToString(),
            maximized->GetNativeView()->GetBoundsInRootWindow().ToString());

  views::Widget* minimized = CreateTestWidget(gfx::Rect(550, 10, 200, 200));
  minimized->Minimize();
  EXPECT_EQ(root_windows[1], minimized->GetNativeView()->GetRootWindow());
  EXPECT_EQ("550,10 200x200", minimized->GetWindowBoundsInScreen().ToString());

  views::Widget* fullscreen = CreateTestWidget(gfx::Rect(850, 10, 200, 200));
  display::Display secondary_display = GetSecondaryDisplay();
  gfx::Rect orig_bounds = fullscreen->GetWindowBoundsInScreen();
  EXPECT_TRUE(secondary_display.work_area().Intersects(orig_bounds));
  EXPECT_FALSE(secondary_display.work_area().Contains(orig_bounds));

  fullscreen->SetFullscreen(true);
  EXPECT_EQ(root_windows[1], fullscreen->GetNativeView()->GetRootWindow());

  EXPECT_EQ("600,0 300x300", fullscreen->GetWindowBoundsInScreen().ToString());
  EXPECT_EQ("0,0 300x300",
            fullscreen->GetNativeView()->GetBoundsInRootWindow().ToString());

  views::Widget* unparented_control = new Widget;
  Widget::InitParams params;
  params.bounds = gfx::Rect(650, 10, 100, 100);
  params.type = Widget::InitParams::TYPE_CONTROL;
  unparented_control->Init(params);
  EXPECT_EQ(root_windows[1],
            unparented_control->GetNativeView()->GetRootWindow());
  EXPECT_EQ(kShellWindowId_UnparentedControlContainer,
            unparented_control->GetNativeView()->parent()->id());

  aura::Window* panel = CreateTestWindowInShellWithDelegateAndType(
      NULL, aura::client::WINDOW_TYPE_PANEL, 0, gfx::Rect(700, 100, 100, 100));
  EXPECT_EQ(root_windows[1], panel->GetRootWindow());
  EXPECT_EQ(kShellWindowId_PanelContainer, panel->parent()->id());

  // Make sure a window that will delete itself when losing focus
  // will not crash.
  aura::WindowTracker tracker;
  DeleteOnBlurDelegate delete_on_blur_delegate;
  aura::Window* d2 = CreateTestWindowInShellWithDelegate(
      &delete_on_blur_delegate, 0, gfx::Rect(50, 50, 100, 100));
  delete_on_blur_delegate.SetWindow(d2);
  aura::client::GetFocusClient(root_windows[0])->FocusWindow(d2);
  tracker.Add(d2);

  UpdateDisplay("600x600");

  // d2 must have been deleted.
  EXPECT_FALSE(tracker.Contains(d2));

  EXPECT_EQ(root_windows[0], normal->GetNativeView()->GetRootWindow());
  EXPECT_EQ("100,20 100x100", normal->GetWindowBoundsInScreen().ToString());
  EXPECT_EQ("100,20 100x100",
            normal->GetNativeView()->GetBoundsInRootWindow().ToString());

  // Maximized area on primary display has 48px for inset at the bottom
  // (kShelfSize).

  // First clear fullscreen status, since both fullscreen and maximized windows
  // share the same desktop workspace, which cancels the shelf status.
  fullscreen->SetFullscreen(false);
  EXPECT_EQ(root_windows[0], maximized->GetNativeView()->GetRootWindow());
  EXPECT_EQ(gfx::Rect(0, 0, 600, 552).ToString(),
            maximized->GetWindowBoundsInScreen().ToString());
  EXPECT_EQ(gfx::Rect(0, 0, 600, 552).ToString(),
            maximized->GetNativeView()->GetBoundsInRootWindow().ToString());

  // Set fullscreen to true, but maximized window's size won't change because
  // it's not visible. see crbug.com/504299.
  fullscreen->SetFullscreen(true);
  EXPECT_EQ(root_windows[0], maximized->GetNativeView()->GetRootWindow());
  EXPECT_EQ(gfx::Rect(0, 0, 600, 552).ToString(),
            maximized->GetWindowBoundsInScreen().ToString());
  EXPECT_EQ(gfx::Rect(0, 0, 600, 552).ToString(),
            maximized->GetNativeView()->GetBoundsInRootWindow().ToString());

  EXPECT_EQ(root_windows[0], minimized->GetNativeView()->GetRootWindow());
  EXPECT_EQ("0,20 200x200", minimized->GetWindowBoundsInScreen().ToString());

  EXPECT_EQ(root_windows[0], fullscreen->GetNativeView()->GetRootWindow());
  EXPECT_TRUE(fullscreen->IsFullscreen());
  EXPECT_EQ("0,0 600x600", fullscreen->GetWindowBoundsInScreen().ToString());
  EXPECT_EQ("0,0 600x600",
            fullscreen->GetNativeView()->GetBoundsInRootWindow().ToString());

  // Test if the restore bounds are correctly updated.
  wm::GetWindowState(maximized->GetNativeView())->Restore();
  EXPECT_EQ("200,20 100x100", maximized->GetWindowBoundsInScreen().ToString());
  EXPECT_EQ("200,20 100x100",
            maximized->GetNativeView()->GetBoundsInRootWindow().ToString());

  fullscreen->SetFullscreen(false);
  EXPECT_EQ("400,20 200x200", fullscreen->GetWindowBoundsInScreen().ToString());
  EXPECT_EQ("400,20 200x200",
            fullscreen->GetNativeView()->GetBoundsInRootWindow().ToString());

  // Test if the unparented widget has moved.
  EXPECT_EQ(root_windows[0],
            unparented_control->GetNativeView()->GetRootWindow());
  EXPECT_EQ(kShellWindowId_UnparentedControlContainer,
            unparented_control->GetNativeView()->parent()->id());

  // Test if the panel has moved.
  EXPECT_EQ(root_windows[0], panel->GetRootWindow());
  EXPECT_EQ(kShellWindowId_PanelContainer, panel->parent()->id());
}

TEST_F(RootWindowControllerTest, MoveWindows_Modal) {
  UpdateDisplay("500x500,500x500");

  aura::Window::Windows root_windows = Shell::GetAllRootWindows();
  // Emulate virtual screen coordinate system.
  root_windows[0]->SetBounds(gfx::Rect(0, 0, 500, 500));
  root_windows[1]->SetBounds(gfx::Rect(500, 0, 500, 500));

  views::Widget* normal = CreateTestWidget(gfx::Rect(300, 10, 100, 100));
  EXPECT_EQ(root_windows[0], normal->GetNativeView()->GetRootWindow());
  EXPECT_TRUE(wm::IsActiveWindow(normal->GetNativeView()));

  views::Widget* modal = CreateModalWidget(gfx::Rect(650, 10, 100, 100));
  EXPECT_EQ(root_windows[1], modal->GetNativeView()->GetRootWindow());
  EXPECT_TRUE(
      GetModalContainer(root_windows[1])->Contains(modal->GetNativeView()));
  EXPECT_TRUE(wm::IsActiveWindow(modal->GetNativeView()));

  ui::test::EventGenerator generator_1st(root_windows[0]);
  generator_1st.ClickLeftButton();
  EXPECT_TRUE(wm::IsActiveWindow(modal->GetNativeView()));

  UpdateDisplay("500x500");
  EXPECT_EQ(root_windows[0], modal->GetNativeView()->GetRootWindow());
  EXPECT_TRUE(wm::IsActiveWindow(modal->GetNativeView()));
  generator_1st.ClickLeftButton();
  EXPECT_TRUE(wm::IsActiveWindow(modal->GetNativeView()));
}

// Make sure lock related windows moves.
TEST_F(RootWindowControllerTest, MoveWindows_LockWindowsInUnified) {
  display_manager()->SetUnifiedDesktopEnabled(true);

  UpdateDisplay("500x500");
  const int kLockScreenWindowId = 1000;

  RootWindowController* controller = Shell::GetPrimaryRootWindowController();

  aura::Window* lock_container =
      controller->GetContainer(kShellWindowId_LockScreenContainer);

  views::Widget* lock_screen =
      CreateModalWidgetWithParent(gfx::Rect(10, 10, 100, 100), lock_container);
  lock_screen->GetNativeWindow()->set_id(kLockScreenWindowId);
  lock_screen->SetFullscreen(true);

  ASSERT_EQ(lock_screen->GetNativeWindow(),
            controller->GetRootWindow()->GetChildById(kLockScreenWindowId));
  EXPECT_EQ("0,0 500x500", lock_screen->GetNativeWindow()->bounds().ToString());

  // Switch to unified.
  UpdateDisplay("500x500,500x500");

  // In unified mode, RWC is created
  controller = Shell::GetPrimaryRootWindowController();

  ASSERT_EQ(lock_screen->GetNativeWindow(),
            controller->GetRootWindow()->GetChildById(kLockScreenWindowId));
  EXPECT_EQ("0,0 500x500", lock_screen->GetNativeWindow()->bounds().ToString());

  // Switch to mirror.
  display_manager()->SetMirrorMode(display::MirrorMode::kNormal, base::nullopt);
  EXPECT_TRUE(display_manager()->IsInMirrorMode());

  controller = Shell::GetPrimaryRootWindowController();
  ASSERT_EQ(lock_screen->GetNativeWindow(),
            controller->GetRootWindow()->GetChildById(kLockScreenWindowId));
  EXPECT_EQ("0,0 500x500", lock_screen->GetNativeWindow()->bounds().ToString());

  // Switch to unified.
  display_manager()->SetMirrorMode(display::MirrorMode::kOff, base::nullopt);
  EXPECT_TRUE(display_manager()->IsInUnifiedMode());

  controller = Shell::GetPrimaryRootWindowController();

  ASSERT_EQ(lock_screen->GetNativeWindow(),
            controller->GetRootWindow()->GetChildById(kLockScreenWindowId));
  EXPECT_EQ("0,0 500x500", lock_screen->GetNativeWindow()->bounds().ToString());

  // Switch to single display.
  UpdateDisplay("600x500");
  EXPECT_FALSE(display_manager()->IsInUnifiedMode());
  EXPECT_FALSE(display_manager()->IsInMirrorMode());

  controller = Shell::GetPrimaryRootWindowController();

  ASSERT_EQ(lock_screen->GetNativeWindow(),
            controller->GetRootWindow()->GetChildById(kLockScreenWindowId));
  EXPECT_EQ("0,0 600x500", lock_screen->GetNativeWindow()->bounds().ToString());
}

TEST_F(RootWindowControllerTest, ModalContainer) {
  UpdateDisplay("600x600");
  RootWindowController* controller = Shell::GetPrimaryRootWindowController();
  EXPECT_TRUE(Shell::Get()->session_controller()->IsActiveUserSessionStarted());
  EXPECT_EQ(GetLayoutManager(controller, kShellWindowId_SystemModalContainer),
            controller->GetSystemModalLayoutManager(NULL));

  views::Widget* session_modal_widget =
      CreateModalWidget(gfx::Rect(300, 10, 100, 100));
  EXPECT_EQ(GetLayoutManager(controller, kShellWindowId_SystemModalContainer),
            controller->GetSystemModalLayoutManager(
                session_modal_widget->GetNativeWindow()));

  Shell::Get()->session_controller()->LockScreenAndFlushForTest();
  EXPECT_TRUE(Shell::Get()->session_controller()->IsScreenLocked());
  EXPECT_EQ(
      GetLayoutManager(controller, kShellWindowId_LockSystemModalContainer),
      controller->GetSystemModalLayoutManager(nullptr));

  aura::Window* lock_container =
      controller->GetContainer(kShellWindowId_LockScreenContainer);
  views::Widget* lock_modal_widget =
      CreateModalWidgetWithParent(gfx::Rect(300, 10, 100, 100), lock_container);
  EXPECT_EQ(
      GetLayoutManager(controller, kShellWindowId_LockSystemModalContainer),
      controller->GetSystemModalLayoutManager(
          lock_modal_widget->GetNativeWindow()));
  EXPECT_EQ(GetLayoutManager(controller, kShellWindowId_SystemModalContainer),
            controller->GetSystemModalLayoutManager(
                session_modal_widget->GetNativeWindow()));

  GetSessionControllerClient()->UnlockScreen();
}

TEST_F(RootWindowControllerTest, ModalContainerNotLoggedInLoggedIn) {
  UpdateDisplay("600x600");

  // Configure login screen environment.
  SessionController* session_controller = Shell::Get()->session_controller();
  ClearLogin();
  EXPECT_EQ(0, session_controller->NumberOfLoggedInUsers());
  EXPECT_FALSE(session_controller->IsActiveUserSessionStarted());

  RootWindowController* controller = Shell::GetPrimaryRootWindowController();
  EXPECT_EQ(
      GetLayoutManager(controller, kShellWindowId_LockSystemModalContainer),
      controller->GetSystemModalLayoutManager(NULL));

  aura::Window* lock_container =
      controller->GetContainer(kShellWindowId_LockScreenContainer);
  views::Widget* login_modal_widget =
      CreateModalWidgetWithParent(gfx::Rect(300, 10, 100, 100), lock_container);
  EXPECT_EQ(
      GetLayoutManager(controller, kShellWindowId_LockSystemModalContainer),
      controller->GetSystemModalLayoutManager(
          login_modal_widget->GetNativeWindow()));
  login_modal_widget->Close();

  // Configure user session environment.
  CreateUserSessions(1);
  EXPECT_EQ(1, session_controller->NumberOfLoggedInUsers());
  EXPECT_TRUE(session_controller->IsActiveUserSessionStarted());
  EXPECT_EQ(GetLayoutManager(controller, kShellWindowId_SystemModalContainer),
            controller->GetSystemModalLayoutManager(NULL));

  views::Widget* session_modal_widget =
      CreateModalWidget(gfx::Rect(300, 10, 100, 100));
  EXPECT_EQ(GetLayoutManager(controller, kShellWindowId_SystemModalContainer),
            controller->GetSystemModalLayoutManager(
                session_modal_widget->GetNativeWindow()));
}

TEST_F(RootWindowControllerTest, ModalContainerBlockedSession) {
  UpdateDisplay("600x600");
  RootWindowController* controller = Shell::GetPrimaryRootWindowController();
  aura::Window* lock_container =
      controller->GetContainer(kShellWindowId_LockScreenContainer);
  for (int block_reason = FIRST_BLOCK_REASON;
       block_reason < NUMBER_OF_BLOCK_REASONS; ++block_reason) {
    views::Widget* session_modal_widget =
        CreateModalWidget(gfx::Rect(300, 10, 100, 100));
    EXPECT_EQ(GetLayoutManager(controller, kShellWindowId_SystemModalContainer),
              controller->GetSystemModalLayoutManager(
                  session_modal_widget->GetNativeWindow()));
    EXPECT_EQ(GetLayoutManager(controller, kShellWindowId_SystemModalContainer),
              controller->GetSystemModalLayoutManager(NULL));
    session_modal_widget->Close();

    BlockUserSession(static_cast<UserSessionBlockReason>(block_reason));

    EXPECT_EQ(
        GetLayoutManager(controller, kShellWindowId_LockSystemModalContainer),
        controller->GetSystemModalLayoutManager(NULL));

    views::Widget* lock_modal_widget = CreateModalWidgetWithParent(
        gfx::Rect(300, 10, 100, 100), lock_container);
    EXPECT_EQ(
        GetLayoutManager(controller, kShellWindowId_LockSystemModalContainer),
        controller->GetSystemModalLayoutManager(
            lock_modal_widget->GetNativeWindow()));

    session_modal_widget = CreateModalWidget(gfx::Rect(300, 10, 100, 100));
    EXPECT_EQ(GetLayoutManager(controller, kShellWindowId_SystemModalContainer),
              controller->GetSystemModalLayoutManager(
                  session_modal_widget->GetNativeWindow()));
    session_modal_widget->Close();

    lock_modal_widget->Close();
    UnblockUserSession();
  }
}

TEST_F(RootWindowControllerTest, GetWindowForFullscreenMode) {
  UpdateDisplay("600x600");
  RootWindowController* controller = Shell::GetPrimaryRootWindowController();

  Widget* w1 = CreateTestWidget(gfx::Rect(0, 0, 100, 100));
  w1->Maximize();
  Widget* w2 = CreateTestWidget(gfx::Rect(0, 0, 100, 100));
  w2->SetFullscreen(true);
  // |w3| is a transient child of |w2|.
  Widget* w3 = Widget::CreateWindowWithParentAndBounds(
      NULL, w2->GetNativeWindow(), gfx::Rect(0, 0, 100, 100));

  // Test that GetWindowForFullscreenMode() finds the fullscreen window when one
  // of its transient children is active.
  w3->Activate();
  EXPECT_EQ(w2->GetNativeWindow(), controller->GetWindowForFullscreenMode());

  // If the topmost window is not fullscreen, it returns NULL.
  w1->Activate();
  EXPECT_EQ(NULL, controller->GetWindowForFullscreenMode());
  w1->Close();
  w3->Close();

  // Only w2 remains, if minimized GetWindowForFullscreenMode should return
  // NULL.
  w2->Activate();
  EXPECT_EQ(w2->GetNativeWindow(), controller->GetWindowForFullscreenMode());
  w2->Minimize();
  EXPECT_EQ(NULL, controller->GetWindowForFullscreenMode());
}

TEST_F(RootWindowControllerTest, MultipleDisplaysGetWindowForFullscreenMode) {
  UpdateDisplay("600x600,600x600");
  Shell::RootWindowControllerList controllers =
      Shell::Get()->GetAllRootWindowControllers();

  Widget* w1 = CreateTestWidget(gfx::Rect(0, 0, 100, 100));
  w1->Maximize();
  Widget* w2 = CreateTestWidget(gfx::Rect(0, 0, 100, 100));
  w2->SetFullscreen(true);
  Widget* w3 = CreateTestWidget(gfx::Rect(600, 0, 100, 100));

  EXPECT_EQ(w1->GetNativeWindow()->GetRootWindow(),
            controllers[0]->GetRootWindow());
  EXPECT_EQ(w2->GetNativeWindow()->GetRootWindow(),
            controllers[0]->GetRootWindow());
  EXPECT_EQ(w3->GetNativeWindow()->GetRootWindow(),
            controllers[1]->GetRootWindow());

  w1->Activate();
  EXPECT_EQ(NULL, controllers[0]->GetWindowForFullscreenMode());
  EXPECT_EQ(NULL, controllers[1]->GetWindowForFullscreenMode());

  w2->Activate();
  EXPECT_EQ(w2->GetNativeWindow(),
            controllers[0]->GetWindowForFullscreenMode());
  EXPECT_EQ(NULL, controllers[1]->GetWindowForFullscreenMode());

  // Verify that the first root window controller remains in fullscreen mode
  // when a window on the other display is activated.
  w3->Activate();
  EXPECT_EQ(w2->GetNativeWindow(),
            controllers[0]->GetWindowForFullscreenMode());
  EXPECT_EQ(NULL, controllers[1]->GetWindowForFullscreenMode());
}

// Test that ForWindow() works with multiple displays and child widgets.
TEST_F(RootWindowControllerTest, ForWindow) {
  UpdateDisplay("600x600,600x600");
  Shell::RootWindowControllerList controllers =
      Shell::Get()->GetAllRootWindowControllers();
  ASSERT_EQ(2u, controllers.size());

  // Test a root window.
  EXPECT_EQ(controllers[0],
            RootWindowController::ForWindow(Shell::GetPrimaryRootWindow()));

  // Test a widget on the first display.
  Widget* w1 = CreateTestWidget(gfx::Rect(0, 0, 100, 100));
  EXPECT_EQ(controllers[0],
            RootWindowController::ForWindow(w1->GetNativeWindow()));

  // Test a child widget.
  Widget* w2 = Widget::CreateWindowWithParentAndBounds(
      nullptr, w1->GetNativeWindow(), gfx::Rect(0, 0, 100, 100));
  EXPECT_EQ(controllers[0],
            RootWindowController::ForWindow(w2->GetNativeWindow()));

  // Test a widget on the second display.
  Widget* w3 = CreateTestWidget(gfx::Rect(600, 0, 100, 100));
  EXPECT_EQ(controllers[1],
            RootWindowController::ForWindow(w3->GetNativeWindow()));
}

// Test that user session window can't be focused if user session blocked by
// some overlapping UI.
TEST_F(RootWindowControllerTest, FocusBlockedWindow) {
  UpdateDisplay("600x600");
  RootWindowController* controller = Shell::GetPrimaryRootWindowController();
  aura::Window* lock_container =
      controller->GetContainer(kShellWindowId_LockScreenContainer);
  aura::Window* lock_window =
      Widget::CreateWindowWithParentAndBounds(NULL, lock_container,
                                              gfx::Rect(0, 0, 100, 100))
          ->GetNativeView();
  lock_window->Show();
  aura::Window* session_window =
      CreateTestWidget(gfx::Rect(0, 0, 100, 100))->GetNativeView();
  session_window->Show();

  for (int block_reason = FIRST_BLOCK_REASON;
       block_reason < NUMBER_OF_BLOCK_REASONS; ++block_reason) {
    BlockUserSession(static_cast<UserSessionBlockReason>(block_reason));
    lock_window->Focus();
    EXPECT_TRUE(lock_window->HasFocus());
    session_window->Focus();
    EXPECT_FALSE(session_window->HasFocus());
    UnblockUserSession();
  }
}

// Tracks whether OnWindowDestroying() has been invoked.
class DestroyedWindowObserver : public aura::WindowObserver {
 public:
  DestroyedWindowObserver() : destroyed_(false), window_(NULL) {}
  ~DestroyedWindowObserver() override { Shutdown(); }

  void SetWindow(Window* window) {
    window_ = window;
    window->AddObserver(this);
  }

  bool destroyed() const { return destroyed_; }

  // WindowObserver overrides:
  void OnWindowDestroying(Window* window) override {
    destroyed_ = true;
    Shutdown();
  }

 private:
  void Shutdown() {
    if (!window_)
      return;
    window_->RemoveObserver(this);
    window_ = NULL;
  }

  bool destroyed_;
  Window* window_;

  DISALLOW_COPY_AND_ASSIGN(DestroyedWindowObserver);
};

// Verifies shutdown doesn't delete windows that are not owned by the parent.
TEST_F(RootWindowControllerTest, DontDeleteWindowsNotOwnedByParent) {
  DestroyedWindowObserver observer1;
  aura::test::TestWindowDelegate delegate1;
  aura::Window* window1 = new aura::Window(&delegate1);
  window1->SetType(aura::client::WINDOW_TYPE_CONTROL);
  window1->set_owned_by_parent(false);
  observer1.SetWindow(window1);
  window1->Init(ui::LAYER_NOT_DRAWN);
  aura::client::ParentWindowWithContext(window1, Shell::GetPrimaryRootWindow(),
                                        gfx::Rect());

  DestroyedWindowObserver observer2;
  aura::Window* window2 = new aura::Window(NULL);
  window2->set_owned_by_parent(false);
  observer2.SetWindow(window2);
  window2->Init(ui::LAYER_NOT_DRAWN);
  Shell::GetPrimaryRootWindow()->AddChild(window2);

  Shell::GetPrimaryRootWindowController()->CloseChildWindows();

  ASSERT_FALSE(observer1.destroyed());
  delete window1;

  ASSERT_FALSE(observer2.destroyed());
  delete window2;
}

// Verify that the context menu gets hidden when entering or exiting tablet
// mode.
TEST_F(RootWindowControllerTest, ContextMenuDisappearsInTabletMode) {
  RootWindowController* controller = Shell::GetPrimaryRootWindowController();

  // Open context menu.
  ui::test::EventGenerator generator(controller->GetRootWindow());
  generator.PressRightButton();
  generator.ReleaseRightButton();
  EXPECT_TRUE(controller->menu_model_);
  EXPECT_TRUE(controller->menu_runner_);

  // Verify menu closes on entering tablet mode.
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  EXPECT_FALSE(controller->menu_model_);
  EXPECT_FALSE(controller->menu_runner_);

  // Open context menu.
  generator.PressRightButton();
  generator.ReleaseRightButton();
  EXPECT_TRUE(controller->menu_model_);
  EXPECT_TRUE(controller->menu_runner_);

  // Verify menu closes on exiting tablet mode.
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  EXPECT_FALSE(controller->menu_model_);
  EXPECT_FALSE(controller->menu_runner_);
}

class VirtualKeyboardRootWindowControllerTest
    : public RootWindowControllerTest {
 public:
  VirtualKeyboardRootWindowControllerTest() = default;
  ~VirtualKeyboardRootWindowControllerTest() override = default;

  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        keyboard::switches::kEnableVirtualKeyboard);
    AshTestBase::SetUp();
    keyboard::SetTouchKeyboardEnabled(true);
    Shell::Get()->CreateKeyboard();
  }

  void TearDown() override {
    keyboard::SetTouchKeyboardEnabled(false);
    AshTestBase::TearDown();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(VirtualKeyboardRootWindowControllerTest);
};

class MockTextInputClient : public ui::DummyTextInputClient {
 public:
  MockTextInputClient() : ui::DummyTextInputClient(ui::TEXT_INPUT_TYPE_TEXT) {}

  void EnsureCaretNotInRect(const gfx::Rect& rect) override {
    caret_exclude_rect_ = rect;
  }

  const gfx::Rect& caret_exclude_rect() const { return caret_exclude_rect_; }

 private:
  gfx::Rect caret_exclude_rect_;

  DISALLOW_COPY_AND_ASSIGN(MockTextInputClient);
};

class TargetHitTestEventHandler : public ui::test::TestEventHandler {
 public:
  TargetHitTestEventHandler() = default;

  // ui::test::TestEventHandler overrides.
  void OnMouseEvent(ui::MouseEvent* event) override {
    if (event->type() == ui::ET_MOUSE_PRESSED)
      ui::test::TestEventHandler::OnMouseEvent(event);
    event->StopPropagation();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TargetHitTestEventHandler);
};

// Test for http://crbug.com/297858. Virtual keyboard container should only show
// on primary root window if no window has touch capability.
TEST_F(VirtualKeyboardRootWindowControllerTest,
       VirtualKeyboardOnPrimaryRootWindowDefault) {
  UpdateDisplay("500x500,500x500");

  aura::Window::Windows root_windows = Shell::GetAllRootWindows();
  aura::Window* primary_root_window = Shell::GetPrimaryRootWindow();
  aura::Window* secondary_root_window = root_windows[0] == primary_root_window
                                            ? root_windows[1]
                                            : root_windows[0];

  keyboard::KeyboardController* controller =
      keyboard::KeyboardController::GetInstance();
  ASSERT_TRUE(controller);
  aura::Window* vk_container_in_primary = Shell::GetContainer(
      primary_root_window, kShellWindowId_VirtualKeyboardContainer);
  aura::Window* vk_container_in_secondary = Shell::GetContainer(
      secondary_root_window, kShellWindowId_VirtualKeyboardContainer);
  aura::Window* vk_window =
      controller->GetContainerWindowWithoutCreationForTest();
  EXPECT_TRUE(vk_container_in_primary->Contains(vk_window));
  EXPECT_FALSE(vk_container_in_secondary->Contains(vk_window));
}

// Test for http://crbug.com/303429. Virtual keyboard container should show on
// a display which has touch capability.
TEST_F(VirtualKeyboardRootWindowControllerTest,
       VirtualKeyboardOnTouchableDisplayOnly) {
  UpdateDisplay("500x500,500x500");
  display::Display secondary_display =
      Shell::Get()->display_manager()->GetSecondaryDisplay();
  display::test::DisplayManagerTestApi(Shell::Get()->display_manager())
      .SetTouchSupport(secondary_display.id(),
                       display::Display::TouchSupport::AVAILABLE);

  // The primary display doesn't have touch capability and the secondary display
  // does.
  ASSERT_NE(display::Display::TouchSupport::AVAILABLE,
            display::Screen::GetScreen()->GetPrimaryDisplay().touch_support());
  ASSERT_EQ(
      display::Display::TouchSupport::AVAILABLE,
      Shell::Get()->display_manager()->GetSecondaryDisplay().touch_support());

  aura::Window::Windows root_windows = Shell::GetAllRootWindows();
  aura::Window* primary_root_window = Shell::GetPrimaryRootWindow();
  aura::Window* secondary_root_window = root_windows[0] == primary_root_window
                                            ? root_windows[1]
                                            : root_windows[0];
  aura::Window* vk_container_in_primary = Shell::GetContainer(
      primary_root_window, kShellWindowId_VirtualKeyboardContainer);
  aura::Window* vk_container_in_secondary = Shell::GetContainer(
      secondary_root_window, kShellWindowId_VirtualKeyboardContainer);
  keyboard::KeyboardController* controller =
      keyboard::KeyboardController::GetInstance();

  controller->ShowKeyboard(false);
  aura::Window* vk_window =
      controller->GetContainerWindowWithoutCreationForTest();
  EXPECT_FALSE(vk_container_in_primary->Contains(vk_window));
  EXPECT_TRUE(vk_container_in_secondary->Contains(vk_window));

  // Move the focus to the primary display.
  aura::Window* focusable_window_in_primary_display =
      CreateTestWindowInShellWithBounds(
          primary_root_window->GetBoundsInScreen());
  ASSERT_EQ(primary_root_window,
            focusable_window_in_primary_display->GetRootWindow());
  focusable_window_in_primary_display->Focus();
  ASSERT_TRUE(focusable_window_in_primary_display->HasFocus());

  // Virtual keyboard shows up on the secondary display even if a window in the
  // primary screen has the focus.
  controller->ShowKeyboard(false);
  EXPECT_FALSE(vk_container_in_primary->Contains(vk_window));
  EXPECT_TRUE(vk_container_in_secondary->Contains(vk_window));
}

// Test for http://crbug.com/303429. If both of displays have touch capability,
// virtual keyboard follows the input focus.
TEST_F(VirtualKeyboardRootWindowControllerTest, FollowInputFocus) {
  UpdateDisplay("500x500,500x500");
  const int64_t primary_display_id =
      display::Screen::GetScreen()->GetPrimaryDisplay().id();
  display::test::DisplayManagerTestApi(Shell::Get()->display_manager())
      .SetTouchSupport(primary_display_id,
                       display::Display::TouchSupport::AVAILABLE);
  const int64_t secondary_display_id =
      Shell::Get()->display_manager()->GetSecondaryDisplay().id();
  display::test::DisplayManagerTestApi(Shell::Get()->display_manager())
      .SetTouchSupport(secondary_display_id,
                       display::Display::TouchSupport::AVAILABLE);

  // Both of displays have touch capability.
  ASSERT_EQ(display::Display::TouchSupport::AVAILABLE,
            display::Screen::GetScreen()->GetPrimaryDisplay().touch_support());
  ASSERT_EQ(
      display::Display::TouchSupport::AVAILABLE,
      Shell::Get()->display_manager()->GetSecondaryDisplay().touch_support());

  aura::Window::Windows root_windows = Shell::GetAllRootWindows();
  aura::Window* primary_root_window = Shell::GetPrimaryRootWindow();
  aura::Window* secondary_root_window = root_windows[0] == primary_root_window
                                            ? root_windows[1]
                                            : root_windows[0];
  aura::Window* vk_container_in_primary = Shell::GetContainer(
      primary_root_window, kShellWindowId_VirtualKeyboardContainer);
  aura::Window* vk_container_in_secondary = Shell::GetContainer(
      secondary_root_window, kShellWindowId_VirtualKeyboardContainer);
  keyboard::KeyboardController* controller =
      keyboard::KeyboardController::GetInstance();
  aura::Window* focusable_window_in_primary_display =
      CreateTestWindowInShellWithBounds(
          primary_root_window->GetBoundsInScreen());
  ASSERT_EQ(primary_root_window,
            focusable_window_in_primary_display->GetRootWindow());
  aura::Window* focusable_window_in_secondary_display =
      CreateTestWindowInShellWithBounds(
          secondary_root_window->GetBoundsInScreen());
  ASSERT_EQ(secondary_root_window,
            focusable_window_in_secondary_display->GetRootWindow());

  controller->ShowKeyboard(false);
  aura::Window* vk_window =
      controller->GetContainerWindowWithoutCreationForTest();
  EXPECT_TRUE(vk_container_in_primary->Contains(vk_window));
  EXPECT_FALSE(vk_container_in_secondary->Contains(vk_window));

  // Move the focus to the secondary display.
  focusable_window_in_secondary_display->Focus();
  ASSERT_TRUE(focusable_window_in_secondary_display->HasFocus());

  controller->ShowKeyboard(false);
  EXPECT_FALSE(vk_container_in_primary->Contains(vk_window));
  EXPECT_TRUE(vk_container_in_secondary->Contains(vk_window));

  // Move back focus to the primary display.
  focusable_window_in_primary_display->Focus();
  ASSERT_TRUE(focusable_window_in_primary_display->HasFocus());

  controller->ShowKeyboard(false);
  EXPECT_TRUE(vk_container_in_primary->Contains(vk_window));
  EXPECT_FALSE(vk_container_in_secondary->Contains(vk_window));
}

// Test for http://crbug.com/303429. Even if both of display don't have touch
// capability, the virtual keyboard shows up on the specified display.
TEST_F(VirtualKeyboardRootWindowControllerTest,
       VirtualKeyboardShowOnSpecifiedDisplay) {
  UpdateDisplay("500x500,500x500");
  display::Display secondary_display = GetSecondaryDisplay();

  aura::Window::Windows root_windows = Shell::GetAllRootWindows();
  aura::Window* primary_root_window = Shell::GetPrimaryRootWindow();
  aura::Window* secondary_root_window = root_windows[0] == primary_root_window
                                            ? root_windows[1]
                                            : root_windows[0];
  aura::Window* vk_container_in_primary = Shell::GetContainer(
      primary_root_window, kShellWindowId_VirtualKeyboardContainer);
  aura::Window* vk_container_in_secondary = Shell::GetContainer(
      secondary_root_window, kShellWindowId_VirtualKeyboardContainer);
  keyboard::KeyboardController* controller =
      keyboard::KeyboardController::GetInstance();
  aura::Window* vk_window =
      controller->GetContainerWindowWithoutCreationForTest();

  ASSERT_TRUE(vk_container_in_primary->Contains(vk_window));
  ASSERT_FALSE(vk_container_in_secondary->Contains(vk_window));

  controller->ShowKeyboardInDisplay(secondary_display);

  EXPECT_FALSE(vk_container_in_primary->Contains(vk_window));
  EXPECT_TRUE(vk_container_in_secondary->Contains(vk_window));
}

// Test for http://crbug.com/263599. Virtual keyboard should be able to receive
// events at blocked user session.
TEST_F(VirtualKeyboardRootWindowControllerTest,
       ClickVirtualKeyboardInBlockedWindow) {
  aura::Window* root_window = Shell::GetPrimaryRootWindow();
  aura::Window* keyboard_container =
      Shell::GetContainer(root_window, kShellWindowId_VirtualKeyboardContainer);
  ASSERT_TRUE(keyboard_container);
  keyboard_container->Show();

  aura::Window* contents_window =
      keyboard::KeyboardController::GetInstance()->ui()->GetContentsWindow();
  contents_window->SetBounds(gfx::Rect());
  contents_window->Show();

  // Make sure no pending mouse events in the queue.
  RunAllPendingInMessageLoop();

  ui::test::TestEventHandler handler;
  root_window->AddPreTargetHandler(&handler);

  ui::test::EventGenerator event_generator(root_window, contents_window);
  event_generator.ClickLeftButton();
  int expected_mouse_presses = 1;
  EXPECT_EQ(expected_mouse_presses, handler.num_mouse_events() / 2);

  for (int block_reason = FIRST_BLOCK_REASON;
       block_reason < NUMBER_OF_BLOCK_REASONS; ++block_reason) {
    BlockUserSession(static_cast<UserSessionBlockReason>(block_reason));
    event_generator.ClickLeftButton();
    expected_mouse_presses++;
    EXPECT_EQ(expected_mouse_presses, handler.num_mouse_events() / 2);
    UnblockUserSession();
  }
  root_window->RemovePreTargetHandler(&handler);
}

// Test for http://crbug.com/299787. RootWindowController should delete
// the old container since the keyboard controller creates a new window in
// GetWindowContainer().
TEST_F(VirtualKeyboardRootWindowControllerTest,
       DeleteOldContainerOnVirtualKeyboardInit) {
  keyboard::KeyboardController* controller =
      keyboard::KeyboardController::GetInstance();
  aura::Window* keyboard_container =
      controller->GetContainerWindowWithoutCreationForTest();
  ASSERT_TRUE(keyboard_container);
  // Track the keyboard container window.
  aura::WindowTracker tracker;
  tracker.Add(keyboard_container);
  // Reinitialize the keyboard.
  Shell::Get()->CreateKeyboard();
  // keyboard_container should no longer be present.
  EXPECT_FALSE(tracker.Contains(keyboard_container));
}

// Test for crbug.com/342524. After user login, the work space should restore to
// full screen.
TEST_F(VirtualKeyboardRootWindowControllerTest, RestoreWorkspaceAfterLogin) {
  aura::Window* root_window = Shell::GetPrimaryRootWindow();
  aura::Window* keyboard_container =
      Shell::GetContainer(root_window, kShellWindowId_VirtualKeyboardContainer);
  keyboard_container->Show();
  keyboard::KeyboardController* controller =
      keyboard::KeyboardController::GetInstance();
  aura::Window* contents_window = controller->ui()->GetContentsWindow();
  contents_window->SetBounds(
      keyboard::KeyboardBoundsFromRootBounds(root_window->bounds(), 100));
  contents_window->Show();

  gfx::Rect before =
      display::Screen::GetScreen()->GetPrimaryDisplay().work_area();

  // Notify keyboard bounds changing.
  controller->NotifyContentsBoundsChanging(keyboard_container->bounds());

  if (!keyboard::IsKeyboardOverscrollEnabled()) {
    gfx::Rect after =
        display::Screen::GetScreen()->GetPrimaryDisplay().work_area();
    EXPECT_LT(after, before);
  }

  // Mock a login user profile change to reinitialize the keyboard.
  mojom::SessionInfoPtr info = mojom::SessionInfo::New();
  info->state = session_manager::SessionState::ACTIVE;
  Shell::Get()->session_controller()->SetSessionInfo(std::move(info));
  EXPECT_EQ(display::Screen::GetScreen()->GetPrimaryDisplay().work_area(),
            before);
}

// Ensure that system modal dialogs do not block events targeted at the virtual
// keyboard.
TEST_F(VirtualKeyboardRootWindowControllerTest, ClickWithActiveModalDialog) {
  keyboard::KeyboardController* controller =
      keyboard::KeyboardController::GetInstance();
  aura::Window* root_window = Shell::GetPrimaryRootWindow();
  aura::Window* container_window = controller->GetContainerWindow();
  ASSERT_TRUE(container_window);
  ASSERT_TRUE(root_window->Contains(container_window));
  container_window->Show();

  aura::Window* contents_window = controller->ui()->GetContentsWindow();
  contents_window->SetName("KeyboardContentsWindow");
  contents_window->SetBounds(
      keyboard::KeyboardBoundsFromRootBounds(root_window->bounds(), 100));

  ui::test::TestEventHandler handler;
  root_window->AddPreTargetHandler(&handler);
  ui::test::EventGenerator root_window_event_generator(root_window);
  ui::test::EventGenerator keyboard_event_generator(root_window,
                                                    contents_window);

  views::Widget* modal_widget = CreateModalWidget(gfx::Rect(300, 10, 100, 100));

  // Verify that mouse events to the root window are block with a visble modal
  // dialog.
  root_window_event_generator.ClickLeftButton();
  EXPECT_EQ(0, handler.num_mouse_events());

  // Verify that event dispatch to the virtual keyboard is unblocked.
  keyboard_event_generator.ClickLeftButton();
  EXPECT_EQ(1, handler.num_mouse_events() / 2);

  modal_widget->Close();

  // Verify that mouse events are now unblocked to the root window.
  root_window_event_generator.ClickLeftButton();
  EXPECT_EQ(2, handler.num_mouse_events() / 2);
  root_window->RemovePreTargetHandler(&handler);
}

// Ensure that the visible area for scrolling the text caret excludes the
// region occluded by the on-screen keyboard.
TEST_F(VirtualKeyboardRootWindowControllerTest, EnsureCaretInWorkArea) {
  keyboard::KeyboardController* keyboard_controller =
      keyboard::KeyboardController::GetInstance();
  keyboard::KeyboardUI* ui = keyboard_controller->ui();

  MockTextInputClient text_input_client;
  ui::InputMethod* input_method = ui->GetInputMethod();
  ASSERT_TRUE(input_method);
  input_method->SetFocusedTextInputClient(&text_input_client);

  aura::Window* root_window = Shell::GetPrimaryRootWindow();
  ASSERT_TRUE(keyboard_controller->GetContainerWindowWithoutCreationForTest());
  keyboard_controller->GetContainerWindow()->Show();

  const int keyboard_height = 100;
  aura::Window* contents_window = ui->GetContentsWindow();
  contents_window->SetBounds(keyboard::KeyboardBoundsFromRootBounds(
      root_window->bounds(), keyboard_height));
  contents_window->Show();

  ui->EnsureCaretInWorkArea(contents_window->GetBoundsInScreen());
  ASSERT_EQ(root_window->bounds().width(),
            text_input_client.caret_exclude_rect().width());
  ASSERT_EQ(keyboard_height, text_input_client.caret_exclude_rect().height());

  input_method->SetFocusedTextInputClient(nullptr);
}

TEST_F(VirtualKeyboardRootWindowControllerTest,
       EnsureCaretInWorkAreaWithMultipleDisplays) {
  UpdateDisplay("500x500,600x600");
  const int64_t primary_display_id =
      display::Screen::GetScreen()->GetPrimaryDisplay().id();
  const int64_t secondary_display_id = GetSecondaryDisplay().id();
  ASSERT_NE(primary_display_id, secondary_display_id);

  aura::Window::Windows root_windows = Shell::GetAllRootWindows();
  ASSERT_EQ(static_cast<size_t>(2), root_windows.size());
  aura::Window* primary_root_window = root_windows[0];
  aura::Window* secondary_root_window = root_windows[1];

  keyboard::KeyboardController* keyboard_controller =
      keyboard::KeyboardController::GetInstance();
  keyboard::KeyboardUI* ui = keyboard_controller->ui();

  MockTextInputClient text_input_client;
  ui::InputMethod* input_method = ui->GetInputMethod();
  ASSERT_TRUE(input_method);
  input_method->SetFocusedTextInputClient(&text_input_client);

  const int keyboard_height = 100;
  // Check that the keyboard on the primary screen doesn't cover the window on
  // the secondary screen.
  aura::Window* keyboard_container = keyboard_controller->GetContainerWindow();
  ASSERT_TRUE(keyboard_container);
  keyboard_container->Show();
  aura::Window* contents_window = ui->GetContentsWindow();
  contents_window->SetBounds(keyboard::KeyboardBoundsFromRootBounds(
      primary_root_window->bounds(), keyboard_height));
  contents_window->Show();

  ui->EnsureCaretInWorkArea(contents_window->GetBoundsInScreen());
  EXPECT_TRUE(primary_root_window->GetBoundsInScreen().Contains(
      text_input_client.caret_exclude_rect()));
  EXPECT_EQ(primary_root_window->GetBoundsInScreen().width(),
            text_input_client.caret_exclude_rect().width());
  EXPECT_FALSE(secondary_root_window->GetBoundsInScreen().Contains(
      text_input_client.caret_exclude_rect()));

  // Move the keyboard into the secondary display and check that the keyboard
  // doesn't cover the window on the primary screen.
  keyboard_controller->ShowKeyboardInDisplay(GetSecondaryDisplay());
  contents_window->SetBounds(keyboard::KeyboardBoundsFromRootBounds(
      secondary_root_window->bounds(), keyboard_height));

  ui->EnsureCaretInWorkArea(contents_window->GetBoundsInScreen());
  EXPECT_FALSE(primary_root_window->GetBoundsInScreen().Contains(
      text_input_client.caret_exclude_rect()));
  EXPECT_TRUE(secondary_root_window->GetBoundsInScreen().Contains(
      text_input_client.caret_exclude_rect()));
  EXPECT_EQ(secondary_root_window->GetBoundsInScreen().width(),
            text_input_client.caret_exclude_rect().width());

  input_method->SetFocusedTextInputClient(nullptr);
}

// Tests that the virtual keyboard does not block context menus. The virtual
// keyboard should appear in front of most content, but not context menus. See
// crbug/377180.
TEST_F(VirtualKeyboardRootWindowControllerTest, ZOrderTest) {
  UpdateDisplay("800x600");
  keyboard::KeyboardController* keyboard_controller =
      keyboard::KeyboardController::GetInstance();
  keyboard::KeyboardUI* ui = keyboard_controller->ui();

  aura::Window* root_window = Shell::GetPrimaryRootWindow();
  aura::Window* keyboard_container = keyboard_controller->GetContainerWindow();
  ASSERT_TRUE(keyboard_container);
  keyboard_container->Show();

  const int keyboard_height = 200;
  aura::Window* contents_window = ui->GetContentsWindow();
  gfx::Rect keyboard_bounds = keyboard::KeyboardBoundsFromRootBounds(
      root_window->bounds(), keyboard_height);
  contents_window->SetBounds(keyboard_bounds);
  contents_window->Show();

  ui::test::EventGenerator generator(root_window);

  // Cover the screen with two windows: a normal window on the left side and a
  // context menu on the right side. When the virtual keyboard is displayed it
  // partially occludes the normal window, but not the context menu. Compute
  // positions for generating synthetic click events to perform hit tests,
  // ensuring the correct window layering. 'top' is above the VK, whereas
  // 'bottom' lies within the VK. 'left' is centered in the normal window, and
  // 'right' is centered in the context menu.
  int window_height = keyboard_bounds.bottom();
  int window_width = keyboard_bounds.width() / 2;
  int left = window_width / 2;
  int right = 3 * window_width / 2;
  int top = keyboard_bounds.y() / 2;
  int bottom = window_height - keyboard_height / 2;

  // Normal window is partially occluded by the virtual keyboard.
  aura::test::TestWindowDelegate delegate;
  std::unique_ptr<aura::Window> normal(
      CreateTestWindowInShellWithDelegateAndType(
          &delegate, aura::client::WINDOW_TYPE_NORMAL, 0,
          gfx::Rect(0, 0, window_width, window_height)));
  normal->set_owned_by_parent(false);
  normal->Show();
  TargetHitTestEventHandler normal_handler;
  normal->AddPreTargetHandler(&normal_handler);

  // Test that only the click on the top portion of the window is picked up. The
  // click on the bottom hits the virtual keyboard instead.
  generator.MoveMouseTo(left, top);
  generator.ClickLeftButton();
  EXPECT_EQ(1, normal_handler.num_mouse_events());
  generator.MoveMouseTo(left, bottom);
  generator.ClickLeftButton();
  EXPECT_EQ(1, normal_handler.num_mouse_events());

  // Menu overlaps virtual keyboard.
  aura::test::TestWindowDelegate delegate2;
  std::unique_ptr<aura::Window> menu(CreateTestWindowInShellWithDelegateAndType(
      &delegate2, aura::client::WINDOW_TYPE_MENU, 0,
      gfx::Rect(window_width, 0, window_width, window_height)));
  menu->set_owned_by_parent(false);
  menu->Show();
  TargetHitTestEventHandler menu_handler;
  menu->AddPreTargetHandler(&menu_handler);

  // Test that both clicks register.
  generator.MoveMouseTo(right, top);
  generator.ClickLeftButton();
  EXPECT_EQ(1, menu_handler.num_mouse_events());
  generator.MoveMouseTo(right, bottom);
  generator.ClickLeftButton();
  EXPECT_EQ(2, menu_handler.num_mouse_events());

  // Cleanup to ensure that the test windows are destroyed before their
  // delegates.
  normal.reset();
  menu.reset();
}

// Tests that the virtual keyboard correctly resizes with a change to display
// orientation. See crbug/417612.
TEST_F(VirtualKeyboardRootWindowControllerTest, DisplayRotation) {
  UpdateDisplay("800x600");
  keyboard::KeyboardController* keyboard_controller =
      keyboard::KeyboardController::GetInstance();
  aura::Window* keyboard_container = keyboard_controller->GetContainerWindow();
  keyboard_controller->ShowKeyboard(false);
  keyboard_controller->ui()->GetContentsWindow()->SetBounds(
      gfx::Rect(0, 400, 800, 200));
  EXPECT_EQ("0,400 800x200", keyboard_container->bounds().ToString());

  UpdateDisplay("600x800");
  EXPECT_EQ("0,600 600x200", keyboard_container->bounds().ToString());
}

}  // namespace ash
