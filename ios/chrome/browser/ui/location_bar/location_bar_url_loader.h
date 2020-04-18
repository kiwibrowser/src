// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_LOCATION_BAR_URL_LOADER_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_LOCATION_BAR_URL_LOADER_H_

#import <Foundation/Foundation.h>

#include "ui/base/page_transition_types.h"

class GURL;

// A means of loading URLs for the location bar.
@protocol LocationBarURLLoader
- (void)loadGURLFromLocationBar:(const GURL&)url
                     transition:(ui::PageTransition)transition;
@end

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_LOCATION_BAR_URL_LOADER_H_
