// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_SHARE_TO_DATA_H_
#define IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_SHARE_TO_DATA_H_

#import <UIKit/UIKit.h>

#include "ios/chrome/browser/ui/activity_services/chrome_activity_item_thumbnail_generator.h"
#include "url/gurl.h"

@interface ShareToData : NSObject

// Designated initializer.
- (id)initWithShareURL:(const GURL&)shareURL
            visibleURL:(const GURL&)visibleURL
                 title:(NSString*)title
       isOriginalTitle:(BOOL)isOriginalTitle
       isPagePrintable:(BOOL)isPagePrintable
    thumbnailGenerator:(ThumbnailGeneratorBlock)thumbnailGenerator;

// The URL to be shared with share extensions. This URL is the canonical URL of
// the page.
@property(nonatomic, readonly) const GURL& shareURL;
// The visible URL of the page.
@property(nonatomic, readonly) const GURL& visibleURL;

// NSURL versions of 'shareURL' and 'passwordManagerURL'. Use only for passing
// to libraries that take NSURL.
@property(nonatomic, readonly) NSURL* shareNSURL;
@property(nonatomic, readonly) NSURL* passwordManagerNSURL;

@property(nonatomic, readonly, copy) NSString* title;
@property(nonatomic, readonly, assign) BOOL isOriginalTitle;
@property(nonatomic, readonly, assign) BOOL isPagePrintable;
@property(nonatomic, strong) UIImage* image;
@property(nonatomic, copy) ThumbnailGeneratorBlock thumbnailGenerator;

@end

#endif  // IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_SHARE_TO_DATA_H_
