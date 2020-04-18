// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/public/provider/chrome/browser/signin/test_signin_resources_provider.h"

#import <UIKit/UIKit.h>

#include "ui/base/test/ios/ui_image_test_utils.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

TestSigninResourcesProvider::TestSigninResourcesProvider()
    : default_avatar_(ui::test::uiimage_utils::UIImageWithSizeAndSolidColor(
          CGSizeMake(32, 32),
          UIColor.whiteColor)) {
  // A real UIImage for default_avatar_ is required to be cached/resized.
}

TestSigninResourcesProvider::~TestSigninResourcesProvider() {}

UIImage* TestSigninResourcesProvider::GetDefaultAvatar() {
  return default_avatar_;
}

NSString* TestSigninResourcesProvider::GetLocalizedString(
    ios::SigninStringID string_id) {
  return @"Test";
}
