// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/desktop_aura/desktop_window_tree_host_platform.h"

#include "ui/aura/client/drag_drop_client.h"
#include "ui/aura/client/transient_window_client.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/dip_util.h"
#include "ui/platform_window/platform_window.h"
#include "ui/views/corewm/tooltip_aura.h"
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#include "ui/views/window/native_frame_view.h"
#include "ui/wm/core/window_util.h"

namespace views {

////////////////////////////////////////////////////////////////////////////////
// DesktopWindowTreeHostPlatform:

DesktopWindowTreeHostPlatform::DesktopWindowTreeHostPlatform(
    internal::NativeWidgetDelegate* native_widget_delegate,
    DesktopNativeWidgetAura* desktop_native_widget_aura)
    : native_widget_delegate_(native_widget_delegate),
      desktop_native_widget_aura_(desktop_native_widget_aura) {}

DesktopWindowTreeHostPlatform::~DesktopWindowTreeHostPlatform() {
  DCHECK(got_on_closed_);
  desktop_native_widget_aura_->OnDesktopWindowTreeHostDestroyed(this);
  DestroyDispatcher();
}

void DesktopWindowTreeHostPlatform::SetBoundsInDIP(
    const gfx::Rect& bounds_in_dip) {
  DCHECK_NE(0, device_scale_factor());
  SetBoundsInPixels(
      gfx::ConvertRectToPixel(device_scale_factor(), bounds_in_dip),
      viz::LocalSurfaceId());
}

void DesktopWindowTreeHostPlatform::Init(const Widget::InitParams& params) {
  CreateAndSetDefaultPlatformWindow();
  CreateCompositor(viz::FrameSinkId(), params.force_software_compositing);
  aura::WindowTreeHost::OnAcceleratedWidgetAvailable();
  InitHost();
  if (!params.bounds.IsEmpty())
    SetBoundsInDIP(params.bounds);
  window()->Show();
}

void DesktopWindowTreeHostPlatform::OnNativeWidgetCreated(
    const Widget::InitParams& params) {
  native_widget_delegate_->OnNativeWidgetCreated(true);
}

void DesktopWindowTreeHostPlatform::OnWidgetInitDone() {}

void DesktopWindowTreeHostPlatform::OnActiveWindowChanged(bool active) {}

std::unique_ptr<corewm::Tooltip>
DesktopWindowTreeHostPlatform::CreateTooltip() {
  return std::make_unique<corewm::TooltipAura>();
}

std::unique_ptr<aura::client::DragDropClient>
DesktopWindowTreeHostPlatform::CreateDragDropClient(
    DesktopNativeCursorManager* cursor_manager) {
  // TODO: needs PlatformWindow support.
  NOTIMPLEMENTED_LOG_ONCE();
  return nullptr;
}

void DesktopWindowTreeHostPlatform::Close() {
  if (waiting_for_close_now_)
    return;

  // Hide while waiting for the close.
  // Please note that it's better to call WindowTreeHost::Hide, which also calls
  // PlatformWindow::Hide and Compositor::SetVisible(false).
  Hide();

  waiting_for_close_now_ = true;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&DesktopWindowTreeHostPlatform::CloseNow,
                                weak_factory_.GetWeakPtr()));
}

void DesktopWindowTreeHostPlatform::CloseNow() {
  auto weak_ref = weak_factory_.GetWeakPtr();
  // Deleting the PlatformWindow may not result in OnClosed() being called, if
  // not behave as though it was.
  SetPlatformWindow(nullptr);
  if (!weak_ref || got_on_closed_)
    return;

  got_on_closed_ = true;
  desktop_native_widget_aura_->OnHostClosed();
}

aura::WindowTreeHost* DesktopWindowTreeHostPlatform::AsWindowTreeHost() {
  return this;
}

