// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/browser_window_controller.h"

#include <cmath>
#include <numeric>
#include <utility>

#include "base/command_line.h"
#include "base/mac/bundle_locations.h"
#import "base/mac/foundation_util.h"
#include "base/mac/mac_util.h"
#import "base/mac/sdk_forward_declarations.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "build/buildflag.h"
#include "chrome/app/chrome_command_ids.h"  // IDC_*
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/bookmarks/managed_bookmark_service_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/browser/extensions/extension_commands_global_registry.h"
#include "chrome/browser/lifetime/browser_shutdown.h"
#include "chrome/browser/permissions/permission_request_manager.h"
#include "chrome/browser/profiles/avatar_menu.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#include "chrome/browser/ui/bookmarks/bookmark_editor.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_instant_controller.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window_state.h"
#import "chrome/browser/ui/cocoa/background_gradient_view.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bubble_observer_cocoa.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_editor_controller.h"
#import "chrome/browser/ui/cocoa/browser/exclusive_access_controller_views.h"
#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#import "chrome/browser/ui/cocoa/browser_window_cocoa.h"
#import "chrome/browser/ui/cocoa/browser_window_command_handler.h"
#import "chrome/browser/ui/cocoa/browser_window_controller_private.h"
#import "chrome/browser/ui/cocoa/browser_window_layout.h"
#import "chrome/browser/ui/cocoa/browser_window_touch_bar.h"
#import "chrome/browser/ui/cocoa/browser_window_utils.h"
#import "chrome/browser/ui/cocoa/dev_tools_controller.h"
#import "chrome/browser/ui/cocoa/download/download_shelf_controller.h"
#include "chrome/browser/ui/cocoa/extensions/extension_keybinding_registry_cocoa.h"
#import "chrome/browser/ui/cocoa/fast_resize_view.h"
#import "chrome/browser/ui/cocoa/find_bar/find_bar_bridge.h"
#import "chrome/browser/ui/cocoa/find_bar/find_bar_cocoa_controller.h"
#import "chrome/browser/ui/cocoa/framed_browser_window.h"
#import "chrome/browser/ui/cocoa/fullscreen/fullscreen_toolbar_controller.h"
#import "chrome/browser/ui/cocoa/fullscreen/fullscreen_toolbar_visibility_lock_controller.h"
#include "chrome/browser/ui/cocoa/fullscreen_placeholder_view.h"
#import "chrome/browser/ui/cocoa/fullscreen_window.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_container_controller.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/location_bar/autocomplete_text_field_editor.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#import "chrome/browser/ui/cocoa/location_bar/star_decoration.h"
#import "chrome/browser/ui/cocoa/profiles/avatar_base_controller.h"
#import "chrome/browser/ui/cocoa/profiles/avatar_button_controller.h"
#import "chrome/browser/ui/cocoa/profiles/avatar_icon_controller.h"
#import "chrome/browser/ui/cocoa/status_bubble_mac.h"
#import "chrome/browser/ui/cocoa/tab_contents/overlayable_contents_controller.h"
#import "chrome/browser/ui/cocoa/tab_contents/tab_contents_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_view.h"
#import "chrome/browser/ui/cocoa/tabs/tab_view.h"
#import "chrome/browser/ui/cocoa/toolbar/app_toolbar_button.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#include "chrome/browser/ui/cocoa/translate/translate_bubble_bridge_views.h"
#import "chrome/browser/ui/cocoa/translate/translate_bubble_controller.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_delegate.h"
#include "chrome/browser/ui/translate/translate_bubble_model_impl.h"
#include "chrome/browser/ui/window_sizer/window_sizer.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/command.h"
#include "chrome/common/pref_names.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/managed/managed_bookmark_service.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "components/omnibox/browser/omnibox_popup_model.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/translate/core/browser/translate_manager.h"
#include "components/translate/core/browser/translate_ui_delegate.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#import "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/base/cocoa/nsview_additions.h"
#import "ui/base/cocoa/touch_bar_forward_declarations.h"
#include "ui/base/ui_features.h"
#include "ui/display/screen.h"
#import "ui/gfx/mac/coordinate_conversion.h"
#include "ui/gfx/mac/scoped_cocoa_disable_screen_updates.h"
#include "ui/gfx/scrollbar_size.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

// ORGANIZATION: This is a big file. It is (in principle) organized as follows
// (in order):
// 1. Interfaces. Very short, one-time-use classes may include an implementation
//    immediately after their interface.
// 2. The general implementation section, ordered as follows:
//      i. Public methods and overrides.
//     ii. Overrides/implementations of undocumented methods.
//    iii. Delegate methods for various protocols, formal and informal, to which
//        |BrowserWindowController| conforms.
// 3. (temporary) Implementation sections for various categories.
//
// Private methods are defined and implemented separately in
// browser_window_controller_private.{h,mm}.
//
// Not all of the above guidelines are followed and more (re-)organization is
// needed. BUT PLEASE TRY TO KEEP THIS FILE ORGANIZED. I'd rather re-organize as
// little as possible, since doing so messes up the file's history.
//
// TODO(viettrungluu): [crbug.com/35543] on-going re-organization, splitting
// things into multiple files -- the plan is as follows:
// - in general, everything stays in browser_window_controller.h, but is split
//   off into categories (see below)
// - core stuff stays in browser_window_controller.mm
// - ... overrides also stay (without going into a category, in particular)
// - private stuff which everyone needs goes into
//   browser_window_controller_private.{h,mm}; if no one else needs them, they
//   can go in individual files (see below)
// - area/task-specific stuff go in browser_window_controller_<area>.mm
// - ... in categories called "(<Area>)" or "(<PrivateArea>)"
// Plan of action:
// - first re-organize into categories
// - then split into files

// Notes on self-inflicted (not user-inflicted) window resizing and moving:
//
// When the bookmark bar goes from hidden to shown (on a non-NTP) page, or when
// the download shelf goes from hidden to shown, we grow the window downwards in
// order to maintain a constant content area size. When either goes from shown
// to hidden, we consequently shrink the window from the bottom, also to keep
// the content area size constant. To keep things simple, if the window is not
// entirely on-screen, we don't grow/shrink the window.
//
// The complications come in when there isn't enough room (on screen) below the
// window to accomodate the growth. In this case, we grow the window first
// downwards, and then upwards. So, when it comes to shrinking, we do the
// opposite: shrink from the top by the amount by which we grew at the top, and
// then from the bottom -- unless the user moved/resized/zoomed the window, in
// which case we "reset state" and just shrink from the bottom.
//
// A further complication arises due to the way in which "zoom" ("maximize")
// works on Mac OS X. Basically, for our purposes, a window is "zoomed" whenever
// it occupies the full available vertical space. (Note that the green zoom
// button does not track zoom/unzoomed state per se, but basically relies on
// this heuristic.) We don't, in general, want to shrink the window if the
// window is zoomed (scenario: window is zoomed, download shelf opens -- which
// doesn't cause window growth, download shelf closes -- shouldn't cause the
// window to become unzoomed!). However, if we grew the window
// (upwards/downwards) to become zoomed in the first place, we *should* shrink
// the window by the amounts by which we grew (scenario: window occupies *most*
// of vertical space, download shelf opens causing growth so that window
// occupies all of vertical space -- i.e., window is effectively zoomed,
// download shelf closes -- should return the window to its previous state).
//
// A major complication is caused by the way grows/shrinks are handled and
// animated. Basically, the BWC doesn't see the global picture, but it sees
// grows and shrinks in small increments (as dictated by the animation). Thus
// window growth/shrinkage (at the top/bottom) have to be tracked incrementally.
// Allowing shrinking from the zoomed state also requires tracking: We check on
// any shrink whether we're both zoomed and have previously grown -- if so, we
// set a flag, and constrain any resize by the allowed amounts. On further
// shrinks, we check the flag (since the size/position of the window will no
// longer indicate that the window is shrinking from an apparent zoomed state)
// and if it's set we continue to constrain the resize.

using content::RenderWidgetHostView;
using content::WebContents;

namespace {

// Make |window| able to handle Browser commands.
void SetUpBrowserWindowCommandHandler(NSWindow* window) {
  [base::mac::ObjCCastStrict<ChromeEventProcessingWindow>(window)
      setCommandHandler:[[[BrowserWindowCommandHandler alloc] init]
                            autorelease]];
}

// Decouples the command dispatcher associated with |window| from Browser
// command handling. This prevents handlers with a reference to |window|
// attempting to look up a Browser* for it.
void ClearCommandHandler(NSWindow* window) {
  [base::mac::ObjCCastStrict<ChromeEventProcessingWindow>(window)
      setCommandHandler:nil];
}

// Returns true if the Tab Detaching in Fullscreen is enabled. It's enabled by
// default.
bool IsTabDetachingInFullscreenEnabled() {
  return !base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableFullscreenTabDetaching);
}

}  // namespace

@implementation BrowserWindowController

+ (BrowserWindowController*)browserWindowControllerForWindow:(NSWindow*)window {
  return base::mac::ObjCCast<BrowserWindowController>(
      [TabWindowController tabWindowControllerForWindow:window]);
}

+ (BrowserWindowController*)browserWindowControllerForView:(NSView*)view {
  NSWindow* window = [view window];
  return [BrowserWindowController browserWindowControllerForWindow:window];
}

// Load the browser window nib and do any Cocoa-specific initialization.
// Takes ownership of |browser|. Note that the nib also sets this controller
// up as the window's delegate.
- (id)initWithBrowser:(Browser*)browser {
  return [self initWithBrowser:browser takeOwnership:YES];
}

