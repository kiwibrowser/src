// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/payment_details_modifier.h"

#include "base/values.h"
#include "components/payments/core/payment_method_data.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace payments {

// Tests that serializing a default PaymentDetailsModifier yields the expected
// result.
TEST(PaymentRequestTest, EmptyPaymentDetailsModifierDictionary) {
  base::DictionaryValue expected_value;

  std::unique_ptr<base::ListValue> supported_methods_list =
      std::make_unique<base::ListValue>();
  expected_value.SetList("supportedMethods", std::move(supported_methods_list));
  expected_value.SetString("data", "");

  PaymentDetailsModifier payment_details_modifier;
  EXPECT_TRUE(expected_value.Equals(
      payment_details_modifier.ToDictionaryValue().get()));
}

// Tests that serializing a populated PaymentDetailsModifier yields the expected
// result.
TEST(PaymentRequestTest, PopulatedDetailsModifierDictionary) {
  base::DictionaryValue expected_value;

  std::unique_ptr<base::ListValue> supported_methods_list =
      std::make_unique<base::ListValue>();
  supported_methods_list->GetList().emplace_back("basic-card");
  supported_methods_list->GetList().emplace_back("amex");
  expected_value.SetList("supportedMethods", std::move(supported_methods_list));
  expected_value.SetString("data",
                           "{\"supportedNetworks\":[\"visa\",\"mastercard\"]}");
  std::unique_ptr<base::DictionaryValue> item_dict =
      std::make_unique<base::DictionaryValue>();
  item_dict->SetString("label", "Gratuity");
  std::unique_ptr<base::DictionaryValue> amount_dict =
      std::make_unique<base::DictionaryValue>();
  amount_dict->SetString("currency", "USD");
  amount_dict->SetString("value", "139.99");
  item_dict->SetDictionary("amount", std::move(amount_dict));
  item_dict->SetBoolean("pending", false);
  expected_value.SetDictionary("total", std::move(item_dict));

  PaymentDetailsModifier payment_details_modifier;
  payment_details_modifier.method_data.supported_methods.push_back(
      "basic-card");
  payment_details_modifier.method_data.supported_methods.push_back("amex");
  payment_details_modifier.method_data.data =
      "{\"supportedNetworks\":[\"visa\",\"mastercard\"]}";
  payment_details_modifier.total = std::make_unique<PaymentItem>();
  payment_details_modifier.total->label = "Gratuity";
  payment_details_modifier.total->amount->currency = "USD";
  payment_details_modifier.total->amount->value = "139.99";

  EXPECT_TRUE(expected_value.Equals(
      payment_details_modifier.ToDictionaryValue().get()));
}

// Tests that two details modifier objects are not equal if their property
// values differ or one is missing a value present in the other, and equal
// otherwise. Doesn't test all properties of child objects, relying instead on
// their respective tests.
TEST(PaymentRequestTest, PaymentDetailsModifierEquality) {
  PaymentDetailsModifier details_modifier1;
  PaymentDetailsModifier details_modifier2;
  EXPECT_EQ(details_modifier1, details_modifier2);

  std::vector<std::string> supported_methods1;
  supported_methods1.push_back("China UnionPay");
  supported_methods1.push_back("BobPay");
  details_modifier1.method_data.supported_methods = supported_methods1;
  EXPECT_NE(details_modifier1, details_modifier2);
  std::vector<std::string> supported_methods2;
  supported_methods2.push_back("BobPay");
  details_modifier2.method_data.supported_methods = supported_methods2;
  EXPECT_NE(details_modifier1, details_modifier2);
  details_modifier2.method_data.supported_methods = supported_methods1;
  EXPECT_EQ(details_modifier1, details_modifier2);

  details_modifier1.method_data.data =
      "{\"supportedNetworks\":[\"visa\",\"mastercard\"]}";
  EXPECT_NE(details_modifier1, details_modifier2);
  details_modifier2.method_data.data =
      "{\"supportedNetworks\":[\"visa\",\"mastercard\"]}";
  EXPECT_EQ(details_modifier1, details_modifier2);

  details_modifier1.total = std::make_unique<PaymentItem>();
  details_modifier1.total->label = "Total";
  EXPECT_NE(details_modifier1, details_modifier2);
  details_modifier2.total = std::make_unique<PaymentItem>();
  details_modifier2.total->label = "Gratuity";
  EXPECT_NE(details_modifier1, details_modifier2);
  details_modifier2.total->label = "Total";
  EXPECT_EQ(details_modifier1, details_modifier2);

  PaymentItem payment_item;
  payment_item.label = "Tax";
  std::vector<PaymentItem> display_items1;
  display_items1.push_back(payment_item);
  details_modifier1.additional_display_items = display_items1;
  EXPECT_NE(details_modifier1, details_modifier2);
  std::vector<PaymentItem> display_items2;
  display_items2.push_back(payment_item);
  display_items2.push_back(payment_item);
  details_modifier2.additional_display_items = display_items2;
  EXPECT_NE(details_modifier1, details_modifier2);
  details_modifier2.additional_display_items = display_items1;
  EXPECT_EQ(details_modifier1, details_modifier2);
}

}  // namespace payments
