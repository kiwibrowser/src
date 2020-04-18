// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/tabs/tab_strip_background_view.h"

#import "chrome/browser/ui/cocoa/framed_browser_window.h"
#import "ui/base/cocoa/nsview_additions.h"

@implementation TabStripBackgroundView

- (void)drawRect:(NSRect)dirtyRect {
  // Only the top corners are rounded. For simplicity, round all 4 corners but
  // draw the bottom corners outside of the visible bounds.
  float cornerRadius = 4.0;
  NSRect roundedRect = [self bounds];
  roundedRect.origin.y -= cornerRadius;
  roundedRect.size.height += cornerRadius;
  [[NSBezierPath bezierPathWithRoundedRect:roundedRect
                                   xRadius:cornerRadius
                                   yRadius:cornerRadius] addClip];
  BOOL themed = [FramedBrowserWindow drawWindowThemeInDirtyRect:dirtyRect
                                                        forView:self
                                                         bounds:roundedRect
                                           forceBlackBackground:NO];

  // Draw a 1px border on the top edge and top corners.
  if (themed) {
    CGFloat lineWidth = [self cr_lineWidth];
    // Inset the vertical lines by 0.5px so that the top line gets a full pixel.
    // Outset the horizontal lines by 0.5px so that they are not visible, but
    // still get the rounded corners to get a border.
    NSRect strokeRect = NSInsetRect(roundedRect, -lineWidth/2, lineWidth/2);
    NSBezierPath* path = [NSBezierPath bezierPathWithRoundedRect:strokeRect
                                                         xRadius:cornerRadius
                                                         yRadius:cornerRadius];
    [path setLineWidth:lineWidth];
    [[NSColor colorWithCalibratedWhite:1.0 alpha:0.5] set];
    [path stroke];
  }
}

// ThemedWindowDrawing implementation.

- (void)windowDidChangeTheme {
  [self setNeedsDisplay:YES];
}

- (void)windowDidChangeActive {
  [self setNeedsDisplay:YES];
}

@end
