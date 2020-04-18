// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/shell_surface.h"

#include "ash/accessibility/accessibility_delegate.h"
#include "ash/frame/custom_frame_view_ash.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "ash/shell_port.h"
#include "ash/shell_test_api.h"
#include "ash/wm/window_state.h"
#include "ash/wm/wm_event.h"
#include "ash/wm/workspace/workspace_window_resizer.h"
#include "ash/wm/workspace_controller_test_api.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "components/exo/buffer.h"
#include "components/exo/client_controlled_shell_surface.h"
#include "components/exo/display.h"
#include "components/exo/sub_surface.h"
#include "components/exo/surface.h"
#include "components/exo/test/exo_test_base.h"
#include "components/exo/test/exo_test_helper.h"
#include "components/exo/wm_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/base/hit_test.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/layer.h"
#include "ui/display/display.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/screen.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/window_util.h"

namespace exo {
namespace {

using ShellSurfaceTest = test::ExoTestBase;

bool HasBackdrop() {
  ash::WorkspaceController* wc =
      ash::ShellTestApi(ash::Shell::Get()).workspace_controller();
  return !!ash::WorkspaceControllerTestApi(wc).GetBackdropWindow();
}

uint32_t ConfigureFullscreen(uint32_t serial,
                             const gfx::Size& size,
                             ash::mojom::WindowStateType state_type,
                             bool resizing,
                             bool activated,
                             const gfx::Vector2d& origin_offset) {
  EXPECT_EQ(ash::mojom::WindowStateType::FULLSCREEN, state_type);
  return serial;
}

TEST_F(ShellSurfaceTest, AcknowledgeConfigure) {
  gfx::Size buffer_size(32, 32);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  surface->Attach(buffer.get());
  surface->Commit();

  gfx::Point origin(100, 100);
  shell_surface->GetWidget()->SetBounds(gfx::Rect(origin, buffer_size));
  EXPECT_EQ(origin.ToString(),
            surface->window()->GetBoundsInRootWindow().origin().ToString());

  const uint32_t kSerial = 1;
  shell_surface->set_configure_callback(
      base::Bind(&ConfigureFullscreen, kSerial));
  shell_surface->SetFullscreen(true);

  // Surface origin should not change until configure request is acknowledged.
  EXPECT_EQ(origin.ToString(),
            surface->window()->GetBoundsInRootWindow().origin().ToString());

  // Compositor should be locked until configure request is acknowledged.
  ui::Compositor* compositor =
      shell_surface->GetWidget()->GetNativeWindow()->layer()->GetCompositor();
  EXPECT_TRUE(compositor->IsLocked());

  shell_surface->AcknowledgeConfigure(kSerial);
  std::unique_ptr<Buffer> fullscreen_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(
          CurrentContext()->bounds().size())));
  surface->Attach(fullscreen_buffer.get());
  surface->Commit();

  EXPECT_EQ(gfx::Point().ToString(),
            surface->window()->GetBoundsInRootWindow().origin().ToString());
  EXPECT_FALSE(compositor->IsLocked());
}

TEST_F(ShellSurfaceTest, SetParent) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> parent_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> parent_surface(new Surface);
  std::unique_ptr<ShellSurface> parent_shell_surface(
      new ShellSurface(parent_surface.get()));

  parent_surface->Attach(parent_buffer.get());
  parent_surface->Commit();

  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));
  shell_surface->SetParent(parent_shell_surface.get());

  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_EQ(
      parent_shell_surface->GetWidget()->GetNativeWindow(),
      wm::GetTransientParent(shell_surface->GetWidget()->GetNativeWindow()));

  // Use OnSetParent to move shell surface to 10, 10.
  gfx::Point parent_origin =
      parent_shell_surface->GetWidget()->GetWindowBoundsInScreen().origin();
  shell_surface->OnSetParent(
      parent_surface.get(),
      gfx::PointAtOffsetFromOrigin(gfx::Point(10, 10) - parent_origin));
  EXPECT_EQ(gfx::Rect(10, 10, 256, 256),
            shell_surface->GetWidget()->GetWindowBoundsInScreen());
}

