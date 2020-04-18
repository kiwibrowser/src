// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/payments/payment_request_browsertest_base.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/payments/core/journey_logger.h"
#include "components/ukm/test_ukm_recorder.h"
#include "components/ukm/ukm_source.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_test_utils.h"
#include "services/metrics/public/cpp/ukm_builders.h"

namespace payments {

class PaymentRequestJourneyLoggerSelectedPaymentInstrumentTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestJourneyLoggerSelectedPaymentInstrumentTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(
      PaymentRequestJourneyLoggerSelectedPaymentInstrumentTest);
};

// Tests that the selected instrument metric is correctly logged when the
// Payment Request is completed with a credit card.
IN_PROC_BROWSER_TEST_F(PaymentRequestJourneyLoggerSelectedPaymentInstrumentTest,
                       TestSelectedPaymentMethod) {
  NavigateTo("/payment_request_no_shipping_test.html");
  base::HistogramTester histogram_tester;

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

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_OTHER);
}

class PaymentRequestJourneyLoggerNoSupportedPaymentMethodTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestJourneyLoggerNoSupportedPaymentMethodTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(
      PaymentRequestJourneyLoggerNoSupportedPaymentMethodTest);
};

IN_PROC_BROWSER_TEST_F(PaymentRequestJourneyLoggerNoSupportedPaymentMethodTest,
                       OnlyBobpaySupported) {
  NavigateTo("/payment_request_bobpay_test.html");
  base::HistogramTester histogram_tester;

  ResetEventWaiter(DialogEvent::NOT_SUPPORTED_ERROR);
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_buy_button_js =
      "(function() { document.getElementById('buy').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_buy_button_js));
  WaitForObservedEvent();

  histogram_tester.ExpectBucketCount(
      "PaymentRequest.CheckoutFunnel.NoShow",
      JourneyLogger::NOT_SHOWN_REASON_NO_SUPPORTED_PAYMENT_METHOD, 1);

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_OTHER);
}

class PaymentRequestJourneyLoggerMultipleShowTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestJourneyLoggerMultipleShowTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PaymentRequestJourneyLoggerMultipleShowTest);
};

IN_PROC_BROWSER_TEST_F(PaymentRequestJourneyLoggerMultipleShowTest,
                       ShowSameRequest) {
  NavigateTo("/payment_request_multiple_show_test.html");
  base::HistogramTester histogram_tester;

  // Setup a credit card with an associated billing address.
  autofill::AutofillProfile billing_address = autofill::test::GetFullProfile();
  AddAutofillProfile(billing_address);
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(billing_address.guid());
  AddCreditCard(card);  // Visa.

  // Start a Payment Request.
  InvokePaymentRequestUI();

  // Try to show it again.
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_buy_button_js =
      "(function() { document.getElementById('showAgain').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_buy_button_js));

  // Complete the original Payment Request.
  PayWithCreditCardAndWait(base::ASCIIToUTF16("123"));

  // Trying to show the same request twice is not considered a concurrent
  // request.
  EXPECT_TRUE(
      histogram_tester.GetAllSamples("PaymentRequest.CheckoutFunnel.NoShow")
          .empty());

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_OTHER);
}

