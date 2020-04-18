// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/omnibox/omnibox_consumer.h"
#import "ios/chrome/browser/ui/omnibox/omnibox_text_field_ios.h"

// The view controller managing the omnibox textfield and its container view.
@interface OmniboxViewController : UIViewController<OmniboxConsumer>

// The textfield used by this view controller.
@property(nonatomic, readonly, strong) OmniboxTextFieldIOS* textField;

// Designated initializer.
- (instancetype)initWithFont:(UIFont*)font
                   textColor:(UIColor*)textColor
                   tintColor:(UIColor*)tintColor
                   incognito:(BOOL)isIncognito;

@end

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_VIEW_CONTROLLER_H_
