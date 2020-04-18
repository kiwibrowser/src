// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SAD_TAB_SAD_TAB_LEGACY_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_SAD_TAB_SAD_TAB_LEGACY_COORDINATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/web/sad_tab_tab_helper_delegate.h"

@protocol ApplicationCommands;

// Coordinator that displays a SadTab view.
@interface SadTabLegacyCoordinator : NSObject<SadTabTabHelperDelegate>

// The view controller from which to present VCs.
@property(nonatomic, weak) UIViewController* baseViewController;

// The dispatcher for this Coordinator.
@property(nonatomic, weak) id<ApplicationCommands> dispatcher;

@end

#endif  // IOS_CHROME_BROWSER_UI_SAD_TAB_SAD_TAB_LEGACY_COORDINATOR_H_