IN_PROC_BROWSER_TEST_F(PaymentRequestJourneyLoggerMultipleShowTest,
                       StartNewRequest) {
  NavigateTo("/payment_request_multiple_show_test.html");
  // Enable payment handlers, also known as service worker payment apps. The
  // rest of the test is identical to "StartNewRequestWithoutPaymentAppsFeature"
  // testcase above.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(features::kServiceWorkerPaymentApps);

  base::HistogramTester histogram_tester;

  // Setup a credit card with an associated billing address.
  autofill::AutofillProfile billing_address = autofill::test::GetFullProfile();
  AddAutofillProfile(billing_address);
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(billing_address.guid());
  AddCreditCard(card);  // Visa.

  // Start a Payment Request.
  InvokePaymentRequestUI();

  // Get the dialog view of the first request, since the one cached for the
  // tests will be replaced by the second Payment Request.
  PaymentRequestDialogView* first_dialog_view = dialog_view();

  // Try to show a second request.
  ResetEventWaiter(DialogEvent::DIALOG_CLOSED);
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_buy_button_js =
      "(function() { document.getElementById('showSecondRequest').click(); "
      "})();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_buy_button_js));
  WaitForObservedEvent();

  // Complete the original Payment Request.
  PayWithCreditCardAndWait(base::ASCIIToUTF16("123"), first_dialog_view);

  // There is one no show and one shown (verified below).
  histogram_tester.ExpectBucketCount(
      "PaymentRequest.CheckoutFunnel.NoShow",
      JourneyLogger::NOT_SHOWN_REASON_CONCURRENT_REQUESTS, 1);

  // Make sure the correct events were logged.
  // The first set of events is for the second (never shown) request.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(2U, buckets.size());
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);

  // When the payment apps feature is enabled, the checkout funnel is aborted
  // before checking for initial form of payment and the necessary complete
  // suggestions, because querying for available payment apps is asynchronous.
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);

  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_OTHER);
  // This is for the first (completed) request.
  EXPECT_TRUE(buckets[1].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_TRUE(buckets[1].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_TRUE(buckets[1].min &
              JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_TRUE(buckets[1].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_TRUE(buckets[1].min &
              JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_TRUE(buckets[1].min &
              JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[1].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_TRUE(buckets[1].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_TRUE(buckets[1].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_SELECTED_OTHER);
}

IN_PROC_BROWSER_TEST_F(PaymentRequestJourneyLoggerMultipleShowTest,
                       StartNewRequestWithoutPaymentAppsFeature) {
  NavigateTo("/payment_request_multiple_show_test.html");
  // Disable payment handlers, also known as service worker payment apps. The
  // rest of the test is identical to "StartNewRequest" testcase above.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      features::kServiceWorkerPaymentApps);

  base::HistogramTester histogram_tester;

  // Setup a credit card with an associated billing address.
  autofill::AutofillProfile billing_address = autofill::test::GetFullProfile();
  AddAutofillProfile(billing_address);
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(billing_address.guid());
  AddCreditCard(card);  // Visa.

  // Start a Payment Request.
  InvokePaymentRequestUI();

  // Get the dialog view of the first request, since the one cached for the
  // tests will be replaced by the second Payment Request.
  PaymentRequestDialogView* first_dialog_view = dialog_view();

  // Try to show a second request.
  ResetEventWaiter(DialogEvent::DIALOG_CLOSED);
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_buy_button_js =
      "(function() { document.getElementById('showSecondRequest').click(); "
      "})();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_buy_button_js));
  WaitForObservedEvent();

  // Complete the original Payment Request.
  PayWithCreditCardAndWait(base::ASCIIToUTF16("123"), first_dialog_view);

  // There is one no show and one shown (verified below).
  histogram_tester.ExpectBucketCount(
      "PaymentRequest.CheckoutFunnel.NoShow",
      JourneyLogger::NOT_SHOWN_REASON_CONCURRENT_REQUESTS, 1);

  // Make sure the correct events were logged.
  // The first set of events is for the second (never shown) request.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(2U, buckets.size());
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);

  // When the payment apps feature is disabled, the checkout funnel is aborted
  // after checking for initial form of payment and the necessary complete
  // suggestions, because querying for available autofill cards is synchronous.
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);

  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_OTHER);
  // This is for the first (completed) request.
  EXPECT_TRUE(buckets[1].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_TRUE(buckets[1].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_TRUE(buckets[1].min &
              JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_TRUE(buckets[1].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_TRUE(buckets[1].min &
              JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_TRUE(buckets[1].min &
              JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[1].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_TRUE(buckets[1].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_TRUE(buckets[1].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[1].min & JourneyLogger::EVENT_SELECTED_OTHER);
}

class PaymentRequestJourneyLoggerAllSectionStatsTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestJourneyLoggerAllSectionStatsTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PaymentRequestJourneyLoggerAllSectionStatsTest);
};

// Tests that the correct number of suggestions shown for each section is logged
// when a Payment Request is completed.
IN_PROC_BROWSER_TEST_F(PaymentRequestJourneyLoggerAllSectionStatsTest,
                       NumberOfSuggestionsShown_Completed) {
  NavigateTo("/payment_request_contact_details_and_free_shipping_test.html");
  base::HistogramTester histogram_tester;

  // Setup a credit card with an associated billing address.
  autofill::AutofillProfile billing_address = autofill::test::GetFullProfile();
  AddAutofillProfile(billing_address);
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(billing_address.guid());
  AddCreditCard(card);  // Visa.

  // Add another address.
  AddAutofillProfile(autofill::test::GetFullProfile2());

  // Complete the Payment Request.
  InvokePaymentRequestUI();
  ResetEventWaiter(DialogEvent::DIALOG_CLOSED);
  PayWithCreditCardAndWait(base::ASCIIToUTF16("123"));

  // Expect the appropriate number of suggestions shown to be logged.
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.NumberOfSuggestionsShown.PaymentMethod.Completed", 1, 1);
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.NumberOfSuggestionsShown.ShippingAddress.Completed", 2,
      1);
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.NumberOfSuggestionsShown.ContactInfo.Completed", 2, 1);

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_OTHER);
}

// Tests that the correct number of suggestions shown for each section is logged
// when a Payment Request is aborted by the user.
IN_PROC_BROWSER_TEST_F(PaymentRequestJourneyLoggerAllSectionStatsTest,
                       NumberOfSuggestionsShown_UserAborted) {
  NavigateTo("/payment_request_contact_details_and_free_shipping_test.html");
  base::HistogramTester histogram_tester;

  // Setup a credit card with an associated billing address.
  autofill::AutofillProfile billing_address = autofill::test::GetFullProfile();
  AddAutofillProfile(billing_address);
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(billing_address.guid());
  AddCreditCard(card);  // Visa.

  // Add another address.
  AddAutofillProfile(autofill::test::GetFullProfile2());

  // The user aborts the Payment Request.
  InvokePaymentRequestUI();
  ClickOnCancel();

  // Expect the appropriate number of suggestions shown to be logged.
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.NumberOfSuggestionsShown.PaymentMethod.UserAborted", 1,
      1);
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.NumberOfSuggestionsShown.ShippingAddress.UserAborted", 2,
      1);
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.NumberOfSuggestionsShown.ContactInfo.UserAborted", 2, 1);

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_OTHER);
}

class PaymentRequestJourneyLoggerNoShippingSectionStatsTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestJourneyLoggerNoShippingSectionStatsTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(
      PaymentRequestJourneyLoggerNoShippingSectionStatsTest);
};

// Tests that the correct number of suggestions shown for each section is logged
// when a Payment Request is completed.
IN_PROC_BROWSER_TEST_F(PaymentRequestJourneyLoggerNoShippingSectionStatsTest,
                       NumberOfSuggestionsShown_Completed) {
  NavigateTo("/payment_request_contact_details_test.html");
  base::HistogramTester histogram_tester;

  // Setup a credit card with an associated billing address.
  autofill::AutofillProfile billing_address = autofill::test::GetFullProfile();
  AddAutofillProfile(billing_address);
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(billing_address.guid());
  AddCreditCard(card);  // Visa.

  // Add another address.
  AddAutofillProfile(autofill::test::GetFullProfile2());

  // Complete the Payment Request.
  InvokePaymentRequestUI();
  ResetEventWaiter(DialogEvent::DIALOG_CLOSED);
  PayWithCreditCardAndWait(base::ASCIIToUTF16("123"));

  // Expect the appropriate number of suggestions shown to be logged.
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.NumberOfSuggestionsShown.PaymentMethod.Completed", 1, 1);
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.NumberOfSuggestionsShown.ContactInfo.Completed", 2, 1);

  // There should be no log for shipping address since it was not requested.
  histogram_tester.ExpectTotalCount(
      "PaymentRequest.NumberOfSuggestionsShown.ShippingAddress.Completed", 0);

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_OTHER);
}

// Tests that the correct number of suggestions shown for each section is logged
// when a Payment Request is aborted by the user.
IN_PROC_BROWSER_TEST_F(PaymentRequestJourneyLoggerNoShippingSectionStatsTest,
                       NumberOfSuggestionsShown_UserAborted) {
  NavigateTo("/payment_request_contact_details_test.html");
  base::HistogramTester histogram_tester;

  // Setup a credit card with an associated billing address.
  autofill::AutofillProfile billing_address = autofill::test::GetFullProfile();
  AddAutofillProfile(billing_address);
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(billing_address.guid());
  AddCreditCard(card);  // Visa.

  // Add another address.
  AddAutofillProfile(autofill::test::GetFullProfile2());

  // The user aborts the Payment Request.
  InvokePaymentRequestUI();
  ClickOnCancel();

  // Expect the appropriate number of suggestions shown to be logged.
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.NumberOfSuggestionsShown.PaymentMethod.UserAborted", 1,
      1);
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.NumberOfSuggestionsShown.ContactInfo.UserAborted", 2, 1);

  // There should be no log for shipping address since it was not requested.
  histogram_tester.ExpectTotalCount(
      "PaymentRequest.NumberOfSuggestionsShown.ShippingAddress.UserAborted", 0);

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_OTHER);
}

