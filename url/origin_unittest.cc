// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "base/logging.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

void ExpectParsedUrlsEqual(const GURL& a, const GURL& b) {
  EXPECT_EQ(a, b);
  const url::Parsed& a_parsed = a.parsed_for_possibly_invalid_spec();
  const url::Parsed& b_parsed = b.parsed_for_possibly_invalid_spec();
  EXPECT_EQ(a_parsed.scheme.begin, b_parsed.scheme.begin);
  EXPECT_EQ(a_parsed.scheme.len, b_parsed.scheme.len);
  EXPECT_EQ(a_parsed.username.begin, b_parsed.username.begin);
  EXPECT_EQ(a_parsed.username.len, b_parsed.username.len);
  EXPECT_EQ(a_parsed.password.begin, b_parsed.password.begin);
  EXPECT_EQ(a_parsed.password.len, b_parsed.password.len);
  EXPECT_EQ(a_parsed.host.begin, b_parsed.host.begin);
  EXPECT_EQ(a_parsed.host.len, b_parsed.host.len);
  EXPECT_EQ(a_parsed.port.begin, b_parsed.port.begin);
  EXPECT_EQ(a_parsed.port.len, b_parsed.port.len);
  EXPECT_EQ(a_parsed.path.begin, b_parsed.path.begin);
  EXPECT_EQ(a_parsed.path.len, b_parsed.path.len);
  EXPECT_EQ(a_parsed.query.begin, b_parsed.query.begin);
  EXPECT_EQ(a_parsed.query.len, b_parsed.query.len);
  EXPECT_EQ(a_parsed.ref.begin, b_parsed.ref.begin);
  EXPECT_EQ(a_parsed.ref.len, b_parsed.ref.len);
}

TEST(OriginTest, UniqueOriginComparison) {
  url::Origin unique_origin;
  EXPECT_EQ("", unique_origin.scheme());
  EXPECT_EQ("", unique_origin.host());
  EXPECT_EQ(0, unique_origin.port());
  EXPECT_TRUE(unique_origin.unique());
  EXPECT_FALSE(unique_origin.IsSameOriginWith(unique_origin));

  const char* const urls[] = {"data:text/html,Hello!",
                              "javascript:alert(1)",
                              "file://example.com:443/etc/passwd",
                              "yay",
                              "http::///invalid.example.com/"};

  for (auto* test_url : urls) {
    SCOPED_TRACE(test_url);
    GURL url(test_url);
    url::Origin origin = url::Origin::Create(url);
    EXPECT_EQ("", origin.scheme());
    EXPECT_EQ("", origin.host());
    EXPECT_EQ(0, origin.port());
    EXPECT_TRUE(origin.unique());
    EXPECT_FALSE(origin.IsSameOriginWith(origin));
    EXPECT_FALSE(unique_origin.IsSameOriginWith(origin));
    EXPECT_FALSE(origin.IsSameOriginWith(unique_origin));

    ExpectParsedUrlsEqual(GURL(origin.Serialize()), origin.GetURL());
  }
}

TEST(OriginTest, ConstructFromTuple) {
  struct TestCases {
    const char* const scheme;
    const char* const host;
    const uint16_t port;
  } cases[] = {
      {"http", "example.com", 80},
      {"http", "example.com", 123},
      {"https", "example.com", 443},
  };

  for (const auto& test_case : cases) {
    testing::Message scope_message;
    scope_message << test_case.scheme << "://" << test_case.host << ":"
                  << test_case.port;
    SCOPED_TRACE(scope_message);
    url::Origin origin = url::Origin::CreateFromNormalizedTuple(
        test_case.scheme, test_case.host, test_case.port);

    EXPECT_EQ(test_case.scheme, origin.scheme());
    EXPECT_EQ(test_case.host, origin.host());
    EXPECT_EQ(test_case.port, origin.port());
  }
}