void DesktopWindowTreeHostPlatform::ShowWindowWithState(
    ui::WindowShowState show_state) {
  if (compositor()) {
    platform_window()->Show();
    compositor()->SetVisible(true);
  }

  switch (show_state) {
    case ui::SHOW_STATE_MAXIMIZED:
      platform_window()->Maximize();
      break;
    case ui::SHOW_STATE_MINIMIZED:
      platform_window()->Minimize();
      break;
    case ui::SHOW_STATE_FULLSCREEN:
      // TODO(sky): this isn't necessarily the same as explicitly setting
      // fullscreen.
      platform_window()->ToggleFullscreen();
      break;
    default:
      break;
  }

  if (native_widget_delegate_->CanActivate()) {
    if (show_state != ui::SHOW_STATE_INACTIVE)
      Activate();

    // SetInitialFocus() should be always be called, even for
    // SHOW_STATE_INACTIVE. If the window has to stay inactive, the method will
    // do the right thing.
    // Activate() might fail if the window is non-activatable. In this case, we
    // should pass SHOW_STATE_INACTIVE to SetInitialFocus() to stop the initial
    // focused view from getting focused. See https://crbug.com/515594 for
    // example.
    native_widget_delegate_->SetInitialFocus(
        IsActive() ? show_state : ui::SHOW_STATE_INACTIVE);
  }
}

void DesktopWindowTreeHostPlatform::ShowMaximizedWithBounds(
    const gfx::Rect& restored_bounds) {
  // TODO: support |restored_bounds|.
  ShowWindowWithState(ui::SHOW_STATE_MAXIMIZED);
}

bool DesktopWindowTreeHostPlatform::IsVisible() const {
  // TODO: needs PlatformWindow support.
  NOTIMPLEMENTED_LOG_ONCE();
  return true;
}

void DesktopWindowTreeHostPlatform::SetSize(const gfx::Size& size) {
  gfx::Rect screen_bounds =
      gfx::ConvertRectToDIP(device_scale_factor(), GetBoundsInPixels());
  screen_bounds.set_size(size);
  SetBoundsInDIP(screen_bounds);
}

void DesktopWindowTreeHostPlatform::StackAbove(aura::Window* window) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void DesktopWindowTreeHostPlatform::StackAtTop() {
  NOTIMPLEMENTED_LOG_ONCE();
}

void DesktopWindowTreeHostPlatform::CenterWindow(const gfx::Size& size) {
  gfx::Rect bounds_to_center_in = GetWorkAreaBoundsInScreen();

  // If there is a transient parent and it fits |size|, then center over it.
  aura::Window* content_window = desktop_native_widget_aura_->content_window();
  if (wm::GetTransientParent(content_window)) {
    gfx::Rect transient_parent_bounds =
        wm::GetTransientParent(content_window)->GetBoundsInScreen();
    if (transient_parent_bounds.height() >= size.height() &&
        transient_parent_bounds.width() >= size.width()) {
      bounds_to_center_in = transient_parent_bounds;
    }
  }

  gfx::Rect resulting_bounds(bounds_to_center_in);
  resulting_bounds.ClampToCenteredSize(size);
  SetBoundsInDIP(resulting_bounds);
}

void DesktopWindowTreeHostPlatform::GetWindowPlacement(
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  NOTIMPLEMENTED_LOG_ONCE();
  *bounds = gfx::Rect(0, 0, 640, 840);
  *show_state = ui::SHOW_STATE_NORMAL;
}

gfx::Rect DesktopWindowTreeHostPlatform::GetWindowBoundsInScreen() const {
  gfx::Rect bounds =
      gfx::ConvertRectToDIP(device_scale_factor(), GetBoundsInPixels());
  bounds += display::Screen::GetScreen()
                ->GetDisplayNearestWindow(const_cast<aura::Window*>(window()))
                .bounds()
                .OffsetFromOrigin();
  return bounds;
}

gfx::Rect DesktopWindowTreeHostPlatform::GetClientAreaBoundsInScreen() const {
  // View-to-screen coordinate system transformations depend on this returning
  // the full window bounds, for example View::ConvertPointToScreen().
  return GetWindowBoundsInScreen();
}

gfx::Rect DesktopWindowTreeHostPlatform::GetRestoredBounds() const {
  NOTIMPLEMENTED_LOG_ONCE();
  return gfx::Rect(0, 0, 640, 840);
}

std::string DesktopWindowTreeHostPlatform::GetWorkspace() const {
  return std::string();
}

gfx::Rect DesktopWindowTreeHostPlatform::GetWorkAreaBoundsInScreen() const {
  // TODO(sky): GetDisplayNearestWindow() should take a const aura::Window*.
  return display::Screen::GetScreen()
      ->GetDisplayNearestWindow(const_cast<aura::Window*>(window()))
      .work_area();
}

