// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_VIEW_RESIZER_PONG_H_
#define CHROME_BROWSER_UI_COCOA_VIEW_RESIZER_PONG_H_

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/view_resizer.h"

@interface ViewResizerPong : NSObject<ViewResizer> {
 @private
  CGFloat height_;
}
@property(nonatomic) CGFloat height;

- (void)resizeView:(NSView*)view newHeight:(CGFloat)height;
@end

#endif  // CHROME_BROWSER_UI_COCOA_VIEW_RESIZER_PONG_H_
