// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/native_widget_mac.h"

#import <Cocoa/Cocoa.h>

#include <utility>

#include "base/command_line.h"
#import "base/mac/bind_objc_block.h"
#include "base/mac/foundation_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/crash/core/common/crash_key.h"
#import "ui/base/cocoa/constrained_window/constrained_window_animation.h"
#import "ui/base/cocoa/window_size_constants.h"
#include "ui/base/ui_base_switches.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/font_list.h"
#import "ui/gfx/mac/coordinate_conversion.h"
#import "ui/gfx/mac/nswindow_frame_controls.h"
#include "ui/native_theme/native_theme.h"
#include "ui/native_theme/native_theme_mac.h"
#import "ui/views/cocoa/bridged_content_view.h"
#import "ui/views/cocoa/bridged_native_widget.h"
#include "ui/views/cocoa/cocoa_mouse_capture.h"
#import "ui/views/cocoa/drag_drop_client_mac.h"
#import "ui/views/cocoa/native_widget_mac_nswindow.h"
#import "ui/views/cocoa/views_nswindow_delegate.h"
#include "ui/views/widget/drop_helper.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/window/native_frame_view.h"

// Self-owning animation delegate that starts a hide animation, then calls
// -[NSWindow close] when the animation ends, releasing itself.
@interface ViewsNSWindowCloseAnimator : NSObject<NSAnimationDelegate> {
 @private
  base::scoped_nsobject<NSWindow> window_;
  base::scoped_nsobject<NSAnimation> animation_;
}

+ (void)closeWindowWithAnimation:(NSWindow*)window;

@end

