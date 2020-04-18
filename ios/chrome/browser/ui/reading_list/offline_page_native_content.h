// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_READING_LIST_OFFLINE_PAGE_NATIVE_CONTENT_H_
#define IOS_CHROME_BROWSER_UI_READING_LIST_OFFLINE_PAGE_NATIVE_CONTENT_H_

#import "ios/chrome/browser/ui/static_content/static_html_native_content.h"

class GURL;

namespace web {
class BrowserState;
class WebState;
}

@protocol UrlLoader;

// A |StaticHtmlNativeContent| that displays offline pages located in the
// application data folder that have been distilled by DOMdistiller.
@interface OfflinePageNativeContent : StaticHtmlNativeContent

// Initialization method.
// |loader| is the loader to use to follow navigation. Cannot be nil.
// |browserState| is the user browser state and must not be null.
// |URL| is the url of the page. The format of the URL must be
// chrome://offline/distillation_id/page.html
// If |URL| contain a virtual URL in its query params, this will be returned by
// the |OfflinePageNativeContent virtualURL| method.
- (instancetype)initWithLoader:(id<UrlLoader>)loader
                  browserState:(web::BrowserState*)browserState
                      webState:(web::WebState*)webState
                           URL:(const GURL&)URL NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithLoader:(id<UrlLoader>)loader
      staticHTMLViewController:(StaticHtmlViewController*)HTMLViewController
                           URL:(const GURL&)URL NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_READING_LIST_OFFLINE_PAGE_NATIVE_CONTENT_H_
