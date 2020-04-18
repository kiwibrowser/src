// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/fake_account_fetcher_service_builder.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/suggestions/image_decoder_impl.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/chrome_signin_client_factory.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "components/signin/core/browser/fake_account_fetcher_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"

// static
std::unique_ptr<KeyedService> FakeAccountFetcherServiceBuilder::BuildForTests(
    content::BrowserContext* context) {
  FakeAccountFetcherService* service = new FakeAccountFetcherService();
  Profile* profile = Profile::FromBrowserContext(context);
  service->Initialize(ChromeSigninClientFactory::GetForProfile(profile),
                      ProfileOAuth2TokenServiceFactory::GetForProfile(profile),
                      AccountTrackerServiceFactory::GetForProfile(profile),
                      std::make_unique<suggestions ::ImageDecoderImpl>());
  return std::unique_ptr<KeyedService>(service);
}
