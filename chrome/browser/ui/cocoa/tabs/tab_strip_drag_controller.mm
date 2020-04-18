// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/tabs/tab_strip_drag_controller.h"

#include <Carbon/Carbon.h>

#include "base/mac/scoped_cftyperef.h"
#import "base/mac/sdk_forward_declarations.h"
#import "chrome/browser/ui/cocoa/tabs/tab_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_controller_target.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_view.h"
#import "chrome/browser/ui/cocoa/tabs/tab_view.h"
#import "chrome/browser/ui/cocoa/tabs/tab_window_controller.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#include "ui/gfx/mac/scoped_cocoa_disable_screen_updates.h"

constexpr CGFloat kHorizTearDistance = 10.0;  // Using the same value as Views.
constexpr CGFloat kVertTearDistance = 20.0;
constexpr CGFloat kLargeVertTearDistance = 100.0;
constexpr NSTimeInterval kTearDuration = 0.333;

// Returns whether |screenPoint| is inside the bounds of |view|.
static BOOL PointIsInsideView(NSPoint screenPoint, NSView* view) {
  if ([view window] == nil)
    return NO;
  NSPoint windowPoint =
      ui::ConvertPointFromScreenToWindow([view window], screenPoint);
  NSPoint viewPoint = [view convertPoint:windowPoint fromView:nil];
  return [view mouse:viewPoint inRect:[view bounds]];
}

@interface TabStripDragController (Private)
- (NSArray*)selectedTabViews;
- (BOOL)canDragSelectedTabs;
- (void)resetDragControllers;
- (NSArray*)dropTargetsForController:(TabWindowController*)dragController;
- (void)setWindowBackgroundVisibility:(BOOL)shouldBeVisible;
- (void)endDrag:(NSEvent*)event;
- (void)continueDrag:(NSEvent*)event;
@end

////////////////////////////////////////////////////////////////////////////////

@implementation TabStripDragController

@synthesize draggedTab = draggedTab_;

- (id)initWithTabStripController:(TabStripController*)controller {
  if ((self = [super init])) {
    tabStrip_ = controller;
  }
  return self;
}

- (void)dealloc {
  [NSObject cancelPreviousPerformRequestsWithTarget:self];
  [super dealloc];
}

- (void)maybeStartDrag:(NSEvent*)theEvent forTab:(TabController*)tab {
  [self resetDragControllers];

  // Resolve overlay back to original window.
  sourceWindow_ = [[tab view] window];
  if ([sourceWindow_ isKindOfClass:[NSPanel class]]) {
    sourceWindow_ = [sourceWindow_ parentWindow];
  }

  sourceWindowFrame_ = [sourceWindow_ frame];
  sourceTabFrame_ = [[tab view] frame];
  sourceController_ =
      [TabWindowController tabWindowControllerForWindow:sourceWindow_];
  draggedTab_ = tab;
  tabWasDragged_ = NO;
  tearTime_ = 0.0;
  draggingWithinTabStrip_ = YES;
  chromeIsVisible_ = NO;

  moveWindowOnDrag_ = ![self canDragSelectedTabs] ||
                      ![sourceController_ tabDraggingAllowed];
  // If we are dragging a tab, a window with a single tab should immediately
  // snap off and not drag within the tab strip.
  if (!moveWindowOnDrag_)
    draggingWithinTabStrip_ = [sourceController_ numberOfTabs] > 1;

  dragOrigin_ = [NSEvent mouseLocation];

  // When spinning the event loop, a tab can get detached, which could lead to
  // our own destruction. Keep ourselves around while spinning the loop as well
  // as the tab controller being dragged.
  base::scoped_nsobject<TabStripDragController> keepAlive([self retain]);
  base::scoped_nsobject<TabController> keepAliveTab([tab retain]);

  // Because we move views between windows, we need to handle the event loop
  // ourselves. Ideally we should use the standard event loop.
  while (1) {
    const NSUInteger mask =
        NSLeftMouseUpMask | NSLeftMouseDraggedMask | NSKeyUpMask;
    theEvent =
        [NSApp nextEventMatchingMask:mask
                           untilDate:[NSDate distantFuture]
                              inMode:NSDefaultRunLoopMode
                             dequeue:YES];

    // Ensure that any window changes that happen while handling this event
    // appear atomically.
    gfx::ScopedCocoaDisableScreenUpdates disabler;

    NSEventType type = [theEvent type];
    if (type == NSKeyUp) {
      if ([theEvent keyCode] == kVK_Escape) {
        // Cancel the drag and restore the previous state.
        if (draggingWithinTabStrip_) {
          // Simply pretend the tab wasn't dragged (far enough).
          tabWasDragged_ = NO;
        } else {
          [targetController_ removePlaceholder];
          [[sourceController_ window] makeMainWindow];
          if ([sourceController_ numberOfTabs] < 2) {
            // Revert to a single-tab window.
            targetController_ = nil;
          } else {
            // Change the target to the source controller.
            targetController_ = sourceController_;
            [targetController_ insertPlaceholderForTab:[tab tabView]
                                                 frame:sourceTabFrame_];
          }
        }
        // Simply end the drag at this point.
        [self endDrag:theEvent];
        break;
      }
    } else if (type == NSLeftMouseDragged) {
      [self continueDrag:theEvent];
    } else if (type == NSLeftMouseUp) {
      [tab selectTab:self];
      [self endDrag:theEvent];
      break;
    } else {
      // TODO(viettrungluu): [crbug.com/23830] We can receive right-mouse-ups
      // (and maybe even others?) for reasons I don't understand. So we
      // explicitly check for both events we're expecting, and log others. We
      // should figure out what's going on.
      LOG(WARNING) << "Spurious event received of type " << type << ".";
    }
  }
}