TEST(OriginTest, ConstructFromGURL) {
  url::Origin different_origin =
      url::Origin::Create(GURL("https://not-in-the-list.test/"));

  struct TestCases {
    const char* const url;
    const char* const expected_scheme;
    const char* const expected_host;
    const uint16_t expected_port;
  } cases[] = {
      // IP Addresses
      {"http://192.168.9.1/", "http", "192.168.9.1", 80},
      {"http://[2001:db8::1]/", "http", "[2001:db8::1]", 80},

      // Punycode
      {"http://☃.net/", "http", "xn--n3h.net", 80},
      {"blob:http://☃.net/", "http", "xn--n3h.net", 80},

      // Generic URLs
      {"http://example.com/", "http", "example.com", 80},
      {"http://example.com:123/", "http", "example.com", 123},
      {"https://example.com/", "https", "example.com", 443},
      {"https://example.com:123/", "https", "example.com", 123},
      {"http://user:pass@example.com/", "http", "example.com", 80},
      {"http://example.com:123/?query", "http", "example.com", 123},
      {"https://example.com/#1234", "https", "example.com", 443},
      {"https://u:p@example.com:123/?query#1234", "https", "example.com", 123},

      // Registered URLs
      {"ftp://example.com/", "ftp", "example.com", 21},
      {"gopher://example.com/", "gopher", "example.com", 70},
      {"ws://example.com/", "ws", "example.com", 80},
      {"wss://example.com/", "wss", "example.com", 443},

      // file: URLs
      {"file:///etc/passwd", "file", "", 0},
      {"file://example.com/etc/passwd", "file", "example.com", 0},

      // Filesystem:
      {"filesystem:http://example.com/type/", "http", "example.com", 80},
      {"filesystem:http://example.com:123/type/", "http", "example.com", 123},
      {"filesystem:https://example.com/type/", "https", "example.com", 443},
      {"filesystem:https://example.com:123/type/", "https", "example.com", 123},

      // Blob:
      {"blob:http://example.com/guid-goes-here", "http", "example.com", 80},
      {"blob:http://example.com:123/guid-goes-here", "http", "example.com", 123},
      {"blob:https://example.com/guid-goes-here", "https", "example.com", 443},
      {"blob:http://u:p@example.com/guid-goes-here", "http", "example.com", 80},
  };

  for (const auto& test_case : cases) {
    SCOPED_TRACE(test_case.url);
    GURL url(test_case.url);
    EXPECT_TRUE(url.is_valid());
    url::Origin origin = url::Origin::Create(url);
    EXPECT_EQ(test_case.expected_scheme, origin.scheme());
    EXPECT_EQ(test_case.expected_host, origin.host());
    EXPECT_EQ(test_case.expected_port, origin.port());
    EXPECT_FALSE(origin.unique());
    EXPECT_TRUE(origin.IsSameOriginWith(origin));
    EXPECT_FALSE(different_origin.IsSameOriginWith(origin));
    EXPECT_FALSE(origin.IsSameOriginWith(different_origin));

    ExpectParsedUrlsEqual(GURL(origin.Serialize()), origin.GetURL());
  }
}

TEST(OriginTest, Serialization) {
  struct TestCases {
    const char* const url;
    const char* const expected;
  } cases[] = {
      {"http://192.168.9.1/", "http://192.168.9.1"},
      {"http://[2001:db8::1]/", "http://[2001:db8::1]"},
      {"http://☃.net/", "http://xn--n3h.net"},
      {"http://example.com/", "http://example.com"},
      {"http://example.com:123/", "http://example.com:123"},
      {"https://example.com/", "https://example.com"},
      {"https://example.com:123/", "https://example.com:123"},
      {"file:///etc/passwd", "file://"},
      {"file://example.com/etc/passwd", "file://"},
  };

  for (const auto& test_case : cases) {
    SCOPED_TRACE(test_case.url);
    GURL url(test_case.url);
    EXPECT_TRUE(url.is_valid());
    url::Origin origin = url::Origin::Create(url);
    std::string serialized = origin.Serialize();
    ExpectParsedUrlsEqual(GURL(serialized), origin.GetURL());

    EXPECT_EQ(test_case.expected, serialized);

    // The '<<' operator should produce the same serialization as Serialize().
    std::stringstream out;
    out << origin;
    EXPECT_EQ(test_case.expected, out.str());
  }
}

TEST(OriginTest, Comparison) {
  // These URLs are arranged in increasing order:
  const char* const urls[] = {
      "data:uniqueness",
      "http://a:80",
      "http://b:80",
      "https://a:80",
      "https://b:80",
      "http://a:81",
      "http://b:81",
      "https://a:81",
      "https://b:81",
  };

  for (size_t i = 0; i < arraysize(urls); i++) {
    GURL current_url(urls[i]);
    url::Origin current = url::Origin::Create(current_url);
    for (size_t j = i; j < arraysize(urls); j++) {
      GURL compare_url(urls[j]);
      url::Origin to_compare = url::Origin::Create(compare_url);
      EXPECT_EQ(i < j, current < to_compare) << i << " < " << j;
      EXPECT_EQ(j < i, to_compare < current) << j << " < " << i;
    }
  }
}

TEST(OriginTest, UnsafelyCreate) {
  struct TestCase {
    const char* scheme;
    const char* host;
    uint16_t port;
  } cases[] = {
      {"http", "example.com", 80},
      {"http", "example.com", 123},
      {"https", "example.com", 443},
      {"https", "example.com", 123},
      {"file", "", 0},
      {"file", "example.com", 0},
  };

  for (const auto& test : cases) {
    SCOPED_TRACE(testing::Message() << test.scheme << "://" << test.host << ":"
                                    << test.port);
    url::Origin origin = url::Origin::UnsafelyCreateOriginWithoutNormalization(
        test.scheme, test.host, test.port);
    EXPECT_EQ(test.scheme, origin.scheme());
    EXPECT_EQ(test.host, origin.host());
    EXPECT_EQ(test.port, origin.port());
    EXPECT_FALSE(origin.unique());
    EXPECT_TRUE(origin.IsSameOriginWith(origin));

    ExpectParsedUrlsEqual(GURL(origin.Serialize()), origin.GetURL());
  }
}

