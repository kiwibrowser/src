// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_CONTROLLER_H_

// A class acting as the Objective-C controller for the Browser
// object. Handles interactions between Cocoa and the cross-platform
// code. Each window has a single toolbar and, by virtue of being a
// TabWindowController, a tab strip along the top.
// Note that under the hood the BrowserWindowController is neither an
// NSWindowController nor its window's delegate, though it receives all
// NSWindowDelegate methods as if it were.

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/extensions/browser_extension_window_controller.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bubble_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_window_controller.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#import "chrome/browser/ui/cocoa/url_drop_target.h"
#import "chrome/browser/ui/cocoa/view_resizer.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_context.h"
#include "chrome/browser/ui/tabs/tab_utils.h"
#include "components/translate/core/common/translate_errors.h"
#include "ui/base/accelerators/accelerator_manager.h"
#include "ui/gfx/geometry/rect.h"

@class AvatarBaseController;
class BookmarkBubbleObserverCocoa;
class Browser;
class BrowserWindow;
class BrowserWindowCocoa;
@class BrowserWindowFullscreenTransition;
@class BrowserWindowTouchBar;
@class DevToolsController;
@class DownloadShelfController;
class ExtensionKeybindingRegistryCocoa;
class ExclusiveAccessController;
class ExclusiveAccessContext;
@class FindBarCocoaController;
@class FullscreenModeController;
@class FullscreenToolbarController;
@class FullscreenToolbarVisibilityLockController;
@class FullscreenWindow;
@class InfoBarContainerController;
class LocationBarViewMac;
@class OverlayableContentsController;
class StatusBubbleMac;
@class TabStripController;
@class TabStripView;
@class ToolbarController;
@class TranslateBubbleController;

namespace content {
class WebContents;
}

namespace extensions {
class Command;
}

constexpr const gfx::Size kMinCocoaTabbedWindowSize(400, 272);
constexpr const gfx::Size kMinCocoaPopupWindowSize(100, 122);

