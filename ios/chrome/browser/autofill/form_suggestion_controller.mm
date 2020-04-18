// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/autofill/form_suggestion_controller.h"

#include <memory>

#include "base/mac/foundation_util.h"
#include "base/mac/scoped_block.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_popup_delegate.h"
#import "components/autofill/ios/browser/form_suggestion.h"
#import "components/autofill/ios/browser/form_suggestion_provider.h"
#import "ios/chrome/browser/autofill/form_input_accessory_view_controller.h"
#import "ios/chrome/browser/autofill/form_suggestion_view.h"
#import "ios/chrome/browser/passwords/password_generation_utils.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/web/public/url_scheme_util.h"
#include "ios/web/public/web_state/form_activity_params.h"
#import "ios/web/public/web_state/js/crw_js_injection_receiver.h"
#import "ios/web/public/web_state/ui/crw_web_view_proxy.h"
#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Struct that describes suggestion state.
struct AutofillSuggestionState {
  AutofillSuggestionState(const std::string& form_name,
                          const std::string& field_name,
                          const std::string& field_identifier,
                          const std::string& typed_value);
  // The name of the form for autofill.
  std::string form_name;
  // The name of the field for autofill.
  std::string field_name;
  // The identifier of the field for autofill.
  std::string field_identifier;
  // The user-typed value in the field.
  std::string typed_value;
  // The suggestions for the form field. An array of |FormSuggestion|.
  NSArray* suggestions;
};

AutofillSuggestionState::AutofillSuggestionState(
    const std::string& form_name,
    const std::string& field_name,
    const std::string& field_identifier,
    const std::string& typed_value)
    : form_name(form_name),
      field_name(field_name),
      field_identifier(field_identifier),
      typed_value(typed_value) {}

}  // namespace

@interface FormSuggestionController () <FormInputAccessoryViewProvider> {
  // Form navigation delegate.
  __weak id<FormInputAccessoryViewDelegate> _delegate;

  // Callback to update the accessory view.
  AccessoryViewReadyCompletion accessoryViewUpdateBlock_;

  // Autofill suggestion state.
  std::unique_ptr<AutofillSuggestionState> _suggestionState;

  // Providers for suggestions, sorted according to the order in which
  // they should be asked for suggestions, with highest priority in front.
  NSArray* _suggestionProviders;

  // Access to WebView from the CRWWebController.
  id<CRWWebViewProxy> _webViewProxy;
}

// Returns an autoreleased input accessory view that shows |suggestions|.
- (UIView*)suggestionViewWithSuggestions:(NSArray*)suggestions;

// Updates keyboard for |suggestionState|.
- (void)updateKeyboard:(AutofillSuggestionState*)suggestionState;

// Updates keyboard with |suggestions|.
- (void)updateKeyboardWithSuggestions:(NSArray*)suggestions;

// Clears state in between page loads.
- (void)resetSuggestionState;

@end

@implementation FormSuggestionController {
  // The WebState this instance is observing. Will be null after
  // -webStateDestroyed: has been called.
  web::WebState* _webState;

  // Bridge to observe the web state from Objective-C.
  std::unique_ptr<web::WebStateObserverBridge> _webStateObserverBridge;

  // Manager for FormSuggestion JavaScripts.
  JsSuggestionManager* _jsSuggestionManager;

  // The provider for the current set of suggestions.
  __weak id<FormSuggestionProvider> _provider;
}

- (instancetype)initWithWebState:(web::WebState*)webState
                       providers:(NSArray*)providers
             JsSuggestionManager:(JsSuggestionManager*)jsSuggestionManager {
  self = [super init];
  if (self) {
    DCHECK(webState);
    _webState = webState;
    _webStateObserverBridge =
        std::make_unique<web::WebStateObserverBridge>(self);
    _webState->AddObserver(_webStateObserverBridge.get());
    _webViewProxy = webState->GetWebViewProxy();
    _jsSuggestionManager = jsSuggestionManager;
    _suggestionProviders = [providers copy];
  }
  return self;
}

- (instancetype)initWithWebState:(web::WebState*)webState
                       providers:(NSArray*)providers {
  JsSuggestionManager* jsSuggestionManager =
      base::mac::ObjCCast<JsSuggestionManager>(
          [webState->GetJSInjectionReceiver()
              instanceOfClass:[JsSuggestionManager class]]);
  return [self initWithWebState:webState
                      providers:providers
            JsSuggestionManager:jsSuggestionManager];
}

- (void)dealloc {
  if (_webState) {
    _webState->RemoveObserver(_webStateObserverBridge.get());
    _webStateObserverBridge.reset();
    _webState = nullptr;
  }
}

