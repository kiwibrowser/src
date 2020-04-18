// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/tablet_mode/tablet_mode_window_manager.h"

#include <string>

#include "ash/public/cpp/ash_switches.h"
#include "ash/public/cpp/shelf_prefs.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/public/cpp/window_state_type.h"
#include "ash/root_window_controller.h"
#include "ash/screen_util.h"
#include "ash/session/session_controller.h"
#include "ash/session/test_session_controller_client.h"
#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/shell_test_api.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/mru_window_tracker.h"
#include "ash/wm/overview/window_selector_controller.h"
#include "ash/wm/switchable_windows.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "ash/wm/window_properties.h"
#include "ash/wm/window_resizer.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_state_observer.h"
#include "ash/wm/window_util.h"
#include "ash/wm/wm_event.h"
#include "base/command_line.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/test/test_windows.h"
#include "ui/aura/window.h"
#include "ui/base/hit_test.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/widget/widget.h"

namespace ash {

// A helper function to set the shelf auto-hide preference. This has the same
// effect as the user toggling the shelf context menu option.
void SetShelfAutoHideBehaviorPref(int64_t display_id,
                                  ShelfAutoHideBehavior behavior) {
  PrefService* prefs =
      Shell::Get()->session_controller()->GetLastActiveUserPrefService();
  if (!prefs)
    return;
  SetShelfAutoHideBehaviorPref(prefs, display_id, behavior);
}

class TabletModeWindowManagerTest : public AshTestBase {
 public:
  TabletModeWindowManagerTest() = default;
  ~TabletModeWindowManagerTest() override = default;

  // Initialize parameters for test windows.  If |can_maximize| is not
  // set, |max_size| is the upper limiting size for the window,
  // whereas an empty size means that there is no limit.
  struct InitParams {
    InitParams(aura::client::WindowType t) : type(t) {}

    aura::client::WindowType type = aura::client::WINDOW_TYPE_NORMAL;
    gfx::Rect bounds;
    gfx::Size max_size;
    bool can_maximize = true;
    bool can_resize = true;
    bool show_on_creation = true;
  };

  // Creates a window which has a fixed size.
  aura::Window* CreateFixedSizeNonMaximizableWindow(
      aura::client::WindowType type,
      const gfx::Rect& bounds) {
    InitParams params(type);
    params.bounds = bounds;
    params.can_maximize = false;
    params.can_resize = false;
    return CreateWindowInWatchedContainer(params);
  }

  // Creates a window which can not be maximized, but resized. |max_size|
  // denotes the maximal possible size, if the size is empty, the window has no
  // upper limit. Note: This function will only work with a single root window.
  aura::Window* CreateNonMaximizableWindow(aura::client::WindowType type,
                                           const gfx::Rect& bounds,
                                           const gfx::Size& max_size) {
    InitParams params(type);
    params.bounds = bounds;
    params.max_size = max_size;
    params.can_maximize = false;
    return CreateWindowInWatchedContainer(params);
  }

  // Creates a maximizable and resizable window.
  aura::Window* CreateWindow(aura::client::WindowType type,
                             const gfx::Rect bounds) {
    InitParams params(type);
    params.bounds = bounds;
    return CreateWindowInWatchedContainer(params);
  }

  // Creates a window which also has a widget.
  aura::Window* CreateWindowWithWidget(const gfx::Rect& bounds) {
    views::Widget* widget = new views::Widget();
    views::Widget::InitParams params;
    // Note: The widget will get deleted with the window.
    widget->Init(params);
    widget->Show();
    aura::Window* window = widget->GetNativeWindow();
    window->SetBounds(bounds);
    window->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_NORMAL);

    return window;
  }

  // Create the tablet mode window manager.
  TabletModeWindowManager* CreateTabletModeWindowManager() {
    EXPECT_FALSE(tablet_mode_window_manager());
    Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
    return tablet_mode_window_manager();
  }

  // Destroy the tablet mode window manager.
  void DestroyTabletModeWindowManager() {
    Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(
        false);
    EXPECT_FALSE(tablet_mode_window_manager());
  }

  // Get the tablet window manager.
  TabletModeWindowManager* tablet_mode_window_manager() {
    return Shell::Get()
        ->tablet_mode_controller()
        ->tablet_mode_window_manager_.get();
  }

  // Resize our desktop.
  void ResizeDesktop(int width_delta) {
    gfx::Size size =
        display::Screen::GetScreen()
            ->GetDisplayNearestWindow(Shell::GetPrimaryRootWindow())
            .size();
    size.Enlarge(0, width_delta);
    UpdateDisplay(size.ToString());
  }

  // Create a window in one of the containers which are watched by the
  // TabletModeWindowManager. Note that this only works with one root window.
  aura::Window* CreateWindowInWatchedContainer(const InitParams& params) {
    aura::test::TestWindowDelegate* delegate = NULL;
    if (!params.can_maximize) {
      delegate = aura::test::TestWindowDelegate::CreateSelfDestroyingDelegate();
      delegate->set_window_component(HTCAPTION);
      if (!params.max_size.IsEmpty())
        delegate->set_maximum_size(params.max_size);
    }
    aura::Window* window = aura::test::CreateTestWindowWithDelegateAndType(
        delegate, params.type, 0, params.bounds, NULL, params.show_on_creation);
    int32_t behavior = ui::mojom::kResizeBehaviorNone;
    behavior |= params.can_resize ? ui::mojom::kResizeBehaviorCanResize : 0;
    behavior |= params.can_maximize ? ui::mojom::kResizeBehaviorCanMaximize : 0;
    window->SetProperty(aura::client::kResizeBehaviorKey, behavior);
    aura::Window* container = Shell::GetContainer(
        Shell::GetPrimaryRootWindow(), wm::kSwitchableWindowContainerIds[0]);
    container->AddChild(window);
    return window;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TabletModeWindowManagerTest);
};

// Test that creating the object and destroying it without any windows should
// not cause any problems.
TEST_F(TabletModeWindowManagerTest, SimpleStart) {
  TabletModeWindowManager* manager = CreateTabletModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(0, manager->GetNumberOfManagedWindows());
  DestroyTabletModeWindowManager();
}

// Test that existing windows will handled properly when going into tablet
// mode.
TEST_F(TabletModeWindowManagerTest, PreCreateWindows) {
  // Bounds for windows we know can be controlled.
  gfx::Rect rect1(10, 10, 200, 50);
  gfx::Rect rect2(10, 60, 200, 50);
  gfx::Rect rect3(20, 140, 100, 100);
  // Bounds for anything else.
  gfx::Rect rect(80, 90, 100, 110);
  std::unique_ptr<aura::Window> w1(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect1));
  std::unique_ptr<aura::Window> w2(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect2));
  std::unique_ptr<aura::Window> w3(CreateFixedSizeNonMaximizableWindow(
      aura::client::WINDOW_TYPE_NORMAL, rect3));
  std::unique_ptr<aura::Window> w4(
      CreateWindow(aura::client::WINDOW_TYPE_PANEL, rect));
  std::unique_ptr<aura::Window> w5(
      CreateWindow(aura::client::WINDOW_TYPE_POPUP, rect));
  std::unique_ptr<aura::Window> w6(
      CreateWindow(aura::client::WINDOW_TYPE_CONTROL, rect));
  std::unique_ptr<aura::Window> w7(
      CreateWindow(aura::client::WINDOW_TYPE_MENU, rect));
  std::unique_ptr<aura::Window> w8(
      CreateWindow(aura::client::WINDOW_TYPE_TOOLTIP, rect));
  EXPECT_FALSE(wm::GetWindowState(w1.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w2.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w3.get())->IsMaximized());
  EXPECT_EQ(rect1.ToString(), w1->bounds().ToString());
  EXPECT_EQ(rect2.ToString(), w2->bounds().ToString());
  EXPECT_EQ(rect3.ToString(), w3->bounds().ToString());

  // Create the manager and make sure that all qualifying windows were detected
  // and changed.
  TabletModeWindowManager* manager = CreateTabletModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(3, manager->GetNumberOfManagedWindows());
  EXPECT_TRUE(wm::GetWindowState(w1.get())->IsMaximized());
  EXPECT_TRUE(wm::GetWindowState(w2.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w3.get())->IsMaximized());
  EXPECT_NE(rect3.origin().ToString(), w3->bounds().origin().ToString());
  EXPECT_EQ(rect3.size().ToString(), w3->bounds().size().ToString());

  // All other windows should not have been touched.
  EXPECT_FALSE(wm::GetWindowState(w4.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w5.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w6.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w7.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w8.get())->IsMaximized());
  EXPECT_EQ(rect.ToString(), w4->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w5->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w6->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w7->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w8->bounds().ToString());

  // Destroy the manager again and check that the windows return to their
  // previous state.
  DestroyTabletModeWindowManager();
  EXPECT_FALSE(wm::GetWindowState(w1.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w2.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w3.get())->IsMaximized());
  EXPECT_EQ(rect1.ToString(), w1->bounds().ToString());
  EXPECT_EQ(rect2.ToString(), w2->bounds().ToString());
  EXPECT_EQ(rect3.ToString(), w3->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w4->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w5->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w6->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w7->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w8->bounds().ToString());
}

