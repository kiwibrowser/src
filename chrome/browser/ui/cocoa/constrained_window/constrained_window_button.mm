// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_button.h"

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/key_equivalent_constants.h"
#include "skia/ext/skia_utils_mac.h"
#import "third_party/molokocacao/NSBezierPath+MCAdditions.h"
#include "ui/gfx/scoped_ns_graphics_context_save_gstate_mac.h"

@interface ConstrainedWindowButton ()
- (BOOL)isMouseReallyInside;
@end

namespace {

enum ButtonState {
  BUTTON_NORMAL,
  BUTTON_HOVER,
  BUTTON_PRESSED,
  BUTTON_DISABLED,
};

const CGFloat kButtonHeight = 28;
const CGFloat kButtonPaddingX = 14;

ButtonState cellButtonState(id<ConstrainedWindowButtonDrawableCell> cell) {
  if (!cell)
    return BUTTON_NORMAL;
  if (![cell isEnabled])
    return BUTTON_DISABLED;
  if ([cell isHighlighted])
    return BUTTON_PRESSED;
  if ([cell isMouseInside])
    return BUTTON_HOVER;
  return BUTTON_NORMAL;
}

// The functions below use hex color values to make it easier to compare
// the code with the spec. Table of hex values are also more readable
// then tables of NSColor constructors.

NSGradient* GetButtonGradient(ButtonState button_state) {
  const SkColor start[] = {0xFFF0F0F0, 0xFFF4F4F4, 0xFFEBEBEB, 0xFFEDEDED};
  const SkColor end[] = {0xFFE0E0E0, 0xFFE4E4E4, 0xFFDBDBDB, 0xFFDEDEDE};

  NSColor* start_color = skia::SkColorToCalibratedNSColor(start[button_state]);
  NSColor* end_color = skia::SkColorToCalibratedNSColor(end[button_state]);
  return [[[NSGradient alloc] initWithColorsAndLocations:
      start_color, 0.0,
      start_color, 0.38,
      end_color, 1.0,
      nil] autorelease];
}

NSShadow* GetButtonHighlight(ButtonState button_state) {
  const SkColor color[] = {0xBFFFFFFF, 0xF2FFFFFF, 0x24000000, 0x00000000};

  NSShadow* shadow = [[[NSShadow alloc] init] autorelease];
  [shadow setShadowColor:skia::SkColorToCalibratedNSColor(color[button_state])];
  [shadow setShadowOffset:NSMakeSize(0, -1)];
  [shadow setShadowBlurRadius:2];
  return shadow;
}

NSShadow* GetButtonShadow(ButtonState button_state) {
  const SkColor color[] = {0x14000000, 0x1F000000, 0x00000000, 0x00000000};

  NSShadow* shadow = [[[NSShadow alloc] init] autorelease];
  [shadow setShadowColor:skia::SkColorToCalibratedNSColor(color[button_state])];
  [shadow setShadowOffset:NSMakeSize(0, -1)];
  [shadow setShadowBlurRadius:0];
  return shadow;
}

NSColor* GetButtonBorderColor(ButtonState button_state) {
  const SkColor color[] = {0x40000000, 0x4D000000, 0x4D000000, 0x1F000000};

  return skia::SkColorToCalibratedNSColor(color[button_state]);
}

NSAttributedString* GetButtonAttributedString(
    NSString* title,
    NSString* key_equivalent,
    id<ConstrainedWindowButtonDrawableCell> cell) {
  const SkColor text_color[] = {0xFF333333, 0XFF000000, 0xFF000000, 0xFFAAAAAA};
  // The shadow color should be 0xFFF0F0F0 but that doesn't show up so use
  // pure white instead.
  const SkColor shadow_color[] =
      {0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF};

  base::scoped_nsobject<NSShadow> shadow([[NSShadow alloc] init]);
  [shadow setShadowColor:
      skia::SkColorToCalibratedNSColor(shadow_color[cellButtonState(cell)])];
  [shadow setShadowOffset:NSMakeSize(0, -1)];
  [shadow setShadowBlurRadius:0];

  base::scoped_nsobject<NSMutableParagraphStyle> paragraphStyle(
      [[NSMutableParagraphStyle alloc] init]);
  [paragraphStyle setAlignment:NSCenterTextAlignment];

  NSFont* font = nil;
  if ([key_equivalent isEqualToString:kKeyEquivalentReturn])
    font = [NSFont boldSystemFontOfSize:12];
  else
    font = [NSFont systemFontOfSize:12];

  NSDictionary* attributes = [NSDictionary dictionaryWithObjectsAndKeys:
      font, NSFontAttributeName,
      skia::SkColorToCalibratedNSColor(text_color[cellButtonState(cell)]),
      NSForegroundColorAttributeName,
      shadow.get(), NSShadowAttributeName,
      paragraphStyle.get(), NSParagraphStyleAttributeName,
      nil];
  return [[[NSAttributedString alloc] initWithString:title
                                          attributes:attributes] autorelease];
}

void DrawBackgroundAndShadow(const NSRect& frame,
                             id<ConstrainedWindowButtonDrawableCell> cell,
                             NSView* view) {
  NSBezierPath* path = [NSBezierPath bezierPathWithRoundedRect:frame
                                                       xRadius:2
                                                       yRadius:2];
  [ConstrainedWindowButton DrawBackgroundAndShadowForPath:path
                                                 withCell:cell
                                                   inView:view];
}

void DrawInnerHighlight(const NSRect& frame,
                        id<ConstrainedWindowButtonDrawableCell> cell,
                        NSView* view) {
  NSBezierPath* path =
      [NSBezierPath bezierPathWithRoundedRect:NSInsetRect(frame, 1, 1)
                                      xRadius:2
                                      yRadius:2];
  [ConstrainedWindowButton DrawInnerHighlightForPath:path
                                            withCell:cell
                                              inView:view];
}

void DrawBorder(const NSRect& frame,
                id<ConstrainedWindowButtonDrawableCell> cell,
                NSView* view) {
  NSBezierPath* path =
      [NSBezierPath bezierPathWithRoundedRect:NSInsetRect(frame, 0.5, 0.5)
                                      xRadius:2
                                      yRadius:2];
  [ConstrainedWindowButton DrawBorderForPath:path
                                    withCell:cell
                                      inView:view];
}

}  // namespace

