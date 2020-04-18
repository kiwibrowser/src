// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_CONTROLLER_DELEGATE_H_
#define IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_CONTROLLER_DELEGATE_H_

#import <Foundation/Foundation.h>

#import "cwv_export.h"

NS_ASSUME_NONNULL_BEGIN

@class CWVAutofillController;
@class CWVAutofillFormSuggestion;
@class CWVCreditCard;
@class CWVCreditCardVerifier;

// Storage policies for autofill data.
typedef NS_ENUM(NSInteger, CWVStoragePolicy) {
  CWVStoragePolicyReject = 0,  // Do not store.
  CWVStoragePolicyAllow,       // Allow storage.
};

CWV_EXPORT
// Protocol to receive callbacks related to autofill.
// |fieldName| is the 'name' attribute of a html field.
// |formName| is the 'name' attribute of a html <form>.
// |value| is the 'value' attribute of the html field.
// Example:
// <form name='_formName_'>
//   <input name='_fieldName_' value='_value_'>
// </form>
@protocol CWVAutofillControllerDelegate<NSObject>

@optional

// Called when a form field element receives a "focus" event.
- (void)autofillController:(CWVAutofillController*)autofillController
    didFocusOnFieldWithName:(NSString*)fieldName
            fieldIdentifier:(NSString*)fieldIdentifier
                   formName:(NSString*)formName
                      value:(NSString*)value;

// Called when a form field element receives an "input" event.
- (void)autofillController:(CWVAutofillController*)autofillController
    didInputInFieldWithName:(NSString*)fieldName
            fieldIdentifier:(NSString*)fieldIdentifier
                   formName:(NSString*)formName
                      value:(NSString*)value;

// Called when a form field element receives a "blur" (un-focused) event.
- (void)autofillController:(CWVAutofillController*)autofillController
    didBlurOnFieldWithName:(NSString*)fieldName
           fieldIdentifier:(NSString*)fieldIdentifier
                  formName:(NSString*)formName
                     value:(NSString*)value;

// Called when a form was submitted. |userInitiated| is YES if form is submitted
// as a result of user interaction.
- (void)autofillController:(CWVAutofillController*)autofillController
     didSubmitFormWithName:(NSString*)formName
             userInitiated:(BOOL)userInitiated
               isMainFrame:(BOOL)isMainFrame;

// Called when user needs to decide on whether or not to save the card locally.
// This can happen if user is signed out or sync is disabled.
// Pass final decision to |decisionHandler|. Must only be called once.
// If not implemented, assumes CWVStoragePolicyReject.
- (void)autofillController:(CWVAutofillController*)autofillController
    decidePolicyForLocalStorageOfCreditCard:(CWVCreditCard*)creditCard
                            decisionHandler:(void (^)(CWVStoragePolicy policy))
                                                decisionHandler;

// Called when the user needs to use |verifier| to verify a credit card.
// Lifetime of |verifier| should be managed by the delegate.
- (void)autofillController:(CWVAutofillController*)autofillController
    verifyCreditCardWithVerifier:(CWVCreditCardVerifier*)verifier;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_CONTROLLER_DELEGATE_H_