// The same test as the above but while a system modal dialog is shown.
TEST_F(TabletModeWindowManagerTest, GoingToMaximizedWithModalDialogPresent) {
  // Bounds for windows we know can be controlled.
  gfx::Rect rect1(10, 10, 200, 50);
  gfx::Rect rect2(10, 60, 200, 50);
  gfx::Rect rect3(20, 140, 100, 100);
  // Bounds for anything else.
  gfx::Rect rect(80, 90, 100, 110);
  std::unique_ptr<aura::Window> w1(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect1));
  std::unique_ptr<aura::Window> w2(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect2));
  std::unique_ptr<aura::Window> w3(CreateFixedSizeNonMaximizableWindow(
      aura::client::WINDOW_TYPE_NORMAL, rect3));
  std::unique_ptr<aura::Window> w4(
      CreateWindow(aura::client::WINDOW_TYPE_PANEL, rect));
  std::unique_ptr<aura::Window> w5(
      CreateWindow(aura::client::WINDOW_TYPE_POPUP, rect));
  std::unique_ptr<aura::Window> w6(
      CreateWindow(aura::client::WINDOW_TYPE_CONTROL, rect));
  std::unique_ptr<aura::Window> w7(
      CreateWindow(aura::client::WINDOW_TYPE_MENU, rect));
  std::unique_ptr<aura::Window> w8(
      CreateWindow(aura::client::WINDOW_TYPE_TOOLTIP, rect));
  EXPECT_FALSE(wm::GetWindowState(w1.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w2.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w3.get())->IsMaximized());
  EXPECT_EQ(rect1.ToString(), w1->bounds().ToString());
  EXPECT_EQ(rect2.ToString(), w2->bounds().ToString());
  EXPECT_EQ(rect3.ToString(), w3->bounds().ToString());

  // Enable system modal dialog, and make sure both shelves are still hidden.
  ShellTestApi().SimulateModalWindowOpenForTest(true);
  EXPECT_TRUE(Shell::IsSystemModalWindowOpen());

  // Create the manager and make sure that all qualifying windows were detected
  // and changed.
  TabletModeWindowManager* manager = CreateTabletModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(3, manager->GetNumberOfManagedWindows());
  EXPECT_TRUE(wm::GetWindowState(w1.get())->IsMaximized());
  EXPECT_TRUE(wm::GetWindowState(w2.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w3.get())->IsMaximized());
  EXPECT_NE(rect3.origin().ToString(), w3->bounds().origin().ToString());
  EXPECT_EQ(rect3.size().ToString(), w3->bounds().size().ToString());

  // All other windows should not have been touched.
  EXPECT_FALSE(wm::GetWindowState(w4.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w5.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w6.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w7.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w8.get())->IsMaximized());
  EXPECT_EQ(rect.ToString(), w4->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w5->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w6->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w7->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w8->bounds().ToString());

  // Destroy the manager again and check that the windows return to their
  // previous state.
  DestroyTabletModeWindowManager();
  EXPECT_FALSE(wm::GetWindowState(w1.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w2.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w3.get())->IsMaximized());
  EXPECT_EQ(rect1.ToString(), w1->bounds().ToString());
  EXPECT_EQ(rect2.ToString(), w2->bounds().ToString());
  EXPECT_EQ(rect3.ToString(), w3->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w4->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w5->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w6->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w7->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w8->bounds().ToString());
}

// Test that non-maximizable windows get properly handled when going into
// tablet mode.
TEST_F(TabletModeWindowManagerTest,
       PreCreateNonMaximizableButResizableWindows) {
  // The window bounds.
  gfx::Rect rect(10, 10, 200, 50);
  gfx::Size max_size(300, 200);
  gfx::Size empty_size;
  std::unique_ptr<aura::Window> unlimited_window(CreateNonMaximizableWindow(
      aura::client::WINDOW_TYPE_NORMAL, rect, empty_size));
  std::unique_ptr<aura::Window> limited_window(CreateNonMaximizableWindow(
      aura::client::WINDOW_TYPE_NORMAL, rect, max_size));
  std::unique_ptr<aura::Window> fixed_window(
      CreateFixedSizeNonMaximizableWindow(aura::client::WINDOW_TYPE_NORMAL,
                                          rect));
  EXPECT_FALSE(wm::GetWindowState(unlimited_window.get())->IsMaximized());
  EXPECT_EQ(rect.ToString(), unlimited_window->bounds().ToString());
  EXPECT_FALSE(wm::GetWindowState(limited_window.get())->IsMaximized());
  EXPECT_EQ(rect.ToString(), limited_window->bounds().ToString());
  EXPECT_FALSE(wm::GetWindowState(fixed_window.get())->IsMaximized());
  EXPECT_EQ(rect.ToString(), fixed_window->bounds().ToString());

  gfx::Size workspace_size =
      screen_util::GetMaximizedWindowBoundsInParent(unlimited_window.get())
          .size();

  // Create the manager and make sure that all qualifying windows were detected
  // and changed.
  TabletModeWindowManager* manager = CreateTabletModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(3, manager->GetNumberOfManagedWindows());
  // The unlimited window should have the size of the workspace / parent window.
  EXPECT_FALSE(wm::GetWindowState(unlimited_window.get())->IsMaximized());
  EXPECT_EQ("0,0", unlimited_window->bounds().origin().ToString());
  EXPECT_EQ(workspace_size.ToString(),
            unlimited_window->bounds().size().ToString());
  // The limited window should have the size of the upper possible bounds.
  EXPECT_FALSE(wm::GetWindowState(limited_window.get())->IsMaximized());
  EXPECT_NE(rect.origin().ToString(),
            limited_window->bounds().origin().ToString());
  EXPECT_EQ(max_size.ToString(), limited_window->bounds().size().ToString());
  // The fixed size window should have the size of the original window.
  EXPECT_FALSE(wm::GetWindowState(fixed_window.get())->IsMaximized());
  EXPECT_NE(rect.origin().ToString(),
            fixed_window->bounds().origin().ToString());
  EXPECT_EQ(rect.size().ToString(), fixed_window->bounds().size().ToString());

  // Destroy the manager again and check that the windows return to their
  // previous state.
  DestroyTabletModeWindowManager();
  EXPECT_FALSE(wm::GetWindowState(unlimited_window.get())->IsMaximized());
  EXPECT_EQ(rect.ToString(), unlimited_window->bounds().ToString());
  EXPECT_FALSE(wm::GetWindowState(limited_window.get())->IsMaximized());
  EXPECT_EQ(rect.ToString(), limited_window->bounds().ToString());
  EXPECT_FALSE(wm::GetWindowState(fixed_window.get())->IsMaximized());
  EXPECT_EQ(rect.ToString(), fixed_window->bounds().ToString());
}

// Test that creating windows while a maximizer exists picks them properly up.
TEST_F(TabletModeWindowManagerTest, CreateWindows) {
  TabletModeWindowManager* manager = CreateTabletModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(0, manager->GetNumberOfManagedWindows());

  // Create the windows and see that the window manager picks them up.
  // Rects for windows we know can be controlled.
  gfx::Rect rect1(10, 10, 200, 50);
  gfx::Rect rect2(10, 60, 200, 50);
  gfx::Rect rect3(20, 140, 100, 100);
  // One rect for anything else.
  gfx::Rect rect(80, 90, 100, 110);
  std::unique_ptr<aura::Window> w1(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect1));
  std::unique_ptr<aura::Window> w2(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect2));
  std::unique_ptr<aura::Window> w3(CreateFixedSizeNonMaximizableWindow(
      aura::client::WINDOW_TYPE_NORMAL, rect3));
  std::unique_ptr<aura::Window> w4(
      CreateWindow(aura::client::WINDOW_TYPE_PANEL, rect));
  std::unique_ptr<aura::Window> w5(
      CreateWindow(aura::client::WINDOW_TYPE_POPUP, rect));
  std::unique_ptr<aura::Window> w6(
      CreateWindow(aura::client::WINDOW_TYPE_CONTROL, rect));
  std::unique_ptr<aura::Window> w7(
      CreateWindow(aura::client::WINDOW_TYPE_MENU, rect));
  std::unique_ptr<aura::Window> w8(
      CreateWindow(aura::client::WINDOW_TYPE_TOOLTIP, rect));
  EXPECT_TRUE(wm::GetWindowState(w1.get())->IsMaximized());
  EXPECT_TRUE(wm::GetWindowState(w2.get())->IsMaximized());
  EXPECT_EQ(3, manager->GetNumberOfManagedWindows());
  EXPECT_FALSE(wm::GetWindowState(w3.get())->IsMaximized());

  // Make sure that the position of the unresizable window is in the middle of
  // the screen.
  gfx::Size work_area_size =
      screen_util::GetDisplayWorkAreaBoundsInParent(w3.get()).size();
  gfx::Point center =
      gfx::Point((work_area_size.width() - rect3.size().width()) / 2,
                 (work_area_size.height() - rect3.size().height()) / 2);
  gfx::Rect centered_window_bounds = gfx::Rect(center, rect3.size());
  EXPECT_EQ(centered_window_bounds.ToString(), w3->bounds().ToString());

  // All other windows should not have been touched.
  EXPECT_FALSE(wm::GetWindowState(w4.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w5.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w6.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w7.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w8.get())->IsMaximized());
  EXPECT_EQ(rect.ToString(), w4->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w5->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w6->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w7->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w8->bounds().ToString());

  // After the tablet mode was disabled all windows fall back into the mode
  // they were created for.
  DestroyTabletModeWindowManager();
  EXPECT_FALSE(wm::GetWindowState(w1.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w2.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w3.get())->IsMaximized());
  EXPECT_EQ(rect1.ToString(), w1->bounds().ToString());
  EXPECT_EQ(rect2.ToString(), w2->bounds().ToString());
  EXPECT_EQ(rect3.ToString(), w3->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w4->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w5->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w6->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w7->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w8->bounds().ToString());
}

// Test that a window which got created while the tablet mode window manager
// is active gets restored to a usable (non tiny) size upon switching back.
TEST_F(TabletModeWindowManagerTest,
       CreateWindowInMaximizedModeRestoresToUsefulSize) {
  TabletModeWindowManager* manager = CreateTabletModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(0, manager->GetNumberOfManagedWindows());

  // We pass in an empty rectangle to simulate a window creation with no
  // particular size.
  gfx::Rect empty_rect(0, 0, 0, 0);
  std::unique_ptr<aura::Window> window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, empty_rect));
  EXPECT_TRUE(wm::GetWindowState(window.get())->IsMaximized());
  EXPECT_NE(empty_rect.ToString(), window->bounds().ToString());
  gfx::Rect maximized_size = window->bounds();

  // Destroy the tablet mode and check that the resulting size of the window
  // is remaining as it is (but not maximized).
  DestroyTabletModeWindowManager();

  EXPECT_FALSE(wm::GetWindowState(window.get())->IsMaximized());
  EXPECT_EQ(maximized_size.ToString(), window->bounds().ToString());
}

