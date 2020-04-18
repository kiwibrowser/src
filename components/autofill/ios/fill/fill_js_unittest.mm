// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Foundation/Foundation.h>
#include <stddef.h>

#include "base/macros.h"
#include "base/strings/sys_string_conversions.h"
#import "ios/web/public/test/web_js_test.h"
#import "ios/web/public/test/web_test_with_web_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
class FillJsTest : public web::WebJsTest<web::WebTestWithWebState> {
 public:
  FillJsTest()
      : web::WebJsTest<web::WebTestWithWebState>(
            @[ @"chrome_bundle_main_frame" ]) {}
};

TEST_F(FillJsTest, GetCanonicalActionForForm) {
  struct TestData {
    NSString* html_action;
    NSString* expected_action;
  } test_data[] = {
      // Empty action.
      {nil, @"baseurl/"},
      // Absolute urls.
      {@"http://foo1.com/bar", @"http://foo1.com/bar"},
      {@"http://foo2.com/bar#baz", @"http://foo2.com/bar"},
      {@"http://foo3.com/bar?baz", @"http://foo3.com/bar"},
      // Relative urls.
      {@"action.php", @"baseurl/action.php"},
      {@"action.php?abc", @"baseurl/action.php"},
      // Non-http protocols.
      {@"data:abc", @"data:abc"},
      {@"javascript:login()", @"javascript:login()"},
  };

  for (size_t i = 0; i < arraysize(test_data); i++) {
    TestData& data = test_data[i];
    NSString* html_action =
        data.html_action == nil
            ? @""
            : [NSString stringWithFormat:@"action='%@'", data.html_action];
    NSString* html = [NSString stringWithFormat:
                                   @"<html><body>"
                                    "<form %@></form>"
                                    "</body></html>",
                                   html_action];

    LoadHtmlAndInject(html);
    id result = ExecuteJavaScript(
        @"__gCrWeb.fill.getCanonicalActionForForm(document.body.children[0])");
    NSString* base_url = base::SysUTF8ToNSString(BaseUrl());
    NSString* expected_action =
        [data.expected_action stringByReplacingOccurrencesOfString:@"baseurl/"
                                                        withString:base_url];
    EXPECT_NSEQ(expected_action, result)
        << " in test " << i << ": "
        << base::SysNSStringToUTF8(data.html_action);
  }
}

}  // namespace
