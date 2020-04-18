// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/browser_window_controller_private.h"

#include <cmath>

#import "base/auto_reset.h"
#include "base/command_line.h"
#include "base/mac/bind_objc_block.h"
#include "base/mac/foundation_util.h"
#include "base/mac/mac_util.h"
#import "base/mac/scoped_nsobject.h"
#import "base/mac/sdk_forward_declarations.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/permissions/permission_request_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/ui/bookmarks/bookmark_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window_state.h"
#import "chrome/browser/ui/cocoa/browser/exclusive_access_controller_views.h"
#import "chrome/browser/ui/cocoa/browser_window_fullscreen_transition.h"
#import "chrome/browser/ui/cocoa/browser_window_layout.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet_controller.h"
#import "chrome/browser/ui/cocoa/dev_tools_controller.h"
#import "chrome/browser/ui/cocoa/fast_resize_view.h"
#import "chrome/browser/ui/cocoa/find_bar/find_bar_cocoa_controller.h"
#import "chrome/browser/ui/cocoa/floating_bar_backing_view.h"
#import "chrome/browser/ui/cocoa/framed_browser_window.h"
#import "chrome/browser/ui/cocoa/fullscreen/fullscreen_toolbar_controller.h"
#import "chrome/browser/ui/cocoa/fullscreen_window.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_container_controller.h"
#include "chrome/browser/ui/cocoa/last_active_browser_cocoa.h"
#include "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#import "chrome/browser/ui/cocoa/profiles/avatar_button_controller.h"
#import "chrome/browser/ui/cocoa/profiles/avatar_icon_controller.h"
#import "chrome/browser/ui/cocoa/status_bubble_mac.h"
#import "chrome/browser/ui/cocoa/tab_contents/overlayable_contents_controller.h"
#import "chrome/browser/ui/cocoa/tab_contents/tab_contents_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_view.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/cocoa/appkit_utils.h"
#import "ui/base/cocoa/focus_tracker.h"
#import "ui/base/cocoa/nsview_additions.h"
#include "ui/base/ui_base_types.h"

using content::RenderWidgetHostView;
using content::WebContents;

@interface NSView (PrivateAPI)
// Returns the fullscreen button's origin in window coordinates. This method is
// only available on NSThemeFrame (the contentView's superview), and it should
// not be relied on to exist on macOS >10.9 (which doesn't have a separate
// fullscreen button). TabbedBrowserWindow's NSThemeFrame subclass centers it
// vertically in the tabstrip (if there is a tabstrip), and shifts it to the
// left of the old-style avatar icon if necessary.
- (NSPoint)_fullScreenButtonOrigin;
@end

namespace {

// The screen on which the window was fullscreened, and whether the device had
// multiple screens available.
enum WindowLocation {
  PRIMARY_SINGLE_SCREEN = 0,
  PRIMARY_MULTIPLE_SCREEN = 1,
  SECONDARY_MULTIPLE_SCREEN = 2,
  WINDOW_LOCATION_COUNT = 3
};

}  // namespace

@interface NSWindow (NSPrivateApis)
// Note: These functions are private, use -[NSObject respondsToSelector:]
// before calling them.
- (NSWindow*)_windowForToolbar;
@end

@implementation BrowserWindowController(Private)

// Create the tab strip controller.
- (void)createTabStripController {
  DCHECK([overlayableContentsController_ activeContainer]);
  DCHECK([[overlayableContentsController_ activeContainer] window]);
  tabStripController_.reset([[TabStripController alloc]
      initWithView:[self tabStripView]
        switchView:[overlayableContentsController_ activeContainer]
           browser:browser_.get()
          delegate:self]);
}

- (void)updateFullscreenCollectionBehavior {
  // Set the window to participate in Lion Fullscreen mode.  Setting this flag
  // has no effect on Snow Leopard or earlier.  Panels can share a fullscreen
  // space with a tabbed window, but they can not be primary fullscreen
  // windows.
  // This ensures the fullscreen button is appropriately positioned. It must
  // be done before calling layoutSubviews because the new avatar button's
  // position depends on the fullscreen button's position, as well as
  // TabStripController's trailingIndentForControls.
  // The fullscreen button's position may depend on the old avatar button's
  // width, but that does not require calling layoutSubviews first.
  NSWindow* window = [self window];
  NSUInteger collectionBehavior = [window collectionBehavior];
  collectionBehavior &= ~NSWindowCollectionBehaviorFullScreenAuxiliary;
  collectionBehavior &= ~NSWindowCollectionBehaviorFullScreenPrimary;
  collectionBehavior |= browser_->type() == Browser::TYPE_TABBED ||
                                browser_->type() == Browser::TYPE_POPUP
                            ? NSWindowCollectionBehaviorFullScreenPrimary
                            : NSWindowCollectionBehaviorFullScreenAuxiliary;
  [window setCollectionBehavior:collectionBehavior];
}