// Test that non-maximizable windows get properly handled when created in
// tablet mode.
TEST_F(TabletModeWindowManagerTest, CreateNonMaximizableButResizableWindows) {
  // Create the manager and make sure that all qualifying windows were detected
  // and changed.
  TabletModeWindowManager* manager = CreateTabletModeWindowManager();
  ASSERT_TRUE(manager);

  gfx::Rect rect(10, 10, 200, 50);
  gfx::Size max_size(300, 200);
  gfx::Size empty_size;
  std::unique_ptr<aura::Window> unlimited_window(CreateNonMaximizableWindow(
      aura::client::WINDOW_TYPE_NORMAL, rect, empty_size));
  std::unique_ptr<aura::Window> limited_window(CreateNonMaximizableWindow(
      aura::client::WINDOW_TYPE_NORMAL, rect, max_size));
  std::unique_ptr<aura::Window> fixed_window(
      CreateFixedSizeNonMaximizableWindow(aura::client::WINDOW_TYPE_NORMAL,
                                          rect));

  gfx::Size workspace_size =
      screen_util::GetMaximizedWindowBoundsInParent(unlimited_window.get())
          .size();

  // All windows should be sized now as big as possible and be centered.
  EXPECT_EQ(3, manager->GetNumberOfManagedWindows());
  // The unlimited window should have the size of the workspace / parent window.
  EXPECT_FALSE(wm::GetWindowState(unlimited_window.get())->IsMaximized());
  EXPECT_EQ("0,0", unlimited_window->bounds().origin().ToString());
  EXPECT_EQ(workspace_size.ToString(),
            unlimited_window->bounds().size().ToString());
  // The limited window should have the size of the upper possible bounds.
  EXPECT_FALSE(wm::GetWindowState(limited_window.get())->IsMaximized());
  EXPECT_NE(rect.origin().ToString(),
            limited_window->bounds().origin().ToString());
  EXPECT_EQ(max_size.ToString(), limited_window->bounds().size().ToString());
  // The fixed size window should have the size of the original window.
  EXPECT_FALSE(wm::GetWindowState(fixed_window.get())->IsMaximized());
  EXPECT_NE(rect.origin().ToString(),
            fixed_window->bounds().origin().ToString());
  EXPECT_EQ(rect.size().ToString(), fixed_window->bounds().size().ToString());

  // Destroy the manager again and check that the windows return to their
  // creation state.
  DestroyTabletModeWindowManager();

  EXPECT_FALSE(wm::GetWindowState(unlimited_window.get())->IsMaximized());
  EXPECT_EQ(rect.ToString(), unlimited_window->bounds().ToString());
  EXPECT_FALSE(wm::GetWindowState(limited_window.get())->IsMaximized());
  EXPECT_EQ(rect.ToString(), limited_window->bounds().ToString());
  EXPECT_FALSE(wm::GetWindowState(fixed_window.get())->IsMaximized());
  EXPECT_EQ(rect.ToString(), fixed_window->bounds().ToString());
}

// Create a string which consists of the bounds and the state for comparison.
std::string GetPlacementString(const gfx::Rect& bounds,
                               ui::WindowShowState state) {
  return bounds.ToString() + base::StringPrintf(" %d", state);
}

// Retrieves the window's restore state override - if any - and returns it as a
// string.
std::string GetPlacementOverride(aura::Window* window) {
  gfx::Rect* bounds = window->GetProperty(kRestoreBoundsOverrideKey);
  if (bounds) {
    gfx::Rect restore_bounds = *bounds;
    ui::WindowShowState restore_state = ToWindowShowState(
        window->GetProperty(kRestoreWindowStateTypeOverrideKey));
    return GetPlacementString(restore_bounds, restore_state);
  }
  return std::string();
}

// Test that the restore state will be kept at its original value for
// session restoration purposes.
TEST_F(TabletModeWindowManagerTest, TestRestoreIntegrety) {
  gfx::Rect bounds(10, 10, 200, 50);
  std::unique_ptr<aura::Window> normal_window(CreateWindowWithWidget(bounds));
  std::unique_ptr<aura::Window> maximized_window(
      CreateWindowWithWidget(bounds));
  wm::GetWindowState(maximized_window.get())->Maximize();

  EXPECT_EQ(std::string(), GetPlacementOverride(normal_window.get()));
  EXPECT_EQ(std::string(), GetPlacementOverride(maximized_window.get()));

  TabletModeWindowManager* manager = CreateTabletModeWindowManager();
  ASSERT_TRUE(manager);

  // With the maximization the override states should be returned in its
  // pre-maximized state.
  EXPECT_EQ(GetPlacementString(bounds, ui::SHOW_STATE_NORMAL),
            GetPlacementOverride(normal_window.get()));
  EXPECT_EQ(GetPlacementString(bounds, ui::SHOW_STATE_MAXIMIZED),
            GetPlacementOverride(maximized_window.get()));

  // Changing a window's state now does not change the returned result.
  wm::GetWindowState(maximized_window.get())->Minimize();
  EXPECT_EQ(GetPlacementString(bounds, ui::SHOW_STATE_MAXIMIZED),
            GetPlacementOverride(maximized_window.get()));

  // Destroy the manager again and check that the overrides get reset.
  DestroyTabletModeWindowManager();
  EXPECT_EQ(std::string(), GetPlacementOverride(normal_window.get()));
  EXPECT_EQ(std::string(), GetPlacementOverride(maximized_window.get()));

  // Changing a window's state now does not bring the overrides back.
  wm::GetWindowState(maximized_window.get())->Restore();
  gfx::Rect new_bounds(10, 10, 200, 50);
  maximized_window->SetBounds(new_bounds);

  EXPECT_EQ(std::string(), GetPlacementOverride(maximized_window.get()));
}

// Test that windows which got created before the maximizer was created can be
// destroyed while the maximizer is still running.
TEST_F(TabletModeWindowManagerTest, PreCreateWindowsDeleteWhileActive) {
  TabletModeWindowManager* manager = NULL;
  {
    // Bounds for windows we know can be controlled.
    gfx::Rect rect1(10, 10, 200, 50);
    gfx::Rect rect2(10, 60, 200, 50);
    gfx::Rect rect3(20, 140, 100, 100);
    // Bounds for anything else.
    std::unique_ptr<aura::Window> w1(
        CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect1));
    std::unique_ptr<aura::Window> w2(
        CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect2));
    std::unique_ptr<aura::Window> w3(CreateFixedSizeNonMaximizableWindow(
        aura::client::WINDOW_TYPE_NORMAL, rect3));

    // Create the manager and make sure that all qualifying windows were
    // detected and changed.
    manager = CreateTabletModeWindowManager();
    ASSERT_TRUE(manager);
    EXPECT_EQ(3, manager->GetNumberOfManagedWindows());
  }
  EXPECT_EQ(0, manager->GetNumberOfManagedWindows());
  DestroyTabletModeWindowManager();
}

// Test that windows which got created while the maximizer was running can get
// destroyed before the maximizer gets destroyed.
TEST_F(TabletModeWindowManagerTest, CreateWindowsAndDeleteWhileActive) {
  TabletModeWindowManager* manager = CreateTabletModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(0, manager->GetNumberOfManagedWindows());
  {
    // Bounds for windows we know can be controlled.
    gfx::Rect rect1(10, 10, 200, 50);
    gfx::Rect rect2(10, 60, 200, 50);
    gfx::Rect rect3(20, 140, 100, 100);
    std::unique_ptr<aura::Window> w1(
        CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect1));
    std::unique_ptr<aura::Window> w2(
        CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect2));
    std::unique_ptr<aura::Window> w3(CreateFixedSizeNonMaximizableWindow(
        aura::client::WINDOW_TYPE_NORMAL, rect3));
    // Check that the windows got automatically maximized as well.
    EXPECT_EQ(3, manager->GetNumberOfManagedWindows());
    EXPECT_TRUE(wm::GetWindowState(w1.get())->IsMaximized());
    EXPECT_TRUE(wm::GetWindowState(w2.get())->IsMaximized());
    EXPECT_FALSE(wm::GetWindowState(w3.get())->IsMaximized());
  }
  EXPECT_EQ(0, manager->GetNumberOfManagedWindows());
  DestroyTabletModeWindowManager();
}

// Test that windows which were maximized stay maximized.
TEST_F(TabletModeWindowManagerTest, MaximizedShouldRemainMaximized) {
  // Bounds for windows we know can be controlled.
  gfx::Rect rect(10, 10, 200, 50);
  std::unique_ptr<aura::Window> window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::GetWindowState(window.get())->Maximize();

  // Create the manager and make sure that the window gets detected.
  TabletModeWindowManager* manager = CreateTabletModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(1, manager->GetNumberOfManagedWindows());
  EXPECT_TRUE(wm::GetWindowState(window.get())->IsMaximized());

  // Destroy the manager again and check that the window will remain maximized.
  DestroyTabletModeWindowManager();
  EXPECT_TRUE(wm::GetWindowState(window.get())->IsMaximized());
  wm::GetWindowState(window.get())->Restore();
  EXPECT_EQ(rect.ToString(), window->bounds().ToString());
}

