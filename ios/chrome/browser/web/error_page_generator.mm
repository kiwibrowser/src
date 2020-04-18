// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/error_page_generator.h"

#import "ios/chrome/browser/web/error_page_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ErrorPageGenerator {
  // Stores the HTML generated from the NSError in the initializer.
  NSString* _HTML;
}

- (instancetype)initWithError:(NSError*)error
                       isPost:(BOOL)isPost
                  isIncognito:(BOOL)isIncognito {
  self = [super init];
  if (self) {
    _HTML = GetErrorPage(error, isPost, isIncognito);
  }
  return self;
}

#pragma mark - HtmlGenerator

- (void)generateHtml:(HtmlCallback)callback {
  callback(_HTML);
}

@end
