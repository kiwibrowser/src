// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_AUTOFILL_AUTOFILL_TOOLTIP_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_AUTOFILL_AUTOFILL_TOOLTIP_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/info_bubble_view.h"

@class AutofillBubbleController;
@class AutofillTooltip;

// Controller for the Tooltip view, which handles displaying/hiding the
// tooltip bubble on hover.
@interface AutofillTooltipController : NSViewController {
 @private
  base::scoped_nsobject<AutofillTooltip> view_;
  AutofillBubbleController* bubbleController_;
  NSString* message_;
  info_bubble::BubbleArrowLocation arrowLocation_;

  // Indicates whether a tooltip bubble should show. YES when hovering on icon
  // or tooltip bubble.
  BOOL shouldDisplayTooltip_;

  // Tracks whether mouse pointer currently hovers above bubble.
  BOOL isHoveringOnBubble_;
}

// |message| to display in the tooltip.
@property(copy, nonatomic) NSString* message;

// Maximal width of the text excluding insets.
@property(nonatomic) CGFloat maxTooltipWidth;

- (id)initWithArrowLocation:(info_bubble::BubbleArrowLocation)arrowLocation;
- (void)setImage:(NSImage*)image;

@end;

#endif  // CHROME_BROWSER_UI_COCOA_AUTOFILL_AUTOFILL_TOOLTIP_CONTROLLER_H_
