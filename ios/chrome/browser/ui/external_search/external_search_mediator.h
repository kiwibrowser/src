// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_EXTERNAL_SEARCH_EXTERNAL_SEARCH_MEDIATOR_H_
#define IOS_CHROME_BROWSER_UI_EXTERNAL_SEARCH_EXTERNAL_SEARCH_MEDIATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/commands/external_search_commands.h"

// Mediator of ExternalSearchCoordinator, it handles the opening of the External
// Search app.
@interface ExternalSearchMediator : NSObject<ExternalSearchCommands>

// The application object used to open the External Search app. Default is
// UIApplication.sharedApplication.
@property(nonatomic, null_resettable) UIApplication* application;

@end

#endif  // IOS_CHROME_BROWSER_UI_EXTERNAL_SEARCH_EXTERNAL_SEARCH_MEDIATOR_H_
