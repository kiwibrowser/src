// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_WEBUI_CRW_WEB_UI_MANAGER_H_
#define IOS_WEB_WEBUI_CRW_WEB_UI_MANAGER_H_

#import <Foundation/Foundation.h>

#include <memory>

#import "ios/web/public/web_state/web_state_observer_bridge.h"
#import "ios/web/webui/url_fetcher_block_adapter.h"

class GURL;

namespace web {
class WebStateImpl;
}  // namespace web

// Class for managing WebUI pages. Manages fetching of resources and post-load
// operations such as favicon loading. Initialized instances of CRWWebUIManager
// will automatically register as an observer of webState, and should be kept in
// scope for the lifetime of the WebUI page.
@interface CRWWebUIManager : NSObject<CRWWebStateObserver>

// Designated initializer.
- (instancetype)initWithWebState:(web::WebStateImpl*)webState;

// Starts loading WebUI for the given URL.
- (void)loadWebUIForURL:(const GURL&)URL;

@end

@interface CRWWebUIManager (UsedOnlyForTesting)  // Testing API.

// Returns URLFetcherBlockAdapter for fetching resource for URL. Can be
// overwritten by test classes to mock resource retrieval.
- (std::unique_ptr<web::URLFetcherBlockAdapter>)
    fetcherForURL:(const GURL&)URL
completionHandler:(web::URLFetcherBlockAdapterCompletion)handler;

@end

#endif  // IOS_WEB_WEBUI_CRW_WEB_UI_MANAGER_H_