- (void)saveWindowPositionIfNeeded {
  if (!chrome::ShouldSaveWindowPlacement(browser_.get()))
    return;

  // If we're in fullscreen mode, save the position of the regular window
  // instead.
  NSWindow* window =
      [self isInAnyFullscreenMode] ? savedRegularWindow_ : [self window];

  // Window positions are stored relative to the origin of the primary monitor.
  NSRect monitorFrame = [[[NSScreen screens] firstObject] frame];
  NSScreen* windowScreen = [window screen];

  // Start with the window's frame, which is in virtual coordinates.
  // Do some y twiddling to flip the coordinate system.
  gfx::Rect bounds(NSRectToCGRect([window frame]));
  bounds.set_y(monitorFrame.size.height - bounds.y() - bounds.height());

  // Browser::SaveWindowPlacement saves information for session restore.
  ui::WindowShowState show_state = ui::SHOW_STATE_NORMAL;
  if ([window isMiniaturized])
    show_state = ui::SHOW_STATE_MINIMIZED;
  else if ([self isInAnyFullscreenMode])
    show_state = ui::SHOW_STATE_FULLSCREEN;
  chrome::SaveWindowPlacement(browser_.get(), bounds, show_state);

  // |windowScreen| can be nil (for example, if the monitor arrangement was
  // changed while in fullscreen mode).  If we see a nil screen, return without
  // saving.
  // TODO(rohitrao): We should just not save anything for fullscreen windows.
  // http://crbug.com/36479.
  if (!windowScreen)
    return;

  // Only save main window information to preferences.
  PrefService* prefs = browser_->profile()->GetPrefs();
  if (!prefs || browser_.get() != chrome::GetLastActiveBrowser())
    return;

  // Save the current work area, in flipped coordinates.
  gfx::Rect workArea(NSRectToCGRect([windowScreen visibleFrame]));
  workArea.set_y(monitorFrame.size.height - workArea.y() - workArea.height());

  std::unique_ptr<DictionaryPrefUpdate> update =
      chrome::GetWindowPlacementDictionaryReadWrite(
          chrome::GetWindowName(browser_.get()),
          browser_->profile()->GetPrefs());
  base::DictionaryValue* windowPreferences = update->Get();
  windowPreferences->SetInteger("left", bounds.x());
  windowPreferences->SetInteger("top", bounds.y());
  windowPreferences->SetInteger("right", bounds.right());
  windowPreferences->SetInteger("bottom", bounds.bottom());
  windowPreferences->SetBoolean("maximized", false);
  windowPreferences->SetBoolean("always_on_top", false);
  windowPreferences->SetInteger("work_area_left", workArea.x());
  windowPreferences->SetInteger("work_area_top", workArea.y());
  windowPreferences->SetInteger("work_area_right", workArea.right());
  windowPreferences->SetInteger("work_area_bottom", workArea.bottom());
}

- (NSRect)window:(NSWindow*)window
willPositionSheet:(NSWindow*)sheet
       usingRect:(NSRect)defaultSheetLocation {
  // Position the sheet as follows:
  //  - If the bookmark bar is shown (attached to the normal toolbar), position
  //    the sheet below the bookmark bar.
  //  - If the bookmark bar is hidden or shown as a bubble (on the NTP when the
  //    bookmark bar is disabled), position the sheet immediately below the
  //    normal toolbar.
  //  - If the bookmark bar is currently animating, position the sheet according
  //    to where the bar will be when the animation ends.
  CGFloat defaultSheetY = defaultSheetLocation.origin.y;
  if ([self supportsBookmarkBar] &&
      [bookmarkBarController_ currentState] == BookmarkBar::SHOW) {
    defaultSheetY = NSMinY([[bookmarkBarController_ view] frame]);
  } else if ([self hasToolbar]) {
    defaultSheetY = NSMinY([[toolbarController_ view] frame]);
  } else {
    // The toolbar is not shown in popup and application modes. The sheet
    // should be located at the top of the window, under the title of the
    // window.
    defaultSheetY = NSMaxY([[window contentView] frame]);
  }

  // AppKit may shift the window up to fit the sheet on screen, but it will
  // never adjust the height of the sheet, or the origin of the sheet relative
  // to the window. Adjust the origin to prevent sheets from extending past the
  // bottom of the screen.

  // Don't allow the sheet to extend past the bottom of the window. This logic
  // intentionally ignores the size of the screens, since the window might span
  // multiple screens, and AppKit may reposition the window.
  CGFloat sheetHeight = NSHeight([sheet frame]);
  defaultSheetY = std::max(defaultSheetY, sheetHeight);

  // It doesn't make sense to provide a Y higher than the height of the window.
  CGFloat windowHeight = NSHeight([window frame]);
  defaultSheetY = std::min(defaultSheetY, windowHeight);

  defaultSheetLocation.origin.y = defaultSheetY;
  return defaultSheetLocation;
}

- (void)layoutSubviews {
  // TODO(spqchan): Change blockLayoutSubviews so that it only blocks the web
  // content from resizing.
  if (blockLayoutSubviews_)
    return;

  // Suppress title drawing if necessary.
  if ([self.window respondsToSelector:@selector(setShouldHideTitle:)])
    [(id)self.window setShouldHideTitle:![self hasTitleBar]];

  [bookmarkBarController_ updateHiddenState];
  [self updateSubviewZOrder];

  base::scoped_nsobject<BrowserWindowLayout> layout(
      [[BrowserWindowLayout alloc] init]);
  [self updateLayoutParameters:layout];
  [self applyLayout:layout];

  [toolbarController_ setDividerOpacity:[self toolbarDividerOpacity]];

  // Will update the location of the permission bubble when showing/hiding the
  // top level toolbar in fullscreen.
  [self updatePermissionBubbleAnchor];

  browser_->GetBubbleManager()->UpdateAllBubbleAnchors();
}

- (void)applyTabStripLayout:(const chrome::TabStripLayout&)layout {
  // Update the presence of the window controls.
  if (layout.addCustomWindowControls)
    [tabStripController_ addCustomWindowControls];
  else
    [tabStripController_ removeCustomWindowControls];

  // Update the layout of the avatar.
  if (!NSIsEmptyRect(layout.avatarFrame)) {
    NSView* avatarButton = [avatarButtonController_ view];
    [avatarButton setFrame:layout.avatarFrame];
    [avatarButton setHidden:NO];
  }

  // Check if the tab strip's frame has changed.
  BOOL requiresRelayout =
      !NSEqualRects([[self tabStripView] frame], layout.frame);

  // Check if the leading indent has changed.
  if (layout.leadingIndent != [tabStripController_ leadingIndentForControls]) {
    [tabStripController_ setLeadingIndentForControls:layout.leadingIndent];
    requiresRelayout = YES;
  }

  // Check if the trailing indent has changed.
  if (layout.trailingIndent !=
      [tabStripController_ trailingIndentForControls]) {
    [tabStripController_ setTrailingIndentForControls:layout.trailingIndent];
    requiresRelayout = YES;
  }

  // It is undesirable to force tabs relayout when the tap strip's frame did
  // not change, because it will interrupt tab animations in progress.
  // In addition, there appears to be an AppKit bug on <10.9 where interrupting
  // a tab animation resulted in the tab frame being the animator's target
  // frame instead of the interrupting setFrame. (See http://crbug.com/415093)
  if (requiresRelayout) {
    [[self tabStripView] setFrame:layout.frame];
    [tabStripController_ layoutTabsWithoutAnimation];
  }
}

