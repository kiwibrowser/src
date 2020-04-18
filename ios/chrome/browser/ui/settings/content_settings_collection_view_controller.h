// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_CONTENT_SETTINGS_COLLECTION_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_CONTENT_SETTINGS_COLLECTION_VIEW_CONTROLLER_H_

#import "ios/chrome/browser/ui/settings/settings_root_collection_view_controller.h"
#import "ios/chrome/browser/web/mailto_handler_manager.h"

namespace ios {
class ChromeBrowserState;
}  // namespace ios

// Controller for the UI that allows the user to change content settings like
// blocking popups.
@interface ContentSettingsCollectionViewController
    : SettingsRootCollectionViewController<MailtoHandlerManagerObserver>

// The designated initializer. |browserState| must not be nil.
- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
    NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithLayout:(UICollectionViewLayout*)layout
                         style:(CollectionViewControllerStyle)style
    NS_UNAVAILABLE;
@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_CONTENT_SETTINGS_COLLECTION_VIEW_CONTROLLER_H_
