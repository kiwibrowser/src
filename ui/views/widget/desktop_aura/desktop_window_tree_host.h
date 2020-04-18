// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_WINDOW_TREE_HOST_H_
#define UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_WINDOW_TREE_HOST_H_

#include <memory>

#include "ui/aura/window_event_dispatcher.h"
#include "ui/base/ui_base_types.h"
#include "ui/views/views_export.h"
#include "ui/views/widget/widget.h"

namespace aura {
class WindowTreeHost;
class Window;

namespace client {
class DragDropClient;
}
}  // namespace aura

namespace gfx {
class ImageSkia;
class Rect;
}

namespace views {
namespace corewm {
class Tooltip;
}

namespace internal {
class NativeWidgetDelegate;
}

class DesktopNativeCursorManager;
class DesktopNativeWidgetAura;

class VIEWS_EXPORT DesktopWindowTreeHost {
 public:
  virtual ~DesktopWindowTreeHost() {}

  static DesktopWindowTreeHost* Create(
      internal::NativeWidgetDelegate* native_widget_delegate,
      DesktopNativeWidgetAura* desktop_native_widget_aura);

  // Sets up resources needed before the WindowEventDispatcher has been created.
  // It is expected this calls InitHost() on the WindowTreeHost.
  virtual void Init(const Widget::InitParams& params) = 0;

  // Invoked once the DesktopNativeWidgetAura has been created.
  virtual void OnNativeWidgetCreated(const Widget::InitParams& params) = 0;

  // Called from DesktopNativeWidgetAura::OnWidgetInitDone().
  virtual void OnWidgetInitDone() = 0;

  // Called from DesktopNativeWidgetAura::OnWindowActivated().
  virtual void OnActiveWindowChanged(bool active) = 0;

  // Creates and returns the Tooltip implementation to use. Return value is
  // owned by DesktopNativeWidgetAura and lives as long as
  // DesktopWindowTreeHost.
  virtual std::unique_ptr<corewm::Tooltip> CreateTooltip() = 0;

  // Creates and returns the DragDropClient implementation to use. Return value
  // is owned by DesktopNativeWidgetAura and lives as long as
  // DesktopWindowTreeHost.
  virtual std::unique_ptr<aura::client::DragDropClient> CreateDragDropClient(
      DesktopNativeCursorManager* cursor_manager) = 0;

  virtual void Close() = 0;
  virtual void CloseNow() = 0;

  virtual aura::WindowTreeHost* AsWindowTreeHost() = 0;

  virtual void ShowWindowWithState(ui::WindowShowState show_state) = 0;
  virtual void ShowMaximizedWithBounds(const gfx::Rect& restored_bounds) = 0;

  virtual bool IsVisible() const = 0;

  virtual void SetSize(const gfx::Size& size) = 0;
  virtual void StackAbove(aura::Window* window) = 0;
  virtual void StackAtTop() = 0;
  virtual void CenterWindow(const gfx::Size& size) = 0;
  virtual void GetWindowPlacement(gfx::Rect* bounds,
                                  ui::WindowShowState* show_state) const = 0;
  virtual gfx::Rect GetWindowBoundsInScreen() const = 0;
  virtual gfx::Rect GetClientAreaBoundsInScreen() const = 0;
  virtual gfx::Rect GetRestoredBounds() const = 0;
  virtual std::string GetWorkspace() const = 0;

  virtual gfx::Rect GetWorkAreaBoundsInScreen() const = 0;

  // Sets the shape of the root window. If |native_shape| is nullptr then the
  // window reverts to rectangular.
  virtual void SetShape(std::unique_ptr<Widget::ShapeRects> native_shape) = 0;

  virtual void Activate() = 0;
  virtual void Deactivate() = 0;
  virtual bool IsActive() const = 0;
  virtual void Maximize() = 0;
  virtual void Minimize() = 0;
  virtual void Restore() = 0;
  virtual bool IsMaximized() const = 0;
  virtual bool IsMinimized() const = 0;

  virtual bool HasCapture() const = 0;

  virtual void SetAlwaysOnTop(bool always_on_top) = 0;
  virtual bool IsAlwaysOnTop() const = 0;

  virtual void SetVisibleOnAllWorkspaces(bool always_visible) = 0;
  virtual bool IsVisibleOnAllWorkspaces() const = 0;

  // Returns true if the title changed.
  virtual bool SetWindowTitle(const base::string16& title) = 0;

  virtual void ClearNativeFocus() = 0;

  virtual Widget::MoveLoopResult RunMoveLoop(
      const gfx::Vector2d& drag_offset,
      Widget::MoveLoopSource source,
      Widget::MoveLoopEscapeBehavior escape_behavior) = 0;
  virtual void EndMoveLoop() = 0;

  virtual void SetVisibilityChangedAnimationsEnabled(bool value) = 0;

  virtual NonClientFrameView* CreateNonClientFrameView() = 0;

  // Determines whether the window should use native title bar and borders.
  virtual bool ShouldUseNativeFrame() const = 0;
  // Determines whether the window contents should be rendered transparently
  // (for example, so that they can overhang onto the window title bar).
  virtual bool ShouldWindowContentsBeTransparent() const = 0;
  virtual void FrameTypeChanged() = 0;

  virtual void SetFullscreen(bool fullscreen) = 0;
  virtual bool IsFullscreen() const = 0;

  virtual void SetOpacity(float opacity) = 0;

  virtual void SetWindowIcons(const gfx::ImageSkia& window_icon,
                              const gfx::ImageSkia& app_icon) = 0;

  virtual void InitModalType(ui::ModalType modal_type) = 0;

  virtual void FlashFrame(bool flash_frame) = 0;

  // Returns true if the Widget was closed but is still showing because of
  // animations.
  virtual bool IsAnimatingClosed() const = 0;

  // Returns true if the Widget supports translucency.
  virtual bool IsTranslucentWindowOpacitySupported() const = 0;

  // Called when the window's size constraints change.
  virtual void SizeConstraintsChanged() = 0;

  // Returns true if the transparency of the DesktopNativeWidgetAura's
  // |content_window_| should change.
  virtual bool ShouldUpdateWindowTransparency() const = 0;

  // A return value of true indicates DesktopNativeCursorManager should be
  // used, a return value of false indicates the DesktopWindowTreeHost manages
  // cursors itself.
  virtual bool ShouldUseDesktopNativeCursorManager() const = 0;

  // Returns whether a VisibilityController should be created.
  virtual bool ShouldCreateVisibilityController() const = 0;
};

}  // namespace views

#endif  // UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_WINDOW_TREE_HOST_H_
