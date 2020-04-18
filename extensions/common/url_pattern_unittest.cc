// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/url_pattern.h"

#include <stddef.h>

#include <memory>

#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "extensions/common/constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

// See url_pattern.h for examples of valid and invalid patterns.

static const int kAllSchemes =
    URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS |
    URLPattern::SCHEME_FILE | URLPattern::SCHEME_FTP |
    URLPattern::SCHEME_CHROMEUI | URLPattern::SCHEME_EXTENSION |
    URLPattern::SCHEME_FILESYSTEM | URLPattern::SCHEME_WS |
    URLPattern::SCHEME_WSS | URLPattern::SCHEME_DATA;

TEST(ExtensionURLPatternTest, ParseInvalid) {
  const struct {
    const char* pattern;
    URLPattern::ParseResult expected_result;
  } kInvalidPatterns[] = {
      {"http", URLPattern::PARSE_ERROR_MISSING_SCHEME_SEPARATOR},
      {"http:", URLPattern::PARSE_ERROR_WRONG_SCHEME_SEPARATOR},
      {"http:/", URLPattern::PARSE_ERROR_WRONG_SCHEME_SEPARATOR},
      {"about://", URLPattern::PARSE_ERROR_WRONG_SCHEME_SEPARATOR},
      {"http://", URLPattern::PARSE_ERROR_EMPTY_HOST},
      {"http:///", URLPattern::PARSE_ERROR_EMPTY_HOST},
      {"http:// /", URLPattern::PARSE_ERROR_EMPTY_HOST},
      {"http://:1234/", URLPattern::PARSE_ERROR_EMPTY_HOST},
      {"http://*foo/bar", URLPattern::PARSE_ERROR_INVALID_HOST_WILDCARD},
      {"http://foo.*.bar/baz", URLPattern::PARSE_ERROR_INVALID_HOST_WILDCARD},
      {"http://fo.*.ba:123/baz", URLPattern::PARSE_ERROR_INVALID_HOST_WILDCARD},
      {"http:/bar", URLPattern::PARSE_ERROR_WRONG_SCHEME_SEPARATOR},
      {"http://bar", URLPattern::PARSE_ERROR_EMPTY_PATH},
      {"http://foo.*/bar", URLPattern::PARSE_ERROR_INVALID_HOST_WILDCARD}};

  for (size_t i = 0; i < arraysize(kInvalidPatterns); ++i) {
    URLPattern pattern(URLPattern::SCHEME_ALL);
    EXPECT_EQ(kInvalidPatterns[i].expected_result,
              pattern.Parse(kInvalidPatterns[i].pattern))
        << kInvalidPatterns[i].pattern;
  }

  {
    // Cannot use a C string, because this contains a null byte.
    std::string null_host("http://\0www/", 12);
    URLPattern pattern(URLPattern::SCHEME_ALL);
    EXPECT_EQ(URLPattern::PARSE_ERROR_INVALID_HOST,
              pattern.Parse(null_host))
        << null_host;
  }
}

TEST(ExtensionURLPatternTest, Ports) {
  const struct {
    const char* pattern;
    URLPattern::ParseResult expected_result;
    const char* expected_port;
  } kTestPatterns[] = {
      {"http://foo:1234/", URLPattern::PARSE_SUCCESS, "1234"},
      {"http://foo:1234/bar", URLPattern::PARSE_SUCCESS, "1234"},
      {"http://*.foo:1234/", URLPattern::PARSE_SUCCESS, "1234"},
      {"http://*.foo:1234/bar", URLPattern::PARSE_SUCCESS, "1234"},
      {"http://foo:/", URLPattern::PARSE_ERROR_INVALID_PORT, "*"},
      {"http://*:1234/", URLPattern::PARSE_SUCCESS, "1234"},
      {"http://*:*/", URLPattern::PARSE_SUCCESS, "*"},
      {"http://foo:*/", URLPattern::PARSE_SUCCESS, "*"},
      {"http://*.foo:/", URLPattern::PARSE_ERROR_INVALID_PORT, "*"},
      {"http://foo:com/", URLPattern::PARSE_ERROR_INVALID_PORT, "*"},
      {"http://foo:123456/", URLPattern::PARSE_ERROR_INVALID_PORT, "*"},
      {"http://foo:80:80/monkey", URLPattern::PARSE_ERROR_INVALID_PORT, "*"},
      {"file://foo:1234/bar", URLPattern::PARSE_SUCCESS, "*"},
      {"chrome://foo:1234/bar", URLPattern::PARSE_ERROR_INVALID_PORT, "*"},

      // Port-like strings in the path should not trigger a warning.
      {"http://*/:1234", URLPattern::PARSE_SUCCESS, "*"},
      {"http://*.foo/bar:1234", URLPattern::PARSE_SUCCESS, "*"},
      {"http://foo/bar:1234/path", URLPattern::PARSE_SUCCESS, "*"},
      {"http://*.foo.*/:1234", URLPattern::PARSE_SUCCESS, "*"}};

  for (size_t i = 0; i < arraysize(kTestPatterns); ++i) {
    URLPattern pattern(URLPattern::SCHEME_ALL);
    EXPECT_EQ(kTestPatterns[i].expected_result,
              pattern.Parse(kTestPatterns[i].pattern,
                            URLPattern::ALLOW_WILDCARD_FOR_EFFECTIVE_TLD))
        << "Got unexpected result for URL pattern: "
        << kTestPatterns[i].pattern;
    EXPECT_EQ(kTestPatterns[i].expected_port, pattern.port())
        << "Got unexpected port for URL pattern: " << kTestPatterns[i].pattern;
  }
}

// all pages for a given scheme
TEST(ExtensionURLPatternTest, Match1) {
  URLPattern pattern(kAllSchemes);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern.Parse("http://*/*"));
  EXPECT_EQ("http", pattern.scheme());
  EXPECT_EQ("", pattern.host());
  EXPECT_TRUE(pattern.match_subdomains());
  EXPECT_TRUE(pattern.match_effective_tld());
  EXPECT_FALSE(pattern.match_all_urls());
  EXPECT_EQ("/*", pattern.path());
  EXPECT_TRUE(pattern.MatchesURL(GURL("http://google.com")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("http://yahoo.com")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("http://google.com/foo")));
  EXPECT_FALSE(pattern.MatchesURL(GURL("https://google.com")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("http://74.125.127.100/search")));
}