// Test that minimized windows do neither get maximized nor restored upon
// entering tablet mode and get restored to their previous state after
// leaving.
TEST_F(TabletModeWindowManagerTest, MinimizedWindowBehavior) {
  // Bounds for windows we know can be controlled.
  gfx::Rect rect(10, 10, 200, 50);
  std::unique_ptr<aura::Window> initially_minimized_window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  std::unique_ptr<aura::Window> initially_normal_window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  std::unique_ptr<aura::Window> initially_maximized_window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::GetWindowState(initially_minimized_window.get())->Minimize();
  wm::GetWindowState(initially_maximized_window.get())->Maximize();

  // Create the manager and make sure that the window gets detected.
  TabletModeWindowManager* manager = CreateTabletModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(3, manager->GetNumberOfManagedWindows());
  EXPECT_TRUE(
      wm::GetWindowState(initially_minimized_window.get())->IsMinimized());
  EXPECT_TRUE(wm::GetWindowState(initially_normal_window.get())->IsMaximized());
  EXPECT_TRUE(
      wm::GetWindowState(initially_maximized_window.get())->IsMaximized());
  // Now minimize the second window to check that upon leaving the window
  // will get restored to its minimized state.
  wm::GetWindowState(initially_normal_window.get())->Minimize();
  wm::GetWindowState(initially_maximized_window.get())->Minimize();
  EXPECT_TRUE(
      wm::GetWindowState(initially_minimized_window.get())->IsMinimized());
  EXPECT_TRUE(wm::GetWindowState(initially_normal_window.get())->IsMinimized());
  EXPECT_TRUE(
      wm::GetWindowState(initially_maximized_window.get())->IsMinimized());

  // Destroy the manager again and check that the window will get minimized.
  DestroyTabletModeWindowManager();
  EXPECT_TRUE(
      wm::GetWindowState(initially_minimized_window.get())->IsMinimized());
  EXPECT_FALSE(
      wm::GetWindowState(initially_normal_window.get())->IsMinimized());
  EXPECT_TRUE(
      wm::GetWindowState(initially_maximized_window.get())->IsMaximized());
}

// Check that resizing the desktop does reposition unmaximizable, unresizable &
// managed windows.
TEST_F(TabletModeWindowManagerTest, DesktopSizeChangeMovesUnmaximizable) {
  UpdateDisplay("400x400");
  // This window will move because it does not fit the new bounds.
  gfx::Rect rect(20, 300, 100, 100);
  std::unique_ptr<aura::Window> window1(CreateFixedSizeNonMaximizableWindow(
      aura::client::WINDOW_TYPE_NORMAL, rect));
  EXPECT_EQ(rect.ToString(), window1->bounds().ToString());

  // This window will not move because it does fit the new bounds.
  gfx::Rect rect2(20, 140, 100, 100);
  std::unique_ptr<aura::Window> window2(CreateFixedSizeNonMaximizableWindow(
      aura::client::WINDOW_TYPE_NORMAL, rect2));

  // Turning on the manager will reposition (but not resize) the window.
  TabletModeWindowManager* manager = CreateTabletModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(2, manager->GetNumberOfManagedWindows());
  gfx::Rect moved_bounds(window1->bounds());
  EXPECT_NE(rect.origin().ToString(), moved_bounds.origin().ToString());
  EXPECT_EQ(rect.size().ToString(), moved_bounds.size().ToString());

  // Simulating a desktop resize should move the window again.
  UpdateDisplay("300x300");
  gfx::Rect new_moved_bounds(window1->bounds());
  EXPECT_NE(rect.origin().ToString(), new_moved_bounds.origin().ToString());
  EXPECT_EQ(rect.size().ToString(), new_moved_bounds.size().ToString());
  EXPECT_NE(moved_bounds.origin().ToString(), new_moved_bounds.ToString());

  // Turning off the mode should not restore to the initial coordinates since
  // the new resolution is smaller and the window was on the edge.
  DestroyTabletModeWindowManager();
  EXPECT_NE(rect.ToString(), window1->bounds().ToString());
  EXPECT_EQ(rect2.ToString(), window2->bounds().ToString());
}

// Check that windows return to original location if desktop size changes to
// something else and back while in tablet mode.
TEST_F(TabletModeWindowManagerTest, SizeChangeReturnWindowToOriginalPos) {
  gfx::Rect rect(20, 140, 100, 100);
  std::unique_ptr<aura::Window> window(CreateFixedSizeNonMaximizableWindow(
      aura::client::WINDOW_TYPE_NORMAL, rect));

  // Turning on the manager will reposition (but not resize) the window.
  TabletModeWindowManager* manager = CreateTabletModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(1, manager->GetNumberOfManagedWindows());
  gfx::Rect moved_bounds(window->bounds());
  EXPECT_NE(rect.origin().ToString(), moved_bounds.origin().ToString());
  EXPECT_EQ(rect.size().ToString(), moved_bounds.size().ToString());

  // Simulating a desktop resize should move the window again.
  ResizeDesktop(-10);
  gfx::Rect new_moved_bounds(window->bounds());
  EXPECT_NE(rect.origin().ToString(), new_moved_bounds.origin().ToString());
  EXPECT_EQ(rect.size().ToString(), new_moved_bounds.size().ToString());
  EXPECT_NE(moved_bounds.origin().ToString(), new_moved_bounds.ToString());

  // Then resize back to the original desktop size which should move windows
  // to their original location after leaving the tablet mode.
  ResizeDesktop(10);
  DestroyTabletModeWindowManager();
  EXPECT_EQ(rect.ToString(), window->bounds().ToString());
}

// Check that enabling of the tablet mode does not have an impact on the MRU
// order of windows.
TEST_F(TabletModeWindowManagerTest, ModeChangeKeepsMRUOrder) {
  gfx::Rect rect(20, 140, 100, 100);
  std::unique_ptr<aura::Window> w1(CreateFixedSizeNonMaximizableWindow(
      aura::client::WINDOW_TYPE_NORMAL, rect));
  std::unique_ptr<aura::Window> w2(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  std::unique_ptr<aura::Window> w3(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  std::unique_ptr<aura::Window> w4(CreateFixedSizeNonMaximizableWindow(
      aura::client::WINDOW_TYPE_NORMAL, rect));
  std::unique_ptr<aura::Window> w5(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));

  // The windows should be in the reverse order of creation in the MRU list.
  {
    aura::Window::Windows windows =
        Shell::Get()->mru_window_tracker()->BuildMruWindowList();

    EXPECT_EQ(w1.get(), windows[4]);
    EXPECT_EQ(w2.get(), windows[3]);
    EXPECT_EQ(w3.get(), windows[2]);
    EXPECT_EQ(w4.get(), windows[1]);
    EXPECT_EQ(w5.get(), windows[0]);
  }

  // Activating the window manager should keep the order.
  TabletModeWindowManager* manager = CreateTabletModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(5, manager->GetNumberOfManagedWindows());
  {
    aura::Window::Windows windows =
        Shell::Get()->mru_window_tracker()->BuildMruWindowList();
    // We do not test maximization here again since that was done already.
    EXPECT_EQ(w1.get(), windows[4]);
    EXPECT_EQ(w2.get(), windows[3]);
    EXPECT_EQ(w3.get(), windows[2]);
    EXPECT_EQ(w4.get(), windows[1]);
    EXPECT_EQ(w5.get(), windows[0]);
  }

  // Destroying should still keep the order.
  DestroyTabletModeWindowManager();
  {
    aura::Window::Windows windows =
        Shell::Get()->mru_window_tracker()->BuildMruWindowList();
    // We do not test maximization here again since that was done already.
    EXPECT_EQ(w1.get(), windows[4]);
    EXPECT_EQ(w2.get(), windows[3]);
    EXPECT_EQ(w3.get(), windows[2]);
    EXPECT_EQ(w4.get(), windows[1]);
    EXPECT_EQ(w5.get(), windows[0]);
  }
}

// Check that a restore state change does always restore to maximized.
TEST_F(TabletModeWindowManagerTest, IgnoreRestoreStateChages) {
  gfx::Rect rect(20, 140, 100, 100);
  std::unique_ptr<aura::Window> w1(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* window_state = wm::GetWindowState(w1.get());
  CreateTabletModeWindowManager();
  EXPECT_TRUE(window_state->IsMaximized());
  window_state->Minimize();
  EXPECT_TRUE(window_state->IsMinimized());
  window_state->Restore();
  EXPECT_TRUE(window_state->IsMaximized());
  window_state->Restore();
  EXPECT_TRUE(window_state->IsMaximized());
  DestroyTabletModeWindowManager();
}

// Check that minimize and restore do the right thing.
TEST_F(TabletModeWindowManagerTest, TestMinimize) {
  gfx::Rect rect(10, 10, 100, 100);
  std::unique_ptr<aura::Window> window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* window_state = wm::GetWindowState(window.get());
  EXPECT_EQ(rect.ToString(), window->bounds().ToString());
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  EXPECT_TRUE(window_state->IsMaximized());
  EXPECT_FALSE(window_state->IsMinimized());
  EXPECT_TRUE(window->IsVisible());

  window_state->Minimize();
  EXPECT_FALSE(window_state->IsMaximized());
  EXPECT_TRUE(window_state->IsMinimized());
  EXPECT_FALSE(window->IsVisible());

  window_state->Maximize();
  EXPECT_TRUE(window_state->IsMaximized());
  EXPECT_FALSE(window_state->IsMinimized());
  EXPECT_TRUE(window->IsVisible());

  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  EXPECT_FALSE(window_state->IsMaximized());
  EXPECT_FALSE(window_state->IsMinimized());
  EXPECT_TRUE(window->IsVisible());
}

// Tests that minimized window can restore to pre-minimized show state after
// entering and leaving tablet mode (https://crbug.com/783310).
TEST_F(TabletModeWindowManagerTest, MinimizedEnterAndLeaveTabletMode) {
  gfx::Rect rect(10, 10, 100, 100);
  std::unique_ptr<aura::Window> window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* window_state = wm::GetWindowState(window.get());
  window_state->Minimize();
  EXPECT_TRUE(window_state->IsMinimized());
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  EXPECT_TRUE(window_state->IsMinimized());
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  EXPECT_TRUE(window_state->IsMinimized());

  window_state->Unminimize();
  EXPECT_FALSE(window_state->IsMinimized());
  window_state->Minimize();
  EXPECT_TRUE(window_state->IsMinimized());
}

// Tests that pre-minimized window show state is persistent after entering and
// leaving tablet mode, that is not cleared in tablet mode.
TEST_F(TabletModeWindowManagerTest, PersistPreMinimizedShowState) {
  gfx::Rect rect(10, 10, 100, 100);
  std::unique_ptr<aura::Window> window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* window_state = wm::GetWindowState(window.get());
  window_state->Maximize();
  window_state->Minimize();
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED,
            window->GetProperty(aura::client::kPreMinimizedShowStateKey));

  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  window_state->Unminimize();
  // Check that pre-minimized window show state is not cleared due to
  // unminimizing in tablet mode.
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED,
            window->GetProperty(aura::client::kPreMinimizedShowStateKey));
  window_state->Minimize();
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED,
            window->GetProperty(aura::client::kPreMinimizedShowStateKey));

  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  window_state->Unminimize();
  EXPECT_TRUE(window_state->IsMaximized());
}

