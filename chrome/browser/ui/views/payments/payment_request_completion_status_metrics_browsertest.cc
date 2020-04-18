// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/views/payments/payment_request_browsertest_base.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/payments/core/journey_logger.h"
#include "content/public/test/browser_test_utils.h"
#include "url/gurl.h"

namespace payments {

class PaymentRequestCompletionStatusMetricsTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestCompletionStatusMetricsTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PaymentRequestCompletionStatusMetricsTest);
};

IN_PROC_BROWSER_TEST_F(PaymentRequestCompletionStatusMetricsTest, Completed) {
  NavigateTo("/payment_request_can_make_payment_metrics_test.html");
  base::HistogramTester histogram_tester;

  // Setup a credit card with an associated billing address so CanMakePayment
  // returns true.
  autofill::AutofillProfile billing_address = autofill::test::GetFullProfile();
  AddAutofillProfile(billing_address);
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(billing_address.guid());
  AddCreditCard(card);

  // Start the Payment Request and expect CanMakePayment to be called before the
  // Payment Request is shown.
  ResetEventWaiterForSequence({DialogEvent::CAN_MAKE_PAYMENT_CALLED,
                               DialogEvent::CAN_MAKE_PAYMENT_RETURNED,
                               DialogEvent::DIALOG_OPENED});
  ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(), "queryShow();"));
  WaitForObservedEvent();

  // Complete the Payment Request.
  PayWithCreditCardAndWait(base::ASCIIToUTF16("123"));

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_INITIAL_FORM_OF_PAYMENT);
  EXPECT_TRUE(buckets[0].min &
              JourneyLogger::EVENT_HAD_NECESSARY_COMPLETE_SUGGESTIONS);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_SHIPPING);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_NAME);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_PHONE);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_REQUEST_PAYER_EMAIL);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_FALSE);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_CAN_MAKE_PAYMENT_TRUE);
}

IN_PROC_BROWSER_TEST_F(PaymentRequestCompletionStatusMetricsTest,
                       MerchantAborted_Reload) {
  NavigateTo("/payment_request_can_make_payment_metrics_test.html");
  base::HistogramTester histogram_tester;

  // Start the Payment Request.
  ResetEventWaiter(DialogEvent::DIALOG_OPENED);
  ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(), "noQueryShow();"));
  WaitForObservedEvent();

  // The merchant reloads the page.
  ResetEventWaiter(DialogEvent::DIALOG_CLOSED);
  ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(),
                                     "(function() { location.reload(); })();"));
  WaitForObservedEvent();

  // Make sure the metrics are logged correctly.
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.CheckoutFunnel.Aborted",
      JourneyLogger::ABORT_REASON_MERCHANT_NAVIGATION, 1);

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
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
}

IN_PROC_BROWSER_TEST_F(PaymentRequestCompletionStatusMetricsTest,
                       MerchantAborted_Navigation) {
  NavigateTo("/payment_request_can_make_payment_metrics_test.html");
  base::HistogramTester histogram_tester;

  // Start the Payment Request.
  ResetEventWaiter(DialogEvent::DIALOG_OPENED);
  ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(), "noQueryShow();"));
  WaitForObservedEvent();

  // The merchant navigates away.
  ResetEventWaiter(DialogEvent::DIALOG_CLOSED);
  ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(),
                                     "(function() { window.location.href = "
                                     "'/payment_request_email_test.html'; "
                                     "})();"));
  WaitForObservedEvent();

  // Make sure the metrics are logged correctly.
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.CheckoutFunnel.Aborted",
      JourneyLogger::ABORT_REASON_MERCHANT_NAVIGATION, 1);

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
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
}

IN_PROC_BROWSER_TEST_F(PaymentRequestCompletionStatusMetricsTest,
                       MerchantAborted_Abort) {
  NavigateTo("/payment_request_can_make_payment_metrics_test.html");
  base::HistogramTester histogram_tester;

  // Start the Payment Request.
  ResetEventWaiter(DialogEvent::DIALOG_OPENED);
  ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(), "noQueryShow();"));
  WaitForObservedEvent();

  // The merchant aborts the Payment Request.
  ResetEventWaiterForSequence(
      {DialogEvent::ABORT_CALLED, DialogEvent::DIALOG_CLOSED});
  const std::string click_buy_button_js =
      "(function() { document.getElementById('abort').click(); })();";
  ASSERT_TRUE(
      content::ExecuteScript(GetActiveWebContents(), click_buy_button_js));
  WaitForObservedEvent();

  // Make sure the metrics are logged correctly.
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.CheckoutFunnel.Aborted",
      JourneyLogger::ABORT_REASON_ABORTED_BY_MERCHANT, 1);

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
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
}

