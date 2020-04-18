// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_JS_CONTEXTUAL_SEARCH_MANAGER_H_
#define IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_JS_CONTEXTUAL_SEARCH_MANAGER_H_

#import <UIKit/UIKit.h>

#import "ios/web/public/web_state/js/crw_js_injection_manager.h"

#include "base/ios/block_types.h"

// JsContextualSearchManager manages the scripts related to contextual search.
@interface JsContextualSearchManager : CRWJSInjectionManager

// Asynchronously fetches the search context for |point| in the web view.
// |handler| is called back with the JSON-encoded search context, or an
// empty dictionary if no search context was found at |point|
- (void)fetchContextFromSelectionAtPoint:(CGPoint)point
                       completionHandler:(void (^)(NSString*))handler;

// Sets the JavaScript events listeners on selection change and DOM mutation.
// |mutationDelay| is the timeout of the DOM mutation events.
// |bodyTouchDelay| is the timeout of the body touch events.
- (void)enableEventListenersWithMutationDelay:(CGFloat)mutationDelay
                               bodyTouchDelay:(CGFloat)bodyTouchDelay;

// Disables the JavaScript events listeners on selection change and DOM
// mutation.
- (void)disableListeners;

// Expands the highlight zone to the specified range.
- (void)expandHighlightToStartOffset:(int)startOffset
                           endOffset:(int)endOffset
                   completionHandler:(web::JavaScriptResultBlock)completion;

// Retrieve the position of the highlight in the web view.
- (void)highlightRectsWithCompletionHandler:
    (web::JavaScriptResultBlock)completion;

// Clears the highlight info.
- (void)clearHighlight;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_JS_CONTEXTUAL_SEARCH_MANAGER_H_
