// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/tabs/tab_strip_controller.h"

#import <QuartzCore/QuartzCore.h>

#include <cmath>
#include <limits>
#include <string>

#include "base/command_line.h"
#include "base/mac/foundation_util.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_nsautorelease_pool.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/stl_util.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/autocomplete/autocomplete_classifier_factory.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/browser/favicon/favicon_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#include "chrome/browser/ui/cocoa/drag_util.h"
#import "chrome/browser/ui/cocoa/image_button_cell.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/new_tab_button.h"
#import "chrome/browser/ui/cocoa/tab_contents/favicon_util_mac.h"
#import "chrome/browser/ui/cocoa/tab_contents/tab_contents_controller.h"
#import "chrome/browser/ui/cocoa/tabs/alert_indicator_button_cocoa.h"
#import "chrome/browser/ui/cocoa/tabs/tab_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_drag_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_view.h"
#import "chrome/browser/ui/cocoa/tabs/tab_view.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "chrome/browser/ui/find_bar/find_bar.h"
#include "chrome/browser/ui/find_bar/find_bar_controller.h"
#include "chrome/browser/ui/find_bar/find_tab_helper.h"
#include "chrome/browser/ui/tab_ui_helper.h"
#include "chrome/browser/ui/tabs/tab_menu_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_delegate.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/grit/components_scaled_resources.h"
#include "components/omnibox/browser/autocomplete_classifier.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/prefs/pref_service.h"
#include "components/url_formatter/url_fixer.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "skia/ext/skia_utils_mac.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMNSAnimation+Duration.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"
#include "ui/base/cocoa/animation_utils.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/base/cocoa/tracking_area.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/models/list_selection_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/mac/scoped_cocoa_disable_screen_updates.h"
#include "ui/resources/grit/ui_resources.h"

using base::UserMetricsAction;
using content::OpenURLParams;
using content::Referrer;
using content::WebContents;

namespace {

// A value to indicate tab layout should use the full available width of the
// view.
const CGFloat kUseFullAvailableWidth = -1.0;

// The amount by which pinned tabs are separated from normal tabs.
const CGFloat kLastPinnedTabSpacing = 2.0;

// The amount by which the new tab button is offset (from the tabs).
const CGFloat kNewTabButtonOffset = 10.0;

const NSTimeInterval kAnimationDuration = 0.125;

// Helper class for doing NSAnimationContext calls that takes a bool to disable
// all the work.  Useful for code that wants to conditionally animate.
class ScopedNSAnimationContextGroup {
 public:
  explicit ScopedNSAnimationContextGroup(bool animate)
      : animate_(animate) {
    if (animate_) {
      [NSAnimationContext beginGrouping];
    }
  }

  ~ScopedNSAnimationContextGroup() {
    if (animate_) {
      [NSAnimationContext endGrouping];
    }
  }

  void SetCurrentContextDuration(NSTimeInterval duration) {
    if (animate_) {
      [[NSAnimationContext currentContext] gtm_setDuration:duration
                                                 eventMask:NSLeftMouseUpMask];
    }
  }

  void SetCurrentContextShortestDuration() {
    if (animate_) {
      // The minimum representable time interval.  This used to stop an
      // in-progress animation as quickly as possible.
      const NSTimeInterval kMinimumTimeInterval =
          std::numeric_limits<NSTimeInterval>::min();
      // Directly set the duration to be short, avoiding the Steve slowmotion
      // ettect the gtm_setDuration: provides.
      [[NSAnimationContext currentContext] setDuration:kMinimumTimeInterval];
    }
  }

private:
  bool animate_;
  DISALLOW_COPY_AND_ASSIGN(ScopedNSAnimationContextGroup);
};

CGFloat FlipXInView(NSView* view, CGFloat width, CGFloat x) {
  if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout())
    return [view frame].size.width - x - width;
  return x;
}

NSRect FlipRectInView(NSView* view, NSRect rect) {
  rect.origin.x = FlipXInView(view, NSWidth(rect), NSMinX(rect));
  return rect;
}

}  // namespace

@interface NSView (PrivateAPI)
// Called by AppKit to check if dragging this view should move the window.
// NSButton overrides this method in the same way so dragging window buttons
// has no effect. NSView implementation returns NSZeroRect so the whole view
// area can be dragged.
- (NSRect)_opaqueRectForWindowMoveWhenInTitlebar;
@end

@interface TabStripController (Private)
- (void)addSubviewToPermanentList:(NSView*)aView;
- (void)regenerateSubviewList;
- (NSInteger)indexForContentsView:(NSView*)view;
- (NSImage*)iconImageForContents:(content::WebContents*)contents
                         atIndex:(NSInteger)modelIndex;
- (void)updateIconsForContents:(content::WebContents*)contents
                       atIndex:(NSInteger)modelIndex;
- (void)layoutTabsWithAnimation:(BOOL)animate
             regenerateSubviews:(BOOL)doUpdate;
- (void)animationDidStop:(CAAnimation*)animation
           forController:(TabController*)controller
                finished:(BOOL)finished;
- (NSInteger)indexFromModelIndex:(NSInteger)index;
- (void)clickNewTabButton:(id)sender;
- (NSInteger)numberOfOpenTabs;
- (NSInteger)numberOfOpenPinnedTabs;
- (NSInteger)numberOfOpenNonPinnedTabs;
- (void)mouseMoved:(NSEvent*)event;
- (void)setTabTrackingAreasEnabled:(BOOL)enabled;
- (void)droppingURLsAt:(NSPoint)point
            givesIndex:(NSInteger*)index
           disposition:(WindowOpenDisposition*)disposition
           activateTab:(BOOL)activateTab;
- (void)setNewTabButtonHoverState:(BOOL)showHover;
- (void)themeDidChangeNotification:(NSNotification*)notification;
- (BOOL)doesAnyOtherWebContents:(content::WebContents*)selected
                 haveAlertState:(TabAlertState)state;
@end

// A simple view class that contains the traffic light buttons. This class
// ensures that the buttons display the icons when the mouse hovers over
// them by overriding the _mouseInGroup method.
@interface CustomWindowControlsView : NSView {
 @private
  BOOL mouseInside_;
}

// Overrides the undocumented NSView method: _mouseInGroup. When the traffic
// light buttons are drawn, they call _mouseInGroup from the superview. If
// _mouseInGroup returns YES, the buttons will draw themselves with the icons
// inside.
- (BOOL)_mouseInGroup:(NSButton*)button;
- (void)setMouseInside:(BOOL)isInside;

@end

@implementation CustomWindowControlsView

- (void)setMouseInside:(BOOL)isInside {
  if (mouseInside_ != isInside) {
    mouseInside_ = isInside;
    for (NSButton* button : [self subviews])
      [button setNeedsDisplay];
  }
}

- (BOOL)_mouseInGroup:(NSButton*)button {
  return mouseInside_;
}

@end

// A simple view class that prevents the Window Server from dragging the area
// behind tabs. Sometimes core animation confuses it. Unfortunately, it can also
// falsely pick up clicks during rapid tab closure, so we have to account for
// that.
@interface TabStripControllerDragBlockingView : NSView {
  TabStripController* controller_;  // weak; owns us
}

- (id)initWithFrame:(NSRect)frameRect
         controller:(TabStripController*)controller;

// Runs a nested runloop to do window move tracking. Overriding
// -mouseDownCanMoveWindow with a dynamic result instead doesn't work:
// http://www.cocoabuilder.com/archive/cocoa/219261-conditional-mousedowncanmovewindow-for-nsview.html
// http://www.cocoabuilder.com/archive/cocoa/92973-brushed-metal-window-dragging.html
- (void)trackClickForWindowMove:(NSEvent*)event;
@end

@implementation TabStripControllerDragBlockingView
- (BOOL)mouseDownCanMoveWindow {
  return NO;
}

- (NSRect)_opaqueRectForWindowMoveWhenInTitlebar {
 return [self bounds];
}

- (id)initWithFrame:(NSRect)frameRect
         controller:(TabStripController*)controller {
  if ((self = [super initWithFrame:frameRect])) {
    controller_ = controller;
  }
  return self;
}

// In "rapid tab closure" mode (i.e., the user is clicking close tab buttons in
// rapid succession), the animations confuse Cocoa's hit testing (which appears
// to use cached results, among other tricks), so this view can somehow end up
// getting a mouse down event. Thus we do an explicit hit test during rapid tab
// closure, and if we find that we got a mouse down we shouldn't have, we send
// it off to the appropriate view.
- (void)mouseDown:(NSEvent*)event {
  NSView* superview = [self superview];
  NSPoint hitLocation =
      [[superview superview] convertPoint:[event locationInWindow]
                                 fromView:nil];
  NSView* hitView = [superview hitTest:hitLocation];

  if ([controller_ inRapidClosureMode]) {
    if (hitView != self) {
      [hitView mouseDown:event];
      return;
    }
  }

  if (hitView == self) {
    BrowserWindowController* windowController =
        [BrowserWindowController browserWindowControllerForView:self];
    if (![windowController isInAnyFullscreenMode]) {
      [self trackClickForWindowMove:event];
      return;
    }
  }
  [super mouseDown:event];
}

- (void)trackClickForWindowMove:(NSEvent*)event {
  NSWindow* window = [self window];
  NSPoint frameOrigin = [window frame].origin;
  NSPoint lastEventLoc =
      ui::ConvertPointFromWindowToScreen(window, [event locationInWindow]);
  while ((event = [NSApp nextEventMatchingMask:
      NSLeftMouseDownMask|NSLeftMouseDraggedMask|NSLeftMouseUpMask
                                    untilDate:[NSDate distantFuture]
                                       inMode:NSEventTrackingRunLoopMode
                                      dequeue:YES]) &&
      [event type] != NSLeftMouseUp) {
    base::mac::ScopedNSAutoreleasePool pool;

    NSPoint now =
        ui::ConvertPointFromWindowToScreen(window, [event locationInWindow]);
    frameOrigin.x += now.x - lastEventLoc.x;
    frameOrigin.y += now.y - lastEventLoc.y;
    [window setFrameOrigin:frameOrigin];
    lastEventLoc = now;
  }
}

@end

#pragma mark -

// A delegate, owned by the CAAnimation system, that is alerted when the
// animation to close a tab is completed. Calls back to the given tab strip
// to let it know that |controller_| is ready to be removed from the model.
// Since we only maintain weak references, the tab strip must call -invalidate:
// to prevent the use of dangling pointers.
@interface TabCloseAnimationDelegate : NSObject <CAAnimationDelegate> {
 @private
  TabStripController* strip_;  // weak; owns us indirectly
  TabController* controller_;  // weak
}

// Will tell |strip| when the animation for |controller|'s view has completed.
// These should not be nil, and will not be retained.
- (id)initWithTabStrip:(TabStripController*)strip
         tabController:(TabController*)controller;

// Invalidates this object so that no further calls will be made to
// |strip_|.  This should be called when |strip_| is released, to
// prevent attempts to call into the released object.
- (void)invalidate;

// CAAnimation delegate methods
- (void)animationDidStart:(CAAnimation*)animation;
- (void)animationDidStop:(CAAnimation*)animation finished:(BOOL)finished;

@end

@implementation TabCloseAnimationDelegate

- (id)initWithTabStrip:(TabStripController*)strip
         tabController:(TabController*)controller {
  if ((self = [super init])) {
    DCHECK(strip && controller);
    strip_ = strip;
    controller_ = controller;
  }
  return self;
}

- (void)invalidate {
  strip_ = nil;
  controller_ = nil;
}

- (void)animationDidStart:(CAAnimation*)theAnimation {
  // CAAnimationDelegate method added on OSX 10.12.
}
- (void)animationDidStop:(CAAnimation*)animation finished:(BOOL)finished {
  [strip_ animationDidStop:animation
             forController:controller_
                  finished:finished];
}

@end

#pragma mark -

