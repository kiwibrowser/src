// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/views/payments/payment_request_browsertest_base.h"
#include "chrome/browser/ui/views/payments/payment_request_dialog_view_ids.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/test/browser_test_utils.h"

namespace payments {

class PaymentRequestPaymentResponseAutofillPaymentInstrumentTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestPaymentResponseAutofillPaymentInstrumentTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(
      PaymentRequestPaymentResponseAutofillPaymentInstrumentTest);
};

// Tests that the PaymentResponse contains all the required fields for an
// Autofill payment instrument.
IN_PROC_BROWSER_TEST_F(
    PaymentRequestPaymentResponseAutofillPaymentInstrumentTest,
    TestPaymentResponse) {
  NavigateTo("/payment_request_no_shipping_test.html");
  // Setup a credit card with an associated billing address.
  autofill::AutofillProfile billing_address = autofill::test::GetFullProfile();
  AddAutofillProfile(billing_address);
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(billing_address.guid());
  AddCreditCard(card);  // Visa.

  // Complete the Payment Request.
  InvokePaymentRequestUI();
  ResetEventWaiter(DialogEvent::DIALOG_CLOSED);
  PayWithCreditCardAndWait(base::ASCIIToUTF16("123"));

  // Test that the card details were sent to the merchant.
  ExpectBodyContains({"\"cardNumber\": \"4111111111111111\"",
                      "\"cardSecurityCode\": \"123\"",
                      "\"cardholderName\": \"Test User\"",
                      "\"expiryMonth\": \"11\"", "\"expiryYear\": \"2022\""});

  // Test that the billing address was sent to the merchant.
  ExpectBodyContains(
      {"\"billingAddress\": {", "\"666 Erebus St.\"", "\"Apt 8\"",
       "\"city\": \"Elysium\"", "\"dependentLocality\": \"\"",
       "\"country\": \"US\"", "\"sortingCode\": \"\"", "\"languageCode\": \"\"",
       "\"organization\": \"Underworld\"", "\"phone\": \"+16502111111\"",
       "\"postalCode\": \"91111\"", "\"recipient\": \"John H. Doe\"",
       "\"region\": \"CA\""});
}

class PaymentRequestPaymentResponseShippingAddressTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestPaymentResponseShippingAddressTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PaymentRequestPaymentResponseShippingAddressTest);
};

// Tests that the PaymentResponse contains all the required fields for a
// shipping address and shipping option.
IN_PROC_BROWSER_TEST_F(PaymentRequestPaymentResponseShippingAddressTest,
                       TestPaymentResponse) {
  NavigateTo("/payment_request_free_shipping_test.html");
  // Create a billing address and a card that uses it.
  autofill::AutofillProfile billing_address = autofill::test::GetFullProfile();
  AddAutofillProfile(billing_address);
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(billing_address.guid());
  AddCreditCard(card);  // Visa.

  // Create a shipping address with a higher frecency score, so that it is
  // selected as the defautl shipping address.
  autofill::AutofillProfile shipping_address =
      autofill::test::GetFullProfile2();
  shipping_address.set_use_count(2000);
  AddAutofillProfile(shipping_address);

  // Complete the Payment Request.
  InvokePaymentRequestUI();
  ResetEventWaiter(DialogEvent::DIALOG_CLOSED);
  PayWithCreditCardAndWait(base::ASCIIToUTF16("123"));

  // Test that the shipping address was sent to the merchant.
  ExpectBodyContains({"\"country\": \"US\"", "\"123 Main Street\"",
                      "\"Unit 1\"", "\"region\": \"MI\"",
                      "\"city\": \"Greensdale\"", "\"dependentLocality\": \"\"",
                      "\"postalCode\": \"48838\"", "\"sortingCode\": \"\"",
                      "\"languageCode\": \"\"", "\"organization\": \"ACME\"",
                      "\"recipient\": \"Jane A. Smith\"",
                      "\"phone\": \"+13105557889\""});

  // Test that the shipping option was sent to the merchant.
  ExpectBodyContains({"\"shippingOption\": \"freeShippingOption\""});
}

class PaymentRequestPaymentResponseAllContactDetailsTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestPaymentResponseAllContactDetailsTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PaymentRequestPaymentResponseAllContactDetailsTest);
};

// Tests that the PaymentResponse contains all the required fields for contact
// details when all three details are requested.
IN_PROC_BROWSER_TEST_F(PaymentRequestPaymentResponseAllContactDetailsTest,
                       TestPaymentResponse) {
  NavigateTo("/payment_request_contact_details_and_free_shipping_test.html");
  // Setup a credit card with an associated billing address.
  autofill::AutofillProfile billing_address = autofill::test::GetFullProfile();
  AddAutofillProfile(billing_address);
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(billing_address.guid());
  AddCreditCard(card);  // Visa.

  // Complete the Payment Request.
  InvokePaymentRequestUI();
  ResetEventWaiter(DialogEvent::DIALOG_CLOSED);
  PayWithCreditCardAndWait(base::ASCIIToUTF16("123"));

  // Test that the contact details were sent to the merchant.
  ExpectBodyContains({"\"payerName\": \"John H. Doe\"",
                      "\"payerEmail\": \"johndoe@hades.com\"",
                      "\"payerPhone\": \"+16502111111\""});
}

class PaymentRequestPaymentResponseOneContactDetailTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestPaymentResponseOneContactDetailTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PaymentRequestPaymentResponseOneContactDetailTest);
};

// Tests that the PaymentResponse contains all the required fields for contact
// details when all ont detail is requested.
IN_PROC_BROWSER_TEST_F(PaymentRequestPaymentResponseOneContactDetailTest,
                       TestPaymentResponse) {
  NavigateTo("/payment_request_email_and_free_shipping_test.html");
  // Setup a credit card with an associated billing address.
  autofill::AutofillProfile billing_address = autofill::test::GetFullProfile();
  AddAutofillProfile(billing_address);
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(billing_address.guid());
  AddCreditCard(card);  // Visa.

  // Complete the Payment Request.
  InvokePaymentRequestUI();
  ResetEventWaiter(DialogEvent::DIALOG_CLOSED);
  PayWithCreditCardAndWait(base::ASCIIToUTF16("123"));

  // Test that the contact details were sent to the merchant.
  ExpectBodyContains({"\"payerName\": null",
                      "\"payerEmail\": \"johndoe@hades.com\"",
                      "\"payerPhone\": null"});
}

}  // namespace payments
