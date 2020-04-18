// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTHENTICATION_UNIFIED_CONSENT_IDENTITY_CHOOSER_IDENTITY_CHOOSER_CELL_H_
#define IOS_CHROME_BROWSER_UI_AUTHENTICATION_UNIFIED_CONSENT_IDENTITY_CHOOSER_IDENTITY_CHOOSER_CELL_H_

#import <UIKit/UIKit.h>

@class IdentityView;

// Cell to display an user identity or the "Add Account…" button.
@interface IdentityChooserCell : UITableViewCell

// Initializes IdentityChooserCell instance.
- (instancetype)initWithStyle:(UITableViewCellStyle)style
              reuseIdentifier:(NSString*)reuseIdentifier
    NS_DESIGNATED_INITIALIZER;

// -[IdentityChooserCell initWithStyle:reuseIdentifier:] should be used.
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

// Configures the cell with the title, subtitle and image. If |checked| is YES,
// the cell displays a checkmark.
- (void)configureCellWithTitle:(NSString*)title
                      subtitle:(NSString*)subtitle
                         image:(UIImage*)image
                       checked:(BOOL)checked;

@end

#endif  // IOS_CHROME_BROWSER_UI_AUTHENTICATION_UNIFIED_CONSENT_IDENTITY_CHOOSER_IDENTITY_CHOOSER_CELL_H_
