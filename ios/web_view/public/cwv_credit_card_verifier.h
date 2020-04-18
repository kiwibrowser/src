// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_CREDIT_CARD_VERIFIER_H_
#define IOS_WEB_VIEW_PUBLIC_CWV_CREDIT_CARD_VERIFIER_H_

#import <UIKit/UIKit.h>

#import "cwv_export.h"

NS_ASSUME_NONNULL_BEGIN

@class CWVCreditCard;

CWV_EXPORT
// Helps with verifying credit cards for autofill, updating expired expiration
// dates, and saving the card locally.
@interface CWVCreditCardVerifier : NSObject

// The credit card that is pending verification.
@property(nonatomic, readonly) CWVCreditCard* creditCard;

// Whether or not this card can be saved locally.
@property(nonatomic, readonly) BOOL canStoreLocally;

// The last |storeLocally| value that was used when verifying. Can be used to
// set initial state for UI.
@property(nonatomic, readonly) BOOL lastStoreLocallyValue;

// Returns a recommended title to display in the navigation bar to the user.
@property(nonatomic, readonly) NSString* navigationTitle;

// Returns the instruction message to show the user for verifying |creditCard|.
// Depends on |needsUpdateForExpirationDate| and |canSaveLocally|.
@property(nonatomic, readonly) NSString* instructionMessage;

// Returns a recommended button label for a confirm/OK button.
@property(nonatomic, readonly) NSString* confirmButtonLabel;

// Returns an image that indicates where on the card you may find the CVC.
@property(nonatomic, readonly) UIImage* CVCHintImage;

// The expected length of the CVC depending on |creditCard|'s network.
// e.g. 3 for Visa and 4 for American Express.
@property(nonatomic, readonly) NSInteger expectedCVCLength;

// YES if |creditCard|'s current expiration date has expired and needs updating.
@property(nonatomic, readonly) BOOL needsUpdateForExpirationDate;

- (instancetype)init NS_UNAVAILABLE;

// Attempts |creditCard| verification.
// |CVC| Card verification code. e.g. 3 digit code on the back of Visa cards or
// 4 digit code in the front of American Express cards.
// |month| 2 digit expiration month. e.g. 08 for August.
// |year| 4 digit expiration year. e.g. 2019.
// |storeLocally| Whether or not to save |creditCard| locally. If YES, user will
// not be asked again to verify this card. Ignored if |canSaveLocally| is NO.
// |completionHandler| Use to receive verification results. Must wait for
// handler to return before attempting another verification.
// |error| Contains the error message if unsuccessful. Empty if successful.
// |retryAllowed| YES if user may attempt verification again.
- (void)verifyWithCVC:(NSString*)CVC
      expirationMonth:(NSString*)expirationMonth
       expirationYear:(NSString*)expirationYear
         storeLocally:(BOOL)storeLocally
    completionHandler:
        (void (^)(NSString* errorMessage, BOOL retryAllowed))completionHandler;

// Returns YES if |CVC| is all digits and matches |expectedCVCLength|.
- (BOOL)isCVCValid:(NSString*)CVC;

// Returns YES if |month| and |year| is in the future.
// |month| Two digits. e.g. 08 for August.
// |year| Four digits. e.g. 2020.
- (BOOL)isExpirationDateValidForMonth:(NSString*)month year:(NSString*)year;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_CREDIT_CARD_VERIFIER_H_
