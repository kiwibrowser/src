// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_SYNC_CREATE_PASSPHRASE_COLLECTION_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_SYNC_CREATE_PASSPHRASE_COLLECTION_VIEW_CONTROLLER_H_

#import "ios/chrome/browser/ui/settings/sync_encryption_passphrase_collection_view_controller.h"

@class UITextField;

// Controller to allow user to specify encryption passphrase for Sync.
@interface SyncCreatePassphraseCollectionViewController
    : SyncEncryptionPassphraseCollectionViewController
@end

@interface SyncCreatePassphraseCollectionViewController (UsedForTesting)
@property(nonatomic, readonly) UITextField* confirmPassphrase;
@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_SYNC_CREATE_PASSPHRASE_COLLECTION_VIEW_CONTROLLER_H_
