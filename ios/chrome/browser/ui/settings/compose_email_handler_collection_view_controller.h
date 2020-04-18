// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _IOS_CHROME_BROWSER_UI_SETTINGS_COMPOSE_EMAIL_HANDLER_COLLECTION_VIEW_CONTROLLER_H_
#define _IOS_CHROME_BROWSER_UI_SETTINGS_COMPOSE_EMAIL_HANDLER_COLLECTION_VIEW_CONTROLLER_H_

#import "ios/chrome/browser/ui/settings/settings_root_collection_view_controller.h"

@class MailtoHandlerManager;

// UI for Compose Email settings to specify which installed Mail client app to
// use for handling mailto: URLs. There must be at least 2 Mail client apps
// installed before this UI should be shown because there is no need to make
// a choice when there is only 1 Mail client app.
@interface ComposeEmailHandlerCollectionViewController
    : SettingsRootCollectionViewController

- (instancetype)initWithManager:(MailtoHandlerManager*)manager
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithLayout:(UICollectionViewLayout*)layout
                         style:(CollectionViewControllerStyle)style
    NS_UNAVAILABLE;

@end

#endif  // _IOS_CHROME_BROWSER_UI_SETTINGS_COMPOSE_EMAIL_HANDLER_COLLECTION_VIEW_CONTROLLER_H_
