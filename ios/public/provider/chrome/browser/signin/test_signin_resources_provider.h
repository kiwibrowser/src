// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_SIGNIN_TEST_SIGNIN_RESOURCES_PROVIDER_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_SIGNIN_TEST_SIGNIN_RESOURCES_PROVIDER_H_

#include "ios/public/provider/chrome/browser/signin/signin_resources_provider.h"


@class UIImage;

class TestSigninResourcesProvider : public ios::SigninResourcesProvider {
 public:
  TestSigninResourcesProvider();
  ~TestSigninResourcesProvider() override;

  UIImage* GetDefaultAvatar() override;
  NSString* GetLocalizedString(ios::SigninStringID string_id) override;

 private:
  UIImage* default_avatar_;

  DISALLOW_COPY_AND_ASSIGN(TestSigninResourcesProvider);
};

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_SIGNIN_TEST_SIGNIN_RESOURCES_PROVIDER_H_