// Private(TestingAPI) init routine with testing options.
- (id)initWithBrowser:(Browser*)browser takeOwnership:(BOOL)ownIt {
  bool hasTabStrip = browser->SupportsWindowFeature(Browser::FEATURE_TABSTRIP);
  bool hasTitleBar = browser->SupportsWindowFeature(Browser::FEATURE_TITLEBAR);
  if ((self = [super initTabWindowControllerWithTabStrip:hasTabStrip
                                                titleBar:hasTitleBar])) {
    DCHECK(browser);
    initializing_ = YES;
    browser_.reset(browser);
    ownsBrowser_ = ownIt;
    NSWindow* window = [self window];
    SetUpBrowserWindowCommandHandler(window);

    // Make the content view for the window have a layer. This will make all
    // sub-views have layers. This is necessary to ensure correct layer
    // ordering of all child views and their layers.
    [[window contentView] setWantsLayer:YES];
    windowShim_.reset(new BrowserWindowCocoa(browser, self));

    // This has to happen before -enforceMinWindowSize: is called further down.
    [[self window]
        setMinSize:(browser->is_type_tabbed() ? kMinCocoaTabbedWindowSize
                                              : kMinCocoaPopupWindowSize)
                       .ToCGSize()];

    // Lion will attempt to automagically save and restore the UI. This
    // functionality appears to be leaky (or at least interacts badly with our
    // architecture) and thus BrowserWindowController never gets released. This
    // prevents the browser from being able to quit <http://crbug.com/79113>.
    [window setRestorable:NO];

    // Get the windows to swish in on Lion.
    [window setAnimationBehavior:NSWindowAnimationBehaviorDocumentWindow];

    // Get the most appropriate size for the window, then enforce the
    // minimum width and height. The window shim will handle flipping
    // the coordinates for us so we can use it to save some code.
    // Note that this may leave a significant portion of the window
    // offscreen, but there will always be enough window onscreen to
    // drag the whole window back into view.
    ui::WindowShowState show_state = ui::SHOW_STATE_DEFAULT;
    gfx::Rect desiredContentRect;
    chrome::GetSavedWindowBoundsAndShowState(browser_.get(),
                                             &desiredContentRect,
                                             &show_state);
    gfx::Rect windowRect = desiredContentRect;
    windowRect = [self enforceMinWindowSize:windowRect];

    // When we are given x/y coordinates of 0 on a created popup window, assume
    // none were given by the window.open() command.
    if (browser_->is_type_popup() &&
        windowRect.x() == 0 && windowRect.y() == 0) {
      gfx::Size size = windowRect.size();
      windowRect.set_origin(WindowSizer::GetDefaultPopupOrigin(size));
    }

    // Creates the manager for fullscreen and fullscreen bubbles.
    exclusiveAccessController_.reset(
        new ExclusiveAccessController(self, browser_.get()));

    // Size and position the window.  Note that it is not yet onscreen.  Popup
    // windows may get resized later on in this function, once the actual size
    // of the toolbar/tabstrip is known.
    windowShim_->SetBounds(windowRect);

    // Puts the incognito badge on the window frame, if necessary.
    [self installAvatar];

    // Create a sub-controller for the docked devTools and add its view to the
    // hierarchy.
    devToolsController_.reset([[DevToolsController alloc] init]);
    [[devToolsController_ view] setFrame:[[self tabContentArea] bounds]];
    [[self tabContentArea] addSubview:[devToolsController_ view]];

    // Create the overlayable contents controller.  This provides the switch
    // view that TabStripController needs.
    overlayableContentsController_.reset(
        [[OverlayableContentsController alloc] init]);
    [[overlayableContentsController_ view]
        setFrame:[[devToolsController_ view] bounds]];
    [[devToolsController_ view]
        addSubview:[overlayableContentsController_ view]];

    // Create a controller for the tab strip, giving it the model object for
    // this window's Browser and the tab strip view. The controller will handle
    // registering for the appropriate tab notifications from the back-end and
    // managing the creation of new tabs.
    [self createTabStripController];

    // Create a controller for the toolbar, giving it the toolbar model object
    // and the toolbar view from the nib. The controller will handle
    // registering for the appropriate command state changes from the back-end.
    // Adds the toolbar to the content area.
    toolbarController_.reset([[ToolbarController alloc]
        initWithCommands:browser->command_controller()
                 profile:browser->profile()
                 browser:browser]);
    [[toolbarController_ toolbarView] setResizeDelegate:self];
    [toolbarController_ setHasToolbar:[self hasToolbar]
                       hasLocationBar:[self hasLocationBar]];

    // Create a sub-controller for the bookmark bar.
    bookmarkBarController_.reset([[BookmarkBarController alloc]
        initWithBrowser:browser_.get()
           initialWidth:NSWidth([[[self window] contentView] frame])
               delegate:self]);
    // This call loads the view.
    BookmarkBarToolbarView* bookmarkBarView =
        [bookmarkBarController_ controlledView];
    [bookmarkBarView setResizeDelegate:self];

    [bookmarkBarController_ setBookmarkBarEnabled:[self supportsBookmarkBar]];

    // Create the infobar container view, so we can pass it to the
    // ToolbarController.
    infoBarContainerController_.reset(
        [[InfoBarContainerController alloc] initWithResizeDelegate:self]);

    // We don't want to try and show the bar before it gets placed in its parent
    // view, so this step shoudn't be inside the bookmark bar controller's
    // |-awakeFromNib|.
    windowShim_->BookmarkBarStateChanged(
        BookmarkBar::DONT_ANIMATE_STATE_CHANGE);

    [self updateFullscreenCollectionBehavior];

    [self layoutSubviews];

    // For non-trusted, non-app popup windows, |desiredContentRect| contains the
    // desired height of the content, not of the whole window.  Now that all the
    // views are laid out, measure the current content area size and grow if
    // needed. The window has not been placed onscreen yet, so this extra resize
    // will not cause visible jank.
    if (chrome::SavedBoundsAreContentBounds(browser_.get())) {
      CGFloat deltaH = desiredContentRect.height() -
                       NSHeight([[self tabContentArea] frame]);
      // Do not shrink the window, as that may break minimum size invariants.
      if (deltaH > 0) {
        // Convert from tabContentArea coordinates to window coordinates.
        NSSize convertedSize =
            [[self tabContentArea] convertSize:NSMakeSize(0, deltaH)
                                        toView:nil];
        NSRect frame = [[self window] frame];
        frame.size.height += convertedSize.height;
        frame.origin.y -= convertedSize.height;
        [[self window] setFrame:frame display:NO];
      }
    }

    // Create the bridge for the status bubble.
    statusBubble_ = new StatusBubbleMac([self window], self);

    // This must be done after the view is added to the window since it relies
    // on the window bounds to determine whether to show buttons or not.
    if ([self hasToolbar])  // Do not create the buttons in popups.
      [toolbarController_ createBrowserActionButtons];

    extensionKeybindingRegistry_.reset(
        new ExtensionKeybindingRegistryCocoa(browser_->profile(),
            [self window],
            extensions::ExtensionKeybindingRegistry::ALL_EXTENSIONS,
            windowShim_.get()));

    blockLayoutSubviews_ = NO;

    // We are done initializing now.
    initializing_ = NO;
  }
  return self;
}

- (void)dealloc {
  browser_->tab_strip_model()->CloseAllTabs();

  DCHECK([self window]);
  ClearCommandHandler([self window]);

  // Explicitly release |fullscreenToolbarController_| here, as it may call
  // back to this BWC in |-dealloc|.
  [fullscreenToolbarController_ exitFullscreenMode];
  fullscreenToolbarController_.reset();

  // Explicitly release |fullscreenTransition_| here since it may call back to
  // this BWC in |-dealloc|. Reset the fullscreen variables.
  if (fullscreenTransition_) {
    [fullscreenTransition_ browserWillBeDestroyed];
    [self resetCustomAppKitFullscreenVariables];
  }

  // Under certain testing configurations we may not actually own the browser.
  if (ownsBrowser_ == NO)
    ignore_result(browser_.release());

  [[NSNotificationCenter defaultCenter] removeObserver:self];

  // Inform reference counted objects that the Browser will be destroyed. This
  // ensures they invalidate their weak Browser* to prevent use-after-free.
  // These may outlive the Browser if they are retained by something else. For
  // example, since 10.10, the Nib loader internally creates an NSDictionary
  // that retains NSViewControllers and is autoreleased, so there is no way to
  // guarantee that the [super dealloc] call below will also call dealloc on the
  // controllers.
  [toolbarController_ browserWillBeDestroyed];
  [tabStripController_ browserWillBeDestroyed];
  [findBarCocoaController_ browserWillBeDestroyed];
  [downloadShelfController_ browserWillBeDestroyed];
  [bookmarkBarController_ browserWillBeDestroyed];
  [avatarButtonController_ browserWillBeDestroyed];
  [bookmarkBubbleController_ browserWillBeDestroyed];

  [super dealloc];
}

// Hack to address crbug.com/667274
// On TouchBar MacBooks, the touch bar machinery retains a reference
// to the browser window controller (which is an NSTouchBarProvider by
// default) but doesn't release it if Chrome quits before it takes the
// key window (for example, quitting from the Dock icon context menu.)
//
// If the window denies being a touch bar provider, it's never added
// to the set of providers and the reference is never taken. This
// prevents us from providing a touch bar from the window directly
// but descendant responders can still provide one.
//
// rdar://29467717
- (BOOL)conformsToProtocol:(Protocol*)protocol {
  if ([protocol isEqual:NSProtocolFromString(@"NSFunctionBarProvider")] ||
      [protocol isEqual:NSProtocolFromString(@"NSTouchBarProvider")]) {
    return NO;
  }
  return [super conformsToProtocol:protocol];
}

- (gfx::Rect)enforceMinWindowSize:(gfx::Rect)bounds {
  gfx::Rect checkedBounds = bounds;

  NSSize minSize = [[self window] minSize];
  if (bounds.width() < minSize.width)
      checkedBounds.set_width(minSize.width);
  if (bounds.height() < minSize.height)
      checkedBounds.set_height(minSize.height);

  return checkedBounds;
}

- (BrowserWindow*)browserWindow {
  return windowShim_.get();
}

- (ToolbarController*)toolbarController {
  return toolbarController_.get();
}

- (TabStripController*)tabStripController {
  return tabStripController_.get();
}

- (FindBarCocoaController*)findBarCocoaController {
  return findBarCocoaController_.get();
}

- (InfoBarContainerController*)infoBarContainerController {
  return infoBarContainerController_.get();
}

- (StatusBubbleMac*)statusBubble {
  return statusBubble_;
}

- (LocationBarViewMac*)locationBarBridge {
  return [toolbarController_ locationBarBridge];
}

