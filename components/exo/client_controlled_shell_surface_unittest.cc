// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/client_controlled_shell_surface.h"

#include "ash/display/screen_orientation_controller.h"
#include "ash/frame/caption_buttons/caption_button_model.h"
#include "ash/frame/caption_buttons/frame_caption_button_container_view.h"
#include "ash/frame/custom_frame_view_ash.h"
#include "ash/frame/header_view.h"
#include "ash/frame/wide_frame_view.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/public/interfaces/window_pin_type.mojom.h"
#include "ash/shell.h"
#include "ash/shell_test_api.h"
#include "ash/system/tray/system_tray.h"
#include "ash/wm/splitview/split_view_controller.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "ash/wm/window_positioning_utils.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "ash/wm/wm_event.h"
#include "ash/wm/workspace_controller_test_api.h"
#include "base/strings/utf_string_conversions.h"
#include "components/exo/buffer.h"
#include "components/exo/display.h"
#include "components/exo/pointer.h"
#include "components/exo/sub_surface.h"
#include "components/exo/surface.h"
#include "components/exo/test/exo_test_base.h"
#include "components/exo/test/exo_test_helper.h"
#include "components/exo/wm_helper.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tree_host.h"
#include "ui/compositor_extra/shadow.h"
#include "ui/display/display.h"
#include "ui/display/test/display_manager_test_api.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event_targeter.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/shadow_controller.h"
#include "ui/wm/core/shadow_types.h"

namespace exo {
namespace {
using ClientControlledShellSurfaceTest = test::ExoTestBase;

bool HasBackdrop() {
  ash::WorkspaceController* wc =
      ash::ShellTestApi(ash::Shell::Get()).workspace_controller();
  return !!ash::WorkspaceControllerTestApi(wc).GetBackdropWindow();
}

bool IsWidgetPinned(views::Widget* widget) {
  ash::mojom::WindowPinType type =
      widget->GetNativeWindow()->GetProperty(ash::kWindowPinTypeKey);
  return type == ash::mojom::WindowPinType::PINNED ||
         type == ash::mojom::WindowPinType::TRUSTED_PINNED;
}

int GetShadowElevation(aura::Window* window) {
  return window->GetProperty(wm::kShadowElevationKey);
}

void EnableTabletMode(bool enable) {
  ash::Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(
      enable);
}

}  // namespace

TEST_F(ClientControlledShellSurfaceTest, SetPinned) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));

  std::unique_ptr<Surface> surface(new Surface);
  surface->Attach(buffer.get());
  surface->Commit();

  auto shell_surface(
      exo_test_helper()->CreateClientControlledShellSurface(surface.get()));

  shell_surface->SetPinned(ash::mojom::WindowPinType::TRUSTED_PINNED);
  EXPECT_TRUE(IsWidgetPinned(shell_surface->GetWidget()));

  shell_surface->SetPinned(ash::mojom::WindowPinType::NONE);
  EXPECT_FALSE(IsWidgetPinned(shell_surface->GetWidget()));

  shell_surface->SetPinned(ash::mojom::WindowPinType::PINNED);
  EXPECT_TRUE(IsWidgetPinned(shell_surface->GetWidget()));

  shell_surface->SetPinned(ash::mojom::WindowPinType::NONE);
  EXPECT_FALSE(IsWidgetPinned(shell_surface->GetWidget()));
}

TEST_F(ClientControlledShellSurfaceTest, SetSystemUiVisibility) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface =
      exo_test_helper()->CreateClientControlledShellSurface(surface.get());
  surface->Attach(buffer.get());
  surface->Commit();

  shell_surface->SetSystemUiVisibility(true);
  EXPECT_TRUE(
      ash::wm::GetWindowState(shell_surface->GetWidget()->GetNativeWindow())
          ->autohide_shelf_when_maximized_or_fullscreen());

  shell_surface->SetSystemUiVisibility(false);
  EXPECT_FALSE(
      ash::wm::GetWindowState(shell_surface->GetWidget()->GetNativeWindow())
          ->autohide_shelf_when_maximized_or_fullscreen());
}

TEST_F(ClientControlledShellSurfaceTest, SetTopInset) {
  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface =
      exo_test_helper()->CreateClientControlledShellSurface(surface.get());

  surface->Attach(buffer.get());
  surface->Commit();

  aura::Window* window = shell_surface->GetWidget()->GetNativeWindow();
  ASSERT_TRUE(window);
  EXPECT_EQ(0, window->GetProperty(aura::client::kTopViewInset));
  int top_inset_height = 20;
  shell_surface->SetTopInset(top_inset_height);
  surface->Commit();
  EXPECT_EQ(top_inset_height, window->GetProperty(aura::client::kTopViewInset));
}

TEST_F(ClientControlledShellSurfaceTest, ModalWindowDefaultActive) {
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface =
      exo_test_helper()->CreateClientControlledShellSurface(surface.get(),
                                                            /*is_modal=*/true);

  gfx::Size desktop_size(640, 480);
  std::unique_ptr<Buffer> desktop_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(desktop_size)));
  surface->Attach(desktop_buffer.get());
  surface->SetInputRegion(gfx::Rect(10, 10, 100, 100));
  ASSERT_FALSE(shell_surface->GetWidget());
  shell_surface->SetSystemModal(true);
  surface->Commit();

  EXPECT_TRUE(ash::Shell::IsSystemModalWindowOpen());
  EXPECT_TRUE(shell_surface->GetWidget()->IsActive());
}

