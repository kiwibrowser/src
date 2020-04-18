// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/payments/payment_request_browsertest_base.h"
#include "chrome/browser/ui/views/payments/payment_request_dialog_view_ids.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/field_types.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace payments {

// A simple PaymentRequest which simply requests 'visa' or 'mastercard' and
// nothing else.
class PaymentSheetViewControllerNoShippingTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentSheetViewControllerNoShippingTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PaymentSheetViewControllerNoShippingTest);
};

// With no data present, the pay button should be disabled.
IN_PROC_BROWSER_TEST_F(PaymentSheetViewControllerNoShippingTest, NoData) {
  NavigateTo("/payment_request_no_shipping_test.html");
  InvokePaymentRequestUI();

  EXPECT_FALSE(IsPayButtonEnabled());
}

// With a supported card (Visa) present, the pay button should be enabled.
IN_PROC_BROWSER_TEST_F(PaymentSheetViewControllerNoShippingTest,
                       SupportedCard) {
  NavigateTo("/payment_request_no_shipping_test.html");
  autofill::AutofillProfile profile(autofill::test::GetFullProfile());
  AddAutofillProfile(profile);
  autofill::CreditCard card(autofill::test::GetCreditCard());  // Visa card.
  card.set_billing_address_id(profile.guid());
  AddCreditCard(card);

  InvokePaymentRequestUI();
  EXPECT_TRUE(IsPayButtonEnabled());
}

// With only an unsupported card (Amex) in the database, the pay button should
// be disabled.
IN_PROC_BROWSER_TEST_F(PaymentSheetViewControllerNoShippingTest,
                       UnsupportedCard) {
  NavigateTo("/payment_request_no_shipping_test.html");
  AddCreditCard(autofill::test::GetCreditCard2());  // Amex card.

  InvokePaymentRequestUI();
  EXPECT_FALSE(IsPayButtonEnabled());
}

// If shipping and contact info are not requested, their rows should not be
// present.
IN_PROC_BROWSER_TEST_F(PaymentSheetViewControllerNoShippingTest,
                       NoShippingNoContactRows) {
  NavigateTo("/payment_request_no_shipping_test.html");
  InvokePaymentRequestUI();

  EXPECT_NE(nullptr, dialog_view()->GetViewByID(static_cast<int>(
                         DialogViewID::PAYMENT_SHEET_SUMMARY_SECTION)));
  EXPECT_NE(nullptr,
            dialog_view()->GetViewByID(static_cast<int>(
                DialogViewID::PAYMENT_SHEET_PAYMENT_METHOD_SECTION_BUTTON)));
  EXPECT_EQ(nullptr,
            dialog_view()->GetViewByID(static_cast<int>(
                DialogViewID::PAYMENT_SHEET_SHIPPING_ADDRESS_SECTION)));
  EXPECT_EQ(nullptr, dialog_view()->GetViewByID(static_cast<int>(
                         DialogViewID::PAYMENT_SHEET_SHIPPING_OPTION_SECTION)));
  EXPECT_EQ(nullptr, dialog_view()->GetViewByID(static_cast<int>(
                         DialogViewID::PAYMENT_SHEET_CONTACT_INFO_SECTION)));
}

// Accepts 'visa' cards and requests the full contact details.
class PaymentSheetViewControllerContactDetailsTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentSheetViewControllerContactDetailsTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PaymentSheetViewControllerContactDetailsTest);
};

// With no data present, the pay button should be disabled.
IN_PROC_BROWSER_TEST_F(PaymentSheetViewControllerContactDetailsTest, NoData) {
  NavigateTo("/payment_request_contact_details_and_free_shipping_test.html");
  InvokePaymentRequestUI();

  EXPECT_FALSE(IsPayButtonEnabled());
}

// With a supported card (Visa) present, the pay button is still disabled
// because there is no contact details.
IN_PROC_BROWSER_TEST_F(PaymentSheetViewControllerContactDetailsTest,
                       SupportedCard_NoContactInfo) {
  NavigateTo("/payment_request_contact_details_and_free_shipping_test.html");
  AddCreditCard(autofill::test::GetCreditCard());  // Visa card.

  InvokePaymentRequestUI();
  EXPECT_FALSE(IsPayButtonEnabled());
}

// With a supported card (Visa) present and a complete address profile, there is
// enough information to enable the pay button.
IN_PROC_BROWSER_TEST_F(PaymentSheetViewControllerContactDetailsTest,
                       SupportedCard_CompleteContactInfo) {
  NavigateTo("/payment_request_contact_details_and_free_shipping_test.html");
  autofill::AutofillProfile profile(autofill::test::GetFullProfile());
  AddAutofillProfile(profile);
  autofill::CreditCard card(autofill::test::GetCreditCard());  // Visa card.
  card.set_billing_address_id(profile.guid());
  AddCreditCard(card);

  InvokePaymentRequestUI();
  EXPECT_TRUE(IsPayButtonEnabled());
}

