// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/tabs/tab_window_controller.h"

#import <objc/runtime.h>

#include "base/logging.h"
#import "base/mac/foundation_util.h"
#import "base/mac/sdk_forward_declarations.h"
#import "chrome/browser/ui/cocoa/browser_window_layout.h"
#import "chrome/browser/ui/cocoa/fast_resize_view.h"
#import "chrome/browser/ui/cocoa/framed_browser_window.h"
#import "chrome/browser/ui/cocoa/tabbed_browser_window.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_background_view.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_view.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "chrome/browser/ui/window_sizer/window_sizer.h"
#import "ui/base/cocoa/focus_tracker.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/theme_provider.h"

// As of macOS 10.13 NSWindow lifetimes after closing are unpredictable. Chrome
// frees resources on window close so this new behavior created problems such as
// browser tests failing to complete (see https://crbug.com/749196 ). To work
// around this new behavior TabWindowController no longer acts as the NSWindow's
// NSWindowController or NSWindowDelegate but instead uses a
// TabWindowControllerProxy instance. The TabWindowController and its subclasses
// still expect to receive NSWindowDelegate messages, which the
// TabWindowControllerProxy forwards along.
@interface TabWindowControllerProxy : NSWindowController<NSWindowDelegate>
@property(assign, nonatomic) TabWindowController* tabWindowController;
@end

@implementation TabWindowControllerProxy

@synthesize tabWindowController = tabWindowController_;

- (void)dealloc {
  // The TabWindowControllerProxy should outlive the TabWindowController.
  DCHECK(!tabWindowController_);
  [super dealloc];
}

- (BOOL)respondsToSelector:(SEL)aSelector {
  if ([super respondsToSelector:aSelector]) {
    return YES;
  }
  return [self.tabWindowController respondsToSelector:aSelector];
}

- (NSMethodSignature*)methodSignatureForSelector:(SEL)aSelector {
  NSMethodSignature* signature = [super methodSignatureForSelector:aSelector];

  return signature
             ? signature
             : [self.tabWindowController methodSignatureForSelector:aSelector];
}

- (void)forwardInvocation:(NSInvocation*)anInvocation {
  [anInvocation setTarget:self.tabWindowController];
  [anInvocation invoke];
}

@end

@interface TabWindowController () {
  base::scoped_nsobject<TabWindowControllerProxy> nsWindowController_;
}

- (void)setUseOverlay:(BOOL)useOverlay;

// The tab strip background view should always be inserted as the back-most
// subview of the contentView.
- (void)insertTabStripBackgroundViewIntoWindow:(NSWindow*)window
                                      titleBar:(BOOL)hasTitleBar;

// Called when NSWindowWillEnterFullScreenNotification notification received.
// Makes visual effects view hidden as it should not be displayed in fullscreen.
- (void)windowWillEnterFullScreenNotification:(NSNotification*)notification;

// Called when NSWindowWillExitFullScreenNotification notification received.
// Makes visual effects view visible since it was hidden in fullscreen.
- (void)windowWillExitFullScreenNotification:(NSNotification*)notification;

@end

@interface TabWindowOverlayWindow : NSWindow
@end

@implementation TabWindowOverlayWindow

- (const ui::ThemeProvider*)themeProvider {
  return [[self parentWindow] themeProvider];
}

- (ThemedWindowStyle)themedWindowStyle {
  return [[self parentWindow] themedWindowStyle];
}

- (NSPoint)themeImagePositionForAlignment:(ThemeImageAlignment)alignment {
  return [[self parentWindow] themeImagePositionForAlignment:alignment];
}

- (BOOL)hasDarkTheme {
  return [[self parentWindow] hasDarkTheme];
}

- (BOOL)inIncognitoModeWithSystemTheme {
  return [[self parentWindow] inIncognitoModeWithSystemTheme];
}

@end

// Subview of the window's contentView, contains everything but the tab strip.
@interface ChromeContentView : NSView
@end

@implementation ChromeContentView

// NSView overrides.