TEST_F(ClientControlledShellSurfaceTest, UpdateModalWindow) {
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface = exo_test_helper()->CreateClientControlledShellSurface(
      surface.get(), /*is_modal=*/true);
  gfx::Size desktop_size(640, 480);
  std::unique_ptr<Buffer> desktop_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(desktop_size)));
  surface->Attach(desktop_buffer.get());
  surface->SetInputRegion(cc::Region());
  surface->Commit();

  EXPECT_FALSE(ash::Shell::IsSystemModalWindowOpen());
  EXPECT_FALSE(shell_surface->GetWidget()->IsActive());

  // Creating a surface without input region should not make it modal.
  std::unique_ptr<Display> display(new Display);
  std::unique_ptr<Surface> child = display->CreateSurface();
  gfx::Size buffer_size(128, 128);
  std::unique_ptr<Buffer> child_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  child->Attach(child_buffer.get());
  std::unique_ptr<SubSurface> sub_surface(
      display->CreateSubSurface(child.get(), surface.get()));
  surface->SetSubSurfacePosition(child.get(), gfx::Point(10, 10));
  child->Commit();
  surface->Commit();
  EXPECT_FALSE(ash::Shell::IsSystemModalWindowOpen());
  EXPECT_FALSE(shell_surface->GetWidget()->IsActive());

  // Making the surface opaque shouldn't make it modal either.
  child->SetBlendMode(SkBlendMode::kSrc);
  child->Commit();
  surface->Commit();
  EXPECT_FALSE(ash::Shell::IsSystemModalWindowOpen());
  EXPECT_FALSE(shell_surface->GetWidget()->IsActive());

  // Setting input regions won't make it modal either.
  surface->SetInputRegion(gfx::Rect(10, 10, 100, 100));
  surface->Commit();
  EXPECT_FALSE(ash::Shell::IsSystemModalWindowOpen());
  EXPECT_FALSE(shell_surface->GetWidget()->IsActive());

  // Only SetSystemModal changes modality.
  shell_surface->SetSystemModal(true);

  EXPECT_TRUE(ash::Shell::IsSystemModalWindowOpen());
  EXPECT_TRUE(shell_surface->GetWidget()->IsActive());

  shell_surface->SetSystemModal(false);

  EXPECT_FALSE(ash::Shell::IsSystemModalWindowOpen());
  EXPECT_FALSE(shell_surface->GetWidget()->IsActive());

  // If the non modal system window was active,
  shell_surface->GetWidget()->Activate();
  EXPECT_TRUE(shell_surface->GetWidget()->IsActive());

  shell_surface->SetSystemModal(true);
  EXPECT_TRUE(ash::Shell::IsSystemModalWindowOpen());
  EXPECT_TRUE(shell_surface->GetWidget()->IsActive());

  shell_surface->SetSystemModal(false);
  EXPECT_FALSE(ash::Shell::IsSystemModalWindowOpen());
  EXPECT_TRUE(shell_surface->GetWidget()->IsActive());
}

TEST_F(ClientControlledShellSurfaceTest,
       ModalWindowSetSystemModalBeforeCommit) {
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface = exo_test_helper()->CreateClientControlledShellSurface(
      surface.get(), /*is_modal=*/true);
  gfx::Size desktop_size(640, 480);
  std::unique_ptr<Buffer> desktop_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(desktop_size)));
  surface->Attach(desktop_buffer.get());
  surface->SetInputRegion(cc::Region());

  // Set SetSystemModal before any commit happens. Widget is not created at
  // this time.
  EXPECT_FALSE(shell_surface->GetWidget());
  shell_surface->SetSystemModal(true);

  surface->Commit();

  // It is expected that modal window is shown.
  EXPECT_TRUE(shell_surface->GetWidget());
  EXPECT_TRUE(ash::Shell::IsSystemModalWindowOpen());

  // Now widget is created and setting modal state should be applied
  // immediately.
  shell_surface->SetSystemModal(false);
  EXPECT_FALSE(ash::Shell::IsSystemModalWindowOpen());
}

TEST_F(ClientControlledShellSurfaceTest, SurfaceShadow) {
  gfx::Size buffer_size(128, 128);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface =
      exo_test_helper()->CreateClientControlledShellSurface(surface.get());
  surface->Attach(buffer.get());
  surface->Commit();

  aura::Window* window = shell_surface->GetWidget()->GetNativeWindow();

  // 1) Initial state, no shadow (SurfaceFrameType is NONE);
  EXPECT_FALSE(wm::ShadowController::GetShadowForWindow(window));
  std::unique_ptr<Display> display(new Display);

  // 2) Just creating a sub surface won't create a shadow.
  std::unique_ptr<Surface> child = display->CreateSurface();
  std::unique_ptr<Buffer> child_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  child->Attach(child_buffer.get());
  std::unique_ptr<SubSurface> sub_surface(
      display->CreateSubSurface(child.get(), surface.get()));
  surface->Commit();

  EXPECT_FALSE(wm::ShadowController::GetShadowForWindow(window));

  // 3) Create a shadow.
  surface->SetFrame(SurfaceFrameType::SHADOW);
  shell_surface->SetShadowBounds(gfx::Rect(10, 10, 100, 100));
  surface->Commit();
  ui::Shadow* shadow = wm::ShadowController::GetShadowForWindow(window);
  ASSERT_TRUE(shadow);
  EXPECT_TRUE(shadow->layer()->visible());

  gfx::Rect before = shadow->layer()->bounds();

  // 4) Shadow bounds is independent of the sub surface.
  gfx::Size new_buffer_size(256, 256);
  std::unique_ptr<Buffer> new_child_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(new_buffer_size)));
  child->Attach(new_child_buffer.get());
  child->Commit();
  surface->Commit();

  EXPECT_EQ(before, shadow->layer()->bounds());

  // 4) Updating the widget's window bounds should not change the shadow bounds.
  // TODO(oshima): The following scenario only worked with Xdg/ShellSurface,
  // which never uses SetShadowBounds. This is broken with correct scenario, and
  // will be fixed when the bounds control is delegated to the client.
  //
  // window->SetBounds(gfx::Rect(10, 10, 100, 100));
  // EXPECT_EQ(before, shadow->layer()->bounds());

  // 5) This should disable shadow.
  shell_surface->SetShadowBounds(gfx::Rect());
  surface->Commit();

  EXPECT_EQ(wm::kShadowElevationNone, GetShadowElevation(window));
  EXPECT_FALSE(shadow->layer()->visible());

  // 6) This should enable non surface shadow again.
  shell_surface->SetShadowBounds(gfx::Rect(10, 10, 100, 100));
  surface->Commit();

  EXPECT_EQ(wm::kShadowElevationDefault, GetShadowElevation(window));
  EXPECT_TRUE(shadow->layer()->visible());
}

