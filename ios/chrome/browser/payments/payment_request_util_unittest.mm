// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/payments/payment_request_util.h"

#include "base/json/json_writer.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/payments/core/basic_card_response.h"
#include "components/payments/core/payment_address.h"
#include "components/payments/core/payment_response.h"
#include "components/payments/mojom/payment_request_data.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace payment_request_util {

using PaymentRequestUtilTest = PlatformTest;

// Tests that serializing a default PaymentResponse yields the expected result.
TEST_F(PaymentRequestUtilTest,
       PaymentResponseToDictionaryValue_EmptyResponseDictionary) {
  base::DictionaryValue expected_value;

  expected_value.SetString("requestId", "");
  expected_value.SetString("methodName", "");
  expected_value.Set("details", std::make_unique<base::Value>());
  expected_value.Set("shippingAddress", std::make_unique<base::Value>());
  expected_value.SetString("shippingOption", "");
  expected_value.SetString("payerName", "");
  expected_value.SetString("payerEmail", "");
  expected_value.SetString("payerPhone", "");

  payments::PaymentResponse payment_response;
  EXPECT_TRUE(expected_value.Equals(
      PaymentResponseToDictionaryValue(payment_response).get()));
}

// Tests that serializing a populated PaymentResponse yields the expected
// result.
TEST_F(PaymentRequestUtilTest,
       PaymentResponseToDictionaryValue_PopulatedResponseDictionary) {
  base::DictionaryValue expected_value;

  auto details = std::make_unique<base::DictionaryValue>();
  details->SetString("cardNumber", "1111-1111-1111-1111");
  details->SetString("cardholderName", "Jon Doe");
  details->SetString("expiryMonth", "02");
  details->SetString("expiryYear", "2090");
  details->SetString("cardSecurityCode", "111");
  auto billing_address = std::make_unique<base::DictionaryValue>();
  billing_address->SetString("country", "");
  billing_address->Set("addressLine", std::make_unique<base::ListValue>());
  billing_address->SetString("region", "");
  billing_address->SetString("dependentLocality", "");
  billing_address->SetString("city", "");
  billing_address->SetString("postalCode", "90210");
  billing_address->SetString("languageCode", "");
  billing_address->SetString("sortingCode", "");
  billing_address->SetString("organization", "");
  billing_address->SetString("recipient", "");
  billing_address->SetString("phone", "");
  details->Set("billingAddress", std::move(billing_address));
  expected_value.Set("details", std::move(details));
  expected_value.SetString("requestId", "12345");
  expected_value.SetString("methodName", "American Express");
  auto shipping_address = std::make_unique<base::DictionaryValue>();
  shipping_address->SetString("country", "");
  shipping_address->Set("addressLine", std::make_unique<base::ListValue>());
  shipping_address->SetString("region", "");
  shipping_address->SetString("dependentLocality", "");
  shipping_address->SetString("city", "");
  shipping_address->SetString("postalCode", "94115");
  shipping_address->SetString("languageCode", "");
  shipping_address->SetString("sortingCode", "");
  shipping_address->SetString("organization", "");
  shipping_address->SetString("recipient", "");
  shipping_address->SetString("phone", "");
  expected_value.Set("shippingAddress", std::move(shipping_address));
  expected_value.SetString("shippingOption", "666");
  expected_value.SetString("payerName", "Jane Doe");
  expected_value.SetString("payerEmail", "jane@example.com");
  expected_value.SetString("payerPhone", "1234-567-890");

  payments::PaymentResponse payment_response;
  payment_response.payment_request_id = "12345";
  payment_response.method_name = "American Express";

  payments::BasicCardResponse payment_response_details;
  payment_response_details.card_number =
      base::ASCIIToUTF16("1111-1111-1111-1111");
  payment_response_details.cardholder_name = base::ASCIIToUTF16("Jon Doe");
  payment_response_details.expiry_month = base::ASCIIToUTF16("02");
  payment_response_details.expiry_year = base::ASCIIToUTF16("2090");
  payment_response_details.card_security_code = base::ASCIIToUTF16("111");
  payment_response_details.billing_address->postal_code = "90210";
  std::unique_ptr<base::DictionaryValue> response_value =
      payment_response_details.ToDictionaryValue();
  std::string payment_response_stringified_details;
  base::JSONWriter::Write(*response_value,
                          &payment_response_stringified_details);
  payment_response.details = payment_response_stringified_details;

  payment_response.shipping_address = payments::mojom::PaymentAddress::New();
  payment_response.shipping_address->postal_code = "94115";
  payment_response.shipping_option = "666";
  payment_response.payer_name = base::ASCIIToUTF16("Jane Doe");
  payment_response.payer_email = base::ASCIIToUTF16("jane@example.com");
  payment_response.payer_phone = base::ASCIIToUTF16("1234-567-890");
  EXPECT_TRUE(expected_value.Equals(
      PaymentResponseToDictionaryValue(payment_response).get()));
}

}  // namespace payment_request_util
