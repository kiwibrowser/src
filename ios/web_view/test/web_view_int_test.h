// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_TEST_WEB_VIEW_INT_TEST_H_
#define IOS_WEB_VIEW_TEST_WEB_VIEW_INT_TEST_H_

#import <Foundation/Foundation.h>
#include <memory>
#include <string>

#include "testing/platform_test.h"

NS_ASSUME_NONNULL_BEGIN

namespace net {
namespace test_server {
class EmbeddedTestServer;
}  // namespace test_server
}  // namespace net

@class CWVWebView;
class GURL;
@class NSURL;

namespace ios_web_view {

// A test fixture for testing CWVWebView. A test server is also created to
// support loading content. The server supports the urls returned by the GetUrl*
// methods below.
class WebViewIntTest : public PlatformTest {
 protected:
  WebViewIntTest();
  ~WebViewIntTest() override;

  // Returns URL to an html page with title set to |title|.
  GURL GetUrlForPageWithTitle(const std::string& title);

  // Returns URL to an html page with |html| within page's body tags.
  GURL GetUrlForPageWithHtmlBody(const std::string& html);

  // Returns URL to an html page with title set to |title| and |body| within
  // the page's body tags.
  GURL GetUrlForPageWithTitleAndBody(const std::string& title,
                                     const std::string& body);

  // PlatformTest methods.
  void SetUp() override;

  // CWVWebView created with default configuration and frame equal to screen
  // bounds.
  CWVWebView* web_view_;

  // Embedded server for handling requests sent to the URLs returned by the
  // GetURL* methods.
  std::unique_ptr<net::test_server::EmbeddedTestServer> test_server_;
};

}  // namespace ios_web_view

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_TEST_WEB_VIEW_INT_TEST_H_