// all domains
TEST(ExtensionURLPatternTest, Match2) {
  URLPattern pattern(kAllSchemes);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern.Parse("https://*/foo*"));
  EXPECT_EQ("https", pattern.scheme());
  EXPECT_EQ("", pattern.host());
  EXPECT_TRUE(pattern.match_subdomains());
  EXPECT_TRUE(pattern.match_effective_tld());
  EXPECT_FALSE(pattern.match_all_urls());
  EXPECT_EQ("/foo*", pattern.path());
  EXPECT_TRUE(pattern.MatchesURL(GURL("https://www.google.com/foo")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("https://www.google.com/foobar")));
  EXPECT_FALSE(pattern.MatchesURL(GURL("http://www.google.com/foo")));
  EXPECT_FALSE(pattern.MatchesURL(GURL("https://www.google.com/")));
  EXPECT_TRUE(pattern.MatchesURL(
      GURL("filesystem:https://www.google.com/foobar/")));
}

// subdomains
TEST(URLPatternTest, Match3) {
  URLPattern pattern(kAllSchemes);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS,
            pattern.Parse("http://*.google.com/foo*bar"));
  EXPECT_EQ("http", pattern.scheme());
  EXPECT_EQ("google.com", pattern.host());
  EXPECT_TRUE(pattern.match_subdomains());
  EXPECT_TRUE(pattern.match_effective_tld());
  EXPECT_FALSE(pattern.match_all_urls());
  EXPECT_EQ("/foo*bar", pattern.path());
  EXPECT_TRUE(pattern.MatchesURL(GURL("http://google.com/foobar")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("http://www.google.com/foo?bar")));
  EXPECT_TRUE(pattern.MatchesURL(
      GURL("http://monkey.images.google.com/foooobar")));
  EXPECT_FALSE(pattern.MatchesURL(GURL("http://yahoo.com/foobar")));
  EXPECT_TRUE(pattern.MatchesURL(
      GURL("filesystem:http://google.com/foo/bar")));
  EXPECT_FALSE(pattern.MatchesURL(
      GURL("filesystem:http://google.com/temporary/foobar")));
}

// glob escaping
TEST(ExtensionURLPatternTest, Match5) {
  URLPattern pattern(kAllSchemes);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern.Parse("file:///foo?bar\\*baz"));
  EXPECT_EQ("file", pattern.scheme());
  EXPECT_EQ("", pattern.host());
  EXPECT_FALSE(pattern.match_subdomains());
  EXPECT_TRUE(pattern.match_effective_tld());
  EXPECT_FALSE(pattern.match_all_urls());
  EXPECT_EQ("/foo?bar\\*baz", pattern.path());
  EXPECT_TRUE(pattern.MatchesURL(GURL("file:///foo?bar\\hellobaz")));
  EXPECT_FALSE(pattern.MatchesURL(GURL("file:///fooXbar\\hellobaz")));
}

// ip addresses
TEST(ExtensionURLPatternTest, Match6) {
  URLPattern pattern(kAllSchemes);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern.Parse("http://127.0.0.1/*"));
  EXPECT_EQ("http", pattern.scheme());
  EXPECT_EQ("127.0.0.1", pattern.host());
  EXPECT_FALSE(pattern.match_subdomains());
  EXPECT_TRUE(pattern.match_effective_tld());
  EXPECT_FALSE(pattern.match_all_urls());
  EXPECT_EQ("/*", pattern.path());
  EXPECT_TRUE(pattern.MatchesURL(GURL("http://127.0.0.1")));
}

// subdomain matching with ip addresses
TEST(ExtensionURLPatternTest, Match7) {
  URLPattern pattern(kAllSchemes);
  // allowed, but useless
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern.Parse("http://*.0.0.1/*"));
  EXPECT_EQ("http", pattern.scheme());
  // Canonicalization forces 0.0.1 to 0.0.0.1.
  EXPECT_EQ("0.0.0.1", pattern.host());
  EXPECT_TRUE(pattern.match_subdomains());
  EXPECT_TRUE(pattern.match_effective_tld());
  EXPECT_FALSE(pattern.match_all_urls());
  EXPECT_EQ("/*", pattern.path());
  // Subdomain matching is never done if the argument has an IP address host.
  EXPECT_FALSE(pattern.MatchesURL(GURL("http://127.0.0.1")));
}

// unicode
TEST(ExtensionURLPatternTest, Match8) {
  URLPattern pattern(kAllSchemes);
  // The below is the ASCII encoding of the following URL:
  // http://*.\xe1\x80\xbf/a\xc2\x81\xe1*
  EXPECT_EQ(URLPattern::PARSE_SUCCESS,
            pattern.Parse("http://*.xn--gkd/a%C2%81%E1*"));
  EXPECT_EQ("http", pattern.scheme());
  EXPECT_EQ("xn--gkd", pattern.host());
  EXPECT_TRUE(pattern.match_subdomains());
  EXPECT_TRUE(pattern.match_effective_tld());
  EXPECT_FALSE(pattern.match_all_urls());
  EXPECT_EQ("/a%C2%81%E1*", pattern.path());
  EXPECT_TRUE(pattern.MatchesURL(
      GURL("http://abc.\xe1\x80\xbf/a\xc2\x81\xe1xyz")));
  EXPECT_TRUE(pattern.MatchesURL(
      GURL("http://\xe1\x80\xbf/a\xc2\x81\xe1\xe1")));
}

// chrome://
TEST(ExtensionURLPatternTest, Match9) {
  URLPattern pattern(kAllSchemes);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern.Parse("chrome://favicon/*"));
  EXPECT_EQ("chrome", pattern.scheme());
  EXPECT_EQ("favicon", pattern.host());
  EXPECT_FALSE(pattern.match_subdomains());
  EXPECT_TRUE(pattern.match_effective_tld());
  EXPECT_FALSE(pattern.match_all_urls());
  EXPECT_EQ("/*", pattern.path());
  EXPECT_TRUE(pattern.MatchesURL(GURL("chrome://favicon/http://google.com")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("chrome://favicon/https://google.com")));
  EXPECT_FALSE(pattern.MatchesURL(GURL("chrome://history")));
}

