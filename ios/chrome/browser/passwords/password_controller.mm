// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/passwords/password_controller.h"

#include <stddef.h>

#include <algorithm>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#import "base/mac/bind_objc_block.h"
#include "base/mac/foundation_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string16.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/password_form.h"
#include "components/autofill/core/common/password_form_fill_data.h"
#include "components/autofill/ios/browser/autofill_util.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/infobars/core/infobar_manager.h"
#include "components/password_manager/core/browser/form_parsing/ios_form_parser.h"
#include "components/password_manager/core/browser/password_bubble_experiment.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/password_manager/core/browser/password_manager_client.h"
#include "components/password_manager/core/browser/password_manager_driver.h"
#include "components/sync/driver/sync_service.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/infobars/infobar_manager_impl.h"
#include "ios/chrome/browser/passwords/account_select_fill_data.h"
#include "ios/chrome/browser/passwords/credential_manager.h"
#include "ios/chrome/browser/passwords/credential_manager_features.h"
#import "ios/chrome/browser/passwords/ios_chrome_save_password_infobar_delegate.h"
#import "ios/chrome/browser/passwords/ios_chrome_update_password_infobar_delegate.h"
#import "ios/chrome/browser/passwords/js_password_manager.h"
#import "ios/chrome/browser/passwords/notify_auto_signin_view_controller.h"
#import "ios/chrome/browser/passwords/password_form_filler.h"
#import "ios/chrome/browser/ssl/insecure_input_tab_helper.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"
#import "ios/chrome/browser/ui/commands/application_commands.h"
#include "ios/chrome/browser/web/tab_id_tab_helper.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/web/public/origin_util.h"
#include "ios/web/public/url_scheme_util.h"
#import "ios/web/public/web_state/js/crw_js_injection_receiver.h"
#import "ios/web/public/web_state/web_state.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using autofill::FormData;
using autofill::PasswordForm;
using password_manager::AccountSelectFillData;
using password_manager::FillData;
using password_manager::PasswordFormManagerForUI;
using password_manager::PasswordManager;
using password_manager::PasswordManagerClient;
using password_manager::PasswordManagerDriver;

typedef void (^PasswordSuggestionsAvailableCompletion)(
    const AccountSelectFillData*);

namespace {
// Types of password infobars to display.
enum class PasswordInfoBarType { SAVE, UPDATE };

// Types of password suggestion in the keyboard accessory. Used for metrics
// collection.
enum class PasswordSuggestionType {
  // Credentials are listed.
  CREDENTIALS = 0,
  // Only "Show All" is listed.
  SHOW_ALL = 1,
  COUNT
};

// Duration for notify user auto-sign in dialog being displayed.
constexpr int kNotifyAutoSigninDuration = 3;  // seconds

// Script command prefix for form changes. Possible command to be sent from
// injected JS is 'form.buttonClicked'.
constexpr char kCommandPrefix[] = "passwordForm";

// The string ' •••' appended to the username in the suggestion.
NSString* const kSuggestionSuffix = @" ••••••••";

void LogSuggestionClicked(PasswordSuggestionType type) {
  UMA_HISTOGRAM_ENUMERATION("PasswordManager.SuggestionClicked", type,
                            PasswordSuggestionType::COUNT);
}

void LogSuggestionShown(PasswordSuggestionType type) {
  UMA_HISTOGRAM_ENUMERATION("PasswordManager.SuggestionShown", type,
                            PasswordSuggestionType::COUNT);
}
}  // namespace

@interface PasswordController ()

// This is set to YES as soon as the associated WebState is destroyed.
@property(readonly) BOOL isWebStateDestroyed;

// View controller for auto sign-in notification, owned by this
// PasswordController.
@property(nonatomic, strong)
    NotifyUserAutoSigninViewController* notifyAutoSigninViewController;

@end

@interface PasswordController ()<FormSuggestionProvider, PasswordFormFiller>

// Parses the |jsonString| which contatins the password forms found on a web
// page to populate the |forms| vector.
- (void)getPasswordForms:(std::vector<autofill::PasswordForm>*)forms
           fromFormsJSON:(NSString*)jsonString
                 pageURL:(const GURL&)pageURL;

// Informs the |passwordManager_| of the password forms (if any were present)
// that have been found on the page.
- (void)didFinishPasswordFormExtraction:
    (const std::vector<autofill::PasswordForm>&)forms;