TEST_F(ClientControlledShellSurfaceTest, ShadowWithStateChange) {
  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface =
      exo_test_helper()->CreateClientControlledShellSurface(surface.get());

  // Postion the widget at 10,10 so that we get non zero offset.
  const gfx::Size content_size(100, 100);
  const gfx::Rect original_bounds(gfx::Point(10, 10), content_size);
  shell_surface->SetGeometry(original_bounds);
  surface->Attach(buffer.get());
  surface->SetFrame(SurfaceFrameType::SHADOW);
  surface->Commit();

  // Placing a shadow at screen origin will make the shadow's origin (-10, -10).
  const gfx::Rect shadow_bounds(content_size);

  // Expected shadow position/bounds in parent coordinates.
  const gfx::Point expected_shadow_origin(-10, -10);
  const gfx::Rect expected_shadow_bounds(expected_shadow_origin, content_size);

  views::Widget* widget = shell_surface->GetWidget();
  aura::Window* window = widget->GetNativeWindow();
  ui::Shadow* shadow = wm::ShadowController::GetShadowForWindow(window);

  shell_surface->SetShadowBounds(shadow_bounds);
  surface->Commit();
  EXPECT_EQ(wm::kShadowElevationDefault, GetShadowElevation(window));

  EXPECT_TRUE(shadow->layer()->visible());
  // Origin must be in sync.
  EXPECT_EQ(expected_shadow_origin, shadow->content_bounds().origin());

  const gfx::Rect work_area =
      display::Screen::GetScreen()->GetPrimaryDisplay().work_area();
  // Maximizing window hides the shadow.
  widget->Maximize();
  ASSERT_TRUE(widget->IsMaximized());
  EXPECT_FALSE(shadow->layer()->visible());

  shell_surface->SetShadowBounds(work_area);
  surface->Commit();
  EXPECT_FALSE(shadow->layer()->visible());

  // Restoring bounds will re-enable shadow. It's content size is set to work
  // area,/ thus not visible until new bounds is committed.
  widget->Restore();
  EXPECT_TRUE(shadow->layer()->visible());
  const gfx::Rect shadow_in_maximized(expected_shadow_origin, work_area.size());
  EXPECT_EQ(shadow_in_maximized, shadow->content_bounds());

  // The bounds is updated.
  shell_surface->SetShadowBounds(shadow_bounds);
  surface->Commit();
  EXPECT_EQ(expected_shadow_bounds, shadow->content_bounds());
}

TEST_F(ClientControlledShellSurfaceTest, ShadowWithTransform) {
  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface =
      exo_test_helper()->CreateClientControlledShellSurface(surface.get());

  // Postion the widget at 10,10 so that we get non zero offset.
  const gfx::Size content_size(100, 100);
  const gfx::Rect original_bounds(gfx::Point(10, 10), content_size);
  shell_surface->SetGeometry(original_bounds);
  surface->Attach(buffer.get());
  surface->SetFrame(SurfaceFrameType::SHADOW);
  surface->Commit();

  aura::Window* window = shell_surface->GetWidget()->GetNativeWindow();
  ui::Shadow* shadow = wm::ShadowController::GetShadowForWindow(window);

  // Placing a shadow at screen origin will make the shadow's origin (-10, -10).
  const gfx::Rect shadow_bounds(content_size);

  // Shadow bounds relative to its parent should not be affected by a transform.
  gfx::Transform transform;
  transform.Translate(50, 50);
  window->SetTransform(transform);
  shell_surface->SetShadowBounds(shadow_bounds);
  surface->Commit();
  EXPECT_TRUE(shadow->layer()->visible());
  EXPECT_EQ(gfx::Rect(-10, -10, 100, 100), shadow->content_bounds());
}

TEST_F(ClientControlledShellSurfaceTest, ShadowStartMaximized) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));

  std::unique_ptr<Surface> surface(new Surface);

  auto shell_surface =
      exo_test_helper()->CreateClientControlledShellSurface(surface.get());
  shell_surface->SetMaximized();
  surface->Attach(buffer.get());
  surface->SetFrame(SurfaceFrameType::SHADOW);
  surface->Commit();

  views::Widget* widget = shell_surface->GetWidget();
  aura::Window* window = widget->GetNativeWindow();

  // There is no shadow when started in maximized state.
  EXPECT_FALSE(wm::ShadowController::GetShadowForWindow(window));

  // Sending a shadow bounds in maximized state won't create a shadow.
  shell_surface->SetShadowBounds(gfx::Rect(10, 10, 100, 100));
  surface->Commit();
  EXPECT_FALSE(wm::ShadowController::GetShadowForWindow(window));

  // Restore the window and make sure the shadow is created, visible and
  // has the latest bounds.
  widget->Restore();
  ui::Shadow* shadow = wm::ShadowController::GetShadowForWindow(window);
  ASSERT_TRUE(shadow);
  EXPECT_TRUE(shadow->layer()->visible());
  EXPECT_EQ(gfx::Rect(10, 10, 100, 100), shadow->content_bounds());
}

