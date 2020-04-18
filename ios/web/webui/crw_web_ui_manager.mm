// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/webui/crw_web_ui_manager.h"

#include <memory>
#include <vector>

#include "base/json/string_escape.h"
#import "base/mac/bind_objc_block.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/stringprintf.h"
#import "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "ios/web/grit/ios_web_resources.h"
#include "ios/web/public/browser_state.h"
#import "ios/web/public/web_client.h"
#import "ios/web/public/web_state/navigation_context.h"
#import "ios/web/public/web_state/web_state_observer_bridge.h"
#import "ios/web/web_state/web_state_impl.h"
#import "ios/web/webui/crw_web_ui_page_builder.h"
#import "ios/web/webui/url_fetcher_block_adapter.h"
#import "net/base/mac/url_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Prefix for JavaScript messages.
const char kScriptCommandPrefix[] = "webui";
}

@interface CRWWebUIManager () <CRWWebUIPageBuilderDelegate>

// Composes WebUI page for webUIURL and invokes completionHandler with the
// result.
- (void)loadWebUIPageForURL:(const GURL&)webUIURL
          completionHandler:(void (^)(NSString*))completionHandler;

// Retrieves resource for URL and invokes completionHandler with the result.
- (void)fetchResourceWithURL:(const GURL&)URL
           completionHandler:(void (^)(NSData*))completionHandler;

// Handles JavaScript message from the WebUI page.
- (BOOL)handleWebUIJSMessage:(const base::DictionaryValue&)message;

// Removes favicon callback from web state.
- (void)resetWebState;

// Removes fetcher from vector of active fetchers.
- (void)removeFetcher:(web::URLFetcherBlockAdapter*)fetcher;

@end

@implementation CRWWebUIManager {
  // Set of live WebUI fetchers for retrieving data.
  std::vector<std::unique_ptr<web::URLFetcherBlockAdapter>> _fetchers;
  // Bridge to observe the web state from Objective-C.
  std::unique_ptr<web::WebStateObserverBridge> _webStateObserverBridge;
  // The WebState this instance is observing. Will be null after
  // -webStateDestroyed: has been called.
  web::WebStateImpl* _webState;
}

- (instancetype)initWithWebState:(web::WebStateImpl*)webState {
  if (self = [super init]) {
    DCHECK(webState);
    _webState = webState;
    _webStateObserverBridge =
        std::make_unique<web::WebStateObserverBridge>(self);
    _webState->AddObserver(_webStateObserverBridge.get());

    __weak CRWWebUIManager* weakSelf = self;
    _webState->AddScriptCommandCallback(
        base::BindBlockArc(
            ^bool(const base::DictionaryValue& message, const GURL&, bool) {
              return [weakSelf handleWebUIJSMessage:message];
            }),
        kScriptCommandPrefix);
  }
  return self;
}

- (void)dealloc {
  [self resetWebState];
}

- (void)loadWebUIForURL:(const GURL&)URL {
  // If URL is not an application specific URL, ignore the navigation.
  GURL URLCopy(URL);
  if (!web::GetWebClient()->IsAppSpecificURL(URLCopy))
    return;

  __weak CRWWebUIManager* weakSelf = self;
  [self loadWebUIPageForURL:URLCopy
          completionHandler:^(NSString* HTML) {
            CRWWebUIManager* strongSelf = weakSelf;
            if (strongSelf && strongSelf->_webState) {
              strongSelf->_webState->LoadWebUIHtml(
                  base::SysNSStringToUTF16(HTML), URLCopy);
            }
          }];
}

#pragma mark - CRWWebStateObserver Methods

- (void)webState:(web::WebState*)webState didLoadPageWithSuccess:(BOOL)success {
  DCHECK_EQ(webState, _webState);
  // All WebUI pages are HTML based.
  _webState->SetContentsMimeType("text/html");
}

- (void)webStateDestroyed:(web::WebState*)webState {
  DCHECK_EQ(webState, _webState);
  [self resetWebState];
}

#pragma mark - CRWWebUIPageBuilderDelegate Methods

- (void)webUIPageBuilder:(CRWWebUIPageBuilder*)webUIPageBuilder
    fetchResourceWithURL:(const GURL&)resourceURL
       completionHandler:(web::WebUIDelegateCompletion)completionHandler {
  GURL URL(resourceURL);
  [self fetchResourceWithURL:URL
           completionHandler:^(NSData* data) {
             NSString* resource =
                 [[NSString alloc] initWithData:data
                                       encoding:NSUTF8StringEncoding];
             completionHandler(resource, URL);
           }];
}

#pragma mark - Private Methods

- (void)loadWebUIPageForURL:(const GURL&)webUIURL
          completionHandler:(void (^)(NSString*))handler {
  CRWWebUIPageBuilder* pageBuilder =
      [[CRWWebUIPageBuilder alloc] initWithDelegate:self];
  [pageBuilder buildWebUIPageForURL:webUIURL completionHandler:handler];
}

- (void)fetchResourceWithURL:(const GURL&)URL
           completionHandler:(void (^)(NSData*))completionHandler {
  __weak CRWWebUIManager* weakSelf = self;
  web::URLFetcherBlockAdapterCompletion fetcherCompletion =
      ^(NSData* data, web::URLFetcherBlockAdapter* fetcher) {
        completionHandler(data);
        [weakSelf removeFetcher:fetcher];
      };

  _fetchers.push_back(
      [self fetcherForURL:URL completionHandler:fetcherCompletion]);
  _fetchers.back()->Start();
}

- (BOOL)handleWebUIJSMessage:(const base::DictionaryValue&)message {
  std::string command;
  if (!message.GetString("message", &command)) {
    DLOG(WARNING) << "Malformed message received";
    return NO;
  }
  const base::ListValue* arguments = nullptr;
  if (!message.GetList("arguments", &arguments)) {
    DLOG(WARNING) << "JS message parameter not found: arguments";
    return NO;
  }

  if (!arguments) {
    DLOG(WARNING) << "No arguments provided to " << command;
    return NO;
  }

  DLOG(WARNING) << "Unknown webui command received: " << command;
  return NO;
}

- (void)resetWebState {
  if (_webState) {
    _webState->RemoveScriptCommandCallback(kScriptCommandPrefix);
    _webState->RemoveObserver(_webStateObserverBridge.get());
    _webState = nullptr;
  }
}

- (void)removeFetcher:(web::URLFetcherBlockAdapter*)fetcher {
  _fetchers.erase(std::find_if(
      _fetchers.begin(), _fetchers.end(),
      [fetcher](const std::unique_ptr<web::URLFetcherBlockAdapter>& ptr) {
        return ptr.get() == fetcher;
      }));
}

#pragma mark - Testing-Only Methods

- (std::unique_ptr<web::URLFetcherBlockAdapter>)
    fetcherForURL:(const GURL&)URL
completionHandler:(web::URLFetcherBlockAdapterCompletion)handler {
  return std::make_unique<web::URLFetcherBlockAdapter>(
      URL, _webState->GetBrowserState()->GetRequestContext(), handler);
}

@end