- (BOOL)placeBookmarkBarBelowInfoBar {
  // If we are currently displaying the NTP detached bookmark bar or animating
  // to/from it (from/to anything else), we display the bookmark bar below the
  // info bar.
  return [bookmarkBarController_ isInState:BookmarkBar::DETACHED] ||
         [bookmarkBarController_ isAnimatingToState:BookmarkBar::DETACHED] ||
         [bookmarkBarController_ isAnimatingFromState:BookmarkBar::DETACHED];
}

- (void)layoutTabContentArea:(NSRect)newFrame {
  NSView* tabContentView = [self tabContentArea];
  NSRect tabContentFrame = [tabContentView frame];

  tabContentFrame = newFrame;
  [tabContentView setFrame:tabContentFrame];
}

- (void)adjustToolbarAndBookmarkBarForCompression:(CGFloat)compression {
  CGFloat newHeight =
      [toolbarController_ desiredHeightForCompression:compression];
  NSRect toolbarFrame = [[toolbarController_ view] frame];
  CGFloat deltaH = newHeight - toolbarFrame.size.height;

  if (deltaH == 0)
    return;

  toolbarFrame.size.height = newHeight;
  NSRect bookmarkFrame = [[bookmarkBarController_ view] frame];
  bookmarkFrame.size.height = bookmarkFrame.size.height - deltaH;
  [[toolbarController_ view] setFrame:toolbarFrame];
  [[bookmarkBarController_ view] setFrame:bookmarkFrame];
  [self layoutSubviews];
}

// Fullscreen methods

- (void)moveViewsForImmersiveFullscreen:(BOOL)fullscreen
                          regularWindow:(NSWindow*)regularWindow
                       fullscreenWindow:(NSWindow*)fullscreenWindow {
  NSWindow* sourceWindow = fullscreen ? regularWindow : fullscreenWindow;
  NSWindow* destWindow = fullscreen ? fullscreenWindow : regularWindow;

  // Close the bookmark bubble, if it's open.  Use |-ok:| instead of |-cancel:|
  // or |-close| because that matches the behavior when the bubble loses key
  // status.
  [bookmarkBubbleController_ ok:self];

  // Save the current first responder so we can restore after views are moved.
  base::scoped_nsobject<FocusTracker> focusTracker(
      [[FocusTracker alloc] initWithWindow:sourceWindow]);

  // Retain the tab strip view while we remove it from its superview.
  base::scoped_nsobject<NSView> tabStripView;
  if ([self hasTabStrip]) {
    tabStripView.reset([[self tabStripView] retain]);
    [tabStripView removeFromSuperview];
  }

  // Disable autoresizing of subviews while we move views around. This prevents
  // spurious renderer resizes.
  [self.chromeContentView setAutoresizesSubviews:NO];
  [self.chromeContentView removeFromSuperview];

  // Have to do this here, otherwise later calls can crash because the window
  // has no delegate.
  [sourceWindow setDelegate:nil];
  [destWindow setDelegate:[self nsWindowController]];

  // With this call, valgrind complains that a "Conditional jump or move depends
  // on uninitialised value(s)".  The error happens in -[NSThemeFrame
  // drawOverlayRect:].  I'm pretty convinced this is an Apple bug, but there is
  // no visual impact.  I have been unable to tickle it away with other window
  // or view manipulation Cocoa calls.  Stack added to suppressions_mac.txt.
  [self.chromeContentView setAutoresizesSubviews:YES];
  [[destWindow contentView] addSubview:self.chromeContentView
                            positioned:NSWindowBelow
                            relativeTo:nil];
  [self.chromeContentView setFrame:[[destWindow contentView] bounds]];

  // Move the incognito badge if present.
  if ([self shouldShowAvatar]) {
    NSView* avatarButtonView = [avatarButtonController_ view];

    [avatarButtonView removeFromSuperview];
    [avatarButtonView setHidden:YES];  // Will be shown in layout.
    [[destWindow contentView] addSubview:avatarButtonView];
  }

  // Add the tab strip after setting the content view and moving the incognito
  // badge (if any), so that the tab strip will be on top (in the z-order).
  if ([self hasTabStrip])
    [[destWindow contentView] addSubview:tabStripView];

  [sourceWindow setWindowController:nil];
  [self setWindow:destWindow];
  [destWindow setWindowController:[self nsWindowController]];

  // Move the status bubble over, if we have one.
  if (statusBubble_)
    statusBubble_->SwitchParentWindow(destWindow);

  [self updatePermissionBubbleAnchor];

  // Move the title over.
  [destWindow setTitle:[sourceWindow title]];

  // The window needs to be onscreen before we can set its first responder.
  // Ordering the window to the front can change the active Space (either to
  // the window's old Space or to the application's assigned Space). To prevent
  // this by temporarily change the collectionBehavior.
  NSWindowCollectionBehavior behavior = [sourceWindow collectionBehavior];
  [destWindow setCollectionBehavior:
      NSWindowCollectionBehaviorMoveToActiveSpace];
  [destWindow makeKeyAndOrderFront:self];
  [destWindow setCollectionBehavior:behavior];

  if (![focusTracker restoreFocusInWindow:destWindow]) {
    // During certain types of fullscreen transitions, the view that had focus
    // may have gone away (e.g., the one for a Flash FS widget).  In this case,
    // FocusTracker will fail to restore focus to anything, so we set the focus
    // to the tab contents as a reasonable fall-back.
    [self focusTabContents];
  }
  [sourceWindow orderOut:self];
}

