// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_SHOWCASE_MANUAL_FILL_PASSWORD_PICKER_VIEW_CONTROLLER_H_
#define IOS_SHOWCASE_MANUAL_FILL_PASSWORD_PICKER_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

@protocol ManualFillContentDelegate;

// This class mock a list of usernames and passwords in a collection view.
@interface PasswordPickerViewController
    : UICollectionViewController<UICollectionViewDelegateFlowLayout>

// Unavailable. Use `initWithDelegate:`.
- (instancetype)init NS_UNAVAILABLE;

// Unavailable. Use `initWithDelegate:`.
- (instancetype)initWithCollectionViewLayout:(UICollectionViewLayout*)layout
    NS_UNAVAILABLE;

// Unavailable. Use `initWithDelegate:`.
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

// Unavailable. Use `initWithDelegate:`.
- (instancetype)initWithNibName:(NSString*)nibNameOrNil
                         bundle:(NSBundle*)nibBundleOrNil NS_UNAVAILABLE;

// Instances an object with the desired delegate.
//
// @param delegate The object interested in the user selection from the
//                 presented options.
// @return A fresh object with the passed delegate.
- (instancetype)initWithDelegate:(id<ManualFillContentDelegate>)delegate
    NS_DESIGNATED_INITIALIZER;

@end

#endif  // IOS_SHOWCASE_MANUAL_FILL_PASSWORD_PICKER_VIEW_CONTROLLER_H_
