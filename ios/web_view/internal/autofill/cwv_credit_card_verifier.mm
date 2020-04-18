// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/autofill/cwv_credit_card_verifier_internal.h"

#include <memory>

#include "base/strings/sys_string_conversions.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/ui/card_unmask_prompt_controller_impl.h"
#include "components/autofill/core/browser/ui/card_unmask_prompt_view.h"
#import "ios/web_view/internal/autofill/cwv_credit_card_internal.h"
#include "ui/base/resource/resource_bundle.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface CWVCreditCardVerifier ()

// Used to receive |GotVerificationResult| from WebViewCardUnmaskPromptView.
- (void)didReceiveVerificationResultWithErrorMessage:(NSString*)errorMessage
                                        retryAllowed:(BOOL)retryAllowed;

@end

namespace ios_web_view {
// Webview implementation of CardUnmaskPromptView.
class WebViewCardUnmaskPromptView : public autofill::CardUnmaskPromptView {
 public:
  explicit WebViewCardUnmaskPromptView(CWVCreditCardVerifier* verifier)
      : verifier_(verifier) {}

  // CardUnmaskPromptView:
  void Show() override {
    // No op.
  }
  void ControllerGone() override {
    // No op.
  }
  void DisableAndWaitForVerification() override {
    // No op.
  }
  void GotVerificationResult(const base::string16& error_message,
                             bool allow_retry) override {
    NSString* ns_error_message = base::SysUTF16ToNSString(error_message);
    [verifier_ didReceiveVerificationResultWithErrorMessage:ns_error_message
                                               retryAllowed:allow_retry];
  }

 private:
  __weak CWVCreditCardVerifier* verifier_;
};
}  // namespace ios_web_view

@implementation CWVCreditCardVerifier {
  // The main class that is wrapped by this class.
  std::unique_ptr<autofill::CardUnmaskPromptControllerImpl>
      _unmaskingController;
  // Used to interface with |_unmaskingController|.
  std::unique_ptr<ios_web_view::WebViewCardUnmaskPromptView> _unmaskingView;
  // Completion handler for verification results. This is provided by the
  // client and copied internally to be invoked later from
  // |didReceiveVerificationResultWithErrorMessage:retryAllowed:|.
  void (^_completionHandler)(NSString* errorMessage, BOOL retryAllowed);
}

@synthesize creditCard = _creditCard;

- (instancetype)initWithPrefs:(PrefService*)prefs
               isOffTheRecord:(BOOL)isOffTheRecord
                   creditCard:(const autofill::CreditCard&)creditCard
                       reason:(autofill::AutofillClient::UnmaskCardReason)reason
                     delegate:
                         (base::WeakPtr<autofill::CardUnmaskDelegate>)delegate {
  self = [super init];
  if (self) {
    _creditCard = [[CWVCreditCard alloc] initWithCreditCard:creditCard];
    _unmaskingView =
        std::make_unique<ios_web_view::WebViewCardUnmaskPromptView>(self);
    _unmaskingController =
        std::make_unique<autofill::CardUnmaskPromptControllerImpl>(
            prefs, isOffTheRecord);
    _unmaskingController->ShowPrompt(_unmaskingView.get(), creditCard, reason,
                                     delegate);
  }
  return self;
}

- (void)dealloc {
  _unmaskingController->OnUnmaskDialogClosed();
}

#pragma mark - Public Methods

- (BOOL)canStoreLocally {
  return _unmaskingController->CanStoreLocally();
}

- (BOOL)lastStoreLocallyValue {
  return _unmaskingController->GetStoreLocallyStartState();
}

- (NSString*)navigationTitle {
  return base::SysUTF16ToNSString(_unmaskingController->GetWindowTitle());
}

- (NSString*)instructionMessage {
  return base::SysUTF16ToNSString(
      _unmaskingController->GetInstructionsMessage());
}

- (NSString*)confirmButtonLabel {
  return base::SysUTF16ToNSString(_unmaskingController->GetOkButtonLabel());
}

- (UIImage*)CVCHintImage {
  int resourceID = _unmaskingController->GetCvcImageRid();
  return ui::ResourceBundle::GetSharedInstance()
      .GetNativeImageNamed(resourceID)
      .ToUIImage();
}

- (NSInteger)expectedCVCLength {
  return _unmaskingController->GetExpectedCvcLength();
}

- (BOOL)needsUpdateForExpirationDate {
  return _unmaskingController->ShouldRequestExpirationDate();
}

- (void)verifyWithCVC:(NSString*)CVC
      expirationMonth:(NSString*)expirationMonth
       expirationYear:(NSString*)expirationYear
         storeLocally:(BOOL)storeLocally
    completionHandler:
        (void (^)(NSString* errorMessage, BOOL retryAllowed))completionHandler {
  DCHECK(!_completionHandler);
  _unmaskingController->OnUnmaskResponse(
      base::SysNSStringToUTF16(CVC), base::SysNSStringToUTF16(expirationMonth),
      base::SysNSStringToUTF16(expirationYear), storeLocally);
  _completionHandler = completionHandler;
}

- (BOOL)isCVCValid:(NSString*)CVC {
  return _unmaskingController->InputCvcIsValid(base::SysNSStringToUTF16(CVC));
}

- (BOOL)isExpirationDateValidForMonth:(NSString*)month year:(NSString*)year {
  return _unmaskingController->InputExpirationIsValid(
      base::SysNSStringToUTF16(month), base::SysNSStringToUTF16(year));
}

#pragma mark - Private Methods

- (void)didReceiveVerificationResultWithErrorMessage:(NSString*)errorMessage
                                        retryAllowed:(BOOL)retryAllowed {
  DCHECK(_completionHandler);
  _completionHandler(errorMessage, retryAllowed);
  _completionHandler = nil;
}

#pragma mark - Internal Methods

- (void)didReceiveUnmaskVerificationResult:
    (autofill::AutofillClient::PaymentsRpcResult)result {
  _unmaskingController->OnVerificationResult(result);
}

@end
