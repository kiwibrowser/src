// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_FAVICON_FAVICON_ATTRIBUTES_PRIVATE_H_
#define IOS_CHROME_BROWSER_UI_FAVICON_FAVICON_ATTRIBUTES_PRIVATE_H_

@interface FaviconAttributes (initializers)

// Designated initializer. Either |image| or all of |textColor|,
// |backgroundColor| and |monogram| must be not nil.
- (instancetype)initWithImage:(UIImage*)image
                     monogram:(NSString*)monogram
                    textColor:(UIColor*)textColor
              backgroundColor:(UIColor*)backgroundColor
       defaultBackgroundColor:(BOOL)defaultBackgroundColor;

@end

#endif  // IOS_CHROME_BROWSER_UI_FAVICON_FAVICON_ATTRIBUTES_PRIVATE_H_