IN_PROC_BROWSER_TEST_F(PaymentRequestCompletionStatusMetricsTest,
                       UserAborted_Navigation) {
  NavigateTo("/payment_request_can_make_payment_metrics_test.html");
  base::HistogramTester histogram_tester;

  // Start the Payment Request.
  ResetEventWaiter(DialogEvent::DIALOG_OPENED);
  ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(), "noQueryShow();"));
  WaitForObservedEvent();

  // Navigate away.
  NavigateTo("/payment_request_email_test.html");

  // Make sure the metrics are logged correctly.
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.CheckoutFunnel.Aborted",
      JourneyLogger::ABORT_REASON_USER_NAVIGATION, 1);

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
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
}

IN_PROC_BROWSER_TEST_F(PaymentRequestCompletionStatusMetricsTest,
                       UserAborted_CancelButton) {
  NavigateTo("/payment_request_can_make_payment_metrics_test.html");
  base::HistogramTester histogram_tester;

  // Start the Payment Request.
  ResetEventWaiter(DialogEvent::DIALOG_OPENED);
  ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(), "noQueryShow();"));
  WaitForObservedEvent();

  // Click on the cancel button.
  ClickOnCancel();

  // Make sure the metrics are logged correctly.
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.CheckoutFunnel.Aborted",
      JourneyLogger::ABORT_REASON_ABORTED_BY_USER, 1);

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
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
}

IN_PROC_BROWSER_TEST_F(PaymentRequestCompletionStatusMetricsTest,
                       UserAborted_TabClosed) {
  NavigateTo("/payment_request_can_make_payment_metrics_test.html");
  base::HistogramTester histogram_tester;

  // Start the Payment Request.
  ResetEventWaiter(DialogEvent::DIALOG_OPENED);
  ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(), "noQueryShow();"));
  WaitForObservedEvent();

  // Close the tab containing the Payment Request.
  ResetEventWaiterForSequence({DialogEvent::DIALOG_CLOSED});
  chrome::CloseTab(browser());
  WaitForObservedEvent();

  // Make sure the metrics are logged correctly.
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.CheckoutFunnel.Aborted",
      JourneyLogger::ABORT_REASON_ABORTED_BY_USER, 1);

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
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
}

IN_PROC_BROWSER_TEST_F(PaymentRequestCompletionStatusMetricsTest,
                       UserAborted_Reload) {
  NavigateTo("/payment_request_can_make_payment_metrics_test.html");
  base::HistogramTester histogram_tester;

  // Start the Payment Request.
  ResetEventWaiter(DialogEvent::DIALOG_OPENED);
  ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(), "noQueryShow();"));
  WaitForObservedEvent();

  // Reload the page containing the Payment Request.
  ResetEventWaiterForSequence({DialogEvent::DIALOG_CLOSED});
  chrome::Reload(browser(), WindowOpenDisposition::CURRENT_TAB);
  WaitForObservedEvent();

  // Make sure the metrics are logged correctly.
  histogram_tester.ExpectUniqueSample(
      "PaymentRequest.CheckoutFunnel.Aborted",
      JourneyLogger::ABORT_REASON_USER_NAVIGATION, 1);

  // Make sure the correct events were logged.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_SHOWN);
  EXPECT_TRUE(buckets[0].min & JourneyLogger::EVENT_USER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_OTHER_ABORTED);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_PAY_CLICKED);
  EXPECT_FALSE(buckets[0].min &
               JourneyLogger::EVENT_RECEIVED_INSTRUMENT_DETAILS);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_SKIPPED_SHOW);
  EXPECT_FALSE(buckets[0].min & JourneyLogger::EVENT_COMPLETED);
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
}

class PaymentRequestInitiatedCompletionStatusMetricsTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestInitiatedCompletionStatusMetricsTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PaymentRequestInitiatedCompletionStatusMetricsTest);
};

IN_PROC_BROWSER_TEST_F(PaymentRequestInitiatedCompletionStatusMetricsTest,
                       Aborted_NotShown) {
  base::HistogramTester histogram_tester;
  NavigateTo("/initiated_test.html");

  // Navigate away.
  NavigateTo("/payment_request_email_test.html");

  // No abort reason should be logged.
  histogram_tester.ExpectTotalCount("PaymentRequest.CheckoutFunnel.Aborted", 0);

  // There is one sample, because the request was initiated.
  std::vector<base::Bucket> buckets =
      histogram_tester.GetAllSamples("PaymentRequest.Events");
  ASSERT_EQ(1U, buckets.size());
  EXPECT_EQ(JourneyLogger::EVENT_INITIATED | JourneyLogger::EVENT_USER_ABORTED |
                JourneyLogger::EVENT_REQUEST_METHOD_BASIC_CARD |
                JourneyLogger::EVENT_REQUEST_METHOD_OTHER,
            buckets[0].min);
}

}  // namespace payments