@implementation ConstrainedWindowButton

+ (Class)cellClass {
  return [ConstrainedWindowButtonCell class];
}

- (void)updateTrackingAreas {
  BOOL mouseInView = [self isMouseReallyInside];

  if (trackingArea_.get())
    [self removeTrackingArea:trackingArea_.get()];

  NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited |
                                  NSTrackingActiveInActiveApp |
                                  NSTrackingInVisibleRect;
  if (mouseInView)
    options |= NSTrackingAssumeInside;

  trackingArea_.reset([[CrTrackingArea alloc]
                        initWithRect:NSZeroRect
                             options:options
                               owner:self
                            userInfo:nil]);
  [self addTrackingArea:trackingArea_.get()];
  if ([[self cell] isMouseInside] != mouseInView) {
    [[self cell] setIsMouseInside:mouseInView];
    [self setNeedsDisplay:YES];
  }
}

- (void)mouseEntered:(NSEvent*)theEvent {
  [[self cell] setIsMouseInside:YES];
  [self setNeedsDisplay:YES];
}

- (void)mouseExited:(NSEvent*)theEvent {
  [[self cell] setIsMouseInside:YES];
  [[self cell] setIsMouseInside:NO];
  [self setNeedsDisplay:YES];
}

- (BOOL)isMouseReallyInside {
  BOOL mouseInView = NO;
  NSWindow* window = [self window];
  if (window) {
    NSPoint mousePoint = [window mouseLocationOutsideOfEventStream];
    mousePoint = [self convertPoint:mousePoint fromView:nil];
    mouseInView = [self mouse:mousePoint inRect:[self bounds]];
  }
  return mouseInView;
}

- (void)sizeToFit {
  [super sizeToFit];
  NSSize size = [self frame].size;
  if (size.width < constrained_window_button::kButtonMinWidth) {
    size.width = constrained_window_button::kButtonMinWidth;
    [self setFrameSize:size];
  }
}

+ (void)DrawBackgroundAndShadowForPath:(NSBezierPath*)path
                          withCell:(id<ConstrainedWindowButtonDrawableCell>)cell
                            inView:(NSView*)view {
  ButtonState buttonState = cellButtonState(cell);
  {
    gfx::ScopedNSGraphicsContextSaveGState scopedGState;
    [GetButtonShadow(buttonState) set];
    [[[view window] backgroundColor] set];
    [path fill];
  }
  [GetButtonGradient(buttonState) drawInBezierPath:path angle:90.0];
}

+ (void)DrawInnerHighlightForPath:(NSBezierPath*)path
                         withCell:(id<ConstrainedWindowButtonDrawableCell>)cell
                           inView:(NSView*)view {
  [path fillWithInnerShadow:GetButtonHighlight(cellButtonState(cell))];
}

+ (void)DrawBorderForPath:(NSBezierPath*)path
                 withCell:(id<ConstrainedWindowButtonDrawableCell>)cell
                   inView:(NSView*)view {
  if ([[[view window] firstResponder] isEqual:view])
    [[NSColor colorWithCalibratedRed:0.30 green:0.57 blue:1.0 alpha:1.0] set];
  else
    [GetButtonBorderColor(cellButtonState(cell)) set];
  [path stroke];
}

@end

@implementation ConstrainedWindowButtonCell

@synthesize isMouseInside = isMouseInside_;

- (void)drawBezelWithFrame:(NSRect)frame inView:(NSView*)controlView {
  // Inset to leave room for shadow.
  --frame.size.height;

  // Background and shadow
  DrawBackgroundAndShadow(frame, self, controlView);

  // Inner highlight
  DrawInnerHighlight(frame, self, controlView);

  // Border
  DrawBorder(frame, self, controlView);
}

- (void)drawInteriorWithFrame:(NSRect)frame inView:(NSView*)controlView {
  // Inset to leave room for shadow.
  --frame.size.height;
  NSAttributedString* title = GetButtonAttributedString(
     [self title], [self keyEquivalent], self);
  [self drawTitle:title withFrame:frame inView:controlView];
}

- (NSSize)cellSize {
  NSAttributedString* title = GetButtonAttributedString(
      [self title], [self keyEquivalent], self);
  NSSize size = [title size];
  size.height = std::max(size.height, kButtonHeight);
  size.width += kButtonPaddingX * 2;
  return size;
}

- (NSAttributedString*)getAttributedTitle {
  return GetButtonAttributedString(
      [self title], [self keyEquivalent], self);
}

@end
