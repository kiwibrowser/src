/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/weborigin/origin_access_entry.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_public_suffix_list.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

class OriginAccessEntryTestSuffixList : public blink::WebPublicSuffixList {
 public:
  size_t GetPublicSuffixLength(const blink::WebString&) override {
    return length_;
  }

  void SetPublicSuffix(const blink::WebString& suffix) {
    length_ = suffix.length();
  }

 private:
  size_t length_;
};

class OriginAccessEntryTestPlatform : public TestingPlatformSupport {
 public:
  blink::WebPublicSuffixList* PublicSuffixList() override {
    return &suffix_list_;
  }

  void SetPublicSuffix(const blink::WebString& suffix) {
    suffix_list_.SetPublicSuffix(suffix);
  }

 private:
  OriginAccessEntryTestSuffixList suffix_list_;
};

TEST(OriginAccessEntryTest, PublicSuffixListTest) {
  ScopedTestingPlatformSupport<OriginAccessEntryTestPlatform> platform;
  platform->SetPublicSuffix("com");

  scoped_refptr<const SecurityOrigin> origin =
      SecurityOrigin::CreateFromString("http://www.google.com");
  OriginAccessEntry entry1("http", "google.com",
                           OriginAccessEntry::kAllowSubdomains);
  OriginAccessEntry entry2("http", "hamster.com",
                           OriginAccessEntry::kAllowSubdomains);
  OriginAccessEntry entry3("http", "com", OriginAccessEntry::kAllowSubdomains);
  EXPECT_EQ(OriginAccessEntry::kMatchesOrigin, entry1.MatchesOrigin(*origin));
  EXPECT_EQ(OriginAccessEntry::kDoesNotMatchOrigin,
            entry2.MatchesOrigin(*origin));
  EXPECT_EQ(OriginAccessEntry::kMatchesOriginButIsPublicSuffix,
            entry3.MatchesOrigin(*origin));
}

TEST(OriginAccessEntryTest, AllowSubdomainsTest) {
  struct TestCase {
    const char* protocol;
    const char* host;
    const char* origin;
    OriginAccessEntry::MatchResult expected_origin;
    OriginAccessEntry::MatchResult expected_domain;
  } inputs[] = {
      {"http", "example.com", "http://example.com/",
       OriginAccessEntry::kMatchesOrigin, OriginAccessEntry::kMatchesOrigin},
      {"http", "example.com", "http://www.example.com/",
       OriginAccessEntry::kMatchesOrigin, OriginAccessEntry::kMatchesOrigin},
      {"http", "example.com", "http://www.www.example.com/",
       OriginAccessEntry::kMatchesOrigin, OriginAccessEntry::kMatchesOrigin},
      {"http", "www.example.com", "http://example.com/",
       OriginAccessEntry::kDoesNotMatchOrigin,
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"http", "www.example.com", "http://www.example.com/",
       OriginAccessEntry::kMatchesOrigin, OriginAccessEntry::kMatchesOrigin},
      {"http", "www.example.com", "http://www.www.example.com/",
       OriginAccessEntry::kMatchesOrigin, OriginAccessEntry::kMatchesOrigin},
      {"http", "com", "http://example.com/",
       OriginAccessEntry::kMatchesOriginButIsPublicSuffix,
       OriginAccessEntry::kMatchesOriginButIsPublicSuffix},
      {"http", "com", "http://www.example.com/",
       OriginAccessEntry::kMatchesOriginButIsPublicSuffix,
       OriginAccessEntry::kMatchesOriginButIsPublicSuffix},
      {"http", "com", "http://www.www.example.com/",
       OriginAccessEntry::kMatchesOriginButIsPublicSuffix,
       OriginAccessEntry::kMatchesOriginButIsPublicSuffix},
      {"https", "example.com", "http://example.com/",
       OriginAccessEntry::kDoesNotMatchOrigin,
       OriginAccessEntry::kMatchesOrigin},
      {"https", "example.com", "http://www.example.com/",
       OriginAccessEntry::kDoesNotMatchOrigin,
       OriginAccessEntry::kMatchesOrigin},
      {"https", "example.com", "http://www.www.example.com/",
       OriginAccessEntry::kDoesNotMatchOrigin,
       OriginAccessEntry::kMatchesOrigin},
      {"http", "example.com", "http://beispiel.de/",
       OriginAccessEntry::kDoesNotMatchOrigin,
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"http", "example.com", "https://beispiel.de/",
       OriginAccessEntry::kDoesNotMatchOrigin,
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"http", "", "http://example.com/", OriginAccessEntry::kMatchesOrigin,
       OriginAccessEntry::kMatchesOrigin},
      {"http", "", "http://beispiel.de/", OriginAccessEntry::kMatchesOrigin,
       OriginAccessEntry::kMatchesOrigin},
      {"https", "", "http://beispiel.de/",
       OriginAccessEntry::kDoesNotMatchOrigin,
       OriginAccessEntry::kMatchesOrigin},
  };

  ScopedTestingPlatformSupport<OriginAccessEntryTestPlatform> platform;
  platform->SetPublicSuffix("com");

  for (const auto& test : inputs) {
    SCOPED_TRACE(testing::Message()
                 << "Host: " << test.host << ", Origin: " << test.origin);
    scoped_refptr<const SecurityOrigin> origin_to_test =
        SecurityOrigin::CreateFromString(test.origin);
    OriginAccessEntry entry1(test.protocol, test.host,
                             OriginAccessEntry::kAllowSubdomains);
    EXPECT_EQ(test.expected_origin, entry1.MatchesOrigin(*origin_to_test));
    EXPECT_EQ(test.expected_domain, entry1.MatchesDomain(*origin_to_test));
  }
}