- (void)continueDrag:(NSEvent*)theEvent {
  CHECK(draggedTab_);

  // Cancel any delayed -continueDrag: requests that may still be pending.
  [NSObject cancelPreviousPerformRequestsWithTarget:self];

  // Special-case this to keep the logic below simpler.
  if (moveWindowOnDrag_) {
    if ([sourceController_ windowMovementAllowed]) {
      NSPoint thisPoint = [NSEvent mouseLocation];
      NSPoint origin = sourceWindowFrame_.origin;
      origin.x += (thisPoint.x - dragOrigin_.x);
      origin.y += (thisPoint.y - dragOrigin_.y);
      [sourceWindow_ setFrameOrigin:NSMakePoint(origin.x, origin.y)];
    }  // else do nothing.
    return;
  }

  if (draggingWithinTabStrip_) {
    NSPoint thisPoint = [NSEvent mouseLocation];
    CGFloat horizOffset = thisPoint.x - dragOrigin_.x;
    CGFloat vertOffset = thisPoint.y - dragOrigin_.y;
    BOOL stillVisible = YES;

    // If the tab hasn't been torn out of the vertical dead zone, and has never
    // been torn out of the horizontal dead zone, return. This prevents the tab
    // from sticking if it's dragged back near its original position.
    if (fabs(horizOffset) <= kHorizTearDistance && !outOfTabHorizDeadZone_ &&
        fabs(vertOffset) <= kVertTearDistance)
      return;
    if (fabs(horizOffset) > kHorizTearDistance)
      outOfTabHorizDeadZone_ = YES;

    // If the tab is pulled out of either dead zone, set tabWasDragged_ to YES
    // and call insertPlaceholderForTab:frame:.
    if (outOfTabHorizDeadZone_ || fabs(vertOffset) > kVertTearDistance) {
      tabWasDragged_ = YES;
      TabView* tabView = [draggedTab_ tabView];
      [sourceController_
          insertPlaceholderForTab:tabView
                            frame:[tabView centerScanRect:NSOffsetRect(
                                                              sourceTabFrame_,
                                                              horizOffset, 0)]];
    }

    // Check if the tab has been pulled out of the tab strip. Emulate the
    // behavior of native tab dragging, where the tab will:
    //  - Be easy to tear off if it's still within the horizontal dead zone.
    //  - Be harder to tear off if it's left the horizontal dead zone.
    // Note the |tabTornOff| calculation assumes the following:
    static_assert(
        kLargeVertTearDistance >= kVertTearDistance,
        "Check |outOfTabHorizDeadZone_|'s value if this is no longer true.");
    BOOL tabTornOff =
        (!outOfTabHorizDeadZone_ && fabs(vertOffset) > kVertTearDistance) ||
        fabs(vertOffset) > kLargeVertTearDistance;
    // Whether the tab remains inside the bounds of the window.
    stillVisible = [sourceController_ isTabFullyVisible:[draggedTab_ tabView]];

    if ([sourceController_ tabTearingAllowed] &&
        (tabTornOff || !stillVisible)) {
      draggingWithinTabStrip_ = NO;
      // When you finally leave the strip, we treat that as the origin.
      dragOrigin_.x = thisPoint.x;
      // The above call to insertPlaceholderForTab:frame: updates
      // |draggedTab_|'s frame, but does so using an animator, which means the
      // new frame gets set only after the next turn of the run loop. As a
      // result, at this point in the code the updated frame is only pending,
      // which means that when the tab gets torn off into its own window, its
      // location in that new window will be based on its current, un-updated
      // location. As a result, the tab will appear at an unexpected location
      // in the new window relative to the mouse. The difference between the
      // expected and actual locations will be more-pronounced the faster you
      // drag the mouse horizontally when tearing it off from its current
      // window. To fix, explicitly set the tab's new location so that it's
      // correct at tearoff time. See http://crbug.com/541674 .
      NSRect newTabFrame = [[draggedTab_ tabView] frame];
      newTabFrame.origin.x = trunc(sourceTabFrame_.origin.x + horizOffset);

      // Ensure that the tab won't extend beyond the right edge of the tab area
      // in the tab strip.
      CGFloat tabWidthBeyondRightEdge =
          MAX(NSMaxX(newTabFrame) - [tabStrip_ tabAreaRightEdge], 0.0);
      if (tabWidthBeyondRightEdge) {
        // Adjust the tab's x-origin so that it just touches the right edge of
        // the tab area.
        newTabFrame.origin.x -= tabWidthBeyondRightEdge;
        // Offset the new window's drag location so that the tab will still be
        // positioned correctly beneath the mouse (being careful to convert the
        // view frame offset to screen coordinates).
        NSWindow* tabViewWindow = [[draggedTab_ tabView] window];
        horizDragOffset_ = [tabViewWindow convertRectToScreen:
            NSMakeRect(0.0, 0.0, tabWidthBeyondRightEdge, 1.0)].size.width;
      }

      [[draggedTab_ tabView] setFrameOrigin:newTabFrame.origin];
    }

    // Else, still dragging within the tab strip, wait for the next drag
    // event.
    return;
  }

  NSPoint thisPoint = [NSEvent mouseLocation];

  // Iterate over possible targets checking for the one the mouse is in.
  // If the tab is just in the frame, bring the window forward to make it
  // easier to drop something there. If it's in the tab strip, set the new
  // target so that it pops into that window. We can't cache this because we
  // need the z-order to be correct.
  NSArray* targets = [self dropTargetsForController:draggedController_];
  TabWindowController* newTarget = nil;
  for (TabWindowController* target in targets) {
    NSRect windowFrame = [[target window] frame];
    if (NSPointInRect(thisPoint, windowFrame)) {
      [[target window] orderFront:self];
      if (PointIsInsideView(thisPoint, [target tabStripView])) {
        newTarget = target;
      }
      break;
    }
  }

  // If we're now targeting a new window, re-layout the tabs in the old
  // target and reset how long we've been hovering over this new one.
  if (targetController_ != newTarget) {
    [targetController_ removePlaceholder];
    targetController_ = newTarget;
    if (!newTarget) {
      tearTime_ = [NSDate timeIntervalSinceReferenceDate];
      tearOrigin_ = [dragWindow_ frame].origin;
    }
  }

  // Create or identify the dragged controller.
  if (!draggedController_) {
    // Detach from the current window and put it in a new window. If there are
    // no more tabs remaining after detaching, the source window is about to
    // go away (it's been autoreleased) so we need to ensure we don't reference
    // it any more. In that case the new controller becomes our source
    // controller.
    NSArray* tabs = [self selectedTabViews];
    draggedController_ =
        [sourceController_ detachTabsToNewWindow:tabs
                                      draggedTab:[draggedTab_ tabView]];

    dragWindow_ = [draggedController_ window];
    [dragWindow_ setAlphaValue:0.0];
    if ([sourceController_ hasLiveTabs]) {
      if (PointIsInsideView(thisPoint, [sourceController_ tabStripView])) {
        // We don't want to remove the source window's placeholder here because
        // the new tab button may briefly flash in and out if we remove and add
        // back the placeholder.
        // Instead, we will remove the placeholder later when the target window
        // actually changes.
        targetController_ = sourceController_;
      } else {
        [sourceController_ removePlaceholder];
      }
    } else {
      [sourceController_ removePlaceholder];
      sourceController_ = draggedController_;
      sourceWindow_ = dragWindow_;
    }

    // Disable window animation before calling |orderFront:| when detaching
    // to a new window.
    NSWindowAnimationBehavior savedAnimationBehavior =
        NSWindowAnimationBehaviorDefault;
    bool didSaveAnimationBehavior = false;
    if ([dragWindow_ respondsToSelector:@selector(animationBehavior)] &&
        [dragWindow_ respondsToSelector:@selector(setAnimationBehavior:)]) {
      didSaveAnimationBehavior = true;
      savedAnimationBehavior = [dragWindow_ animationBehavior];
      [dragWindow_ setAnimationBehavior:NSWindowAnimationBehaviorNone];
    }

    // If dragging the tab only moves the current window, do not show overlay
    // so that sheets stay on top of the window.
    // Bring the target window to the front and make sure it has a border.
    [dragWindow_ setLevel:NSFloatingWindowLevel];
    [dragWindow_ setHasShadow:YES];
    [dragWindow_ orderFront:nil];
    [dragWindow_ makeMainWindow];
    [draggedController_ showOverlay];
    dragOverlay_ = [draggedController_ overlayWindow];
    [dragOverlay_ setHasShadow:YES];
    // Force the new tab button to be hidden. We'll reset it on mouse up.
    [draggedController_ showNewTabButton:NO];
    tearTime_ = [NSDate timeIntervalSinceReferenceDate];
    tearOrigin_ = sourceWindowFrame_.origin;

    // Restore window animation behavior.
    if (didSaveAnimationBehavior)
      [dragWindow_ setAnimationBehavior:savedAnimationBehavior];
  }

  // TODO(pinkerton): http://crbug.com/25682 demonstrates a way to get here by
  // some weird circumstance that doesn't first go through mouseDown:. We
  // really shouldn't go any farther.
  if (!draggedController_ || !sourceController_)
    return;

  // When the user first tears off the window, we want slide the window to
  // the current mouse location (to reduce the jarring appearance). We do this
  // by calling ourselves back with additional -continueDrag: calls (not actual
  // events). |tearProgress| is a normalized measure of how far through this
  // tear "animation" (of length kTearDuration) we are and has values [0..1].
  // We use sqrt() so the animation is non-linear (slow down near the end
  // point).
  NSTimeInterval tearProgress =
      [NSDate timeIntervalSinceReferenceDate] - tearTime_;
  tearProgress /= kTearDuration;  // Normalize.
  tearProgress = sqrtf(MAX(MIN(tearProgress, 1.0), 0.0));

  // Move the dragged window to the right place on the screen.
  // TODO(spqchan): Write a test to check if the window is at the right place.
  // See http://crbug.com/687647.
  NSPoint origin = sourceWindowFrame_.origin;
  origin.x += (thisPoint.x - dragOrigin_.x);
  origin.y += (thisPoint.y - dragOrigin_.y);
  if ([sourceController_ isInAnyFullscreenMode]) {
    origin.y +=
        [sourceController_ menubarOffset] + [sourceController_ menubarHeight];
  }

  if (tearProgress < 1) {
    // If the tear animation is not complete, call back to ourself with the
    // same event to animate even if the mouse isn't moving. We need to make
    // sure these get cancelled in -endDrag:.
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    [self performSelector:@selector(continueDrag:)
               withObject:theEvent
               afterDelay:1.0f/30.0f];

    // Set the current window origin based on how far we've progressed through
    // the tear animation.
    origin.x = (1 - tearProgress) * tearOrigin_.x + tearProgress * origin.x;
    origin.y = (1 - tearProgress) * tearOrigin_.y + tearProgress * origin.y;
  }

  if (targetController_) {
    // In order to "snap" two windows of different sizes together at their
    // toolbar, we can't just use the origin of the target frame. We also have
    // to take into consideration the difference in height.
    NSRect targetFrame = [[targetController_ window] frame];
    NSRect sourceFrame = [dragWindow_ frame];
    origin.y = NSMinY(targetFrame) + [targetController_ menubarOffset] +
               (NSHeight(targetFrame) - NSHeight(sourceFrame));
  }
  [dragWindow_ setFrameOrigin:
      NSMakePoint(origin.x + horizDragOffset_, origin.y)];

  // If we're not hovering over any window, make the window fully
  // opaque. Otherwise, find where the tab might be dropped and insert
  // a placeholder so it appears like it's part of that window.
  if (targetController_) {
    if (![[targetController_ window] isKeyWindow])
      [[targetController_ window] orderFront:nil];

    // Compute where placeholder should go and insert it into the
    // destination tab strip.
    // The placeholder frame is the rect that contains all dragged tabs.
    NSRect tabFrame = NSZeroRect;
    for (NSView* tabView in [draggedController_ tabViews]) {
      tabFrame = NSUnionRect(tabFrame, [tabView frame]);
    }
    tabFrame = [dragWindow_ convertRectToScreen:tabFrame];
    tabFrame = [[targetController_ window] convertRectFromScreen:tabFrame];
    tabFrame = [[targetController_ tabStripView]
                convertRect:tabFrame fromView:nil];
    [targetController_ insertPlaceholderForTab:[draggedTab_ tabView]
                                         frame:tabFrame];
    [targetController_ layoutTabs];
  } else {
    [dragWindow_ makeKeyAndOrderFront:nil];
  }

  // Adjust the visibility of the window background. If there is a drop target,
  // we want to hide the window background so the tab stands out for
  // positioning. If not, we want to show it so it looks like a new window will
  // be realized.
  BOOL chromeShouldBeVisible = targetController_ == nil;
  [self setWindowBackgroundVisibility:chromeShouldBeVisible];
}