// Autofills |username| and |password| into the form specified by |formData|,
// invoking |completionHandler| when finished with YES if successful and
// NO otherwise. |completionHandler| may be nil.
- (void)fillPasswordForm:(const autofill::PasswordFormFillData&)formData
            withUsername:(const base::string16&)username
                password:(const base::string16&)password
       completionHandler:(void (^)(BOOL))completionHandler;

// Autofills credentials into the page. Credentials and input fields are
// specified by |fillData|. Invoking |completionHandler| when finished with YES
// if successful and NO otherwise. |completionHandler| may be nil.
- (void)fillPasswordFormWithFillData:(const password_manager::FillData&)fillData
                   completionHandler:(void (^)(BOOL))completionHandler;

// Uses JavaScript to find password forms. Calls |completionHandler| with the
// extracted information used for matching and saving passwords. Calls
// |completionHandler| with an empty vector if no password forms are found.
// |completionHandler| cannot be nil.
- (void)findPasswordFormsWithCompletionHandler:
    (void (^)(const std::vector<autofill::PasswordForm>&))completionHandler;

// Finds all password forms in DOM and sends them to the password store for
// fetching stored credentials.
- (void)findPasswordFormsAndSendThemToPasswordStore;

// Finds the currently submitted password form and calls |completionHandler|
// with the populated data structure. |found| is YES if the current form was
// found successfully, NO otherwise. |completionHandler| cannot be nil.
- (void)extractSubmittedPasswordForm:(const std::string&)formName
                   completionHandler:
                       (void (^)(BOOL found,
                                 const autofill::PasswordForm& form))
                           completionHandler;

// Displays infobar for |form| with |type|. If |type| is UPDATE, the user
// is prompted to update the password. If |type| is SAVE, the user is prompted
// to save the password.
- (void)showInfoBarForForm:(std::unique_ptr<PasswordFormManagerForUI>)form
               infoBarType:(PasswordInfoBarType)type;

// Handler for injected JavaScript callbacks.
- (BOOL)handleScriptCommand:(const base::DictionaryValue&)JSONCommand;

@end

namespace {

// Constructs an array of FormSuggestions, each corresponding to a username/
// password pair in |AccountSelectFillData|. "Show all" item is appended.
NSArray* BuildSuggestions(const AccountSelectFillData& fillData,
                          NSString* formName,
                          NSString* fieldIdentifier,
                          NSString* fieldType) {
  base::string16 form_name = base::SysNSStringToUTF16(formName);
  base::string16 field_identifier = base::SysNSStringToUTF16(fieldIdentifier);
  bool is_password_field = [fieldType isEqualToString:@"password"];

  NSMutableArray* suggestions = [NSMutableArray array];
  PasswordSuggestionType suggestion_type = PasswordSuggestionType::SHOW_ALL;
  if (fillData.IsSuggestionsAvailable(form_name, field_identifier,
                                      is_password_field)) {
    std::vector<password_manager::UsernameAndRealm> username_and_realms_ =
        fillData.RetrieveSuggestions(form_name, field_identifier,
                                     is_password_field);

    // Add credentials.
    for (const auto& username_and_realm : username_and_realms_) {
      NSString* value = [base::SysUTF16ToNSString(username_and_realm.username)
          stringByAppendingString:kSuggestionSuffix];
      NSString* origin =
          username_and_realm.realm.empty()
              ? nil
              : base::SysUTF8ToNSString(username_and_realm.realm);

      [suggestions addObject:[FormSuggestion suggestionWithValue:value
                                              displayDescription:origin
                                                            icon:nil
                                                      identifier:0]];
      suggestion_type = PasswordSuggestionType::CREDENTIALS;
    }
  }

  // Add "Show all".
  NSString* showAll = l10n_util::GetNSString(IDS_IOS_SHOW_ALL_PASSWORDS);
  [suggestions addObject:[FormSuggestion suggestionWithValue:showAll
                                          displayDescription:nil
                                                        icon:nil
                                                  identifier:1]];
  LogSuggestionShown(suggestion_type);

  return [suggestions copy];
}

// Serializes arguments into a JSON string that can be used by the JS side
// of PasswordController.
NSString* SerializeFillData(const GURL& origin,
                            const GURL& action,
                            const base::string16& username_element,
                            const base::string16& username_value,
                            const base::string16& password_element,
                            const base::string16& password_value) {
  base::DictionaryValue rootDict;
  rootDict.SetString("origin", origin.spec());
  rootDict.SetString("action", action.spec());

  auto fieldList = std::make_unique<base::ListValue>();

  auto usernameField = std::make_unique<base::DictionaryValue>();
  usernameField->SetString("name", username_element);
  usernameField->SetString("value", username_value);
  fieldList->Append(std::move(usernameField));

  auto passwordField = std::make_unique<base::DictionaryValue>();
  passwordField->SetString("name", password_element);
  passwordField->SetString("value", password_value);
  fieldList->Append(std::move(passwordField));

  rootDict.Set("fields", std::move(fieldList));

  std::string jsonString;
  base::JSONWriter::Write(rootDict, &jsonString);
  return base::SysUTF8ToNSString(jsonString);
}

// Serializes |formData| into a JSON string that can be used by the JS side
// of PasswordController.
NSString* SerializePasswordFormFillData(
    const autofill::PasswordFormFillData& formData) {
  return SerializeFillData(
      formData.origin, formData.action, formData.username_field.name,
      formData.username_field.value, formData.password_field.name,
      formData.password_field.value);
}

NSString* SerializeFillData(const FillData& fillData) {
  return SerializeFillData(fillData.origin, fillData.action,
                           fillData.username_element, fillData.username_value,
                           fillData.password_element, fillData.password_value);
}

// Returns true if the trust level for the current page URL of |web_state| is
// kAbsolute. If |page_url| is not null, fills it with the current page URL.
bool GetPageURLAndCheckTrustLevel(web::WebState* web_state, GURL* page_url) {
  auto trustLevel = web::URLVerificationTrustLevel::kNone;
  GURL dummy;
  if (!page_url)
    page_url = &dummy;
  *page_url = web_state->GetCurrentURL(&trustLevel);
  return trustLevel == web::URLVerificationTrustLevel::kAbsolute;
}

}  // namespace

