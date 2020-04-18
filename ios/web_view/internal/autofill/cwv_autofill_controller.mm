// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/autofill/cwv_autofill_controller_internal.h"

#include <memory>

#include "base/callback.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "components/autofill/core/browser/autofill_manager.h"
#include "components/autofill/core/browser/popup_item_ids.h"
#import "components/autofill/ios/browser/autofill_agent.h"
#include "components/autofill/ios/browser/autofill_driver_ios.h"
#include "components/autofill/ios/browser/autofill_driver_ios_bridge.h"
#import "components/autofill/ios/browser/js_autofill_manager.h"
#import "components/autofill/ios/browser/js_suggestion_manager.h"
#include "components/keyed_service/core/service_access_type.h"
#include "ios/web/public/web_state/form_activity_params.h"
#import "ios/web/public/web_state/js/crw_js_injection_receiver.h"
#import "ios/web/public/web_state/web_state_observer_bridge.h"
#include "ios/web_view/internal/app/application_context.h"
#import "ios/web_view/internal/autofill/cwv_autofill_client_ios_bridge.h"
#import "ios/web_view/internal/autofill/cwv_autofill_suggestion_internal.h"
#import "ios/web_view/internal/autofill/cwv_credit_card_internal.h"
#import "ios/web_view/internal/autofill/cwv_credit_card_verifier_internal.h"
#import "ios/web_view/internal/autofill/web_view_autofill_client_ios.h"
#include "ios/web_view/internal/autofill/web_view_personal_data_manager_factory.h"
#include "ios/web_view/internal/signin/web_view_identity_manager_factory.h"
#include "ios/web_view/internal/web_view_browser_state.h"
#include "ios/web_view/internal/webdata_services/web_view_web_data_service_wrapper_factory.h"
#import "ios/web_view/public/cwv_autofill_controller_delegate.h"

@interface CWVAutofillController ()<AutofillDriverIOSBridge,
                                    CRWWebStateObserver,
                                    CWVAutofillClientIOSBridge>

@end

@implementation CWVAutofillController {
  // Bridge to observe the |webState|.
  std::unique_ptr<web::WebStateObserverBridge> _webStateObserverBridge;

  // Autofill agent associated with |webState|.
  AutofillAgent* _autofillAgent;

  // Autofill manager associated with |webState|.
  autofill::AutofillManager* _autofillManager;

  // Autofill client associated with |webState|.
  std::unique_ptr<autofill::WebViewAutofillClientIOS> _autofillClient;

  // Javascript autofill manager associated with |webState|.
  JsAutofillManager* _JSAutofillManager;

  // Javascript suggestion manager associated with |webState|.
  JsSuggestionManager* _JSSuggestionManager;

  // The |webState| which this autofill controller should observe.
  web::WebState* _webState;

  // The current credit card verifier. Can be nil if no verification is pending.
  // Held weak because |_delegate| is responsible for maintaing its lifetime.
  __weak CWVCreditCardVerifier* _verifier;
}

@synthesize delegate = _delegate;

