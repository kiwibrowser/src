// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_SPOTLIGHT_TEST_SPOTLIGHT_PROVIDER_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_SPOTLIGHT_TEST_SPOTLIGHT_PROVIDER_H_

#import <Foundation/Foundation.h>

#import "ios/public/provider/chrome/browser/spotlight/spotlight_provider.h"

class TestSpotlightProvider : public SpotlightProvider {
 public:
  bool IsSpotlightEnabled() override;
  NSString* GetBookmarkDomain() override;
  NSString* GetTopSitesDomain() override;
  NSString* GetActionsDomain() override;
  NSString* GetCustomAttributeItemID() override;
  NSArray* GetAdditionalKeywords() override;
};

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_SPOTLIGHT_TEST_SPOTLIGHT_PROVIDER_H_
