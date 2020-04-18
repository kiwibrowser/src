// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/image_util/image_util.h"

#include "ui/gfx/color_analysis.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

UIColor* DominantColorForImage(const gfx::Image& image, CGFloat opacity) {
  SkColor color = color_utils::CalculateKMeanColorOfBitmap(*image.ToSkBitmap());
  UIColor* result = [UIColor colorWithRed:SkColorGetR(color) / 255.0
                                    green:SkColorGetG(color) / 255.0
                                     blue:SkColorGetB(color) / 255.0
                                    alpha:opacity];
  return result;
}

UIImage* StretchableImageFromUIImage(UIImage* image,
                                     NSInteger left_cap_width,
                                     NSInteger top_cap_height) {
  UIEdgeInsets insets = UIEdgeInsetsMake(
      top_cap_height, left_cap_width, image.size.height - top_cap_height + 1.0,
      image.size.width - left_cap_width + 1.0);
  return [image resizableImageWithCapInsets:insets];
}

UIImage* StretchableImageNamed(NSString* name) {
  UIImage* image = [UIImage imageNamed:name];
  if (!image)
    return nil;
  // Returns a copy of |image| configured to stretch at the center pixel.
  return StretchableImageFromUIImage(image, floor(image.size.width / 2.0),
                                     floor(image.size.height / 2.0));
}

UIImage* StretchableImageNamed(NSString* name,
                               NSInteger left_cap_width,
                               NSInteger top_cap_height) {
  UIImage* image = [UIImage imageNamed:name];
  if (!image)
    return nil;

  return StretchableImageFromUIImage(image, left_cap_width, top_cap_height);
}