// *://
TEST(ExtensionURLPatternTest, Match10) {
  URLPattern pattern(kAllSchemes);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern.Parse("*://*/*"));
  EXPECT_TRUE(pattern.MatchesScheme("http"));
  EXPECT_TRUE(pattern.MatchesScheme("https"));
  EXPECT_FALSE(pattern.MatchesScheme("chrome"));
  EXPECT_FALSE(pattern.MatchesScheme("file"));
  EXPECT_FALSE(pattern.MatchesScheme("ftp"));
  EXPECT_TRUE(pattern.match_subdomains());
  EXPECT_TRUE(pattern.match_effective_tld());
  EXPECT_FALSE(pattern.match_all_urls());
  EXPECT_EQ("/*", pattern.path());
  EXPECT_TRUE(pattern.MatchesURL(GURL("http://127.0.0.1")));
  EXPECT_FALSE(pattern.MatchesURL(GURL("chrome://favicon/http://google.com")));
  EXPECT_FALSE(pattern.MatchesURL(GURL("file:///foo/bar")));
  EXPECT_FALSE(pattern.MatchesURL(GURL("file://localhost/foo/bar")));
}

// <all_urls>
TEST(ExtensionURLPatternTest, Match11) {
  URLPattern pattern(kAllSchemes);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern.Parse("<all_urls>"));
  EXPECT_TRUE(pattern.MatchesScheme("chrome"));
  EXPECT_TRUE(pattern.MatchesScheme("http"));
  EXPECT_TRUE(pattern.MatchesScheme("https"));
  EXPECT_TRUE(pattern.MatchesScheme("file"));
  EXPECT_TRUE(pattern.MatchesScheme("filesystem"));
  EXPECT_TRUE(pattern.MatchesScheme(extensions::kExtensionScheme));
  EXPECT_TRUE(pattern.match_subdomains());
  EXPECT_TRUE(pattern.match_effective_tld());
  EXPECT_TRUE(pattern.match_all_urls());
  EXPECT_EQ("/*", pattern.path());
  EXPECT_TRUE(pattern.MatchesURL(GURL("chrome://favicon/http://google.com")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("http://127.0.0.1")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("file:///foo/bar")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("file://localhost/foo/bar")));

  // Make sure the properties are the same when creating an <all_urls> pattern
  // via SetMatchAllURLs and by parsing <all_urls>.
  URLPattern pattern2(kAllSchemes);
  pattern2.SetMatchAllURLs(true);

  EXPECT_EQ(pattern.valid_schemes(), pattern2.valid_schemes());
  EXPECT_EQ(pattern.match_subdomains(), pattern2.match_subdomains());
  EXPECT_EQ(pattern.path(), pattern2.path());
  EXPECT_EQ(pattern.match_all_urls(), pattern2.match_all_urls());
  EXPECT_EQ(pattern.scheme(), pattern2.scheme());
  EXPECT_EQ(pattern.port(), pattern2.port());
  EXPECT_EQ(pattern.GetAsString(), pattern2.GetAsString());
}

// SCHEME_ALL matches all schemes.
TEST(ExtensionURLPatternTest, Match12) {
  URLPattern pattern(URLPattern::SCHEME_ALL);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern.Parse("<all_urls>"));
  EXPECT_TRUE(pattern.MatchesScheme("chrome"));
  EXPECT_TRUE(pattern.MatchesScheme("http"));
  EXPECT_TRUE(pattern.MatchesScheme("https"));
  EXPECT_TRUE(pattern.MatchesScheme("file"));
  EXPECT_TRUE(pattern.MatchesScheme("filesystem"));
  EXPECT_TRUE(pattern.MatchesScheme("javascript"));
  EXPECT_TRUE(pattern.MatchesScheme("data"));
  EXPECT_TRUE(pattern.MatchesScheme("about"));
  EXPECT_TRUE(pattern.MatchesScheme(extensions::kExtensionScheme));
  EXPECT_TRUE(pattern.match_subdomains());
  EXPECT_TRUE(pattern.match_effective_tld());
  EXPECT_TRUE(pattern.match_all_urls());
  EXPECT_EQ("/*", pattern.path());
  EXPECT_TRUE(pattern.MatchesURL(GURL("chrome://favicon/http://google.com")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("http://127.0.0.1")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("file:///foo/bar")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("file://localhost/foo/bar")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("chrome://newtab")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("about:blank")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("about:version")));
  EXPECT_TRUE(pattern.MatchesURL(
      GURL("data:text/html;charset=utf-8,<html>asdf</html>")));
}

static const struct MatchPatterns {
  const char* pattern;
  const char* matches;
} kMatch13UrlPatternTestCases[] = {
  {"about:*", "about:blank"},
  {"about:blank", "about:blank"},
  {"about:*", "about:version"},
  {"chrome-extension://*/*", "chrome-extension://FTW"},
  {"data:*", "data:monkey"},
  {"javascript:*", "javascript:atemyhomework"},
};

// SCHEME_ALL and specific schemes.
TEST(ExtensionURLPatternTest, Match13) {
  for (size_t i = 0; i < arraysize(kMatch13UrlPatternTestCases); ++i) {
    URLPattern pattern(URLPattern::SCHEME_ALL);
    EXPECT_EQ(URLPattern::PARSE_SUCCESS,
              pattern.Parse(kMatch13UrlPatternTestCases[i].pattern))
        << " while parsing " << kMatch13UrlPatternTestCases[i].pattern;
    EXPECT_TRUE(pattern.MatchesURL(
        GURL(kMatch13UrlPatternTestCases[i].matches)))
        << " while matching " << kMatch13UrlPatternTestCases[i].matches;
  }

  // Negative test.
  URLPattern pattern(URLPattern::SCHEME_ALL);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern.Parse("data:*"));
  EXPECT_FALSE(pattern.MatchesURL(GURL("about:blank")));
}

// file scheme with empty hostname
TEST(ExtensionURLPatternTest, Match14) {
  URLPattern pattern(kAllSchemes);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern.Parse("file:///foo*"));
  EXPECT_EQ("file", pattern.scheme());
  EXPECT_EQ("", pattern.host());
  EXPECT_FALSE(pattern.match_subdomains());
  EXPECT_FALSE(pattern.match_all_urls());
  EXPECT_TRUE(pattern.match_effective_tld());
  EXPECT_EQ("/foo*", pattern.path());
  EXPECT_FALSE(pattern.MatchesURL(GURL("file://foo")));
  EXPECT_FALSE(pattern.MatchesURL(GURL("file://foobar")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("file:///foo")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("file:///foobar")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("file://localhost/foo")));
}

// file scheme without hostname part
TEST(ExtensionURLPatternTest, Match15) {
  URLPattern pattern(kAllSchemes);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern.Parse("file://foo*"));
  EXPECT_EQ("file", pattern.scheme());
  EXPECT_EQ("", pattern.host());
  EXPECT_FALSE(pattern.match_subdomains());
  EXPECT_FALSE(pattern.match_all_urls());
  EXPECT_TRUE(pattern.match_effective_tld());
  EXPECT_EQ("/foo*", pattern.path());
  EXPECT_FALSE(pattern.MatchesURL(GURL("file://foo")));
  EXPECT_FALSE(pattern.MatchesURL(GURL("file://foobar")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("file:///foo")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("file:///foobar")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("file://localhost/foo")));
}