TEST_F(ClientControlledShellSurfaceTest, Frame) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));

  std::unique_ptr<Surface> surface(new Surface);

  gfx::Rect client_bounds(20, 50, 300, 200);
  gfx::Rect fullscreen_bounds(0, 0, 800, 500);
  // The window bounds is the client bounds + frame size.
  gfx::Rect normal_window_bounds(20, 18, 300, 232);

  auto shell_surface =
      exo_test_helper()->CreateClientControlledShellSurface(surface.get());
  surface->Attach(buffer.get());
  shell_surface->SetGeometry(client_bounds);
  surface->SetFrame(SurfaceFrameType::NORMAL);
  surface->Commit();

  views::Widget* widget = shell_surface->GetWidget();
  ash::CustomFrameViewAsh* frame_view = static_cast<ash::CustomFrameViewAsh*>(
      widget->non_client_view()->frame_view());

  // Normal state.
  EXPECT_TRUE(frame_view->visible());
  EXPECT_EQ(normal_window_bounds, widget->GetWindowBoundsInScreen());
  EXPECT_EQ(client_bounds,
            frame_view->GetClientBoundsForWindowBounds(normal_window_bounds));

  // Maximized
  shell_surface->SetMaximized();
  shell_surface->SetGeometry(fullscreen_bounds);
  surface->Commit();

  EXPECT_TRUE(frame_view->visible());
  EXPECT_EQ(fullscreen_bounds, widget->GetWindowBoundsInScreen());
  EXPECT_EQ(
      gfx::Size(800, 468),
      frame_view->GetClientBoundsForWindowBounds(fullscreen_bounds).size());

  // AutoHide
  surface->SetFrame(SurfaceFrameType::AUTOHIDE);
  EXPECT_TRUE(frame_view->visible());
  EXPECT_EQ(fullscreen_bounds, widget->GetWindowBoundsInScreen());
  EXPECT_EQ(fullscreen_bounds,
            frame_view->GetClientBoundsForWindowBounds(fullscreen_bounds));

  // Fullscreen state.
  shell_surface->SetFullscreen(true);
  surface->Commit();
  EXPECT_TRUE(frame_view->visible());
  EXPECT_EQ(fullscreen_bounds, widget->GetWindowBoundsInScreen());
  EXPECT_EQ(fullscreen_bounds,
            frame_view->GetClientBoundsForWindowBounds(fullscreen_bounds));

  // Restore to normal state.
  shell_surface->SetRestored();
  shell_surface->SetGeometry(client_bounds);
  surface->SetFrame(SurfaceFrameType::NORMAL);
  surface->Commit();
  EXPECT_TRUE(frame_view->visible());
  EXPECT_EQ(normal_window_bounds, widget->GetWindowBoundsInScreen());
  EXPECT_EQ(client_bounds,
            frame_view->GetClientBoundsForWindowBounds(normal_window_bounds));

  // No frame. The all bounds are same as client bounds.
  shell_surface->SetRestored();
  shell_surface->SetGeometry(client_bounds);
  surface->SetFrame(SurfaceFrameType::NONE);
  surface->Commit();
  EXPECT_FALSE(frame_view->visible());
  EXPECT_EQ(client_bounds, widget->GetWindowBoundsInScreen());
  EXPECT_EQ(client_bounds,
            frame_view->GetClientBoundsForWindowBounds(client_bounds));
}

TEST_F(ClientControlledShellSurfaceTest, CompositorLockInRotation) {
  UpdateDisplay("800x600");
  const gfx::Size buffer_size(800, 600);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface =
      exo_test_helper()->CreateClientControlledShellSurface(surface.get());
  ash::Shell* shell = ash::Shell::Get();
  shell->tablet_mode_controller()->EnableTabletModeWindowManager(true);

  // Start in maximized.
  shell_surface->SetMaximized();
  surface->Attach(buffer.get());
  surface->Commit();

  gfx::Rect maximum_bounds =
      display::Screen::GetScreen()->GetPrimaryDisplay().bounds();
  shell_surface->SetGeometry(maximum_bounds);
  shell_surface->SetOrientation(Orientation::LANDSCAPE);
  surface->Commit();

  ui::Compositor* compositor =
      shell_surface->GetWidget()->GetNativeWindow()->layer()->GetCompositor();

  EXPECT_FALSE(compositor->IsLocked());

  UpdateDisplay("800x600/r");

  EXPECT_TRUE(compositor->IsLocked());

  shell_surface->SetOrientation(Orientation::PORTRAIT);
  surface->Commit();

  EXPECT_FALSE(compositor->IsLocked());
}

// If system tray is shown by click. It should be activated if user presses tab
// key while shell surface is active.
TEST_F(ClientControlledShellSurfaceTest, KeyboardNavigationWithSystemTray) {
  const gfx::Size buffer_size(800, 600);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface());
  auto shell_surface =
      exo_test_helper()->CreateClientControlledShellSurface(surface.get());

  surface->Attach(buffer.get());
  surface->Commit();

  EXPECT_TRUE(shell_surface->GetWidget()->IsActive());

  // Show system tray by perfoming a gesture tap at tray.
  ash::SystemTray* system_tray = GetPrimarySystemTray();
  ui::GestureEvent tap(0, 0, 0, base::TimeTicks(),
                       ui::GestureEventDetails(ui::ET_GESTURE_TAP));
  system_tray->PerformAction(tap);
  ASSERT_TRUE(system_tray->GetWidget());

  // Confirm that system tray is not active at this time.
  EXPECT_TRUE(shell_surface->GetWidget()->IsActive());
  EXPECT_FALSE(
      system_tray->GetSystemBubble()->bubble_view()->GetWidget()->IsActive());

  // Send tab key event.
  ui::test::EventGenerator& event_generator = GetEventGenerator();
  event_generator.PressKey(ui::VKEY_TAB, ui::EF_NONE);
  event_generator.ReleaseKey(ui::VKEY_TAB, ui::EF_NONE);

  // Confirm that system tray is activated.
  EXPECT_FALSE(shell_surface->GetWidget()->IsActive());
  EXPECT_TRUE(
      system_tray->GetSystemBubble()->bubble_view()->GetWidget()->IsActive());
}