- (NSView*)floatingBarBackingView {
  return floatingBarBackingView_;
}

- (OverlayableContentsController*)overlayableContentsController {
  return overlayableContentsController_;
}

- (Profile*)profile {
  return browser_->profile();
}

- (AvatarBaseController*)avatarButtonController {
  return avatarButtonController_.get();
}

- (void)destroyBrowser {
  [NSApp removeWindowsItem:[self window]];

  // This is invoked from chrome::SessionEnding() which will terminate the
  // process without spinning another RunLoop. So no need to perform an
  // autorelease. Note this is currently controlled by an experiment. See
  // features::kDesktopFastShutdown in chrome/browser/features.cc.
  DCHECK_EQ(browser_shutdown::GetShutdownType(), browser_shutdown::END_SESSION);
}

// Called when the window meets the criteria to be closed (ie,
// |-windowShouldClose:| returns YES). We must be careful to preserve the
// semantics of BrowserWindow::Close() and not call the Browser's dtor directly
// from this method.
- (void)windowWillClose:(NSNotification*)notification {
  // Speculative fix for http://crbug.com/671213. It seems possible that AppKit
  // may invoke -windowWillClose: twice under rare conditions. That would cause
  // the logic below to post a second -autorelease, resulting in a double free.
  // (Well, actually, a zombie access when the closure tries to call release on
  // the strongly captured |self| pointer).
  DCHECK(!didWindowWillClose_) << "If hit, please update crbug.com/671213.";
  if (didWindowWillClose_)
    return;

  didWindowWillClose_ = YES;

  DCHECK_EQ([notification object], [self window]);
  DCHECK(browser_->tab_strip_model()->empty());
  [savedRegularWindow_ close];

  // We delete statusBubble here because we need to kill off the dependency
  // that its window has on our window before our window goes away.
  delete statusBubble_;
  statusBubble_ = NULL;

  // We can't actually use |-autorelease| here because there's an embedded
  // run loop in the |-performClose:| which contains its own autorelease pool.
  // Instead call it after a zero-length delay, which gets us back to the main
  // event loop.
  [self performSelector:@selector(autorelease)
             withObject:nil
             afterDelay:0];
}

- (void)updateDevToolsForContents:(WebContents*)contents {
  BOOL layout_changed =
      [devToolsController_ updateDevToolsForWebContents:contents
                                            withProfile:browser_->profile()];
  if (layout_changed && [findBarCocoaController_ isFindBarVisible])
    [self layoutSubviews];
}

// Called when the user wants to close a window or from the shutdown process.
// The Browser object is in control of whether or not we're allowed to close. It
// may defer closing due to several states, such as onUnload handlers needing to
// be fired. If closing is deferred, the Browser will handle the processing
// required to get us to the closing state and (by watching for all the tabs
// going away) will again call to close the window when it's finally ready.
- (BOOL)windowShouldClose:(id)sender {
  // Disable updates while closing all tabs to avoid flickering.
  gfx::ScopedCocoaDisableScreenUpdates disabler;
  // Give beforeunload handlers the chance to cancel the close before we hide
  // the window below.
  if (!browser_->ShouldCloseWindow())
    return NO;

  // saveWindowPositionIfNeeded: only works if we are the last active
  // window, but orderOut: ends up activating another window, so we
  // have to save the window position before we call orderOut:.
  [self saveWindowPositionIfNeeded];

  bool fast_tab_closing_enabled =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableFastUnload);

  if (!browser_->tab_strip_model()->empty()) {
    // Tab strip isn't empty.  Hide the frame (so it appears to have closed
    // immediately) and close all the tabs, allowing the renderers to shut
    // down. When the tab strip is empty we'll be called back again.
    [[self window] orderOut:self];
    browser_->OnWindowClosing();
    if (fast_tab_closing_enabled)
      browser_->tab_strip_model()->CloseAllTabs();
    return NO;
  } else if (fast_tab_closing_enabled &&
        !browser_->HasCompletedUnloadProcessing()) {
    // The browser needs to finish running unload handlers.
    // Hide the window (so it appears to have closed immediately), and
    // the browser will call us back again when it is ready to close.
    [[self window] orderOut:self];
    return NO;
  }

  // the tab strip is empty, it's ok to close the window
  return YES;
}

// Called right after our window became the main window.
- (void)windowDidBecomeMain:(NSNotification*)notification {
  // Set this window as active even if the previously active window was the
  // same one. This is needed for tracking visibility changes of a browser.
  if (browser_->window())
    BrowserList::SetLastActive(browser_.get());

  // Always saveWindowPositionIfNeeded when becoming main, not just
  // when |browser_| is not the last active browser. See crbug.com/536280 .
  [self saveWindowPositionIfNeeded];

  NSView* rootView = [[[self window] contentView] superview];
  [rootView cr_recursivelyInvokeBlock:^(id view) {
      if ([view conformsToProtocol:@protocol(ThemedWindowDrawing)])
        [view windowDidChangeActive];
  }];

  extensions::ExtensionCommandsGlobalRegistry::Get(browser_->profile())
      ->set_registry_for_active_window(extensionKeybindingRegistry_.get());
}

- (void)windowDidResignMain:(NSNotification*)notification {
  NSView* rootView = [[[self window] contentView] superview];
  [rootView cr_recursivelyInvokeBlock:^(id view) {
      if ([view conformsToProtocol:@protocol(ThemedWindowDrawing)])
        [view windowDidChangeActive];
  }];

  extensions::ExtensionCommandsGlobalRegistry::Get(browser_->profile())
      ->set_registry_for_active_window(nullptr);

  if (browser_->window())
    BrowserList::NotifyBrowserNoLongerActive(browser_.get());
}

// Called when we have been minimized.
- (void)windowDidMiniaturize:(NSNotification *)notification {
  [self saveWindowPositionIfNeeded];
}

// Called when we have been unminimized.
- (void)windowDidDeminiaturize:(NSNotification *)notification {
  // Make sure the window's show_state (which is now ui::SHOW_STATE_NORMAL)
  // gets saved.
  [self saveWindowPositionIfNeeded];
}

// Called when the user clicks the zoom button (or selects it from the Window
// menu) to determine the "standard size" of the window, based on the content
// and other factors. If the current size/location differs nontrivally from the
// standard size, Cocoa resizes the window to the standard size, and saves the
// current size as the "user size". If the current size/location is the same (up
// to a fudge factor) as the standard size, Cocoa resizes the window to the
// saved user size. (It is possible for the two to coincide.) In this way, the
// zoom button acts as a toggle. We determine the standard size based on the
// content, but enforce a minimum width (calculated using the dimensions of the
// screen) to ensure websites with small intrinsic width (such as google.com)
// don't end up with a wee window. Moreover, we always declare the standard
// width to be at least as big as the current width, i.e., we never want zooming
// to the standard width to shrink the window. This is consistent with other
// browsers' behaviour, and is desirable in multi-tab situations. Note, however,
// that the "toggle" behaviour means that the window can still be "unzoomed" to
// the user size.
// Note: this method is also called from -isZoomed. If the returned zoomed rect
// equals the current window's frame, -isZoomed returns YES.
- (NSRect)windowWillUseStandardFrame:(NSWindow*)window
                        defaultFrame:(NSRect)frame {
  // Forget that we grew the window up (if we in fact did).
  [self resetWindowGrowthState];

  // |frame| already fills the current screen. Never touch y and height since we
  // always want to fill vertically.

  // If the shift key is down, maximize. Hopefully this should make the
  // "switchers" happy.
  if ([[NSApp currentEvent] modifierFlags] & NSShiftKeyMask) {
    return frame;
  }

  // To prevent strange results on portrait displays, the basic minimum zoomed
  // width is the larger of: 60% of available width, 60% of available height
  // (bounded by available width).
  const CGFloat kProportion = 0.6;
  CGFloat zoomedWidth =
      std::max(kProportion * NSWidth(frame),
               std::min(kProportion * NSHeight(frame), NSWidth(frame)));

  WebContents* contents = browser_->tab_strip_model()->GetActiveWebContents();
  if (contents) {
    CGFloat intrinsicWidth =
        static_cast<CGFloat>(contents->GetPreferredSize().width());
    // If the intrinsic width is bigger, then make it the zoomed width.
    zoomedWidth = std::max(zoomedWidth,
                           std::min(intrinsicWidth, NSWidth(frame)));
  }

  // Never shrink from the current size on zoom (see above).
  NSRect currentFrame = [[self window] frame];
  zoomedWidth = std::max(zoomedWidth, NSWidth(currentFrame));

  // |frame| determines our maximum extents. We need to set the origin of the
  // frame -- and only move it left if necessary.
  if (currentFrame.origin.x + zoomedWidth > NSMaxX(frame))
    frame.origin.x = NSMaxX(frame) - zoomedWidth;
  else
    frame.origin.x = currentFrame.origin.x;

  // Set the width. Don't touch y or height.
  frame.size.width = zoomedWidth;

  return frame;
}

- (void)activate {
  [BrowserWindowUtils activateWindowForController:[self nsWindowController]];
}

// Determine whether we should let a window zoom/unzoom to the given |newFrame|.
// We avoid letting unzoom move windows between screens, because it's really
// strange and unintuitive.
- (BOOL)windowShouldZoom:(NSWindow*)window toFrame:(NSRect)newFrame {
  // Figure out which screen |newFrame| is on.
  NSScreen* newScreen = nil;
  CGFloat newScreenOverlapArea = 0.0;
  for (NSScreen* screen in [NSScreen screens]) {
    NSRect overlap = NSIntersectionRect(newFrame, [screen frame]);
    CGFloat overlapArea = NSWidth(overlap) * NSHeight(overlap);
    if (overlapArea > newScreenOverlapArea) {
      newScreen = screen;
      newScreenOverlapArea = overlapArea;
    }
  }
  // If we're somehow not on any screen, allow the zoom.
  if (!newScreen)
    return YES;

  // If the new screen is the current screen, we can return a definitive YES.
  // Note: This check is not strictly necessary, but just short-circuits in the
  // "no-brainer" case. To test the complicated logic below, comment this out!
  NSScreen* curScreen = [window screen];
  if (newScreen == curScreen)
    return YES;

  // Worry a little: What happens when a window is on two (or more) screens?
  // E.g., what happens in a 50-50 scenario? Cocoa may reasonably elect to zoom
  // to the other screen rather than staying on the officially current one. So
  // we compare overlaps with the current window frame, and see if Cocoa's
  // choice was reasonable (allowing a small rounding error). This should
  // hopefully avoid us ever erroneously denying a zoom when a window is on
  // multiple screens.
  NSRect curFrame = [window frame];
  NSRect newScrIntersectCurFr = NSIntersectionRect([newScreen frame], curFrame);
  NSRect curScrIntersectCurFr = NSIntersectionRect([curScreen frame], curFrame);
  if (NSWidth(newScrIntersectCurFr) * NSHeight(newScrIntersectCurFr) >=
      (NSWidth(curScrIntersectCurFr) * NSHeight(curScrIntersectCurFr) - 1.0)) {
    return YES;
  }

  // If it wasn't reasonable, return NO.
  return NO;
}

