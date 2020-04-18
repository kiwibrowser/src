// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/apps/native_app_window_cocoa.h"

#include "base/command_line.h"
#include "base/mac/foundation_util.h"
#include "base/mac/mac_util.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/apps/app_shim/extension_app_shim_handler_mac.h"
#include "chrome/browser/profiles/profile.h"
#import "chrome/browser/ui/cocoa/apps/titlebar_background_view.h"
#include "chrome/browser/ui/cocoa/browser_window_utils.h"
#import "chrome/browser/ui/cocoa/chrome_event_processing_window.h"
#include "chrome/browser/ui/cocoa/extensions/extension_keybinding_registry_cocoa.h"
#include "chrome/browser/ui/cocoa/extensions/extension_view_mac.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/web_contents.h"
#include "extensions/common/extension.h"
#include "skia/ext/skia_utils_mac.h"
#include "third_party/skia/include/core/SkRegion.h"
#import "ui/gfx/mac/nswindow_frame_controls.h"
#include "ui/gfx/skia_util.h"

// NOTE: State Before Update.
//
// Internal state, such as |is_maximized_|, must be set before the window
// state is changed so that it is accurate when e.g. a resize results in a call
// to |OnNativeWindowChanged|.

// NOTE: Maximize and Zoom.
//
// Zooming is implemented manually in order to implement maximize functionality
// and to support non resizable windows. The window will be resized explicitly
// in the |WindowWillZoom| call.
//
// Attempting maximize and restore functionality with non resizable windows
// using the native zoom method did not work, even with
// windowWillUseStandardFrame, as the window would not restore back to the
// desired size.

using extensions::AppWindow;

@interface NSWindow (NSPrivateApis)
- (void)setBottomCornerRounded:(BOOL)rounded;
- (BOOL)_isTitleHidden;
@end

namespace {

const int kActivateThrottlePeriodSeconds = 2;

NSRect GfxToCocoaBounds(gfx::Rect bounds) {
  typedef AppWindow::BoundsSpecification BoundsSpecification;

  NSRect main_screen_rect = [[[NSScreen screens] firstObject] frame];

  // If coordinates are unspecified, center window on primary screen.
  if (bounds.x() == BoundsSpecification::kUnspecifiedPosition)
    bounds.set_x(floor((NSWidth(main_screen_rect) - bounds.width()) / 2));
  if (bounds.y() == BoundsSpecification::kUnspecifiedPosition)
    bounds.set_y(floor((NSHeight(main_screen_rect) - bounds.height()) / 2));

  // Convert to Mac coordinates.
  NSRect cocoa_bounds = NSRectFromCGRect(bounds.ToCGRect());
  cocoa_bounds.origin.y = NSHeight(main_screen_rect) - NSMaxY(cocoa_bounds);
  return cocoa_bounds;
}

// Return a vector of non-draggable regions that fill a window of size
// |width| by |height|, but leave gaps where the window should be draggable.
std::vector<gfx::Rect> CalculateNonDraggableRegions(
    const std::vector<extensions::DraggableRegion>& regions,
    int width,
    int height) {
  std::vector<gfx::Rect> result;
  if (regions.empty()) {
    result.push_back(gfx::Rect(0, 0, width, height));
  } else {
    std::unique_ptr<SkRegion> draggable(
        AppWindow::RawDraggableRegionsToSkRegion(regions));
    std::unique_ptr<SkRegion> non_draggable(new SkRegion);
    non_draggable->op(0, 0, width, height, SkRegion::kUnion_Op);
    non_draggable->op(*draggable, SkRegion::kDifference_Op);
    for (SkRegion::Iterator it(*non_draggable); !it.done(); it.next()) {
      result.push_back(gfx::SkIRectToRect(it.rect()));
    }
  }
  return result;
}

}  // namespace

@implementation NativeAppWindowController

@synthesize appWindow = appWindow_;

- (void)setTitlebarBackgroundView:(NSView*)view {
  titlebar_background_view_.reset([view retain]);
}

- (void)windowWillClose:(NSNotification*)notification {
  if (appWindow_)
    appWindow_->WindowWillClose();
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
  if (appWindow_)
    appWindow_->WindowDidBecomeKey();
}

