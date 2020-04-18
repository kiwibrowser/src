// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/public/provider/chrome/browser/signin/signin_resources_provider.h"

#include <MacTypes.h>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios {

SigninResourcesProvider::SigninResourcesProvider() {
}

SigninResourcesProvider::~SigninResourcesProvider() {
}

UIImage* SigninResourcesProvider::GetDefaultAvatar() {
  return nil;
}

NSString* SigninResourcesProvider::GetLocalizedString(
    SigninStringID string_id) {
  return nil;
}

}  // namespace ios
