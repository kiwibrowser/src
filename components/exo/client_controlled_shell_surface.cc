// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/client_controlled_shell_surface.h"

#include <utility>

#include "ash/frame/caption_buttons/caption_button_model.h"
#include "ash/frame/custom_frame_view_ash.h"
#include "ash/frame/header_view.h"
#include "ash/frame/wide_frame_view.h"
#include "ash/public/cpp/immersive/immersive_fullscreen_controller.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/public/cpp/window_state_type.h"
#include "ash/public/interfaces/window_pin_type.mojom.h"
#include "ash/shell.h"
#include "ash/wm/client_controlled_state.h"
#include "ash/wm/drag_details.h"
#include "ash/wm/drag_window_resizer.h"
#include "ash/wm/toplevel_window_event_handler.h"
#include "ash/wm/window_positioning_utils.h"
#include "ash/wm/window_properties.h"
#include "ash/wm/window_resizer.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_state_delegate.h"
#include "ash/wm/window_util.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"
#include "components/exo/surface.h"
#include "components/exo/wm_helper.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/class_property.h"
#include "ui/compositor/compositor_lock.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/coordinate_conversion.h"
#include "ui/wm/core/window_util.h"

namespace exo {

namespace {

ClientControlledShellSurface::DelegateFactoryCallback& GetFactoryForTesting() {
  using CallbackType = ClientControlledShellSurface::DelegateFactoryCallback;
  static base::NoDestructor<CallbackType> factory;
  return *factory;
}

// Maximum amount of time to wait for contents that match the display's
// orientation in tablet mode.
// TODO(oshima): Looks like android is generating unnecessary frames.
// Fix it on Android side and reduce the timeout.
constexpr int kOrientationLockTimeoutMs = 2500;

// Minimal WindowResizer that unlike DefaultWindowResizer does not handle
// dragging and resizing windows.
class CustomWindowResizer : public ash::WindowResizer {
 public:
  explicit CustomWindowResizer(ash::wm::WindowState* window_state)
      : WindowResizer(window_state) {}

  // Overridden from ash::WindowResizer:
  void Drag(const gfx::Point& location, int event_flags) override {}
  void CompleteDrag() override {}
  void RevertDrag() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(CustomWindowResizer);
};

Orientation SizeToOrientation(const gfx::Size& size) {
  DCHECK_NE(size.width(), size.height());
  return size.width() > size.height() ? Orientation::LANDSCAPE
                                      : Orientation::PORTRAIT;
}

// A ClientControlledStateDelegate that sends the state/bounds
// change request to exo client.
class ClientControlledStateDelegate
    : public ash::wm::ClientControlledState::Delegate {
 public:
  explicit ClientControlledStateDelegate(
      ClientControlledShellSurface* shell_surface)
      : shell_surface_(shell_surface) {}
  ~ClientControlledStateDelegate() override {}

  // Overridden from ash::wm::ClientControlledState::Delegate:
  void HandleWindowStateRequest(
      ash::wm::WindowState* window_state,
      ash::mojom::WindowStateType next_state) override {
    shell_surface_->OnWindowStateChangeEvent(window_state->GetStateType(),
                                             next_state);
  }
  void HandleBoundsRequest(ash::wm::WindowState* window_state,
                           ash::mojom::WindowStateType requested_state,
                           const gfx::Rect& bounds) override {
    gfx::Rect bounds_in_screen(bounds);
    ::wm::ConvertRectToScreen(window_state->window()->GetRootWindow(),
                              &bounds_in_screen);
    int64_t display_id = display::Screen::GetScreen()
                             ->GetDisplayNearestWindow(window_state->window())
                             .id();

    shell_surface_->OnBoundsChangeEvent(
        window_state->GetStateType(), requested_state, display_id,
        bounds_in_screen,
        window_state->drag_details()
            ? window_state->drag_details()->bounds_change
            : 0);
  }

 private:
  ClientControlledShellSurface* shell_surface_;
  DISALLOW_COPY_AND_ASSIGN(ClientControlledStateDelegate);
};

// A WindowStateDelegate that implements ToggleFullscreen behavior for
// client controlled window.
class ClientControlledWindowStateDelegate
    : public ash::wm::WindowStateDelegate {
 public:
  explicit ClientControlledWindowStateDelegate(
      ClientControlledShellSurface* shell_surface,
      ash::wm::ClientControlledState::Delegate* delegate)
      : shell_surface_(shell_surface), delegate_(delegate) {}
  ~ClientControlledWindowStateDelegate() override {}

  // Overridden from ash::wm::WindowStateDelegate:
  bool ToggleFullscreen(ash::wm::WindowState* window_state) override {
    ash::mojom::WindowStateType next_state;
    aura::Window* window = window_state->window();
    switch (window_state->GetStateType()) {
      case ash::mojom::WindowStateType::DEFAULT:
      case ash::mojom::WindowStateType::NORMAL:
        window->SetProperty(aura::client::kPreFullscreenShowStateKey,
                            ui::SHOW_STATE_NORMAL);
        next_state = ash::mojom::WindowStateType::FULLSCREEN;
        break;
      case ash::mojom::WindowStateType::MAXIMIZED:
        window->SetProperty(aura::client::kPreFullscreenShowStateKey,
                            ui::SHOW_STATE_MAXIMIZED);
        next_state = ash::mojom::WindowStateType::FULLSCREEN;
        break;
      case ash::mojom::WindowStateType::FULLSCREEN:
        switch (window->GetProperty(aura::client::kPreFullscreenShowStateKey)) {
          case ui::SHOW_STATE_DEFAULT:
          case ui::SHOW_STATE_NORMAL:
            next_state = ash::mojom::WindowStateType::NORMAL;
            break;
          case ui::SHOW_STATE_MAXIMIZED:
            next_state = ash::mojom::WindowStateType::MAXIMIZED;
            break;
          case ui::SHOW_STATE_MINIMIZED:
            next_state = ash::mojom::WindowStateType::MINIMIZED;
            break;
          case ui::SHOW_STATE_FULLSCREEN:
          case ui::SHOW_STATE_INACTIVE:
          case ui::SHOW_STATE_END:
            NOTREACHED() << " unknown state :"
                         << window->GetProperty(
                                aura::client::kPreFullscreenShowStateKey);
            return false;
        }
        break;
      case ash::mojom::WindowStateType::MINIMIZED: {
        ui::WindowShowState pre_full_state =
            window->GetProperty(aura::client::kPreMinimizedShowStateKey);
        if (pre_full_state != ui::SHOW_STATE_FULLSCREEN) {
          window->SetProperty(aura::client::kPreFullscreenShowStateKey,
                              pre_full_state);
        }
        next_state = ash::mojom::WindowStateType::FULLSCREEN;
        break;
      }
      default:
        // TODO(oshima|xdai): Handle SNAP state.
        return false;
    }
    delegate_->HandleWindowStateRequest(window_state, next_state);
    return true;
  }

  void OnDragStarted(int component) override {
    shell_surface_->OnDragStarted(component);
  }

  void OnDragFinished(bool canceled, const gfx::Point& location) override {
    shell_surface_->OnDragFinished(canceled, location);
  }

 private:
  ClientControlledShellSurface* shell_surface_;
  ash::wm::ClientControlledState::Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(ClientControlledWindowStateDelegate);
};

bool IsPinned(const ash::wm::WindowState* window_state) {
  return window_state->IsPinned() || window_state->IsTrustedPinned();
}

class CaptionButtonModel : public ash::CaptionButtonModel {
 public:
  CaptionButtonModel(uint32_t visible_button_mask, uint32_t enabled_button_mask)
      : visible_button_mask_(visible_button_mask),
        enabled_button_mask_(enabled_button_mask) {}

