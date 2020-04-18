// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/web_state/ui/html_element_fetch_request.h"

#include "base/time/time.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface HTMLElementFetchRequest ()
// Completion handler to call with found DOM element.
@property(nonatomic, copy) void (^foundElementHandler)(NSDictionary*);
@end

@implementation HTMLElementFetchRequest

@synthesize creationTime = _creationTime;
@synthesize foundElementHandler = _foundElementHandler;

- (instancetype)initWithFoundElementHandler:
    (void (^)(NSDictionary*))foundElementHandler {
  self = [super init];
  if (self) {
    _creationTime = base::TimeTicks::Now();
    _foundElementHandler = foundElementHandler;
  }
  return self;
}

- (void)runHandlerWithResponse:(NSDictionary*)response {
  if (_foundElementHandler) {
    _foundElementHandler(response);
  }
}

- (void)invalidate {
  _foundElementHandler = nullptr;
}

@end