// In general, there is a one-to-one correspondence between TabControllers,
// TabViews, TabContentsControllers, and the WebContents in the
// TabStripModel. In the steady-state, the indices line up so an index coming
// from the model is directly mapped to the same index in the parallel arrays
// holding our views and controllers. This is also true when new tabs are
// created (even though there is a small period of animation) because the tab is
// present in the model while the TabView is animating into place. As a result,
// nothing special need be done to handle "new tab" animation.
//
// This all goes out the window with the "close tab" animation. The animation
// kicks off in |-tabDetachedWithContents:atIndex:| with the notification that
// the tab has been removed from the model. The simplest solution at this
// point would be to remove the views and controllers as well, however once
// the TabView is removed from the view list, the tab z-order code takes care of
// removing it from the tab strip and we'll get no animation. That means if
// there is to be any visible animation, the TabView needs to stay around until
// its animation is complete. In order to maintain consistency among the
// internal parallel arrays, this means all structures are kept around until
// the animation completes. At this point, though, the model and our internal
// structures are out of sync: the indices no longer line up. As a result,
// there is a concept of a "model index" which represents an index valid in
// the TabStripModel. During steady-state, the "model index" is just the same
// index as our parallel arrays (as above), but during tab close animations,
// it is different, offset by the number of tabs preceding the index which
// are undergoing tab closing animation. As a result, the caller needs to be
// careful to use the available conversion routines when accessing the internal
// parallel arrays (e.g., -indexFromModelIndex:). Care also needs to be taken
// during tab layout to ignore closing tabs in the total width calculations and
// in individual tab positioning (to avoid moving them right back to where they
// were).
//
// In order to prevent actions being taken on tabs which are closing, the tab
// itself gets marked as such so it no longer will send back its select action
// or allow itself to be dragged. In addition, drags on the tab strip as a
// whole are disabled while there are tabs closing.

@implementation TabStripController

@synthesize leadingIndentForControls = leadingIndentForControls_;
@synthesize trailingIndentForControls = trailingIndentForControls_;

+ (CGFloat)tabAnimationDuration {
  return kAnimationDuration;
}

- (id)initWithView:(TabStripView*)view
        switchView:(NSView*)switchView
           browser:(Browser*)browser
          delegate:(id<TabStripControllerDelegate>)delegate {
  DCHECK(view && switchView && browser && delegate);
  if ((self = [super init])) {
    tabStripView_.reset([view retain]);
    [tabStripView_ setController:self];
    switchView_ = switchView;
    browser_ = browser;
    tabStripModel_ = browser_->tab_strip_model();
    hoverTabSelector_.reset(new HoverTabSelector(tabStripModel_));
    delegate_ = delegate;
    bridge_.reset(new TabStripModelObserverBridge(tabStripModel_, self));
    dragController_.reset(
        [[TabStripDragController alloc] initWithTabStripController:self]);
    tabContentsArray_.reset([[NSMutableArray alloc] init]);
    tabArray_.reset([[NSMutableArray alloc] init]);
    NSWindow* browserWindow = [view window];

    // Important note: any non-tab subviews not added to |permanentSubviews_|
    // (see |-addSubviewToPermanentList:|) will be wiped out.
    permanentSubviews_.reset([[NSMutableArray alloc] init]);

    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    defaultFavicon_.reset(
        rb.GetNativeImageNamed(IDR_DEFAULT_FAVICON).CopyNSImage());

    [self setLeadingIndentForControls:[[self class]
                                          defaultLeadingIndentForControls]];
    [self setTrailingIndentForControls:0];

    // Add this invisible view first so that it is ordered below other views.
    dragBlockingView_.reset(
        [[TabStripControllerDragBlockingView alloc] initWithFrame:NSZeroRect
                                                       controller:self]);
    [self addSubviewToPermanentList:dragBlockingView_];

    newTabButton_ = [view getNewTabButton];
    [newTabButton_ setWantsLayer:YES];
    [self addSubviewToPermanentList:newTabButton_];
    [newTabButton_ setTarget:self];
    [newTabButton_ setAction:@selector(clickNewTabButton:)];

    newTabButtonShowingHoverImage_ = NO;
    newTabTrackingArea_.reset(
        [[CrTrackingArea alloc] initWithRect:[newTabButton_ bounds]
                                     options:(NSTrackingMouseEnteredAndExited |
                                              NSTrackingActiveAlways)
                                       owner:self
                                    userInfo:nil]);
    if (browserWindow)  // Nil for Browsers without a tab strip (e.g. popups).
      [newTabTrackingArea_ clearOwnerWhenWindowWillClose:browserWindow];
    [newTabButton_ addTrackingArea:newTabTrackingArea_.get()];
    targetFrames_.reset([[NSMutableDictionary alloc] init]);

    newTabTargetFrame_ = NSZeroRect;
    availableResizeWidth_ = kUseFullAvailableWidth;

    closingControllers_.reset([[NSMutableSet alloc] init]);

    // Install the permanent subviews.
    [self regenerateSubviewList];

    // Watch for notifications that the tab strip view has changed size so
    // we can tell it to layout for the new size.
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(tabViewFrameChanged:)
               name:NSViewFrameDidChangeNotification
             object:tabStripView_];

    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(themeDidChangeNotification:)
               name:kBrowserThemeDidChangeNotification
             object:nil];

    trackingArea_.reset([[CrTrackingArea alloc]
        initWithRect:NSZeroRect  // Ignored by NSTrackingInVisibleRect
             options:NSTrackingMouseEnteredAndExited |
                     NSTrackingMouseMoved |
                     NSTrackingActiveAlways |
                     NSTrackingInVisibleRect
               owner:self
            userInfo:nil]);
    if (browserWindow)  // Nil for Browsers without a tab strip (e.g. popups).
      [trackingArea_ clearOwnerWhenWindowWillClose:browserWindow];
    [tabStripView_ addTrackingArea:trackingArea_.get()];

    // Check to see if the mouse is currently in our bounds so we can
    // enable the tracking areas.  Otherwise we won't get hover states
    // or tab gradients if we load the window up under the mouse.
    NSPoint mouseLoc = [[view window] mouseLocationOutsideOfEventStream];
    mouseLoc = [view convertPoint:mouseLoc fromView:nil];
    if (NSPointInRect(mouseLoc, [view bounds])) {
      [self setTabTrackingAreasEnabled:YES];
      mouseInside_ = YES;
    }

    // Set accessibility descriptions. http://openradar.appspot.com/7496255
    NSString* description = l10n_util::GetNSStringWithFixup(IDS_ACCNAME_NEWTAB);
    [[newTabButton_ cell]
        accessibilitySetOverrideValue:description
                         forAttribute:NSAccessibilityDescriptionAttribute];

    // Controller may have been (re-)created by switching layout modes, which
    // means the tab model is already fully formed with tabs. Need to walk the
    // list and create the UI for each.
    const int existingTabCount = tabStripModel_->count();
    const content::WebContents* selection =
        tabStripModel_->GetActiveWebContents();
    for (int i = 0; i < existingTabCount; ++i) {
      content::WebContents* currentContents =
          tabStripModel_->GetWebContentsAt(i);
      [self insertTabWithContents:currentContents
                          atIndex:i
                     inForeground:NO];
      if (selection == currentContents) {
        // Must manually force a selection since the model won't send
        // selection messages in this scenario.
        [self
            activateTabWithContents:currentContents
                   previousContents:NULL
                            atIndex:i
                             reason:TabStripModelObserver::CHANGE_REASON_NONE];
      }
    }
    // Don't lay out the tabs until after the controller has been fully
    // constructed.
    if (existingTabCount) {
      [self performSelectorOnMainThread:@selector(layoutTabs)
                             withObject:nil
                          waitUntilDone:NO];
    }
  }
  return self;
}

- (void)dealloc {
  [self browserWillBeDestroyed];
  [super dealloc];
}

- (void)browserWillBeDestroyed {
  [tabStripView_ setController:nil];

  if (trackingArea_.get())
    [tabStripView_ removeTrackingArea:trackingArea_.get()];

  [newTabButton_ removeTrackingArea:newTabTrackingArea_.get()];
  // Invalidate all closing animations so they don't call back to us after
  // we're gone.
  for (TabController* controller in closingControllers_.get()) {
    NSView* view = [controller view];
    [[[view animationForKey:@"frameOrigin"] delegate] invalidate];
  }
  [[NSNotificationCenter defaultCenter] removeObserver:self];

  browser_ = nullptr;
}

+ (CGFloat)defaultTabHeight {
  return [TabController defaultTabHeight];
}

+ (CGFloat)defaultLeadingIndentForControls {
  // Default indentation leaves enough room so tabs don't overlap with the
  // window controls.
  return 70.0;
}

+ (CGFloat)tabOverlap {
  // The overlap value needs to be <= the x position of the favicon within a
  // tab. Else, every time the throbber is painted, the throbber's invalidation
  // will also invalidate parts of the tab to the left, and two tabs's
  // backgrounds need to be painted on each throbber frame instead of one.
  const CGFloat kTabOverlap = 18.0;
  return kTabOverlap;
}

// Finds the TabContentsController associated with the given index into the tab
// model and swaps out the sole child of the contentArea to display its
// contents.
- (void)swapInTabAtIndex:(NSInteger)modelIndex {
  DCHECK(modelIndex >= 0 && modelIndex < tabStripModel_->count());
  NSInteger index = [self indexFromModelIndex:modelIndex];
  TabContentsController* controller = [tabContentsArray_ objectAtIndex:index];

  // Make sure we do not draw any transient arrangements of views.
  gfx::ScopedCocoaDisableScreenUpdates cocoa_disabler;
  // Make sure that any layers that move are not animated to their new
  // positions.
  ScopedCAActionDisabler ca_disabler;

  // Ensure the nib is loaded. Sizing won't occur until it's added to the view
  // hierarchy with -ensureContentsVisibleInSuperview:.
  [controller view];

  // Remove the old view from the view hierarchy to suppress resizes. We know
  // there's only one child of |switchView_| because we're the one who put it
  // there. There may not be any children in the case of a tab that's been
  // closed, in which case there's nothing removed.
  [[[switchView_ subviews] firstObject] removeFromSuperview];

  // Prepare the container with any infobars or docked devtools it wants.
  [delegate_ onActivateTabWithContents:[controller webContents]];

  // Sizes the WebContents to match the possibly updated size of |switchView_|,
  // then adds it and starts auto-resizing again.
  [controller ensureContentsVisibleInSuperview:switchView_];
}

// Create a new tab view and set its cell correctly so it draws the way we want
// it to. It will be sized and positioned by |-layoutTabs| so there's no need to
// set the frame here. This also creates the view as hidden, it will be
// shown during layout.
- (TabController*)newTab {
  TabController* controller = [[[TabController alloc] init] autorelease];
  [controller setTarget:self];
  [controller setAction:@selector(selectTab:)];
  [[controller view] setHidden:YES];

  return controller;
}

// (Private) Handles a click on the new tab button.
- (void)clickNewTabButton:(id)sender {
  base::RecordAction(UserMetricsAction("NewTab_Button"));
  UMA_HISTOGRAM_ENUMERATION("Tab.NewTab", TabStripModel::NEW_TAB_BUTTON,
                            TabStripModel::NEW_TAB_ENUM_COUNT);
  tabStripModel_->delegate()->AddTabAt(GURL(), -1, true);
}

// (Private) Returns the number of open tabs in the tab strip. This is the
// number of TabControllers we know about (as there's a 1-to-1 mapping from
// these controllers to a tab) less the number of closing tabs.
- (NSInteger)numberOfOpenTabs {
  return static_cast<NSInteger>(tabStripModel_->count());
}