TEST_F(ClientControlledShellSurfaceTest, Maximize) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface(
      exo_test_helper()->CreateClientControlledShellSurface(surface.get()));

  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_FALSE(HasBackdrop());
  shell_surface->SetMaximized();
  EXPECT_FALSE(HasBackdrop());
  surface->Commit();
  EXPECT_TRUE(HasBackdrop());
  EXPECT_TRUE(shell_surface->GetWidget()->IsMaximized());

  // Enable backdrop only if the shell surface doesn't cover the display.
  display::Display display = display::Screen::GetScreen()->GetPrimaryDisplay();
  shell_surface->SetGeometry(display.bounds());
  surface->Commit();
  EXPECT_FALSE(HasBackdrop());

  shell_surface->SetGeometry(gfx::Rect(0, 0, 100, display.bounds().height()));
  surface->Commit();
  EXPECT_TRUE(HasBackdrop());

  shell_surface->SetGeometry(gfx::Rect(0, 0, display.bounds().width(), 100));
  surface->Commit();
  EXPECT_TRUE(HasBackdrop());

  // Toggle maximize.
  ash::wm::WMEvent maximize_event(ash::wm::WM_EVENT_TOGGLE_MAXIMIZE);
  aura::Window* window = shell_surface->GetWidget()->GetNativeWindow();

  ash::wm::GetWindowState(window)->OnWMEvent(&maximize_event);
  EXPECT_FALSE(shell_surface->GetWidget()->IsMaximized());
  EXPECT_FALSE(HasBackdrop());

  ash::wm::GetWindowState(window)->OnWMEvent(&maximize_event);
  EXPECT_TRUE(shell_surface->GetWidget()->IsMaximized());
  EXPECT_TRUE(HasBackdrop());
}

TEST_F(ClientControlledShellSurfaceTest, Restore) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface(
      exo_test_helper()->CreateClientControlledShellSurface(surface.get()));

  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_FALSE(HasBackdrop());
  // Note: Remove contents to avoid issues with maximize animations in tests.
  shell_surface->SetMaximized();
  EXPECT_FALSE(HasBackdrop());
  surface->Commit();
  EXPECT_TRUE(HasBackdrop());

  shell_surface->SetRestored();
  EXPECT_TRUE(HasBackdrop());
  surface->Commit();
  EXPECT_FALSE(HasBackdrop());
}

TEST_F(ClientControlledShellSurfaceTest, SetFullscreen) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface(
      exo_test_helper()->CreateClientControlledShellSurface(surface.get()));

  shell_surface->SetFullscreen(true);
  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_TRUE(HasBackdrop());

  // Enable backdrop only if the shell surface doesn't cover the display.
  display::Display display = display::Screen::GetScreen()->GetPrimaryDisplay();
  shell_surface->SetGeometry(display.bounds());
  surface->Commit();
  EXPECT_FALSE(HasBackdrop());

  shell_surface->SetGeometry(gfx::Rect(0, 0, 100, display.bounds().height()));
  surface->Commit();
  EXPECT_TRUE(HasBackdrop());

  shell_surface->SetGeometry(gfx::Rect(0, 0, display.bounds().width(), 100));
  surface->Commit();
  EXPECT_TRUE(HasBackdrop());

  shell_surface->SetFullscreen(false);
  surface->Commit();
  EXPECT_FALSE(HasBackdrop());
  EXPECT_NE(CurrentContext()->bounds().ToString(),
            shell_surface->GetWidget()->GetWindowBoundsInScreen().ToString());
}

TEST_F(ClientControlledShellSurfaceTest, ToggleFullscreen) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface(
      exo_test_helper()->CreateClientControlledShellSurface(surface.get()));

  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_FALSE(HasBackdrop());

  shell_surface->SetMaximized();
  surface->Commit();
  EXPECT_TRUE(HasBackdrop());

  ash::wm::WMEvent event(ash::wm::WM_EVENT_TOGGLE_FULLSCREEN);
  aura::Window* window = shell_surface->GetWidget()->GetNativeWindow();

  // Enter fullscreen mode.
  ash::wm::GetWindowState(window)->OnWMEvent(&event);
  EXPECT_TRUE(HasBackdrop());

  // Leave fullscreen mode.
  ash::wm::GetWindowState(window)->OnWMEvent(&event);
  EXPECT_TRUE(HasBackdrop());
}

TEST_F(ClientControlledShellSurfaceTest,
       DefaultDeviceScaleFactorForcedScaleFactor) {
  double scale = 1.5;
  display::Display::SetForceDeviceScaleFactor(scale);

  int64_t display_id = display::Screen::GetScreen()->GetPrimaryDisplay().id();
  display::Display::SetInternalDisplayId(display_id);

  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface(
      exo_test_helper()->CreateClientControlledShellSurface(surface.get()));

  surface->Attach(buffer.get());
  surface->Commit();
  gfx::Transform transform;
  transform.Scale(1.0 / scale, 1.0 / scale);

  EXPECT_EQ(
      transform.ToString(),
      shell_surface->host_window()->layer()->GetTargetTransform().ToString());
}

TEST_F(ClientControlledShellSurfaceTest,
       DefaultDeviceScaleFactorFromDisplayManager) {
  int64_t display_id = display::Screen::GetScreen()->GetPrimaryDisplay().id();
  display::Display::SetInternalDisplayId(display_id);
  gfx::Size size(1920, 1080);

  display::DisplayManager* display_manager =
      ash::Shell::Get()->display_manager();

  double scale = 1.25;
  display::ManagedDisplayMode mode(size, 60.f, false /* overscan */,
                                   true /*native*/, 1.0, scale);
  mode.set_is_default(true);

  display::ManagedDisplayInfo::ManagedDisplayModeList mode_list;
  mode_list.push_back(mode);

  display::ManagedDisplayInfo native_display_info(display_id, "test", false);
  native_display_info.SetManagedDisplayModes(mode_list);

  native_display_info.SetBounds(gfx::Rect(size));
  native_display_info.set_device_scale_factor(scale);

  std::vector<display::ManagedDisplayInfo> display_info_list;
  display_info_list.push_back(native_display_info);

  display_manager->OnNativeDisplaysChanged(display_info_list);
  display_manager->UpdateInternalManagedDisplayModeListForTest();

  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface(
      exo_test_helper()->CreateClientControlledShellSurface(surface.get()));

  surface->Attach(buffer.get());
  surface->Commit();

  gfx::Transform transform;
  transform.Scale(1.0 / scale, 1.0 / scale);

  EXPECT_EQ(
      transform.ToString(),
      shell_surface->host_window()->layer()->GetTargetTransform().ToString());
}