@implementation PasswordController {
  std::unique_ptr<PasswordManager> passwordManager_;
  std::unique_ptr<PasswordManagerClient> passwordManagerClient_;
  std::unique_ptr<PasswordManagerDriver> passwordManagerDriver_;
  std::unique_ptr<CredentialManager> credentialManager_;

  JsPasswordManager* passwordJsManager_;

  AccountSelectFillData fillData_;

  // The WebState this instance is observing. Will be null after
  // -webStateDestroyed: has been called.
  web::WebState* webState_;

  // Bridge to observe WebState from Objective-C.
  std::unique_ptr<web::WebStateObserverBridge> webStateObserverBridge_;

  // Timer for hiding "Signing in as ..." notification.
  base::OneShotTimer notifyAutoSigninTimer_;

  // True indicates that a request for credentials has been sent to the password
  // store.
  BOOL sentRequestToStore_;

  // The completion to inform FormSuggestionController that suggestions are
  // available for a given form and field.
  PasswordSuggestionsAvailableCompletion suggestionsAvailableCompletion_;

  // User credential waiting to be displayed in autosign-in snackbar, once tab
  // becomes active.
  std::unique_ptr<autofill::PasswordForm> pendingAutoSigninPasswordForm_;
}

@synthesize isWebStateDestroyed = _isWebStateDestroyed;

@synthesize baseViewController = _baseViewController;

@synthesize dispatcher = _dispatcher;

@synthesize delegate = _delegate;

@synthesize notifyAutoSigninViewController = _notifyAutoSigninViewController;

- (instancetype)initWithWebState:(web::WebState*)webState {
  self = [self initWithWebState:webState
                         client:nullptr];
  return self;
}