// file scheme with hostname
TEST(ExtensionURLPatternTest, Match16) {
  URLPattern pattern(kAllSchemes);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern.Parse("file://localhost/foo*"));
  EXPECT_EQ("file", pattern.scheme());
  // Since hostname is ignored for file://.
  EXPECT_EQ("", pattern.host());
  EXPECT_FALSE(pattern.match_subdomains());
  EXPECT_TRUE(pattern.match_effective_tld());
  EXPECT_FALSE(pattern.match_all_urls());
  EXPECT_EQ("/foo*", pattern.path());
  EXPECT_FALSE(pattern.MatchesURL(GURL("file://foo")));
  EXPECT_FALSE(pattern.MatchesURL(GURL("file://foobar")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("file:///foo")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("file:///foobar")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("file://localhost/foo")));
}

// Specific port
TEST(ExtensionURLPatternTest, Match17) {
  URLPattern pattern(kAllSchemes);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS,
            pattern.Parse("http://www.example.com:80/foo"));
  EXPECT_EQ("http", pattern.scheme());
  EXPECT_EQ("www.example.com", pattern.host());
  EXPECT_FALSE(pattern.match_subdomains());
  EXPECT_FALSE(pattern.match_all_urls());
  EXPECT_EQ("/foo", pattern.path());
  EXPECT_EQ("80", pattern.port());
  EXPECT_TRUE(pattern.MatchesURL(GURL("http://www.example.com:80/foo")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("http://www.example.com/foo")));
  EXPECT_FALSE(pattern.MatchesURL(GURL("http://www.example.com:8080/foo")));
  EXPECT_FALSE(pattern.MatchesURL(
      GURL("filesystem:http://www.example.com:8080/foo/")));
  EXPECT_FALSE(pattern.MatchesURL(
      GURL("filesystem:http://www.example.com/f/foo")));
}

// Explicit port wildcard
TEST(ExtensionURLPatternTest, Match18) {
  URLPattern pattern(kAllSchemes);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS,
            pattern.Parse("http://www.example.com:*/foo"));
  EXPECT_EQ("http", pattern.scheme());
  EXPECT_EQ("www.example.com", pattern.host());
  EXPECT_FALSE(pattern.match_subdomains());
  EXPECT_FALSE(pattern.match_all_urls());
  EXPECT_EQ("/foo", pattern.path());
  EXPECT_EQ("*", pattern.port());
  EXPECT_TRUE(pattern.MatchesURL(GURL("http://www.example.com:80/foo")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("http://www.example.com/foo")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("http://www.example.com:8080/foo")));
  EXPECT_FALSE(pattern.MatchesURL(
      GURL("filesystem:http://www.example.com:8080/foo/")));
}

// chrome-extension://
TEST(ExtensionURLPatternTest, Match19) {
  URLPattern pattern(URLPattern::SCHEME_EXTENSION);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS,
            pattern.Parse("chrome-extension://ftw/*"));
  EXPECT_EQ(extensions::kExtensionScheme, pattern.scheme());
  EXPECT_EQ("ftw", pattern.host());
  EXPECT_FALSE(pattern.match_subdomains());
  EXPECT_FALSE(pattern.match_all_urls());
  EXPECT_EQ("/*", pattern.path());
  EXPECT_TRUE(pattern.MatchesURL(GURL("chrome-extension://ftw")));
  EXPECT_TRUE(pattern.MatchesURL(
      GURL("chrome-extension://ftw/http://google.com")));
  EXPECT_TRUE(pattern.MatchesURL(
      GURL("chrome-extension://ftw/https://google.com")));
  EXPECT_FALSE(pattern.MatchesURL(GURL("chrome-extension://foobar")));
  EXPECT_TRUE(pattern.MatchesURL(
      GURL("filesystem:chrome-extension://ftw/t/file.txt")));
}

// effective TLD wildcard
TEST(URLPatternTest, EffectiveTldWildcard) {
  URLPattern pattern(kAllSchemes);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS,
            pattern.Parse("http://*.google.*/foo*bar",
                          URLPattern::ALLOW_WILDCARD_FOR_EFFECTIVE_TLD));
  EXPECT_EQ("http", pattern.scheme());
  EXPECT_EQ("google", pattern.host());
  EXPECT_TRUE(pattern.match_subdomains());
  EXPECT_FALSE(pattern.match_effective_tld());
  EXPECT_FALSE(pattern.match_all_urls());
  EXPECT_EQ("/foo*bar", pattern.path());
  EXPECT_TRUE(pattern.MatchesURL(GURL("http://google.com/foobar")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("http://www.google.com.br/foo?bar")));
  EXPECT_TRUE(
      pattern.MatchesURL(GURL("http://monkey.images.google.co.uk/foooobar")));
  EXPECT_FALSE(pattern.MatchesURL(GURL("http://yahoo.com/foobar")));
  EXPECT_TRUE(pattern.MatchesURL(GURL("filesystem:http://google.com/foo/bar")));
  EXPECT_FALSE(pattern.MatchesURL(
      GURL("filesystem:http://google.com/temporary/foobar")));
  URLPattern pattern_sub(kAllSchemes);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS,
            pattern_sub.Parse("https://maps.google.*/",
                              URLPattern::ALLOW_WILDCARD_FOR_EFFECTIVE_TLD));
  EXPECT_EQ("https", pattern_sub.scheme());
  EXPECT_EQ("maps.google", pattern_sub.host());
  EXPECT_FALSE(pattern_sub.match_subdomains());
  EXPECT_FALSE(pattern_sub.match_all_urls());
  EXPECT_EQ("/", pattern_sub.path());
  EXPECT_TRUE(pattern_sub.MatchesURL(GURL("https://maps.google.co.uk/")));
  EXPECT_FALSE(pattern_sub.MatchesURL(GURL("https://sub.maps.google.co.uk/")));
}

static const struct GetAsStringPatterns {
  const char* pattern;
} kGetAsStringTestCases[] = {
    {"http://www/"},
    {"http://*/*"},
    {"chrome://*/*"},
    {"chrome://newtab/"},
    {"about:*"},
    {"about:blank"},
    {"chrome-extension://*/*"},
    {"chrome-extension://ftw/"},
    {"data:*"},
    {"data:monkey"},
    {"javascript:*"},
    {"javascript:atemyhomework"},
    {"http://www.example.com:8080/foo"},
};

