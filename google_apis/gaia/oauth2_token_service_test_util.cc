// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gaia/oauth2_token_service_test_util.h"

#include "base/strings/stringprintf.h"

namespace {
const char kValidTokenResponse[] =
    "{"
    "  \"access_token\": \"%s\","
    "  \"expires_in\": %d,"
    "  \"token_type\": \"Bearer\""
    "}";
}

std::string GetValidTokenResponse(const std::string& token, int expiration) {
  return base::StringPrintf(kValidTokenResponse, token.c_str(), expiration);
}

TestingOAuth2TokenServiceConsumer::TestingOAuth2TokenServiceConsumer()
    : OAuth2TokenService::Consumer("test"),
      number_of_successful_tokens_(0),
      last_error_(GoogleServiceAuthError::AuthErrorNone()),
      number_of_errors_(0) {
}

TestingOAuth2TokenServiceConsumer::~TestingOAuth2TokenServiceConsumer() {
}

void TestingOAuth2TokenServiceConsumer::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& token,
    const base::Time& expiration_date) {
  last_token_ = token;
  ++number_of_successful_tokens_;
}

void TestingOAuth2TokenServiceConsumer::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  last_error_ = error;
  ++number_of_errors_;
}