class PaymentRequestJourneyLoggerNoContactDetailSectionStatsTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestJourneyLoggerNoContactDetailSectionStatsTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(
      PaymentRequestJourneyLoggerNoContactDetailSectionStatsTest);
};

// Tests that the correct number of suggestions shown for each section is logged
// when a Payment Request is completed.
IN_PROC_BROWSER_TEST_F(
    PaymentRequestJourneyLoggerNoContactDetailSectionStatsTest,
    NumberOfSuggestionsShown_Completed) {
  NavigateTo("/payment_request_free_shipping_test.html");
  base::HistogramTester histogram_tester;

  // Setup a credit card with an associated billing address.
  autofill::AutofillProfile billing_address = autofill::test::GetFullProfile();
  AddAutofillProfile(billing_address);
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(billing_address.guid());
  AddCreditCard(card);  // Visa.

  // Add another address.
  AddAutofillProfile(autofill::test::GetFullProfile2());

  // Complete the Payment Request.
  InvokePaymentRequestUI();
  ResetEventWaiter(DialogEvent::DIALOG_CLOSED);
  PayWithCreditCardAndWait(base::ASCIIToUTF16("123"));

  // Expect the appropriate number of suggestions shown to be logged.
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.NumberOfSuggestionsShown.PaymentMethod.Completed", 1, 1);
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.NumberOfSuggestionsShown.ShippingAddress.Completed", 2,
      1);

  // There should be no log for contact info since it was not requested.
  histogram_tester.ExpectTotalCount(
      "PaymentRequest.NumberOfSuggestionsShown.ContactInfo.Completed", 0);

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_OTHER);
}