TEST(ExtensionURLPatternTest, GetAsString) {
  for (size_t i = 0; i < arraysize(kGetAsStringTestCases); ++i) {
    URLPattern pattern(URLPattern::SCHEME_ALL);
    EXPECT_EQ(URLPattern::PARSE_SUCCESS,
              pattern.Parse(kGetAsStringTestCases[i].pattern))
        << "Error parsing " << kGetAsStringTestCases[i].pattern;
    EXPECT_EQ(kGetAsStringTestCases[i].pattern,
              pattern.GetAsString());
  }
}

testing::AssertionResult Overlaps(const URLPattern& pattern1,
                                  const URLPattern& pattern2) {
  if (!pattern1.OverlapsWith(pattern2)) {
    return testing::AssertionFailure()
        << pattern1.GetAsString() << " does not overlap " <<
                                     pattern2.GetAsString();
  }
  if (!pattern2.OverlapsWith(pattern1)) {
    return testing::AssertionFailure()
        << pattern2.GetAsString() << " does not overlap " <<
                                     pattern1.GetAsString();
  }
  return testing::AssertionSuccess()
      << pattern1.GetAsString() << " overlaps with " << pattern2.GetAsString();
}

TEST(ExtensionURLPatternTest, Overlaps) {
  URLPattern pattern1(kAllSchemes, "http://www.google.com/foo/*");
  URLPattern pattern2(kAllSchemes, "https://www.google.com/foo/*");
  URLPattern pattern3(kAllSchemes, "http://*.google.com/foo/*");
  URLPattern pattern4(kAllSchemes, "http://*.yahooo.com/foo/*");
  URLPattern pattern5(kAllSchemes, "http://www.yahooo.com/bar/*");
  URLPattern pattern6(kAllSchemes,
                      "http://www.yahooo.com/bar/baz/*");
  URLPattern pattern7(kAllSchemes, "file:///*");
  URLPattern pattern8(kAllSchemes, "*://*/*");
  URLPattern pattern9(URLPattern::SCHEME_HTTPS, "*://*/*");
  URLPattern pattern10(kAllSchemes, "<all_urls>");

  EXPECT_TRUE(Overlaps(pattern1, pattern1));
  EXPECT_FALSE(Overlaps(pattern1, pattern2));
  EXPECT_TRUE(Overlaps(pattern1, pattern3));
  EXPECT_FALSE(Overlaps(pattern1, pattern4));
  EXPECT_FALSE(Overlaps(pattern3, pattern4));
  EXPECT_FALSE(Overlaps(pattern4, pattern5));
  EXPECT_TRUE(Overlaps(pattern5, pattern6));

  // Test that scheme restrictions work.
  EXPECT_TRUE(Overlaps(pattern1, pattern8));
  EXPECT_FALSE(Overlaps(pattern1, pattern9));
  EXPECT_TRUE(Overlaps(pattern1, pattern10));

  // Test that '<all_urls>' includes file URLs, while scheme '*' does not.
  EXPECT_FALSE(Overlaps(pattern7, pattern8));
  EXPECT_TRUE(Overlaps(pattern7, pattern10));

  // Test that wildcard schemes are handled correctly, especially when compared
  // to each-other.
  URLPattern pattern11(kAllSchemes, "http://example.com/*");
  URLPattern pattern12(kAllSchemes, "*://example.com/*");
  URLPattern pattern13(kAllSchemes, "*://example.com/foo/*");
  URLPattern pattern14(kAllSchemes, "*://google.com/*");
  EXPECT_TRUE(Overlaps(pattern8, pattern12));
  EXPECT_TRUE(Overlaps(pattern9, pattern12));
  EXPECT_TRUE(Overlaps(pattern10, pattern12));
  EXPECT_TRUE(Overlaps(pattern11, pattern12));
  EXPECT_TRUE(Overlaps(pattern12, pattern13));
  EXPECT_TRUE(Overlaps(pattern11, pattern13));
  EXPECT_FALSE(Overlaps(pattern14, pattern12));
  EXPECT_FALSE(Overlaps(pattern14, pattern13));
}

TEST(ExtensionURLPatternTest, ConvertToExplicitSchemes) {
  URLPatternList all_urls(URLPattern(
      kAllSchemes,
      "<all_urls>").ConvertToExplicitSchemes());

  URLPatternList all_schemes(URLPattern(
      kAllSchemes,
      "*://google.com/foo").ConvertToExplicitSchemes());

  URLPatternList monkey(URLPattern(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS |
      URLPattern::SCHEME_FTP,
      "http://google.com/monkey").ConvertToExplicitSchemes());

  ASSERT_EQ(10u, all_urls.size());
  ASSERT_EQ(2u, all_schemes.size());
  ASSERT_EQ(1u, monkey.size());

  EXPECT_EQ("http://*/*", all_urls[0].GetAsString());
  EXPECT_EQ("https://*/*", all_urls[1].GetAsString());
  EXPECT_EQ("file:///*", all_urls[2].GetAsString());
  EXPECT_EQ("ftp://*/*", all_urls[3].GetAsString());
  EXPECT_EQ("chrome://*/*", all_urls[4].GetAsString());
  EXPECT_EQ("chrome-extension://*/*", all_urls[5].GetAsString());
  EXPECT_EQ("filesystem://*/*", all_urls[6].GetAsString());
  EXPECT_EQ("ws://*/*", all_urls[7].GetAsString());
  EXPECT_EQ("wss://*/*", all_urls[8].GetAsString());
  EXPECT_EQ("data:/*", all_urls[9].GetAsString());

  EXPECT_EQ("http://google.com/foo", all_schemes[0].GetAsString());
  EXPECT_EQ("https://google.com/foo", all_schemes[1].GetAsString());

  EXPECT_EQ("http://google.com/monkey", monkey[0].GetAsString());
}

TEST(ExtensionURLPatternTest, IgnorePorts) {
  std::string pattern_str = "http://www.example.com:8080/foo";
  GURL url("http://www.example.com:1234/foo");

  URLPattern pattern(kAllSchemes);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern.Parse(pattern_str));

  EXPECT_EQ(pattern_str, pattern.GetAsString());
  EXPECT_FALSE(pattern.MatchesURL(url));
}

