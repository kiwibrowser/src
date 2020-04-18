// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/rappor/public/rappor_utils.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace rappor {

// Test that extracting the sample works correctly for different schemes.
TEST(RapporSamplingTest, GetDomainAndRegistrySampleFromGURLTest) {
  EXPECT_EQ("google.com", GetDomainAndRegistrySampleFromGURL(
      GURL("https://www.GoOgLe.com:80/blah")));
  EXPECT_EQ("file://", GetDomainAndRegistrySampleFromGURL(
      GURL("file://foo/bar/baz")));
  EXPECT_EQ("chrome-extension://abc1234", GetDomainAndRegistrySampleFromGURL(
      GURL("chrome-extension://abc1234/foo.html")));
  EXPECT_EQ("chrome-search://local-ntp", GetDomainAndRegistrySampleFromGURL(
      GURL("chrome-search://local-ntp/local-ntp.html")));
  EXPECT_EQ("localhost", GetDomainAndRegistrySampleFromGURL(
      GURL("http://localhost:8000/foo.html")));
  EXPECT_EQ("localhost", GetDomainAndRegistrySampleFromGURL(
      GURL("http://127.0.0.1/foo.html")));
  EXPECT_EQ("localhost",
            GetDomainAndRegistrySampleFromGURL(GURL("http://[::1]/foo.html")));
  EXPECT_EQ("ip_address", GetDomainAndRegistrySampleFromGURL(
      GURL("http://192.168.0.1/foo.html")));
  EXPECT_EQ("ip_address", GetDomainAndRegistrySampleFromGURL(
      GURL("http://[2001:db8::1]/")));
  EXPECT_EQ("", GetDomainAndRegistrySampleFromGURL(
      GURL("http://www/")));
  EXPECT_EQ("www.corp", GetDomainAndRegistrySampleFromGURL(
      GURL("http://www.corp/")));
}

// Make sure recording a sample during tests, when the Rappor service is NULL,
// doesn't cause a crash.
TEST(RapporSamplingTest, SmokeTest) {
  SampleDomainAndRegistryFromGURL(nullptr, std::string(), GURL());
}

}  // namespace rappor
