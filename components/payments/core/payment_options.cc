// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/payment_options.h"

#include "base/values.h"

namespace payments {

namespace {

// These are defined as part of the spec at:
// https://w3c.github.io/payment-request/#paymentoptions-dictionary
static const char kPaymentOptionsRequestPayerEmail[] = "requestPayerEmail";
static const char kPaymentOptionsRequestPayerName[] = "requestPayerName";
static const char kPaymentOptionsRequestPayerPhone[] = "requestPayerPhone";
static const char kPaymentOptionsRequestShipping[] = "requestShipping";
static const char kPaymentOptionsShippingTypeDelivery[] = "delivery";
static const char kPaymentOptionsShippingTypePickup[] = "pickup";
static const char kPaymentOptionsShippingType[] = "shippingType";

}  // namespace

PaymentOptions::PaymentOptions()
    : request_payer_name(false),
      request_payer_email(false),
      request_payer_phone(false),
      request_shipping(false),
      shipping_type(payments::PaymentShippingType::SHIPPING) {}
PaymentOptions::~PaymentOptions() = default;

bool PaymentOptions::operator==(const PaymentOptions& other) const {
  return request_payer_name == other.request_payer_name &&
         request_payer_email == other.request_payer_email &&
         request_payer_phone == other.request_payer_phone &&
         request_shipping == other.request_shipping &&
         shipping_type == other.shipping_type;
}

bool PaymentOptions::operator!=(const PaymentOptions& other) const {
  return !(*this == other);
}

bool PaymentOptions::FromDictionaryValue(const base::DictionaryValue& value) {
  value.GetBoolean(kPaymentOptionsRequestPayerName, &request_payer_name);

  value.GetBoolean(kPaymentOptionsRequestPayerEmail, &request_payer_email);

  value.GetBoolean(kPaymentOptionsRequestPayerPhone, &request_payer_phone);

  value.GetBoolean(kPaymentOptionsRequestShipping, &request_shipping);

  std::string shipping_type_string;
  value.GetString(kPaymentOptionsShippingType, &shipping_type_string);
  if (shipping_type_string == kPaymentOptionsShippingTypeDelivery) {
    shipping_type = payments::PaymentShippingType::DELIVERY;
  } else if (shipping_type_string == kPaymentOptionsShippingTypePickup) {
    shipping_type = payments::PaymentShippingType::PICKUP;
  } else {
    shipping_type = payments::PaymentShippingType::SHIPPING;
  }

  return true;
}

}  // namespace payments
