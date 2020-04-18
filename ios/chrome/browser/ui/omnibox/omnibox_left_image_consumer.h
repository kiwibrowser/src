// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_LEFT_IMAGE_CONSUMER_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_LEFT_IMAGE_CONSUMER_H_

#import <UIKit/UIKit.h>

// Describes an object that accepts a left image for the omnibox. The left image
// is used for showing the current selected suggestion icon, when the
// suggestions popup is visible.
@protocol OmniboxLeftImageConsumer

// The |imageId| is a resource id.
- (void)setLeftImageId:(int)imageId;

@end

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_LEFT_IMAGE_CONSUMER_H_