@interface BrowserWindowController
    : TabWindowController<BookmarkBarControllerDelegate,
                          ViewResizer,
                          TabStripControllerDelegate> {
 @private
  // The ordering of these members is important as it determines the order in
  // which they are destroyed. |browser_| needs to be destroyed last as most of
  // the other objects hold weak references to it or things it owns
  // (tab/toolbar/bookmark models, profiles, etc).
  std::unique_ptr<Browser> browser_;
  NSWindow* savedRegularWindow_;
  std::unique_ptr<BrowserWindowCocoa> windowShim_;
  base::scoped_nsobject<ToolbarController> toolbarController_;
  base::scoped_nsobject<TabStripController> tabStripController_;
  base::scoped_nsobject<FindBarCocoaController> findBarCocoaController_;
  base::scoped_nsobject<InfoBarContainerController> infoBarContainerController_;
  base::scoped_nsobject<DownloadShelfController> downloadShelfController_;
  base::scoped_nsobject<BookmarkBarController> bookmarkBarController_;
  base::scoped_nsobject<DevToolsController> devToolsController_;
  base::scoped_nsobject<OverlayableContentsController>
      overlayableContentsController_;
  base::scoped_nsobject<FullscreenToolbarController>
      fullscreenToolbarController_;
  std::unique_ptr<ExclusiveAccessController> exclusiveAccessController_;
  base::scoped_nsobject<BrowserWindowFullscreenTransition>
      fullscreenTransition_;
  base::scoped_nsobject<BrowserWindowTouchBar> touchBar_;

  // Strong. StatusBubble is a special case of a strong reference that
  // we don't wrap in a scoped_ptr because it is acting the same
  // as an NSWindowController in that it wraps a window that must
  // be shut down before our destructors are called.
  StatusBubbleMac* statusBubble_;

  std::unique_ptr<BookmarkBubbleObserverCocoa> bookmarkBubbleObserver_;
  BookmarkBubbleController* bookmarkBubbleController_;  // Weak.
  BOOL initializing_;  // YES while we are currently in initWithBrowser:
  BOOL ownsBrowser_;  // Only ever NO when testing

  TranslateBubbleController* translateBubbleController_;  // Weak.

  // The total amount by which we've grown the window up or down (to display a
  // bookmark bar and/or download shelf), respectively; reset to 0 when moved
  // away from the bottom/top or resized (or zoomed).
  CGFloat windowTopGrowth_;
  CGFloat windowBottomGrowth_;

  // YES only if we're shrinking the window from an apparent zoomed state (which
  // we'll only do if we grew it to the zoomed state); needed since we'll then
  // restrict the amount of shrinking by the amounts specified above. Reset to
  // NO on growth.
  BOOL isShrinkingFromZoomed_;

  // The view controller that manages the incognito badge or the multi-profile
  // avatar button. Depending on whether the --new-profile-management flag is
  // used, the multi-profile button can either be the avatar's icon badge or a
  // button with the profile's name. If the flag is used, the button is always
  // shown, otherwise the view will always be in the view hierarchy but will
  // be hidden unless it's appropriate to show it (i.e. if there's more than
  // one profile).
  base::scoped_nsobject<AvatarBaseController> avatarButtonController_;

  // Lazily created view which draws the background for the floating set of bars
  // in presentation mode (for window types having a floating bar; it remains
  // nil for those which don't).
  // TODO(spqchan): Rename this to "fullscreenToolbarBackingView"
  base::scoped_nsobject<NSView> floatingBarBackingView_;

  // The borderless window used in fullscreen mode when Cocoa's System
  // Fullscreen API is not being used (or not available, before OS 10.7).
  base::scoped_nsobject<NSWindow> fullscreenWindow_;

  // True between |-windowWillEnterFullScreen:| and |-windowDidEnterFullScreen:|
  // to indicate that the window is in the process of transitioning into
  // AppKit fullscreen mode.
  BOOL enteringAppKitFullscreen_;

  // True between |-windowWillExitFullScreen:| and |-windowDidExitFullScreen:|
  // to indicate that the window is in the process of transitioning out of
  // AppKit fullscreen mode.
  BOOL exitingAppKitFullscreen_;

  // True between |enterImmersiveFullscreen| and |-windowDidEnterFullScreen:|
  // to indicate that the window is in the process of transitioning into
  // AppKit fullscreen mode.
  BOOL enteringImmersiveFullscreen_;

  // When the window is in the process of entering AppKit Fullscreen, this
  // property indicates whether the window is being fullscreened on the
  // primary screen.
  BOOL enteringAppKitFullscreenOnPrimaryScreen_;

  // This flag is set to true when |customWindowsToEnterFullScreenForWindow:|
  // and |customWindowsToExitFullScreenForWindow:| are called and did not
  // return nil.
  BOOL isUsingCustomAnimation_;

  // True if a call to exit AppKit fullscreen was made during the transition to
  // fullscreen.
  BOOL shouldExitAfterEnteringFullscreen_;

  // True if AppKit has finished exiting fullscreen before the exit animation
  // is completed. This flag is used to ensure that |windowDidExitFullscreen|
  // is called after the exit fullscreen animation is complete.
  BOOL appKitDidExitFullscreen_;

  // The size of the original (non-fullscreen) window.  This is saved just
  // before entering fullscreen mode and is only valid when |-isFullscreen|
  // returns YES.
  NSRect savedRegularWindowFrame_;

  // The proportion of the floating bar which is shown.
  CGFloat floatingBarShownFraction_;

  // If this ivar is set to YES, layoutSubviews calls will be ignored. This is
  // used in fullscreen transition to prevent spurious resize messages from
  // being sent to the renderer, which causes the transition to be janky.
  BOOL blockLayoutSubviews_;

  // Set when AppKit invokes -windowWillClose: to protect against possible
  // crashes. See http://crbug.com/671213.
  BOOL didWindowWillClose_;

  // The Extension Command Registry used to determine which keyboard events to
  // handle.
  std::unique_ptr<ExtensionKeybindingRegistryCocoa>
      extensionKeybindingRegistry_;
}

// A convenience class method which returns the |BrowserWindowController| for
// |window|, or nil if neither |window| nor its parent or any other ancestor
// has one.
+ (BrowserWindowController*)browserWindowControllerForWindow:(NSWindow*)window;

// A convenience class method which gets the |BrowserWindowController| for a
// given view.  This is the controller for the window containing |view|, if it
// is a BWC, or the first controller in the parent-window chain that is a
// BWC. This method returns nil if no window in the chain has a BWC.
+ (BrowserWindowController*)browserWindowControllerForView:(NSView*)view;

