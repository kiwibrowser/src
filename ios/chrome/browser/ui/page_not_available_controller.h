// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAGE_NOT_AVAILABLE_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_PAGE_NOT_AVAILABLE_CONTROLLER_H_

#import "ios/chrome/browser/ui/native_content_controller.h"

// A native controller for the case where the URL is not valid and/or the page
// is not available.
@interface PageNotAvailableController : NativeContentController

// Property to allow setting a custom description for the content.
@property(nonatomic, copy) NSString* descriptionText;

// Designated initializer.
- (instancetype)initWithUrl:(const GURL&)url NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithNibName:(NSString*)nibName
                            url:(const GURL&)url NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAGE_NOT_AVAILABLE_CONTROLLER_H_
