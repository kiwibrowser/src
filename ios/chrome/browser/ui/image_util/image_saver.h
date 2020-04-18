// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_IMAGE_UTIL_IMAGE_SAVER_H_
#define IOS_CHROME_BROWSER_UI_IMAGE_UTIL_IMAGE_SAVER_H_

#import <UIKit/UIKit.h>

#include "components/image_fetcher/core/request_metadata.h"

// Object saving images to the system's album.
@interface ImageSaver : NSObject

// Init the ImageSaver with a |baseViewController| used to display alerts.
- (instancetype)initWithBaseViewController:
    (UIViewController*)baseViewController;

// Saves the image's |data|, with |metadata| to the system's album.
- (void)saveImageData:(NSData*)data
         withMetadata:(const image_fetcher::RequestMetadata&)metadata;

@end

#endif  // IOS_CHROME_BROWSER_UI_IMAGE_UTIL_IMAGE_SAVER_H_