// (Private) Returns the number of open, pinned tabs.
- (NSInteger)numberOfOpenPinnedTabs {
  // Ask the model for the number of pinned tabs. Note that tabs which are in
  // the process of closing (i.e., whose controllers are in
  // |closingControllers_|) have already been removed from the model.
  return tabStripModel_->IndexOfFirstNonPinnedTab();
}

// (Private) Returns the number of open, non-pinned tabs.
- (NSInteger)numberOfOpenNonPinnedTabs {
  NSInteger number = [self numberOfOpenTabs] - [self numberOfOpenPinnedTabs];
  DCHECK_GE(number, 0);
  return number;
}

// Given an index into the tab model, returns the index into the tab controller
// or tab contents controller array accounting for tabs that are currently
// closing. For example, if there are two tabs in the process of closing before
// |index|, this returns |index| + 2. If there are no closing tabs, this will
// return |index|.
- (NSInteger)indexFromModelIndex:(NSInteger)index {
  DCHECK_GE(index, 0);
  if (index < 0)
    return index;

  NSInteger i = 0;
  for (TabController* controller in tabArray_.get()) {
    if ([closingControllers_ containsObject:controller]) {
      DCHECK([[controller tabView] isClosing]);
      ++index;
    }
    if (i == index)  // No need to check anything after, it has no effect.
      break;
    ++i;
  }
  return index;
}

// Given an index into |tabArray_|, return the corresponding index into
// |tabStripModel_| or NSNotFound if the specified tab does not exist in
// the model (if it's closing, for example).
- (NSInteger)modelIndexFromIndex:(NSInteger)index {
  NSInteger modelIndex = 0;
  NSInteger arrayIndex = 0;
  for (TabController* controller in tabArray_.get()) {
    if (![closingControllers_ containsObject:controller]) {
      if (arrayIndex == index)
        return modelIndex;
      ++modelIndex;
    } else if (arrayIndex == index) {
      // Tab is closing - no model index.
      return NSNotFound;
    }
    ++arrayIndex;
  }
  return NSNotFound;
}

// Returns the index of the subview |view|. Returns -1 if not present. Takes
// closing tabs into account such that this index will correctly match the tab
// model. If |view| is in the process of closing, returns -1, as closing tabs
// are no longer in the model.
- (NSInteger)modelIndexForTabView:(NSView*)view {
  NSInteger index = 0;
  for (TabController* current in tabArray_.get()) {
    // If |current| is closing, skip it.
    if ([closingControllers_ containsObject:current])
      continue;
    else if ([current view] == view)
      return index;
    ++index;
  }
  return -1;
}

// Returns the index of the contents subview |view|. Returns -1 if not present.
// Takes closing tabs into account such that this index will correctly match the
// tab model. If |view| is in the process of closing, returns -1, as closing
// tabs are no longer in the model.
- (NSInteger)modelIndexForContentsView:(NSView*)view {
  NSInteger index = 0;
  NSInteger i = 0;
  for (TabContentsController* current in tabContentsArray_.get()) {
    // If the TabController corresponding to |current| is closing, skip it.
    TabController* controller = [tabArray_ objectAtIndex:i];
    if ([closingControllers_ containsObject:controller]) {
      ++i;
      continue;
    } else if ([current view] == view) {
      return index;
    }
    ++index;
    ++i;
  }
  return -1;
}

- (NSArray*)selectedViews {
  NSMutableArray* views = [NSMutableArray arrayWithCapacity:[tabArray_ count]];
  for (TabController* tab in tabArray_.get()) {
    if ([tab selected])
      [views addObject:[tab tabView]];
  }
  return views;
}

// Returns the view at the given index, using the array of TabControllers to
// get the associated view. Returns nil if out of range.
- (NSView*)viewAtIndex:(NSUInteger)index {
  if (index >= [tabArray_ count])
    return NULL;
  return [[tabArray_ objectAtIndex:index] view];
}

- (NSUInteger)viewsCount {
  return [tabArray_ count];
}

// Called when the user clicks a tab. Tell the model the selection has changed,
// which feeds back into us via a notification.
- (void)selectTab:(id)sender {
  DCHECK([sender isKindOfClass:[NSView class]]);
  int index = [self modelIndexForTabView:sender];
  NSUInteger modifiers = [[NSApp currentEvent] modifierFlags];
  if (tabStripModel_->ContainsIndex(index)) {
    if (modifiers & NSCommandKeyMask && modifiers & NSShiftKeyMask) {
      tabStripModel_->AddSelectionFromAnchorTo(index);
    } else if (modifiers & NSShiftKeyMask) {
      tabStripModel_->ExtendSelectionTo(index);
    } else if (modifiers & NSCommandKeyMask) {
      tabStripModel_->ToggleSelectionAt(index);
    } else {
      tabStripModel_->ActivateTabAt(index, true);
    }
  }
}

// Called when the user clicks the tab audio indicator to mute the tab.
- (void)toggleMute:(id)sender {
  DCHECK([sender isKindOfClass:[TabView class]]);
  NSInteger index = [self modelIndexForTabView:sender];
  if (!tabStripModel_->ContainsIndex(index))
    return;
  WebContents* contents = tabStripModel_->GetWebContentsAt(index);
  chrome::SetTabAudioMuted(contents, !contents->IsAudioMuted(),
                           TabMutedReason::AUDIO_INDICATOR, std::string());
}

// Called when the user closes a tab. Asks the model to close the tab. |sender|
// is the TabView that is potentially going away.
- (void)closeTab:(id)sender {
  DCHECK([sender isKindOfClass:[TabView class]]);

  // Cancel any pending tab transition.
  hoverTabSelector_->CancelTabTransition();

  if ([hoveredTab_ isEqual:sender])
    [self setHoveredTab:nil];

  NSInteger index = [self modelIndexForTabView:sender];
  if (!tabStripModel_->ContainsIndex(index))
    return;

  base::RecordAction(UserMetricsAction("CloseTab_Mouse"));
  const NSInteger numberOfOpenTabs = [self numberOfOpenTabs];
  if (numberOfOpenTabs > 1) {
    bool isClosingLastTab = index == numberOfOpenTabs - 1;
    if (!isClosingLastTab) {
      // Limit the width available for laying out tabs so that tabs are not
      // resized until a later time (when the mouse leaves the tab strip).
      // However, if the tab being closed is a pinned tab, break out of
      // rapid-closure mode since the mouse is almost guaranteed not to be over
      // the closebox of the adjacent tab (due to the difference in widths).
      // TODO(pinkerton): re-visit when handling tab overflow.
      // http://crbug.com/188
      if (tabStripModel_->IsTabPinned(index)) {
        availableResizeWidth_ = kUseFullAvailableWidth;
      } else {
        NSView* penultimateTab = [self viewAtIndex:numberOfOpenTabs - 2];
        availableResizeWidth_ =
            cocoa_l10n_util::ShouldDoExperimentalRTLLayout()
                ? FlipXInView(tabStripView_, 0, NSMinX([penultimateTab frame]))
                : NSMaxX([penultimateTab frame]);
      }
    } else {
      // If the trailing tab is closed, change the available width so that
      // another tab's close button lands below the cursor (assuming the tabs
      // are currently below their maximum width and can grow).
      NSView* lastTab = [self viewAtIndex:numberOfOpenTabs - 1];
      availableResizeWidth_ =
          cocoa_l10n_util::ShouldDoExperimentalRTLLayout()
              ? FlipXInView(tabStripView_, 0, NSMinX([lastTab frame]))
              : NSMaxX([lastTab frame]);
    }
    tabStripModel_->CloseWebContentsAt(
        index,
        TabStripModel::CLOSE_USER_GESTURE |
        TabStripModel::CLOSE_CREATE_HISTORICAL_TAB);
  } else {
    // Use the standard window close if this is the last tab
    // this prevents the tab from being removed from the model until after
    // the window dissapears
    [[tabStripView_ window] performClose:nil];
  }
}

// Dispatch context menu commands for the given tab controller.
- (void)commandDispatch:(TabStripModel::ContextMenuCommand)command
          forController:(TabController*)controller {
  int index = [self modelIndexForTabView:[controller view]];
  if (tabStripModel_->ContainsIndex(index))
    tabStripModel_->ExecuteContextMenuCommand(index, command);
}

// Returns YES if the specificed command should be enabled for the given
// controller.
- (BOOL)isCommandEnabled:(TabStripModel::ContextMenuCommand)command
           forController:(TabController*)controller {
  int index = [self modelIndexForTabView:[controller view]];
  if (!tabStripModel_->ContainsIndex(index))
    return NO;
  return tabStripModel_->IsContextMenuCommandEnabled(index, command) ? YES : NO;
}

// Returns a context menu model for a given controller. Caller owns the result.
- (ui::SimpleMenuModel*)contextMenuModelForController:(TabController*)controller
    menuDelegate:(ui::SimpleMenuModel::Delegate*)delegate {
  int index = [self modelIndexForTabView:[controller view]];
  return new TabMenuModel(delegate, tabStripModel_, index);
}

// Returns a weak reference to the controller that manages dragging of tabs.
- (id<TabDraggingEventTarget>)dragController {
  return dragController_.get();
}

- (void)insertPlaceholderForTab:(TabView*)tab frame:(NSRect)frame {
  placeholderTab_ = tab;
  placeholderFrame_ = frame;
  [self layoutTabsWithAnimation:initialLayoutComplete_ regenerateSubviews:NO];
}

- (BOOL)isDragSessionActive {
  return placeholderTab_ != nil;
}

- (BOOL)isTabFullyVisible:(TabView*)tab {
  NSRect frame = [tab frame];
  if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout()) {
    return NSMinX(frame) >= [self trailingIndentForControls] &&
           NSMaxX(frame) <= [self tabAreaRightEdge];
  } else {
    return NSMinX(frame) >= [self leadingIndentForControls] &&
           NSMaxX(frame) <= [self tabAreaRightEdge];
  }
}

- (CGFloat)tabAreaRightEdge {
  CGFloat rightEdge = cocoa_l10n_util::ShouldDoExperimentalRTLLayout()
                          ? [self leadingIndentForControls]
                          : [self trailingIndentForControls];
  return NSMaxX([tabStripView_ frame]) - rightEdge;
}

- (void)showNewTabButton:(BOOL)show {
  forceNewTabButtonHidden_ = show ? NO : YES;
  if (forceNewTabButtonHidden_)
    [newTabButton_ setHidden:YES];
}

