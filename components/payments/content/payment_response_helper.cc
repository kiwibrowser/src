// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/payment_response_helper.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_country.h"
#include "components/autofill/core/browser/autofill_data_util.h"
#include "components/autofill/core/browser/autofill_type.h"
#include "components/autofill/core/browser/phone_number_i18n.h"
#include "components/payments/content/payment_request_spec.h"
#include "components/payments/core/payment_request_data_util.h"
#include "components/payments/core/payment_request_delegate.h"

namespace payments {

PaymentResponseHelper::PaymentResponseHelper(
    const std::string& app_locale,
    PaymentRequestSpec* spec,
    PaymentInstrument* selected_instrument,
    PaymentRequestDelegate* payment_request_delegate,
    autofill::AutofillProfile* selected_shipping_profile,
    autofill::AutofillProfile* selected_contact_profile,
    Delegate* delegate)
    : app_locale_(app_locale),
      is_waiting_for_shipping_address_normalization_(false),
      is_waiting_for_instrument_details_(false),
      spec_(spec),
      delegate_(delegate),
      selected_instrument_(selected_instrument),
      payment_request_delegate_(payment_request_delegate),
      selected_contact_profile_(selected_contact_profile),
      weak_ptr_factory_(this) {
  DCHECK(spec_);
  DCHECK(selected_instrument_);
  DCHECK(delegate_);

  is_waiting_for_instrument_details_ = true;

  // Start to normalize the shipping address, if necessary.
  if (spec_->request_shipping()) {
    DCHECK(selected_shipping_profile);
    DCHECK(spec_->selected_shipping_option());

    is_waiting_for_shipping_address_normalization_ = true;

    payment_request_delegate_->GetAddressNormalizer()->NormalizeAddressAsync(
        *selected_shipping_profile,
        /*timeout_seconds=*/5,
        base::BindOnce(&PaymentResponseHelper::OnAddressNormalized,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  // Start to get the instrument details. Will call back into
  // OnInstrumentDetailsReady.
  selected_instrument_->InvokePaymentApp(this);
}

PaymentResponseHelper::~PaymentResponseHelper() {}

void PaymentResponseHelper::OnInstrumentDetailsReady(
    const std::string& method_name,
    const std::string& stringified_details) {
  method_name_ = method_name;
  stringified_details_ = stringified_details;
  is_waiting_for_instrument_details_ = false;

  if (!is_waiting_for_shipping_address_normalization_)
    GeneratePaymentResponse();
}

void PaymentResponseHelper::OnAddressNormalized(
    bool success,
    const autofill::AutofillProfile& normalized_profile) {
  if (is_waiting_for_shipping_address_normalization_) {
    shipping_address_ = normalized_profile;
    is_waiting_for_shipping_address_normalization_ = false;

    if (!is_waiting_for_instrument_details_)
      GeneratePaymentResponse();
  }
}

void PaymentResponseHelper::GeneratePaymentResponse() {
  DCHECK(!is_waiting_for_instrument_details_);
  DCHECK(!is_waiting_for_shipping_address_normalization_);

  mojom::PaymentResponsePtr payment_response = mojom::PaymentResponse::New();

  // Make sure that we return the method name that the merchant specified for
  // this instrument: cards can be either specified through their name (e.g.,
  // "visa") or through basic-card's supportedNetworks.
  payment_response->method_name =
      spec_->IsMethodSupportedThroughBasicCard(method_name_)
          ? kBasicCardMethodName
          : method_name_;
  payment_response->stringified_details = stringified_details_;

  // Shipping Address section
  if (spec_->request_shipping()) {
    payment_response->shipping_address =
        data_util::GetPaymentAddressFromAutofillProfile(shipping_address_,
                                                        app_locale_);
    payment_response->shipping_option = spec_->selected_shipping_option()->id;
  }

  // Contact Details section.
  if (spec_->request_payer_name()) {
    DCHECK(selected_contact_profile_);
    payment_response->payer_name = base::UTF16ToUTF8(
        selected_contact_profile_->GetInfo(autofill::NAME_FULL, app_locale_));
  }
  if (spec_->request_payer_email()) {
    DCHECK(selected_contact_profile_);
    payment_response->payer_email = base::UTF16ToUTF8(
        selected_contact_profile_->GetRawInfo(autofill::EMAIL_ADDRESS));
  }
  if (spec_->request_payer_phone()) {
    DCHECK(selected_contact_profile_);

    // Try to format the phone number to the E.164 format to send in the Payment
    // Response, as defined in the Payment Request spec. If it's not possible,
    // send the original. More info at:
    // https://w3c.github.io/browser-payment-api/#paymentrequest-updated-algorithm
    const std::string original_number =
        base::UTF16ToUTF8(selected_contact_profile_->GetInfo(
            autofill::PHONE_HOME_WHOLE_NUMBER, app_locale_));

    const std::string default_region_code =
        autofill::AutofillCountry::CountryCodeForLocale(app_locale_);
    payment_response->payer_phone = autofill::i18n::FormatPhoneForResponse(
        original_number, default_region_code);
  }

  delegate_->OnPaymentResponseReady(std::move(payment_response));
}

}  // namespace payments
