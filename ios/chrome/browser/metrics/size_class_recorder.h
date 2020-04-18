// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_METRICS_SIZE_CLASS_RECORDER_H_
#define IOS_CHROME_BROWSER_METRICS_SIZE_CLASS_RECORDER_H_

#import <UIKit/UIKit.h>

// Reports metrics for the size classes uses on iPad iOS 9+.
@interface SizeClassRecorder : NSObject

- (instancetype)initWithHorizontalSizeClass:(UIUserInterfaceSizeClass)sizeClass
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

// Call this method upon horizontal size class changes to report the used size
// class to UMA. It will be reported under Tab.HorizontalSizeClassUsed.
- (void)horizontalSizeClassDidChange:(UIUserInterfaceSizeClass)newSizeClass;

// Call this class method when a page has loaded to record the size class at
// that time. It will be reported inder Tab.PageLoadInHorizontalSizeClass.
+ (void)pageLoadedWithHorizontalSizeClass:(UIUserInterfaceSizeClass)sizeClass;

@end

#endif  // IOS_CHROME_BROWSER_METRICS_SIZE_CLASS_RECORDER_H_
