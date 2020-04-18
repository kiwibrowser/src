// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/autofill/autofill_controller.h"

#include <stdint.h>

#include <memory>
#include <utility>

#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_manager.h"
#include "components/autofill/core/browser/popup_item_ids.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#import "components/autofill/ios/browser/autofill_agent.h"
#import "components/autofill/ios/browser/autofill_client_ios_bridge.h"
#include "components/autofill/ios/browser/autofill_driver_ios.h"
#include "components/autofill/ios/browser/autofill_driver_ios_bridge.h"
#import "components/autofill/ios/browser/form_suggestion.h"
#import "components/autofill/ios/browser/form_suggestion_provider.h"
#include "components/infobars/core/infobar_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/infobars/infobar_manager_impl.h"
#include "ios/chrome/browser/pref_names.h"
#import "ios/chrome/browser/ui/autofill/chrome_autofill_client_ios.h"
#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using autofill::AutofillPopupDelegate;

@interface AutofillController ()<AutofillClientIOSBridge,
                                 AutofillDriverIOSBridge> {
  AutofillAgent* _autofillAgent;
  std::unique_ptr<autofill::ChromeAutofillClientIOS> _autofillClient;
  autofill::AutofillManager* _autofillManager;  // weak
}

@end

@implementation AutofillController

@synthesize browserState = _browserState;

- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
                            webState:(web::WebState*)webState
                       autofillAgent:(AutofillAgent*)autofillAgent
           passwordGenerationManager:
               (password_manager::PasswordGenerationManager*)
                   passwordGenerationManager
                     downloadEnabled:(BOOL)downloadEnabled {
  DCHECK(browserState);
  DCHECK(webState);
  self = [super init];
  if (self) {
    _browserState = browserState;
    infobars::InfoBarManager* infobarManager =
        InfoBarManagerImpl::FromWebState(webState);
    DCHECK(infobarManager);
    _autofillClient.reset(new autofill::ChromeAutofillClientIOS(
        browserState, webState, infobarManager, self,
        passwordGenerationManager));
    autofill::AutofillDriverIOS::CreateForWebStateAndDelegate(
        webState, _autofillClient.get(), self,
        GetApplicationContext()->GetApplicationLocale(),
        downloadEnabled
            ? autofill::AutofillManager::ENABLE_AUTOFILL_DOWNLOAD_MANAGER
            : autofill::AutofillManager::DISABLE_AUTOFILL_DOWNLOAD_MANAGER);
    _autofillAgent = autofillAgent;
    _autofillManager =
        autofill::AutofillDriverIOS::FromWebState(webState)->autofill_manager();
  }
  return self;
}

- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
           passwordGenerationManager:
               (password_manager::PasswordGenerationManager*)
                   passwordGenerationManager
                            webState:(web::WebState*)webState {
  AutofillAgent* autofillAgent =
      [[AutofillAgent alloc] initWithPrefService:browserState->GetPrefs()
                                        webState:webState];
  return [self initWithBrowserState:browserState
                           webState:webState
                      autofillAgent:autofillAgent
          passwordGenerationManager:passwordGenerationManager
                    downloadEnabled:YES];
}

- (void)dealloc {
  DCHECK(!_autofillAgent);  // detachFromWebController must have been called.
}

- (id<FormSuggestionProvider>)suggestionProvider {
  return _autofillAgent;
}

- (void)detachFromWebState {
  _autofillManager = nullptr;
  [_autofillAgent detachFromWebState];
  _autofillAgent = nil;
}

- (void)setBaseViewController:(UIViewController*)baseViewController {
  _autofillClient->SetBaseViewController(baseViewController);
}

#pragma mark - AutofillClientIOSBridge

- (void)
showAutofillPopup:(const std::vector<autofill::Suggestion>&)popup_suggestions
    popupDelegate:(const base::WeakPtr<AutofillPopupDelegate>&)delegate {
  DCHECK(
      _browserState->GetPrefs()->GetBoolean(autofill::prefs::kAutofillEnabled));
  // Convert the suggestions into an NSArray for the keyboard.
  NSMutableArray* suggestions = [[NSMutableArray alloc] init];
  for (size_t i = 0; i < popup_suggestions.size(); ++i) {
    // In the Chromium implementation the identifiers represent rows on the
    // drop down of options. These include elements that aren't relevant to us
    // such as separators ... see blink::WebAutofillClient::MenuItemIDSeparator
    // for example. We can't include that enum because it's from WebKit, but
    // fortunately almost all the entries we are interested in (profile or
    // autofill entries) are zero or positive. The only negative entry we are
    // interested in is autofill::POPUP_ITEM_ID_CLEAR_FORM, used to show the
    // "clear form" button.
    NSString* value = nil;
    NSString* displayDescription = nil;
    if (popup_suggestions[i].frontend_id >= 0) {
      // Value will contain the text to be filled in the selected element while
      // displayDescription will contain a summary of the data to be filled in
      // the other elements.
      value = base::SysUTF16ToNSString(popup_suggestions[i].value);
      displayDescription = base::SysUTF16ToNSString(popup_suggestions[i].label);
    } else if (popup_suggestions[i].frontend_id ==
               autofill::POPUP_ITEM_ID_CLEAR_FORM) {
      // Show the "clear form" button.
      value = base::SysUTF16ToNSString(popup_suggestions[i].value);
    }

    if (!value)
      continue;

    FormSuggestion* suggestion = [FormSuggestion
        suggestionWithValue:value
         displayDescription:displayDescription
                       icon:base::SysUTF16ToNSString(popup_suggestions[i].icon)
                 identifier:popup_suggestions[i].frontend_id];
    [suggestions addObject:suggestion];
  }
  [_autofillAgent onSuggestionsReady:suggestions popupDelegate:delegate];

  // The parameter is an optional callback.
  if (delegate)
    delegate->OnPopupShown();
}

- (void)hideAutofillPopup {
  [_autofillAgent onSuggestionsReady:@[]
                       popupDelegate:base::WeakPtr<AutofillPopupDelegate>()];
}

#pragma mark - AutofillDriverIOSBridge

- (void)onFormDataFilled:(uint16_t)query_id
                  result:(const autofill::FormData&)result {
  DCHECK(
      _browserState->GetPrefs()->GetBoolean(autofill::prefs::kAutofillEnabled));
  [_autofillAgent onFormDataFilled:result];
  if (_autofillManager)
    _autofillManager->OnDidFillAutofillFormData(result, base::TimeTicks::Now());
}

- (void)sendAutofillTypePredictionsToRenderer:
    (const std::vector<autofill::FormDataPredictions>&)forms {
  [_autofillAgent renderAutofillTypePredictions:forms];
}

@end
