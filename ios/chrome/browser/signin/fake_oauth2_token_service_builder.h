// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_FAKE_OAUTH2_TOKEN_SERVICE_BUILDER_H_
#define IOS_CHROME_BROWSER_SIGNIN_FAKE_OAUTH2_TOKEN_SERVICE_BUILDER_H_

#include <memory>

namespace web {
class BrowserState;
}

class KeyedService;

// Helper function to be used with
// BrowserStateKeyedServiceFactory::SetTestingFactory() that returns a
// FakeProfileOAuth2TokenService object.
std::unique_ptr<KeyedService> BuildFakeOAuth2TokenService(
    web::BrowserState* context);

#endif  // IOS_CHROME_BROWSER_SIGNIN_FAKE_OAUTH2_TOKEN_SERVICE_BUILDER_H_