// Since Auto Layout and frame-based layout behave differently in small but
// important ways (e.g. Auto Layout can restrict window resizing, frame-based
// layout doesn't log a warning when a view's autoresizing mask can't be
// maintained), ensure that it's on instead of letting it depend on content.
+ (BOOL)requiresConstraintBasedLayout {
  // TODO(sdy): Turn back on (or remove) after investigating a performance
  // regression: https://crbug.com/706931
  return NO;
}

@end

@implementation TabWindowController

+ (TabWindowController*)tabWindowControllerForWindow:(NSWindow*)window {
  while (window) {
    TabWindowControllerProxy* nsWindowController =
        base::mac::ObjCCast<TabWindowControllerProxy>(
            [window windowController]);

    if (nsWindowController) {
      return [nsWindowController tabWindowController];
    }
    window = [window parentWindow];
  }
  return nil;
}

- (id)initTabWindowControllerWithTabStrip:(BOOL)hasTabStrip
                                 titleBar:(BOOL)hasTitleBar {
  const CGFloat kDefaultWidth = WindowSizer::kWindowMaxDefaultWidth;
  const CGFloat kDefaultHeight = 600;

  if (self = [self init]) {
    NSRect contentRect = NSMakeRect(60, 229, kDefaultWidth, kDefaultHeight);
    base::scoped_nsobject<FramedBrowserWindow> window(
        [(hasTabStrip
              ? [TabbedBrowserWindow alloc]
              : [FramedBrowserWindow alloc]) initWithContentRect:contentRect]);
    [window setReleasedWhenClosed:YES];
    [window setAutorecalculatesKeyViewLoop:YES];

    nsWindowController_.reset(
        [[TabWindowControllerProxy alloc] initWithWindow:window]);
    [nsWindowController_ setTabWindowController:self];

    [[self window] setDelegate:nsWindowController_];

    // Insert ourselves into the repsonder chain. First, find the responder
    // that comes before nsWindowController_.
    NSResponder* nextResponderToCheck = [self window];
    while ([nextResponderToCheck nextResponder] &&
           [nextResponderToCheck nextResponder] != nsWindowController_.get()) {
      nextResponderToCheck = [nextResponderToCheck nextResponder];
    }
    // If nextResponder is nil, nsWindowController_ is not in the responder
    // chain.
    DCHECK([nextResponderToCheck nextResponder]);
    // Insert before nsWindowController_.
    [nextResponderToCheck setNextResponder:self];
    [self setNextResponder:nsWindowController_];

    chromeContentView_.reset([[ChromeContentView alloc]
        initWithFrame:NSMakeRect(0, 0, kDefaultWidth, kDefaultHeight)]);
    [chromeContentView_
        setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [chromeContentView_ setWantsLayer:YES];
    [[[self window] contentView] addSubview:chromeContentView_];

    tabContentArea_.reset(
        [[FastResizeView alloc] initWithFrame:[chromeContentView_ bounds]]);
    [tabContentArea_
        setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [chromeContentView_ addSubview:tabContentArea_];

    // tabStripBackgroundView_ draws the theme image behind the tab strip area.
    // When making a tab dragging window (setUseOverlay:), this view stays in
    // the parent window so that it can be translucent, while the tab strip view
    // moves to the child window and stays opaque.
    NSView* windowView = [window contentView];
    CGFloat paintHeight = [FramedBrowserWindow browserFrameViewPaintHeight];
    tabStripBackgroundView_.reset([[TabStripBackgroundView alloc]
        initWithFrame:NSMakeRect(0, NSMaxY([windowView bounds]) - paintHeight,
                                 NSWidth([windowView bounds]), paintHeight)]);
    [tabStripBackgroundView_
        setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin];
    [self insertTabStripBackgroundViewIntoWindow:window titleBar:hasTitleBar];

    tabStripView_.reset([[TabStripView alloc]
        initWithFrame:NSMakeRect(0, 0, kDefaultWidth,
                                 chrome::kTabStripHeight)]);
    [tabStripView_ setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin];
    if (hasTabStrip)
      [windowView addSubview:tabStripView_];

    // |windowWillEnterFullScreen:| and |windowWillExitFullScreen:| are
    // already called because self is a delegate for the window. However this
    // class is designed for subclassing and can not implement
    // NSWindowDelegate methods (because subclasses can do so as well and they
    // should be able to). TODO(crbug.com/654656): Move |visualEffectView_| to
    // subclass.
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(windowWillEnterFullScreenNotification:)
               name:NSWindowWillEnterFullScreenNotification
             object:window];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(windowWillExitFullScreenNotification:)
               name:NSWindowWillExitFullScreenNotification
             object:window];
  }

  return self;
}