// Load the browser window nib and do any Cocoa-specific initialization.
// Takes ownership of |browser|.
- (id)initWithBrowser:(Browser*)browser;

// Call to make the browser go away from other places in the cross-platform
// code.
- (void)destroyBrowser;

// Ensure bounds for the window abide by the minimum window size.
- (gfx::Rect)enforceMinWindowSize:(gfx::Rect)bounds;

// Access the C++ bridge between the NSWindow and the rest of Chromium.
- (BrowserWindow*)browserWindow;

// Return a weak pointer to the toolbar controller.
- (ToolbarController*)toolbarController;

// Return a weak pointer to the tab strip controller.
- (TabStripController*)tabStripController;

// Return a weak pointer to the find bar controller.
- (FindBarCocoaController*)findBarCocoaController;

// Access the ObjC controller that contains the infobars.
- (InfoBarContainerController*)infoBarContainerController;

// Access the C++ bridge object representing the status bubble for the window.
- (StatusBubbleMac*)statusBubble;

// Access the C++ bridge object representing the location bar.
- (LocationBarViewMac*)locationBarBridge;

// Returns a weak pointer to the floating bar backing view;
- (NSView*)floatingBarBackingView;

// Returns a weak pointer to the overlayable contents controller.
- (OverlayableContentsController*)overlayableContentsController;

// Access the Profile object that backs this Browser.
- (Profile*)profile;

// Access the avatar button controller.
- (AvatarBaseController*)avatarButtonController;

// Forces the toolbar (and transitively the location bar) to update its current
// state.  If |tab| is non-NULL, we're switching (back?) to this tab and should
// restore any previous location bar state (such as user editing) as well.
- (void)updateToolbarWithContents:(content::WebContents*)tab;

// Resets the toolbar's tab state for |tab|.
- (void)resetTabState:(content::WebContents*)tab;

// Sets whether or not the current page in the frontmost tab is bookmarked.
- (void)setStarredState:(BOOL)isStarred;

// Sets whether or not the current page is translated.
- (void)setCurrentPageIsTranslated:(BOOL)on;

// Invoked via BrowserWindowCocoa::OnActiveTabChanged, happens whenever a
// new tab becomes active.
- (void)onActiveTabChanged:(content::WebContents*)oldContents
                        to:(content::WebContents*)newContents;

// Happens when the zoom level is changed in the active tab, the active tab is
// changed, or a new browser window or tab is created. |canShowBubble| denotes
// whether it would be appropriate to show a zoom bubble or not.
- (void)zoomChangedForActiveTab:(BOOL)canShowBubble;

// Called to tell the selected tab to update its loading state.
// |force| is set if the update is due to changing tabs, as opposed to
// the page-load finishing.  See comment in reload_button_cocoa.h.
- (void)setIsLoading:(BOOL)isLoading force:(BOOL)force;

// Brings this controller's window to the front.
- (void)activate;

// Called by FrameBrowserWindow when |makeFirstResponder:| is called.
// This method checks to see if a view in TopChrome has the first responder
// status. If it does, it will lock the fullscreen toolbar so that the toolbar
// will remain dropped down when the user is still interacting with it via
// keyboard access. Otherwise, it will release the toolbar.
- (void)firstResponderUpdated:(NSResponder*)responder;

// Make the location bar the first responder, if possible.
- (void)focusLocationBar:(BOOL)selectAll;

// Make the (currently-selected) tab contents the first responder, if possible.
- (void)focusTabContents;

// Returns the frame of the regular (non-fullscreened) window (even if the
// window is currently in fullscreen mode).  The frame is returned in Cocoa
// coordinates (origin in bottom-left).
- (NSRect)regularWindowFrame;

// Whether or not to show the avatar, which is either the incognito icon or the
// user's profile avatar.
- (BOOL)shouldShowAvatar;

// Whether or not to show the new avatar button used by --new-profile-maagement.
- (BOOL)shouldUseNewAvatarButton;

- (BOOL)isBookmarkBarVisible;

// Returns YES if the bookmark bar is currently animating.
- (BOOL)isBookmarkBarAnimating;

- (BookmarkBarController*)bookmarkBarController;

- (DevToolsController*)devToolsController;

- (BOOL)isDownloadShelfVisible;

// Lazily creates the download shelf in visible state if it doesn't exist yet.
- (void)createAndAddDownloadShelf;

// Returns the download shelf controller, if it exists.
- (DownloadShelfController*)downloadShelf;