// Lay out all tabs in the order of their TabContentsControllers, which matches
// the ordering in the TabStripModel. This call isn't that expensive, though
// it is O(n) in the number of tabs. Tabs will animate to their new position
// if the window is visible and |animate| is YES.
// TODO(pinkerton): Note this doesn't do too well when the number of min-sized
// tabs would cause an overflow. http://crbug.com/188
- (void)layoutTabsWithAnimation:(BOOL)animate
             regenerateSubviews:(BOOL)doUpdate {
  DCHECK([NSThread isMainThread]);
  if (![tabArray_ count])
    return;

  const CGFloat kMaxTabWidth = [TabController maxTabWidth];
  const CGFloat kMinTabWidth = [TabController minTabWidth];
  const CGFloat kMinActiveTabWidth = [TabController minActiveTabWidth];
  const CGFloat kPinnedTabWidth = [TabController pinnedTabWidth];
  const CGFloat kTabOverlap = [TabStripController tabOverlap];

  NSRect enclosingRect = NSZeroRect;
  ScopedNSAnimationContextGroup mainAnimationGroup(animate);
  mainAnimationGroup.SetCurrentContextDuration(kAnimationDuration);

  // Update the current subviews and their z-order if requested.
  if (doUpdate)
    [self regenerateSubviewList];

  // Compute the base width of tabs given how much room we're allowed. Note that
  // pinned tabs have a fixed width. We may not be able to use the entire width
  // if the user is quickly closing tabs. This may be negative, but that's okay
  // (taken care of by |MAX()| when calculating tab sizes).
  CGFloat availableSpace = 0;
  if ([self inRapidClosureMode]) {
    availableSpace = availableResizeWidth_;
  } else {
    availableSpace = NSWidth([tabStripView_ frame]);

    // Account for the width of the new tab button.
    availableSpace -=
        NSWidth([newTabButton_ frame]) + kNewTabButtonOffset - kTabOverlap;
    // Account for the trailing controls if not in rapid closure mode.
    // (In rapid closure mode, the available width is set based on the
    // position of the trailing tab, not based on the width of the tab strip,
    // so the trailing controls have already been accounted for.)
    availableSpace -= [self trailingIndentForControls];
  }

  // Need to leave room for the leading controls even in rapid closure mode.
  availableSpace -= [self leadingIndentForControls];

  // This may be negative, but that's okay (taken care of by |MAX()| when
  // calculating tab sizes). "pinned" tabs in horizontal mode just get a special
  // section, they don't change size.
  CGFloat availableSpaceForNonPinned = availableSpace;
  if ([self numberOfOpenPinnedTabs]) {
    availableSpaceForNonPinned -=
        [self numberOfOpenPinnedTabs] * (kPinnedTabWidth - kTabOverlap);
    availableSpaceForNonPinned -= kLastPinnedTabSpacing;
  }

  // Initialize |nonPinnedTabWidth| in case there aren't any non-pinned tabs;
  // this value shouldn't actually be used.
  CGFloat nonPinnedTabWidth = kMaxTabWidth;
  CGFloat nonPinnedTabWidthFraction = 0;
  NSInteger numberOfNonPinnedTabs = MIN(
      [self numberOfOpenNonPinnedTabs],
      (availableSpaceForNonPinned - kTabOverlap) /
          (kMinTabWidth - kTabOverlap));

  if (numberOfNonPinnedTabs) {
    // Find the width of a non-pinned tab. This only applies to horizontal
    // mode. Add in the amount we "get back" from the tabs overlapping.
    nonPinnedTabWidth =
        ((availableSpaceForNonPinned - kTabOverlap) / numberOfNonPinnedTabs) +
        kTabOverlap;

    // Clamp the width between the max and min.
    nonPinnedTabWidth = MAX(MIN(nonPinnedTabWidth, kMaxTabWidth), kMinTabWidth);

    // When there are multiple tabs, we'll have one active and some inactive
    // tabs.  If the desired width was between the minimum sizes of these types,
    // try to shrink the tabs with the smaller minimum.  For example, if we have
    // a strip of width 10 with 4 tabs, the desired width per tab will be 2.5.
    // If selected tabs have a minimum width of 4 and unselected tabs have
    // minimum width of 1, the above code would set *unselected_width = 2.5,
    // *selected_width = 4, which results in a total width of 11.5.  Instead, we
    // want to set *unselected_width = 2, *selected_width = 4, for a total width
    // of 10.
    if (numberOfNonPinnedTabs > 1 && nonPinnedTabWidth < kMinActiveTabWidth) {
      nonPinnedTabWidth = (availableSpaceForNonPinned - kMinActiveTabWidth) /
                            (numberOfNonPinnedTabs - 1) + kTabOverlap;
      if (nonPinnedTabWidth < kMinTabWidth) {
        // The above adjustment caused the tabs to not fit, show 1 less tab.
        --numberOfNonPinnedTabs;
        nonPinnedTabWidth = ((availableSpaceForNonPinned - kTabOverlap) /
                                numberOfNonPinnedTabs) + kTabOverlap;
      }
    }

    // Separate integral and fractional parts.
    CGFloat integralPart = std::floor(nonPinnedTabWidth);
    nonPinnedTabWidthFraction = nonPinnedTabWidth - integralPart;
    nonPinnedTabWidth = integralPart;
  }

  BOOL visible = [[tabStripView_ window] isVisible];

  CGFloat offset = [self leadingIndentForControls];
  bool hasPlaceholderGap = false;
  // Whether or not the last tab processed by the loop was a pinned tab.
  BOOL isLastTabPinned = NO;
  CGFloat tabWidthAccumulatedFraction = 0;
  NSInteger laidOutNonPinnedTabs = 0;

  // Lay everything out as if it was LTR and flip at the end
  // for RTL, if necessary.
  for (TabController* tab in tabArray_.get()) {
    // Ignore a tab that is going through a close animation.
    if ([closingControllers_ containsObject:tab])
      continue;

    BOOL isPlaceholder = [[tab view] isEqual:placeholderTab_];
    NSRect tabFrame = [[tab view] frame];
    tabFrame.size.height = [[self class] defaultTabHeight];
    tabFrame.origin.y = 0;
    tabFrame.origin.x = offset;

    // If the tab is hidden, we consider it a new tab. We make it visible
    // and animate it in.
    BOOL newTab = [[tab view] isHidden];
    if (newTab)
      [[tab view] setHidden:NO];

    if (isPlaceholder) {
      // Move the current tab to the correct location instantly.
      // We need a duration or else it doesn't cancel an inflight animation.
      ScopedNSAnimationContextGroup localAnimationGroup(animate);
      localAnimationGroup.SetCurrentContextShortestDuration();
      tabFrame.origin.x = placeholderFrame_.origin.x;
      id target = animate ? [[tab view] animator] : [tab view];
      [target setFrame:tabFrame];

      // Store the frame by identifier to avoid redundant calls to animator.
      NSValue* identifier = [NSValue valueWithPointer:[tab view]];
      [targetFrames_ setObject:[NSValue valueWithRect:tabFrame]
                        forKey:identifier];
      continue;
    }

    if (placeholderTab_ && !hasPlaceholderGap) {
      // If the back edge is behind the placeholder's back edge, but the
      // mid is in front of it of it, slide over to make space for it.
      bool shouldLeaveGap;
      if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout()) {
        const CGFloat tabMidpoint =
            NSMidX(FlipRectInView(tabStripView_, tabFrame));
        shouldLeaveGap = tabMidpoint < NSMaxX(placeholderFrame_);
      } else {
        shouldLeaveGap = NSMidX(tabFrame) > NSMinX(placeholderFrame_);
      }
      if (shouldLeaveGap) {
        hasPlaceholderGap = true;
        offset += NSWidth(placeholderFrame_);
        offset -= kTabOverlap;
        tabFrame.origin.x = offset;
      }
    }

    // Set the width. Selected tabs are slightly wider when things get really
    // small and thus we enforce a different minimum width.
    BOOL isPinned = [tab pinned];
    if (isPinned) {
      tabFrame.size.width = kPinnedTabWidth;
    } else {
      // Tabs have non-integer widths. Assign the integer part to the tab, and
      // keep an accumulation of the fractional parts. When the fractional
      // accumulation gets to be more than one pixel, assign that to the current
      // tab being laid out. This is vaguely inspired by Bresenham's line
      // algorithm.
      tabFrame.size.width = nonPinnedTabWidth;
      tabWidthAccumulatedFraction += nonPinnedTabWidthFraction;

      if (tabWidthAccumulatedFraction >= 1.0) {
        ++tabFrame.size.width;
        --tabWidthAccumulatedFraction;
      }

      // In case of rounding error, give any left over pixels to the last tab.
      if (laidOutNonPinnedTabs == numberOfNonPinnedTabs - 1 &&
          tabWidthAccumulatedFraction > 0.5) {
        ++tabFrame.size.width;
      }

      ++laidOutNonPinnedTabs;
    }

    if ([tab active])
      tabFrame.size.width = MAX(tabFrame.size.width, kMinActiveTabWidth);

    // If this is the first non-pinned tab, then add a bit of spacing between
    // this and the last pinned tab.
    if (!isPinned && isLastTabPinned) {
      offset += kLastPinnedTabSpacing;
      tabFrame.origin.x = offset;
    }
    isLastTabPinned = isPinned;

    // Flip if in RTL mode.
    tabFrame.origin.x =
        FlipXInView(tabStripView_, tabFrame.size.width, tabFrame.origin.x);

    if (laidOutNonPinnedTabs > numberOfNonPinnedTabs) {
      // There is not enough space to fit this tab.
      tabFrame.size.width = 0;
      [self setFrame:tabFrame ofTabView:[tab view]];
      continue;
    }

    // Animate a new tab in by putting it below the horizon unless told to put
    // it in a specific location (i.e., from a drop).
    if (newTab && visible && animate) {
      if (NSEqualRects(droppedTabFrame_, NSZeroRect)) {
        [[tab view] setFrame:NSOffsetRect(tabFrame, 0, -NSHeight(tabFrame))];
      } else {
        [[tab view] setFrame:droppedTabFrame_];
        droppedTabFrame_ = NSZeroRect;
      }
    }

    // Check the frame by identifier to avoid redundant calls to animator.
    id frameTarget = visible && animate ? [[tab view] animator] : [tab view];
    NSValue* identifier = [NSValue valueWithPointer:[tab view]];
    NSValue* oldTargetValue = [targetFrames_ objectForKey:identifier];
    if (!oldTargetValue ||
        !NSEqualRects([oldTargetValue rectValue], tabFrame)) {
      // Redraw the tab once it moves to its final location. Because we're
      // using Core Animation, each tab caches its contents until told to
      // redraw. Without forcing a redraw at the end of the move, tabs will
      // display the wrong content when using a theme that creates transparent
      // tabs.
      ScopedNSAnimationContextGroup subAnimationGroup(animate);
      subAnimationGroup.SetCurrentContextDuration(kAnimationDuration);
      NSView* tabView = [tab view];
      [[NSAnimationContext currentContext] setCompletionHandler:^{
        [tabView setNeedsDisplay:YES];
      }];

      [frameTarget setFrame:tabFrame];
      [targetFrames_ setObject:[NSValue valueWithRect:tabFrame]
                        forKey:identifier];
    }

    enclosingRect = NSUnionRect(tabFrame, enclosingRect);

    offset += NSWidth(tabFrame);
    offset -= kTabOverlap;
  }

  // Hide the new tab button if we're explicitly told to. It may already
  // be hidden, doing it again doesn't hurt. Otherwise position it
  // appropriately, showing it if necessary.
  if (forceNewTabButtonHidden_) {
    [newTabButton_ setHidden:YES];
  } else {
    NSRect newTabNewFrame = [newTabButton_ frame];
    // We've already ensured there's enough space for the new tab button
    // so we don't have to check it against the available space. We do need
    // to make sure we put it after any placeholder.
    CGFloat maxTabX = MAX(offset, NSMaxX(placeholderFrame_) - kTabOverlap);
    if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout()) {
      maxTabX = FlipXInView(tabStripView_, [newTabButton_ frame].size.width,
                            maxTabX) -
                (2 * kNewTabButtonOffset);
    }
    newTabNewFrame.origin = NSMakePoint(maxTabX + kNewTabButtonOffset, 0);
    if ([tabContentsArray_ count])
      [newTabButton_ setHidden:NO];

    if (!NSEqualRects(newTabTargetFrame_, newTabNewFrame)) {
      // Set the new tab button image correctly based on where the cursor is.
      NSWindow* window = [tabStripView_ window];
      NSPoint currentMouse = [window mouseLocationOutsideOfEventStream];
      currentMouse = [tabStripView_ convertPoint:currentMouse fromView:nil];

      BOOL shouldShowHover = [newTabButton_ pointIsOverButton:currentMouse];
      [self setNewTabButtonHoverState:shouldShowHover];

      // Move the new tab button into place. We want to animate the new tab
      // button if it's moving back (closing a tab), but not when it's
      // moving forward (inserting a new tab). If moving forward, we need
      // to use a very small duration to make sure we cancel any in-flight
      // animation to the left.
      if (visible && animate) {
        ScopedNSAnimationContextGroup localAnimationGroup(true);
        BOOL movingBack = NSMinX(newTabNewFrame) < NSMinX(newTabTargetFrame_);
        if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout())
          movingBack = !movingBack;

        if (!movingBack) {
          localAnimationGroup.SetCurrentContextShortestDuration();
        }
        [[newTabButton_ animator] setFrame:newTabNewFrame];
        newTabTargetFrame_ = newTabNewFrame;
      } else {
        [newTabButton_ setFrame:newTabNewFrame];
        newTabTargetFrame_ = newTabNewFrame;
      }
    }
  }

  [dragBlockingView_ setFrame:enclosingRect];

  // Mark that we've successfully completed layout of at least one tab.
  initialLayoutComplete_ = YES;
}

