// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/download/download_show_all_cell.h"

#import "chrome/browser/themes/theme_properties.h"
#import "chrome/browser/themes/theme_service.h"
#import "chrome/browser/ui/cocoa/download/background_theme.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "chrome/grit/theme_resources.h"

// Distance from top border to icon.
const CGFloat kImagePaddingTop = 7;

// Distance from left border to icon.
const CGFloat kImagePaddingLeft = 11;

// Width of icon.
const CGFloat kImageWidth = 16;

// Height of icon.
const CGFloat kImageHeight = 16;

// x distance between image and title.
const CGFloat kImageTextPadding = 4;

// x coordinate of download name string, in view coords.
const CGFloat kTextPosLeft =
    kImagePaddingLeft + kImageWidth + kImageTextPadding;

// Distance from end of title to right border.
const CGFloat kTextPaddingRight = 13;

// y coordinate of title, in view coords.
const CGFloat kTextPosTop = 10;

// Width of outer stroke
const CGFloat kOuterStrokeWidth = 1;

@interface DownloadShowAllCell(Private)
- (const ui::ThemeProvider*)backgroundThemeWrappingProvider:
    (const ui::ThemeProvider*)provider;
- (BOOL)pressedWithDefaultTheme;
- (NSColor*)titleColor;
@end

@implementation DownloadShowAllCell

- (void)setInitialState {
  [self setFont:[NSFont systemFontOfSize:
      [NSFont systemFontSizeForControlSize:NSSmallControlSize]]];
}

// For nib instantiations
- (id)initWithCoder:(NSCoder*)decoder {
  if ((self = [super initWithCoder:decoder])) {
    [self setInitialState];
  }
  return self;
}

// For programmatic instantiations.
- (id)initTextCell:(NSString*)string {
  if ((self = [super initTextCell:string])) {
    [self setInitialState];
  }
  return self;
}

- (void)setShowsBorderOnlyWhileMouseInside:(BOOL)showOnly {
  // Override to make sure it doesn't do anything if it's called accidentally.
}

- (const ui::ThemeProvider*)backgroundThemeWrappingProvider:
    (const ui::ThemeProvider*)provider {
  if (!themeProvider_.get()) {
    themeProvider_.reset(new BackgroundTheme(provider));
  }

  return themeProvider_.get();
}

// Returns if the button was pressed while the default theme was active.
- (BOOL)pressedWithDefaultTheme {
  const ui::ThemeProvider* themeProvider =
      [[[self controlView] window] themeProvider];
  bool isDefaultTheme =
      !themeProvider->HasCustomImage(IDR_THEME_BUTTON_BACKGROUND);
  return isDefaultTheme && [self isHighlighted];
}

// Returns the text color that should be used to draw title text.
- (NSColor*)titleColor {
  const ui::ThemeProvider* themeProvider =
      [[[self controlView] window] themeProvider];
  if (!themeProvider || [self pressedWithDefaultTheme])
    return [NSColor alternateSelectedControlTextColor];
  return themeProvider->GetNSColor(ThemeProperties::COLOR_BOOKMARK_TEXT);
}

- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView*)controlView {
  NSRect drawFrame = NSInsetRect(cellFrame, 1.5, 1.5);
  NSRect innerFrame = NSInsetRect(cellFrame, 2, 2);

  const float radius = 3;
  NSWindow* window = [controlView window];
  BOOL active = [window isKeyWindow] || [window isMainWindow];

  // In the default theme, draw download items with the bookmark button
  // gradient. For some themes, this leads to unreadable text, so draw the item
  // with a background that looks like windows (some transparent white) if a
  // theme is used. Use custom theme object with a white color gradient to trick
  // the superclass into drawing what we want.
  const ui::ThemeProvider* themeProvider =
      [[[self controlView] window] themeProvider];
  if (!themeProvider)
    return;

  bool isDefaultTheme =
      !themeProvider->HasCustomImage(IDR_THEME_BUTTON_BACKGROUND);

  NSGradient* bgGradient = nil;
  if (!isDefaultTheme) {
    themeProvider = [self backgroundThemeWrappingProvider:themeProvider];
    bgGradient = themeProvider->GetNSGradient(
        active ? ThemeProperties::GRADIENT_TOOLBAR_BUTTON :
                 ThemeProperties::GRADIENT_TOOLBAR_BUTTON_INACTIVE);
  }

  NSBezierPath* buttonInnerPath =
      [NSBezierPath bezierPathWithRoundedRect:drawFrame
                                      xRadius:radius
                                      yRadius:radius];

  // Stroke the borders and appropriate fill gradient.
  [self drawBorderAndFillForTheme:themeProvider
                      controlView:controlView
                        innerPath:buttonInnerPath
              showClickedGradient:[self isHighlighted]
            showHighlightGradient:[self isMouseInside]
                       hoverAlpha:0.0
                           active:active
                        cellFrame:cellFrame
                  defaultGradient:bgGradient];

  [self drawInteriorWithFrame:innerFrame inView:controlView];
}

- (NSDictionary*)textAttributes {
  return [NSDictionary dictionaryWithObjectsAndKeys:
      [self titleColor], NSForegroundColorAttributeName,
      [self font], NSFontAttributeName,
      nil];
}

- (void)drawInteriorWithFrame:(NSRect)cellFrame inView:(NSView*)controlView {
  // Draw title
  NSPoint primaryPos = NSMakePoint(
      cellFrame.origin.x + kTextPosLeft, kTextPosTop);
  [[self title] drawAtPoint:primaryPos withAttributes:[self textAttributes]];

  // Draw icon
  [[self image] drawInRect:[self imageRectForBounds:cellFrame]
                  fromRect:NSZeroRect
                 operation:NSCompositeSourceOver
                  fraction:[self isEnabled] ? 1.0 : 0.5
            respectFlipped:YES
                     hints:nil];
}

- (NSRect)imageRectForBounds:(NSRect)cellFrame {
  return NSMakeRect(cellFrame.origin.x + kImagePaddingLeft,
                    cellFrame.origin.y + kImagePaddingTop,
                    kImageWidth,
                    kImageHeight);
}

- (NSSize)cellSize {
  NSSize size = [super cellSize];

  // Custom width:
  NSSize textSize = [[self title] sizeWithAttributes:[self textAttributes]];
  size.width = kTextPosLeft + textSize.width + kTextPaddingRight +
      kOuterStrokeWidth * 2;
  return size;
}
@end
