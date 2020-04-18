// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/payment_request_state.h"

#include <algorithm>
#include <set>
#include <utility>

#include "base/feature_list.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_country.h"
#include "components/autofill/core/browser/autofill_data_util.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/payments/content/content_payment_request_delegate.h"
#include "components/payments/content/payment_manifest_web_data_service.h"
#include "components/payments/content/payment_response_helper.h"
#include "components/payments/content/service_worker_payment_instrument.h"
#include "components/payments/core/autofill_payment_instrument.h"
#include "components/payments/core/journey_logger.h"
#include "components/payments/core/payment_instrument.h"
#include "components/payments/core/payment_request_data_util.h"
#include "content/public/common/content_features.h"

namespace payments {

PaymentRequestState::PaymentRequestState(
    content::WebContents* web_contents,
    const GURL& top_level_origin,
    const GURL& frame_origin,
    PaymentRequestSpec* spec,
    Delegate* delegate,
    const std::string& app_locale,
    autofill::PersonalDataManager* personal_data_manager,
    ContentPaymentRequestDelegate* payment_request_delegate,
    JourneyLogger* journey_logger)
    : is_ready_to_pay_(false),
      get_all_instruments_finished_(true),
      is_waiting_for_merchant_validation_(false),
      app_locale_(app_locale),
      spec_(spec),
      delegate_(delegate),
      personal_data_manager_(personal_data_manager),
      journey_logger_(journey_logger),
      selected_shipping_profile_(nullptr),
      selected_shipping_option_error_profile_(nullptr),
      selected_contact_profile_(nullptr),
      selected_instrument_(nullptr),
      number_of_pending_sw_payment_instruments_(0),
      payment_request_delegate_(payment_request_delegate),
      profile_comparator_(app_locale, *spec),
      weak_ptr_factory_(this) {
  if (base::FeatureList::IsEnabled(features::kServiceWorkerPaymentApps)) {
    get_all_instruments_finished_ = false;
    ServiceWorkerPaymentAppFactory::GetInstance()->GetAllPaymentApps(
        web_contents,
        payment_request_delegate_->GetPaymentManifestWebDataService(),
        spec_->method_data(),
        /*may_crawl_for_installable_payment_apps=*/
        !spec_->supports_basic_card(),
        base::BindOnce(&PaymentRequestState::GetAllPaymentAppsCallback,
                       weak_ptr_factory_.GetWeakPtr(), web_contents,
                       top_level_origin, frame_origin),
        base::BindOnce([]() {
          /* Nothing needs to be done after writing cache. This callback is used
           * only in tests. */
        }));
  } else {
    PopulateProfileCache();
    SetDefaultProfileSelections();
  }
  spec_->AddObserver(this);
}

PaymentRequestState::~PaymentRequestState() {}

void PaymentRequestState::GetAllPaymentAppsCallback(
    content::WebContents* web_contents,
    const GURL& top_level_origin,
    const GURL& frame_origin,
    content::PaymentAppProvider::PaymentApps apps,
    ServiceWorkerPaymentAppFactory::InstallablePaymentApps installable_apps) {
  number_of_pending_sw_payment_instruments_ =
      apps.size() + installable_apps.size();
  if (number_of_pending_sw_payment_instruments_ == 0U) {
    FinishedGetAllSWPaymentInstruments();
    return;
  }

  for (auto& app : apps) {
    std::unique_ptr<ServiceWorkerPaymentInstrument> instrument =
        std::make_unique<ServiceWorkerPaymentInstrument>(
            web_contents->GetBrowserContext(), top_level_origin, frame_origin,
            spec_, std::move(app.second), payment_request_delegate_);
    instrument->ValidateCanMakePayment(
        base::BindOnce(&PaymentRequestState::OnSWPaymentInstrumentValidated,
                       weak_ptr_factory_.GetWeakPtr()));
    available_instruments_.push_back(std::move(instrument));
  }

  for (auto& installable_app : installable_apps) {
    std::unique_ptr<ServiceWorkerPaymentInstrument> instrument =
        std::make_unique<ServiceWorkerPaymentInstrument>(
            web_contents, top_level_origin, frame_origin, spec_,
            std::move(installable_app.second), installable_app.first.spec(),
            payment_request_delegate_);
    instrument->ValidateCanMakePayment(
        base::BindOnce(&PaymentRequestState::OnSWPaymentInstrumentValidated,
                       weak_ptr_factory_.GetWeakPtr()));
    available_instruments_.push_back(std::move(instrument));
  }
}

void PaymentRequestState::OnSWPaymentInstrumentValidated(
    ServiceWorkerPaymentInstrument* instrument,
    bool result) {
  // Remove service worker payment instruments failed on validation.
  if (!result) {
    for (size_t i = 0; i < available_instruments_.size(); i++) {
      if (available_instruments_[i].get() == instrument) {
        available_instruments_.erase(available_instruments_.begin() + i);
        break;
      }
    }
  }

  if (--number_of_pending_sw_payment_instruments_ > 0)
    return;

  FinishedGetAllSWPaymentInstruments();
}

void PaymentRequestState::FinishedGetAllSWPaymentInstruments() {
  PopulateProfileCache();
  SetDefaultProfileSelections();

  get_all_instruments_finished_ = true;
  NotifyOnGetAllPaymentInstrumentsFinished();

  // Fullfill the pending CanMakePayment call.
  if (can_make_payment_callback_)
    CheckCanMakePayment(std::move(can_make_payment_callback_));

  // Fullfill the pending AreRequestedMethodsSupported call.
  if (are_requested_methods_supported_callback_)
    CheckRequestedMethodsSupported(
        std::move(are_requested_methods_supported_callback_));
}

void PaymentRequestState::OnPaymentResponseReady(
    mojom::PaymentResponsePtr payment_response) {
  delegate_->OnPaymentResponseAvailable(std::move(payment_response));
}

void PaymentRequestState::OnSpecUpdated() {
  if (spec_->selected_shipping_option_error().empty()) {
    selected_shipping_option_error_profile_ = nullptr;
  } else {
    selected_shipping_option_error_profile_ = selected_shipping_profile_;
    selected_shipping_profile_ = nullptr;
  }
  is_waiting_for_merchant_validation_ = false;
  UpdateIsReadyToPayAndNotifyObservers();
}

void PaymentRequestState::CanMakePayment(StatusCallback callback) {
  if (!get_all_instruments_finished_) {
    can_make_payment_callback_ = std::move(callback);
    return;
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&PaymentRequestState::CheckCanMakePayment,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PaymentRequestState::CheckCanMakePayment(StatusCallback callback) {
  bool can_make_payment_value = false;
  for (const auto& instrument : available_instruments_) {
    if (instrument->IsValidForCanMakePayment()) {
      can_make_payment_value = true;
      break;
    }
  }
  std::move(callback).Run(can_make_payment_value);
}

void PaymentRequestState::AreRequestedMethodsSupported(
    StatusCallback callback) {
  if (!get_all_instruments_finished_) {
    are_requested_methods_supported_callback_ = std::move(callback);
    return;
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&PaymentRequestState::CheckRequestedMethodsSupported,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PaymentRequestState::CheckRequestedMethodsSupported(
    StatusCallback callback) {
  DCHECK(get_all_instruments_finished_);

  std::move(callback).Run(!spec_->supported_card_networks().empty() ||
                          !available_instruments_.empty());
}

std::string PaymentRequestState::GetAuthenticatedEmail() const {
  return payment_request_delegate_->GetAuthenticatedEmail();
}

void PaymentRequestState::AddObserver(Observer* observer) {
  CHECK(observer);
  observers_.AddObserver(observer);
}

void PaymentRequestState::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void PaymentRequestState::GeneratePaymentResponse() {
  DCHECK(is_ready_to_pay());

  // Once the response is ready, will call back into OnPaymentResponseReady.
  response_helper_ = std::make_unique<PaymentResponseHelper>(
      app_locale_, spec_, selected_instrument_, payment_request_delegate_,
      selected_shipping_profile_, selected_contact_profile_, this);
}

void PaymentRequestState::RecordUseStats() {
  if (spec_->request_shipping()) {
    DCHECK(selected_shipping_profile_);
    personal_data_manager_->RecordUseOf(*selected_shipping_profile_);
  }

  if (spec_->request_payer_name() || spec_->request_payer_email() ||
      spec_->request_payer_phone()) {
    DCHECK(selected_contact_profile_);

    // If the same address was used for both contact and shipping, the stats
    // should only be updated once.
    if (!spec_->request_shipping() || (selected_shipping_profile_->guid() !=
                                       selected_contact_profile_->guid())) {
      personal_data_manager_->RecordUseOf(*selected_contact_profile_);
    }
  }

  selected_instrument_->RecordUse();
}

void PaymentRequestState::AddAutofillPaymentInstrument(
    bool selected,
    const autofill::CreditCard& card) {
  std::string basic_card_network =
      autofill::data_util::GetPaymentRequestData(card.network())
          .basic_card_issuer_network;
  if (!spec_->supported_card_networks_set().count(basic_card_network) ||
      !spec_->supported_card_types_set().count(card.card_type())) {
    return;
  }

  // The total number of card types: credit, debit, prepaid, unknown.
  constexpr size_t kTotalNumberOfCardTypes = 4U;

  // Whether the card type (credit, debit, prepaid) matches thetype that the
  // merchant has requested exactly. This should be false for unknown card
  // types, if the merchant cannot accept some card types.
  bool matches_merchant_card_type_exactly =
      card.card_type() != autofill::CreditCard::CARD_TYPE_UNKNOWN ||
      spec_->supported_card_types_set().size() == kTotalNumberOfCardTypes;

  // AutofillPaymentInstrument makes a copy of |card| so it is effectively
  // owned by this object.
  auto instrument = std::make_unique<AutofillPaymentInstrument>(
      basic_card_network, card, matches_merchant_card_type_exactly,
      shipping_profiles_, app_locale_, payment_request_delegate_);
  available_instruments_.push_back(std::move(instrument));

  if (selected)
    SetSelectedInstrument(available_instruments_.back().get());
}

void PaymentRequestState::AddAutofillShippingProfile(
    bool selected,
    const autofill::AutofillProfile& profile) {
  profile_cache_.push_back(
      std::make_unique<autofill::AutofillProfile>(profile));
  // TODO(tmartino): Implement deduplication rules specific to shipping
  // profiles.
  autofill::AutofillProfile* new_cached_profile = profile_cache_.back().get();
  shipping_profiles_.push_back(new_cached_profile);

  if (selected)
    SetSelectedShippingProfile(new_cached_profile);
}

void PaymentRequestState::AddAutofillContactProfile(
    bool selected,
    const autofill::AutofillProfile& profile) {
  profile_cache_.push_back(
      std::make_unique<autofill::AutofillProfile>(profile));
  autofill::AutofillProfile* new_cached_profile = profile_cache_.back().get();
  contact_profiles_.push_back(new_cached_profile);

  if (selected)
    SetSelectedContactProfile(new_cached_profile);
}

void PaymentRequestState::SetSelectedShippingOption(
    const std::string& shipping_option_id) {
  spec_->StartWaitingForUpdateWith(
      PaymentRequestSpec::UpdateReason::SHIPPING_OPTION);
  // This will inform the merchant and will lead to them calling updateWith with
  // new PaymentDetails.
  delegate_->OnShippingOptionIdSelected(shipping_option_id);
}

void PaymentRequestState::SetSelectedShippingProfile(
    autofill::AutofillProfile* profile) {
  spec_->StartWaitingForUpdateWith(
      PaymentRequestSpec::UpdateReason::SHIPPING_ADDRESS);
  selected_shipping_profile_ = profile;

  // The user should not be able to click on pay until the callback from the
  // merchant.
  is_waiting_for_merchant_validation_ = true;

  // Start the normalization of the shipping address.
  payment_request_delegate_->GetAddressNormalizer()->NormalizeAddressAsync(
      *selected_shipping_profile_, /*timeout_seconds=*/2,
      base::BindOnce(&PaymentRequestState::OnAddressNormalized,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PaymentRequestState::SetSelectedContactProfile(
    autofill::AutofillProfile* profile) {
  selected_contact_profile_ = profile;
  UpdateIsReadyToPayAndNotifyObservers();
}

void PaymentRequestState::SetSelectedInstrument(PaymentInstrument* instrument) {
  selected_instrument_ = instrument;
  UpdateIsReadyToPayAndNotifyObservers();
}

const std::string& PaymentRequestState::GetApplicationLocale() {
  return app_locale_;
}

autofill::PersonalDataManager* PaymentRequestState::GetPersonalDataManager() {
  return personal_data_manager_;
}

autofill::RegionDataLoader* PaymentRequestState::GetRegionDataLoader() {
  return payment_request_delegate_->GetRegionDataLoader();
}

bool PaymentRequestState::IsPaymentAppInvoked() const {
  return !!response_helper_;
}

autofill::AddressNormalizer* PaymentRequestState::GetAddressNormalizer() {
  return payment_request_delegate_->GetAddressNormalizer();
}

void PaymentRequestState::PopulateProfileCache() {
  std::vector<autofill::AutofillProfile*> profiles =
      personal_data_manager_->GetProfilesToSuggest();

  std::vector<autofill::AutofillProfile*> raw_profiles_for_filtering;
  raw_profiles_for_filtering.reserve(profiles.size());

  // PaymentRequest may outlive the Profiles returned by the Data Manager.
  // Thus, we store copies, and return a vector of pointers to these copies
  // whenever Profiles are requested.
  for (size_t i = 0; i < profiles.size(); i++) {
    profile_cache_.push_back(
        std::make_unique<autofill::AutofillProfile>(*profiles[i]));
    raw_profiles_for_filtering.push_back(profile_cache_.back().get());
  }

  contact_profiles_ = profile_comparator()->FilterProfilesForContact(
      raw_profiles_for_filtering);
  shipping_profiles_ = profile_comparator()->FilterProfilesForShipping(
      raw_profiles_for_filtering);

  // Set the number of suggestions shown for the sections requested by the
  // merchant.
  if (spec_->request_payer_name() || spec_->request_payer_phone() ||
      spec_->request_payer_email()) {
    bool has_complete_contact =
        contact_profiles_.empty()
            ? false
            : profile_comparator()->IsContactInfoComplete(contact_profiles_[0]);
    journey_logger_->SetNumberOfSuggestionsShown(
        JourneyLogger::Section::SECTION_CONTACT_INFO, contact_profiles_.size(),
        has_complete_contact);
  }
  if (spec_->request_shipping()) {
    bool has_complete_shipping =
        shipping_profiles_.empty()
            ? false
            : profile_comparator()->IsShippingComplete(shipping_profiles_[0]);
    journey_logger_->SetNumberOfSuggestionsShown(
        JourneyLogger::Section::SECTION_SHIPPING_ADDRESS,
        shipping_profiles_.size(), has_complete_shipping);
  }

  // Create the list of available instruments. A copy of each card will be made
  // by their respective AutofillPaymentInstrument.
  const std::vector<autofill::CreditCard*>& cards =
      personal_data_manager_->GetCreditCardsToSuggest(
          /*include_server_cards=*/true);
  for (autofill::CreditCard* card : cards)
    AddAutofillPaymentInstrument(/*selected=*/false, *card);
}

void PaymentRequestState::SetDefaultProfileSelections() {
  // Only pre-select an address if the merchant provided at least one selected
  // shipping option, and the top profile is complete. Assumes that profiles
  // have already been sorted for completeness and frecency.
  if (!shipping_profiles().empty() && spec_->selected_shipping_option() &&
      profile_comparator()->IsShippingComplete(shipping_profiles_[0])) {
    selected_shipping_profile_ = shipping_profiles()[0];
  }

  // Contact profiles were ordered by completeness in addition to frecency;
  // the first one is the best default selection.
  if (!contact_profiles().empty() &&
      profile_comparator()->IsContactInfoComplete(contact_profiles_[0]))
    selected_contact_profile_ = contact_profiles()[0];

  // TODO(crbug.com/702063): Change this code to prioritize instruments by use
  // count and other means, and implement a way to modify this function's return
  // value.
  const std::vector<std::unique_ptr<PaymentInstrument>>& instruments =
      available_instruments();
  auto first_complete_instrument =
      std::find_if(instruments.begin(), instruments.end(),
                   [](const std::unique_ptr<PaymentInstrument>& instrument) {
                     return instrument->IsCompleteForPayment() &&
                            instrument->IsExactlyMatchingMerchantRequest();
                   });
  selected_instrument_ = first_complete_instrument == instruments.end()
                             ? nullptr
                             : first_complete_instrument->get();
  UpdateIsReadyToPayAndNotifyObservers();

  bool has_complete_instrument =
      available_instruments().empty()
          ? false
          : available_instruments()[0]->IsCompleteForPayment();

  journey_logger_->SetNumberOfSuggestionsShown(
      JourneyLogger::Section::SECTION_PAYMENT_METHOD,
      available_instruments().size(), has_complete_instrument);
}

void PaymentRequestState::UpdateIsReadyToPayAndNotifyObservers() {
  is_ready_to_pay_ =
      ArePaymentDetailsSatisfied() && ArePaymentOptionsSatisfied();
  NotifyOnSelectedInformationChanged();
}

void PaymentRequestState::NotifyOnGetAllPaymentInstrumentsFinished() {
  for (auto& observer : observers_)
    observer.OnGetAllPaymentInstrumentsFinished();
}

void PaymentRequestState::NotifyOnSelectedInformationChanged() {
  for (auto& observer : observers_)
    observer.OnSelectedInformationChanged();
}

bool PaymentRequestState::ArePaymentDetailsSatisfied() {
  // There is no need to check for supported networks, because only supported
  // instruments are listed/created in the flow.
  return selected_instrument_ != nullptr &&
         selected_instrument_->IsCompleteForPayment();
}

bool PaymentRequestState::ArePaymentOptionsSatisfied() {
  if (is_waiting_for_merchant_validation_)
    return false;

  if (!profile_comparator()->IsShippingComplete(selected_shipping_profile_))
    return false;

  if (spec_->request_shipping() && !spec_->selected_shipping_option())
    return false;

  return profile_comparator()->IsContactInfoComplete(selected_contact_profile_);
}

void PaymentRequestState::OnAddressNormalized(
    bool success,
    const autofill::AutofillProfile& normalized_profile) {
  delegate_->OnShippingAddressSelected(
      data_util::GetPaymentAddressFromAutofillProfile(normalized_profile,
                                                      app_locale_));
}

}  // namespace payments