  // Overridden from ash::CaptionButtonModel:
  bool IsVisible(ash::CaptionButtonIcon icon) const override {
    return visible_button_mask_ & (1 << icon);
  }
  bool IsEnabled(ash::CaptionButtonIcon icon) const override {
    return enabled_button_mask_ & (1 << icon);
  }
  bool InZoomMode() const override {
    return visible_button_mask_ & (1 << ash::CAPTION_BUTTON_ICON_ZOOM);
  }

 private:
  uint32_t visible_button_mask_;
  uint32_t enabled_button_mask_;

  DISALLOW_COPY_AND_ASSIGN(CaptionButtonModel);
};

}  // namespace

class ClientControlledShellSurface::ScopedSetBoundsLocally {
 public:
  explicit ScopedSetBoundsLocally(ClientControlledShellSurface* shell_surface)
      : state_(shell_surface->client_controlled_state_) {
    state_->set_bounds_locally(true);
  }
  ~ScopedSetBoundsLocally() { state_->set_bounds_locally(false); }

 private:
  ash::wm::ClientControlledState* const state_;

  DISALLOW_COPY_AND_ASSIGN(ScopedSetBoundsLocally);
};

class ClientControlledShellSurface::ScopedLockedToRoot {
 public:
  explicit ScopedLockedToRoot(views::Widget* widget)
      : window_(widget->GetNativeWindow()) {
    window_->SetProperty(ash::kLockedToRootKey, true);
  }
  ~ScopedLockedToRoot() { window_->ClearProperty(ash::kLockedToRootKey); }

 private:
  aura::Window* const window_;