- (instancetype)initWithWebState:(web::WebState*)webState
                   autofillAgent:(AutofillAgent*)autofillAgent
               JSAutofillManager:(JsAutofillManager*)JSAutofillManager
             JSSuggestionManager:(JsSuggestionManager*)JSSuggestionManager {
  self = [super init];
  if (self) {
    DCHECK(webState);
    _webState = webState;

    ios_web_view::WebViewBrowserState* browserState =
        ios_web_view::WebViewBrowserState::FromBrowserState(
            _webState->GetBrowserState());
    _autofillAgent = autofillAgent;

    _webStateObserverBridge =
        std::make_unique<web::WebStateObserverBridge>(self);
    _webState->AddObserver(_webStateObserverBridge.get());

    _autofillClient.reset(new autofill::WebViewAutofillClientIOS(
        browserState->GetPrefs(),
        ios_web_view::WebViewPersonalDataManagerFactory::GetForBrowserState(
            browserState->GetRecordingBrowserState()),
        _webState, self,
        ios_web_view::WebViewIdentityManagerFactory::GetForBrowserState(
            browserState->GetRecordingBrowserState()),
        ios_web_view::WebViewWebDataServiceWrapperFactory::
            GetAutofillWebDataForBrowserState(
                browserState, ServiceAccessType::EXPLICIT_ACCESS)));
    autofill::AutofillDriverIOS::CreateForWebStateAndDelegate(
        _webState, _autofillClient.get(), self,
        ios_web_view::ApplicationContext::GetInstance()->GetApplicationLocale(),
        autofill::AutofillManager::ENABLE_AUTOFILL_DOWNLOAD_MANAGER);
    _autofillManager = autofill::AutofillDriverIOS::FromWebState(_webState)
                           ->autofill_manager();

    _JSAutofillManager = JSAutofillManager;

    _JSSuggestionManager = JSSuggestionManager;
  }
  return self;
}

- (void)dealloc {
  if (_webState) {
    _webState->RemoveObserver(_webStateObserverBridge.get());
    _webStateObserverBridge.reset();
    _webState = nullptr;
  }
}

#pragma mark - Public Methods

- (void)clearFormWithName:(NSString*)formName
        completionHandler:(nullable void (^)(void))completionHandler {
  [_JSAutofillManager clearAutofilledFieldsForFormNamed:formName
                                      completionHandler:^{
                                        if (completionHandler) {
                                          completionHandler();
                                        }
                                      }];
}

- (void)fetchSuggestionsForFormWithName:(NSString*)formName
                              fieldName:(NSString*)fieldName
                        fieldIdentifier:(NSString*)fieldIdentifier
                      completionHandler:
                          (void (^)(NSArray<CWVAutofillSuggestion*>*))
                              completionHandler {
  __weak CWVAutofillController* weakSelf = self;
  id availableHandler = ^(BOOL suggestionsAvailable) {
    CWVAutofillController* strongSelf = weakSelf;
    if (!strongSelf) {
      completionHandler(@[]);
      return;
    }
    id retrieveHandler = ^(NSArray* suggestions,
                           id<FormSuggestionProvider> delegate) {
      NSMutableArray* autofillSuggestions = [NSMutableArray array];
      for (FormSuggestion* formSuggestion in suggestions) {
        CWVAutofillSuggestion* autofillSuggestion =
            [[CWVAutofillSuggestion alloc]
                initWithFormSuggestion:formSuggestion
                              formName:formName
                             fieldName:fieldName
                       fieldIdentifier:fieldIdentifier];
        [autofillSuggestions addObject:autofillSuggestion];
      }
      completionHandler([autofillSuggestions copy]);
    };
    [strongSelf->_autofillAgent retrieveSuggestionsForForm:formName
                                                 fieldName:fieldName
                                           fieldIdentifier:fieldIdentifier
                                                 fieldType:@""
                                                      type:nil
                                                typedValue:@" "
                                                  webState:strongSelf->_webState
                                         completionHandler:retrieveHandler];
  };
  // It is necessary to call |checkIfSuggestionsAvailableForForm| before
  // |retrieveSuggestionsForForm| because the former actually queries the db,
  // while the latter merely returns them.
  [_autofillAgent checkIfSuggestionsAvailableForForm:formName
                                           fieldName:fieldName
                                     fieldIdentifier:fieldIdentifier

                                           fieldType:@""
                                                type:nil
                                          typedValue:@" "
                                         isMainFrame:YES
                                            webState:_webState
                                   completionHandler:availableHandler];
}

- (void)fillSuggestion:(CWVAutofillSuggestion*)suggestion
     completionHandler:(nullable void (^)(void))completionHandler {
  [_autofillAgent didSelectSuggestion:suggestion.formSuggestion
                            fieldName:suggestion.fieldName
                      fieldIdentifier:suggestion.fieldIdentifier
                                 form:suggestion.formName
                    completionHandler:^{
                      if (completionHandler) {
                        completionHandler();
                      }
                    }];
}