TEST(ExtensionURLPatternTest, IgnoreMissingBackslashes) {
  std::string pattern_str1 = "http://www.example.com/example";
  std::string pattern_str2 = "http://www.example.com/example/*";
  GURL url1("http://www.example.com/example");
  GURL url2("http://www.example.com/example/");

  URLPattern pattern1(kAllSchemes);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern1.Parse(pattern_str1));
  URLPattern pattern2(kAllSchemes);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern2.Parse(pattern_str2));

  // Same patterns should match same urls.
  EXPECT_TRUE(pattern1.MatchesURL(url1));
  EXPECT_TRUE(pattern2.MatchesURL(url2));
  // The not terminated path should match the terminated pattern.
  EXPECT_TRUE(pattern2.MatchesURL(url1));
  // The terminated path however should not match the unterminated pattern.
  EXPECT_FALSE(pattern1.MatchesURL(url2));
}

TEST(ExtensionURLPatternTest, Equals) {
  const struct {
    const char* pattern1;
    const char* pattern2;
    bool expected_equal;
  } kEqualsTestCases[] = {
    // schemes
    { "http://en.google.com/blah/*/foo",
      "https://en.google.com/blah/*/foo",
      false
    },
    { "https://en.google.com/blah/*/foo",
      "https://en.google.com/blah/*/foo",
      true
    },
    { "https://en.google.com/blah/*/foo",
      "ftp://en.google.com/blah/*/foo",
      false
    },

    // subdomains
    { "https://en.google.com/blah/*/foo",
      "https://fr.google.com/blah/*/foo",
      false
    },
    { "https://www.google.com/blah/*/foo",
      "https://*.google.com/blah/*/foo",
      false
    },
    { "https://*.google.com/blah/*/foo",
      "https://*.google.com/blah/*/foo",
      true
    },

    // domains
    { "http://en.example.com/blah/*/foo",
      "http://en.google.com/blah/*/foo",
      false
    },

    // ports
    { "http://en.google.com:8000/blah/*/foo",
      "http://en.google.com/blah/*/foo",
      false
    },
    { "http://fr.google.com:8000/blah/*/foo",
      "http://fr.google.com:8000/blah/*/foo",
      true
    },
    { "http://en.google.com:8000/blah/*/foo",
      "http://en.google.com:8080/blah/*/foo",
      false
    },

    // paths
    { "http://en.google.com/blah/*/foo",
      "http://en.google.com/blah/*",
      false
    },
    { "http://en.google.com/*",
      "http://en.google.com/",
      false
    },
    { "http://en.google.com/*",
      "http://en.google.com/*",
      true
    },

    // all_urls
    { "<all_urls>",
      "<all_urls>",
      true
    },
    { "<all_urls>",
      "http://*/*",
      false
    }
  };

  for (size_t i = 0; i < arraysize(kEqualsTestCases); ++i) {
    std::string message = kEqualsTestCases[i].pattern1;
    message += " ";
    message += kEqualsTestCases[i].pattern2;

    URLPattern pattern1(URLPattern::SCHEME_ALL);
    URLPattern pattern2(URLPattern::SCHEME_ALL);

    pattern1.Parse(kEqualsTestCases[i].pattern1);
    pattern2.Parse(kEqualsTestCases[i].pattern2);
    EXPECT_EQ(kEqualsTestCases[i].expected_equal, pattern1 == pattern2)
        << message;
  }
}

TEST(ExtensionURLPatternTest, CanReusePatternWithParse) {
  URLPattern pattern1(URLPattern::SCHEME_ALL);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern1.Parse("http://aa.com/*"));
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern1.Parse("http://bb.com/*"));

  EXPECT_TRUE(pattern1.MatchesURL(GURL("http://bb.com/path")));
  EXPECT_FALSE(pattern1.MatchesURL(GURL("http://aa.com/path")));

  URLPattern pattern2(URLPattern::SCHEME_ALL, URLPattern::kAllUrlsPattern);
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern2.Parse("http://aa.com/*"));

  EXPECT_FALSE(pattern2.MatchesURL(GURL("http://bb.com/path")));
  EXPECT_TRUE(pattern2.MatchesURL(GURL("http://aa.com/path")));
  EXPECT_FALSE(pattern2.MatchesURL(GURL("http://sub.aa.com/path")));

  URLPattern pattern3(URLPattern::SCHEME_ALL, "http://aa.com/*");
  EXPECT_EQ(URLPattern::PARSE_SUCCESS, pattern3.Parse("http://aa.com:88/*"));
  EXPECT_FALSE(pattern3.MatchesURL(GURL("http://aa.com/path")));
  EXPECT_TRUE(pattern3.MatchesURL(GURL("http://aa.com:88/path")));
}

// Returns success if neither |a| nor |b| encompasses the other.
testing::AssertionResult NeitherContains(const URLPattern& a,
                                         const URLPattern& b) {
  if (a.Contains(b))
    return testing::AssertionFailure() << a.GetAsString() << " encompasses " <<
                                          b.GetAsString();
  if (b.Contains(a))
    return testing::AssertionFailure() << b.GetAsString() << " encompasses " <<
                                          a.GetAsString();
  return testing::AssertionSuccess() <<
      "Neither " << a.GetAsString() << " nor " << b.GetAsString() <<
      " encompass the other";
}

// Returns success if |a| encompasses |b| but not the other way around.
testing::AssertionResult StrictlyContains(const URLPattern& a,
                                          const URLPattern& b) {
  if (!a.Contains(b))
    return testing::AssertionFailure() << a.GetAsString() <<
                                          " does not encompass " <<
                                          b.GetAsString();
  if (b.Contains(a))
    return testing::AssertionFailure() << b.GetAsString() << " encompasses " <<
                                          a.GetAsString();
  return testing::AssertionSuccess() << a.GetAsString() <<
                                        " strictly encompasses " <<
                                        b.GetAsString();
}

