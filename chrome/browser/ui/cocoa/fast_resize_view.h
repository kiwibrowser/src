// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_FAST_RESIZE_VIEW_H_
#define CHROME_BROWSER_UI_COCOA_FAST_RESIZE_VIEW_H_

#import <Cocoa/Cocoa.h>

// A Cocoa view originally created to support fast re-painting to white on
// resize. This is done by CoreAnimation now, so this view remains only as the
// first opaque ancestor of accelerated web contents views.
@interface FastResizeView : NSView

@end

#endif  // CHROME_BROWSER_UI_COCOA_FAST_RESIZE_VIEW_H_
