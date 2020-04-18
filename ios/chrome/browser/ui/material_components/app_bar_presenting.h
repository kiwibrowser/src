// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_MATERIAL_COMPONENTS_APP_BAR_PRESENTING_H_
#define IOS_CHROME_BROWSER_UI_MATERIAL_COMPONENTS_APP_BAR_PRESENTING_H_

#import <UIKit/UIKit.h>

@class MDCAppBar;

// An object conforming to this protocol is capable of creating and managing an
// MDCAppBar. Typically, UIViewController's can implement this protocol to vend
// the app bar they can optionally be presenting.
@protocol AppBarPresenting<NSObject>

// The installed app bar, if any.
@property(nonatomic, readonly, strong) MDCAppBar* appBar;

@end

#endif  // IOS_CHROME_BROWSER_UI_MATERIAL_COMPONENTS_APP_BAR_PRESENTING_H_