TEST_F(ClientControlledShellSurfaceTest, MouseAndTouchTarget) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface(
      exo_test_helper()->CreateClientControlledShellSurface(surface.get()));

  const gfx::Rect original_bounds(0, 0, 256, 256);
  shell_surface->SetGeometry(original_bounds);
  shell_surface->set_client_controlled_move_resize(false);
  surface->Attach(buffer.get());
  surface->Commit();

  EXPECT_TRUE(shell_surface->CanResize());

  aura::Window* window = shell_surface->GetWidget()->GetNativeWindow();
  aura::Window* root = window->GetRootWindow();
  ui::EventTargeter* targeter =
      root->GetHost()->dispatcher()->GetDefaultEventTargeter();

  gfx::Point mouse_location(256 + 5, 150);

  ui::MouseEvent mouse(ui::ET_MOUSE_MOVED, mouse_location, mouse_location,
                       ui::EventTimeForNow(), ui::EF_NONE, ui::EF_NONE);
  EXPECT_EQ(window, targeter->FindTargetForEvent(root, &mouse));

  // Move 20px further away. Touch event can hit the window but
  // mouse event will not.
  gfx::Point touch_location(256 + 25, 150);
  ui::MouseEvent touch(ui::ET_TOUCH_PRESSED, touch_location, touch_location,
                       ui::EventTimeForNow(), ui::EF_NONE, ui::EF_NONE);
  EXPECT_EQ(window, targeter->FindTargetForEvent(root, &touch));

  ui::MouseEvent mouse_with_touch_loc(ui::ET_MOUSE_MOVED, touch_location,
                                      touch_location, ui::EventTimeForNow(),
                                      ui::EF_NONE, ui::EF_NONE);
  EXPECT_FALSE(window->Contains(static_cast<aura::Window*>(
      targeter->FindTargetForEvent(root, &mouse_with_touch_loc))));

  // Touching futher away shouldn't hit the window.
  gfx::Point no_touch_location(256 + 35, 150);
  ui::MouseEvent no_touch(ui::ET_TOUCH_PRESSED, no_touch_location,
                          no_touch_location, ui::EventTimeForNow(), ui::EF_NONE,
                          ui::EF_NONE);
  EXPECT_FALSE(window->Contains(static_cast<aura::Window*>(
      targeter->FindTargetForEvent(root, &no_touch))));
}

// The shell surface in SystemModal container should be unresizable.
TEST_F(ClientControlledShellSurfaceTest,
       ShellSurfaceInSystemModalIsUnresizable) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface =
      exo_test_helper()->CreateClientControlledShellSurface(surface.get(),
                                                            /*is_modal=*/true);
  surface->Attach(buffer.get());
  surface->Commit();

  EXPECT_FALSE(shell_surface->GetWidget()->widget_delegate()->CanResize());
}

// The shell surface in SystemModal container should not become target
// at the edge.
TEST_F(ClientControlledShellSurfaceTest, ShellSurfaceInSystemModalHitTest) {
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface =
      exo_test_helper()->CreateClientControlledShellSurface(surface.get(),
                                                            /*is_modal=*/true);
  shell_surface->set_client_controlled_move_resize(false);

  display::Display display = display::Screen::GetScreen()->GetPrimaryDisplay();

  gfx::Size desktop_size(640, 480);
  std::unique_ptr<Buffer> desktop_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(desktop_size)));
  surface->Attach(desktop_buffer.get());
  surface->SetInputRegion(gfx::Rect(0, 0, 0, 0));
  shell_surface->SetGeometry(display.bounds());
  surface->Commit();

  EXPECT_FALSE(shell_surface->GetWidget()->widget_delegate()->CanResize());
  aura::Window* window = shell_surface->GetWidget()->GetNativeWindow();
  aura::Window* root = window->GetRootWindow();

  ui::MouseEvent event(ui::ET_MOUSE_MOVED, gfx::Point(100, 0),
                       gfx::Point(100, 0), ui::EventTimeForNow(), 0, 0);
  aura::WindowTargeter targeter;
  aura::Window* found =
      static_cast<aura::Window*>(targeter.FindTargetForEvent(root, &event));
  EXPECT_FALSE(window->Contains(found));
}