- (void)updatePermissionBubbleAnchor {
  PermissionRequestManager* manager = [self permissionRequestManager];
  if (manager)
    manager->UpdateAnchorPosition();
}

- (void)enterImmersiveFullscreen {
  [self recordEnterFullscreenMetrics:IMMERSIVE_FULLSCREEN];

  // Set to NO by |-windowDidEnterFullScreen:|.
  enteringImmersiveFullscreen_ = YES;

  // Fade to black.
  const CGDisplayReservationInterval kFadeDurationSeconds = 0.6;
  Boolean didFadeOut = NO;
  CGDisplayFadeReservationToken token;
  if (CGAcquireDisplayFadeReservation(kFadeDurationSeconds, &token)
      == kCGErrorSuccess) {
    didFadeOut = YES;
    CGDisplayFade(token, kFadeDurationSeconds / 2, kCGDisplayBlendNormal,
        kCGDisplayBlendSolidColor, 0.0, 0.0, 0.0, /*synchronous=*/true);
  }

  // Create the fullscreen window.
  fullscreenWindow_.reset([[self createFullscreenWindow] retain]);
  savedRegularWindow_ = [[self window] retain];
  savedRegularWindowFrame_ = [savedRegularWindow_ frame];

  [self moveViewsForImmersiveFullscreen:YES
                          regularWindow:[self window]
                       fullscreenWindow:fullscreenWindow_.get()];
  [self adjustUIForEnteringFullscreen];

  [fullscreenWindow_ display];

  // AppKit is helpful and prevents NSWindows from having the same height as
  // the screen while the menu bar is showing. This only applies to windows on
  // a secondary screen, in a separate space. Calling [NSWindow
  // setFrame:display:] with the screen's height will always reduce the
  // height by the height of the MenuBar. Calling the method with any other
  // height works fine. The relevant method in the 10.10 AppKit SDK is called:
  // _canAdjustSizeForScreensHaveSeparateSpacesIfFillingSecondaryScreen
  //
  // TODO(erikchen): Refactor the logic to allow the window to be shown after
  // the menubar has been hidden. This would remove the need for this hack.
  // http://crbug.com/403203
  NSRect frame = [[[self window] screen] frame];
  if (!NSEqualRects(frame, [fullscreenWindow_ frame]))
    [fullscreenWindow_ setFrame:[[[self window] screen] frame] display:YES];

  [self layoutSubviews];

  [self windowDidEnterFullScreen:nil];

  // Fade back in.
  if (didFadeOut) {
    CGDisplayFade(token, kFadeDurationSeconds / 2, kCGDisplayBlendSolidColor,
        kCGDisplayBlendNormal, 0.0, 0.0, 0.0, /*synchronous=*/false);
    CGReleaseDisplayFadeReservation(token);
  }
}

- (void)exitImmersiveFullscreen {
  // Fade to black.
  const CGDisplayReservationInterval kFadeDurationSeconds = 0.6;
  Boolean didFadeOut = NO;
  CGDisplayFadeReservationToken token;
  if (CGAcquireDisplayFadeReservation(kFadeDurationSeconds, &token)
      == kCGErrorSuccess) {
    didFadeOut = YES;
    CGDisplayFade(token, kFadeDurationSeconds / 2, kCGDisplayBlendNormal,
        kCGDisplayBlendSolidColor, 0.0, 0.0, 0.0, /*synchronous=*/true);
  }

  [self windowWillExitFullScreen:nil];

  [self moveViewsForImmersiveFullscreen:NO
                          regularWindow:savedRegularWindow_
                       fullscreenWindow:fullscreenWindow_.get()];

  // When exiting fullscreen mode, we need to call layoutSubviews manually.
  [savedRegularWindow_ autorelease];
  savedRegularWindow_ = nil;

  // No close event is thrown when a window is dealloc'd after orderOut.
  // Explicitly close the window to notify bubbles.
  [fullscreenWindow_.get() close];
  fullscreenWindow_.reset();
  [self layoutSubviews];

  [self windowDidExitFullScreen:nil];

  // Fade back in.
  if (didFadeOut) {
    CGDisplayFade(token, kFadeDurationSeconds / 2, kCGDisplayBlendSolidColor,
        kCGDisplayBlendNormal, 0.0, 0.0, 0.0, /*synchronous=*/false);
    CGReleaseDisplayFadeReservation(token);
  }
}

- (void)showFullscreenExitBubbleIfNecessary {
  // This method is called in response to
  // |-updateFullscreenExitBubbleURL:bubbleType:|. If we're in the middle of the
  // transition into fullscreen (i.e., using the AppKit Fullscreen API), do not
  // show the bubble because it will cause visual jank
  // (http://crbug.com/130649). This will be called again as part of
  // |-windowDidEnterFullScreen:|, so arrange to do that work then instead.
  if (enteringAppKitFullscreen_)
    return;

  switch (exclusiveAccessController_->bubble_type()) {
    case EXCLUSIVE_ACCESS_BUBBLE_TYPE_NONE:
    case EXCLUSIVE_ACCESS_BUBBLE_TYPE_BROWSER_FULLSCREEN_EXIT_INSTRUCTION:
      // Show no exit instruction bubble on Mac when in Browser Fullscreen.
      exclusiveAccessController_->Destroy();
      break;

    default:
      exclusiveAccessController_->Show();
  }
}

- (void)contentViewDidResize:(NSNotification*)notification {
  [self layoutSubviews];
}

- (void)registerForContentViewResizeNotifications {
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(contentViewDidResize:)
             name:NSViewFrameDidChangeNotification
           object:[[self window] contentView]];
}

