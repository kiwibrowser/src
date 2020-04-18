// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_CORE_BROWSER_CHILD_ACCOUNT_INFO_FETCHER_H_
#define COMPONENTS_SIGNIN_CORE_BROWSER_CHILD_ACCOUNT_INFO_FETCHER_H_

#include <memory>
#include <string>

#include "build/build_config.h"

#if defined(OS_ANDROID)
#include <jni.h>
#endif

namespace invalidation {
class InvalidationService;
}
namespace net {
class URLRequestContextGetter;
}
class AccountFetcherService;
class OAuth2TokenService;

class ChildAccountInfoFetcher {
 public:
  // Caller takes ownership of the fetcher and keeps it alive in order to
  // receive updates.
  static std::unique_ptr<ChildAccountInfoFetcher> CreateFrom(
      const std::string& account_id,
      AccountFetcherService* fetcher_service,
      OAuth2TokenService* token_service,
      net::URLRequestContextGetter* request_context_getter,
      invalidation::InvalidationService* invalidation_service);
  virtual ~ChildAccountInfoFetcher();

  static void InitializeForTests();
};

#endif  // COMPONENTS_SIGNIN_CORE_BROWSER_CHILD_ACCOUNT_INFO_FETCHER_H_
