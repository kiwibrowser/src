// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_AUTOFILL_AUTOFILL_BUBBLE_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_AUTOFILL_AUTOFILL_BUBBLE_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/base_bubble_controller.h"
#import "chrome/browser/ui/cocoa/info_bubble_view.h"

// Bubble controller for field validation error bubbles.
@interface AutofillBubbleController : BaseBubbleController {
 @private
   base::scoped_nsobject<NSTextField> label_;
   NSSize inset_;  // Amount the label is inset from the window.
}

// Creates an error bubble with the given |message|. You need to call
// -showWindow: to make the bubble visible. It will autorelease itself when the
// user dismisses the bubble.
- (id)initWithParentWindow:(NSWindow*)parentWindow
                   message:(NSString*)message;

// Designated initializer. Creates a bubble with given |message| and insets the
// text content by |inset|, with the arrow positioned at |arrowLocation|.
- (id)initWithParentWindow:(NSWindow*)parentWindow
                   message:(NSString*)message
                     inset:(NSSize)inset
             maxLabelWidth:(CGFloat)maxLabelWidth
             arrowLocation:(info_bubble::BubbleArrowLocation)arrowLocation;

// Update the current text with |message|.
- (void)setMessage:(NSString*)message;

@end


#endif  // CHROME_BROWSER_UI_COCOA_AUTOFILL_AUTOFILL_BUBBLE_CONTROLLER_H_