- (void)dealloc {
  // Restore the reponder chain. Find the responder before us in the chain.
  NSResponder* nextResponderToCheck = [self window];
  while ([nextResponderToCheck nextResponder] &&
         [nextResponderToCheck nextResponder] != self) {
    nextResponderToCheck = [nextResponderToCheck nextResponder];
  }
  [nextResponderToCheck setNextResponder:[self nextResponder]];

  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [overlayWindow_ setDelegate:nil];
  [[self window] setDelegate:nil];
  [nsWindowController_ setTabWindowController:nil];
  [nsWindowController_ setWindow:nil];
  [super dealloc];
}

- (NSWindow*)window {
  return [nsWindowController_ window];
}

- (void)setWindow:(NSWindow*)aWindow {
  [nsWindowController_ setWindow:aWindow];
}

- (NSWindowController*)nsWindowController {
  return nsWindowController_;
}

- (NSVisualEffectView*)visualEffectView {
  return visualEffectView_;
}

- (NSView*)tabStripBackgroundView {
  return tabStripBackgroundView_;
}

- (TabStripView*)tabStripView {
  return tabStripView_;
}

- (FastResizeView*)tabContentArea {
  return tabContentArea_;
}

- (NSView*)chromeContentView {
  return chromeContentView_;
}

- (void)removeOverlay {
  [self setUseOverlay:NO];
  if (closeDeferred_) {
    // See comment in BrowserWindowCocoa::Close() about orderOut:.
    [[self window] orderOut:self];
    [[self window] performClose:self];  // Autoreleases the controller.
  }
}

- (void)showOverlay {
  [self setUseOverlay:YES];
}

// If |useOverlay| is YES, creates a new overlay window and puts the tab strip
// and the content area inside of it. This allows it to have a different opacity
// from the title bar. If NO, returns everything to the previous state and
// destroys the overlay window until it's needed again. The tab strip and window
// contents are returned to the original window.
- (void)setUseOverlay:(BOOL)useOverlay {
  [NSObject cancelPreviousPerformRequestsWithTarget:self
                                           selector:@selector(removeOverlay)
                                             object:nil];
  NSWindow* window = [self window];
  if (useOverlay && !overlayWindow_) {
    DCHECK(!originalContentView_);

    overlayWindow_ = [[TabWindowOverlayWindow alloc]
                         initWithContentRect:[window frame]
                                   styleMask:NSBorderlessWindowMask
                                     backing:NSBackingStoreBuffered
                                       defer:NO];
    [overlayWindow_ setTitle:@"overlay"];
    [overlayWindow_ setBackgroundColor:[NSColor clearColor]];
    [overlayWindow_ setOpaque:NO];
    [overlayWindow_ setDelegate:self];
    [[overlayWindow_ contentView] setWantsLayer:YES];

    originalContentView_ = self.chromeContentView;
    [window addChildWindow:overlayWindow_ ordered:NSWindowAbove];

    // Explicitly set the responder to be nil here (for restoring later).
    // If the first responder were to be left non-nil here then
    // [RenderWidgethostViewCocoa resignFirstResponder] would be called,
    // followed by RenderWidgetHost::Blur(), which would result in an unexpected
    // loss of focus.
    focusBeforeOverlay_.reset([[FocusTracker alloc] initWithWindow:window]);
    [window makeFirstResponder:nil];

    // Move the original window's tab strip view and content view to the overlay
    // window. The content view is added as a subview of the overlay window's
    // content view (rather than using setContentView:) because the overlay
    // window has a different content size (due to it being borderless).
    [[overlayWindow_ contentView] addSubview:[self tabStripView]];
    [[self tabStripView] setInATabDraggingOverlayWindow:YES];
    [[overlayWindow_ contentView] addSubview:originalContentView_];

    [overlayWindow_ orderFront:nil];
  } else if (!useOverlay && overlayWindow_) {
    DCHECK(originalContentView_);

    // Return the original window's tab strip view and content view to their
    // places. The TabStripView should be in front of the content view, and the
    // avatar button, if present, should be in front of the TabStripView.
    [[window contentView] addSubview:originalContentView_];
    originalContentView_.frame = [[window contentView] bounds];
    [[window contentView] addSubview:[self tabStripView]];
    [[window contentView] addSubview:[self avatarView]];
    [[self tabStripView] setInATabDraggingOverlayWindow:NO];
    [[window contentView] updateTrackingAreas];

    [focusBeforeOverlay_ restoreFocusInWindow:window];
    focusBeforeOverlay_.reset();

    [window display];
    [window removeChildWindow:overlayWindow_];

    [overlayWindow_ setDelegate:nil];
    [overlayWindow_ orderOut:nil];
    [overlayWindow_ release];
    overlayWindow_ = nil;
    originalContentView_ = nil;
  } else {
    NOTREACHED();
  }
}

