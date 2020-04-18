// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_CLIENT_CONTROLLED_SHELL_SURFACE_H_
#define COMPONENTS_EXO_CLIENT_CONTROLLED_SHELL_SURFACE_H_

#include <memory>
#include <string>

#include "ash/display/screen_orientation_controller.h"
#include "ash/display/window_tree_host_manager.h"
#include "ash/wm/client_controlled_state.h"
#include "base/callback.h"
#include "base/macros.h"
#include "components/exo/shell_surface_base.h"
#include "ui/base/hit_test.h"
#include "ui/compositor/compositor_lock.h"
#include "ui/display/display_observer.h"

namespace ash {
class CustomFrameViewAsh;
class ImmersiveFullscreenController;
class WideFrameView;

namespace mojom {
enum class WindowPinType;
}
}  // namespace ash

namespace exo {
class Surface;

enum class Orientation { PORTRAIT, LANDSCAPE };

// This class implements a ShellSurface whose window state and bounds are
// controlled by a remote shell client rather than the window manager. The
// position specified as part of the geometry is relative to the origin of
// the screen coordinate system.
class ClientControlledShellSurface
    : public ShellSurfaceBase,
      public display::DisplayObserver,
      public ash::WindowTreeHostManager::Observer,
      public ui::CompositorLockClient {
 public:
  using GeometryChangedCallback =
      base::RepeatingCallback<void(const gfx::Rect& geometry)>;

  ClientControlledShellSurface(Surface* surface,
                               bool can_minimize,
                               int container);
  ~ClientControlledShellSurface() override;

  void set_geometry_changed_callback(const GeometryChangedCallback& callback) {
    geometry_changed_callback_ = callback;
  }

  void set_client_controlled_move_resize(bool client_controlled_move_resize) {
    client_controlled_move_resize_ = client_controlled_move_resize;
  }

  // Called when the client was maximized.
  void SetMaximized();

  // Called when the client was minimized.
  void SetMinimized();

  // Called when the client was restored.
  void SetRestored();

  // Called when the client changed the fullscreen state.
  void SetFullscreen(bool fullscreen);

  // Called when the client was snapped to left.
  void SetSnappedToLeft();

  // Called when the client was snapped to right.
  void SetSnappedToRight();

  // Called when the client was set to PIP.
  void SetPip();

  // Set the callback to run when the surface state changed.
  using StateChangedCallback =
      base::RepeatingCallback<void(ash::mojom::WindowStateType old_state_type,
                                   ash::mojom::WindowStateType new_state_type)>;
  void set_state_changed_callback(
      const StateChangedCallback& state_changed_callback) {
    state_changed_callback_ = state_changed_callback;
  }

  // Set the callback to run when the surface bounds changed.
  using BoundsChangedCallback = base::RepeatingCallback<void(
      ash::mojom::WindowStateType current_state,
      ash::mojom::WindowStateType requested_state,
      int64_t display_id,
      const gfx::Rect& bounds,
      bool is_resize,
      int bounds_change)>;
  void set_bounds_changed_callback(
      const BoundsChangedCallback& bounds_changed_callback) {
    bounds_changed_callback_ = bounds_changed_callback;
  }

  // Set the callback to run when the drag operation started.
  using DragStartedCallback = base::RepeatingCallback<void(int direction)>;
  void set_drag_started_callback(const DragStartedCallback& callback) {
    drag_started_callback_ = callback;
  }

  // Set the callback to run when the drag operation finished.
  using DragFinishedCallback = base::RepeatingCallback<void(int, int, bool)>;
  void set_drag_finished_callback(const DragFinishedCallback& callback) {
    drag_finished_callback_ = callback;
  }

  // Pin/unpin the surface. Pinned surface cannot be switched to
  // other windows unless its explicitly unpinned.
  void SetPinned(ash::mojom::WindowPinType type);

  // Sets the surface to be on top of all other windows.
  void SetAlwaysOnTop(bool always_on_top);

  // Controls the visibility of the system UI when this surface is active.
  void SetSystemUiVisibility(bool autohide);

  // Set orientation for surface.
  void SetOrientation(Orientation orientation);

  // Set shadow bounds in surface coordinates. Empty bounds disable the shadow.
  void SetShadowBounds(const gfx::Rect& bounds);

  void SetScale(double scale);

  // Set top inset for surface.
  void SetTopInset(int height);

  // Set resize outset for surface.
  void SetResizeOutset(int outset);

  // Sends the window state change event to client.
  void OnWindowStateChangeEvent(ash::mojom::WindowStateType old_state,
                                ash::mojom::WindowStateType next_state);

  // Sends the window bounds change event to client. |display_id| specifies in
  // which display the surface should live in. |drag_bounds_change| is
  // a masked value of ash::WindowResizer::kBoundsChange_Xxx, and specifies
  // how the bounds was changed. The bounds change event may also come from a
  // snapped window state change |requested_state|.
  void OnBoundsChangeEvent(ash::mojom::WindowStateType current_state,
                           ash::mojom::WindowStateType requested_state,
                           int64_t display_id,
                           const gfx::Rect& bounds,
                           int drag_bounds_change);

  // Sends the window drag events to client.
  void OnDragStarted(int component);
  void OnDragFinished(bool cancel, const gfx::Point& location);

  // Starts the drag operation.
  void StartDrag(int component, const gfx::Point& location);

  // Set if the surface can be maximzied.
  void SetCanMaximize(bool can_maximize);

  // Update the auto hide frame state.
  void UpdateAutoHideFrame();

  // Set the frame button state. The |visible_button_mask| and
  // |enabled_button_mask| is a bit mask whose position is defined
  // in ash::CaptionButtonIcon enum.
  void SetFrameButtons(uint32_t frame_visible_button_mask,
                       uint32_t frame_enabled_button_mask);

  // Set the extra title for the surface.
  void SetExtraTitle(const base::string16& extra_title);

  // Set specific orientation lock for this surface. When this surface is in
  // foreground and the display can be rotated (e.g. tablet mode), apply the
  // behavior defined by |orientation_lock|. See more details in
  // //ash/display/screen_orientation_controller.h.
  void SetOrientationLock(ash::OrientationLockType orientation_lock);

  // Overridden from SurfaceDelegate:
  void OnSurfaceCommit() override;
  bool IsInputEnabled(Surface* surface) const override;
  void OnSetFrame(SurfaceFrameType type) override;
  void OnSetFrameColors(SkColor active_color, SkColor inactive_color) override;

  // Overridden from views::WidgetDelegate:
  bool CanMaximize() const override;
  views::NonClientFrameView* CreateNonClientFrameView(
      views::Widget* widget) override;
  void SaveWindowPlacement(const gfx::Rect& bounds,
                           ui::WindowShowState show_state) override;
  bool GetSavedWindowPlacement(const views::Widget* widget,
                               gfx::Rect* bounds,
                               ui::WindowShowState* show_state) const override;

  // Overridden from views::View:
  gfx::Size GetMaximumSize() const override;
  void OnDeviceScaleFactorChanged(float old_dsf, float new_dsf) override;

  // Overridden from aura::WindowObserver:
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds,
                             ui::PropertyChangeReason reason) override;
  void OnWindowAddedToRootWindow(aura::Window* window) override;

