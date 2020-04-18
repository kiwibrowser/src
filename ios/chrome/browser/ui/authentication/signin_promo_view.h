// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTHENTICATION_SIGN_PROMO_VIEW_H_
#define IOS_CHROME_BROWSER_UI_AUTHENTICATION_SIGN_PROMO_VIEW_H_

#import <UIKit/UIKit.h>

#include "components/signin/core/browser/signin_metrics.h"

@class MDCFlatButton;
@protocol SigninPromoViewDelegate;

typedef NS_ENUM(NSInteger, SigninPromoViewMode) {
  // No identity available on the device.
  SigninPromoViewModeColdState,
  // At least one identity is available on the device and the user can sign
  // without entering their credentials.
  SigninPromoViewModeWarmState,
};

extern NSString* const kSigninPromoViewId;
extern NSString* const kSigninPromoPrimaryButtonId;
extern NSString* const kSigninPromoSecondaryButtonId;
extern NSString* const kSigninPromoCloseButtonId;

// This class creates an image view, a label and 2 buttons. This view can be
// configured with 2 modes : "Cold State" and "Warm State".
// + "Cold State" mode displays the chomium icon in the image view, and only
//   displays the primary button.
// + "Warm State" mode displays the image view (big than the cold state mode),
//   displays both buttons.
//
//  For the warm state, the owner should set:
//   - the image for |imageView|, using -[SigninPromoView setProfileImage:]
//   - the label for |textLabel|
//   - the title for |primaryButton|
//   - the title for |secondaryButton|
@interface SigninPromoView : UIView

@property(nonatomic, weak) id<SigninPromoViewDelegate> delegate;
@property(nonatomic) SigninPromoViewMode mode;
@property(nonatomic, readonly) UIImageView* imageView;
@property(nonatomic, readonly) UILabel* textLabel;
@property(nonatomic, readonly) MDCFlatButton* primaryButton;
@property(nonatomic, readonly) MDCFlatButton* secondaryButton;
// Hidden by default.
@property(nonatomic, readonly) UIButton* closeButton;

// Horizontal padding used for |textLabel|, |primaryButton| and
// |secondaryButton|. Used to compute the preferred max layout width of
// |textLabel|.
@property(nonatomic, readonly) CGFloat horizontalPadding;

- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

// Sets the image in |imageView|. This method will add a circular background
// using CircularImageFromImage() (so if the image is not squared, it will be
// cropped first). Must only be called in the "Warm State" mode.
- (void)setProfileImage:(UIImage*)image;

// Resets the view to be reused.
- (void)prepareForReuse;

@end

#endif  // IOS_CHROME_BROWSER_UI_AUTHENTICATION_SIGN_PROMO_VIEW_H_