// Adjusts the window height by the given amount.
- (BOOL)adjustWindowHeightBy:(CGFloat)deltaH {
  // By not adjusting the window height when initializing, we can ensure that
  // the window opens with the same size that was saved on close.
  if (initializing_ || [self isInAnyFullscreenMode] || deltaH == 0)
    return NO;

  NSWindow* window = [self window];
  NSRect windowFrame = [window frame];
  NSRect workarea = [[window screen] visibleFrame];

  // Prevent the window from growing smaller than its minimum height:
  // http://crbug.com/230400 .
  if (deltaH < 0) {
    CGFloat minWindowHeight = [window minSize].height;
    if (windowFrame.size.height + deltaH < minWindowHeight) {
      // |deltaH| + |windowFrame.size.height| = |minWindowHeight|.
      deltaH = minWindowHeight - windowFrame.size.height;
    }
    if (deltaH == 0) {
      return NO;
    }
  }

  // If the window is not already fully in the workarea, do not adjust its frame
  // at all.
  if (!NSContainsRect(workarea, windowFrame))
    return NO;

  // Record the position of the top/bottom of the window, so we can easily check
  // whether we grew the window upwards/downwards.
  CGFloat oldWindowMaxY = NSMaxY(windowFrame);
  CGFloat oldWindowMinY = NSMinY(windowFrame);

  // We are "zoomed" if we occupy the full vertical space.
  bool isZoomed = (windowFrame.origin.y == workarea.origin.y &&
                   NSHeight(windowFrame) == NSHeight(workarea));

  // If we're shrinking the window....
  if (deltaH < 0) {
    bool didChange = false;

    // Don't reset if not currently zoomed since shrinking can take several
    // steps!
    if (isZoomed)
      isShrinkingFromZoomed_ = YES;

    // If we previously grew at the top, shrink as much as allowed at the top
    // first.
    if (windowTopGrowth_ > 0) {
      CGFloat shrinkAtTopBy = MIN(-deltaH, windowTopGrowth_);
      windowFrame.size.height -= shrinkAtTopBy;  // Shrink the window.
      deltaH += shrinkAtTopBy;            // Update the amount left to shrink.
      windowTopGrowth_ -= shrinkAtTopBy;  // Update the growth state.
      didChange = true;
    }

    // Similarly for the bottom (not an "else if" since we may have to
    // simultaneously shrink at both the top and at the bottom). Note that
    // |deltaH| may no longer be nonzero due to the above.
    if (deltaH < 0 && windowBottomGrowth_ > 0) {
      CGFloat shrinkAtBottomBy = MIN(-deltaH, windowBottomGrowth_);
      windowFrame.origin.y += shrinkAtBottomBy;     // Move the window up.
      windowFrame.size.height -= shrinkAtBottomBy;  // Shrink the window.
      deltaH += shrinkAtBottomBy;               // Update the amount left....
      windowBottomGrowth_ -= shrinkAtBottomBy;  // Update the growth state.
      didChange = true;
    }

    // If we're shrinking from zoomed but we didn't change the top or bottom
    // (since we've reached the limits imposed by |window...Growth_|), then stop
    // here. Don't reset |isShrinkingFromZoomed_| since we might get called
    // again for the same shrink.
    if (isShrinkingFromZoomed_ && !didChange)
      return NO;
  } else {
    isShrinkingFromZoomed_ = NO;

    // Don't bother with anything else.
    if (isZoomed)
      return NO;
  }

  // Shrinking from zoomed is handled above (and is constrained by
  // |window...Growth_|).
  if (!isShrinkingFromZoomed_) {
    // Resize the window down until it hits the bottom of the workarea, then if
    // needed continue resizing upwards.  Do not resize the window to be taller
    // than the current workarea.
    // Resize the window as requested, keeping the top left corner fixed.
    windowFrame.origin.y -= deltaH;
    windowFrame.size.height += deltaH;

    // If the bottom left corner is now outside the visible frame, move the
    // window up to make it fit, but make sure not to move the top left corner
    // out of the visible frame.
    if (windowFrame.origin.y < workarea.origin.y) {
      windowFrame.origin.y = workarea.origin.y;
      windowFrame.size.height =
          std::min(NSHeight(windowFrame), NSHeight(workarea));
    }

    // Record (if applicable) how much we grew the window in either direction.
    // (N.B.: These only record growth, not shrinkage.)
    if (NSMaxY(windowFrame) > oldWindowMaxY)
      windowTopGrowth_ += NSMaxY(windowFrame) - oldWindowMaxY;
    if (NSMinY(windowFrame) < oldWindowMinY)
      windowBottomGrowth_ += oldWindowMinY - NSMinY(windowFrame);
  }

  // Disable subview resizing while resizing the window, or else we will get
  // unwanted renderer resizes.  The calling code must call layoutSubviews to
  // make things right again.
  NSView* chromeContentView = [self chromeContentView];
  BOOL autoresizesSubviews = [chromeContentView autoresizesSubviews];
  [chromeContentView setAutoresizesSubviews:NO];

  // On Yosemite the toolbar can flicker when hiding or showing the bookmarks
  // bar. Here, |chromeContentView| is set to not autoresize its subviews during
  // the window resize. Because |chromeContentView| is not flipped, if the
  // window is getting shorter, the toolbar will move up within the window.
  // Soon after, a call to layoutSubviews corrects its position. Passing NO to
  // setFrame:display: should keep the toolbarView's intermediate position
  // hidden, as should the prior call to disable screen updates. For some
  // reason, neither prevents the toolbarView's intermediate position from
  // becoming visible. Its subsequent appearance in its correct location causes
  // the flicker. It may be that the Appkit assumes that updating the window
  // immediately is not a big deal given that everything in it is layer-backed.
  // Indeed, turning off layer backing for all ancestors of the toolbarView
  // causes the flicker to go away.
  //
  // By shifting the toolbarView enough so that it's in its correct location
  // immediately after the call to setFrame:display:, the toolbar will be in
  // the right spot when the Appkit prematurely flushes the window contents to
  // the screen. http://crbug.com/444080 .
  if ([self hasToolbar]) {
    NSView* toolbarView = [toolbarController_ view];
    NSRect currentWindowFrame = [window frame];
    NSRect toolbarViewFrame = [toolbarView frame];
    toolbarViewFrame.origin.y += windowFrame.size.height -
        currentWindowFrame.size.height;
    [toolbarView setFrame:toolbarViewFrame];
  }

  [window setFrame:windowFrame display:NO];
  [chromeContentView setAutoresizesSubviews:autoresizesSubviews];
  return YES;
}

// Main method to resize browser window subviews.  This method should be called
// when resizing any child of the content view, rather than resizing the views
// directly.  If the view is already the correct height, does not force a
// relayout.
- (void)resizeView:(NSView*)view newHeight:(CGFloat)height {
  // We should only ever be called for one of the following four views.
  // |downloadShelfController_| may be nil. If we are asked to size the bookmark
  // bar directly, its superview must be this controller's content view.
  DCHECK(view);
  DCHECK(view == [toolbarController_ view] ||
         view == [infoBarContainerController_ view] ||
         view == [downloadShelfController_ view] ||
         view == [bookmarkBarController_ view]);

  // The infobar has insufficient information to determine its new height. It
  // knows the total height of all of the info bars (which is what it passes
  // into this method), but knows nothing about the maximum arrow height, which
  // is determined by this class.
  if (view == [infoBarContainerController_ view]) {
    base::scoped_nsobject<BrowserWindowLayout> layout(
        [[BrowserWindowLayout alloc] init]);
    [self updateLayoutParameters:layout];
    // Use the new height for the info bar.
    [layout setInfoBarHeight:height];

    chrome::LayoutOutput output = [layout computeLayout];

    height = NSHeight(output.infoBarFrame);
  }

  // Change the height of the view and call |-layoutSubViews|. We set the height
  // here without regard to where the view is on the screen or whether it needs
  // to "grow up" or "grow down."  The below call to |-layoutSubviews| will
  // position each view correctly.
  NSRect frame = [view frame];
  if (NSHeight(frame) == height)
    return;

  // Disable screen updates to prevent flickering.
  gfx::ScopedCocoaDisableScreenUpdates disabler;

  // Grow or shrink the window by the amount of the height change.  We adjust
  // the window height only in two cases:
  // 1) We are adjusting the height of the bookmark bar and it is currently
  // animating either open or closed.
  // 2) We are adjusting the height of the download shelf.
  //
  // We do not adjust the window height for bookmark bar changes on the NTP.
  BOOL shouldAdjustBookmarkHeight =
      [bookmarkBarController_ isAnimatingBetweenState:BookmarkBar::HIDDEN
                                             andState:BookmarkBar::SHOW];

  if ((shouldAdjustBookmarkHeight && view == [bookmarkBarController_ view]) ||
      view == [downloadShelfController_ view]) {
    CGFloat deltaH = height - NSHeight(frame);
    [self adjustWindowHeightBy:deltaH];
  }

  frame.size.height = height;
  // TODO(rohitrao): Determine if calling setFrame: twice is bad.
  [view setFrame:frame];
  [self layoutSubviews];

}

