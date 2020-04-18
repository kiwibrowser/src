// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_UIVIEW_SIZECLASSSUPPORT_H_
#define IOS_CHROME_BROWSER_UI_UIVIEW_SIZECLASSSUPPORT_H_

#import <UIKit/UIKit.h>

// An enum type to describe size classes.
typedef NS_ENUM(NSInteger, SizeClassIdiom) {
  COMPACT = 0,
  REGULAR,
  UNSPECIFIED,
  SIZE_CLASS_COUNT = UNSPECIFIED,
};

// UIView category that exposes SizeClassIdiom getters.
@interface UIView (SizeClassSupport)

// Convenience getters for the view's width and height SizeClassIdioms.  If the
// view's size class is unspecified, these functions return the size class idiom
// of the application's key window.
@property(nonatomic, readonly) SizeClassIdiom cr_widthSizeClass;
@property(nonatomic, readonly) SizeClassIdiom cr_heightSizeClass;

@end

#endif  // IOS_CHROME_BROWSER_UI_UIVIEW_SIZECLASSSUPPORT_H_
