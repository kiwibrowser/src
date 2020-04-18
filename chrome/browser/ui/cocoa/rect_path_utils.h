// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_RECT_PATH_UTILS_H_
#define CHROME_BROWSER_UI_COCOA_RECT_PATH_UTILS_H_

#import <Cocoa/Cocoa.h>

namespace rect_path_utils {

enum RoundedCornerFlags {
  RoundedCornerAll = 0,
  RoundedCornerLeft = 1
};

NSBezierPath *RectPathWithInset(RoundedCornerFlags roundedCornerFlags,
                                const NSRect frame,
                                const CGFloat insetX,
                                const CGFloat insetY,
                                const CGFloat outerRadius);

void FillRectWithInset(RoundedCornerFlags roundedCornerFlags,
                       const NSRect frame,
                       const CGFloat insetX,
                       const CGFloat insetY,
                       const CGFloat outerRadius,
                       NSColor *color);

void FrameRectWithInset(RoundedCornerFlags roundedCornerFlags,
                        const NSRect frame,
                        const CGFloat insetX,
                        const CGFloat insetY,
                        const CGFloat outerRadius,
                        const CGFloat lineWidth,
                        NSColor *color);

} // namespace rect_path_utils

#endif  // CHROME_BROWSER_UI_COCOA_RECT_PATH_UTILS_H_