  // Overridden from display::DisplayObserver:
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t changed_metrics) override;

  // Overridden from ash::WindowTreeHostManager::Observer:
  void OnDisplayConfigurationChanged() override;

  // Overridden from ui::CompositorLockClient:
  void CompositorLockTimedOut() override;

  // A factory callback to create ClientControlledState::Delegate.
  using DelegateFactoryCallback = base::RepeatingCallback<
      std::unique_ptr<ash::wm::ClientControlledState::Delegate>(void)>;

  // Set the factory callback for unit test.
  static void SetClientControlledStateDelegateFactoryForTest(
      const DelegateFactoryCallback& callback);

  ash::WideFrameView* wide_frame_for_test() { return wide_frame_; }

 private:
  class ScopedSetBoundsLocally;
  class ScopedLockedToRoot;

  // Overridden from ShellSurface:
  void SetWidgetBounds(const gfx::Rect& bounds) override;
  gfx::Rect GetShadowBounds() const override;
  void InitializeWindowState(ash::wm::WindowState* window_state) override;
  float GetScale() const override;
  aura::Window* GetDragWindow() override;
  std::unique_ptr<ash::WindowResizer> CreateWindowResizer(
      aura::Window* window,
      int component) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  gfx::Rect GetWidgetBounds() const override;
  gfx::Point GetSurfaceOrigin() const override;

  // Update frame status. This may create (or destroy) a wide frame
  // that spans the full work area width if the surface didn't cover
  // the work area.
  void UpdateFrame();

  void UpdateCaptionButtonModel();

  void UpdateBackdrop();

  void UpdateFrameWidth();

  void AttemptToStartDrag(int component, const gfx::Point& location);

  // Lock the compositor if it's not already locked, or extends the
  // lock timeout if it's already locked.
  // TODO(reveman): Remove this when using configure callbacks for orientation.
  // crbug.com/765954
  void EnsureCompositorIsLockedForOrientationChange();

  ash::wm::WindowState* GetWindowState();
  ash::CustomFrameViewAsh* GetFrameView();
  const ash::CustomFrameViewAsh* GetFrameView() const;

  GeometryChangedCallback geometry_changed_callback_;
  int64_t primary_display_id_;

  int top_inset_height_ = 0;
  int pending_top_inset_height_ = 0;

  double scale_ = 1.0;
  double pending_scale_ = 1.0;

  uint32_t frame_visible_button_mask_ = 0;
  uint32_t frame_enabled_button_mask_ = 0;

  StateChangedCallback state_changed_callback_;
  BoundsChangedCallback bounds_changed_callback_;
  DragStartedCallback drag_started_callback_;
  DragFinishedCallback drag_finished_callback_;

  // TODO(reveman): Use configure callbacks for orientation. crbug.com/765954
  Orientation pending_orientation_ = Orientation::LANDSCAPE;
  Orientation orientation_ = Orientation::LANDSCAPE;
  Orientation expected_orientation_ = Orientation::LANDSCAPE;

  ash::wm::ClientControlledState* client_controlled_state_ = nullptr;

  ash::mojom::WindowStateType pending_window_state_ =
      ash::mojom::WindowStateType::NORMAL;

  bool can_maximize_ = true;

  std::unique_ptr<ash::ImmersiveFullscreenController>
      immersive_fullscreen_controller_;

  ash::WideFrameView* wide_frame_ = nullptr;

  std::unique_ptr<ui::CompositorLock> orientation_compositor_lock_;

  // The orientation to be applied when widget is being created. Only set when
  // widget is not created yet orientation lock is being set.
  ash::OrientationLockType initial_orientation_lock_ =
      ash::OrientationLockType::kAny;

  DISALLOW_COPY_AND_ASSIGN(ClientControlledShellSurface);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_CLIENT_CONTROLLED_SHELL_SURFACE_H_
