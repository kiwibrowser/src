// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/public/provider/chrome/browser/external_search/external_search_provider.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

bool ExternalSearchProvider::IsExternalSearchEnabled() {
  return false;
}

NSURL* ExternalSearchProvider::GetLaunchURL() {
  return nil;
}

NSString* ExternalSearchProvider::GetButtonImageName() {
  return nil;
}

int ExternalSearchProvider::GetButtonIdsAccessibilityLabel() {
  return 0;
}

NSString* ExternalSearchProvider::GetButtonAccessibilityIdentifier() {
  return nil;
}