- (BOOL)handledByExtensionCommand:(NSEvent*)event
    priority:(ui::AcceleratorManager::HandlerPriority)priority {
  return extensionKeybindingRegistry_->ProcessKeyEvent(
      content::NativeWebKeyboardEvent(event), priority);
}

// StatusBubble delegate method: tell the status bubble the frame it should
// position itself in.
- (NSRect)statusBubbleBaseFrame {
  NSView* view = [overlayableContentsController_ view];
  return [view convertRect:[view bounds] toView:nil];
}

- (void)updateToolbarWithContents:(WebContents*)tab {
  [toolbarController_ updateToolbarWithContents:tab];
}

- (void)resetTabState:(WebContents*)tab {
  [toolbarController_ resetTabState:tab];
}

- (void)setStarredState:(BOOL)isStarred {
  [toolbarController_ setStarredState:isStarred];

  if ([touchBar_ isStarred] != isStarred) {
    [touchBar_ setIsStarred:isStarred];
    [self invalidateTouchBar];
  }
}

- (void)setCurrentPageIsTranslated:(BOOL)on {
  [toolbarController_ setTranslateIconLit:on];
}

- (void)onActiveTabChanged:(content::WebContents*)oldContents
                        to:(content::WebContents*)newContents {
  if ([self isInAnyFullscreenMode]) {
    [[self fullscreenToolbarController] revealToolbarForWebContents:newContents
                                                       inForeground:YES];
  }
  [self invalidateTouchBar];
}

- (void)zoomChangedForActiveTab:(BOOL)canShowBubble {
  [toolbarController_ zoomChangedForActiveTab:canShowBubble];
}

// Accept tabs from a BrowserWindowController with the same Profile.
- (BOOL)canReceiveFrom:(TabWindowController*)source {
  BrowserWindowController* realSource =
      base::mac::ObjCCast<BrowserWindowController>(source);
  if (!realSource || browser_->profile() != realSource->browser_->profile()) {
    return NO;
  }

  // Can't drag a tab from a normal browser to a pop-up
  if (browser_->type() != realSource->browser_->type()) {
    return NO;
  }

  return YES;
}

// Move a given tab view to the location of the current placeholder. If there is
// no placeholder, it will go at the end. |controller| is the window controller
// of a tab being dropped from a different window. It will be nil if the drag is
// within the window, otherwise the tab is removed from that window before being
// placed into this one. The implementation will call |-removePlaceholder| since
// the drag is now complete.  This also calls |-layoutTabs| internally so
// clients do not need to call it again.
- (void)moveTabViews:(NSArray*)views
      fromController:(TabWindowController*)dragController {
  if (dragController) {
    // Moving between windows.
    NSView* activeTabView = [dragController activeTabView];
    BrowserWindowController* dragBWC =
        base::mac::ObjCCastStrict<BrowserWindowController>(dragController);

    // We will drop the tabs starting at indexOfPlaceholder, and increment from
    // there. We remove the placehoder before dropping the tabs, so that the
    // new tab animation's destination frame is correct.
    int tabIndex = [tabStripController_ indexOfPlaceholder];
    [self removePlaceholder];

    for (NSView* view in views) {
      // Figure out the WebContents to drop into our tab model from the source
      // window's model.
      int index = [dragBWC->tabStripController_ modelIndexForTabView:view];
      WebContents* contents =
          dragBWC->browser_->tab_strip_model()->GetWebContentsAt(index);
      // The tab contents may have gone away if given a window.close() while it
      // is being dragged. If so, bail, we've got nothing to drop.
      if (!contents)
        continue;

      // Convert |view|'s frame (which starts in the source tab strip's
      // coordinate system) to the coordinate system of the destination tab
      // strip. This needs to be done before being detached so the window
      // transforms can be performed.
      NSRect destinationFrame = [view frame];
      NSPoint tabOrigin = destinationFrame.origin;
      tabOrigin = [[dragController tabStripView] convertPoint:tabOrigin
                                                       toView:nil];
      tabOrigin = ui::ConvertPointFromWindowToScreen([dragController window],
                                                     tabOrigin);
      tabOrigin = ui::ConvertPointFromScreenToWindow([self window], tabOrigin);
      tabOrigin = [[self tabStripView] convertPoint:tabOrigin fromView:nil];
      destinationFrame.origin = tabOrigin;

      // Before the tab is detached from its originating tab strip, store the
      // pinned state so that it can be maintained between the windows.
      bool isPinned = dragBWC->browser_->tab_strip_model()->IsTabPinned(index);

      // Now that we have enough information about the tab, we can remove it
      // from the dragging window. We need to do this *before* we add it to the
      // new window as this will remove the WebContents' delegate.
      [dragController detachTabView:view];

      // Deposit it into our model at the appropriate location (it already knows
      // where it should go from tracking the drag). Doing this sets the tab's
      // delegate to be the Browser.
      [tabStripController_ dropWebContents:contents
                                   atIndex:tabIndex++
                                 withFrame:destinationFrame
                               asPinnedTab:isPinned
                                  activate:view == activeTabView];
    }
  } else {
    // Moving within a window.
    for (NSView* view in views) {
      int index = [tabStripController_ modelIndexForTabView:view];
      [tabStripController_ moveTabFromIndex:index];
    }
    [self removePlaceholder];
  }
}

// Tells the tab strip to forget about this tab in preparation for it being
// put into a different tab strip, such as during a drop on another window.
- (void)detachTabView:(NSView*)view {
  int index = [tabStripController_ modelIndexForTabView:view];

  // TODO(erikchen): While it might be nice to fix ownership semantics here,
  // realistically the code is going to be deleted in the not-too-distant
  // future.
  browser_->tab_strip_model()->DetachWebContentsAt(index).release();
}

- (NSArray*)tabViews {
  return [tabStripController_ tabViews];
}

- (NSView*)activeTabView {
  return [tabStripController_ activeTabView];
}

- (void)setIsLoading:(BOOL)isLoading force:(BOOL)force {
  [toolbarController_ setIsLoading:isLoading force:force];
  [touchBar_ setIsPageLoading:isLoading];
}

- (void)firstResponderUpdated:(NSResponder*)responder {
  if (![self isInAppKitFullscreen] ||
      [fullscreenToolbarController_ toolbarStyle] ==
          FullscreenToolbarStyle::TOOLBAR_NONE) {
    return;
  }

  if (!responder) {
    [self releaseToolbarVisibilityForOwner:self withAnimation:YES];
    return;
  }

  if (![responder isKindOfClass:[NSView class]])
    return;

  // If the view is in the download shelf or the tab content area, don't
  // lock the toolbar.
  NSView* view = base::mac::ObjCCastStrict<NSView>(responder);
  if (![view isDescendantOf:[[self window] contentView]] ||
      [view isDescendantOf:[downloadShelfController_ view]] ||
      [view isDescendantOf:[self tabContentArea]]) {
    [self releaseToolbarVisibilityForOwner:self withAnimation:YES];
    return;
  }

  [self lockToolbarVisibilityForOwner:self withAnimation:YES];
}

// Make the location bar the first responder, if possible.
- (void)focusLocationBar:(BOOL)selectAll {
  [toolbarController_ focusLocationBar:selectAll];
}

- (void)focusTabContents {
  content::WebContents* const activeWebContents =
      browser_->tab_strip_model()->GetActiveWebContents();
  if (activeWebContents)
    activeWebContents->Focus();
}

- (void)layoutTabs {
  [tabStripController_ layoutTabs];
}

- (TabWindowController*)detachTabsToNewWindow:(NSArray*)tabViews
                                   draggedTab:(NSView*)draggedTab {
  DCHECK_GT([tabViews count], 0U);

  // Disable screen updates so that this appears as a single visual change.
  gfx::ScopedCocoaDisableScreenUpdates disabler;

  // Set the window size. Need to do this before we detach the tab so it's
  // still in the window. We have to flip the coordinates as that's what
  // is expected by the Browser code.
  NSWindow* sourceWindow = [draggedTab window];
  NSRect windowRect = [sourceWindow frame];
  NSScreen* screen = [sourceWindow screen];
  windowRect.origin.y =
      NSHeight([screen frame]) - NSMaxY(windowRect) + [self menubarOffset];
  gfx::Rect browserRect(windowRect.origin.x, windowRect.origin.y,
                        NSWidth(windowRect), NSHeight(windowRect));

  std::vector<TabStripModelDelegate::NewStripContents> contentses;
  TabStripModel* model = browser_->tab_strip_model();

  for (TabView* tabView in tabViews) {
    // Fetch the tab contents for the tab being dragged.
    int index = [tabStripController_ modelIndexForTabView:tabView];
    bool isPinned = model->IsTabPinned(index);
    bool isActive = (index == model->active_index());

    TabStripModelDelegate::NewStripContents item;
    item.web_contents = model->DetachWebContentsAt(index);
    item.add_types =
        (isActive ? TabStripModel::ADD_ACTIVE : TabStripModel::ADD_NONE) |
        (isPinned ? TabStripModel::ADD_PINNED : TabStripModel::ADD_NONE);
    contentses.push_back(std::move(item));
  }

  // Create a new window with the dragged tabs in its model.
  Browser* newBrowser =
      browser_->tab_strip_model()->delegate()->CreateNewStripWithContents(
          std::move(contentses), browserRect, false);

  // Get the new controller by asking the new window for its delegate.
  BrowserWindowController* controller = [BrowserWindowController
      browserWindowControllerForWindow:newBrowser->window()->GetNativeWindow()];
  DCHECK(controller && [controller isKindOfClass:[TabWindowController class]]);

  // Ensure that the window will appear on top of the source window in
  // fullscreen mode.
  if ([self isInAppKitFullscreen]) {
    NSWindow* window = [controller window];
    NSUInteger collectionBehavior = [window collectionBehavior];
    collectionBehavior &= ~NSWindowCollectionBehaviorFullScreenPrimary;
    collectionBehavior |= NSWindowCollectionBehaviorFullScreenAuxiliary;
    [window setCollectionBehavior:collectionBehavior];
    [window setLevel:NSFloatingWindowLevel];

    controller->savedRegularWindowFrame_ = savedRegularWindowFrame_;
  }

  // And make sure we use the correct frame in the new view.
  TabStripController* tabStripController = [controller tabStripController];
  NSView* tabStrip = [self tabStripView];
  NSEnumerator* tabEnumerator = [tabViews objectEnumerator];
  for (NSView* newView in [tabStripController tabViews]) {
    NSView* oldView = [tabEnumerator nextObject];
    if (oldView) {
      // Pushes tabView's frame back inside the tabstrip.
      NSRect sourceTabRect = [oldView frame];
      NSSize tabOverflow =
          [self overflowFrom:[tabStrip convertRect:sourceTabRect toView:nil]
                          to:[tabStrip frame]];
      NSRect tabRect =
          NSOffsetRect(sourceTabRect, -tabOverflow.width, -tabOverflow.height);
      // Force the added tab to the right size (remove stretching.)
      tabRect.size.height = [TabStripController defaultTabHeight];

      [tabStripController setFrame:tabRect ofTabView:newView];
    }
  }

  return controller;
}

