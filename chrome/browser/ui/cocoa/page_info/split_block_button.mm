// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/page_info/split_block_button.h"

#include <cmath>

#include "base/logging.h"
#include "base/mac/scoped_nsobject.h"
#include "chrome/grit/generated_resources.h"
#include "skia/ext/skia_utils_mac.h"
#import "ui/base/cocoa/menu_controller.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/models/simple_menu_model.h"

namespace {

enum MouseLocation {
  kInsideLeftCell,
  kInsideRightCell,
  kNotInside,
};

enum CornerType {
  kRounded,
  kAngled,
};

NSBezierPath* PathWithCornerStyles(NSRect frame,
                                   CornerType leftCornerStyle,
                                   CornerType rightCornerStyle) {
  base::scoped_nsobject<NSBezierPath> path([[NSBezierPath bezierPath] retain]);
  const CGFloat x0 = NSMinX(frame);
  const CGFloat x1 = NSMaxX(frame);
  const CGFloat y0 = NSMinY(frame);
  const CGFloat y1 = NSMaxY(frame);
  const CGFloat radius = 2;

  // Start at the center bottom.  Draw left and up, including both left corners.
  [path moveToPoint:NSMakePoint(std::floor((x1 - x0) * .5), y0)];
  if (leftCornerStyle == kAngled) {
    [path lineToPoint:NSMakePoint(x0, y0)];
    [path lineToPoint:NSMakePoint(x0, y1)];
  } else {
    [path appendBezierPathWithArcFromPoint:NSMakePoint(x0, y0)
                                   toPoint:NSMakePoint(x0, y0 + radius)
                                    radius:radius];
    [path appendBezierPathWithArcFromPoint:NSMakePoint(x0, y1)
                                   toPoint:NSMakePoint(x0 + radius, y1)
                                    radius:radius];
  }
  // Draw through the upper right-hand and lower-right-hand corners.
  if (rightCornerStyle == kAngled) {
    [path lineToPoint:NSMakePoint(x1, y1)];
    [path lineToPoint:NSMakePoint(x1, y0)];
  } else {
    [path appendBezierPathWithArcFromPoint:NSMakePoint(x1, y1)
                                   toPoint:NSMakePoint(x1, y1 - radius)
                                    radius:radius];
    [path appendBezierPathWithArcFromPoint:NSMakePoint(x1, y0)
                                   toPoint:NSMakePoint(x1 - radius, y0)
                                    radius:radius];
  }
  return path.autorelease();
}

void DrawBezel(id<ConstrainedWindowButtonDrawableCell> cell,
               CornerType leftCorners,
               CornerType rightCorners,
               NSRect frame,
               NSView* view) {
  if ([cell isMouseInside]) {
    base::scoped_nsobject<NSBezierPath> path(
        [PathWithCornerStyles(frame, leftCorners, rightCorners) retain]);
    [ConstrainedWindowButton DrawBackgroundAndShadowForPath:path
                                                   withCell:cell
                                                     inView:view];
    [ConstrainedWindowButton DrawInnerHighlightForPath:path
                                              withCell:cell
                                                inView:view];
  }
}

}  // namespace

// A button cell used by SplitBlockButton, containing the title.
@interface SplitButtonTitleCell : ConstrainedWindowButtonCell
- (NSRect)rect;
@end

// A button cell used by SplitBlockButton, containing the popup menu.
@interface SplitButtonPopUpCell
    : NSPopUpButtonCell<ConstrainedWindowButtonDrawableCell> {
 @private
  BOOL isMouseInside_;
  base::scoped_nsobject<MenuControllerCocoa> menuController_;
  std::unique_ptr<ui::SimpleMenuModel> menuModel_;
}

// Designated initializer.
- (id)initWithMenuDelegate:(ui::SimpleMenuModel::Delegate*)menuDelegate;

- (NSRect)rect;

@end

@implementation SplitBlockButton