- (void)removeSuggestion:(CWVAutofillSuggestion*)suggestion {
  // Identifier is greater than 0 when it is an autofill profile.
  if (suggestion.formSuggestion.identifier > 0) {
    _autofillManager->RemoveAutofillProfileOrCreditCard(
        suggestion.formSuggestion.identifier);
  } else {
    _autofillManager->RemoveAutocompleteEntry(
        base::SysNSStringToUTF16(suggestion.fieldName),
        base::SysNSStringToUTF16(suggestion.value));
  }
}

- (void)focusPreviousField {
  [_JSSuggestionManager selectPreviousElement];
}

- (void)focusNextField {
  [_JSSuggestionManager selectNextElement];
}

- (void)checkIfPreviousAndNextFieldsAreAvailableForFocusWithCompletionHandler:
    (void (^)(BOOL previous, BOOL next))completionHandler {
  [_JSSuggestionManager
      fetchPreviousAndNextElementsPresenceWithCompletionHandler:
          completionHandler];
}

#pragma mark - CWVAutofillClientIOSBridge

- (void)showAutofillPopup:(const std::vector<autofill::Suggestion>&)suggestions
            popupDelegate:
                (const base::WeakPtr<autofill::AutofillPopupDelegate>&)
                    delegate {
  NSMutableArray* formSuggestions = [[NSMutableArray alloc] init];
  for (const auto& suggestion : suggestions) {
    NSString* value = nil;
    NSString* displayDescription = nil;
    if (suggestion.frontend_id >= 0) {
      value = base::SysUTF16ToNSString(suggestion.value);
      displayDescription = base::SysUTF16ToNSString(suggestion.label);
    }

    // Suggestions without values are typically special suggestions such as
    // clear form or go to autofill settings. They are not supported by
    // CWVAutofillController.
    if (!value) {
      continue;
    }

    NSString* icon = base::SysUTF16ToNSString(suggestion.icon);
    NSInteger identifier = suggestion.frontend_id;

    FormSuggestion* formSuggestion =
        [FormSuggestion suggestionWithValue:value
                         displayDescription:displayDescription
                                       icon:icon
                                 identifier:identifier];
    [formSuggestions addObject:formSuggestion];
  }

  [_autofillAgent onSuggestionsReady:formSuggestions popupDelegate:delegate];
  if (delegate) {
    delegate->OnPopupShown();
  }
}

- (void)hideAutofillPopup {
  [_autofillAgent
      onSuggestionsReady:@[]
           popupDelegate:base::WeakPtr<autofill::AutofillPopupDelegate>()];
}

- (void)confirmSaveCreditCardLocally:(const autofill::CreditCard&)creditCard
                            callback:(const base::RepeatingClosure&)callback {
  if ([_delegate respondsToSelector:@selector
                 (autofillController:decidePolicyForLocalStorageOfCreditCard
                                       :decisionHandler:)]) {
    CWVCreditCard* card = [[CWVCreditCard alloc] initWithCreditCard:creditCard];
    __block base::RepeatingClosure scopedCallback = callback;
    [_delegate autofillController:self
        decidePolicyForLocalStorageOfCreditCard:card
                                decisionHandler:^(CWVStoragePolicy policy) {
                                  if (policy == CWVStoragePolicyAllow) {
                                    if (scopedCallback) {
                                      scopedCallback.Run();
                                      scopedCallback.Reset();
                                    }
                                  }
                                }];
  }
}