- (void)detachedWindowEnterFullscreenIfNeeded:(TabWindowController*)source {
  // Ensure that this is only called when the tab is detached into its own
  // window (in which the overlay window will be present).
  DCHECK([self overlayWindow]);

  if (([[source window] styleMask] & NSFullScreenWindowMask)
      == NSFullScreenWindowMask) {
    [self updateFullscreenCollectionBehavior];

    // Since the detached window in fullscreen will have the size of the
    // screen, it will set |savedRegularWindowFrame_| to the screen size after
    // it enters fullscreen. Make sure that we have the correct value for the
    // |savedRegularWindowFrame_|.
    NSRect regularWindowFrame = savedRegularWindowFrame_;
    [[self window] toggleFullScreen:nil];
    savedRegularWindowFrame_ = regularWindowFrame;
  }
}

- (void)insertPlaceholderForTab:(TabView*)tab
                          frame:(NSRect)frame {
  [super insertPlaceholderForTab:tab frame:frame];
  [tabStripController_ insertPlaceholderForTab:tab frame:frame];
}

- (void)removePlaceholder {
  [super removePlaceholder];
  [tabStripController_ insertPlaceholderForTab:nil frame:NSZeroRect];
}

- (BOOL)isDragSessionActive {
  // The tab can be dragged within the existing tab strip or detached
  // into its own window (then the overlay window will be present).
  return [[self tabStripController] isDragSessionActive] ||
         [self overlayWindow] != nil;
}

- (BOOL)tabDraggingAllowed {
  return [tabStripController_ tabDraggingAllowed];
}

- (BOOL)tabTearingAllowed {
  return ![self isInAnyFullscreenMode] || IsTabDetachingInFullscreenEnabled();
}

- (BOOL)windowMovementAllowed {
  return ![self isInAnyFullscreenMode] || [self overlayWindow];
}

- (BOOL)isTabFullyVisible:(TabView*)tab {
  return [tabStripController_ isTabFullyVisible:tab];
}

- (void)showNewTabButton:(BOOL)show {
  [tabStripController_ showNewTabButton:show];
}

- (BOOL)shouldShowAvatar {
  if (![self hasTabStrip])
    return NO;
  if (browser_->profile()->IsOffTheRecord())
    return YES;

  ProfileAttributesEntry* entry;
  return g_browser_process->profile_manager()
      ->GetProfileAttributesStorage()
      .GetProfileAttributesWithPath(browser_->profile()->GetPath(), &entry);
}

- (BOOL)shouldUseNewAvatarButton {
  return profiles::IsRegularOrGuestSession(browser_.get());
}

- (BOOL)isBookmarkBarVisible {
  return [bookmarkBarController_ isVisible];
}

- (BOOL)isBookmarkBarAnimating {
  return [bookmarkBarController_ isAnimationRunning];
}

- (BookmarkBarController*)bookmarkBarController {
  return bookmarkBarController_;
}

- (DevToolsController*)devToolsController {
  return devToolsController_;
}

- (BOOL)isDownloadShelfVisible {
  return downloadShelfController_ != nil &&
      [downloadShelfController_ isVisible];
}

- (void)createAndAddDownloadShelf {
  if (!downloadShelfController_.get()) {
    downloadShelfController_.reset([[DownloadShelfController alloc]
        initWithBrowser:browser_.get() resizeDelegate:self]);
    [self.chromeContentView addSubview:[downloadShelfController_ view]];
    [self layoutSubviews];
  }
}

- (DownloadShelfController*)downloadShelf {
  return downloadShelfController_;
}

- (void)addFindBar:(FindBarCocoaController*)findBarCocoaController {
  // Shouldn't call addFindBar twice.
  DCHECK(!findBarCocoaController_.get());

  // Create a controller for the findbar.
  findBarCocoaController_.reset([findBarCocoaController retain]);
  [self layoutSubviews];
  [self updateSubviewZOrder];
}

- (NSWindow*)createFullscreenWindow {
  NSWindow* window = [[[FullscreenWindow alloc]
      initForScreen:[[self window] screen]] autorelease];
  SetUpBrowserWindowCommandHandler(window);
  return window;
}

- (NSInteger)numberOfTabs {
  // count() includes pinned tabs.
  return browser_->tab_strip_model()->count();
}

- (BOOL)hasLiveTabs {
  return !browser_->tab_strip_model()->empty();
}

- (NSString*)activeTabTitle {
  WebContents* contents = browser_->tab_strip_model()->GetActiveWebContents();
  return base::SysUTF16ToNSString(contents->GetTitle());
}

- (NSRect)regularWindowFrame {
  return [self isInAnyFullscreenMode] ? savedRegularWindowFrame_
                                      : [[self window] frame];
}

// (Override of |TabWindowController| method.)
- (BOOL)hasTabStrip {
  return [self supportsWindowFeature:Browser::FEATURE_TABSTRIP];
}

- (BOOL)isTabDraggable:(NSView*)tabView {
  // TODO(avi, thakis): ConstrainedWindowSheetController has no api to move
  // tabsheets between windows. Until then, we have to prevent having to move a
  // tabsheet between windows, e.g. no tearing off of tabs.
  int index = [tabStripController_ modelIndexForTabView:tabView];
  WebContents* contents = browser_->tab_strip_model()->GetWebContentsAt(index);
  if (!contents)
    return NO;

  const web_modal::WebContentsModalDialogManager* manager =
      web_modal::WebContentsModalDialogManager::FromWebContents(contents);
  return !manager || !manager->IsDialogActive();
}

- (CGFloat)menubarOffset {
  return [[self fullscreenToolbarController] computeLayout].menubarOffset;
}

// TabStripControllerDelegate protocol.
- (void)onActivateTabWithContents:(WebContents*)contents {
  // Update various elements that are interested in knowing the current
  // WebContents.

  // Update all the UI bits.
  windowShim_->UpdateTitleBar();

  // Update the bookmark bar.
  // TODO(viettrungluu): perhaps update to not terminate running animations (if
  // applicable)?
  windowShim_->BookmarkBarStateChanged(
      BookmarkBar::DONT_ANIMATE_STATE_CHANGE);

  [infoBarContainerController_ changeWebContents:contents];

  // Must do this after bookmark and infobar updates to avoid
  // unnecesary resize in contents.
  [devToolsController_ updateDevToolsForWebContents:contents
                                        withProfile:browser_->profile()];
}

- (void)onTabChanged:(TabChangeType)change withContents:(WebContents*)contents {
  // Update titles if this is the currently selected tab and if it isn't just
  // the loading state which changed.
  if (change != TabChangeType::kLoadingOnly)
    windowShim_->UpdateTitleBar();

  // Update the bookmark bar if this is the currently selected tab. This for
  // transitions between the NTP (showing its floating bookmark bar) and normal
  // web pages (showing no bookmark bar).
  // TODO(viettrungluu): perhaps update to not terminate running animations?
  windowShim_->BookmarkBarStateChanged(BookmarkBar::DONT_ANIMATE_STATE_CHANGE);
}

- (void)onTabDetachedWithContents:(WebContents*)contents {
  [infoBarContainerController_ tabDetachedWithContents:contents];
}

- (void)onTabInsertedWithContents:(content::WebContents*)contents
                     inForeground:(BOOL)inForeground {
  if ([self isInAnyFullscreenMode] && !inForeground)
    [[self fullscreenToolbarController]
        revealToolbarForWebContents:contents
                       inForeground:inForeground];

  if (inForeground) {
    AppToolbarButton* appMenuButton =
        static_cast<AppToolbarButton*>([toolbarController_ appMenuButton]);
    [appMenuButton animateIfPossibleWithDelay:YES];
  }
}

- (void)userChangedTheme {
  NSView* rootView = [[[self window] contentView] superview];
  [rootView cr_recursivelyInvokeBlock:^(id view) {
      if ([view conformsToProtocol:@protocol(ThemedWindowDrawing)])
        [view windowDidChangeTheme];

      // TODO(andresantoso): Remove this once all themed views respond to
      // windowDidChangeTheme above.
      [view setNeedsDisplay:YES];
  }];
}

- (const ui::ThemeProvider*)themeProvider {
  return &ThemeService::GetThemeProviderForProfile(browser_->profile());
}

- (ThemedWindowStyle)themedWindowStyle {
  ThemedWindowStyle style = 0;
  if (browser_->profile()->IsOffTheRecord())
    style |= THEMED_INCOGNITO;

  if (browser_->is_devtools())
    style |= THEMED_DEVTOOLS;
  if (browser_->is_type_popup())
    style |= THEMED_POPUP;

  return style;
}

- (NSPoint)themeImagePositionForAlignment:(ThemeImageAlignment)alignment {
  NSView* windowChromeView = [[[self window] contentView] superview];
  NSView* tabStripView = nil;
  if ([self hasTabStrip])
    tabStripView = [self tabStripView];
  return [BrowserWindowUtils themeImagePositionFor:windowChromeView
                                      withTabStrip:tabStripView
                                         alignment:alignment];
}

- (NSPoint)bookmarkBubblePoint {
  return [toolbarController_ bookmarkBubblePoint];
}

