// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/payment_item.h"

#include "base/values.h"

namespace payments {

namespace {

// These are defined as part of the spec at:
// https://w3c.github.io/payment-request/#dom-paymentitem
static const char kPaymentItemAmount[] = "amount";
static const char kPaymentItemLabel[] = "label";
static const char kPaymentItemPending[] = "pending";

}  // namespace

PaymentItem::PaymentItem()
    : amount(mojom::PaymentCurrencyAmount::New()), pending(false) {}

PaymentItem::~PaymentItem() = default;

PaymentItem::PaymentItem(const PaymentItem& other) {
  *this = other;
}

bool PaymentItem::operator==(const PaymentItem& other) const {
  return label == other.label && amount.Equals(other.amount) &&
         pending == other.pending;
}

bool PaymentItem::operator!=(const PaymentItem& other) const {
  return !(*this == other);
}

PaymentItem& PaymentItem::operator=(const PaymentItem& other) {
  label = other.label;
  if (other.amount) {
    amount = other.amount->Clone();
  } else {
    amount.reset();
  }
  pending = other.pending;
  return *this;
}

bool PaymentItem::FromDictionaryValue(const base::DictionaryValue& value) {
  if (!value.GetString(kPaymentItemLabel, &label)) {
    return false;
  }

  const base::DictionaryValue* amount_dict = nullptr;
  if (!value.GetDictionary(kPaymentItemAmount, &amount_dict)) {
    return false;
  }
  amount = mojom::PaymentCurrencyAmount::New();
  if (!PaymentCurrencyAmountFromDictionaryValue(*amount_dict, amount.get())) {
    return false;
  }

  // Pending is optional.
  value.GetBoolean(kPaymentItemPending, &pending);

  return true;
}

std::unique_ptr<base::DictionaryValue> PaymentItem::ToDictionaryValue() const {
  auto result = std::make_unique<base::DictionaryValue>();
  result->SetString(kPaymentItemLabel, label);
  result->SetDictionary(kPaymentItemAmount,
                        PaymentCurrencyAmountToDictionaryValue(*amount));
  result->SetBoolean(kPaymentItemPending, pending);

  return result;
}

}  // namespace payments
