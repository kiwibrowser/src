// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_APP_MENU_MENU_TRACKED_BUTTON_H_
#define CHROME_BROWSER_UI_COCOA_APP_MENU_MENU_TRACKED_BUTTON_H_

#import <Cocoa/Cocoa.h>

// A MenuTrackedButton is meant to be used whenever a button is placed inside
// the custom view of an NSMenuItem. If the user opens the menu in a non-sticky
// fashion (i.e. clicks, holds, and drags) and then releases the mouse over
// a MenuTrackedButton, it will |-performClick:| itself.
//
// To create the hover state effects, there are two code paths.  When the menu
// is opened sticky, a tracking rect produces mouse entered/exit events that
// allow for setting the cell's highlight property.  When in a drag cycle,
// however, the only event received is |-mouseDragged:|.  Therefore, a
// delayed selector is scheduled to poll the mouse location after each drag
// event.  This checks if the user is still over the button after the drag
// events stop being sent, indicating either the user is hovering without
// movement or that the mouse is no longer over the receiver.
@interface MenuTrackedButton : NSButton {
 @private
  // If the button received a |-mouseEntered:| event. This short-circuits the
  // custom drag tracking logic.
  BOOL didEnter_;

  // Whether or not the user is in a click-drag-release event sequence. If so
  // and this receives a |-mouseUp:|, then this will click itself.
  BOOL tracking_;

  // In order to get hover effects when the menu is sticky-opened, a tracking
  // rect needs to be installed on the button.
  NSTrackingRectTag trackingTag_;
}

@property(nonatomic, readonly, getter=isTracking) BOOL tracking;

@end

#endif  // CHROME_BROWSER_UI_COCOA_APP_MENU_MENU_TRACKED_BUTTON_H_