// Tests unminimizing in tablet mode and then existing tablet mode should have
// pre-minimized window show state.
TEST_F(TabletModeWindowManagerTest, UnminimizeInTabletMode) {
  // Tests restoring to maximized show state.
  gfx::Rect rect(10, 10, 100, 100);
  std::unique_ptr<aura::Window> window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* window_state = wm::GetWindowState(window.get());
  window_state->Maximize();
  window_state->Minimize();
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  window_state->Unminimize();
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  EXPECT_TRUE(window_state->IsMaximized());

  // Tests restoring to normal show state.
  window_state->Restore();
  EXPECT_EQ(gfx::Rect(10, 10, 100, 100), window->GetBoundsInScreen());
  window_state->Minimize();
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  window_state->Unminimize();
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  EXPECT_EQ(gfx::Rect(10, 10, 100, 100), window->GetBoundsInScreen());
}

// Check that a full screen window remains full screen upon entering maximize
// mode. Furthermore, checks that this window is not full screen upon exiting
// tablet mode if it was un-full-screened while in tablet mode.
TEST_F(TabletModeWindowManagerTest, KeepFullScreenModeOn) {
  gfx::Rect rect(20, 140, 100, 100);
  std::unique_ptr<aura::Window> w1(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* window_state = wm::GetWindowState(w1.get());

  Shelf* shelf = GetPrimaryShelf();

  // Allow the shelf to hide and set the pref.
  SetShelfAutoHideBehaviorPref(GetPrimaryDisplay().id(),
                               SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS);
  EXPECT_EQ(SHELF_AUTO_HIDE, shelf->GetVisibilityState());

  wm::WMEvent event(wm::WM_EVENT_TOGGLE_FULLSCREEN);
  window_state->OnWMEvent(&event);

  // With full screen, the shelf should get hidden.
  EXPECT_TRUE(window_state->IsFullscreen());
  EXPECT_EQ(SHELF_HIDDEN, shelf->GetVisibilityState());

  CreateTabletModeWindowManager();

  // The Full screen mode should continue to be on.
  EXPECT_TRUE(window_state->IsFullscreen());
  EXPECT_FALSE(window_state->IsMaximized());
  EXPECT_EQ(SHELF_HIDDEN, shelf->GetVisibilityState());

  // When exiting fullscreen, tablet mode should still be enabled, and the shelf
  // should be forced to visible for tablet mode.
  window_state->OnWMEvent(&event);
  EXPECT_FALSE(window_state->IsFullscreen());
  EXPECT_TRUE(window_state->IsMaximized());
  EXPECT_EQ(SHELF_VISIBLE, shelf->GetVisibilityState());

  // The shelf auto-hide preference should be restored when exiting tablet mode.
  DestroyTabletModeWindowManager();
  EXPECT_FALSE(window_state->IsFullscreen());
  EXPECT_TRUE(window_state->IsMaximized());
  EXPECT_EQ(SHELF_AUTO_HIDE, shelf->GetVisibilityState());
}

// Similar to the fullscreen mode, the pinned mode should be kept as well.
TEST_F(TabletModeWindowManagerTest, KeepPinnedModeOn_Case1) {
  // Scenario: in the default state, pin a window, enter to the tablet mode,
  // then unpin.
  gfx::Rect rect(20, 140, 100, 100);
  std::unique_ptr<aura::Window> w1(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* window_state = wm::GetWindowState(w1.get());
  EXPECT_FALSE(window_state->IsPinned());

  // Pin the window.
  {
    wm::WMEvent event(wm::WM_EVENT_PIN);
    window_state->OnWMEvent(&event);
  }
  EXPECT_TRUE(window_state->IsPinned());

  // Enter tablet mode. The pinned mode should continue to be on.
  CreateTabletModeWindowManager();
  EXPECT_TRUE(window_state->IsPinned());

  // Then unpin.
  window_state->Restore();
  EXPECT_FALSE(window_state->IsPinned());

  // Exit tablet mode. The window should not be back to the pinned mode.
  DestroyTabletModeWindowManager();
  EXPECT_FALSE(window_state->IsPinned());
}

TEST_F(TabletModeWindowManagerTest, KeepPinnedModeOn_Case2) {
  // Scenario: in the tablet mode, pin a window, exit tablet mode, then unpin.
  gfx::Rect rect(20, 140, 100, 100);
  std::unique_ptr<aura::Window> w1(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* window_state = wm::GetWindowState(w1.get());
  EXPECT_FALSE(window_state->IsPinned());

  // Enter tablet mode.
  CreateTabletModeWindowManager();
  EXPECT_FALSE(window_state->IsPinned());

  // Pin the window.
  {
    wm::WMEvent event(wm::WM_EVENT_PIN);
    window_state->OnWMEvent(&event);
  }
  EXPECT_TRUE(window_state->IsPinned());

  // Exit tablet mode. The pinned mode should continue to be on.
  DestroyTabletModeWindowManager();
  EXPECT_TRUE(window_state->IsPinned());

  // Then unpin.
  window_state->Restore();
  EXPECT_FALSE(window_state->IsPinned());

  // Enter tablet mode again for verification. The window should not be back to
  // the pinned mode.
  CreateTabletModeWindowManager();
  EXPECT_FALSE(window_state->IsPinned());

  // Exit tablet mode.
  DestroyTabletModeWindowManager();
  EXPECT_FALSE(window_state->IsPinned());
}

TEST_F(TabletModeWindowManagerTest, KeepPinnedModeOn_Case3) {
  // Scenario: in the default state, pin a window, enter to the tablet mode,
  // exit from the tablet mode, then unpin.
  gfx::Rect rect(20, 140, 100, 100);
  std::unique_ptr<aura::Window> w1(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* window_state = wm::GetWindowState(w1.get());
  EXPECT_FALSE(window_state->IsPinned());

  // Pin the window.
  {
    wm::WMEvent event(wm::WM_EVENT_PIN);
    window_state->OnWMEvent(&event);
  }
  EXPECT_TRUE(window_state->IsPinned());

  // Enter tablet mode. The pinned mode should continue to be on.
  CreateTabletModeWindowManager();
  EXPECT_TRUE(window_state->IsPinned());

  // Exit tablet mode. The pinned mode should continue to be on, too.
  DestroyTabletModeWindowManager();
  EXPECT_TRUE(window_state->IsPinned());

  // Then unpin.
  window_state->Restore();
  EXPECT_FALSE(window_state->IsPinned());

  // Enter tablet mode again for verification. The window should not be back to
  // the pinned mode.
  CreateTabletModeWindowManager();
  EXPECT_FALSE(window_state->IsPinned());

  // Exit tablet mode.
  DestroyTabletModeWindowManager();
}

TEST_F(TabletModeWindowManagerTest, KeepPinnedModeOn_Case4) {
  // Scenario: in tablet mode, pin a window, exit tablet mode, enter tablet mode
  // again, then unpin.
  gfx::Rect rect(20, 140, 100, 100);
  std::unique_ptr<aura::Window> w1(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* window_state = wm::GetWindowState(w1.get());
  EXPECT_FALSE(window_state->IsPinned());

  // Enter tablet mode.
  CreateTabletModeWindowManager();
  EXPECT_FALSE(window_state->IsPinned());

  // Pin the window.
  {
    wm::WMEvent event(wm::WM_EVENT_PIN);
    window_state->OnWMEvent(&event);
  }
  EXPECT_TRUE(window_state->IsPinned());

  // Exit tablet mode. The pinned mode should continue to be on.
  DestroyTabletModeWindowManager();
  EXPECT_TRUE(window_state->IsPinned());

  // Enter tablet mode again. The pinned mode should continue to be on, too.
  CreateTabletModeWindowManager();
  EXPECT_TRUE(window_state->IsPinned());

  // Then unpin.
  window_state->Restore();
  EXPECT_FALSE(window_state->IsPinned());

  // Exit tablet mode. The window should not be back to the pinned mode.
  DestroyTabletModeWindowManager();
  EXPECT_FALSE(window_state->IsPinned());
}

// Verifies that if a window is un-full-screened while in tablet mode,
// other changes to that window's state (such as minimizing it) are
// preserved upon exiting tablet mode.
TEST_F(TabletModeWindowManagerTest, MinimizePreservedAfterLeavingFullscreen) {
  gfx::Rect rect(20, 140, 100, 100);
  std::unique_ptr<aura::Window> w1(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* window_state = wm::GetWindowState(w1.get());

  Shelf* shelf = GetPrimaryShelf();

  // Allow the shelf to hide and enter full screen.
  shelf->SetAutoHideBehavior(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS);
  wm::WMEvent event(wm::WM_EVENT_TOGGLE_FULLSCREEN);
  window_state->OnWMEvent(&event);
  ASSERT_FALSE(window_state->IsMinimized());

  // Enter tablet mode, exit full screen, and then minimize the window.
  CreateTabletModeWindowManager();
  window_state->OnWMEvent(&event);
  window_state->Minimize();
  ASSERT_TRUE(window_state->IsMinimized());

  // The window should remain minimized when exiting tablet mode.
  DestroyTabletModeWindowManager();
  EXPECT_TRUE(window_state->IsMinimized());
}

// Tests that the auto-hide behavior is set to never auto-hide on tablet mode
// and gets reset based on pref after exiting tablet mode.
TEST_F(TabletModeWindowManagerTest, DisableAutoHideBehaviorOnTabletMode) {
  Shelf* shelf = GetPrimaryShelf();
  SetShelfAutoHideBehaviorPref(GetPrimaryDisplay().id(),
                               SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS);
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS, shelf->auto_hide_behavior());
  CreateTabletModeWindowManager();
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER, shelf->auto_hide_behavior());
  DestroyTabletModeWindowManager();
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS, shelf->auto_hide_behavior());
}

// Check that full screen mode can be turned on in tablet mode and remains
// upon coming back.
TEST_F(TabletModeWindowManagerTest, AllowFullScreenMode) {
  gfx::Rect rect(20, 140, 100, 100);
  std::unique_ptr<aura::Window> w1(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* window_state = wm::GetWindowState(w1.get());

  Shelf* shelf = GetPrimaryShelf();

  // Allow the shelf to hide and set the pref.
  SetShelfAutoHideBehaviorPref(GetPrimaryDisplay().id(),
                               SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS);

  EXPECT_FALSE(window_state->IsFullscreen());
  EXPECT_FALSE(window_state->IsMaximized());
  EXPECT_EQ(SHELF_AUTO_HIDE, shelf->GetVisibilityState());

  CreateTabletModeWindowManager();

  // Fullscreen should stay off, but the shelf is made visible in tablet mode.
  EXPECT_FALSE(window_state->IsFullscreen());
  EXPECT_TRUE(window_state->IsMaximized());
  EXPECT_EQ(SHELF_VISIBLE, shelf->GetVisibilityState());

  // After going into fullscreen mode, the shelf should be hidden.
  wm::WMEvent event(wm::WM_EVENT_TOGGLE_FULLSCREEN);
  window_state->OnWMEvent(&event);
  EXPECT_TRUE(window_state->IsFullscreen());
  EXPECT_FALSE(window_state->IsMaximized());
  EXPECT_EQ(SHELF_HIDDEN, shelf->GetVisibilityState());

  // With the destruction of the manager we should remain in full screen.
  DestroyTabletModeWindowManager();
  EXPECT_TRUE(window_state->IsFullscreen());
  EXPECT_FALSE(window_state->IsMaximized());
  EXPECT_EQ(SHELF_HIDDEN, shelf->GetVisibilityState());
}

// Check that the full screen mode will stay active when the tablet mode is
// ended.
TEST_F(TabletModeWindowManagerTest,
       FullScreenModeRemainsWhenCreatedInMaximizedMode) {
  CreateTabletModeWindowManager();

  gfx::Rect rect(20, 140, 100, 100);
  std::unique_ptr<aura::Window> w1(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* window_state = wm::GetWindowState(w1.get());
  wm::WMEvent event_full_screen(wm::WM_EVENT_TOGGLE_FULLSCREEN);
  window_state->OnWMEvent(&event_full_screen);
  EXPECT_TRUE(window_state->IsFullscreen());

  // After the tablet mode manager is ended, full screen will remain.
  DestroyTabletModeWindowManager();
  EXPECT_TRUE(window_state->IsFullscreen());
}

// Check that the full screen mode will stay active throughout a maximzied mode
// session.
TEST_F(TabletModeWindowManagerTest,
       FullScreenModeRemainsThroughTabletModeSwitch) {
  gfx::Rect rect(20, 140, 100, 100);
  std::unique_ptr<aura::Window> w1(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* window_state = wm::GetWindowState(w1.get());
  wm::WMEvent event_full_screen(wm::WM_EVENT_TOGGLE_FULLSCREEN);
  window_state->OnWMEvent(&event_full_screen);
  EXPECT_TRUE(window_state->IsFullscreen());

  CreateTabletModeWindowManager();
  EXPECT_TRUE(window_state->IsFullscreen());
  DestroyTabletModeWindowManager();
  EXPECT_TRUE(window_state->IsFullscreen());
}

// Check that an empty window does not get restored to a tiny size.
TEST_F(TabletModeWindowManagerTest,
       CreateAndMaximizeInTabletModeShouldRetoreToGoodSizeGoingToDefault) {
  CreateTabletModeWindowManager();
  gfx::Rect rect;
  std::unique_ptr<aura::Window> w1(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  w1->Show();
  wm::WindowState* window_state = wm::GetWindowState(w1.get());
  EXPECT_TRUE(window_state->IsMaximized());

  // There is a calling order in which the restore bounds can get set to an
  // empty rectangle. We simulate this here.
  window_state->SetRestoreBoundsInScreen(rect);
  EXPECT_TRUE(window_state->GetRestoreBoundsInScreen().IsEmpty());

  // Setting the window to a new size will physically not change the window,
  // but the restore size should get updated so that a restore later on will
  // return to this size.
  gfx::Rect requested_bounds(10, 20, 50, 70);
  w1->SetBounds(requested_bounds);
  EXPECT_TRUE(window_state->IsMaximized());
  EXPECT_EQ(requested_bounds.ToString(),
            window_state->GetRestoreBoundsInScreen().ToString());

  DestroyTabletModeWindowManager();

  EXPECT_FALSE(window_state->IsMaximized());
  EXPECT_EQ(w1->bounds().ToString(), requested_bounds.ToString());
}

// Check that non maximizable windows cannot be dragged by the user.
TEST_F(TabletModeWindowManagerTest, TryToDesktopSizeDragUnmaximizable) {
  gfx::Rect rect(10, 10, 100, 100);
  std::unique_ptr<aura::Window> window(CreateFixedSizeNonMaximizableWindow(
      aura::client::WINDOW_TYPE_NORMAL, rect));
  EXPECT_EQ(rect.ToString(), window->bounds().ToString());

  // 1. Move the mouse over the caption and check that dragging the window does
  // change the location.
  ui::test::EventGenerator generator(Shell::GetPrimaryRootWindow());
  generator.MoveMouseTo(gfx::Point(rect.x() + 2, rect.y() + 2));
  generator.PressLeftButton();
  generator.MoveMouseBy(10, 5);
  RunAllPendingInMessageLoop();
  generator.ReleaseLeftButton();
  gfx::Point first_dragged_origin = window->bounds().origin();
  EXPECT_EQ(rect.x() + 10, first_dragged_origin.x());
  EXPECT_EQ(rect.y() + 5, first_dragged_origin.y());

  // 2. Check that turning on the manager will stop allowing the window from
  // dragging.
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  gfx::Rect center_bounds(window->bounds());
  EXPECT_NE(rect.origin().ToString(), center_bounds.origin().ToString());
  generator.MoveMouseTo(
      gfx::Point(center_bounds.x() + 1, center_bounds.y() + 1));
  generator.PressLeftButton();
  generator.MoveMouseBy(10, 5);
  RunAllPendingInMessageLoop();
  generator.ReleaseLeftButton();
  EXPECT_EQ(center_bounds.x(), window->bounds().x());
  EXPECT_EQ(center_bounds.y(), window->bounds().y());
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);

  // 3. Releasing the mazimize manager again will restore the window to its
  // previous bounds and
  generator.MoveMouseTo(
      gfx::Point(first_dragged_origin.x() + 1, first_dragged_origin.y() + 1));
  generator.PressLeftButton();
  generator.MoveMouseBy(10, 5);
  RunAllPendingInMessageLoop();
  generator.ReleaseLeftButton();
  EXPECT_EQ(first_dragged_origin.x() + 10, window->bounds().x());
  EXPECT_EQ(first_dragged_origin.y() + 5, window->bounds().y());
}

// Test that overview is exited before entering / exiting tablet mode so that
// the window changes made by TabletModeWindowManager do not conflict with
// those made in WindowOverview.
TEST_F(TabletModeWindowManagerTest, ExitsOverview) {
  // Bounds for windows we know can be controlled.
  gfx::Rect rect1(10, 10, 200, 50);
  gfx::Rect rect2(10, 60, 200, 50);
  std::unique_ptr<aura::Window> w1(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect1));
  std::unique_ptr<aura::Window> w2(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect2));

  WindowSelectorController* window_selector_controller =
      Shell::Get()->window_selector_controller();
  ASSERT_TRUE(window_selector_controller->ToggleOverview());
  ASSERT_TRUE(window_selector_controller->IsSelecting());
  TabletModeWindowManager* manager = CreateTabletModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_FALSE(window_selector_controller->IsSelecting());

  ASSERT_TRUE(window_selector_controller->ToggleOverview());
  ASSERT_TRUE(window_selector_controller->IsSelecting());
  // Destroy the manager again and check that the windows return to their
  // previous state.
  DestroyTabletModeWindowManager();
  EXPECT_FALSE(window_selector_controller->IsSelecting());
}

// Test that an edge swipe from the top will end full screen mode.
TEST_F(TabletModeWindowManagerTest, ExitFullScreenWithEdgeSwipeFromTop) {
  gfx::Rect rect(10, 10, 200, 50);
  std::unique_ptr<aura::Window> background_window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  std::unique_ptr<aura::Window> foreground_window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* background_window_state =
      wm::GetWindowState(background_window.get());
  wm::WindowState* foreground_window_state =
      wm::GetWindowState(foreground_window.get());
  wm::ActivateWindow(foreground_window.get());
  CreateTabletModeWindowManager();

  // Fullscreen both windows.
  wm::WMEvent event(wm::WM_EVENT_TOGGLE_FULLSCREEN);
  background_window_state->OnWMEvent(&event);
  foreground_window_state->OnWMEvent(&event);
  EXPECT_TRUE(background_window_state->IsFullscreen());
  EXPECT_TRUE(foreground_window_state->IsFullscreen());
  EXPECT_EQ(foreground_window.get(), wm::GetActiveWindow());

  // Do an edge swipe top into screen.
  ui::test::EventGenerator generator(Shell::GetPrimaryRootWindow());
  generator.GestureScrollSequence(gfx::Point(50, 0), gfx::Point(50, 100),
                                  base::TimeDelta::FromMilliseconds(20), 10);

  EXPECT_FALSE(foreground_window_state->IsFullscreen());
  EXPECT_TRUE(background_window_state->IsFullscreen());

  // Do a second edge swipe top into screen.
  generator.GestureScrollSequence(gfx::Point(50, 0), gfx::Point(50, 100),
                                  base::TimeDelta::FromMilliseconds(20), 10);

  EXPECT_FALSE(foreground_window_state->IsFullscreen());
  EXPECT_TRUE(background_window_state->IsFullscreen());

  DestroyTabletModeWindowManager();
}

// Test that an edge swipe from the bottom will end full screen mode.
TEST_F(TabletModeWindowManagerTest, ExitFullScreenWithEdgeSwipeFromBottom) {
  gfx::Rect rect(10, 10, 200, 50);
  std::unique_ptr<aura::Window> background_window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  std::unique_ptr<aura::Window> foreground_window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* background_window_state =
      wm::GetWindowState(background_window.get());
  wm::WindowState* foreground_window_state =
      wm::GetWindowState(foreground_window.get());
  wm::ActivateWindow(foreground_window.get());
  CreateTabletModeWindowManager();

  // Fullscreen both windows.
  wm::WMEvent event(wm::WM_EVENT_TOGGLE_FULLSCREEN);
  background_window_state->OnWMEvent(&event);
  foreground_window_state->OnWMEvent(&event);
  EXPECT_TRUE(background_window_state->IsFullscreen());
  EXPECT_TRUE(foreground_window_state->IsFullscreen());
  EXPECT_EQ(foreground_window.get(), wm::GetActiveWindow());

  // Do an edge swipe bottom into screen.
  ui::test::EventGenerator generator(Shell::GetPrimaryRootWindow());
  int y = Shell::GetPrimaryRootWindow()->bounds().bottom();
  generator.GestureScrollSequence(gfx::Point(50, y), gfx::Point(50, y - 100),
                                  base::TimeDelta::FromMilliseconds(20), 10);

  EXPECT_FALSE(foreground_window_state->IsFullscreen());
  EXPECT_TRUE(background_window_state->IsFullscreen());

  DestroyTabletModeWindowManager();
}

// Test that an edge touch press at the top will end full screen mode.
TEST_F(TabletModeWindowManagerTest, ExitFullScreenWithEdgeTouchAtTop) {
  gfx::Rect rect(10, 10, 200, 50);
  std::unique_ptr<aura::Window> background_window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  std::unique_ptr<aura::Window> foreground_window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* background_window_state =
      wm::GetWindowState(background_window.get());
  wm::WindowState* foreground_window_state =
      wm::GetWindowState(foreground_window.get());
  wm::ActivateWindow(foreground_window.get());
  CreateTabletModeWindowManager();

  // Fullscreen both windows.
  wm::WMEvent event(wm::WM_EVENT_TOGGLE_FULLSCREEN);
  background_window_state->OnWMEvent(&event);
  foreground_window_state->OnWMEvent(&event);
  EXPECT_TRUE(background_window_state->IsFullscreen());
  EXPECT_TRUE(foreground_window_state->IsFullscreen());
  EXPECT_EQ(foreground_window.get(), wm::GetActiveWindow());

  // Touch tap on the top edge.
  ui::test::EventGenerator generator(Shell::GetPrimaryRootWindow());
  generator.GestureTapAt(gfx::Point(100, 0));
  EXPECT_FALSE(foreground_window_state->IsFullscreen());
  EXPECT_TRUE(background_window_state->IsFullscreen());

  // Try the same again and see that nothing changes.
  generator.GestureTapAt(gfx::Point(100, 0));
  EXPECT_FALSE(foreground_window_state->IsFullscreen());
  EXPECT_TRUE(background_window_state->IsFullscreen());

  DestroyTabletModeWindowManager();
}

// Test that an edge touch press at the bottom will end full screen mode.
TEST_F(TabletModeWindowManagerTest, ExitFullScreenWithEdgeTouchAtBottom) {
  gfx::Rect rect(10, 10, 200, 50);
  std::unique_ptr<aura::Window> background_window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  std::unique_ptr<aura::Window> foreground_window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* background_window_state =
      wm::GetWindowState(background_window.get());
  wm::WindowState* foreground_window_state =
      wm::GetWindowState(foreground_window.get());
  wm::ActivateWindow(foreground_window.get());
  CreateTabletModeWindowManager();

  // Fullscreen both windows.
  wm::WMEvent event(wm::WM_EVENT_TOGGLE_FULLSCREEN);
  background_window_state->OnWMEvent(&event);
  foreground_window_state->OnWMEvent(&event);
  EXPECT_TRUE(background_window_state->IsFullscreen());
  EXPECT_TRUE(foreground_window_state->IsFullscreen());
  EXPECT_EQ(foreground_window.get(), wm::GetActiveWindow());

  // Touch tap on the bottom edge.
  ui::test::EventGenerator generator(Shell::GetPrimaryRootWindow());
  generator.GestureTapAt(
      gfx::Point(100, Shell::GetPrimaryRootWindow()->bounds().bottom() - 1));
  EXPECT_FALSE(foreground_window_state->IsFullscreen());
  EXPECT_TRUE(background_window_state->IsFullscreen());

  // Try the same again and see that nothing changes.
  generator.GestureTapAt(
      gfx::Point(100, Shell::GetPrimaryRootWindow()->bounds().bottom() - 1));
  EXPECT_FALSE(foreground_window_state->IsFullscreen());
  EXPECT_TRUE(background_window_state->IsFullscreen());

  DestroyTabletModeWindowManager();
}

// Test that an edge swipe from the top on an immersive mode window will not end
// full screen mode.
TEST_F(TabletModeWindowManagerTest, NoExitImmersiveModeWithEdgeSwipeFromTop) {
  std::unique_ptr<aura::Window> window(CreateWindow(
      aura::client::WINDOW_TYPE_NORMAL, gfx::Rect(10, 10, 200, 50)));
  wm::WindowState* window_state = wm::GetWindowState(window.get());
  wm::ActivateWindow(window.get());
  CreateTabletModeWindowManager();

  // Fullscreen the window.
  wm::WMEvent event(wm::WM_EVENT_TOGGLE_FULLSCREEN);
  window_state->OnWMEvent(&event);
  EXPECT_TRUE(window_state->IsFullscreen());
  EXPECT_FALSE(window_state->IsInImmersiveFullscreen());
  EXPECT_EQ(window.get(), wm::GetActiveWindow());

  window_state->SetInImmersiveFullscreen(true);

  // Do an edge swipe top into screen.
  ui::test::EventGenerator generator(Shell::GetPrimaryRootWindow());
  generator.GestureScrollSequence(gfx::Point(50, 0), gfx::Point(50, 100),
                                  base::TimeDelta::FromMilliseconds(20), 10);

  // It should have not exited full screen or immersive mode.
  EXPECT_TRUE(window_state->IsFullscreen());
  EXPECT_TRUE(window_state->IsInImmersiveFullscreen());

  DestroyTabletModeWindowManager();
}

// Test that an edge swipe from the bottom will not end immersive mode.
TEST_F(TabletModeWindowManagerTest,
       NoExitImmersiveModeWithEdgeSwipeFromBottom) {
  std::unique_ptr<aura::Window> window(CreateWindow(
      aura::client::WINDOW_TYPE_NORMAL, gfx::Rect(10, 10, 200, 50)));
  wm::WindowState* window_state = wm::GetWindowState(window.get());
  wm::ActivateWindow(window.get());
  CreateTabletModeWindowManager();

  // Fullscreen the window.
  wm::WMEvent event(wm::WM_EVENT_TOGGLE_FULLSCREEN);
  window_state->OnWMEvent(&event);
  EXPECT_TRUE(window_state->IsFullscreen());
  EXPECT_FALSE(window_state->IsInImmersiveFullscreen());
  EXPECT_EQ(window.get(), wm::GetActiveWindow());
  window_state->SetInImmersiveFullscreen(true);
  EXPECT_TRUE(window_state->IsInImmersiveFullscreen());

  // Do an edge swipe bottom into screen.
  ui::test::EventGenerator generator(Shell::GetPrimaryRootWindow());
  int y = Shell::GetPrimaryRootWindow()->bounds().bottom();
  generator.GestureScrollSequence(gfx::Point(50, y), gfx::Point(50, y - 100),
                                  base::TimeDelta::FromMilliseconds(20), 10);

  // The window should still be full screen and immersive.
  EXPECT_TRUE(window_state->IsFullscreen());
  EXPECT_TRUE(window_state->IsInImmersiveFullscreen());

  DestroyTabletModeWindowManager();
}

// Tests that windows with the always-on-top property are not managed by
// the TabletModeWindowManager while tablet mode is engaged (i.e.,
// they remain free-floating).
TEST_F(TabletModeWindowManagerTest, AlwaysOnTopWindows) {
  gfx::Rect rect1(10, 10, 200, 50);
  gfx::Rect rect2(20, 140, 100, 100);

  // Create two windows with the always-on-top property.
  std::unique_ptr<aura::Window> w1(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect1));
  std::unique_ptr<aura::Window> w2(CreateFixedSizeNonMaximizableWindow(
      aura::client::WINDOW_TYPE_NORMAL, rect2));
  w1->SetProperty(aura::client::kAlwaysOnTopKey, true);
  w2->SetProperty(aura::client::kAlwaysOnTopKey, true);
  EXPECT_FALSE(wm::GetWindowState(w1.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w2.get())->IsMaximized());
  EXPECT_EQ(rect1.ToString(), w1->bounds().ToString());
  EXPECT_EQ(rect2.ToString(), w2->bounds().ToString());

  // Enter tablet mode. Neither window should be managed because they have
  // the always-on-top property set, which means that none of their properties
  // should change.
  TabletModeWindowManager* manager = CreateTabletModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(0, manager->GetNumberOfManagedWindows());
  EXPECT_FALSE(wm::GetWindowState(w1.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w2.get())->IsMaximized());
  EXPECT_EQ(rect1.ToString(), w1->bounds().ToString());
  EXPECT_EQ(rect2.ToString(), w2->bounds().ToString());

  // Remove the always-on-top property from both windows while in maximize
  // mode. The windows should become managed, which means they should be
  // maximized/centered and no longer be draggable.
  w1->SetProperty(aura::client::kAlwaysOnTopKey, false);
  w2->SetProperty(aura::client::kAlwaysOnTopKey, false);
  EXPECT_EQ(2, manager->GetNumberOfManagedWindows());
  EXPECT_TRUE(wm::GetWindowState(w1.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w2.get())->IsMaximized());
  EXPECT_NE(rect1.origin().ToString(), w1->bounds().origin().ToString());
  EXPECT_NE(rect1.size().ToString(), w1->bounds().size().ToString());
  EXPECT_NE(rect2.origin().ToString(), w2->bounds().origin().ToString());
  EXPECT_EQ(rect2.size().ToString(), w2->bounds().size().ToString());

  // Applying the always-on-top property to both windows while in maximize
  // mode should cause both windows to return to their original size,
  // position, and state.
  w1->SetProperty(aura::client::kAlwaysOnTopKey, true);
  w2->SetProperty(aura::client::kAlwaysOnTopKey, true);
  EXPECT_EQ(0, manager->GetNumberOfManagedWindows());
  EXPECT_FALSE(wm::GetWindowState(w1.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w2.get())->IsMaximized());
  EXPECT_EQ(rect1.ToString(), w1->bounds().ToString());
  EXPECT_EQ(rect2.ToString(), w2->bounds().ToString());

  // The always-on-top windows should not change when leaving tablet mode.
  DestroyTabletModeWindowManager();
  EXPECT_FALSE(wm::GetWindowState(w1.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w2.get())->IsMaximized());
  EXPECT_EQ(rect1.ToString(), w1->bounds().ToString());
  EXPECT_EQ(rect2.ToString(), w2->bounds().ToString());
}

// Tests that windows that can control maximized bounds are not maximized
// and not tracked.
TEST_F(TabletModeWindowManagerTest, DontMaximizeClientManagedWindows) {
  gfx::Rect rect(10, 10, 200, 50);
  std::unique_ptr<aura::Window> window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));

  wm::GetWindowState(window.get())->set_allow_set_bounds_direct(true);

  TabletModeWindowManager* manager = CreateTabletModeWindowManager();
  EXPECT_FALSE(wm::GetWindowState(window.get())->IsMaximized());
  EXPECT_EQ(0, manager->GetNumberOfManagedWindows());
}

// Verify that if tablet mode is started in the lock screen, windows will still
// be maximized after leaving the lock screen.
TEST_F(TabletModeWindowManagerTest, CreateManagerInLockScreen) {
  gfx::Rect rect(10, 10, 200, 50);
  std::unique_ptr<aura::Window> window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  ASSERT_FALSE(wm::GetWindowState(window.get())->IsMaximized());

  // Create the tablet mode window manager while inside the lock screen.
  GetSessionControllerClient()->RequestLockScreen();
  CreateTabletModeWindowManager();
  GetSessionControllerClient()->UnlockScreen();

  EXPECT_TRUE(wm::GetWindowState(window.get())->IsMaximized());

  DestroyTabletModeWindowManager();
  EXPECT_FALSE(wm::GetWindowState(window.get())->IsMaximized());
}

namespace {

class TestObserver : public wm::WindowStateObserver {
 public:
  TestObserver() = default;
  ~TestObserver() override = default;

  // wm::WindowStateObserver:
  void OnPreWindowStateTypeChange(wm::WindowState* window_state,
                                  mojom::WindowStateType old_type) override {
    pre_count_++;
    last_old_state_ = old_type;
  }

  void OnPostWindowStateTypeChange(wm::WindowState* window_state,
                                   mojom::WindowStateType old_type) override {
    post_count_++;
    post_layer_visibility_ = window_state->window()->layer()->visible();
    EXPECT_EQ(last_old_state_, old_type);
  }

  int GetPreCountAndReset() {
    int r = pre_count_;
    pre_count_ = 0;
    return r;
  }

  int GetPostCountAndReset() {
    int r = post_count_;
    post_count_ = 0;
    return r;
  }

  bool GetPostLayerVisibilityAndReset() {
    bool r = post_layer_visibility_;
    post_layer_visibility_ = false;
    return r;
  }

  mojom::WindowStateType GetLastOldStateAndReset() {
    mojom::WindowStateType r = last_old_state_;
    last_old_state_ = mojom::WindowStateType::DEFAULT;
    return r;
  }

 private:
  int pre_count_ = 0;
  int post_count_ = 0;
  bool post_layer_visibility_ = false;
  mojom::WindowStateType last_old_state_ = mojom::WindowStateType::DEFAULT;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

}  // namespace

TEST_F(TabletModeWindowManagerTest, StateTypeChange) {
  TestObserver observer;
  gfx::Rect rect(10, 10, 200, 50);
  std::unique_ptr<aura::Window> window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));

  CreateTabletModeWindowManager();

  wm::WindowState* window_state = wm::GetWindowState(window.get());
  window_state->AddObserver(&observer);

  window->Show();
  EXPECT_TRUE(window_state->IsMaximized());
  EXPECT_EQ(0, observer.GetPreCountAndReset());
  EXPECT_EQ(0, observer.GetPostCountAndReset());

  // Window is already in tablet mode.
  wm::WMEvent maximize_event(wm::WM_EVENT_MAXIMIZE);
  window_state->OnWMEvent(&maximize_event);
  EXPECT_EQ(0, observer.GetPreCountAndReset());
  EXPECT_EQ(0, observer.GetPostCountAndReset());

  wm::WMEvent fullscreen_event(wm::WM_EVENT_FULLSCREEN);
  window_state->OnWMEvent(&fullscreen_event);
  EXPECT_EQ(1, observer.GetPreCountAndReset());
  EXPECT_EQ(1, observer.GetPostCountAndReset());
  EXPECT_EQ(mojom::WindowStateType::MAXIMIZED,
            observer.GetLastOldStateAndReset());

  window_state->OnWMEvent(&maximize_event);
  EXPECT_EQ(1, observer.GetPreCountAndReset());
  EXPECT_EQ(1, observer.GetPostCountAndReset());
  EXPECT_EQ(mojom::WindowStateType::FULLSCREEN,
            observer.GetLastOldStateAndReset());

  wm::WMEvent minimize_event(wm::WM_EVENT_MINIMIZE);
  window_state->OnWMEvent(&minimize_event);
  EXPECT_EQ(1, observer.GetPreCountAndReset());
  EXPECT_EQ(1, observer.GetPostCountAndReset());
  EXPECT_EQ(mojom::WindowStateType::MAXIMIZED,
            observer.GetLastOldStateAndReset());

  wm::WMEvent restore_event(wm::WM_EVENT_NORMAL);
  window_state->OnWMEvent(&restore_event);
  EXPECT_EQ(1, observer.GetPreCountAndReset());
  EXPECT_EQ(1, observer.GetPostCountAndReset());
  EXPECT_EQ(mojom::WindowStateType::MINIMIZED,
            observer.GetLastOldStateAndReset());
  EXPECT_EQ(true, observer.GetPostLayerVisibilityAndReset());

  window_state->RemoveObserver(&observer);

  DestroyTabletModeWindowManager();
}