// Retains the given FindBarCocoaController and adds its view to this
// browser window.  Must only be called once per
// BrowserWindowController.
- (void)addFindBar:(FindBarCocoaController*)findBarCocoaController;

// The user changed the theme.
- (void)userChangedTheme;

// Consults the Command Registry to see if this |event| needs to be handled as
// an extension command and returns YES if so (NO otherwise).
// Only extensions with the given |priority| are considered.
- (BOOL)handledByExtensionCommand:(NSEvent*)event
    priority:(ui::AcceleratorManager::HandlerPriority)priority;

// Delegate method for the status bubble to query its base frame.
- (NSRect)statusBubbleBaseFrame;

// Show the bookmark bubble (e.g. user just clicked on the STAR)
- (void)showBookmarkBubbleForURL:(const GURL&)url
               alreadyBookmarked:(BOOL)alreadyBookmarked;

// Show the translate bubble.
- (void)showTranslateBubbleForWebContents:(content::WebContents*)contents
                                     step:(translate::TranslateStep)step
                                errorType:
                                    (translate::TranslateErrors::Type)errorType;

// Dismiss the permission bubble
- (void)dismissPermissionBubble;

// Shows or hides the docked web inspector depending on |contents|'s state.
- (void)updateDevToolsForContents:(content::WebContents*)contents;

// Gets the current theme provider.
- (const ui::ThemeProvider*)themeProvider;

// Gets the window style.
- (ThemedWindowStyle)themedWindowStyle;

// Returns the position in window coordinates that the top left of a theme
// image with |alignment| should be painted at. If the window does not have a
// tab strip, the offset for THEME_IMAGE_ALIGN_WITH_FRAME is always returned.
// The result of this method can be used in conjunction with
// [NSGraphicsContext cr_setPatternPhase:] to set the offset of pattern colors.
- (NSPoint)themeImagePositionForAlignment:(ThemeImageAlignment)alignment;

// Return the point to which a bubble window's arrow should point, in window
// coordinates.
- (NSPoint)bookmarkBubblePoint;

// Called by BookmarkBubbleObserverCocoa when the bubble is closed.
- (void)bookmarkBubbleClosed;

// Called when the Add Search Engine dialog is closed.
- (void)sheetDidEnd:(NSWindow*)sheet
         returnCode:(NSInteger)code
            context:(void*)context;

// Executes the command registered by the extension that has the given id.
- (void)executeExtensionCommand:(const std::string&)extension_id
                        command:(const extensions::Command&)command;

// Sets the alert state of the tab e.g. audio playing, media recording, etc.
// See TabUtils::TabAlertState for a list of all possible alert states.
- (void)setAlertState:(TabAlertState)alertState;

// Returns current alert state, determined by the alert state of tabs, set by
// UpdateAlertState.
- (TabAlertState)alertState;

// Returns the BrowserWindowTouchBar object associated with the window.
- (BrowserWindowTouchBar*)browserWindowTouchBar;

// Invalidates the browser's touch bar.
- (void)invalidateTouchBar;

// Indicates whether the toolbar is visible to the user. Toolbar is usually
// triggered by moving mouse cursor to the top of the monitor.
- (BOOL)isToolbarShowing;

@end  // @interface BrowserWindowController


// Methods having to do with the window type (normal/popup/app, and whether the
// window has various features.
@interface BrowserWindowController(WindowType)

// Determines whether this controller's window supports a given feature (i.e.,
// whether a given feature is or can be shown in the window).
// TODO(viettrungluu): |feature| is really should be |Browser::Feature|, but I
// don't want to include browser.h (and you can't forward declare enums).
- (BOOL)supportsWindowFeature:(int)feature;

// Called to check whether or not this window has a normal title bar (YES if it
// does, NO otherwise). (E.g., normal browser windows do not, pop-ups do.)
- (BOOL)hasTitleBar;

// Called to check whether or not this window has a toolbar (YES if it does, NO
// otherwise). (E.g., normal browser windows do, pop-ups do not.)
- (BOOL)hasToolbar;

// Called to check whether or not this window has a location bar (YES if it
// does, NO otherwise). (E.g., normal browser windows do, pop-ups may or may
// not.)
- (BOOL)hasLocationBar;