- (void)windowDidResignKey:(NSNotification*)notification {
  if (appWindow_)
    appWindow_->WindowDidResignKey();
}

- (void)windowDidBecomeMain:(NSNotification*)notification {
  [titlebar_background_view_ setNeedsDisplay:YES];
}

- (void)windowDidResignMain:(NSNotification*)notification {
  [titlebar_background_view_ setNeedsDisplay:YES];
}

- (void)windowDidResize:(NSNotification*)notification {
  if (appWindow_)
    appWindow_->WindowDidResize();
}

- (void)windowDidEndLiveResize:(NSNotification*)notification {
  if (appWindow_)
    appWindow_->WindowDidFinishResize();
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification {
  if (appWindow_)
    appWindow_->WindowDidEnterFullscreen();
}

- (void)windowDidExitFullScreen:(NSNotification*)notification {
  if (appWindow_)
    appWindow_->WindowDidExitFullscreen();
}

- (void)windowDidMove:(NSNotification*)notification {
  if (appWindow_)
    appWindow_->WindowDidMove();
}

- (void)windowDidMiniaturize:(NSNotification*)notification {
  if (appWindow_)
    appWindow_->WindowDidMiniaturize();
}

- (void)windowDidDeminiaturize:(NSNotification*)notification {
  if (appWindow_)
    appWindow_->WindowDidDeminiaturize();
}

- (BOOL)windowShouldZoom:(NSWindow*)window
                 toFrame:(NSRect)newFrame {
  if (appWindow_)
    appWindow_->WindowWillZoom();
  return NO;  // See top of file NOTE: Maximize and Zoom.
}

// Allow non resizable windows (without NSResizableWindowMask) to enter
// fullscreen by passing through the full size in willUseFullScreenContentSize.
- (NSSize)window:(NSWindow *)window
    willUseFullScreenContentSize:(NSSize)proposedSize {
  return proposedSize;
}

- (BOOL)handledByExtensionCommand:(NSEvent*)event
    priority:(ui::AcceleratorManager::HandlerPriority)priority {
  if (appWindow_)
    return appWindow_->HandledByExtensionCommand(event, priority);
  return NO;
}

@end

@interface AppNSWindow : ChromeEventProcessingWindow
@end

@implementation AppNSWindow

// Similar to ChromeBrowserWindow, don't draw the title, but allow it to be seen
// in menus, Expose, etc.
- (BOOL)_isTitleHidden {
  return YES;
}

@end

@interface AppFramelessNSWindow : AppNSWindow
@end

@implementation AppFramelessNSWindow

+ (NSRect)frameRectForContentRect:(NSRect)contentRect
                        styleMask:(NSUInteger)mask {
  return contentRect;
}

+ (NSRect)contentRectForFrameRect:(NSRect)frameRect
                        styleMask:(NSUInteger)mask {
  return frameRect;
}

- (NSRect)frameRectForContentRect:(NSRect)contentRect {
  return contentRect;
}

- (NSRect)contentRectForFrameRect:(NSRect)frameRect {
  return frameRect;
}

@end

@interface ControlRegionView : NSView
@end

@implementation ControlRegionView

- (BOOL)mouseDownCanMoveWindow {
  return NO;
}

- (NSView*)hitTest:(NSPoint)aPoint {
  return nil;
}

@end

@interface NSView (WebContentsView)
- (void)setMouseDownCanMoveWindow:(BOOL)can_move;
@end

NativeAppWindowCocoa::NativeAppWindowCocoa(
    AppWindow* app_window,
    const AppWindow::CreateParams& params)
    : app_window_(app_window),
      has_frame_(params.frame == AppWindow::FRAME_CHROME),
      is_hidden_with_app_(false),
      is_maximized_(false),
      is_fullscreen_(false),
      is_resizable_(params.resizable),
      shows_resize_controls_(true),
      shows_fullscreen_controls_(true),
      has_frame_color_(params.has_frame_color),
      active_frame_color_(params.active_frame_color),
      inactive_frame_color_(params.inactive_frame_color) {
  Observe(WebContents());

  Class window_class = has_frame_ ?
      [AppNSWindow class] : [AppFramelessNSWindow class];

  // Estimate the initial bounds of the window. Once the frame insets are known,
  // the window bounds and constraints can be set precisely.
  NSRect cocoa_bounds = GfxToCocoaBounds(
      params.GetInitialWindowBounds(gfx::Insets()));
  base::scoped_nsobject<NSWindow> window([[window_class alloc]
      initWithContentRect:cocoa_bounds
                styleMask:GetWindowStyleMask()
                  backing:NSBackingStoreBuffered
                    defer:NO]);
  [window setReleasedWhenClosed:NO];  // Owned by the window controller.

  std::string name;
  const extensions::Extension* extension = app_window_->GetExtension();
  if (extension)
    name = extension->name();
  [window setTitle:base::SysUTF8ToNSString(name)];
  [[window contentView] setWantsLayer:YES];

  if (params.always_on_top)
    gfx::SetNSWindowAlwaysOnTop(window, true);

  gfx::SetNSWindowVisibleOnAllWorkspaces(window,
                                         params.visible_on_all_workspaces);

  window_controller_.reset(
      [[NativeAppWindowController alloc] initWithWindow:window]);

  if (has_frame_ && has_frame_color_) {
    TitlebarBackgroundView* view =
        [TitlebarBackgroundView addToNSWindow:window
                                  activeColor:active_frame_color_
                                inactiveColor:inactive_frame_color_];
    [window_controller_ setTitlebarBackgroundView:view];
  }

  NSView* view = WebContents()->GetNativeView();
  [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

  InstallView();

  [window setDelegate:window_controller_];
  [window_controller_ setAppWindow:this];

  // We can now compute the precise window bounds and constraints.
  gfx::Insets insets = GetFrameInsets();
  SetBounds(params.GetInitialWindowBounds(insets));
  SetContentSizeConstraints(params.GetContentMinimumSize(insets),
                            params.GetContentMaximumSize(insets));

  // Initialize |restored_bounds_|.
  restored_bounds_ = [window frame];

  extension_keybinding_registry_.reset(new ExtensionKeybindingRegistryCocoa(
      Profile::FromBrowserContext(app_window_->browser_context()),
      window,
      extensions::ExtensionKeybindingRegistry::PLATFORM_APPS_ONLY,
      NULL));
}

NSUInteger NativeAppWindowCocoa::GetWindowStyleMask() const {
  NSUInteger style_mask = NSTitledWindowMask | NSClosableWindowMask |
                          NSMiniaturizableWindowMask |
                          NSTexturedBackgroundWindowMask;
  if (shows_resize_controls_)
    style_mask |= NSResizableWindowMask;
  return style_mask;
}

void NativeAppWindowCocoa::InstallView() {
  NSView* view = WebContents()->GetNativeView();
  if (has_frame_) {
    [view setFrame:[[window() contentView] bounds]];
    [[window() contentView] addSubview:view];
    if (!shows_fullscreen_controls_)
      [[window() standardWindowButton:NSWindowZoomButton] setEnabled:NO];
    if (!shows_resize_controls_)
      [window() setShowsResizeIndicator:NO];
  } else {
    // TODO(jeremya): find a cleaner way to send this information to the
    // WebContentsViewCocoa view.
    DCHECK([view
        respondsToSelector:@selector(setMouseDownCanMoveWindow:)]);
    [view setMouseDownCanMoveWindow:YES];

    NSView* frameView = [[window() contentView] superview];
    [view setFrame:[frameView bounds]];
    [frameView addSubview:view];

    [[window() standardWindowButton:NSWindowZoomButton] setHidden:YES];
    [[window() standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
    [[window() standardWindowButton:NSWindowCloseButton] setHidden:YES];

    // Some third-party OS X utilities check the zoom button's enabled state to
    // determine whether to show custom UI on hover, so we disable it here to
    // prevent them from doing so in a frameless app window.
    [[window() standardWindowButton:NSWindowZoomButton] setEnabled:NO];

    UpdateDraggableRegionViews();
  }
}

void NativeAppWindowCocoa::UninstallView() {
  NSView* view = WebContents()->GetNativeView();
  [view removeFromSuperview];
}

bool NativeAppWindowCocoa::IsActive() const {
  return [window() isKeyWindow];
}

bool NativeAppWindowCocoa::IsMaximized() const {
  return is_maximized_ && !IsMinimized();
}

bool NativeAppWindowCocoa::IsMinimized() const {
  return [window() isMiniaturized];
}

bool NativeAppWindowCocoa::IsFullscreen() const {
  return is_fullscreen_;
}

void NativeAppWindowCocoa::SetFullscreen(int fullscreen_types) {
  bool fullscreen = (fullscreen_types != AppWindow::FULLSCREEN_TYPE_NONE);
  if (fullscreen == is_fullscreen_)
    return;

  // 10.11 posts an _endLiveResize event just before exiting fullscreen, so
  // ensure the window reports as fullscreen while the window is transitioning
  // to ensure the window bounds are not incorrectly captured as the last known
  // restored bounds.
  if (fullscreen)
    is_fullscreen_ = true;

  // If going fullscreen, but the window is constrained (fullscreen UI control
  // is disabled), temporarily enable it. It will be disabled again on leaving
  // fullscreen.
  if (fullscreen && !shows_fullscreen_controls_)
    gfx::SetNSWindowCanFullscreen(window(), true);
  [window() toggleFullScreen:nil];
  is_fullscreen_ = fullscreen;
}

bool NativeAppWindowCocoa::IsFullscreenOrPending() const {
  return is_fullscreen_;
}

gfx::NativeWindow NativeAppWindowCocoa::GetNativeWindow() const {
  return window();
}

gfx::Rect NativeAppWindowCocoa::GetRestoredBounds() const {
  // Flip coordinates based on the primary screen.
  NSScreen* screen = [[NSScreen screens] firstObject];
  NSRect frame = restored_bounds_;
  gfx::Rect bounds(frame.origin.x, 0, NSWidth(frame), NSHeight(frame));
  bounds.set_y(NSHeight([screen frame]) - NSMaxY(frame));
  return bounds;
}

ui::WindowShowState NativeAppWindowCocoa::GetRestoredState() const {
  if (IsMaximized())
    return ui::SHOW_STATE_MAXIMIZED;
  if (IsFullscreen())
    return ui::SHOW_STATE_FULLSCREEN;
  return ui::SHOW_STATE_NORMAL;
}

gfx::Rect NativeAppWindowCocoa::GetBounds() const {
  // Flip coordinates based on the primary screen.
  NSScreen* screen = [[NSScreen screens] firstObject];
  NSRect frame = [window() frame];
  gfx::Rect bounds(frame.origin.x, 0, NSWidth(frame), NSHeight(frame));
  bounds.set_y(NSHeight([screen frame]) - NSMaxY(frame));
  return bounds;
}

void NativeAppWindowCocoa::Show() {
  if (is_hidden_with_app_) {
    apps::ExtensionAppShimHandler::UnhideWithoutActivationForWindow(
        app_window_);
    is_hidden_with_app_ = false;
  }

  // Workaround for http://crbug.com/459306. When requests to change key windows
  // on Mac overlap, AppKit may attempt to make two windows simultaneously have
  // key status. This causes key events to go the wrong window, and key status
  // to get "stuck" until Chrome is deactivated. To reduce the possibility of
  // this occurring, throttle activation requests. To balance a possible Hide(),
  // always show the window, but don't make it key.
  base::Time now = base::Time::Now();
  if (now - last_activate_ <
      base::TimeDelta::FromSeconds(kActivateThrottlePeriodSeconds)) {
    [window() orderFront:window_controller_];
    return;
  }

  last_activate_ = now;

  [window() makeKeyAndOrderFront:nil];
  [NSApp activateIgnoringOtherApps:YES];
}

void NativeAppWindowCocoa::ShowInactive() {
  [window() orderFront:window_controller_];
}

void NativeAppWindowCocoa::Hide() {
  HideWithoutMarkingHidden();
}

bool NativeAppWindowCocoa::IsVisible() const {
  return [window() isVisible];
}

void NativeAppWindowCocoa::Close() {
  [window() close];
}

void NativeAppWindowCocoa::Activate() {
  Show();
}

void NativeAppWindowCocoa::Deactivate() {
  // TODO(jcivelli): http://crbug.com/51364 Implement me.
  NOTIMPLEMENTED();
}

void NativeAppWindowCocoa::Maximize() {
  if (is_fullscreen_)
    return;

  UpdateRestoredBounds();
  is_maximized_ = true;  // See top of file NOTE: State Before Update.
  [window() setFrame:[[window() screen] visibleFrame] display:YES animate:YES];
  if (IsMinimized())
    [window() deminiaturize:window_controller_];
}

void NativeAppWindowCocoa::Minimize() {
  [window() miniaturize:window_controller_];
}

void NativeAppWindowCocoa::Restore() {
  DCHECK(!IsFullscreenOrPending());   // SetFullscreen, not Restore, expected.

  if (is_maximized_) {
    is_maximized_ = false;  // See top of file NOTE: State Before Update.
    [window() setFrame:restored_bounds() display:YES animate:YES];
  }
  if (IsMinimized())
    [window() deminiaturize:window_controller_];
}

void NativeAppWindowCocoa::SetBounds(const gfx::Rect& bounds) {
  // Enforce minimum/maximum bounds.
  gfx::Rect checked_bounds = bounds;

  NSSize min_size = [window() minSize];
  if (bounds.width() < min_size.width)
    checked_bounds.set_width(min_size.width);
  if (bounds.height() < min_size.height)
    checked_bounds.set_height(min_size.height);
  NSSize max_size = [window() maxSize];
  if (checked_bounds.width() > max_size.width)
    checked_bounds.set_width(max_size.width);
  if (checked_bounds.height() > max_size.height)
    checked_bounds.set_height(max_size.height);

  NSRect cocoa_bounds = GfxToCocoaBounds(checked_bounds);
  [window() setFrame:cocoa_bounds display:YES];
  // setFrame: without animate: does not trigger a windowDidEndLiveResize: so
  // call it here.
  WindowDidFinishResize();
}

void NativeAppWindowCocoa::UpdateWindowIcon() {
  // TODO(junmin): implement.
}

void NativeAppWindowCocoa::UpdateWindowTitle() {
  base::string16 title = app_window_->GetTitle();
  [window() setTitle:base::SysUTF16ToNSString(title)];
}

void NativeAppWindowCocoa::UpdateShape(std::unique_ptr<ShapeRects> rects) {
  NOTIMPLEMENTED();
}

void NativeAppWindowCocoa::UpdateDraggableRegions(
    const std::vector<extensions::DraggableRegion>& regions) {
  // Draggable region is not supported for non-frameless window.
  if (has_frame_)
    return;

  draggable_regions_ = regions;
  UpdateDraggableRegionViews();
}

SkRegion* NativeAppWindowCocoa::GetDraggableRegion() {
  return NULL;
}

void NativeAppWindowCocoa::HandleKeyboardEvent(
    const content::NativeWebKeyboardEvent& event) {
  if (event.skip_in_browser ||
      event.GetType() == content::NativeWebKeyboardEvent::kChar) {
    return;
  }
  [window() redispatchKeyEvent:event.os_event];
}

void NativeAppWindowCocoa::UpdateDraggableRegionViews() {
  if (has_frame_)
    return;

  // All ControlRegionViews should be added as children of the WebContentsView,
  // because WebContentsView will be removed and re-added when entering and
  // leaving fullscreen mode.
  NSView* webView = WebContents()->GetNativeView();
  NSInteger webViewWidth = NSWidth([webView bounds]);
  NSInteger webViewHeight = NSHeight([webView bounds]);

  // Remove all ControlRegionViews that are added last time.
  // Note that [webView subviews] returns the view's mutable internal array and
  // it should be copied to avoid mutating the original array while enumerating
  // it.
  base::scoped_nsobject<NSArray> subviews([[webView subviews] copy]);
  for (NSView* subview in subviews.get())
    if ([subview isKindOfClass:[ControlRegionView class]])
      [subview removeFromSuperview];

  // Draggable regions is implemented by having the whole web view draggable
  // (mouseDownCanMoveWindow) and overlaying regions that are not draggable.
  std::vector<gfx::Rect> system_drag_exclude_areas =
      CalculateNonDraggableRegions(
          draggable_regions_, webViewWidth, webViewHeight);

  // Create and add a ControlRegionView for each region that needs to be
  // excluded from the dragging.
  for (std::vector<gfx::Rect>::const_iterator iter =
           system_drag_exclude_areas.begin();
       iter != system_drag_exclude_areas.end();
       ++iter) {
    base::scoped_nsobject<NSView> controlRegion(
        [[ControlRegionView alloc] initWithFrame:NSZeroRect]);
    [controlRegion setFrame:NSMakeRect(iter->x(),
                                       webViewHeight - iter->bottom(),
                                       iter->width(),
                                       iter->height())];
    [webView addSubview:controlRegion];
  }
}

void NativeAppWindowCocoa::FlashFrame(bool flash) {
  apps::ExtensionAppShimHandler::RequestUserAttentionForWindow(
      app_window_,
      flash ? apps::APP_SHIM_ATTENTION_CRITICAL
            : apps::APP_SHIM_ATTENTION_CANCEL);
}

bool NativeAppWindowCocoa::IsAlwaysOnTop() const {
  return gfx::IsNSWindowAlwaysOnTop(window());
}

void NativeAppWindowCocoa::RenderViewCreated(content::RenderViewHost* rvh) {
  if (IsActive())
    WebContents()->RestoreFocus();
}

bool NativeAppWindowCocoa::IsFrameless() const {
  return !has_frame_;
}

bool NativeAppWindowCocoa::HasFrameColor() const {
  return has_frame_color_;
}

SkColor NativeAppWindowCocoa::ActiveFrameColor() const {
  return active_frame_color_;
}

SkColor NativeAppWindowCocoa::InactiveFrameColor() const {
  return inactive_frame_color_;
}

gfx::Insets NativeAppWindowCocoa::GetFrameInsets() const {
  if (!has_frame_)
    return gfx::Insets();

  // Flip the coordinates based on the main screen.
  NSInteger screen_height =
      NSHeight([[[NSScreen screens] firstObject] frame]);

  NSRect frame_nsrect = [window() frame];
  gfx::Rect frame_rect(NSRectToCGRect(frame_nsrect));
  frame_rect.set_y(screen_height - NSMaxY(frame_nsrect));

  NSRect content_nsrect = [window() contentRectForFrameRect:frame_nsrect];
  gfx::Rect content_rect(NSRectToCGRect(content_nsrect));
  content_rect.set_y(screen_height - NSMaxY(content_nsrect));

  return frame_rect.InsetsFrom(content_rect);
}

bool NativeAppWindowCocoa::CanHaveAlphaEnabled() const {
  return false;
}

void NativeAppWindowCocoa::SetActivateOnPointer(bool activate_on_pointer) {
  NOTIMPLEMENTED();
}

gfx::NativeView NativeAppWindowCocoa::GetHostView() const {
  return WebContents()->GetNativeView();
}

gfx::Point NativeAppWindowCocoa::GetDialogPosition(const gfx::Size& size) {
  NOTIMPLEMENTED();
  return gfx::Point();
}

gfx::Size NativeAppWindowCocoa::GetMaximumDialogSize() {
  NOTIMPLEMENTED();
  return gfx::Size();
}

void NativeAppWindowCocoa::AddObserver(
    web_modal::ModalDialogHostObserver* observer) {
  NOTIMPLEMENTED();
}

void NativeAppWindowCocoa::RemoveObserver(
    web_modal::ModalDialogHostObserver* observer) {
  NOTIMPLEMENTED();
}

void NativeAppWindowCocoa::WindowWillClose() {
  [window_controller_ setAppWindow:NULL];
  app_window_->OnNativeWindowChanged();
  app_window_->OnNativeClose();
}

void NativeAppWindowCocoa::WindowDidBecomeKey() {
  app_window_->OnNativeWindowActivated();

  WebContents()->RestoreFocus();
}

void NativeAppWindowCocoa::WindowDidResignKey() {
  // If our app is still active and we're still the key window, ignore this
  // message, since it just means that a menu extra (on the "system status bar")
  // was activated; we'll get another |-windowDidResignKey| if we ever really
  // lose key window status.
  if ([NSApp isActive] && ([NSApp keyWindow] == window()))
    return;

  WebContents()->StoreFocus();
}

void NativeAppWindowCocoa::WindowDidFinishResize() {
  // Update |is_maximized_| if needed:
  // - Exit maximized state if resized.
  // - Consider us maximized if resize places us back to maximized location.
  //   This happens when returning from fullscreen.
  NSRect frame = [window() frame];
  NSRect screen = [[window() screen] visibleFrame];
  if (!NSEqualSizes(frame.size, screen.size))
    is_maximized_ = false;
  else if (NSEqualPoints(frame.origin, screen.origin))
    is_maximized_ = true;

  UpdateRestoredBounds();
}

void NativeAppWindowCocoa::WindowDidResize() {
  app_window_->OnNativeWindowChanged();
  UpdateDraggableRegionViews();
}

void NativeAppWindowCocoa::WindowDidMove() {
  UpdateRestoredBounds();
  app_window_->OnNativeWindowChanged();
}

void NativeAppWindowCocoa::WindowDidMiniaturize() {
  app_window_->OnNativeWindowChanged();
}

void NativeAppWindowCocoa::WindowDidDeminiaturize() {
  app_window_->OnNativeWindowChanged();
}

void NativeAppWindowCocoa::WindowDidEnterFullscreen() {
  is_maximized_ = false;
  is_fullscreen_ = true;
  app_window_->OnNativeWindowChanged();
}

void NativeAppWindowCocoa::WindowDidExitFullscreen() {
  is_fullscreen_ = false;
  if (!shows_fullscreen_controls_)
    gfx::SetNSWindowCanFullscreen(window(), false);

  WindowDidFinishResize();

  app_window_->OnNativeWindowChanged();
}

void NativeAppWindowCocoa::WindowWillZoom() {
  // See top of file NOTE: Maximize and Zoom.
  if (IsMaximized())
    Restore();
  else
    Maximize();
}

bool NativeAppWindowCocoa::HandledByExtensionCommand(
    NSEvent* event,
    ui::AcceleratorManager::HandlerPriority priority) {
  return extension_keybinding_registry_->ProcessKeyEvent(
      content::NativeWebKeyboardEvent(event), priority);
}

void NativeAppWindowCocoa::ShowWithApp() {
  is_hidden_with_app_ = false;
  if (!app_window_->is_hidden())
    ShowInactive();
}

void NativeAppWindowCocoa::HideWithApp() {
  is_hidden_with_app_ = true;
  HideWithoutMarkingHidden();
}

gfx::Size NativeAppWindowCocoa::GetContentMinimumSize() const {
  return size_constraints_.GetMinimumSize();
}

gfx::Size NativeAppWindowCocoa::GetContentMaximumSize() const {
  return size_constraints_.GetMaximumSize();
}

void NativeAppWindowCocoa::SetContentSizeConstraints(
    const gfx::Size& min_size, const gfx::Size& max_size) {
  // Update the size constraints.
  size_constraints_.set_minimum_size(min_size);
  size_constraints_.set_maximum_size(max_size);

  // Update the window controls.
  shows_resize_controls_ =
      is_resizable_ && !size_constraints_.HasFixedSize();
  shows_fullscreen_controls_ =
      is_resizable_ && !size_constraints_.HasMaximumSize() && has_frame_;

  gfx::ApplyNSWindowSizeConstraints(window(), min_size, max_size,
                                    shows_resize_controls_,
                                    shows_fullscreen_controls_);
}

void NativeAppWindowCocoa::SetAlwaysOnTop(bool always_on_top) {
  gfx::SetNSWindowAlwaysOnTop(window(), always_on_top);
}

void NativeAppWindowCocoa::SetVisibleOnAllWorkspaces(bool always_visible) {
  gfx::SetNSWindowVisibleOnAllWorkspaces(window(), always_visible);
}

NativeAppWindowCocoa::~NativeAppWindowCocoa() {
}

AppNSWindow* NativeAppWindowCocoa::window() const {
  NSWindow* window = [window_controller_ window];
  CHECK(!window || [window isKindOfClass:[AppNSWindow class]]);
  return static_cast<AppNSWindow*>(window);
}

content::WebContents* NativeAppWindowCocoa::WebContents() const {
  return app_window_->web_contents();
}

void NativeAppWindowCocoa::UpdateRestoredBounds() {
  if (IsRestored(*this))
    restored_bounds_ = [window() frame];
}

void NativeAppWindowCocoa::HideWithoutMarkingHidden() {
  [window() orderOut:window_controller_];
}
