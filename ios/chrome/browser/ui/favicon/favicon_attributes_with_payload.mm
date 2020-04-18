// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/favicon/favicon_attributes_with_payload.h"

#import "ios/chrome/browser/ui/favicon/favicon_attributes+private.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation FaviconAttributesWithPayload

@synthesize iconType = _iconType;

+ (instancetype)attributesWithImage:(UIImage*)image {
  DCHECK(image);
  return [[self alloc] initWithImage:image
                            monogram:nil
                           textColor:nil
                     backgroundColor:nil
              defaultBackgroundColor:NO];
}

+ (instancetype)attributesWithMonogram:(NSString*)monogram
                             textColor:(UIColor*)textColor
                       backgroundColor:(UIColor*)backgroundColor
                defaultBackgroundColor:(BOOL)defaultBackgroundColor {
  return [[self alloc] initWithImage:nil
                            monogram:monogram
                           textColor:textColor
                     backgroundColor:backgroundColor
              defaultBackgroundColor:defaultBackgroundColor];
}

@end