TEST(ExtensionURLPatternTest, Subset) {
  URLPattern pattern1(kAllSchemes, "http://www.google.com/foo/*");
  URLPattern pattern2(kAllSchemes, "https://www.google.com/foo/*");
  URLPattern pattern3(kAllSchemes, "http://*.google.com/foo/*");
  URLPattern pattern4(kAllSchemes, "http://*.yahooo.com/foo/*");
  URLPattern pattern5(kAllSchemes, "http://www.yahooo.com/bar/*");
  URLPattern pattern6(kAllSchemes, "http://www.yahooo.com/bar/baz/*");
  URLPattern pattern7(kAllSchemes, "file:///*");
  URLPattern pattern8(kAllSchemes, "*://*/*");
  URLPattern pattern9(URLPattern::SCHEME_HTTPS, "*://*/*");
  URLPattern pattern10(kAllSchemes, "<all_urls>");
  URLPattern pattern11(kAllSchemes, "http://example.com/*");
  URLPattern pattern12(kAllSchemes, "*://example.com/*");
  URLPattern pattern13(kAllSchemes, "*://example.com/foo/*");
  URLPattern pattern14(kAllSchemes, "http://yahoo.com/*");
  URLPattern pattern15(kAllSchemes, "http://*.yahoo.com/*");

  // All patterns should encompass themselves.
  EXPECT_TRUE(pattern1.Contains(pattern1));
  EXPECT_TRUE(pattern2.Contains(pattern2));
  EXPECT_TRUE(pattern3.Contains(pattern3));
  EXPECT_TRUE(pattern4.Contains(pattern4));
  EXPECT_TRUE(pattern5.Contains(pattern5));
  EXPECT_TRUE(pattern6.Contains(pattern6));
  EXPECT_TRUE(pattern7.Contains(pattern7));
  EXPECT_TRUE(pattern8.Contains(pattern8));
  EXPECT_TRUE(pattern9.Contains(pattern9));
  EXPECT_TRUE(pattern10.Contains(pattern10));
  EXPECT_TRUE(pattern11.Contains(pattern11));
  EXPECT_TRUE(pattern12.Contains(pattern12));
  EXPECT_TRUE(pattern13.Contains(pattern13));

  // pattern1's relationship to the other patterns.
  EXPECT_TRUE(NeitherContains(pattern1, pattern2));
  EXPECT_TRUE(StrictlyContains(pattern3, pattern1));
  EXPECT_TRUE(NeitherContains(pattern1, pattern4));
  EXPECT_TRUE(NeitherContains(pattern1, pattern5));
  EXPECT_TRUE(NeitherContains(pattern1, pattern6));
  EXPECT_TRUE(NeitherContains(pattern1, pattern7));
  EXPECT_TRUE(StrictlyContains(pattern8, pattern1));
  EXPECT_TRUE(NeitherContains(pattern1, pattern9));
  EXPECT_TRUE(StrictlyContains(pattern10, pattern1));
  EXPECT_TRUE(NeitherContains(pattern1, pattern11));
  EXPECT_TRUE(NeitherContains(pattern1, pattern12));
  EXPECT_TRUE(NeitherContains(pattern1, pattern13));

  // pattern2's relationship to the other patterns.
  EXPECT_TRUE(NeitherContains(pattern2, pattern3));
  EXPECT_TRUE(NeitherContains(pattern2, pattern4));
  EXPECT_TRUE(NeitherContains(pattern2, pattern5));
  EXPECT_TRUE(NeitherContains(pattern2, pattern6));
  EXPECT_TRUE(NeitherContains(pattern2, pattern7));
  EXPECT_TRUE(StrictlyContains(pattern8, pattern2));
  EXPECT_TRUE(StrictlyContains(pattern9, pattern2));
  EXPECT_TRUE(StrictlyContains(pattern10, pattern2));
  EXPECT_TRUE(NeitherContains(pattern2, pattern11));
  EXPECT_TRUE(NeitherContains(pattern2, pattern12));
  EXPECT_TRUE(NeitherContains(pattern2, pattern13));

  // Specifically test file:// URLs.
  EXPECT_TRUE(NeitherContains(pattern7, pattern8));
  EXPECT_TRUE(NeitherContains(pattern7, pattern9));
  EXPECT_TRUE(StrictlyContains(pattern10, pattern7));

  // <all_urls> encompasses everything.
  EXPECT_TRUE(StrictlyContains(pattern10, pattern1));
  EXPECT_TRUE(StrictlyContains(pattern10, pattern2));
  EXPECT_TRUE(StrictlyContains(pattern10, pattern3));
  EXPECT_TRUE(StrictlyContains(pattern10, pattern4));
  EXPECT_TRUE(StrictlyContains(pattern10, pattern5));
  EXPECT_TRUE(StrictlyContains(pattern10, pattern6));
  EXPECT_TRUE(StrictlyContains(pattern10, pattern7));
  EXPECT_TRUE(StrictlyContains(pattern10, pattern8));
  EXPECT_TRUE(StrictlyContains(pattern10, pattern9));
  EXPECT_TRUE(StrictlyContains(pattern10, pattern11));
  EXPECT_TRUE(StrictlyContains(pattern10, pattern12));
  EXPECT_TRUE(StrictlyContains(pattern10, pattern13));

  // More...
  EXPECT_TRUE(StrictlyContains(pattern12, pattern11));
  EXPECT_TRUE(NeitherContains(pattern11, pattern13));
  EXPECT_TRUE(StrictlyContains(pattern12, pattern13));
  EXPECT_TRUE(StrictlyContains(pattern15, pattern14));
}

TEST(ExtensionURLPatternTest, MatchesSingleOrigin) {
  EXPECT_FALSE(
      URLPattern(URLPattern::SCHEME_ALL, "http://*/").MatchesSingleOrigin());
  EXPECT_FALSE(URLPattern(URLPattern::SCHEME_ALL, "https://*.google.com/*")
                   .MatchesSingleOrigin());
  EXPECT_TRUE(URLPattern(URLPattern::SCHEME_ALL, "http://google.com/")
                  .MatchesSingleOrigin());
  EXPECT_TRUE(URLPattern(URLPattern::SCHEME_ALL, "http://google.com/*")
                  .MatchesSingleOrigin());
  EXPECT_TRUE(URLPattern(URLPattern::SCHEME_ALL, "http://www.google.com/")
                  .MatchesSingleOrigin());
  EXPECT_FALSE(URLPattern(URLPattern::SCHEME_ALL, "*://www.google.com/")
                   .MatchesSingleOrigin());
  EXPECT_FALSE(URLPattern(URLPattern::SCHEME_ALL, "http://*.com/")
                   .MatchesSingleOrigin());
  EXPECT_FALSE(URLPattern(URLPattern::SCHEME_ALL, "http://*.google.com/foo/bar")
                   .MatchesSingleOrigin());
  EXPECT_TRUE(
      URLPattern(URLPattern::SCHEME_ALL, "http://www.google.com/foo/bar")
          .MatchesSingleOrigin());
  EXPECT_FALSE(URLPattern(URLPattern::SCHEME_HTTPS, "*://*.google.com/foo/bar")
                   .MatchesSingleOrigin());
  EXPECT_TRUE(URLPattern(URLPattern::SCHEME_HTTPS, "https://www.google.com/")
                  .MatchesSingleOrigin());
  EXPECT_FALSE(URLPattern(URLPattern::SCHEME_HTTP,
                          "http://*.google.com/foo/bar").MatchesSingleOrigin());
  EXPECT_TRUE(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.google.com/foo/bar")
          .MatchesSingleOrigin());
}