void DesktopWindowTreeHostPlatform::SetShape(
    std::unique_ptr<Widget::ShapeRects> native_shape) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void DesktopWindowTreeHostPlatform::Activate() {
  // TODO: needs PlatformWindow support.
  NOTIMPLEMENTED_LOG_ONCE();
}

void DesktopWindowTreeHostPlatform::Deactivate() {
  // TODO: needs PlatformWindow support.
  NOTIMPLEMENTED_LOG_ONCE();
}

bool DesktopWindowTreeHostPlatform::IsActive() const {
  return is_active_;
}

void DesktopWindowTreeHostPlatform::Maximize() {
  platform_window()->Maximize();
}

void DesktopWindowTreeHostPlatform::Minimize() {
  platform_window()->Minimize();
}

void DesktopWindowTreeHostPlatform::Restore() {
  platform_window()->Restore();
}

bool DesktopWindowTreeHostPlatform::IsMaximized() const {
  return platform_window()->GetPlatformWindowState() ==
         ui::PlatformWindowState::PLATFORM_WINDOW_STATE_MAXIMIZED;
}

bool DesktopWindowTreeHostPlatform::IsMinimized() const {
  return platform_window()->GetPlatformWindowState() ==
         ui::PlatformWindowState::PLATFORM_WINDOW_STATE_MINIMIZED;
}

bool DesktopWindowTreeHostPlatform::HasCapture() const {
  return platform_window()->HasCapture();
}

void DesktopWindowTreeHostPlatform::SetAlwaysOnTop(bool always_on_top) {
  // TODO: needs PlatformWindow support.
  NOTIMPLEMENTED_LOG_ONCE();
}

bool DesktopWindowTreeHostPlatform::IsAlwaysOnTop() const {
  // TODO: needs PlatformWindow support.
  return false;
}

void DesktopWindowTreeHostPlatform::SetVisibleOnAllWorkspaces(
    bool always_visible) {
  // TODO: needs PlatformWindow support.
  NOTIMPLEMENTED_LOG_ONCE();
}

bool DesktopWindowTreeHostPlatform::IsVisibleOnAllWorkspaces() const {
  // TODO: needs PlatformWindow support.
  return false;
}

bool DesktopWindowTreeHostPlatform::SetWindowTitle(
    const base::string16& title) {
  // TODO: needs PlatformWindow support.
  NOTIMPLEMENTED_LOG_ONCE();
  return false;
}

void DesktopWindowTreeHostPlatform::ClearNativeFocus() {
  // TODO: needs PlatformWindow support.
  NOTIMPLEMENTED_LOG_ONCE();
}

Widget::MoveLoopResult DesktopWindowTreeHostPlatform::RunMoveLoop(
    const gfx::Vector2d& drag_offset,
    Widget::MoveLoopSource source,
    Widget::MoveLoopEscapeBehavior escape_behavior) {
  // TODO: needs PlatformWindow support.
  NOTIMPLEMENTED_LOG_ONCE();
  return Widget::MOVE_LOOP_CANCELED;
}

void DesktopWindowTreeHostPlatform::EndMoveLoop() {
  // TODO: needs PlatformWindow support.
  NOTIMPLEMENTED_LOG_ONCE();
}

void DesktopWindowTreeHostPlatform::SetVisibilityChangedAnimationsEnabled(
    bool value) {
  // TODO: needs PlatformWindow support.
  NOTIMPLEMENTED_LOG_ONCE();
}

NonClientFrameView* DesktopWindowTreeHostPlatform::CreateNonClientFrameView() {
  return ShouldUseNativeFrame() ? new NativeFrameView(GetWidget()) : nullptr;
}

bool DesktopWindowTreeHostPlatform::ShouldUseNativeFrame() const {
  return false;
}

bool DesktopWindowTreeHostPlatform::ShouldWindowContentsBeTransparent() const {
  return false;
}

void DesktopWindowTreeHostPlatform::FrameTypeChanged() {}

void DesktopWindowTreeHostPlatform::SetFullscreen(bool fullscreen) {
  if (IsFullscreen() != fullscreen)
    platform_window()->ToggleFullscreen();
}

bool DesktopWindowTreeHostPlatform::IsFullscreen() const {
  return platform_window()->GetPlatformWindowState() ==
         ui::PlatformWindowState::PLATFORM_WINDOW_STATE_FULLSCREEN;
}