- (id)initWithMenuDelegate:(ui::SimpleMenuModel::Delegate*)menuDelegate {
  if (self = [super initWithFrame:NSZeroRect]) {
    leftCell_.reset([[SplitButtonTitleCell alloc] init]);
    rightCell_.reset(
        [[SplitButtonPopUpCell alloc] initWithMenuDelegate:menuDelegate]);
    [leftCell_ setTitle:l10n_util::GetNSString(IDS_PERMISSION_DENY)];
    [leftCell_ setEnabled:YES];
    [rightCell_ setEnabled:YES];
  }
  return self;
}

+ (Class)cellClass {
  return nil;
}

- (NSString*)title {
  return [leftCell_ title];
}

- (void)setAction:(SEL)action {
  [leftCell_ setAction:action];
}

- (void)setTarget:(id)target {
  [leftCell_ setTarget:target];
}

- (void)drawRect:(NSRect)rect {
  // Copy the base class:  inset to leave room for the shadow.
  --rect.size.height;

  // This function assumes that |rect| is always the same as [self frame].
  // If that changes, the drawing functions will need to be adjusted.
  const CGFloat radius = 2;
  NSBezierPath* path = [NSBezierPath bezierPathWithRoundedRect:rect
                                                       xRadius:radius
                                                       yRadius:radius];
  [ConstrainedWindowButton DrawBackgroundAndShadowForPath:path
                                                 withCell:nil
                                                   inView:self];

  // Use intersection rects for the cell drawing, to ensure the height
  // adjustment is honored.
  [leftCell_ setControlView:self];
  [leftCell_ drawWithFrame:NSIntersectionRect(rect, [self leftCellRect])
                    inView:self];

  [rightCell_ setControlView:self];
  [rightCell_ drawWithFrame:NSIntersectionRect(rect, [self rightCellRect])
                     inView:self];

  // Draw the border.
  path = [NSBezierPath bezierPathWithRoundedRect:NSInsetRect(rect, 0.5, 0.5)
                                         xRadius:radius
                                         yRadius:radius];
  [ConstrainedWindowButton DrawBorderForPath:path withCell:nil inView:self];
}

- (void)updateTrackingAreas {
  [self updateTrackingArea:&leftTrackingArea_
                   forCell:leftCell_
                  withRect:[self leftCellRect]];

  [self updateTrackingArea:&rightTrackingArea_
                   forCell:rightCell_
                  withRect:[self rightCellRect]];
}

- (void)updateTrackingArea:(ui::ScopedCrTrackingArea*)trackingArea
                   forCell:(id<ConstrainedWindowButtonDrawableCell>)cell
                  withRect:(NSRect)rect {
  DCHECK(trackingArea);
  NSTrackingAreaOptions options =
      NSTrackingMouseEnteredAndExited | NSTrackingActiveInActiveApp;
  [self removeTrackingArea:trackingArea->get()];
  trackingArea->reset([[CrTrackingArea alloc] initWithRect:rect
                                                   options:options
                                                     owner:self
                                                  userInfo:nil]);
  [self addTrackingArea:trackingArea->get()];
}

- (void)mouseEntered:(NSEvent*)theEvent {
  [self mouseMoved:theEvent];
}

- (void)mouseExited:(NSEvent*)theEvent {
  [self mouseMoved:theEvent];
}

- (void)mouseMoved:(NSEvent*)theEvent {
  MouseLocation location = [self mouseLocationForEvent:theEvent];
  [rightCell_ setIsMouseInside:NO];
  [leftCell_ setIsMouseInside:NO];
  if (location == kInsideLeftCell)
    [leftCell_ setIsMouseInside:YES];
  else if (location == kInsideRightCell)
    [rightCell_ setIsMouseInside:YES];
  [self setNeedsDisplay:YES];
}