// Tests that the correct number of suggestions shown for each section is logged
// when a Payment Request is aborted by the user.
IN_PROC_BROWSER_TEST_F(
    PaymentRequestJourneyLoggerNoContactDetailSectionStatsTest,
    NumberOfSuggestionsShown_UserAborted) {
  NavigateTo("/payment_request_free_shipping_test.html");
  base::HistogramTester histogram_tester;

  // Setup a credit card with an associated billing address.
  autofill::AutofillProfile billing_address = autofill::test::GetFullProfile();
  AddAutofillProfile(billing_address);
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(billing_address.guid());
  AddCreditCard(card);  // Visa.

  // Add another address.
  AddAutofillProfile(autofill::test::GetFullProfile2());

  // The user aborts the Payment Request.
  InvokePaymentRequestUI();
  ClickOnCancel();

  // Expect the appropriate number of suggestions shown to be logged.
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.NumberOfSuggestionsShown.PaymentMethod.UserAborted", 1,
      1);
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.NumberOfSuggestionsShown.ShippingAddress.UserAborted", 2,
      1);

  // There should be no log for contact info since it was not requested.
  histogram_tester.ExpectTotalCount(
      "PaymentRequest.NumberOfSuggestionsShown.ContactInfo.UserAborted", 0);

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_OTHER);
}

class PaymentRequestNotShownTest : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestNotShownTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PaymentRequestNotShownTest);
};

IN_PROC_BROWSER_TEST_F(PaymentRequestNotShownTest, OnlyNotShownMetricsLogged) {
  NavigateTo("/payment_request_can_make_payment_metrics_test.html");
  base::HistogramTester histogram_tester;

  ResetEventWaiterForSequence({DialogEvent::CAN_MAKE_PAYMENT_CALLED,
                               DialogEvent::CAN_MAKE_PAYMENT_RETURNED});

  // Initiate a Payment Request without showing it.
  ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(), "queryNoShow();"));

  WaitForObservedEvent();

  // Navigate away to abort the Payment Request and trigger the logs.
  NavigateTo("/payment_request_email_test.html");

  // Abort should not be logged.
  histogram_tester.ExpectTotalCount("PaymentRequest.CheckoutFunnel.Aborted", 0);

  // Some events should be logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  // Only USER_ABORTED and CAN_MAKE_PAYMENT_FALSE should be logged.
  EXPECT_EQ(JourneyLogger::EVENT_USER_ABORTED |
                JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE |
                JourneyLogger::EVENT_REQUEST_METHOD_OTHER |
                JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD,
            buckets[0].min);

  // Make sure that the metrics that required the Payment Request to be shown
  // are not logged.
  histogram_tester.ExpectTotalCount("PaymentRequest.SelectedPaymentMethod", 0);
  histogram_tester.ExpectTotalCount("PaymentRequest.NumberOfSelectionAdds", 0);
  histogram_tester.ExpectTotalCount("PaymentRequest.NumberOfSelectionChanges",
                                    0);
  histogram_tester.ExpectTotalCount("PaymentRequest.NumberOfSelectionEdits", 0);
  histogram_tester.ExpectTotalCount("PaymentRequest.NumberOfSuggestionsShown",
                                    0);
}

class PaymentRequestCompleteSuggestionsForEverythingTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestCompleteSuggestionsForEverythingTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PaymentRequestCompleteSuggestionsForEverythingTest);
};

IN_PROC_BROWSER_TEST_F(PaymentRequestCompleteSuggestionsForEverythingTest,
                       UserHadCompleteSuggestionsForEverything) {
  NavigateTo("/payment_request_email_test.html");
  base::HistogramTester histogram_tester;

  // Add an address and a credit card on file.
  autofill::AutofillProfile billing_address = autofill::test::GetFullProfile();
  AddAutofillProfile(billing_address);
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(billing_address.guid());
  AddCreditCard(card);  // Visa.

  // Show a Payment Request.
  InvokePaymentRequestUI();

  // Navigate away to abort the Payment Request and trigger the logs.
  NavigateTo("/payment_request_email_test.html");

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_OTHER);
}

IN_PROC_BROWSER_TEST_F(PaymentRequestCompleteSuggestionsForEverythingTest,
                       UserDidNotHaveCompleteSuggestionsForEverything_NoCard) {
  NavigateTo("/payment_request_email_test.html");
  base::HistogramTester histogram_tester;

  // Add an address.
  AddAutofillProfile(autofill::test::GetFullProfile());

  // Show a Payment Request. The user has no form of payment on file.
  InvokePaymentRequestUI();

  // Navigate away to abort the Payment Request and trigger the logs.
  NavigateTo("/payment_request_email_test.html");

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_OTHER);
}

IN_PROC_BROWSER_TEST_F(
    PaymentRequestCompleteSuggestionsForEverythingTest,
    UserDidNotHaveCompleteSuggestionsForEverything_CardNetworkNotSupported) {
  NavigateTo("/payment_request_email_test.html");
  base::HistogramTester histogram_tester;

  // Add an address and an AMEX credit card on file. AMEX is not supported by
  // the merchant.
  autofill::AutofillProfile billing_address = autofill::test::GetFullProfile();
  AddAutofillProfile(billing_address);
  autofill::CreditCard card = autofill::test::GetCreditCard2();
  card.set_billing_address_id(billing_address.guid());
  AddCreditCard(card);  // AMEX.

  // Show a Payment Request.
  InvokePaymentRequestUI();

  // Navigate away to abort the Payment Request and trigger the logs.
  NavigateTo("/payment_request_email_test.html");

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_OTHER);
}

class PaymentRequestIframeTest : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestIframeTest() {}

  void PreRunTestOnMainThread() override {
    InProcessBrowserTest::PreRunTestOnMainThread();

    test_ukm_recorder_ = std::make_unique<ukm::TestAutoSetUkmRecorder>();
  }

  std::unique_ptr<ukm::TestAutoSetUkmRecorder> test_ukm_recorder_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PaymentRequestIframeTest);
};

IN_PROC_BROWSER_TEST_F(PaymentRequestIframeTest, CrossOriginIframe) {
  base::HistogramTester histogram_tester;

  GURL main_frame_url =
      https_server()->GetURL("a.com", "/payment_request_main.html");
  ui_test_utils::NavigateToURL(browser(), main_frame_url);

  // The iframe calls show() immediately.
  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  GURL iframe_url =
      https_server()->GetURL("b.com", "/payment_request_iframe.html");
  ResetEventWaiter(DialogEvent::DIALOG_OPENED);
  EXPECT_TRUE(content::NavigateIframeToURL(tab, "test", iframe_url));
  WaitForObservedEvent();

  // Simulate that the user cancels the PR.
  ClickOnCancel();

  int64_t expected_step_metric =
      JourneyLogger::EVENT_SHOWN |
      JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD |
      JourneyLogger::EVENT_USER_ABORTED;

  // Make sure the correct UMA events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_EQ(expected_step_metric, buckets[0].min);

  // Important: Even though the Payment Request is in the iframe, no UKM was
  // logged for the iframe URL, only for the main frame.
  for (const auto& kv : test_ukm_recorder_->GetSources()) {
    EXPECT_NE(iframe_url, kv.second->url());
  }

  // Make sure the UKM was logged correctly.
  auto entries = test_ukm_recorder_->GetEntriesByName(
      ukm::builders::PaymentRequest_CheckoutEvents::kEntryName);
  EXPECT_EQ(1u, entries.size());
  for (const auto* const entry : entries) {
    test_ukm_recorder_->ExpectEntrySourceHasUrl(entry, main_frame_url);
    EXPECT_EQ(2U, entry->metrics.size());
    test_ukm_recorder_->ExpectEntryMetric(
        entry,
        ukm::builders::PaymentRequest_CheckoutEvents::kCompletionStatusName,
        JourneyLogger::COMPLETION_STATUS_USER_ABORTED);
    test_ukm_recorder_->ExpectEntryMetric(
        entry, ukm::builders::PaymentRequest_CheckoutEvents::kEventsName,
        expected_step_metric);
  }
}

