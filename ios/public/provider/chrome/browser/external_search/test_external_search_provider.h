// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_EXTERNAL_SEARCH_TEST_EXTERNAL_SEARCH_PROVIDER_H
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_EXTERNAL_SEARCH_TEST_EXTERNAL_SEARCH_PROVIDER_H

#import <Foundation/Foundation.h>

#import "ios/public/provider/chrome/browser/external_search/external_search_provider.h"

class TestExternalSearchProvider : public ExternalSearchProvider {
 public:
  bool IsExternalSearchEnabled() override;
  NSURL* GetLaunchURL() override;
  NSString* GetButtonImageName() override;
  int GetButtonIdsAccessibilityLabel() override;
  NSString* GetButtonAccessibilityIdentifier() override;
};

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_EXTERNAL_SEARCH_TEST_EXTERNAL_SEARCH_PROVIDER_H
