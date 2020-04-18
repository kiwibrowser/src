// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TABS_TAB_STRIP_DRAG_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_TABS_TAB_STRIP_DRAG_CONTROLLER_H_

#import <Cocoa/Cocoa.h>
#include <map>

@class TabController;
@class TabStripController;
@class TabWindowController;

////////////////////////////////////////////////////////////////////////////////

// This protocol is used to carry mouse events to the TabStripDragController,
// which manages the logic for actually dragging tabs.
@protocol TabDraggingEventTarget

// Initiates a dragging session with a mouseDown event. The tab controller
// passed here is the one used for the rest of the dragging session.
- (void)maybeStartDrag:(NSEvent*)event forTab:(TabController*)tab;

@end

////////////////////////////////////////////////////////////////////////////////

// This controller is owned by the TabStripController and is used to delegate
// all the logic for tab dragging from the TabView's events.
@interface TabStripDragController : NSObject<TabDraggingEventTarget> {
 @private
  TabStripController* tabStrip_;  // Weak; owns this.

  // These are released on mouseUp:
  BOOL moveWindowOnDrag_;  // Set if the only tab of a window is dragged.
  BOOL tabWasDragged_;  // Has the tab been dragged?
  BOOL outOfTabHorizDeadZone_;   // Moved out of its horizontal dead zone?
  BOOL draggingWithinTabStrip_;  // Did drag stay in the current tab strip?
  BOOL chromeIsVisible_;

  NSTimeInterval tearTime_;  // Time since tear happened
  NSPoint tearOrigin_;  // Origin of the tear rect
  NSPoint dragOrigin_;  // Origin point of the drag

  TabWindowController* sourceController_;  // weak. controller starting the drag
  NSWindow* sourceWindow_;  // Weak. The window starting the drag.
  NSRect sourceWindowFrame_;
  NSRect sourceTabFrame_;

  TabController* draggedTab_;  // Weak. The tab controller being dragged.

  TabWindowController* draggedController_;  // Weak. Controller being dragged.
  NSWindow* dragWindow_;  // Weak. The window being dragged
  NSWindow* dragOverlay_;  // Weak. The overlay being dragged

  TabWindowController* targetController_;  // weak. Controller being targeted

  CGFloat horizDragOffset_;
}

// The tab being dragged, or nil if not dragging a tab.
@property(readonly) TabController* draggedTab;

// Designated initializer.
- (id)initWithTabStripController:(TabStripController*)controller;

// TabDraggingEventTarget methods are also implemented.

@end

#endif  // CHROME_BROWSER_UI_COCOA_TABS_TAB_STRIP_DRAG_CONTROLLER_H_
