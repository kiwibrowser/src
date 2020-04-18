// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_SYNC_SETTINGS_COLLECTION_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_SYNC_SETTINGS_COLLECTION_VIEW_CONTROLLER_H_

#import "ios/chrome/browser/ui/settings/settings_root_collection_view_controller.h"

namespace ios {
class ChromeBrowserState;
}  // namespace ios

// The a11y identifier of the view controller's view.
extern NSString* const kSettingsSyncId;
// Notification when a switch account operation will start.
extern NSString* const kSwitchAccountWillStartNotification;
// Notification when a switch account operation did finish.
extern NSString* const kSwitchAccountDidFinishNotification;

@interface SyncSettingsCollectionViewController
    : SettingsRootCollectionViewController

// |browserState| must not be nil.
// |allowSwitchSyncAccount| indicates whether switching sync account is allowed
// on the screen.
- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
              allowSwitchSyncAccount:(BOOL)allowSwitchSyncAccount
    NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithLayout:(UICollectionViewLayout*)layout
                         style:(CollectionViewControllerStyle)style
    NS_UNAVAILABLE;

@end

@interface SyncSettingsCollectionViewController (UsedForTesting)
// Returns YES if a sync error cell should be displayed.
- (BOOL)shouldDisplaySyncError;
// Return YES if the Sync settings should be disabled because of a Sync error.
- (BOOL)shouldDisableSettingsOnSyncError;
// Returns YES if the encryption cell should display an error.
- (BOOL)shouldDisplayEncryptionError;
@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_SYNC_SETTINGS_COLLECTION_VIEW_CONTROLLER_H_