- (void)onNoSuggestionsAvailable {
}

- (void)detachFromWebState {
  if (_webState) {
    _webState->RemoveObserver(_webStateObserverBridge.get());
    _webStateObserverBridge.reset();
    _webState = nullptr;
  }
}

#pragma mark -
#pragma mark CRWWebStateObserver

- (void)webStateDestroyed:(web::WebState*)webState {
  DCHECK_EQ(_webState, webState);
  [self detachFromWebState];
}

- (void)webState:(web::WebState*)webState didLoadPageWithSuccess:(BOOL)success {
  DCHECK_EQ(_webState, webState);
  [self processPage:webState];
}

- (void)processPage:(web::WebState*)webState {
  [self resetSuggestionState];

  web::URLVerificationTrustLevel trustLevel =
      web::URLVerificationTrustLevel::kNone;
  const GURL pageURL(webState->GetCurrentURL(&trustLevel));
  if (trustLevel != web::URLVerificationTrustLevel::kAbsolute) {
    DLOG(WARNING) << "Page load not handled on untrusted page";
    return;
  }

  if (web::UrlHasWebScheme(pageURL) && webState->ContentIsHTML())
    [_jsSuggestionManager inject];
}

- (void)setWebViewProxy:(id<CRWWebViewProxy>)webViewProxy {
  _webViewProxy = webViewProxy;
}

- (void)retrieveSuggestionsForForm:(const web::FormActivityParams&)params
                          webState:(web::WebState*)webState {
  __weak FormSuggestionController* weakSelf = self;
  NSString* strongFormName = base::SysUTF8ToNSString(params.form_name);
  NSString* strongFieldName = base::SysUTF8ToNSString(params.field_name);
  NSString* strongFieldIdentifier =
      base::SysUTF8ToNSString(params.field_identifier);
  NSString* strongFieldType = base::SysUTF8ToNSString(params.field_type);
  NSString* strongType = base::SysUTF8ToNSString(params.type);
  NSString* strongValue =
      base::SysUTF8ToNSString(_suggestionState.get()->typed_value);
  BOOL is_main_frame = params.is_main_frame;

  // Build a block for each provider that will invoke its completion with YES
  // if the provider can provide suggestions for the specified form/field/type
  // and NO otherwise.
  NSMutableArray* findProviderBlocks = [[NSMutableArray alloc] init];
  for (NSUInteger i = 0; i < [_suggestionProviders count]; i++) {
    passwords::PipelineBlock block =
        ^(void (^completion)(BOOL success)) {
          // Access all the providers through |self| to guarantee that both
          // |self| and all the providers exist when the block is executed.
          // |_suggestionProviders| is immutable, so the subscripting is
          // always valid.
          FormSuggestionController* strongSelf = weakSelf;
          if (!strongSelf)
            return;
          id<FormSuggestionProvider> provider =
              strongSelf->_suggestionProviders[i];
          [provider checkIfSuggestionsAvailableForForm:strongFormName
                                             fieldName:strongFieldName
                                       fieldIdentifier:strongFieldIdentifier
                                             fieldType:strongFieldType
                                                  type:strongType
                                            typedValue:strongValue
                                           isMainFrame:is_main_frame
                                              webState:webState
                                     completionHandler:completion];
        };
    [findProviderBlocks addObject:block];
  }

  // Once the suggestions are retrieved, update the suggestions UI.
  SuggestionsReadyCompletion readyCompletion =
      ^(NSArray* suggestions, id<FormSuggestionProvider> provider) {
        [weakSelf onSuggestionsReady:suggestions provider:provider];
      };

  // Once a provider is found, use it to retrieve suggestions.
  passwords::PipelineCompletionBlock completion = ^(NSUInteger providerIndex) {
    if (providerIndex == NSNotFound) {
      [weakSelf onNoSuggestionsAvailable];
      return;
    }
    FormSuggestionController* strongSelf = weakSelf;
    if (!strongSelf)
      return;
    id<FormSuggestionProvider> provider =
        strongSelf->_suggestionProviders[providerIndex];
    [provider retrieveSuggestionsForForm:strongFormName
                               fieldName:strongFieldName
                         fieldIdentifier:strongFieldIdentifier
                               fieldType:strongFieldType
                                    type:strongType
                              typedValue:strongValue
                                webState:webState
                       completionHandler:readyCompletion];
  };

  // Run all the blocks in |findProviderBlocks| until one invokes its
  // completion with YES. The first one to do so will be passed to
  // |completion|.
  passwords::RunSearchPipeline(findProviderBlocks, completion);
}