// When we're told to layout from the public API we usually want to animate,
// except when it's the first time.
- (void)layoutTabs {
  [self layoutTabsWithAnimation:initialLayoutComplete_ regenerateSubviews:YES];
}

- (void)layoutTabsWithoutAnimation {
  [self layoutTabsWithAnimation:NO regenerateSubviews:YES];
}

// Handles setting the title of the tab based on the given |contents|. Uses
// a canned string if |contents| is NULL.
- (void)setTabTitle:(TabController*)tab withContents:(WebContents*)contents {
  base::string16 title;
  if (contents)
    title = TabUIHelper::FromWebContents(contents)->GetTitle();
  [tab setTitle:base::SysUTF16ToNSString(title)];

  NSString* toolTip = base::SysUTF16ToNSString(chrome::AssembleTabTooltipText(
      title, [self alertStateForContents:contents]));
  [tab setToolTip:toolTip];
  if ([tab tabView] == hoveredTab_)
    [toolTipView_ setToolTip:toolTip];
}

// Called when a notification is received from the model to insert a new tab
// at |modelIndex|.
- (void)insertTabWithContents:(content::WebContents*)contents
                      atIndex:(NSInteger)modelIndex
                 inForeground:(bool)inForeground {
  DCHECK(contents);
  DCHECK(modelIndex == TabStripModel::kNoTab ||
         tabStripModel_->ContainsIndex(modelIndex));

  // Cancel any pending tab transition.
  hoverTabSelector_->CancelTabTransition();

  // Take closing tabs into account.
  NSInteger index = [self indexFromModelIndex:modelIndex];

  // Make a new tab. Load the contents of this tab from the nib and associate
  // the new controller with |contents| so it can be looked up later.
  base::scoped_nsobject<TabContentsController> contentsController(
      [[TabContentsController alloc]
          initWithContents:contents
                   isPopup:browser_->is_type_popup()]);
  [tabContentsArray_ insertObject:contentsController atIndex:index];

  // Make a new tab and add it to the strip. Keep track of its controller.
  TabController* newController = [self newTab];
  [newController setPinned:tabStripModel_->IsTabPinned(modelIndex)];
  [newController setUrl:contents->GetURL()];
  [tabArray_ insertObject:newController atIndex:index];
  NSView* newView = [newController view];

  // Set the originating frame to just below the strip so that it animates
  // upwards as it's being initially layed out. Oddly, this works while doing
  // something similar in |-layoutTabs| confuses the window server.
  [newView setFrame:NSOffsetRect([newView frame],
                                 0, -[[self class] defaultTabHeight])];

  [self setTabTitle:newController withContents:contents];

  // If a tab is being inserted, we can again use the entire tab strip width
  // for layout.
  availableResizeWidth_ = kUseFullAvailableWidth;

  // We don't need to call |-layoutTabs| if the tab will be in the foreground
  // because it will get called when the new tab is selected by the tab model.
  // Whenever |-layoutTabs| is called, it'll also add the new subview.
  if (!inForeground) {
    [self layoutTabs];
  }

  // During normal loading, we won't yet have a favicon and we'll get
  // subsequent state change notifications to show the throbber, but when we're
  // dragging a tab out into a new window, we have to put the tab's favicon
  // into the right state up front as we won't be told to do it from anywhere
  // else.
  [self updateIconsForContents:contents atIndex:modelIndex];

  [delegate_ onTabInsertedWithContents:contents inForeground:inForeground];
}

// Called when a notification is received from the model to close the tab with
// the WebContents.
- (void)tabClosingWithContents:(content::WebContents*)contents
                       atIndex:(NSInteger)index {
  wasHidingThrobberSet_.erase(contents);
}

// Called before |contents| is deactivated.
- (void)tabDeactivatedWithContents:(content::WebContents*)contents {
  contents->StoreFocus();
}

// Called when a notification is received from the model to select a particular
// tab. Swaps in the toolbar and content area associated with |newContents|.
- (void)activateTabWithContents:(content::WebContents*)newContents
               previousContents:(content::WebContents*)oldContents
                        atIndex:(NSInteger)modelIndex
                         reason:(int)reason {
  // It's possible for |newContents| to be null when the final tab in a tab
  // strip is closed.
  if (newContents && modelIndex != TabStripModel::kNoTab)
    TabUIHelper::FromWebContents(newContents)->set_was_active_at_least_once();

  // Take closing tabs into account.
  if (oldContents) {
    for (TabContentsController* controller in tabContentsArray_.get()) {
      if (controller.webContents == oldContents) {
        [controller willBecomeUnselectedTab];
      }
    }
  }

  NSUInteger activeIndex = [self indexFromModelIndex:modelIndex];

  [tabArray_ enumerateObjectsUsingBlock:^(TabController* current,
                                          NSUInteger index,
                                          BOOL* stop) {
      [current setActive:index == activeIndex];
      [self updateIconsForContents:newContents atIndex:modelIndex];
  }];

  // Tell the new tab contents it is about to become the selected tab. Here it
  // can do things like make sure the toolbar is up to date.
  TabContentsController* newController =
      [tabContentsArray_ objectAtIndex:activeIndex];
  [newController willBecomeSelectedTab];

  // Relayout for new tabs and to let the selected tab grow to be larger in
  // size than surrounding tabs if the user has many. This also raises the
  // selected tab to the top.
  [self layoutTabs];

  // Swap in the contents for the new tab.
  [self swapInTabAtIndex:modelIndex];

  if (newContents)
    newContents->RestoreFocus();
}

- (void)tabSelectionChanged {
  // First get the vector of indices, which is allays sorted in ascending order.
  ui::ListSelectionModel::SelectedIndices selection(
      tabStripModel_->selection_model().selected_indices());
  // Iterate through all of the tabs, selecting each as necessary.
  ui::ListSelectionModel::SelectedIndices::iterator iter = selection.begin();
  int i = 0;
  for (TabController* current in tabArray_.get()) {
    BOOL selected = iter != selection.end() &&
        [self indexFromModelIndex:*iter] == i;
    [current setSelected:selected];
    [self updateIconsForContents:tabStripModel_->GetWebContentsAt(i) atIndex:i];
    if (selected)
      ++iter;
    ++i;
  }
}

- (void)tabReplacedWithContents:(content::WebContents*)newContents
               previousContents:(content::WebContents*)oldContents
                        atIndex:(NSInteger)modelIndex {
  NSInteger index = [self indexFromModelIndex:modelIndex];
  TabContentsController* oldController =
      [tabContentsArray_ objectAtIndex:index];
  DCHECK_EQ(oldContents, [oldController webContents]);

  // Simply create a new TabContentsController for |newContents| and place it
  // into the array, replacing |oldContents|.  An ActiveTabChanged notification
  // will follow, at which point we will install the new view.
  base::scoped_nsobject<TabContentsController> newController(
      [[TabContentsController alloc]
          initWithContents:newContents
                   isPopup:browser_->is_type_popup()]);

  // Bye bye, |oldController|.
  [tabContentsArray_ replaceObjectAtIndex:index withObject:newController];

  // Fake a tab changed notification to force tab titles and favicons to update.
  [self tabChangedWithContents:newContents
                       atIndex:modelIndex
                    changeType:TabChangeType::kAll];
}

// Remove all knowledge about this tab and its associated controller, and remove
// the view from the strip.
- (void)removeTab:(TabController*)controller {
  // Cancel any pending tab transition.
  hoverTabSelector_->CancelTabTransition();

  NSUInteger index = [tabArray_ indexOfObject:controller];

  // Release the tab contents controller so those views get destroyed. This
  // will remove all the tab content Cocoa views from the hierarchy. A
  // subsequent "select tab" notification will follow from the model. To
  // tell us what to swap in in its absence.
  [tabContentsArray_ removeObjectAtIndex:index];

  // Remove the view from the tab strip.
  NSView* tab = [controller view];
  [tab removeFromSuperview];

  // Remove ourself as an observer.
  [[NSNotificationCenter defaultCenter]
      removeObserver:self
                name:NSViewDidUpdateTrackingAreasNotification
              object:tab];

  // Clear the tab controller's target.
  // TODO(viettrungluu): [crbug.com/23829] Find a better way to handle the tab
  // controller's target.
  [controller setTarget:nil];

  if ([hoveredTab_ isEqual:tab])
    [self setHoveredTab:nil];

  NSValue* identifier = [NSValue valueWithPointer:tab];
  [targetFrames_ removeObjectForKey:identifier];

  // Once we're totally done with the tab, delete its controller
  [tabArray_ removeObjectAtIndex:index];
}

// Called by the CAAnimation delegate when the tab completes the closing
// animation.
- (void)animationDidStop:(CAAnimation*)animation
           forController:(TabController*)controller
                finished:(BOOL)finished{
  [(TabCloseAnimationDelegate *)[animation delegate] invalidate];
  [closingControllers_ removeObject:controller];
  [self removeTab:controller];
}

// Save off which TabController is closing and tell its view's animator
// where to move the tab to. Registers a delegate to call back when the
// animation is complete in order to remove the tab from the model.
- (void)startClosingTabWithAnimation:(TabController*)closingTab {
  DCHECK([NSThread isMainThread]);

  // Cancel any pending tab transition.
  hoverTabSelector_->CancelTabTransition();

  // Save off the controller into the set of animating tabs. This alerts
  // the layout method to not do anything with it and allows us to correctly
  // calculate offsets when working with indices into the model.
  [closingControllers_ addObject:closingTab];

  // Mark the tab as closing. This prevents it from generating any drags or
  // selections while it's animating closed.
  [[closingTab tabView] setClosing:YES];

  // Register delegate (owned by the animation system).
  NSView* tabView = [closingTab view];
  CAAnimation* animation = [[tabView animationForKey:@"frameOrigin"] copy];
  [animation autorelease];
  base::scoped_nsobject<TabCloseAnimationDelegate> delegate(
      [[TabCloseAnimationDelegate alloc] initWithTabStrip:self
                                            tabController:closingTab]);
  [animation setDelegate:delegate.get()];  // Retains delegate.
  NSMutableDictionary* animationDictionary =
      [NSMutableDictionary dictionaryWithDictionary:[tabView animations]];
  [animationDictionary setObject:animation forKey:@"frameOrigin"];
  [tabView setAnimations:animationDictionary];

  // Periscope down! Animate the tab.
  NSRect newFrame = [tabView frame];
  newFrame = NSOffsetRect(newFrame, 0, -newFrame.size.height);
  ScopedNSAnimationContextGroup animationGroup(true);
  animationGroup.SetCurrentContextDuration(kAnimationDuration);
  [[tabView animator] setFrame:newFrame];
}

// Called when a notification is received from the model that the given tab
// has gone away. Start an animation then force a layout to put everything
// in motion.
- (void)tabDetachedWithContents:(content::WebContents*)contents
                        atIndex:(NSInteger)modelIndex {
  // Take closing tabs into account.
  NSInteger index = [self indexFromModelIndex:modelIndex];

  // Cancel any pending tab transition.
  hoverTabSelector_->CancelTabTransition();

  TabController* tab = [tabArray_ objectAtIndex:index];
  if (tabStripModel_->count() > 0) {
    [self startClosingTabWithAnimation:tab];
    [self layoutTabs];
  } else {
    // Don't remove the tab, as that makes the window look jarring without any
    // tabs. Instead, simply mark it as closing to prevent the tab from
    // generating any drags or selections.
    [[tab tabView] setClosing:YES];
  }

  [delegate_ onTabDetachedWithContents:contents];
}

