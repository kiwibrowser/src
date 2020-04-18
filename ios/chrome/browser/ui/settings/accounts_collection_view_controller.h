// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_ACCOUNTS_COLLECTION_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_ACCOUNTS_COLLECTION_VIEW_CONTROLLER_H_

#import "ios/chrome/browser/sync/sync_observer_bridge.h"
#import "ios/chrome/browser/ui/settings/settings_navigation_controller.h"
#import "ios/chrome/browser/ui/settings/settings_root_collection_view_controller.h"

// The accessibility identifier of the view controller's view.
extern NSString* const kSettingsAccountsId;
// The accessibility identifier of the header of the accounts list.
extern NSString* const kSettingsHeaderId;
// The accessibility identifier of the add account cell.
extern NSString* const kSettingsAccountsAddAccountCellId;
// The accessibility identifier of the signout cell.
extern NSString* const kSettingsAccountsSignoutCellId;
// The accessibility identifier of the sync account cell.
extern NSString* const kSettingsAccountsSyncCellId;

@protocol ApplicationCommands;
@protocol ApplicationSettingsCommands;
namespace ios {
class ChromeBrowserState;
}  // namespace ios

// Collection View that handles the settings for accounts when the user is
// signed in
// to Chrome.
@interface AccountsCollectionViewController
    : SettingsRootCollectionViewController<SettingsControllerProtocol>

// |browserState| must not be nil.
// If |closeSettingsOnAddAccount| is YES, then this account table view
// controller will close the setting screen when an account is added.
- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
           closeSettingsOnAddAccount:(BOOL)closeSettingsOnAddAccount
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithLayout:(UICollectionViewLayout*)layout
                         style:(CollectionViewControllerStyle)style
    NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_ACCOUNTS_COLLECTION_VIEW_CONTROLLER_H_
