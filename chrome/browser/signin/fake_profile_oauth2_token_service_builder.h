// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_FAKE_PROFILE_OAUTH2_TOKEN_SERVICE_BUILDER_H_
#define CHROME_BROWSER_SIGNIN_FAKE_PROFILE_OAUTH2_TOKEN_SERVICE_BUILDER_H_

#include <memory>

class KeyedService;

namespace content {
class BrowserContext;
}

// Helper function to be used with
// BrowserContextKeyedServiceFactory::SetTestingFactory() that returns a
// FakeProfileOAuth2TokenService object.
std::unique_ptr<KeyedService> BuildFakeProfileOAuth2TokenService(
    content::BrowserContext* context);

// Helper function to be used with
// BrowserContextKeyedServiceFactory::SetTestingFactory() that creates a
// FakeProfileOAuth2TokenService object that posts fetch responses on the
// current message loop.
std::unique_ptr<KeyedService> BuildAutoIssuingFakeProfileOAuth2TokenService(
    content::BrowserContext* context);

#endif  // CHROME_BROWSER_SIGNIN_FAKE_PROFILE_OAUTH2_TOKEN_SERVICE_BUILDER_H_
