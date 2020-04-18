// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_ERROR_PAGE_GENERATOR_H_
#define IOS_CHROME_BROWSER_WEB_ERROR_PAGE_GENERATOR_H_

#import <Foundation/Foundation.h>

#import "ios/chrome/browser/ui/static_content/static_html_view_controller.h"

// This class is needed because StaticHtmlViewController retains the
// HtmlGenerator.
@interface ErrorPageGenerator : NSObject<HtmlGenerator>
- (instancetype)initWithError:(NSError*)error
                       isPost:(BOOL)isPost
                  isIncognito:(BOOL)isIncognito NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;
@end

#endif  // IOS_CHROME_BROWSER_WEB_ERROR_PAGE_GENERATOR_H_
