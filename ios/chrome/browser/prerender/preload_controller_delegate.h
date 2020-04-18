// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PRERENDER_PRELOAD_CONTROLLER_DELEGATE_H_
#define IOS_CHROME_BROWSER_PRERENDER_PRELOAD_CONTROLLER_DELEGATE_H_

#import <UIKit/UIKit.h>

@class PreloadController;

// A protocol implemented by a delegate of PreloadController
@protocol PreloadControllerDelegate

// Should preload controller request a desktop site.
- (BOOL)preloadShouldUseDesktopUserAgent;

// Returns YES if the given |url| should be backed by a native controller.
- (BOOL)preloadHasNativeControllerForURL:(const GURL&)url;

// Called to retrieve the height of any header that is overlaying on top of the
// native content.
- (CGFloat)
nativeContentHeaderHeightForPreloadController:(PreloadController*)controller
                                     webState:(web::WebState*)webState;

@end

#endif  // IOS_CHROME_BROWSER_PRERENDER_PRELOAD_CONTROLLER_DELEGATE_H_
