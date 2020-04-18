// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/core/url_utils.h"

#include "components/dom_distiller/core/url_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace dom_distiller {

namespace url_utils {

TEST(DomDistillerUrlUtilsTest, TestPathUtil) {
  const std::string single_key = "mypath?foo=bar";
  EXPECT_EQ("bar", GetValueForKeyInUrlPathQuery(single_key, "foo"));
  const std::string two_keys = "mypath?key1=foo&key2=bar";
  EXPECT_EQ("foo", GetValueForKeyInUrlPathQuery(two_keys, "key1"));
  EXPECT_EQ("bar", GetValueForKeyInUrlPathQuery(two_keys, "key2"));
  const std::string multiple_same_key = "mypath?key=foo&key=bar";
  EXPECT_EQ("foo", GetValueForKeyInUrlPathQuery(multiple_same_key, "key"));
}

TEST(DomDistillerUrlUtilsTest, TestGetValueForKeyInUrlPathQuery) {
  // Tests an invalid url.
  const std::string invalid_url = "http://%40[::1]/";
  EXPECT_EQ("", GetValueForKeyInUrlPathQuery(invalid_url, "key"));

  // Test a valid URL with the key we are searching for.
  const std::string valid_url_with_key = "http://www.google.com?key=foo";
  EXPECT_EQ("foo", GetValueForKeyInUrlPathQuery(valid_url_with_key, "key"));

  // Test a valid URL without the key we are searching for.
  const std::string valid_url_no_key = "http://www.google.com";
  EXPECT_EQ("", GetValueForKeyInUrlPathQuery(valid_url_no_key, "key"));

  // Test a valid URL with 2 values of the key we are searching for.
  const std::string valid_url_two_keys =
      "http://www.google.com?key=foo&key=bar";
  EXPECT_EQ("foo", GetValueForKeyInUrlPathQuery(valid_url_two_keys, "key"));
}

std::string ThroughDistiller(const std::string& url) {
  return GetOriginalUrlFromDistillerUrl(
      GetDistillerViewUrlFromUrl(kDomDistillerScheme, GURL(url), 123)).spec();
}

std::string GetOriginalUrlFromDistillerUrl(const std::string& url) {
  return GetOriginalUrlFromDistillerUrl(GURL(url)).spec();
}

TEST(DomDistillerUrlUtilsTest, TestDistillerEndToEnd) {
  // Tests a normal url.
  const std::string url = "http://example.com/";
  EXPECT_EQ(url, ThroughDistiller(url));
  EXPECT_EQ(url, GetOriginalUrlFromDistillerUrl(url));

  // Tests a url with arguments and anchor points.
  const std::string url_arguments =
      "https://example.com/?key=value&key=value2&key2=value3#here";
  EXPECT_EQ(url_arguments, ThroughDistiller(url_arguments));
  EXPECT_EQ(url_arguments, GetOriginalUrlFromDistillerUrl(url_arguments));

  // Tests a url with file:// scheme.
  const std::string url_file = "file:///home/userid/path/index.html";
  EXPECT_EQ(url_file, ThroughDistiller(url_file));
  EXPECT_EQ(url_file, GetOriginalUrlFromDistillerUrl(url_file));
}
}  // namespace url_utils

}  // namespace dom_distiller
