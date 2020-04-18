// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_cookie_access_policy.h"

#include "testing/gtest/include/gtest/gtest.h"

class GURL;

using android_webview::AwStaticCookiePolicy;
using testing::Test;

class AwStaticCookiePolicyTest : public Test {
 public:
  static const GURL kUrlFirstParty;
  static const GURL kUrlThirdParty;

  AwStaticCookiePolicyTest() {}

  void expectFirstPartyAccess(const AwStaticCookiePolicy& policy,
                              bool expectedResult) {
    EXPECT_EQ(expectedResult, policy.AllowSet(kUrlFirstParty, kUrlFirstParty));
    EXPECT_EQ(expectedResult, policy.AllowGet(kUrlFirstParty, kUrlFirstParty));
  }

  void expectThirdPartyAccess(const AwStaticCookiePolicy& policy,
                              bool expectedResult) {
    EXPECT_EQ(expectedResult, policy.AllowSet(kUrlFirstParty, kUrlThirdParty));
    EXPECT_EQ(expectedResult, policy.AllowGet(kUrlFirstParty, kUrlThirdParty));
  }
};

const GURL AwStaticCookiePolicyTest::kUrlFirstParty =
    GURL("http://first.example");
const GURL AwStaticCookiePolicyTest::kUrlThirdParty =
    GURL("http://third.example");

TEST_F(AwStaticCookiePolicyTest, BlockAllCookies) {
  AwStaticCookiePolicy policy(false /* allow_cookies */,
                              false /* allow_third_party_cookies */);
  expectFirstPartyAccess(policy, false);
  expectThirdPartyAccess(policy, false);
}

TEST_F(AwStaticCookiePolicyTest, BlockAllCookiesWithThirdPartySet) {
  AwStaticCookiePolicy policy(false /* allow_cookies */,
                              true  /* allow_third_party_cookies */);
  expectFirstPartyAccess(policy, false);
  expectThirdPartyAccess(policy, false);
}

TEST_F(AwStaticCookiePolicyTest, FirstPartyCookiesOnly) {
  AwStaticCookiePolicy policy(true  /* allow_cookies */,
                              false /* allow_third_party_cookies */);
  expectFirstPartyAccess(policy, true);
  expectThirdPartyAccess(policy, false);
}

TEST_F(AwStaticCookiePolicyTest, AllowAllCookies) {
  AwStaticCookiePolicy policy(true /* allow_cookies */,
                              true /* allow_third_party_cookies */);
  expectFirstPartyAccess(policy, true);
  expectThirdPartyAccess(policy, true);
}

