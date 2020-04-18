// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/fake_refresh_token_store.h"

namespace {
const char kRefreshTokenValue[] = "1/lkjalseLKJlsiJgr45jbv";
}

namespace remoting {
namespace test {

FakeRefreshTokenStore::FakeRefreshTokenStore()
    : refresh_token_value_(kRefreshTokenValue),
      refresh_token_write_succeeded_(true),
      refresh_token_write_attempted_(false) {
}

FakeRefreshTokenStore::~FakeRefreshTokenStore() = default;

std::string FakeRefreshTokenStore::FetchRefreshToken() {
  return refresh_token_value_;
}

bool FakeRefreshTokenStore::StoreRefreshToken(
    const std::string& refresh_token) {
  // Record the information passed to us to write.
  refresh_token_write_attempted_ = true;
  stored_refresh_token_value_ = refresh_token;

  return refresh_token_write_succeeded_;
}

}  // namespace test
}  // namespace remoting
