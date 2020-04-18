// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TOOLBAR_TOOLBAR_VIEW_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_TOOLBAR_TOOLBAR_VIEW_COCOA_H_

#import <Cocoa/Cocoa.h>
#import "chrome/browser/ui/cocoa/animatable_view.h"

// A view that handles any special rendering of the toolbar bar.  At
// this time it only draws a gradient.  Future changes (e.g. themes)
// may require new functionality here.

@interface ToolbarView : AnimatableView {
 @private
  // The opacity of the divider line (at the bottom of the toolbar); used when
  // the detached bookmark bar is morphing to the normal bar and vice versa.
  CGFloat dividerOpacity_;
}

@property(assign, nonatomic) CGFloat dividerOpacity;
@end

#endif  // CHROME_BROWSER_UI_COCOA_TOOLBAR_TOOLBAR_VIEW_COCOA_H_
