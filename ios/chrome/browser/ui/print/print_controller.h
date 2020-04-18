// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PRINT_PRINT_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_PRINT_PRINT_CONTROLLER_H_

#import <UIKit/UIKit.h>

#include "base/memory/ref_counted.h"

namespace net {
class URLRequestContextGetter;
}  // namespace net

// Interface for printing.
@interface PrintController : NSObject

- (instancetype)initWithContextGetter:
    (scoped_refptr<net::URLRequestContextGetter>)getter
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

// Shows print UI for |view| with |title| on |viewController|.
- (void)printView:(UIView*)view
         withTitle:(NSString*)title
    viewController:(UIViewController*)viewController;

// Dismisses the print dialog with animation if |animated|.
- (void)dismissAnimated:(BOOL)animated;

@end

#endif  // IOS_CHROME_BROWSER_UI_PRINT_PRINT_CONTROLLER_H_