// Test the snap functionalities in splitscreen in tablet mode.
TEST_F(ClientControlledShellSurfaceTest, SnapWindowInSplitViewModeTest) {
  UpdateDisplay("807x607");
  ash::Shell* shell = ash::Shell::Get();
  shell->tablet_mode_controller()->EnableTabletModeWindowManager(true);

  const gfx::Size buffer_size(800, 600);
  std::unique_ptr<Buffer> buffer1(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface1(new Surface);
  auto shell_surface1 =
      exo_test_helper()->CreateClientControlledShellSurface(surface1.get());
  // Start in maximized.
  shell_surface1->SetMaximized();
  surface1->Attach(buffer1.get());
  surface1->Commit();

  aura::Window* window1 = shell_surface1->GetWidget()->GetNativeWindow();
  ash::wm::WindowState* window_state1 = ash::wm::GetWindowState(window1);
  ash::wm::ClientControlledState* state1 =
      static_cast<ash::wm::ClientControlledState*>(
          ash::wm::WindowState::TestApi::GetStateImpl(window_state1));
  EXPECT_EQ(window_state1->GetStateType(),
            ash::mojom::WindowStateType::MAXIMIZED);

  // Snap window to left.
  ash::SplitViewController* split_view_controller =
      shell->split_view_controller();
  split_view_controller->SnapWindow(window1, ash::SplitViewController::LEFT);
  state1->set_bounds_locally(true);
  window1->SetBounds(split_view_controller->GetSnappedWindowBoundsInScreen(
      window1, ash::SplitViewController::LEFT));
  state1->set_bounds_locally(false);
  EXPECT_EQ(window_state1->GetStateType(),
            ash::mojom::WindowStateType::LEFT_SNAPPED);
  EXPECT_EQ(shell_surface1->GetWidget()->GetWindowBoundsInScreen(),
            split_view_controller->GetSnappedWindowBoundsInScreen(
                window1, ash::SplitViewController::LEFT));
  EXPECT_TRUE(HasBackdrop());
  split_view_controller->EndSplitView();

  // Snap window to right.
  split_view_controller->SnapWindow(window1, ash::SplitViewController::RIGHT);
  state1->set_bounds_locally(true);
  window1->SetBounds(split_view_controller->GetSnappedWindowBoundsInScreen(
      window1, ash::SplitViewController::RIGHT));
  state1->set_bounds_locally(false);
  EXPECT_EQ(window_state1->GetStateType(),
            ash::mojom::WindowStateType::RIGHT_SNAPPED);
  EXPECT_EQ(shell_surface1->GetWidget()->GetWindowBoundsInScreen(),
            split_view_controller->GetSnappedWindowBoundsInScreen(
                window1, ash::SplitViewController::RIGHT));
  EXPECT_TRUE(HasBackdrop());
}

// The shell surface in SystemModal container should not become target
// at the edge.
TEST_F(ClientControlledShellSurfaceTest, ClientIniatedResize) {
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface =
      exo_test_helper()->CreateClientControlledShellSurface(surface.get());
  shell_surface->set_client_controlled_move_resize(false);

  display::Display display = display::Screen::GetScreen()->GetPrimaryDisplay();

  gfx::Size window_size(100, 100);
  std::unique_ptr<Buffer> desktop_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(window_size)));
  surface->Attach(desktop_buffer.get());
  shell_surface->SetGeometry(gfx::Rect(window_size));
  surface->Commit();
  EXPECT_TRUE(shell_surface->GetWidget()->widget_delegate()->CanResize());
  shell_surface->StartDrag(HTTOP, gfx::Point(0, 0));

  aura::Window* window = shell_surface->GetWidget()->GetNativeWindow();
  // Client cannot start drag if mouse isn't pressed.
  ash::wm::WindowState* window_state = ash::wm::GetWindowState(window);
  ASSERT_FALSE(window_state->is_dragged());

  // Client can start drag only when the mouse is pressed on the widget.
  ui::test::EventGenerator& event_generator = GetEventGenerator();
  event_generator.MoveMouseToCenterOf(window);
  event_generator.PressLeftButton();
  shell_surface->StartDrag(HTTOP, gfx::Point(0, 0));
  ASSERT_TRUE(window_state->is_dragged());
  event_generator.ReleaseLeftButton();
  ASSERT_FALSE(window_state->is_dragged());

  // Press pressed outside of the window.
  event_generator.MoveMouseTo(gfx::Point(200, 50));
  event_generator.PressLeftButton();
  shell_surface->StartDrag(HTTOP, gfx::Point(0, 0));
  ASSERT_FALSE(window_state->is_dragged());
}

TEST_F(ClientControlledShellSurfaceTest, CaptionButtonModel) {
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface =
      exo_test_helper()->CreateClientControlledShellSurface(surface.get());

  std::unique_ptr<Buffer> desktop_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(gfx::Size(64, 64))));
  surface->Attach(desktop_buffer.get());
  shell_surface->SetGeometry(gfx::Rect(0, 0, 64, 64));
  surface->Commit();

  constexpr ash::CaptionButtonIcon kAllButtons[] = {
      ash::CAPTION_BUTTON_ICON_MINIMIZE,
      ash::CAPTION_BUTTON_ICON_MAXIMIZE_RESTORE,
      ash::CAPTION_BUTTON_ICON_CLOSE,
      ash::CAPTION_BUTTON_ICON_BACK,
      ash::CAPTION_BUTTON_ICON_MENU,
  };
  constexpr uint32_t kAllButtonMask =
      1 << ash::CAPTION_BUTTON_ICON_MINIMIZE |
      1 << ash::CAPTION_BUTTON_ICON_MAXIMIZE_RESTORE |
      1 << ash::CAPTION_BUTTON_ICON_CLOSE | 1 << ash::CAPTION_BUTTON_ICON_BACK |
      1 << ash::CAPTION_BUTTON_ICON_MENU;

  ash::CustomFrameViewAsh* frame_view = static_cast<ash::CustomFrameViewAsh*>(
      shell_surface->GetWidget()->non_client_view()->frame_view());
  ash::FrameCaptionButtonContainerView* container =
      static_cast<ash::HeaderView*>(frame_view->GetHeaderView())
          ->caption_button_container();

  // Visible
  for (auto visible : kAllButtons) {
    uint32_t visible_buttons = 1 << visible;
    shell_surface->SetFrameButtons(visible_buttons, 0);
    const ash::CaptionButtonModel* model = container->model();
    for (auto not_visible : kAllButtons) {
      if (not_visible != visible)
        EXPECT_FALSE(model->IsVisible(not_visible));
    }
    EXPECT_TRUE(model->IsVisible(visible));
    EXPECT_FALSE(model->IsEnabled(visible));
  }

  // Enable
  for (auto enabled : kAllButtons) {
    uint32_t enabled_buttons = 1 << enabled;
    shell_surface->SetFrameButtons(kAllButtonMask, enabled_buttons);
    const ash::CaptionButtonModel* model = container->model();
    for (auto not_enabled : kAllButtons) {
      if (not_enabled != enabled)
        EXPECT_FALSE(model->IsEnabled(not_enabled));
    }
    EXPECT_TRUE(model->IsEnabled(enabled));
    EXPECT_TRUE(model->IsVisible(enabled));
  }

  // Zoom mode
  EXPECT_FALSE(container->model()->InZoomMode());
  shell_surface->SetFrameButtons(
      kAllButtonMask | 1 << ash::CAPTION_BUTTON_ICON_ZOOM, kAllButtonMask);
  EXPECT_TRUE(container->model()->InZoomMode());
}

TEST_F(ClientControlledShellSurfaceTest, SetExtraTitle) {
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(gfx::Size(64, 64))));
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface =
      exo_test_helper()->CreateClientControlledShellSurface(surface.get());
  surface->Attach(buffer.get());
  surface->Commit();

  // The window title should include the debugging info, if any, and should only
  // be shown (in the frame) when there is debugging info. See
  // https://crbug.com/831383.
  const aura::Window* window = shell_surface->GetWidget()->GetNativeWindow();
  const views::WidgetDelegate* widget_delegate =
      shell_surface->GetWidget()->widget_delegate();

  shell_surface->SetExtraTitle(base::ASCIIToUTF16("extra"));
  EXPECT_EQ(base::ASCIIToUTF16(" (extra)"), window->GetTitle());
  EXPECT_TRUE(widget_delegate->ShouldShowWindowTitle());

  shell_surface->SetTitle(base::ASCIIToUTF16("title"));
  EXPECT_EQ(base::ASCIIToUTF16("title (extra)"), window->GetTitle());
  EXPECT_TRUE(widget_delegate->ShouldShowWindowTitle());

  shell_surface->SetExtraTitle(base::string16());
  EXPECT_EQ(base::ASCIIToUTF16("title"), window->GetTitle());
  EXPECT_FALSE(widget_delegate->ShouldShowWindowTitle());
}

