// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/bubble_view.h"

#include "chrome/browser/themes/theme_properties.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMNSBezierPath+RoundRect.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMNSColor+Luminance.h"
#include "ui/base/theme_provider.h"

// The roundedness of the edges of the bubble. This matches the value used on
// Lion for window corners.
const int kBubbleCornerRadius = 3;
const float kWindowEdge = 0.7f;

@implementation BubbleView

- (id)initWithFrame:(NSRect)frame {
  if ((self = [super initWithFrame:frame])) {
    cornerFlags_ = kRoundedAllCorners;
  }
  return self;
}

// Sets the string displayed in the bubble. A copy of the string is made.
- (void)setContent:(NSString*)content {
  if ([content_ isEqualToString:content])
    return;
  content_.reset([content copy]);
  [self setNeedsDisplay:YES];
}

// Sets which corners will be rounded.
- (void)setCornerFlags:(unsigned long)flags {
  if (cornerFlags_ == flags)
    return;
  cornerFlags_ = flags;
  [self setNeedsDisplay:YES];
}

- (void)setThemeProvider:(NSWindow*)provider {
  if (themeProvider_ == provider)
    return;
  themeProvider_ = provider;
  [self setNeedsDisplay:YES];
}

- (NSString*)content {
  return content_.get();
}

- (unsigned long)cornerFlags {
  return cornerFlags_;
}

// The font used to display the content string.
- (NSFont*)font {
  return [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
}

// Draws the themed background and the text. Will draw a gray bg if no theme.
- (void)drawRect:(NSRect)rect {
  float topLeftRadius =
      cornerFlags_ & kRoundedTopLeftCorner ? kBubbleCornerRadius : 0;
  float topRightRadius =
      cornerFlags_ & kRoundedTopRightCorner ? kBubbleCornerRadius : 0;
  float bottomLeftRadius =
      cornerFlags_ & kRoundedBottomLeftCorner ? kBubbleCornerRadius : 0;
  float bottomRightRadius =
      cornerFlags_ & kRoundedBottomRightCorner ? kBubbleCornerRadius : 0;

  const ui::ThemeProvider* themeProvider = themeProvider_
                                               ? [themeProvider_ themeProvider]
                                               : [[self window] themeProvider];

  // Background / Edge

  NSRect bounds = [self bounds];
  bounds = NSInsetRect(bounds, 0.5, 0.5);
  NSBezierPath* border =
      [NSBezierPath gtm_bezierPathWithRoundRect:bounds
                            topLeftCornerRadius:topLeftRadius
                           topRightCornerRadius:topRightRadius
                         bottomLeftCornerRadius:bottomLeftRadius
                        bottomRightCornerRadius:bottomRightRadius];

  if (themeProvider)
    [themeProvider->GetNSColor(ThemeProperties::COLOR_TOOLBAR) set];
  [border fill];

  [[NSColor colorWithDeviceWhite:kWindowEdge alpha:1.0f] set];
  [border stroke];

  // Text
  NSColor* textColor = [NSColor blackColor];
  if (themeProvider)
    textColor = themeProvider->GetNSColor(ThemeProperties::COLOR_TAB_TEXT);
  NSFont* textFont = [self font];
  base::scoped_nsobject<NSShadow> textShadow([[NSShadow alloc] init]);
  [textShadow setShadowBlurRadius:0.0f];
  [textShadow.get() setShadowColor:[textColor gtm_legibleTextColor]];
  [textShadow.get() setShadowOffset:NSMakeSize(0.0f, -1.0f)];

  NSDictionary* textDict = [NSDictionary dictionaryWithObjectsAndKeys:
      textColor, NSForegroundColorAttributeName,
      textFont, NSFontAttributeName,
      textShadow.get(), NSShadowAttributeName,
      nil];
  [content_ drawAtPoint:NSMakePoint(kBubbleViewTextPositionX,
                                    kBubbleViewTextPositionY)
         withAttributes:textDict];
}

@end