TEST_F(ShellSurfaceTest, Maximize) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_FALSE(HasBackdrop());
  shell_surface->Maximize();
  EXPECT_FALSE(HasBackdrop());
  surface->Commit();
  EXPECT_FALSE(HasBackdrop());
  EXPECT_EQ(CurrentContext()->bounds().width(),
            shell_surface->GetWidget()->GetWindowBoundsInScreen().width());
  EXPECT_TRUE(shell_surface->GetWidget()->IsMaximized());

  // Toggle maximize.
  ash::wm::WMEvent maximize_event(ash::wm::WM_EVENT_TOGGLE_MAXIMIZE);
  aura::Window* window = shell_surface->GetWidget()->GetNativeWindow();

  ash::wm::GetWindowState(window)->OnWMEvent(&maximize_event);
  EXPECT_FALSE(shell_surface->GetWidget()->IsMaximized());
  EXPECT_FALSE(HasBackdrop());

  ash::wm::GetWindowState(window)->OnWMEvent(&maximize_event);
  EXPECT_TRUE(shell_surface->GetWidget()->IsMaximized());
  EXPECT_FALSE(HasBackdrop());
}

TEST_F(ShellSurfaceTest, Minimize) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  EXPECT_TRUE(shell_surface->CanMinimize());

  // Minimizing can be performed before the surface is committed.
  shell_surface->Minimize();
  EXPECT_TRUE(shell_surface->GetWidget()->IsMinimized());

  // Confirm that attaching and commiting doesn't reset the state.
  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_TRUE(shell_surface->GetWidget()->IsMinimized());

  shell_surface->Restore();
  EXPECT_FALSE(shell_surface->GetWidget()->IsMinimized());

  std::unique_ptr<Surface> child_surface(new Surface);
  std::unique_ptr<ShellSurface> child_shell_surface(
      new ShellSurface(child_surface.get()));

  // Transient shell surfaces cannot be minimized.
  child_surface->SetParent(surface.get(), gfx::Point());
  child_surface->Attach(buffer.get());
  child_surface->Commit();
  EXPECT_FALSE(child_shell_surface->CanMinimize());
}

TEST_F(ShellSurfaceTest, Restore) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_FALSE(HasBackdrop());
  // Note: Remove contents to avoid issues with maximize animations in tests.
  shell_surface->Maximize();
  EXPECT_FALSE(HasBackdrop());
  shell_surface->Restore();
  EXPECT_FALSE(HasBackdrop());
  EXPECT_EQ(
      buffer_size.ToString(),
      shell_surface->GetWidget()->GetWindowBoundsInScreen().size().ToString());
}

TEST_F(ShellSurfaceTest, SetFullscreen) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  shell_surface->SetFullscreen(true);
  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_FALSE(HasBackdrop());
  EXPECT_EQ(CurrentContext()->bounds().ToString(),
            shell_surface->GetWidget()->GetWindowBoundsInScreen().ToString());
  shell_surface->SetFullscreen(false);
  surface->Commit();
  EXPECT_FALSE(HasBackdrop());
  EXPECT_NE(CurrentContext()->bounds().ToString(),
            shell_surface->GetWidget()->GetWindowBoundsInScreen().ToString());
}

TEST_F(ShellSurfaceTest, SetTitle) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  shell_surface->SetTitle(base::string16(base::ASCIIToUTF16("test")));
  surface->Attach(buffer.get());
  surface->Commit();

  // NativeWindow's title is used within the overview mode, so it should
  // have the specified title.
  EXPECT_EQ(base::ASCIIToUTF16("test"),
            shell_surface->GetWidget()->GetNativeWindow()->GetTitle());
  // The titlebar shouldn't show the title.
  EXPECT_FALSE(
      shell_surface->GetWidget()->widget_delegate()->ShouldShowWindowTitle());
}

TEST_F(ShellSurfaceTest, SetApplicationId) {
  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  EXPECT_FALSE(shell_surface->GetWidget());
  shell_surface->SetApplicationId("pre-widget-id");

  surface->Attach(buffer.get());
  surface->Commit();
  aura::Window* window = shell_surface->GetWidget()->GetNativeWindow();
  EXPECT_EQ("pre-widget-id", *ShellSurface::GetApplicationId(window));
  shell_surface->SetApplicationId("test");
  EXPECT_EQ("test", *ShellSurface::GetApplicationId(window));

  shell_surface->SetApplicationId(nullptr);
  EXPECT_EQ(nullptr, ShellSurface::GetApplicationId(window));
}

TEST_F(ShellSurfaceTest, SetStartupId) {
  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  EXPECT_FALSE(shell_surface->GetWidget());
  shell_surface->SetStartupId("pre-widget-id");

  surface->Attach(buffer.get());
  surface->Commit();
  aura::Window* window = shell_surface->GetWidget()->GetNativeWindow();
  EXPECT_EQ("pre-widget-id", *ShellSurface::GetStartupId(window));
  shell_surface->SetStartupId("test");
  EXPECT_EQ("test", *ShellSurface::GetStartupId(window));

  shell_surface->SetStartupId(nullptr);
  EXPECT_EQ(nullptr, ShellSurface::GetStartupId(window));
}

