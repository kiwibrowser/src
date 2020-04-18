// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/mock_access_token_fetcher.h"

#include <utility>

namespace remoting {
namespace test {

using ::testing::_;
using ::testing::Invoke;

MockAccessTokenFetcher::MockAccessTokenFetcher() = default;
MockAccessTokenFetcher::~MockAccessTokenFetcher() = default;

void MockAccessTokenFetcher::SetAccessTokenFetcher(
    std::unique_ptr<AccessTokenFetcher> fetcher) {
  internal_access_token_fetcher_ = std::move(fetcher);

  ON_CALL(*this, GetAccessTokenFromAuthCode(_, _))
      .WillByDefault(Invoke(internal_access_token_fetcher_.get(),
                            &AccessTokenFetcher::GetAccessTokenFromAuthCode));
  ON_CALL(*this, GetAccessTokenFromRefreshToken(_, _))
      .WillByDefault(
          Invoke(internal_access_token_fetcher_.get(),
                 &AccessTokenFetcher::GetAccessTokenFromRefreshToken));
}

}  // namespace test
}  // namespace remoting