- (void)mouseDown:(NSEvent*)theEvent {
  MouseLocation downLocation = [self mouseLocationForEvent:theEvent];
  NSCell* focusCell = nil;
  NSRect rect;
  if (downLocation == kInsideLeftCell) {
    focusCell = leftCell_.get();
    rect = [self leftCellRect];
  } else if (downLocation == kInsideRightCell) {
    focusCell = rightCell_.get();
    rect = [self rightCellRect];
  }

  do {
    MouseLocation location = [self mouseLocationForEvent:theEvent];
    if (location != kNotInside) {
      [focusCell setHighlighted:YES];
      [self setNeedsDisplay:YES];

      if ([focusCell trackMouse:theEvent
                         inRect:rect
                         ofView:self
                   untilMouseUp:NO]) {
        [focusCell setState:![focusCell state]];
        [self setNeedsDisplay:YES];
        break;
      } else {
        // The above -trackMouse call returned NO, so we know that
        // the mouse left the cell before a mouse up event occurred.
        [focusCell setHighlighted:NO];
        [self setNeedsDisplay:YES];
      }
    }
    const NSUInteger mask = NSLeftMouseUpMask | NSLeftMouseDraggedMask;
    theEvent = [[self window] nextEventMatchingMask:mask];
  } while ([theEvent type] != NSLeftMouseUp);
}

- (MouseLocation)mouseLocationForEvent:(NSEvent*)theEvent {
  MouseLocation location = kNotInside;
  NSPoint mousePoint =
      [self convertPoint:[theEvent locationInWindow] fromView:nil];
  if ([self mouse:mousePoint inRect:[leftCell_ rect]])
    location = kInsideLeftCell;
  else if ([self mouse:mousePoint inRect:[self rightCellRect]])
    location = kInsideRightCell;
  return location;
}

- (void)sizeToFit {
  NSSize leftSize = [leftCell_ cellSize];
  NSSize rightSize = [rightCell_ cellSize];
  NSSize size = NSMakeSize(
      std::ceil(std::max(leftSize.width + rightSize.width,
                         constrained_window_button::kButtonMinWidth)),
      std::ceil(std::max(leftSize.height, rightSize.height)));
  [self setFrameSize:size];
}

- (NSRect)leftCellRect {
  return [leftCell_ rect];
}

- (NSRect)rightCellRect {
  NSRect leftFrame, rightFrame;
  NSDivideRect([self bounds], &leftFrame, &rightFrame,
               NSWidth([self leftCellRect]), NSMinXEdge);
  return rightFrame;
}

// Accessor for Testing.
- (NSMenu*)menu {
  return [rightCell_ menu];
}

@end

@implementation SplitButtonTitleCell

- (void)drawBezelWithFrame:(NSRect)frame inView:(NSView*)controlView {
  DrawBezel(self, kRounded, kAngled, frame, controlView);
}

- (NSRect)rect {
  NSSize size = [self cellSize];
  return NSMakeRect(0, 0, std::ceil(size.width), std::ceil(size.height));
}

@end

@implementation SplitButtonPopUpCell

@synthesize isMouseInside = isMouseInside_;

- (id)initWithMenuDelegate:(ui::SimpleMenuModel::Delegate*)menuDelegate {
  if (self = [super initTextCell:@"" pullsDown:YES]) {
    [self setControlSize:NSSmallControlSize];
    [self setArrowPosition:NSPopUpArrowAtCenter];
    [self setBordered:NO];
    [self setBackgroundColor:[NSColor clearColor]];
    menuModel_.reset(new ui::SimpleMenuModel(menuDelegate));
    menuModel_->AddItemWithStringId(0, IDS_PERMISSION_CUSTOMIZE);
    menuController_.reset([[MenuControllerCocoa alloc]
                 initWithModel:menuModel_.get()
        useWithPopUpButtonCell:NO]);
    [self setMenu:[menuController_ menu]];
    [self setUsesItemFromMenu:NO];
  }
  return self;
}

- (void)drawBorderAndBackgroundWithFrame:(NSRect)frame
                                  inView:(NSView*)controlView {
  // The arrow, which is what should be drawn by the base class, is drawn
  // during -drawBezelWithFrame.  The only way to draw our own border with
  // the default arrow is to make the cell unbordered, and draw the border
  // from -drawBorderAndBackgroundWithFrame, rather than simply overriding
  // -drawBezelWithFrame.
  DrawBezel(self, kAngled, kRounded, frame, controlView);
  [super drawBorderAndBackgroundWithFrame:NSOffsetRect(frame, -4, 0)
                                   inView:controlView];
}

- (NSRect)rect {
  NSSize size = [self cellSize];
  return NSMakeRect(0, 0, std::ceil(size.width), std::ceil(size.height));
}

@end
