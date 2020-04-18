// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/basic_card_response.h"

#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace payments {

// Tests that two credit card response objects are not equal if their property
// values differ or one is missing a value present in the other, and equal
// otherwise. Doesn't test all properties of child objects, relying instead on
// their respective tests.
TEST(PaymentRequestTest, BasicCardResponseEquality) {
  BasicCardResponse card_response1;
  BasicCardResponse card_response2;
  EXPECT_EQ(card_response1, card_response2);

  card_response1.cardholder_name = base::ASCIIToUTF16("Shadow Moon");
  EXPECT_NE(card_response1, card_response2);
  card_response2.cardholder_name = base::ASCIIToUTF16("Mad Sweeney");
  EXPECT_NE(card_response1, card_response2);
  card_response2.cardholder_name = base::ASCIIToUTF16("Shadow Moon");
  EXPECT_EQ(card_response1, card_response2);

  card_response1.card_number = base::ASCIIToUTF16("4111111111111111");
  EXPECT_NE(card_response1, card_response2);
  card_response2.card_number = base::ASCIIToUTF16("1111");
  EXPECT_NE(card_response1, card_response2);
  card_response2.card_number = base::ASCIIToUTF16("4111111111111111");
  EXPECT_EQ(card_response1, card_response2);

  card_response1.expiry_month = base::ASCIIToUTF16("01");
  EXPECT_NE(card_response1, card_response2);
  card_response2.expiry_month = base::ASCIIToUTF16("11");
  EXPECT_NE(card_response1, card_response2);
  card_response2.expiry_month = base::ASCIIToUTF16("01");
  EXPECT_EQ(card_response1, card_response2);

  card_response1.expiry_year = base::ASCIIToUTF16("27");
  EXPECT_NE(card_response1, card_response2);
  card_response2.expiry_year = base::ASCIIToUTF16("72");
  EXPECT_NE(card_response1, card_response2);
  card_response2.expiry_year = base::ASCIIToUTF16("27");
  EXPECT_EQ(card_response1, card_response2);

  card_response1.expiry_year = base::ASCIIToUTF16("123");
  EXPECT_NE(card_response1, card_response2);
  card_response2.expiry_year = base::ASCIIToUTF16("999");
  EXPECT_NE(card_response1, card_response2);
  card_response2.expiry_year = base::ASCIIToUTF16("123");
  EXPECT_EQ(card_response1, card_response2);

  mojom::PaymentAddressPtr billing_address1 = mojom::PaymentAddress::New();
  billing_address1->postal_code = "90210";
  mojom::PaymentAddressPtr billing_address2 = mojom::PaymentAddress::New();
  billing_address2->postal_code = "01209";
  card_response1.billing_address = billing_address1.Clone();
  EXPECT_NE(card_response1, card_response2);
  card_response2.billing_address = billing_address2.Clone();
  EXPECT_NE(card_response1, card_response2);
  card_response2.billing_address = billing_address1.Clone();
  EXPECT_EQ(card_response1, card_response2);
}

}  // namespace payments
