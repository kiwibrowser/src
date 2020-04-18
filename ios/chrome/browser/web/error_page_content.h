// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_ERROR_PAGE_CONTENT_H_
#define IOS_CHROME_BROWSER_WEB_ERROR_PAGE_CONTENT_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/static_content/static_html_native_content.h"

namespace web {
class BrowserState;
}

@interface ErrorPageContent : StaticHtmlNativeContent

// Initialization. |loader| and |url| are passed up to StaticHtmlNativeContent;
// |loader| cannot be nil.
// |browserState| is the user browser state and must not be null.
// |error| (in conjunction with |isPost| and |isIncognito|) is used to generate
// an HTML page that will be stored in the HTML generator object.
- (id)initWithLoader:(id<UrlLoader>)loader
        browserState:(web::BrowserState*)browserState
                 url:(const GURL&)url
               error:(NSError*)error
              isPost:(BOOL)isPost
         isIncognito:(BOOL)isIncognito;
@end

#endif  // IOS_CHROME_BROWSER_WEB_ERROR_PAGE_CONTENT_H_
