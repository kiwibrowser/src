// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NATIVE_CONTENT_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_NATIVE_CONTENT_CONTROLLER_H_

#import <Foundation/Foundation.h>

#import "ios/web/public/web_state/ui/crw_native_content.h"

class GURL;
@class UIView;

// Abstract base class for controllers that implement the behavior for native
// views that are presented inside the web content area. Automatically removes
// |view| from the view hierarchy when it is destroyed. Subclasses are
// responsible for setting the view (usually through loading a nib) and the
// page title.
@interface NativeContentController : NSObject<CRWNativeContent>

// Top-level view.
@property(nonatomic, strong) IBOutlet UIView* view;
@property(nonatomic, copy) NSString* title;
@property(nonatomic, readonly, assign) const GURL& url;

// Initializer that attempts to load the nib specified in |nibName|, which may
// be nil. The |url| is the url to be loaded.
- (instancetype)initWithNibName:(NSString*)nibName
                            url:(const GURL&)url NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

// Initializer with the |url| to be loaded.
- (instancetype)initWithURL:(const GURL&)url;

@end

#endif  // IOS_CHROME_BROWSER_UI_NATIVE_CONTENT_CONTROLLER_H_