- (void)deregisterForContentViewResizeNotifications {
  [[NSNotificationCenter defaultCenter]
      removeObserver:self
                name:NSViewFrameDidChangeNotification
              object:[[self window] contentView]];
}

- (NSSize)window:(NSWindow*)window
    willUseFullScreenContentSize:(NSSize)proposedSize {
  return proposedSize;
}

- (void)windowWillEnterFullScreen:(NSNotification*)notification {
  [self recordEnterFullscreenMetrics:CANONICAL_FULLSCREEN];

  if (notification)  // For System Fullscreen when non-nil.
    [self registerForContentViewResizeNotifications];

  [[tabStripController_ activeTabContentsController]
      setBlockFullscreenResize:YES];

  NSWindow* window = [self window];
  savedRegularWindowFrame_ = [window frame];

  enteringAppKitFullscreen_ = YES;
  enteringAppKitFullscreenOnPrimaryScreen_ =
      [[[self window] screen] isEqual:[[NSScreen screens] firstObject]];

  [self setSheetHiddenForFullscreenTransition:YES];
  [self adjustUIForEnteringFullscreen];
  browser_->WindowFullscreenStateWillChange();
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification {
  [tabStripController_ setVisualEffectsDisabledForFullscreen:YES];

  // In Yosemite, some combination of the titlebar and toolbar always show in
  // full-screen mode. We do not want either to show. Search for the window
  // that contains the views, and hide it if the window contains our custom
  // toolbar. There is no need to ever unhide the view. http://crbug.com/380235
  if (base::mac::IsAtLeastOS10_10()) {
    for (NSWindow* window in [[NSApplication sharedApplication] windows]) {
      if (![window
              isKindOfClass:NSClassFromString(@"NSToolbarFullScreenWindow")]) {
        continue;
      }

      // Hide the toolbar if it's for a FramedBrowserWindow and the
      // FramedBrowserWindow doesn't contain our custom toolbar view.
      if (![window respondsToSelector:@selector(_windowForToolbar)])
        continue;

      NSWindow* windowForToolbar = [window _windowForToolbar];
      if ([windowForToolbar isKindOfClass:[FramedBrowserWindow class]]) {
        BrowserWindowController* bwc = [BrowserWindowController
            browserWindowControllerForWindow:windowForToolbar];
        if ([bwc hasToolbar])
          [[window contentView] setHidden:YES];
      }
    }
  }

  if (notification)  // For System Fullscreen when non-nil.
    [self deregisterForContentViewResizeNotifications];

  enteringImmersiveFullscreen_ = NO;

  [self resetCustomAppKitFullscreenVariables];
  [[tabStripController_ activeTabContentsController]
      updateFullscreenWidgetFrame];

  [self invalidateTouchBar];

  [self showFullscreenExitBubbleIfNecessary];
  browser_->WindowFullscreenStateChanged();

  if (shouldExitAfterEnteringFullscreen_) {
    shouldExitAfterEnteringFullscreen_ = NO;

    // At 10.13, -windowDidEnteredFullscreen: is called before the AppKit
    // fullscreen transition is complete. This causes AppKit to emit "not in
    // fullscreen state" and ignore the call when we try to toggle fullscreen
    // in the same runloop that entered it. To handle this case, invoke
    // -toggleFullscreen: asynchronously.
    [self exitAppKitFullscreenAsync:!base::mac::IsAtMostOS10_12()];
  }

  // In macOS 10.12 and earlier, the web content's NSTrackingInVisibleRect
  // doesn't work correctly after the window enters fullscreen (See
  // https://crbug.com/170058). To work around it, update the tracking area
  // after we enter fullscreen.
  if (base::mac::IsAtMostOS10_12()) {
    WebContents* webContents = [self webContents];
    if (webContents && webContents->GetRenderWidgetHostView()) {
      [webContents->GetRenderWidgetHostView()->GetNativeView()
              updateTrackingAreas];
    }
  }
}

- (void)windowWillExitFullScreen:(NSNotification*)notification {
  [tabStripController_ setVisualEffectsDisabledForFullscreen:NO];

  // macOS 10.12 and earlier have issues with exiting fullscreen while a window
  // is on the detached/low power path (playing a video with no UI visible).
  // See crbug/644133 for some discussion. This workaround kicks the window off
  // the low power path as the transition begins.
  if (base::mac::IsAtMostOS10_12()) {
    CALayer* detachmentBlockerLayer = [CALayer layer];
    detachmentBlockerLayer.backgroundColor =
        CGColorGetConstantColor(kCGColorBlack);
    detachmentBlockerLayer.frame = CGRectMake(0, 0, 1, 1);

    [CATransaction begin];
    [CATransaction setCompletionBlock:^{
      [detachmentBlockerLayer removeFromSuperlayer];
    }];
    [self.window.contentView.layer addSublayer:detachmentBlockerLayer];
    [CATransaction commit];
  }

  if (notification)  // For System Fullscreen when non-nil.
    [self registerForContentViewResizeNotifications];
  exitingAppKitFullscreen_ = YES;

  // Like windowWillEnterFullScreen, if we use custom animations,
  // adjustUIForExitingFullscreen should be called after the layout resizes in
  // startCustomAnimationToExitFullScreenWithDuration.
  if (isUsingCustomAnimation_) {
    blockLayoutSubviews_ = YES;
    [self.chromeContentView setAutoresizesSubviews:NO];
    [self setSheetHiddenForFullscreenTransition:YES];
  } else {
    [self adjustUIForExitingFullscreen];
  }
  browser_->WindowFullscreenStateWillChange();
}

- (void)windowDidExitFullScreen:(NSNotification*)notification {
  DCHECK(exitingAppKitFullscreen_);

  // If the custom transition isn't complete, then just set the flag and
  // return. Once the transition is completed, windowDidExitFullscreen will
  // be called again.
  if (isUsingCustomAnimation_ &&
      ![fullscreenTransition_ isTransitionCompleted]) {
    appKitDidExitFullscreen_ = YES;
    return;
  }

  if (notification)  // For System Fullscreen when non-nil.
    [self deregisterForContentViewResizeNotifications];

  browser_->WindowFullscreenStateChanged();
  [self.chromeContentView setAutoresizesSubviews:YES];

  [self releaseToolbarVisibilityForOwner:self withAnimation:NO];

  [self resetCustomAppKitFullscreenVariables];

  [self invalidateTouchBar];

  // Ensures that the permission bubble shows up properly at the front.
  PermissionRequestManager* manager = [self permissionRequestManager];
  if (manager && manager->IsBubbleVisible()) {
    NSWindow* bubbleWindow = manager->GetBubbleWindow();
    DCHECK(bubbleWindow);
    [bubbleWindow orderFront:nil];
  }

  // Ensure that the window is layout properly.
  [self layoutSubviews];
}

- (void)windowDidFailToEnterFullScreen:(NSWindow*)window {
  [self deregisterForContentViewResizeNotifications];
  [self resetCustomAppKitFullscreenVariables];
  [self adjustUIForExitingFullscreen];
}

- (void)windowDidFailToExitFullScreen:(NSWindow*)window {
  [self deregisterForContentViewResizeNotifications];
  [self resetCustomAppKitFullscreenVariables];

  // Force a relayout to try and get the window back into a reasonable state.
  [self layoutSubviews];
}

- (void)setSheetHiddenForFullscreenTransition:(BOOL)shoudHide {
  if (!isUsingCustomAnimation_)
    return;

  ConstrainedWindowSheetController* sheetController =
      [ConstrainedWindowSheetController
          controllerForParentWindow:[self window]];
  if (shoudHide)
    [sheetController hideSheetForFullscreenTransition];
  else
    [sheetController unhideSheetForFullscreenTransition];
}

- (void)adjustUIForExitingFullscreen {
  exclusiveAccessController_->Destroy();
  [fullscreenToolbarController_ exitFullscreenMode];
  fullscreenToolbarController_.reset();

  // Force the bookmark bar z-order to update.
  [[bookmarkBarController_ view] removeFromSuperview];
  [self layoutSubviews];
}

- (void)adjustUIForEnteringFullscreen {
  DCHECK([self isInAnyFullscreenMode]);
  if (!fullscreenToolbarController_) {
    fullscreenToolbarController_.reset(
        [[FullscreenToolbarController alloc] initWithBrowserController:self]);
  }

  [fullscreenToolbarController_ enterFullscreenMode];

  if (!floatingBarBackingView_.get() &&
      ([self hasTabStrip] || [self hasToolbar] || [self hasLocationBar])) {
    floatingBarBackingView_.reset(
        [[FloatingBarBackingView alloc] initWithFrame:NSZeroRect]);
    [floatingBarBackingView_
        setAutoresizingMask:(NSViewWidthSizable | NSViewMinYMargin)];
  }

  // Force the bookmark bar z-order to update.
  [[bookmarkBarController_ view] removeFromSuperview];
  [self layoutSubviews];
}

- (void)recordEnterFullscreenMetrics:(FullscreenStyle)style {
  // Record fullscreen source.
  FullscreenController* controller =
      browser_->exclusive_access_manager()->fullscreen_controller();
  FullscreenSource source = FullscreenSource::BROWSER;
  if (controller->IsWindowFullscreenForTabOrPending())
    source = FullscreenSource::TAB;
  else if (controller->IsExtensionFullscreenOrPending())
    source = FullscreenSource::EXTENSION;

  UMA_HISTOGRAM_ENUMERATION("OSX.Fullscreen.Enter.Source", source,
                            FullscreenSource::FULLSCREEN_SOURCE_COUNT);

  // Record fullscreen style.
  UMA_HISTOGRAM_ENUMERATION("OSX.Fullscreen.Enter.Style", style,
                            FULLSCREEN_STYLE_COUNT);

  // Record screen location.
  NSArray* screens = [NSScreen screens];
  bool primary_screen =
      [[[self window] screen] isEqual:[screens objectAtIndex:0]];
  bool multiple_screens = [screens count] > 1;

  WindowLocation location = PRIMARY_SINGLE_SCREEN;
  if (multiple_screens) {
    location =
        primary_screen ? PRIMARY_MULTIPLE_SCREEN : SECONDARY_MULTIPLE_SCREEN;
  }

  UMA_HISTOGRAM_ENUMERATION("OSX.Fullscreen.Enter.WindowLocation", location,
                            WINDOW_LOCATION_COUNT);
}

- (CGFloat)toolbarDividerOpacity {
  return [bookmarkBarController_ toolbarDividerOpacity];
}

- (void)enterAppKitFullscreen {
  [[self window] toggleFullScreen:nil];
}

- (void)exitAppKitFullscreenAsync:(BOOL)async {
  // If we're in the process of entering fullscreen, toggleSystemFullscreen
  // will get ignored. Set |shouldExitAfterEnteringFullscreen_| to true so
  // the browser will exit fullscreen immediately after it enters it.
  if (enteringAppKitFullscreen_) {
    shouldExitAfterEnteringFullscreen_ = YES;
    return;
  }

  if (async) {
    [[self window] performSelector:@selector(toggleFullScreen:)
                        withObject:nil
                        afterDelay:0];
  } else {
    [[self window] toggleFullScreen:nil];
  }
}

- (NSRect)fullscreenButtonFrame {
  NSButton* fullscreenButton =
      [[self window] standardWindowButton:NSWindowFullScreenButton];
  if (!fullscreenButton)
    return NSZeroRect;

  NSRect buttonFrame = [fullscreenButton frame];

  // When called from -windowWillExitFullScreen:, the button's frame may not
  // be updated yet to match the new window size.
  // We need to return where the button should be positioned.
  NSView* rootView = [[[self window] contentView] superview];
  if ([rootView respondsToSelector:@selector(_fullScreenButtonOrigin)])
    buttonFrame.origin = [rootView _fullScreenButtonOrigin];

  return buttonFrame;
}

- (void)updateLayoutParameters:(BrowserWindowLayout*)layout {
  [layout setContentViewSize:[[[self window] contentView] bounds].size];

  NSSize windowSize = (fullscreenTransition_.get())
                          ? [fullscreenTransition_ desiredWindowLayoutSize]
                          : [[self window] frame].size;

  [layout setWindowSize:windowSize];

  [layout setInAnyFullscreen:[self isInAnyFullscreenMode]];

  FullscreenToolbarLayout fullscreenToolbarLayout =
      [fullscreenToolbarController_ computeLayout];
  [layout setFullscreenToolbarStyle:fullscreenToolbarLayout.toolbarStyle];
  [layout setFullscreenMenubarOffset:fullscreenToolbarLayout.menubarOffset];
  [layout setFullscreenToolbarFraction:fullscreenToolbarLayout.toolbarFraction];

  [layout setHasTabStrip:[self hasTabStrip]];
  [layout setFullscreenButtonFrame:[self fullscreenButtonFrame]];

  if ([self shouldShowAvatar]) {
    NSView* avatar = [avatarButtonController_ view];
    [layout setShouldShowAvatar:YES];
    [layout setShouldUseNewAvatar:[self shouldUseNewAvatarButton]];
    [layout
        setIsGenericAvatar:[avatarButtonController_ shouldUseGenericButton]];
    [layout setAvatarSize:[avatar frame].size];
    [layout setAvatarLineWidth:[[avatar superview] cr_lineWidth]];
  }

  [layout setHasToolbar:[self hasToolbar]];
  [layout setToolbarHeight:NSHeight([[toolbarController_ view] bounds])];

  [layout setHasLocationBar:[self hasLocationBar]];

  [layout setPlaceBookmarkBarBelowInfoBar:[self placeBookmarkBarBelowInfoBar]];
  [layout setBookmarkBarHidden:[bookmarkBarController_ view].isHidden];
  [layout setBookmarkBarHeight:
      NSHeight([[bookmarkBarController_ view] bounds])];

  [layout setInfoBarHeight:[infoBarContainerController_ heightOfInfoBars]];

  [layout setHasDownloadShelf:(downloadShelfController_.get() != nil)];
  [layout setDownloadShelfHeight:
      NSHeight([[downloadShelfController_ view] bounds])];
}

- (void)applyLayout:(BrowserWindowLayout*)layout {
  chrome::LayoutOutput output = [layout computeLayout];

  if (!NSIsEmptyRect(output.tabStripLayout.frame))
    [self applyTabStripLayout:output.tabStripLayout];

  if (!NSIsEmptyRect(output.toolbarFrame))
    [[toolbarController_ view] setFrame:output.toolbarFrame];

  if (!NSIsEmptyRect(output.bookmarkFrame))
    [bookmarkBarController_ layoutToFrame:output.bookmarkFrame];

  // The info bar is never hidden. Sometimes it has zero effective height.
  [[infoBarContainerController_ view] setFrame:output.infoBarFrame];

  [[downloadShelfController_ view] setFrame:output.downloadShelfFrame];

  [self layoutTabContentArea:output.contentAreaFrame];

  if (!NSIsEmptyRect(output.fullscreenBackingBarFrame)) {
    [floatingBarBackingView_ setFrame:output.fullscreenBackingBarFrame];
    [fullscreenToolbarController_
        updateToolbarFrame:output.fullscreenBackingBarFrame];
  }

  [findBarCocoaController_
      positionFindBarViewAtMaxY:output.findBarMaxY
                       maxWidth:NSWidth(output.contentAreaFrame)];
}

- (void)updateSubviewZOrder {
  if ([self isInAnyFullscreenMode])
    [self updateSubviewZOrderFullscreen];
  else
    [self updateSubviewZOrderNormal];
}

- (void)updateSubviewZOrderNormal {
  base::scoped_nsobject<NSMutableArray> subviews([[NSMutableArray alloc] init]);
  if ([downloadShelfController_ view])
    [subviews addObject:[downloadShelfController_ view]];
  if ([bookmarkBarController_ view])
    [subviews addObject:[bookmarkBarController_ view]];
  if ([toolbarController_ view])
    [subviews addObject:[toolbarController_ view]];
  if ([infoBarContainerController_ view])
    [subviews addObject:[infoBarContainerController_ view]];
  if ([self tabContentArea])
    [subviews addObject:[self tabContentArea]];
  if ([findBarCocoaController_ view])
    [subviews addObject:[findBarCocoaController_ view]];

  [self setContentViewSubviews:subviews];
}

- (void)updateSubviewZOrderFullscreen {
  base::scoped_nsobject<NSMutableArray> subviews([[NSMutableArray alloc] init]);

  if ([downloadShelfController_ view])
    [subviews addObject:[downloadShelfController_ view]];
  if ([self tabContentArea])
    [subviews addObject:[self tabContentArea]];

  if ([self placeBookmarkBarBelowInfoBar]) {
    if ([bookmarkBarController_ view])
      [subviews addObject:[bookmarkBarController_ view]];
    if (floatingBarBackingView_)
      [subviews addObject:floatingBarBackingView_];
  } else {
    if (floatingBarBackingView_)
      [subviews addObject:floatingBarBackingView_];
    if ([bookmarkBarController_ view])
      [subviews addObject:[bookmarkBarController_ view]];
  }

  if ([toolbarController_ view])
    [subviews addObject:[toolbarController_ view]];

  if ([infoBarContainerController_ view])
    [subviews addObject:[infoBarContainerController_ view]];

  if ([findBarCocoaController_ view])
    [subviews addObject:[findBarCocoaController_ view]];

  [self setContentViewSubviews:subviews];
}

- (void)setContentViewSubviews:(NSArray*)subviews {
  // Subviews already match.
  if ([[self.chromeContentView subviews] isEqual:subviews])
    return;

  // The tabContentArea isn't a subview, so just set all the subviews.
  NSView* tabContentArea = [self tabContentArea];
  if (![[self.chromeContentView subviews] containsObject:tabContentArea]) {
    [self.chromeContentView setSubviews:subviews];
    return;
  }

  // Removing the location bar from the window causes it to resign first
  // responder. Remember the location bar's focus state in order to restore
  // it before returning.
  BOOL locationBarHadFocus = [toolbarController_ locationBarHasFocus];
  FullscreenToolbarVisibilityLockController* visibilityLockController = nil;
  if (locationBarHadFocus) {
    // The location bar, by being focused, has a visibility lock on the toolbar,
    // and the location bar's removal from the view hierarchy will allow the
    // toolbar to hide. Create a temporary visibility lock on the toolbar for
    // the duration of the view hierarchy change.
    visibilityLockController = [self fullscreenToolbarVisibilityLockController];
    [visibilityLockController lockToolbarVisibilityForOwner:self
                                              withAnimation:NO];
  }

  // Remove all subviews that aren't the tabContentArea.
  for (NSView* view in [[[self.chromeContentView subviews] copy] autorelease]) {
    if (view != tabContentArea)
      [view removeFromSuperview];
  }

  // Add in the subviews below the tabContentArea.
  NSInteger index = [subviews indexOfObject:tabContentArea];
  for (int i = index - 1; i >= 0; --i) {
    NSView* view = [subviews objectAtIndex:i];
    [self.chromeContentView addSubview:view
                            positioned:NSWindowBelow
                            relativeTo:nil];
  }

  // Add in the subviews above the tabContentArea.
  for (NSUInteger i = index + 1; i < [subviews count]; ++i) {
    NSView* view = [subviews objectAtIndex:i];
    [self.chromeContentView addSubview:view
                            positioned:NSWindowAbove
                            relativeTo:nil];
  }

  // Restore the location bar's focus state and remove the temporary visibility
  // lock.
  if (locationBarHadFocus) {
    [self focusLocationBar:YES];
    [visibilityLockController releaseToolbarVisibilityForOwner:self
                                                 withAnimation:NO];
  }
}

- (BOOL)shouldUseCustomAppKitFullscreenTransition:(BOOL)enterFullScreen {
  // Use the native transition on 10.13+: https://crbug.com/741478.
  if (base::mac::IsAtLeastOS10_13())
    return NO;

  // Disable the custom exit animation in OSX 10.9: http://crbug.com/526327#c3.
  if (base::mac::IsOS10_9() && !enterFullScreen)
    return NO;

  NSView* root = [[self.window contentView] superview];
  if (!root.layer)
    return NO;

  return YES;
}

- (void)resetCustomAppKitFullscreenVariables {
  [self setSheetHiddenForFullscreenTransition:NO];
  [self.chromeContentView setAutoresizesSubviews:YES];

  fullscreenTransition_.reset();
  [[tabStripController_ activeTabContentsController]
      setBlockFullscreenResize:NO];
  blockLayoutSubviews_ = NO;

  enteringAppKitFullscreen_ = NO;
  exitingAppKitFullscreen_ = NO;
  isUsingCustomAnimation_ = NO;
}

- (NSArray*)customWindowsToEnterFullScreenForWindow:(NSWindow*)window {
  DCHECK([window isEqual:self.window]);

  if (![self shouldUseCustomAppKitFullscreenTransition:YES])
    return nil;

  fullscreenTransition_.reset(
      [[BrowserWindowFullscreenTransition alloc] initEnterWithController:self]);

  NSArray* customWindows =
      [fullscreenTransition_ customWindowsForFullScreenTransition];

  isUsingCustomAnimation_ = customWindows != nil;
  return customWindows;
}

- (NSArray*)customWindowsToExitFullScreenForWindow:(NSWindow*)window {
  DCHECK([window isEqual:self.window]);

  if (![self shouldUseCustomAppKitFullscreenTransition:NO])
    return nil;

  fullscreenTransition_.reset(
      [[BrowserWindowFullscreenTransition alloc] initExitWithController:self]);

  NSArray* customWindows =
      [fullscreenTransition_ customWindowsForFullScreenTransition];
  isUsingCustomAnimation_ = customWindows != nil;
  return customWindows;
}

- (void)window:(NSWindow*)window
    startCustomAnimationToEnterFullScreenWithDuration:(NSTimeInterval)duration {
  DCHECK([window isEqual:self.window]);
  [fullscreenTransition_ startCustomFullScreenAnimationWithDuration:duration];
}

- (void)window:(NSWindow*)window
    startCustomAnimationToExitFullScreenWithDuration:(NSTimeInterval)duration {
  DCHECK([window isEqual:self.window]);

  [fullscreenTransition_ startCustomFullScreenAnimationWithDuration:duration];

  base::AutoReset<BOOL> autoReset(&blockLayoutSubviews_, NO);
  [self adjustUIForExitingFullscreen];
}

- (BOOL)shouldConstrainFrameRect {
  if ([fullscreenTransition_ shouldWindowBeUnconstrained])
    return NO;

  return [super shouldConstrainFrameRect];
}

- (WebContents*)webContents {
  return browser_->tab_strip_model()->GetActiveWebContents();
}

- (PermissionRequestManager*)permissionRequestManager {
  if (WebContents* contents = [self webContents])
    return PermissionRequestManager::FromWebContents(contents);
  return nil;
}

- (FullscreenToolbarVisibilityLockController*)
    fullscreenToolbarVisibilityLockController {
  return [fullscreenToolbarController_ visibilityLockController];
}

@end  // @implementation BrowserWindowController(Private)