TEST_F(ShellSurfaceTest, Move) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  // Map shell surface.
  surface->Commit();

  // The interactive move should end when surface is destroyed.
  shell_surface->Move();

  // Test that destroying the shell surface before move ends is OK.
  shell_surface.reset();
}

TEST_F(ShellSurfaceTest, Resize) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  // Map shell surface.
  surface->Commit();

  // The interactive resize should end when surface is destroyed.
  shell_surface->Resize(HTBOTTOMRIGHT);

  // Test that destroying the surface before resize ends is OK.
  surface.reset();
}

TEST_F(ShellSurfaceTest, SetGeometry) {
  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  gfx::Rect geometry(16, 16, 32, 32);
  shell_surface->SetGeometry(geometry);
  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_EQ(
      geometry.size().ToString(),
      shell_surface->GetWidget()->GetWindowBoundsInScreen().size().ToString());
  EXPECT_EQ(gfx::Rect(gfx::Point() - geometry.OffsetFromOrigin(), buffer_size)
                .ToString(),
            shell_surface->host_window()->bounds().ToString());
}

TEST_F(ShellSurfaceTest, SetMinimumSize) {
  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  gfx::Size size(50, 50);
  shell_surface->SetMinimumSize(size);
  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_EQ(size, shell_surface->GetMinimumSize());
  EXPECT_EQ(size, shell_surface->GetWidget()->GetMinimumSize());
  EXPECT_EQ(size, shell_surface->GetWidget()
                      ->GetNativeWindow()
                      ->delegate()
                      ->GetMinimumSize());

  gfx::Size size_with_frame(50, 82);
  surface->SetFrame(SurfaceFrameType::NORMAL);
  EXPECT_EQ(size, shell_surface->GetMinimumSize());
  EXPECT_EQ(size_with_frame, shell_surface->GetWidget()->GetMinimumSize());
  EXPECT_EQ(size_with_frame, shell_surface->GetWidget()
                                 ->GetNativeWindow()
                                 ->delegate()
                                 ->GetMinimumSize());
}

TEST_F(ShellSurfaceTest, SetMaximumSize) {
  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  gfx::Size size(100, 100);
  shell_surface->SetMaximumSize(size);
  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_EQ(size, shell_surface->GetMaximumSize());
}

void Close(int* close_call_count) {
  (*close_call_count)++;
}

TEST_F(ShellSurfaceTest, CloseCallback) {
  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  int close_call_count = 0;
  shell_surface->set_close_callback(
      base::Bind(&Close, base::Unretained(&close_call_count)));

  surface->Attach(buffer.get());
  surface->Commit();

  EXPECT_EQ(0, close_call_count);
  shell_surface->GetWidget()->Close();
  EXPECT_EQ(1, close_call_count);
}

void DestroyShellSurface(std::unique_ptr<ShellSurface>* shell_surface) {
  shell_surface->reset();
}

TEST_F(ShellSurfaceTest, SurfaceDestroyedCallback) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  shell_surface->set_surface_destroyed_callback(
      base::Bind(&DestroyShellSurface, base::Unretained(&shell_surface)));

  surface->Commit();

  EXPECT_TRUE(shell_surface.get());
  surface.reset();
  EXPECT_FALSE(shell_surface.get());
}

uint32_t Configure(gfx::Size* suggested_size,
                   ash::mojom::WindowStateType* has_state_type,
                   bool* is_resizing,
                   bool* is_active,
                   const gfx::Size& size,
                   ash::mojom::WindowStateType state_type,
                   bool resizing,
                   bool activated,
                   const gfx::Vector2d& origin_offset) {
  *suggested_size = size;
  *has_state_type = state_type;
  *is_resizing = resizing;
  *is_active = activated;
  return 0;
}

