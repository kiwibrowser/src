// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_CRYPTAUTH_ACCESS_TOKEN_FETCHER_H_
#define COMPONENTS_CRYPTAUTH_CRYPTAUTH_ACCESS_TOKEN_FETCHER_H_

#include <string>

#include "base/callback_forward.h"

namespace cryptauth {

// Simple interface for fetching the OAuth2 access token that authorizes
// CryptAuth API calls. Do not reuse this after calling FetchAccessToken();
// instead, create a new instance.
class CryptAuthAccessTokenFetcher {
 public:
  virtual ~CryptAuthAccessTokenFetcher() {}

  // Fetches the access token asynchronously, invoking the callback upon
  // completion. If the fetch fails, the callback will be invoked with an empty
  // string.
  typedef base::Callback<void(const std::string&)> AccessTokenCallback;
  virtual void FetchAccessToken(const AccessTokenCallback& callback) = 0;
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_CRYPTAUTH_ACCESS_TOKEN_FETCHER_H_
