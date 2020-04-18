// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_FAKE_ACCOUNT_FETCHER_SERVICE_BUILDER_H_
#define CHROME_BROWSER_SIGNIN_FAKE_ACCOUNT_FETCHER_SERVICE_BUILDER_H_

#include <memory>

#include "base/macros.h"

namespace content {
class BrowserContext;
}
class KeyedService;

class FakeAccountFetcherServiceBuilder {
 public:
  static std::unique_ptr<KeyedService> BuildForTests(
      content::BrowserContext* context);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(FakeAccountFetcherServiceBuilder);
};

#endif  // CHROME_BROWSER_SIGNIN_FAKE_ACCOUNT_FETCHER_SERVICE_BUILDER_H_