  DISALLOW_COPY_AND_ASSIGN(ScopedLockedToRoot);
};

////////////////////////////////////////////////////////////////////////////////
// ClientControlledShellSurface, public:

ClientControlledShellSurface::ClientControlledShellSurface(Surface* surface,
                                                           bool can_minimize,
                                                           int container)
    : ShellSurfaceBase(surface, gfx::Point(), true, can_minimize, container),
      primary_display_id_(
          display::Screen::GetScreen()->GetPrimaryDisplay().id()) {
  WMHelper::GetInstance()->AddDisplayConfigurationObserver(this);
  display::Screen::GetScreen()->AddObserver(this);
}

ClientControlledShellSurface::~ClientControlledShellSurface() {
  if (wide_frame_)
    wide_frame_->Close();

  WMHelper::GetInstance()->RemoveDisplayConfigurationObserver(this);
  display::Screen::GetScreen()->RemoveObserver(this);
}

void ClientControlledShellSurface::SetMaximized() {
  TRACE_EVENT0("exo", "ClientControlledShellSurface::SetMaximized");
  pending_window_state_ = ash::mojom::WindowStateType::MAXIMIZED;
}

void ClientControlledShellSurface::SetMinimized() {
  TRACE_EVENT0("exo", "ClientControlledShellSurface::SetMinimized");
  pending_window_state_ = ash::mojom::WindowStateType::MINIMIZED;
}

void ClientControlledShellSurface::SetRestored() {
  TRACE_EVENT0("exo", "ClientControlledShellSurface::SetRestored");
  pending_window_state_ = ash::mojom::WindowStateType::NORMAL;
}

void ClientControlledShellSurface::SetFullscreen(bool fullscreen) {
  TRACE_EVENT1("exo", "ClientControlledShellSurface::SetFullscreen",
               "fullscreen", fullscreen);
  pending_window_state_ = fullscreen ? ash::mojom::WindowStateType::FULLSCREEN
                                     : ash::mojom::WindowStateType::NORMAL;
}

void ClientControlledShellSurface::SetSnappedToLeft() {
  TRACE_EVENT0("exo", "ClientControlledShellSurface::SetSnappedToLeft");
  pending_window_state_ = ash::mojom::WindowStateType::LEFT_SNAPPED;
}

void ClientControlledShellSurface::SetSnappedToRight() {
  TRACE_EVENT0("exo", "ClientControlledShellSurface::SetSnappedToRight");
  pending_window_state_ = ash::mojom::WindowStateType::RIGHT_SNAPPED;
}

void ClientControlledShellSurface::SetPip() {
  TRACE_EVENT0("exo", "ClientControlledShellSurface::SetPip");
  pending_window_state_ = ash::mojom::WindowStateType::PIP;
}

void ClientControlledShellSurface::SetPinned(ash::mojom::WindowPinType type) {
  TRACE_EVENT1("exo", "ClientControlledShellSurface::SetPinned", "type",
               static_cast<int>(type));

  if (!widget_)
    CreateShellSurfaceWidget(ui::SHOW_STATE_NORMAL);

  // Note: This will ask client to configure its surface even if pinned
  // state doesn't change.
  ScopedConfigure scoped_configure(this, true);
  widget_->GetNativeWindow()->SetProperty(ash::kWindowPinTypeKey, type);
}

void ClientControlledShellSurface::SetSystemUiVisibility(bool autohide) {
  TRACE_EVENT1("exo", "ClientControlledShellSurface::SetSystemUiVisibility",
               "autohide", autohide);

  if (!widget_)
    CreateShellSurfaceWidget(ui::SHOW_STATE_NORMAL);

  ash::wm::SetAutoHideShelf(widget_->GetNativeWindow(), autohide);
}

void ClientControlledShellSurface::SetAlwaysOnTop(bool always_on_top) {
  TRACE_EVENT1("exo", "ClientControlledShellSurface::SetAlwaysOnTop",
               "always_on_top", always_on_top);

  if (!widget_)
    CreateShellSurfaceWidget(ui::SHOW_STATE_NORMAL);

  widget_->GetNativeWindow()->SetProperty(aura::client::kAlwaysOnTopKey,
                                          always_on_top);
}

void ClientControlledShellSurface::SetOrientation(Orientation orientation) {
  TRACE_EVENT1("exo", "ClientControlledShellSurface::SetOrientation",
               "orientation",
               orientation == Orientation::PORTRAIT ? "portrait" : "landscape");
  pending_orientation_ = orientation;
}

void ClientControlledShellSurface::SetShadowBounds(const gfx::Rect& bounds) {
  TRACE_EVENT1("exo", "ClientControlledShellSurface::SetShadowBounds", "bounds",
               bounds.ToString());
  auto shadow_bounds =
      bounds.IsEmpty() ? base::nullopt : base::make_optional(bounds);
  if (shadow_bounds_ != shadow_bounds) {
    shadow_bounds_ = shadow_bounds;
    shadow_bounds_changed_ = true;
  }
}

void ClientControlledShellSurface::SetScale(double scale) {
  TRACE_EVENT1("exo", "ClientControlledShellSurface::SetScale", "scale", scale);

  if (scale <= 0.0) {
    DLOG(WARNING) << "Surface scale must be greater than 0";
    return;
  }

  pending_scale_ = scale;
}

void ClientControlledShellSurface::SetTopInset(int height) {
  TRACE_EVENT1("exo", "ClientControlledShellSurface::SetTopInset", "height",
               height);

  pending_top_inset_height_ = height;
}

void ClientControlledShellSurface::SetResizeOutset(int outset) {
  TRACE_EVENT1("exo", "ClientControlledShellSurface::SetResizeOutset", "outset",
               outset);
  if (client_controlled_move_resize_) {
    if (root_surface())
      root_surface()->SetInputOutset(outset);
  }
}

void ClientControlledShellSurface::OnWindowStateChangeEvent(
    ash::mojom::WindowStateType current_state,
    ash::mojom::WindowStateType next_state) {
  if (state_changed_callback_)
    state_changed_callback_.Run(current_state, next_state);
}

void ClientControlledShellSurface::StartDrag(int component,
                                             const gfx::Point& location) {
  TRACE_EVENT2("exo", "ClientControlledShellSurface::StartResize", "component",
               component, "location", location.ToString());

  if (!widget_ || (client_controlled_move_resize_ && component != HTCAPTION))
    return;
  AttemptToStartDrag(component, location);
}

void ClientControlledShellSurface::AttemptToStartDrag(
    int component,
    const gfx::Point& location) {
  aura::Window* target = widget_->GetNativeWindow();
  ash::ToplevelWindowEventHandler* toplevel_handler =
      ash::Shell::Get()->toplevel_window_event_handler();
  aura::Window* mouse_pressed_handler =
      target->GetHost()->dispatcher()->mouse_pressed_handler();
  // Start dragging only if ...
  // 1) touch guesture is in progres.
  // 2) mouse was pressed on the target or its subsurfaces.
  if (toplevel_handler->gesture_target() ||
      (mouse_pressed_handler && target->Contains(mouse_pressed_handler))) {
    gfx::Point point_in_root(location);
    wm::ConvertPointFromScreen(target->GetRootWindow(), &point_in_root);
    toplevel_handler->AttemptToStartDrag(
        target, point_in_root, component,
        ash::wm::WmToplevelWindowEventHandler::EndClosure());
  }
}

void ClientControlledShellSurface::SetCanMaximize(bool can_maximize) {
  TRACE_EVENT1("exo", "ClientControlledShellSurface::SetCanMaximize",
               "can_maximzie", can_maximize);
  can_maximize_ = can_maximize;
  if (widget_)
    widget_->OnSizeConstraintsChanged();
}

void ClientControlledShellSurface::UpdateAutoHideFrame() {
  if (immersive_fullscreen_controller_) {
    bool enabled = (frame_type_ == SurfaceFrameType::AUTOHIDE &&
                    GetWindowState()->IsMaximizedOrFullscreenOrPinned());
    immersive_fullscreen_controller_->SetEnabled(
        ash::ImmersiveFullscreenController::WINDOW_TYPE_OTHER, enabled);
  }
}

void ClientControlledShellSurface::SetFrameButtons(
    uint32_t visible_button_mask,
    uint32_t enabled_button_mask) {
  if (frame_visible_button_mask_ == visible_button_mask &&
      frame_enabled_button_mask_ == enabled_button_mask) {
    return;
  }
  frame_visible_button_mask_ = visible_button_mask;
  frame_enabled_button_mask_ = enabled_button_mask;

  if (widget_)
    UpdateCaptionButtonModel();
}

void ClientControlledShellSurface::SetExtraTitle(
    const base::string16& extra_title) {
  TRACE_EVENT1("exo", "ClientControlledShellSurface::SetExtraTitle",
               "extra_title", base::UTF16ToUTF8(extra_title));
  // The extra title is used in the window frame.
  extra_title_ = extra_title;
  if (widget_)
    widget_->UpdateWindowTitle();
}

void ClientControlledShellSurface::SetOrientationLock(
    ash::OrientationLockType orientation_lock) {
  TRACE_EVENT1("exo", "ClientControlledShellSurface::SetOrientationLock",
               "orientation_lock", static_cast<int>(orientation_lock));

  if (!widget_) {
    initial_orientation_lock_ = orientation_lock;
    return;
  }

  ash::Shell* shell = ash::Shell::Get();
  shell->screen_orientation_controller()->LockOrientationForWindow(
      widget_->GetNativeWindow(), orientation_lock);
}

void ClientControlledShellSurface::OnBoundsChangeEvent(
    ash::mojom::WindowStateType current_state,
    ash::mojom::WindowStateType requested_state,
    int64_t display_id,
    const gfx::Rect& window_bounds,
    int bounds_change) {
  // Do no update the bounds unless we have geometry from client.
  if (!geometry().IsEmpty() && !window_bounds.IsEmpty() &&
      bounds_changed_callback_) {
    // Sends the client bounds, which matches the geometry
    // when frame is enabled.
    ash::CustomFrameViewAsh* frame_view = GetFrameView();

    // The client's geometry uses fullscreen in client controlled,
    // (but the surface is placed under the frame), so just use
    // the window bounds instead for maximixed stte.
    gfx::Rect client_bounds =
        widget_->IsMaximized()
            ? window_bounds
            : frame_view->GetClientBoundsForWindowBounds(window_bounds);
    gfx::Size current_size = frame_view->GetBoundsForClientView().size();
    bool is_resize = client_bounds.size() != current_size;
    bounds_changed_callback_.Run(current_state, requested_state, display_id,
                                 client_bounds, is_resize, bounds_change);
  }
}

void ClientControlledShellSurface::OnDragStarted(int component) {
  if (drag_started_callback_)
    drag_started_callback_.Run(component);
}

void ClientControlledShellSurface::OnDragFinished(bool canceled,
                                                  const gfx::Point& location) {
  if (drag_finished_callback_)
    drag_finished_callback_.Run(location.x(), location.y(), canceled);
}
////////////////////////////////////////////////////////////////////////////////
// SurfaceDelegate overrides:

void ClientControlledShellSurface::OnSurfaceCommit() {
  if (!widget_) {
    // Modify the |origin_| to the |pending_geometry_| to place the window
    // on the intended display. See b/77472684 for the details.
    if (!pending_geometry_.IsEmpty())
      origin_ = pending_geometry_.origin();
    CreateShellSurfaceWidget(ash::ToWindowShowState(pending_window_state_));
  }

  ash::wm::WindowState* window_state = GetWindowState();
  if (window_state->GetStateType() != pending_window_state_) {
    if (!IsPinned(window_state)) {
      ash::wm::ClientControlledState::BoundsChangeAnimationType animation_type =
          ash::wm::ClientControlledState::kAnimationNone;
      switch (pending_window_state_) {
        case ash::mojom::WindowStateType::NORMAL:
          if (widget_->IsMaximized() || widget_->IsFullscreen()) {
            animation_type =
                ash::wm::ClientControlledState::kAnimationCrossFade;
          }
          break;

        case ash::mojom::WindowStateType::MAXIMIZED:
        case ash::mojom::WindowStateType::FULLSCREEN:
          animation_type = ash::wm::ClientControlledState::kAnimationCrossFade;
          break;

        default:
          break;
      }
      client_controlled_state_->EnterNextState(
          window_state, pending_window_state_, animation_type);
    } else {
      VLOG(1) << "State change was requested while it is pinned";
    }
  }

  ShellSurfaceBase::OnSurfaceCommit();
  UpdateFrame();
  UpdateBackdrop();

  if (geometry_changed_callback_)
    geometry_changed_callback_.Run(GetVisibleBounds());

  // Apply new top inset height.
  if (pending_top_inset_height_ != top_inset_height_) {
    widget_->GetNativeWindow()->SetProperty(aura::client::kTopViewInset,
                                            pending_top_inset_height_);
    top_inset_height_ = pending_top_inset_height_;
  }

  // Update surface scale.
  if (pending_scale_ != scale_) {
    gfx::Transform transform;
    DCHECK_NE(pending_scale_, 0.0);
    transform.Scale(1.0 / pending_scale_, 1.0 / pending_scale_);
    host_window()->SetTransform(transform);
    scale_ = pending_scale_;
  }

  orientation_ = pending_orientation_;
  if (expected_orientation_ == orientation_)
    orientation_compositor_lock_.reset();
}

bool ClientControlledShellSurface::IsInputEnabled(Surface* surface) const {
  // Client-driven dragging/resizing relies on implicit grab, which ensures that
  // mouse/touch events are delivered to the focused surface until release, even
  // if they fall outside surface bounds. However, if the client destroys the
  // surface with implicit grab, the drag/resize is prematurely ended. Prevent
  // this by delivering all input events to the root surface, which shares the
  // lifetime of the shell surface.
  // TODO(domlaskowski): Remove once the client is provided with an API to hook
  // into server-driven dragging/resizing.
  return surface == root_surface();
}

void ClientControlledShellSurface::OnSetFrame(SurfaceFrameType type) {
  ShellSurfaceBase::OnSetFrame(type);
  frame_type_ = type;
  UpdateAutoHideFrame();
}

void ClientControlledShellSurface::OnSetFrameColors(SkColor active_color,
                                                    SkColor inactive_color) {
  ShellSurfaceBase::OnSetFrameColors(active_color, inactive_color);
  if (wide_frame_) {
    aura::Window* window = wide_frame_->GetWidget()->GetNativeWindow();
    window->SetProperty(ash::kFrameActiveColorKey, active_color);
    window->SetProperty(ash::kFrameInactiveColorKey, inactive_color);
  }
}

////////////////////////////////////////////////////////////////////////////////
// aura::WindowObserver overrides:
void ClientControlledShellSurface::OnWindowBoundsChanged(
    aura::Window* window,
    const gfx::Rect& old_bounds,
    const gfx::Rect& new_bounds,
    ui::PropertyChangeReason reason) {}

void ClientControlledShellSurface::OnWindowAddedToRootWindow(
    aura::Window* window) {
  ScopedLockedToRoot scoped_locked_to_root(widget_);
  UpdateWidgetBounds();
}

////////////////////////////////////////////////////////////////////////////////
// views::WidgetDelegate overrides:

bool ClientControlledShellSurface::CanMaximize() const {
  return can_maximize_;
}

views::NonClientFrameView*
ClientControlledShellSurface::CreateNonClientFrameView(views::Widget* widget) {
  ash::wm::WindowState* window_state = GetWindowState();
  std::unique_ptr<ash::wm::ClientControlledState::Delegate> delegate =
      GetFactoryForTesting()
          ? GetFactoryForTesting().Run()
          : std::make_unique<ClientControlledStateDelegate>(this);

  auto window_delegate = std::make_unique<ClientControlledWindowStateDelegate>(
      this, delegate.get());
  auto state =
      std::make_unique<ash::wm::ClientControlledState>(std::move(delegate));
  client_controlled_state_ = state.get();
  window_state->SetStateObject(std::move(state));
  window_state->SetDelegate(std::move(window_delegate));
  ash::CustomFrameViewAsh* frame_view = static_cast<ash::CustomFrameViewAsh*>(
      ShellSurfaceBase::CreateNonClientFrameView(widget));
  immersive_fullscreen_controller_ =
      std::make_unique<ash::ImmersiveFullscreenController>();
  frame_view->InitImmersiveFullscreenControllerForView(
      immersive_fullscreen_controller_.get());
  return frame_view;
}

void ClientControlledShellSurface::SaveWindowPlacement(
    const gfx::Rect& bounds,
    ui::WindowShowState show_state) {}

bool ClientControlledShellSurface::GetSavedWindowPlacement(
    const views::Widget* widget,
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// views::View overrides:

gfx::Size ClientControlledShellSurface::GetMaximumSize() const {
  // On ChromeOS, a window with non empty maximum size is non-maximizable,
  // even if CanMaximize() returns true. ClientControlledShellSurface
  // sololy depends on |can_maximize_| to determine if it is maximizable,
  // so just return empty size because the maximum size in
  // ClientControlledShellSurface is used only to tell the resizability,
  // but not real maximum size.
  return gfx::Size();
}

void ClientControlledShellSurface::OnDeviceScaleFactorChanged(float old_dsf,
                                                              float new_dsf) {
  views::View::OnDeviceScaleFactorChanged(old_dsf, new_dsf);
  UpdateFrameWidth();
}

////////////////////////////////////////////////////////////////////////////////
// display::DisplayObserver overrides:

void ClientControlledShellSurface::OnDisplayMetricsChanged(
    const display::Display& new_display,
    uint32_t changed_metrics) {
  if (!widget_ || !widget_->IsActive() ||
      !WMHelper::GetInstance()->IsTabletModeWindowManagerEnabled()) {
    return;
  }

  const display::Screen* screen = display::Screen::GetScreen();
  display::Display current_display =
      screen->GetDisplayNearestWindow(widget_->GetNativeWindow());
  if (current_display.id() != new_display.id() ||
      !(changed_metrics & display::DisplayObserver::DISPLAY_METRIC_ROTATION)) {
    return;
  }

  Orientation target_orientation = SizeToOrientation(new_display.size());
  if (orientation_ == target_orientation)
    return;
  expected_orientation_ = target_orientation;
  EnsureCompositorIsLockedForOrientationChange();
}

////////////////////////////////////////////////////////////////////////////////
// ash::WindowTreeHostManager::Observer overrides:

void ClientControlledShellSurface::OnDisplayConfigurationChanged() {
  const display::Screen* screen = display::Screen::GetScreen();
  int64_t primary_display_id = screen->GetPrimaryDisplay().id();
  if (primary_display_id == primary_display_id_)
    return;

  display::Display old_primary_display;
  if (screen->GetDisplayWithDisplayId(primary_display_id_,
                                      &old_primary_display)) {
    // Give the client a chance to adjust window positions before switching to
    // the new coordinate system. Retain the old origin by reverting the origin
    // delta until the next configure is acknowledged.
    gfx::Vector2d delta = gfx::Point() - old_primary_display.bounds().origin();
    origin_offset_ -= delta;
    pending_origin_offset_accumulator_ += delta;

    if (widget_) {
      UpdateWidgetBounds();
      UpdateShadow();
    }

    Configure();
  }

  primary_display_id_ = primary_display_id;
}

////////////////////////////////////////////////////////////////////////////////
// ui::CompositorLockClient overrides:

void ClientControlledShellSurface::CompositorLockTimedOut() {
  orientation_compositor_lock_.reset();
}

////////////////////////////////////////////////////////////////////////////////
// ShellSurface overrides:

void ClientControlledShellSurface::SetWidgetBounds(const gfx::Rect& bounds) {
  if ((!client_controlled_move_resize_ && !GetWindowState()->is_dragged()) ||
      (client_controlled_move_resize_ &&
       (!resizer_ || resizer_->details().window_component != HTCAPTION))) {
    {
      // Calculate a minimum window visibility required bounds.
      aura::Window* window = widget_->GetNativeWindow();
      gfx::Rect root_rect(bounds);
      wm::ConvertRectFromScreen(window->GetRootWindow(), &root_rect);
      ash::wm::ClientControlledState::AdjustBoundsForMinimumWindowVisibility(
          window, &root_rect);
      gfx::Rect screen_rect(root_rect);
      wm::ConvertRectToScreen(window->GetRootWindow(), &screen_rect);

      if (bounds != screen_rect) {
        {
          ScopedSetBoundsLocally scoped_set_bounds(this);
          window->SetBounds(root_rect);
        }
        // Request the client a new bounds to ensure that it has enough visible
        // area.
        auto state_type = GetWindowState()->GetStateType();
        int64_t display_id =
            display::Screen::GetScreen()
                ->GetDisplayNearestWindow(window->GetRootWindow())
                .id();
        OnBoundsChangeEvent(state_type, state_type, display_id, screen_rect, 0);
      } else {
        ScopedSetBoundsLocally scoped_set_bounds(this);
        widget_->SetBounds(bounds);
      }
    }
    UpdateSurfaceBounds();
    return;
  }

  // TODO(domlaskowski): Synchronize window state transitions with the client,
  // and abort client-side dragging on transition to fullscreen.
  // See crbug.com/699746.
  DLOG_IF(ERROR, widget_->GetWindowBoundsInScreen().size() != bounds.size())
      << "Window size changed during client-driven drag";

  // Convert from screen to display coordinates.
  gfx::Point origin = bounds.origin();
  wm::ConvertPointFromScreen(widget_->GetNativeWindow()->parent(), &origin);

  // Move the window relative to the current display.
  {
    ScopedSetBoundsLocally scoped_set_bounds(this);
    widget_->GetNativeWindow()->SetBounds(gfx::Rect(origin, bounds.size()));
  }
  UpdateSurfaceBounds();

  // Render phantom windows when beyond the current display.
  if (resizer_)
    resizer_->Drag(GetMouseLocation(), 0);
}

gfx::Rect ClientControlledShellSurface::GetShadowBounds() const {
  gfx::Rect shadow_bounds = ShellSurfaceBase::GetShadowBounds();
  const ash::CustomFrameViewAsh* frame_view = GetFrameView();
  if (frame_view->visible()) {
    // The client controlled geometry is only for the client
    // area. When the chrome side frame is enabled, the shadow height
    // has to include the height of the frame, and the total height is
    // equals to the window height computed by
    // |GetWindowBoundsForClientBounds|.
    shadow_bounds.set_size(
        frame_view->GetWindowBoundsForClientBounds(shadow_bounds).size());
  }

  if (geometry_changed_callback_.is_null()) {
    aura::Window* window = widget_->GetNativeWindow();

    // Convert from screen to display coordinates.
    shadow_bounds -= origin_offset_;
    wm::ConvertRectFromScreen(window->parent(), &shadow_bounds);

    // Convert from display to window coordinates.
    shadow_bounds -= window->bounds().OffsetFromOrigin();
  }

  return shadow_bounds;
}

void ClientControlledShellSurface::InitializeWindowState(
    ash::wm::WindowState* window_state) {
  // Allow the client to request bounds that do not fill the entire work area
  // when maximized, or the entire display when fullscreen.
  window_state->set_allow_set_bounds_direct(true);
  window_state->set_ignore_keyboard_bounds_change(true);
  if (container_ == ash::kShellWindowId_SystemModalContainer)
    DisableMovement();
  ash::CustomFrameViewAsh* frame_view = GetFrameView();
  frame_view->SetCaptionButtonModel(std::make_unique<CaptionButtonModel>(
      frame_visible_button_mask_, frame_enabled_button_mask_));
  UpdateAutoHideFrame();
  UpdateFrameWidth();
  if (initial_orientation_lock_ != ash::OrientationLockType::kAny)
    SetOrientationLock(initial_orientation_lock_);
}

float ClientControlledShellSurface::GetScale() const {
  return scale_;
}

aura::Window* ClientControlledShellSurface::GetDragWindow() {
  // Set capture on the root surface rather than the focus surface, because
  // the client may destroy the latter during dragging/resizing.
  return root_surface() ? root_surface()->window() : nullptr;
}

std::unique_ptr<ash::WindowResizer>
ClientControlledShellSurface::CreateWindowResizer(aura::Window* window,
                                                  int component) {
  ash::wm::WindowState* window_state = GetWindowState();
  DCHECK(!window_state->drag_details());
  window_state->CreateDragDetails(GetMouseLocation(), component,
                                  wm::WINDOW_MOVE_SOURCE_MOUSE);

  std::unique_ptr<ash::WindowResizer> resizer =
      std::make_unique<CustomWindowResizer>(window_state);

  if (component == HTCAPTION) {
    // Chained with a CustomWindowResizer, DragWindowResizer does not handle
    // dragging. It only renders phantom windows and moves the window to the
    // target root window when dragging ends.
    resizer.reset(
        ash::DragWindowResizer::Create(resizer.release(), window_state));
  }

  return resizer;
}

bool ClientControlledShellSurface::OnMouseDragged(const ui::MouseEvent&) {
  // TODO(domlaskowski): When VKEY_ESCAPE is pressed during dragging, the client
  // destroys the window, but should instead revert the drag to be consistent
  // with ShellSurface::OnKeyEvent. See crbug.com/699746.
  return false;
}

gfx::Rect ClientControlledShellSurface::GetWidgetBounds() const {
  const ash::CustomFrameViewAsh* frame_view = GetFrameView();
  if (frame_view->visible()) {
    // The client's geometry uses entire display area in client
    // controlled in maximized, and the surface is placed under the
    // frame. Just use the visible bounds (geometry) for the widget
    // bounds.
    if (widget_->IsMaximized())
      return GetVisibleBounds();
    return frame_view->GetWindowBoundsForClientBounds(GetVisibleBounds());
  }

  gfx::Rect bounds(GetVisibleBounds());
  bounds.Offset(-origin_offset_.x(), -origin_offset_.y());
  return bounds;
}

gfx::Point ClientControlledShellSurface::GetSurfaceOrigin() const {
  DCHECK(resize_component_ == HTCAPTION);
  if (geometry_changed_callback_)
    return gfx::Point();
  // TODO(oshima): geometry_changed_callback_ must be always set by now, so
  // this is not necessary any more. Remove this.
  return gfx::Point() -
         (GetVisibleBounds().origin() - origin_offset_).OffsetFromOrigin();
}

////////////////////////////////////////////////////////////////////////////////
// ClientControlledShellSurface, private:

void ClientControlledShellSurface::UpdateFrame() {
  if (!widget_ || !GetFrameView()->visible())
    return;
  gfx::Rect work_area =
      display::Screen::GetScreen()
          ->GetDisplayNearestWindow(widget_->GetNativeWindow())
          .work_area();

  ash::wm::WindowState* window_state = GetWindowState();
  if (window_state->IsMaximizedOrFullscreenOrPinned() &&
      work_area.width() != geometry().width()) {
    if (!wide_frame_) {
      wide_frame_ = ash::WideFrameView::Create(widget_);
      immersive_fullscreen_controller_->SetEnabled(
          ash::ImmersiveFullscreenController::WINDOW_TYPE_OTHER, false);
      wide_frame_->Init(immersive_fullscreen_controller_.get());
      wide_frame_->Show();
      UpdateCaptionButtonModel();
    }
  } else {
    if (wide_frame_) {
      wide_frame_->Close();
      wide_frame_ = nullptr;
      immersive_fullscreen_controller_->SetEnabled(
          ash::ImmersiveFullscreenController::WINDOW_TYPE_OTHER, false);
      GetFrameView()->InitImmersiveFullscreenControllerForView(
          immersive_fullscreen_controller_.get());
      UpdateCaptionButtonModel();
    }
    UpdateFrameWidth();
  }
  // The autohide should be applied when the window state is in
  // maximzied, fullscreen or pinned. Update the auto hide state
  // inside commit.
  UpdateAutoHideFrame();
}

void ClientControlledShellSurface::UpdateCaptionButtonModel() {
  auto model = std::make_unique<CaptionButtonModel>(frame_visible_button_mask_,
                                                    frame_enabled_button_mask_);
  if (wide_frame_)
    wide_frame_->SetCaptionButtonModel(std::move(model));
  else
    GetFrameView()->SetCaptionButtonModel(std::move(model));
}

void ClientControlledShellSurface::UpdateBackdrop() {
  aura::Window* window = widget_->GetNativeWindow();
  const display::Display display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(window);
  bool enable_backdrop =
      (widget_->IsFullscreen() || widget_->IsMaximized()) &&
      !widget_->GetWindowBoundsInScreen().Contains(display.work_area());
  ash::BackdropWindowMode target_backdrop_mode =
      enable_backdrop ? ash::BackdropWindowMode::kEnabled
                      : ash::BackdropWindowMode::kAuto;

  if (window->GetProperty(ash::kBackdropWindowMode) != target_backdrop_mode)
    window->SetProperty(ash::kBackdropWindowMode, target_backdrop_mode);
}

void ClientControlledShellSurface::UpdateFrameWidth() {
  int width = -1;
  if (shadow_bounds_) {
    float device_scale_factor =
        GetWidget()->GetNativeWindow()->layer()->device_scale_factor();
    float dsf_to_default_dsf = device_scale_factor / scale_;
    width = gfx::ToRoundedInt(shadow_bounds_->width() * dsf_to_default_dsf);
  }
  static_cast<ash::HeaderView*>(GetFrameView()->GetHeaderView())
      ->SetWidthInPixels(width);
}

void ClientControlledShellSurface::
    EnsureCompositorIsLockedForOrientationChange() {
  if (!orientation_compositor_lock_) {
    ui::Compositor* compositor =
        widget_->GetNativeWindow()->layer()->GetCompositor();
    orientation_compositor_lock_ = compositor->GetCompositorLock(
        this, base::TimeDelta::FromMilliseconds(kOrientationLockTimeoutMs));
  }
}

ash::wm::WindowState* ClientControlledShellSurface::GetWindowState() {
  return ash::wm::GetWindowState(widget_->GetNativeWindow());
}

ash::CustomFrameViewAsh* ClientControlledShellSurface::GetFrameView() {
  return static_cast<ash::CustomFrameViewAsh*>(
      widget_->non_client_view()->frame_view());
}

const ash::CustomFrameViewAsh* ClientControlledShellSurface::GetFrameView()
    const {
  return static_cast<const ash::CustomFrameViewAsh*>(
      widget_->non_client_view()->frame_view());
}

// static
void ClientControlledShellSurface::
    SetClientControlledStateDelegateFactoryForTest(
        const DelegateFactoryCallback& callback) {
  auto& factory = GetFactoryForTesting();
  factory = callback;
}

}  // namespace exo
