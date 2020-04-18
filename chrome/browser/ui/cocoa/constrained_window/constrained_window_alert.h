// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_ALERT_H_
#define CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_ALERT_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"

// This class implements an alert that has a constrained window look and feel
// (close button on top right, WebUI style buttons, etc...).  The alert can be
// shown by using the window accessor and calling -[window orderFont:]. Normally
// this would be done by ConstrainedWindowSheetController.
@interface ConstrainedWindowAlert : NSObject {
 @private
  base::scoped_nsobject<NSTextField> informativeTextField_;
  base::scoped_nsobject<NSTextField> messageTextField_;
  base::scoped_nsobject<NSButton> linkView_;
  base::scoped_nsobject<NSView> accessoryView_;
  base::scoped_nsobject<NSMutableArray> buttons_;
  base::scoped_nsobject<NSButton> closeButton_;
  base::scoped_nsobject<NSWindow> window_;
}

@property(nonatomic, copy) NSString* informativeText;
@property(nonatomic, copy) NSString* messageText;
@property(nonatomic, retain) NSView* accessoryView;
@property(nonatomic, readonly) NSArray* buttons;
@property(nonatomic, readonly) NSButton* closeButton;
@property(nonatomic, readonly) NSWindow* window;

// Default initializer.
- (id)init;

// Adds a button with the given |title|. Newly added buttons are positioned in
// order from right to left.
- (void)addButtonWithTitle:(NSString*)title
             keyEquivalent:(NSString*)keyEquivalent
                    target:(id)target
                    action:(SEL)action;

// Sets the |text|, the |target| and the |action| of a left-aligned link
// positioned above the buttons. If |text| is empty, no link is displayed.
- (void)setLinkText:(NSString*)text
             target:(id)target
             action:(SEL)action;

// Lays out the controls in the alert. This should be called before the window
// is displayed.
- (void)layout;

@end

@interface ConstrainedWindowAlert (ExposedForTesting)
@property(nonatomic, readonly) NSButton* linkView;
@end

#endif  // CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_ALERT_H_