// Test that the restore state will be kept at its original value for
// session restoration purposes.
TEST_F(TabletModeWindowManagerTest, SetPropertyOnUnmanagedWindow) {
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  InitParams params(aura::client::WINDOW_TYPE_NORMAL);
  params.bounds = gfx::Rect(10, 10, 100, 100);
  params.show_on_creation = false;
  std::unique_ptr<aura::Window> window(CreateWindowInWatchedContainer(params));
  wm::GetWindowState(window.get())->set_allow_set_bounds_direct(true);
  window->SetProperty(aura::client::kAlwaysOnTopKey, true);
  window->Show();
}

// Test that there is no window resizer in tablet mode.
TEST_F(TabletModeWindowManagerTest, NoWindowResizerInTabletMode) {
  gfx::Rect rect(10, 10, 200, 200);
  std::unique_ptr<aura::Window> window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  std::unique_ptr<WindowResizer> resizer(CreateWindowResizer(
      window.get(), gfx::Point(), HTCAPTION, ::wm::WINDOW_MOVE_SOURCE_MOUSE));
  EXPECT_TRUE(resizer.get());
  resizer.reset();

  CreateTabletModeWindowManager();
  resizer = CreateWindowResizer(window.get(), gfx::Point(), HTCAPTION,
                                ::wm::WINDOW_MOVE_SOURCE_MOUSE);
  EXPECT_FALSE(resizer.get());
}

// Test that the minimized window bounds doesn't change until it's unminimized.
TEST_F(TabletModeWindowManagerTest, DontChangeBoundsForMinimizedWindow) {
  gfx::Rect rect(10, 10, 200, 50);
  std::unique_ptr<aura::Window> window(
      CreateWindow(aura::client::WINDOW_TYPE_NORMAL, rect));
  wm::WindowState* window_state = wm::GetWindowState(window.get());
  window_state->Minimize();
  EXPECT_TRUE(window_state->IsMinimized());

  TabletModeWindowManager* manager = CreateTabletModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(1, manager->GetNumberOfManagedWindows());
  EXPECT_TRUE(window_state->IsMinimized());
  EXPECT_EQ(window->bounds(), rect);

  WindowSelectorController* window_selector_controller =
      Shell::Get()->window_selector_controller();
  window_selector_controller->ToggleOverview();
  EXPECT_EQ(window->bounds(), rect);

  // Exit overview mode will update all windows' bounds. However, if the window
  // is minimized, the bounds will not be updated.
  window_selector_controller->ToggleOverview();
  EXPECT_EQ(window->bounds(), rect);
}

}  // namespace ash