- (NSWindow*)overlayWindow {
  return overlayWindow_;
}

- (BOOL)shouldConstrainFrameRect {
  // If we currently have an overlay window, do not attempt to change the
  // window's size, as our overlay window doesn't know how to resize properly.
  return overlayWindow_ == nil;
}

- (BOOL)canReceiveFrom:(TabWindowController*)source {
  // subclass must implement
  NOTIMPLEMENTED();
  return NO;
}

- (void)moveTabViews:(NSArray*)views
      fromController:(TabWindowController*)dragController {
  NOTIMPLEMENTED();
}

- (NSArray*)tabViews {
  NOTIMPLEMENTED();
  return nil;
}

- (NSView*)activeTabView {
  NOTIMPLEMENTED();
  return nil;
}

- (void)layoutTabs {
  // subclass must implement
  NOTIMPLEMENTED();
}

- (TabWindowController*)detachTabsToNewWindow:(NSArray*)tabViews
                                   draggedTab:(NSView*)draggedTab {
  // subclass must implement
  NOTIMPLEMENTED();
  return NULL;
}

- (void)detachedWindowEnterFullscreenIfNeeded:(TabWindowController*)source {
  // Subclasses should implement this.
  NOTIMPLEMENTED();
}

- (void)insertPlaceholderForTab:(TabView*)tab frame:(NSRect)frame {
  [self showNewTabButton:NO];
}

- (void)removePlaceholder {
  [self showNewTabButton:YES];
}

- (BOOL)isDragSessionActive {
  NOTIMPLEMENTED();
  return NO;
}

- (BOOL)tabDraggingAllowed {
  return YES;
}

- (BOOL)tabTearingAllowed {
  return YES;
}

- (BOOL)windowMovementAllowed {
  return YES;
}

- (BOOL)isTabFullyVisible:(TabView*)tab {
  // Subclasses should implement this, but it's not necessary.
  return YES;
}

- (void)showNewTabButton:(BOOL)show {
  // subclass must implement
  NOTIMPLEMENTED();
}

- (void)detachTabView:(NSView*)view {
  // subclass must implement
  NOTIMPLEMENTED();
}

- (NSInteger)numberOfTabs {
  // subclass must implement
  NOTIMPLEMENTED();
  return 0;
}

- (BOOL)hasLiveTabs {
  // subclass must implement
  NOTIMPLEMENTED();
  return NO;
}

- (CGFloat)menubarOffset {
  // Subclasses should implement this.
  NOTIMPLEMENTED();
  return 0;
}

- (CGFloat)menubarHeight {
  // The height of the menubar. We can't use |-[NSMenu menuBarHeight]| since it
  // returns 0 when the menu bar is hidden.
  const CGFloat kMenubarHeight = 22;
  return kMenubarHeight;
}

