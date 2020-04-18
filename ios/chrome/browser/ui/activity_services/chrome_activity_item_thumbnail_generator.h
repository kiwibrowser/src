// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_CHROME_ACTIVITY_ITEM_THUMBNAIL_GENERATOR_H_
#define IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_CHROME_ACTIVITY_ITEM_THUMBNAIL_GENERATOR_H_

#import <CoreGraphics/CoreGraphics.h>
#import <UIKit/UIKit.h>

@class Tab;

// Block returning a thumbnail at the specified size. May return nil.
typedef UIImage* (^ThumbnailGeneratorBlock)(const CGSize&);

namespace activity_services {

// Returns a thumbnail generator for the tab |tab|. |tab| must not be nil.
ThumbnailGeneratorBlock ThumbnailGeneratorForTab(Tab* tab);

}  // namespace activity_services

#endif  // IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_CHROME_ACTIVITY_ITEM_THUMBNAIL_GENERATOR_H_
