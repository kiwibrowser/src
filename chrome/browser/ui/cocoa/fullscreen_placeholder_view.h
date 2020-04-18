// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_FULLSCREEN_PLACEHOLDER_VIEW_H_
#define CHROME_BROWSER_UI_COCOA_FULLSCREEN_PLACEHOLDER_VIEW_H_

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CoreImage.h>

@class FullscreenPlaceholderView;

@interface FullscreenPlaceholderView : NSView

// Formats the screenshot displayed on the tab content area when on fullscreen
- (id)initWithFrame:(NSRect)frameRect image:(CGImageRef)screenshot;

@end  // @interface FullscreenPlaceholderView : NSView

#endif  // CHROME_BROWSER_UI_COCOA_FULLSCREEN_PLACEHOLDER_VIEW_H_
