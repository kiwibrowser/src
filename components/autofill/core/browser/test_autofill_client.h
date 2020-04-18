// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_TEST_AUTOFILL_CLIENT_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_TEST_AUTOFILL_CLIENT_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/compiler_specific.h"
#include "base/i18n/rtl.h"
#include "base/macros.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "components/prefs/pref_service.h"
#include "components/ukm/test_ukm_recorder.h"
#include "services/identity/public/cpp/identity_test_environment.h"

namespace autofill {

// This class is for easier writing of tests.
class TestAutofillClient : public AutofillClient {
 public:
  TestAutofillClient();
  ~TestAutofillClient() override;

  // AutofillClient implementation.
  PersonalDataManager* GetPersonalDataManager() override;
  scoped_refptr<AutofillWebDataService> GetDatabase() override;
  PrefService* GetPrefs() override;
  syncer::SyncService* GetSyncService() override;
  identity::IdentityManager* GetIdentityManager() override;
  ukm::UkmRecorder* GetUkmRecorder() override;
  AddressNormalizer* GetAddressNormalizer() override;
  void ShowAutofillSettings() override;
  void ShowUnmaskPrompt(const CreditCard& card,
                        UnmaskCardReason reason,
                        base::WeakPtr<CardUnmaskDelegate> delegate) override;
  void OnUnmaskVerificationResult(PaymentsRpcResult result) override;
  void ConfirmSaveCreditCardLocally(const CreditCard& card,
                                    const base::Closure& callback) override;
  void ConfirmSaveCreditCardToCloud(
      const CreditCard& card,
      std::unique_ptr<base::DictionaryValue> legal_message,
      const base::Closure& callback) override;
  void ConfirmCreditCardFillAssist(const CreditCard& card,
                                   const base::Closure& callback) override;
  void LoadRiskData(
      const base::Callback<void(const std::string&)>& callback) override;
  bool HasCreditCardScanFeature() override;
  void ScanCreditCard(const CreditCardScanCallback& callback) override;
  void ShowAutofillPopup(
      const gfx::RectF& element_bounds,
      base::i18n::TextDirection text_direction,
      const std::vector<Suggestion>& suggestions,
      base::WeakPtr<AutofillPopupDelegate> delegate) override;
  void UpdateAutofillPopupDataListValues(
      const std::vector<base::string16>& values,
      const std::vector<base::string16>& labels) override;
  void HideAutofillPopup() override;
  bool IsAutocompleteEnabled() override;
  void PropagateAutofillPredictions(
      content::RenderFrameHost* rfh,
      const std::vector<autofill::FormStructure*>& forms) override;
  void DidFillOrPreviewField(const base::string16& autofilled_value,
                             const base::string16& profile_full_name) override;
  void DidInteractWithNonsecureCreditCardInput() override;
  // By default, TestAutofillClient will report that the context is
  // secure. This can be adjusted by calling set_form_origin() with an
  // http:// URL.
  bool IsContextSecure() override;
  bool ShouldShowSigninPromo() override;
  bool IsAutofillSupported() override;
  bool AreServerCardsSupported() override;
  void ExecuteCommand(int id) override;

  void SetPrefs(std::unique_ptr<PrefService> prefs) {
    prefs_ = std::move(prefs);
  }

  void set_form_origin(const GURL& url) { form_origin_ = url; }

  void set_sync_service(syncer::SyncService* test_sync_service) {
    test_sync_service_ = test_sync_service;
  }

 private:
  identity::IdentityTestEnvironment identity_test_env_;
  syncer::SyncService* test_sync_service_ = nullptr;

  // NULL by default.
  std::unique_ptr<PrefService> prefs_;
  GURL form_origin_;

  DISALLOW_COPY_AND_ASSIGN(TestAutofillClient);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_TEST_AUTOFILL_CLIENT_H_
