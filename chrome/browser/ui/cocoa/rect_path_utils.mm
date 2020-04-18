// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/rect_path_utils.h"

#import "third_party/google_toolbox_for_mac/src/AppKit/GTMNSBezierPath+RoundRect.h"

namespace rect_path_utils {

NSBezierPath* RectPathWithInset(RoundedCornerFlags roundedCornerFlags,
                                const NSRect frame,
                                const CGFloat insetX,
                                const CGFloat insetY,
                                const CGFloat outerRadius) {
  NSRect insetFrame = NSInsetRect(frame, insetX, insetY);

  if (outerRadius > 0.0) {
    CGFloat leftRadius = outerRadius - insetX;
    CGFloat rightRadius =
        (roundedCornerFlags == RoundedCornerLeft) ? 0 : leftRadius;

    return [NSBezierPath gtm_bezierPathWithRoundRect:insetFrame
                                 topLeftCornerRadius:leftRadius
                                topRightCornerRadius:rightRadius
                              bottomLeftCornerRadius:leftRadius
                             bottomRightCornerRadius:rightRadius];
  } else {
    return [NSBezierPath bezierPathWithRect:insetFrame];
  }
}

// Similar to |NSRectFill()|, additionally sets |color| as the fill
// color.  |outerRadius| greater than 0.0 uses rounded corners, with
// inset backed out of the radius.
void FillRectWithInset(RoundedCornerFlags roundedCornerFlags,
                       const NSRect frame,
                       const CGFloat insetX,
                       const CGFloat insetY,
                       const CGFloat outerRadius,
                       NSColor* color) {
  NSBezierPath* path =
      RectPathWithInset(roundedCornerFlags, frame, insetX, insetY, outerRadius);
  [color setFill];
  [path fill];
}

// Similar to |NSFrameRectWithWidth()|, additionally sets |color| as
// the stroke color (as opposed to the fill color).  |outerRadius|
// greater than 0.0 uses rounded corners, with inset backed out of the
// radius.
void FrameRectWithInset(RoundedCornerFlags roundedCornerFlags,
                        const NSRect frame,
                        const CGFloat insetX,
                        const CGFloat insetY,
                        const CGFloat outerRadius,
                        const CGFloat lineWidth,
                        NSColor* color) {
  const CGFloat finalInsetX = insetX + (lineWidth / 2.0);
  const CGFloat finalInsetY = insetY + (lineWidth / 2.0);
  NSBezierPath* path =
      RectPathWithInset(roundedCornerFlags, frame, finalInsetX, finalInsetY,
                        outerRadius);
  [color setStroke];
  [path setLineWidth:lineWidth];
  [path stroke];
}

}  // namespace rect_path_utils