// A helper routine for creating an NSImageView to hold the favicon or app icon
// for |contents|.
- (NSImage*)iconImageForContents:(content::WebContents*)contents
                         atIndex:(NSInteger)modelIndex {
  extensions::TabHelper* extensions_tab_helper =
      extensions::TabHelper::FromWebContents(contents);
  BOOL isApp = extensions_tab_helper->is_app();
  NSImage* image = nil;
  // Favicons come from the renderer, and the renderer draws everything in the
  // system color space.
  CGColorSpaceRef colorSpace = base::mac::GetSystemColorSpace();
  if (isApp) {
    SkBitmap* icon = extensions_tab_helper->GetExtensionAppIcon();
    if (icon)
      image = skia::SkBitmapToNSImageWithColorSpace(*icon, colorSpace);
  } else {
    TabController* tab = [tabArray_ objectAtIndex:modelIndex];
    image = mac::FaviconForWebContents(contents, [[tab tabView] iconColor]);
  }

  // Either we don't have a valid favicon or there was some issue converting it
  // from an SkBitmap. Either way, just show the default.
  if (!image)
    image = defaultFavicon_.get();

  return image;
}

// Updates the current loading state, replacing the icon view with a favicon,
// a throbber, the default icon, or nothing at all.
- (void)updateIconsForContents:(content::WebContents*)contents
                       atIndex:(NSInteger)modelIndex {
  if (!contents)
    return;

  // Take closing tabs into account.
  NSInteger index = [self indexFromModelIndex:modelIndex];
  TabController* tabController = [tabArray_ objectAtIndex:index];
  TabUIHelper* tabUIHelper = TabUIHelper::FromWebContents(contents);

  bool oldShowIcon = [tabController showIcon];
  bool tabIsCrashed = contents->IsCrashed();
  bool showIcon = favicon::ShouldDisplayFavicon(contents) || tabIsCrashed ||
                  tabStripModel_->IsTabPinned(modelIndex);

  TabLoadingState oldLoadingState = [tabController loadingState];
  TabLoadingState newLoadingState = kTabDone;
  if (tabIsCrashed) {
    newLoadingState = kTabCrashed;
  } else if (contents->IsWaitingForResponse()) {
    newLoadingState = kTabWaiting;
  } else if (contents->IsLoadingToDifferentDocument()) {
    newLoadingState = kTabLoading;
  }

  // Use TabUIHelper to determine if we would like to hide the throbber and
  // override the favicon. We want to hide the throbber for 2 cases. 1) when a
  // new tab is opened in the background and its initial navigation is delayed,
  // and 2) when a tab is created by session restore. For the 1st one, there is
  // no favicon available when the WebContents' initial navigation is delayed.
  // So TabUIHelper will fetch the favicon from history if available and use
  // that. For the 2nd case, TabUIhelper will return an empty favicon, so the
  // WebContents' favicon is used.
  NSImage* newImage = nil;
  if (tabUIHelper->ShouldHideThrobber()) {
    wasHidingThrobberSet_.insert(contents);

    gfx::Image favicon = tabUIHelper->GetFavicon();
    newImage = favicon.IsEmpty()
                   ? [self iconImageForContents:contents atIndex:modelIndex]
                   : favicon.AsNSImage();
  } else if (base::ContainsKey(wasHidingThrobberSet_, contents) ||
             newLoadingState == kTabDone ||
             oldLoadingState != newLoadingState || oldShowIcon != showIcon) {
    wasHidingThrobberSet_.erase(contents);

    if (showIcon && newLoadingState == kTabDone) {
      newImage = [self iconImageForContents:contents atIndex:modelIndex];
    }
  }

  [tabController setIconImage:newImage
              forLoadingState:newLoadingState
                     showIcon:showIcon];

  TabAlertState alertState = [self alertStateForContents:contents];
  [self updateWindowAlertState:alertState forWebContents:contents];
  [tabController setAlertState:alertState];

  [tabController updateVisibility];
}

// Called when a notification is received from the model that the given tab
// has been updated. |loading| will be YES when we only want to update the
// throbber state, not anything else about the (partially) loading tab.
- (void)tabChangedWithContents:(content::WebContents*)contents
                       atIndex:(NSInteger)modelIndex
                    changeType:(TabChangeType)change {
  // Take closing tabs into account.
  NSInteger index = [self indexFromModelIndex:modelIndex];

  if (modelIndex == tabStripModel_->active_index())
    [delegate_ onTabChanged:change withContents:contents];

  TabController* tabController = [tabArray_ objectAtIndex:index];

  if (change != TabChangeType::kLoadingOnly)
    [self setTabTitle:tabController withContents:contents];

  [self updateIconsForContents:contents atIndex:modelIndex];

  TabContentsController* updatedController =
      [tabContentsArray_ objectAtIndex:index];
  [updatedController tabDidChange:contents];
}

// Called when a tab is moved (usually by drag&drop). Keep our parallel arrays
// in sync with the tab strip model. It can also be pinned/unpinned
// simultaneously, so we need to take care of that.
- (void)tabMovedWithContents:(content::WebContents*)contents
                   fromIndex:(NSInteger)modelFrom
                     toIndex:(NSInteger)modelTo {
  // Take closing tabs into account.
  NSInteger from = [self indexFromModelIndex:modelFrom];
  NSInteger to = [self indexFromModelIndex:modelTo];

  // Cancel any pending tab transition.
  hoverTabSelector_->CancelTabTransition();

  base::scoped_nsobject<TabContentsController> movedTabContentsController(
      [[tabContentsArray_ objectAtIndex:from] retain]);
  [tabContentsArray_ removeObjectAtIndex:from];
  [tabContentsArray_ insertObject:movedTabContentsController.get()
                          atIndex:to];
  base::scoped_nsobject<TabController> movedTabController(
      base::mac::ObjCCastStrict<TabController>(
          [[tabArray_ objectAtIndex:from] retain]));
  [tabArray_ removeObjectAtIndex:from];
  [tabArray_ insertObject:movedTabController.get() atIndex:to];

  // The tab moved, which means that the pinned tab state may have changed.
  if (tabStripModel_->IsTabPinned(modelTo) != [movedTabController pinned])
    [self tabPinnedStateChangedWithContents:contents atIndex:modelTo];

  [self layoutTabs];
}

// Called when a tab is pinned or unpinned without moving.
- (void)tabPinnedStateChangedWithContents:(content::WebContents*)contents
                                atIndex:(NSInteger)modelIndex {
  // Take closing tabs into account.
  NSInteger index = [self indexFromModelIndex:modelIndex];

  TabController* tabController =
      base::mac::ObjCCastStrict<TabController>([tabArray_ objectAtIndex:index]);

  // Don't do anything if the change was already picked up by the move event.
  if (tabStripModel_->IsTabPinned(modelIndex) == [tabController pinned])
    return;

  [tabController setPinned:tabStripModel_->IsTabPinned(modelIndex)];
  [tabController setUrl:contents->GetURL()];
  [self updateIconsForContents:contents atIndex:modelIndex];
  // If the tab is being restored and it's pinned, the pinned state is set after
  // the tab has already been rendered, so re-layout the tabstrip. In all other
  // cases, the state is set before the tab is rendered so this isn't needed.
  [self layoutTabs];
}

- (void)tabBlockedStateChangedWithContents:(content::WebContents*)contents
                                   atIndex:(NSInteger)modelIndex {
  // Take closing tabs into account.
  NSInteger index = [self indexFromModelIndex:modelIndex];

  TabController* tabController =
      base::mac::ObjCCastStrict<TabController>([tabArray_ objectAtIndex:index]);

  [tabController setBlocked:tabStripModel_->IsTabBlocked(modelIndex)];
}

- (void)tabAtIndex:(NSInteger)modelIndex needsAttention:(bool)attention {
  // Take closing tabs into account.
  NSInteger index = [self indexFromModelIndex:modelIndex];

  TabController* tabController =
      base::mac::ObjCCastStrict<TabController>([tabArray_ objectAtIndex:index]);

  [tabController setNeedsAttention:attention];
}

- (void)setFrame:(NSRect)frame ofTabView:(NSView*)view {
  NSValue* identifier = [NSValue valueWithPointer:view];
  [targetFrames_ setObject:[NSValue valueWithRect:frame]
                    forKey:identifier];
  [view setFrame:frame];
}

- (TabStripModel*)tabStripModel {
  return tabStripModel_;
}

- (NSArray*)tabViews {
  NSMutableArray* views = [NSMutableArray arrayWithCapacity:[tabArray_ count]];
  for (TabController* tab in tabArray_.get()) {
    [views addObject:[tab tabView]];
  }
  return views;
}

- (NSView*)activeTabView {
  int activeIndex = tabStripModel_->active_index();
  // Take closing tabs into account. They can't ever be selected.
  activeIndex = [self indexFromModelIndex:activeIndex];
  return [self viewAtIndex:activeIndex];
}

- (int)indexOfPlaceholder {
  // Use |tabArray_| here instead of the tab strip count in order to get the
  // correct index when there are closing tabs to the left of the placeholder.
  const int count = [tabArray_ count];

  // No placeholder, return the end of the strip.
  if (placeholderTab_ == nil)
    return count;
  BOOL isRTL = cocoa_l10n_util::ShouldDoExperimentalRTLLayout();
  double placeholderX =
      isRTL ? NSMaxX(placeholderFrame_) : placeholderFrame_.origin.x;
  int index = 0;
  int location = 0;
  while (index < count) {
    // Ignore closing tabs for simplicity. The only drawback of this is that
    // if the placeholder is placed right before one or several contiguous
    // currently closing tabs, the associated TabController will start at the
    // end of the closing tabs.
    if ([closingControllers_ containsObject:[tabArray_ objectAtIndex:index]]) {
      index++;
      continue;
    }
    NSView* curr = [self viewAtIndex:index];
    // The placeholder tab works by changing the frame of the tab being dragged
    // to be the bounds of the placeholder, so we need to skip it while we're
    // iterating, otherwise we'll end up off by one.  Note This only effects
    // dragging to the right, not to the left.
    if (curr == placeholderTab_) {
      index++;
      continue;
    }
    if (isRTL ? placeholderX >= NSMaxX([curr frame])
              : placeholderX <= NSMinX([curr frame]))
      break;
    index++;
    location++;
  }
  return location;
}

// Move the given tab at index |from| in this window to the location of the
// current placeholder.
- (void)moveTabFromIndex:(NSInteger)from {
  int toIndex = [self indexOfPlaceholder];
  // Cancel any pending tab transition.
  hoverTabSelector_->CancelTabTransition();
  tabStripModel_->MoveWebContentsAt(from, toIndex, true);
}

// Drop a given WebContents at the location of the current placeholder.
// If there is no placeholder, it will go at the end. Used when dragging from
// another window when we don't have access to the WebContents as part of our
// strip. |frame| is in the coordinate system of the tab strip view and
// represents where the user dropped the new tab so it can be animated into its
// correct location when the tab is added to the model. If the tab was pinned in
// its previous window, setting |pinned| to YES will propagate that state to the
// new window. Pinned tabs are pinned tabs; the |pinned| state is the caller's
// responsibility.
- (void)dropWebContents:(WebContents*)contents
                atIndex:(int)modelIndex
              withFrame:(NSRect)frame
            asPinnedTab:(BOOL)pinned
               activate:(BOOL)activate {
  // Mark that the new tab being created should start at |frame|. It will be
  // reset as soon as the tab has been positioned.
  droppedTabFrame_ = frame;

  // Insert it into this tab strip. We want it in the foreground and to not
  // inherit the current tab's group.
  tabStripModel_->InsertWebContentsAt(
      modelIndex, base::WrapUnique(contents),
      (activate ? TabStripModel::ADD_ACTIVE : TabStripModel::ADD_NONE) |
          (pinned ? TabStripModel::ADD_PINNED : TabStripModel::ADD_NONE));
}

