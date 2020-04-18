// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/credit_card_save_manager.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string16.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "components/autofill/core/browser/autofill_experiments.h"
#include "components/autofill/core/browser/autofill_metrics.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/autofill_type.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/form_structure.h"
#include "components/autofill/core/browser/payments/payments_client.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/browser/validation.h"
#include "components/autofill/core/common/autofill_clock.h"
#include "components/autofill/core/common/autofill_util.h"
#include "components/prefs/pref_service.h"
#include "services/identity/public/cpp/identity_manager.h"
#include "url/gurl.h"

namespace autofill {

namespace {

// If |name| consists of three whitespace-separated parts and the second of the
// three parts is a single character or a single character followed by a period,
// returns the result of joining the first and third parts with a space.
// Otherwise, returns |name|.
//
// Note that a better way to do this would be to use SplitName from
// src/components/autofill/core/browser/contact_info.cc. However, for now we
// want the logic of which variations of names are considered to be the same to
// exactly match the logic applied on the Payments server.
base::string16 RemoveMiddleInitial(const base::string16& name) {
  std::vector<base::StringPiece16> parts =
      base::SplitStringPiece(name, base::kWhitespaceUTF16,
                             base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (parts.size() == 3 && (parts[1].length() == 1 ||
                            (parts[1].length() == 2 &&
                             base::EndsWith(parts[1], base::ASCIIToUTF16("."),
                                            base::CompareCase::SENSITIVE)))) {
    parts.erase(parts.begin() + 1);
    return base::JoinString(parts, base::ASCIIToUTF16(" "));
  }
  return name;
}

}  // namespace

CreditCardSaveManager::CreditCardSaveManager(
    AutofillClient* client,
    payments::PaymentsClient* payments_client,
    const std::string& app_locale,
    PersonalDataManager* personal_data_manager)
    : client_(client),
      payments_client_(payments_client),
      app_locale_(app_locale),
      personal_data_manager_(personal_data_manager),
      weak_ptr_factory_(this) {
  if (payments_client_) {
    payments_client_->SetSaveDelegate(this);
  }
}

CreditCardSaveManager::~CreditCardSaveManager() {}

void CreditCardSaveManager::OfferCardLocalSave(const CreditCard& card) {
  if (observer_for_testing_)
    observer_for_testing_->OnOfferLocalSave();
  client_->ConfirmSaveCreditCardLocally(
      card, base::Bind(base::IgnoreResult(
                           &PersonalDataManager::SaveImportedCreditCard),
                       base::Unretained(personal_data_manager_), card));
}

void CreditCardSaveManager::AttemptToOfferCardUploadSave(
    const FormStructure& submitted_form,
    const CreditCard& card,
    const bool uploading_local_card) {
  upload_request_ = payments::PaymentsClient::UploadRequestDetails();
  upload_request_.card = card;
  uploading_local_card_ = uploading_local_card;

  // Ideally, in order to upload a card, we must have both:
  //  1) Card with CVC
  //  2) 1+ recently-used or modified addresses that meet the client-side
  //     validation rules
  // We perform all checks before returning or logging in order to know where we
  // stand with regards to card upload information. If the "send detected
  // values" experiment is disabled and any problems were found, we do not offer
  // to save the card. We could fall back to a local save, but we believe that
  // sometimes offering upload and sometimes offering local save is a confusing
  // user experience.

  // Alternatively, if the "send detected values" experiment is enabled, always
  // ping Google Payments regardless and ask if save is allowed. Include what
  // data was found as part of the request, and let Payments decide whether
  // upload save should be offered.
  found_cvc_field_ = false;
  found_value_in_cvc_field_ = false;
  found_cvc_value_in_non_cvc_field_ = false;

  for (const auto& field : submitted_form) {
    const bool is_valid_cvc = IsValidCreditCardSecurityCode(
        field->value, upload_request_.card.network());
    if (field->Type().GetStorableType() == CREDIT_CARD_VERIFICATION_CODE) {
      found_cvc_field_ = true;
      if (!field->value.empty())
        found_value_in_cvc_field_ = true;
      if (is_valid_cvc) {
        upload_request_.cvc = field->value;
        break;
      }
    } else if (is_valid_cvc &&
               field->Type().GetStorableType() == UNKNOWN_TYPE) {
      found_cvc_value_in_non_cvc_field_ = true;
    }
  }

  // Upload requires that recently used or modified addresses meet the
  // client-side validation rules. This call also begins setting the value of
  // |upload_decision_metrics_|.
  SetProfilesForCreditCardUpload(card, &upload_request_);

  pending_upload_request_origin_ = submitted_form.main_frame_origin();

  if (upload_request_.cvc.empty()) {
    // Apply the CVC decision to |upload_decision_metrics_| to denote a problem
    // was found.
    upload_decision_metrics_ |= GetCVCCardUploadDecisionMetric();
  }

  // If any problems were found across CVC/name/address,
  // |upload_decision_metrics_| will be non-zero. If the "send detected values"
  // experiment is on, continue anyway and just let Payments know that not
  // everything was found, as Payments may still allow the card to be saved.
  // If the experiment is off, follow the legacy logic of aborting upload save.
  if (upload_decision_metrics_ &&
      !IsAutofillUpstreamSendDetectedValuesExperimentEnabled()) {
    LogCardUploadDecisions(upload_decision_metrics_);
    pending_upload_request_origin_ = url::Origin();
    if (observer_for_testing_)
      observer_for_testing_->OnDecideToNotRequestUploadSave();
    return;
  }

  // Add active experiments to the request payload.
  if (IsAutofillUpstreamSendDetectedValuesExperimentEnabled()) {
    upload_request_.active_experiments.push_back(
        kAutofillUpstreamSendDetectedValues.name);
  }
  if (IsAutofillUpstreamSendPanFirstSixExperimentEnabled()) {
    upload_request_.active_experiments.push_back(
        kAutofillUpstreamSendPanFirstSix.name);
  }
  if (IsAutofillUpstreamUpdatePromptExplanationExperimentEnabled()) {
    upload_request_.active_experiments.push_back(
        kAutofillUpstreamUpdatePromptExplanation.name);
  }

  // All required data is available, start the upload process.
  if (observer_for_testing_)
    observer_for_testing_->OnDecideToRequestUploadSave();
  payments_client_->GetUploadDetails(
      upload_request_.profiles, GetDetectedValues(),
      base::UTF16ToASCII(CreditCard::StripSeparators(card.number()))
          .substr(0, 6),
      upload_request_.active_experiments, app_locale_);
}

bool CreditCardSaveManager::IsCreditCardUploadEnabled() {
  // If observer_for_testing_ is set, assume we are in a browsertest and
  // credit card upload should be enabled by default.
  return observer_for_testing_ ||
         ::autofill::IsCreditCardUploadEnabled(
             client_->GetPrefs(), client_->GetSyncService(),
             client_->GetIdentityManager()->GetPrimaryAccountInfo().email);
}

void CreditCardSaveManager::OnDidUploadCard(
    AutofillClient::PaymentsRpcResult result,
    const std::string& server_id) {
  // We don't do anything user-visible if the upload attempt fails. If the
  // upload succeeds and we can store unmasked cards on this OS, we will keep a
  // copy of the card as a full server card on the device.
  if (result == AutofillClient::SUCCESS && !server_id.empty() &&
      OfferStoreUnmaskedCards()) {
    upload_request_.card.set_record_type(CreditCard::FULL_SERVER_CARD);
    upload_request_.card.SetServerStatus(CreditCard::OK);
    upload_request_.card.set_server_id(server_id);
    DCHECK(personal_data_manager_);
    if (personal_data_manager_)
      personal_data_manager_->AddFullServerCreditCard(upload_request_.card);
  }
}

void CreditCardSaveManager::OnDidGetUploadDetails(
    AutofillClient::PaymentsRpcResult result,
    const base::string16& context_token,
    std::unique_ptr<base::DictionaryValue> legal_message) {
  if (observer_for_testing_)
    observer_for_testing_->OnReceivedGetUploadDetailsResponse();
  if (result == AutofillClient::SUCCESS) {
    // Do *not* call payments_client_->Prepare() here. We shouldn't send
    // credentials until the user has explicitly accepted a prompt to upload.
    upload_request_.context_token = context_token;
    user_did_accept_upload_prompt_ = false;
    client_->ConfirmSaveCreditCardToCloud(
        upload_request_.card, std::move(legal_message),
        base::Bind(&CreditCardSaveManager::OnUserDidAcceptUpload,
                   weak_ptr_factory_.GetWeakPtr()));
    client_->LoadRiskData(
        base::Bind(&CreditCardSaveManager::OnDidGetUploadRiskData,
                   weak_ptr_factory_.GetWeakPtr()));
    upload_decision_metrics_ |= AutofillMetrics::UPLOAD_OFFERED;
    AutofillMetrics::LogUploadOfferedCardOriginMetric(
        uploading_local_card_ ? AutofillMetrics::OFFERING_UPLOAD_OF_LOCAL_CARD
                              : AutofillMetrics::OFFERING_UPLOAD_OF_NEW_CARD);
  } else {
    // If the upload details request failed, fall back to a local save. The
    // reasoning here is as follows:
    // - This will sometimes fail intermittently, in which case it might be
    // better to not fall back, because sometimes offering upload and sometimes
    // offering local save is a poor user experience.
    // - However, in some cases, our local configuration limiting the feature to
    // countries that Payments is known to support will not match Payments' own
    // determination of what country the user is located in. In these cases,
    // the upload details request will consistently fail and if we don't fall
    // back to a local save then the user will never be offered any kind of
    // credit card save.

    // Additional note: If the "send detected values" experiment is enabled,
    // only offer to save locally if CVC + name + address were all found, as
    // this signifies a legacy decision of "Payments doesn't want this card".
    // We can revisit this decision in the future, but the reasoning here is as
    // follows:
    // - If any of them were not found, surfacing local save would begin to
    // create a scenario where different card types or different checkout forms
    // could reasonably surface different save dialogs to the user, and that
    // would cause unnecessary confusion.
    // - We already don't offer to save at all in these cases today, so this
    // decision doesn't disable any upload chances, it just enables *less*
    // upload chances.
    int detected_values = GetDetectedValues();
    bool found_name_and_postal_code_and_cvc =
        (detected_values & DetectedValue::CARDHOLDER_NAME ||
         detected_values & DetectedValue::ADDRESS_NAME) &&
        detected_values & DetectedValue::POSTAL_CODE &&
        detected_values & DetectedValue::CVC;
    if (!IsAutofillUpstreamSendDetectedValuesExperimentEnabled() ||
        found_name_and_postal_code_and_cvc) {
      if (observer_for_testing_)
        observer_for_testing_->OnOfferLocalSave();
      client_->ConfirmSaveCreditCardLocally(
          upload_request_.card,
          base::BindRepeating(
              base::IgnoreResult(&PersonalDataManager::SaveImportedCreditCard),
              base::Unretained(personal_data_manager_), upload_request_.card));
    }
    upload_decision_metrics_ |=
        AutofillMetrics::UPLOAD_NOT_OFFERED_GET_UPLOAD_DETAILS_FAILED;
  }

  // Assert that we've either detected the CVC or the "send detected values"
  // experiment is enabled.
  DCHECK(IsAutofillUpstreamSendDetectedValuesExperimentEnabled() ||
         (found_cvc_field_ && found_value_in_cvc_field_));

  LogCardUploadDecisions(upload_decision_metrics_);
  pending_upload_request_origin_ = url::Origin();
}

void CreditCardSaveManager::SetProfilesForCreditCardUpload(
    const CreditCard& card,
    payments::PaymentsClient::UploadRequestDetails* upload_request) {
  std::vector<AutofillProfile> candidate_profiles;
  const base::Time now = AutofillClock::Now();
  const base::TimeDelta fifteen_minutes = base::TimeDelta::FromMinutes(15);
  // Reset |upload_decision_metrics_| to begin logging detected problems.
  upload_decision_metrics_ = 0;
  bool has_profile = false;
  bool has_modified_profile = false;

  // First, collect all of the addresses used or modified recently.
  for (AutofillProfile* profile : personal_data_manager_->GetProfiles()) {
    has_profile = true;
    if ((now - profile->modification_date()) < fifteen_minutes) {
      has_modified_profile = true;
      candidate_profiles.push_back(*profile);
    } else if ((now - profile->use_date()) < fifteen_minutes) {
      candidate_profiles.push_back(*profile);
    }
  }

  AutofillMetrics::LogHasModifiedProfileOnCreditCardFormSubmission(
      has_modified_profile);

  if (candidate_profiles.empty()) {
    upload_decision_metrics_ |=
        has_profile
            ? AutofillMetrics::UPLOAD_NOT_OFFERED_NO_RECENTLY_USED_ADDRESS
            : AutofillMetrics::UPLOAD_NOT_OFFERED_NO_ADDRESS_PROFILE;
  }

  // If any of the names on the card or the addresses don't match the
  // candidate set is invalid. This matches the rules for name matching applied
  // server-side by Google Payments and ensures that we don't send upload
  // requests that are guaranteed to fail.
  const base::string16 card_name =
      card.GetInfo(AutofillType(CREDIT_CARD_NAME_FULL), app_locale_);
  base::string16 verified_name;
  if (candidate_profiles.empty()) {
    verified_name = card_name;
  } else {
    bool found_conflicting_names = false;
    verified_name = RemoveMiddleInitial(card_name);
    for (const AutofillProfile& profile : candidate_profiles) {
      const base::string16 address_name =
          RemoveMiddleInitial(profile.GetInfo(NAME_FULL, app_locale_));
      if (address_name.empty())
        continue;
      if (verified_name.empty()) {
        verified_name = address_name;
      } else if (!base::EqualsCaseInsensitiveASCII(verified_name,
                                                   address_name)) {
        found_conflicting_names = true;
        break;
      }
    }
    if (found_conflicting_names) {
      upload_decision_metrics_ |=
          AutofillMetrics::UPLOAD_NOT_OFFERED_CONFLICTING_NAMES;
    }
  }

  // If neither the card nor any of the addresses have a name associated with
  // them, the candidate set is invalid.
  if (verified_name.empty()) {
    upload_decision_metrics_ |= AutofillMetrics::UPLOAD_NOT_OFFERED_NO_NAME;
  }

  // If any of the candidate addresses have a non-empty zip that doesn't match
  // any other non-empty zip, then the candidate set is invalid.
  base::string16 verified_zip;
  const AutofillType kZipCode(ADDRESS_HOME_ZIP);
  for (const AutofillProfile& profile : candidate_profiles) {
    const base::string16 zip = profile.GetRawInfo(ADDRESS_HOME_ZIP);
    if (!zip.empty()) {
      if (verified_zip.empty()) {
        verified_zip = zip;
      } else {
        // To compare two zips, we check to see if either is a prefix of the
        // other. This allows us to consider a 5-digit zip and a zip+4 to be a
        // match if the first 5 digits are the same without hardcoding any
        // specifics of how postal codes are represented. (They can be numeric
        // or alphanumeric and vary from 3 to 10 digits long by country. See
        // https://en.wikipedia.org/wiki/Postal_code#Presentation.) The Payments
        // backend will apply a more sophisticated address-matching procedure.
        // This check is simply meant to avoid offering upload in cases that are
        // likely to fail.
        if (!(StartsWith(verified_zip, zip, base::CompareCase::SENSITIVE) ||
              StartsWith(zip, verified_zip, base::CompareCase::SENSITIVE))) {
          upload_decision_metrics_ |=
              AutofillMetrics::UPLOAD_NOT_OFFERED_CONFLICTING_ZIPS;
          break;
        }
      }
    }
  }

  // If none of the candidate addresses have a zip, the candidate set is
  // invalid.
  if (verified_zip.empty() && !candidate_profiles.empty())
    upload_decision_metrics_ |= AutofillMetrics::UPLOAD_NOT_OFFERED_NO_ZIP_CODE;

  // Only set |upload_request->profiles| if upload is allowed (either all
  // required data was found, or "send detected values" experiment is enabled).
  if (!upload_decision_metrics_ ||
      IsAutofillUpstreamSendDetectedValuesExperimentEnabled()) {
    upload_request->profiles.assign(candidate_profiles.begin(),
                                    candidate_profiles.end());
    if (!has_modified_profile)
      for (const AutofillProfile& profile : candidate_profiles)
        UMA_HISTOGRAM_COUNTS_1000(
            "Autofill.DaysSincePreviousUseAtSubmission.Profile",
            (profile.use_date() - profile.previous_use_date()).InDays());
  }
}

int CreditCardSaveManager::GetDetectedValues() const {
  int detected_values = 0;

  // Report detecting CVC if it was found.
  if (!upload_request_.cvc.empty()) {
    detected_values |= DetectedValue::CVC;
  }

  // If cardholder name exists, set it as detected as long as
  // UPLOAD_NOT_OFFERED_CONFLICTING_NAMES was not set.
  if (!upload_request_.card
           .GetInfo(AutofillType(CREDIT_CARD_NAME_FULL), app_locale_)
           .empty() &&
      !(upload_decision_metrics_ &
        AutofillMetrics::UPLOAD_NOT_OFFERED_CONFLICTING_NAMES)) {
    detected_values |= DetectedValue::CARDHOLDER_NAME;
  }

  // Go through the upload request's profiles and set the following as detected:
  //  - ADDRESS_NAME, as long as UPLOAD_NOT_OFFERED_CONFLICTING_NAMES was not
  //    set
  //  - POSTAL_CODE, as long as UPLOAD_NOT_OFFERED_CONFLICTING_ZIPS was not set
  //  - Any other address fields found on any addresses, regardless of conflicts
  for (const AutofillProfile& profile : upload_request_.profiles) {
    if (!profile.GetInfo(NAME_FULL, app_locale_).empty() &&
        !(upload_decision_metrics_ &
          AutofillMetrics::UPLOAD_NOT_OFFERED_CONFLICTING_NAMES)) {
      detected_values |= DetectedValue::ADDRESS_NAME;
    }
    if (!profile.GetInfo(ADDRESS_HOME_ZIP, app_locale_).empty() &&
        !(upload_decision_metrics_ &
          AutofillMetrics::UPLOAD_NOT_OFFERED_CONFLICTING_ZIPS)) {
      detected_values |= DetectedValue::POSTAL_CODE;
    }
    if (!profile.GetInfo(ADDRESS_HOME_LINE1, app_locale_).empty()) {
      detected_values |= DetectedValue::ADDRESS_LINE;
    }
    if (!profile.GetInfo(ADDRESS_HOME_CITY, app_locale_).empty()) {
      detected_values |= DetectedValue::LOCALITY;
    }
    if (!profile.GetInfo(ADDRESS_HOME_STATE, app_locale_).empty()) {
      detected_values |= DetectedValue::ADMINISTRATIVE_AREA;
    }
    if (!profile.GetRawInfo(ADDRESS_HOME_COUNTRY).empty()) {
      detected_values |= DetectedValue::COUNTRY_CODE;
    }
  }

  // If the billing_customer_number Priority Preference is non-zero, it means
  // the user has a Google Payments account. Include a bit for existence of this
  // account (NOT the id itself), as it will help determine if a new Payments
  // customer might need to be created when save is accepted.
  if (static_cast<int64_t>(payments_client_->GetPrefService()->GetDouble(
          prefs::kAutofillBillingCustomerNumber)) != 0)
    detected_values |= DetectedValue::HAS_GOOGLE_PAYMENTS_ACCOUNT;

  return detected_values;
}

void CreditCardSaveManager::OnUserDidAcceptUpload() {
  user_did_accept_upload_prompt_ = true;
  // Populating risk data and offering upload occur asynchronously.
  // If |risk_data| has already been loaded, send the upload card request.
  // Otherwise, continue to wait and let OnDidGetUploadRiskData handle it.
  if (!upload_request_.risk_data.empty())
    SendUploadCardRequest();
}

void CreditCardSaveManager::OnDidGetUploadRiskData(
    const std::string& risk_data) {
  upload_request_.risk_data = risk_data;
  // Populating risk data and offering upload occur asynchronously.
  // If the dialog has already been accepted, send the upload card request.
  // Otherwise, continue to wait for the user to accept the save dialog.
  if (user_did_accept_upload_prompt_)
    SendUploadCardRequest();
}

void CreditCardSaveManager::SendUploadCardRequest() {
  if (observer_for_testing_)
    observer_for_testing_->OnSentUploadCardRequest();
  upload_request_.app_locale = app_locale_;
  upload_request_.billing_customer_number =
      static_cast<int64_t>(payments_client_->GetPrefService()->GetDouble(
          prefs::kAutofillBillingCustomerNumber));

  AutofillMetrics::LogUploadAcceptedCardOriginMetric(
      uploading_local_card_
          ? AutofillMetrics::USER_ACCEPTED_UPLOAD_OF_LOCAL_CARD
          : AutofillMetrics::USER_ACCEPTED_UPLOAD_OF_NEW_CARD);
  payments_client_->UploadCard(upload_request_);
}

AutofillMetrics::CardUploadDecisionMetric
CreditCardSaveManager::GetCVCCardUploadDecisionMetric() const {
  // This function assumes a valid CVC was not found.
  if (found_cvc_field_) {
    return found_value_in_cvc_field_ ? AutofillMetrics::INVALID_CVC_VALUE
                                     : AutofillMetrics::CVC_VALUE_NOT_FOUND;
  }
  return found_cvc_value_in_non_cvc_field_
             ? AutofillMetrics::FOUND_POSSIBLE_CVC_VALUE_IN_NON_CVC_FIELD
             : AutofillMetrics::CVC_FIELD_NOT_FOUND;
}

void CreditCardSaveManager::LogCardUploadDecisions(
    int upload_decision_metrics) {
  AutofillMetrics::LogCardUploadDecisionMetrics(upload_decision_metrics);
  AutofillMetrics::LogCardUploadDecisionsUkm(
      client_->GetUkmRecorder(), pending_upload_request_origin_.GetURL(),
      upload_decision_metrics);
}

}  // namespace autofill
