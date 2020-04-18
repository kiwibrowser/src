// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_ACCESSIBILITY_UTIL_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_ACCESSIBILITY_UTIL_H_

#import <Foundation/Foundation.h>

// Utility class to build accessibility labels. Will join all non-nil items with
// a ", " when building the resulting string.
@interface AccessibilityLabelBuilder : NSObject

// Appends the specified |item| only if it is non-nil.
- (void)appendItem:(NSString*)item;

// Builds a string with all the components concatenated with a ", ".
- (NSString*)buildAccessibilityLabel;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_ACCESSIBILITY_UTIL_H_
