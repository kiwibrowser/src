// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_UTIL_RELAXED_BOUNDS_CONSTRAINTS_HITTEST_H_
#define IOS_CHROME_BROWSER_UI_UTIL_RELAXED_BOUNDS_CONSTRAINTS_HITTEST_H_

#import <Foundation/Foundation.h>

// This protocol is meant to be implemented by subclasses of UIView. It exposes
// a property |hitTestBoundsContraintRelaxed| that when set to YES, inform that
// bounds constraints on the hitTest: method shouldn't be taken into account.
// The implementer of that protocol must override the hitTest: method and check
// for that property.
@protocol RelaxedBoundsConstraintsHitTestSupport<NSObject>

@optional
@property(nonatomic, assign) BOOL hitTestBoundsContraintRelaxed;

@end

#endif  // IOS_CHROME_BROWSER_UI_UTIL_RELAXED_BOUNDS_CONSTRAINTS_HITTEST_H_
