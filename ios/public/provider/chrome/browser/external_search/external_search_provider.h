// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_EXTERNAL_SEARCH_EXTERNAL_SEARCH_PROVIDER_H
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_EXTERNAL_SEARCH_EXTERNAL_SEARCH_PROVIDER_H

#import <Foundation/Foundation.h>

#import "base/macros.h"

class ExternalSearchProvider {
 public:
  ExternalSearchProvider() {}
  virtual ~ExternalSearchProvider() {}

  // Returns if the provider provides valid values on all other methods.
  virtual bool IsExternalSearchEnabled();

  // Returns the URL to open to launch the External Search app.
  virtual NSURL* GetLaunchURL();

  // Returns the image to display on the button to trigger an External Search,
  virtual NSString* GetButtonImageName();

  // Returns the accessibility label message ID to set on the button to trigger
  // an External Search,
  virtual int GetButtonIdsAccessibilityLabel();

  // Returns the accessibility identifier to set on the button to trigger an
  // External Search,
  virtual NSString* GetButtonAccessibilityIdentifier();

 private:
  DISALLOW_COPY_AND_ASSIGN(ExternalSearchProvider);
};

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_EXTERNAL_SEARCH_EXTERNAL_SEARCH_PROVIDER_H