IN_PROC_BROWSER_TEST_F(PaymentRequestIframeTest, IframeNavigation_UserAborted) {
  NavigateTo("/payment_request_free_shipping_with_iframe_test.html");
  base::HistogramTester histogram_tester;

  // Show a Payment Request.
  InvokePaymentRequestUI();

  // Make the iframe navigate away.
  EXPECT_TRUE(NavigateIframeToURL(GetActiveWebContents(), "theIframe",
                                  GURL("https://www.example.com")));

  // Simulate that the user cancels the PR.
  ClickOnCancel();

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_OTHER);
}

IN_PROC_BROWSER_TEST_F(PaymentRequestIframeTest, IframeNavigation_Completed) {
  NavigateTo("/payment_request_free_shipping_with_iframe_test.html");
  base::HistogramTester histogram_tester;

  // Setup a credit card with an associated billing address.
  autofill::AutofillProfile billing_address = autofill::test::GetFullProfile();
  AddAutofillProfile(billing_address);
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(billing_address.guid());
  AddCreditCard(card);  // Visa.

  // Add another address.
  AddAutofillProfile(autofill::test::GetFullProfile2());

  // Show a Payment Request.
  InvokePaymentRequestUI();

  // Make the iframe navigate away.
  EXPECT_TRUE(NavigateIframeToURL(GetActiveWebContents(), "theIframe",
                                  GURL("https://www.example.com")));

  // Complete the Payment Request.
  ResetEventWaiter(DialogEvent::DIALOG_CLOSED);
  PayWithCreditCardAndWait(base::ASCIIToUTF16("123"));

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_OTHER);
}

IN_PROC_BROWSER_TEST_F(PaymentRequestIframeTest, HistoryPushState_UserAborted) {
  NavigateTo("/payment_request_free_shipping_with_iframe_test.html");
  base::HistogramTester histogram_tester;

  // Show a Payment Request.
  InvokePaymentRequestUI();

  // PushState on the history.
  ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(),
                                     "window.history.pushState(\"\", \"\", "
                                     "\"/favicon/"
                                     "pushstate_with_favicon_pushed.html\");"));

  // Simulate that the user cancels the PR.
  ClickOnCancel();

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_OTHER);
}

IN_PROC_BROWSER_TEST_F(PaymentRequestIframeTest, HistoryPushState_Completed) {
  NavigateTo("/payment_request_free_shipping_with_iframe_test.html");
  base::HistogramTester histogram_tester;

  // Setup a credit card with an associated billing address.
  autofill::AutofillProfile billing_address = autofill::test::GetFullProfile();
  AddAutofillProfile(billing_address);
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(billing_address.guid());
  AddCreditCard(card);  // Visa.

  // Add another address.
  AddAutofillProfile(autofill::test::GetFullProfile2());

  // Show a Payment Request.
  InvokePaymentRequestUI();

  // PushState on the history.
  ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(),
                                     "window.history.pushState(\"\", \"\", "
                                     "\"/favicon/"
                                     "pushstate_with_favicon_pushed.html\");"));

  // Complete the Payment Request.
  ResetEventWaiter(DialogEvent::DIALOG_CLOSED);
  PayWithCreditCardAndWait(base::ASCIIToUTF16("123"));

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_METHOD_OTHER);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SELECTED_CREDIT_CARD);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_GOOGLE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SELECTED_OTHER);
}

}  // namespace payments