// Called to check whether or not this window can have bookmark bar (YES if it
// does, NO otherwise). (E.g., normal browser windows may, pop-ups may not.)
- (BOOL)supportsBookmarkBar;

// Called to check if this controller's window is a tabbed window (e.g., not a
// pop-up window). Returns YES if it is, NO otherwise.
// Note: The |-has...| methods are usually preferred, so this method is largely
// deprecated.
- (BOOL)isTabbedWindow;

// Returns the size of the original (non-fullscreen) window.
- (NSRect)savedRegularWindowFrame;

// Returns true if the browser is in the process of entering/exiting
// fullscreen.
- (BOOL)isFullscreenTransitionInProgress;

@end  // @interface BrowserWindowController(WindowType)

// Fullscreen terminology:
//
// ----------------------------------------------------------------------------
// There are 2 APIs that cause the window to get resized, and possibly move
// spaces.
//
// + AppKitFullscreen API: AppKit touts a feature known as "fullscreen". This
// involves moving the current window to a different space, and resizing the
// window to take up the entire size of the screen.
//
// + Immersive fullscreen: An alternative to AppKitFullscreen API. Uses on 10.9
//  on certain HTML/Flash content. This is a method defined by Chrome.
//
// The Immersive fullscreen API can be called after the AppKitFullscreen API.
// Calling the AppKitFullscreen API while immersive fullscreen API has been
// invoked causes all fullscreen modes to exit.
//
// ----------------------------------------------------------------------------
//
// There are several "fullscreen modes" bantered around. Technically, any
// fullscreen API can be combined with any sliding style.
//
// + System fullscreen***deprecated***: This term is confusing. Don't use it.
// It either refers to the AppKitFullscreen API, or the behavior that users
// expect to see when they click the fullscreen button, or some Chrome specific
// implementation that uses the AppKitFullscreen API.
//
// + Canonical Fullscreen: When a user clicks on the fullscreen button, they
// expect a fullscreen behavior similar to other AppKit apps.
//  - AppKitFullscreen API + TOOLBAR_PRESENT/TOOLBAR_HIDDEN.
//  - The button click directly invokes the AppKitFullscreen API. This class
//  get a callback, and calls adjustUIForOmniboxFullscreen.
//  - There is a menu item that is intended to invoke the same behavior. When
//  the user clicks the menu item, or use its hotkey, this class invokes the
//  AppKitFullscreen API.
//
// + HTML5 fullscreen. Uses AppKitFullscreen in 10.10+, otherwise Immersive.
//
// There are more fullscreen styles on OSX than other OSes. However, all OSes
// share the same cross-platform code for entering fullscreen
// (FullscreenController). It is important for OSX fullscreen logic to track
// how the user triggered fullscreen mode.
// There are currently 5 possible mechanisms:
//   - User clicks the AppKit Fullscreen button.
//     -- This invokes -[BrowserWindowController windowWillEnterFullscreen:]
//   - User selects the menu item "Enter Full Screen".
//     -- This invokes FullscreenController::ToggleFullscreenModeInternal(
//        BROWSER)
//   - User requests fullscreen via an extension.
//     -- This invokes FullscreenController::ToggleFullscreenModeInternal(
//        BROWSER)
//     -- The corresponding URL will be the url of the extension.
//   - User requests fullscreen via Flash or JavaScript apis.
//     -- This invokes FullscreenController::ToggleFullscreenModeInternal(
//        BROWSER)
//     -- browser_->fullscreen_controller()->
//        IsWindowFullscreenForTabOrPending() returns true.
//     -- The corresponding URL will be the url of the web page.

// Methods having to do with fullscreen mode.
@interface BrowserWindowController(Fullscreen)

// Enters Browser AppKit Fullscreen.
- (void)enterBrowserFullscreen;

// Updates the UI for tab fullscreen by adding or removing the tab strip and
// toolbar from the current window. The window must already be in fullscreen.
- (void)updateUIForTabFullscreen:
    (ExclusiveAccessContext::TabFullscreenState)state;

// Exits extension fullscreen if we're currently in the mode. Returns YES
// if we exited fullscreen.
- (BOOL)exitExtensionFullscreenIfPossible;

// Updates the contents of the fullscreen exit bubble with |url| and
// |bubbleType|.
- (void)updateFullscreenExitBubble;

// Returns YES if the browser window is currently in or entering fullscreen via
// the built-in immersive mechanism.
- (BOOL)isInImmersiveFullscreen;

