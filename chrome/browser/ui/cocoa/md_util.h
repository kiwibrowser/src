// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_MD_UTIL_H_
#define CHROME_BROWSER_UI_COCOA_MD_UTIL_H_

#import <QuartzCore/QuartzCore.h>

@interface CAMediaTimingFunction (ChromeBrowserMDUtil)
@property(class, readonly)
    CAMediaTimingFunction* cr_materialEaseInTimingFunction;
@property(class, readonly)
    CAMediaTimingFunction* cr_materialEaseOutTimingFunction;
@property(class, readonly)
    CAMediaTimingFunction* cr_materialEaseInOutTimingFunction;
@end

#endif  // CHROME_BROWSER_UI_COCOA_MD_UTIL_H_
