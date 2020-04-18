// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/payment_shipping_option.h"

#include "base/values.h"

namespace payments {

namespace {

// These are defined as part of the spec at:
// https://w3c.github.io/browser-payment-api/#dom-paymentshippingoption
static const char kPaymentShippingOptionAmount[] = "amount";
static const char kPaymentShippingOptionId[] = "id";
static const char kPaymentShippingOptionLabel[] = "label";
static const char kPaymentShippingOptionSelected[] = "selected";

}  // namespace

PaymentShippingOption::PaymentShippingOption() : selected(false) {
  amount = mojom::PaymentCurrencyAmount::New();
}

PaymentShippingOption::PaymentShippingOption(
    const PaymentShippingOption& other) {
  *this = other;
}

PaymentShippingOption::~PaymentShippingOption() = default;

bool PaymentShippingOption::operator==(
    const PaymentShippingOption& other) const {
  return id == other.id && label == other.label &&
         amount.Equals(other.amount) && selected == other.selected;
}

bool PaymentShippingOption::operator!=(
    const PaymentShippingOption& other) const {
  return !(*this == other);
}

PaymentShippingOption& PaymentShippingOption::operator=(
    const PaymentShippingOption& other) {
  id = other.id;
  label = other.label;
  if (other.amount) {
    amount = other.amount.Clone();
  } else {
    amount.reset();
  }
  selected = other.selected;
  return *this;
}

bool PaymentShippingOption::FromDictionaryValue(
    const base::DictionaryValue& value) {
  if (!value.GetString(kPaymentShippingOptionId, &id)) {
    return false;
  }

  if (!value.GetString(kPaymentShippingOptionLabel, &label)) {
    return false;
  }

  const base::DictionaryValue* amount_dict = nullptr;
  if (!value.GetDictionary(kPaymentShippingOptionAmount, &amount_dict)) {
    return false;
  }
  amount = mojom::PaymentCurrencyAmount::New();
  if (!PaymentCurrencyAmountFromDictionaryValue(*amount_dict, amount.get())) {
    return false;
  }

  // Selected is optional.
  value.GetBoolean(kPaymentShippingOptionSelected, &selected);

  return true;
}

}  // namespace payments
