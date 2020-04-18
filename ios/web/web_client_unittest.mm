// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/web_client.h"

#import <Foundation/Foundation.h>

#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using WebClientTest = PlatformTest;

// Tests WebClient::PrepareErrorPage method.
TEST_F(WebClientTest, PrepareErrorPage) {
  web::WebClient web_client;

  NSString* const description = @"a pretty bad error";
  NSError* error =
      [NSError errorWithDomain:NSURLErrorDomain
                          code:NSURLErrorNotConnectedToInternet
                      userInfo:@{NSLocalizedDescriptionKey : description}];

  NSString* html = nil;
  web_client.PrepareErrorPage(error, /*is_post=*/false,
                              /*is_off_the_record=*/false, &html);
  EXPECT_NSEQ(description, html);
}