TEST(OriginAccessEntryTest, AllowRegisterableDomainsTest) {
  struct TestCase {
    const char* protocol;
    const char* host;
    const char* origin;
    OriginAccessEntry::MatchResult expected;
  } inputs[] = {
      {"http", "example.com", "http://example.com/",
       OriginAccessEntry::kMatchesOrigin},
      {"http", "example.com", "http://www.example.com/",
       OriginAccessEntry::kMatchesOrigin},
      {"http", "example.com", "http://www.www.example.com/",
       OriginAccessEntry::kMatchesOrigin},
      {"http", "www.example.com", "http://example.com/",
       OriginAccessEntry::kMatchesOrigin},
      {"http", "www.example.com", "http://www.example.com/",
       OriginAccessEntry::kMatchesOrigin},
      {"http", "www.example.com", "http://www.www.example.com/",
       OriginAccessEntry::kMatchesOrigin},
      {"http", "com", "http://example.com/",
       OriginAccessEntry::kMatchesOriginButIsPublicSuffix},
      {"http", "com", "http://www.example.com/",
       OriginAccessEntry::kMatchesOriginButIsPublicSuffix},
      {"http", "com", "http://www.www.example.com/",
       OriginAccessEntry::kMatchesOriginButIsPublicSuffix},
      {"https", "example.com", "http://example.com/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"https", "example.com", "http://www.example.com/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"https", "example.com", "http://www.www.example.com/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"http", "example.com", "http://beispiel.de/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"http", "", "http://example.com/", OriginAccessEntry::kMatchesOrigin},
      {"http", "", "http://beispiel.de/", OriginAccessEntry::kMatchesOrigin},
      {"https", "", "http://beispiel.de/",
       OriginAccessEntry::kDoesNotMatchOrigin},
  };

  ScopedTestingPlatformSupport<OriginAccessEntryTestPlatform> platform;
  platform->SetPublicSuffix("com");

  for (const auto& test : inputs) {
    scoped_refptr<const SecurityOrigin> origin_to_test =
        SecurityOrigin::CreateFromString(test.origin);
    OriginAccessEntry entry1(test.protocol, test.host,
                             OriginAccessEntry::kAllowRegisterableDomains);

    SCOPED_TRACE(testing::Message()
                 << "Host: " << test.host << ", Origin: " << test.origin
                 << ", Domain: " << entry1.Registerable().Utf8().data());
    EXPECT_EQ(test.expected, entry1.MatchesOrigin(*origin_to_test));
  }
}

TEST(OriginAccessEntryTest, AllowRegisterableDomainsTestWithDottedSuffix) {
  struct TestCase {
    const char* protocol;
    const char* host;
    const char* origin;
    OriginAccessEntry::MatchResult expected;
  } inputs[] = {
      {"http", "example.appspot.com", "http://example.appspot.com/",
       OriginAccessEntry::kMatchesOrigin},
      {"http", "example.appspot.com", "http://www.example.appspot.com/",
       OriginAccessEntry::kMatchesOrigin},
      {"http", "example.appspot.com", "http://www.www.example.appspot.com/",
       OriginAccessEntry::kMatchesOrigin},
      {"http", "www.example.appspot.com", "http://example.appspot.com/",
       OriginAccessEntry::kMatchesOrigin},
      {"http", "www.example.appspot.com", "http://www.example.appspot.com/",
       OriginAccessEntry::kMatchesOrigin},
      {"http", "www.example.appspot.com", "http://www.www.example.appspot.com/",
       OriginAccessEntry::kMatchesOrigin},
      {"http", "appspot.com", "http://example.appspot.com/",
       OriginAccessEntry::kMatchesOriginButIsPublicSuffix},
      {"http", "appspot.com", "http://www.example.appspot.com/",
       OriginAccessEntry::kMatchesOriginButIsPublicSuffix},
      {"http", "appspot.com", "http://www.www.example.appspot.com/",
       OriginAccessEntry::kMatchesOriginButIsPublicSuffix},
      {"https", "example.appspot.com", "http://example.appspot.com/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"https", "example.appspot.com", "http://www.example.appspot.com/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"https", "example.appspot.com", "http://www.www.example.appspot.com/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"http", "example.appspot.com", "http://beispiel.de/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"http", "", "http://example.appspot.com/",
       OriginAccessEntry::kMatchesOrigin},
      {"http", "", "http://beispiel.de/", OriginAccessEntry::kMatchesOrigin},
      {"https", "", "http://beispiel.de/",
       OriginAccessEntry::kDoesNotMatchOrigin},
  };

  ScopedTestingPlatformSupport<OriginAccessEntryTestPlatform> platform;
  platform->SetPublicSuffix("appspot.com");

  for (const auto& test : inputs) {
    scoped_refptr<const SecurityOrigin> origin_to_test =
        SecurityOrigin::CreateFromString(test.origin);
    OriginAccessEntry entry1(test.protocol, test.host,
                             OriginAccessEntry::kAllowRegisterableDomains);

    SCOPED_TRACE(testing::Message()
                 << "Host: " << test.host << ", Origin: " << test.origin
                 << ", Domain: " << entry1.Registerable().Utf8().data());
    EXPECT_EQ(test.expected, entry1.MatchesOrigin(*origin_to_test));
  }
}

TEST(OriginAccessEntryTest, DisallowSubdomainsTest) {
  struct TestCase {
    const char* protocol;
    const char* host;
    const char* origin;
    OriginAccessEntry::MatchResult expected;
  } inputs[] = {
      {"http", "example.com", "http://example.com/",
       OriginAccessEntry::kMatchesOrigin},
      {"http", "example.com", "http://www.example.com/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"http", "example.com", "http://www.www.example.com/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"http", "com", "http://example.com/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"http", "com", "http://www.example.com/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"http", "com", "http://www.www.example.com/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"https", "example.com", "http://example.com/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"https", "example.com", "http://www.example.com/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"https", "example.com", "http://www.www.example.com/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"http", "example.com", "http://beispiel.de/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"http", "", "http://example.com/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"http", "", "http://beispiel.de/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"https", "", "http://beispiel.de/",
       OriginAccessEntry::kDoesNotMatchOrigin},
  };

  ScopedTestingPlatformSupport<OriginAccessEntryTestPlatform> platform;
  platform->SetPublicSuffix("com");

  for (const auto& test : inputs) {
    SCOPED_TRACE(testing::Message()
                 << "Host: " << test.host << ", Origin: " << test.origin);
    scoped_refptr<const SecurityOrigin> origin_to_test =
        SecurityOrigin::CreateFromString(test.origin);
    OriginAccessEntry entry1(test.protocol, test.host,
                             OriginAccessEntry::kDisallowSubdomains);
    EXPECT_EQ(test.expected, entry1.MatchesOrigin(*origin_to_test));
  }
}

TEST(OriginAccessEntryTest, IPAddressTest) {
  struct TestCase {
    const char* protocol;
    const char* host;
    bool is_ip_address;
  } inputs[] = {
      {"http", "1.1.1.1", true},
      {"http", "1.1.1.1.1", false},
      {"http", "example.com", false},
      {"http", "hostname.that.ends.with.a.number1", false},
      {"http", "2001:db8::1", false},
      {"http", "[2001:db8::1]", true},
      {"http", "2001:db8::a", false},
      {"http", "[2001:db8::a]", true},
      {"http", "", false},
  };

  ScopedTestingPlatformSupport<OriginAccessEntryTestPlatform> platform;
  platform->SetPublicSuffix("com");

  for (const auto& test : inputs) {
    SCOPED_TRACE(testing::Message() << "Host: " << test.host);
    OriginAccessEntry entry(test.protocol, test.host,
                            OriginAccessEntry::kDisallowSubdomains);
    EXPECT_EQ(test.is_ip_address, entry.HostIsIPAddress()) << test.host;
  }
}

TEST(OriginAccessEntryTest, IPAddressMatchingTest) {
  struct TestCase {
    const char* protocol;
    const char* host;
    const char* origin;
    OriginAccessEntry::MatchResult expected;
  } inputs[] = {
      {"http", "192.0.0.123", "http://192.0.0.123/",
       OriginAccessEntry::kMatchesOrigin},
      {"http", "0.0.123", "http://192.0.0.123/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"http", "0.123", "http://192.0.0.123/",
       OriginAccessEntry::kDoesNotMatchOrigin},
      {"http", "1.123", "http://192.0.0.123/",
       OriginAccessEntry::kDoesNotMatchOrigin},
  };

  ScopedTestingPlatformSupport<OriginAccessEntryTestPlatform> platform;
  platform->SetPublicSuffix("com");

  for (const auto& test : inputs) {
    SCOPED_TRACE(testing::Message()
                 << "Host: " << test.host << ", Origin: " << test.origin);
    scoped_refptr<const SecurityOrigin> origin_to_test =
        SecurityOrigin::CreateFromString(test.origin);
    OriginAccessEntry entry1(test.protocol, test.host,
                             OriginAccessEntry::kAllowSubdomains);
    EXPECT_EQ(test.expected, entry1.MatchesOrigin(*origin_to_test));

    OriginAccessEntry entry2(test.protocol, test.host,
                             OriginAccessEntry::kDisallowSubdomains);
    EXPECT_EQ(test.expected, entry2.MatchesOrigin(*origin_to_test));
  }
}

}  // namespace blink