- (instancetype)initWithWebState:(web::WebState*)webState
                          client:(std::unique_ptr<PasswordManagerClient>)
                                     passwordManagerClient {
  self = [super init];
  if (self) {
    DCHECK(webState);
    webState_ = webState;
    if (passwordManagerClient)
      passwordManagerClient_ = std::move(passwordManagerClient);
    else
      passwordManagerClient_.reset(new IOSChromePasswordManagerClient(self));
    passwordManager_.reset(new PasswordManager(passwordManagerClient_.get()));
    passwordManagerDriver_.reset(new IOSChromePasswordManagerDriver(self));

    webStateObserverBridge_ =
        std::make_unique<web::WebStateObserverBridge>(self);
    webState_->AddObserver(webStateObserverBridge_.get());
    passwordJsManager_ = [[JsPasswordManager alloc]
        initWithReceiver:webState_->GetJSInjectionReceiver()];
    sentRequestToStore_ = NO;

    if (base::FeatureList::IsEnabled(features::kCredentialManager)) {
      credentialManager_ = std::make_unique<CredentialManager>(
          passwordManagerClient_.get(), webState_);
    }

    __weak PasswordController* weakSelf = self;
    auto callback = base::BindBlockArc(^bool(const base::DictionaryValue& JSON,
                                             const GURL& originURL,
                                             bool userIsInteracting) {
      // |originURL| and |isInteracting| aren't used.
      return [weakSelf handleScriptCommand:JSON];
    });
    webState_->AddScriptCommandCallback(callback, kCommandPrefix);
  }
  return self;
}

- (void)dealloc {
  [self detach];

  if (webState_) {
    webState_->RemoveObserver(webStateObserverBridge_.get());
    webStateObserverBridge_.reset();
    webState_ = nullptr;
  }
}

- (ios::ChromeBrowserState*)browserState {
  return webState_ ? ios::ChromeBrowserState::FromBrowserState(
                         webState_->GetBrowserState())
                   : nullptr;
}

- (const GURL&)lastCommittedURL {
  return webState_ ? webState_->GetLastCommittedURL() : GURL::EmptyGURL();
}

- (void)detach {
  if (webState_) {
    webState_->RemoveScriptCommandCallback(kCommandPrefix);
    webState_->RemoveObserver(webStateObserverBridge_.get());
    webStateObserverBridge_.reset();
    webState_ = nullptr;
  }

  passwordManagerDriver_.reset();
  passwordManager_.reset();
  passwordManagerClient_.reset();
  credentialManager_.reset();
}

#pragma mark -
#pragma mark Properties

- (id<PasswordFormFiller>)passwordFormFiller {
  return self;
}

#pragma mark -
#pragma mark PasswordFormFiller

- (void)findAndFillPasswordForms:(NSString*)username
                        password:(NSString*)password
               completionHandler:(void (^)(BOOL))completionHandler {
  [self findPasswordFormsWithCompletionHandler:^(
            const std::vector<autofill::PasswordForm>& forms) {
    for (const auto& form : forms) {
      autofill::PasswordFormFillData formData;
      std::map<base::string16, const autofill::PasswordForm*> matches;
      // Initialize |matches| to satisfy the expectation from
      // InitPasswordFormFillData() that the preferred match (3rd parameter)
      // should be one of the |matches|.
      matches.insert(std::make_pair(form.username_value, &form));
      autofill::InitPasswordFormFillData(form, matches, &form, false,
                                         &formData);
      [self fillPasswordForm:formData
                withUsername:base::SysNSStringToUTF16(username)
                    password:base::SysNSStringToUTF16(password)
           completionHandler:completionHandler];
    }
  }];
}

#pragma mark -
#pragma mark CRWWebStateObserver

// If Tab was shown, and there is a pending PasswordForm, display autosign-in
// notification.
- (void)webStateWasShown:(web::WebState*)webState {
  DCHECK_EQ(webState_, webState);
  if (pendingAutoSigninPasswordForm_) {
    [self showAutosigninNotification:std::move(pendingAutoSigninPasswordForm_)];
    pendingAutoSigninPasswordForm_.reset();
  }
}

// If Tab was hidden, hide auto sign-in notification.
- (void)webStateWasHidden:(web::WebState*)webState {
  DCHECK_EQ(webState_, webState);
  [self hideAutosigninNotification];
}

