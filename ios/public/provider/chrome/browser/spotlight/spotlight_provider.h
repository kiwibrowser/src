// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_SPOTLIGHT_SPOTLIGHT_PROVIDER_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_SPOTLIGHT_SPOTLIGHT_PROVIDER_H_

#import <Foundation/Foundation.h>

#import "base/macros.h"

class SpotlightProvider {
 public:
  SpotlightProvider() {}
  virtual ~SpotlightProvider() {}

  // Returns if the provider provides valid values on all other methods.
  virtual bool IsSpotlightEnabled();

  // Returns the spotlight domain for bookmarks items.
  virtual NSString* GetBookmarkDomain();

  // Returns the spotlight domain for top sites items.
  virtual NSString* GetTopSitesDomain();

  // Returns the spotlight domain for action items.
  virtual NSString* GetActionsDomain();

  // Returns the id for the custom item containing item ID in Spotlight index.
  virtual NSString* GetCustomAttributeItemID();

  // Returns additional keywords that are added to all Spotlight items.
  virtual NSArray* GetAdditionalKeywords();

 private:
  DISALLOW_COPY_AND_ASSIGN(SpotlightProvider);
};

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_SPOTLIGHT_SPOTLIGHT_PROVIDER_H_