TEST_F(ShellSurfaceTest, ConfigureCallback) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  gfx::Size suggested_size;
  ash::mojom::WindowStateType has_state_type =
      ash::mojom::WindowStateType::NORMAL;
  bool is_resizing = false;
  bool is_active = false;
  shell_surface->set_configure_callback(
      base::Bind(&Configure, base::Unretained(&suggested_size),
                 base::Unretained(&has_state_type),
                 base::Unretained(&is_resizing), base::Unretained(&is_active)));
  // Commit without contents should result in a configure callback with empty
  // suggested size as a mechanims to ask the client size itself.
  surface->Commit();
  EXPECT_EQ(gfx::Size(), suggested_size);

  shell_surface->Maximize();
  shell_surface->AcknowledgeConfigure(0);
  EXPECT_EQ(CurrentContext()->bounds().width(), suggested_size.width());
  EXPECT_EQ(ash::mojom::WindowStateType::MAXIMIZED, has_state_type);
  shell_surface->Restore();
  shell_surface->AcknowledgeConfigure(0);

  shell_surface->SetFullscreen(true);
  shell_surface->AcknowledgeConfigure(0);
  EXPECT_EQ(CurrentContext()->bounds().size().ToString(),
            suggested_size.ToString());
  EXPECT_EQ(ash::mojom::WindowStateType::FULLSCREEN, has_state_type);
  shell_surface->SetFullscreen(false);
  shell_surface->AcknowledgeConfigure(0);

  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  surface->Attach(buffer.get());
  surface->Commit();
  shell_surface->GetWidget()->Activate();
  shell_surface->AcknowledgeConfigure(0);
  EXPECT_TRUE(is_active);
  shell_surface->GetWidget()->Deactivate();
  shell_surface->AcknowledgeConfigure(0);
  EXPECT_FALSE(is_active);

  EXPECT_FALSE(is_resizing);
  shell_surface->Resize(HTBOTTOMRIGHT);
  shell_surface->AcknowledgeConfigure(0);
  EXPECT_TRUE(is_resizing);
}

TEST_F(ShellSurfaceTest, ToggleFullscreen) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_FALSE(HasBackdrop());
  EXPECT_EQ(
      buffer_size.ToString(),
      shell_surface->GetWidget()->GetWindowBoundsInScreen().size().ToString());
  shell_surface->Maximize();
  EXPECT_FALSE(HasBackdrop());
  EXPECT_EQ(CurrentContext()->bounds().width(),
            shell_surface->GetWidget()->GetWindowBoundsInScreen().width());

  ash::wm::WMEvent event(ash::wm::WM_EVENT_TOGGLE_FULLSCREEN);
  aura::Window* window = shell_surface->GetWidget()->GetNativeWindow();

  // Enter fullscreen mode.
  ash::wm::GetWindowState(window)->OnWMEvent(&event);

  EXPECT_FALSE(HasBackdrop());
  EXPECT_EQ(CurrentContext()->bounds().ToString(),
            shell_surface->GetWidget()->GetWindowBoundsInScreen().ToString());

  // Leave fullscreen mode.
  ash::wm::GetWindowState(window)->OnWMEvent(&event);
  EXPECT_FALSE(HasBackdrop());

  // Check that shell surface is maximized.
  EXPECT_EQ(CurrentContext()->bounds().width(),
            shell_surface->GetWidget()->GetWindowBoundsInScreen().width());
}

TEST_F(ShellSurfaceTest, FrameColors) {
  std::unique_ptr<Surface> surface(new Surface);
  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  surface->Attach(buffer.get());
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));
  shell_surface->OnSetFrameColors(SK_ColorRED, SK_ColorTRANSPARENT);
  surface->Commit();

  const ash::CustomFrameViewAsh* frame =
      static_cast<const ash::CustomFrameViewAsh*>(
          shell_surface->GetWidget()->non_client_view()->frame_view());

  // Test if colors set before initial commit are set.
  EXPECT_EQ(SK_ColorRED, frame->GetActiveFrameColorForTest());
  // Frame should be fully opaque.
  EXPECT_EQ(SK_ColorBLACK, frame->GetInactiveFrameColorForTest());

  shell_surface->OnSetFrameColors(SK_ColorTRANSPARENT, SK_ColorBLUE);
  EXPECT_EQ(SK_ColorBLACK, frame->GetActiveFrameColorForTest());
  EXPECT_EQ(SK_ColorBLUE, frame->GetInactiveFrameColorForTest());
}

TEST_F(ShellSurfaceTest, CycleSnap) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_EQ(buffer_size,
            shell_surface->GetWidget()->GetWindowBoundsInScreen().size());

  ash::wm::WMEvent event(ash::wm::WM_EVENT_CYCLE_SNAP_LEFT);
  aura::Window* window = shell_surface->GetWidget()->GetNativeWindow();

  // Enter snapped mode.
  ash::wm::GetWindowState(window)->OnWMEvent(&event);

  EXPECT_EQ(CurrentContext()->bounds().width() / 2,
            shell_surface->GetWidget()->GetWindowBoundsInScreen().width());

  surface->Attach(buffer.get());
  surface->Commit();

  // Commit shouldn't change widget bounds when snapped.
  EXPECT_EQ(CurrentContext()->bounds().width() / 2,
            shell_surface->GetWidget()->GetWindowBoundsInScreen().width());
}

}  // namespace
}  // namespace exo
