// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_DATA_MANAGER_H_
#define IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_DATA_MANAGER_H_

#import <Foundation/Foundation.h>

#import "cwv_export.h"

NS_ASSUME_NONNULL_BEGIN

@class CWVAutofillProfile;
@class CWVCreditCard;
@protocol CWVAutofillDataManagerDelegate;

CWV_EXPORT
// Exposes saved autofill data such as address profiles and credit cards.
@interface CWVAutofillDataManager : NSObject

// Delegate for CWVAutofillDataManagerDelegate.
@property(nonatomic, weak) id<CWVAutofillDataManagerDelegate> delegate;

- (instancetype)init NS_UNAVAILABLE;

// Returns all saved profiles for address autofill in |completionHandler|.
- (void)fetchProfilesWithCompletionHandler:
    (void (^)(NSArray<CWVAutofillProfile*>* profiles))completionHandler;

// Updates the profile.
- (void)updateProfile:(CWVAutofillProfile*)profile;

// Deletes the profile.
- (void)deleteProfile:(CWVAutofillProfile*)profile;

// Returns all saved credit cards for payment autofill in |completionHandler|.
- (void)fetchCreditCardsWithCompletionHandler:
    (void (^)(NSArray<CWVCreditCard*>* creditCards))completionHandler;

// Updates the card.
- (void)updateCreditCard:(CWVCreditCard*)creditCard;

// Deletes the card.
- (void)deleteCreditCard:(CWVCreditCard*)creditCard;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_DATA_MANAGER_H_