// Returns YES if the browser window is currently in or entering fullscreen via
// the AppKit Fullscreen API.
- (BOOL)isInAppKitFullscreen;

// Enters Immersive Fullscreen for the given URL.
- (void)enterWebContentFullscreen;

// Exits the current fullscreen mode.
- (void)exitAnyFullscreen;

// Called by BrowserWindowFullscreenTransition when the exit animation is
// finished.
- (void)exitFullscreenAnimationFinished;

// Resizes the fullscreen window to fit the screen it's currently on.  Called by
// the FullscreenToolbarController when there is a change in monitor placement
// or resolution.
- (void)resizeFullscreenWindow;

// Query/lock/release the requirement that the tab strip/toolbar/attached
// bookmark bar bar cluster is visible (e.g., when one of its elements has
// focus). This is required for the floating bar if it's hidden in fullscreen,
// but should also be called when not in fullscreen mode; see the comments for
// |barVisibilityLocks_| for more details. Double locks/releases by the same
// owner are ignored. If |animate:| is YES, then an animation may be
// performed. In the case of multiple calls, later calls have precedence with
// the rule that |animate:NO| has precedence over |animate:YES|. If |owner| is
// nil in isToolbarVisibilityLockedForOwner, the method returns YES if there are
// any locks.
- (BOOL)isToolbarVisibilityLockedForOwner:(id)owner;
- (void)lockToolbarVisibilityForOwner:(id)owner withAnimation:(BOOL)animate;
- (void)releaseToolbarVisibilityForOwner:(id)owner withAnimation:(BOOL)animate;

// Returns YES if any of the views in the floating bar currently has focus.
- (BOOL)floatingBarHasFocus;

// Returns YES if the fullscreen is for tab content or an extension.
- (BOOL)isFullscreenForTabContentOrExtension;

// Accessor for the controller managing fullscreen ExclusiveAccessContext.
- (ExclusiveAccessController*)exclusiveAccessController;

@end  // @interface BrowserWindowController(Fullscreen)


// Methods which are either only for testing, or only public for testing.
@interface BrowserWindowController (TestingAPI)

// Put the incognito badge or multi-profile avatar on the browser and adjust the
// tab strip accordingly.
- (void)installAvatar;

// Allows us to initWithBrowser withOUT taking ownership of the browser.
- (id)initWithBrowser:(Browser*)browser takeOwnership:(BOOL)ownIt;

// Adjusts the window height by the given amount.  If the window spans from the
// top of the current workspace to the bottom of the current workspace, the
// height is not adjusted.  If growing the window by the requested amount would
// size the window to be taller than the current workspace, the window height is
// capped to be equal to the height of the current workspace.  If the window is
// partially offscreen, its height is not adjusted at all.  This function
// prefers to grow the window down, but will grow up if needed.  Calls to this
// function should be followed by a call to |layoutSubviews|.
// Returns if the window height was changed.
- (BOOL)adjustWindowHeightBy:(CGFloat)deltaH;

// Return an autoreleased NSWindow suitable for fullscreen use.
- (NSWindow*)createFullscreenWindow;

// Resets any saved state about window growth (due to showing the bookmark bar
// or the download shelf), so that future shrinking will occur from the bottom.
- (void)resetWindowGrowthState;

// Computes by how far in each direction, horizontal and vertical, the
// |source| rect doesn't fit into |target|.
- (NSSize)overflowFrom:(NSRect)source
                    to:(NSRect)target;

// Gets the rect, in window base coordinates, that the omnibox popup should be
// positioned relative to.
- (NSRect)omniboxPopupAnchorRect;

// Returns the flag |blockLayoutSubviews_|.
- (BOOL)isLayoutSubviewsBlocked;

// Returns the active tab contents controller's |blockFullscreenResize_| flag.
- (BOOL)isActiveTabContentsControllerResizeBlocked;

// Returns the fullscreen toolbar controller.
- (FullscreenToolbarController*)fullscreenToolbarController;

// Sets the fullscreen toolbar controller.
- (void)setFullscreenToolbarController:(FullscreenToolbarController*)controller;

// Sets |browserWindowTouchbar_|.
- (void)setBrowserWindowTouchBar:(BrowserWindowTouchBar*)touchBar;

@end  // @interface BrowserWindowController (TestingAPI)

#endif  // CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_CONTROLLER_H_
