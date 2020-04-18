// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_APIS_GAIA_OAUTH2_ACCESS_TOKEN_CONSUMER_H_
#define GOOGLE_APIS_GAIA_OAUTH2_ACCESS_TOKEN_CONSUMER_H_

#include <string>

class GoogleServiceAuthError;

namespace base {
class Time;
}

// An interface that defines the callbacks for consumers to which
// OAuth2AccessTokenFetcher can return results.
class OAuth2AccessTokenConsumer {
 public:
  // Success callback. |access_token| will contain a valid OAuth2 access token.
  // |expiration_time| is the date until which the token can be used. This
  // value has a built-in safety margin, so it can be used as-is.
  virtual void OnGetTokenSuccess(const std::string& access_token,
                                 const base::Time& expiration_time) {}
  virtual void OnGetTokenFailure(const GoogleServiceAuthError& error) {}

 protected:
  virtual ~OAuth2AccessTokenConsumer() {}
};

#endif  // GOOGLE_APIS_GAIA_OAUTH2_ACCESS_TOKEN_CONSUMER_H_
