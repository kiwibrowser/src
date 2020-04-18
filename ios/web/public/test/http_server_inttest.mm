// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Foundation/Foundation.h>

#include <memory>
#include <string>

#include "base/strings/sys_string_conversions.h"
#import "base/test/ios/wait_util.h"
#import "ios/web/public/test/http_server/http_server.h"
#import "ios/web/public/test/http_server/string_response_provider.h"
#import "ios/web/test/web_int_test.h"
#import "net/base/mac/url_conversions.h"
#include "net/http/http_response_headers.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// A test fixture for verifying the behavior of web::test::HttpServer.
typedef web::WebIntTest HttpServerTest;

// Tests that a web::test::HttpServer can be started and can send and receive
// requests and response from |TestResponseProvider|.
TEST_F(HttpServerTest, StartAndInterfaceWithResponseProvider) {
  const std::string kHelloWorld = "Hello World";
  std::unique_ptr<web::StringResponseProvider> provider(
      new web::StringResponseProvider(kHelloWorld));

  web::test::HttpServer& server = web::test::HttpServer::GetSharedInstance();
  ASSERT_TRUE(server.IsRunning());
  server.AddResponseProvider(std::move(provider));

  __block NSString* page_result;
  id completion_handler =
      ^(NSData* data, NSURLResponse* response, NSError* error) {
        page_result =
            [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
      };
  NSURL* url = net::NSURLWithGURL(server.MakeUrl("http://whatever"));
  NSURLSessionDataTask* data_task =
      [[NSURLSession sharedSession] dataTaskWithURL:url
                                  completionHandler:completion_handler];
  [data_task resume];
  base::test::ios::WaitUntilCondition(^bool() {
    return page_result;
  });
  EXPECT_NSEQ(page_result, base::SysUTF8ToNSString(kHelloWorld));
}