- (void)webState:(web::WebState*)webState didLoadPageWithSuccess:(BOOL)success {
  DCHECK_EQ(webState_, webState);
  // Clear per-page state.
  fillData_.Reset();
  sentRequestToStore_ = NO;
  suggestionsAvailableCompletion_ = nil;

  // Retrieve the identity of the page. In case the page might be malicous,
  // returns early.
  GURL pageURL;
  if (!GetPageURLAndCheckTrustLevel(webState, &pageURL))
    return;

  if (!web::UrlHasWebScheme(pageURL))
    return;

  // Notify the password manager that the page loaded so it can clear its own
  // per-page state.
  passwordManager_->DidNavigateMainFrame();

  if (!webState->ContentIsHTML()) {
    // If the current page is not HTML, it does not contain any HTML forms.
    [self
        didFinishPasswordFormExtraction:std::vector<autofill::PasswordForm>()];
  }

  [self findPasswordFormsAndSendThemToPasswordStore];
}

- (void)webState:(web::WebState*)webState
    didSubmitDocumentWithFormNamed:(const std::string&)formName
                     userInitiated:(BOOL)userInitiated
                       isMainFrame:(BOOL)isMainFrame {
  DCHECK_EQ(webState_, webState);
  __weak PasswordController* weakSelf = self;
  // This code is racing against the new page loading and will not get the
  // password form data if the page has changed. In most cases this code wins
  // the race.
  // TODO(crbug.com/418827): Fix this by passing in more data from the JS side.
  id completionHandler = ^(BOOL found, const autofill::PasswordForm& form) {
    PasswordController* strongSelf = weakSelf;
    if (strongSelf && ![strongSelf isWebStateDestroyed] &&
        strongSelf.passwordManager) {
      if (isMainFrame) {
        strongSelf.passwordManager->OnPasswordFormSubmitted(
            strongSelf.passwordManagerDriver, form);
      } else {
        // Show a save prompt immediately because for iframes it is very hard to
        // figure out correctness of password forms submission.
        strongSelf.passwordManager->OnPasswordFormSubmittedNoChecks(
            strongSelf.passwordManagerDriver, form);
      }
    }
  };
  [self extractSubmittedPasswordForm:formName
                   completionHandler:completionHandler];
}

- (void)webStateDestroyed:(web::WebState*)webState {
  DCHECK_EQ(webState_, webState);
  _isWebStateDestroyed = YES;
  [self detach];
}

#pragma mark - Private methods.

- (void)findPasswordFormsWithCompletionHandler:
    (void (^)(const std::vector<autofill::PasswordForm>&))completionHandler {
  DCHECK(completionHandler);

  if (!webState_)
    return;

  GURL pageURL;
  if (!GetPageURLAndCheckTrustLevel(webState_, &pageURL)) {
    return;
  }

  __weak PasswordController* weakSelf = self;
  [passwordJsManager_ findPasswordFormsWithCompletionHandler:^(
                          NSString* jsonString) {
    std::vector<autofill::PasswordForm> forms;
    [weakSelf getPasswordForms:&forms fromFormsJSON:jsonString pageURL:pageURL];
    completionHandler(forms);
  }];
}

- (void)findPasswordFormsAndSendThemToPasswordStore {
  // Read all password forms from the page and send them to the password
  // manager.
  __weak PasswordController* weakSelf = self;
  [self findPasswordFormsWithCompletionHandler:^(
            const std::vector<autofill::PasswordForm>& forms) {
    [weakSelf didFinishPasswordFormExtraction:forms];
  }];
}

- (void)getPasswordForms:(std::vector<autofill::PasswordForm>*)forms
           fromFormsJSON:(NSString*)JSONNSString
                 pageURL:(const GURL&)pageURL {
  DCHECK(forms);
  std::vector<autofill::FormData> formsData;
  if (!autofill::ExtractFormsData(JSONNSString, false, base::string16(),
                                  pageURL, &formsData)) {
    return;
  }

  for (const auto& formData : formsData) {
    std::unique_ptr<PasswordForm> form =
        ParseFormData(formData, password_manager::FormParsingMode::FILLING);
    if (form)
      forms->push_back(*form);
  }
}