- (BOOL)isInAnyFullscreenMode {
  // Subclass must implement.
  NOTIMPLEMENTED();
  return NO;
}

- (NSView*)avatarView {
  return nil;
}

- (NSString*)activeTabTitle {
  // subclass must implement
  NOTIMPLEMENTED();
  return @"";
}

- (BOOL)hasTabStrip {
  // Subclasses should implement this.
  NOTIMPLEMENTED();
  return YES;
}

- (BOOL)isTabDraggable:(NSView*)tabView {
  // Subclasses should implement this.
  NOTIMPLEMENTED();
  return YES;
}

// Tell the window that it needs to call performClose: as soon as the current
// drag is complete. This prevents a window (and its overlay) from going away
// during a drag.
- (void)deferPerformClose {
  closeDeferred_ = YES;
}

- (void)insertTabStripBackgroundViewIntoWindow:(NSWindow*)window
                                      titleBar:(BOOL)hasTitleBar {
  DCHECK(tabStripBackgroundView_);

  // In Material Design on 10.10 and higher, the top portion of the window is
  // blurred using an NSVisualEffectView.
  if (@available(macOS 10.10, *)) {
    [window setTitlebarAppearsTransparent:YES];

    // If the window has a normal titlebar, then do not add NSVisualEffectView.
    if (hasTitleBar)
      return;

    // NSVisualEffectView provides hints about text anti-aliasing that are wrong
    // when anything is drawn over it (like a tint or theme image). Wrapping it
    // stops it from being used for hints. See https://crbug.com/593835.
    NSView* visualEffectWrapperView = [[[NSView alloc]
        initWithFrame:[tabStripBackgroundView_ frame]] autorelease];

    visualEffectView_.reset([[NSVisualEffectView alloc]
        initWithFrame:visualEffectWrapperView.bounds]);
    DCHECK(visualEffectView_);

    [visualEffectWrapperView
        setAutoresizingMask:[tabStripBackgroundView_ autoresizingMask]];
    [visualEffectView_
        setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [tabStripBackgroundView_
        setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

    // Set to a default appearance and material. If this is an Incognito window
    // the material and vibrancy should be dark but this method gets called at
    // the start of -[BrowserWindowController initWithBrowser:takeOwnership:],
    // before the |browser_| ivar has been set. Without a browser object we
    // can't check the window's theme. The final setup happens in
    // -[TabStripView setController:], at which point we have access to the
    // theme.
    [visualEffectView_
        setAppearance:[NSAppearance
                          appearanceNamed:NSAppearanceNameVibrantLight]];
    [visualEffectView_ setMaterial:NSVisualEffectMaterialLight];
    [visualEffectView_ setBlendingMode:NSVisualEffectBlendingModeBehindWindow];
    [visualEffectView_ setState:NSVisualEffectStateFollowsWindowActiveState];

    [visualEffectWrapperView addSubview:visualEffectView_];

    [[window contentView] addSubview:visualEffectWrapperView];

    // Make the |tabStripBackgroundView_| a child of the NSVisualEffectView.
    [tabStripBackgroundView_ setFrame:[visualEffectView_ bounds]];
    [visualEffectView_ addSubview:tabStripBackgroundView_];
  } else {
    DCHECK(!chrome::ShouldUseFullSizeContentView());
    [[window contentView] addSubview:tabStripBackgroundView_];
  }
}

// Called when the size of the window content area has changed. Override to
// position specific views. Base class implementation does nothing.
- (void)layoutSubviews {
  NOTIMPLEMENTED();
}

- (void)windowWillEnterFullScreenNotification:(NSNotification*)notification {
  [(visualEffectView_ ? visualEffectView_.get()
                      : tabStripBackgroundView_.get()) setHidden:YES];
}

- (void)windowWillExitFullScreenNotification:(NSNotification*)notification {
  [(visualEffectView_ ? visualEffectView_.get()
                      : tabStripBackgroundView_.get()) setHidden:NO];
}

@end