// Show the bookmark bubble (e.g. user just clicked on the STAR).
- (void)showBookmarkBubbleForURL:(const GURL&)url
               alreadyBookmarked:(BOOL)alreadyMarked {
  if (bookmarkBubbleObserver_.get())
    return;

  bookmarkBubbleObserver_.reset(new BookmarkBubbleObserverCocoa(self));

  if (chrome::ShowPilotDialogsWithViewsToolkit()) {
    chrome::ShowBookmarkBubbleViewsAtPoint(
        gfx::ScreenPointFromNSPoint(ui::ConvertPointFromWindowToScreen(
            [self window], [self bookmarkBubblePoint])),
        [[self window] contentView], bookmarkBubbleObserver_.get(),
        browser_.get(), url, alreadyMarked,
        [self locationBarBridge]->star_decoration());
  } else {
#if BUILDFLAG(MAC_VIEWS_BROWSER)
    NOTREACHED() << "MacViews Browser can't show cocoa dialogs";
#else
    BookmarkModel* model =
        BookmarkModelFactory::GetForBrowserContext(browser_->profile());
    bookmarks::ManagedBookmarkService* managed =
        ManagedBookmarkServiceFactory::GetForProfile(browser_->profile());
    const BookmarkNode* node = model->GetMostRecentlyAddedUserNodeForURL(url);
    bookmarkBubbleController_ = [[BookmarkBubbleController alloc]
        initWithParentWindow:[self window]
              bubbleObserver:bookmarkBubbleObserver_.get()
                     managed:managed
                       model:model
                        node:node
           alreadyBookmarked:alreadyMarked];
    [bookmarkBubbleController_ showWindow:self];
#endif
  }
  DCHECK(bookmarkBubbleObserver_);
}

- (void)bookmarkBubbleClosed {
  // Nil out the weak bookmark bubble controller reference.
  bookmarkBubbleController_ = nil;
  bookmarkBubbleObserver_.reset();
}

// Handle the editBookmarkNode: action sent from bookmark bubble controllers.
- (void)editBookmarkNode:(id)sender {
  BOOL responds = [sender respondsToSelector:@selector(node)];
  DCHECK(responds);
  if (responds) {
    const BookmarkNode* node = [sender node];
    if (node)
      BookmarkEditor::Show([self window], browser_->profile(),
          BookmarkEditor::EditDetails::EditNode(node),
          BookmarkEditor::SHOW_TREE);
  }
}

- (void)showTranslateBubbleForWebContents:(content::WebContents*)contents
                                     step:(translate::TranslateStep)step
                                errorType:(translate::TranslateErrors::Type)
                                errorType {
  if (chrome::ShowAllDialogsWithViewsToolkit()) {
    ShowTranslateBubbleViews([self window], [self locationBarBridge], contents,
                             step, errorType, true);
    return;
  }
  // TODO(hajimehoshi): The similar logic exists at TranslateBubbleView::
  // ShowBubble. This should be unified.
  if (translateBubbleController_) {
    // When the user reads the advanced setting panel, the bubble should not be
    // changed because they are focusing on the bubble.
    if (translateBubbleController_.webContents == contents &&
        translateBubbleController_.model->GetViewState() ==
        TranslateBubbleModel::VIEW_STATE_ADVANCED) {
      return;
    }
    if (step != translate::TRANSLATE_STEP_TRANSLATE_ERROR) {
      TranslateBubbleModel::ViewState viewState =
          TranslateBubbleModelImpl::TranslateStepToViewState(step);
      [translateBubbleController_ switchView:viewState];
    } else {
      [translateBubbleController_ switchToErrorView:errorType];
    }
    return;
  }

  std::string sourceLanguage;
  std::string targetLanguage;
  ChromeTranslateClient::GetTranslateLanguages(
      contents, &sourceLanguage, &targetLanguage);

  std::unique_ptr<translate::TranslateUIDelegate> uiDelegate(
      new translate::TranslateUIDelegate(
          ChromeTranslateClient::GetManagerFromWebContents(contents)
              ->GetWeakPtr(),
          sourceLanguage, targetLanguage));
  std::unique_ptr<TranslateBubbleModel> model(
      new TranslateBubbleModelImpl(step, std::move(uiDelegate)));
  translateBubbleController_ =
      [[TranslateBubbleController alloc] initWithParentWindow:self
                                                        model:std::move(model)
                                                  webContents:contents];
  [translateBubbleController_ showWindow:nil];

  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  [center addObserver:self
             selector:@selector(translateBubbleWindowWillClose:)
                 name:NSWindowWillCloseNotification
               object:[translateBubbleController_ window]];
}

- (void)dismissPermissionBubble {
  PermissionPrompt::Delegate* delegate = [self permissionRequestManager];
  if (delegate)
    delegate->Closing();
}

// Nil out the weak translate bubble controller reference.
- (void)translateBubbleWindowWillClose:(NSNotification*)notification {
  DCHECK_EQ([notification object], [translateBubbleController_ window]);

  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  [center removeObserver:self
                    name:NSWindowWillCloseNotification
                  object:[translateBubbleController_ window]];
  translateBubbleController_ = nil;
}

// If the browser is in incognito mode or has multi-profiles, install the image
// view to decorate the window at the upper right. Use the same base y
// coordinate as the tab strip.
- (void)installAvatar {
  // Install the image into the badge view. Hide it for now; positioning and
  // sizing will be done by the layout code. The AvatarIcon will choose which
  // image to display based on the browser. The AvatarButton will display
  // the browser profile's name unless the browser is incognito.
  NSView* view;
  if ([self shouldUseNewAvatarButton]) {
    avatarButtonController_.reset([[AvatarButtonController alloc]
        initWithBrowser:browser_.get()
                 window:[self window]]);
  } else {
    avatarButtonController_.reset(
      [[AvatarIconController alloc] initWithBrowser:browser_.get()]);
  }
  view = [avatarButtonController_ view];
  if (cocoa_l10n_util::ShouldFlipWindowControlsInRTL())
    [view setAutoresizingMask:NSViewMaxXMargin | NSViewMinYMargin];
  else
    [view setAutoresizingMask:NSViewMinXMargin | NSViewMinYMargin];
  [view setHidden:![self shouldShowAvatar]];

  // Install the view.
  [[[self window] contentView] addSubview:view];
}

// Called when we get a three-finger swipe.
- (void)swipeWithEvent:(NSEvent*)event {
  CGFloat deltaX = [event deltaX];
  CGFloat deltaY = [event deltaY];

  // Map forwards and backwards to history; left is positive, right is negative.
  unsigned int command = 0;
  if (deltaX > 0.5) {
    command = IDC_BACK;
  } else if (deltaX < -0.5) {
    command = IDC_FORWARD;
  } else if (deltaY > 0.5) {
    // TODO(pinkerton): figure out page-up, http://crbug.com/16305
  } else if (deltaY < -0.5) {
    // TODO(pinkerton): figure out page-down, http://crbug.com/16305
  }

  // Ensure the command is valid first (ExecuteCommand() won't do that) and
  // then make it so.
  if (chrome::IsCommandEnabled(browser_.get(), command)) {
    chrome::ExecuteCommandWithDisposition(
        browser_.get(),
        command,
        ui::WindowOpenDispositionFromNSEvent(event));
  }
}

// Delegate method called when window is resized.
- (void)windowDidResize:(NSNotification*)notification {
  [self saveWindowPositionIfNeeded];

  // Resize (and possibly move) the status bubble. Note that we may get called
  // when the status bubble does not exist.
  if (statusBubble_) {
    statusBubble_->UpdateSizeAndPosition();
  }

  [self updatePermissionBubbleAnchor];

  // The FindBar needs to know its own position to properly detect overlaps
  // with find results. The position changes whenever the window is resized,
  // and |layoutSubviews| computes the FindBar's position.
  // TODO: calling |layoutSubviews| here is a waste, find a better way to
  // do this.
  if ([findBarCocoaController_ isFindBarVisible])
    [self layoutSubviews];
}

// Delegate method called when window did move. (See below for why we don't use
// |-windowWillMove:|, which is called less frequently than |-windowDidMove|
// instead.)
- (void)windowDidMove:(NSNotification*)notification {
  [self saveWindowPositionIfNeeded];

  // When dragging tabs, the window is repositioned with direct setFrame: calls
  // which don't automatically reposition child windows. Most dialogs block tab
  // dragging or dismiss on focus loss. Permission bubbles do not, so ensure
  // they are anchored correctly.
  if ([self isDragSessionActive])
    [self updatePermissionBubbleAnchor];

  NSWindow* window = [self window];
  NSRect windowFrame = [window frame];
  NSRect workarea = [[window screen] visibleFrame];

  // We reset the window growth state whenever the window is moved out of the
  // work area or away (up or down) from the bottom or top of the work area.
  // Unfortunately, Cocoa sends |-windowWillMove:| too frequently (including
  // when clicking on the title bar to activate), and of course
  // |-windowWillMove| is called too early for us to apply our heuristic. (The
  // heuristic we use for detecting window movement is that if |windowTopGrowth_
  // > 0|, then we should be at the bottom of the work area -- if we're not,
  // we've moved. Similarly for the other side.)
  if (!NSContainsRect(workarea, windowFrame) ||
      (windowTopGrowth_ > 0 && NSMinY(windowFrame) != NSMinY(workarea)) ||
      (windowBottomGrowth_ > 0 && NSMaxY(windowFrame) != NSMaxY(workarea)))
    [self resetWindowGrowthState];
}

// Delegate method called when window will be resized; not called for
// |-setFrame:display:|.
- (NSSize)windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize {
  [self resetWindowGrowthState];
  return frameSize;
}

// Delegate method: see |NSWindowDelegate| protocol.
- (id)windowWillReturnFieldEditor:(NSWindow*)sender toObject:(id)obj {
  // Ask the toolbar controller if it wants to return a custom field editor
  // for the specific object.
  return [toolbarController_ customFieldEditorForObject:obj];
}

// (Needed for |BookmarkBarControllerDelegate| protocol.)
- (void)bookmarkBar:(BookmarkBarController*)controller
 didChangeFromState:(BookmarkBar::State)oldState
            toState:(BookmarkBar::State)newState {
  [toolbarController_ setDividerOpacity:[self toolbarDividerOpacity]];
  [self adjustToolbarAndBookmarkBarForCompression:
          [controller getDesiredToolbarHeightCompression]];
}

