// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTHENTICATION_UNIFIED_CONSENT_IDENTITY_CHOOSER_IDENTITY_CHOOSER_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_AUTHENTICATION_UNIFIED_CONSENT_IDENTITY_CHOOSER_IDENTITY_CHOOSER_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/table_view/chrome_table_view_controller.h"

@protocol IdentityChooserViewControllerPresentationDelegate;
@protocol IdentityChooserViewControllerSelectionDelegate;

// View controller to display the list of identities, to let the user choose an
// identity. IdentityChooserViewController also displays "Add Account…" cell
// at the end.
@interface IdentityChooserViewController : ChromeTableViewController

// Presentation delegate.
@property(nonatomic, weak) id<IdentityChooserViewControllerPresentationDelegate>
    presentationDelegate;
// Selection delegate.
@property(nonatomic, weak) id<IdentityChooserViewControllerSelectionDelegate>
    selectionDelegate;

// Initialises IdentityChooserViewController.
- (instancetype)init NS_DESIGNATED_INITIALIZER;

// -[IdentityChooserViewController init] should be used.
- (instancetype)initWithTableViewStyle:(UITableViewStyle)style
                           appBarStyle:
                               (ChromeTableViewControllerStyle)appBarStyle
    NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_AUTHENTICATION_UNIFIED_CONSENT_IDENTITY_CHOOSER_IDENTITY_CHOOSER_VIEW_CONTROLLER_H_
