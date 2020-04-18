// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_WEBUI_CRW_WEB_UI_PAGE_BUILDER_H_
#define IOS_WEB_WEBUI_CRW_WEB_UI_PAGE_BUILDER_H_

#import <Foundation/Foundation.h>

#include "url/gurl.h"

namespace web {
// Block type for WebUI page build completion callback.
typedef void (^WebUIPageCompletion)(NSString*);
// Block type for WebUI page builder delegate completion callback.
typedef void (^WebUIDelegateCompletion)(NSString*, const GURL&);
}  // namespace web

@class CRWWebUIPageBuilder;

// Delegate for CRWWebUIPageBuilder. Handles resource retrieval.
@protocol CRWWebUIPageBuilderDelegate<NSObject>
// Fetches resource for resourceURL and invokes completionHandler with the
// result.
- (void)webUIPageBuilder:(CRWWebUIPageBuilder*)webUIPageBuilder
    fetchResourceWithURL:(const GURL&)resourceURL
       completionHandler:(web::WebUIDelegateCompletion)completionHandler;
@end

// Class for requesting and building WebUI HTML pages. Due to limitations on
// custom network protocols for WKWebView, it is not possible to directly load
// WebUI in WKWebView via the network stack. Instead, WebUI resources are
// manually fetched from the network stack and then flattened and loaded into
// the web view.
//
// To flatten the WebUI resources, tags for JS and CSS resources are replaced
// with retrieved resources. For example, if a WebUI HTML resource looks like:
// <html>
//   <script src='chrome://javascript.js></script>
//   <link rel="stylesheet" href="chrome://stylesheet.css">
// </html>
// the flattened version would look like:
// <html>
//   <script>[Content of chrome://javascript.js]</script>
//   <style>[Content of chrome://stylesheet.css]</style>
// </html>
@interface CRWWebUIPageBuilder : NSObject

// Designated initializer.
- (instancetype)initWithDelegate:(id<CRWWebUIPageBuilderDelegate>)delegate;

// Builds and flattens page for webUIURL, and invokes completionHandler with the
// result.
- (void)buildWebUIPageForURL:(const GURL&)webUIURL
           completionHandler:(web::WebUIPageCompletion)completionHandler;

@end

#endif  // IOS_WEB_WEBUI_CRW_WEB_UI_PAGE_BUILDER_H_
