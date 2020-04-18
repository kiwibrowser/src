// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_CREDIT_CARD_SAVE_MANAGER_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_CREDIT_CARD_SAVE_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/strings/string16.h"
#include "build/build_config.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "components/autofill/core/browser/autofill_metrics.h"
#include "components/autofill/core/browser/form_structure.h"
#include "components/autofill/core/browser/payments/payments_client.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "url/origin.h"

namespace autofill {

// Manages logic for determining whether upload credit card save to Google
// Payments is available as well as actioning both local and upload credit card
// save logic.  Owned by FormDataImporter.
class CreditCardSaveManager : public payments::PaymentsClientSaveDelegate {
 public:
  // Possible fields and values detected during credit card form submission, to
  // be sent to Google Payments to better determine if upload credit card save
  // should be offered.  These should stay consistent with the equivalent enum
  // in Google Payments code.
  enum DetectedValue {
    // Set if a valid CVC was detected.  Will always be set if the CVC fix flow
    // is enabled.
    CVC = 1 << 0,
    // Set if a cardholder name was found, *unless* conflicting names were
    // found.
    CARDHOLDER_NAME = 1 << 1,
    // Set if an address name was found, *unless* conflicting names were found.
    ADDRESS_NAME = 1 << 2,
    // Set if an address line was found in any address (regardless of
    // conflicts).
    ADDRESS_LINE = 1 << 3,
    // Set if a locality was found in any address (regardless of conflicts).
    LOCALITY = 1 << 4,
    // Set if an administrative area was found in any address (regardless of
    // conflicts).
    ADMINISTRATIVE_AREA = 1 << 5,
    // Set if a postal code was found in any address, *unless* conflicting
    // postal codes were found.
    POSTAL_CODE = 1 << 6,
    // Set if a country code was found in any address (regardless of conflicts).
    COUNTRY_CODE = 1 << 7,
    // Set if the user is already syncing data from a Google Payments account.
    HAS_GOOGLE_PAYMENTS_ACCOUNT = 1 << 8,
  };

  // An observer class used by browsertests that gets notified whenever
  // particular actions occur.
  class ObserverForTest {
   public:
    virtual void OnOfferLocalSave() = 0;
    virtual void OnDecideToRequestUploadSave() = 0;
    virtual void OnDecideToNotRequestUploadSave() = 0;
    virtual void OnReceivedGetUploadDetailsResponse() = 0;
    virtual void OnSentUploadCardRequest() = 0;
  };

  // The parameters should outlive the CreditCardSaveManager.
  CreditCardSaveManager(AutofillClient* client,
                        payments::PaymentsClient* payments_client,
                        const std::string& app_locale,
                        PersonalDataManager* personal_data_manager);
  virtual ~CreditCardSaveManager();

  // Offers local credit card save to the user.
  void OfferCardLocalSave(const CreditCard& card);

  // Begins the process to offer upload credit card save to the user if the
  // imported card passes all requirements and Google Payments approves.
  void AttemptToOfferCardUploadSave(const FormStructure& submitted_form,
                                    const CreditCard& card,
                                    const bool uploading_local_card);

  // Returns true if all the conditions for enabling the upload of credit card
  // are satisfied.
  virtual bool IsCreditCardUploadEnabled();

  // For testing.
  void SetAppLocale(std::string app_locale) { app_locale_ = app_locale; }

 protected:
  // payments::PaymentsClientSaveDelegate:
  // Exposed for testing.
  void OnDidUploadCard(AutofillClient::PaymentsRpcResult result,
                       const std::string& server_id) override;

 private:
  friend class SaveCardBubbleViewsBrowserTestBase;

  // payments::PaymentsClientSaveDelegate:
  void OnDidGetUploadDetails(
      AutofillClient::PaymentsRpcResult result,
      const base::string16& context_token,
      std::unique_ptr<base::DictionaryValue> legal_message) override;

  // Examines |card| and the stored profiles and if a candidate set of profiles
  // is found that matches the client-side validation rules, assigns the values
  // to |upload_request.profiles|. If any problems are found when determining
  // the candidate set of profiles, sets |upload_decision_metrics_| with the
  // failure reasons. Appends any experiments that were triggered to
  // |upload_request.active_experiments|.
  void SetProfilesForCreditCardUpload(
      const CreditCard& card,
      payments::PaymentsClient::UploadRequestDetails* upload_request);

  // Analyzes the decisions made while importing address profile and credit card
  // data in preparation for upload credit card save, in order to determine what
  // uploadable data is actually available.
  int GetDetectedValues() const;

  // Sets |user_did_accept_upload_prompt_| and calls SendUploadCardRequest if
  // the risk data is available.
  void OnUserDidAcceptUpload();

  // Saves risk data in |uploading_risk_data_| and calls SendUploadCardRequest
  // if the user has accepted the prompt.
  void OnDidGetUploadRiskData(const std::string& risk_data);

  // Finalizes the upload request and calls PaymentsClient::UploadCard().
  void SendUploadCardRequest();

  // Returns metric relevant to the CVC field based on values in
  // |found_cvc_field_|, |found_value_in_cvc_field_| and
  // |found_cvc_value_in_non_cvc_field_|. Only called when a valid CVC was NOT
  // found.
  AutofillMetrics::CardUploadDecisionMetric GetCVCCardUploadDecisionMetric()
      const;

  // Logs the card upload decisions in UKM and UMA.
  // |upload_decision_metrics| is a bitmask of
  // |AutofillMetrics::CardUploadDecisionMetric|.
  void LogCardUploadDecisions(int upload_decision_metrics);

  // For testing.
  void SetEventObserverForTesting(ObserverForTest* observer) {
    observer_for_testing_ = observer;
  }

  AutofillClient* const client_;

  // Handles Payments service requests.
  // Owned by AutofillManager.
  payments::PaymentsClient* payments_client_;

  std::string app_locale_;

  // The personal data manager, used to save and load personal data to/from the
  // web database.  This is overridden by the AutofillManagerTest.
  // Weak reference.
  // May be NULL.  NULL indicates OTR.
  PersonalDataManager* personal_data_manager_;

  // Collected information about a pending upload request.
  payments::PaymentsClient::UploadRequestDetails upload_request_;

  // A bitmask of |AutofillMetrics::CardUploadDecisionMetric| representing the
  // decisions made when determining if credit card upload save should be
  // offered.
  int upload_decision_metrics_ = 0;

  // |true| if the card being offered for upload is already a local card on the
  // device; |false| otherwise.
  bool uploading_local_card_ = false;

  // |true| if the user has opted to upload save their credit card to Google.
  bool user_did_accept_upload_prompt_ = false;

  // |found_cvc_field_| is |true| if there exists a field that is determined to
  // be a CVC field via heuristics.
  bool found_cvc_field_ = false;
  // |found_value_in_cvc_field_| is |true| if a field that is determined to
  // be a CVC field via heuristics has non-empty |value|.
  // |value| may or may not be a valid CVC.
  bool found_value_in_cvc_field_ = false;
  // |found_cvc_value_in_non_cvc_field_| is |true| if a field that is not
  // determined to be a CVC field via heuristics has a valid CVC |value|.
  bool found_cvc_value_in_non_cvc_field_ = false;

  // The origin of the top level frame from which a form is uploaded.
  url::Origin pending_upload_request_origin_;

  // May be null.
  ObserverForTest* observer_for_testing_ = nullptr;

  base::WeakPtrFactory<CreditCardSaveManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CreditCardSaveManager);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_CREDIT_CARD_SAVE_MANAGER_H_
