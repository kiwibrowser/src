// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/autofill/web_view_autofill_client_ios.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/autofill/ios/browser/autofill_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace autofill {

WebViewAutofillClientIOS::WebViewAutofillClientIOS(
    PrefService* pref_service,
    PersonalDataManager* personal_data_manager,
    web::WebState* web_state,
    id<CWVAutofillClientIOSBridge> bridge,
    identity::IdentityManager* identity_manager,
    scoped_refptr<AutofillWebDataService> autofill_web_data_service)
    : pref_service_(pref_service),
      personal_data_manager_(personal_data_manager),
      web_state_(web_state),
      bridge_(bridge),
      identity_manager_(identity_manager),
      autofill_web_data_service_(autofill_web_data_service) {}

WebViewAutofillClientIOS::~WebViewAutofillClientIOS() {
  HideAutofillPopup();
}

PersonalDataManager* WebViewAutofillClientIOS::GetPersonalDataManager() {
  return personal_data_manager_;
}

PrefService* WebViewAutofillClientIOS::GetPrefs() {
  return pref_service_;
}

// TODO(crbug.com/535784): Implement this when adding credit card upload.
syncer::SyncService* WebViewAutofillClientIOS::GetSyncService() {
  return nullptr;
}

identity::IdentityManager* WebViewAutofillClientIOS::GetIdentityManager() {
  return identity_manager_;
}

ukm::UkmRecorder* WebViewAutofillClientIOS::GetUkmRecorder() {
  return nullptr;
}

AddressNormalizer* WebViewAutofillClientIOS::GetAddressNormalizer() {
  return nullptr;
}

void WebViewAutofillClientIOS::ShowAutofillSettings() {
  NOTREACHED();
}

void WebViewAutofillClientIOS::ShowUnmaskPrompt(
    const CreditCard& card,
    UnmaskCardReason reason,
    base::WeakPtr<CardUnmaskDelegate> delegate) {
  [bridge_ showUnmaskPromptForCard:card reason:reason delegate:delegate];
}

void WebViewAutofillClientIOS::OnUnmaskVerificationResult(
    PaymentsRpcResult result) {
  [bridge_ didReceiveUnmaskVerificationResult:result];
}

void WebViewAutofillClientIOS::ConfirmSaveCreditCardLocally(
    const CreditCard& card,
    const base::RepeatingClosure& callback) {
  [bridge_ confirmSaveCreditCardLocally:card callback:callback];
}

void WebViewAutofillClientIOS::ConfirmSaveCreditCardToCloud(
    const CreditCard& card,
    std::unique_ptr<base::DictionaryValue> legal_message,
    const base::Closure& callback) {}

void WebViewAutofillClientIOS::ConfirmCreditCardFillAssist(
    const CreditCard& card,
    const base::Closure& callback) {}

void WebViewAutofillClientIOS::LoadRiskData(
    const base::Callback<void(const std::string&)>& callback) {}

bool WebViewAutofillClientIOS::HasCreditCardScanFeature() {
  return false;
}

void WebViewAutofillClientIOS::ScanCreditCard(
    const CreditCardScanCallback& callback) {
  NOTREACHED();
}

void WebViewAutofillClientIOS::ShowAutofillPopup(
    const gfx::RectF& element_bounds,
    base::i18n::TextDirection text_direction,
    const std::vector<Suggestion>& suggestions,
    base::WeakPtr<AutofillPopupDelegate> delegate) {
  [bridge_ showAutofillPopup:suggestions popupDelegate:delegate];
}

void WebViewAutofillClientIOS::HideAutofillPopup() {
  [bridge_ hideAutofillPopup];
}

bool WebViewAutofillClientIOS::IsAutocompleteEnabled() {
  // For browser, Autocomplete is always enabled as part of Autofill.
  return GetPrefs()->GetBoolean(prefs::kAutofillEnabled);
}

void WebViewAutofillClientIOS::UpdateAutofillPopupDataListValues(
    const std::vector<base::string16>& values,
    const std::vector<base::string16>& labels) {
  NOTREACHED();
}

void WebViewAutofillClientIOS::PropagateAutofillPredictions(
    content::RenderFrameHost* rfh,
    const std::vector<FormStructure*>& forms) {}

void WebViewAutofillClientIOS::DidFillOrPreviewField(
    const base::string16& autofilled_value,
    const base::string16& profile_full_name) {}

scoped_refptr<AutofillWebDataService> WebViewAutofillClientIOS::GetDatabase() {
  return autofill_web_data_service_;
}

void WebViewAutofillClientIOS::DidInteractWithNonsecureCreditCardInput() {}

bool WebViewAutofillClientIOS::IsContextSecure() {
  return IsContextSecureForWebState(web_state_);
}

bool WebViewAutofillClientIOS::ShouldShowSigninPromo() {
  return false;
}

void WebViewAutofillClientIOS::ExecuteCommand(int id) {
  NOTIMPLEMENTED();
}

bool WebViewAutofillClientIOS::IsAutofillSupported() {
  return true;
}

bool WebViewAutofillClientIOS::AreServerCardsSupported() {
  return true;
}

}  // namespace autofill
