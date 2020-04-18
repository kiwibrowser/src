// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_COCOA_BRIDGED_NATIVE_WIDGET_H_
#define UI_VIEWS_COCOA_BRIDGED_NATIVE_WIDGET_H_

#import <Cocoa/Cocoa.h>

#include <memory>
#include <vector>

#import "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"
#import "ui/accelerated_widget_mac/accelerated_widget_mac.h"
#include "ui/accelerated_widget_mac/display_ca_layer_tree.h"
#include "ui/base/ime/input_method_delegate.h"
#include "ui/compositor/layer_owner.h"
#import "ui/views/cocoa/bridged_native_widget_owner.h"
#import "ui/views/cocoa/cocoa_mouse_capture_delegate.h"
#import "ui/views/focus/focus_manager.h"
#include "ui/views/views_export.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_observer.h"

@class BridgedContentView;
@class ModalShowAnimationWithLayer;
@class ViewsNSWindowDelegate;

namespace ui {
class InputMethod;
}

namespace views {
namespace test {
class BridgedNativeWidgetTestApi;
}

class CocoaMouseCapture;
class CocoaWindowMoveLoop;
class DragDropClientMac;
class NativeWidgetMac;
class View;

// A bridge to an NSWindow managed by an instance of NativeWidgetMac or
// DesktopNativeWidgetMac. Serves as a helper class to bridge requests from the
// NativeWidgetMac to the Cocoa window. Behaves a bit like an aura::Window.
class VIEWS_EXPORT BridgedNativeWidget
    : public ui::LayerDelegate,
      public ui::LayerOwner,
      public ui::internal::InputMethodDelegate,
      public CocoaMouseCaptureDelegate,
      public FocusChangeListener,
      public ui::AcceleratedWidgetMacNSView,
      public BridgedNativeWidgetOwner,
      public DialogObserver {
 public:
  // Contains NativeViewHost->gfx::NativeView associations.
  using AssociatedViews = std::map<const views::View*, NSView*>;

  // Ways of changing the visibility of the bridged NSWindow.
  enum WindowVisibilityState {
    HIDE_WINDOW,               // Hides with -[NSWindow orderOut:].
    SHOW_AND_ACTIVATE_WINDOW,  // Shows with -[NSWindow makeKeyAndOrderFront:].
    SHOW_INACTIVE,             // Shows with -[NSWindow orderWindow:..]. Orders
                               // the window above its parent if it has one.
  };

  // Return the size that |window| will take for the given client area |size|,
  // based on its current style mask.
  static gfx::Size GetWindowSizeForClientSize(NSWindow* window,
                                              const gfx::Size& size);

  // Creates one side of the bridge. |parent| must not be NULL.
  explicit BridgedNativeWidget(NativeWidgetMac* parent);
  ~BridgedNativeWidget() override;

  // Initialize the bridge, "retains" ownership of |window|.
  void Init(base::scoped_nsobject<NSWindow> window,
            const Widget::InitParams& params);

  // Invoked at the end of Widget::Init().
  void OnWidgetInitDone();

  // Sets or clears the focus manager to use for tracking focused views.
  // This does NOT take ownership of |focus_manager|.
  void SetFocusManager(FocusManager* focus_manager);

  // Changes the bounds of the window and the hosted layer if present. The
  // origin is a location in screen coordinates except for "child" windows,
  // which are positioned relative to their parent(). SetBounds() considers a
  // "child" window to be one initialized with InitParams specifying all of:
  // a |parent| NSWindow, the |child| attribute, and a |type| that
  // views::GetAuraWindowTypeForWidgetType does not consider a "popup" type.
  void SetBounds(const gfx::Rect& new_bounds);

  // Set or clears the views::View bridged by the content view. This does NOT
  // take ownership of |view|.
  void SetRootView(views::View* view);

  // Sets the desired visibility of the window and updates the visibility of
  // descendant windows where necessary.
  void SetVisibilityState(WindowVisibilityState new_state);

  // Acquiring mouse capture first steals capture from any existing
  // CocoaMouseCaptureDelegate, then captures all mouse events until released.
  void AcquireCapture();
  void ReleaseCapture();
  bool HasCapture();

  // Start moving the window, pinned to the mouse cursor, and monitor events.
  // Return MOVE_LOOP_SUCCESSFUL on mouse up or MOVE_LOOP_CANCELED on premature
  // termination via EndMoveLoop() or when window is destroyed during the drag.
  Widget::MoveLoopResult RunMoveLoop(const gfx::Vector2d& drag_offset);
  void EndMoveLoop();

  // See views::Widget.
  void SetNativeWindowProperty(const char* key, void* value);
  void* GetNativeWindowProperty(const char* key) const;

  // Sets the cursor associated with the NSWindow. Retains |cursor|.
  void SetCursor(NSCursor* cursor);

  // Called internally by the NSWindowDelegate when the window is closing.
  void OnWindowWillClose();

  // Called by the NSWindowDelegate when a fullscreen operation begins. If
  // |target_fullscreen_state| is true, the target state is fullscreen.
  // Otherwise, a transition has begun to come out of fullscreen.
  void OnFullscreenTransitionStart(bool target_fullscreen_state);

  // Called when a fullscreen transition completes. If target_fullscreen_state()
  // does not match |actual_fullscreen_state|, a new transition will begin.
  void OnFullscreenTransitionComplete(bool actual_fullscreen_state);

  // Transition the window into or out of fullscreen. This will immediately
  // invert the value of target_fullscreen_state().
  void ToggleDesiredFullscreenState(bool async = false);

  // Called by the NSWindowDelegate when the size of the window changes.
  void OnSizeChanged();

  // Called once by the NSWindowDelegate when the position of the window has
  // changed.
  void OnPositionChanged();

  // Called by the NSWindowDelegate when the visibility of the window may have
  // changed. For example, due to a (de)miniaturize operation, or the window
  // being reordered in (or out of) the screen list.
  void OnVisibilityChanged();

  // Called by the NSWindowDelegate when the system control tint changes.
  void OnSystemControlTintChanged();

  // Called by the NSWindowDelegate on a scale factor or color space change.
  void OnBackingPropertiesChanged();

  // Called by the NSWindowDelegate when the window becomes or resigns key.
  void OnWindowKeyStatusChangedTo(bool is_key);

  // Called by NativeWidgetMac when the window size constraints change.
  void OnSizeConstraintsChanged();

  // Called by the window show animation when it completes and wants to destroy
  // itself.
  void OnShowAnimationComplete();

  // See widget.h for documentation.
  ui::InputMethod* GetInputMethod();

  // The restored bounds will be derived from the current NSWindow frame unless
  // fullscreen or transitioning between fullscreen states.
  gfx::Rect GetRestoredBounds() const;

  // Creates a ui::Compositor which becomes responsible for drawing the window.
  void CreateLayer(ui::LayerType layer_type, bool translucent);

  // Updates |associated_views_| on NativeViewHost::Attach()/Detach().
  void SetAssociationForView(const views::View* view, NSView* native_view);
  void ClearAssociationForView(const views::View* view);
  // Sorts child NSViews according to NativeViewHosts order in views hierarchy.
  void ReorderChildViews();

  NativeWidgetMac* native_widget_mac() { return native_widget_mac_; }
  BridgedContentView* ns_view() { return bridged_view_; }
  NSWindow* ns_window() { return window_; }

  TooltipManager* tooltip_manager() { return tooltip_manager_.get(); }

  DragDropClientMac* drag_drop_client() { return drag_drop_client_.get(); }

  // The parent widget specified in Widget::InitParams::parent. If non-null, the
  // parent will close children before the parent closes, and children will be
  // raised above their parent when window z-order changes.
  BridgedNativeWidgetOwner* parent() { return parent_; }
  const std::vector<BridgedNativeWidget*>& child_windows() {
    return child_windows_;
  }

  // Re-parent a |native_view| in this Widget to be a child of |new_parent|.
  // |native_view| must either be |ns_view()| or a descendant of |ns_view()|.
  // |native_view| is added as a subview of |new_parent| unless it is the
  // contentView of a top-level Widget. If |native_view| is |ns_view()|, |this|
  // also becomes a child window of |new_parent|'s NSWindow.
  void ReparentNativeView(NSView* native_view, NSView* new_parent);

  bool target_fullscreen_state() const { return target_fullscreen_state_; }
  bool window_visible() const { return window_visible_; }
  bool wants_to_be_visible() const { return wants_to_be_visible_; }

  bool animate() const { return animate_; }
  void set_animate(bool animate) { animate_ = animate; }

  // Overridden from ui::internal::InputMethodDelegate:
  ui::EventDispatchDetails DispatchKeyEventPostIME(ui::KeyEvent* key) override;

 private:
  friend class test::BridgedNativeWidgetTestApi;

  // Closes all child windows. BridgedNativeWidget children will be destroyed.
  void RemoveOrDestroyChildren();

  // Notify descendants of a visibility change.
  void NotifyVisibilityChangeDown();

  // Essentially NativeWidgetMac::GetClientAreaBoundsInScreen().size(), but no
  // coordinate transformations are required from AppKit coordinates.
  gfx::Size GetClientAreaSize() const;

  // Creates an owned ui::Compositor. For consistency, these functions reflect
  // those in aura::WindowTreeHost.
  void CreateCompositor();
  void InitCompositor();
  void DestroyCompositor();

  // Installs the NSView for hosting the composited layer. It is later provided
  // to |compositor_widget_| via AcceleratedWidgetGetNSView().
  void AddCompositorSuperview();

  // Size the layer to match the client area bounds, taking into account display
  // scale factor.
  void UpdateLayerProperties();

  // Immediately return if there is a composited frame matching |size_in_dip|.
  // Otherwise, asks ui::WindowResizeHelperMac to run tasks until a matching
  // frame is ready, or a timeout occurs.
  void MaybeWaitForFrame(const gfx::Size& size_in_dip);

  // Show the window using -[NSApp beginSheet:..], modal for the parent window.
  void ShowAsModalSheet();

  // Overridden from CocoaMouseCaptureDelegate:
  void PostCapturedEvent(NSEvent* event) override;
  void OnMouseCaptureLost() override;
  NSWindow* GetWindow() const override;

  // Returns a properties dictionary associated with the NSWindow.
  // Creates and attaches a new instance if not found.
  NSMutableDictionary* GetWindowProperties() const;

  // Overridden from FocusChangeListener:
  void OnWillChangeFocus(View* focused_before,
                         View* focused_now) override;
  void OnDidChangeFocus(View* focused_before,
                        View* focused_now) override;

  // Overridden from ui::LayerDelegate:
  void OnPaintLayer(const ui::PaintContext& context) override;
  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override;

  // Overridden from ui::AcceleratedWidgetMac:
  NSView* AcceleratedWidgetGetNSView() const override;
  void AcceleratedWidgetCALayerParamsUpdated() override;

  // Overridden from BridgedNativeWidgetOwner:
  NSWindow* GetNSWindow() override;
  gfx::Vector2d GetChildWindowOffset() const override;
  bool IsVisibleParent() const override;
  void RemoveChildWindow(BridgedNativeWidget* child) override;

  // DialogObserver:
  void OnDialogModelChanged() override;

  views::NativeWidgetMac* native_widget_mac_;  // Weak. Owns this.
  base::scoped_nsobject<NSWindow> window_;
  base::scoped_nsobject<ViewsNSWindowDelegate> window_delegate_;
  base::scoped_nsobject<BridgedContentView> bridged_view_;
  base::scoped_nsobject<ModalShowAnimationWithLayer> show_animation_;
  std::unique_ptr<ui::InputMethod> input_method_;
  std::unique_ptr<CocoaMouseCapture> mouse_capture_;
  std::unique_ptr<CocoaWindowMoveLoop> window_move_loop_;
  std::unique_ptr<TooltipManager> tooltip_manager_;
  std::unique_ptr<DragDropClientMac> drag_drop_client_;
  FocusManager* focus_manager_;  // Weak. Owned by our Widget.
  Widget::InitParams::Type widget_type_;

  BridgedNativeWidgetOwner* parent_;  // Weak. If non-null, owns this.
  std::vector<BridgedNativeWidget*> child_windows_;

  base::scoped_nsobject<NSView> compositor_superview_;
  std::unique_ptr<ui::AcceleratedWidgetMac> compositor_widget_;
  std::unique_ptr<ui::DisplayCALayerTree> display_ca_layer_tree_;
  std::unique_ptr<ui::Compositor> compositor_;
  viz::ParentLocalSurfaceIdAllocator parent_local_surface_id_allocator_;

  // Tracks the bounds when the window last started entering fullscreen. Used to
  // provide an answer for GetRestoredBounds(), but not ever sent to Cocoa (it
  // has its own copy, but doesn't provide access to it).
  gfx::Rect bounds_before_fullscreen_;

  // Whether this window wants to be fullscreen. If a fullscreen animation is in
  // progress then it might not be actually fullscreen.
  bool target_fullscreen_state_;

  // Whether this window is in a fullscreen transition, and the fullscreen state
  // can not currently be changed.
  bool in_fullscreen_transition_;

  // Stores the value last read from -[NSWindow isVisible], to detect visibility
  // changes.
  bool window_visible_;

  // If true, the window is either visible, or wants to be visible but is
  // currently hidden due to having a hidden parent.
  bool wants_to_be_visible_;

  // Whether to animate the window (when it is appropriate to do so).
  bool animate_ = true;

  // If true, the window has been made visible or changed shape and the window
  // shadow needs to be invalidated when a frame is received for the new shape.
  bool invalidate_shadow_on_frame_swap_ = false;

  // Whether the window's visibility is suppressed currently. For opaque non-
  // modal windows, the window's alpha value is set to 0, till the frame from
  // the compositor arrives to avoid "blinking".
  bool initial_visibility_suppressed_ = false;

  AssociatedViews associated_views_;

  DISALLOW_COPY_AND_ASSIGN(BridgedNativeWidget);
};

}  // namespace views

#endif  // UI_VIEWS_COCOA_BRIDGED_NATIVE_WIDGET_H_
