// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_SHELL_TEST_EARL_GREY_SHELL_EARL_GREY_H_
#define IOS_WEB_SHELL_TEST_EARL_GREY_SHELL_EARL_GREY_H_

#import <Foundation/Foundation.h>

#include "url/gurl.h"

// Test methods that perform actions on Web Shell. These methods may read or
// alter Web Shell's internal state programmatically or via the UI, but in both
// cases will properly synchronize the UI for Earl Grey tests.
@interface ShellEarlGrey : NSObject

// Loads |URL| in the current WebState with transition of type
// ui::PAGE_TRANSITION_TYPED, and waits for the page to complete loading, or
// a timeout.
+ (void)loadURL:(const GURL&)URL;

// Waits for the current web view to contain |text|. If the condition is not met
// within a timeout, a GREYAssert is induced.
+ (void)waitForWebViewContainingText:(const std::string)text;

// Waits for the current web view to contain a css selector matching |selector|.
// If the condition is not met within a timeout, a GREYAssert is induced.
+ (void)waitForWebViewContainingCSSSelector:(std::string)selector;

// Waits for the current web view to not contain a css selector matching
// |selector|. If the condition is not met within a timeout, a GREYAssert is
// induced.
+ (void)waitForWebViewNotContainingCSSSelector:(std::string)selector;

@end

#endif  // IOS_WEB_SHELL_TEST_EARL_GREY_SHELL_EARL_GREY_H_