// Called when the tab strip view changes size. As we only registered for
// changes on our view, we know it's only for our view. Layout w/out
// animations since they are blocked by the resize nested runloop. We need
// the views to adjust immediately. Neither the tabs nor their z-order are
// changed, so we don't need to update the subviews.
- (void)tabViewFrameChanged:(NSNotification*)info {
  [self layoutTabsWithAnimation:NO regenerateSubviews:NO];
}

// Called when the tracking areas for any given tab are updated. This allows
// the individual tabs to update their hover states correctly.
// Only generates the event if the cursor is in the tab strip.
- (void)tabUpdateTracking:(NSNotification*)notification {
  DCHECK([[notification object] isKindOfClass:[TabView class]]);
  DCHECK(mouseInside_);
  NSWindow* window = [tabStripView_ window];
  NSPoint location = [window mouseLocationOutsideOfEventStream];
  if (NSPointInRect(location, [tabStripView_ frame])) {
    NSEvent* mouseEvent = [NSEvent mouseEventWithType:NSMouseMoved
                                             location:location
                                        modifierFlags:0
                                            timestamp:0
                                         windowNumber:[window windowNumber]
                                              context:nil
                                          eventNumber:0
                                           clickCount:0
                                             pressure:0];
    [self mouseMoved:mouseEvent];
  }
}

- (BOOL)inRapidClosureMode {
  return availableResizeWidth_ != kUseFullAvailableWidth;
}

// Disable tab dragging when there are any pending animations.
- (BOOL)tabDraggingAllowed {
  return [closingControllers_ count] == 0;
}

- (void)mouseMoved:(NSEvent*)event {
  // We don't want the dragged tab to repeatedly redraw its glow unnecessarily.
  // We also want the dragged tab to keep the glow even when it slides behind
  // another tab.
  if ([dragController_ draggedTab])
    return;

  // Use hit test to figure out what view we are hovering over.
  NSView* targetView = [tabStripView_ hitTest:[event locationInWindow]];

  // Set the new tab button hover state iff the mouse is over the button.
  BOOL shouldShowHoverImage = [targetView isKindOfClass:[NewTabButton class]];
  [self setNewTabButtonHoverState:shouldShowHoverImage];

  TabView* tabView = base::mac::ObjCCast<TabView>(targetView);
  if (!tabView)
    tabView = base::mac::ObjCCast<TabView>([targetView superview]);

  if (hoveredTab_ != tabView) {
    [self setHoveredTab:tabView];
  } else {
    [hoveredTab_ mouseMoved:event];
  }
}

- (void)mouseEntered:(NSEvent*)event {
  NSTrackingArea* area = [event trackingArea];
  if ([area isEqual:trackingArea_]) {
    mouseInside_ = YES;
    [self setTabTrackingAreasEnabled:YES];
    [self mouseMoved:event];
  } else if ([area isEqual:customWindowControlsTrackingArea_]) {
    [customWindowControls_ setMouseInside:YES];
  }
}

// Called when the tracking area is in effect which means we're tracking to
// see if the user leaves the tab strip with their mouse. When they do,
// reset layout to use all available width.
- (void)mouseExited:(NSEvent*)event {
  NSTrackingArea* area = [event trackingArea];
  if ([area isEqual:trackingArea_]) {
    mouseInside_ = NO;
    [self setTabTrackingAreasEnabled:NO];
    availableResizeWidth_ = kUseFullAvailableWidth;
    [self setHoveredTab:nil];
    [self layoutTabs];
  } else if ([area isEqual:newTabTrackingArea_]) {
    // If the mouse is moved quickly enough, it is possible for the mouse to
    // leave the tabstrip without sending any mouseMoved: messages at all.
    // Since this would result in the new tab button incorrectly staying in the
    // hover state, disable the hover image on every mouse exit.
    [self setNewTabButtonHoverState:NO];
  } else if ([area isEqual:customWindowControlsTrackingArea_]) {
    [customWindowControls_ setMouseInside:NO];
  }
}

- (TabView*)hoveredTab {
  return hoveredTab_;
}

- (void)setHoveredTab:(TabView*)newHoveredTab {
  if (hoveredTab_) {
    [hoveredTab_ mouseExited:nil];
    [toolTipView_ setFrame:NSZeroRect];
  }

  hoveredTab_ = newHoveredTab;

  if (newHoveredTab) {
    [newHoveredTab mouseEntered:nil];

    // Use a transparent subview to show the hovered tab's tooltip while the
    // mouse pointer is inside the tab's custom shape.
    if (!toolTipView_)
      toolTipView_.reset([[NSView alloc] init]);
    [toolTipView_ setToolTip:[newHoveredTab toolTipText]];
    [toolTipView_ setFrame:[newHoveredTab frame]];
    if (![toolTipView_ superview]) {
      [tabStripView_ addSubview:toolTipView_
                     positioned:NSWindowAbove
                     relativeTo:dragBlockingView_];
    }
  }
}

// Enable/Disable the tracking areas for the tabs. They are only enabled
// when the mouse is in the tabstrip.
- (void)setTabTrackingAreasEnabled:(BOOL)enabled {
  NSNotificationCenter* defaultCenter = [NSNotificationCenter defaultCenter];
  for (TabController* controller in tabArray_.get()) {
    TabView* tabView = [controller tabView];
    if (enabled) {
      // Set self up to observe tabs so hover states will be correct.
      [defaultCenter addObserver:self
                        selector:@selector(tabUpdateTracking:)
                            name:NSViewDidUpdateTrackingAreasNotification
                          object:tabView];
    } else {
      [defaultCenter removeObserver:self
                               name:NSViewDidUpdateTrackingAreasNotification
                             object:tabView];
    }
    [tabView setTrackingEnabled:enabled];
  }
}

// Sets the new tab button's image based on the current hover state.  Does
// nothing if the hover state is already correct.
- (void)setNewTabButtonHoverState:(BOOL)shouldShowHover {
  if (shouldShowHover && !newTabButtonShowingHoverImage_) {
    newTabButtonShowingHoverImage_ = YES;
    [[newTabButton_ cell] setIsMouseInside:YES];
  } else if (!shouldShowHover && newTabButtonShowingHoverImage_) {
    newTabButtonShowingHoverImage_ = NO;
    [[newTabButton_ cell] setIsMouseInside:NO];
  }
}

// Adds the given subview to (the end of) the list of permanent subviews
// (specified from bottom up). These subviews will always be below the
// transitory subviews (tabs). |-regenerateSubviewList| must be called to
// effectuate the addition.
- (void)addSubviewToPermanentList:(NSView*)aView {
  if (aView)
    [permanentSubviews_ addObject:aView];
}

// Update the subviews, keeping the permanent ones (or, more correctly, putting
// in the ones listed in permanentSubviews_), and putting in the current tabs in
// the correct z-order. Any current subviews which is neither in the permanent
// list nor a (current) tab will be removed. So if you add such a subview, you
// should call |-addSubviewToPermanentList:| (or better yet, call that and then
// |-regenerateSubviewList| to actually add it).
- (void)regenerateSubviewList {
  // Remove self as an observer from all the old tabs before a new set of
  // potentially different tabs is put in place.
  [self setTabTrackingAreasEnabled:NO];

  // Subviews to put in (in bottom-to-top order), beginning with the permanent
  // ones.
  NSMutableArray* subviews = [NSMutableArray arrayWithArray:permanentSubviews_];

  NSView* activeTabView = nil;
  // Go through tabs in reverse order, since |subviews| is bottom-to-top.
  for (TabController* tab in [tabArray_ reverseObjectEnumerator]) {
    NSView* tabView = [tab view];
    if ([tab active]) {
      DCHECK(!activeTabView);
      activeTabView = tabView;
    } else {
      [subviews addObject:tabView];
    }
  }
  if (activeTabView) {
    [subviews addObject:activeTabView];
  }
  WithNoAnimation noAnimation;
  [tabStripView_ setSubviews:subviews];
  [self setTabTrackingAreasEnabled:mouseInside_];
}

// Get the index and disposition for a potential URL(s) drop given a point (in
// the |TabStripView|'s coordinates). It considers only the x-coordinate of the
// given point. If it's in the "middle" of a tab, it drops on that tab. If it's
// to the left, it inserts to the left, and similarly for the right.
- (void)droppingURLsAt:(NSPoint)point
            givesIndex:(NSInteger*)index
           disposition:(WindowOpenDisposition*)disposition
           activateTab:(BOOL)activateTab {
  // Proportion of the tab which is considered the "middle" (and causes things
  // to drop on that tab).
  const double kMiddleProportion = 0.5;
  const double kLRProportion = (1.0 - kMiddleProportion) / 2.0;
  const CGFloat kTabOverlap = [TabStripController tabOverlap];

  DCHECK(index && disposition);
  NSInteger i = 0;
  BOOL isRTL = cocoa_l10n_util::ShouldDoExperimentalRTLLayout();
  for (TabController* tab in tabArray_.get()) {
    TabView* view = base::mac::ObjCCastStrict<TabView>([tab view]);

    // Recall that |-[NSView frame]| is in its superview's coordinates, so a
    // |TabView|'s frame is in the coordinates of the |TabStripView| (which
    // matches the coordinate system of |point|).
    NSRect frame = [view frame];

    // Modify the frame to make it "unoverlapped".
    frame.origin.x += kTabOverlap / 2.0;
    frame.size.width -= kTabOverlap;
    if (frame.size.width < 1.0)
      frame.size.width = 1.0;  // try to avoid complete failure

    CGFloat rightEdge = NSMaxX(frame) - kLRProportion * frame.size.width;
    CGFloat leftEdge = frame.origin.x + kLRProportion * frame.size.width;

    // Drop in a new tab before  tab |i|?
    if (isRTL ? point.x > rightEdge : point.x < leftEdge) {
      *index = i;
      if (activateTab) {
        *disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
      } else {
        *disposition = WindowOpenDisposition::NEW_BACKGROUND_TAB;
      }
      return;
    }

    // Drop on tab |i|?
    if (isRTL ? point.x >= leftEdge : point.x <= rightEdge) {
      *index = i;
      *disposition = WindowOpenDisposition::CURRENT_TAB;
      return;
    }

    // (Dropping in a new tab to the right of tab |i| will be taken care of in
    // the next iteration.)
    i++;
  }

  // If we've made it here, we want to append a new tab to the end.
  *index = -1;
  if (activateTab) {
    *disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  } else {
    *disposition = WindowOpenDisposition::NEW_BACKGROUND_TAB;
  }
}

- (void)openURL:(GURL*)url
         inView:(NSView*)view
             at:(NSPoint)point
    activateTab:(BOOL)activateTab {
  // Security: Block JavaScript to prevent self-XSS.
  if (url->SchemeIs(url::kJavaScriptScheme))
    return;

  // Get the index and disposition.
  NSInteger index;
  WindowOpenDisposition disposition;
  [self droppingURLsAt:point
            givesIndex:&index
           disposition:&disposition
           activateTab:activateTab];

  // Either insert a new tab or open in a current tab.
  switch (disposition) {
    case WindowOpenDisposition::NEW_FOREGROUND_TAB:
    case WindowOpenDisposition::NEW_BACKGROUND_TAB: {
      base::RecordAction(UserMetricsAction("Tab_DropURLBetweenTabs"));
      NavigateParams params(browser_, *url, ui::PAGE_TRANSITION_TYPED);
      params.disposition = disposition;
      params.tabstrip_index = index;
      params.tabstrip_add_types =
          TabStripModel::ADD_ACTIVE | TabStripModel::ADD_FORCE_INDEX;
      Navigate(&params);
      break;
    }
    case WindowOpenDisposition::CURRENT_TAB: {
      base::RecordAction(UserMetricsAction("Tab_DropURLOnTab"));
      OpenURLParams params(*url, Referrer(), WindowOpenDisposition::CURRENT_TAB,
                           ui::PAGE_TRANSITION_TYPED, false);
      tabStripModel_->GetWebContentsAt(index)->OpenURL(params);
      tabStripModel_->ActivateTabAt(index, true);
      break;
    }
    default:
      NOTIMPLEMENTED();
  }
}