// With only an unsupported card present and a complete address profile, the pay
// button is disabled.
IN_PROC_BROWSER_TEST_F(PaymentSheetViewControllerContactDetailsTest,
                       UnsupportedCard_CompleteContactInfo) {
  NavigateTo("/payment_request_contact_details_and_free_shipping_test.html");
  AddCreditCard(autofill::test::GetCreditCard2());  // Amex card.
  AddAutofillProfile(autofill::test::GetFullProfile());

  InvokePaymentRequestUI();
  EXPECT_FALSE(IsPayButtonEnabled());
}

// With a supported card (Visa) present and a *incomplete* address profile, the
// pay button is disabled.
IN_PROC_BROWSER_TEST_F(PaymentSheetViewControllerContactDetailsTest,
                       SupportedCard_IncompleteContactInfo) {
  NavigateTo("/payment_request_contact_details_and_free_shipping_test.html");
  AddCreditCard(autofill::test::GetCreditCard());  // Visa card.

  autofill::AutofillProfile profile = autofill::test::GetFullProfile();
  // Remove the name from the profile to be stored.
  profile.SetRawInfo(autofill::NAME_FIRST, base::ASCIIToUTF16(""));
  profile.SetRawInfo(autofill::NAME_MIDDLE, base::ASCIIToUTF16(""));
  profile.SetRawInfo(autofill::NAME_LAST, base::ASCIIToUTF16(""));
  AddAutofillProfile(profile);

  InvokePaymentRequestUI();
  EXPECT_FALSE(IsPayButtonEnabled());
}

// If shipping and contact info are requested, show all the rows.
IN_PROC_BROWSER_TEST_F(PaymentSheetViewControllerContactDetailsTest,
                       AllRowsPresent) {
  NavigateTo("/payment_request_contact_details_and_free_shipping_test.html");
  InvokePaymentRequestUI();

  EXPECT_NE(nullptr, dialog_view()->GetViewByID(static_cast<int>(
                         DialogViewID::PAYMENT_SHEET_SUMMARY_SECTION)));
  // The buttons to select payment methods and shipping address are present.
  EXPECT_NE(nullptr,
            dialog_view()->GetViewByID(static_cast<int>(
                DialogViewID::PAYMENT_SHEET_PAYMENT_METHOD_SECTION_BUTTON)));
  EXPECT_NE(nullptr,
            dialog_view()->GetViewByID(static_cast<int>(
                DialogViewID::PAYMENT_SHEET_SHIPPING_ADDRESS_SECTION_BUTTON)));
  // Shipping option section (or its button) is not yet present.
  EXPECT_EQ(nullptr, dialog_view()->GetViewByID(static_cast<int>(
                         DialogViewID::PAYMENT_SHEET_SHIPPING_OPTION_SECTION)));
  EXPECT_EQ(nullptr,
            dialog_view()->GetViewByID(static_cast<int>(
                DialogViewID::PAYMENT_SHEET_SHIPPING_OPTION_SECTION_BUTTON)));
  // Contact details button is present.
  EXPECT_NE(nullptr,
            dialog_view()->GetViewByID(static_cast<int>(
                DialogViewID::PAYMENT_SHEET_CONTACT_INFO_SECTION_BUTTON)));
}

IN_PROC_BROWSER_TEST_F(PaymentSheetViewControllerContactDetailsTest,
                       AllClickableRowsPresent) {
  NavigateTo("/payment_request_contact_details_and_free_shipping_test.html");
  autofill::AutofillProfile profile(autofill::test::GetFullProfile());
  AddAutofillProfile(profile);
  autofill::CreditCard card(autofill::test::GetCreditCard());  // Visa card.
  card.set_billing_address_id(profile.guid());
  AddCreditCard(card);
  InvokePaymentRequestUI();

  EXPECT_NE(nullptr, dialog_view()->GetViewByID(static_cast<int>(
                         DialogViewID::PAYMENT_SHEET_SUMMARY_SECTION)));
  EXPECT_NE(nullptr, dialog_view()->GetViewByID(static_cast<int>(
                         DialogViewID::PAYMENT_SHEET_PAYMENT_METHOD_SECTION)));
  EXPECT_NE(nullptr,
            dialog_view()->GetViewByID(static_cast<int>(
                DialogViewID::PAYMENT_SHEET_SHIPPING_ADDRESS_SECTION)));
  EXPECT_NE(nullptr, dialog_view()->GetViewByID(static_cast<int>(
                         DialogViewID::PAYMENT_SHEET_SHIPPING_OPTION_SECTION)));
  EXPECT_NE(nullptr, dialog_view()->GetViewByID(static_cast<int>(
                         DialogViewID::PAYMENT_SHEET_CONTACT_INFO_SECTION)));
}

}  // namespace payments
