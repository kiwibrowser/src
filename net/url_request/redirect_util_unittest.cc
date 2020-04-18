// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/url_request/redirect_util.h"

#include <string>

#include "net/http/http_request_headers.h"
#include "net/url_request/redirect_info.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace net {
namespace {

TEST(RedirectUtilTest, UpdateHttpRequest) {
  const GURL original_url("https://www.example.com/test.php");
  const char* kOriginalMethod = "POST";
  const char* kCustomHeader = "Custom-Header-For-Test";
  const char* kCustomHeaderValue = "custom header value";

  struct TestCase {
    const char* new_method;
    const char* new_url;
    bool expected_should_clear_upload;
    // nullptr if the origin header should not exist
    const char* expected_origin_header;
  };
  const TestCase kTests[] = {
      {
          "POST" /* new_method */,
          "https://www.example.com/redirected.php" /* new_url */,
          false /* expected_should_clear_upload */,
          "https://origin.example.com" /* expected_origin_header */
      },
      {
          "GET" /* new_method */,
          "https://www.example.com/redirected.php" /* new_url */,
          true /* expected_should_clear_upload */,
          nullptr /* expected_origin_header */
      },
      {
          "POST" /* new_method */,
          "https://other.example.com/redirected.php" /* new_url */,
          false /* expected_should_clear_upload */,
          "null" /* expected_origin_header */
      },
      {
          "GET" /* new_method */,
          "https://other.example.com/redirected.php" /* new_url */,
          true /* expected_should_clear_upload */,
          nullptr /* expected_origin_header */
      }};

  for (const auto& test : kTests) {
    SCOPED_TRACE(::testing::Message() << "new_method: " << test.new_method
                                      << " new_url: " << test.new_url);
    RedirectInfo redirect_info;
    redirect_info.new_method = test.new_method;
    redirect_info.new_url = GURL(test.new_url);

    HttpRequestHeaders request_headers;
    request_headers.SetHeader(HttpRequestHeaders::kOrigin,
                              "https://origin.example.com");
    request_headers.SetHeader(HttpRequestHeaders::kContentLength, "100");
    request_headers.SetHeader(HttpRequestHeaders::kContentType,
                              "text/plain; charset=utf-8");
    request_headers.SetHeader(kCustomHeader, kCustomHeaderValue);

    bool should_clear_upload = !test.expected_should_clear_upload;

    RedirectUtil::UpdateHttpRequest(original_url, kOriginalMethod,
                                    redirect_info, &request_headers,
                                    &should_clear_upload);
    EXPECT_EQ(test.expected_should_clear_upload, should_clear_upload);
    EXPECT_EQ(!test.expected_should_clear_upload,
              request_headers.HasHeader(HttpRequestHeaders::kContentLength));
    EXPECT_EQ(!test.expected_should_clear_upload,
              request_headers.HasHeader(HttpRequestHeaders::kContentType));
    EXPECT_TRUE(request_headers.HasHeader(kCustomHeader));

    std::string origin_header_value;
    EXPECT_EQ(test.expected_origin_header != nullptr,
              request_headers.GetHeader(HttpRequestHeaders::kOrigin,
                                        &origin_header_value));
    if (test.expected_origin_header) {
      EXPECT_EQ(test.expected_origin_header, origin_header_value);
    }
  }
}

}  // namespace
}  // namespace net
