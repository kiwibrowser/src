// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/web_payment_request.h"

#include "base/values.h"

namespace payments {

namespace {

// These are defined as part of the spec at:
// https://w3c.github.io/payment-request/#paymentrequest-interface
static const char kPaymentRequestDetails[] = "details";
static const char kPaymentRequestId[] = "id";
static const char kPaymentRequestMethodData[] = "methodData";
static const char kPaymentRequestOptions[] = "options";

}  // namespace

WebPaymentRequest::WebPaymentRequest() {}
WebPaymentRequest::~WebPaymentRequest() = default;

bool WebPaymentRequest::operator==(const WebPaymentRequest& other) const {
  return payment_request_id == other.payment_request_id &&
         shipping_address.Equals(other.shipping_address) &&
         shipping_option == other.shipping_option &&
         method_data == other.method_data && details == other.details &&
         options == other.options;
}

bool WebPaymentRequest::operator!=(const WebPaymentRequest& other) const {
  return !(*this == other);
}

WebPaymentRequest::WebPaymentRequest(const WebPaymentRequest& other) {
  *this = other;
}

WebPaymentRequest& WebPaymentRequest::operator=(
    const WebPaymentRequest& other) {
  payment_request_id = other.payment_request_id;
  shipping_address = other.shipping_address ? other.shipping_address->Clone()
                                            : mojom::PaymentAddress::New();
  shipping_option = other.shipping_option;
  method_data = other.method_data;
  details = other.details;
  options = other.options;
  return *this;
}

bool WebPaymentRequest::FromDictionaryValue(
    const base::DictionaryValue& value) {
  method_data.clear();

  if (!value.GetString(kPaymentRequestId, &payment_request_id)) {
    return false;
  }

  // Parse the payment method data.
  const base::ListValue* method_data_list = nullptr;
  // At least one method is required.
  if (!value.GetList(kPaymentRequestMethodData, &method_data_list) ||
      method_data_list->GetSize() == 0) {
    return false;
  }
  for (size_t i = 0; i < method_data_list->GetSize(); ++i) {
    const base::DictionaryValue* method_data_dict;
    if (!method_data_list->GetDictionary(i, &method_data_dict))
      return false;

    payments::PaymentMethodData method_datum;
    if (!method_datum.FromDictionaryValue(*method_data_dict))
      return false;
    method_data.push_back(method_datum);
  }

  // Parse the payment details.
  const base::DictionaryValue* payment_details_dict = nullptr;
  if (!value.GetDictionary(kPaymentRequestDetails, &payment_details_dict) ||
      !details.FromDictionaryValue(*payment_details_dict,
                                   /*requires_total=*/true)) {
    return false;
  }

  // Parse the payment options.
  const base::DictionaryValue* payment_options = nullptr;
  // Options field is optional.
  if (value.GetDictionary(kPaymentRequestOptions, &payment_options))
    if (!options.FromDictionaryValue(*payment_options))
      return false;

  return true;
}

}  // namespace payments