// (Needed for |BookmarkBarControllerDelegate| protocol.)
- (void)bookmarkBar:(BookmarkBarController*)controller
willAnimateFromState:(BookmarkBar::State)oldState
            toState:(BookmarkBar::State)newState {
  [toolbarController_ setDividerOpacity:[self toolbarDividerOpacity]];
  [self adjustToolbarAndBookmarkBarForCompression:
          [controller getDesiredToolbarHeightCompression]];
}

// (Private/TestingAPI)
- (void)resetWindowGrowthState {
  windowTopGrowth_ = 0;
  windowBottomGrowth_ = 0;
  isShrinkingFromZoomed_ = NO;
}

- (NSSize)overflowFrom:(NSRect)source
                    to:(NSRect)target {
  // If |source|'s boundary is outside of |target|'s, set its distance
  // to |x|.  Note that |source| can overflow to both side, but we
  // have nothing to do for such case.
  CGFloat x = 0;
  if (NSMaxX(target) < NSMaxX(source)) // |source| overflows to right
    x = NSMaxX(source) - NSMaxX(target);
  else if (NSMinX(source) < NSMinX(target)) // |source| overflows to left
    x = NSMinX(source) - NSMinX(target);

  // Same as |x| above.
  CGFloat y = 0;
  if (NSMaxY(target) < NSMaxY(source))
    y = NSMaxY(source) - NSMaxY(target);
  else if (NSMinY(source) < NSMinY(target))
    y = NSMinY(source) - NSMinY(target);

  return NSMakeSize(x, y);
}

// (Private/TestingAPI)
- (NSRect)omniboxPopupAnchorRect {
  // Start with toolbar rect.
  NSView* toolbarView = [toolbarController_ view];
  NSRect anchorRect = [toolbarView frame];

  // Adjust to account for height and possible bookmark bar. Compress by 1
  // to account for the separator.
  anchorRect.origin.y =
      NSMaxY(anchorRect) - [toolbarController_ desiredHeightForCompression:1];

  // Shift to window base coordinates.
  return [[toolbarView superview] convertRect:anchorRect toView:nil];
}

- (BOOL)isLayoutSubviewsBlocked {
  return blockLayoutSubviews_;
}

- (BOOL)isActiveTabContentsControllerResizeBlocked {
  return
      [[tabStripController_ activeTabContentsController] blockFullscreenResize];
}

- (void)sheetDidEnd:(NSWindow*)sheet
         returnCode:(NSInteger)code
            context:(void*)context {
  [sheet orderOut:self];
}

- (FullscreenToolbarController*)fullscreenToolbarController {
  return fullscreenToolbarController_.get();
}

- (void)setFullscreenToolbarController:
    (FullscreenToolbarController*)controller {
  fullscreenToolbarController_.reset([controller retain]);
}

- (void)setBrowserWindowTouchBar:(BrowserWindowTouchBar*)touchBar {
  touchBar_.reset(touchBar);
}

- (void)executeExtensionCommand:(const std::string&)extension_id
                        command:(const extensions::Command&)command {
  // Global commands are handled by the ExtensionCommandsGlobalRegistry
  // instance.
  DCHECK(!command.global());
  extensionKeybindingRegistry_->ExecuteCommand(extension_id,
                                               command.accelerator());
}

- (void)setAlertState:(TabAlertState)alertState {
  static_cast<BrowserWindowCocoa*>([self browserWindow])
      ->UpdateAlertState(alertState);
}

- (TabAlertState)alertState {
  return static_cast<BrowserWindowCocoa*>([self browserWindow])->alert_state();
}

- (BrowserWindowTouchBar*)browserWindowTouchBar {
  if (!touchBar_) {
    touchBar_.reset([[BrowserWindowTouchBar alloc]
                initWithBrowser:browser_.get()
        browserWindowController:self]);
  }

  return touchBar_.get();
}

- (void)invalidateTouchBar {
  if ([[self window] respondsToSelector:@selector(setTouchBar:)])
    [[self window] performSelector:@selector(setTouchBar:) withObject:nil];
}

- (BOOL)isToolbarShowing {
  return [fullscreenToolbarController_ mustShowFullscreenToolbar];
}

@end  // @implementation BrowserWindowController

@implementation BrowserWindowController(Fullscreen)

- (void)enterBrowserFullscreen {
  [self enterAppKitFullscreen];
}

- (void)updateUIForTabFullscreen:
    (ExclusiveAccessContext::TabFullscreenState)state {
  DCHECK([self isInAnyFullscreenMode]);
  [fullscreenToolbarController_
      layoutToolbarStyleIsExitingTabFullscreen:
          state == ExclusiveAccessContext::STATE_EXIT_TAB_FULLSCREEN];
}

- (void)updateFullscreenExitBubble {
  [self showFullscreenExitBubbleIfNecessary];
}

- (BOOL)exitExtensionFullscreenIfPossible {
  if (browser_->exclusive_access_manager()
          ->fullscreen_controller()
          ->IsExtensionFullscreenOrPending()) {
    browser_->extension_window_controller()->SetFullscreenMode(NO, GURL());
    return YES;
  }
  return NO;
}

- (BOOL)isInImmersiveFullscreen {
  return fullscreenWindow_.get() != nil || enteringImmersiveFullscreen_;
}

- (BOOL)isInAppKitFullscreen {
  return !exitingAppKitFullscreen_ &&
         (([[self window] styleMask] & NSFullScreenWindowMask) ==
              NSFullScreenWindowMask ||
          enteringAppKitFullscreen_);
}

- (BOOL)isInAnyFullscreenMode {
  return [self isInImmersiveFullscreen] || [self isInAppKitFullscreen];
}

- (NSView*)avatarView {
  return [avatarButtonController_ view];
}

- (void)enterWebContentFullscreen {
  // HTML5 Fullscreen should only use AppKit fullscreen in 10.10+.
  // However, if the user is using multiple monitors and turned off
  // "Separate Space in Each Display", use Immersive Fullscreen so
  // that the other monitors won't blank out.
  display::Screen* screen = display::Screen::GetScreen();
  BOOL hasMultipleMonitors = screen && screen->GetNumDisplays() > 1;

  if (base::mac::IsAtLeastOS10_10() &&
      !(hasMultipleMonitors && ![NSScreen screensHaveSeparateSpaces])) {
    [self enterAppKitFullscreen];
  } else {
    [self enterImmersiveFullscreen];
  }

  if (!exclusiveAccessController_->url().is_empty())
    [self updateFullscreenExitBubble];
}

- (void)exitAnyFullscreen {
  // TODO(erikchen): Fullscreen modes should stack. Should be able to exit
  // Immersive Fullscreen and still be in AppKit Fullscreen.
  if ([self isInAppKitFullscreen])
    [self exitAppKitFullscreenAsync:NO];
  if ([self isInImmersiveFullscreen])
    [self exitImmersiveFullscreen];
}

- (void)exitFullscreenAnimationFinished {
  if (appKitDidExitFullscreen_) {
    [self windowDidExitFullScreen:nil];
    appKitDidExitFullscreen_ = NO;
  }
}

- (void)resizeFullscreenWindow {
  DCHECK([self isInAnyFullscreenMode]);
  if (![self isInAnyFullscreenMode])
    return;

  NSWindow* window = [self window];
  [window setFrame:[[window screen] frame] display:YES];
  [self layoutSubviews];
}

- (BOOL)isToolbarVisibilityLockedForOwner:(id)owner {
  FullscreenToolbarVisibilityLockController* visibilityController =
      [self fullscreenToolbarVisibilityLockController];
  return [visibilityController isToolbarVisibilityLockedForOwner:owner];
}

- (void)lockToolbarVisibilityForOwner:(id)owner withAnimation:(BOOL)animate {
  FullscreenToolbarVisibilityLockController* visibilityController =
      [self fullscreenToolbarVisibilityLockController];
  [visibilityController lockToolbarVisibilityForOwner:owner
                                        withAnimation:animate];
}

- (void)releaseToolbarVisibilityForOwner:(id)owner withAnimation:(BOOL)animate {
  FullscreenToolbarVisibilityLockController* visibilityController =
      [self fullscreenToolbarVisibilityLockController];
  [visibilityController releaseToolbarVisibilityForOwner:owner
                                           withAnimation:animate];
}

- (BOOL)floatingBarHasFocus {
  NSResponder* focused = [[self window] firstResponder];
  return [focused isKindOfClass:[AutocompleteTextFieldEditor class]];
}

- (BOOL)isFullscreenForTabContentOrExtension {
  FullscreenController* controller =
      browser_->exclusive_access_manager()->fullscreen_controller();
  return controller->IsWindowFullscreenForTabOrPending() ||
         controller->IsExtensionFullscreenOrPending();
}

- (ExclusiveAccessController*)exclusiveAccessController {
  return exclusiveAccessController_.get();
}

@end  // @implementation BrowserWindowController(Fullscreen)


@implementation BrowserWindowController(WindowType)

- (BOOL)supportsWindowFeature:(int)feature {
  return browser_->SupportsWindowFeature(
      static_cast<Browser::WindowFeature>(feature));
}

- (BOOL)hasTitleBar {
  return [self supportsWindowFeature:Browser::FEATURE_TITLEBAR];
}

- (BOOL)hasToolbar {
  FullscreenToolbarLayout layout =
      [[self fullscreenToolbarController] computeLayout];
  return layout.toolbarStyle != FullscreenToolbarStyle::TOOLBAR_NONE &&
         [self supportsWindowFeature:Browser::FEATURE_TOOLBAR];
}

- (BOOL)hasLocationBar {
  return [self supportsWindowFeature:Browser::FEATURE_LOCATIONBAR];
}

- (BOOL)supportsBookmarkBar {
  return [self supportsWindowFeature:Browser::FEATURE_BOOKMARKBAR];
}

- (BOOL)isTabbedWindow {
  return browser_->is_type_tabbed();
}

- (NSRect)savedRegularWindowFrame {
  return savedRegularWindowFrame_;
}

- (BOOL)isFullscreenTransitionInProgress {
  return enteringAppKitFullscreen_ || exitingAppKitFullscreen_;
}

@end  // @implementation BrowserWindowController(WindowType)
