// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/autofill/autofill_tooltip_controller.h"

#include "base/mac/foundation_util.h"
#import "chrome/browser/ui/cocoa/autofill/autofill_bubble_controller.h"
#import "ui/base/cocoa/base_view.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/base/cocoa/hover_image_button.h"

// Delay time before tooltip shows/hides.
const NSTimeInterval kTooltipDelay = 0.1;

// How far to inset tooltip contents.
CGFloat kTooltipInset = 10;

#pragma mark AutofillTooltipController - private methods

@interface AutofillTooltipController ()

// Sets hover state for "mouse over InfoBubble".
- (void)setHoveringOnBubble:(BOOL)hoveringOnBubble;

// Update the combined hover state - if either button or bubble is hovered,
// the combined state is considered "hovered". Notifies delegate if the state
// changed.
- (void)updateTooltipDisplayState;

@end

#pragma mark AutofillTooltip

// The actual tooltip control - based on HoverButton, which comes with free
// hover handling.
@interface AutofillTooltip : HoverButton {
 @private
  // Not owned - |tooltipController_| owns this object.
  AutofillTooltipController* tooltipController_;
}

@property(assign, nonatomic) AutofillTooltipController* tooltipController;

@end


@implementation AutofillTooltip

@synthesize tooltipController = tooltipController_;

- (void)drawRect:(NSRect)rect {
 [[self image] drawInRect:rect
                 fromRect:NSZeroRect
                operation:NSCompositeSourceOver
                 fraction:1.0
           respectFlipped:YES
                    hints:nil];
}

- (void)setHoverState:(HoverState)state {
  [super setHoverState:state];
  [tooltipController_ updateTooltipDisplayState];
}

- (BOOL)acceptsFirstResponder {
  return NO;
}

@end

#pragma mark AutofillTrackingView

// A very basic view that only tracks mouseEntered:/mouseExited: and forwards
// them to |tooltipController_|.
@interface AutofillTrackingView : BaseView {
 @private
  // Not owned - tooltip controller owns tracking view and tooltip.
  AutofillTooltipController* tooltipController_;
}

@property(assign, nonatomic) AutofillTooltipController* tooltipController;

@end

@implementation AutofillTrackingView

@synthesize tooltipController = tooltipController_;

- (void)mouseEntered:(NSEvent*)theEvent {
  [tooltipController_ setHoveringOnBubble:YES];
}

- (void)mouseExited:(NSEvent*)theEvent {
  [tooltipController_ setHoveringOnBubble:NO];
}

@end

#pragma mark AutofillTooltipController

@implementation AutofillTooltipController

@synthesize message = message_;
@synthesize maxTooltipWidth = maxTooltipWidth_;

- (id)initWithArrowLocation:(info_bubble::BubbleArrowLocation)arrowLocation {
  if ((self = [super init])) {
    arrowLocation_ = arrowLocation;
    maxTooltipWidth_ = CGFLOAT_MAX;
    view_.reset([[AutofillTooltip alloc] init]);
    [self setView:view_];
    [view_ setTooltipController:self];
  }
  return self;
}

- (void)dealloc {
  [view_ setTooltipController:nil];
  [NSObject cancelPreviousPerformRequestsWithTarget:self];
  [[NSNotificationCenter defaultCenter]
      removeObserver:self
                name:NSWindowWillCloseNotification
              object:[bubbleController_ window]];
  [super dealloc];
}

- (void)setImage:(NSImage*)image {
  [view_ setImage:image];
  [view_ setFrameSize:[image size]];
}

- (void)tooltipWindowWillClose:(NSNotification*)notification {
  bubbleController_ = nil;
}

- (void)displayHover {
  [bubbleController_ close];
  bubbleController_ = [[AutofillBubbleController alloc]
      initWithParentWindow:[[self view] window]
                   message:[self message]
                     inset:NSMakeSize(kTooltipInset, kTooltipInset)
             maxLabelWidth:maxTooltipWidth_
             arrowLocation:arrowLocation_];
  [bubbleController_ setShouldCloseOnResignKey:NO];

  // Handle bubble self-deleting.
  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  [center addObserver:self
             selector:@selector(tooltipWindowWillClose:)
                 name:NSWindowWillCloseNotification
               object:[bubbleController_ window]];

  // Inject a tracking view so controller can track hover events for the bubble.
  base::scoped_nsobject<NSView> oldContentView(
      [[[bubbleController_ window] contentView] retain]);
  base::scoped_nsobject<AutofillTrackingView> trackingView(
      [[AutofillTrackingView alloc] initWithFrame:[oldContentView frame]]);
  [trackingView setTooltipController:self];
  [trackingView setAutoresizesSubviews:YES];
  [oldContentView setFrame:[trackingView bounds]];
  [oldContentView
      setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
  [[bubbleController_ window] setContentView:trackingView];
  [trackingView setSubviews:@[ oldContentView ]];

  // Compute anchor point (in window coords - views might be flipped).
  NSRect viewRect = [view_ convertRect:[view_ bounds] toView:nil];
  NSPoint anchorPoint = NSMakePoint(NSMidX(viewRect), NSMinY(viewRect));
  [bubbleController_ setAnchorPoint:ui::ConvertPointFromWindowToScreen(
      [[self view] window], anchorPoint)];
  [bubbleController_ showWindow:self];
}

- (void)hideHover {
  [bubbleController_ close];
}

- (void)setHoveringOnBubble:(BOOL)hoveringOnBubble {
  isHoveringOnBubble_ = hoveringOnBubble;
  [self updateTooltipDisplayState];
}

- (void)updateTooltipDisplayState {
  BOOL newDisplayState =
      ([view_ hoverState] != kHoverStateNone || isHoveringOnBubble_);

  if (newDisplayState != shouldDisplayTooltip_) {
    shouldDisplayTooltip_ = newDisplayState;

    // Cancel any pending visibility changes.
    [NSObject cancelPreviousPerformRequestsWithTarget:self];

    // If the desired visibility disagrees with current visibility, start a
    // timer to change visibility. (Uses '!!' to force bool values)
    if (!!bubbleController_ ^ !!shouldDisplayTooltip_) {
      SEL sel = shouldDisplayTooltip_ ? @selector(displayHover)
                                      : @selector(hideHover);
      [self performSelector:sel withObject:nil afterDelay:kTooltipDelay];
    }
  }
}

@end
