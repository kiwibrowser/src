// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_AUTOFILL_FORM_INPUT_ACCESSORY_VIEW_H_
#define IOS_CHROME_BROWSER_AUTOFILL_FORM_INPUT_ACCESSORY_VIEW_H_

#import <UIKit/UIKit.h>

@protocol FormInputAccessoryViewDelegate;

// Subview of the accessory view for web forms. Shows a custom view with form
// navigation controls above the keyboard. Subclassed to enable input clicks by
// way of the playInputClick method.
@interface FormInputAccessoryView : UIView<UIInputViewAudioFeedback>

// Sets up the view with the given |customView|. Navigation controls are shown
// and use |delegate| for actions.
- (void)setUpWithNavigationDelegate:(id<FormInputAccessoryViewDelegate>)delegate
                         customView:(UIView*)customView;

// Sets up the view with the given |customView|. Navigation controls are not
// shown.
- (void)setUpWithCustomView:(UIView*)customView;

@end

#endif  // IOS_CHROME_BROWSER_AUTOFILL_FORM_INPUT_ACCESSORY_VIEW_H_
