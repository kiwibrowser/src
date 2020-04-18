// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gaia/oauth2_id_token_decoder.h"

#include <string>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kIdTokenInvalidJwt[] =
    "dummy-header."
    "..."
    ".dummy-signature";
const char kIdTokenInvalidJson[] =
    "dummy-header."
    "YWJj"  // payload: abc
    ".dummy-signature";
const char kIdTokenEmptyServices[] =
    "dummy-header."
    "eyAic2VydmljZXMiOiBbXSB9"  // payload: { "services": [] }
    ".dummy-signature";
const char kIdTokenEmptyServicesHeaderSignature[] =
    "."
    "eyAic2VydmljZXMiOiBbXSB9"  // payload: { "services": [] }
    ".";
const char kIdTokenMissingServices[] =
    "dummy-header."
    "eyAiYWJjIjogIiJ9"  // payload: { "abc": ""}
    ".dummy-signature";
const char kIdTokenNotChildAccount[] =
    "dummy-header."
    "eyAic2VydmljZXMiOiBbImFiYyJdIH0="  // payload: { "services": ["abc"] }
    ".dummy-signature";
const char kIdTokenChildAccount[] =
    "dummy-header."
    "eyAic2VydmljZXMiOiBbInVjYSJdIH0="  // payload: { "services": ["uca"] }
    ".dummy-signature";

class OAuth2IdTokenDecoderTest : public testing::Test {};

TEST_F(OAuth2IdTokenDecoderTest, Invalid) {
  std::string id_token_ = kIdTokenInvalidJwt;
  bool is_child_account = gaia::IsChildAccountFromIdToken(id_token_);
  EXPECT_EQ(is_child_account, false);

  id_token_ = kIdTokenInvalidJson;
  is_child_account = gaia::IsChildAccountFromIdToken(id_token_);
  EXPECT_EQ(is_child_account, false);

  id_token_ = kIdTokenMissingServices;
  is_child_account = gaia::IsChildAccountFromIdToken(id_token_);
  EXPECT_EQ(is_child_account, false);
}

TEST_F(OAuth2IdTokenDecoderTest, NotChild) {
  std::string id_token_ = kIdTokenEmptyServices;
  bool is_child_account = gaia::IsChildAccountFromIdToken(id_token_);
  EXPECT_EQ(is_child_account, false);

  id_token_ = kIdTokenEmptyServicesHeaderSignature;
  is_child_account = gaia::IsChildAccountFromIdToken(id_token_);
  EXPECT_EQ(is_child_account, false);

  id_token_ = kIdTokenNotChildAccount;
  is_child_account = gaia::IsChildAccountFromIdToken(id_token_);
  EXPECT_EQ(is_child_account, false);
}

TEST_F(OAuth2IdTokenDecoderTest, Child) {
  std::string id_token_ = kIdTokenChildAccount;
  bool is_child_account = gaia::IsChildAccountFromIdToken(id_token_);
  EXPECT_EQ(is_child_account, true);
}

}  // namespace
