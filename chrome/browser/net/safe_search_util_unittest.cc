// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/safe_search_util.h"

#include "base/message_loop/message_loop.h"
#include "base/strings/string_piece.h"
#include "chrome/common/url_constants.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class SafeSearchUtilTest : public ::testing::Test {
 protected:
  SafeSearchUtilTest() {}
  ~SafeSearchUtilTest() override {}

  std::unique_ptr<net::URLRequest> CreateRequest(const std::string& url) {
    return context_.CreateRequest(GURL(url), net::DEFAULT_PRIORITY, NULL,
                                  TRAFFIC_ANNOTATION_FOR_TESTS);
  }

  std::unique_ptr<net::URLRequest> CreateYoutubeRequest() {
    return CreateRequest("http://www.youtube.com");
  }

  std::unique_ptr<net::URLRequest> CreateNonYoutubeRequest() {
    return CreateRequest("http://www.notyoutube.com");
  }

  // Does a request using the |url_string| URL and verifies that the expected
  // string is equal to the query part (between ? and #) of the final url of
  // that request.
  void CheckAddedParameters(const std::string& url_string,
                            const std::string& expected_query_parameters) {
    // Show the URL in the trace so we know where we failed.
    SCOPED_TRACE(url_string);

    std::unique_ptr<net::URLRequest> request(CreateRequest(url_string));
    GURL result(url_string);
    safe_search_util::ForceGoogleSafeSearch(request.get(), &result);

    EXPECT_EQ(expected_query_parameters, result.query());
  }

  base::MessageLoop message_loop_;
  net::TestURLRequestContext context_;
};

TEST_F(SafeSearchUtilTest, AddGoogleSafeSearchParams) {
  const std::string kSafeParameter = chrome::kSafeSearchSafeParameter;
  const std::string kSsuiParameter = chrome::kSafeSearchSsuiParameter;
  const std::string kBothParameters = kSafeParameter + "&" + kSsuiParameter;

  // Test the home page.
  CheckAddedParameters("http://google.com/", kBothParameters);

  // Test the search home page.
  CheckAddedParameters("http://google.com/webhp",
                       kBothParameters);

  // Test different valid search pages with parameters.
  CheckAddedParameters("http://google.com/search?q=google",
                       "q=google&" + kBothParameters);

  CheckAddedParameters("http://google.com/?q=google",
                       "q=google&" + kBothParameters);

  CheckAddedParameters("http://google.com/webhp?q=google",
                       "q=google&" + kBothParameters);

  // Test the valid pages with safe set to off.
  CheckAddedParameters("http://google.com/search?q=google&safe=off",
                       "q=google&" + kBothParameters);

  CheckAddedParameters("http://google.com/?q=google&safe=off",
                       "q=google&" + kBothParameters);

  CheckAddedParameters("http://google.com/webhp?q=google&safe=off",
                       "q=google&" + kBothParameters);

  CheckAddedParameters("http://google.com/webhp?q=google&%73afe=off",
                       "q=google&%73afe=off&" + kBothParameters);

  // Test the home page, different TLDs.
  CheckAddedParameters("http://google.de/", kBothParameters);
  CheckAddedParameters("http://google.ro/", kBothParameters);
  CheckAddedParameters("http://google.nl/", kBothParameters);

  // Test the search home page, different TLD.
  CheckAddedParameters("http://google.de/webhp", kBothParameters);

  // Test the search page with parameters, different TLD.
  CheckAddedParameters("http://google.de/search?q=google",
                       "q=google&" + kBothParameters);

  // Test the home page with parameters, different TLD.
  CheckAddedParameters("http://google.de/?q=google",
                       "q=google&" + kBothParameters);

  // Test the search page with the parameters set.
  CheckAddedParameters("http://google.de/?q=google&" + kBothParameters,
                       "q=google&" + kBothParameters);

  // Test some possibly tricky combinations.
  CheckAddedParameters("http://google.com/?q=goog&" + kSafeParameter +
                       "&ssui=one",
                       "q=goog&" + kBothParameters);

  CheckAddedParameters("http://google.de/?q=goog&unsafe=active&" +
                       kSsuiParameter,
                       "q=goog&unsafe=active&" + kBothParameters);

  CheckAddedParameters("http://google.de/?q=goog&safe=off&ssui=off",
                       "q=goog&" + kBothParameters);

  CheckAddedParameters("http://google.de/?q=&tbs=rimg:",
                       "q=&tbs=rimg:&" + kBothParameters);

  // Test various combinations where we should not add anything.
  CheckAddedParameters("http://google.com/?q=goog&" + kSsuiParameter + "&" +
                       kSafeParameter,
                       "q=goog&" + kBothParameters);

  CheckAddedParameters("http://google.com/?" + kSsuiParameter + "&q=goog&" +
                       kSafeParameter,
                       "q=goog&" + kBothParameters);

  CheckAddedParameters("http://google.com/?" + kSsuiParameter + "&" +
                       kSafeParameter + "&q=goog",
                       "q=goog&" + kBothParameters);

  // Test that another website is not affected, without parameters.
  CheckAddedParameters("http://google.com/finance", std::string());

  // Test that another website is not affected, with parameters.
  CheckAddedParameters("http://google.com/finance?q=goog", "q=goog");

  // Test with percent-encoded data (%26 is &)
  CheckAddedParameters("http://google.com/?q=%26%26%26&" + kSsuiParameter +
                       "&" + kSafeParameter + "&param=%26%26%26",
                       "q=%26%26%26&param=%26%26%26&" + kBothParameters);
}

TEST_F(SafeSearchUtilTest, SetYoutubeHeader) {
  std::unique_ptr<net::URLRequest> request = CreateYoutubeRequest();
  net::HttpRequestHeaders headers;
  safe_search_util::ForceYouTubeRestrict(
    request.get(), &headers, safe_search_util::YOUTUBE_RESTRICT_MODERATE);
  std::string value;
  EXPECT_TRUE(headers.GetHeader("Youtube-Restrict", &value));
  EXPECT_EQ("Moderate", value);
}

TEST_F(SafeSearchUtilTest, OverrideYoutubeHeader) {
  std::unique_ptr<net::URLRequest> request = CreateYoutubeRequest();
  net::HttpRequestHeaders headers;
  headers.SetHeader("Youtube-Restrict", "Off");
  safe_search_util::ForceYouTubeRestrict(
    request.get(), &headers, safe_search_util::YOUTUBE_RESTRICT_MODERATE);
  std::string value;
  EXPECT_TRUE(headers.GetHeader("Youtube-Restrict", &value));
  EXPECT_EQ("Moderate", value);
}

TEST_F(SafeSearchUtilTest, DoesntTouchNonYoutubeURL) {
  std::unique_ptr<net::URLRequest> request = CreateNonYoutubeRequest();
  net::HttpRequestHeaders headers;
  headers.SetHeader("Youtube-Restrict", "Off");
  safe_search_util::ForceYouTubeRestrict(
    request.get(), &headers, safe_search_util::YOUTUBE_RESTRICT_MODERATE);
  std::string value;
  EXPECT_TRUE(headers.GetHeader("Youtube-Restrict", &value));
  EXPECT_EQ("Off", value);
}