- (void)endDrag:(NSEvent*)event {
  // Cancel any delayed -continueDrag: requests that may still be pending.
  [NSObject cancelPreviousPerformRequestsWithTarget:self];
  outOfTabHorizDeadZone_ = NO;

  // Special-case this to keep the logic below simpler.
  if (moveWindowOnDrag_) {
    [self resetDragControllers];
    return;
  }

  // TODO(pinkerton): http://crbug.com/25682 demonstrates a way to get here by
  // some weird circumstance that doesn't first go through mouseDown:. We
  // really shouldn't go any farther.
  if (!sourceController_)
    return;

  // We are now free to re-display the new tab button in the window we're
  // dragging. It will show when the next call to -layoutTabs (which happens
  // indirectly by several of the calls below, such as removing the
  // placeholder).
  [draggedController_ showNewTabButton:YES];

  if (draggingWithinTabStrip_) {
    if (tabWasDragged_) {
      // Move tab to new location.
      DCHECK([sourceController_ numberOfTabs]);
      TabWindowController* dropController = sourceController_;
      [dropController moveTabViews:@[ [dropController activeTabView] ]
                    fromController:nil];
    }
  } else if (targetController_) {
    // Move between windows. If |targetController_| is nil, we're not dropping
    // into any existing window.
    [targetController_ moveTabViews:[draggedController_ tabViews]
                     fromController:draggedController_];
    // Force redraw to avoid flashes of old content before returning to event
    // loop.
    [[targetController_ window] display];
    [[targetController_ nsWindowController] showWindow:nil];
    [draggedController_ removeOverlay];
  } else {
    // Only move the window around on screen. Make sure it's set back to
    // normal state (fully opaque, has shadow, has key, in fullscreen if
    // appropriate, etc).
    [draggedController_
        detachedWindowEnterFullscreenIfNeeded:sourceController_];

    [draggedController_ removeOverlay];
    // Don't want to re-show the window if it was closed during the drag.
    if ([dragWindow_ isVisible]) {
      [dragWindow_ setAlphaValue:1.0];
      [dragOverlay_ setHasShadow:NO];
      [dragWindow_ setHasShadow:YES];
      [dragWindow_ makeKeyAndOrderFront:nil];
    }
    [[draggedController_ window] setLevel:NSNormalWindowLevel];
    [draggedController_ removePlaceholder];
  }
  [sourceController_ removePlaceholder];
  chromeIsVisible_ = YES;

  [self resetDragControllers];
}

