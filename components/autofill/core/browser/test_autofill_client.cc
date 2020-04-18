// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/test_autofill_client.h"

#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"

namespace autofill {

TestAutofillClient::TestAutofillClient()
    : form_origin_(GURL("https://example.test")) {}

TestAutofillClient::~TestAutofillClient() {
}

PersonalDataManager* TestAutofillClient::GetPersonalDataManager() {
  return nullptr;
}

scoped_refptr<AutofillWebDataService> TestAutofillClient::GetDatabase() {
  return scoped_refptr<AutofillWebDataService>(nullptr);
}

PrefService* TestAutofillClient::GetPrefs() {
  return prefs_.get();
}

syncer::SyncService* TestAutofillClient::GetSyncService() {
  return test_sync_service_;
}

identity::IdentityManager* TestAutofillClient::GetIdentityManager() {
  return identity_test_env_.identity_manager();
}

ukm::UkmRecorder* TestAutofillClient::GetUkmRecorder() {
  return ukm::UkmRecorder::Get();
}

AddressNormalizer* TestAutofillClient::GetAddressNormalizer() {
  // TODO(crbug.com/788432): Should use a TestAddressNormalizer.
  return nullptr;
}

void TestAutofillClient::ShowAutofillSettings() {
}

void TestAutofillClient::ShowUnmaskPrompt(
    const CreditCard& card,
    UnmaskCardReason reason,
    base::WeakPtr<CardUnmaskDelegate> delegate) {
}

void TestAutofillClient::OnUnmaskVerificationResult(PaymentsRpcResult result) {
}

void TestAutofillClient::ConfirmSaveCreditCardLocally(
    const CreditCard& card,
    const base::Closure& callback) {
}

void TestAutofillClient::ConfirmSaveCreditCardToCloud(
    const CreditCard& card,
    std::unique_ptr<base::DictionaryValue> legal_message,
    const base::Closure& callback) {
  callback.Run();
}

void TestAutofillClient::ConfirmCreditCardFillAssist(
    const CreditCard& card,
    const base::Closure& callback) {
  callback.Run();
}

void TestAutofillClient::LoadRiskData(
    const base::Callback<void(const std::string&)>& callback) {
  callback.Run("some risk data");
}

bool TestAutofillClient::HasCreditCardScanFeature() {
  return false;
}

void TestAutofillClient::ScanCreditCard(
    const CreditCardScanCallback& callback) {
}

void TestAutofillClient::ShowAutofillPopup(
    const gfx::RectF& element_bounds,
    base::i18n::TextDirection text_direction,
    const std::vector<Suggestion>& suggestions,
    base::WeakPtr<AutofillPopupDelegate> delegate) {
}

void TestAutofillClient::UpdateAutofillPopupDataListValues(
    const std::vector<base::string16>& values,
    const std::vector<base::string16>& labels) {
}

void TestAutofillClient::HideAutofillPopup() {
}

bool TestAutofillClient::IsAutocompleteEnabled() {
  return true;
}

void TestAutofillClient::PropagateAutofillPredictions(
    content::RenderFrameHost* rfh,
    const std::vector<FormStructure*>& forms) {
}

void TestAutofillClient::DidFillOrPreviewField(
    const base::string16& autofilled_value,
    const base::string16& profile_full_name) {
}

void TestAutofillClient::DidInteractWithNonsecureCreditCardInput() {}

bool TestAutofillClient::IsContextSecure() {
  // Simplified secure context check for tests.
  return form_origin_.SchemeIs("https");
}

bool TestAutofillClient::ShouldShowSigninPromo() {
  return false;
}

void TestAutofillClient::ExecuteCommand(int id) {}

bool TestAutofillClient::IsAutofillSupported() {
  return true;
}

bool TestAutofillClient::AreServerCardsSupported() {
  return true;
}

}  // namespace autofill
