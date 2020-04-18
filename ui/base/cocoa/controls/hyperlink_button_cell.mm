// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/controls/hyperlink_button_cell.h"

using hyperlink_button_cell::UnderlineBehavior;

@interface HyperlinkButtonCell ()
- (void)customizeButtonCell;
@end

@implementation HyperlinkButtonCell

@dynamic textColor;
@synthesize underlineBehavior = underlineBehavior_;

+ (NSColor*)defaultTextColor {
  // Equates to rgb(51, 103, 214) or #3367D6.
  return [NSColor colorWithCalibratedRed:51.0/255.0
                                   green:103.0/255.0
                                    blue:214.0/255.0
                                   alpha:1.0];
}

+ (NSButton*)buttonWithString:(NSString*)string {
  NSButton* button = [[[NSButton alloc] initWithFrame:NSZeroRect] autorelease];
  base::scoped_nsobject<HyperlinkButtonCell> cell(
      [[HyperlinkButtonCell alloc] initTextCell:string]);
  [cell setAlignment:NSLeftTextAlignment];
  [button setCell:cell.get()];
  [button setBezelStyle:NSRegularSquareBezelStyle];
  return button;
}

// Designated initializer.
- (id)init {
  if ((self = [super init])) {
    [self customizeButtonCell];
  }
  return self;
}

// Initializer called when the cell is loaded from the NIB.
- (id)initWithCoder:(NSCoder*)aDecoder {
  if ((self = [super initWithCoder:aDecoder])) {
    [self customizeButtonCell];
  }
  return self;
}

// Initializer for code-based creation.
- (id)initTextCell:(NSString*)title {
  if ((self = [super initTextCell:title])) {
    [self customizeButtonCell];
  }
  return self;
}

- (id)copyWithZone:(NSZone*)zone {
  NSColor* color = textColor_.release();
  HyperlinkButtonCell* cell = [super copyWithZone:zone];
  cell->textColor_.reset([color copy]);
  textColor_.reset(color);
  return cell;
}

// Because an NSButtonCell has multiple initializers, this method performs the
// common cell customization code.
- (void)customizeButtonCell {
  [self setBordered:NO];
  [self setTextColor:[HyperlinkButtonCell defaultTextColor]];
  [self setUnderlineBehavior:UnderlineBehavior::NEVER];

  CGFloat fontSize = [NSFont systemFontSizeForControlSize:[self controlSize]];
  NSFont* font = [NSFont controlContentFontOfSize:fontSize];
  [self setFont:font];

  // Do not change button appearance when we are pushed.
  [self setHighlightsBy:NSNoCellMask];

  // We need to set this so that we can override |-mouseEntered:| and
  // |-mouseExited:| to change the cursor style on hover states.
  [self setShowsBorderOnlyWhileMouseInside:YES];
}

- (void)setControlSize:(NSControlSize)size {
  [super setControlSize:size];
  [self customizeButtonCell];  // recompute |font|.
}

// Creates the NSDictionary of attributes for the attributed string.
- (NSDictionary*)linkAttributes {
  NSUInteger underlineMask = NSUnderlineStyleNone;
  if (underlineBehavior_ == UnderlineBehavior::ALWAYS ||
      (mouseIsInside_ && [self isEnabled] &&
       underlineBehavior_ == UnderlineBehavior::ON_HOVER)) {
    underlineMask = NSUnderlinePatternSolid | NSUnderlineStyleSingle;
  }

  base::scoped_nsobject<NSMutableParagraphStyle> paragraphStyle(
      [[NSParagraphStyle defaultParagraphStyle] mutableCopy]);
  [paragraphStyle setAlignment:[self alignment]];
  [paragraphStyle setLineBreakMode:[self lineBreakMode]];

  return [NSDictionary dictionaryWithObjectsAndKeys:
      [self textColor], NSForegroundColorAttributeName,
      [NSNumber numberWithInt:underlineMask], NSUnderlineStyleAttributeName,
      [self font], NSFontAttributeName,
      [NSCursor pointingHandCursor], NSCursorAttributeName,
      paragraphStyle.get(), NSParagraphStyleAttributeName,
      nil
  ];
}

// Override the drawing for the cell so that the custom style attributes
// can always be applied and so that ellipses will appear when appropriate.
- (NSRect)drawTitle:(NSAttributedString*)title
          withFrame:(NSRect)frame
             inView:(NSView*)controlView {
  NSDictionary* linkAttributes = [self linkAttributes];
  NSString* plainTitle = [title string];
  [plainTitle drawWithRect:frame
                   options:(NSStringDrawingUsesLineFragmentOrigin |
                            NSStringDrawingTruncatesLastVisibleLine)
                attributes:linkAttributes];
  return frame;
}

// Override the default behavior to draw the border. Instead, change the cursor.
- (void)mouseEntered:(NSEvent*)event {
  mouseIsInside_ = YES;
  if ([self isEnabled])
    [[NSCursor pointingHandCursor] push];
  else
    [[NSCursor currentCursor] push];
  if (underlineBehavior_ == UnderlineBehavior::ON_HOVER)
    [[self controlView] setNeedsDisplay:YES];
}

- (void)mouseExited:(NSEvent*)event {
  mouseIsInside_ = NO;
  [NSCursor pop];
  if (underlineBehavior_ == UnderlineBehavior::ON_HOVER)
    [[self controlView] setNeedsDisplay:YES];
}

// Setters and getters.
- (NSColor*)textColor {
  if ([self isEnabled])
    return textColor_.get();
  else
    return [NSColor disabledControlTextColor];
}

- (void)setTextColor:(NSColor*)color {
  textColor_.reset([color retain]);
}

// Override so that |-sizeToFit| works better with this type of cell.
- (NSSize)cellSize {
  NSSize size = [super cellSize];
  size.width += 2;
  return size;
}

@end