- (void)extractSubmittedPasswordForm:(const std::string&)formName
                   completionHandler:
                       (void (^)(BOOL found,
                                 const autofill::PasswordForm& form))
                           completionHandler {
  DCHECK(completionHandler);

  if (!webState_)
    return;

  GURL pageURL;
  if (!GetPageURLAndCheckTrustLevel(webState_, &pageURL)) {
    completionHandler(NO, autofill::PasswordForm());
    return;
  }

  id extractSubmittedFormCompletionHandler = ^(NSString* jsonString) {
    std::unique_ptr<base::Value> formValue = autofill::ParseJson(jsonString);
    if (!formValue) {
      completionHandler(NO, PasswordForm());
      return;
    }

    FormData formData;
    if (!autofill::ExtractFormData(*formValue, false, base::string16(), pageURL,
                                   &formData)) {
      completionHandler(NO, PasswordForm());
      return;
    }

    std::unique_ptr<PasswordForm> form =
        ParseFormData(formData, password_manager::FormParsingMode::SAVING);
    if (!form) {
      completionHandler(NO, PasswordForm());
      return;
    }

    completionHandler(YES, *form);
  };

  [passwordJsManager_ extractForm:base::SysUTF8ToNSString(formName)
                completionHandler:extractSubmittedFormCompletionHandler];
}

- (void)didFinishPasswordFormExtraction:
    (const std::vector<autofill::PasswordForm>&)forms {
  // Do nothing if |self| has been detached.
  if (!passwordManager_)
    return;

  if (!forms.empty()) {
    // Notify web_state about password forms, so that this can be taken into
    // account for the security state.
    if (webState_ && !web::IsOriginSecure(webState_->GetLastCommittedURL())) {
      InsecureInputTabHelper::GetOrCreateForWebState(webState_)
          ->DidShowPasswordFieldInInsecureContext();
    }

    sentRequestToStore_ = YES;
    // Invoke the password manager callback to autofill password forms
    // on the loaded page.
    passwordManager_->OnPasswordFormsParsed(passwordManagerDriver_.get(),
                                            forms);
  } else {
    [self onNoSavedCredentials];
  }
  // Invoke the password manager callback to check if password was
  // accepted or rejected. If accepted, infobar is presented. If
  // rejected, the provisionally saved password is deleted. On Chrome
  // w/ a renderer, it is the renderer who calls OnPasswordFormsParsed()
  // and OnPasswordFormsRendered(). Bling has to improvised a bit on the
  // ordering of these two calls.
  passwordManager_->OnPasswordFormsRendered(passwordManagerDriver_.get(), forms,
                                            true);
}

#pragma mark -
#pragma mark FormSuggestionProvider

- (id<FormSuggestionProvider>)suggestionProvider {
  return self;
}

- (void)checkIfSuggestionsAvailableForForm:(NSString*)formName
                                 fieldName:(NSString*)fieldName
                           fieldIdentifier:(NSString*)fieldIdentifier
                                 fieldType:(NSString*)fieldType
                                      type:(NSString*)type
                                typedValue:(NSString*)typedValue
                               isMainFrame:(BOOL)isMainFrame
                                  webState:(web::WebState*)webState
                         completionHandler:
                             (SuggestionsAvailableCompletion)completion {
  if (!GetPageURLAndCheckTrustLevel(webState, nullptr)) {
    completion(NO);
    return;
  }

  bool should_send_request_to_store =
      !sentRequestToStore_ && [type isEqual:@"focus"];
  bool is_password_field = [fieldType isEqual:@"password"];

  if (should_send_request_to_store) {
    // Save the completion and go look for suggestions.
    suggestionsAvailableCompletion_ =
        [^(const AccountSelectFillData* fill_data) {
          if (is_password_field) {
            // Always display "Show all" on the password field.
            completion(YES);
          } else if (!fill_data) {
            completion(NO);
          } else {
            completion(fill_data->IsSuggestionsAvailable(
                base::SysNSStringToUTF16(formName),
                base::SysNSStringToUTF16(fieldIdentifier), false));
          }
        } copy];
    [self findPasswordFormsAndSendThemToPasswordStore];
    return;
  }

  completion(is_password_field ||
             fillData_.IsSuggestionsAvailable(
                 base::SysNSStringToUTF16(formName),
                 base::SysNSStringToUTF16(fieldIdentifier), false));
}

- (void)retrieveSuggestionsForForm:(NSString*)formName
                         fieldName:(NSString*)fieldName2
                   fieldIdentifier:(NSString*)fieldIdentifier
                         fieldType:(NSString*)fieldType
                              type:(NSString*)type
                        typedValue:(NSString*)typedValue
                          webState:(web::WebState*)webState
                 completionHandler:(SuggestionsReadyCompletion)completion {
  DCHECK(GetPageURLAndCheckTrustLevel(webState, nullptr));
  completion(BuildSuggestions(fillData_, formName, fieldIdentifier, fieldType),
             self);
}