// (URLDropTargetController protocol)
- (void)dropURLs:(NSArray*)urls inView:(NSView*)view at:(NSPoint)point {
  DCHECK_EQ(view, tabStripView_.get());

  if ([urls count] < 1) {
    NOTREACHED();
    return;
  }

  for (NSInteger index = [urls count] - 1; index >= 0; index--) {
    // Refactor this code.
    // https://crbug.com/665261.
    GURL url = url_formatter::FixupURL(
        base::SysNSStringToUTF8([urls objectAtIndex:index]), std::string());

    // If the URL isn't valid, don't bother.
    if (!url.is_valid())
      continue;

    if (index == static_cast<NSInteger>([urls count]) - 1) {
      [self openURL:&url inView:view at:point activateTab:YES];
    } else {
      [self openURL:&url inView:view at:point activateTab:NO];
    }
  }
}

// (URLDropTargetController protocol)
- (void)dropText:(NSString*)text inView:(NSView*)view at:(NSPoint)point {
  DCHECK_EQ(view, tabStripView_.get());

  // If the input is plain text, classify the input and make the URL.
  AutocompleteMatch match;
  AutocompleteClassifierFactory::GetForProfile(browser_->profile())->Classify(
      base::SysNSStringToUTF16(text), false, false,
      metrics::OmniboxEventProto::BLANK, &match, NULL);
  GURL url(match.destination_url);

  [self openURL:&url inView:view at:point activateTab:YES];
}

// (URLDropTargetController protocol)
- (void)indicateDropURLsInView:(NSView*)view at:(NSPoint)point {
  DCHECK_EQ(view, tabStripView_.get());

  // The minimum y-coordinate at which one should consider place the arrow.
  const CGFloat arrowBaseY = 25;
  const CGFloat kTabOverlap = [TabStripController tabOverlap];

  NSInteger index;
  WindowOpenDisposition disposition;
  [self droppingURLsAt:point
            givesIndex:&index
           disposition:&disposition
           activateTab:YES];

  NSPoint arrowPos = NSMakePoint(0, arrowBaseY);
  if (index == -1) {
    // Append a tab at the end.
    DCHECK(disposition == WindowOpenDisposition::NEW_FOREGROUND_TAB);
    NSInteger lastIndex = [tabArray_ count] - 1;
    NSRect overRect = [[[tabArray_ objectAtIndex:lastIndex] view] frame];
    if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout()) {
      arrowPos.x = NSMinX(overRect) + kTabOverlap / 2.0;
    } else {
      arrowPos.x = NSMaxX(overRect) - kTabOverlap / 2.0;
    }
  } else {
    NSRect overRect = [[[tabArray_ objectAtIndex:index] view] frame];
    switch (disposition) {
      case WindowOpenDisposition::NEW_FOREGROUND_TAB:
        // Insert tab (before the given tab).
        if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout()) {
          arrowPos.x = NSMaxX(overRect) - kTabOverlap / 2.0;
        } else {
          arrowPos.x = NSMinX(overRect) + kTabOverlap / 2.0;
        }
        break;
      case WindowOpenDisposition::CURRENT_TAB:
        // Overwrite the given tab.
        arrowPos.x = overRect.origin.x + overRect.size.width / 2.0;
        break;
      default:
        NOTREACHED();
    }
  }

  [tabStripView_ setDropArrowPosition:arrowPos];
  [tabStripView_ setDropArrowShown:YES];
  [tabStripView_ setNeedsDisplay:YES];

  // Perform a delayed tab transition if hovering directly over a tab.
  if (index != -1 && disposition == WindowOpenDisposition::CURRENT_TAB) {
    NSInteger modelIndex = [self modelIndexFromIndex:index];
    // Only start the transition if it has a valid model index (i.e. it's not
    // in the middle of closing).
    if (modelIndex != NSNotFound) {
      hoverTabSelector_->StartTabTransition(modelIndex);
      return;
    }
  }
  // If a tab transition was not started, cancel the pending one.
  hoverTabSelector_->CancelTabTransition();
}

// (URLDropTargetController protocol)
- (void)hideDropURLsIndicatorInView:(NSView*)view {
  DCHECK_EQ(view, tabStripView_.get());

  // Cancel any pending tab transition.
  hoverTabSelector_->CancelTabTransition();

  if ([tabStripView_ dropArrowShown]) {
    [tabStripView_ setDropArrowShown:NO];
    [tabStripView_ setNeedsDisplay:YES];
  }
}

// (URLDropTargetController protocol)
- (BOOL)isUnsupportedDropData:(id<NSDraggingInfo>)info {
  return drag_util::IsUnsupportedDropData(browser_->profile(), info);
}

- (TabContentsController*)activeTabContentsController {
  int modelIndex = tabStripModel_->active_index();
  if (modelIndex < 0)
    return nil;
  NSInteger index = [self indexFromModelIndex:modelIndex];
  if (index < 0 ||
      index >= (NSInteger)[tabContentsArray_ count])
    return nil;
  return [tabContentsArray_ objectAtIndex:index];
}

- (void)addCustomWindowControls {
  BOOL shouldFlipWindowControls =
      cocoa_l10n_util::ShouldFlipWindowControlsInRTL();
  if (!customWindowControls_) {
    // Make the container view.
    CGFloat height = NSHeight([tabStripView_ frame]);
    CGFloat width = [self leadingIndentForControls];
    if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout() &&
        !shouldFlipWindowControls)
      // The trailing indent is correct in this case, since the controls should
      // stay on the left.
      width = [self trailingIndentForControls];
    CGFloat xOrigin =
        shouldFlipWindowControls ? NSWidth([tabStripView_ frame]) - width : 0;
    NSRect frame = NSMakeRect(xOrigin, 0, width, height);
    customWindowControls_.reset(
        [[CustomWindowControlsView alloc] initWithFrame:frame]);
    [customWindowControls_
        setAutoresizingMask:shouldFlipWindowControls
                                ? NSViewMinXMargin | NSViewHeightSizable
                                : NSViewMaxXMargin | NSViewHeightSizable];

    // Add the traffic light buttons. The horizontal layout was determined by
    // manual inspection on Yosemite.
    CGFloat closeButtonX = 11;
    CGFloat pinnedButtonX = 31;
    CGFloat zoomButtonX = 51;
    if (shouldFlipWindowControls)
      std::swap(closeButtonX, zoomButtonX);

    NSUInteger styleMask = [[tabStripView_ window] styleMask];
    NSButton* closeButton = [NSWindow standardWindowButton:NSWindowCloseButton
                                              forStyleMask:styleMask];

    // Vertically center the buttons in the tab strip.
    CGFloat buttonY = floor((height - NSHeight([closeButton bounds])) / 2);
    [closeButton setFrameOrigin:NSMakePoint(closeButtonX, buttonY)];
    [customWindowControls_ addSubview:closeButton];

    NSButton* miniaturizeButton =
        [NSWindow standardWindowButton:NSWindowMiniaturizeButton
                          forStyleMask:styleMask];
    [miniaturizeButton setFrameOrigin:NSMakePoint(pinnedButtonX, buttonY)];
    [miniaturizeButton setEnabled:NO];
    [customWindowControls_ addSubview:miniaturizeButton];

    NSButton* zoomButton =
        [NSWindow standardWindowButton:NSWindowZoomButton
                          forStyleMask:styleMask];
    [customWindowControls_ addSubview:zoomButton];
    [zoomButton setFrameOrigin:NSMakePoint(zoomButtonX, buttonY)];

    customWindowControlsTrackingArea_.reset([[CrTrackingArea alloc]
        initWithRect:[customWindowControls_ bounds]
             options:(NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways)
               owner:self
            userInfo:nil]);
    [customWindowControls_
        addTrackingArea:customWindowControlsTrackingArea_.get()];
  }
  if (shouldFlipWindowControls &&
      NSMaxX([customWindowControls_ frame]) != NSMaxX([tabStripView_ frame])) {
    NSRect frame = [customWindowControls_ frame];
    frame.origin.x =
        NSMaxX([tabStripView_ frame]) - [self leadingIndentForControls];
    [customWindowControls_ setFrame:frame];
  }
  if (![permanentSubviews_ containsObject:customWindowControls_]) {
    [self addSubviewToPermanentList:customWindowControls_];
    [self regenerateSubviewList];
  }
}

- (void)removeCustomWindowControls {
  if (customWindowControls_)
    [permanentSubviews_ removeObject:customWindowControls_];
  [self regenerateSubviewList];
  [customWindowControls_ setMouseInside:NO];
}

// Gets the tab and the alert state to check whether the window
// alert state should be updated or not. If the tab alert state is
// AUDIO_PLAYING, the window alert state should be set to AUDIO_PLAYING.
// If the tab alert state is AUDIO_MUTING, this method would check if the
// window has no other tab with state AUDIO_PLAYING, then the window
// alert state will be set to AUDIO_MUTING. If the tab alert state is NONE,
// this method checks if the window has no playing or muting tab, then window
// alert state will be set as NONE.
- (void)updateWindowAlertState:(TabAlertState)alertState
                forWebContents:(content::WebContents*)selected {
  NSWindow* window = [tabStripView_ window];
  BrowserWindowController* windowController =
      [BrowserWindowController browserWindowControllerForWindow:window];
  if (alertState == TabAlertState::NONE) {
    if (![self doesAnyOtherWebContents:selected
                        haveAlertState:TabAlertState::AUDIO_PLAYING] &&
        ![self doesAnyOtherWebContents:selected
                        haveAlertState:TabAlertState::AUDIO_MUTING]) {
      [windowController setAlertState:TabAlertState::NONE];
    } else if ([self doesAnyOtherWebContents:selected
                              haveAlertState:TabAlertState::AUDIO_MUTING]) {
      [windowController setAlertState:TabAlertState::AUDIO_MUTING];
    }
  } else if (alertState == TabAlertState::AUDIO_MUTING) {
    if (![self doesAnyOtherWebContents:selected
                        haveAlertState:TabAlertState::AUDIO_PLAYING]) {
      [windowController setAlertState:TabAlertState::AUDIO_MUTING];
    }
  } else {
    [windowController setAlertState:alertState];
  }
}

// Checks if tabs (excluding selected) has alert state equals to the second
// parameter. It returns YES when it finds the first tab with the criterion.
- (BOOL)doesAnyOtherWebContents:(content::WebContents*)selected
                 haveAlertState:(TabAlertState)state {
  const int existingTabCount = tabStripModel_->count();
  for (int i = 0; i < existingTabCount; ++i) {
    content::WebContents* currentContents = tabStripModel_->GetWebContentsAt(i);
    if (selected == currentContents)
      continue;
    TabAlertState currentAlertStateForContents =
        [self alertStateForContents:currentContents];
    if (currentAlertStateForContents == state)
      return YES;
  }
  return NO;
}

- (TabAlertState)alertStateForContents:(content::WebContents*)contents {
  return chrome::GetTabAlertStateForContents(contents);
}

- (void)themeDidChangeNotification:(NSNotification*)notification {
  [newTabButton_ setImages];
  for (int i = 0; i < tabStripModel_->count(); i++) {
    [self updateIconsForContents:tabStripModel_->GetWebContentsAt(i) atIndex:i];
  }
}

- (void)setVisualEffectsDisabledForFullscreen:(BOOL)fullscreen {
  [tabStripView_ setVisualEffectsDisabledForFullscreen:fullscreen];
}

@end