// Private /////////////////////////////////////////////////////////////////////

- (NSArray*)selectedTabViews {
  return [draggedTab_ selected] ? [tabStrip_ selectedViews]
                                : @[ [draggedTab_ tabView] ];
}

- (BOOL)canDragSelectedTabs {
  NSArray* tabs = [self selectedTabViews];

  // If there's more than one potential window to be a drop target, we want to
  // treat a drag of a tab just like dragging around a tab that's already
  // detached. Note that unit tests might have |-numberOfTabs| reporting zero
  // since the model won't be fully hooked up. We need to be prepared for that
  // and not send them into the "magnetic" codepath.
  NSArray* targets = [self dropTargetsForController:sourceController_];
  if (![targets count] && [sourceController_ numberOfTabs] - [tabs count] == 0)
    return NO;  // I.e. ignore dragging *all* tabs in the last Browser window.

  for (TabView* tabView in tabs) {
    if ([tabView isClosing])
      return NO;
  }

  TabWindowController* controller =
      [TabWindowController tabWindowControllerForWindow:sourceWindow_];
  if (controller) {
    for (TabView* tabView in tabs) {
      if (![controller isTabDraggable:tabView])
        return NO;
    }
  }
  return YES;
}

// Call to clear out transient weak references we hold during drags.
- (void)resetDragControllers {
  draggedTab_ = nil;
  draggedController_ = nil;
  dragWindow_ = nil;
  dragOverlay_ = nil;
  sourceController_ = nil;
  sourceWindow_ = nil;
  targetController_ = nil;
  horizDragOffset_ = 0.0;
}

