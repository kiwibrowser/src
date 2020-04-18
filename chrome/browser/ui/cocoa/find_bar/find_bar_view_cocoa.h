// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_FIND_BAR_FIND_BAR_VIEW_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_FIND_BAR_FIND_BAR_VIEW_COCOA_H_

#import <Cocoa/Cocoa.h>

#include "chrome/browser/ui/cocoa/background_gradient_view.h"

// A view that handles painting the border for the FindBar.

@interface FindBarView : BackgroundGradientView {
}

// Specifies that mouse events over this view should be ignored by the
// render host.
- (BOOL)nonWebContentView;

@end

#endif  // CHROME_BROWSER_UI_COCOA_FIND_BAR_FIND_BAR_VIEW_COCOA_H_
