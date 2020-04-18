// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_SAVE_PASSWORDS_COLLECTION_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_SAVE_PASSWORDS_COLLECTION_VIEW_CONTROLLER_H_

#import "ios/chrome/browser/ui/settings/password_details_collection_view_controller_delegate.h"
#import "ios/chrome/browser/ui/settings/settings_root_collection_view_controller.h"

#include <memory>
#include <vector>

namespace autofill {
struct PasswordForm;
}  // namespace autofill

namespace ios {
class ChromeBrowserState;
}  // namespace ios

@protocol ReauthenticationProtocol;
@class PasswordExporter;

@interface SavePasswordsCollectionViewController
    : SettingsRootCollectionViewController

// The designated initializer. |browserState| must not be nil.
- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithLayout:(UICollectionViewLayout*)layout
                         style:(CollectionViewControllerStyle)style
    NS_UNAVAILABLE;

@end

@interface SavePasswordsCollectionViewController (
    Testing)<PasswordDetailsCollectionViewControllerDelegate>

// Callback called when the async request launched from
// |getLoginsFromPasswordStore| finishes.
- (void)onGetPasswordStoreResults:
    (const std::vector<std::unique_ptr<autofill::PasswordForm>>&)result;

// Initializes the password exporter with a (fake) |reauthenticationModule|.
- (void)setReauthenticationModuleForExporter:
    (id<ReauthenticationProtocol>)reauthenticationModule;

// Returns the password exporter to allow setting fake testing objects on it.
- (PasswordExporter*)getPasswordExporter;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_SAVE_PASSWORDS_COLLECTION_VIEW_CONTROLLER_H_
