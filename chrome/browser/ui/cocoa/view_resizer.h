// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_VIEW_RESIZER_H_
#define CHROME_BROWSER_UI_COCOA_VIEW_RESIZER_H_

#import <Foundation/Foundation.h>

@class NSView;

// Defines a protocol that allows controllers to delegate resizing their views
// to their parents.  When a controller needs to change a view's height, rather
// than resizing it directly, it sends a message to its parent asking the parent
// to perform the resize.  This allows the parent to do any re-layout that may
// become necessary due to the resize.
@protocol ViewResizer <NSObject>
- (void)resizeView:(NSView*)view newHeight:(CGFloat)height;

@optional
// Optional method called when an animation is beginning or ending.  Resize
// delegates can implement this method if they need to modify their behavior
// while an animation is running.
- (void)setAnimationInProgress:(BOOL)inProgress;
@end

#endif  // CHROME_BROWSER_UI_COCOA_VIEW_RESIZER_H_
