// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_NEW_TAB_BUTTON_H_
#define CHROME_BROWSER_UI_COCOA_NEW_TAB_BUTTON_H_

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/themed_window.h"

// Overrides hit-test behavior to only accept clicks inside the image of the
// button, not just inside the bounding box. This could be abstracted to general
// use, but no other buttons are so irregularly shaped with respect to their
// bounding box.

@interface NewTabButton : NSButton<ThemedWindowDrawing>

// Returns YES if the given point is over the button.  |point| is in the
// superview's coordinate system.
- (BOOL)pointIsOverButton:(NSPoint)point;

// Sets the images shown by the NewTabButton's different states.
- (void)setImages;

@end


#endif  // CHROME_BROWSER_UI_COCOA_NEW_TAB_BUTTON_H_