// Returns an array of controllers that could be a drop target, ordered front to
// back. It has to be of the appropriate class, and visible (obviously). Note
// that the window cannot be a target for itself.
- (NSArray*)dropTargetsForController:(TabWindowController*)dragController {
  NSMutableArray* targets = [NSMutableArray array];
  NSWindow* dragWindow = [dragController window];
  for (NSWindow* window in [NSApp orderedWindows]) {
    if (window == dragWindow) continue;
    if (![window isVisible]) continue;
    // Skip windows on the wrong space.
    if (![window isOnActiveSpace])
      continue;
    TabWindowController* controller =
        [TabWindowController tabWindowControllerForWindow:window];
    // +tabWindowControllerForWindow: moves from a window to its parent
    // looking for a TabWindowController. This means in some cases
    // window != dragWindow but the returned controller == dragController.
    if (dragController == controller)
      continue;
    if ([controller canReceiveFrom:dragController])
      [targets addObject:controller];
  }
  return targets;
}

// Sets whether the window background should be visible or invisible when
// dragging a tab. The background should be invisible when the mouse is over a
// potential drop target for the tab (the tab strip). It should be visible when
// there's no drop target so the window looks more fully realized and ready to
// become a stand-alone window.
- (void)setWindowBackgroundVisibility:(BOOL)shouldBeVisible {
  if (chromeIsVisible_ == shouldBeVisible)
    return;

  // There appears to be a race-condition in CoreAnimation where if we use
  // animators to set the alpha values, we can't guarantee that we cancel them.
  // This has the side effect of sometimes leaving the dragged window
  // translucent or invisible. As a result, don't animate the alpha change.
  [[draggedController_ overlayWindow] setAlphaValue:1.0];
  if (targetController_) {
    [dragWindow_ setAlphaValue:0.0];
    [[draggedController_ overlayWindow] setHasShadow:YES];
    [[targetController_ window] makeMainWindow];
  } else {
    [dragWindow_ setAlphaValue:0.5];
    [[draggedController_ overlayWindow] setHasShadow:NO];
    [[draggedController_ window] makeMainWindow];
  }
  chromeIsVisible_ = shouldBeVisible;
}

@end