TEST(ExtensionURLPatternTest, TrailingDotDomain) {
  const GURL normal_domain("http://example.com/");
  const GURL trailing_dot_domain("http://example.com./");

  // Both patterns should match trailing dot and non trailing dot domains. More
  // information about this not obvious behaviour can be found in [1].
  //
  // RFC 1738 [2] specifies clearly that the <host> part of a URL is supposed to
  // contain a fully qualified domain name:
  //
  // 3.1. Common Internet Scheme Syntax
  //      //<user>:<password>@<host>:<port>/<url-path>
  //
  //  host
  //      The fully qualified domain name of a network host
  //
  // [1] http://www.dns-sd.org./TrailingDotsInDomainNames.html
  // [2] http://www.ietf.org/rfc/rfc1738.txt

  const URLPattern pattern(URLPattern::SCHEME_HTTP, "*://example.com/*");
  EXPECT_TRUE(pattern.MatchesURL(normal_domain));
  EXPECT_TRUE(pattern.MatchesURL(trailing_dot_domain));

  const URLPattern trailing_pattern(URLPattern::SCHEME_HTTP,
                                    "*://example.com./*");
  EXPECT_TRUE(trailing_pattern.MatchesURL(normal_domain));
  EXPECT_TRUE(trailing_pattern.MatchesURL(trailing_dot_domain));
}

TEST(ExtensionURLPatternTest, MatchesEffectiveTLD) {
  namespace rcd = net::registry_controlled_domains;

  constexpr struct {
    const char* pattern;
    bool matches_public_tld;
    bool matches_public_or_private_tld;
    bool matches_public_or_unknown_tld;
  } tests[] = {
      // <all_urls> obviously implies all hosts.
      {"*://*/*", true, true, true},
      {"*://*:*/*", true, true, true},
      {"<all_urls>", true, true, true},

      // Matching a single scheme effectively all hosts.
      {"http://*/*", true, true, true},
      {"https://*/*", true, true, true},

      // Specifying a path under any origin is effectively all hosts.
      {"*://*/maps", true, true, true},

      // Matching a given (e)TLD is effectively all hosts.
      {"https://*.com/*", true, true, true},
      {"*://*.co.uk/*", true, true, true},

      // Matching an arbitrary domain with a given path or port is effectively
      // all hosts.
      {"*://*.com/maps", true, true, true},
      {"http://*.com:80/*", true, true, true},

      // Typically, we don't include private registries (like appspot.com) as
      // matching an eTLD - there's legitimate reasons to want to always run on
      // *.appspot.com, and we shouldn't say that it's close enough to every
      // site. However, we should correctly report that it's a TLD wildcard
      // pattern if we include private registries.
      {"*://*.appspot.com/*", false, true, false},

      // Unrecognized TLD-like domains should not be treated as matching an
      // effective TLD unless unknown TLDs are explicitly included.
      {"*://*.notatld/*", false, false, true},

      // All example.com sites is clearly not all hosts, or a TLD wildcard.
      {"*://*.example.com/*", false, false, false},
  };

  for (const auto& test : tests) {
    const URLPattern pattern(URLPattern::SCHEME_ALL, test.pattern);
    EXPECT_EQ(test.matches_public_tld,
              pattern.MatchesEffectiveTld(rcd::EXCLUDE_PRIVATE_REGISTRIES))
        << test.pattern;
    EXPECT_EQ(test.matches_public_or_private_tld,
              pattern.MatchesEffectiveTld(rcd::INCLUDE_PRIVATE_REGISTRIES))
        << test.pattern;
    EXPECT_EQ(test.matches_public_or_unknown_tld,
              pattern.MatchesEffectiveTld(rcd::EXCLUDE_PRIVATE_REGISTRIES,
                                          rcd::INCLUDE_UNKNOWN_REGISTRIES))
        << test.pattern;
    EXPECT_EQ(test.matches_public_or_unknown_tld ||
                  test.matches_public_or_private_tld,
              pattern.MatchesEffectiveTld(rcd::INCLUDE_PRIVATE_REGISTRIES,
                                          rcd::INCLUDE_UNKNOWN_REGISTRIES))
        << test.pattern;
  }
}

// Test that URLPattern properly canonicalizes uncanonicalized hosts.
TEST(ExtensionURLPatternTest, UncanonicalizedUrl) {
  {
    // Simple case: canonicalization should lowercase the host. This is
    // important, since gOoGle.com would never be matched in practice.
    const URLPattern pattern(URLPattern::SCHEME_ALL, "*://*.gOoGle.com/*");
    EXPECT_TRUE(pattern.MatchesURL(GURL("https://google.com")));
    EXPECT_TRUE(pattern.MatchesURL(GURL("https://maps.google.com")));
    EXPECT_FALSE(pattern.MatchesURL(GURL("https://example.com")));
    EXPECT_EQ("*://*.google.com/*", pattern.GetAsString());
  }

  {
    // Trickier case: internationalization with UTF8 characters (the first 'g'
    // isn't actually a 'g').
    const URLPattern pattern(URLPattern::SCHEME_ALL, "https://*.ɡoogle.com/*");
    constexpr char kCanonicalizedHost[] = "xn--oogle-qmc.com";
    EXPECT_EQ(kCanonicalizedHost, pattern.host());
    EXPECT_EQ(base::StringPrintf("https://*.%s/*", kCanonicalizedHost),
              pattern.GetAsString());
    EXPECT_FALSE(pattern.MatchesURL(GURL("https://google.com")));
    // The pattern should match the canonicalized host, and the original
    // UTF8 version.
    EXPECT_TRUE(pattern.MatchesURL(
        GURL(base::StringPrintf("https://%s/", kCanonicalizedHost))));
    EXPECT_TRUE(pattern.MatchesHost("ɡoogle.com"));
  }

  {
    // Sometimes, canonicalization can fail (such as here, where we have invalid
    // unicode characters). In that case, URLPattern parsing should also fail.
    URLPattern pattern(URLPattern::SCHEME_ALL);
    EXPECT_EQ(URLPattern::PARSE_ERROR_INVALID_HOST,
              pattern.Parse("https://\xef\xb7\x90zyx.com/*"));
  }
}

}  // namespace
