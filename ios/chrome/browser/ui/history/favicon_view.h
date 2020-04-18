// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_FAVICON_VIEW_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_FAVICON_VIEW_H_

#import <UIKit/UIKit.h>

// View for displaying the favicon of a history entry.
@interface FaviconView : UIView

// Size for the favicon.
@property(nonatomic) CGFloat size;
// Image view for the favicon.
@property(nonatomic, strong) UIImageView* faviconImage;
// Label for fallback favicon placeholder.
@property(nonatomic, strong) UILabel* faviconFallbackLabel;

@end

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_FAVICON_VIEW_H_
