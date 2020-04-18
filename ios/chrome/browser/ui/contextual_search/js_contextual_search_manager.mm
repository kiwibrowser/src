// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/contextual_search/js_contextual_search_manager.h"

#include "base/mac/foundation_util.h"
#include "base/mac/scoped_block.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

static NSString* const kEnableSelectionChangeListener =
    @"__gCrWeb.contextualSearch.enableSelectionChangeListener(%@);";

static NSString* const kSetMutationObserverDelay =
    @"__gCrWeb.contextualSearch.setMutationObserverDelay(%f);";

static NSString* const kDisableMutationObserver =
    @"__gCrWeb.contextualSearch.disableMutationObserver();";

static NSString* const kSetBodyTouchListenerDelay =
    @"__gCrWeb.contextualSearch.setBodyTouchListenerDelay(%f);";

static NSString* const kDisableBodyTouchListener =
    @"__gCrWeb.contextualSearch.disableBodyTouchListener();";

static NSString* const kHandleCtxSearch =
    @"__gCrWeb.contextualSearch.handleTapAtPoint(%f, %f);";

static NSString* const kExpandHighlight =
    @"__gCrWeb.contextualSearch.expandHighlight(%d, %d);";

static NSString* const kHighlightRects =
    @"__gCrWeb.contextualSearch.highlightRects();";

static NSString* const kClearHighlight =
    @"__gCrWeb.contextualSearch.clearHighlight();";

@implementation JsContextualSearchManager

#pragma mark - Protected methods

- (NSString*)scriptPath {
  return @"contextualsearch";
}

#pragma mark - Public methods

- (void)fetchContextFromSelectionAtPoint:(CGPoint)point
                       completionHandler:(void (^)(NSString*))handler {
  NSString* handleContextualSearch =
      [NSString stringWithFormat:kHandleCtxSearch, point.x, point.y];

  web::JavaScriptResultBlock resultHandler = ^(id result, NSError* error) {
    if (error) {
      DLOG(ERROR) << "Error evaluating contextual search javascript: "
                  << base::SysNSStringToUTF8([error description]);
    }
    handler(base::mac::ObjCCastStrict<NSString>(result));
  };

  [self executeJavaScript:handleContextualSearch
        completionHandler:resultHandler];
}

- (void)enableEventListenersWithMutationDelay:(CGFloat)mutationDelay
                               bodyTouchDelay:(CGFloat)bodyTouchDelay {
  NSString* jsForwardString =
      [NSString stringWithFormat:kEnableSelectionChangeListener, @"true"];
  jsForwardString = [jsForwardString
      stringByAppendingFormat:kSetMutationObserverDelay, mutationDelay];
  if (bodyTouchDelay == 0) {
    jsForwardString =
        [jsForwardString stringByAppendingString:kDisableBodyTouchListener];
  } else {
    jsForwardString = [jsForwardString
        stringByAppendingFormat:kSetBodyTouchListenerDelay, bodyTouchDelay];
  }
  [self executeJavaScript:jsForwardString completionHandler:nil];
}

- (void)highlightRectsWithCompletionHandler:
    (web::JavaScriptResultBlock)completion {
  [self executeJavaScript:kHighlightRects completionHandler:completion];
}

- (void)clearHighlight {
  [self executeJavaScript:kClearHighlight completionHandler:nil];
}

- (void)disableListeners {
  NSString* jsForwardString =
      [NSString stringWithFormat:kEnableSelectionChangeListener, @"false"];

  jsForwardString =
      [jsForwardString stringByAppendingString:kDisableMutationObserver];
  jsForwardString =
      [jsForwardString stringByAppendingString:kDisableBodyTouchListener];
  [self executeJavaScript:jsForwardString completionHandler:nil];
}

- (void)expandHighlightToStartOffset:(int)startOffset
                           endOffset:(int)endOffset
                   completionHandler:(web::JavaScriptResultBlock)completion {
  if (startOffset < 0 || endOffset < 0)
    return;
  NSString* expandHightlightString =
      [NSString stringWithFormat:kExpandHighlight, startOffset, endOffset];

  [self executeJavaScript:expandHightlightString completionHandler:completion];
}

@end