TEST_F(ClientControlledShellSurfaceTest, WideFrame) {
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface =
      exo_test_helper()->CreateClientControlledShellSurface(surface.get());

  std::unique_ptr<Buffer> desktop_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(gfx::Size(64, 64))));
  surface->Attach(desktop_buffer.get());
  shell_surface->SetGeometry(gfx::Rect(0, 0, 64, 64));
  shell_surface->SetMaximized();
  surface->SetFrame(SurfaceFrameType::NORMAL);
  surface->Commit();

  auto* wide_frame = shell_surface->wide_frame_for_test();
  ASSERT_TRUE(wide_frame);
  EXPECT_FALSE(wide_frame->header_view()->in_immersive_mode());

  // Set AutoHide mode.
  surface->SetFrame(SurfaceFrameType::AUTOHIDE);
  EXPECT_TRUE(wide_frame->header_view()->in_immersive_mode());

  // Exit AutoHide mode.
  surface->SetFrame(SurfaceFrameType::NORMAL);
  EXPECT_FALSE(wide_frame->header_view()->in_immersive_mode());

  // Unmaximize it and the frame should be normal.
  shell_surface->SetRestored();
  surface->Commit();
  EXPECT_FALSE(shell_surface->wide_frame_for_test());
}

TEST_F(ClientControlledShellSurfaceTest, MultiDisplay) {
  display::test::DisplayManagerTestApi test_api(
      ash::Shell::Get()->display_manager());
  test_api.UpdateDisplay("100x100,100+0-100x100");

  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));

  {
    std::unique_ptr<Surface> surface(new Surface);
    auto shell_surface =
        exo_test_helper()->CreateClientControlledShellSurface(surface.get());

    gfx::Rect geometry(16, 16, 32, 32);
    shell_surface->SetGeometry(geometry);
    surface->Attach(buffer.get());
    surface->Commit();
    EXPECT_EQ(geometry.size().ToString(), shell_surface->GetWidget()
                                              ->GetWindowBoundsInScreen()
                                              .size()
                                              .ToString());

    display::Display display =
        display::Screen::GetScreen()->GetDisplayNearestWindow(
            shell_surface->host_window());
    EXPECT_EQ(gfx::Point(0, 0).ToString(),
              display.bounds().origin().ToString());
  }

  {
    std::unique_ptr<Surface> surface(new Surface);
    auto shell_surface =
        exo_test_helper()->CreateClientControlledShellSurface(surface.get());

    gfx::Rect geometry(116, 16, 32, 32);
    shell_surface->SetGeometry(geometry);
    surface->Attach(buffer.get());
    surface->Commit();
    EXPECT_EQ(geometry.size().ToString(), shell_surface->GetWidget()
                                              ->GetWindowBoundsInScreen()
                                              .size()
                                              .ToString());

    display::Display display =
        display::Screen::GetScreen()->GetDisplayNearestWindow(
            shell_surface->host_window());
    EXPECT_EQ(gfx::Point(100, 0).ToString(),
              display.bounds().origin().ToString());
  }
}

// Set orientation lock to a window.
TEST_F(ClientControlledShellSurfaceTest, SetOrientationLock) {
  display::test::DisplayManagerTestApi(ash::Shell::Get()->display_manager())
      .SetFirstDisplayAsInternalDisplay();

  EnableTabletMode(true);
  ash::ScreenOrientationController* controller =
      ash::Shell::Get()->screen_orientation_controller();

  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);

  auto shell_surface =
      exo_test_helper()->CreateClientControlledShellSurface(surface.get());
  surface->Attach(buffer.get());
  shell_surface->SetMaximized();
  surface->Commit();

  shell_surface->SetOrientationLock(
      ash::OrientationLockType::kLandscapePrimary);
  EXPECT_TRUE(controller->rotation_locked());
  display::Display display(display::Screen::GetScreen()->GetPrimaryDisplay());
  gfx::Size displaySize = display.size();
  EXPECT_GT(displaySize.width(), displaySize.height());

  shell_surface->SetOrientationLock(ash::OrientationLockType::kAny);
  EXPECT_FALSE(controller->rotation_locked());

  EnableTabletMode(false);
}

// Tests adjust bounds locally should also request remote client bounds update.
TEST_F(ClientControlledShellSurfaceTest, AdjustBoundsLocally) {
  UpdateDisplay("800x600");
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(gfx::Size(64, 64))));
  std::unique_ptr<Surface> surface(new Surface);
  auto shell_surface =
      exo_test_helper()->CreateClientControlledShellSurface(surface.get());
  gfx::Rect requested_bounds;
  shell_surface->set_bounds_changed_callback(base::BindRepeating(
      [](gfx::Rect* dst, ash::mojom::WindowStateType current_state,
         ash::mojom::WindowStateType requested_state, int64_t display_id,
         const gfx::Rect& bounds, bool is_resize,
         int bounds_change) { *dst = bounds; },
      base::Unretained(&requested_bounds)));
  surface->Attach(buffer.get());
  surface->Commit();

  gfx::Rect client_bounds(900, 0, 200, 300);
  shell_surface->SetGeometry(client_bounds);
  surface->Commit();

  views::Widget* widget = shell_surface->GetWidget();
  EXPECT_EQ(gfx::Rect(774, 0, 200, 300), widget->GetWindowBoundsInScreen());
  EXPECT_EQ(gfx::Rect(774, 0, 200, 300), requested_bounds);
}

}  // namespace exo