TEST(OriginTest, UnsafelyCreateUniqueOnInvalidInput) {
  struct TestCases {
    const char* scheme;
    const char* host;
    uint16_t port = 80;
  } cases[] = {{"", "", 0},
               {"data", "", 0},
               {"blob", "", 0},
               {"filesystem", "", 0},
               {"data", "example.com"},
               {"http", "☃.net"},
               {"http\nmore", "example.com"},
               {"http\rmore", "example.com"},
               {"http\n", "example.com"},
               {"http\r", "example.com"},
               {"http", "example.com\nnot-example.com"},
               {"http", "example.com\rnot-example.com"},
               {"http", "example.com\n"},
               {"http", "example.com\r"},
               {"http", "example.com", 0},
               {"file", ""}};

  for (const auto& test : cases) {
    SCOPED_TRACE(testing::Message() << test.scheme << "://" << test.host << ":"
                                    << test.port);
    url::Origin origin = url::Origin::UnsafelyCreateOriginWithoutNormalization(
        test.scheme, test.host, test.port);
    EXPECT_EQ("", origin.scheme());
    EXPECT_EQ("", origin.host());
    EXPECT_EQ(0, origin.port());
    EXPECT_TRUE(origin.unique());
    EXPECT_FALSE(origin.IsSameOriginWith(origin));

    ExpectParsedUrlsEqual(GURL(origin.Serialize()), origin.GetURL());
  }
}

TEST(OriginTest, UnsafelyCreateUniqueViaEmbeddedNulls) {
  struct TestCases {
    const char* scheme;
    size_t scheme_length;
    const char* host;
    size_t host_length;
    uint16_t port = 80;
  } cases[] = {{"http\0more", 9, "example.com", 11},
               {"http\0", 5, "example.com", 11},
               {"\0http", 5, "example.com", 11},
               {"http", 4, "example.com\0not-example.com", 27},
               {"http", 4, "example.com\0", 12},
               {"http", 4, "\0example.com", 12}};

  for (const auto& test : cases) {
    SCOPED_TRACE(testing::Message() << test.scheme << "://" << test.host << ":"
                                    << test.port);
    url::Origin origin = url::Origin::UnsafelyCreateOriginWithoutNormalization(
        std::string(test.scheme, test.scheme_length),
        std::string(test.host, test.host_length), test.port);
    EXPECT_EQ("", origin.scheme());
    EXPECT_EQ("", origin.host());
    EXPECT_EQ(0, origin.port());
    EXPECT_TRUE(origin.unique());
    EXPECT_FALSE(origin.IsSameOriginWith(origin));

    ExpectParsedUrlsEqual(GURL(origin.Serialize()), origin.GetURL());
  }
}

TEST(OriginTest, DomainIs) {
  const struct {
    const char* url;
    const char* lower_ascii_domain;
    bool expected_domain_is;
  } kTestCases[] = {
      {"http://google.com/foo", "google.com", true},
      {"http://www.google.com:99/foo", "google.com", true},
      {"http://www.google.com.cn/foo", "google.com", false},
      {"http://www.google.comm", "google.com", false},
      {"http://www.iamnotgoogle.com/foo", "google.com", false},
      {"http://www.google.com/foo", "Google.com", false},

      // If the host ends with a dot, it matches domains with or without a dot.
      {"http://www.google.com./foo", "google.com", true},
      {"http://www.google.com./foo", "google.com.", true},
      {"http://www.google.com./foo", ".com", true},
      {"http://www.google.com./foo", ".com.", true},

      // But, if the host doesn't end with a dot and the input domain does, then
      // it's considered to not match.
      {"http://google.com/foo", "google.com.", false},

      // If the host ends with two dots, it doesn't match.
      {"http://www.google.com../foo", "google.com", false},

      // Filesystem scheme.
      {"filesystem:http://www.google.com:99/foo/", "google.com", true},
      {"filesystem:http://www.iamnotgoogle.com/foo/", "google.com", false},

      // File scheme.
      {"file:///home/user/text.txt", "", false},
      {"file:///home/user/text.txt", "txt", false},
  };

  for (const auto& test_case : kTestCases) {
    SCOPED_TRACE(testing::Message() << "(url, domain): (" << test_case.url
                                    << ", " << test_case.lower_ascii_domain
                                    << ")");
    GURL url(test_case.url);
    ASSERT_TRUE(url.is_valid());
    url::Origin origin = url::Origin::Create(url);

    EXPECT_EQ(test_case.expected_domain_is,
              origin.DomainIs(test_case.lower_ascii_domain));
  }

  // If the URL is invalid, DomainIs returns false.
  GURL invalid_url("google.com");
  ASSERT_FALSE(invalid_url.is_valid());
  EXPECT_FALSE(url::Origin::Create(invalid_url).DomainIs("google.com"));

  // Unique origins.
  EXPECT_FALSE(url::Origin().DomainIs(""));
  EXPECT_FALSE(url::Origin().DomainIs("com"));
}

TEST(OriginTest, DebugAlias) {
  url::Origin origin1 = url::Origin::Create(GURL("https://foo.com/bar"));
  DEBUG_ALIAS_FOR_ORIGIN(origin1_debug_alias, origin1);
  EXPECT_STREQ("https://foo.com", origin1_debug_alias);
}

}  // namespace