- (void)onSuggestionsReady:(NSArray*)suggestions
                  provider:(id<FormSuggestionProvider>)provider {
  // TODO(ios): crbug.com/249916. If we can also pass in the form/field for
  // which |suggestions| are, we should check here if |suggestions| are for
  // the current active element. If not, reset |_suggestionState|.
  if (!_suggestionState) {
    // The suggestion state was reset in between the call to Autofill API (e.g.
    // OnQueryFormFieldAutofill) and this method being called back. Results are
    // therefore no longer relevant.
    return;
  }

  _provider = provider;
  _suggestionState->suggestions = [suggestions copy];
  [self updateKeyboard:_suggestionState.get()];
}

- (void)resetSuggestionState {
  _provider = nil;
  _suggestionState.reset();
}

- (void)clearSuggestions {
  // Note that other parts of the suggestionsState are not reset.
  if (!_suggestionState.get())
    return;
  _suggestionState->suggestions = [[NSArray alloc] init];
  [self updateKeyboard:_suggestionState.get()];
}

- (void)updateKeyboard:(AutofillSuggestionState*)suggestionState {
  if (!suggestionState) {
    if (accessoryViewUpdateBlock_)
      accessoryViewUpdateBlock_(nil, self);
  } else {
    [self updateKeyboardWithSuggestions:suggestionState->suggestions];
  }
}

- (void)updateKeyboardWithSuggestions:(NSArray*)suggestions {
  if (accessoryViewUpdateBlock_) {
    accessoryViewUpdateBlock_([self suggestionViewWithSuggestions:suggestions],
                              self);
  }
}

- (UIView*)suggestionViewWithSuggestions:(NSArray*)suggestions {
  CGRect frame = [_webViewProxy keyboardAccessory].frame;
  // Force the desired height on iPad where the height of the
  // inputAccessoryView is 0.
  if (IsIPadIdiom()) {
    frame.size.height = autofill::kInputAccessoryHeight;
  }
  FormSuggestionView* view =
      [[FormSuggestionView alloc] initWithFrame:frame
                                         client:self
                                    suggestions:suggestions];
  return view;
}

- (void)didSelectSuggestion:(FormSuggestion*)suggestion {
  if (!_suggestionState)
    return;

  // Send the suggestion to the provider. Upon completion advance the cursor
  // for single-field Autofill, or close the keyboard for full-form Autofill.
  __weak FormSuggestionController* weakSelf = self;
  [_provider
      didSelectSuggestion:suggestion
                fieldName:base::SysUTF8ToNSString(_suggestionState->field_name)
          fieldIdentifier:base::SysUTF8ToNSString(
                              _suggestionState->field_identifier)
                     form:base::SysUTF8ToNSString(_suggestionState->form_name)
        completionHandler:^{
          [[weakSelf accessoryViewDelegate] closeKeyboardWithoutButtonPress];
        }];
  _provider = nil;
}

- (id<FormInputAccessoryViewProvider>)accessoryViewProvider {
  return self;
}

#pragma mark FormInputAccessoryViewProvider

- (id<FormInputAccessoryViewDelegate>)accessoryViewDelegate {
  return _delegate;
}

- (void)setAccessoryViewDelegate:(id<FormInputAccessoryViewDelegate>)delegate {
  _delegate = delegate;
}

- (void)
checkIfAccessoryViewIsAvailableForForm:(const web::FormActivityParams&)params
                              webState:(web::WebState*)webState
                     completionHandler:
                         (AccessoryViewAvailableCompletion)completionHandler {
  [self processPage:webState];
  completionHandler(YES);
}

- (void)retrieveAccessoryViewForForm:(const web::FormActivityParams&)params
                            webState:(web::WebState*)webState
            accessoryViewUpdateBlock:
                (AccessoryViewReadyCompletion)accessoryViewUpdateBlock {
  _suggestionState.reset(
      new AutofillSuggestionState(params.form_name, params.field_name,
                                  params.field_identifier, params.value));
  accessoryViewUpdateBlock([self suggestionViewWithSuggestions:@[]], self);
  accessoryViewUpdateBlock_ = [accessoryViewUpdateBlock copy];
  [self retrieveSuggestionsForForm:params webState:webState];
}

- (void)inputAccessoryViewControllerDidReset:
        (FormInputAccessoryViewController*)controller {
  accessoryViewUpdateBlock_ = nil;
  [self resetSuggestionState];
}

- (void)resizeAccessoryView {
  [self updateKeyboard:_suggestionState.get()];
}

- (BOOL)getLogKeyboardAccessoryMetrics {
  return YES;
}

@end