- (void)didSelectSuggestion:(FormSuggestion*)suggestion
                  fieldName:(NSString*)fieldName2
            fieldIdentifier:(NSString*)fieldIdentifier
                       form:(NSString*)formName
          completionHandler:(SuggestionHandledCompletion)completion {
  if (suggestion.identifier == 1) {
    // Navigate to the settings list.
    [self.delegate displaySavedPasswordList];
    completion();
    LogSuggestionClicked(PasswordSuggestionType::SHOW_ALL);
    return;
  }
  LogSuggestionClicked(PasswordSuggestionType::CREDENTIALS);
  base::string16 value = base::SysNSStringToUTF16(suggestion.value);
  DCHECK([suggestion.value hasSuffix:kSuggestionSuffix]);
  value.erase(value.length() - kSuggestionSuffix.length);
  std::unique_ptr<FillData> fillData = fillData_.GetFillData(value);

  if (!fillData)
    completion();

  [self fillPasswordFormWithFillData:*fillData
                   completionHandler:^(BOOL success) {
                     completion();
                   }];
}

#pragma mark - PasswordManagerClientDelegate

- (void)showSavePasswordInfoBar:
    (std::unique_ptr<PasswordFormManagerForUI>)formToSave {
  [self showInfoBarForForm:std::move(formToSave)
               infoBarType:PasswordInfoBarType::SAVE];
}

- (void)showUpdatePasswordInfoBar:
    (std::unique_ptr<PasswordFormManagerForUI>)formToUpdate {
  [self showInfoBarForForm:std::move(formToUpdate)
               infoBarType:PasswordInfoBarType::UPDATE];
}

// Hides auto sign-in notification. Removes the view from superview and destroys
// the controller.
// TODO(crbug.com/435048): Animate disappearance.
- (void)hideAutosigninNotification {
  [self.notifyAutoSigninViewController willMoveToParentViewController:nil];
  [self.notifyAutoSigninViewController.view removeFromSuperview];
  [self.notifyAutoSigninViewController removeFromParentViewController];
  self.notifyAutoSigninViewController = nil;
}

// Shows auto sign-in notification and schedules hiding it after 3 seconds.
// TODO(crbug.com/435048): Animate appearance.
- (void)showAutosigninNotification:
    (std::unique_ptr<autofill::PasswordForm>)formSignedIn {
  if (!webState_)
    return;

  // If a notification is already being displayed, hides the old one, then shows
  // the new one.
  if (self.notifyAutoSigninViewController) {
    notifyAutoSigninTimer_.Stop();
    [self hideAutosigninNotification];
  }

  // Creates view controller then shows the subview.
  self.notifyAutoSigninViewController = [
      [NotifyUserAutoSigninViewController alloc]
      initWithUsername:base::SysUTF16ToNSString(formSignedIn->username_value)
               iconURL:formSignedIn->icon_url
         contextGetter:webState_->GetBrowserState()->GetRequestContext()];
  TabIdTabHelper* tabIdHelper = TabIdTabHelper::FromWebState(webState_);
  if (![_delegate displaySignInNotification:self.notifyAutoSigninViewController
                                  fromTabId:tabIdHelper->tab_id()]) {
    // The notification was not shown. Store the password form in
    // |pendingAutoSigninPasswordForm_| to show the notification later.
    pendingAutoSigninPasswordForm_ = std::move(formSignedIn);
    self.notifyAutoSigninViewController = nil;
    return;
  }

  // Hides notification after 3 seconds.
  __weak PasswordController* weakSelf = self;
  notifyAutoSigninTimer_.Start(
      FROM_HERE, base::TimeDelta::FromSeconds(kNotifyAutoSigninDuration),
      base::BindBlockArc(^{
        [weakSelf hideAutosigninNotification];
      }));
}

#pragma mark -
#pragma mark WebPasswordFormData Adaptation

