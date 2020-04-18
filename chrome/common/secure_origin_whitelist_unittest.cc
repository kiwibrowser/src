// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/test/scoped_command_line.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/common/origin_util.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using content::IsOriginSecure;

namespace chrome {

class SecureOriginWhiteListTest : public testing::Test {
  void TearDown() override {
    // Ensure that we reset the whitelisted origins without any flags applied.
    content::ResetSchemesAndOriginsWhitelist();
  };
};

TEST_F(SecureOriginWhiteListTest, UnsafelyTreatInsecureOriginAsSecure) {
  EXPECT_FALSE(content::IsOriginSecure(GURL("http://example.com/a.html")));
  EXPECT_FALSE(
      content::IsOriginSecure(GURL("http://127.example.com/a.html")));
  // Add http://example.com and http://127.example.com to whitelist by
  // command-line and see if they are now considered secure origins.
  // (The command line is applied via
  // ChromeContentClient::AddSecureSchemesAndOrigins)
  base::test::ScopedCommandLine scoped_command_line;
  base::CommandLine* command_line = scoped_command_line.GetProcessCommandLine();
  command_line->AppendSwitchASCII(
      switches::kUnsafelyTreatInsecureOriginAsSecure,
      "http://example.com,http://127.example.com");
  content::ResetSchemesAndOriginsWhitelist();

  // They should be now white-listed.
  EXPECT_TRUE(content::IsOriginSecure(GURL("http://example.com/a.html")));
  EXPECT_TRUE(content::IsOriginSecure(GURL("http://127.example.com/a.html")));

  // Check that similarly named sites are not considered secure.
  EXPECT_FALSE(content::IsOriginSecure(GURL("http://128.example.com/a.html")));
  EXPECT_FALSE(content::IsOriginSecure(
      GURL("http://foobar.127.example.com/a.html")));
}

TEST_F(SecureOriginWhiteListTest, HostnamePatterns) {
  const struct HostnamePatternCase {
    const char* pattern;
    const char* test_input;
    bool expected_secure;
  } kTestCases[] = {
      {"*.foo.com", "http://bar.foo.com", true},
      {"*.foo.*.bar.com", "http://a.foo.b.bar.com:8000", true},
      // For parsing/canonicalization simplicity, wildcard patterns can be
      // hostnames only, not full origins.
      {"http://*.foo.com", "http://bar.foo.com", false},
      {"*://foo.com", "http://foo.com", false},
      // Wildcards must be beyond eTLD+1.
      {"*.co.uk", "http://foo.co.uk", false},
      {"*.co.uk", "http://co.uk", false},
      {"*.baz", "http://foo.baz", false},
      {"foo.*.com", "http://foo.bar.com", false},
      {"*.foo.baz", "http://a.foo.baz", true},
      // Hostname patterns should be canonicalized.
      {"*.FoO.com", "http://a.foo.com", true},
      {"%2A.foo.com", "http://a.foo.com", false},
      // Hostname patterns must contain a wildcard and a wildcard can only
      // replace a component, not a part of a component.
      {"foo.com", "http://foo.com", false},
      {"test*.foo.com", "http://testblah.foo.com", false},
      {"*foo.com", "http://testfoo.com", false},
      {"foo*.com", "http://footest.com", false},
  };

  for (const auto& test : kTestCases) {
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII(
        switches::kUnsafelyTreatInsecureOriginAsSecure, test.pattern);
    content::ResetSchemesAndOriginsWhitelist();
    EXPECT_EQ(test.expected_secure,
              content::IsOriginSecure(GURL(test.test_input)));
    EXPECT_EQ(test.expected_secure,
              content::IsPotentiallyTrustworthyOrigin(
                  url::Origin::Create(GURL(test.test_input))));
  }
}

}  // namespace chrome
