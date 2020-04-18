// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/public/provider/chrome/browser/signin/signin_error_provider.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios {

SigninErrorProvider::SigninErrorProvider() {}

SigninErrorProvider::~SigninErrorProvider() {}

SigninErrorCategory SigninErrorProvider::GetErrorCategory(NSError* error) {
  return SigninErrorCategory::UNKNOWN_ERROR;
}

bool SigninErrorProvider::IsCanceled(NSError* error) {
  return false;
}

bool SigninErrorProvider::IsForbidden(NSError* error) {
  return false;
}

bool SigninErrorProvider::IsBadRequest(NSError* error) {
  return false;
}

NSString* SigninErrorProvider::GetInvalidGrantJsonErrorKey() {
  return @"invalid_grant_error_key";
}

NSString* SigninErrorProvider::GetSigninErrorDomain() {
  return @"signin_error_domain";
}

int SigninErrorProvider::GetCode(SigninError error) {
  return 0;
}

}  // namesace ios