- (void)fillPasswordForm:(const autofill::PasswordFormFillData&)formData
            withUsername:(const base::string16&)username
                password:(const base::string16&)password
       completionHandler:(void (^)(BOOL))completionHandler {
  if (formData.origin.GetOrigin() != self.lastCommittedURL.GetOrigin()) {
    if (completionHandler)
      completionHandler(NO);
    return;
  }

  // Send JSON over to the web view.
  [passwordJsManager_ fillPasswordForm:SerializePasswordFormFillData(formData)
                          withUsername:base::SysUTF16ToNSString(username)
                              password:base::SysUTF16ToNSString(password)
                     completionHandler:^(BOOL result) {
                       if (completionHandler)
                         completionHandler(result);
                     }];
}

- (void)fillPasswordForm:(const autofill::PasswordFormFillData&)formData
       completionHandler:(void (^)(BOOL))completionHandler {
  fillData_.Add(formData);

  if (suggestionsAvailableCompletion_) {
    suggestionsAvailableCompletion_(&fillData_);
    suggestionsAvailableCompletion_ = nil;
  }

  // Don't fill immediately if waiting for the user to type a username.
  if (formData.wait_for_username) {
    if (completionHandler)
      completionHandler(NO);
    return;
  }

  [self fillPasswordForm:formData
            withUsername:formData.username_field.value
                password:formData.password_field.value
       completionHandler:completionHandler];
}

- (void)onNoSavedCredentials {
  if (suggestionsAvailableCompletion_)
    suggestionsAvailableCompletion_(nullptr);
  suggestionsAvailableCompletion_ = nil;
}

- (void)fillPasswordFormWithFillData:(const password_manager::FillData&)fillData
                   completionHandler:(void (^)(BOOL))completionHandler {
  // Send JSON over to the web view.
  [passwordJsManager_
       fillPasswordForm:SerializeFillData(fillData)
           withUsername:base::SysUTF16ToNSString(fillData.username_value)
               password:base::SysUTF16ToNSString(fillData.password_value)
      completionHandler:^(BOOL result) {
        if (completionHandler)
          completionHandler(result);
      }];
}

- (BOOL)handleScriptCommand:(const base::DictionaryValue&)JSONCommand {
  std::string command;
  if (!JSONCommand.GetString("command", &command))
    return NO;

  if (command != "passwordForm.submitButtonClick")
    return NO;

  GURL pageURL;
  if (!GetPageURLAndCheckTrustLevel(webState_, &pageURL))
    return NO;

  FormData formData;
  if (!autofill::ExtractFormData(JSONCommand, false, base::string16(), pageURL,
                                 &formData)) {
    return NO;
  }

  std::unique_ptr<PasswordForm> form =
      ParseFormData(formData, password_manager::FormParsingMode::SAVING);
  if (!form)
    return NO;

  if (![self isWebStateDestroyed]) {
    self.passwordManager->OnPasswordFormSubmitted(self.passwordManagerDriver,
                                                  *form);
    return YES;
  }

  return NO;
}

- (PasswordManagerClient*)passwordManagerClient {
  return passwordManagerClient_.get();
}

- (PasswordManagerDriver*)passwordManagerDriver {
  return passwordManagerDriver_.get();
}

- (PasswordManager*)passwordManager {
  return passwordManager_.get();
}

- (JsPasswordManager*)passwordJsManager {
  return passwordJsManager_;
}

#pragma mark - Private methods

- (void)showInfoBarForForm:(std::unique_ptr<PasswordFormManagerForUI>)form
               infoBarType:(PasswordInfoBarType)type {
  if (!webState_)
    return;

  bool isSmartLockBrandingEnabled = false;
  if (self.browserState) {
    syncer::SyncService* sync_service =
        IOSChromeProfileSyncServiceFactory::GetForBrowserState(
            self.browserState);
    isSmartLockBrandingEnabled =
        password_bubble_experiment::IsSmartLockUser(sync_service);
  }
  infobars::InfoBarManager* infoBarManager =
      InfoBarManagerImpl::FromWebState(webState_);

  switch (type) {
    case PasswordInfoBarType::SAVE:
      IOSChromeSavePasswordInfoBarDelegate::Create(
          isSmartLockBrandingEnabled, infoBarManager, std::move(form),
          self.dispatcher);
      break;

    case PasswordInfoBarType::UPDATE:
      IOSChromeUpdatePasswordInfoBarDelegate::Create(
          isSmartLockBrandingEnabled, infoBarManager, std::move(form),
          self.baseViewController, self.dispatcher);
      break;
  }
}

@end
