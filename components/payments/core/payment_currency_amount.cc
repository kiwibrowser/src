// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/payment_currency_amount.h"

#include "base/values.h"

namespace payments {

namespace {

// These are defined as part of the spec at:
// https://w3c.github.io/browser-payment-api/#dom-paymentcurrencyamount
static const char kPaymentCurrencyAmountCurrency[] = "currency";
static const char kPaymentCurrencyAmountValue[] = "value";

}  // namespace

bool PaymentCurrencyAmountFromDictionaryValue(
    const base::DictionaryValue& dictionary_value,
    mojom::PaymentCurrencyAmount* amount) {
  if (!dictionary_value.GetString(kPaymentCurrencyAmountCurrency,
                                  &amount->currency)) {
    return false;
  }

  if (!dictionary_value.GetString(kPaymentCurrencyAmountValue,
                                  &amount->value)) {
    return false;
  }

  return true;
}

std::unique_ptr<base::DictionaryValue> PaymentCurrencyAmountToDictionaryValue(
    const mojom::PaymentCurrencyAmount& amount) {
  auto result = std::make_unique<base::DictionaryValue>();
  result->SetString(kPaymentCurrencyAmountCurrency, amount.currency);
  result->SetString(kPaymentCurrencyAmountValue, amount.value);

  return result;
}

}  // namespace payments
