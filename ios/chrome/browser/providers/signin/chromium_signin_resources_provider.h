// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PROVIDERS_SIGNIN_CHROMIUM_SIGNIN_RESOURCES_PROVIDER_H_
#define IOS_CHROME_BROWSER_PROVIDERS_SIGNIN_CHROMIUM_SIGNIN_RESOURCES_PROVIDER_H_

#include "base/macros.h"
#include "ios/public/provider/chrome/browser/signin/signin_resources_provider.h"

// ChromiumSigninResourcesProvider provides resources that the app expects to
// exist, even when signin is disabled.
class ChromiumSigninResourcesProvider : public ios::SigninResourcesProvider {
 public:
  ChromiumSigninResourcesProvider();
  ~ChromiumSigninResourcesProvider() override;

  // SigninResourcesProvider implementation:
  UIImage* GetDefaultAvatar() override;
  NSString* GetLocalizedString(ios::SigninStringID string_id) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromiumSigninResourcesProvider);
};

#endif  // IOS_CHROME_BROWSER_PROVIDERS_SIGNIN_CHROMIUM_SIGNIN_RESOURCES_PROVIDER_H_