void DesktopWindowTreeHostPlatform::SetOpacity(float opacity) {
  // TODO: needs PlatformWindow support.
  NOTIMPLEMENTED_LOG_ONCE();
}

void DesktopWindowTreeHostPlatform::SetWindowIcons(
    const gfx::ImageSkia& window_icon,
    const gfx::ImageSkia& app_icon) {
  // TODO: needs PlatformWindow support.
  NOTIMPLEMENTED_LOG_ONCE();
}

void DesktopWindowTreeHostPlatform::InitModalType(ui::ModalType modal_type) {
  // TODO: needs PlatformWindow support (alternatively, remove as
  // DesktopWindowTreeHostX11 doesn't support at all).
  NOTIMPLEMENTED_LOG_ONCE();
}

void DesktopWindowTreeHostPlatform::FlashFrame(bool flash_frame) {
  // TODO: needs PlatformWindow support.
  NOTIMPLEMENTED_LOG_ONCE();
}

bool DesktopWindowTreeHostPlatform::IsAnimatingClosed() const {
  // TODO: needs PlatformWindow support.
  NOTIMPLEMENTED_LOG_ONCE();
  return false;
}

bool DesktopWindowTreeHostPlatform::IsTranslucentWindowOpacitySupported()
    const {
  // TODO: needs PlatformWindow support.
  NOTIMPLEMENTED_LOG_ONCE();
  return false;
}

void DesktopWindowTreeHostPlatform::SizeConstraintsChanged() {
  // TODO: needs PlatformWindow support.
  NOTIMPLEMENTED_LOG_ONCE();
}

bool DesktopWindowTreeHostPlatform::ShouldUpdateWindowTransparency() const {
  return false;
}

bool DesktopWindowTreeHostPlatform::ShouldUseDesktopNativeCursorManager()
    const {
  return true;
}

bool DesktopWindowTreeHostPlatform::ShouldCreateVisibilityController() const {
  return true;
}

void DesktopWindowTreeHostPlatform::OnClosed() {
  got_on_closed_ = true;
  desktop_native_widget_aura_->OnHostClosed();
}

void DesktopWindowTreeHostPlatform::OnWindowStateChanged(
    ui::PlatformWindowState new_state) {
  // Propagate minimization/restore to compositor to avoid drawing 'blank'
  // frames that could be treated as previews, which show content even if a
  // window is minimized.
  bool visible =
      new_state != ui::PlatformWindowState::PLATFORM_WINDOW_STATE_MINIMIZED;
  if (visible != compositor()->IsVisible()) {
    compositor()->SetVisible(visible);
    native_widget_delegate_->OnNativeWidgetVisibilityChanged(visible);
  }

  // It might require relayouting when state property has been changed.
  if (visible)
    Relayout();
}

void DesktopWindowTreeHostPlatform::OnCloseRequest() {
  GetWidget()->Close();
}

void DesktopWindowTreeHostPlatform::OnAcceleratedWidgetDestroying() {
  native_widget_delegate_->OnNativeWidgetDestroying();
}

void DesktopWindowTreeHostPlatform::OnActivationChanged(bool active) {
  is_active_ = active;
  aura::WindowTreeHostPlatform::OnActivationChanged(active);
  desktop_native_widget_aura_->HandleActivationChanged(active);
}

void DesktopWindowTreeHostPlatform::Relayout() {
  Widget* widget = native_widget_delegate_->AsWidget();
  NonClientView* non_client_view = widget->non_client_view();
  // non_client_view may be NULL, especially during creation.
  if (non_client_view) {
    non_client_view->client_view()->InvalidateLayout();
    non_client_view->InvalidateLayout();
  }
  widget->GetRootView()->Layout();
}

Widget* DesktopWindowTreeHostPlatform::GetWidget() {
  return native_widget_delegate_->AsWidget();
}

////////////////////////////////////////////////////////////////////////////////
// DesktopWindowTreeHost:

// static
DesktopWindowTreeHost* DesktopWindowTreeHost::Create(
    internal::NativeWidgetDelegate* native_widget_delegate,
    DesktopNativeWidgetAura* desktop_native_widget_aura) {
  return new DesktopWindowTreeHostPlatform(native_widget_delegate,
                                           desktop_native_widget_aura);
}

}  // namespace views
