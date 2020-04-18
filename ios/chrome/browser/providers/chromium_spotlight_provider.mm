// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/providers/chromium_spotlight_provider.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

bool ChromiumSpotlightProvider::IsSpotlightEnabled() {
  return true;
}

NSString* ChromiumSpotlightProvider::GetBookmarkDomain() {
  return @"org.chromium.bookmarks";
}

NSString* ChromiumSpotlightProvider::GetTopSitesDomain() {
  return @"org.chromium.topsites";
}

NSString* ChromiumSpotlightProvider::GetActionsDomain() {
  return @"org.chromium.actions";
}

NSString* ChromiumSpotlightProvider::GetCustomAttributeItemID() {
  return @"OrgChromiumItemID";
}

NSArray* ChromiumSpotlightProvider::GetAdditionalKeywords() {
  return @[ @"chromium" ];
}