- (void)
showUnmaskPromptForCard:(const autofill::CreditCard&)creditCard
                 reason:(autofill::AutofillClient::UnmaskCardReason)reason
               delegate:(base::WeakPtr<autofill::CardUnmaskDelegate>)delegate {
  if ([_delegate respondsToSelector:@selector
                 (autofillController:verifyCreditCardWithVerifier:)]) {
    ios_web_view::WebViewBrowserState* browserState =
        ios_web_view::WebViewBrowserState::FromBrowserState(
            _webState->GetBrowserState());
    CWVCreditCardVerifier* verifier = [[CWVCreditCardVerifier alloc]
         initWithPrefs:browserState->GetPrefs()
        isOffTheRecord:browserState->IsOffTheRecord()
            creditCard:creditCard
                reason:reason
              delegate:delegate];
    [_delegate autofillController:self verifyCreditCardWithVerifier:verifier];

    // Store so verifier can receive unmask verification results later on.
    _verifier = verifier;
  }
}

- (void)didReceiveUnmaskVerificationResult:
    (autofill::AutofillClient::PaymentsRpcResult)result {
  [_verifier didReceiveUnmaskVerificationResult:result];
}

#pragma mark - AutofillDriverIOSBridge

- (void)onFormDataFilled:(uint16_t)query_id
                  result:(const autofill::FormData&)result {
  [_autofillAgent onFormDataFilled:result];
  if (_autofillManager) {
    _autofillManager->OnDidFillAutofillFormData(result, base::TimeTicks::Now());
  }
}

- (void)sendAutofillTypePredictionsToRenderer:
    (const std::vector<autofill::FormDataPredictions>&)forms {
  // Not supported.
}

#pragma mark - CRWWebStateObserver

- (void)webState:(web::WebState*)webState
    didRegisterFormActivity:(const web::FormActivityParams&)params {
  DCHECK_EQ(_webState, webState);

  [_JSSuggestionManager inject];

  NSString* nsFormName = base::SysUTF8ToNSString(params.form_name);
  NSString* nsFieldName = base::SysUTF8ToNSString(params.field_name);
  NSString* nsFieldIdentifier =
      base::SysUTF8ToNSString(params.field_identifier);
  NSString* nsValue = base::SysUTF8ToNSString(params.value);
  if (params.type == "focus") {
    if ([_delegate respondsToSelector:@selector
                   (autofillController:didFocusOnFieldWithName:fieldIdentifier
                                         :formName:value:)]) {
      [_delegate autofillController:self
            didFocusOnFieldWithName:nsFieldName
                    fieldIdentifier:nsFieldIdentifier
                           formName:nsFormName
                              value:nsValue];
    }
  } else if (params.type == "input") {
    if ([_delegate respondsToSelector:@selector
                   (autofillController:didInputInFieldWithName:fieldIdentifier
                                         :formName:value:)]) {
      [_delegate autofillController:self
            didInputInFieldWithName:nsFieldName
                    fieldIdentifier:nsFieldIdentifier
                           formName:nsFormName
                              value:nsValue];
    }
  } else if (params.type == "blur") {
    if ([_delegate respondsToSelector:@selector
                   (autofillController:didBlurOnFieldWithName:fieldIdentifier
                                         :formName:value:)]) {
      [_delegate autofillController:self
             didBlurOnFieldWithName:nsFieldName
                    fieldIdentifier:nsFieldIdentifier
                           formName:nsFormName
                              value:nsValue];
    }
  }
}

- (void)webState:(web::WebState*)webState
    didSubmitDocumentWithFormNamed:(const std::string&)formName
                     userInitiated:(BOOL)userInitiated
                       isMainFrame:(BOOL)isMainFrame {
  if ([_delegate respondsToSelector:@selector
                 (autofillController:didSubmitFormWithName:userInitiated
                                       :isMainFrame:)]) {
    [_delegate autofillController:self
            didSubmitFormWithName:base::SysUTF8ToNSString(formName)
                    userInitiated:userInitiated
                      isMainFrame:isMainFrame];
  }
}

- (void)webStateDestroyed:(web::WebState*)webState {
  DCHECK_EQ(_webState, webState);
  [_autofillAgent detachFromWebState];
  _autofillClient.reset();
  _webState->RemoveObserver(_webStateObserverBridge.get());
  _webStateObserverBridge.reset();
  _webState = nullptr;
}

@end
