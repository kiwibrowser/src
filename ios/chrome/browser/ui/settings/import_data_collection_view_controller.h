// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_IMPORT_DATA_COLLECTION_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_IMPORT_DATA_COLLECTION_VIEW_CONTROLLER_H_

#import "ios/chrome/browser/signin/constants.h"
#import "ios/chrome/browser/ui/settings/settings_root_collection_view_controller.h"

@class ImportDataCollectionViewController;

// The accessibility identifier of the Import Data cell.
extern NSString* const kImportDataImportCellId;

// The accessibility identifier of the Keep Data Separate cell.
extern NSString* const kImportDataKeepSeparateCellId;

// Notifies of the user action on the corresponding
// ImportDataCollectionViewController.
@protocol ImportDataControllerDelegate

// Indicates that the user chose the clear data policy to be |shouldClearData|
// when presented with |controller|.
- (void)didChooseClearDataPolicy:(ImportDataCollectionViewController*)controller
                 shouldClearData:(ShouldClearData)shouldClearData;

@end

// Collection View that handles how to import data during account switching.
@interface ImportDataCollectionViewController
    : SettingsRootCollectionViewController

// |fromEmail| is the email of the previously signed in account.
// |toIdentity| is the email of the account switched to.
// |isSignedIn| is whether the user is currently signed in.
//
// |fromEmail| and |toEmail| must not be NULL.
- (instancetype)initWithDelegate:(id<ImportDataControllerDelegate>)delegate
                       fromEmail:(NSString*)fromEmail
                         toEmail:(NSString*)toEmail
                      isSignedIn:(BOOL)isSignedIn NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithLayout:(UICollectionViewLayout*)layout
                         style:(CollectionViewControllerStyle)style
    NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_IMPORT_DATA_COLLECTION_VIEW_CONTROLLER_H_