namespace views {
namespace {

bool AreModalAnimationsEnabled() {
  return !base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableModalAnimations);
}

NSInteger StyleMaskForParams(const Widget::InitParams& params) {
  // If the Widget is modal, it will be displayed as a sheet. This works best if
  // it has NSTitledWindowMask. For example, with NSBorderlessWindowMask, the
  // parent window still accepts input.
  if (params.delegate &&
      params.delegate->GetModalType() == ui::MODAL_TYPE_WINDOW)
    return NSTitledWindowMask;

  // TODO(tapted): Determine better masks when there are use cases for it.
  if (params.remove_standard_frame)
    return NSBorderlessWindowMask;

  if (params.type == Widget::InitParams::TYPE_WINDOW) {
    return NSTitledWindowMask | NSClosableWindowMask |
           NSMiniaturizableWindowMask | NSResizableWindowMask |
           NSTexturedBackgroundWindowMask;
  }
  return NSBorderlessWindowMask;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetMac, public:

NativeWidgetMac::NativeWidgetMac(internal::NativeWidgetDelegate* delegate)
    : delegate_(delegate),
      bridge_(new BridgedNativeWidget(this)),
      ownership_(Widget::InitParams::NATIVE_WIDGET_OWNS_WIDGET) {
}

NativeWidgetMac::~NativeWidgetMac() {
  if (ownership_ == Widget::InitParams::NATIVE_WIDGET_OWNS_WIDGET)
    delete delegate_;
  else
    CloseNow();
}

// static
BridgedNativeWidget* NativeWidgetMac::GetBridgeForNativeWindow(
    gfx::NativeWindow window) {
  id<NSWindowDelegate> window_delegate = [window delegate];
  if ([window_delegate respondsToSelector:@selector(nativeWidgetMac)]) {
    ViewsNSWindowDelegate* delegate =
        base::mac::ObjCCastStrict<ViewsNSWindowDelegate>(window_delegate);
    return [delegate nativeWidgetMac]->bridge_.get();
  }
  return nullptr;  // Not created by NativeWidgetMac.
}

bool NativeWidgetMac::IsWindowModalSheet() const {
  return bridge_ && bridge_->parent() &&
         GetWidget()->widget_delegate()->GetModalType() ==
             ui::MODAL_TYPE_WINDOW;
}

void NativeWidgetMac::WindowDestroying() {
  OnWindowDestroying(GetNativeWindow());
  delegate_->OnNativeWidgetDestroying();
}

void NativeWidgetMac::WindowDestroyed() {
  DCHECK(bridge_);
  bridge_.reset();
  delegate_->OnNativeWidgetDestroyed();
  if (ownership_ == Widget::InitParams::NATIVE_WIDGET_OWNS_WIDGET)
    delete this;
}

int NativeWidgetMac::SheetPositionY() {
  NSView* view = GetNativeView();
  return
      [view convertPoint:NSMakePoint(0, NSHeight([view frame])) toView:nil].y;
}

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetMac, internal::NativeWidgetPrivate implementation:

void NativeWidgetMac::InitNativeWidget(const Widget::InitParams& params) {
  ownership_ = params.ownership;
  name_ = params.name;
  base::scoped_nsobject<NSWindow> window([CreateNSWindow(params) retain]);
  [window setReleasedWhenClosed:NO];  // Owned by scoped_nsobject.
  bridge_->Init(window, params);

  // Only set always-on-top here if it is true since setting it may affect how
  // the window is treated by Expose.
  if (params.keep_on_top)
    SetAlwaysOnTop(true);

  delegate_->OnNativeWidgetCreated(true);

  DCHECK(GetWidget()->GetRootView());
  bridge_->SetRootView(GetWidget()->GetRootView());
  if (auto* focus_manager = GetWidget()->GetFocusManager()) {
    [window makeFirstResponder:bridge_->ns_view()];
    bridge_->SetFocusManager(focus_manager);
  }

  // "Infer" must be handled by ViewsDelegate::OnBeforeWidgetInit().
  DCHECK_NE(Widget::InitParams::INFER_OPACITY, params.opacity);
  bool translucent = params.opacity == Widget::InitParams::TRANSLUCENT_WINDOW;
  bridge_->CreateLayer(params.layer_type, translucent);
}

void NativeWidgetMac::OnWidgetInitDone() {
  OnSizeConstraintsChanged();
  bridge_->OnWidgetInitDone();
}

NonClientFrameView* NativeWidgetMac::CreateNonClientFrameView() {
  return new NativeFrameView(GetWidget());
}

bool NativeWidgetMac::ShouldUseNativeFrame() const {
  return true;
}

bool NativeWidgetMac::ShouldWindowContentsBeTransparent() const {
  // On Windows, this returns true when Aero is enabled which draws the titlebar
  // with translucency.
  return false;
}

void NativeWidgetMac::FrameTypeChanged() {
  // This is called when the Theme has changed; forward the event to the root
  // widget.
  GetWidget()->ThemeChanged();
  GetWidget()->GetRootView()->SchedulePaint();
}

Widget* NativeWidgetMac::GetWidget() {
  return delegate_->AsWidget();
}

const Widget* NativeWidgetMac::GetWidget() const {
  return delegate_->AsWidget();
}

gfx::NativeView NativeWidgetMac::GetNativeView() const {
  // Returns a BridgedContentView, unless there is no views::RootView set.
  return [GetNativeWindow() contentView];
}

gfx::NativeWindow NativeWidgetMac::GetNativeWindow() const {
  return bridge_ ? bridge_->ns_window() : nil;
}

Widget* NativeWidgetMac::GetTopLevelWidget() {
  NativeWidgetPrivate* native_widget = GetTopLevelNativeWidget(GetNativeView());
  return native_widget ? native_widget->GetWidget() : nullptr;
}

const ui::Compositor* NativeWidgetMac::GetCompositor() const {
  return bridge_ && bridge_->layer() ? bridge_->layer()->GetCompositor()
                                     : nullptr;
}

const ui::Layer* NativeWidgetMac::GetLayer() const {
  return bridge_ ? bridge_->layer() : nullptr;
}

void NativeWidgetMac::ReorderNativeViews() {
  if (bridge_)
    bridge_->ReorderChildViews();
}

void NativeWidgetMac::ViewRemoved(View* view) {
  DragDropClientMac* client = bridge_ ? bridge_->drag_drop_client() : nullptr;
  if (client)
    client->drop_helper()->ResetTargetViewIfEquals(view);
}

void NativeWidgetMac::SetNativeWindowProperty(const char* name, void* value) {
  if (bridge_)
    bridge_->SetNativeWindowProperty(name, value);
}

void* NativeWidgetMac::GetNativeWindowProperty(const char* name) const {
  if (bridge_)
    return bridge_->GetNativeWindowProperty(name);

  return nullptr;
}

TooltipManager* NativeWidgetMac::GetTooltipManager() const {
  if (bridge_)
    return bridge_->tooltip_manager();

  return nullptr;
}

void NativeWidgetMac::SetCapture() {
  if (bridge_ && !bridge_->HasCapture())
    bridge_->AcquireCapture();
}

void NativeWidgetMac::ReleaseCapture() {
  if (bridge_)
    bridge_->ReleaseCapture();
}

bool NativeWidgetMac::HasCapture() const {
  return bridge_ && bridge_->HasCapture();
}

ui::InputMethod* NativeWidgetMac::GetInputMethod() {
  return bridge_ ? bridge_->GetInputMethod() : nullptr;
}

void NativeWidgetMac::CenterWindow(const gfx::Size& size) {
  SetSize(
      BridgedNativeWidget::GetWindowSizeForClientSize(GetNativeWindow(), size));
  // Note that this is not the precise center of screen, but it is the standard
  // location for windows like dialogs to appear on screen for Mac.
  // TODO(tapted): If there is a parent window, center in that instead.
  [GetNativeWindow() center];
}

void NativeWidgetMac::GetWindowPlacement(
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  *bounds = GetRestoredBounds();
  if (IsFullscreen())
    *show_state = ui::SHOW_STATE_FULLSCREEN;
  else if (IsMinimized())
    *show_state = ui::SHOW_STATE_MINIMIZED;
  else
    *show_state = ui::SHOW_STATE_NORMAL;
}

bool NativeWidgetMac::SetWindowTitle(const base::string16& title) {
  NSWindow* window = GetNativeWindow();
  NSString* current_title = [window title];
  NSString* new_title = base::SysUTF16ToNSString(title);
  if ([current_title isEqualToString:new_title])
    return false;

  [window setTitle:new_title];
  return true;
}

void NativeWidgetMac::SetWindowIcons(const gfx::ImageSkia& window_icon,
                                     const gfx::ImageSkia& app_icon) {
  // Per-window icons are not really a thing on Mac, so do nothing.
  // TODO(tapted): Investigate whether to use NSWindowDocumentIconButton to set
  // an icon next to the window title. See http://crbug.com/766897.
}

void NativeWidgetMac::InitModalType(ui::ModalType modal_type) {
  if (modal_type == ui::MODAL_TYPE_NONE)
    return;

  // System modal windows not implemented (or used) on Mac.
  DCHECK_NE(ui::MODAL_TYPE_SYSTEM, modal_type);

  // A peculiarity of the constrained window framework is that it permits a
  // dialog of MODAL_TYPE_WINDOW to have a null parent window; falling back to
  // a non-modal window in this case.
  DCHECK(bridge_->parent() || modal_type == ui::MODAL_TYPE_WINDOW);

  // Everything happens upon show.
}

gfx::Rect NativeWidgetMac::GetWindowBoundsInScreen() const {
  return gfx::ScreenRectFromNSRect([GetNativeWindow() frame]);
}

gfx::Rect NativeWidgetMac::GetClientAreaBoundsInScreen() const {
  NSWindow* window = GetNativeWindow();
  return gfx::ScreenRectFromNSRect(
      [window contentRectForFrameRect:[window frame]]);
}

gfx::Rect NativeWidgetMac::GetRestoredBounds() const {
  return bridge_ ? bridge_->GetRestoredBounds() : gfx::Rect();
}

std::string NativeWidgetMac::GetWorkspace() const {
  return std::string();
}

void NativeWidgetMac::SetBounds(const gfx::Rect& bounds) {
  if (bridge_)
    bridge_->SetBounds(bounds);
}

void NativeWidgetMac::SetBoundsConstrained(const gfx::Rect& bounds) {
  if (!bridge_)
    return;

  gfx::Rect new_bounds(bounds);
  NativeWidgetPrivate* ancestor =
      bridge_ && bridge_->parent()
          ? GetNativeWidgetForNativeWindow(bridge_->parent()->GetNSWindow())
          : nullptr;
  if (!ancestor) {
    new_bounds = ConstrainBoundsToDisplayWorkArea(new_bounds);
  } else {
    new_bounds.AdjustToFit(
        gfx::Rect(ancestor->GetWindowBoundsInScreen().size()));
  }
  SetBounds(new_bounds);
}

void NativeWidgetMac::SetSize(const gfx::Size& size) {
  // Ensure the top-left corner stays in-place (rather than the bottom-left,
  // which -[NSWindow setContentSize:] would do).
  SetBounds(gfx::Rect(GetWindowBoundsInScreen().origin(), size));
}

void NativeWidgetMac::StackAbove(gfx::NativeView native_view) {
  NSInteger view_parent = native_view.window.windowNumber;
  [GetNativeWindow() orderWindow:NSWindowAbove relativeTo:view_parent];
}

void NativeWidgetMac::StackAtTop() {
  NOTIMPLEMENTED();
}

void NativeWidgetMac::SetShape(std::unique_ptr<Widget::ShapeRects> shape) {
  NOTIMPLEMENTED();
}

void NativeWidgetMac::Close() {
  if (!bridge_)
    return;

  // Keep |window| on the stack so that the ObjectiveC block below can capture
  // it and properly increment the reference count bound to the posted task.
  NSWindow* window = GetNativeWindow();

  if (IsWindowModalSheet()) {
    // Sheets can't be closed normally. This starts the sheet closing. Once the
    // sheet has finished animating, it will call sheetDidEnd: on the parent
    // window's delegate. Note it still needs to be asynchronous, since code
    // calling Widget::Close() doesn't expect things to be deleted upon return.
    [NSApp performSelector:@selector(endSheet:) withObject:window afterDelay:0];
    return;
  }

  // For other modal types, animate the close.
  if (bridge_->animate() && AreModalAnimationsEnabled() &&
      delegate_->IsModal()) {
    [ViewsNSWindowCloseAnimator closeWindowWithAnimation:window];
    return;
  }

  // Clear the view early to suppress repaints.
  bridge_->SetRootView(nullptr);

  // Widget::Close() ensures [Non]ClientView::CanClose() returns true, so there
  // is no need to call the NSWindow or its delegate's -windowShouldClose:
  // implementation in the manner of -[NSWindow performClose:]. But,
  // like -performClose:, first remove the window from AppKit's display
  // list to avoid crashes like http://crbug.com/156101.
  [window orderOut:nil];

  // Many tests assume that base::RunLoop().RunUntilIdle() is always sufficient
  // to execute a close. However, in rare cases, -performSelector:..afterDelay:0
  // does not do this. So post a regular task.
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, base::BindBlock(^{
    [window close];
  }));
}

void NativeWidgetMac::CloseNow() {
  if (!bridge_)
    return;

  // NSWindows must be retained until -[NSWindow close] returns.
  base::scoped_nsobject<NSWindow> window(GetNativeWindow(),
                                         base::scoped_policy::RETAIN);

  // If there's a bridge at this point, it means there must be a window as well.
  DCHECK(window);
  [window close];
  // Note: |this| is deleted here when ownership_ == NATIVE_WIDGET_OWNS_WIDGET.
}

void NativeWidgetMac::Show() {
  ShowWithWindowState(ui::SHOW_STATE_NORMAL);
}

void NativeWidgetMac::Hide() {
  if (!bridge_)
    return;

  bridge_->SetVisibilityState(BridgedNativeWidget::HIDE_WINDOW);
}

void NativeWidgetMac::ShowMaximizedWithBounds(
    const gfx::Rect& restored_bounds) {
  NOTIMPLEMENTED();
}

void NativeWidgetMac::ShowWithWindowState(ui::WindowShowState state) {
  if (!bridge_)
    return;

  switch (state) {
    case ui::SHOW_STATE_DEFAULT:
    case ui::SHOW_STATE_NORMAL:
    case ui::SHOW_STATE_INACTIVE:
      break;
    case ui::SHOW_STATE_MINIMIZED:
    case ui::SHOW_STATE_MAXIMIZED:
    case ui::SHOW_STATE_FULLSCREEN:
      NOTIMPLEMENTED();
      break;
    case ui::SHOW_STATE_END:
      NOTREACHED();
      break;
  }
  bridge_->SetVisibilityState(state == ui::SHOW_STATE_INACTIVE
      ? BridgedNativeWidget::SHOW_INACTIVE
      : BridgedNativeWidget::SHOW_AND_ACTIVATE_WINDOW);

  // Ignore the SetInitialFocus() result. BridgedContentView should get
  // firstResponder status regardless.
  delegate_->SetInitialFocus(state);
}

bool NativeWidgetMac::IsVisible() const {
  return bridge_ && bridge_->window_visible();
}

void NativeWidgetMac::Activate() {
  if (!bridge_)
    return;

  bridge_->SetVisibilityState(BridgedNativeWidget::SHOW_AND_ACTIVATE_WINDOW);
}

void NativeWidgetMac::Deactivate() {
  NOTIMPLEMENTED();
}

bool NativeWidgetMac::IsActive() const {
  return [GetNativeWindow() isKeyWindow];
}

void NativeWidgetMac::SetAlwaysOnTop(bool always_on_top) {
  gfx::SetNSWindowAlwaysOnTop(GetNativeWindow(), always_on_top);
}

bool NativeWidgetMac::IsAlwaysOnTop() const {
  return gfx::IsNSWindowAlwaysOnTop(GetNativeWindow());
}

void NativeWidgetMac::SetVisibleOnAllWorkspaces(bool always_visible) {
  gfx::SetNSWindowVisibleOnAllWorkspaces(GetNativeWindow(), always_visible);
}

bool NativeWidgetMac::IsVisibleOnAllWorkspaces() const {
  return false;
}

void NativeWidgetMac::Maximize() {
  NOTIMPLEMENTED();  // See IsMaximized().
}

void NativeWidgetMac::Minimize() {
  NSWindow* window = GetNativeWindow();
  // Calling performMiniaturize: will momentarily highlight the button, but
  // AppKit will reject it if there is no miniaturize button.
  if ([window styleMask] & NSMiniaturizableWindowMask)
    [window performMiniaturize:nil];
  else
    [window miniaturize:nil];
}

bool NativeWidgetMac::IsMaximized() const {
  // The window frame isn't altered on Mac unless going fullscreen. The green
  // "+" button just makes the window bigger. So, always false.
  return false;
}

bool NativeWidgetMac::IsMinimized() const {
  return [GetNativeWindow() isMiniaturized];
}

void NativeWidgetMac::Restore() {
  SetFullscreen(false);
  [GetNativeWindow() deminiaturize:nil];
}

void NativeWidgetMac::SetFullscreen(bool fullscreen) {
  if (!bridge_ || fullscreen == IsFullscreen())
    return;

  bridge_->ToggleDesiredFullscreenState();
}

bool NativeWidgetMac::IsFullscreen() const {
  return bridge_ && bridge_->target_fullscreen_state();
}

void NativeWidgetMac::SetOpacity(float opacity) {
  [GetNativeWindow() setAlphaValue:opacity];
}

void NativeWidgetMac::FlashFrame(bool flash_frame) {
  NOTIMPLEMENTED();
}

void NativeWidgetMac::RunShellDrag(View* view,
                                   const ui::OSExchangeData& data,
                                   const gfx::Point& location,
                                   int operation,
                                   ui::DragDropTypes::DragEventSource source) {
  bridge_->drag_drop_client()->StartDragAndDrop(view, data, operation, source);
}

void NativeWidgetMac::SchedulePaintInRect(const gfx::Rect& rect) {
  // |rect| is relative to client area of the window.
  NSWindow* window = GetNativeWindow();
  NSRect client_rect = [window contentRectForFrameRect:[window frame]];
  NSRect target_rect = rect.ToCGRect();

  // Convert to Appkit coordinate system (origin at bottom left).
  target_rect.origin.y =
      NSHeight(client_rect) - target_rect.origin.y - NSHeight(target_rect);
  [GetNativeView() setNeedsDisplayInRect:target_rect];
  if (bridge_ && bridge_->layer())
    bridge_->layer()->SchedulePaint(rect);
}

void NativeWidgetMac::SetCursor(gfx::NativeCursor cursor) {
  if (bridge_)
    bridge_->SetCursor(cursor);
}

bool NativeWidgetMac::IsMouseEventsEnabled() const {
  // On platforms with touch, mouse events get disabled and calls to this method
  // can affect hover states. Since there is no touch on desktop Mac, this is
  // always true. Touch on Mac is tracked in http://crbug.com/445520.
  return true;
}

void NativeWidgetMac::ClearNativeFocus() {
  // To quote DesktopWindowTreeHostX11, "This method is weird and misnamed."
  // The goal is to set focus to the content window, thereby removing focus from
  // any NSView in the window that doesn't belong to toolkit-views.
  [GetNativeWindow() makeFirstResponder:GetNativeView()];
}

gfx::Rect NativeWidgetMac::GetWorkAreaBoundsInScreen() const {
  return gfx::ScreenRectFromNSRect([[GetNativeWindow() screen] visibleFrame]);
}

Widget::MoveLoopResult NativeWidgetMac::RunMoveLoop(
    const gfx::Vector2d& drag_offset,
    Widget::MoveLoopSource source,
    Widget::MoveLoopEscapeBehavior escape_behavior) {
  if (!bridge_)
    return Widget::MOVE_LOOP_CANCELED;

  return bridge_->RunMoveLoop(drag_offset);
}

void NativeWidgetMac::EndMoveLoop() {
  if (bridge_)
    bridge_->EndMoveLoop();
}

void NativeWidgetMac::SetVisibilityChangedAnimationsEnabled(bool value) {
  if (bridge_)
    bridge_->set_animate(value);
}

void NativeWidgetMac::SetVisibilityAnimationDuration(
    const base::TimeDelta& duration) {
  NOTIMPLEMENTED();
}

void NativeWidgetMac::SetVisibilityAnimationTransition(
    Widget::VisibilityTransition transition) {
  NOTIMPLEMENTED();
}

bool NativeWidgetMac::IsTranslucentWindowOpacitySupported() const {
  return false;
}

void NativeWidgetMac::OnSizeConstraintsChanged() {
  bridge_->OnSizeConstraintsChanged();
}

void NativeWidgetMac::RepostNativeEvent(gfx::NativeEvent native_event) {
  NOTIMPLEMENTED();
}

std::string NativeWidgetMac::GetName() const {
  return name_;
}

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetMac, protected:

NativeWidgetMacNSWindow* NativeWidgetMac::CreateNSWindow(
    const Widget::InitParams& params) {
  return [[[NativeWidgetMacNSWindow alloc]
      initWithContentRect:ui::kWindowSizeDeterminedLater
                styleMask:StyleMaskForParams(params)
                  backing:NSBackingStoreBuffered
                    defer:NO] autorelease];
}

////////////////////////////////////////////////////////////////////////////////
// Widget, public:

// static
void Widget::CloseAllSecondaryWidgets() {
  NSArray* starting_windows = [NSApp windows];  // Creates an autoreleased copy.
  for (NSWindow* window in starting_windows) {
    // Ignore any windows that couldn't have been created by NativeWidgetMac or
    // a subclass. GetNativeWidgetForNativeWindow() will later interrogate the
    // NSWindow delegate, but we can't trust that delegate to be a valid object.
    if (![window isKindOfClass:[NativeWidgetMacNSWindow class]])
      continue;

    // Record a crash key to detect when client code may destroy a
    // WidgetObserver without removing it (possibly leaking the Widget).
    // A crash can occur in generic Widget teardown paths when trying to notify.
    // See http://crbug.com/808318.
    static crash_reporter::CrashKeyString<256> window_info_key("windowInfo");
    std::string value = base::SysNSStringToUTF8(
        [NSString stringWithFormat:@"Closing %@ (%@)", [window title],
                                   [window className]]);
    crash_reporter::ScopedCrashKeyString scopedWindowKey(&window_info_key,
                                                         value);

    Widget* widget = GetWidgetForNativeWindow(window);
    if (widget && widget->is_secondary_widget())
      [window close];
  }
}

bool Widget::ConvertRect(const Widget* source,
                         const Widget* target,
                         gfx::Rect* rect) {
  return false;
}

const ui::NativeTheme* Widget::GetNativeTheme() const {
  return ui::NativeTheme::GetInstanceForNativeUi();
}

namespace internal {

////////////////////////////////////////////////////////////////////////////////
// internal::NativeWidgetPrivate, public:

// static
NativeWidgetPrivate* NativeWidgetPrivate::CreateNativeWidget(
    internal::NativeWidgetDelegate* delegate) {
  return new NativeWidgetMac(delegate);
}

// static
NativeWidgetPrivate* NativeWidgetPrivate::GetNativeWidgetForNativeView(
    gfx::NativeView native_view) {
  return GetNativeWidgetForNativeWindow([native_view window]);
}

// static
NativeWidgetPrivate* NativeWidgetPrivate::GetNativeWidgetForNativeWindow(
    gfx::NativeWindow native_window) {
  id<NSWindowDelegate> window_delegate = [native_window delegate];
  if ([window_delegate respondsToSelector:@selector(nativeWidgetMac)]) {
    ViewsNSWindowDelegate* delegate =
        base::mac::ObjCCastStrict<ViewsNSWindowDelegate>(window_delegate);
    return [delegate nativeWidgetMac];
  }
  return nullptr;  // Not created by NativeWidgetMac.
}

// static
NativeWidgetPrivate* NativeWidgetPrivate::GetTopLevelNativeWidget(
    gfx::NativeView native_view) {
  BridgedNativeWidget* bridge =
      NativeWidgetMac::GetBridgeForNativeWindow([native_view window]);
  if (!bridge)
    return nullptr;

  NativeWidgetPrivate* ancestor =
      bridge->parent() ? GetTopLevelNativeWidget(
                             [bridge->parent()->GetNSWindow() contentView])
                       : nullptr;
  return ancestor ? ancestor : bridge->native_widget_mac();
}

// static
void NativeWidgetPrivate::GetAllChildWidgets(gfx::NativeView native_view,
                                             Widget::Widgets* children) {
  BridgedNativeWidget* bridge =
      NativeWidgetMac::GetBridgeForNativeWindow([native_view window]);
  if (!bridge) {
    // The NSWindow is not itself a views::Widget, but it may have children that
    // are. Support returning Widgets that are parented to the NSWindow, except:
    // - Ignore requests for children of an NSView that is not a contentView.
    // - We do not add a Widget for |native_view| to |children| (there is none).
    if ([[native_view window] contentView] != native_view)
      return;

    // Collect -sheets and -childWindows. A window should never appear in both,
    // since that causes AppKit to glitch.
    NSArray* sheet_children = [[native_view window] sheets];
    for (NSWindow* native_child in sheet_children)
      GetAllChildWidgets([native_child contentView], children);

    for (NSWindow* native_child in [[native_view window] childWindows]) {
      DCHECK(![sheet_children containsObject:native_child]);
      GetAllChildWidgets([native_child contentView], children);
    }
    return;
  }

  // If |native_view| is a subview of the contentView, it will share an
  // NSWindow, but will itself be a native child of the Widget. That is, adding
  // bridge->..->GetWidget() to |children| would be adding the _parent_ of
  // |native_view|, not the Widget for |native_view|. |native_view| doesn't have
  // a corresponding Widget of its own in this case (and so can't have Widget
  // children of its own on Mac).
  if (bridge->ns_view() != native_view)
    return;

  // Code expects widget for |native_view| to be added to |children|.
  if (bridge->native_widget_mac()->GetWidget())
    children->insert(bridge->native_widget_mac()->GetWidget());

  // When the NSWindow *is* a Widget, only consider child_windows(). I.e. do not
  // look through -[NSWindow childWindows] as done for the (!bridge) case above.
  // -childWindows does not support hidden windows, and anything in there which
  // is not in child_windows() would have been added by AppKit.
  for (BridgedNativeWidget* child : bridge->child_windows())
    GetAllChildWidgets(child->ns_view(), children);
}

// static
void NativeWidgetPrivate::GetAllOwnedWidgets(gfx::NativeView native_view,
                                             Widget::Widgets* owned) {
  BridgedNativeWidget* bridge =
      NativeWidgetMac::GetBridgeForNativeWindow([native_view window]);
  if (!bridge) {
    GetAllChildWidgets(native_view, owned);
    return;
  }
  if (bridge->ns_view() != native_view)
    return;
  for (BridgedNativeWidget* child : bridge->child_windows())
    GetAllChildWidgets(child->ns_view(), owned);
}

// static
void NativeWidgetPrivate::ReparentNativeView(gfx::NativeView native_view,
                                             gfx::NativeView new_parent) {
  DCHECK_NE(native_view, new_parent);
  if (!new_parent || [native_view superview] == new_parent) {
    NOTREACHED();
    return;
  }

  BridgedNativeWidget* bridge =
      NativeWidgetMac::GetBridgeForNativeWindow([native_view window]);
  BridgedNativeWidget* parent_bridge =
      NativeWidgetMac::GetBridgeForNativeWindow([new_parent window]);
  DCHECK(bridge);
  if (Widget::GetWidgetForNativeView(native_view)->is_top_level() &&
      bridge->parent() == parent_bridge)
    return;

  Widget::Widgets widgets;
  GetAllChildWidgets(native_view, &widgets);

  // First notify all the widgets that they are being disassociated
  // from their previous parent.
  for (auto* child : widgets)
    child->NotifyNativeViewHierarchyWillChange();

  bridge->ReparentNativeView(native_view, new_parent);

  // And now, notify them that they have a brand new parent.
  for (auto* child : widgets)
    child->NotifyNativeViewHierarchyChanged();
}

// static
bool NativeWidgetPrivate::IsMouseButtonDown() {
  return [NSEvent pressedMouseButtons] != 0;
}

// static
gfx::FontList NativeWidgetPrivate::GetWindowTitleFontList() {
  NOTIMPLEMENTED();
  return gfx::FontList();
}

// static
gfx::NativeView NativeWidgetPrivate::GetGlobalCapture(
    gfx::NativeView native_view) {
  return [CocoaMouseCapture::GetGlobalCaptureWindow() contentView];
}

}  // namespace internal
}  // namespace views

@implementation ViewsNSWindowCloseAnimator

- (id)initWithWindow:(NSWindow*)window {
  if ((self = [super init])) {
    window_.reset([window retain]);
    animation_.reset(
        [[ConstrainedWindowAnimationHide alloc] initWithWindow:window]);
    [animation_ setDelegate:self];
    [animation_ setAnimationBlockingMode:NSAnimationNonblocking];
    [animation_ startAnimation];
  }
  return self;
}

+ (void)closeWindowWithAnimation:(NSWindow*)window {
  [[ViewsNSWindowCloseAnimator alloc] initWithWindow:window];
}

- (void)animationDidEnd:(NSAnimation*)animation {
  [window_ close];
  [animation_ setDelegate:nil];
  [self release];
}

@end
